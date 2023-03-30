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

        active = true;
        t = std::make_unique<std::thread>([this]() {
            time_t timestamp;
            bool timerSet;
            int seq = 0;
            bool updated = false;
            std::vector<std::string> sleeping_participants;
            Message *msg;
            while (active)
            {
                if(timestamp + 8 < std::time(0) || !timerSet) // every 8s
                {
                    // so it doesn't have to wait 8s for before the first call
                    if(!timerSet)    
                        timerSet = true;

                    // participants still in sleeping_participants haven't answered and 
                    // the last call are considered to be Sleeping
                    for(auto& hostname : sleeping_participants)
                        table.update(Table::ParticipantStatus::Sleeping, hostname);
                    
                    // resets vectors
                    sleeping_participants.clear();
                    for(auto& participant: table.get_participants_monitoring())
                    {
                        // sleeping_participants is initialized with every participant
                        // currently in the table
                        if (participant.status != Table::ParticipantStatus::Manager)
                            sleeping_participants.push_back(participant.hostname);

                        // send sleep status request 
                        // std::cout << "Manager is sending sleep status request to "
                        //           << participant.hostname << " @ " 
                        //           << participant.ip << std::endl; 
                        Message message = getSleepStatusRequest(seq);
                        if (participant.status != Table::ParticipantStatus::Manager)
                            inetHandler->send(message, participant.ip);
                    }
                    seq++;
                    timestamp = std::time(nullptr); // reset timer
                }
                msg = inetHandler->getFromMonitoringQueue();
                if(msg) // if there is an answer from a participant 
                {
                    // erase participant from sleeping_participants and puts 
                    // it in awaken_participants
                    std::string hostname = msg->hostname;
                    table.update(Table::ParticipantStatus::Awaken, msg->hostname);
                    sleeping_participants.erase(std::find(sleeping_participants.begin(), 
                                                          sleeping_participants.end(), 
                                                          hostname));
                    // std::cout << "Manager got answer from "
                    //           << msg->hostname << " @ "
                    //           << msg->ip << std::endl;
                }
            }
        });
    }

    void MonitoringService::runAsParticipant()
    {
        if (t)
            t->join();

        active = true;
        t = std::make_unique<std::thread>([this]() {
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
                    if(msg && msg->ip == inetHandler->getManagerIp())
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
