#pragma once
#include <cstdint>
#include <cstring>
#include <iostream>

namespace WakeOnLanImpl {
#pragma pack(push, 1)

/**
 * @enum Type
 * The Message type.
 *
 */
enum class Type {
    SleepServiceDiscovery = 'D',    ///< Indicates the message is a SleepServiceDiscovery message.
    SleepStatusRequest = 'R',       ///< Indicates the message is a SleepStatusRequest message.
    SleepServiceExit = 'E',         ///< Indicates the message is a SleepServiceExit message.
    Unknown = 'U',                   ///< Indicates a unknown type was parsed.
    TableUpdate = 'T'                    ///< Indicates the message contains a table update
};

struct TableUpdateHeader {
    uint32_t seq;
    uint8_t noEntries;
};

/**
 * @struct Message
 * The struct represents the messages send/received by the API services.
 */
struct Message {
    Type type;            ///< The message type.
    uint32_t msgSeqNum;   ///< The message sequence number (used by Discovery service).
    char hostname[150];   ///< The source/destination hostname.
    char ip[150];         ///< The source/destination IP address.
    char mac[17];         ///< The source/destination MAC address.
    bool reserved;
    char * data;

    /* fields of TableUpdate
     * HEADER
     * timestamp(80) | hostname (150) | ip(150) | mac(17) | status(1)
     * ...
     * timestamp(80) | hostname (150) | ip(150) | mac(17) | status(1)
     */
};


inline std::ostream& operator<<(std::ostream &os, const Message &message) {
    os << "Message [\n";
    os << '\t' << "Type=" << [message]() {
        switch (message.type) {
            case Type::SleepServiceDiscovery:
                return "SleepServiceDiscovery\n";
            case Type::SleepStatusRequest:
                return "SleepStatusRequest\n";
            case Type::SleepServiceExit:
                return "SleepServiceExit\n";
            default:
                return "";
        }
    }();
    os << "\tSequenceNumber=" << message.msgSeqNum << '\n';
    os << "\tHostname=" << message.hostname << '\n';
    os << "\tIP=" << message.ip << '\n';
    os << "\tPort=" << message.mac << '\n';
    os << "]\n\n";
    return os;
}

#pragma pack(pop)
} //namespace WakeOnLanApiImpl