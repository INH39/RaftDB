
#include "entry.h"
#include <string>


std::string Entry::opTypeToString(OpType op) {
    switch(op) {
        case OpType::PUT: 
            return "PUT";
        case OpType::DELETE: 
            return "DELETE";
        default: 
            return "UNKNOWN";
    }
}

