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

#pragma pack(pop)
} //namespace WakeOnLanApiImpl