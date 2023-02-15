#pragma once
#include <memory>
#include <Config.hpp>

namespace WakeOnLanImpl {
    class ApiInstanceImpl;
}

namespace WakeOnLan {
    class ApiInstance {
    public:
        explicit ApiInstance(const Config &config);

        ~ApiInstance();

        void run();

        void stop();
    private:
        std::unique_ptr<WakeOnLanImpl::ApiInstanceImpl> impl;
    };
} // namespace WakeOnLan
