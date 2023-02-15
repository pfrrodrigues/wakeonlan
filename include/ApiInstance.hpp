#pragma once
#include <memory>
#include <Config.hpp>

namespace WakeOnLanImpl {
    class ApiInstanceImpl;
}

namespace WakeOnLan {
    /**
     * @class ApiInstance
     * This class is the entry point of the Wake-on-LAN API. The class functions are
     * the interfaces an user can use to send commands to change the internal state of the API.
     */
    class ApiInstance {
    public:
        /**
         * ApiInstance constructor.
         * When called the initial context of the API is constructed, including the Network handler for
         * receiving incoming packets, the modules loggers, and the API handler.
         * @param config The local host configuration.
         */
        explicit ApiInstance(const Config &config);

        /**
         * ApiInstance destructor.
         */
        ~ApiInstance();

        /**
         * Runs the services provided by the API. When called, all the main services (discovery, monitoring,
         * and interface) start running. On this point, host is already listening  or transmitting packets on
         * the Wake-on-LAN API preconfigured port (4000) for starting the manager-participants communication.
         *
         * @returns None.
        */
        void run();

        /**
         * Stops the services provided by the API. When called, all the main services (discovery, monitoring,
         * and interface) stop running. On this point, one by one the services clear its contexts before
         * closes the API handler.
         *
         * @returns None.
        */
        void stop();
    private:
        std::unique_ptr<WakeOnLanImpl::ApiInstanceImpl> impl; ///< The API implementation wrapper.
    };
} // namespace WakeOnLan
