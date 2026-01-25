#ifndef EXECUTORS_H
#define EXECUTORS_H

#include "executor.h"
#include "storage.h"
#include "types.h"
#include <memory>
#include <vector>
#include <string>

// Forward declarations
struct Expr;

// Sequential scan executor
// Reads tuples sequentially from storage using a cursor
class SeqScanExecutor : public Executor {
private:
    Storage& storage;
    std::string table_name;
    size_t cursor;
    Table* table;

public:
    SeqScanExecutor(Storage& s, const std::string& table);
    std::optional<Tuple> next() override;
};

// Filter executor
// Applies predicate to tuples from its child
class FilterExecutor : public Executor {
private:
    std::unique_ptr<Executor> child;
    Expr* predicate;
    std::vector<std::string> column_names;  // For expression evaluation

public:
    FilterExecutor(std::unique_ptr<Executor> child, Expr* pred, 
                   const std::vector<std::string>& cols);
    std::optional<Tuple> next() override;
};

// Project executor
// Transforms input tuples into output tuples via projections
class ProjectExecutor : public Executor {
private:
    std::unique_ptr<Executor> child;
    std::vector<Expr*> projections;
    std::vector<std::string> column_names;  // Input column names for expression evaluation

public:
    ProjectExecutor(std::unique_ptr<Executor> child, 
                   const std::vector<Expr*>& proj,
                   const std::vector<std::string>& cols);
    std::optional<Tuple> next() override;
};

#endif // EXECUTORS_H
