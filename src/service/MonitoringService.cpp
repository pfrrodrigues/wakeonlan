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
        t = std::make_unique<std::thread>([this]() {
            time_t timestamp;
            bool timerSet;
            int seq = 0;
            bool updated = false;
            std::vector<std::string> sleeping_participants,
                                     awaken_participants;
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
                    
                    // participants in awaken_participants have answered the last call
                    // and are considered to be Awaken.
                    for(auto& hostname : awaken_participants)
                        table.update(Table::ParticipantStatus::Awaken, hostname);
                    
                    // resets vectors
                    sleeping_participants.clear();
                    awaken_participants.clear();
                    for(auto& participant: table.get_participants_monitoring())
                    {
                        // sleeping_participants is initialized with every participant
                        // currently in the table
                        sleeping_participants.push_back(participant.hostname);

                        // send sleep status request 
                        std::cout << "Manager is sending sleep status request to "
                                  << participant.hostname << " @ " 
                                  << participant.ip << std::endl; 
                        Message message = getSleepStatusRequest(seq);
                        inetHandler->send(message, participant.ip);
                    }
                    seq++;
                    timestamp = std::time(0); // reset timer
                }
                msg = inetHandler->getFromMonitoringQueue();
                if(msg) // if there is an answer from a participant 
                {
                    // erase participant from sleeping_participants and puts 
                    // it in awaken_participants
                    std::string hostname = msg->hostname;
                    awaken_participants.push_back(hostname);
                    sleeping_participants.erase(std::find(sleeping_participants.begin(), 
                                                          sleeping_participants.end(), 
                                                          hostname));
                    std::cout << "Manager got answer from "
                              << msg->hostname << " @ "
                              << msg->ip << std::endl;
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
            Message *msg;
            time_t timestamp = std::time(0);
            int seq;
            while(active)
            {
                switch (inetHandler->getGlobalStatus())
                {
                // has to wait 35s for a sleep status request message or else 
                // assumes it should wait a new discovery message
                case ServiceGlobalStatus::Syncing:
                    if(timestamp + 35 < std::time(0))
                    {
                        inetHandler->changeStatus(ServiceGlobalStatus::WaitingForSync);
                        std::cout << "Timed out waiting for sleep status request, waiting new discovery msg" << std::endl;
                        break;
                    }
                    msg = inetHandler->getFromMonitoringQueue();
                    if(msg)
                    {
                        seq = msg->msgSeqNum;
                        inetHandler->changeStatus(ServiceGlobalStatus::Synchronized);
                        Message answer = getSleepStatusRequest(seq);
                        inetHandler->send(answer, msg->ip);
                        std::cout << "Got sleep status request. "
                                  << "Going to Synced state" << std::endl;
                    }
                    break;
                case ServiceGlobalStatus::Synchronized:
                    // check msg and answer 
                    msg = inetHandler->getFromMonitoringQueue();
                    if(msg)
                    {
                        seq = msg->msgSeqNum;
                        Message answer = getSleepStatusRequest(seq);
                        inetHandler->send(answer, msg->ip);
                        std::cout << "Got sleep status request." << std::endl;
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


} // namespace WakeOnLanImpl