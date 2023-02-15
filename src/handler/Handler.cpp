#include <../src/handler/Handler.hpp>

namespace WakeOnLanImpl {
    Handler::~Handler() {
    }

    Handler::Handler() {}

    ManagerHandler::ManagerHandler(const Config &cfg, Table &t)
            : table(t),
              config(cfg) {
        networkHandler = std::make_shared<NetworkHandler>(4000, config);
        discoveryService = std::make_unique<DiscoveryService>(table, networkHandler);
        monitoringService = std::make_unique<MonitoringService>(table, networkHandler);
        interfaceService = std::make_unique<ManagerInterfaceService>(table, networkHandler);
    }

    ManagerHandler::~ManagerHandler() {}

    ParticipantHandler::ParticipantHandler(const Config &cfg, WakeOnLanImpl::Table &t)
            : table(t),
              config(cfg) {
        networkHandler = std::make_shared<NetworkHandler>(4000, config);
        discoveryService = std::make_unique<DiscoveryService>(table, networkHandler);
        monitoringService = std::make_unique<MonitoringService>(table, networkHandler);
        interfaceService = std::make_unique<ParticipantInterfaceService>(table, networkHandler);
    }

    ParticipantHandler::~ParticipantHandler() {}

    void Handler::run() {}

    void Handler::stop() {}

    void ManagerHandler::run() {
        interfaceService->run();
        discoveryService->run();
        monitoringService->run();
    }

    void ManagerHandler::stop() {
        networkHandler->stop();
        discoveryService->stop();
        monitoringService->stop();
        interfaceService->stop();
        sleep(1);
    }

    void ParticipantHandler::run() {
        interfaceService->run();
        discoveryService->run();
        monitoringService->run();
    }

    void ParticipantHandler::stop() {
        networkHandler->stop();
        discoveryService->stop();
        monitoringService->stop();
        interfaceService->stop();
        sleep(1);
    }
}