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
        tabulate::Table initialize_display_table();
        void run_display_table();
        void run_command_listener();
        static void * start_display_table(void *);
        static void * start_command_listener(void *);

        std::shared_ptr<spdlog::logger> log;
        pthread_t threads[2];
        Table &participantTable;
    };
    
    

} // namespace WakeOnLanImpl