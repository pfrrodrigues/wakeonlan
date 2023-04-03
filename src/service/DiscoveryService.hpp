#pragma once
#include <memory>
#include <ctime>
#include <spdlog/spdlog.h>
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <Types.hpp>

namespace WakeOnLanImpl {
    /**
     * @class DiscoveryService
     * Implements the Discovery service.
     */
    class DiscoveryService {
    public:
        /**
         * DiscoveryService constructor.
         * @param table The singleton table used to store the manager information or the participants information.
         * @param networkHandler The unique Network handler.
         */
        DiscoveryService(Table &table, std::shared_ptr<NetworkHandler> networkHandler);

        /**
         * DiscoveryService destructor.
         */
        ~DiscoveryService();

        /**
         * Runs the Discovery service.
         */
        void run();

        /**
         * Stops the Discovery service.
         */
        void stop();

        /**
         * Indicate to the service that there has been a change in it's role 
         */
        void notifyRoleChange();
    private:
        /**
         * Runs the Discovery service associated to the Manager instance.
         * A dedicated thread is created for running the function logic. The
         * service stops when ::stop() is called.
         */
        void runAsManager();

        /**
         * Runs the Discovery service associated to the Participant instance.
         * A dedicated thread is creation for running the function logic. The
         * service stops when ::stop() is called.
         */
        void runAsParticipant();

        std::unique_ptr<std::thread> t;                 ///< The service dedicated thread.
        bool active;                                    ///< Indicates service is active or not.
        std::time_t lastTimestamp;                      ///< The last timestamp since a SleepServiceDiscovery request was sent by the Manager.
        std::shared_ptr<spdlog::logger> log;            ///< The DiscoveryService logger.
        Table &table;                                   ///< The singleton table.
        std::shared_ptr<NetworkHandler> inetHandler;    ///< A shared pointer to the unique Network handler.
    };
} // namespace WakeOnLanImpl