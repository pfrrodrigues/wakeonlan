#pragma once
#include <spdlog/spdlog.h>
#include <../include/Config.hpp>
#include <../src/handler/Handler.hpp>

namespace WakeOnLanImpl {
    /**
     * @class ApiInstanceImpl
     *
     */
    class ApiInstanceImpl {
    public:
        /**
         * ApiInstanceImpl constructor.
         * @param config The API configuration.
         */
        explicit ApiInstanceImpl(const Config &config);

        /**
         * ApiInstanceImpl destructor.
         */
        ~ApiInstanceImpl();

        /**
         * Runs the API services.
         */
        void run();

        /**
         * Stops the API services.
         */
        void stop();
    private:
        Config config;                          ///< The API configuration.
        std::unique_ptr<Handler> handler;       ///< The API handler.
        std::shared_ptr<spdlog::logger> log;    ///< The ApiInstanceImpl logger.
    };
} // namespace WakeOnLanImpl

