#include <../src/impl/ApiInstanceImpl.hpp>

ApiInstanceImpl::ApiInstanceImpl(const Config &config)
    : config(config) {
    switch (this->config.getHandlerType()) {
        case Manager:
            handler = std::make_unique<ManagerHandler>();
            break;
        case Participant:
            handler = std::make_unique<ParticipantHandler>();
            break;
    }

     /* Log initialization */
    try {
        std::system("mkdir -p logs");
        spdlog::set_async_mode(8192,
                               spdlog::async_overflow_policy::block_retry,
                               nullptr, std::chrono::seconds(1));

        auto log = spdlog::basic_logger_mt("wakeonlan-api", "logs/wakeonlan-api.log");
        log->set_pattern("[%d-%m-%Y %H:%M:%S] [%l] %v");
        log->info("---------------- Wake on Lan API ---------------");
        log->info("Version {}", WAKEONLAN_API_VERSION);
        log->info("Handler: {}", [this]() {
            switch (this->config.getHandlerType()) {
                case Manager:
                    return "Manager";
                    break;
                case Participant:
                    return "Participant";
                    break;
                default:
                    break;
            }
        }());
        log->info("Hostname: {}", this->config.getHostname());
        log->info("IP Address: {}", this->config.getIpAddress());
        log->info("MAC Address: {}", this->config.getMacAddress());
    } catch (spdlog::spdlog_ex &e) {
        std::cout << "Log init failed: " << e.what() << std::endl;
    }
}

ApiInstanceImpl::~ApiInstanceImpl() {}

void ApiInstanceImpl::run() {
    handler->run();
}