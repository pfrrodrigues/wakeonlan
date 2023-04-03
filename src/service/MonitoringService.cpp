#include <../src/service/MonitoringService.hpp>
#include <ctime>

namespace WakeOnLanImpl {
    MonitoringService::MonitoringService(Table &t, std::shared_ptr<NetworkHandler> nh)
        : table(t),
        inetHandler(nh),
        active(false)
    {}

    MonitoringService::~MonitoringService() {
        if (t) {
            if (t->joinable())
                t->join();
        }
    }

    void MonitoringService::run() {
        this->log = spdlog::get("wakeonlan-api");
        log->info("Start Monitoring service");
        Config config = inetHandler->getDeviceConfig();
        switch (config.getHandlerType())
        {
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

    void MonitoringService::runAsManager() {
        if (t)
            t->join();

        log->info("Monitoring service operating as Manager.");
        active = true;
        t = std::make_unique<std::thread>([this]() {
            time_t timestamp;
            bool timerSet;
            bool updated = false;
            std::vector<std::string> sleeping_participants;
            Message *msg;
            auto config = inetHandler->getDeviceConfig();
            while (active)
            {
                if(timestamp + 8 < std::time(nullptr) || !timerSet) // every 8s
                {
                    // so it doesn't have to wait 8s for before the first call
                    if(!timerSet)    
                        timerSet = true;

                    // participants still in sleeping_participants haven't answered and 
                    // the last call are considered to be Sleeping
                    for(auto& hostname : sleeping_participants) {
                        auto ret = table.update(Table::ParticipantStatus::Sleeping, hostname);
                         if ( ret.first ) {
                             /**
                              * Send multicast message
                              */
                             inetHandler->multicast(ret.second, ret.first);
                         }
                    }

                    // resets vectors
                    sleeping_participants.clear();
                    for(auto& participant: table.get_participants_monitoring())
                    {
                        // sleeping_participants is initialized with every participant
                        // currently in the table
                        if (participant.ip != config.getIpAddress()) {
                            sleeping_participants.push_back(participant.hostname);
                            Message message = getSleepStatusRequest(1);
                            inetHandler->send(message, participant.ip);
                        }

                        // send sleep status request 
                        // std::cout << "Manager is sending sleep status request to "
                        //           << participant.hostname << " @ " 
                        //           << participant.ip << std::endl;
                    }
                    timestamp = std::time(nullptr); // reset timer
                }
                msg = inetHandler->getFromMonitoringQueue();
                if(msg && msg->msgSeqNum == 2) // if there is an answer from a participant 
                {
                    // erase participant from sleeping_participants and puts
                    // it in awaken_participants
                    std::string hostname = msg->hostname;
                    auto ret = table.update(Table::ParticipantStatus::Awaken, msg->hostname);
                    sleeping_participants.erase(std::find(sleeping_participants.begin(),
                                                          sleeping_participants.end(),
                                                          hostname));
                    if ( ret.first != 0 ) {
                        /**
                        * Send multicast message
                        */
                        log->info("Member {} have its status changed to AWAKEN", hostname);
                        inetHandler->multicast(ret.second, ret.first);
                        // std::cout << "Manager got answer from "
                        //           << msg->hostname << " @ "
                        //           << msg->ip << std::endl;
                    }
                }
            }
        });
    }

    void MonitoringService::runAsParticipant()
    {
        if (t)
            t->join();

        log->info("Monitoring service operating as Participant.");

        active = true;
        t = std::make_unique<std::thread>([this]() {
            ServiceGlobalStatus status;
            Message *msg;
            time_t timestamp;
            bool timerSet = false;
            while(active)
            {
                status = inetHandler->getGlobalStatus();
                switch (status)
                {
                // has to wait 35s for a sleep status request message or else 
                // assumes it should wait a new discovery message
                case ServiceGlobalStatus::Syncing:
                case ServiceGlobalStatus::Synchronized:
                    if(!timerSet)
                    {
                        timestamp = std::time(nullptr);
                        timerSet = true;
                    }
                    if(timestamp + 17 < std::time(nullptr))
                    {
                        if(std::time(nullptr) - (timestamp + 17) > 1 ) {
                            // participante provavelmente dormiu
                            timestamp = std::time(nullptr);
                            timerSet = true;
                            log->info("Participant has probably slept");
                        }
                        else
                        {
                            inetHandler->changeStatus(ServiceGlobalStatus::WaitingForSync);
                            timerSet = false;
                            // std::cout << "Timed out waiting for sleep status request, waiting new discovery msg" << std::endl;
                            log->info("Participant timedout waiting for sleep status request. Waiting for new sleep service discovery message");
                        }
                        break;
                    }
                    msg = inetHandler->getFromMonitoringQueue();
                    if(msg && msg->ip == inetHandler->getManagerIp())
                    {
                        switch (msg->type) {
                            case Type::SleepStatusRequest:
                            {
                                if(status == ServiceGlobalStatus::Syncing) {
                                    inetHandler->changeStatus(ServiceGlobalStatus::Synchronized);
                                    log->info("Participant has joined the group managed by IP={} MAC={}",
                                              msg->ip, msg->mac);
                                }
                                Message answer = getSleepStatusRequest(2);
                                inetHandler->send(answer, msg->ip);
                                timestamp = std::time(nullptr); // reset timer
                                // std::cout << "Got sleep status request. " << std::endl;
                                break;
                            }
                            case Type::TableUpdate: // isso vai no participant
                            {
                                if (status == Synchronized) {
                                    TableUpdateHeader header{};
                                    size_t offset = 0;
                                    char tmstmp[80];
                                    char hostname[150];
                                    char ip[150];
                                    char mac[17];

                                    header = *(reinterpret_cast<TableUpdateHeader*>(msg->data));
                                    offset += sizeof(TableUpdateHeader);
                                    log->info("Received TableUpdate [ seq_no={} no_entries={} ]", msg->msgSeqNum, (int)header.noEntries);

                                    std::vector<Table::Participant> table;
                                    for (int i=0; i < header.noEntries; i++) {
                                        bzero(tmstmp, WAKEONLAN_FIELD_TIMESTAMP_SIZE);
                                        bzero(hostname, WAKEONLAN_FIELD_HOSTNAME_SIZE);
                                        bzero(ip, WAKEONLAN_FIELD_IP_SIZE);
                                        bzero(mac, WAKEONLAN_FIELD_MAC_SIZE);

                                        strncpy(tmstmp, &msg->data[offset], WAKEONLAN_FIELD_TIMESTAMP_SIZE);
                                        offset += WAKEONLAN_FIELD_TIMESTAMP_SIZE;
                                        memcpy(hostname, &msg->data[offset], WAKEONLAN_FIELD_HOSTNAME_SIZE);
                                        offset += WAKEONLAN_FIELD_HOSTNAME_SIZE;
                                        strncpy(ip, &msg->data[offset], WAKEONLAN_FIELD_IP_SIZE);
                                        offset += WAKEONLAN_FIELD_IP_SIZE;
                                        strncpy(mac, &msg->data[offset], WAKEONLAN_FIELD_MAC_SIZE);
                                        offset += WAKEONLAN_FIELD_MAC_SIZE;

                                        char s;
                                        strncpy(&s, &msg->data[offset], WAKEONLAN_FIELD_STATUS_SIZE);
                                        uint8_t status = (int)s;
                                        offset += WAKEONLAN_FIELD_STATUS_SIZE;

                                        Table::Participant member;
                                        member.electedTimestamp = tmstmp;
                                        member.hostname = hostname;
                                        member.ip = ip;
                                        member.mac = mac;
                                        switch (status) {
                                            case 0:
                                                member.status = Table::ParticipantStatus::Awaken;
                                                break;
                                            case 1:
                                                member.status = Table::ParticipantStatus::Sleeping;
                                                break;
                                            case 2:
                                                member.status = Table::ParticipantStatus::Unknown;
                                                break;
                                            case 3:
                                                member.status = Table::ParticipantStatus::Manager;
                                                break;
                                            default:
                                                break;
                                        }
                                        table.push_back(member);
                                    }
                                    if (this->table.transaction(msg->msgSeqNum, table))
                                        log->info("Processed transaction {}", msg->msgSeqNum);
                                }
                            }
                            break;
                        }
                    }
                    break;
                default: // Unknown or WaitingForSync
                    break;
                }
            }
        });
    }

    Message MonitoringService::getSleepStatusRequest(int seq)
    {
        Config config = inetHandler->getDeviceConfig();
        Message message{};
        message.type = WakeOnLanImpl::Type::SleepStatusRequest;
        bzero(message.hostname, sizeof(message.hostname));
        bzero(message.ip, sizeof(message.ip));
        bzero(message.mac, sizeof(message.mac));
        strncpy(message.hostname, config.getHostname().c_str(), config.getHostname().size());
        strncpy(message.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
        strncpy(message.mac, config.getMacAddress().c_str(), config.getMacAddress().size());
        message.msgSeqNum = seq;

        return message;
    }

    void MonitoringService::stop() {
        log->info("Stop Monitoring service");
    }

    void MonitoringService::notifyRoleChange() {
        // stops active thread
        active = false;
        auto config = inetHandler->getDeviceConfig();
        switch (config.getHandlerType())
        {
        case HandlerType::Manager:
            runAsManager();
            break;
        case HandlerType::Participant:
            runAsParticipant();
        default:
            break;
        }
    }

} // namespace WakeOnLanImpl
