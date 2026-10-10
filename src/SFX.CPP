#if defined(_WIN32)
#define INPUT WIN32_INPUT
#include <windows.h>
#include <mmreg.h>
#include <dsound.h>
#undef INPUT
#endif
#include "engine_globals.h"
#include "myerror.h"
#include "mylib.h"
#include "resread.h"
#include "basetype.h"
#include "sound.h"
#include "sfx.h"
#include <vorbis/vorbisfile.h>
#include <cstdio>
#include <cstring>

SFXBUFFER::SFXBUFFER()
    : m_buffer(0), m_playing(0u), m_sfxIndex(-1), m_pitch(1.0f)
{
}

SFXBUFFER::~SFXBUFFER() noexcept(false)
{
    IDirectSoundBuffer* const buffer = this->m_buffer;
    if (!buffer)
        return;

    const unsigned int refs = buffer->Release();
    if (refs != 0u)
    {
        STRING message("SoundBuffer release !=0");
        STRING category("SFXBUFFER[%i]");
        ::Error->Error(
            category,
            E_ERROR,
            message,
            refs, static_cast<unsigned int>(this->m_sfxIndex));
    }
    this->m_buffer = 0;
}

void SFXBUFFER::Destroy()
{
    IDirectSoundBuffer* buffer = this->m_buffer;
    if (buffer)
    {
        (void)buffer->Stop();
        buffer = this->m_buffer;
        (void)buffer->Release();
        this->m_buffer = 0;
    }
    this->m_buffer = 0;
    this->m_playing = 0u;
    this->m_lastPlayTick = 0u;
    this->m_sfxIndex = -1;
}

void SFXBUFFER::Play(int pan, int volume, float pitch)
{
    if (this->m_sfxIndex < 0)
        return;
    if (!this->m_buffer)
        return;

    this->m_volume = volume;
    this->m_pan = pan;
    this->m_pitch = pitch;
    (void)this->m_buffer->SetPan(pan);
    (void)this->m_buffer->SetVolume(volume);
    if (pitch < 1.0f || pitch > 1.0f)
    {
        IDirectSoundBuffer* const buffer = this->m_buffer;
        const double base = static_cast<double>(this->m_baseFrequency);
        const int frequency = TruncateDoubleToInt32(base / static_cast<double>(pitch));
        (void)buffer->SetFrequency(static_cast<unsigned int>(frequency));
    }

    if (this->m_playing == 0u)
    {
        SOUND* const sound = ::Sound;
        const int sfxIndex = this->m_sfxIndex;
        unsigned int playFlags = 0u;
        if (sound->m_sfx && sfxIndex >= 0 && sfxIndex <= sound->m_sfxCount)
        {
            const SFX& record = sound->m_sfx[sfxIndex];
            if (record.m_bufferCount != 0u && (record.Property & P_LOOP) != 0u)
                playFlags = DSBPLAY_LOOPING;
        }
        const unsigned int playResult = this->m_buffer->Play(0u, 0u, playFlags);
        if (playResult == static_cast<unsigned int>(DSERR_BUFFERLOST))
        {
            {
                STRING message("buffer is lost");
                STRING category("SFXBUFFER[%i]");
                ::Error->Error(category, E_ERROR, message,
                    0u, static_cast<unsigned int>(this->m_sfxIndex));
            }
            ::Sound->ReLoadBuffers();
        }
    }
    this->m_playing = 1u;
    this->m_lastPlayTick = ::CurrentTime;
}

void SFXBUFFER::Resume()
{
    const int sfxIndex = this->m_sfxIndex;
    IDirectSoundBuffer* const buffer = this->m_buffer;
    if (sfxIndex < 0 || !buffer || this->m_playing == 0u)
        return;

    SOUND* const sound = ::Sound;
    unsigned int playFlags = 0u;
    if (sound->m_sfx && sfxIndex >= 0 && sfxIndex <= sound->m_sfxCount)
    {
        const SFX& record = sound->m_sfx[sfxIndex];
        if (record.m_bufferCount != 0u && (record.Property & P_LOOP) != 0u)
            playFlags = DSBPLAY_LOOPING;
    }
    const unsigned int playResult = buffer->Play(0u, 0u, playFlags);
    if (playResult == static_cast<unsigned int>(DSERR_BUFFERLOST))
    {
        (void)::Error->Log("!!!ERROR!!! SFXBUFFER::buffer is lost");
        ::Sound->ReLoadBuffers();
    }
}

