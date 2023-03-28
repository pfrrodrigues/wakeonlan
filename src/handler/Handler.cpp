#include <../src/handler/Handler.hpp>

namespace WakeOnLanImpl {
    Handler::Handler(const Config &cfg, Table &t)
            : table(t),
              config(cfg) {
        networkHandler = std::make_shared<NetworkHandler>(4000, config);
        discoveryService = std::make_unique<DiscoveryService>(table, networkHandler);
        monitoringService = std::make_unique<MonitoringService>(table, networkHandler);
        interfaceService = std::make_unique<InterfaceService>(table, networkHandler);
        electionService = std::make_unique<ElectionService>(table, networkHandler);
    }

    Handler::~Handler() {}

    void Handler::run() {
        interfaceService->run();
        discoveryService->run();
        monitoringService->run();
        electionService->run();

        active = true;
        t = std::make_unique<std::thread>([this]() {
            HandlerType electionResult;
            auto config = networkHandler->getDeviceConfig();
            while(active){
                switch (networkHandler->getGlobalStatus())
                {
                case ManagerFailure:
                    // 1. run an election 
                    electionResult = electionService->startElection();
                    // 2. notify services if there is a status change
                    if (electionResult != config.getHandlerType())
                    {
                        config = networkHandler->changeHandlerType(electionResult);
                        networkHandler->changeStatus(ServiceGlobalStatus::Synchronized);

                        discoveryService->notifyRoleChange();
                        monitoringService->notifyRoleChange();
                        interfaceService->notifyRoleChange();
                    }

                    break;
                default:
                    break;
                }   
            }
        });

    }

    void Handler::stop() {
        active  = false;

        networkHandler->stop();
        discoveryService->stop();
        monitoringService->stop();
        interfaceService->stop();
        electionService->stop();
        sleep(1);
    }

}