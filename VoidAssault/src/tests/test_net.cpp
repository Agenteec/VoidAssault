#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include "enet/ENetServer.h"
#include "enet/ENetClient.h"


std::atomic<bool> serverRunning{ false };


std::atomic<bool> running = false;


int mainTest() {
    if (enet_initialize() != 0) {
        printf("An error occurred while initializing ENet.\n");
        return 1;
    }

    ENetAddress address = { 0 };

    address.host = ENET_HOST_ANY; /* Bind the server to the default localhost.     */
    address.port = 7777; /* Bind the server to port 7777. */

#define MAX_CLIENTS 32

    /* create a server */
    ENetHost* server = enet_host_create(&address, MAX_CLIENTS, 2, 0, 0);

    if (server == NULL) {
        printf("An error occurred while trying to create an ENet server host.\n");
        return 1;
    }
    running.store(true);
    printf("Started a server...\n");


    std::thread([&]() {
        ENetEvent event;
        while (enet_host_service(server, &event, 1000) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n", event.peer->address.host, event.peer->address.port);
                /* Store any relevant client information here. */
                event.peer->data = (void*)"Client information";
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                printf("A packet of length %lu containing %s was received from %s on channel %u.\n",
                    event.packet->dataLength,
                    event.packet->data,
                    event.peer->data,
                    event.channelID);
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("%s disconnected.\n", event.peer->data);
                /* Reset the peer's client information. */
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                printf("%s disconnected due to timeout.\n", event.peer->data);
                /* Reset the peer's client information. */
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
            }
        }
        }).detach();
    while (!running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    ENetHost* client = { 0 };
    client = enet_host_create(NULL /* create a client host */,
        1 /* only allow 1 outgoing connection */,
        2 /* allow up 2 channels to be used, 0 and 1 */,
        0 /* assume any amount of incoming bandwidth */,
        0 /* assume any amount of outgoing bandwidth */);
    if (client == NULL) {
        fprintf(stderr,
            "An error occurred while trying to create an ENet client host.\n");
        exit(EXIT_FAILURE);
    }

    //ENetAddress address = { 0 };
    ENetEvent event;
    ENetPeer* peer = { 0 };
    /* Connect to some.server.net:1234. */
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 7777;
    /* Initiate the connection, allocating the two channels 0 and 1. */
    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL) {
        fprintf(stderr,
            "No available peers for initiating an ENet connection.\n");
        exit(EXIT_FAILURE);
    }
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        puts("Connection to some.server.net:1234 succeeded.");
    }
    else {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);
        puts("Connection to some.server.net:1234 failed.");
    }

    // Receive some events
    enet_host_service(client, &event, 5000);

    // Disconnect
    enet_peer_disconnect(peer, 0);

    uint8_t disconnected = false;
    /* Allow up to 3 seconds for the disconnect to succeed
     * and drop any packets received packets.
     */
    while (enet_host_service(client, &event, 3000) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            puts("Disconnection succeeded.");
            disconnected = true;
            break;
        }
    }

    // Drop connection, since disconnection didn't successed
    if (!disconnected) {
        enet_peer_reset(peer);
    }

    enet_host_destroy(client);
    enet_deinitialize();
    return 0;
}



int mainClient() {


    if (enet_initialize() != 0) {
        std::cerr << "FATAL: Failed to initialize ENet!" << std::endl;
        return -1;
    }
    ENetClient::Shared client;
    ENetServer::Shared server;
    //atexit(enet_deinitialize);

    std::cout << "--- STARTING NETWORK TEST ---" << std::endl;
    int port = 7777;
    server = ENetServer::alloc();
    if (!server->start(port)) {
        std::cerr << "SERVER: Failed to start on port " << port << "!" << std::endl;
        return 1;
    }
    std::cout << "SERVER: Listening on port " << port << std::endl;
    serverRunning = true;
    std::thread sThread([&]() {


        while (serverRunning) {
            auto msgs = server->poll();

            for (auto& msg : msgs) {
                if (msg->type() == MessageType::CONNECT) {
                    std::cout << "SERVER: Client connected! ID: " << msg->peerId() << std::endl;
                }
                else if (msg->type() == MessageType::DATA) {
                    std::string s;
                    msg->stream()->read(s);
                    std::cout << "SERVER: Received data: " << s << std::endl;
                }
                else if (msg->type() == MessageType::DISCONNECT) {
                    std::cout << "SERVER: Client disconnected." << std::endl;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        server->stop();
        server.reset();
        std::cout << "SERVER: Stopped." << std::endl;
        
        
        
        });

    while (!serverRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    client = ENetClient::alloc();
    std::cout << "CLIENT: Connecting to 127.0.0.1:" << port << "..." << std::endl;

    if (client->connect("127.0.0.1", port)) {
        std::cout << "CLIENT: Connected successfully!" << std::endl;

        auto stream = StreamBuffer::alloc();
        stream->write(std::string("Hello Server!"));
        client->send(DeliveryType::RELIABLE, stream);
        std::cout << "CLIENT: Data sent." << std::endl;

        std::cout << "CLIENT: Polling loop started..." << std::endl;
        for (int i = 0; i < 50; i++) {
            client->poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    else {
        std::cerr << "CLIENT: Failed to connect. Check firewall or server status." << std::endl;
    }
    std::cout << "--- STOPPING ---" << std::endl;
    serverRunning = false;

    if (sThread.joinable()) {
        sThread.join();
    }

    client.reset();
    

    std::cout << "--- TEST FINISHED ---" << std::endl;
    return 0;
}


int main()
{
    //mainTest();
    mainClient();
}