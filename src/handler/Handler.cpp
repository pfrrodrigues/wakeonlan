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
}

ManagerHandler::~ManagerHandler() {
}

ParticipantHandler::ParticipantHandler(const Config &cfg, WakeOnLanImpl::Table &t)
    : table(t),
      config(cfg) {
        networkHandler = std::make_shared<WakeOnLanImpl::NetworkHandler>(4000, config);
        discoveryService = std::make_unique<WakeOnLanImpl::DiscoveryService>(table, networkHandler);
        monitoringService = std::make_unique<WakeOnLanImpl::MonitoringService>(table, networkHandler);
    }

ParticipantHandler::~ParticipantHandler() {
}

void Handler::run() {}

void ManagerHandler::run() {
    discoveryService->run();
    sleep(1);
    monitoringService->run();
}

void ParticipantHandler::run() {
    discoveryService->run();
    monitoringService->run();
}