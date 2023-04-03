#include <../src/service/DiscoveryService.hpp>
#include <memory>

namespace WakeOnLanImpl {
#define WAKEONLAN_SYN 1
#define WAKEONLAN_SYN_ACK 2
#define WAKEONLAN_BROADCAST_ADDRESS "255.255.255.255"
#define WAKEONLAN_DISCOVERY_REQUEST_WINDOW 10
#define WAKEONLAN_DISCOVERY_TIMEOUT 12 

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
        t = std::make_unique<std::thread>([this](){
            auto config = inetHandler->getDeviceConfig();
            while (active) {
                Message *m;
                if (inetHandler->getGlobalStatus() == Unknown) {
                    inetHandler->changeStatus(Synchronized);
                }

                if (inetHandler->getGlobalStatus() == Synchronized) {
                    /* Sent a broadcast packet each 10 seconds */
                    if (lastTimestamp < (std::time(nullptr) - WAKEONLAN_DISCOVERY_REQUEST_WINDOW)) {
                        if(lastTimestamp - (std::time(nullptr) - WAKEONLAN_DISCOVERY_REQUEST_WINDOW) > 2)
                        {
                            log->info("Probably fell asleep");
                            inetHandler->changeStatus(ServiceGlobalStatus::NotSynchronized);
                        }
                        else{
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
                                    newParticipant.electedTimestamp = (char*)"N/A";

                                    auto ret = table.insert(newParticipant);
                                    if ( ret.first != 0) {
                                        log->info("Participant has joined the group [Hostname={}, IP={}, MAC={}]",
                                                  newParticipant.hostname, newParticipant.ip, newParticipant.mac);

                                        /**
                                         * Send multicast message
                                         */
                                        /* Manager information */
                                        inetHandler->multicast(ret.second, ret.first);
                                    }
                                    else {
                                        log->warn("Failed to insert participant on the group [Hostname={}, IP={}, MAC={}]",
                                                  newParticipant.hostname, newParticipant.ip, newParticipant.mac);
                                    }
                                }
                                         // SYNC message                  // sender's mac is different from self's
                                else if (m->msgSeqNum == WAKEONLAN_SYN && config.getMacAddress().compare(m->mac) != 0) { 
                                    log->info("Two managers online. Declaring manager failure.");
                                    Table::Participant p;
                                    p.ip = m->ip;
                                    p.mac = m->mac;
                                    p.hostname = m->hostname;
                                    p.status = Table::ParticipantStatus::Manager;
                                    auto ret = table.insert(p);
                                    if (ret.first) {
                                        log->info("Concurrent manager added to group [Hostname={}, IP={}, MAC={}]",
                                                  m->hostname, m->ip,m->mac);
                                    }
                                    inetHandler->changeStatus(ManagerFailure);
                                }
                                break;
                            case Type::SleepServiceExit:
                            {
                                auto ret = table.remove(m->hostname);
                                if (ret.first)
                                {
                                    log->info("Participant was removed from the group [Hostname={}, IP={}, MAC={}]",
                                              m->hostname, m->ip,m->mac);
                                    inetHandler->multicast(ret.second, ret.first);
                                }
                            }
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
        t = std::make_unique<std::thread>([this](){
            auto serviceStatus = inetHandler->getGlobalStatus();
            auto config = inetHandler->getDeviceConfig();
            bool timerSet = false;
            time_t timer;

            if (serviceStatus == Unknown)
                inetHandler->changeStatus(WaitingForSync);

            while (active) {
                Message *m;
                m = inetHandler->getFromDiscoveryQueue();
                if (m != nullptr && m->type == Type::SleepServiceDiscovery) {
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

                            inetHandler->setManagerIp(m->ip);

                            inetHandler->send(message, m->ip);
                            inetHandler->changeStatus(Syncing);
                        }
                        break;
                    default:
                        break;
                    }
                }
                if(inetHandler->getGlobalStatus() == WaitingForSync)
                {
                    if(!timerSet){
                        timerSet = true;
                        timer = std::time(0);
                    }
                    else if(std::time(0) - timer > WAKEONLAN_DISCOVERY_TIMEOUT)
                    {
                        log->info("Discovery service has timed-out. Declaring manager failure.");
                        inetHandler->changeStatus(ManagerFailure);
                    }
                }
            }
        });
    }

    void DiscoveryService::run() {
        auto config = inetHandler->getDeviceConfig();
        this->log = spdlog::get("wakeonlan-api");
        log->info("Start Discovery service");
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

    void DiscoveryService::notifyRoleChange() {
        active = false;
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
}