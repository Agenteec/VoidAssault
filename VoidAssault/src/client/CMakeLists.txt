if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux" OR LINUX)
    file(GLOB CLIENT_SOURCES "*.cpp" "Scenes/*.cpp")
    add_executable(GameClient
        ${CLIENT_SOURCES}
     "scenes/GameplayScene.cpp" 
     "scenes/MainMenuScene.cpp"
         "scenes/GameplayScene.h" 
     "scenes/MainMenuScene.h"
    )

    #if(WIN32)
      #  set_target_properties(GameClient PROPERTIES WIN32_EXECUTABLE ON)
    #endif()

    target_link_libraries(GameClient PRIVATE raylib GameEngine GameCommon enet::enet_shared)

    target_include_directories(GameClient PRIVATE ${ENET_EXTERNAL_INCLUDE_DIR})
    target_include_directories(GameClient PRIVATE ${RAYGUI_EXTERNAL_INCLUDE_DIR})
    target_include_directories(GameClient PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()