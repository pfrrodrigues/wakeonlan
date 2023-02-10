#include <../src/service/DiscoveryService.hpp>
#include <memory>

namespace WakeOnLanImpl {
    DiscoveryService::DiscoveryService(Table &t, std::shared_ptr<NetworkHandler> nh)
        : table(t),
        inetHandler(nh),
        active(false)
    {}

    DiscoveryService::~DiscoveryService() {
        if (t) {
            if (t->joinable())
                t->join();
        }
    }

    void DiscoveryService::runAsManager() {
        if (t)
            t->join();

        t = std::make_unique<std::thread>([this](){
            auto config = inetHandler->getDeviceConfig();
            Message message{};
            message.type = WakeOnLanImpl::Type::SleepServiceDiscovery;
            bzero(message.hostname, sizeof(message.hostname));
            bzero(message.ip, sizeof(message.ip));
            bzero(message.mac, sizeof(message.mac));
            strncpy(message.hostname, config.getHostname().c_str(), config.getHostname().size());
            strncpy(message.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
            strncpy(message.mac, config.getMacAddress().c_str(), config.getMacAddress().size());

            uint32_t seq = 1;
            while (true) {
                std::cout << "Manager is sending a discovery packet with sequence number " << seq << "...\n";
                sleep(2);
                message.msgSeqNum = seq;
                inetHandler->send(message, "255.255.255.255");
                sleep(1);
                auto m = inetHandler->getFromDiscoveryQueue();
                if (m != nullptr) {
		    if (m->ip != config.getIpAddress()) {
		            std::cout << "Manager has received a discovery response\n";
		            // std::cout << *m << std::endl;
			    sleep(15);
			    inetHandler->wakeUp(m->mac);
			    std::cout << "Sending a WOL package for the host " << m->mac << std::endl;
		            seq++;
		            m = nullptr;
		    }
                }
            }
        });
    }

    void DiscoveryService::runAsParticipant() {
        if (t)
            t->join();

        t = std::make_unique<std::thread>([this](){
            bool done = false;
            while (!done) {
                auto m = inetHandler->getFromDiscoveryQueue();
                if (m != nullptr) {
                    std::cout << "Participant has received a discovery request\n";
                    std::cout << *m;

                    std::cout << "Sending the discovery response with sequence number " << m->msgSeqNum + 1 << "...\n";
                    auto config = inetHandler->getDeviceConfig();
                    Message message{};
                    message.type = WakeOnLanImpl::Type::SleepServiceDiscovery;
                    message.msgSeqNum = m->msgSeqNum + 1;
                    bzero(message.hostname, sizeof(message.hostname));
                    bzero(message.ip, sizeof(message.ip));
                    bzero(message.mac, sizeof(message.mac));
                    strncpy(message.hostname, config.getHostname().c_str(), config.getHostname().size());
                    strncpy(message.ip, config.getIpAddress().c_str(), config.getIpAddress().size());
                    strncpy(message.mac, config.getMacAddress().c_str(), config.getMacAddress().size());
                    inetHandler->send(message, m->ip);
                    m = nullptr;
                }
            }
        });
    }

    void DiscoveryService::run() {
        auto config = inetHandler->getDeviceConfig();
        switch (config.getHandlerType()) {
            case HandlerType::Manager:
                runAsManager();
                break;
            case HandlerType::Participant:
                runAsParticipant();
                break;
            default:
                break;
        }
    }
}
