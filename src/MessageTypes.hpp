#pragma once
#include <cstdint>
#include <cstring>
#include <iostream>

namespace WakeOnLanImpl {
#pragma pack(push, 1)

enum class Type {
    SleepServiceDiscovery = 'D',
    SleepStatusRequest = 'R',
    SleepServiceExit = 'E',
    Unknown = 'U'
};

struct Message {
    Type type;
    uint32_t msgSeqNum;
    char hostname[150];
    char ip[150];
    char mac[17];
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