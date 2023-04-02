#pragma once
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <../src/MessageTypes.hpp>

#define MAGIC_PACKET_SIZE 102

namespace WakeOnLanImpl {
    /**
     * @class UdpSocket
     * This class implements the base functions is a wrapper to a UDP socket. Other classes
     * can use it to send/receive the API packets.
     */
    class UdpSocket {
    public:
        /**
         * UdpSocket constructor
         * @param ip The UDP socket IP address.
         * @param port The UDP socket port.
         */
        UdpSocket(const std::string &ip, const uint16_t &port);

        /**
         * UdpSocket destructor
         */
        ~UdpSocket() = default;

        /**
        * Sends a message.
        *
        * @param message The message to be sent.
        * @return None.
        */
        void send(const Message &message);
        void send(const char * buffer, size_t size);

        /**
        * Sends a magic packet.
         *
        * @param buffer The buffer containing the magic packet information.
        * @return None.
        */
        void sendMagicPacket(const char buffer[MAGIC_PACKET_SIZE]);

        /**
        * Receives a message.
        *
        * @param message A pointer to a Message object for receiving the message.
        * @return None.
        */
        void receive(Message *message);

        /**
        * Closes the UDP socket.
        *
        * @return None.
        */
        void closeSocket();
    private:
        int fd;                 ///< The socket file descriptor.
        sockaddr_in sockaddr;   ///< The socket configuration struct.
    };
} // namespace WakeOnLanImpl