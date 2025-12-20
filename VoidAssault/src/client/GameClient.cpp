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
    useRelay = false;
    ChangeScene(std::make_shared<MainMenuScene>(this));
}

void GameClient::ConnectToLobby(const std::string& ip, int port, uint32_t lobbyId) {
    useRelay = false;
    relayLobbyId = lobbyId;

    TraceLog(LOG_INFO, ">> Requesting P2P hole punch via Master Server...");

    if (!netClient->isConnected()) {
        ClientConfig& cfg = ConfigManager::GetClient();
        netClient->connect(cfg.masterServerIp, cfg.masterServerPort);
    }

    Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
    ser.value1b(GamePacket::P2P_REQUEST);
    P2PRequestPacket req; req.lobbyId = lobbyId;
    ser.object(req); ser.adapter().flush();

    netClient->send(DeliveryType::RELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
}

void GameClient::SendGamePacket(DeliveryType type, StreamBuffer::Shared stream) {
    if (!netClient || !netClient->isConnected()) return;

    if (useRelay) {
        RelayPacket relayPkt;
        relayPkt.targetId = relayLobbyId;
        relayPkt.isReliable = (type == DeliveryType::RELIABLE);
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

float GameClient::GetUIScale() const {
    int h = GetScreenHeight();
    float scale = (float)h / 720.0f;
#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    scale *= 1.1f;
#endif
    if (scale < 0.8f) scale = 0.8f;
    return scale;
}
void GameClient::StartP2PPunch(const std::string& targetIp, uint16_t targetPort) {
    if (targetIp.empty()) return;

    ENetAddress targetAddr;
    enet_address_set_host(&targetAddr, targetIp.c_str());
    targetAddr.port = targetPort;

    const char* punchData = "PUNCH";
    ENetBuffer buffer;
    buffer.data = (void*)punchData;
    buffer.dataLength = strlen(punchData);

    ENetSocket socketToUse;
    if (localServer && localServer->isRunning()) {
        socketToUse = localServer->getNetServer()->getHost()->socket;
        TraceLog(LOG_INFO, ">> P2P: Host punching from Server Socket to %s:%d", targetIp.c_str(), targetPort);
    }
    else {
        socketToUse = netClient->getHost()->socket;
        TraceLog(LOG_INFO, ">> P2P: Client punching from Client Socket to %s:%d", targetIp.c_str(), targetPort);
    }

    for (int i = 0; i < 12; i++) {
        enet_socket_send(socketToUse, &targetAddr, &buffer, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
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
                    TraceLog(LOG_INFO, ">> CLIENT: Connected to %s", useRelay ? "Relay" : "Host Directly");

                    if (!useRelay) {
                        Buffer buffer; OutputAdapter adapter(buffer);
                        bitsery::Serializer<OutputAdapter> serializer(std::move(adapter));
                        serializer.value1b(GamePacket::JOIN);
                        JoinPacket jp; jp.name = ConfigManager::GetClient().playerName;
                        serializer.object(jp);
                        serializer.adapter().flush();
                        netClient->send(DeliveryType::RELIABLE, StreamBuffer::alloc(buffer.data(), buffer.size()));
                    }

                    if (!std::dynamic_pointer_cast<GameplayScene>(currentScene)) {
                        ChangeScene(std::make_shared<GameplayScene>(this));
                    }
                }
                else if (msg->type() == MessageType::DATA) {
                    StreamBuffer::Shared payload = msg->stream();
                    const auto& raw = payload->buffer();
                    size_t offset = payload->tellg();
                    InputAdapter ia(raw.begin() + offset, raw.end());
                    bitsery::Deserializer<InputAdapter> des(std::move(ia));
                    uint8_t pktType; des.value1b(pktType);

                    if (pktType == GamePacket::P2P_SIGNAL) {
                        P2PSignalPacket sig;
                        des.object(sig);
                        if (des.adapter().error() == bitsery::ReaderError::NoError) {

                            std::string finalIp = sig.publicIp;
                            if (sig.publicIp == sig.yourIp) {
                                finalIp = "127.0.0.1";
                                TraceLog(LOG_INFO, ">> P2P: Same network detected, using 127.0.0.1");
                            }

                            std::thread([this, sig, finalIp]() {
                                this->StartP2PPunch(sig.publicIp, sig.publicPort);

                                if (!sig.isHost) {
                                    std::this_thread::sleep_for(std::chrono::milliseconds(400));
                                    TraceLog(LOG_INFO, ">> P2P: Connecting to %s:%d", finalIp.c_str(), sig.publicPort);

                                    this->netClient->disconnect();
                                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                    this->netClient->connect(finalIp, sig.publicPort);
                                }
                                }).detach();
                        }
                        continue;
                    }

                    if (useRelay && pktType == GamePacket::RELAY_TO_CLIENT) {
                        RelayPacket rp;
                        des.object(rp);
                        if (des.adapter().error() == bitsery::ReaderError::NoError) {
                            payload = StreamBuffer::alloc(rp.data.data(), rp.data.size());
                        }
                        else {
                            continue;
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
            audio.Update();

            BeginDrawing();
            ClearBackground(Theme::COL_BACKGROUND);

            currentScene->Draw();
            currentScene->DrawGUI();

            EndDrawing();
        }
    }
}