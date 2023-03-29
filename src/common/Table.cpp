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

    bool Table::insert_replicate(const Participant &participant, const std::shared_ptr<NetworkHandler>& networkHandler) {
        auto config = networkHandler->getDeviceConfig();

        bool returnCode = false;
        std::lock_guard<std::mutex> lk(tableMutex);
        returnCode = data.insert(std::make_pair(participant.hostname, participant)).second;
        if (returnCode) {
            updated = true;
            cv.notify_one();
        }

        Message tableMsg{};
        unsigned long n_participants = get_participants_monitoring().size();
        unsigned long buffer_size = n_participants * sizeof(Participant);
        unsigned long buffer_offset = sizeof(Participant);
        tableMsg.buffer = (char*) malloc(buffer_size);
        unsigned long offset = 0;
        for (auto entry : data) {
            memcpy(&tableMsg.buffer[offset], &entry.second, buffer_offset); // apenas Participant para  o buffer
            offset += buffer_offset;
        }

        for(auto& part: get_participants_monitoring()) {
            if (part.status == ParticipantStatus::Awaken) {
                strncpy(tableMsg.hostname, part.hostname.c_str(), config.getHostname().size());
                strncpy(tableMsg.ip, part.ip.c_str(), config.getIpAddress().size());
                strncpy(tableMsg.mac, part.mac.c_str(), config.getMacAddress().size());
                networkHandler->send(tableMsg, part.ip);
            }
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

    bool Table::update_replicate(const ParticipantStatus &status, const std::string &hostname, const std::shared_ptr<NetworkHandler>& networkHandler) {
        auto config = networkHandler->getDeviceConfig();

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

        Message tableMsg{};
        unsigned long n_participants = get_participants_monitoring().size();
        unsigned long buffer_size = n_participants * sizeof(Participant);
        unsigned long buffer_offset = sizeof(Participant);
        tableMsg.buffer = (char*) malloc(buffer_size);
        unsigned long offset = 0;
        for (auto entry : data) {
            memcpy(&tableMsg.buffer[offset], &entry.second, buffer_offset);
            offset += buffer_offset;
        }

        for(auto& part: get_participants_monitoring()) {
            if (part.status == ParticipantStatus::Awaken) {
                strncpy(tableMsg.hostname, part.hostname.c_str(), config.getHostname().size());
                strncpy(tableMsg.ip, part.ip.c_str(), config.getIpAddress().size());
                strncpy(tableMsg.mac, part.mac.c_str(), config.getMacAddress().size());
                networkHandler->send(tableMsg, part.ip);
            }
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

    bool Table::remove_replicate(const std::string &hostname, const std::shared_ptr<NetworkHandler>& networkHandler) {
        auto config = networkHandler->getDeviceConfig();

        bool returnCode = false;
        std::lock_guard<std::mutex> lk(tableMutex);
        if (data.count(hostname))
            if (data.erase(hostname)) {
                returnCode = true;
                updated = true;
                cv.notify_one();
            }

        Message tableMsg{};
        unsigned long n_participants = get_participants_monitoring().size();
        unsigned long buffer_size = n_participants * sizeof(Participant);
        unsigned long buffer_offset = sizeof(Participant);
        tableMsg.buffer = (char*) malloc(buffer_size);
        unsigned long offset = 0;
        for (auto entry : data) {
            memcpy(&tableMsg.buffer[offset], &entry.second, buffer_offset);
            offset += buffer_offset;
        }

        for(auto& part: get_participants_monitoring()) {
            if (part.status == ParticipantStatus::Awaken) {
                strncpy(tableMsg.hostname, part.hostname.c_str(), config.getHostname().size());
                strncpy(tableMsg.ip, part.ip.c_str(), config.getIpAddress().size());
                strncpy(tableMsg.mac, part.mac.c_str(), config.getMacAddress().size());
                networkHandler->send(tableMsg, part.ip);
            }
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
} // namespace WakeOnLanImpl
