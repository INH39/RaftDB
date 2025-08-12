
#ifndef ENTRY_H
#define ENTRY_H

#include <string>  
#include <cstdint>  


enum class OpType {
    PUT,
    DELETE
};

struct Entry {
public:
    std::uint64_t term;
    std::uint64_t index;
    OpType operation;
    std::string key;
    std::string value;

    Entry(std::uint64_t term, std::uint64_t index, OpType operation, 
          const std::string& key, const std::string& value)
        : term(term), index(index), operation(operation), key(key), value(value) {}

    static std::string opTypeToString(OpType op);
};


#endif // ENTRY_H

