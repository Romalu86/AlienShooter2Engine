#if defined(_WIN32)
#define INPUT WIN32_INPUT
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <dshow.h>
#undef INPUT
#endif
#include "engine_globals.h"
#include "myerror.h"
#include "mylib.h"
#include "sound.h"
#include "sfx.h"
#include "resread.h"
#include "basetype.h"
#include <vorbis/vorbisfile.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <exception>

#include <new>

OGG_MUSIC::OGG_MUSIC(const STRING& source) : MUSIC(source)
{
    playing = 0u;
    SOUND* const manager = ::Sound;
    buffer = manager->CreateSoundBufferFromOgg(source, &vorbis, &file, 262144);
    if (!buffer)
        Error(E_CREATE, "SoundBuffer", 0u);
}

OGG_MUSIC::~OGG_MUSIC() noexcept(false)
{
    Stop();
    IDirectSoundBuffer* const currentBuffer = buffer;
    if (currentBuffer)
    {
        const unsigned long remainingReferences = currentBuffer->Release();
        if (remainingReferences != 0u)
            Error(E_ERROR, "SoundBuffer release !=0", remainingReferences);
        buffer = 0;
    }
    if (file)
    {
        (void)ov_clear_noclose(&vorbis);
        (void)std::fclose(file);
        file = 0;
    }
}

void OGG_MUSIC::Play()
{
    IDirectSoundBuffer* currentBuffer = buffer;
    if (!currentBuffer || !file)
        return;
    (void)currentBuffer->Stop();
    std::rewind(file);
    currentBuffer = buffer;
    streamOffset = 16u;
    state = 0u;
    (void)currentBuffer->SetCurrentPosition(0u);
    (void)ov_raw_seek(&vorbis, 0);
    playing = 1u;
    Tact();
    currentBuffer = buffer;
    (void)currentBuffer->Play(0u, 0u, DSBPLAY_LOOPING);
}

void OGG_MUSIC::Stop()
{
    playing = 0u;
    IDirectSoundBuffer* const currentBuffer = buffer;
    if (currentBuffer)
        (void)currentBuffer->Stop();
}

void OGG_MUSIC::Pause()
{
    if (playing == 0u)
        return;
    IDirectSoundBuffer* const currentBuffer = buffer;
    if (currentBuffer)
        (void)currentBuffer->Stop();
}

void OGG_MUSIC::Resume()
{
    if (playing == 0u)
        return;
    IDirectSoundBuffer* const currentBuffer = buffer;
    if (currentBuffer)
        (void)currentBuffer->Play(0u, 0u, DSBPLAY_LOOPING);
}

int OGG_MUSIC::IsPlaying()
{
    IDirectSoundBuffer* const currentBuffer = buffer;
    if (!currentBuffer || playing == 0u)
        return 0;
    DWORD bufferStatus;
    (void)currentBuffer->GetStatus(&bufferStatus);
    if ((bufferStatus & DSBSTATUS_PLAYING) == 0u)
        playing = 0u;
    return static_cast<int>(playing);
}

void OGG_MUSIC::SetVolume(int volume)
{
    IDirectSoundBuffer* const currentBuffer = buffer;
    this->volume = volume;
    if (currentBuffer)
        (void)currentBuffer->SetVolume(volume);
}

void OGG_MUSIC::Tact()
{
    static char decodedSamples[4096];
    if (!buffer || playing == 0u)
        return;
    DWORD playCursor, writeCursor;
    (void)buffer->GetCurrentPosition(&playCursor, &writeCursor);
    while (state == 0u)
    {
        const unsigned int writeOffset = streamOffset;
        const unsigned int distance = playCursor - writeOffset +
            (playCursor < writeOffset ? 262144u : 0u);
        if (distance < 4096u)
            break;
        int logicalBitstreamIndex;
        const long decodedBytes = ov_read(&vorbis, decodedSamples,
            4096, 0, 2, 1, &logicalBitstreamIndex);
        if (decodedBytes == 0)
        {
            state = streamOffset < playCursor ? 2u : 1u;
            continue;
        }
        if (decodedBytes < 0)
        {
            Error(E_ERROR, "decode", 0u);
            continue;
        }
        void* firstRegion;
        void* secondRegion;
        DWORD firstRegionBytes, secondRegionBytes;
        const HRESULT lockResult = buffer->Lock(streamOffset,
            static_cast<DWORD>(decodedBytes), &firstRegion, &firstRegionBytes,
            &secondRegion, &secondRegionBytes, 0u);
        if (lockResult >= 0)
        {
            if (decodedBytes <= static_cast<long>(firstRegionBytes))
            {
                std::memcpy(firstRegion, decodedSamples,
                    static_cast<unsigned int>(decodedBytes));
            }
            else
            {
                std::memcpy(firstRegion, decodedSamples, firstRegionBytes);
                if (secondRegionBytes)
                    std::memcpy(secondRegion, decodedSamples + firstRegionBytes,
                        static_cast<unsigned int>(decodedBytes) - firstRegionBytes);
            }
            (void)buffer->Unlock(firstRegion, firstRegionBytes,
                secondRegion, secondRegionBytes);
        }
        streamOffset = (streamOffset + static_cast<unsigned int>(decodedBytes)) & 0x3FFFFu;
    }
    if (state != 0u)
    {
        const unsigned int writeOffset = streamOffset;
        const unsigned int distance = playCursor - writeOffset +
            (playCursor < writeOffset ? 262144u : 0u);
        if (distance >= 4096u)
        {
            void* firstRegion;
            void* secondRegion;
            DWORD firstRegionBytes, secondRegionBytes;
            const HRESULT lockResult = buffer->Lock(writeOffset, distance,
                &firstRegion, &firstRegionBytes, &secondRegion, &secondRegionBytes, 0u);
            if (lockResult >= 0)
            {
                std::memset(firstRegion, 0, firstRegionBytes);
                if (secondRegionBytes)
                    std::memset(secondRegion, 0, secondRegionBytes);
                (void)buffer->Unlock(firstRegion, firstRegionBytes,
                    secondRegion, secondRegionBytes);
            }
        }
    }
    if (state == 2u && playCursor < streamOffset)
        state = 1u;
    if (state == 1u && playCursor >= streamOffset)
        Stop();
}

