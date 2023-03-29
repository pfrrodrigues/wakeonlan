#include <mutex>
#include <ApiInstance.hpp>
#include <../src/impl/ApiInstanceImpl.hpp>
#include <../src/common/Table.hpp>
using namespace WakeOnLanImpl;

namespace WakeOnLan {
    ApiInstance::ApiInstance(const Config &config) {
//        if (!config.isValid())
//            throw std::runtime_error("Config was not properly configured");
        impl = std::make_unique<ApiInstanceImpl>(config);
        std::mutex mtx;
        std::lock_guard<std::mutex> lk(mtx);
        Table::get();
    }

    ApiInstance::~ApiInstance() {}

    void ApiInstance::run() {
        impl->run();
    }

    void ApiInstance::stop() {
        impl->stop();
    }
}