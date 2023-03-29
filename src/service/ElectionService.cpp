#include <../src/service/ElectionService.hpp>
#include <ctime>

namespace WakeOnLanImpl {
    #define WAKEONLAN_ELECTION_TIMEOUT 5 

    ElectionService::ElectionService(Table &t, std::shared_ptr<NetworkHandler> nh)
        : table(t),
        inetHandler(nh),
        active(false),
        ongoingElection(false),
        lastWin(0)
    {}

    ElectionService::~ElectionService() {
        if (t) {
            if (t->joinable())
                t->join();
        }
    }

    void ElectionService::run() {
        // check message queue 
        // if coordinator message, there's a new manager, assuming you are Synchronized
        // if election message
        // if there's no ongoing election
            // answer it 
            // startElection();
        // if there is an ongoing election, ignore message
    }

    void ElectionService::stop() {
        active = false;
    }

    HandlerType ElectionService::startElection() {
        ongoingElection = true;
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
        // TODO

        // wait N seconds 
        time_t timer = std::time(0);
        while(std::time(0) - timer <= WAKEONLAN_ELECTION_TIMEOUT) {
            // if got an answer, you lost
            // TODO: check msg queue
            //       if got answer
            //       return HandlerType::Participant;
        }
        // if no answer was received, you won!
        return HandlerType::Manager;
    }

    void ElectionService::announceVictory()
    {
        ongoingElection = false;
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
            if (result > 0)
                possibleWinners.push_back(p);                
        }
        return possibleWinners;
    }


} // namespace WakeOnLanImpl