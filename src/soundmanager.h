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
    void cleanup();

private:
    SoundManager() = default;
    ~SoundManager() = default;
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    std::map<std::string, Mix_Chunk*> m_sounds;
};
