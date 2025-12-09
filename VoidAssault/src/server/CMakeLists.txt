#server module CMakeLists.txt
add_executable(GameServer
    main_server.cpp
)
target_include_directories(GameServer PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})
target_link_libraries(GameServer PRIVATE GameEngine GameCommon enet::enet_shared)
