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

        active = true;
        this->log = spdlog::get("wakeonlan-api");

        t = std::make_unique<std::thread>([this]() {
            log->info("Start Monitoring service");
            time_t timestamp;
            bool timerSet;
            int seq = 0;
            bool updated = false;
            std::vector<std::string> sleeping_participants;
            Message *msg;
            auto config = inetHandler->getDeviceConfig();
            while (active)
            {
                if(timestamp + 8 < std::time(0) || !timerSet) // every 8s
                {
                    // so it doesn't have to wait 8s for before the first call
                    if(!timerSet)    
                        timerSet = true;

                    // participants still in sleeping_participants haven't answered and 
                    // the last call are considered to be Sleeping
                    for(auto& hostname : sleeping_participants) {
                        table.update(Table::ParticipantStatus::Sleeping, hostname);
                        // TODO: send multicast aqui
                        /**
                         * MULTICAST
                         */
                        auto group = table.get_participants_monitoring();
                        auto seq_no = table.seq;
                        Message msg{};
                        msg.type = Type::TableUpdate;
                        msg.msgSeqNum = 1;
                        bzero(msg.hostname, sizeof(msg.hostname));
                        bzero(msg.ip, sizeof(msg.ip));
                        bzero(msg.mac, sizeof(msg.mac));
                        strncpy(msg.hostname, config.getHostname().c_str(), config.getHostname().size());
                        strncpy(msg.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
                        strncpy(msg.mac, config.getMacAddress().c_str(), config.getMacAddress().size());
                        msg.reserved = true;

                        // allocates data pointer and inserts participants information on the message
                        // TableHeader info
                        size_t offset = 0;
                        uint32_t seq = seq_no;
                        uint8_t noEntries = group.size();
                        char electedTimestamp[80];
                        char host[150];
                        char ip[150];
                        char mac[17];

                        msg.data = new char[sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1)];
                        bzero(msg.data, sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1));

                        memcpy(&msg.data[offset], &seq, sizeof(seq));
                        offset += sizeof(seq);

                        memcpy(&msg.data[offset], &noEntries, sizeof(noEntries));
                        offset += sizeof(noEntries);

                        // messages
                        for (auto & member : group) {
                            bzero(electedTimestamp, 80);
                            bzero(host, 150);
                            bzero(ip, 150);
                            bzero(mac, 17);

                            memcpy(electedTimestamp, member.electedTimestamp.c_str(), member.electedTimestamp.size());
                            memcpy(host, member.hostname.c_str(), member.hostname.size());
                            memcpy(ip, member.ip.c_str(), member.ip.size());
                            memcpy(mac, member.mac.c_str(), member.mac.size());


                            memcpy(&msg.data[offset], electedTimestamp, 80);
                            offset += 80;

                            memcpy(&msg.data[offset], host, 150);
                            offset += 150;

                            memcpy(&msg.data[offset], ip, 150);
                            offset += 150;

                            memcpy(&msg.data[offset], mac, 17);
                            offset += 17;

                            auto status = static_cast<uint8_t>(member.status);
                            memcpy(&msg.data[offset], &status, 1);
                            offset += 1;
                        }

                        if (msg.reserved) {
                            char buff[326 + sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1)];
                            memcpy(buff, &msg, 326);
                            memcpy(&buff[326], msg.data, sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1));

                            for (auto & member : group) {
                                if (member.status != Table::ParticipantStatus::Manager
                                    || member.status != Table::ParticipantStatus::Unknown)
                                    inetHandler->send(buff, member.ip, 326 + sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1));
                            }
                        }
                        delete[] msg.data;
                    }

                    // resets vectors
                    sleeping_participants.clear();
                    for(auto& participant: table.get_participants_monitoring())
                    {
                        // sleeping_participants is initialized with every participant
                        // currently in the table
                        if (participant.status != Table::ParticipantStatus::Manager) {
                            sleeping_participants.push_back(participant.hostname);
                            Message message = getSleepStatusRequest(seq);
                            inetHandler->send(message, participant.ip);
                        }

                        // send sleep status request 
                        // std::cout << "Manager is sending sleep status request to "
                        //           << participant.hostname << " @ " 
                        //           << participant.ip << std::endl;
                    }
                    seq++;
                    timestamp = std::time(nullptr); // reset timer
                }
                msg = inetHandler->getFromMonitoringQueue();
                if(msg) // if there is an answer from a participant 
                {
                    switch (msg->type) {
                        case Type::SleepStatusRequest:
                        {
                            // erase participant from sleeping_participants and puts
                            // it in awaken_participants
                            std::string hostname = msg->hostname;
                            table.update(Table::ParticipantStatus::Awaken, msg->hostname);
                            sleeping_participants.erase(std::find(sleeping_participants.begin(),
                                                                  sleeping_participants.end(),
                                                                  hostname));

                            // TODO: send multicast aqui
                            /**
                             * MULTICAST
                             */
                            auto group = table.get_participants_monitoring();
                            auto seq_no = table.seq;
                            Message multicastMsg{};
                            multicastMsg.type = Type::TableUpdate;
                            multicastMsg.msgSeqNum = 1;
                            bzero(multicastMsg.hostname, sizeof(multicastMsg.hostname));
                            bzero(multicastMsg.ip, sizeof(multicastMsg.ip));
                            bzero(multicastMsg.mac, sizeof(multicastMsg.mac));
                            strncpy(multicastMsg.hostname, config.getHostname().c_str(), config.getHostname().size());
                            strncpy(multicastMsg.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
                            strncpy(multicastMsg.mac, config.getMacAddress().c_str(), config.getMacAddress().size());
                            multicastMsg.reserved = true;

                            // allocates data pointer and inserts participants information on the message
                            // TableHeader info
                            size_t offset = 0;
                            uint32_t seq = seq_no;
                            uint8_t noEntries = group.size();
                            char electedTimestamp[80];
                            char host[150];
                            char ip[150];
                            char mac[17];

                            multicastMsg.data = new char[sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1)];
                            bzero(multicastMsg.data, sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1));

                            memcpy(&multicastMsg.data[offset], &seq, sizeof(seq));
                            offset += sizeof(seq);

                            memcpy(&multicastMsg.data[offset], &noEntries, sizeof(noEntries));
                            offset += sizeof(noEntries);

                            // messages
                            for (auto & member : group) {
                                bzero(electedTimestamp, 80);
                                bzero(host, 150);
                                bzero(ip, 150);
                                bzero(mac, 17);

                                memcpy(electedTimestamp, member.electedTimestamp.c_str(), member.electedTimestamp.size());
                                memcpy(host, member.hostname.c_str(), member.hostname.size());
                                memcpy(ip, member.ip.c_str(), member.ip.size());
                                memcpy(mac, member.mac.c_str(), member.mac.size());


                                memcpy(&multicastMsg.data[offset], electedTimestamp, 80);
                                offset += 80;

                                memcpy(&multicastMsg.data[offset], host, 150);
                                offset += 150;

                                memcpy(&multicastMsg.data[offset], ip, 150);
                                offset += 150;

                                memcpy(&multicastMsg.data[offset], mac, 17);
                                offset += 17;

                                auto status = static_cast<uint8_t>(member.status);
                                memcpy(&multicastMsg.data[offset], &status, 1);
                                offset += 1;
                            }

                            if (multicastMsg.reserved) {
                                char buff[326 + sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1)];
                                memcpy(buff, &multicastMsg, 326);
                                memcpy(&buff[326], multicastMsg.data, sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1));

                                for (auto & member : group) {
                                    if (member.status != Table::ParticipantStatus::Manager
                                        || member.status != Table::ParticipantStatus::Unknown)
                                        inetHandler->send(buff, member.ip, 326 + sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1));
                                }
                            }
                            delete[] multicastMsg.data;

                            // std::cout << "Manager got answer from "
                            //           << msg->hostname << " @ "
                            //           << msg->ip << std::endl;
                        }
                            break;
                        case Type::TableUpdate:
                            TableUpdateHeader header{};
                            size_t offset = 0;
                            char tmstmp[80];
                            char hostname[150];
                            char ip[150];
                            char mac[17];

                            header = *(reinterpret_cast<TableUpdateHeader*>(msg->data));
                            std::cout << "header { seq_no=" << header.seq << " no_entries=" << (int)header.noEntries << " }\n";
                            offset += sizeof(TableUpdateHeader);

                            log->info("Received TableUpdate [ seq_no={} no_entries={} ]", header.seq, (int)header.noEntries);

                            std::vector<Table::Participant> updatedTable;
                            for (int i=0; i < header.noEntries; i++) {
                                bzero(tmstmp, 80);
                                bzero(hostname, 150);
                                bzero(ip, 150);
                                bzero(mac, 17);

                                strncpy(tmstmp, &msg->data[offset], 80);
                                offset += 80;

                                memcpy(hostname, &msg->data[offset], 150);
                                offset += 150;

                                strncpy(ip, &msg->data[offset], 150);
                                offset += 150;

                                strncpy(mac, &msg->data[offset], 17);
                                offset += 17;

                                char s;
                                strncpy(&s, &msg->data[offset], 1);
                                uint8_t status = (int)s;
                                offset += 1;

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
                                updatedTable.push_back(member);
                            }

                            if (table.transaction(header.seq, updatedTable))
                                log->info("Processed transaction {}", header.seq);

                            delete [] msg->data;
                            break;
                    }
                }
            }
        });
    }

    void MonitoringService::runAsParticipant()
    {
        if (t)
            t->join();

        active = true;
        this->log = spdlog::get("wakeonlan-api");

        t = std::make_unique<std::thread>([this]() {
            log->info("Start Monitoring service");
            ServiceGlobalStatus status;
            Message *msg;
            time_t timestamp;
            bool timerSet = false;
            int seq;
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
                    if(timestamp + 35 < std::time(nullptr) && status == ServiceGlobalStatus::Syncing)
                    {
                        inetHandler->changeStatus(ServiceGlobalStatus::WaitingForSync);
                        timerSet = false;
                        // std::cout << "Timed out waiting for sleep status request, waiting new discovery msg" << std::endl;
                        log->info("Participant timedout waiting for sleep status request. Waiting for new sleep service discovery message");
                        break;
                    }
                    msg = inetHandler->getFromMonitoringQueue();
                    if(msg)
                    {
                        if(status == ServiceGlobalStatus::Syncing) {
                            inetHandler->changeStatus(ServiceGlobalStatus::Synchronized);
                            log->info("Participant has joined the group managed by IP={} MAC={}",
                                      msg->ip, msg->mac);
                        }
                        seq = msg->msgSeqNum;
                        Message answer = getSleepStatusRequest(seq);
                        inetHandler->send(answer, msg->ip);
                        timestamp = std::time(nullptr); // reset timer
                        // std::cout << "Got sleep status request. " << std::endl;
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


} // namespace WakeOnLanImpl
