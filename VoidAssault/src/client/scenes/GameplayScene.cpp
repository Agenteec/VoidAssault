// client/scenes/GameplayScene.cpp
#include "GameplayScene.h"
#include "GameClient.h"
#include <iostream>

#define DEBUG

GameplayScene::GameplayScene(GameClient* g) : Scene(g) {}

void GameplayScene::Enter() {
    float w = (float)game->GetWidth();
    float h = (float)game->GetHeight();
    camera.zoom = 1.0f;
    camera.offset = { w / 2, h / 2 };

    myPlayerId = 0;
    isPredictedInit = false;
    predictedPos = { 0, 0 };

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    leftStick = std::make_unique<VirtualJoystick>(Vector2{ 150, h - 150 }, 40.0f, 90.0f);
    rightStick = std::make_unique<VirtualJoystick>(Vector2{ w - 150, h - 150 }, 40.0f, 90.0f);
    rightStick->SetColors({ 60, 0, 0, 150 }, { 220, 50, 50, 200 });
#endif
}

void GameplayScene::Exit() {
    if (game->netClient) game->netClient->disconnect();
    game->StopHost();
}

void GameplayScene::OnMessage(Message::Shared msg) {
    const auto& rawData = msg->stream()->buffer();
    size_t offset = msg->stream()->tellg();

    if (rawData.empty() || offset >= rawData.size()) return;

    InputAdapter adapter(rawData.begin() + offset, rawData.end());
    bitsery::Deserializer<InputAdapter> deserializer(std::move(adapter));

    uint8_t packetTypeInt = 0;
    deserializer.value1b(packetTypeInt);

    if (deserializer.adapter().error() != bitsery::ReaderError::NoError) return;

    if (packetTypeInt == GamePacket::INIT) {
        InitPacket pkt;
        deserializer.object(pkt);
        if (deserializer.adapter().error() == bitsery::ReaderError::NoError) {
            myPlayerId = pkt.playerId;
            TraceLog(LOG_INFO, "INIT received. My ID: %d", myPlayerId);
        }
    }
    else if (packetTypeInt == GamePacket::SNAPSHOT) {
        WorldSnapshotPacket snap;
        deserializer.object(snap);

        if (deserializer.adapter().error() == bitsery::ReaderError::NoError) {
            snapshotManager.PushSnapshot(snap);
            lastServerTime = snap.serverTime;

            for (const auto& ent : snap.entities) {
                if (ent.id == myPlayerId) {
                    if (!isPredictedInit) {
                        predictedPos = ent.position;
                        predictedRot = ent.rotation;
                        isPredictedInit = true;
                    }
                    else {
                        float dist = Vector2Distance(predictedPos, ent.position);
                        if (dist > 50.0f) {
                            predictedPos = ent.position;
                        }
                        else {
                            predictedPos = Vector2Lerp(predictedPos, ent.position, 0.1f);
                        }
                    }
                    break;
                }
            }
        }
    }
}

void GameplayScene::Update(float dt) {
    clientTime += dt;

    if (abs(clientTime - lastServerTime) > 1.0) {
        clientTime = lastServerTime;
    }
    else {
        clientTime = Lerp((float)clientTime, (float)lastServerTime, 0.05f);
    }

    PlayerInputPacket pkt = {};
    pkt.movement = { 0, 0 };
    pkt.aimTarget = { 0, 0 };
    pkt.isShooting = false;
    const float PLAYER_SPEED = 200.0f;

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    if (leftStick && rightStick) {
        leftStick->Update();
        rightStick->Update();
        pkt.movement = leftStick->GetAxis();

        if (isPredictedInit) {
            predictedPos.x += pkt.movement.x * PLAYER_SPEED * dt;
            predictedPos.y += pkt.movement.y * PLAYER_SPEED * dt;
        }

        Vector2 aimDir = rightStick->GetAxis();
        if (Vector2Length(aimDir) > 0.1f) {
            pkt.isShooting = true;
            pkt.aimTarget = Vector2Add(predictedPos, Vector2Scale(aimDir, 300.0f));
            predictedRot = atan2f(aimDir.y, aimDir.x) * RAD2DEG;
        }
    }
#else
    if (IsKeyDown(KEY_W)) pkt.movement.y -= 1.0f;
    if (IsKeyDown(KEY_S)) pkt.movement.y += 1.0f;
    if (IsKeyDown(KEY_A)) pkt.movement.x -= 1.0f;
    if (IsKeyDown(KEY_D)) pkt.movement.x += 1.0f;

    if (Vector2Length(pkt.movement) > 1.0f) pkt.movement = Vector2Normalize(pkt.movement);

    if (isPredictedInit) {
        predictedPos.x += pkt.movement.x * PLAYER_SPEED * dt;
        predictedPos.y += pkt.movement.y * PLAYER_SPEED * dt;
    }

    pkt.aimTarget = GetScreenToWorld2D(GetMousePosition(), camera);
    pkt.isShooting = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    Vector2 diff = Vector2Subtract(pkt.aimTarget, predictedPos);
    predictedRot = atan2f(diff.y, diff.x) * RAD2DEG;
#endif

    if (game->netClient && game->netClient->isConnected()) {
        Buffer buffer;
        OutputAdapter adapter(buffer);
        bitsery::Serializer<OutputAdapter> serializer(std::move(adapter));

        uint8_t type = GamePacket::INPUT;
        serializer.value1b(type);
        serializer.object(pkt);
        serializer.adapter().flush();

        auto stream = StreamBuffer::alloc(buffer.data(), buffer.size());
        game->netClient->send(DeliveryType::UNRELIABLE, stream);
    }

    if (isPredictedInit) {
        camera.target = Vector2Lerp(camera.target, predictedPos, 0.1f);
    }
}

