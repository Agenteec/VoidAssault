    file(GLOB CLIENT_SOURCES "*.cpp" "scenes/*.cpp" "vircontrols/*.cpp")
#file(GLOB_RECURSE Imgui_SFML_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../imgui-sfml/*.*")
if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux" OR LINUX)

    add_executable(GameClient
        ${CLIENT_SOURCES}
    )

    #if(WIN32)
      #  set_target_properties(GameClient PROPERTIES WIN32_EXECUTABLE ON)
    #endif()

    target_link_libraries(GameClient PRIVATE raylib GameEngine GameCommon enet::enet_shared nlohmann_json Bitsery::bitsery)

    target_include_directories(GameClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(GameClient PRIVATE ${PROJECT_INCLUDE_DIR})
    target_include_directories(GameClient PRIVATE ${ENET_EXTERNAL_INCLUDE_DIR})
    target_include_directories(GameClient PRIVATE ${RAYGUI_EXTERNAL_INCLUDE_DIR})
    target_include_directories(GameClient PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})
    target_include_directories(GameClient PUBLIC ${BITSERY_EXTERNAL_INCLUDE_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${GLOBAL_OUTPUT_DIR})

elseif(ANDROID)
    add_library(gameclient SHARED ${CLIENT_SOURCES})

    target_link_libraries(gameclient  PRIVATE android log
        raylib
    GameEngine 
    GameCommon
    enet::enet_shared
    chipmunk_static
    nlohmann_json
    Bitsery::bitsery
    lz4_shared)
    target_include_directories(gameclient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(gameclient PRIVATE ${PROJECT_INCLUDE_DIR})
    target_include_directories(gameclient PRIVATE ${ENET_EXTERNAL_INCLUDE_DIR})
    target_include_directories(gameclient PRIVATE ${RAYGUI_EXTERNAL_INCLUDE_DIR})
    target_include_directories(gameclient PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})
    target_include_directories(gameclient PUBLIC ${RAYMOB_EXTERNAL_INCLUDE_DIR})
    target_include_directories(gameclient PUBLIC ${BITSERY_EXTERNAL_INCLUDE_DIR})

target_include_directories(gameclient PUBLIC "${CMAKE_SOURCE_DIR}/external")
endif()





