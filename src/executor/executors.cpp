#include "executors.h"
#include "evaluator.h"
#include <stdexcept>

// SeqScanExecutor implementation
SeqScanExecutor::SeqScanExecutor(Storage& s, const std::string& table)
    : storage(s), table_name(table), cursor(0) {
    this->table = &storage.get_table(table_name);
}

std::optional<Tuple> SeqScanExecutor::next() {
    if (cursor >= table->size()) {
        return std::nullopt;  // No more tuples
    }
    return (*table)[cursor++];
}

// FilterExecutor implementation
FilterExecutor::FilterExecutor(std::unique_ptr<Executor> child, Expr* pred, 
                               const std::vector<std::string>& cols)
    : child(std::move(child)), predicate(pred), column_names(cols) {
}

std::optional<Tuple> FilterExecutor::next() {
    while (true) {
        std::optional<Tuple> tuple = child->next();
        if (!tuple.has_value()) {
            return std::nullopt;  // No more tuples from child
        }
        
        // Evaluate predicate on this tuple
        if (evaluate_predicate(predicate, tuple.value(), column_names)) {
            return tuple;  // Tuple passes filter
        }
        // Otherwise, continue loop to get next tuple
    }
}

// ProjectExecutor implementation
ProjectExecutor::ProjectExecutor(std::unique_ptr<Executor> child, 
                                  const std::vector<Expr*>& proj,
                                  const std::vector<std::string>& cols)
    : child(std::move(child)), projections(proj), column_names(cols) {
}

std::optional<Tuple> ProjectExecutor::next() {
    std::optional<Tuple> input_tuple = child->next();
    if (!input_tuple.has_value()) {
        return std::nullopt;  // No more tuples from child
    }
    
    // Evaluate each projection expression
    Tuple output_tuple;
    for (Expr* proj : projections) {
        Value value = evaluate_expr(proj, input_tuple.value(), column_names);
        output_tuple.push_back(value);
    }
    
    return output_tuple;
}
