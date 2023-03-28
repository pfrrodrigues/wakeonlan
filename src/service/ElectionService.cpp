#include <../src/service/ElectionService.hpp>
#include <ctime>

namespace WakeOnLanImpl {
    ElectionService::ElectionService(Table &t, std::shared_ptr<NetworkHandler> nh)
        : table(t),
        inetHandler(nh),
        active(false),
        lastWin(0)
    {}

    ElectionService::~ElectionService() {
        if (t) {
            if (t->joinable())
                t->join();
        }
    }

    void ElectionService::run() {

    }

    void ElectionService::stop() {
        active = false;
    }

    HandlerType ElectionService::startElection() {
        std::vector<Table::Participant> participants = table.get_participants_monitoring();
        auto config = inetHandler->getDeviceConfig();
        if (participants.empty()) { // no one else is in the service
            // add self to table as manager 
            Table::Participant self = {
                .hostname = config.getHostname(),
                .ip = config.getIpAddress(),
                .mac = config.getMacAddress(),
                .status = Table::ParticipantStatus::Manager // first entity on server will be manager
            };
            table.insert(self);
            lastWin = std::time(0);
            return HandlerType::Manager;
        }
        else {
            return HandlerType::Participant;
        }
    }

} // namespace WakeOnLanImpl