#pragma once
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <spdlog/spdlog.h>
#include <tabulate/table.hpp>
#include <pthread.h>

namespace WakeOnLanImpl {
    /**
     * @class InterfaceService 
     * ...
     */
    class InterfaceService
    {
    public:
        InterfaceService(Table &table, std::shared_ptr<NetworkHandler> networkHandler);
        ~InterfaceService() = default;

        virtual void run();
        void stop();

    protected:
        std::vector<std::string> splitCmd(std::string cmd);

        std::shared_ptr<spdlog::logger> log;
        std::vector<pthread_t> threads;
        Table &participantTable;
        std::shared_ptr<NetworkHandler> networkHandler;
        bool keepRunning;
    };

    class ManagerInterfaceService : public InterfaceService
    {
        #define DISPLAY_TABLE_REFRESH_RATE 5 // seconds 
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

        tabulate::Table initializeDisplayTable();
        void runDisplayTable();
        static void * startDisplayTable(void * param);
        
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
    private:
        static void * startCommandListener(void * param);
        void runCommandListener();
        std::string parseInput(std::string cmd);
        std::string processExitCmd();

        bool isConnected();
    };

} // namespace WakeOnLanImpl