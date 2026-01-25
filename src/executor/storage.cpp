#include "storage.h"
#include <stdexcept>

Storage::Storage() {
    // Initialize with dummy data: [[1, "Alice", 20], [2, "Bob", 17], [3, "Carol", 25]]
    // Assuming table name is "users" with columns: [id, name, age]
    Table users_table;
    users_table.push_back({1, std::string("Alice"), 20});
    users_table.push_back({2, std::string("Bob"), 17});
    users_table.push_back({3, std::string("Carol"), 25});
    
    tables["users"] = users_table;
}

Table& Storage::get_table(const std::string& name) {
    if (tables.find(name) == tables.end()) {
        // Create empty table if it doesn't exist
        tables[name] = Table();
    }
    return tables[name];
}

void Storage::insert(const std::string& table, const Tuple& tuple) {
    get_table(table).push_back(tuple);
}

bool Storage::has_table(const std::string& name) const {
    return tables.find(name) != tables.end();
}
