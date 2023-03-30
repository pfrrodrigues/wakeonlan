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
        if (data.find(participant.hostname) != data.end())
            return true;
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
            if(it->second.status != status)
            {
                it->second.status = status;
                updated = true;
                cv.notify_one();
            }
            returnCode = true;
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
    
    std::vector<Table::Participant> Table::get_participants_interface() {
        std::vector<Table::Participant> participants;
        std::unique_lock<std::mutex> lk(tableMutex);
        while (!updated) cv.wait(lk);
        for(auto& entry: data)
            participants.push_back(entry.second);
        updated = false;
        return participants;
    }

    std::vector<Table::Participant> Table::get_participants_monitoring() {
        std::vector<Table::Participant> participants;
        std::unique_lock<std::mutex> lk(tableMutex);
        for(auto& entry: data)
            participants.push_back(entry.second);
        return participants;
    }

    Table::Participant Table::get_manager()
    {
        Participant empty_participant;
        empty_participant.status = ParticipantStatus::Unknown;

        if (data.empty())
            return empty_participant;

        std::unique_lock<std::mutex> lk(tableMutex);
        for(auto& entry: data)
            if(entry.second.status == ParticipantStatus::Manager)
                return entry.second;
        
        // if couldn't find manager
        return empty_participant;
    }

} // namespace WakeOnLanImpl
