#include "GameplayScene.h"
#include "GameClient.h"
#include <iostream>
#include <cstdio>
#include <algorithm>
#include <string>
#include <vector>
#include <cmath>

static const Color COL_INV_EMPTY = { 0, 0, 0, 100 };
static const Color COL_INV_BORDER = { 150, 150, 150, 200 };

GameplayScene::GameplayScene(GameClient* g) : Scene(g) {
    myInventory.resize(6, 255);
}

void GameplayScene::Enter() {
    float w = (float)game->GetWidth();
    float h = (float)game->GetHeight();
    camera.zoom = 1.0f;
    camera.offset = { w / 2, h / 2 };

    myPlayerId = 0;
    isPredictedInit = false;
    predictedPos = { 0, 0 };

    myLevel = 1;
    myCurrentXp = 0.0f;
    myMaxXp = 100.0f;
    myMaxHealth = 100.0f;
    myScrap = 0;

    currentWave = 1;
    selectedBuildType = 0;

    std::fill(myInventory.begin(), myInventory.end(), 255);
    lastFrameEntityIds.clear();
    gunAnimOffset = 0.0f;

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    float joyY = h - 180.0f;
    float joyOffset = 180.0f;
    leftStick = std::make_unique<VirtualJoystick>(Vector2{ joyOffset, joyY }, 50.0f, 100.0f);
    rightStick = std::make_unique<VirtualJoystick>(Vector2{ w - joyOffset, joyY }, 50.0f, 100.0f);
    rightStick->SetColors({ 200, 200, 200, 100 }, { 220, 50, 50, 200 });
#endif
}

void GameplayScene::Exit() {
    if (game->netClient) game->netClient->disconnect();
    game->StopHost();
}

void GameplayScene::SendAction(const ActionPacket& act) {
    if (game->netClient && game->netClient->isConnected()) {
        Buffer buffer; OutputAdapter adapter(buffer);
        bitsery::Serializer<OutputAdapter> serializer(std::move(adapter));
        serializer.value1b(GamePacket::ACTION);
        serializer.object(act);
        serializer.adapter().flush();
        game->netClient->send(DeliveryType::RELIABLE, StreamBuffer::alloc(buffer.data(), buffer.size()));
    }
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
        InitPacket pkt; deserializer.object(pkt);
        if (deserializer.adapter().error() == bitsery::ReaderError::NoError) myPlayerId = pkt.playerId;
    }
    else if (packetTypeInt == GamePacket::SNAPSHOT) {
        WorldSnapshotPacket snap; deserializer.object(snap);
        if (deserializer.adapter().error() == bitsery::ReaderError::NoError) {
            currentWave = snap.wave;
            snapshotManager.PushSnapshot(snap);
            lastServerTime = snap.serverTime;
            for (const auto& ent : snap.entities) {
                if (ent.id == myPlayerId) {
                    if (!isPredictedInit) {
                        predictedPos = ent.position;
                        isPredictedInit = true;
                    }
                    else {
                        float dist = Vector2Distance(predictedPos, ent.position);
                        if (dist > 150.0f) predictedPos = ent.position;
                        else predictedPos = Vector2Lerp(predictedPos, ent.position, 0.1f);
                    }
                    myHealth = ent.health;
                    break;
                }
            }
        }
    }
    else if (packetTypeInt == GamePacket::STATS) {
        PlayerStatsPacket stats; deserializer.object(stats);
        if (deserializer.adapter().error() == bitsery::ReaderError::NoError) {
            myLevel = stats.level;
            myCurrentXp = stats.currentXp;
            myMaxXp = stats.maxXp;
            myMaxHealth = stats.maxHealth;
            myDamage = stats.damage;
            mySpeed = stats.speed;
            myScrap = stats.scrap;
            myKills = stats.kills;
                        if (stats.inventory.size() == 6) myInventory = stats.inventory;
        }
    }
    else if (packetTypeInt == GamePacket::EVENT) {
        EventPacket evt; deserializer.object(evt);
        if (deserializer.adapter().error() == bitsery::ReaderError::NoError) {
            if (evt.type == 0) particles.SpawnExplosion(evt.pos, 4, evt.color);
            else if (evt.type == 1) particles.SpawnExplosion(evt.pos, 20, evt.color);
            else if (evt.type == 2) particles.SpawnExplosion(evt.pos, 10, evt.color);
        }
    }
}

