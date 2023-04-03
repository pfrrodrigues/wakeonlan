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

    Config::Config()
            : handlerType(Participant) {
        try {
            /* Get the host currently-active interface */
            std::ifstream ifs;
            std::string getIfaceCmd(
                    "(ip route get 1.1.1.1 | grep -Po '(?<=dev\\s)\\w+' | cut -f1 -d ' ') > iface.out"
            );
            std::system(getIfaceCmd.c_str());
            ifs.open("iface.out");
            getline(ifs, interface);
            std::system("rm -f iface.out");
            ifs.close();

            /* Get the MAC address */
            std::string command("cat /sys/class/net/");
            command.append(interface)
                    .append("/address > mac.out");
            if (std::system(command.c_str()) == -1)
                ConfigParserException("No such interface: " + std::string(interface));
            ifs.open("mac.out");
            getline(ifs, mac);
            std::transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
            std::system("rm -f mac.out");
            ifs.close();

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

    std::string Config::getIface() const { return interface; }

    std::string Config::getHostname() const { return hostname; }

    std::string Config::getIpAddress() const { return ip; }

    std::string Config::getMacAddress() const { return mac; }

    HandlerType Config::getHandlerType() const { return handlerType; }
}