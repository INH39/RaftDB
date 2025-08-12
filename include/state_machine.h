
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <unordered_map>
#include <string>
#include <optional>
#include "entry.h"


class StateMachine {
private:
    std::unordered_map<std::string, std::string> data;

public:
    void applyEntry(const Entry& entry);
    std::optional<std::string> get(const std::string& key) const;
    void getCurrentState() const;
    const std::unordered_map<std::string, std::string>& getData() const;
};


#endif // STATE_MACHINE_H
