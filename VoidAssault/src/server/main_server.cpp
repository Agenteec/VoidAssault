#include "../engine/ServerHost.h"
#include <iostream>
#include <string>

int main() {
    srand(time(NULL));
    if (enet_initialize() != 0) {
        std::cerr << "An error occurred while initializing ENet.\n";
        return 1;
    }
//    std::string configPath = "./";
//
//#if defined(ANDROID) || defined(PLATFORM_ANDROID)
//
//    configPath = GetApplicationDirectory();
//#endif
//
//    ConfigManager::Initialize(configPath);


    
#ifdef WIN32
    InitWindow(100, 100, "ServerHeadless");
    SetWindowState(FLAG_WINDOW_HIDDEN);
#endif // WIN32



    ServerHost server;
    if (server.Start(7777)) {
        std::cout << "Dedicated Server started on port 7777. Type 'quit' to stop.\n";

        std::string cmd;
        while (std::cin >> cmd) {
            if (cmd == "quit") break;
        }

        server.Stop();
    }
    enet_deinitialize();
    return 0;
}