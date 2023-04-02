#include <../src/service/DiscoveryService.hpp>
#include <memory>

namespace WakeOnLanImpl {
#define WAKEONLAN_SYN 1
#define WAKEONLAN_SYN_ACK 2
#define WAKEONLAN_BROADCAST_ADDRESS "255.255.255.255"
#define WAKEONLAN_DISCOVERY_REQUEST_WINDOW 10

    DiscoveryService::DiscoveryService(Table &t, std::shared_ptr<NetworkHandler> nh)
        : table(t),
        inetHandler(nh),
        active(false),
        lastTimestamp(std::time(nullptr))
    {}

    DiscoveryService::~DiscoveryService() {
        if (t) {
            if (t->joinable())
                t->join();
        }
    }

    void DiscoveryService::runAsManager() {
        if (t)
            t->join();

        active = true;
        this->log = spdlog::get("wakeonlan-api");

        t = std::make_unique<std::thread>([this](){
            log->info("Start Discovery service");
            auto config = inetHandler->getDeviceConfig();
            while (active) {
                Message *m;
                if (inetHandler->getGlobalStatus() == Unknown) {
                    inetHandler->changeStatus(Synchronized);
                    Table::Participant p;
                    p.electedTimestamp = (char*)"N/A";
                    p.ip = config.getIpAddress();
                    p.mac = config.getMacAddress();
                    p.hostname = config.getHostname();
                    p.status = Table::ParticipantStatus::Manager;
                    table.insert(p);
                    log->info("Added manager entry on the group table");
                }

                if (inetHandler->getGlobalStatus() == Synchronized) {
                    /* Sent a broadcast packet each 10 seconds */
                    if (lastTimestamp < (std::time(nullptr) - WAKEONLAN_DISCOVERY_REQUEST_WINDOW)) {
                        Message broadcastMsg{};
                        broadcastMsg.type = WakeOnLanImpl::Type::SleepServiceDiscovery;
                        broadcastMsg.msgSeqNum = WAKEONLAN_SYN;
                        bzero(broadcastMsg.hostname, sizeof(broadcastMsg.hostname));
                        bzero(broadcastMsg.ip, sizeof(broadcastMsg.ip));
                        bzero(broadcastMsg.mac, sizeof(broadcastMsg.mac));
                        strncpy(broadcastMsg.hostname, config.getHostname().c_str(), config.getHostname().size());
                        strncpy(broadcastMsg.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
                        strncpy(broadcastMsg.mac, config.getMacAddress().c_str(), config.getMacAddress().size());
                        inetHandler->send(broadcastMsg, WAKEONLAN_BROADCAST_ADDRESS);

                        lastTimestamp = std::time(nullptr);
                    }

                    m = inetHandler->getFromDiscoveryQueue();
                    if (m != nullptr) {
                        switch (m->type) {
                            case Type::SleepServiceDiscovery:
                                if (m->msgSeqNum == WAKEONLAN_SYN_ACK) {
                                    Table::Participant newParticipant;
                                    newParticipant.ip = m->ip;
                                    newParticipant.mac = m->mac;
                                    newParticipant.hostname = m->hostname;
                                    newParticipant.status = Table::ParticipantStatus::Unknown;

                                    auto ret = table.insert(newParticipant);
                                    if ( ret.first != 0) {
                                        log->info("Participant has joined the group [Hostname={}, IP={}, MAC={}]",
                                                  newParticipant.hostname, newParticipant.ip, newParticipant.mac);

                                        /**
                                         * Send multicast message
                                         */
                                        /* Manager information */
                                        Message multicastMsg{};
                                        multicastMsg.type = Type::TableUpdate;
                                        multicastMsg.msgSeqNum = ret.first;
                                        bzero(multicastMsg.hostname, sizeof(multicastMsg.hostname));
                                        bzero(multicastMsg.ip, sizeof(multicastMsg.ip));
                                        bzero(multicastMsg.mac, sizeof(multicastMsg.mac));
                                        strncpy(multicastMsg.hostname, config.getHostname().c_str(), config.getHostname().size());
                                        strncpy(multicastMsg.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
                                        strncpy(multicastMsg.mac, config.getMacAddress().c_str(), config.getMacAddress().size());

                                        size_t offset = 0;
                                        uint8_t noEntries = ret.second.size();
                                        char electedTimestamp[WAKEONLAN_FIELD_TIMESTAMP_SIZE];
                                        char hostname[WAKEONLAN_FIELD_HOSTNAME_SIZE];
                                        char ip[WAKEONLAN_FIELD_IP_SIZE];
                                        char mac[WAKEONLAN_FIELD_MAC_SIZE];

                                        memcpy(&multicastMsg.data[offset], &noEntries, sizeof(noEntries));
                                        offset += sizeof(noEntries);

                                        /* Inserts table entries on the message */
                                        assert(ret.second.size() < 5);
                                        for (auto & member : ret.second) {
                                            bzero(electedTimestamp, WAKEONLAN_FIELD_TIMESTAMP_SIZE);
                                            bzero(hostname, WAKEONLAN_FIELD_HOSTNAME_SIZE);
                                            bzero(ip, WAKEONLAN_FIELD_IP_SIZE);
                                            bzero(mac, WAKEONLAN_FIELD_MAC_SIZE);

                                            memcpy(electedTimestamp, member.electedTimestamp.c_str(), member.electedTimestamp.size());
                                            memcpy(hostname, member.hostname.c_str(), member.hostname.size());
                                            memcpy(ip, member.ip.c_str(), member.ip.size());
                                            memcpy(mac, member.mac.c_str(), member.mac.size());

                                            memcpy(&multicastMsg.data[offset], electedTimestamp, WAKEONLAN_FIELD_TIMESTAMP_SIZE);
                                            offset += WAKEONLAN_FIELD_TIMESTAMP_SIZE;

                                            memcpy(&multicastMsg.data[offset], hostname, WAKEONLAN_FIELD_HOSTNAME_SIZE);
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
                                        for (const auto & member : ret.second) {
                                            if (member.status != Table::ParticipantStatus::Manager
                                                || member.status != Table::ParticipantStatus::Unknown) {
                                                inetHandler->send(multicastMsg, member.ip);
                                                log->info("Sent a TableUpdate UNICAST message to {}", member.ip);
                                            }
                                        }
                                    }
                                    else {
                                        log->warn("Failed to insert participant on the group [Hostname={}, IP={}, MAC={}]",
                                                  newParticipant.hostname, newParticipant.ip, newParticipant.mac);
                                    }
                                }
                                break;
                            case Type::SleepServiceExit:
                                if (table.remove(m->hostname))
                                    log->info("Participant was removed from the group [Hostname={}, IP={}, MAC={}]",
                                              m->hostname, m->ip,m->mac);
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        });
    }

    void DiscoveryService::runAsParticipant() {
        if (t)
            t->join();

        active = true;
        this->log = spdlog::get("wakeonlan-api");

        t = std::make_unique<std::thread>([this](){
            log->info("Start Discovery service");
            auto serviceStatus = inetHandler->getGlobalStatus();
            auto config = inetHandler->getDeviceConfig();

            if (serviceStatus == Unknown)
                inetHandler->changeStatus(WaitingForSync);

            while (active) {
                Message *m;

                m = inetHandler->getFromDiscoveryQueue();
                if (m != nullptr) {
                    switch (m->type) {
                        case Type::SleepServiceDiscovery:
                            switch (inetHandler->getGlobalStatus()) {
                                case WaitingForSync:
                                    if (m->msgSeqNum == WAKEONLAN_SYN) {
                                        Message message{};
                                        message.type = WakeOnLanImpl::Type::SleepServiceDiscovery;
                                        message.msgSeqNum = WAKEONLAN_SYN_ACK;
                                        bzero(message.hostname, sizeof(message.hostname));
                                        bzero(message.ip, sizeof(message.ip));
                                        bzero(message.mac, sizeof(message.mac));
                                        strncpy(message.hostname, config.getHostname().c_str(), config.getHostname().size());
                                        strncpy(message.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
                                        strncpy(message.mac, config.getMacAddress().c_str(), config.getMacAddress().size());

                                        /* Add manager on the table */
                                        {
                                            Table::Participant p;
                                            p.ip = m->ip;
                                            p.hostname = m->hostname;
                                            p.mac = m->mac;
                                            p.status = Table::ParticipantStatus::Awaken;
                                            table.insert(p);
                                        }

                                        inetHandler->send(message, m->ip);
                                        inetHandler->changeStatus(Syncing);
                                    }
                                    break;
                                default:
                                    break;
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
        });
    }

    void DiscoveryService::run() {
        auto config = inetHandler->getDeviceConfig();
        switch (config.getHandlerType()) {
            case HandlerType::Manager:
                runAsManager();
                break;
            case HandlerType::Participant:
                runAsParticipant();
                break;
            default:
                break;
        }
    }

    void DiscoveryService::stop() {
        log->info("Stop Discovery service");
    }
}