void GameplayScene::Update(float dt) {
    if (gunAnimOffset > 0.0f) gunAnimOffset -= dt * 60.0f;
    particles.Update(dt);

    clientTime += dt;
    if (abs(clientTime - lastServerTime) > 2.0) clientTime = lastServerTime;
    else clientTime = Lerp((float)clientTime, (float)lastServerTime, 0.05f);

    PlayerInputPacket pkt = {};
    pkt.movement = { 0, 0 };
    pkt.aimTarget = { 0, 0 };
    pkt.isShooting = false;

    float currentSpeed = (mySpeed > 0) ? mySpeed : 220.0f;
    const float MAP_SIZE = 4000.0f;
    float myRadius = 20.0f + (myLevel - 1) * 2.0f;

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    if (leftStick && rightStick) {
        leftStick->Update(); rightStick->Update();
        pkt.movement = leftStick->GetAxis();
        Vector2 aimDir = rightStick->GetAxis();
        if (Vector2Length(aimDir) > 0.1f) {
            pkt.isShooting = true;
            pkt.aimTarget = Vector2Add(predictedPos, Vector2Scale(aimDir, 300.0f));
            predictedRot = atan2f(aimDir.y, aimDir.x) * RAD2DEG;
        }
        else pkt.aimTarget = Vector2Add(predictedPos, { 300, 0 });
    }
#else
    if (IsKeyDown(KEY_W)) pkt.movement.y -= 1.0f;
    if (IsKeyDown(KEY_S)) pkt.movement.y += 1.0f;
    if (IsKeyDown(KEY_A)) pkt.movement.x -= 1.0f;
    if (IsKeyDown(KEY_D)) pkt.movement.x += 1.0f;
    if (Vector2Length(pkt.movement) > 1.0f) pkt.movement = Vector2Normalize(pkt.movement);

    pkt.aimTarget = GetScreenToWorld2D(GetMousePosition(), camera);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (selectedBuildType != 0) {
            ActionPacket act;
            act.type = selectedBuildType;
            act.target = pkt.aimTarget;             SendAction(act);
            if (!IsKeyDown(KEY_LEFT_SHIFT)) selectedBuildType = 0;
        }
        else {
                        ActionPacket act;
            act.type = ActionType::UPGRADE_BUILDING;
            act.target = pkt.aimTarget;
            SendAction(act);
        }
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && selectedBuildType == 0) {
        pkt.isShooting = true;
    }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) selectedBuildType = 0;

    Vector2 diff = Vector2Subtract(pkt.aimTarget, predictedPos);
    predictedRot = atan2f(diff.y, diff.x) * RAD2DEG;
#endif

    if (pkt.isShooting && gunAnimOffset <= 1.0f) gunAnimOffset = 12.0f;

    if (isPredictedInit) {
        Vector2 velocity = Vector2Scale(pkt.movement, currentSpeed * dt);
        Vector2 nextPos = Vector2Add(predictedPos, velocity);
        if (nextPos.x < myRadius) nextPos.x = myRadius;
        if (nextPos.y < myRadius) nextPos.y = myRadius;
        if (nextPos.x > MAP_SIZE - myRadius) nextPos.x = MAP_SIZE - myRadius;
        if (nextPos.y > MAP_SIZE - myRadius) nextPos.y = MAP_SIZE - myRadius;
        predictedPos = nextPos;
        camera.target = Vector2Lerp(camera.target, predictedPos, 0.1f);
    }

    if (game->netClient && game->netClient->isConnected()) {
        Buffer buffer; OutputAdapter adapter(buffer); bitsery::Serializer<OutputAdapter> serializer(std::move(adapter));
        serializer.value1b(GamePacket::INPUT); serializer.object(pkt); serializer.adapter().flush();
        game->netClient->send(DeliveryType::UNRELIABLE, StreamBuffer::alloc(buffer.data(), buffer.size()));
    }
}

