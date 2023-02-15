#include <memory>
#include <unistd.h>
#include <csignal>
#include <ApiInstance.hpp>

bool quit = false;

void signalHandler(int signum) {
    quit = true;
}

void terminate() {
    pid_t pid = getpid();
    std::string killCmd("kill ");
    killCmd.append(std::to_string(pid));
    system(killCmd.c_str());
}

int main(int argc, char** argv) {
    signal(SIGINT, signalHandler);
    /*************************************
    * Setting up the API configuration
    *************************************/
    /*
     * Example calling syntax
     * [] - optional
     * <> - mandatory
     * ./app ['manager'] <interface>
     * */
    WakeOnLan::Config config(argv, argc);

    /*************************************
    * API instantiation
    *************************************/
    WakeOnLan::ApiInstance api(config);

    /*************************************
    * Run the API
    *************************************/
    api.run();
    while (!quit) {}
    api.stop();
    terminate();
}