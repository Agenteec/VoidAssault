#include "GameplayScene.h"
#include "MainMenuScene.h" // Теперь это безопасно

GameplayScene::GameplayScene(GameClient* g) : Scene(g) {}

void GameplayScene::Enter() {
    camera.zoom = 1.0f;
    camera.offset = { (float)game->GetWidth() / 2, (float)game->GetHeight() / 2 };
}

void GameplayScene::Exit() {
    game->network.Disconnect();
    game->StopHost();
    worldEntities.clear();
}

void GameplayScene::OnPacketReceived(const uint8_t* data, size_t size) {
    PacketHeader* header = (PacketHeader*)data;

    if (header->type == PacketType::server_INIT) {
        InitPacket* init = (InitPacket*)data;
        myPlayerId = init->yourPlayerId;
    }
    else if (header->type == PacketType::server_SNAPSHOT) {
        WorldSnapshotPacket* snap = (WorldSnapshotPacket*)data;
        EntityState* states = (EntityState*)(data + sizeof(WorldSnapshotPacket));

        std::vector<uint32_t> receivedIds;
        for (uint32_t i = 0; i < snap->entityCount; i++) {
            receivedIds.push_back(states[i].id);
            worldEntities[states[i].id].PushState(states[i]);
        }
        for (auto it = worldEntities.begin(); it != worldEntities.end();) {
            bool found = false;
            for (uint32_t id : receivedIds) if (id == it->first) found = true;
            if (!found) it = worldEntities.erase(it);
            else ++it;
        }
    }
}

void GameplayScene::Update(float dt) {
    PlayerInputPacket pkt;
    pkt.type = PacketType::client_INPUT;
    pkt.movement = { 0, 0 };
    if (IsKeyDown(KEY_W)) pkt.movement.y -= 1.0f;
    if (IsKeyDown(KEY_S)) pkt.movement.y += 1.0f;
    if (IsKeyDown(KEY_A)) pkt.movement.x -= 1.0f;
    if (IsKeyDown(KEY_D)) pkt.movement.x += 1.0f;

    pkt.aimTarget = GetScreenToWorld2D(GetMousePosition(), camera);
    pkt.isShooting = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    game->network.SendInput(pkt);

    for (auto& [id, ent] : worldEntities) {
        ent.UpdateInterpolation();
    }

    if (worldEntities.count(myPlayerId)) {
        Vector2 target = worldEntities[myPlayerId].renderPos;
        camera.target = Vector2Lerp(camera.target, target, 0.1f);
    }
}

void GameplayScene::Draw() {
    BeginMode2D(camera);
    DrawGrid(100, 50);
    DrawRectangleLines(0, 0, 2000, 2000, GRAY);

    for (auto& [id, ent] : worldEntities) {
        EntityState& st = ent.current;

        if (st.type == EntityType::PLAYER) {
            DrawCircleV(ent.renderPos, st.radius, st.color);
            Vector2 barrelEnd = {
                ent.renderPos.x + cosf(ent.renderRot * DEG2RAD) * 40,
                ent.renderPos.y + sinf(ent.renderRot * DEG2RAD) * 40
            };
            DrawLineEx(ent.renderPos, barrelEnd, 15, DARKGRAY);

            float hpPct = st.health / st.maxHealth;
            DrawRectangle((int)ent.renderPos.x - 20, (int)ent.renderPos.y - 40, 40, 5, RED);
            DrawRectangle((int)ent.renderPos.x - 20, (int)ent.renderPos.y - 40, (int)(40 * hpPct), 5, GREEN);
        }
        else if (st.type == EntityType::BULLET) {
            DrawCircleV(ent.renderPos, st.radius, RED);
        }
    }
    EndMode2D();
}

void GameplayScene::DrawGUI() {
    DrawFPS(10, 10);
    if (!game->network.isConnected) {
        DrawText("Connecting to server...", game->GetWidth() / 2 - 100, 20, 20, RED);
    }

    // Здесь конфликтов больше быть не должно, так как raygui_wrapper подчистил макросы
    if (GuiButton(Rectangle{ (float)game->GetWidth() - 100, 10, 90, 30 }, "DISCONNECT")) {
        game->ChangeScene(std::make_shared<MainMenuScene>(game));
    }
}