DIRECT_SHOW_MUSIC::~DIRECT_SHOW_MUSIC() noexcept(false)
{
    Stop();
    if (seeking)
    {
        seeking->Release();
        seeking = 0;
    }
    if (control)
    {
        control->Release();
        control = 0;
    }
    if (graph)
    {
        graph->Release();
        graph = 0;
    }
}

void DIRECT_SHOW_MUSIC::Play()
{
    IMediaSeeking* const mediaSeeking = seeking;
    if (mediaSeeking)
    {
        LONGLONG startPosition = 0;
        const HRESULT seekResult = mediaSeeking->SetPositions(&startPosition,
            AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning);
        if (seekResult < 0)
        {
            IMediaControl* const mediaControl = control;
            if (mediaControl)
                (void)mediaControl->Stop();
        }
    }
    IMediaControl* const mediaControl = control;
    if (mediaControl)
        (void)mediaControl->Run();
}

void DIRECT_SHOW_MUSIC::Stop()
{
    IMediaControl* const mediaControl = control;
    if (mediaControl)
        (void)mediaControl->Stop();
}

void DIRECT_SHOW_MUSIC::Pause()
{
    IMediaControl* const mediaControl = control;
    if (mediaControl)
        (void)mediaControl->Pause();
}

void DIRECT_SHOW_MUSIC::Resume()
{
    IMediaControl* const mediaControl = control;
    if (mediaControl)
        (void)mediaControl->Run();
}

int DIRECT_SHOW_MUSIC::IsPlaying()
{
    IMediaControl* const mediaControl = control;
    if (!mediaControl)
        return 0;
    OAFilterState state;
    (void)mediaControl->GetState(3000, &state);
    if (state != State_Running)
        return 0;
    LONGLONG currentPosition, stopPosition;
    IMediaSeeking* const mediaSeeking = seeking;
    (void)mediaSeeking->GetPositions(&currentPosition, &stopPosition);
    return currentPosition != stopPosition;
}

void DIRECT_SHOW_MUSIC::SetVolume(int volume)
{
    IGraphBuilder* const currentGraph = graph;
    IBasicAudio* basicAudio = 0;
    if (!currentGraph)
        return;
    this->volume = volume;
    HRESULT audioResult = currentGraph->QueryInterface(IID_IBasicAudio,
        reinterpret_cast<void**>(&basicAudio));
    if (audioResult < 0)
    {
        Error(E_GET, "BasicAudio", static_cast<unsigned long>(audioResult));
        return;
    }
    long currentVolume;
    audioResult = basicAudio->get_Volume(&currentVolume);
    if (audioResult != E_NOTIMPL)
    {
        if (audioResult >= 0)
        {
            audioResult = basicAudio->put_Volume(volume);
            if (audioResult < 0)
                Error(E_SET, "Volume", static_cast<unsigned long>(audioResult));
        }
        else
        {
            Error(E_GET, "Volume", static_cast<unsigned long>(audioResult));
        }
    }
    (void)basicAudio->Release();
}

SOUND::~SOUND() noexcept(false)
{
    MUSIC* const music = m_music;
    if (music)
    {
        music->~MUSIC();
        MUSIC::operator delete(music);
        m_music = 0;
    }
    MUSIC* const speech = m_speech;
    if (speech)
    {
        speech->~MUSIC();
        MUSIC::operator delete(speech);
        m_speech = 0;
    }
    Disable();
    if (m_sfx)
    {
        unsigned int* const header = reinterpret_cast<unsigned int*>(m_sfx) - 1;
        SFX* const objects = m_sfx;
        unsigned int remaining = *header;
        try
        {
            while (static_cast<int>(--remaining) >= 0)
                objects[remaining].~SFX();
        }
        catch (...)
        {
            try
            {
                while (static_cast<int>(--remaining) >= 0)
                    objects[remaining].~SFX();
            }
            catch (...)
            {
                std::terminate();
            }
            throw;
        }
        CrtFreePointer(header);
    }
    m_sfx = 0;
    m_sfxCount = 0;
}

void SOUND::ReleaseDS()
{
    SOUND* const sound = this;
    if (sound->m_dsState != 0u)return;

    for (int slotIndex = 0;slotIndex<30;++slotIndex)sound->m_channels[slotIndex].Destroy();
    for (int slotIndex = 0;slotIndex<sound->m_sfxCount;++slotIndex)sound->m_sfx[slotIndex].Release();

    IDirectSound* backend = sound->m_directSound;
    const unsigned int remainingReferences = backend->Release();
    if (remainingReferences != 0u)
    {
        {
            STRING message("DirectSound release !=0");
            STRING category("SOUND");
            ::Error->Error(
            category, E_ERROR,
            message, remainingReferences);
        }
    }

    sound->m_directSound = 0;
    for (unsigned int slotIndex = 0;slotIndex<64u;++slotIndex)sound->m_requests[slotIndex].sfxId = -1;
    sound->m_dsState = 1u;
}

int SOUND::GetNoPlayed() const
{
    const SOUND* const sound = this;
    if (sound->m_dsState != 0u)return 0;
    int playingChannelCount = 0;
    for (unsigned int channelIndex = 0;channelIndex<30u;++channelIndex)if (sound->m_channels[channelIndex].m_playing != 0u)++playingChannelCount;
    return playingChannelCount;
}

