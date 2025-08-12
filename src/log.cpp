
#include "entry.h"
#include "log.h"       
#include <iostream>  
#include <fstream>
#include <string>
#include <cstdint> 


void Log::writeToFile(){
    std::ofstream file;
    std::string line = "";

    file.open(this->filepath, std::ios_base::out | std::ios_base::trunc);
    if(!file.is_open()){
        // ios_base::out creates new file if file doesnt exist
        std::cout << "Log file created: " << this->filepath << std::endl;
    }

    for(int i = 0; i < entries.size(); i++){
        std::string line = "";
        std::string attr = "";
        Entry e = entries[i];

        line += "term:";
        attr = std::to_string(e.term); // term:
        line += attr + "\n";

        line += "index:";
        attr = std::to_string(e.index); // index:
        line += attr + "\n";

        line += "operation:";
        attr = Entry::opTypeToString(e.operation); // operation:
        line += attr + "\n";

        line += "key:";
        attr = e.key; // key:
        line += attr + "\n";

        line += "value:";
        attr = e.value; // value:
        line += attr + "\n";

        line += "--------------------\n"; // --------------------

        file << line;
    }
    file.close();

    std::cout << "Log file written: " << this->filepath << std::endl;
    std::cout << "Number of entries: " << this->size() << std::endl;
}

void Log::readFromFile(){
    std::ifstream file;
    std::string line = "";
    std::uint64_t pos = 0;
    std::string k = "";
    std::string v = "";

    file.open(this->filepath, std::ios_base::in);
    if(!file.is_open()){
        std::ofstream new_file(this->filepath);
        new_file.close();
        std::cout << "Log file created: " << this->filepath << std::endl;
        return;
    }
    else{
        while(file.good()){

            std::getline(file, line); // term:
            pos = line.find(":");
            if(pos == std::string::npos) continue;
            k = line.substr(0, pos);
            v = line.substr(pos + 1);
            std::uint64_t term = std::stoull(v);

            std::getline(file, line); // index:
            pos = line.find(":");
            if(pos == std::string::npos) continue;
            k = line.substr(0, pos);
            v = line.substr(pos + 1);  
            std::uint64_t index = std::stoull(v);

            std::getline(file, line); // operation:
            pos = line.find(":");
            if(pos == std::string::npos) continue;
            k = line.substr(0, pos);
            v = line.substr(pos + 1); 
            OpType operation = (v == "PUT") ? OpType::PUT : OpType::DELETE;

            std::getline(file, line); // key:
            pos = line.find(":");
            if(pos == std::string::npos) continue;
            k = line.substr(0, pos);
            v = line.substr(pos + 1); 
            std::string key = v;

            std::getline(file, line); // value:
            pos = line.find(":");
            if(pos == std::string::npos) continue;
            k = line.substr(0, pos);
            v = line.substr(pos + 1); 
            std::string value = v;

            std::getline(file, line); // --------------------

            Entry e = Entry(term, index, operation, key, value);
            this->entries.push_back(e);
        }
        file.close();
    }
    std::cout << "Log file read: " << this->filepath << std::endl;
    std::cout << "Number of entries: " << this->size() << std::endl;
}

Log::Log(const std::string& filepath) 
    : filepath(filepath) 
{
    this->readFromFile();
}

void Log::appendEntry(const Entry& entry){
    this->entries.push_back(entry);
    this->writeToFile();
    std::cout << "New entry added" << std::endl;
}

void Log::truncateFromIndex(std::uint64_t start_index){
    if(start_index <= this->entries.size()){
        this->entries.erase(this->entries.begin() + start_index - 1, this->entries.end());
        this->writeToFile();
    }
}

std::optional<Entry> Log::getEntry(std::uint64_t index) const {
    if(index < 1 || index > this->entries.size()){
        return std::nullopt;
    }
    return this->entries[index - 1];
}

std::vector<Entry> Log::getEntryRange(std::uint64_t start_index, std::uint64_t end_index) const {
    std::vector<Entry> range_entries;
    if(start_index > end_index){
        std::cout << "Invalid range (start > end)." << std::endl;
        return range_entries;
    }
    else{
        for(int i = start_index; i <= end_index; i++){
            auto maybe_entry = this->getEntry(i);
            if(maybe_entry.has_value()){
                range_entries.push_back(maybe_entry.value());
            }
        }
        return range_entries;
    }
}

std::uint64_t Log::size() const {
    return this->entries.size();
}








