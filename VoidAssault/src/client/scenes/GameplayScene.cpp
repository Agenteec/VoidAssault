#include "GameplayScene.h"
#include "MainMenuScene.h"
#include "GameClient.h"

GameplayScene::GameplayScene(GameClient* g) : Scene(g) {}

void GameplayScene::Enter() {
    float w = (float)game->GetWidth();
    float h = (float)game->GetHeight();

    camera.zoom = 1.0f;
    camera.offset = { w / 2, h / 2 };

    // Инициализация джойстиков
    leftStick = std::make_unique<VirtualJoystick>(Vector2{ 150, h - 150 }, 40.0f, 90.0f);
    rightStick = std::make_unique<VirtualJoystick>(Vector2{ w - 150, h - 150 }, 40.0f, 90.0f);
    rightStick->SetColors({ 60, 0, 0, 150 }, { 220, 50, 50, 200 });
}

void GameplayScene::Exit() {
    // Отключаемся при выходе
    if (game->netClient) game->netClient->disconnect();
    game->StopHost();
    worldEntities.clear();
}

void GameplayScene::OnMessage(Message::Shared msg) {
    // Получаем поток данных
    auto stream = msg->stream();

    // Читаем тип пакета (первый байт)
    uint8_t packetType = 0;
    stream->read(packetType);

    if (packetType == GamePacket::INIT) {
        // Читаем InitData (просто ID)
        stream->read(myPlayerId);
    }
    else if (packetType == GamePacket::SNAPSHOT) {
        // Читаем количество сущностей
        uint32_t entityCount = 0;
        stream->read(entityCount);

        std::vector<uint32_t> receivedIds;
        for (uint32_t i = 0; i < entityCount; i++) {
            EntityState st;
            stream >> st; // Десериализация сущности (через оператор >> в NetworkPackets.h)

            receivedIds.push_back(st.id);
            worldEntities[st.id].PushState(st);
        }

        // Удаляем устаревшие сущности
        for (auto it = worldEntities.begin(); it != worldEntities.end();) {
            bool found = false;
            for (uint32_t id : receivedIds) if (id == it->first) found = true;
            if (!found) it = worldEntities.erase(it);
            else ++it;
        }
    }
}

void GameplayScene::Update(float dt) {
    // 1. Обновляем джойстики
    leftStick->Update();
    rightStick->Update();

    PlayerInputPacket pkt;
    // pkt.type больше не нужно, тип пишется в stream отдельно

    // 2. Логика движения
    Vector2 moveDir = leftStick->GetAxis();
    pkt.movement = moveDir;

    // Поддержка клавиатуры (WASD)
    if (IsKeyDown(KEY_W)) pkt.movement.y -= 1.0f;
    if (IsKeyDown(KEY_S)) pkt.movement.y += 1.0f;
    if (IsKeyDown(KEY_A)) pkt.movement.x -= 1.0f;
    if (IsKeyDown(KEY_D)) pkt.movement.x += 1.0f;

    // Нормализация
    if (Vector2Length(pkt.movement) > 1.0f) {
        pkt.movement = Vector2Normalize(pkt.movement);
    }

    // 3. Логика стрельбы
    Vector2 aimDir = rightStick->GetAxis();

    if (Vector2Length(aimDir) > 0.2f) { // Deadzone
        pkt.isShooting = true;
        if (worldEntities.count(myPlayerId)) {
            Vector2 playerPos = worldEntities[myPlayerId].renderPos;
            pkt.aimTarget = Vector2Add(playerPos, Vector2Scale(aimDir, 300.0f));
        }
    }
    else {
        pkt.aimTarget = GetScreenToWorld2D(GetMousePosition(), camera);
        pkt.isShooting = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    }

    if (game->netClient && game->netClient->isConnected()) {
        auto stream = StreamBuffer::alloc();

        stream->write((uint8_t)GamePacket::INPUT);
        stream << pkt;


        game->netClient->send(DeliveryType::UNRELIABLE, stream);
    }

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
            if (st.maxHealth > 0) {
                DrawRectangle((int)ent.renderPos.x - 20, (int)ent.renderPos.y - 40, 40, 5, RED);
                DrawRectangle((int)ent.renderPos.x - 20, (int)ent.renderPos.y - 40, (int)(40 * hpPct), 5, GREEN);
            }
        }
        else if (st.type == EntityType::BULLET) {
            DrawCircleV(ent.renderPos, st.radius, RED);
        }
    }
    EndMode2D();
}

void GameplayScene::DrawGUI() {
    DrawFPS(10, 10);

    if (game->netClient && !game->netClient->isConnected()) {
        DrawText("Connecting to server...", game->GetWidth() / 2 - 100, 20, 20, RED);
    }

    leftStick->Draw();
    rightStick->Draw();

    if (GuiButton(Rectangle{ (float)game->GetWidth() - 100, 10, 90, 30 }, "DISCONNECT")) {
        game->ReturnToMenu();
    }
}