void GameplayScene::Draw() {
    BeginMode2D(camera);
    ClearBackground(Theme::COL_BACKGROUND);
    int gridW = 4000; int gridH = 4000;
    DrawRectangleLines(0, 0, gridW, gridH, GRAY);

        for (int i = 0; i <= gridW; i += 50) DrawLine(i, 0, i, gridH, Fade(Theme::COL_GRID, 0.3f));
    for (int i = 0; i <= gridH; i += 50) DrawLine(0, i, gridW, i, Fade(Theme::COL_GRID, 0.3f));

    auto DrawDiepTank = [&](Vector2 pos, float rot, Color mainCol, float radius, float hp, float maxHp, bool isMe) {
        Color outlineCol = { 85, 85, 85, 255 };
        float outlineThick = 3.0f;
        Vector2 dir = { cosf(rot * DEG2RAD), sinf(rot * DEG2RAD) };
        float baseLen = radius * 1.8f;
        float currentLen = isMe ? (baseLen - gunAnimOffset) : baseLen;
        float barrelWidth = radius * 0.8f;
        Vector2 bStart = pos;
        Vector2 bEnd = Vector2Add(pos, Vector2Scale(dir, currentLen));
        Vector2 perp = { -dir.y, dir.x };
        Vector2 c1 = Vector2Add(bStart, Vector2Scale(perp, barrelWidth * 0.5f));
        Vector2 c2 = Vector2Subtract(bStart, Vector2Scale(perp, barrelWidth * 0.5f));
        Vector2 c3 = Vector2Subtract(bEnd, Vector2Scale(perp, barrelWidth * 0.5f));
        Vector2 c4 = Vector2Add(bEnd, Vector2Scale(perp, barrelWidth * 0.5f));
        DrawLineEx(c1, c4, outlineThick * 2, outlineCol); DrawLineEx(c2, c3, outlineThick * 2, outlineCol); DrawLineEx(c4, c3, outlineThick * 2, outlineCol);
        DrawTriangle(c1, c2, c4, LIGHTGRAY); DrawTriangle(c2, c3, c4, LIGHTGRAY);
        DrawCircleV(pos, radius + (outlineThick / 2.0f), outlineCol);
        DrawCircleV(pos, radius, mainCol);
        };

    if (!snapshotManager.history.empty()) {
        double renderTime = clientTime - snapshotManager.INTERPOLATION_DELAY;
        const auto& latestEntities = snapshotManager.history.back().entities;

        for (const auto& ent : latestEntities) {
            EntityState renderState;
            if (!snapshotManager.GetInterpolatedState(ent.id, renderTime, renderState)) renderState = ent;

            if (ent.id == myPlayerId && isPredictedInit) {
                float myRadius = 20.0f + (myLevel - 1) * 2.0f;
                DrawDiepTank(predictedPos, predictedRot, Theme::COL_ACCENT, myRadius, myHealth, myMaxHealth, true);
                continue;
            }

            if (renderState.type == EntityType::PLAYER) {
                float otherRadius = 20.0f + (renderState.level - 1) * 2.0f;
                DrawDiepTank(renderState.position, renderState.rotation, Theme::COLOR_RED, otherRadius, renderState.health, renderState.maxHealth, false);
            }
            else if (renderState.type == EntityType::BULLET) {
                DrawCircleV(renderState.position, renderState.radius + 1.5f, { 85, 85, 85, 255 });
                DrawCircleV(renderState.position, renderState.radius, Theme::COLOR_RED);
            }
            else if (renderState.type == EntityType::ENEMY) {
                float radius = renderState.radius; Vector2 pos = renderState.position; float rot = renderState.rotation + 90.0f;
                Color enemyColor = Theme::COLOR_RED; int sides = 3;
                switch (renderState.subtype) {
                case EnemyType::FAST: enemyColor = { 255, 105, 180, 255 }; break;
                case EnemyType::TANK: enemyColor = { 139, 0, 0, 255 }; sides = 4; break;
                case EnemyType::BOSS: enemyColor = { 128, 0, 128, 255 }; sides = 5; break;
                }
                DrawPoly(pos, sides, radius + 4.0f, rot, { 85, 85, 85, 255 }); DrawPoly(pos, sides, radius, rot, enemyColor);
                float hpPct = renderState.health / renderState.maxHealth;
                if (hpPct < 0.99f) {
                    DrawRectangle((int)pos.x - 20, (int)pos.y + (int)radius + 10, 40, 6, { 50,50,50,200 });
                    DrawRectangle((int)pos.x - 19, (int)pos.y + (int)radius + 11, (int)(38 * hpPct), 4, GREEN);
                }
            }
            else if (renderState.type == EntityType::ARTIFACT) {
                float rot = (float)GetTime() * 100.0f;
                DrawPoly(renderState.position, 6, 16.0f, rot, { 85, 85, 85, 255 }); DrawPoly(renderState.position, 6, 13.0f, rot, ORANGE);
            }
            else if (renderState.type == EntityType::WALL) {
                Rectangle r = { renderState.position.x - 25, renderState.position.y - 25, 50, 50 };
                DrawRectangleRec(r, GRAY); DrawRectangleLinesEx(r, 2, DARKGRAY);
                float hpPct = renderState.health / renderState.maxHealth;
                if (hpPct < 1.0f) {
                    DrawRectangle(r.x, r.y - 8, 50, 4, RED); DrawRectangle(r.x, r.y - 8, 50 * hpPct, 4, GREEN);
                }
            }
            else if (renderState.type == EntityType::TURRET) {
                DrawCircleV(renderState.position, 20, PURPLE); DrawCircleLines(renderState.position.x, renderState.position.y, 20, BLACK);
                DrawCircleV(renderState.position, 8, DARKPURPLE);
                float hpPct = renderState.health / renderState.maxHealth;
                if (hpPct < 1.0f) {
                    DrawRectangle(renderState.position.x - 20, renderState.position.y - 30, 40, 4, RED);
                    DrawRectangle(renderState.position.x - 20, renderState.position.y - 30, 40 * hpPct, 4, GREEN);
                }
            }
            else if (renderState.type == EntityType::MINE) {
                DrawCircleV(renderState.position, 15, Fade(ORANGE, 0.5f)); DrawCircleLines(renderState.position.x, renderState.position.y, 15, RED);
            }
        }
    }

        if (selectedBuildType > 0) {
        Vector2 mPos = GetScreenToWorld2D(GetMousePosition(), camera);

                float gridX = roundf(mPos.x / 50.0f) * 50.0f;
        float gridY = roundf(mPos.y / 50.0f) * 50.0f;
        Vector2 snapPos = { gridX, gridY };

        Color ghostCol = Fade(GREEN, 0.5f);
        if (Vector2Distance(predictedPos, snapPos) > 400.0f) ghostCol = Fade(RED, 0.5f);

        if (selectedBuildType == ActionType::BUILD_WALL) DrawRectangle(snapPos.x - 25, snapPos.y - 25, 50, 50, ghostCol);
        else if (selectedBuildType == ActionType::BUILD_TURRET) DrawCircleV(snapPos, 20, ghostCol);
        else if (selectedBuildType == ActionType::BUILD_MINE) DrawCircleV(snapPos, 15, ghostCol);
    }

    particles.Draw();
    EndMode2D();
}

