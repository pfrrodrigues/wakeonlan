#pragma once
#include <string>
#include <../include/Types.hpp>

namespace WakeOnLan {
    /**
     * @class Config
     * The Config class is responsible for parsing the parameters provided to its constructor in order to
     * set the configuration used on the initial state of the API.
     */
    class Config {
    public:
         /**
          * Config constructor
          *
          * @param params The parameters provided for Config. In case of the API instance must be a
          * manager handler, the parameters must be 'manager <interface>' and quantity equals 2. Otherwise,
          * params[0] contains the interface name to bind and quantity equals 1.
          * @param quantity The parameters quantity.
          */
        Config(char** params, const int &quantity);

        /**
         * Gets the hostname.
         *
         * @returns A string containing the hostname.
         */
        std::string getHostname();

        /**
         * Gets the IP address.
         *
         * @returns A string containing the IP address.
         */
        std::string getIpAddress();

        /**
         * Gets the MAC address.
         *
         * @returns A string containing the MAC address.
         */
        std::string getMacAddress();

        /**
         * Gets the interface.
         *
         * @returns A string containing the interface.
         */
        std::string getIface();

        /**
         * Gets the handler type.
         *
         * @returns A enum containing the handler type.
         */
        HandlerType getHandlerType();
        
        void setHandlerType(const HandlerType &ht);
    private:
        HandlerType handlerType; ///< The configured handler type. Default is Participant.
        std::string hostname;    ///< The hostname of the local host.
        std::string ip;          ///< The local host IP address.
        std::string mac;         ///< The local host MAC address.
        std::string interface;   ///< The local host interface for receiving incoming packets.
    };
} // namespace WakeOnLan