int SOUND::LoadBuffer(int sfxId)
{
    SOUND* const sound = this;
    if (sound->m_dsState != 0u)return -1;
    if (sound->m_sfx && (sfxId<0 || sfxId>sound->m_sfxCount || sound->m_sfx[sfxId].m_bufferCount == 0u))
    {
        {
            STRING message("nsfx");
            STRING category("SOUND");
            ::Error->Error(category, E_INVALID, message, static_cast<unsigned int>(sfxId));
        }
        return -1;
    }

    SFX* const record = &sound->m_sfx[sfxId];
    const unsigned int currentTick = ::CurrentTime;
    for (int channelIndex = 0;channelIndex<30;++channelIndex)
    {
        SFXBUFFER& channel = sound->m_channels[channelIndex];
        if (channel.m_sfxIndex != sfxId)continue;
        if (currentTick-channel.m_lastPlayTick <= 40u)return -1;
        if (record->m_bufferCount == 1u && channel.m_playing == 0u)return channelIndex;
    }

    int selectedChannelIndex = 0;
    while (selectedChannelIndex<30 && sound->m_channels[selectedChannelIndex].m_sfxIndex >= 0)++selectedChannelIndex;
    if (selectedChannelIndex >= 30)
    {
        selectedChannelIndex = 0;
        while (selectedChannelIndex<30 && sound->m_channels[selectedChannelIndex].m_playing != 0u)++selectedChannelIndex;
        if (selectedChannelIndex >= 30)return -1;
        sound->m_channels[selectedChannelIndex].Destroy();
    }

    IDirectSoundBuffer* duplicate = sound->m_sfx[sfxId].Duplicate(sound->m_directSound);
    if (!duplicate)return -1;
    sound->m_channels[selectedChannelIndex].Load(sfxId, duplicate);
    return selectedChannelIndex;
}

void SOUND::StopSFX(int sfxId)
{
    SOUND* const sound = this;
    if (sound->m_dsState != 0u)return;
    if (sfxId == -1)
    {
        StopSound();
        return;
    }
    for (int channelIndex = 0;channelIndex<30;++channelIndex) if (sound->m_channels[channelIndex].m_sfxIndex == sfxId)sound->m_channels[channelIndex].Destroy();
}

void SOUND::PlaySFXFromCoor(int sfxId, const VECTOR2& position, float pitch)
{
    SOUND* const sound = this;
    if (sound->m_dsState != 0u)return;
    if (sound->m_sfx && (sfxId<0 || sfxId>sound->m_sfxCount || sound->m_sfx[sfxId].m_bufferCount == 0u))
    {
        {
            STRING message("nsfx");
            STRING category("SOUND");
            ::Error->Error(category, E_INVALID, message, static_cast<unsigned int>(sfxId));
        }
        return;
    }
    const unsigned int flags = sound->m_sfx[sfxId].Property;
    if ((flags & (P_DIST_INDEPEND | P_DIR_INDEPEND)) == (P_DIST_INDEPEND | P_DIR_INDEPEND))
    {
        PlaySFX(sfxId, 0, 0, 1.0f);
        return;
    }
    const float positionX = position.x, positionY = position.y;
    const float absoluteX = static_cast<float>(std::fabs(positionX)), absoluteY = static_cast<float>(std::fabs(positionY));
    const int distance = TruncateFloatToInt32((absoluteX>absoluteY)?absoluteX:absoluteY);
    if (flags & P_DIST_INDEPEND)
    {
        const int pan = static_cast<int>(static_cast<unsigned int>(TruncateFloatToInt32(positionX))<<2);
        PlaySFX(sfxId, pan, 0, pitch);
        return;
    }
    const int attenuation = static_cast<int>((0u-static_cast<unsigned int>(distance))<<1);
    if (flags & P_DIR_INDEPEND)
    {
        PlaySFX(sfxId, 0, attenuation, pitch);
        return;
    }
    const int pan = static_cast<int>(static_cast<unsigned int>(TruncateFloatToInt32(positionX))<<2);
    PlaySFX(sfxId, pan, attenuation, pitch);
}

void SOUND::PlaySFX(int sfxId, int pan, int attenuation, float pitch)
{
    SOUND* const sound = this;
    if (sound->m_dsState != 0u)return;
    if (sound->m_sfx && (sfxId<0 || sfxId>sound->m_sfxCount || sound->m_sfx[sfxId].m_bufferCount == 0u))
    {
        {
            STRING message("nsfx");
            STRING category("SOUND");
            ::Error->Error(category, E_INVALID, message, static_cast<unsigned int>(sfxId));
        }
        return;
    }
    const int volumeShift = static_cast<int>((static_cast<unsigned int>(sound->m_soundVolume)-100u)<<5);
    attenuation = static_cast<int>(static_cast<unsigned int>(attenuation)+static_cast<unsigned int>(volumeShift));
    if (attenuation<-3000)return;
    if (pan>10000)pan = 10000;
    else if (pan<-10000)pan = -10000;
    SFX& requestedRecord = sound->m_sfx[sfxId];
    if (requestedRecord.Property & P_VIP)attenuation = volumeShift;
    int requestSlot = 0;
    while (requestSlot<64 && sound->m_requests[requestSlot].sfxId >= 0)++requestSlot;
    if (requestSlot >= 64)
    {
        int replacementIndex = 0;
        bool alreadyRequested = false;
        for (int requestIndex = 1;requestIndex<64;++requestIndex)
        {
            const int requestedSfxId = sound->m_requests[requestIndex].sfxId;
            if (requestedSfxId == sfxId)alreadyRequested = true;
            const int candidateId = sound->m_requests[replacementIndex].sfxId;
            const unsigned char currentPriority = sound->m_sfx[requestedSfxId].Priority;
            const unsigned char candidatePriority = sound->m_sfx[candidateId].Priority;
            if (currentPriority<candidatePriority)replacementIndex = requestIndex;
        }
        if (replacementIndex == 0 && alreadyRequested)return;
        requestSlot = replacementIndex;
    }
    SOUND_REQUEST& request = sound->m_requests[requestSlot];
    request.sfxId = sfxId;
    request.pan = pan;
    request.attenuation = attenuation;
    request.pitch = pitch;
}

