#pragma once
#include <memory>
#include <iostream>
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <../src/service/DiscoveryService.hpp>
#include <../src/service/MonitoringService.hpp>
#include <../src/service/InterfaceService.hpp>

class Handler {
public:
    virtual void run();
    Handler();
    virtual ~Handler();
};

class ManagerHandler : public Handler {
public:
    void run() override;
    explicit ManagerHandler(const Config &config, WakeOnLanImpl::Table& = WakeOnLanImpl::Table::get());
    virtual ~ManagerHandler();
private:
    std::unique_ptr<WakeOnLanImpl::DiscoveryService> discoveryService;
    std::unique_ptr<WakeOnLanImpl::MonitoringService> monitoringService;
    std::unique_ptr<WakeOnLanImpl::ManagerInterfaceService> interfaceService;
    std::shared_ptr<WakeOnLanImpl::NetworkHandler> networkHandler;
    WakeOnLanImpl::Table& table;
    Config config;
};

class ParticipantHandler : public Handler {
public:
    void run() override;
    explicit ParticipantHandler(const Config &config, WakeOnLanImpl::Table& = WakeOnLanImpl::Table::get());
    virtual ~ParticipantHandler();
private:
    std::unique_ptr<WakeOnLanImpl::DiscoveryService> discoveryService;
    std::unique_ptr<WakeOnLanImpl::MonitoringService> monitoringService;
    std::unique_ptr<WakeOnLanImpl::ParticipantInterfaceService> interfaceService;
    std::shared_ptr<WakeOnLanImpl::NetworkHandler> networkHandler;
    WakeOnLanImpl::Table& table;
    Config config;
};