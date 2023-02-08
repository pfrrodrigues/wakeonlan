#pragma once
#include <memory>
#include <iostream>
#include <../src/common/Table.hpp>
#include <../src/handler/NetworkHandler.hpp>
#include <../src/service/DiscoveryService.hpp>
#include <../src/service/ManagementService.hpp>
#include <../src/service/MonitoringService.hpp>
#include <../src/service/InterfaceService.hpp>

class Handler {
public:
    virtual void run();
private:
    // TODO: include services and the network handler here
};

class ManagerHandler : public Handler {
public:
    void run() override;
private:
    // TODO: include the table here
};

class ParticipantHandler : public Handler {
public:
    void run() override;
private:
};