void SOUND::MusicTact()
{
    MUSIC* playback = this->m_music;
    if (!playback)return;
    (void)playback->Tact();
    const unsigned int fadeStart = this->m_fadeStart;
    if (fadeStart != 0u)
    {
        const unsigned int now = ::RealCurrentTime;
        const unsigned int duration = this->m_fadeDuration;
        const unsigned int elapsed = now-fadeStart;
        if (elapsed >= duration)
        {
            this->m_fadeStart = 0u;
            const char* const name = this->m_musicName.value;
            if (std::strcmp(name, StringGlobals::EmptyText()) != 0)
            {
                (void)PlayFile(m_musicName, m_musicMode);
            }
            else
            {
                (void)StopMusic(0u);
            }
            return;
        }
        int level = static_cast<int>((fadeStart-now)*3000u)/static_cast<int>(duration);
        const int volume = this->m_musicVolume;
        if (volume >= 0)level = static_cast<int>(static_cast<unsigned int>(level)+
            ((static_cast<unsigned int>(volume)-100u)<<5));
        playback = this->m_music;
        (void)playback->SetVolume(level);
        return;
    }
    if (Random2(15) == 0)
    {
        playback = this->m_music;
        if (playback->IsPlaying() == 0u)
        {
            const char* const name = this->m_musicName.value;
            if (std::strcmp(name, StringGlobals::EmptyText()) != 0)
            {
                (void)PlayFile(m_musicName, m_musicMode);
                return;
            }
            else
            {
                (void)StopMusic(0u);
                return;
            }
        }
    }
    playback = this->m_music;
    MUSIC* speech = this->m_speech;
    const bool speechPlaying = speech && speech->IsPlaying() != 0u;
    playback = this->m_music;
    if (speechPlaying)
    {
        const int target = static_cast<int>((static_cast<unsigned int>(this->m_musicVolume)<<5)-4200u);
        const int current = playback->volume;
        if (current == target)return;
        const int next = static_cast<int>(static_cast<unsigned int>(current)-20u);
        (void)playback->SetVolume(next >= target?next:target);
        return;
    }
    const int target = static_cast<int>((static_cast<unsigned int>(this->m_musicVolume)-100u)<<5);
    const int current = playback->volume;
    if (current == target)return;
    const int next = static_cast<int>(static_cast<unsigned int>(current)+20u);
    (void)playback->SetVolume(next <= target?next:target);
}

void SOUND::Tact()
{

    SOUND* const sound = this;
    MusicTact();
    MUSIC* const speech = sound->m_speech;
    if (speech)(void)speech->Tact();
    if (sound->m_dsState != 0u)return;

    int pendingCount = 0;

    for (int requestIndex = 0;requestIndex<64;++requestIndex)
    {
        SOUND_REQUEST& request = sound->m_requests[requestIndex];
        const int sfxId = request.sfxId;
        if (sfxId<0)continue;

        int duplicateCount = 0;
        for (int channelIndex = 0;channelIndex<30;++channelIndex)
        {
            SFXBUFFER& channel = sound->m_channels[channelIndex];
            if (channel.m_sfxIndex != sfxId || channel.m_playing == 0u)continue;

            bool nearbyLoop = false;
            if (sound->m_sfx && sfxId >= 0 && sfxId <= sound->m_sfxCount)
            {
                SFX& record = sound->m_sfx[sfxId];
                if (record.m_bufferCount != 0u && (record.Property & P_LOOP) != 0u)
                {
                    const unsigned int volumeDifference = static_cast<unsigned int>(channel.m_volume) -
                        static_cast<unsigned int>(request.attenuation);
                    const int volumeDelta = static_cast<int>(volumeDifference > 2147483647u ?
                        0u - volumeDifference : volumeDifference);
                    if (volumeDelta<500)
                    {
                        const unsigned int panDifference = static_cast<unsigned int>(channel.m_pan) -
                            static_cast<unsigned int>(request.pan);
                        const int panDelta = static_cast<int>(panDifference > 2147483647u ?
                            0u - panDifference : panDifference);
                        nearbyLoop = (panDelta<500);
                    }
                }
            }

            if (nearbyLoop)
            {
                request.sfxId = -1;
                channel.Play(request.pan, request.attenuation, 1.0f);
                break;
            }

            ++duplicateCount;

            SFX& record = sound->m_sfx[sfxId];
            const int duplicateLimit = (record.Property & P_VIP)
                ? 999999
                : (static_cast<int>(record.Priority)+10)/10;
            if (duplicateCount>duplicateLimit)
            {
                request.sfxId = -1;
                break;
            }
        }

        if (request.sfxId >= 0)++pendingCount;
    }

    int freeCount = 0;
    for (int channelIndex = 0;channelIndex<30;++channelIndex)
        if (sound->m_channels[channelIndex].CheckPlay() == 0)++freeCount;

    while (pendingCount>freeCount)
    {
        if (sound->m_requests[0].sfxId<0)sound->m_requests[0].attenuation = 0;

        bool found = false;
        int candidateIndex = 0;
        int candidateAttenuation = sound->m_requests[0].attenuation;
        for (int requestIndex = 0;requestIndex<64;++requestIndex)
        {
            SOUND_REQUEST& request = sound->m_requests[requestIndex];
            const int sfxId = request.sfxId;
            if (sfxId<0)continue;

            SFX& record = sound->m_sfx[sfxId];
            if ((record.Property & P_VIP) != 0u)continue;
            if (request.attenuation>candidateAttenuation)continue;

            found = true;
            candidateIndex = requestIndex;
            candidateAttenuation = request.attenuation;
        }

        if (!found)break;
        sound->m_requests[candidateIndex].sfxId = -1;
        --pendingCount;
    }

    for (int channelIndex = 0;channelIndex<30 && pendingCount>freeCount;++channelIndex)
    {
        SFXBUFFER& channel = sound->m_channels[channelIndex];
        if (channel.m_playing == 0u)continue;

        const int sfxId = channel.m_sfxIndex;
        SFX& record = sound->m_sfx[sfxId];
        if ((record.Property & P_VIP) != 0u)continue;

        IDirectSoundBuffer* const buffer = channel.m_buffer;
        channel.m_playing = 0u;
        if (buffer)(void)buffer->Stop();
        ++freeCount;
    }

    for (int requestIndex = 0;requestIndex<64;++requestIndex)
    {
        SOUND_REQUEST& request = sound->m_requests[requestIndex];
        const int sfxId = request.sfxId;
        if (sfxId<0)continue;

        const int channelIndex = LoadBuffer(sfxId);
        if (channelIndex >= 0)
            sound->m_channels[channelIndex].Play(request.pan, request.attenuation, request.pitch);
        request.sfxId = -1;
    }
}

