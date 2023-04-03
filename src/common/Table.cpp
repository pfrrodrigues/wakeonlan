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

    std::pair<uint32_t, std::vector<Table::Participant>> Table::insert(const Participant &participant) {
        bool opSucceded = false;
        uint32_t updateSeqNo = 0;
        std::lock_guard<std::mutex> lk(tableMutex);
        opSucceded = data.insert(std::make_pair(participant.hostname, participant)).second;
        if (opSucceded) {
            if (data.size() == 2)
                seq = 1;
            else if (data.size() > 2)
                seq++;
            updateSeqNo = seq;
            updated = true;
            cv.notify_one();
        }
        std::vector<Table::Participant> members;
        members.reserve(data.size());
        for(auto& entry: data)
            members.push_back(entry.second);
        return std::make_pair(updateSeqNo, members);
    }

    bool Table::transaction(const uint32_t & seqNo, const std::vector<Participant> & tbl) {
        bool returnCode = false;
        std::lock_guard<std::mutex> lk(tableMutex);

        data.clear();
        for (const auto& member : tbl) {
            returnCode = data.insert(std::make_pair(member.hostname, member)).second;
            if (!returnCode) {
                log->error("Error on processing transaction {}", seqNo);
            }
        }
        seq = seqNo;

        if (returnCode) {
            updated = true;
            cv.notify_one();
        }

        return returnCode;
    }

    std::pair<uint32_t, std::vector<Table::Participant>> Table::update(const ParticipantStatus &status, const std::string &hostname) {
        bool opSucceded = false;
        uint32_t updateSeqNo = 0;
        std::lock_guard<std::mutex> lk(tableMutex);
        auto it = data.find(hostname);
        if (it != data.end()) {
            if(it->second.status != status)
            {
                it->second.status = status;
                updated = true;
                cv.notify_one();
                seq++;
                updateSeqNo = seq;
            }
        }
        std::vector<Table::Participant> members;
        members.reserve(data.size());
        for(auto& entry: data)
            members.push_back(entry.second);
        return std::make_pair(updateSeqNo, members);
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
