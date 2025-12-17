#server module CMakeLists.txt
add_executable(GameServer
    main_server.cpp)
target_include_directories(GameServer PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})
target_link_libraries(GameServer PRIVATE GameEngine GameCommon enet::enet_shared)

add_executable(MasterServer
    main_master.cpp
)
target_include_directories(MasterServer PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})
target_link_libraries(MasterServer PRIVATE GameCommon enet::enet_shared)
