#pragma once
#include <mutex>
#include <queue>
#include <thread>
#include <memory>
#include <string>
#include <cstring>
#include <../src/common/UdpSocket.hpp>
#include <../include/Config.hpp>
#include <spdlog/spdlog.h>

using namespace WakeOnLan;

namespace WakeOnLanImpl {

    /**
    * @enum ServiceGlobalStatus
    * The global status of the services. Used by the API services for controlling their internal behavior.
    */
    enum ServiceGlobalStatus {
        WaitingForSync = 0,     ///< Indicates a SleepServiceDiscovery request was not received yet.
        Syncing = 1,            ///< Indicates a SleepServiceDiscovery request was received and a response to that was sent.
        Synchronized = 2,       ///< Indicates a SleepStatusRequest was receive. At this point, a participant has sure the manager add it on the group table.
        Unknown = 3,            ///< Indicates the initial state of the services.
        ManagerFailure = 4      ///< Indicates that a service has detected a failure in the Manager. 
    };

    /**
     * @class NetworkHandler
     * This class is responsible for providing a broker engine to the API services,
     * abstracting the network handling from it through the encoding and decoding from
     * messages/network format to network format/messages. The handler creates a dedicated
     * thread to binds a socket using the port passed to the class constructor for
     * receiving messages designated to the running services. Messages are buffered on internal
     * queues and a class user can recovery messages received by the handler calling the appropriated functions.
     */
    class NetworkHandler {
    public:
        /**
         * NetworkHandler constructor
         * @param port The port used in the bound socket created to receive messages.
         * @param config The host configuration
         */
        explicit NetworkHandler(const uint32_t &port, const Config &config);

        /**
         * NetworkHandler destructor
         */
        ~NetworkHandler();

        /**
        * Sends a message to the network. The function uses the IP passed as parameters to set
        * the destination IP address. The handler sets the destination port based on the the port
        * specified in the class constructor, creating a communication channel between hosts running
        * the same service in the same port. An user of the function can send broadcast messages
        * passing the address '255.255.255.255' on the ip parameter.
        *
        * @param message A reference to a Message to be sent to the network.
        * @returns A bool indicating the message was sent.
        */
        bool send(const Message &message, const std::string &ip);

        /**
         * Gets the older Message from the Discovery service queue. Messages of type SleepServiceDiscovery and
         * SleepServiceExit received by the handler are placed on a dedicated queue and can be gotten through this
         * function. Every message returned by the function is removed from the queue, so reading from the queue
         * is a unitary operation on the perspective of the specific message being recovered. In case of the queue
         * is empty a nullptr is returned, indicating there are not messages in the queue.
         *
         * @return a pointer to a Message received by the handler and designated to the Discovery service.
         */
        Message* getFromDiscoveryQueue();

        /**
         * Gets the older Message from the Monitoring service queue. Messages of type SleepStatusRequest
         * received by the handler are placed on a dedicated queue and can be gotten through this
         * function. Every message returned by the function is removed from the queue, so reading from the queue
         * is a unitary operation on the perspective of the specific message being recovered. In case of the queue
         * is empty a nullptr is returned, indicating there are not messages in the queue.
         *
         * @returns a pointer to a Message received by the handler and designated to the Monitoring service.
         */
        Message* getFromMonitoringQueue();

        /**
         * Sends a WOL packet to a target host.
         * @param mac A MAC address in string format.
         * @returns A bool indicating the WOL packet was sent.
         */
        bool wakeUp(const std::string &mac);

        /**
         * Gets the device config.
         * @returns A const Config reference.
         */
        const Config & getDeviceConfig();

        /**
         * Updates the global service status.
         * @returns None.
         */
        void changeStatus(const ServiceGlobalStatus &gs);

        /**
         * Gets the global status.
         * @returns A const ServiceGlobalStatus reference.
         */
        const ServiceGlobalStatus& getGlobalStatus();

        /**
         * Stops the Network handler.
         * @return None.
         */
        void stop();
    private:
        std::unique_ptr<std::thread> t;         ///< The thread used to receive messages.
        std::mutex inetMutex;                   ///< The mutex for controlling internal issues.
        std::queue<Message> discoveryQueue;     ///< The queue buffering messages designated to the Discovery service.
        std::queue<Message> monitoringQueue;    ///< The queue buffering messages designated to the Monitoring service.
        uint32_t  port;                         ///< The port the handler service is running on.
        Config config;                          ///< The API configuration.
        ServiceGlobalStatus globalStatus;       ///< The services global status.
        std::mutex gsMutex;                     ///< The mutex used to handle global status access.
        std::shared_ptr<spdlog::logger> log;    ///< The Network handler logger.
        bool active;                            ///< The bool indicating whether service is active or not.
    };
} // namespace WakeOnLanImpl