int SOUND::PauseMusic()
{
    MUSIC* music = this->m_music;
    if (music)(void)music->Pause();
    const int device = this->m_auxDeviceId;
    if (device >= 0)(void)auxSetVolume(static_cast<unsigned int>(device), this->m_auxRestoreVolume);
    if (this->m_mciMusicOpen != 0u && mciSendStringA("stop FWMUSIC", 0, 0u, this->m_window) != 0u)return 1;
    return 0;
}

int SOUND::ResumeMusic()
{
    char command[256];
    MUSIC* music = this->m_music;
    if (music)(void)music->Resume();
    const int device = this->m_auxDeviceId;
    if (device >= 0)(void)auxSetVolume(static_cast<unsigned int>(device), this->m_auxMusicVolume);
    if (this->m_mciMusicOpen == 0u)return 0;
    const char* text;
    if (this->m_mciPlayToCondition >= 0)
    {
        (void)std::sprintf(command, "play FWMUSIC to %i notify", static_cast<unsigned int>(this->m_mciPlayToValue)+1u);
        text = command;
    }
    else text = "play FWMUSIC notify";
    return mciSendStringA(text, 0, 0u, this->m_window)?1:0;
}

int SOUND::StopMusic(unsigned long fade)
{
    MUSIC* music = this->m_music;
    if (music)
    {
        if (fade)
        {
            this->m_fadeDuration = fade;
            this->m_fadeStart = ::RealCurrentTime;
        }
        else
        {
            this->m_fadeStart = 0u;
            this->m_fadeDuration = 0u;
            (void)music->Stop();
        }
    }
    this->m_musicName = StringGlobals::EmptyText();
    this->m_musicMode = 0u;
    this->m_mciPlayToCondition = -1;
    if (this->m_mciMusicOpen != 0u)
    {
        (void)mciSendStringA("stop FWMUSIC", 0, 0u, 0);
        if (mciSendStringA("close FWMUSIC", 0, 0u, 0) != 0u)return 1;
        this->m_mciMusicOpen = 0u;
    }
    return 0;
}

void SOUND::StopSound()
{
    StopSpeech();
    SOUND* const sound = this;
    if (sound->m_dsState != 0u)return;
    for (int channelIndex = 0;channelIndex<30;++channelIndex)
    {
        SFXBUFFER& channel = sound->m_channels[channelIndex];
        IDirectSoundBuffer* buffer = channel.m_buffer;
        channel.m_playing = 0u;
        if (buffer)(void)buffer->Stop();
    }
}

void SOUND::PauseSound()
{
    PauseSpeech();
    SOUND* const sound = this;
    if (sound->m_dsState != 0u)return;
    for (int channelIndex = 0;channelIndex<30;++channelIndex)
    {
        SFXBUFFER& channel = sound->m_channels[channelIndex];
        if (channel.m_playing == 0u)continue;
        IDirectSoundBuffer* buffer = channel.m_buffer;
        if (buffer)(void)buffer->Stop();
    }
}

void SOUND::ResumeSound()
{
    ResumeSpeech();
    SOUND* const sound = this;
    if (sound->m_dsState != 0u)return;
    for (int channelIndex = 0;channelIndex<30;++channelIndex)sound->m_channels[channelIndex].Resume();
}

void SOUND::Pause()
{
    (void)PauseMusic();
    PauseSound();
}

void SOUND::Resume()
{
    (void)ResumeMusic();
    ResumeSound();
}

void SOUND::VolumeSound(int volume)
{
    VolumeSpeech(volume);
    if (volume<0)volume = 0;
    else if (volume>100)volume = 100;
    this->m_soundVolume = volume;
}

void SOUND::VolumeMusic(int volume)
{
    if (volume < 0)
        volume = 0;
    else if (volume > 100)
        volume = 100;

    if (volume == 0)
    {
        if (m_music)
            DisableMusic();
    }
    else if (!m_music)
    {
        const unsigned int channelVolume = static_cast<unsigned int>((volume * 65535) / 100);
        m_auxMusicVolume = (channelVolume << 16) | channelVolume;
        const int device = m_auxDeviceId;
        if (device >= 0)
            (void)auxSetVolume(static_cast<unsigned int>(device), m_auxMusicVolume);
    }

    m_musicVolume = volume;
    if (!m_music && volume != 0)
        (void)PlayFile(m_musicName, m_musicMode);
    MUSIC* const playback = m_music;
    if (playback)
        playback->SetVolume((volume - 100) * 32);
}

int SOUND::FadeAndPlayFile(const STRING& name, int mode, unsigned long fade)
{
    if (this->m_music == 0)
    {
        return PlayFile(name, mode);
    }
    this->m_musicName = name;
    this->m_musicMode = mode;
    this->m_fadeStart = ::RealCurrentTime;
    this->m_fadeDuration = fade;
    return 0;
}

void SOUND::VolumeSpeech(int volume)
{
    if (volume<0)volume = 0;
    else if (volume>100)volume = 100;
    if (volume == 0 && this->m_speech != 0)this->DisableSpeech();
    MUSIC* speech = this->m_speech;
    this->m_speechVolume = volume;
    if (speech)(void)speech->SetVolume(static_cast<int>((static_cast<unsigned int>(volume)-100u)<<5));
}

void SOUND::PauseSpeech()
{
    MUSIC* speech = this->m_speech;
    if (speech) (void)speech->Pause();
}

void SOUND::ResumeSpeech()
{
    MUSIC* speech = this->m_speech;
    if (speech) (void)speech->Resume();
}

void SOUND::StopSpeech()
{
    MUSIC* speech = this->m_speech;
    if (speech) (void)speech->Stop();
}

void SOUND::ReLoadBuffers()
{
    SOUND* const sound = this;
    if (!sound->m_sfx || sound->m_sfxCount <= 0)return;
    for (int sfxIndex = 0;sfxIndex<sound->m_sfxCount;++sfxIndex)
    {
        SFX& record = sound->m_sfx[sfxIndex];
        record.Load(record.Wave, record.Property, record.Priority,
            record.ForceFeedBack, this);
    }
}

