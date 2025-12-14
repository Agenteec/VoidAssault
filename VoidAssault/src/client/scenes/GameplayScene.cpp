#include "GameplayScene.h"
#include "GameClient.h"
#include <iostream>
#include <cstdio>
#include <algorithm>

GameplayScene::GameplayScene(GameClient* g) : Scene(g) {}

void GameplayScene::Enter() {
    float w = (float)game->GetWidth();
    float h = (float)game->GetHeight();
    camera.zoom = 1.0f;
    camera.offset = { w / 2, h / 2 };

    myPlayerId = 0;
    isPredictedInit = false;
    predictedPos = { 0, 0 };
    predictedRot = 0.0f;

    gunAnimOffset = 0.0f;
    myLevel = 1;
    myCurrentXp = 0.0f;
    myMaxXp = 100.0f;
    myHealth = 100.0f;
    myMaxHealth = 100.0f;
    myDamage = 0.0f;
    mySpeed = 0.0f;

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    leftStick = std::make_unique<VirtualJoystick>(Vector2{ 150, h - 150 }, 40.0f, 90.0f);
    rightStick = std::make_unique<VirtualJoystick>(Vector2{ w - 150, h - 150 }, 40.0f, 90.0f);
    rightStick->SetColors({ 180, 180, 180, 150 }, { 220, 50, 50, 200 });
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
            TraceLog(LOG_INFO, "INIT received. ID: %d", myPlayerId);
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
                        if (dist > 100.0f) {
                            predictedPos = ent.position;
                        }
                        else {
                            predictedPos = Vector2Lerp(predictedPos, ent.position, 0.15f);
                        }
                    }
                    myHealth = ent.health;
                    break;
                }
            }
        }
    }
    else if (packetTypeInt == GamePacket::STATS) {
        PlayerStatsPacket stats;
        deserializer.object(stats);
        if (deserializer.adapter().error() == bitsery::ReaderError::NoError) {
            myLevel = stats.level;
            myCurrentXp = stats.currentXp;
            myMaxXp = stats.maxXp;
            myMaxHealth = stats.maxHealth;
            myDamage = stats.damage;
            mySpeed = stats.speed;
        }
    }
}

