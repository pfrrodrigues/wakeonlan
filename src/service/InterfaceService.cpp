#include <../src/service/InterfaceService.hpp>
#include <iostream>
#include <unistd.h>

namespace WakeOnLanImpl {
    InterfaceService::InterfaceService(Table &table) : participantTable(table)
    {
    }

    void InterfaceService::run()
    {
        int ret;
        ret = pthread_create(&threads[0], NULL, &start_display_table, this);
        ret = pthread_create(&threads[1], NULL, &start_command_listener, this);

        pthread_join(threads[0], NULL);
        pthread_join(threads[1], NULL);
    }

    void * InterfaceService::start_display_table(void * param)
    {
        InterfaceService *obj = (InterfaceService *) param;
        obj->run_display_table();
        return NULL;
    }

    tabulate::Table InterfaceService::initialize_display_table()
    {
        tabulate::Table displayTable;
        // add header
        displayTable.add_row({"Hostname",
                              "IP Address",
                              "MAC Address",
                              "Status"});
        // format header
        for (auto& cell : displayTable[0])
            cell.format()
                    .font_background_color(tabulate::Color::white)
                    .font_color(tabulate::Color::white);
        return displayTable;
    }

    void InterfaceService::run_display_table()
    {
        tabulate::Table display;
        std::cout << "\033[2J"; // clears terminal and moves cursor to (0,0)
        while (true)
        {
            std::vector<Table::Participant> participants = participantTable.get_participants();
            display = initialize_display_table();
            for (size_t i=0; i<participants.size(); i++)
            {
                display.add_row({participants[i].hostname,
                                 participants[i].ip,
                                 participants[i].mac,
                                 participants[i].status == Table::ParticipantStatus::Awaken ? "AWAKE" : "ASLEEP"});
                display[i+1][3].format()
                    .font_color(participants[i].status == Table::ParticipantStatus::Awaken ? tabulate::Color::green
                                                                                           : tabulate::Color::red) ;     
            }
            std::cout <<"\033[0;0H"  // sets cursor position to (0,0)
                      << display     // prints table
                      << std::endl   
                      << std::endl
                      << ">> ";
            std::flush(std::cout);  
            sleep(REFRESH_RATE);
        }
    }

    void * InterfaceService::start_command_listener(void * param)
    {
        InterfaceService *obj = (InterfaceService *) param;
        obj->run_command_listener();
        return NULL;
    }

    void InterfaceService::run_command_listener()
    {
        std::string cmd;
        while(true)
        {
            std::cout << ">> ";
            std::getline(std::cin, cmd);
            std::cout << "\033[2A"
                      << "\033[K"
                      << cmd 
                      << '!' 
                      << std::endl
                      << "\033[K";
            std::flush(std::cout);
        }
    }
}