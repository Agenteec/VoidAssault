#Engine module CMakeLists.txt
add_library(GameEngine SHARED
    
    ECS/GameObject.h
    ECS/Player.h
    ECS/Bullet.h
    Scenes/GameScene.h
    ServerHost.h
 "ServerHost.h"
 ServerHost.cpp
)

target_include_directories(GameEngine PUBLIC ${CHIPMUNK2D_EXTERNAL_INCLUDE_DIR})
target_include_directories(GameEngine PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})

target_link_libraries(GameEngine PUBLIC chipmunk_static GameCommon enet::enet_shared raylib)
target_include_directories(GameEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
    target_link_libraries(GameEngine PRIVATE log)
endif()