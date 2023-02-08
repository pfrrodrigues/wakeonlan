#pragma once
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <../src/MessageTypes.hpp>

#define MAGIC_PACKET_SIZE 102

namespace WakeOnLanImpl {
    class UdpSocket {
    public:
        UdpSocket(const std::string &ip, const uint16_t &port);

        ~UdpSocket() = default;

        void send(const Message &message);

        void sendMagicPacket(const char buffer[MAGIC_PACKET_SIZE]);

        void receive(Message *message);

        void closeSocket();
    private:
        int fd;
        sockaddr_in sockaddr;
    };
} // namespace WakeOnLanImpl