#pragma once
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

namespace WakeOnLanImpl {
    class UdpSocket {
    public:
        UdpSocket(const std::string &ip, const uint16_t &port);

        ~UdpSocket() = default;

        void send(char* buffer, const size_t &size);

        void receive(char buffer[2048]);

        void closeSocket();
    private:
        int fd;
        sockaddr_in sockaddr;
    };
} // namespace WakeOnLanImpl