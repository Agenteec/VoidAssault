#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include "raylib.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct ClientConfig {
    std::string playerName = "Player";
    std::string lastIp = "127.0.0.1";
    std::string language = "ru";
    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 0.8f;
    bool fullscreen = false;
    int targetFPS = 60;
    int lastPort = 7777;
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