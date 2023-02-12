#include <../src/common/Table.hpp>
#include <../src/service/MonitoringService.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
    WakeOnLanImpl::Table &table = WakeOnLanImpl::Table::get();
    Config config(argv, argc);
    std::shared_ptr<WakeOnLanImpl::NetworkHandler> networkHandler;
    networkHandler = std::make_shared<WakeOnLanImpl::NetworkHandler>(4000, config);
    struct WakeOnLanImpl::Table::Participant p0 = {.hostname = "picard",
                                                   .ip = "192.168.0.110",
                                                   .mac = "5c:cd:5b:4e:da:7c",
                                                   .status = WakeOnLanImpl::Table::ParticipantStatus::Awaken },
                                             p1 = {.hostname = "p1",
                                                   .ip = "127.0.0.1",
                                                   .mac = "FF:FF:FF:FF:FF:FF",
                                                   .status = WakeOnLanImpl::Table::ParticipantStatus::Sleeping },
                                             p2 = {.hostname = "p2",
                                                   .ip = "127.0.0.1",
                                                   .mac = "FF:FF:FF:FF:FF:FF",
                                                   .status = WakeOnLanImpl::Table::ParticipantStatus::Awaken };

    if(config.getHandlerType() == HandlerType::Manager)
    {
        WakeOnLanImpl::MonitoringService monitoring(table, networkHandler);
        sleep(1);
        table.insert(p0);
        std::cout << "inserted participant in table" << std::endl;
        monitoring.run();
        // sleep(5);
        // table.insert(p1);
        // std::cout << "inserted p1" << std::endl;
        // table.insert(p2);
        // std::cout << "inserted p2" << std::endl;

    }
    else
    {
        WakeOnLanImpl::MonitoringService monitoring(table, networkHandler);
        monitoring.run();
        // sleep(4);
        table.insert(p0);
        std::cout << "inserted manager in table" << std::endl;
    }
    return 0;
}