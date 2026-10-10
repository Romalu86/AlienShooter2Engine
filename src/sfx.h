#pragma once
#include "mylib.h"

struct IDirectSoundBuffer;
struct IDirectSound;
class SOUND;

enum
{
    P_LOOP = 1,
    P_DIST_INDEPEND = 2,
    P_DIR_INDEPEND = 4,
    P_VIP = 8,
    P_FFB_LOOP = 0x10,
    P_FFB_DIST_INDEPEND = 0x20,
    P_FFB_DIR_INDEPEND = 0x40
};

class SFXBUFFER
{
public:
    SFXBUFFER();
    virtual ~SFXBUFFER() noexcept(false);

    void Destroy();
    void Play(int pan,int volume,float pitch);
    void Resume();
    int CheckPlay();
    void Load(int index,IDirectSoundBuffer* buffer);

    IDirectSoundBuffer* m_buffer;
    unsigned int m_playing;
    int          m_sfxIndex;
    int          m_volume;
    int          m_pan;
    float        m_pitch;
    unsigned long m_baseFrequency;
    unsigned int m_lastPlayTick;
};

class SFX
{
public:
    SFX();
    ~SFX() noexcept(false);

    void Release();
    IDirectSoundBuffer* Duplicate(IDirectSound* directSound);
    void Load(STRING* wave,unsigned long property,unsigned char priority,STRING* forceFeedBack,SOUND* sound);

    STRING       Wave[8];
    STRING       ForceFeedBack[8];
    unsigned long Property;
    unsigned char Priority;
    IDirectSoundBuffer* m_buffers[8];
    unsigned int m_bufferCount;
};

