#pragma once
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <spdlog/spdlog.h>
#include <pthread.h>

namespace WakeOnLanImpl {
    /**
     * @class InterfaceService
     * Implements the Interface service.
     */
    class InterfaceService
    {
    public:
        /**
         * InterfaceService constructor.
         * @param table The singleton table used to store the manager information or the participants information.
         * @param inetHandler The unique Network handler.
         */
        InterfaceService(Table &table, std::shared_ptr<NetworkHandler> inetHandler);

        /**
         * Interface destructor.
         */
        ~InterfaceService() = default;
        /**
        * Runs the Interface service.
        */
        void run();

        /**
         * Stops the Interface service.
         */
        void stop();

        /**
         * Indicate to the service that there has been a change in it's role 
         */
        void notifyRoleChange();
    private:
        /**
         * Splits the user command.
         * @param cmd The user command.
         * @return A vector containing the parsed strings.
         */
        std::vector<std::string> splitCmd(std::string cmd);

        static void * startCommandListener(void * param);

        void runCommandListener();

        std::string parseInputManager(std::string cmd);
        std::string parseInputParticipant(std::string cmd);

        void initializeDisplayTable();
        void runDisplayTable();

        static void * startDisplayTable(void * param);
        
        std::string processExitCmd();
        void sendExitMsg();
        std::string processWakeupCmd(std::string hostname);

        std::vector<Table::Participant> lastSyncParticipants;
        int numParticipants;
        Table::Participant lastSyncManager;
        std::shared_ptr<spdlog::logger> log;            ///< The InterfaceService logger.
        std::vector<pthread_t> threads;                 ///< The vector of the dedicated threads.
        Table &participantTable;                        ///< The singleton table.
        std::shared_ptr<NetworkHandler> inetHandler; ///< The unique network handler.
        bool keepRunning;                               /// Indicates the services must keep running.
    };

} // namespace WakeOnLanImpl