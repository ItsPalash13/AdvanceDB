#include "executor_factory.h"
#include "executors.h"
#include "../planner/plan.h"
#include <stdexcept>

std::unique_ptr<Executor> build_executor(Plan* plan, Storage& storage, 
                                         const std::map<std::string, std::vector<std::string>>& schema) {
    if (!plan) {
        throw std::runtime_error("Null plan");
    }

    switch (plan->type) {
        case PlanType::SeqScan: {
            SeqScanPlan* scan_plan = static_cast<SeqScanPlan*>(plan);
            return std::make_unique<SeqScanExecutor>(storage, scan_plan->table);
        }

        case PlanType::Filter: {
            FilterPlan* filter_plan = static_cast<FilterPlan*>(plan);
            std::unique_ptr<Executor> child = build_executor(filter_plan->source.get(), storage, schema);
            
            // Get column names for the table (needed for expression evaluation)
            // We need to find the base table from the child plan
            std::vector<std::string> column_names;
            Plan* child_plan = filter_plan->source.get();
            
            // Traverse down to find the base table
            while (child_plan) {
                if (child_plan->type == PlanType::SeqScan) {
                    SeqScanPlan* scan = static_cast<SeqScanPlan*>(child_plan);
                    auto it = schema.find(scan->table);
                    if (it != schema.end()) {
                        column_names = it->second;
                    }
                    break;
                } else if (child_plan->type == PlanType::Filter) {
                    FilterPlan* filter = static_cast<FilterPlan*>(child_plan);
                    child_plan = filter->source.get();
                } else if (child_plan->type == PlanType::Project) {
                    ProjectPlan* project = static_cast<ProjectPlan*>(child_plan);
                    child_plan = project->source.get();
                } else {
                    break;
                }
            }
            
            return std::make_unique<FilterExecutor>(std::move(child), filter_plan->predicate, column_names);
        }

        case PlanType::Project: {
            ProjectPlan* project_plan = static_cast<ProjectPlan*>(plan);
            std::unique_ptr<Executor> child = build_executor(project_plan->source.get(), storage, schema);
            
            // Get column names for the input (needed for expression evaluation)
            std::vector<std::string> column_names;
            Plan* child_plan = project_plan->source.get();
            
            // Traverse down to find the base table
            while (child_plan) {
                if (child_plan->type == PlanType::SeqScan) {
                    SeqScanPlan* scan = static_cast<SeqScanPlan*>(child_plan);
                    auto it = schema.find(scan->table);
                    if (it != schema.end()) {
                        column_names = it->second;
                    }
                    break;
                } else if (child_plan->type == PlanType::Filter) {
                    FilterPlan* filter = static_cast<FilterPlan*>(child_plan);
                    child_plan = filter->source.get();
                } else if (child_plan->type == PlanType::Project) {
                    ProjectPlan* project = static_cast<ProjectPlan*>(child_plan);
                    child_plan = project->source.get();
                } else {
                    break;
                }
            }
            
            return std::make_unique<ProjectExecutor>(std::move(child), project_plan->projections, column_names);
        }

        default:
            throw std::runtime_error("Unsupported plan type in executor factory");
    }
}
