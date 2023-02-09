#include <../src/handler/Handler.hpp>

Handler::~Handler() {
}

Handler::Handler() {}

ManagerHandler::ManagerHandler(const Config &cfg, WakeOnLanImpl::Table &t)
    : table(t),
      config(cfg) {
    networkHandler = std::make_shared<WakeOnLanImpl::NetworkHandler>(4000, config);
    discoveryService = std::make_unique<WakeOnLanImpl::DiscoveryService>(table, networkHandler);
    interfaceService = std::make_unique<WakeOnLanImpl::ManagerInterfaceService>(table, networkHandler);
}

ManagerHandler::~ManagerHandler() {
}

ParticipantHandler::ParticipantHandler(const Config &cfg, WakeOnLanImpl::Table &t)
    : table(t),
      config(cfg) {
        networkHandler = std::make_shared<WakeOnLanImpl::NetworkHandler>(4000, config);
        discoveryService = std::make_unique<WakeOnLanImpl::DiscoveryService>(table, networkHandler);
        interfaceService = std::make_unique<WakeOnLanImpl::ParticipantInterfaceService>(table, networkHandler);
    }

ParticipantHandler::~ParticipantHandler() {
}

void Handler::run() {}

void ManagerHandler::run() {
    discoveryService->run();
}

void ParticipantHandler::run() {
    discoveryService->run();
}