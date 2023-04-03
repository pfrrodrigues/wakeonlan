#include <../src/handler/NetworkHandler.hpp>
#define WON_PORT 9
#define SERVICE_PORT 4000
#define BROADCAST_ADDRESS "255.255.255.255"
#define LOCAL_SERVER_ADDRESS "0.0.0.0"

namespace WakeOnLanImpl {
    NetworkHandler::NetworkHandler(const uint32_t &port, const Config &cfg)
    : port(port),
      config(cfg),
      globalStatus(Unknown),
      active(false)
    {
        log = spdlog::get("wakeonlan-api");
        log->info("Start Network handler");

        t = std::make_unique<std::thread>([this, port](){
            this->active = true;
            UdpSocket socket(LOCAL_SERVER_ADDRESS, port);
            log->info("Network handler (internal): listening on port {}", port);
            while (active) {
                Message response{};
                response.type = Type::Unknown;
                socket.receive(&response);

                if (response.type != Type::Unknown) {
                    switch (response.type) {
                        case Type::SleepStatusRequest:
                        {
                            std::lock_guard<std::mutex> lk(inetMutex);
                            monitoringQueue.push(response);
                        }
                        break;
                        case Type::SleepServiceDiscovery:
                        case Type::SleepServiceExit:
                        {
                            std::lock_guard<std::mutex> lk(inetMutex);
                            discoveryQueue.push(response);
                        }
                        break;
                        case Type::TableUpdate:
                        {
                            std::lock_guard<std::mutex> lk(inetMutex);
                            monitoringQueue.push(response);
                        }
                        break;
                        case Type::ElectionServiceElection:
                        case Type::ElectionServiceAnswer:
                        case Type::ElectionServiceCoordinator:
                        {
                            std::lock_guard<std::mutex> lk(inetMutex);
                            electionQueue.push(response);
                        }
                        break;
                        default:
                            log->warn("Network handler (internal): received a message with unknown type");
                            break;
                    }
                }
            }
            socket.closeSocket();
            log->info("Network handler (internal): listener socket is closed");
        });
    }

    NetworkHandler::~NetworkHandler() {
        if (t->joinable()) {
            t->join();
        }
    }

    bool NetworkHandler::send(const Message &message, const std::string &ip) {
        std::lock_guard<std::mutex> lk(inetMutex);
        UdpSocket socket(ip, SERVICE_PORT);
        socket.send(message);
        socket.closeSocket();
        return true;
    }
    
