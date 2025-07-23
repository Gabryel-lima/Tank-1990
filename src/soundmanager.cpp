// sound_manager.cpp
#include "soundmanager.h"
#include <iostream>

SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

bool SoundManager::init() {
    if (Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {
        std::cerr << "Mix_OpenAudio: " << Mix_GetError() << "\n";
        return false;
    }
    return true;
}

void SoundManager::loadSounds() {
    m_sounds["shell_exp"] = Mix_LoadWAV("resources/sound/ShellExplosion.wav");
    m_sounds["player_exp"] = Mix_LoadWAV("resources/sound/TankExplosion.wav");
    m_sounds["shoot"] = Mix_LoadWAV("resources/sound/test/shoot.ogg");
    m_sounds["game_over"] = Mix_LoadWAV("resources/sound/test/gameover.ogg");
    m_sounds["bonus"] = Mix_LoadWAV("resources/sound/test/bonus.ogg");
    m_sounds["level_starting"] = Mix_LoadWAV("resources/sound/test/levelstarting.ogg");
    m_sounds["brick_hit"] = Mix_LoadWAV("resources/sound/test/brickhit.ogg");

    for (auto& [name, chunk] : m_sounds) {
        if (!chunk) {
            std::cerr << "Erro ao carregar som [" << name << "]: " << Mix_GetError() << "\n";
        } else {
            Mix_VolumeChunk(chunk, 64);  // 50% volume
        }
    }
}

void SoundManager::playSound(const std::string& name, int loops) {
    auto it = m_sounds.find(name);
    if (it != m_sounds.end() && it->second) {
        Mix_PlayChannel(-1, it->second, loops);
    }
}

void SoundManager::setVolume(int volume) {
    for (auto& [_, chunk] : m_sounds)
        Mix_VolumeChunk(chunk, volume);
}

void SoundManager::cleanup() {
    for (auto& [_, chunk] : m_sounds) {
        Mix_FreeChunk(chunk);
    }
    m_sounds.clear();
    Mix_CloseAudio();
}
