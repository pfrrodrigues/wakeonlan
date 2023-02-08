#include <../src/service/InterfaceService.hpp>
#include <iostream>
#include <unistd.h>

namespace WakeOnLanImpl {
    // InterfaceService
    InterfaceService::InterfaceService(Table &table, std::shared_ptr<NetworkHandler> netHandler) 
        : participantTable(table)
    {
        networkHandler = netHandler;
    }

    void InterfaceService::stop()
    {
        keepRunning = false;
        for (auto thread : threads)
            pthread_join(thread, NULL);
    }

    void InterfaceService::run() {}

    std::vector<std::string> InterfaceService::splitCmd(std::string cmd)
    {
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
        return words;
    }

    // ManagerInterfaceService

    void ManagerInterfaceService::run()
    {
        keepRunning = true;
        numParticipants = 0;

        pthread_t table_th, cmd_th;
        threads.push_back(table_th);
        threads.push_back(cmd_th);

        int ret;
        ret = pthread_create(&threads[0], NULL, &startDisplayTable, this);
        ret = pthread_create(&threads[1], NULL, &startCommandListener, this);
    }

    void * ManagerInterfaceService::startDisplayTable(void * param)
    {
        ManagerInterfaceService *obj = (ManagerInterfaceService *) param;
        obj->runDisplayTable();
        return NULL;
    }

    tabulate::Table ManagerInterfaceService::initializeDisplayTable()
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

    void ManagerInterfaceService::runDisplayTable()
    {
        tabulate::Table display;
        std::cout << "\033[2J"; // clears terminal and moves cursor to (0,0)
        int newNumParticipants;
        while (keepRunning)
        {
            newNumParticipants = 0;
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
                newNumParticipants++;    
            }

            std::cout <<"\033[?25l"           // hides cursor
                      <<"\033[s"              // saves cursor position
                      <<"\033[0;0H"           // sets cursor position to (0,0)
                      << display << std::endl // prints table
                      <<"\033[1B>> ";         // moves cursor 1 line down and draws "<< "
            if(newNumParticipants == numParticipants)
                std::cout << "\033[u";        // returns cursor to saved position     
            std::cout << "\033[?25h";         // shows cursor
            std::flush(std::cout);  

            numParticipants = newNumParticipants;
            sleep(DISPLAY_TABLE_REFRESH_RATE);
        }
    }

    void * ManagerInterfaceService::startCommandListener(void * param)
    {
        ManagerInterfaceService *obj = (ManagerInterfaceService *) param;
        obj->runCommandListener();
        return NULL;
    }

    void ManagerInterfaceService::runCommandListener()
    {
        std::string cmd, response;
        while(keepRunning)
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

    std::string ManagerInterfaceService::parseInput(std::string cmd)
    {
        std::vector<std::string> args = splitCmd(cmd);            
        if (args.size() == 0)
            return "";
        if (args[0] == "wakeup" || args[0] == "WAKEUP")
        {
            if(args.size() != 2)
                return "Correct usage: WAKEUP <hostname>";
            std::string response = processWakeupCmd(args[1]);
            return response;
        }
        return "Available commands: WAKEUP <hostname>";
    }

    
    std::string ManagerInterfaceService::processWakeupCmd(std::string hostname)
    {
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

        networkHandler->wakeUp(mac); 
        return "Waking up " + hostname + " @ " + mac + ".";
    }

    // ParticipantInterfaceService

    void ParticipantInterfaceService::run()
    {
        keepRunning = true;

        pthread_t cmd_th;
        threads.push_back(cmd_th);

        int ret;
        ret = pthread_create(&threads[0], NULL, &startCommandListener, this);
    }

    void * ParticipantInterfaceService::startCommandListener(void * param)
    {
        ParticipantInterfaceService *obj = (ParticipantInterfaceService *) param;
        obj->runCommandListener();
        return NULL;
    }

    bool ParticipantInterfaceService::isConnected()
    {
        std::vector<Table::Participant> manager = participantTable.get_participants();
        return manager.size() > 0;
    }

    void ParticipantInterfaceService::runCommandListener()
    {
        // wait connection
        if(!isConnected())
        {
            std::cout << "\033[2J"
                      << "Waiting connection to manager..."
                      << std::endl;
            while (!isConnected());
                sleep(CONNECTION_CHECK_RATE);
        }

        // receive input
        std::cout << "\033[2J" // clears terminal and moves cursor to (0,0)
                  << "You are connected."
                  << std::endl;         
        std::string cmd, response;
        while(keepRunning)
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

    std::string ParticipantInterfaceService::parseInput(std::string cmd)
    {
        std::vector<std::string> args = splitCmd(cmd);            
        if (args.size() == 0)
            return "";
        if (args[0] == "exit" || args[0] == "EXIT")
        {
            if(args.size() != 1)
                return "Correct usage: EXIT";
            std::string response = processExitCmd();
            return response;
        }
        return "Available commands: EXIT";
    }

    std::string ParticipantInterfaceService::processExitCmd()
    {
        // TODO: get device info from NetworkHandler
        std::vector<Table::Participant> manager = participantTable.get_participants();
        Message exit_msg = 
            { .type = Type::SleepServiceExit,
              .hostname = "tbd",
              .ip = "127.0.0.1",
              .mac = "FFFFFFFFFFFF" };
        networkHandler->send(exit_msg, manager[0].ip);     
        
        return "Sending exit message to manager " + manager[0].hostname + " @ " + manager[0].ip;    
    }
}