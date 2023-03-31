#include <../src/service/ElectionService.hpp>
#include <ctime>

namespace WakeOnLanImpl {
    #define WAKEONLAN_ELECTION_TIMEOUT 5 

    ElectionService::ElectionService(Table &t, std::shared_ptr<NetworkHandler> nh)
        : table(t),
        inetHandler(nh),
        active(false),
        ongoingElection(false),
        ongoingElectionAnswered(false),
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
                        log->info("Machine with IP {} is the new manager, election over.", m->ip);
                    break;
                    case Type::ElectionServiceElection:
                    {
                        if(!ongoingElection)
                        {
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
                            // send election messages to possible contenders/declare you won
                            startElection();
                        }
                        // if there is an ongoing election, ignore message
                        log->info("Got election message from IP {} but election is already happening.", m->ip);
                    }
                    break;
                    case Type::ElectionServiceAnswer:
                        log->info("Got answer from IP {}", m->ip);
                        ongoingElectionAnswered = true;
                    break;
                    default:
                        break;
                    }
            }
        });
    }

    void ElectionService::stop() {
        active = false;
    }

    HandlerType ElectionService::startElection() {
        log->info("Starting new election.");
        ongoingElection = true;
        ongoingElectionAnswered = false;
        std::vector<Table::Participant> contenders = getContenders();
        if (sendElectionMsgs(contenders) == HandlerType::Manager) // got no answer, assuming you won 
        {
            announceVictory();
            return HandlerType::Manager;
        }
        return HandlerType::Participant;
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
        while(std::time(0) - timer <= WAKEONLAN_ELECTION_TIMEOUT) {
            if(ongoingElectionAnswered)
                return HandlerType::Participant;
        }
        // if no answer was received, you won!
        if(ongoingElectionAnswered)
            return HandlerType::Participant;
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
        if (participants.empty()) { // no one else is in the service --> first entity to join
            // add self to table as manager 
            Table::Participant self = {
                .hostname = config.getHostname(),
                .ip = config.getIpAddress(),
                .mac = config.getMacAddress(),
                .status = Table::ParticipantStatus::Manager // first entity on server will be manager
                // TODO .lastWin = std::time(0); or something like that
            };
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
                table.update(Table::ParticipantStatus::Manager, config.getHostname());
            // make sure that everyone else is in the table a participant
            else {
                if(p.status == Table::ParticipantStatus::Manager)
                    // changing status to unknown because you can't know if you had to managers or
                    // if you are stepping up to substitute a fallen manager -- MonitoringService will decide
                    table.update(Table::ParticipantStatus::Unknown, p.hostname);
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
            result = self.getMacAddress().compare(p.mac);
            // if (result == 0), we have a draw, as MAC address is unique, assuming it's self
            // if (result <  0), compared mac address is shorter and will not be included
            log->info("Comparing your MAC {} to opponents MAC {}. Result was {}.", self.getMacAddress(), p.mac, result);
            if (result > 0)
                possibleWinners.push_back(p);                
        }
        return possibleWinners;
    }


} // namespace WakeOnLanImpl