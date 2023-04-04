# wakeonlan
A friendly C++ library for managing and ingress on Wake-on-LAN groups.

## Minimum Dependencies
* CMake 3.5.1
* C++ 14
* GCC 5.5.0
* Linux environment supporting the POSIX standard

## Cloning Repository and Building the library
```bash
$ git clone git@github.com:pfrrodrigues/wakeonlan.git --recurse-submodules
$ cd wakeonlan
$ mkdir build && cd build
$ cmake .. && make -j 2
```

Optionally, you can set the build type of your preference:
```bash
$ cmake -DCMAKE_BUILD_TYPE=Release .. && make
```
or
```bash
$ cmake -DCMAKE_BUILD_TYPE=Debug .. && make
```

After execute one of the above commands a shared API library will
be available on the build directory for use.

## Getting started
CMakeLists.txt generates a basic example application (wolapp) for users start 
exploring the Wake-on-LAN API. In order to run it, one can just execute the application found on
the generated build directory.
```bash
$ ./wolapp
```

### Example Application
```c++
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
    * API configuration
    *************************************/
    WakeOnLan::Config config;

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
```

### API configuration
In order to start the API, the Config struct is needed. This struct is
responsible for internally finds out the local host interface to use for receiving
incoming packets and for setting up the device configuration needed to run the service.

```c++
WakeOnLan::Config config;
```

The Config struct provides a set of functions for user checks if the configuration was constructed correctly.
* _getHostname()_ - Gets the local host hostname.
* _getIface()_ - Gets the local host interface.
* _getIpAddress()_ - Gets the local host IP address.
* _getMacAddress()_ - Gets the local host MAC address.
* _getHandlerType()_ - Gets the API handler type.

### API instantiation
Once a Config object is created users can create a ApiInstance object to instantiate the API.
```c++
WakeOnLan::ApiInstance api(config);
```

### Supported interfaces
The API abstracts and encapsulates all the behavior of the services provided by it. To start running 
the services user must just make a call to _run()_ function. At this point, the API is already listening 
on port 4000 (service port) for incoming packets and already to respond to the requests messages coming 
from the services running on other hosts.

```c++
api.run();
```
* For stop the services one can just call the _stop()_ function.
```c++
api.stop();
```

### The API log file
The wakeonlan API provides a log file for users checks the API behavior and main events occurred. The 
log file is named _wakeonlan-api.log_ and can be found on the _log/_ folder created inside the 
directory where the program was executed. On the example, _logs/_ folder was created inside the
_build/_ directory.

Example:
```
[15-02-2023 19:37:06] [info] ---------------- Wake on Lan API ---------------
[15-02-2023 19:37:06] [info] Version 1.0.0
[15-02-2023 19:37:06] [info] Handler: Manager
[15-02-2023 19:37:06] [info] Hostname: sisop-2
[15-02-2023 19:37:06] [info] IP Address: 192.168.1.10
[15-02-2023 19:37:06] [info] MAC Address: 00:D5:7D:3C:54:A9
[15-02-2023 19:37:06] [info] Start Network handler
[15-02-2023 19:37:06] [info] Network handler (internal): listening on port 4000
[15-02-2023 19:37:06] [info] Starting handler
[15-02-2023 19:37:06] [info] Start Interface service
[15-02-2023 19:37:06] [info] Start Discovery service
[15-02-2023 19:37:06] [info] STATUS UPDATE: Synchronized
[15-02-2023 19:37:06] [info] Start Monitoring service
[15-02-2023 19:37:08] [info] Network handler (internal): listener socket is closed
[15-02-2023 19:37:09] [info] Stop Network handler
[15-02-2023 19:37:10] [info] Stop Discovery service
[15-02-2023 19:37:10] [info] Stop Monitoring service
[15-02-2023 19:37:10] [info] Stop Interface service
[15-02-2023 19:37:11] [info] Stopping handler
```
