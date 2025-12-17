#include "GameClient.h"
#include "scenes/MainMenuScene.h"
#include "scenes/GameplayScene.h"
#include "../engine/Utils/ConfigManager.h"
#include "Theme.h"
#include <thread>
#include <chrono>

GameClient::GameClient() {
    ClientConfig& cfg = ConfigManager::GetClient();
    screenWidth = cfg.resolutionWidth;
    screenHeight = cfg.resolutionHeight;

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    InitWindow(0, 0, "Void Assault");
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Void Assault");
#endif

    SetTargetFPS(cfg.targetFPS);
    SetWindowMinSize(800, 600);

    netClient = ENetClient::alloc();

    float scale = GetUIScale();
    GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(20 * scale));
}

GameClient::~GameClient() {
    StopHost();
    if (currentScene) currentScene->Exit();
    if (netClient) netClient->disconnect();
    CloseWindow();
}

void GameClient::ChangeScene(std::shared_ptr<Scene> newScene) {
    nextScene = newScene;
}

void GameClient::ReturnToMenu() {
    if (netClient) netClient->disconnect();
    useRelay = false;     ChangeScene(std::make_shared<MainMenuScene>(this));
}

int GameClient::StartHost(int startPort, bool publicServer) {
    StopHost();
    localServer = std::make_unique<ServerHost>();
        for (int p = startPort; p < startPort + 10; p++) {
        if (localServer->Start(p, publicServer)) {
            TraceLog(LOG_INFO, "Local Server Started on port %d", p);
            return p;
        }
    }
    TraceLog(LOG_ERROR, "Failed to start local server. All ports busy?");
    localServer.reset();
    return -1;
}

void GameClient::StopHost() {
    if (localServer) {
        localServer->Stop();
        localServer.reset();
        TraceLog(LOG_INFO, "Local Server Stopped.");
    }
}

void GameClient::ConnectToLobby(const std::string& ip, int port, uint32_t lobbyId) {
    useRelay = false;
    relayLobbyId = 0;

    TraceLog(LOG_INFO, "Attempting direct connection to %s:%d", ip.c_str(), port);

        bool directSuccess = netClient->connect(ip, port);

    if (directSuccess) {
        TraceLog(LOG_INFO, "Direct connection successful!");
    }
    else {
        TraceLog(LOG_WARNING, "Direct connection failed. Falling back to Relay via Master Server.");

                ClientConfig& cfg = ConfigManager::GetClient();
        if (netClient->connect(cfg.masterServerIp, cfg.masterServerPort)) {
            useRelay = true;
            relayLobbyId = lobbyId;
            TraceLog(LOG_INFO, "Connected to Master Server for Relay. Lobby ID: %d", lobbyId);

                                    Buffer buffer; OutputAdapter adapter(buffer);
            bitsery::Serializer<OutputAdapter> serializer(std::move(adapter));
            serializer.value1b(GamePacket::JOIN);
            JoinPacket jp; jp.name = ConfigManager::GetClient().playerName;
            serializer.object(jp);
            serializer.adapter().flush();

                        SendGamePacket(DeliveryType::RELIABLE, StreamBuffer::alloc(buffer.data(), buffer.size()));
        }
        else {
            TraceLog(LOG_ERROR, "Failed to connect to Master Server (Relay unreachable).");
        }
    }
}

void GameClient::SendGamePacket(DeliveryType type, StreamBuffer::Shared stream) {
    if (!netClient || !netClient->isConnected()) return;

    if (useRelay) {
                RelayPacket relayPkt;
        relayPkt.targetId = relayLobbyId;
        relayPkt.data = stream->buffer(); 
        Buffer rBuf; OutputAdapter rAd(rBuf);
        bitsery::Serializer<OutputAdapter> rSer(std::move(rAd));
        rSer.value1b(GamePacket::RELAY_TO_SERVER);
        rSer.object(relayPkt);
        rSer.adapter().flush();

        netClient->send(type, StreamBuffer::alloc(rBuf.data(), rBuf.size()));
    }
    else {
                netClient->send(type, stream);
    }
}

float GameClient::GetUIScale() const {
    int h = GetScreenHeight();
    float scale = (float)h / 720.0f;
#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    scale *= 1.1f;
#endif
    if (scale < 0.8f) scale = 0.8f;
    return scale;
}

void GameClient::Run() {
    ChangeScene(std::make_shared<MainMenuScene>(this));

    while (!WindowShouldClose()) {
        if (nextScene) {
            if (currentScene) currentScene->Exit();
            currentScene = nextScene;
            currentScene->Enter();
            nextScene = nullptr;
        }
        float dt = GetFrameTime();

        if (netClient) {
            auto msgs = netClient->poll();

            for (auto& msg : msgs) {
                                
                if (msg->type() == MessageType::CONNECT) {
                    if (!useRelay) {
                        TraceLog(LOG_INFO, ">> CLIENT: Connected to server (Direct)!");
                        if (!std::dynamic_pointer_cast<GameplayScene>(currentScene)) {
                            ChangeScene(std::make_shared<GameplayScene>(this));
                        }
                    }
                    else {
                        TraceLog(LOG_INFO, ">> CLIENT: Connected to Master (Relay Mode Ready).");
                                                if (!std::dynamic_pointer_cast<GameplayScene>(currentScene)) {
                            ChangeScene(std::make_shared<GameplayScene>(this));
                        }
                    }
                }
                else if (msg->type() == MessageType::DISCONNECT) {
                    TraceLog(LOG_INFO, ">> CLIENT: Disconnected.");
                    if (std::dynamic_pointer_cast<GameplayScene>(currentScene)) {
                        ReturnToMenu();
                    }
                }
                else if (msg->type() == MessageType::DATA) {
                    StreamBuffer::Shared payload = msg->stream();

                                        if (useRelay) {
                        const auto& buf = payload->buffer();
                        size_t offset = payload->tellg();
                        if (!buf.empty()) {
                            InputAdapter ia(buf.begin() + offset, buf.end());
                            bitsery::Deserializer<InputAdapter> des(std::move(ia));
                            uint8_t pktType; des.value1b(pktType);

                            if (pktType == GamePacket::RELAY_TO_CLIENT) {
                                RelayPacket rp; des.object(rp);
                                if (des.adapter().error() == bitsery::ReaderError::NoError) {
                                                                        payload = StreamBuffer::alloc(rp.data.data(), rp.data.size());
                                }
                                else {
                                    continue;                                 }
                            }
                            else {
                                                                                                payload->seekg(offset);
                            }
                        }
                    }

                    if (currentScene) {
                                                auto innerMsg = Message::alloc(msg->id(), MessageType::DATA, payload);
                        currentScene->OnMessage(innerMsg);
                    }
                }
            }
        }

        if (currentScene) {
            currentScene->Update(dt);

            BeginDrawing();
            ClearBackground(Theme::COL_BACKGROUND);
            currentScene->Draw();
            currentScene->DrawGUI();
            EndDrawing();
        }
    }
}