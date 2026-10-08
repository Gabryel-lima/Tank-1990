// soundmanager.h
#pragma once
#include <SDL2/SDL_mixer.h>
#include <string>
#include <map>

class SoundManager {
public:
    static SoundManager& getInstance();

    bool init();
    void loadSounds();
    // Plays the sound associated with 'name'. 
    // 'loops' specifies the number of times to loop the sound (0 = play once, -1 = infinite).
    void playSound(const std::string& name, int loops = 0);
    void setVolume(int volume);
    // Sem som enquanto true (a demonstração no fundo do menu não toca nada)
    void setMuted(bool muted) { m_muted = muted; }
    void cleanup();

private:
    SoundManager() = default;
    ~SoundManager() = default;
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    std::map<std::string, Mix_Chunk*> m_sounds;
    bool m_muted = false;
    bool m_play_error_shown = false;
};
