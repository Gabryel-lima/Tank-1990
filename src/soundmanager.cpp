// sound_manager.cpp
#include "soundmanager.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

bool SoundManager::init() {
    // Mesma taxa, formato e bloco do CharyRick (44,1 kHz, float, 512 amostras), que toca no
    // EmuELEC com este mesmo SDL: o padrão do mixer (S16, 1024) abria o dispositivo sem erro e sem
    // nenhum som no Y6. Os sons do jogo são de 44,1 e 48 kHz; o mixer converte cada um
    if (Mix_OpenAudioDevice(44100, AUDIO_F32SYS, 2, 512, nullptr, 0) == -1) {
        std::cerr << "Mix_OpenAudio: " << Mix_GetError() << "\n";
        return false;
    }
    // O que o SDL abriu de verdade (vai para o log.txt do console de TV): sem som e sem erro,
    // aqui se vê se o driver é o "dummy" ou se o formato é outro
    int freq = 0, channels = 0;
    Uint16 format = 0;
    Mix_QuerySpec(&freq, &format, &channels);
    const char* driver = SDL_GetCurrentAudioDriver();
    std::cout << "Audio: driver=" << (driver != nullptr ? driver : "?") << " " << freq << " Hz, "
              << channels << " canais, formato 0x" << std::hex << format << std::dec << std::endl;
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
    // Nem todos os sons estão sendo utilizados. Melhor conferir daqui para baixo...
    m_sounds["steelhit"] = Mix_LoadWAV("resources/sound/test/steelhit.ogg");
    m_sounds["shieldhit"] = Mix_LoadWAV("resources/sound/test/shieldhit.ogg");
    m_sounds["tbonushit"] = Mix_LoadWAV("resources/sound/test/tbonushit.ogg");
    m_sounds["pause"] = Mix_LoadWAV("resources/sound/test/pause.ogg");
    m_sounds["life"] = Mix_LoadWAV("resources/sound/test/life.ogg");
    m_sounds["ice"] = Mix_LoadWAV("resources/sound/test/ice.ogg");
    m_sounds["fexplosion"] = Mix_LoadWAV("resources/sound/test/fexplosion.ogg");
    m_sounds["eexplosion"] = Mix_LoadWAV("resources/sound/test/eexplosion.ogg");

    // 64 de 128 (50%) é o volume de sempre. Os sons têm pico entre -11 e -24 dBFS e, a 50%, o
    // jogo sai uns 10 dB mais baixo que o CharyRick: na TV, com o volume do sistema baixo, quase
    // some. TANK_VOLUME (0 a 128) troca o valor sem recompilar; o lançador do EmuELEC a define
    int volume = 64;
    if (const char* v = std::getenv("TANK_VOLUME"); v != nullptr) {
        char* end = nullptr;
        long n = std::strtol(v, &end, 10);
        // Valor que não é número fica de fora: "abc" não pode virar 0 e calar o jogo
        if (end != v && *end == '\0')
            volume = static_cast<int>(std::clamp(n, 0L, static_cast<long>(MIX_MAX_VOLUME)));
    }

    for (auto& [name, chunk] : m_sounds) {
        if (!chunk) {
            std::cerr << "Erro ao carregar som [" << name << "]: " << Mix_GetError() << "\n";
        } else {
            Mix_VolumeChunk(chunk, volume);
        }
    }
}

void SoundManager::playSound(const std::string& name, int loops) {
    if (m_muted) return;
    auto it = m_sounds.find(name);
    if (it != m_sounds.end() && it->second) {
        // Falha ao tocar (sem canal livre, áudio fechado): avisa uma vez, para o log mostrar
        int channel = Mix_PlayChannel(-1, it->second, loops);
        if (channel < 0 && !m_play_error_shown) {
            std::cerr << "Erro ao tocar som [" << name << "]: " << Mix_GetError() << "\n";
            m_play_error_shown = true;
        }
        // O primeiro som que o jogo toca de verdade fica no log: canal, volume e quantos canais
        // tocam, para separar "o mixer não tocou" de "tocou e o sistema não reproduziu"
        if (channel >= 0 && !m_first_play_logged) {
            std::cout << "Primeiro som [" << name << "]: canal " << channel << ", volume "
                      << Mix_Volume(channel, -1) << "/" << Mix_VolumeChunk(it->second, -1)
                      << ", tocando " << Mix_Playing(-1) << ", canais " << Mix_AllocateChannels(-1)
                      << std::endl;
            m_first_play_logged = true;
        }
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
