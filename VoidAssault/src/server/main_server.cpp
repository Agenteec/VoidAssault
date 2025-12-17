#include "../engine/ServerHost.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

std::atomic<bool> keepRunning{ true };

void SignalHandler(int signum) {
    std::cout << "\nSignal " << signum << " received. Stopping server...\n";
    keepRunning = false;
}

int main() {
    srand(time(NULL));
    if (enet_initialize() != 0) {
        std::cerr << "An error occurred while initializing ENet.\n";
        return 1;
    }

#ifdef WIN32
    InitWindow(100, 100, "ServerHeadless");
    SetWindowState(FLAG_WINDOW_HIDDEN);
#else
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
#endif

    ServerHost server;
    if (server.Start(7777, 128)) {
        std::cout << "Dedicated Server started on port 7777.\n";

#ifdef WIN32
        std::cout << "Type 'quit' to stop.\n";
        std::string cmd;
        while (std::cin >> cmd) {
            if (cmd == "quit") break;
        }
#else
        while (keepRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
#endif

        server.Stop();
    }
    enet_deinitialize();
    return 0;
}