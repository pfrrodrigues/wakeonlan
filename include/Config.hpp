#pragma once
#include <string>
#include <../include/Types.hpp>

class Config {
public:
    Config(char** params, const int &size);

    std::string getHostname();

    std::string getIpAddress();

    std::string getMacAddress();

    std::string getIface();

    HandlerType getHandlerType();
private:
    HandlerType handlerType; /// Default is ::Participant
    std::string hostname;
    std::string ip;
    std::string mac;
    std::string interface;
};
