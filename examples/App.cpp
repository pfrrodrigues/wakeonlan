#include <iostream>
#include <memory>
#include <ApiInstance.hpp>

int main(int argc, char** argv) {
    /*************************************
    * Setting up the API configuration
    *************************************/
    /*
     * Example calling syntax
     * [] - optional
     * <> - mandatory
     * ./app ['manager'] <interface>
     * */
    Config config(argv, argc);

    /*************************************
    * API instantiation
    *************************************/
    ApiInstance api(config);

    /*************************************
    * Run the API
    *************************************/
    api.run();
}