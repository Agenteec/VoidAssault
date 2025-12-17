// engine\Utils\ConfigManager.cpp
#include "ConfigManager.h"

GameConfig ConfigManager::config;
std::string ConfigManager::configPath;
Font ConfigManager::mainFont = { 0 };
std::map<std::string, std::string> ConfigManager::localizedStrings;

void ConfigManager::Initialize(const std::string& savePath) {
	if (!std::filesystem::exists(savePath)) {
		try { std::filesystem::create_directories(savePath); }
		catch (const std::exception& e) { std::cerr << e.what() << std::endl; }
	}
	configPath = savePath + "config.json";
	Load();
	LoadFonts();
	LoadLanguage(config.client.language);
}

void ConfigManager::CreateDefaultConfig() {
	config.client.playerName = "Player";
	config.client.lastIp = "127.0.0.1";
	config.server.port = 7777;
	config.client.favoriteServers.push_back({ "Localhost", "127.0.0.1", 7777 });
	config.client.resolutionWidth = 1280;
	config.client.resolutionHeight = 720;
	config.client.masterServerIp = MASTER_IP;
	config.client.masterServerPort = 8080;
	config.server.serverName = "My Void Server";
	Save();
}

void ConfigManager::Save() {
	json j;
	json jFav = json::array();
	for (const auto& s : config.client.favoriteServers) {
		jFav.push_back({ {"name", s.name}, {"ip", s.ip}, {"port", s.port} });
	}
	j["client"] = {
	{"playerName", config.client.playerName},
	{"lastIp", config.client.lastIp},
	{"lastPort", config.client.lastPort},
	{"language", config.client.language},
	{"fullscreen", config.client.fullscreen},
	{"targetFPS", config.client.targetFPS},
	{"resW", config.client.resolutionWidth},
	{"resH", config.client.resolutionHeight},
	{"masterIp", config.client.masterServerIp},
	{"masterPort", config.client.masterServerPort},
	{"favorites", jFav}
	};
	j["server"] = {
		{"port", config.server.port},
		{"pvpDamageFactor", config.server.pvpDamageFactor},
		{"maxPlayers", config.server.maxPlayers},
		{"tickRate", config.server.tickRate},
		{"serverName", config.server.serverName}
	};

	std::ofstream file(configPath);
	if (file.is_open()) file << j.dump(4);
}

void ConfigManager::Load() {
	if (!std::filesystem::exists(configPath)) { CreateDefaultConfig(); return; }
	std::ifstream file(configPath);
	if (file.is_open()) {
		try {
			json j = json::parse(file);
			if (j.contains("client")) {
				auto& c = j["client"];
				config.client.playerName = c.value("playerName", "Player");
				config.client.lastIp = c.value("lastIp", "127.0.0.1");
				config.client.lastPort = c.value("lastPort", 7777);
				config.client.targetFPS = c.value("targetFPS", 60);
				config.client.resolutionWidth = c.value("resW", 1280);
				config.client.resolutionHeight = c.value("resH", 720);
				config.client.masterServerIp = c.value("masterIp", "127.0.0.1");
				config.client.masterServerPort = c.value("masterPort", 8080);

				if (c.contains("favorites")) {
					config.client.favoriteServers.clear();
					for (auto& elem : c["favorites"]) {
						SavedServer s;
						s.name = elem.value("name", "Server");
						s.ip = elem.value("ip", "127.0.0.1");
						s.port = elem.value("port", 7777);
						config.client.favoriteServers.push_back(s);
					}
				}
			}
			if (j.contains("server")) {
				config.server.port = j["server"].value("port", 7777);
				config.server.pvpDamageFactor = j["server"].value("pvpDamageFactor", 1.0f);
				config.server.maxPlayers = j["server"].value("maxPlayers", 8);
				config.server.tickRate = j["server"].value("tickRate", 60);
				config.server.serverName = j["server"].value("serverName", "Void Server");
			}
		}
		catch (...) { CreateDefaultConfig(); }
	}
}

void ConfigManager::LoadFonts() {
	int codepoints[512] = { 0 };
	for (int i = 0; i < 95; i++) codepoints[i] = 32 + i;
	for (int i = 0; i < 255; i++) codepoints[96 + i] = 0x400 + i;
	const char* fontPath = "assets/fonts/Roboto-Regular.ttf";
	if (FileExists(fontPath)) {
		mainFont = LoadFontEx(fontPath, 64, codepoints, 512);
		GenTextureMipmaps(&mainFont.texture);
		SetTextureFilter(mainFont.texture, TEXTURE_FILTER_BILINEAR);
	}
	else { mainFont = GetFontDefault(); }
}

void ConfigManager::UnloadResources() { if (mainFont.texture.id != 0) UnloadFont(mainFont); }

void ConfigManager::LoadLanguage(const std::string& langCode) {
	localizedStrings.clear();
	std::string path = "assets/lang/lang_" + langCode + ".json";
	char* text = LoadFileText(path.c_str());
	if (text) {
		try {
			json j = json::parse(text);
			for (auto& element : j.items()) localizedStrings[element.key()] = element.value().get<std::string>();
		}
		catch (...) {}
		UnloadFileText(text);
	}
}

const char* ConfigManager::Text(const std::string& key) {
	if (localizedStrings.count(key)) return localizedStrings[key].c_str();
	return key.c_str();
}

std::string ConfigManager::GetCurrentLangName()
{
	if (localizedStrings.count("lang_name")) return localizedStrings["lang_name"];
	return config.client.language;
}

void ConfigManager::CycleLanguage()
{
	if (config.client.language == "en") {
		config.client.language = "ru";
	}
	else {
		config.client.language = "en";
	}
	LoadLanguage(config.client.language);
	Save();
}

ClientConfig& ConfigManager::GetClient() { return config.client; }
ServerConfig& ConfigManager::GetServer() { return config.server; }
Font ConfigManager::GetFont() { return mainFont; }
void ConfigManager::SetFont(Font font) { mainFont = font; }