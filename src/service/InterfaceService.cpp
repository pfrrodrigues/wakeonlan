#include <../src/service/InterfaceService.hpp>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <signal.h>

namespace WakeOnLanImpl {
    // InterfaceService
    InterfaceService::InterfaceService(Table &table, std::shared_ptr<NetworkHandler> netHandler) 
        : participantTable(table),
          config(netHandler->getDeviceConfig())
    {
        inetHandler = netHandler;
    }

    void InterfaceService::stop()
    {
        log->info("Stop Interface service");
        if(config.getHandlerType() == HandlerType::Participant)
            sendExitMsg();
        keepRunning = false;
    }

    void InterfaceService::notifyRoleChange() {
        config = inetHandler->getDeviceConfig();
        std::cout << "\033[s"   // saves cursor position
                  << "\033[1A"  // moves cursor 1 line up
                  << "\033[2K"   // clears line 
                  << "This machine is now a " << ((config.getHandlerType() == HandlerType::Manager) ? "manager" : "participant")
                  << std::endl 
                  << "\033[u";  // returns to previous position 
    }

    void InterfaceService::run()
    {
        keepRunning = true;
        numParticipants = 0;
        log = spdlog::get("wakeonlan-api");
        log->info("Start Interface service");
        pthread_t table_th, cmd_th;
        threads.push_back(table_th);
        threads.push_back(cmd_th);

        std::cout << "\033[2J" ;    // clears terminal and moves cursor to (0,0)
                //   << "\033[1;0f";  // set cursor position to line 3, col 0
        std::flush(std::cout);

        int ret;
        ret = pthread_create(&threads[0], NULL, &startDisplayTable, this);
        ret = pthread_create(&threads[1], NULL, &startCommandListener, this);
    }

    void * InterfaceService::startDisplayTable(void * param)
    {
        InterfaceService *obj = (InterfaceService *) param;
        obj->runDisplayTable();
        return NULL;
    }

    void InterfaceService::initializeDisplayTable()
    {
        // print header
        std::cout << "\033[30;47m" << std::left << std::setw(20) << std::setfill(' ') << "HOSTNAME" << "\033[0m ";
        std::cout << "\033[30;47m" << std::left << std::setw(20) << std::setfill(' ') << "IP ADDRESS" << "\033[0m ";
        std::cout << "\033[30;47m" << std::left << std::setw(20) << std::setfill(' ') << "MAC ADDRESS" << "\033[0m ";
        std::cout << "\033[30;47m" << std::left << std::setw(10) << std::setfill(' ') << "STATUS" << "\033[0m\n";
        return;
    }

