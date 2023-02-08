#include <../src/handler/NetworkHandler.hpp>
#define WON_PORT 9
#define SERVICE_PORT 4000
#define BROADCAST_ADDRESS "255.255.255.255"
#define LOCAL_SERVER_ADDRESS "0.0.0.0"

namespace WakeOnLanImpl {
    NetworkHandler::NetworkHandler(const uint32_t &port)
    : port(port)
    {
        t = std::make_unique<std::thread>([this, port](){
            UdpSocket socket(LOCAL_SERVER_ADDRESS, port); // TODO: this must be closed
            while (true) {
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
                        case Type::Unknown:
                        default:
                            break;
                    }
                }
            }
        });
    }

    NetworkHandler::~NetworkHandler() {
        if (t) {
            if (t->joinable())
                t->join();
        }
    }

    bool NetworkHandler::send(const Message &message, const std::string &ip) {
        UdpSocket socket(ip, SERVICE_PORT);
        socket.send(message);
        return true;
    }

    Message* NetworkHandler::getFromDiscoveryQueue() {
        // TODO: this must be done to free message after returned it
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
        // TODO: this must be done to free message after returned it
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
} // namespace WakeOnLanImpl;