int SOUND::PlaySpeech(STRING sourceName)
{
    if (std::strcmp(sourceName.value, StringGlobals::EmptyText()) == 0)
        return 1;

    MUSIC* const current = m_speech;
    const bool reuse = current && std::strcmp(sourceName.value, current->name.value) == 0;
    if (!reuse)
    {
        if (current)
        {
            current->~MUSIC();
            MUSIC::operator delete(current);
            m_speech = 0;
        }
        if (m_speechVolume == 0)
            return 0;

        const bool isOgg = std::strstr(sourceName.value, ".ogg") != 0 ||
            std::strstr(sourceName.value, ".OGG") != 0 ||
            std::strstr(sourceName.value, ".Ogg") != 0;
        void* const storage = CrtNew(isOgg ? sizeof(OGG_MUSIC) : sizeof(DIRECT_SHOW_MUSIC));
        MUSIC* playback = 0;
        if (storage)
        {
            try
            {
                if (isOgg)
                    playback = ::new (storage) OGG_MUSIC(sourceName);
                else
                    playback = ::new (storage) DIRECT_SHOW_MUSIC(sourceName);
            }
            catch (...)
            {
                CrtFreePointer(storage);
                throw;
            }
        }
        m_speech = playback;
    }
    const int volume = m_speechVolume;
    if (volume>0)
        m_speech->SetVolume(static_cast<int>((static_cast<unsigned int>(volume)-100u)<<5));
    m_speech->Play();
    return 0;
}

void MUSIC::operator delete(void* memory)
{
    CrtFreePointer(memory);
}

MUSIC::~MUSIC() noexcept(false)
{
}
int MUSIC::IsPlaying()
{
    return 0;
}
void MUSIC::Tact()
{
}
void MUSIC::Pause()
{
}
void MUSIC::Resume()
{
}
void MUSIC::Stop()
{
}
void MUSIC::Play()
{
}
void MUSIC::SetVolume(int value)
{
    volume = value;
}

void MUSIC::Error(TYPE_ERROR type, const STRING& message, unsigned long value)
{
    STRING category("MUSIC '%s'");
    ::Error->Error(category, type, message, value, name.value);
}

void SOUND::InitDS()
{
    if (m_dsState == 0u || m_directSound)
        return;
    HRESULT result = ::DirectSoundCreate8(0, &m_directSound, 0);
    if (result < 0)
    {
        m_dsState = 2u;
        STRING message("DirectSound");
        STRING category("SOUND");
        ::Error->Error(category, E_CREATE, message, static_cast<unsigned long>(result));
        return;
    }
    if (m_flags & 1u)
    {
        result = m_directSound->SetCooperativeLevel(m_window, DSSCL_PRIORITY);
        if (result < 0)
        {
            m_flags &= ~1u;
            STRING message("PriorityLevel");
            STRING category("SOUND");
            ::Error->Error(category, E_SET, message, static_cast<unsigned long>(result));
        }
    }
    if ((m_flags & 1u) == 0u)
        result = m_directSound->SetCooperativeLevel(m_window, DSSCL_NORMAL);
    if (result < 0)
    {
        (void)m_directSound->Release();
        m_directSound = 0;
        m_dsState = 3u;
        STRING message("CooperativeLevel");
        STRING category("SOUND");
        ::Error->Error(category, E_SET, message, static_cast<unsigned long>(result));
        return;
    }
    DSBUFFERDESC description = {};
    description.dwSize = sizeof(description);
    description.dwFlags = DSBCAPS_PRIMARYBUFFER;
    IDirectSoundBuffer* primaryBuffer;
    result = m_directSound->CreateSoundBuffer(&description, &primaryBuffer, 0);
    if (result < 0)
    {
        STRING message("Primary sound buffer");
        STRING category("SOUND");
        ::Error->Error(category, E_CREATE, message, static_cast<unsigned long>(result));
    }
    else
    {
        if (primaryBuffer->Play(0u, 0u, DSBPLAY_LOOPING) < 0)
        {
            STRING message("unable play Primary");
            STRING category("SOUND");
            ::Error->Error(category, E_INVALID, message, 0u);
        }
        if (m_flags & 1u)
        {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = 44100;
            waveFormat.nAvgBytesPerSec = 176400;
            waveFormat.nBlockAlign = 4;
            waveFormat.wBitsPerSample = 16;
            waveFormat.cbSize = 0;
            result = primaryBuffer->SetFormat(&waveFormat);
            if (result < 0)
            {
                STRING message("format primary buffer");
                STRING category("SOUND");
                ::Error->Error(category, E_SET, message, static_cast<unsigned long>(result));
            }
        }
        (void)primaryBuffer->Release();
    }
    m_dsState = 0u;
    for (int index = 0; index < m_sfxCount; ++index)
    {
        SFX& record = m_sfx[index];
        record.Load(record.Wave, record.Property, record.Priority, record.ForceFeedBack, this);
    }
}

int SOUND::PlayFile(STRING sourceName, int mode)
{
    if (std::strcmp(sourceName.value, StringGlobals::EmptyText()) == 0)
        return 1;

    m_musicMode = mode;
    m_fadeStart = 0u;
    if (mode != 0)
        m_musicName = sourceName;
    else
        m_musicName = STRING(StringGlobals::EmptyText());

    MUSIC* const current = m_music;
    if (!current || std::strcmp(sourceName.value, current->name.value) != 0)
    {
        if (current)
        {
            current->~MUSIC();
            MUSIC::operator delete(current);
        }
        m_music = 0;
        if (m_musicVolume == 0)
            return 0;

        const bool isOgg = std::strstr(sourceName.value, ".ogg") != 0 ||
            std::strstr(sourceName.value, ".OGG") != 0 ||
            std::strstr(sourceName.value, ".Ogg") != 0;
        void* const storage = CrtNew(isOgg ? sizeof(OGG_MUSIC) : sizeof(DIRECT_SHOW_MUSIC));
        MUSIC* playback = 0;
        if (storage)
        {
            try
            {
                if (isOgg)
                    playback = ::new (storage) OGG_MUSIC(sourceName);
                else
                    playback = ::new (storage) DIRECT_SHOW_MUSIC(sourceName);
            }
            catch (...)
            {
                CrtFreePointer(storage);
                throw;
            }
        }
        m_music = playback;
    }
    const int volume = m_musicVolume;
    if (volume>0)
        m_music->SetVolume(static_cast<int>((static_cast<unsigned int>(volume)-100u)<<5));
    m_music->Play();
    return 0;
}

