
#pragma once
#include <memory>
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <Types.hpp>

namespace WakeOnLanImpl {
    /**
     * @class MonitoringService
     * Implements the Monitoring service.
     */
    class MonitoringService
    {
    public:
        /**
         * MonitoringService constructor.
         * @param table The singleton table used to store the manager information or the participants information.
         * @param networkHandler The unique Network handler.
         */
        MonitoringService(Table &table, std::shared_ptr<NetworkHandler> networkHandler);

        /**
         * MonitoringService destructor.
         */
        ~MonitoringService();

        /**
         * Runs the Monitoring service.
         */
        void run();

        /**
         * Runs the Monitoring service.
         */
        void stop();
    private:
        /**
         * For every entry in table, if participant is:
         * - WaitingForSync: does nothing (doesn't happen)
         * - Syncing: does nothing (doesn't happen)
         * - Unknown: does nothing (doesn't happen)
         * - Synchronized: sends sleep status request messages every 8s.
         * 
         */
        void runAsManager();

        /**
         * If participant is:
         * - WaitingForSync: does nothing
         * - Syncing: listens for sleep status request message for 35s.
         *   If a message is received, changes status to Synchronized and sends status.
         *   If no message is received, changes status to WaitingForSync          
         * - Synchronized: listens for sleep status request, answering with current status
         * - Unknown: does nothing
         * 
         */
        void runAsParticipant();

        /**
         * Gets the received SleepStatusRequest message.
         * @param seq
         * @return A message.
         */
        Message getSleepStatusRequest(int seq);

        Table &table;                                   ///< The singleton table.
        std::shared_ptr<NetworkHandler> inetHandler;    ///< A shared pointer to the unique Network handler.
        std::unique_ptr<std::thread> t;                 ///< The service dedicated thread.
        bool active;                                    ///< Indicates service is active or not.
        std::shared_ptr<spdlog::logger> log;            ///< The DiscoveryService logger.
    };
    
} // namespace WakeOnLanImpl