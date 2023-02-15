#pragma once
#include <memory>
#include <iostream>
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <../src/service/DiscoveryService.hpp>
#include <../src/service/MonitoringService.hpp>
#include <../src/service/InterfaceService.hpp>

using namespace WakeOnLan;

namespace WakeOnLanImpl {
    class Handler {
    public:
        Handler();
        virtual ~Handler();
        virtual void run();
        virtual void stop();
    };

    class ManagerHandler : public Handler {
    public:
        void run() override;
        void stop() override;
        explicit ManagerHandler(const Config &config, Table& = Table::get());
        virtual ~ManagerHandler();
    private:
        std::unique_ptr<DiscoveryService> discoveryService;
        std::unique_ptr<MonitoringService> monitoringService;
        std::unique_ptr<ManagerInterfaceService> interfaceService;
        std::shared_ptr<NetworkHandler> networkHandler;
        Table& table;
        Config config;
    };

    class ParticipantHandler : public Handler {
    public:
        void run() override;
        void stop() override;
        explicit ParticipantHandler(const Config &config, Table& = Table::get());
        virtual ~ParticipantHandler();
    private:
        std::unique_ptr<DiscoveryService> discoveryService;
        std::unique_ptr<MonitoringService> monitoringService;
        std::unique_ptr<ParticipantInterfaceService> interfaceService;
        std::shared_ptr<NetworkHandler> networkHandler;
        Table& table;
        Config config;
    };
} // namespace WakeOnLanImpl