#pragma once
#include <spdlog/spdlog.h>
#include <../include/Config.hpp>
#include <../src/handler/Handler.hpp>

namespace WakeOnLanImpl {
    class ApiInstanceImpl {
    public:
        explicit ApiInstanceImpl(const Config &config);
        ~ApiInstanceImpl();
        void run();
        void stop();
    private:
        Config config;
        std::unique_ptr<Handler> handler;
        std::shared_ptr<spdlog::logger> log;
    };
} // namespace WakeOnLanImpl

