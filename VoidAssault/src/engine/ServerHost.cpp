#define _CRT_SECURE_NO_WARNINGS
#include "ServerHost.h"
#include "../common/NetworkPackets.h"
#include <iostream>
#include <chrono>

ServerHost::ServerHost() : running(false) {
    // Выделяем память под сервер
    netServer = ENetServer::alloc();
}

ServerHost::~ServerHost() {
    Stop();
}

bool ServerHost::Start(int port) {
    if (running) return false;

    // Запуск через обертку
    if (!netServer->start(port)) {
        std::cerr << "SERVER: Failed to start on port " << port << std::endl;
        return false;
    }

    std::cout << "SERVER: Started on port " << port << std::endl;
    running = true;
    serverThread = std::thread(&ServerHost::ServerLoop, this);
    return true;
}

void ServerHost::Stop() {
    running = false;
    if (serverThread.joinable()) {
        serverThread.join();
    }
    // Остановка обертки
    netServer->stop();
}

void ServerHost::ServerLoop() {
    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();
    double accumulator = 0.0;
    const double dt = 1.0 / 60.0;
    double snapshotTimer = 0.0;

    while (running && netServer->isRunning()) {
        auto currentTime = clock::now();
        std::chrono::duration<double> frameTime = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += frameTime.count();
        snapshotTimer += frameTime.count();

        // 1. Обработка входящих сообщений
        auto msgs = netServer->poll();

        for (auto& msg : msgs) {
            uint32_t peerId = msg->peerId();

            if (msg->type() == MessageType::CONNECT) {
                std::cout << "SERVER: Client " << peerId << " connected.\n";

                // Создаем игрока, используя ID пира
                // (Придется доработать CreatePlayer в GameScene, см. ниже, 
                // или просто передадим этот ID)
                auto player = gameScene.CreatePlayerWithId(peerId);

                // Отправляем INIT пакет
                auto stream = StreamBuffer::alloc();
                stream->write((uint8_t)GamePacket::INIT);
                stream->write(player->id);

                // Шлем надежно конкретному клиенту
                netServer->send(peerId, DeliveryType::RELIABLE, stream);
            }
            else if (msg->type() == MessageType::DISCONNECT) {
                std::cout << "SERVER: Client " << peerId << " disconnected.\n";
                // Удаляем игрока
                gameScene.objects.erase(peerId);
            }
            else if (msg->type() == MessageType::DATA) {
                // Читаем Payload
                auto stream = msg->stream();
                uint8_t packetType = 0;
                stream->read(packetType);

                if (packetType == GamePacket::INPUT) {
                    PlayerInputPacket input;
                    stream >> input; // Десериализация

                    if (gameScene.objects.count(peerId)) {
                        auto p = std::dynamic_pointer_cast<Player>(gameScene.objects[peerId]);
                        if (p) {
                            p->ApplyInput(input.movement);
                            p->aimTarget = input.aimTarget;
                            p->wantsToShoot = input.isShooting;
                        }
                    }
                }
            }
        }

        // 2. Обновление мира
        while (accumulator >= dt) {
            gameScene.Update((float)dt);
            accumulator -= dt;
        }

        // 3. Рассылка состояния (Snapshot) - 20 раз в секунду
        if (snapshotTimer >= 0.05) {
            BroadcastSnapshot();
            snapshotTimer = 0;
        }

        // Разгрузка CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void ServerHost::BroadcastSnapshot() {
    if (netServer->numClients() == 0) return;

    auto stream = StreamBuffer::alloc(4096); // Буфер побольше

    stream->write((uint8_t)GamePacket::SNAPSHOT);
    stream->write((uint32_t)gameScene.objects.size());

    for (auto& [id, obj] : gameScene.objects) {
        EntityState state;
        state.id = obj->id;

        if (obj->body) {
            state.position = ToRay(cpBodyGetPosition(obj->body));
            state.rotation = (float)cpBodyGetAngle(obj->body) * RAD2DEG;
        }
        else {
            state.position = { 0,0 };
            state.rotation = 0;
        }

        state.health = obj->health;
        state.maxHealth = obj->maxHealth;
        state.type = obj->type;
        state.radius = (obj->type == EntityType::BULLET) ? 5.0f : 20.0f;
        state.color = obj->color;

        stream << state;
    }

    netServer->broadcast(DeliveryType::UNRELIABLE, stream);
}