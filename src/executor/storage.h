#ifndef STORAGE_H
#define STORAGE_H

#include "types.h"
#include <map>
#include <string>

// Dummy in-memory storage for testing
class Storage {
private:
    std::map<std::string, Table> tables;

public:
    // Constructor: initializes with dummy data
    Storage();
    
    // Get reference to table data
    Table& get_table(const std::string& name);
    
    // Insert a tuple into a table
    void insert(const std::string& table, const Tuple& tuple);
    
    // Check if table exists
    bool has_table(const std::string& name) const;
};

#endif // STORAGE_H
