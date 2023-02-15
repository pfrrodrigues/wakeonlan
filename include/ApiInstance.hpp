#pragma once
#include <memory>
#include <Config.hpp>

class ApiInstanceImpl;

class ApiInstance {
public:
    explicit ApiInstance(const Config &config);

    ~ApiInstance();

    void run();

    void stop();
private:
    std::unique_ptr<ApiInstanceImpl> impl;
};
