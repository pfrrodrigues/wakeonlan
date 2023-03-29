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
                                    Table::Participant p;
                                    p.ip = m->ip;
                                    p.mac = m->mac;
                                    p.hostname = m->hostname;
                                    p.status = Table::ParticipantStatus::Unknown;

                                    if (table.insert_replicate(p, inetHandler)) {
                                        log->info("Participant has joined the group [Hostname={}, IP={}, MAC={}]",
                                                  m->hostname, m->ip,m->mac);
                                    }
                                }
                                break;
                            case Type::SleepServiceExit:
                                if (table.remove_replicate(m->hostname, inetHandler))
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
                                            table.insert_replicate(p, inetHandler);
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