#define _CRT_SECURE_NO_WARNINGS
#include "ServerHost.h"
#include "../common/NetworkPackets.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include "Utils/ConfigManager.h"

#if defined(__linux__) || defined(__APPLE__)
#include <signal.h>
#endif

// Проверка: является ли ID клиента "виртуальным" (Relay)
static bool IsRelayClient(uint32_t id, const std::vector<uint32_t>& relayIds) {
    return std::find(relayIds.begin(), relayIds.end(), id) != relayIds.end();
}

double GetSystemTime() {
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = now - start;
    return diff.count();
}

ServerHost::ServerHost() : running(false) {
    netServer = ENetServer::alloc();
    masterClient = ENetClient::alloc();
}

ServerHost::~ServerHost() {
    Stop();
}

bool ServerHost::Start(int port, bool registerOnMaster = true) {
    if (running) return false;

#if defined(__linux__) || defined(__APPLE__)
    signal(SIGPIPE, SIG_IGN);
#endif

    ServerConfig& cfg = ConfigManager::GetServer();
    gameScene.pvpFactor = cfg.pvpDamageFactor;
    useMasterServer = registerOnMaster;
    if (!netServer->start(port, cfg.maxPlayers)) return false;

    std::cout << "SERVER: Started on port " << port << (useMasterServer ? " (Public)" : " (Offline/LAN)") << std::endl;
    std::cout << "SERVER: Name '" << cfg.serverName << "', Max Players: " << cfg.maxPlayers << "\n";

    running = true;
    serverThread = std::thread(&ServerHost::ServerLoop, this);
    return true;
}

void ServerHost::Stop() {
    running = false;
    if (serverThread.joinable()) serverThread.join();
    netServer->stop();
    if (masterClient) masterClient->disconnect();
    relayClientIds.clear();
}

void ServerHost::RegisterWithMaster() {
    if (!useMasterServer) return;
    ClientConfig& cCfg = ConfigManager::GetClient();
    ServerConfig& sCfg = ConfigManager::GetServer();

    if (masterClient->connect(cCfg.masterServerIp, cCfg.masterServerPort)) {
        std::cout << "SERVER: Connected to Master Server at " << cCfg.masterServerIp << ":" << cCfg.masterServerPort << "\n";
        connectedToMaster = true;

        Buffer buffer; OutputAdapter adapter(buffer);
        bitsery::Serializer<OutputAdapter> serializer(std::move(adapter));
        serializer.value1b(GamePacket::MASTER_REGISTER);

        MasterRegisterPacket pkt;
        pkt.gamePort = (uint16_t)ConfigManager::GetServer().port;
        pkt.serverName = sCfg.serverName;
        pkt.maxPlayers = (uint8_t)sCfg.maxPlayers;

        serializer.object(pkt);
        serializer.adapter().flush();
        masterClient->send(DeliveryType::RELIABLE, StreamBuffer::alloc(buffer.data(), buffer.size()));
    }
    else {
        std::cout << "SERVER: Failed to connect to Master Server (Relay unavailable).\n";
        connectedToMaster = false;
    }
}

void ServerHost::UpdateMasterHeartbeat(float dt) {
    if (!useMasterServer || !connectedToMaster) return;

    masterHeartbeatTimer += dt;
    if (masterHeartbeatTimer >= 5.0f) {
        masterHeartbeatTimer = 0.0f;

        int playerCount = 0;
        for (const auto& p : gameScene.objects) {
            if (p.second->type == EntityType::PLAYER) playerCount++;
        }

        Buffer buffer; OutputAdapter adapter(buffer);
        bitsery::Serializer<OutputAdapter> serializer(std::move(adapter));
        serializer.value1b(GamePacket::MASTER_HEARTBEAT);

        MasterHeartbeatPacket pkt;
        pkt.currentPlayers = (uint8_t)playerCount;
        pkt.wave = (uint8_t)waveCount;

        serializer.object(pkt);
        serializer.adapter().flush();
        masterClient->send(DeliveryType::UNRELIABLE, StreamBuffer::alloc(buffer.data(), buffer.size()));
    }
}

