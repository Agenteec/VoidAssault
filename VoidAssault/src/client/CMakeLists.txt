# Собираем все исходники клиента в одну переменную
file(GLOB CLIENT_SOURCES 
    "*.cpp" 
    "Scenes/*.cpp"
    # Обязательно включаем реализацию GUI, которую мы создали
    "raygui_impl.cpp" 
)

# --- Ветка для Windows / Linux ---
if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux" OR LINUX)
    add_executable(gameclient ${CLIENT_SOURCES})

    if(WIN32)
        # Отключаем консоль (если нужно) и GDI
        target_compile_definitions(gameclient PRIVATE NOGDI)
        # set_target_properties(gameclient PROPERTIES WIN32_EXECUTABLE ON) 
        if(MSVC)
            set_target_properties(gameclient PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
        endif()
    endif()

    # Копирование ассетов для ПК
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# --- Ветка для Android ---
elseif(ANDROID)
    # Android требует SHARED библиотеку
    add_library(gameclient  SHARED ${CLIENT_SOURCES})

    # Специфичные библиотеки Android
    target_link_libraries(gameclient  PRIVATE android log)
endif()

# --- Общие настройки линковки (И ДЛЯ ПК, И ДЛЯ ANDROID) ---
# Важно: GameEngine, GameCommon, ENet и Raylib нужны везде!
target_link_libraries(gameclient PRIVATE 
    raylib
    GameEngine 
    GameCommon
    enet::enet_shared
    chipmunk_static
)

# Общие пути к заголовкам
target_include_directories(gameclient PRIVATE ${ENET_EXTERNAL_INCLUDE_DIR})
target_include_directories(gameclient PRIVATE ${RAYGUI_EXTERNAL_INCLUDE_DIR})
# Путь к папке с fix_win32_compatibility.h
target_include_directories(gameclient PUBLIC "${CMAKE_SOURCE_DIR}/external")