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
                }

                if (inetHandler->getGlobalStatus() == Synchronized) {
                    /* Sent a broadcast packet each 10 seconds */
                    if (lastTimestamp < (std::time(nullptr) - WAKEONLAN_DISCOVERY_REQUEST_WINDOW)) {

                        Message broadcastMsg{};
//                        Message msg{};
//                        msg.type = Type::TableUpdate;
//                        msg.msgSeqNum = 1;
//                        bzero(msg.hostname, sizeof(msg.hostname));
//                        bzero(msg.ip, sizeof(msg.ip));
//                        bzero(msg.mac, sizeof(msg.mac));
//                        strncpy(msg.hostname, config.getHostname().c_str(), config.getHostname().size());
//                        strncpy(msg.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
//                        strncpy(msg.mac, config.getMacAddress().c_str(), config.getMacAddress().size());
//                        msg.reserved = true;
//
//                        /* se coloca na tabela */
//                        auto mt  = std::chrono::system_clock::now();
//                        ::time_t tick = std::chrono::system_clock::to_time_t(mt);
//                        struct tm * tick_time = ::gmtime(&tick);
//                        char buffer[80];
//                        Table::Participant p;
//                        ::strftime(buffer, 80, "%d-%m-%Y %H:%M:%S", tick_time);
//                        p.electedTimestamp = buffer;
//                        p.ip = config.getIpAddress();
//                        p.mac = config.getMacAddress();
//                        p.hostname = config.getHostname();
//                        p.status = Table::ParticipantStatus::Manager;
//
//                        /* Colocar participantes no buffer */
//                        // header
//                        // timestamp(80) | hostname (150) | ip(150) | mac(17) | status(1)
////                        msg.data = (char*) malloc(sizeof(TableUpdateHeader) + 80 + 150 + 150 + 17 + 1);
//                        size_t offset = 0;
//                        // header
//                        uint32_t seq = 1;
//                        uint8_t noEntries = 2;
//                        char electedTimestamp[80];
//                        char hostname[150];
//                        char ip[150];
//                        char mac[17];
//
////                        msg.data = (char*) malloc(sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1));
//                        msg.data = new char[sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1)];
//                        bzero(msg.data, 403+sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1));
//
//                        memcpy(&msg.data[offset], &seq, sizeof(seq));
//                        offset += sizeof(seq);
//
//                        memcpy(&msg.data[offset], &noEntries, sizeof(noEntries));
//                        offset += sizeof(noEntries);
//
//                        // messages
//                        for (int i=0; i<noEntries; i++) {
//                            bzero(electedTimestamp, 80);
//                            bzero(hostname, 150);
//                            bzero(ip, 150);
//                            bzero(mac, 17);
//                            if (i == 0) {
//                                memcpy(electedTimestamp, p.electedTimestamp.c_str(), p.electedTimestamp.size());
//                                memcpy(hostname, p.hostname.c_str(), p.hostname.size());
//                                memcpy(ip, p.ip.c_str(), p.ip.size());
//                                memcpy(mac, p.mac.c_str(), p.mac.size());
//                            }
//                            else {
//                                strncpy(electedTimestamp, (char*)"N/A", 3);
//                                strncpy(hostname, (char*)"participant@hostname", 20);
//                                strncpy(ip, (char*)"127.0.0.1", 9);
//                                strncpy(mac, (char*)"11:22:33:44:55:66", 17);
//                            }
//
//                            memcpy(&msg.data[offset], electedTimestamp, 80);
//                            offset += 80;
//
//                            memcpy(&msg.data[offset], hostname, 150);
//                            offset += 150;
//
//                            memcpy(&msg.data[offset], ip, 150);
//                            offset += 150;
//
//                            memcpy(&msg.data[offset], mac, 17);
//                            offset += 17;
//
//                            uint8_t status;
//                            if (i==2)
//                                std::cout << "";
//                            if (i==0)
//                             status = static_cast<uint8_t>(p.status);
//                            else
//                                status = static_cast<uint8_t>(Table::ParticipantStatus::Awaken);
//                            memcpy(&msg.data[offset], &status, 1);
//                            offset += 1;
//                        }
//
//                        char buff[326 + sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1)];
//                        if (msg.reserved) {
//                            memcpy(buff, &msg, 326);
//                            memcpy(&buff[326], msg.data, sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1));
//
//                            inetHandler->send(buff, "127.0.0.1", 326 + sizeof(TableUpdateHeader) + noEntries * (80 + 150 + 150 + 17 + 1));
//                        }
//                        else {
//
//                            inetHandler->send(msg, WAKEONLAN_BROADCAST_ADDRESS);
//                        }
//                      delete [] msg.data;
//

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

                                    if (table.insert(p)) {
                                        log->info("Participant has joined the group [Hostname={}, IP={}, MAC={}]",
                                                  m->hostname, m->ip,m->mac);

                                        /* Multicast group */
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
                                        char hostname[150];
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
                                            bzero(hostname, 150);
                                            bzero(ip, 150);
                                            bzero(mac, 17);

                                            memcpy(electedTimestamp, member.electedTimestamp.c_str(), member.electedTimestamp.size());
                                            memcpy(hostname, member.hostname.c_str(), member.hostname.size());
                                            memcpy(ip, member.ip.c_str(), member.ip.size());
                                            memcpy(mac, member.mac.c_str(), member.mac.size());


                                            memcpy(&msg.data[offset], electedTimestamp, 80);
                                            offset += 80;

                                            memcpy(&msg.data[offset], hostname, 150);
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