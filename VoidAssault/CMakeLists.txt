project ("VoidAssault" VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENET_TEST OFF CACHE BOOL "Build enet tests" FORCE)
set(ENET_STATIC OFF CACHE BOOL "Build enet as a static library" FORCE)
set(ENET_SHARED ON CACHE BOOL "Build enet as a shared library" FORCE)

set(BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(INSTALL_CHIPMUNK OFF CACHE BOOL "" FORCE)

set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC ON CACHE BOOL "" FORCE)

set(GLOBAL_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_DIR})

if(WIN32)
    add_compile_definitions(ENET_DLL)
    add_compile_definitions(NOMINMAX)

endif()
if(UNIX AND NOT APPLE AND NOT ANDROID)
    add_compile_options(-fno-fast-math)
endif()

if(ANDROID)

 if(ANDROID_ABI STREQUAL "arm64-v8a" OR ANDROID_ABI STREQUAL "x86_64")
        message(STATUS "Building for 64-bit ABI (${ANDROID_ABI}), enabling CP_USE_DOUBLES.")
        add_definitions(-DCP_USE_DOUBLES=1)
    else()
        message(STATUS "Building for 32-bit ABI (${ANDROID_ABI}), CP_USE_DOUBLES DISABLED.")
        add_definitions(-DCP_USE_DOUBLES=0)
    endif()


    message(STATUS "Building for Android platform")
    string(REGEX MATCH "android-([0-9]+)" _ ${ANDROID_PLATFORM} FORCE)
    set(ANDROID_API_VERSION ${CMAKE_MATCH_1} CACHE STRING "" FORCE)
    set(ANDROID_ARCH ${CMAKE_ANDROID_ARCH} CACHE STRING "" FORCE)
    set(PLATFORM "Android" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(external/raylib)
add_subdirectory(external/json)
add_subdirectory(external/enet)
add_subdirectory(external/Chipmunk2D)
add_subdirectory(external/bitsery)


set(RAYGUI_EXTERNAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/raygui")
set(FIX_EXTERNAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/fix")
set(ENET_EXTERNAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/enet/include")
set(CHIPMUNK2D_EXTERNAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/Chipmunk2D/include")
set(BITSERY_EXTERNAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/bitsery/include")

set(PROJECT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
add_subdirectory(src/engine)
add_subdirectory(src/server)
add_subdirectory(src/client)
add_subdirectory(src/common)
add_subdirectory(src/tests)


