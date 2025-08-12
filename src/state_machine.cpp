
#include "state_machine.h"
#include "entry.h"
#include <string>
#include <unordered_map>
#include <optional>
#include <iostream>


void StateMachine::applyEntry(const Entry& entry) {
    if(entry.operation == OpType::PUT){
        bool exists = data.find(entry.key) != data.end();
        this->data[entry.key] = entry.value;

        if(exists) std::cout << "Data has been updated" << std::endl;
        else std::cout << "Data has been inserted" << std::endl;
    }
    else if(entry.operation == OpType::DELETE){
        auto it = this->data.find(entry.key);
        if(it != this->data.end()){
            this->data.erase(entry.key);
            std::cout << "Data has been deleted" << std::endl;
        }
        else{
            std::cout << "Error: Trying to delete inexistant data" << std::endl;
        }
    }
    else{
        std::cout << "Invalid operation type" << std::endl;
    }
}

std::optional<std::string> StateMachine::get(const std::string& key) const {
    auto it = this->data.find(key);
    if(it != this->data.end()){
        return it->second; 
    }
    else{
        return std::nullopt;
    }
}

void StateMachine::getCurrentState() const {
    std::cout << "\n----------------------- \nKey = Value \n-----------------------" << std::endl;
    for(const auto& pair : this->data){
        std::cout << "  " << pair.first << " = " << pair.second << std::endl;
    }
    std::cout << "-----------------------" << std::endl;
}

const std::unordered_map<std::string, std::string>& StateMachine::getData() const {
    return this->data;
}
