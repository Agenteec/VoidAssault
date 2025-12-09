add_library(GameCommon SHARED
    NetworkPackets.h Common.cpp
)

target_link_libraries(GameCommon PUBLIC enet::enet_shared raylib)
target_include_directories(GameCommon PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})
target_include_directories(GameCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})