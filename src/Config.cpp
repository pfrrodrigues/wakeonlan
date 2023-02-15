#include <iostream>
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <Config.hpp>

namespace WakeOnLan {
    class ConfigParserException : public std::runtime_error {
    public:
        explicit ConfigParserException(const std::string &message) : std::runtime_error(message.data()) {}
    };

    Config::Config(char **params, const int &quantity)
            : handlerType(Participant) {
        try {
            if (quantity < 1) {
                std::stringstream ss;
                ss << "ConfigParserException: few arguments, but application expects 1 or 2.\n";
                ss << "./wolapp ['manager'] <interface>";
                throw ConfigParserException(ss.str());
            }

            if (quantity == 2) {
                std::string arg(params[0]);
                if (arg != "manager") {
                    std::stringstream ss;
                    ss << "ConfigParserException: error in interpret handler type '";
                    ss << arg << "'\n";
                    ss << "./wolapp ['manager'] <interface>";
                    throw ConfigParserException(ss.str());
                }
                handlerType = Manager;
                interface = params[1];
            }
            else {
                interface = params[0];
            }

            /* Get the MAC address */
            std::string command("cat /sys/class/net/");
            command.append(interface)
                    .append("/address > mac.out");
            std::system(command.c_str());
            std::ifstream ifs("mac.out");
            getline(ifs, mac);
            std::transform(mac.begin(), mac.end(), mac.begin(), ::toupper);

            /* Get the hostname */
            char host[150];
            gethostname(host, sizeof(host) - 1);
            hostname = host;

            /* Get the IP address */
            int fd;
            struct ifreq ifr{};
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            ifr.ifr_addr.sa_family = AF_INET;
            strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ-1);
            ioctl(fd, SIOCGIFADDR, &ifr);
            close(fd);
            ip = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

        }
        catch (ConfigParserException &e) {
            std::cerr << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    std::string Config::getIface() { return interface; }

    std::string Config::getHostname() { return hostname; }

    std::string Config::getIpAddress() { return ip; }

    std::string Config::getMacAddress() { return mac; }

    HandlerType Config::getHandlerType() { return handlerType; }

}