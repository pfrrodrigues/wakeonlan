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

        HandlerType startElection();
    private:
        std::unique_ptr<std::thread> t;                 ///< The service dedicated thread.
        bool active;                                    ///< Indicates service is active or not.
        std::shared_ptr<spdlog::logger> log;            ///< The ElectionService logger.
        Table &table;                                   ///< The singleton table.
        std::shared_ptr<NetworkHandler> inetHandler;    ///< A shared pointer to the unique Network handler.
        time_t lastWin;
    };
} // namespace WakeOnLanImpl