void GameplayScene::DrawGUI() {
    int w = GetScreenWidth(); int h = GetScreenHeight(); float uiScale = game->GetUIScale();
    GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(20 * uiScale)); float padding = 20 * uiScale;

        DrawText(TextFormat("Scrap: %d", myScrap), padding, padding, 20 * uiScale, GOLD);
    char waveText[32]; sprintf(waveText, "WAVE %d", currentWave);
    DrawText(waveText, (w - MeasureText(waveText, 30 * uiScale)) / 2, padding, 30 * uiScale, Theme::COL_ACCENT);

        const char* builds[] = { "Wall (10)", "Turret (50)", "Mine (25)" };
    int costs[] = { 10, 50, 25 };
    float bY = h - 200 * uiScale;
    for (int i = 0; i < 3; i++) {
        Rectangle bRect = { w - 130 * uiScale, bY + i * (45 * uiScale), 110 * uiScale, 40 * uiScale };
        int type = i + 1;
        if (myScrap >= costs[i]) {
            if (GuiButton(bRect, builds[i])) selectedBuildType = (selectedBuildType == type) ? 0 : type;
        }
        else {
            GuiDisable(); GuiButton(bRect, builds[i]); GuiEnable();
        }
        if (selectedBuildType == type) DrawRectangleLinesEx(bRect, 2, GREEN);
    }

    if (selectedBuildType != 0) {
        DrawText("LMB: Build, RMB: Cancel", w - 250 * uiScale, h - 50 * uiScale, 16 * uiScale, WHITE);
    }
    else {
        DrawText("Click building to Upgrade", w - 250 * uiScale, h - 30 * uiScale, 16 * uiScale, LIGHTGRAY);
    }

    if (GuiButton(Rectangle{ w - 80 * uiScale - padding, padding, 80 * uiScale, 30 * uiScale }, "MENU")) game->ReturnToMenu();

        float barW = 500 * uiScale; if (barW > w * 0.6f) barW = w * 0.6f;
    float centerX = w / 2.0f; float bottomY = h - padding;
    float xpY = bottomY - 15 * uiScale;
    float xpPct = (myMaxXp > 0) ? (myCurrentXp / myMaxXp) : 0.0f;
    DrawRectangle(centerX - barW / 2, xpY, barW, 15 * uiScale, Fade(BLACK, 0.6f));
    DrawRectangle(centerX - barW / 2 + 2, xpY + 2, (int)((barW - 4) * xpPct), 15 * uiScale - 4, { 255, 232, 105, 255 });
    char lvlTxt[32]; sprintf(lvlTxt, "Lvl %d", myLevel); DrawText(lvlTxt, centerX - MeasureText(lvlTxt, 14 * uiScale) / 2, xpY, 14 * uiScale, BLACK);

    float hpBarH = 22 * uiScale; float hpY = xpY - hpBarH - (5 * uiScale);
    float hpPct = (myMaxHealth > 0) ? (myHealth / myMaxHealth) : 0.0f;
    DrawRectangle(centerX - barW / 2, hpY, barW, hpBarH, Fade(BLACK, 0.6f));
    DrawRectangle(centerX - barW / 2 + 2, hpY + 2, (int)((barW - 4) * hpPct), hpBarH - 4, Theme::COLOR_RED);
    char hpTxt[32]; sprintf(hpTxt, "%.0f / %.0f", myHealth, myMaxHealth); DrawText(hpTxt, centerX - MeasureText(hpTxt, 16 * uiScale) / 2, hpY + 2, 16 * uiScale, WHITE);

        float slotSize = 40 * uiScale; float invStartX = centerX - (slotSize * 6 + 25 * uiScale) / 2;
    float invY = hpY - slotSize - (15 * uiScale);
#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    invY -= 80 * uiScale;
#endif
    for (int i = 0; i < 6; i++) {
        Rectangle slotRect = { invStartX + i * (slotSize + 5 * uiScale), invY, slotSize, slotSize };
        DrawRectangleRec(slotRect, COL_INV_EMPTY); DrawRectangleLinesEx(slotRect, 2, COL_INV_BORDER);
        if (i < myInventory.size() && myInventory[i] != 255) {
            Color iconColor = WHITE; const char* label = "?";
            switch (myInventory[i]) {
            case 0: iconColor = { 255, 100, 100, 255 }; label = "D"; break;
            case 1: iconColor = { 100, 100, 255, 255 }; label = "S"; break;
            case 2: iconColor = { 100, 255, 100, 255 }; label = "H"; break;
            case 3: iconColor = { 255, 255, 100, 255 }; label = "R"; break;
            }
            DrawRectangle(slotRect.x + 4, slotRect.y + 4, slotRect.width - 8, slotRect.height - 8, iconColor);
            DrawText(label, slotRect.x + 10 * uiScale, slotRect.y + 5 * uiScale, 24 * uiScale, BLACK);
        }
    }

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    if (leftStick) leftStick->Draw(); if (rightStick) rightStick->Draw();
#endif
}