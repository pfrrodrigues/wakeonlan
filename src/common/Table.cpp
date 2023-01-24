#include <../src/common/Table.hpp>
#include <iostream>

namespace WakeOnLanImpl {
    Table &Table::get() {
        static Table instance;
        return instance;
    }

    bool Table::insert(const Participant &participant) {
        bool returnCode = false;
        std::lock_guard<std::mutex> lk(tableMutex);
        returnCode = data.insert(std::make_pair(participant.hostname, participant)).second;
        return returnCode;
    }

    bool Table::update(const ParticipantStatus &status, const std::string &hostname) {
        bool returnCode = false;
        std::lock_guard<std::mutex> lk(tableMutex);
        if (auto it = data.find(hostname); it != data.end()) {
            it->second.status = status;
            returnCode = true;
        }
        return returnCode;
    }

    bool Table::remove(const std::string &hostname) {
        bool returnCode = false;
        std::lock_guard<std::mutex> lk(tableMutex);
        if (data.count(hostname))
            if (data.erase(hostname))
                returnCode = true;
        return returnCode;
    }
} // namespace WakeOnLanImpl