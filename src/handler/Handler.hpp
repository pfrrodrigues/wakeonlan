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
    /**
     * @class Handler
     * The API handler. Handler is a generic class inherited by ManagerHandler and ParticipantHandler.
     */
    class Handler {
    public:
        /**
         * Handler constructor.
         */
        Handler(const Config &config, Table& = Table::get());

        /**
         * Handler virtual destructor.
         */
        virtual ~Handler();

        /**
         * Runs the services supported by the handler.
         * @returns None.
         */
        void run();

        /**
         * Stops the services supported by the handler.
         * @returns None.
         */
        void stop();
    private:
        std::unique_ptr<DiscoveryService> discoveryService;             ///< The DiscoveryService instance.
        std::unique_ptr<MonitoringService> monitoringService;           ///< The MonitoringService instance.
        std::unique_ptr<InterfaceService> interfaceService;      ///< The InterfaceService instance.
        std::shared_ptr<NetworkHandler> networkHandler;                 ///< The Network handler unique instance.
        Table& table;                                                   ///< The singleton table.
        Config config;                                                  ///< The API configuration.
    };
} // namespace WakeOnLanImpl