int SFXBUFFER::CheckPlay()
{
    IDirectSoundBuffer* buffer = this->m_buffer;
    if (this->m_sfxIndex < 0 || !buffer || this->m_playing == 0u)
        return 0;

    DWORD bufferStatus;
    (void)buffer->GetStatus(&bufferStatus);
    if ((bufferStatus & DSBSTATUS_PLAYING) == 0u)
        this->m_playing = 0u;

    const int sfxIndex = this->m_sfxIndex;
    SOUND* const sound = ::Sound;
    if (sound->m_sfx && sfxIndex >= 0 && sfxIndex <= sound->m_sfxCount)
    {
        const SFX& record = sound->m_sfx[sfxIndex];
        if (record.m_bufferCount != 0u && (record.Property & P_LOOP) != 0u &&
            ::CurrentTime - this->m_lastPlayTick > 200u)
        {
            this->m_playing = 0u;
            buffer = this->m_buffer;
            if (buffer)
                (void)buffer->Stop();
        }
    }
    return static_cast<int>(this->m_playing);
}

void SFXBUFFER::Load(int index, IDirectSoundBuffer* buffer)
{
    this->m_sfxIndex = index;
    IDirectSoundBuffer* const previousBuffer = this->m_buffer;
    if (previousBuffer)
    {
        const unsigned int remainingReferences = previousBuffer->Release();
        if (remainingReferences != 0u)
        {
            STRING message("SoundBuffer(Load()) release !=0");
            STRING category("SFXBUFFER[%i]");
            ::Error->Error(
                category,
                E_ERROR,
                message,
                remainingReferences, static_cast<unsigned int>(this->m_sfxIndex));
        }
        this->m_buffer = 0;
    }
    this->m_buffer = buffer;
    if (buffer)
        (void)buffer->GetFrequency(&this->m_baseFrequency);
    this->m_playing = 0u;
    this->m_lastPlayTick = 0u;
}

SFX::SFX()
{
    m_bufferCount = 0u;
    for (unsigned int bufferIndex = 0; bufferIndex < 8u; ++bufferIndex)
        m_buffers[bufferIndex] = 0;
}

SFX::~SFX() noexcept(false)
{
    Release();
}

void SFX::Release()
{
    for (unsigned int bufferIndex = 0; bufferIndex < 8u; ++bufferIndex)
    {
        IDirectSoundBuffer* const buffer = this->m_buffers[bufferIndex];
        if (buffer)
        {
            (void)buffer->Release();
            this->m_buffers[bufferIndex] = 0;
        }
    }
}

IDirectSoundBuffer* SFX::Duplicate(IDirectSound* directSound)
{
    const int sourceBufferIndex = Random2(static_cast<int>(this->m_bufferCount - 1u));
    if (!this->m_buffers[sourceBufferIndex])
        return 0;

    const unsigned int referenceCount = this->m_buffers[sourceBufferIndex]->AddRef();
    IDirectSoundBuffer* result = this->m_buffers[sourceBufferIndex];
    if (referenceCount > 2u)
    {
        (void)this->m_buffers[sourceBufferIndex]->Release();
        IDirectSoundBuffer* duplicate = 0;
        const int duplicateResult = static_cast<int>(
            directSound->DuplicateSoundBuffer(this->m_buffers[sourceBufferIndex], &duplicate));
        if (duplicateResult < 0)
        {
            (void)::Error->Log("!!!ERROR!!!SFX:'%s' %X Couldn't duplicate buffer", this, static_cast<unsigned int>(duplicateResult));
        }
        result = duplicate;
    }
    if (result)
    {
        (void)result->SetCurrentPosition(0u);
        (void)result->SetFrequency(DSBFREQUENCY_ORIGINAL);
    }
    return result;
}