    void InterfaceService::runDisplayTable()
    {
        std::cout << "\033[2J"; // clears terminal and moves cursor to (0,0)
        // std::cout << "\n>> ";
        std::flush(std::cout);
        int newNumParticipants;
        while (keepRunning)
        {
            newNumParticipants = 0;
            lastSyncParticipants = participantTable.get_participants_interface();

            std::cout <<"\033[?25l"           // hides cursor
                      <<"\033[s"              // saves cursor position
                      <<"\033[" << 3 << ";0f";          // set cursor position to line 3, col 0
            std::flush(std::cout);
            initializeDisplayTable();
            for(int i=0; i<numParticipants; i++)
                std::cout << "\033[K\n";
            if(numParticipants)
                std::cout << "\033[" << numParticipants << "A";
            for (size_t i=0; i<lastSyncParticipants.size(); i++)
            {
                std::string  status;
                switch (lastSyncParticipants[i].status) {
                    case Table::ParticipantStatus::Awaken:
                        status = "\033[92mAWAKEN\033[0m";
                        break;
                    case Table::ParticipantStatus::Sleeping:
                        status = "\033[91mSLEEPING\033[0m";
                        break;
                    case Table::ParticipantStatus::Unknown:
                        status = "\033[93mUNKNOWN\033[0m";
                        break;
                    case Table::ParticipantStatus::Manager:
                        status = "\033[94mMANAGER\033[0m";
                        break;
                    default:
                        break;
                }
                std::string you = config.getIpAddress() == lastSyncParticipants[i].hostname ? " (you)" : "";
                std::cout <<"\033[K";
                std::cout << std::left << std::setw(20) << std::setfill(' ') << lastSyncParticipants[i].hostname << you << "|";
                std::cout << std::left << std::setw(20) << std::setfill(' ') << lastSyncParticipants[i].ip << "|";
                std::cout << std::left << std::setw(20) << std::setfill(' ') << lastSyncParticipants[i].mac << "|";
                std::cout << std::left << std::setw(10) << std::setfill(' ') << status << "\n";
                newNumParticipants++;    
            }
            std::cout << std::endl;

            // std::cout << "\033[1B>> ";         // moves cursor 1 line down and draws "<< "
            // if(newNumParticipants == numParticipants)
            std::cout << "\033[u";        // returns cursor to saved position     
            std::cout << "\033[?25h";         // shows cursor
            // std::cout << "<< ";
            std::flush(std::cout);  

            numParticipants = newNumParticipants;
            // sleep(DISPLAY_TABLE_REFRESH_RATE);
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
        std::cout << "Starting as participant. Available commands: EXIT" << std::endl;
        std::string cmd, response;
        while(keepRunning)
        {
            std::cout << ">> ";
            if(!std::getline(std::cin, cmd))
	        {
                kill(getpid(), SIGINT);
		        return;
            }
            switch (config.getHandlerType())
            {
            case HandlerType::Manager:
                response = parseInputManager(cmd);
                break;
            case HandlerType::Participant:
                response = parseInputParticipant(cmd);
            default:
                break;
            }
            std::cout << "\033[2A"  // moves cursor 2 lines up
                      << "\033[K"   // clears line 
                      << response   
                      << std::endl 
                      << "\033[K";  // clears previous input 
            std::flush(std::cout);
        }
    }

    std::string InterfaceService::parseInputManager(std::string cmd)
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
        return "Available commands for MANAGER: WAKEUP <hostname>";
    }

    
    std::string InterfaceService::processWakeupCmd(std::string hostname)
    {

        std::string mac;
        bool hostnameFound = false; 
        for (auto& participant : lastSyncParticipants)
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

        inetHandler->wakeUp(mac); 
        return "Waking up " + hostname + " @ " + mac + ".";
    }

    std::string InterfaceService::parseInputParticipant(std::string cmd)
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
        return "Available commands as PARTICIPANT: EXIT";
    }

    std::string InterfaceService::processExitCmd()
    {
        kill(getpid(), SIGINT);
        return "Sending exit message to manager @ " + inetHandler->getManagerIp();  
    }

    void InterfaceService::sendExitMsg()
    {
        Message exit_msg;
        exit_msg.type = Type::SleepServiceExit;
        bzero(exit_msg.hostname, sizeof(exit_msg.hostname));
        bzero(exit_msg.ip, sizeof(exit_msg.ip));
        bzero(exit_msg.mac, sizeof(exit_msg.mac));
        strncpy(exit_msg.hostname, config.getHostname().c_str(), config.getHostname().size());
        strncpy(exit_msg.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
        strncpy(exit_msg.mac, config.getMacAddress().c_str(), config.getMacAddress().size());
        inetHandler->send(exit_msg, inetHandler->getManagerIp());     
    }
    
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
}
    // void ParticipantInterfaceService::runCommandListener()
    // {
    //     // wait connection
    //     std::cout << "Waiting connection to manager...";
    //     std::flush(std::cout);
    //     std::vector<Table::Participant> ms = participantTable.get_participants_interface();
    //     if(ms.size() != 0)
    //         std::cout << "Something went wrong..";
    //     lastSyncManager = ms[0];

    //     // receive input
    //     std::cout << "\033[2J" // clears terminal and moves cursor to (0,0)
    //               << "You are connected."
    //               << std::endl;         
    //     std::string cmd, response;
    //     while(keepRunning)
    //     {
    //         std::cout << ">> ";
    //         if(!std::getline(std::cin, cmd))
	//         {
    //             kill(getpid(), SIGINT);
    //             return;
    //         }
    //         response = parseInput(cmd);
    //         std::cout << "\033[2A"  // moves cursor 2 lines up
    //                   << "\033[K"   // clears line 
    //                   << response   
    //                   << std::endl 
    //                   << "\033[K";  // clears previous input 
    //         std::flush(std::cout);
    //     }
    //     processExitCmd();
    // }
