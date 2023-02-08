#pragma once
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <spdlog/spdlog.h>
#include <tabulate/table.hpp>
#include <pthread.h>

namespace WakeOnLanImpl {
    #define REFRESH_RATE 5 // seconds 
    /**
     * @class InterfaceService 
     * ...
     */
    class InterfaceService
    {
    public:
        InterfaceService(Table &table, std::shared_ptr<NetworkHandler> networkHandler);
        ~InterfaceService() = default;

        void run();

    private:
        /**
         * Creates tabulate::Table object and adds headers 
         * 
         * @return display table
         */
        tabulate::Table initializeDisplayTable();

        void runDisplayTable();
        
        void runCommandListener();

        /**
         * Starts thread that runs runDisplayTable()
         * 
         * @param param Reference to current object (e.g. this)
         * @return NULL
         */
        static void * startDisplayTable(void * param);
        
        /**
         * Starts thread that runs runCommandListener()
         * 
         * @param param Reference to current object (e.g. this)
         * @return NULL
         */
        static void * startCommandListener(void * param);
        
        std::string parseInput(std::string cmd);
        std::vector<std::string> splitCmd(std::string cmd);

        std::string processWakeupCmd(std::string hostname);
        std::string processExitCmd();

        std::shared_ptr<spdlog::logger> log;
        pthread_t threads[2];
        Table &participantTable;
        int numParticipants;
        std::shared_ptr<NetworkHandler> networkHandler;
    };
} // namespace WakeOnLanImpl