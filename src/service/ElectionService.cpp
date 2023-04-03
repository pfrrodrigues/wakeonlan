#include <../src/service/ElectionService.hpp>
#include <ctime>

namespace WakeOnLanImpl {
    #define WAKEONLAN_ELECTION_TIMEOUT 25 
    #define WAKEONLAN_ELECTION_ANSWER_TIMEOUT 5

    ElectionService::ElectionService(Table &t, std::shared_ptr<NetworkHandler> nh)
        : table(t),
        inetHandler(nh),
        active(false),
        ongoingElection(false),
        ongoingElectionAnswered(false),
        unreadElection(false),
        lastWin(0)
    {}

    ElectionService::~ElectionService() {
        if (t) {
            if (t->joinable())
                t->join();
        }
    }

    void ElectionService::run() {
        if (t)
            t->join();

        log = spdlog::get("wakeonlan-api");
        log->info("Start Election service");
        active = true;
        t = std::make_unique<std::thread>([this](){
            auto config = inetHandler->getDeviceConfig();
            Message *m;
            while (active)
            {
                // check message queue 
                m = inetHandler->getFromElectionQueue();
                if (m != nullptr)
                    switch (m->type)
                    {
                    case Type::ElectionServiceCoordinator:
                        ongoingElection = false;
                        inetHandler->setManagerIp(m->ip);
                        {
                            std::lock_guard<std::mutex> lk(newElectionMutex);
                            newElectionResult = HandlerType::Participant;
                            unreadElection = true;
                        }
                        log->info("Machine with IP {} is the new manager, election over.", m->ip);
                    break;
                    case Type::ElectionServiceElection:
                    {
                        log->info("Got election message from IP {}.", m->ip);
                        // answer election message  
                        Message answer{};
                        answer.type = WakeOnLanImpl::Type::ElectionServiceAnswer;
                        bzero(answer.hostname, sizeof(answer.hostname));
                        bzero(answer.ip, sizeof(answer.ip));
                        bzero(answer.mac, sizeof(answer.mac));
                        strncpy(answer.hostname, config.getHostname().c_str(), config.getHostname().size());
                        strncpy(answer.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
                        strncpy(answer.mac, config.getMacAddress().c_str(), config.getMacAddress().size());
                        inetHandler->send(answer, m->ip);
                        if(!ongoingElection)
                        {
                            startElection();
                        }
                    }
                    break;
                    case Type::ElectionServiceAnswer:
                        log->info("Got election answer from IP {}", m->ip);
                        ongoingElectionAnswered = true;
                    break;
                    default:
                        break;
                    }
                if (ongoingElection && std::time(0) > ongoingElectionStart + WAKEONLAN_ELECTION_TIMEOUT)
                {
                    log->info("Election timed-out");
                    ongoingElection = false;
                }
            }
        });
    }

    void ElectionService::stop() {
        active = false;
    }

    HandlerType ElectionService::getNewElectionResult() {
        if(unreadElection)
        {
            std::lock_guard<std::mutex> lk(newElectionMutex);
            unreadElection = false;
            return newElectionResult;
        }
        auto config = inetHandler->getDeviceConfig();
        return config.getHandlerType();
    }

    HandlerType ElectionService::startElection() {
        log->info("Starting new election.");
        ongoingElection = true;
        ongoingElectionAnswered = false;
        ongoingElectionStart = std::time(0);
        std::vector<Table::Participant> contenders = getContenders();
        if (sendElectionMsgs(contenders) == HandlerType::Manager) // got no answer, assuming you won 
        {
            announceVictory();
            return HandlerType::Manager;
        }
        return HandlerType::Participant;
    }

    bool ElectionService::isElectionOver() {
        return !ongoingElection;
    }
    
    HandlerType ElectionService::sendElectionMsgs(std::vector<Table::Participant> contenders)
    {
        if (contenders.empty()) // you won the election!
            return HandlerType::Manager;
        
        // send election messages to contenders
        Config config = inetHandler->getDeviceConfig();
        Message electionMsg{};
        electionMsg.type = WakeOnLanImpl::Type::ElectionServiceElection;
        bzero(electionMsg.hostname, sizeof(electionMsg.hostname));
        bzero(electionMsg.ip, sizeof(electionMsg.ip));
        bzero(electionMsg.mac, sizeof(electionMsg.mac));
        strncpy(electionMsg.hostname, config.getHostname().c_str(), config.getHostname().size());
        strncpy(electionMsg.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
        strncpy(electionMsg.mac, config.getMacAddress().c_str(), config.getMacAddress().size());
        for (auto contender: contenders)
            inetHandler->send(electionMsg, contender.ip);

        // wait N seconds 
        time_t timer = std::time(0);
        while(std::time(0) - timer <= WAKEONLAN_ELECTION_ANSWER_TIMEOUT) {
            if(ongoingElectionAnswered)
                return HandlerType::Participant;
        }
        // if no answer was received, you won!
        if(ongoingElectionAnswered)
            return HandlerType::Participant;
        
        log->info("No answer to election messages");
        return HandlerType::Manager;
    }

    void ElectionService::sendCoordinatorMsgs() 
    {
        // could be substituted by multicast logic later
        Config config = inetHandler->getDeviceConfig();
        std::vector<Table::Participant> participants = table.get_participants_monitoring();
        Message coordinatorMsg{};
        coordinatorMsg.type = WakeOnLanImpl::Type::ElectionServiceCoordinator;
        bzero(coordinatorMsg.hostname, sizeof(coordinatorMsg.hostname));
        bzero(coordinatorMsg.ip, sizeof(coordinatorMsg.ip));
        bzero(coordinatorMsg.mac, sizeof(coordinatorMsg.mac));
        strncpy(coordinatorMsg.hostname, config.getHostname().c_str(), config.getHostname().size());
        strncpy(coordinatorMsg.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
        strncpy(coordinatorMsg.mac, config.getMacAddress().c_str(), config.getMacAddress().size());
        for (auto participant : participants)
        {
            if(participant.ip != config.getIpAddress())
                inetHandler->send(coordinatorMsg, participant.ip);
        }
        ongoingElection = false;
    }

    void ElectionService::announceVictory()
    {
        log->info("Election won. Sending coordinator messages to all on group.");
        ongoingElection = false;
        inetHandler->changeStatus(ServiceGlobalStatus::Synchronized);
        
        std::vector<Table::Participant> participants = table.get_participants_monitoring();
        auto config = inetHandler->getDeviceConfig();
        // if no one else is in the service, adds self
        auto mtLastWin  = std::chrono::system_clock::now();
        lastWin = std::chrono::system_clock::to_time_t(mtLastWin);
        if (participants.empty()) { // no one else is in the service --> first entity to join
            // add self to table as manager 
            Table::Participant self;
            self.hostname = config.getHostname();
            self.ip = config.getIpAddress();
            self.mac = config.getMacAddress();
            self.status = Table::ParticipantStatus::Manager; // first entity on server will be manager
            auto mt  = std::chrono::system_clock::now();
            ::time_t t = std::chrono::system_clock::to_time_t(mt);
            struct tm * ti = ::gmtime(&t);
            char buffer[80];
            ::strftime(buffer, 80, "%d-%m-%Y %H:%M:%S", ti);
            self.electedTimestamp = buffer;
            table.insert(self);
            return; 
        }
        // if there are more participants in the service 
        //     send coordinator message to everyone in the table
        sendCoordinatorMsgs();

        for(auto p : participants)
        {
            // make sure that you are in the table as manager
            if(p.mac == config.getMacAddress())  // using Mac address as unique identifier
            {
                auto ret = table.update(Table::ParticipantStatus::Manager, config.getHostname());
                if(ret.first)
                {
                    inetHandler->multicast(ret.second, ret.first);
                }
            }
            // make sure that everyone else is in the table a participant
            else {
                if(p.status == Table::ParticipantStatus::Manager)
                {
                    // changing status to unknown because you can't know if you had to managers or
                    // if you are stepping up to substitute a fallen manager -- MonitoringService will decide
                    auto ret = table.update(Table::ParticipantStatus::Unknown, p.hostname);
                    if(ret.first)
                    {
                        inetHandler->multicast(ret.second, ret.first);
                    }
                }
            }
        }
    }

    std::vector<Table::Participant> ElectionService::getContenders()
    {
        // TODO: add lastWin field to table and use it in the comparison, temporarly using only MAC address
        std::vector<Table::Participant> participants = table.get_participants_monitoring();
        auto self = inetHandler->getDeviceConfig();
        std::vector<Table::Participant> possibleWinners;
        int result;
        for(auto p : participants) {
            // struct std::tm tm{};
            // time_t t;
            // log->info("Participant timestamp: {}", p.electedTimestamp);
            // if(p.electedTimestamp == "N/A")
            // {
            //     t = 0;
            // }
            // else 
            // {
            //     strptime(p.electedTimestamp.c_str(), "%d-%m-%Y %H:%M:%S", &tm);
            //     auto electedTime = tm;

            //     t = ::mktime(&electedTime);
            // }
            // if(lastWin < t && p.ip != self.getIpAddress())
            // {
            //     log->info("Participant {} has bigger timestamp", p.hostname);
            //     possibleWinners.push_back(p);
            // }
            // else if(lastWin == t)
            // {
                result = self.getIpAddress().compare(p.ip);
                // if (result == 0), we have a draw, as MAC address is unique, assuming it's self
                // if (result <  0), compared mac address is shorter and will not be included
                log->info("Comparing your IP {} to opponents IP {}. Result was {}.", self.getIpAddress(), p.ip, result);
                if (result < 0)
                    possibleWinners.push_back(p);   
            // }             
        }
        return possibleWinners;
    }


} // namespace WakeOnLanImpl