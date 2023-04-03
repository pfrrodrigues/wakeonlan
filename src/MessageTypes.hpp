#pragma once
#include <cstdint>
#include <cstring>
#include <iostream>

namespace WakeOnLanImpl {
#define WAKEONLAN_FIELD_TIMESTAMP_SIZE 80
#define WAKEONLAN_FIELD_HOSTNAME_SIZE 150
#define WAKEONLAN_FIELD_IP_SIZE 150
#define WAKEONLAN_FIELD_MAC_SIZE 17
#define WAKEONLAN_FIELD_STATUS_SIZE 1
#pragma pack(push, 1)

/**
 * @enum Type
 * The Message type.
 */
enum class Type {
    SleepServiceDiscovery = 'D',      ///< Indicates the message is a SleepServiceDiscovery message.
    SleepStatusRequest = 'R',         ///< Indicates the message is a SleepStatusRequest message.
    SleepServiceExit = 'E',           ///< Indicates the message is a SleepServiceExit message.
    ElectionServiceElection = 'L',    ///< Indicates the message is a ElectionService election message.
    ElectionServiceCoordinator = 'C', ///< Indicates the message is a ElectionService coordinator message.
    ElectionServiceAnswer = 'A',      ///< Indicates the message is a ElectionService answer message.
    Unknown = 'U'  ,                  ///< Indicates a unknown type was parsed.
    TableUpdate = 'T'               ///< Indicates the message contains a table update.
};

struct TableUpdateHeader {
    uint8_t noEntries;              ///< Number of table entries contained on the message.
};

/**
 * @struct Message
 * The struct represents the messages send/received by the API services.
 */
struct Message {
    Type type;                                        ///< The message type.
    uint32_t msgSeqNum;                               ///< The message sequence number (used by Discovery service and TableUpdate).
    char hostname[WAKEONLAN_FIELD_HOSTNAME_SIZE];     ///< The source/destination hostname.
    char ip[WAKEONLAN_FIELD_IP_SIZE];                 ///< The source/destination IP address.
    char mac[WAKEONLAN_FIELD_MAC_SIZE];               ///< The source/destination MAC address.
    char data[sizeof(TableUpdateHeader) + 4 *
            ( WAKEONLAN_FIELD_TIMESTAMP_SIZE
              + WAKEONLAN_FIELD_HOSTNAME_SIZE
              + WAKEONLAN_FIELD_IP_SIZE
              + WAKEONLAN_FIELD_MAC_SIZE
              + WAKEONLAN_FIELD_STATUS_SIZE )];       ///< Contains the table entries (used on TableUpdate message).
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