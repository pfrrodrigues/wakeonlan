#include <condition_variable>
#include <iostream>
#include <../src/common/Table.hpp>

namespace WakeOnLanImpl {
    bool updated = false;
    std::condition_variable cv;

    Table &Table::get() {
        static Table instance;
        return instance;
    }

    bool Table::insert(const Participant &participant) {
        bool returnCode = false;
        std::lock_guard<std::mutex> lk(tableMutex);
        returnCode = data.insert(std::make_pair(participant.hostname, participant)).second;
        if (returnCode) {
            updated = true;
            cv.notify_one();
        }
        return returnCode;
    }

    bool Table::update(const ParticipantStatus &status, const std::string &hostname) {
	bool returnCode = false;
        std::lock_guard<std::mutex> lk(tableMutex);
        auto it = data.find(hostname);
        if (it != data.end()) {
            it->second.status = status;
            returnCode = true;
            updated = true;
            cv.notify_one();
        }
        return returnCode;
    }

    bool Table::remove(const std::string &hostname) {
        bool returnCode = false;
        std::lock_guard<std::mutex> lk(tableMutex);
        if (data.count(hostname))
            if (data.erase(hostname)) {
                returnCode = true;
                updated = true;
                cv.notify_one();
            }
        return returnCode;
    }
    
    std::vector<Table::Participant> Table::get_participants() {
        std::vector<Table::Participant> participants;
        std::unique_lock<std::mutex> lk(tableMutex);
        while (!updated) cv.wait(lk);
        for(auto& entry: data)
            participants.push_back(entry.second);
        updated = false;
        return participants;
    }
} // namespace WakeOnLanImpl
