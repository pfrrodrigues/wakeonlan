#include <../src/handler/Handler.hpp>

Handler::~Handler() {
}

Handler::Handler() {}

ManagerHandler::ManagerHandler(const Config &cfg, WakeOnLanImpl::Table &t)
    : table(t),
      config(cfg) {
    networkHandler = std::make_shared<WakeOnLanImpl::NetworkHandler>(4000, config);
    discoveryService = std::make_unique<WakeOnLanImpl::DiscoveryService>(table, networkHandler);
    monitoringService = std::make_unique<WakeOnLanImpl::MonitoringService>(table, networkHandler);
    interfaceService = std::make_unique<WakeOnLanImpl::ManagerInterfaceService>(table, networkHandler);
}

ManagerHandler::~ManagerHandler() {
}

ParticipantHandler::ParticipantHandler(const Config &cfg, WakeOnLanImpl::Table &t)
    : table(t),
      config(cfg) {
        networkHandler = std::make_shared<WakeOnLanImpl::NetworkHandler>(4000, config);
        discoveryService = std::make_unique<WakeOnLanImpl::DiscoveryService>(table, networkHandler);
        monitoringService = std::make_unique<WakeOnLanImpl::MonitoringService>(table, networkHandler);
        interfaceService = std::make_unique<WakeOnLanImpl::ParticipantInterfaceService>(table, networkHandler);
    }

ParticipantHandler::~ParticipantHandler() {
}

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