#include <../src/service/InterfaceService.hpp>
#include <iostream>
#include <unistd.h>

namespace WakeOnLanImpl {
    InterfaceService::InterfaceService(Table &table) 
        : participantTable(table)
    {
    }

    void InterfaceService::run()
    {
        numParticipants = 0;

        int ret;
        ret = pthread_create(&threads[0], NULL, &startDisplayTable, this);
        ret = pthread_create(&threads[1], NULL, &startCommandListener, this);

        pthread_join(threads[0], NULL);
        pthread_join(threads[1], NULL);
    }

    void * InterfaceService::startDisplayTable(void * param)
    {
        InterfaceService *obj = (InterfaceService *) param;
        obj->runDisplayTable();
        return NULL;
    }

    tabulate::Table InterfaceService::initializeDisplayTable()
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

    void InterfaceService::runDisplayTable()
    {
        tabulate::Table display;
        std::cout << "\033[2J"; // clears terminal and moves cursor to (0,0)
        int newParticipants;
        while (true)
        {
            newParticipants = 0;
            std::vector<Table::Participant> participants = participantTable.get_participants();
            display = initializeDisplayTable();
            for (size_t i=0; i<participants.size(); i++)
            {
                display.add_row({participants[i].hostname,
                                 participants[i].ip,
                                 participants[i].mac,
                                 participants[i].status == Table::ParticipantStatus::Awaken ? "AWAKE" : "ASLEEP"});
                display[i+1][3].format()
                    .font_color(participants[i].status == 
                                Table::ParticipantStatus::Awaken ? tabulate::Color::green
                                                                 : tabulate::Color::red) ; 
                newParticipants++;    
            }

            std::cout <<"\033[?25l"  // hides cursor
                      <<"\033[s"     // saves cursor position
                      <<"\033[0;0H"  // sets cursor position to (0,0)
                      << display     // prints table
                      << std::endl
                      <<"\033[1B>> "; // moves cursor 1 line down and draws "<< "
            if(newParticipants == numParticipants)
                std::cout << "\033[u"; // returns cursor to saved position     
            std::cout << "\033[?25h"; // shows cursor
            std::flush(std::cout);  

            numParticipants = newParticipants;
            sleep(REFRESH_RATE);
        }
    }

    void * InterfaceService::startCommandListener(void * param)
    {
        InterfaceService *obj = (InterfaceService *) param;
        obj->runCommandListener();
        return NULL;
    }

    void InterfaceService::runCommandListener()
    {
        std::string cmd, response;
        while(true)
        {
            std::cout << ">> ";
            std::getline(std::cin, cmd);
            response = parseInput(cmd);
            std::cout << "\033[2A"  // moves cursor 2 lines up
                      << "\033[K"   // clears line 
                      << response   
                      << std::endl 
                      << "\033[K";  // clears previous input 
            std::flush(std::cout);
        }
    }

    std::string InterfaceService::parseInput(std::string cmd)
    {
        /*
         * Missing handler integration if manager/client usage rules 
         * are to be enforced.
         * Also missing message integration for sending out 
         * wakeup and exit messages.
         */
        std::string word;
        std::vector<std::string> words;
        int pos = cmd.find(' '), start = 0;
        while (pos != std::string::npos)
        {   
            word = cmd.substr(start, pos - start);
            if (word.size() > 1)
                words.push_back(word);
            start = pos + 1;
            pos = cmd.find(' ', start);
        }
        word = cmd.substr(start);
        if (word.size() > 1)
            words.push_back(word);
            
        if (words.size() == 0)
            return "";
        if (words[0] == "exit" || words[0] == "EXIT")
        {
            if(words.size() != 1)
                return "Correct usage: EXIT";
            std::string response = processExitCmd(); 
            return response;
        }
        if (words[0] == "wakeup" || words[0] == "WAKEUP")
        {
            if(words.size() != 2)
                return "Correct usage: WAKEUP <hostname>";
            else 
            {
                std::string response = processWakeupCmd(words[1]);
                return response;
            }
        }
        return "Available commands: EXIT | WAKEUP <hostname>";
    }
    
    std::string InterfaceService::processWakeupCmd(std::string hostname)
    {
        /* waiting integration with network handler */
        std::vector<Table::Participant> participants = participantTable.get_participants();

        std::string mac;
        bool hostnameFound = false; 
        for (auto& participant : participants)
            if(participant.hostname == hostname)
            {
                if(participant.status == Table::ParticipantStatus::Awaken)
                    return hostname + " already awake.";
                hostnameFound = true;
                mac = participant.mac;
                break;
            }
        if (!hostnameFound)
            return hostname + " not found.";
        
        // send wakeup message to mac address
        return "Waking up " + hostname + " @ " + mac + ".";
    }

    std::string InterfaceService::processExitCmd()
    {
        /* waiting integration with network handler */
        return "Exiting service...";    
    }
}