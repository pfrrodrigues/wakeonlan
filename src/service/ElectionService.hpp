#pragma once
#include <memory>
#include <ctime>
#include <spdlog/spdlog.h>
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <Types.hpp>

namespace WakeOnLanImpl {
    /**
     * @class ElectionService
     * Implements the election service.
     */
    class ElectionService
    {
    public:
        /**
         * ElectionService constructor.
         * @param table The singleton table used to store the manager information or the participants information.
         * @param networkHandler The unique Network handler.
         */
        ElectionService(Table &table, std::shared_ptr<NetworkHandler> networkHandler);
        
        /**
         * ElectionService destructor
         */
        ~ElectionService();

        void run();
        
        void stop();

        /**
         * Starts a new election and returns the result.         
         * @return HandlerType, new role as decided in the election.
         */
        HandlerType startElection();

        /**
         * Get results from elections started by another participant.         * 
         * @return HandlerType, new role as decided in the election or current role if no new elections.
         */
        HandlerType getNewElectionResult();
    private:
        std::unique_ptr<std::thread> t;                 ///< The service dedicated thread.
        bool active;                                    ///< Indicates service is active or not.
        bool ongoingElection;                           ///< Indicates there is an election happening
        bool ongoingElectionAnswered;                   ///< Indicates there was an answer to the election message in current election
        time_t ongoingElectionStart;
        std::shared_ptr<spdlog::logger> log;            ///< The ElectionService logger.
        Table &table;                                   ///< The singleton table.
        std::shared_ptr<NetworkHandler> inetHandler;    ///< A shared pointer to the unique Network handler.
        time_t lastWin;

        std::mutex newElectionMutex;
        bool unreadElection;
        HandlerType newElectionResult;

        void announceVictory();
        void sendCoordinatorMsgs();
        HandlerType sendElectionMsgs(std::vector<Table::Participant> contenders);
        void sendAnswerMsg();
        std::vector<Table::Participant> getContenders();

    };
} // namespace WakeOnLanImpl