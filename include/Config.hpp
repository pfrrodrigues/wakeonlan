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
          */
        Config();

        /**
         * Gets the hostname.
         *
         * @returns A string containing the hostname.
         */
        std::string getHostname() const;

        /**
         * Gets the IP address.
         *
         * @returns A string containing the IP address.
         */
        std::string getIpAddress() const;

        /**
         * Gets the MAC address.
         *
         * @returns A string containing the MAC address.
         */
        std::string getMacAddress() const;

        /**
         * Gets the interface.
         *
         * @returns A string containing the interface.
         */
        std::string getIface() const;

        /**
         * Gets the handler type.
         *
         * @returns A enum containing the handler type.
         */
        HandlerType getHandlerType() const;
    private:
        HandlerType handlerType; ///< The configured handler type. Default is Participant.
        std::string hostname;    ///< The hostname of the local host.
        std::string ip;          ///< The local host IP address.
        std::string mac;         ///< The local host MAC address.
        std::string interface;   ///< The local host interface for receiving incoming packets.
    };
} // namespace WakeOnLan
