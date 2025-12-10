#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include <vector> // Важно для vector
#include "raylib.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// Структура для избранного сервера
struct SavedServer {
    std::string name;
    std::string ip;
    int port;
};

struct ClientConfig {
    std::string playerName = "Player";
    std::string lastIp = "127.0.0.1";
    std::string language = "ru";

    // Новые поля для Master Server
    std::string masterServerIp = "127.0.0.1";
    int masterServerPort = 7777; // Обычно мастер висит на другом порту, например 8080, но пусть будет 7777 если так задумано

    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 0.8f;
    bool fullscreen = false;
    int targetFPS = 60;
    int lastPort = 7777;

    // Список избранного
    std::vector<SavedServer> favoriteServers;
};

struct ServerConfig {
    int port = 7777;
    int maxPlayers = 8;
    int tickRate = 60;
    std::string serverName = "Void Assault Server";
    std::string password = "";
};

struct GameConfig {
    ClientConfig client;
    ServerConfig server;
};

class ConfigManager {
private:
    static GameConfig config;
    static std::string configPath;
    static Font mainFont;
    static std::map<std::string, std::string> localizedStrings;

    static void CreateDefaultConfig();

public:
    static void Initialize(const std::string& savePath);

    static void Save();
    static void Load();

    static void LoadLanguage(const std::string& langCode);
    static const char* Text(const std::string& key);

    static ClientConfig& GetClient();
    static ServerConfig& GetServer();

    static void LoadFonts();
    static Font GetFont();
    static void SetFont(Font font);
    static void UnloadResources();
};