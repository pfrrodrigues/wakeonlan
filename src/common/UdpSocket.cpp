#include <../src/common/UdpSocket.hpp>
#define BROADCAST_ADDRESS "255.255.255.255"
#define LOCAL_SERVER_ADDRESS "0.0.0.0"

namespace WakeOnLanImpl {
    UdpSocket::UdpSocket(const std::string &ip, const uint16_t &port) {
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("socket (AF_INET, SOCK_DGRAM, 0)");
            exit(1);    // TODO: this must be handled
        }

        u_int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes)) == -1) {
            perror("setsockopt (SO_REUSEADDR)");
            exit(1);    // TODO: this must be handled
        }

        if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes)) == -1) {
            perror("setsockopt (SO_BROADCAST)");
            exit(1);    // TODO: this must be handled
        }

        struct sockaddr_in destSockAddr{};
        memset(&destSockAddr, 0, sizeof(destSockAddr));
        destSockAddr.sin_family = AF_INET;
        destSockAddr.sin_port = htons(port);
        if (ip == BROADCAST_ADDRESS) {
            destSockAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        }
        else if (ip == LOCAL_SERVER_ADDRESS) {
            destSockAddr.sin_addr.s_addr = INADDR_ANY;
            if (bind(fd, (struct sockaddr *) &destSockAddr, sizeof(struct sockaddr)) < 0)
                perror("error in bind server");
        }
        else {
            destSockAddr.sin_addr.s_addr = inet_addr(ip.c_str());
        }
        sockaddr = destSockAddr;
    }

    void UdpSocket::send(char *buffer, const size_t &size) {
        sendto(fd,
               buffer,
               size,
               MSG_CONFIRM,
               (const struct sockaddr *) &sockaddr,
               sizeof(sockaddr)
        );
    }

    void UdpSocket::receive(char *buffer) {
        struct sockaddr_in client_addr{};
        uint32_t size = sizeof(client_addr);
        int n = recvfrom(fd,
                         (char*)buffer,
                         2048,
                         0,
                         (struct sockaddr *) &client_addr,
                         reinterpret_cast<socklen_t *>(&size));
        if (n < 0)
            printf("ERROR on recvfrom"); // TODO: this must be handled
    }

    void UdpSocket::closeSocket() {
        close(fd);
    }

} // namespace WakeOnLanApiImpl