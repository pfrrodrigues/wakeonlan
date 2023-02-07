#pragma once
#include <../src/common/Table.hpp>
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
        InterfaceService(Table &table);
        ~InterfaceService() = default;

        void run();

    private:
        /**
         * Creates tabulate::Table object and adds headers 
         *
         * @return display table
         */
        tabulate::Table initialize_display_table();

        /**
         * Prints updated Table every REFRESH_RATE seconds 
         * 
         */
        void run_display_table();
        
        /**
         * Listens to user input  
         * 
         */
        void run_command_listener();

        /**
         * Starts thread that runs run_display_table()
         * 
         * @param param Reference to current object (e.g. this)
         * @return NULL
         */
        static void * start_display_table(void * param);
        
        /**
         * Starts thread that runs run_command_listener()
         * 
         * @param param Reference to current object (e.g. this)
         * @return NULL
         */
        static void * start_command_listener(void * param);
        
        std::string parse_input(std::string cmd);
        std::string process_wakeup_cmd(std::string hostname);
        std::string process_exit_cmd();

        std::shared_ptr<spdlog::logger> log;
        pthread_t threads[2];
        Table &participantTable;
        int numParticipants;
    };
    
    

} // namespace WakeOnLanImpl