IDirectSoundBuffer* SOUND::CreateSoundBufferFromOgg(const STRING& sourceName,
    OggVorbis_File* vorbis, std::FILE** fileOut, int requestedBytes) const
{
    if (!m_directSound)
    {
        *fileOut = 0;
        return 0;
    }
    {
        STRING mode("rb");
        const char* const path = sourceName.value;
        *fileOut = *path ? std::fopen(path, mode.value) : 0;
    }
    if (!*fileOut)
    {
        STRING category("SOUND");
        ::Error->Error(category, E_OPEN, sourceName, 0u);
        return 0;
    }
    if (ov_open(*fileOut, vorbis, 0, 0) < 0)
    {
        {
            STRING category("SOUND");
            ::Error->Error(category, E_INVALID, sourceName, 0u);
        }
        (void)std::fclose(*fileOut);
        *fileOut = 0;
        return 0;
    }
    vorbis_info* const info = ov_info(vorbis, -1);
    WAVEFORMATEX waveFormat;
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = static_cast<WORD>(info->channels);
    waveFormat.nSamplesPerSec = static_cast<DWORD>(info->rate);
    waveFormat.nAvgBytesPerSec = static_cast<DWORD>(info->rate) * static_cast<DWORD>(info->channels) * 2u;
    waveFormat.nBlockAlign = static_cast<WORD>(static_cast<unsigned int>(info->channels) * 2u);
    waveFormat.wBitsPerSample = 16;
    waveFormat.cbSize = sizeof(WAVEFORMATEX);

    DSBUFFERDESC description = {};
    description.dwSize = sizeof(description);
    description.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;
    if (requestedBytes != 0)
        description.dwBufferBytes = static_cast<DWORD>(requestedBytes);
    else
    {
        const DWORD sampleCount = static_cast<DWORD>(ov_pcm_total(vorbis, -1));
        description.dwBufferBytes = sampleCount * static_cast<DWORD>(info->channels) * 2u;
    }
    description.lpwfxFormat = &waveFormat;
    IDirectSoundBuffer* buffer;
    const HRESULT result = m_directSound->CreateSoundBuffer(&description, &buffer, 0);
    if (result < 0)
    {
        STRING message("SoundBuffer for ogg");
        STRING category("SOUND");
        ::Error->Error(category, E_CREATE, message, static_cast<unsigned long>(result));
        return 0;
    }
    return buffer;
}

IDirectSoundBuffer* SOUND::CreateSoundBufferFromWav(const STRING& sourceName,
    RESOURCE* resource, int) const
{
    if (!m_directSound)
        return 0;
    const char* const path = sourceName.value;
    if (std::strcmp(path, "wav\\null.wav") == 0 ||
        std::strcmp(path, "null.wav") == 0 || std::strcmp(path, "null") == 0)
        return 0;

    if (resource->OpenForRead(sourceName, mmioFOURCC('W', 'A', 'V', 'E')) != 0)
        return 0;
    if (resource->GoBegin(mmioFOURCC('f', 'm', 't', ' ')) != 0)
    {
        ::Error->Log("!!!ERROR!!!SFX:'%s' 'fmt ' not found", sourceName.value);
        return 0;
    }
    const unsigned int formatBytes = resource->RecordSize();
    if (formatBytes < 14u)
    {
        ::Error->Log("!!!ERROR!!!SFX:'%s' incorrect size %i", sourceName.value, formatBytes);
        return 0;
    }
    WAVEFORMATEX waveFormat;
    (void)resource->Read(&waveFormat, sizeof(waveFormat));
    if (resource->GoNext(mmioFOURCC('d', 'a', 't', 'a')) != 0 &&
        resource->GoBegin(mmioFOURCC('d', 'a', 't', 'a')) != 0)
    {
        ::Error->Log("!!!ERROR!!!SFX:'%s' 'data' not found", sourceName.value);
        return 0;
    }

    const unsigned int dataBytes = resource->RecordSize();
    DSBUFFERDESC description = {};
    description.dwSize = sizeof(description);
    description.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME;
    description.dwBufferBytes = dataBytes;
    description.lpwfxFormat = &waveFormat;
    IDirectSoundBuffer* buffer;
    const HRESULT result = m_directSound->CreateSoundBuffer(&description, &buffer, 0);
    if (result < 0)
    {
        STRING message("SoundBuffer");
        STRING category("SOUND");
        ::Error->Error(category, E_CREATE, message, static_cast<unsigned long>(result));
        return 0;
    }
    return buffer;
}

