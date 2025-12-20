set(LZ4_SOURCES "${LZ4_EXTERNAL_INCLUDE_DIR}/lz4.c")
file(GLOB COMMON_SOURCES "*.cpp" "net/*.cpp" "serial/*.cpp" "enet/*.cpp" "time/*.cpp" ${LZ4_SOURCES})

add_library(GameCommon SHARED ${COMMON_SOURCES})

target_link_libraries(GameCommon PUBLIC enet::enet_shared raylib Bitsery::bitsery)
target_include_directories(GameCommon PUBLIC ${FIX_EXTERNAL_INCLUDE_DIR})
target_include_directories(GameCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(GameCommon PUBLIC ${BITSERY_EXTERNAL_INCLUDE_DIR})
target_include_directories(GameCommon PUBLIC ${LZ4_EXTERNAL_INCLUDE_DIR})