void GameplayScene::Update(float dt) {
    if (gunAnimOffset > 0.0f) {
        gunAnimOffset -= dt * 60.0f;         if (gunAnimOffset < 0.0f) gunAnimOffset = 0.0f;
    }

    clientTime += dt;
    if (abs(clientTime - lastServerTime) > 2.0) {
        clientTime = lastServerTime;
    }
    else {
        clientTime = Lerp((float)clientTime, (float)lastServerTime, 0.05f);
    }

    PlayerInputPacket pkt = {};
    pkt.movement = { 0, 0 };
    pkt.aimTarget = { 0, 0 };
    pkt.isShooting = false;

    float currentSpeed = (mySpeed > 0) ? mySpeed : 220.0f;

    const float MAP_SIZE = 2000.0f;
    float myRadius = 20.0f + (myLevel - 1) * 2.0f;

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    if (leftStick && rightStick) {
        leftStick->Update();
        rightStick->Update();
        pkt.movement = leftStick->GetAxis();

        Vector2 aimDir = rightStick->GetAxis();
        if (Vector2Length(aimDir) > 0.1f) {
            pkt.isShooting = true;
            pkt.aimTarget = Vector2Add(predictedPos, Vector2Scale(aimDir, 300.0f));
            predictedRot = atan2f(aimDir.y, aimDir.x) * RAD2DEG;
        }
        else {
            if (Vector2Length(pkt.movement) > 0.1f) {
                predictedRot = atan2f(pkt.movement.y, pkt.movement.x) * RAD2DEG;
                pkt.aimTarget = Vector2Add(predictedPos, Vector2Scale(pkt.movement, 300.0f));
            }
            else {
                pkt.aimTarget = Vector2Add(predictedPos, { 300, 0 });
            }
        }
    }
#else
    if (IsKeyDown(KEY_W)) pkt.movement.y -= 1.0f;
    if (IsKeyDown(KEY_S)) pkt.movement.y += 1.0f;
    if (IsKeyDown(KEY_A)) pkt.movement.x -= 1.0f;
    if (IsKeyDown(KEY_D)) pkt.movement.x += 1.0f;

    if (Vector2Length(pkt.movement) > 1.0f) pkt.movement = Vector2Normalize(pkt.movement);

    pkt.aimTarget = GetScreenToWorld2D(GetMousePosition(), camera);
    pkt.isShooting = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    Vector2 diff = Vector2Subtract(pkt.aimTarget, predictedPos);
    predictedRot = atan2f(diff.y, diff.x) * RAD2DEG;
#endif

    if (pkt.isShooting && gunAnimOffset <= 1.0f) {
        gunAnimOffset = 12.0f;
    }

    if (isPredictedInit) {
        Vector2 velocity = Vector2Scale(pkt.movement, currentSpeed * dt);
        Vector2 nextPos = Vector2Add(predictedPos, velocity);

        if (nextPos.x < myRadius) nextPos.x = myRadius;
        if (nextPos.y < myRadius) nextPos.y = myRadius;
        if (nextPos.x > MAP_SIZE - myRadius) nextPos.x = MAP_SIZE - myRadius;
        if (nextPos.y > MAP_SIZE - myRadius) nextPos.y = MAP_SIZE - myRadius;

        predictedPos = nextPos;
    }

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

    DrawRectangleLines(0, 0, gridW, gridH, GRAY);

    for (int i = 0; i <= gridW; i += 50) {
        DrawLine(i, 0, i, gridH, Fade(Theme::COL_GRID, 0.6f));
    }
    for (int i = 0; i <= gridH; i += 50) {
        DrawLine(0, i, gridW, i, Fade(Theme::COL_GRID, 0.6f));
    }

    auto DrawDiepTank = [&](Vector2 pos, float rot, Color mainCol, float radius, float hp, float maxHp, bool isMe) {
        Color outlineCol = { 85, 85, 85, 255 };         float outlineThick = 3.0f;

        Vector2 dir = { cosf(rot * DEG2RAD), sinf(rot * DEG2RAD) };

        float baseLen = radius * 1.8f;

        float currentLen = isMe ? (baseLen - gunAnimOffset) : baseLen;
        float barrelWidth = radius * 0.8f;

        Vector2 bStart = pos;
        Vector2 bEnd = Vector2Add(pos, Vector2Scale(dir, currentLen));

        Vector2 perp = { -dir.y, dir.x };         Vector2 c1 = Vector2Add(bStart, Vector2Scale(perp, barrelWidth * 0.5f));
        Vector2 c2 = Vector2Subtract(bStart, Vector2Scale(perp, barrelWidth * 0.5f));
        Vector2 c3 = Vector2Subtract(bEnd, Vector2Scale(perp, barrelWidth * 0.5f));
        Vector2 c4 = Vector2Add(bEnd, Vector2Scale(perp, barrelWidth * 0.5f));

        DrawLineEx(c1, c4, outlineThick * 2, outlineCol);
        DrawLineEx(c2, c3, outlineThick * 2, outlineCol);
        DrawLineEx(c4, c3, outlineThick * 2, outlineCol);

        DrawTriangle(c1, c2, c4, LIGHTGRAY);
        DrawTriangle(c2, c3, c4, LIGHTGRAY);

        if (isMe && gunAnimOffset > 8.0f) {
            Vector2 flashPos = Vector2Add(bEnd, Vector2Scale(dir, 10.0f));
            DrawCircleV(flashPos, radius * 0.7f, Fade(WHITE, 0.8f));
            DrawCircleV(flashPos, radius * 0.5f, Fade(GOLD, 0.6f));
        }

        DrawCircleV(pos, radius + (outlineThick / 2.0f), outlineCol);         DrawCircleV(pos, radius, mainCol);
        float hpPct = (maxHp > 0) ? (hp / maxHp) : 0.0f;
        if (hpPct < 0.99f) {
            float barW = radius * 2.2f;
            float barH = 6.0f;
            Vector2 barPos = { pos.x - barW / 2, pos.y + radius + 8.0f };

            DrawRectangleV(barPos, { barW, barH }, outlineCol);             DrawRectangle((int)barPos.x + 1, (int)barPos.y + 1, (int)((barW - 2) * hpPct), (int)barH - 2, GREEN);
        }
        };

    if (isPredictedInit) {
        float myRadius = 20.0f + (myLevel - 1) * 2.0f;
        DrawDiepTank(predictedPos, predictedRot, Theme::COL_ACCENT, myRadius, myHealth, myMaxHealth, true);
    }

    if (!snapshotManager.history.empty()) {
        double renderTime = clientTime - snapshotManager.INTERPOLATION_DELAY;
        const auto& latestEntities = snapshotManager.history.back().entities;

        for (const auto& ent : latestEntities) {
            if (ent.id == myPlayerId) continue;

            EntityState renderState;
            if (snapshotManager.GetInterpolatedState(ent.id, renderTime, renderState)) {

                if (renderState.type == EntityType::PLAYER) {
                    float otherRadius = 20.0f + (renderState.level - 1) * 2.0f;
                    Color tankCol = Theme::COLOR_RED;
                    DrawDiepTank(renderState.position, renderState.rotation, tankCol, otherRadius, renderState.health, renderState.maxHealth, false);
                }
                else if (renderState.type == EntityType::BULLET) {
                    Color bCol = Theme::COLOR_RED;
                    Color outlineCol = { 85, 85, 85, 255 };
                    DrawCircleV(renderState.position, renderState.radius + 1.5f, outlineCol);
                    DrawCircleV(renderState.position, renderState.radius, bCol);
                }
                else if (renderState.type == EntityType::ARTIFACT) {
                    float rot = (float)GetTime() * 100.0f;
                    DrawPoly(renderState.position, 4, 16.0f, rot, { 85, 85, 85, 255 });                     DrawPoly(renderState.position, 4, 13.0f, rot, GOLD);
                }
            }
        }
    }
    EndMode2D();
}

