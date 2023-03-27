#include <../src/handler/Handler.hpp>

namespace WakeOnLanImpl {
    Handler::Handler(const Config &cfg, Table &t)
            : table(t),
              config(cfg) {
        networkHandler = std::make_shared<NetworkHandler>(4000, config);
        discoveryService = std::make_unique<DiscoveryService>(table, networkHandler);
        monitoringService = std::make_unique<MonitoringService>(table, networkHandler);
        interfaceService = std::make_unique<InterfaceService>(table, networkHandler);
    }

    Handler::~Handler() {}

    void Handler::run() {
        interfaceService->run();
        discoveryService->run();
        monitoringService->run();
    }

    void Handler::stop() {
        networkHandler->stop();
        discoveryService->stop();
        monitoringService->stop();
        interfaceService->stop();
        sleep(1);
    }

}