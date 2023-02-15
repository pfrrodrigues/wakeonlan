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
         * @param networkHandler The unique Network handler.
         */
        InterfaceService(Table &table, std::shared_ptr<NetworkHandler> networkHandler);

        /**
         * Interface destructor.
         */
        ~InterfaceService() = default;
        /**
        * Runs the Interface service.
        */
        virtual void run();

        /**
         * Stops the Interface service.
         */
        void stop();

    protected:
        /**
         * Splits the user command.
         * @param cmd The user command.
         * @return A vector containing the parsed strings.
         */
        std::vector<std::string> splitCmd(std::string cmd);

        std::shared_ptr<spdlog::logger> log;            ///< The InterfaceService logger.
        std::vector<pthread_t> threads;                 ///< The vector of the dedicated threads.
        Table &participantTable;                        ///< The singleton table.
        std::shared_ptr<NetworkHandler> networkHandler; ///< The unique network handler.
        bool keepRunning;                               /// Indicates the services must keep running.
    };

    class ManagerInterfaceService : public InterfaceService
    {
        #define DISPLAY_TABLE_REFRESH_RATE 0 // seconds 
    public:
        ManagerInterfaceService(Table &table, std::shared_ptr<NetworkHandler> networkHandler)
            : InterfaceService(table, networkHandler) { }

        ~ManagerInterfaceService() = default;
    
        void run() override;
    private:
        static void * startCommandListener(void * param);

        void runCommandListener();

        std::string parseInput(std::string cmd);

        std::string processWakeupCmd(std::string hostname);


        void initializeDisplayTable();

        void runDisplayTable();

        static void * startDisplayTable(void * param);
        
        std::vector<Table::Participant> lastSyncParticipants;
        int numParticipants;
    };

    class ParticipantInterfaceService : public InterfaceService
    {
        #define CONNECTION_CHECK_RATE 1
    public:
        ParticipantInterfaceService(Table &table, std::shared_ptr<NetworkHandler> networkHandler)
            : InterfaceService(table, networkHandler) { }

        ~ParticipantInterfaceService() = default;
    
        void run() override;

        void stop();
    private:
        static void * startCommandListener(void * param);

        void runCommandListener();

        std::string parseInput(std::string cmd);

        std::string processExitCmd();

        void sendExitMsg();

        Table::Participant manager;
    };

} // namespace WakeOnLanImpl