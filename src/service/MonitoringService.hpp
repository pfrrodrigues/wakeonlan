
#pragma once
#include <memory>
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <Types.hpp>

namespace WakeOnLanImpl {
    class MonitoringService
    {
    public:
        MonitoringService(Table &table, std::shared_ptr<NetworkHandler> networkHandler);

        ~MonitoringService();
        
        void run();
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

        Message getSleepStatusRequest(int seq);

        Table &table;
        std::shared_ptr<NetworkHandler> inetHandler;
        std::unique_ptr<std::thread> t;
        bool active;
        std::shared_ptr<spdlog::logger> log;
    };
    
} // namespace WakeOnLanImpl