void GameplayScene::DrawGUI() {
    int w = game->GetWidth();
    int h = game->GetHeight();

    float xpPct = (myMaxXp > 0) ? (myCurrentXp / myMaxXp) : 0.0f;
    int barW = 600;
    int barH = 20;
    int barX = (w - barW) / 2;
    int barY = h - barH - 20;

    DrawRectangle(barX, barY, barW, barH, { 60, 60, 60, 200 });
    DrawRectangle(barX + 2, barY + 2, (int)((barW - 4) * xpPct), barH - 4, { 255, 232, 105, 255 });
    char lvlText[32];
    sprintf(lvlText, "Level %d", myLevel);
    int txtW = MeasureText(lvlText, 20);
    DrawText(lvlText, w / 2 - txtW / 2, barY - 25, 20, BLACK);

    float hpPct = (myMaxHealth > 0) ? (myHealth / myMaxHealth) : 0.0f;
    DrawRectangle(20, h - 40, 200, 20, { 60, 60, 60, 200 });     DrawRectangle(22, h - 38, (int)(196 * hpPct), 16, Theme::COLOR_RED);
    char hpText[32];
    sprintf(hpText, "%.0f / %.0f", myHealth, myMaxHealth);
    DrawText(hpText, 30, h - 38, 10, WHITE);

    char statsText[128];
    sprintf(statsText, "Damage: %.1f\nSpeed: %.0f", myDamage, mySpeed);
    DrawText(statsText, w - 140, 50, 20, DARKGRAY);

    if (game->netClient && !game->netClient->isConnected()) {
        const char* txt = "Connecting to server...";
        DrawText(txt, w / 2 - MeasureText(txt, 20) / 2, 20, 20, Theme::COL_WARNING);
    }
    else {
        DrawFPS(10, 10);
    }

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    if (leftStick) leftStick->Draw();
    if (rightStick) rightStick->Draw();
#endif

    if (GuiButton(Rectangle{ (float)w - 100, 10, 90, 30 }, "MENU")) {
        game->ReturnToMenu();
    }
}