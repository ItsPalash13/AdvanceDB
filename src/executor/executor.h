#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "types.h"
#include <optional>
#include <vector>
#include <map>
#include <string>

// Forward declarations
struct Plan;
class Storage;

// Base Executor interface
// Each executor implements the iterator model with a next() method
class Executor {
public:
    virtual ~Executor() = default;
    virtual std::optional<Tuple> next() = 0;
};

// Main execution entry point
// Takes a plan, storage, and schema, then executes and returns all result tuples
std::vector<Tuple> execute_plan(Plan* plan, Storage& storage, 
                                 const std::map<std::string, std::vector<std::string>>& schema);

#endif // EXECUTOR_H
