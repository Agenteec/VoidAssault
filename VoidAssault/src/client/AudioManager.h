#pragma once
#include "raylib.h"
#include <map>
#include <string>
#include <iostream>

class AudioManager {
    std::map<std::string, Sound> sounds;
    std::map<std::string, Music> musics;
    float masterVolume = 1.0f;

public:
    AudioManager() {
        InitAudioDevice();
    }

    ~AudioManager() {
        for (auto& s : sounds) UnloadSound(s.second);
        for (auto& m : musics) UnloadMusicStream(m.second);
        CloseAudioDevice();
    }

    void LoadSnd(const std::string& key, const std::string& path) {
        Sound s = LoadSound(path.c_str());
        if (s.frameCount > 0) sounds[key] = s;
        else std::cout << "Failed to load sound: " << path << std::endl;
    }

    void LoadMus(const std::string& key, const std::string& path) {
        Music m = LoadMusicStream(path.c_str());
        if (m.frameCount > 0) musics[key] = m;
    }

    void PlaySnd(const std::string& key) {
        if (sounds.count(key)) {
            SetSoundVolume(sounds[key], masterVolume);
            PlaySound(sounds[key]);
        }
    }

    void PlayMus(const std::string& key) {
        if (musics.count(key)) {
            if (!IsMusicStreamPlaying(musics[key])) PlayMusicStream(musics[key]);
            SetMusicVolume(musics[key], masterVolume);
            UpdateMusicStream(musics[key]);
        }
    }

    void Update() {
        for (auto& pair : musics) {
            if (IsMusicStreamPlaying(pair.second)) UpdateMusicStream(pair.second);
        }
    }

    void SetVolume(float vol) { masterVolume = vol; }
    float GetVolume() const { return masterVolume; }
};