void SFX::Load(STRING* wave, unsigned long property, unsigned char priority,
    STRING* forceFeedBack, SOUND* sound)
{
    static char decodedSamples[4096];
    RESOURCE resourceOwner;
    RESOURCE* const resource = &resourceOwner;
    Release();
    for (unsigned int soundVariantIndex = 0; soundVariantIndex < 8u; ++soundVariantIndex)
    {
        this->Wave[soundVariantIndex] = wave[soundVariantIndex];
        this->ForceFeedBack[soundVariantIndex] = forceFeedBack[soundVariantIndex];
    }
    this->Priority = priority;
    this->Property = property;
    for (unsigned int soundVariantIndex = 0; soundVariantIndex < 8u; ++soundVariantIndex)
    {
        STRING* const nameObject = &this->Wave[soundVariantIndex];
        const char* const fileName = nameObject->value;
        if (std::strcmp(fileName, StringGlobals::EmptyText()) == 0)
            break;
        IDirectSoundBuffer** const bufferSlot = &this->m_buffers[soundVariantIndex];
        const bool isOgg = std::strstr(fileName, ".ogg") != 0 ||
            std::strstr(fileName, ".OGG") != 0 ||
            std::strstr(fileName, ".Ogg") != 0;
        if (!isOgg)
        {
            IDirectSoundBuffer* buffer =
                sound->CreateSoundBufferFromWav(*nameObject, resource, 0);
            *bufferSlot = buffer;
            if (buffer)
            {
                void* firstRegion;
                void* secondRegion;
                DWORD firstRegionBytes, secondRegionBytes;
                const unsigned int waveDataBytes = resource->RecordSize();
                const int lockResult = static_cast<int>(buffer->Lock(
                    0u,
                    waveDataBytes,
                    &firstRegion,
                    &firstRegionBytes,
                    &secondRegion,
                    &secondRegionBytes,
                    0u));
                if (lockResult >= 0)
                {
                    (void)resource->RESOURCE::Read(firstRegion, firstRegionBytes);
                    if (secondRegionBytes)
                        (void)resource->RESOURCE::Read(secondRegion, secondRegionBytes);
                    (void)(*bufferSlot)->Unlock(
                        firstRegion, firstRegionBytes, secondRegion, secondRegionBytes);
                }
            }
            resource->Close();
        }
        else
        {
            OggVorbis_File oggFile;
            std::FILE* file;
            IDirectSoundBuffer* buffer =
                sound->CreateSoundBufferFromOgg(*nameObject, &oggFile, &file, 0);
            *bufferSlot = buffer;
            unsigned int decodedOffset = 0u;
            if (buffer)
            {
                for (;;)
                {
                    int logicalBitstreamIndex;
                    const int decodedBytes = static_cast<int>(ov_read(
                        &oggFile,
                        decodedSamples,
                        sizeof(decodedSamples),
                        0,
                        2,
                        1,
                        &logicalBitstreamIndex));
                    if (decodedBytes == 0)
                        break;
                    if (decodedBytes < 0)
                    {
                        STRING text("decode ogg");
                        STRING category("SFX");
                        ::Error->Error(category, E_ERROR, text,
                            static_cast<unsigned int>(decodedBytes));
                        continue;
                    }
                    void* firstRegion;
                    void* secondRegion;
                    DWORD firstRegionBytes, secondRegionBytes;
                    const int lockResult = static_cast<int>((*bufferSlot)->Lock(
                        decodedOffset,
                        static_cast<unsigned int>(decodedBytes),
                        &firstRegion,
                        &firstRegionBytes,
                        &secondRegion,
                        &secondRegionBytes,
                        0u));
                    if (lockResult >= 0)
                    {
                        std::memcpy(firstRegion, decodedSamples, firstRegionBytes);
                        if (secondRegionBytes)
                            std::memcpy(secondRegion,
                                decodedSamples + firstRegionBytes, secondRegionBytes);
                        (void)(*bufferSlot)->Unlock(
                            firstRegion, firstRegionBytes, secondRegion, secondRegionBytes);
                    }
                    decodedOffset += static_cast<unsigned int>(decodedBytes);
                }
            }
            if (file)
            {
                (void)std::fclose(file);
                (void)ov_clear_noclose(&oggFile);
                file = 0;
            }
        }
        this->m_bufferCount = soundVariantIndex + 1u;
    }
}

