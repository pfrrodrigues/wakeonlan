#include <../src/impl/ApiInstanceImpl.hpp>

namespace WakeOnLanImpl {
    ApiInstanceImpl::ApiInstanceImpl(const Config &config)
            : config(config) {
        /* Log initialization */
        try {
            std::system("mkdir -p logs");
            spdlog::set_async_mode(8192,
                                   spdlog::async_overflow_policy::block_retry,
                                   nullptr, std::chrono::seconds(1));

            log = spdlog::basic_logger_mt("wakeonlan-api", "logs/wakeonlan-api.log");
            log->set_pattern("[%d-%m-%Y %H:%M:%S] [%l] %v");
            log->info("---------------- Wake on Lan API ---------------");
            log->info("Version {}", WAKEONLAN_API_VERSION);
            log->info("Handler: {}", [this]() {
                switch (this->config.getHandlerType()) {
                    case Manager:
                        return "Manager";
                    case Participant:
                        return "Participant";
                    default:
                        break;
                }
            }());
            log->info("Host Interface: {}", this->config.getIface());
            log->info("Hostname: {}", this->config.getHostname());
            log->info("IP Address: {}", this->config.getIpAddress());
            log->info("MAC Address: {}", this->config.getMacAddress());
        } catch (spdlog::spdlog_ex &e) {
            std::cout << "Log init failed: " << e.what() << std::endl;
        }

        handler = std::make_unique<Handler>(config);
    }

    ApiInstanceImpl::~ApiInstanceImpl() {}

    void ApiInstanceImpl::run() {
        log->info("Starting handler");
        handler->run();
    }

    void ApiInstanceImpl::stop() {
        handler->stop();
        log->info("Stopping handler");
    }
}
