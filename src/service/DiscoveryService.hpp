#pragma once
#include <memory>
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <Types.hpp>

namespace WakeOnLanImpl {
    class DiscoveryService {
    public:
        DiscoveryService(Table &table, std::shared_ptr<NetworkHandler> networkHandler);

        ~DiscoveryService();

        void run();

        Table &table;
        std::shared_ptr<NetworkHandler> inetHandler;
    private:
        void runAsManager();

        void runAsParticipant();

        std::unique_ptr<std::thread> t;
        bool active;
    };
}