    bool NetworkHandler::multicast(std::vector<Table::Participant> group, uint32_t seqNo)
    {
        Message multicastMsg{};
        multicastMsg.type = Type::TableUpdate;
        multicastMsg.msgSeqNum = seqNo;
        bzero(multicastMsg.hostname, sizeof(multicastMsg.hostname));
        bzero(multicastMsg.ip, sizeof(multicastMsg.ip));
        bzero(multicastMsg.mac, sizeof(multicastMsg.mac));
        strncpy(multicastMsg.hostname, config.getHostname().c_str(), config.getHostname().size());
        strncpy(multicastMsg.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
        strncpy(multicastMsg.mac, config.getMacAddress().c_str(), config.getMacAddress().size());

        size_t offset = 0;
        uint8_t noEntries = group.size();
        char electedTimestamp[WAKEONLAN_FIELD_TIMESTAMP_SIZE];
        char host[WAKEONLAN_FIELD_HOSTNAME_SIZE];
        char ip[WAKEONLAN_FIELD_IP_SIZE];
        char mac[WAKEONLAN_FIELD_MAC_SIZE];

        memcpy(&multicastMsg.data[offset], &noEntries, sizeof(noEntries));
        offset += sizeof(noEntries);

        /* Inserts table entries on the message */
        for (auto & member : group) {
            bzero(electedTimestamp, WAKEONLAN_FIELD_TIMESTAMP_SIZE);
            bzero(host, WAKEONLAN_FIELD_HOSTNAME_SIZE);
            bzero(ip, WAKEONLAN_FIELD_IP_SIZE);
            bzero(mac, WAKEONLAN_FIELD_MAC_SIZE);

            memcpy(electedTimestamp, member.electedTimestamp.c_str(), member.electedTimestamp.size());
            memcpy(host, member.hostname.c_str(), member.hostname.size());
            memcpy(ip, member.ip.c_str(), member.ip.size());
            memcpy(mac, member.mac.c_str(), member.mac.size());

            memcpy(&multicastMsg.data[offset], electedTimestamp, WAKEONLAN_FIELD_TIMESTAMP_SIZE);
            offset += WAKEONLAN_FIELD_TIMESTAMP_SIZE;

            memcpy(&multicastMsg.data[offset], host, WAKEONLAN_FIELD_HOSTNAME_SIZE);
            offset += WAKEONLAN_FIELD_HOSTNAME_SIZE;

            memcpy(&multicastMsg.data[offset], ip, WAKEONLAN_FIELD_IP_SIZE);
            offset += WAKEONLAN_FIELD_IP_SIZE;

            memcpy(&multicastMsg.data[offset], mac, WAKEONLAN_FIELD_MAC_SIZE);
            offset += WAKEONLAN_FIELD_MAC_SIZE;

            auto status = static_cast<uint8_t>(member.status);
            memcpy(&multicastMsg.data[offset], &status, WAKEONLAN_FIELD_STATUS_SIZE);
            offset += WAKEONLAN_FIELD_STATUS_SIZE;
        }

        log->info("Sending a MULTICAST message to the group [seq={} no_entries={}]", multicastMsg.msgSeqNum, noEntries);
        for (auto & member : group) {
            if (member.status != Table::ParticipantStatus::Manager
                && member.status != Table::ParticipantStatus::Unknown) {
                send(multicastMsg, member.ip);
                log->info("Sent a TableUpdate UNICAST message to {}", member.ip);
            }
        }

    }

    Message* NetworkHandler::getFromDiscoveryQueue() {
        std::lock_guard<std::mutex> lk(inetMutex);
        static bool free = false;
        if (free) {
            discoveryQueue.pop();
        }

        if (!discoveryQueue.empty()) {
            Message * k = &discoveryQueue.front();
            free = true;
            return k;
        }
        else {
            free = false;
            return nullptr;
        }
    }

    Message* NetworkHandler::getFromMonitoringQueue() {
        std::lock_guard<std::mutex> lk(inetMutex);
        static bool free = false;
        if (free) {
            monitoringQueue.pop();
        }

        if (!monitoringQueue.empty()) {
            Message * k = &monitoringQueue.front();
            free = true;
            return k;
        }
        else {
            free = false;
            return nullptr;
        }
    }

    Message* NetworkHandler::getFromElectionQueue() {
        std::lock_guard<std::mutex> lk(inetMutex);
        static bool free = false;
        if (free) {
            electionQueue.pop();
        }

        if (!electionQueue.empty()) {
            Message * k = &electionQueue.front();
            free = true;
            return k;
        }
        else {
            free = false;
            return nullptr;
        }
    }

    bool NetworkHandler::wakeUp(const std::string &mac) {
        std::lock_guard<std::mutex> lk(inetMutex);
        UdpSocket socket(BROADCAST_ADDRESS, WON_PORT);
        size_t pos;
        std::string delimiter = ":";
        std::string byte;
        std::string hex_mac;
        std::string copy = mac;
        std::string buffer("\xff\xff\xff\xff\xff\xff");

        while ((pos = copy.find(delimiter)) != std::string::npos) {
            byte = copy.substr(0, pos);
            char *ptr;
            long ret;
            ret = strtol(byte.c_str(), &ptr, 16);
            char b = (char)ret;
            hex_mac.append(1, b);
            copy.erase(0, pos + delimiter.size());
        }
        byte = copy.substr(0, pos);
        char *ptr;
        long ret;
        ret = strtol(byte.c_str(), &ptr, 16);
        char b = (char)ret;
        hex_mac.append(1, b);

        for (int i=0; i<16; i++)
            buffer.append(hex_mac);

        socket.sendMagicPacket(buffer.c_str());
        socket.closeSocket();
        return true;
    }

    void NetworkHandler::changeStatus(const ServiceGlobalStatus &gs) {
        std::lock_guard<std::mutex> lk(gsMutex);
        globalStatus = gs;
        log = spdlog::get("wakeonlan-api");
        log->info("STATUS UPDATE: {}", [this]() {
            switch (globalStatus) {
                case WaitingForSync:
                    return "WaitingForSync";
                case Syncing:
                    return "Syncing";
                case Synchronized:
                    return "Synchronized";
                case Unknown:
                    return "Unknown";
                case ManagerFailure:
                    return "ManagerFailure";
                case NotSynchronized:
                    return "NotSynchronized";
                default:
                    return "NOT IMPLEMENTED";
            }
        }());
    }

    Config NetworkHandler::changeHandlerType(const HandlerType &ht) {
        config.setHandlerType(ht);
        return config;
    }

    const ServiceGlobalStatus& NetworkHandler::getGlobalStatus() {
        std::lock_guard<std::mutex> lk(gsMutex);
        return globalStatus;
    }

    const Config &NetworkHandler::getDeviceConfig() { return config; }

    void NetworkHandler::setManagerIp(std::string ip) {managerIp = ip;}

    std::string NetworkHandler::getManagerIp() {return managerIp;}

    void NetworkHandler::stop() {
        active = false;
        sleep(1);
        log->info("Stop Network handler");
    }
} // namespace WakeOnLanImpl;
