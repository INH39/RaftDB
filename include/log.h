
#ifndef LOG_H
#define LOG_H

#include <vector> 
#include <string>     
#include <optional>    
#include <cstdint>  
#include "entry.h"  


class Log {
private:
    std::string filepath;
    std::vector<Entry> entries;

    void writeToFile();
    void readFromFile();

public:
    Log(const std::string& filepath);
    
    void appendEntry(const Entry& entry);
    void truncateFromIndex(std::uint64_t start_index);
    std::optional<Entry> getEntry(std::uint64_t index) const;
    std::vector<Entry> getEntryRange(std::uint64_t start_index, std::uint64_t end_index) const;
    std::uint64_t size() const;


};


#endif // LOG_H