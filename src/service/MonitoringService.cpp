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
        seq = 0;
        t = std::make_unique<std::thread>([this]() {
            timestamp = std::time(0);
            bool updated = false;
            std::vector<std::string> sleeping_participants,
                                     awaken_participants;
            Message *msg;
            while (active)
            {
                // every 8s
                if(timestamp + 8 < std::time(0))
                {
                    for(auto& hostname : sleeping_participants)
                        table.update(Table::ParticipantStatus::Sleeping, hostname);
                    for(auto& hostname : awaken_participants)
                        table.update(Table::ParticipantStatus::Awaken, hostname);

                    sleeping_participants.clear();
                    awaken_participants.clear();
                    for(auto& participant: table.get_participants_monitoring())
                    {
                        sleeping_participants.push_back(participant.hostname);

                        // send sleep status request 
                        Message message = getSleepStatusRequest();
                        inetHandler->send(message, participant.ip);
                    }
                    seq++;
                    timestamp = std::time(0);
                }
                msg = inetHandler->getFromMonitoringQueue();
                if(msg)
                {
                    std::string hostname = msg->hostname;
                    awaken_participants.push_back(hostname);
                    sleeping_participants.erase(std::find(sleeping_participants.begin(), 
                                                          sleeping_participants.end(), 
                                                          hostname));
                }
            }
        });
    }

    void MonitoringService::runAsParticipant()
    {
        if (t)
            t->join();

        active = true;
        t = std::make_unique<std::thread>([this](bool timerSet) {
            Message *msg;
            bool timerSet = false;
            while(active)
            {
                switch (inetHandler->getGlobalStatus())
                {
                case ServiceGlobalStatus::Syncing:
                    if(!timerSet)
                    {
                        timestamp = std::time(0);
                        timerSet = true;
                    }
                    if(timestamp + 35 < std::time(0))
                    {
                        inetHandler->changeStatus(ServiceGlobalStatus::WaitingForSync);
                        break;
                    }
                    msg = inetHandler->getFromMonitoringQueue();
                    if(msg)
                    {
                        seq = msg->msgSeqNum;
                        inetHandler->changeStatus(ServiceGlobalStatus::Synchronized);
                        Message answer = getSleepStatusRequest();
                        inetHandler->send(answer, msg->ip);
                    }
                    break;
                case ServiceGlobalStatus::Synchronized:
                    // check msg and answer 
                    msg = inetHandler->getFromMonitoringQueue();
                    if(msg)
                    {
                        seq = msg->msgSeqNum;
                        Message answer = getSleepStatusRequest();
                        inetHandler->send(answer, msg->ip);
                    }
                    break;
                default: // Unknown or WaitingForSync
                    break;
                }
            }
        });
    }

    Message MonitoringService::getSleepStatusRequest()
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