void GameplayScene::Draw() {
    BeginMode2D(camera);

    ClearBackground(Theme::COL_BACKGROUND);
    int gridW = 2000; int gridH = 2000;
    for (int i = 0; i <= gridW; i += 50) DrawLine(i, 0, i, gridH, Fade(GRAY, 0.2f));
    for (int i = 0; i <= gridH; i += 50) DrawLine(0, i, gridW, i, Fade(GRAY, 0.2f));
    DrawRectangleLines(0, 0, gridW, gridH, GRAY);

    if (isPredictedInit) {
        Color myColor = BLUE;
        DrawCircleV(predictedPos, 20.0f, myColor);
        Vector2 barrelEnd = {
            predictedPos.x + cosf(predictedRot * DEG2RAD) * 40,
            predictedPos.y + sinf(predictedRot * DEG2RAD) * 40
        };
        DrawLineEx(predictedPos, barrelEnd, 15, DARKGRAY);
    }

#ifdef DEBUG
    if (!snapshotManager.history.empty()) {
        for (const auto& ent : snapshotManager.history.back().entities) {
            if (ent.id == myPlayerId) {
                DrawCircleV(ent.position, 20.0f, Fade(RED, 0.3f));
                DrawLineV(predictedPos, ent.position, RED);
                break;
            }
        }
    }
#endif

    if (!snapshotManager.history.empty()) {
        double renderTime = clientTime - snapshotManager.INTERPOLATION_DELAY;
        const auto& latestEntities = snapshotManager.history.back().entities;

        for (const auto& ent : latestEntities) {
            if (ent.id == myPlayerId) continue;

            EntityState renderState;
            if (snapshotManager.GetInterpolatedState(ent.id, renderTime, renderState)) {
                if (renderState.type == EntityType::PLAYER) {
                    DrawCircleV(renderState.position, renderState.radius, renderState.color);
                    Vector2 barrelEnd = {
                        renderState.position.x + cosf(renderState.rotation * DEG2RAD) * 40,
                        renderState.position.y + sinf(renderState.rotation * DEG2RAD) * 40
                    };
                    DrawLineEx(renderState.position, barrelEnd, 15, DARKGRAY);

                    float hpPct = renderState.health / renderState.maxHealth;
                    if (renderState.maxHealth > 0) {
                        DrawRectangle((int)renderState.position.x - 20, (int)renderState.position.y - 40, 40, 5, RED);
                        DrawRectangle((int)renderState.position.x - 20, (int)renderState.position.y - 40, (int)(40 * hpPct), 5, GREEN);
                    }
                }
                else if (renderState.type == EntityType::BULLET) {
                    DrawCircleV(renderState.position, renderState.radius, RED);
                }
            }
        }
    }
    EndMode2D();
}

void GameplayScene::DrawGUI() {
    DrawFPS(10, 10);

    if (game->netClient && !game->netClient->isConnected()) {
        DrawText("Connecting to server...", game->GetWidth() / 2 - 100, 20, 20, RED);
    }

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    if (leftStick) leftStick->Draw();
    if (rightStick) rightStick->Draw();
#endif

#ifdef DEBUG
    DrawText("DEBUG MODE", 10, 40, 20, RED);
    DrawText(TextFormat("Ping (fake): %.2f ms", snapshotManager.INTERPOLATION_DELAY * 1000), 10, 60, 20, GREEN);
#endif

    if (GuiButton(Rectangle{ (float)game->GetWidth() - 100, 10, 90, 30 }, "DISCONNECT")) {
        game->ReturnToMenu();
    }
}