#include <../src/handler/NetworkHandler.hpp>
#define WON_PORT 9
#define SERVICE_PORT 4000
#define BROADCAST_ADDRESS "255.255.255.255"
#define LOCAL_SERVER_ADDRESS "0.0.0.0"

namespace WakeOnLanImpl {
    NetworkHandler::NetworkHandler(const uint32_t &port, const Config &cfg)
    : port(port),
      config(cfg),
      globalStatus(Unknown),
      active(false)
    {
        log = spdlog::get("wakeonlan-api");
        log->info("Start Network handler");

        t = std::make_unique<std::thread>([this, port](){
            this->active = true;
            UdpSocket socket(LOCAL_SERVER_ADDRESS, port);
            log->info("Network Handler (internal): listening on port {}", port);
            while (active) {
                Message response{};
                response.type = Type::Unknown;
                socket.receive(&response);

                if (response.type != Type::Unknown) {
                    switch (response.type) {
                        case Type::SleepStatusRequest:
                            monitoringQueue.push(response);
                            break;
                        case Type::SleepServiceDiscovery:
                        case Type::SleepServiceExit:
                            discoveryQueue.push(response);
                            break;
                        default:
                            break;
                    }
                }
            }
            socket.closeSocket();
            log->info("Network handler (internal): listener socket is closed");
        });
    }

    NetworkHandler::~NetworkHandler() {
        if (t->joinable()) {
            t->join();
        }
    }

    bool NetworkHandler::send(const Message &message, const std::string &ip) {
        std::lock_guard<std::mutex> lk(inetMutex);
        UdpSocket socket(ip, SERVICE_PORT);
        socket.send(message);
        return true;
    }

    Message* NetworkHandler::getFromDiscoveryQueue() {
        std::lock_guard<std::mutex> lk(inetMutex);
        static bool free = false;
        if (free) {
            discoveryQueue.pop();
        }

        if (!discoveryQueue.empty()) {
            Message * k = &discoveryQueue.front();
            free = true;
            return k;
        }
        else {
            free = false;
            return nullptr;
        }
    }

    Message* NetworkHandler::getFromMonitoringQueue() {
        std::lock_guard<std::mutex> lk(inetMutex);
        static bool free = false;
        if (free) {
            monitoringQueue.pop();
        }

        if (!monitoringQueue.empty()) {
            Message * k = &monitoringQueue.front();
            free = true;
            return k;
        }
        else {
            free = false;
            return nullptr;
        }
    }

    bool NetworkHandler::wakeUp(const std::string &mac) {
        std::lock_guard<std::mutex> lk(inetMutex);
        UdpSocket socket(BROADCAST_ADDRESS, WON_PORT);
        size_t pos;
        std::string delimiter = ":";
        std::string byte;
        std::string hex_mac;
        std::string copy = mac;
        std::string buffer("\xff\xff\xff\xff\xff\xff");

        while ((pos = copy.find(delimiter)) != std::string::npos) {
            byte = copy.substr(0, pos);
            char *ptr;
            long ret;
            ret = strtol(byte.c_str(), &ptr, 16);
            char b = (char)ret;
            hex_mac.append(1, b);
            copy.erase(0, pos + delimiter.size());
        }
        byte = copy.substr(0, pos);
        char *ptr;
        long ret;
        ret = strtol(byte.c_str(), &ptr, 16);
        char b = (char)ret;
        hex_mac.append(1, b);

        for (int i=0; i<16; i++)
            buffer.append(hex_mac);

        socket.sendMagicPacket(buffer.c_str());
        return true;
    }

    void NetworkHandler::changeStatus(const ServiceGlobalStatus &gs) {
        std::lock_guard<std::mutex> lk(gsMutex);
        globalStatus = gs;
        log = spdlog::get("wakeonlan-api");
        log->info("STATUS UPDATE: {}", [this]() {
            switch (globalStatus) {
                case WaitingForSync:
                    return "WaitingForSync";
                case Syncing:
                    return "Syncing";
                case Synchronized:
                    return "Synchronized";
                case Unknown:
                    return "Unknown";
                default:
                    break;
            }
        }());
    }

    const ServiceGlobalStatus& NetworkHandler::getGlobalStatus() {
        std::lock_guard<std::mutex> lk(gsMutex);
        return globalStatus;
    }

    const Config &NetworkHandler::getDeviceConfig() { return config; }

    void NetworkHandler::stop() {
        active = false;
        sleep(1);
        log->info("Stop Network handler");
    }
} // namespace WakeOnLanImpl;