void SOUND::LoadSFX(RESOURCE* resource)
{
    STRING wave[8], forceFeedBack[8];
    const unsigned int loadStart = timeGetTime();
    if (m_dsState != 0u)
        return;
    if (!resource->FileHandle())
    {
        STRING message("res");
        STRING category("SOUND");
        ::Error->Error(category, E_OPEN, message, 0u);
        return;
    }
    if (m_sfx)
    {
        unsigned int* const header = reinterpret_cast<unsigned int*>(m_sfx) - 1;
        SFX* const objects = m_sfx;
        unsigned int remaining = *header;
        try
        {
            while (static_cast<int>(--remaining) >= 0)
                objects[remaining].~SFX();
        }
        catch (...)
        {
            try
            {
                while (static_cast<int>(--remaining) >= 0)
                    objects[remaining].~SFX();
            }
            catch (...)
            {
                std::terminate();
            }
            throw;
        }
        CrtFreePointer(header);
        m_sfx = 0;
    }
    const unsigned int count = resource->GetNoSubRes(mmioFOURCC('S', 'F', 'X', ' '));
    m_sfxCount = static_cast<int>(count);
    if (count == 0u)
    {
        STRING message("SFX ");
        STRING category("SOUND");
        ::Error->Error(category, E_SECTION, message, 0u);
        return;
    }

    const unsigned int objectCount = count + 1u;
    const unsigned int allocationBytes = sizeof(unsigned int) + objectCount * sizeof(SFX);
    unsigned int* const header = static_cast<unsigned int*>(CrtNew(allocationBytes));
    SFX* objects = 0;
    if (header)
    {
        *header = objectCount;
        objects = reinterpret_cast<SFX*>(header + 1);
        unsigned int constructed = 0u;
        try
        {
            while (static_cast<int>(constructed) < static_cast<int>(objectCount))
            {
                ::new (&objects[constructed]) SFX;
                ++constructed;
            }
        }
        catch (...)
        {
            try
            {
                while (static_cast<int>(--constructed) >= 0)
                    objects[constructed].~SFX();
            }
            catch (...)
            {
                std::terminate();
            }
            CrtFreePointer(header);
            throw;
        }
    }
    m_sfx = objects;
    if (!m_sfx)
    {
        STRING message("LoadSfx");
        STRING category("SOUND");
        ::Error->Error(category, E_MEMORY, message, static_cast<unsigned int>(m_sfxCount));
        return;
    }
    if (resource->GoBegin(mmioFOURCC('S', 'F', 'X', ' ')) != 0)
        return;

    unsigned int index = 0u;
    unsigned long property;
    unsigned char priority;
    do
    {
        (void)resource->Read(&property, sizeof(property));
        (void)resource->Read(&priority, sizeof(priority));
        for (unsigned int variant = 0; variant < 8u; ++variant)
            wave[variant].Read(resource);
        for (unsigned int variant = 0; variant < 8u; ++variant)
            forceFeedBack[variant].Read(resource);
        m_sfx[index].Load(wave, property, priority, forceFeedBack, this);
        ++index;
    }
    while (resource->GoNextSub(mmioFOURCC('S', 'F', 'X', ' ')) == 0);

    const unsigned int quality = m_flags & 1u;
    const unsigned int elapsed = timeGetTime() - loadStart;
    ::Error->Log("LoadSFX::No   =%-15i   sizeof(SFX)   =%-5i    load time     =%-6ims Quality        =%i", m_sfxCount, static_cast<int>(sizeof(SFX)), elapsed, quality);
}

SOUND::SOUND(HWND__* window, RESOURCE* resource, int enabled)
{

    m_flags = (m_flags&~1u)|(enabled?1u:0u);
    m_musicVolume = 100;
    m_soundVolume = 100;
    m_speechVolume = 100;
    m_window = window;
    m_musicMode = 0;
    m_mciMusicOpen = 0u;
    m_sfxCount = 0;
    m_sfx = 0;
    m_directSound = 0;
    m_mciPlayToCondition = -1;
    m_dsState = 1u;
    m_waveOutDeviceId = -1;
    m_auxDeviceId = -1;
    m_speech = 0;
    m_music = 0;
    m_fadeStart = 0u;
    InitDS();
    LoadSFX(resource);
}

DIRECT_SHOW_MUSIC::DIRECT_SHOW_MUSIC(const STRING& sourceName) : MUSIC(sourceName)
{
    graph = 0;
    control = 0;
    seeking = 0;
    HRESULT mediaResult = CoCreateInstance(CLSID_FilterGraph, 0,
        CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
        IID_IGraphBuilder, reinterpret_cast<void**>(&graph));
    const HRESULT createResult = mediaResult;
    if (mediaResult < 0)
    {
        Error(E_CREATE, "GraphBuilder", static_cast<unsigned long>(mediaResult));
        return;
    }
    if (!control)
    {
        mediaResult = graph->QueryInterface(IID_IMediaControl,
            reinterpret_cast<void**>(&control));
        if (mediaResult < 0)
        {
            Error(E_CREATE, "MediaControl", static_cast<unsigned long>(createResult));
            return;
        }
    }
    if (!seeking)
    {
        mediaResult = graph->QueryInterface(IID_IMediaSeeking,
            reinterpret_cast<void**>(&seeking));
        if (mediaResult < 0)
        {
            Error(E_CREATE, "MediaSeeking", 0u);
            return;
        }
    }
    const char* const path = sourceName.value;
    std::FILE* sourceFile = 0;
    if (*path)
        sourceFile = std::fopen(path, "rb");
    const bool exists = sourceFile != 0;
    if (sourceFile)
        (void)std::fclose(sourceFile);
    if (!exists)
    {
        Error(E_OPEN, sourceName, 0u);
        return;
    }
    wchar_t wideSourcePath[1024];
    sourceName.ToWideChar(reinterpret_cast<unsigned short*>(wideSourcePath), 1024);
    mediaResult = graph->RenderFile(wideSourcePath, 0);
    if (mediaResult < 0)
        Error(E_INVALID, "RenderFile", static_cast<unsigned long>(mediaResult));
}

void SOUND::DisableSound()
{
    if (m_dsState != 0u)return;
    ReleaseDS();
    const int index = m_waveOutDeviceId;
    if (index >= 0)(void)waveOutSetVolume(reinterpret_cast<HWAVEOUT>(static_cast<UINT_PTR>(index)), m_waveOutRestoreVolume);
}

void SOUND::DisableMusic()
{
    MUSIC* const music = this->m_music;
    if (music)
    {
        music->~MUSIC();
        MUSIC::operator delete(music);
        this->m_music = 0u;
    }
    if (this->m_mciMusicOpen != 0u)
    {
        (void)mciSendStringA("stop FWMUSIC", 0, 0u, 0);
        (void)mciSendStringA("close FWMUSIC", 0, 0u, 0);
        this->m_mciMusicOpen = 0u;
    }
    const int handle = this->m_auxDeviceId;
    if (handle >= 0)(void)auxSetVolume(static_cast<unsigned int>(handle), this->m_auxRestoreVolume);
    this->m_mciPlayToValue = -1;
    this->m_fadeStart = 0u;
}

void SOUND::Disable()
{
    DisableMusic();
    DisableSound();
    this->DisableSpeech();
}

void SOUND::DisableSpeech()
{
    MUSIC* const speech = this->m_speech;
    if (speech)
    {
        speech->~MUSIC();
        MUSIC::operator delete(speech);
        this->m_speech = 0u;
    }
}