void ServerHost::SendToClient(uint32_t peerId, DeliveryType type, StreamBuffer::Shared stream) {
    if (IsRelayClient(peerId, relayClientIds)) {
        RelayPacket rp;
        rp.targetId = peerId;
        rp.isReliable = (type == DeliveryType::RELIABLE);
        rp.data = stream->buffer();

        Buffer buf; OutputAdapter ad(buf);
        bitsery::Serializer<OutputAdapter> ser(std::move(ad));
        ser.value1b(GamePacket::RELAY_TO_CLIENT);
        ser.object(rp);
        ser.adapter().flush();

        if (masterClient && masterClient->isConnected())
            masterClient->send(type, StreamBuffer::alloc(buf.data(), buf.size()));
    }
    else {
        netServer->send(peerId, type, stream);
    }
}

void ServerHost::ProcessGamePacket(uint32_t peerId, StreamBuffer::Shared stream) {
    const auto& buf = stream->buffer();
    size_t offset = stream->tellg();
    if (buf.empty() || offset >= buf.size()) return;

    InputAdapter ia(buf.begin() + offset, buf.end());
    bitsery::Deserializer<InputAdapter> des(std::move(ia));
    uint8_t type; des.value1b(type);

    if (type == GamePacket::JOIN) {
        JoinPacket pkt; des.object(pkt);
        if (des.adapter().error() == bitsery::ReaderError::NoError) {
            // Создаем или обновляем игрока
            if (!gameScene.objects.count(peerId)) {
                std::cout << "Client joined (ID: " << peerId << ")\n";
                auto player = gameScene.CreatePlayerWithId(peerId);
                player->name = pkt.name;

                // Шлем INIT
                InitPacket initPkt; initPkt.playerId = player->id;
                Buffer outBuf; OutputAdapter ad(outBuf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
                ser.value1b(GamePacket::INIT); ser.object(initPkt); ser.adapter().flush();

                SendToClient(peerId, DeliveryType::RELIABLE, StreamBuffer::alloc(outBuf.data(), outBuf.size()));
            }
            else {
                auto p = std::dynamic_pointer_cast<Player>(gameScene.objects[peerId]);
                if (p) p->name = pkt.name;
            }
        }
    }
    else if (type == GamePacket::INPUT) {
        PlayerInputPacket inp; des.object(inp);
        if (des.adapter().error() == bitsery::ReaderError::NoError) {
            if (gameScene.objects.count(peerId)) {
                auto p = std::dynamic_pointer_cast<Player>(gameScene.objects[peerId]);
                if (p) {
                    p->ApplyInput(inp.movement);
                    p->aimTarget = inp.aimTarget;
                    p->wantsToShoot = inp.isShooting;
                }
            }
        }
    }
    else if (type == GamePacket::ACTION) {
        ActionPacket act; des.object(act);
        if (des.adapter().error() == bitsery::ReaderError::NoError) {
            if (gameScene.objects.count(peerId)) {
                if (act.type == ActionType::UPGRADE_BUILDING) gameScene.TryUpgrade(peerId, act.target);
                else gameScene.TryBuild(peerId, act.type, act.target);
            }
        }
    }
    else if (type == GamePacket::ADMIN_CMD) {
        AdminCommandPacket pkt; des.object(pkt);
        if (des.adapter().error() == bitsery::ReaderError::NoError) gameScene.HandleAdminCommand(peerId, pkt);
    }
}

void ServerHost::ServerLoop() {
    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();
    double accumulator = 0.0;
    int tickRate = ConfigManager::GetServer().tickRate;
    double dt = 1.0 / (double)tickRate;

    double snapshotTimer = 0.0;
    double statsTimer = 0.0;

    waveCount = 1;
    waveTimer = 0.0;
    timeToNextWave = 5.0;

    RegisterWithMaster();

    while (running) {
        if (!netServer->isRunning()) break;

        auto currentTime = clock::now();
        double frameTime = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;
        if (frameTime > 0.25) frameTime = 0.25;

        // 1. DIRECT POLL
        auto msgs = netServer->poll();
        for (auto& msg : msgs) {
            uint32_t peerId = msg->peerId();
            if (msg->type() == MessageType::CONNECT) {
                std::cout << "Direct Client " << peerId << " connected.\n";
                auto player = gameScene.CreatePlayerWithId(peerId);
                InitPacket initPkt; initPkt.playerId = player->id;
                Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
                ser.value1b(GamePacket::INIT); ser.object(initPkt); ser.adapter().flush();
                netServer->send(peerId, DeliveryType::RELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
            }
            else if (msg->type() == MessageType::DISCONNECT) {
                std::cout << "Direct Client " << peerId << " disconnected.\n";
                gameScene.objects.erase(peerId);
            }
            else if (msg->type() == MessageType::DATA) {
                ProcessGamePacket(peerId, msg->stream());
            }
        }

        // 2. RELAY POLL
        UpdateMasterHeartbeat((float)frameTime);
        if (masterClient) {
            auto masterMsgs = masterClient->poll();
            for (auto& msg : masterMsgs) {
                if (msg->type() == MessageType::DATA) {
                    const auto& buf = msg->stream()->buffer();
                    size_t offset = msg->stream()->tellg();
                    if (!buf.empty()) {
                        InputAdapter ia(buf.begin() + offset, buf.end());
                        bitsery::Deserializer<InputAdapter> des(std::move(ia));
                        uint8_t type; des.value1b(type);

                        if (type == GamePacket::RELAY_TO_SERVER) {
                            RelayPacket rp; des.object(rp);
                            if (des.adapter().error() == bitsery::ReaderError::NoError) {
                                uint32_t relayId = rp.targetId;
                                if (!IsRelayClient(relayId, relayClientIds)) {
                                    relayClientIds.push_back(relayId);
                                }
                                auto innerStream = StreamBuffer::alloc(rp.data.data(), rp.data.size());
                                ProcessGamePacket(relayId, innerStream);
                            }
                        }
                    }
                }
                else if (msg->type() == MessageType::DISCONNECT) {
                    std::cout << "Disconnected from Master Server (Relay lost).\n";
                    connectedToMaster = false;
                    for (uint32_t rid : relayClientIds) gameScene.objects.erase(rid);
                    relayClientIds.clear();
                }
            }
        }

        // LOGIC START
        int totalClients = netServer->numClients() + (int)relayClientIds.size();
        if (totalClients == 0) {
            bool hasEntities = false;
            for (auto& pair : gameScene.objects) {
                if (pair.second->type != EntityType::PLAYER && pair.second->type != EntityType::WALL) {
                    hasEntities = true; break;
                }
            }
            if (waveCount > 1 || hasEntities) {
                auto it = gameScene.objects.begin();
                while (it != gameScene.objects.end()) {
                    if (it->second->type == EntityType::ENEMY || it->second->type == EntityType::BULLET) it = gameScene.objects.erase(it);
                    else ++it;
                }
                waveCount = 1; waveTimer = 0;
            }
        }

        accumulator += frameTime;
        snapshotTimer += frameTime;
        statsTimer += frameTime;
        waveTimer += frameTime;

        if (waveTimer >= timeToNextWave && totalClients > 0) {
            waveTimer = 0.0;
            timeToNextWave = 20.0 + (waveCount * 2.0);
            int currentEnemies = 0;
            for (auto& [id, obj] : gameScene.objects) { if (obj->type == EntityType::ENEMY) currentEnemies++; }
            if (currentEnemies < 120) {
                int enemiesToSpawn = 5 + (waveCount * 2);
                if (enemiesToSpawn > 60) enemiesToSpawn = 60;
                for (int i = 0; i < enemiesToSpawn; i++) gameScene.SpawnEnemy();
                if (waveCount % 5 == 0) gameScene.SpawnEnemy(EnemyType::BOSS);
                waveCount++;
            }
        }

        int maxPhysicsSteps = 5;
        int steps = 0;
        while (accumulator >= dt && steps < maxPhysicsSteps) {
            gameScene.Update((float)dt);
            accumulator -= dt;
            steps++;
        }
        if (accumulator > dt) accumulator = 0.0;

        if (!gameScene.pendingEvents.empty()) {
            for (const auto& evt : gameScene.pendingEvents) {
                Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
                ser.value1b(GamePacket::EVENT); ser.object(evt); ser.adapter().flush();
                BroadcastSnapshot(); // Hack reuse, better make explicit broadcast
                // Wait, use proper Broadcast below
                BroadcastToAll(DeliveryType::UNRELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
            }
            gameScene.pendingEvents.clear();
        }

        if (snapshotTimer >= 0.033) {
            BroadcastSnapshot();
            snapshotTimer = 0;
        }

        if (statsTimer >= 0.2) {
            for (auto& [id, obj] : gameScene.objects) {
                if (obj->type == EntityType::PLAYER) {
                    auto p = std::dynamic_pointer_cast<Player>(obj);
                    PlayerStatsPacket stats;
                    stats.level = p->level; stats.currentXp = p->currentXp; stats.maxXp = p->maxXp;
                    stats.maxHealth = p->maxHealth; stats.damage = p->curDamage; stats.speed = p->curSpeed;
                    stats.scrap = p->scrap; stats.kills = p->kills; stats.inventory.assign(std::begin(p->inventory), std::end(p->inventory));
                    stats.isAdmin = p->isAdmin;

                    Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
                    ser.value1b(GamePacket::STATS); ser.object(stats); ser.adapter().flush();
                    SendToClient(id, DeliveryType::UNRELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
                }
            }
            statsTimer = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void ServerHost::BroadcastToAll(DeliveryType type, StreamBuffer::Shared stream) {
    netServer->broadcast(type, stream);
    if (!relayClientIds.empty() && masterClient && masterClient->isConnected()) {
        for (uint32_t rid : relayClientIds) SendToClient(rid, type, stream);
    }
}

void ServerHost::BroadcastSnapshot() {
    if (netServer->numClients() == 0 && relayClientIds.empty()) return;

    WorldSnapshotPacket snap;
    snap.serverTime = GetSystemTime();
    snap.wave = waveCount;
    for (auto& [id, obj] : gameScene.objects) {
        EntityState state; state.id = obj->id;
        if (obj->body) { cpVect pos = cpBodyGetPosition(obj->body); state.position = ToRay(pos); state.rotation = obj->rotation; }
        else { state.position = { 0,0 }; state.rotation = 0; }
        state.health = obj->health; state.maxHealth = obj->maxHealth; state.type = obj->type; state.color = obj->color;
        state.level = 1; state.kills = 0; state.radius = 20.0f; state.subtype = 0; state.ownerId = 0;
        if (obj->type == EntityType::PLAYER) {
            auto p = std::dynamic_pointer_cast<Player>(obj);
            if (p) { state.level = p->level; state.kills = p->kills; state.name = p->name; }
        }
        else if (obj->type == EntityType::BULLET) state.radius = 5.0f;
        else if (obj->type == EntityType::ENEMY) {
            auto e = std::dynamic_pointer_cast<Enemy>(obj); if (e) state.subtype = e->enemyType;
        }
        else if (obj->type == EntityType::WALL || obj->type == EntityType::TURRET || obj->type == EntityType::MINE) {
            auto c = std::dynamic_pointer_cast<Construct>(obj); if (c) { state.ownerId = c->ownerId; state.level = c->level; }
            if (obj->type == EntityType::WALL) state.radius = 25.0f; else if (obj->type == EntityType::TURRET) state.radius = 20.0f; else state.radius = 15.0f;
        }
        snap.entities.push_back(state);
    }
    Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
    ser.value1b(GamePacket::SNAPSHOT); ser.object(snap); ser.adapter().flush();

    BroadcastToAll(DeliveryType::UNRELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
}