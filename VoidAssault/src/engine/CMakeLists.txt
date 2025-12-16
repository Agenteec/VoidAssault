add_library(GameEngine SHARED
    Utils/ConfigManager.h
    Utils/ConfigManager.cpp
    ECS/GameObject.h
    ECS/Player.h
    ECS/Bullet.h
    Scenes/GameScene.h
    ServerHost.h
 "ServerHost.h"
 ServerHost.cpp
 "Utils/ConfigManager.h" "ECS/PhysicsUtils.h" "ECS/Enemy.h" "ECS/Artifact.h" "ECS/Construct.h" "Utils/MasterServerIP.h")
target_include_directories(GameEngine PUBLIC ${CHIPMUNK2D_EXTERNAL_INCLUDE_DIR})
target_include_directories(GameEngine PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})
target_include_directories(GameEngine PUBLIC ${BITSTERY_EXTERNAL_INCLUDE_DIR})

target_link_libraries(GameEngine PUBLIC chipmunk_static GameCommon enet::enet_shared raylib nlohmann_json Bitsery::bitsery)
target_include_directories(GameEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
    target_link_libraries(GameEngine PRIVATE log)
endif()