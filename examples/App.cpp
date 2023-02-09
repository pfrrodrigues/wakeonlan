#include <tabulate/table.hpp>
#include <../src/common/Table.hpp>
#include <../src/service/InterfaceService.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
    WakeOnLanImpl::Table &table = WakeOnLanImpl::Table::get();
    Config config(argv, argc);
    std::shared_ptr<WakeOnLanImpl::NetworkHandler> networkHandler;
    networkHandler = std::make_shared<WakeOnLanImpl::NetworkHandler>(4000, config);
    struct WakeOnLanImpl::Table::Participant p0 = {.hostname = "p0",
                                                   .ip = "127.0.0.1",
                                                   .mac = "FF:FF:FF:FF:FF:FF",
                                                   .status = WakeOnLanImpl::Table::ParticipantStatus::Awaken },
                                             p1 = {.hostname = "p1",
                                                   .ip = "127.0.0.1",
                                                   .mac = "FF:FF:FF:FF:FF:FF",
                                                   .status = WakeOnLanImpl::Table::ParticipantStatus::Sleeping },
                                             p2 = {.hostname = "p2",
                                                   .ip = "127.0.0.1",
                                                   .mac = "FF:FF:FF:FF:FF:FF",
                                                   .status = WakeOnLanImpl::Table::ParticipantStatus::Awaken };

    // WakeOnLanImpl::ParticipantInterfaceService interface(table, networkHandler);
    WakeOnLanImpl::ManagerInterfaceService interface(table, networkHandler);
    interface.run();


    sleep(2);
    table.insert(p0);
    sleep(2);
    table.insert(p1);
    sleep(2);
    table.insert(p2);
    while(true){}

    return 0;
}
