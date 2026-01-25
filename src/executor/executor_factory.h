#ifndef EXECUTOR_FACTORY_H
#define EXECUTOR_FACTORY_H

#include "executor.h"
#include "storage.h"
#include <memory>
#include <map>
#include <vector>
#include <string>

// Forward declarations
struct Plan;

// Build executor tree from plan tree
// Recursively creates executors for each plan node
std::unique_ptr<Executor> build_executor(Plan* plan, Storage& storage, 
                                         const std::map<std::string, std::vector<std::string>>& schema);

#endif // EXECUTOR_FACTORY_H
