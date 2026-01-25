#include "executor.h"
#include "executor_factory.h"
#include "storage.h"
#include "../planner/plan.h"
#include <vector>

std::vector<Tuple> execute_plan(Plan* plan, Storage& storage, 
                                 const std::map<std::string, std::vector<std::string>>& schema) {
    // Build executor tree from plan tree
    std::unique_ptr<Executor> root_executor = build_executor(plan, storage, schema);
    
    // Pull all tuples from root executor
    std::vector<Tuple> results;
    while (true) {
        std::optional<Tuple> tuple = root_executor->next();
        if (!tuple.has_value()) {
            break;  // No more tuples
        }
        results.push_back(tuple.value());
    }
    
    return results;
}
