#pragma once
#include "basetype.h"
#include "sfx.h"
#include "myerror.h"
#include <vorbis/vorbisfile.h>
#include <cstdio>
#include <stddef.h>
class RESOURCE;
struct HWND__;
struct IDirectSound8;
struct IGraphBuilder;
struct IMediaControl;
struct IMediaSeeking;

class MUSIC
{
public:
    explicit MUSIC(const STRING& source) : name(source), volume(0)
    {
    }
    virtual ~MUSIC() noexcept(false);
    static void operator delete(void* memory);
    virtual int IsPlaying();
    virtual void Tact();
    virtual void Pause();
    virtual void Resume();
    virtual void Stop();
    virtual void Play();
    virtual void SetVolume(int value);
    void Error(TYPE_ERROR type, const STRING& message, unsigned long value);

    STRING name;
    int volume;
};

class OGG_MUSIC : public MUSIC
{
public:
    explicit OGG_MUSIC(const STRING& source);
    virtual ~OGG_MUSIC() noexcept(false);
    virtual int IsPlaying();
    virtual void Tact();
    virtual void Pause();
    virtual void Resume();
    virtual void Stop();
    virtual void Play();
    virtual void SetVolume(int value);

    IDirectSoundBuffer* buffer;
    unsigned int playing;
    unsigned int state;
    OggVorbis_File vorbis;
    std::FILE* file;
    unsigned int streamOffset;
};

class DIRECT_SHOW_MUSIC : public MUSIC
{
public:
    explicit DIRECT_SHOW_MUSIC(const STRING& source);
    virtual ~DIRECT_SHOW_MUSIC() noexcept(false);
    virtual int IsPlaying();
    virtual void Pause();
    virtual void Resume();
    virtual void Stop();
    virtual void Play();
    virtual void SetVolume(int value);

    IGraphBuilder* graph;
    IMediaControl* control;
    IMediaSeeking* seeking;
};

struct SOUND_REQUEST
{
    int   sfxId=-1;
    int   m_04;
    int   pan;
    int   attenuation;
    float pitch=1.0f;
};

class SOUND
{
public:
    SOUND(HWND__* window,RESOURCE* resource,int enabled);
    ~SOUND() noexcept(false);
    void DisableSound();
    void DisableSpeech();
    void LoadSFX(RESOURCE* resource);
    void Pause();
    void Resume();
    int GetNoPlayed() const;
    void StopSFX(int sfxId);
    void PlaySFXFromCoor(int sfxId,const VECTOR2& position,float pitch);
    void PlaySFX(int sfxId,int pan,int attenuation,float pitch);
    void MusicTact();
    void Tact();
    int PauseMusic();
    int ResumeMusic();
    int StopMusic(unsigned long fade);
    void StopSound();
    void PauseSound();
    void ResumeSound();
    void VolumeSound(int volume);
    void VolumeMusic(int volume);
    int PlayFile(STRING name,int mode);
    int PlaySpeech(STRING name);
    int FadeAndPlayFile(const STRING& name,int mode,unsigned long fade);
    void VolumeSpeech(int volume);
    void PauseSpeech();
    void ResumeSpeech();
    void StopSpeech();
    void ReLoadBuffers();
    IDirectSoundBuffer* CreateSoundBufferFromOgg(const STRING& name,
        OggVorbis_File* vorbis, std::FILE** fileOut, int requestedBytes) const;
    IDirectSoundBuffer* CreateSoundBufferFromWav(const STRING& name,
        RESOURCE* resource, int mode) const;
    void DisableMusic();
    void Disable();

private:
    void ReleaseDS();
    void InitDS();
    int LoadBuffer(int sfxId);

public:

    unsigned int  m_flags;
    int           m_sfxCount;
    SFX*          m_sfx;
    SOUND_REQUEST m_requests[64];
    HWND__*       m_window;
    IDirectSound8* m_directSound;
    SFXBUFFER     m_channels[30];
    int           m_mciPlayToValue;
    int           m_mciPlayToCondition;
    unsigned char m_reserved954[12];
    unsigned int  m_dsState;
    int           m_auxDeviceId;
    int           m_waveOutDeviceId;
    unsigned int  m_auxRestoreVolume;
    unsigned int  m_waveOutRestoreVolume;
    unsigned int  m_auxMusicVolume;
    unsigned int  m_978;
    int           m_soundVolume;
    int           m_musicVolume;
    int           m_speechVolume;
    int           m_musicMode;
    MUSIC*        m_music;
    MUSIC*        m_speech;
    STRING        m_musicName;
    unsigned int  m_fadeDuration;
    unsigned int  m_fadeStart;
    unsigned int  m_mciMusicOpen;
};

