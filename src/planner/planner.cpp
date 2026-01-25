#include "planner.h"
#include "../parser/statements/select.h"
#include "../parser/statements/insert.h"
#include "../parser/statements/update.h"
#include "../parser/statements/delete.h"
#include <memory>
#include <stdexcept>

// Returns true if the plan uses cursor-based scans that can be invalidated
static bool needs_collection(const Plan* plan) {
    if (!plan) return false;
    
    switch (plan->type) {
        case PlanType::SeqScan:
            return true;  // SeqScan uses cursors
        case PlanType::IndexScan:
            return true;  // IndexScan uses cursors (future)
        case PlanType::Filter: {
            // Recursively check the source
            const FilterPlan* filter = static_cast<const FilterPlan*>(plan);
            return needs_collection(filter->source.get());
        }
        case PlanType::Project: {
            // Recursively check the source
            const ProjectPlan* project = static_cast<const ProjectPlan*>(plan);
            return needs_collection(project->source.get());
        }
        case PlanType::Values:
            return false;  // Values doesn't use cursors
        case PlanType::Collect:
            return false;  // Already collected
        default:
            return false;  // Conservative: assume no cursor for unknown types
    }
}

// Build plan for SELECT statement
static std::unique_ptr<Plan> build_select_plan(const SelectStmt& select_stmt) {
    // Step 1: Create base sequential scan
    std::unique_ptr<Plan> plan = std::make_unique<SeqScanPlan>(select_stmt.table);
    
    // Step 2: Add WHERE filter if present
    if (select_stmt.where != nullptr) {
        plan = std::make_unique<FilterPlan>(select_stmt.where, std::move(plan));
    }
    
    // Step 3: Add ORDER BY sort if present
    // Sort requires all input, so insert Collect before it
    if (!select_stmt.order_by.empty()) {
        // Insert Collect to materialize input before sorting
        plan = std::make_unique<CollectPlan>(std::move(plan));
        plan = std::make_unique<SortPlan>(select_stmt.order_by, std::move(plan));
    }
    
    // Step 4: Add projection for SELECT columns (top level)
    if (!select_stmt.columns.empty()) {
        plan = std::make_unique<ProjectPlan>(select_stmt.columns, std::move(plan));
    }
    
    return plan;
}

// Build plan for INSERT statement
static std::unique_ptr<Plan> build_insert_plan(const InsertStmt& insert_stmt) {
    // Step 1: Create Values plan with the values expressions
    auto values_plan = std::make_unique<ValuesPlan>(insert_stmt.values);
    
    // Step 2: Create Insert plan with table, columns, and values as source
    auto insert_plan = std::make_unique<InsertPlan>(
        insert_stmt.table,
        insert_stmt.columns,
        std::move(values_plan)
    );
    
    return insert_plan;
}

// Build plan for UPDATE statement
static std::unique_ptr<Plan> build_update_plan(const UpdateStmt& update_stmt) {
    // Step 1: Create base sequential scan
    std::unique_ptr<Plan> plan = std::make_unique<SeqScanPlan>(update_stmt.table);
    
    // Step 2: Add WHERE filter if present
    if (update_stmt.where != nullptr) {
        plan = std::make_unique<FilterPlan>(update_stmt.where, std::move(plan));
    }
    
    // Step 3: Insert Collect if source uses cursor-based scans
    // This ensures cursor safety: mutations won't invalidate the scan cursor
    if (needs_collection(plan.get())) {
        plan = std::make_unique<CollectPlan>(std::move(plan));
    }
    
    // Step 4: Create Update plan with table, assignments, and collected scan as source
    plan = std::make_unique<UpdatePlan>(
        update_stmt.table,
        update_stmt.assignments,
        std::move(plan)
    );
    
    return plan;
}

// Build plan for DELETE statement
static std::unique_ptr<Plan> build_delete_plan(const DeleteStmt& delete_stmt) {
    // Step 1: Create base sequential scan
    std::unique_ptr<Plan> plan = std::make_unique<SeqScanPlan>(delete_stmt.table);
    
    // Step 2: Add WHERE filter if present
    if (delete_stmt.where != nullptr) {
        plan = std::make_unique<FilterPlan>(delete_stmt.where, std::move(plan));
    }
    
    // Step 3: Insert Collect if source uses cursor-based scans
    // This ensures cursor safety: mutations won't invalidate the scan cursor
    if (needs_collection(plan.get())) {
        plan = std::make_unique<CollectPlan>(std::move(plan));
    }
    
    // Step 4: Create Delete plan with table and collected scan/filter as source
    plan = std::make_unique<DeletePlan>(
        delete_stmt.table,
        std::move(plan)
    );
    
    return plan;
}

// Main build_plan function that dispatches based on statement type
std::unique_ptr<Plan> build_plan(const Statement& stmt) {
    switch (stmt.get_type()) {
        case StatementType::Select:
            return build_select_plan(stmt.as_select());
            
        case StatementType::Insert:
            return build_insert_plan(stmt.as_insert());
            
        case StatementType::Update:
            return build_update_plan(stmt.as_update());
            
        case StatementType::Delete:
            return build_delete_plan(stmt.as_delete());
            
        case StatementType::Create:
            // CREATE statements don't need execution plans (they're DDL)
            // For now, we'll throw an error or return nullptr
            throw std::runtime_error("CREATE statements do not require execution plans");
            
        default:
            throw std::runtime_error("Unsupported statement type for planning");
    }
}
