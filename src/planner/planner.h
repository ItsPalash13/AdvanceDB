#ifndef PLANNER_H
#define PLANNER_H

#include "plan.h"
#include "../parser/statements/statement.h"
#include <memory>

// Main function to build a plan from a parsed statement
std::unique_ptr<Plan> build_plan(const Statement& stmt);

#endif // PLANNER_H
