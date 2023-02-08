#include <tabulate/table.hpp>
#include <../src/common/Table.hpp>
#include <../src/service/InterfaceService.hpp>
#include <iostream>

int main(int argc, char const *argv[])
{
    WakeOnLanImpl::Table &table = WakeOnLanImpl::Table::get();
    std::shared_ptr<WakeOnLanImpl::NetworkHandler> networkHandler;

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
    if (!table.insert(p0))
        std::cout << "Failed to insert\n";
    if (!table.insert(p1))
        std::cout << "Failed to insert\n";
    if (!table.insert(p2))
        std::cout << "Failed to insert\n";

    WakeOnLanImpl::InterfaceService interface(table, networkHandler);
    interface.run();

    return 0;
}
