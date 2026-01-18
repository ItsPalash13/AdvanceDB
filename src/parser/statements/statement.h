#ifndef STATEMENT_H
#define STATEMENT_H

#include <variant>
#include <stdexcept>
#include "select.h"
#include "create.h"
#include "insert.h"
#include "update.h"
#include "delete.h"

// Statement type enum
enum class StatementType {
    Select,
    Create,
    Insert,
    Update,
    Delete
};

// Statement abstraction class using std::variant
class Statement {
private:
    std::variant<SelectStmt, CreateStmt, InsertStmt, UpdateStmt, DeleteStmt> data;

public:
    // Constructor for Select statement
    Statement(SelectStmt stmt) : data(stmt) {}

    // Constructor for Create statement
    Statement(CreateStmt stmt) : data(stmt) {}

    // Constructor for Insert statement
    Statement(InsertStmt stmt) : data(stmt) {}

    // Constructor for Update statement
    Statement(UpdateStmt stmt) : data(stmt) {}

    // Constructor for Delete statement
    Statement(DeleteStmt stmt) : data(stmt) {}

    // Copy constructor (default is fine with std::variant)
    Statement(const Statement& other) = default;

    // Assignment operator (default is fine with std::variant)
    Statement& operator=(const Statement& other) = default;

    // Destructor (default is fine with std::variant)
    ~Statement() = default;

    // Get statement type
    StatementType get_type() const {
        if (std::holds_alternative<SelectStmt>(data)) {
            return StatementType::Select;
        }
        if (std::holds_alternative<CreateStmt>(data)) {
            return StatementType::Create;
        }
        if (std::holds_alternative<InsertStmt>(data)) {
            return StatementType::Insert;
        }
        if (std::holds_alternative<UpdateStmt>(data)) {
            return StatementType::Update;
        }
        if (std::holds_alternative<DeleteStmt>(data)) {
            return StatementType::Delete;
        }
        throw std::runtime_error("Unknown statement type");
    }

    // Access SELECT statement
    SelectStmt& as_select() {
        if (!std::holds_alternative<SelectStmt>(data)) {
            throw std::runtime_error("Statement is not a SELECT statement");
        }
        return std::get<SelectStmt>(data);
    }

    const SelectStmt& as_select() const {
        if (!std::holds_alternative<SelectStmt>(data)) {
            throw std::runtime_error("Statement is not a SELECT statement");
        }
        return std::get<SelectStmt>(data);
    }

    // Access CREATE statement
    CreateStmt& as_create() {
        if (!std::holds_alternative<CreateStmt>(data)) {
            throw std::runtime_error("Statement is not a CREATE statement");
        }
        return std::get<CreateStmt>(data);
    }

    const CreateStmt& as_create() const {
        if (!std::holds_alternative<CreateStmt>(data)) {
            throw std::runtime_error("Statement is not a CREATE statement");
        }
        return std::get<CreateStmt>(data);
    }

    // Access INSERT statement
    InsertStmt& as_insert() {
        if (!std::holds_alternative<InsertStmt>(data)) {
            throw std::runtime_error("Statement is not an INSERT statement");
        }
        return std::get<InsertStmt>(data);
    }

    const InsertStmt& as_insert() const {
        if (!std::holds_alternative<InsertStmt>(data)) {
            throw std::runtime_error("Statement is not an INSERT statement");
        }
        return std::get<InsertStmt>(data);
    }

    // Access UPDATE statement
    UpdateStmt& as_update() {
        if (!std::holds_alternative<UpdateStmt>(data)) {
            throw std::runtime_error("Statement is not an UPDATE statement");
        }
        return std::get<UpdateStmt>(data);
    }

    const UpdateStmt& as_update() const {
        if (!std::holds_alternative<UpdateStmt>(data)) {
            throw std::runtime_error("Statement is not an UPDATE statement");
        }
        return std::get<UpdateStmt>(data);
    }

    // Access DELETE statement
    DeleteStmt& as_delete() {
        if (!std::holds_alternative<DeleteStmt>(data)) {
            throw std::runtime_error("Statement is not a DELETE statement");
        }
        return std::get<DeleteStmt>(data);
    }

    const DeleteStmt& as_delete() const {
        if (!std::holds_alternative<DeleteStmt>(data)) {
            throw std::runtime_error("Statement is not a DELETE statement");
        }
        return std::get<DeleteStmt>(data);
    }
};

// Forward declaration
class Parser;

// Main parse_statement function - returns Statement abstraction
Statement parse_statement(Parser& parser);

#endif // STATEMENT_H
