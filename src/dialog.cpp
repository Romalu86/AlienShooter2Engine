#include "mylib.h"
#include "engine_globals.h"
#include "basetype.h"
#include "dialogSrv.h"
#include "zm/Utils/zStaticStorage.h"
#include <new>

namespace
{
StaticObjectStorage<STRING> fallbackPhrase;
StaticObjectStorage<STRING> fallbackVoiceFile;
unsigned char fallbackPhraseInitialized = 0;
unsigned char fallbackVoiceFileInitialized = 0;

void __cdecl DestroyFallbackPhrase()
{
    fallbackPhrase.Get().~STRING();
}

void __cdecl DestroyFallbackVoiceFile()
{
    fallbackVoiceFile.Get().~STRING();
}
}

DIALOG::DIALOG()
{
    m_phrases.m_data = 0;
    m_phrases.m_count = 0;
    m_phrases.m_capacity = 0;
    m_coor.x = 0.0f;
    m_coor.y = 0.0f;
    m_coor.z = 50.0f;
    m_unitVid[0] = 0;
    m_unitVid[1] = 0;
    m_radio = 0;
    m_faceDirection = 0;
}

DIALOG::~DIALOG()
{
}

extern "C" void* DialogCtor(void* self)
{
    return ::new (self) DIALOG();
}

const STRING& DIALOG::GetPhrase(int index) const
{
    if (index<m_phrases.m_count)
        return m_phrases.m_data[index].m_text;

    unsigned char& guard = fallbackPhraseInitialized;
    if ((guard&1u) == 0u)
    {
        guard = static_cast<unsigned char>(guard | 1u);
        ::new (fallbackPhrase.Address()) STRING(StringGlobals::EmptyText());
        (void) RegisterCrtAtExit(&DestroyFallbackPhrase);
    }
    return fallbackPhrase.Get();
}

int DIALOG::GetSFX(int index) const
{
    if (index >= m_phrases.m_count) return 0;
    return static_cast<int>(m_phrases.m_data[index].m_sfx);
}

int DIALOG::GetDuration(int index) const
{
    int duration = 0;
    if (index<m_phrases.m_count)
        duration = static_cast<int>(m_phrases.m_data[index].GetDuration());

    const DialogSrv* const service = ::g_pDlgSrv;
    const int durationPercent = static_cast<int>(service->m_textDurationPercent);
    return MultiplySigned32Wrap(durationPercent, duration)/100;
}

const STRING& DIALOG::GetVoiceFile(int index) const
{
    if (index<m_phrases.m_count)
        return m_phrases.m_data[index].m_voiceFile;

    unsigned char& guard = fallbackVoiceFileInitialized;
    if ((guard&1u) == 0u)
    {
        guard = static_cast<unsigned char>(guard | 1u);
        ::new (fallbackVoiceFile.Address()) STRING(StringGlobals::EmptyText());
        (void) RegisterCrtAtExit(&DestroyFallbackVoiceFile);
    }
    return fallbackVoiceFile.Get();
}

unsigned long PHRASE::GetDuration() const
{
    if (m_duration != 0u) return m_duration;

    int lineCount = BitsToSigned32(static_cast<unsigned int>(m_text.CountChar('\n'))+1u);
    if (lineCount<1) lineCount = 1;
    else if (lineCount>5) lineCount = 5;

    const DialogSrv* const service = ::g_pDlgSrv;
    switch (lineCount)
    {
    case 1:
            return service->m_oneLineDuration;
    case 2:
            return service->m_twoLineDuration;
    case 3:
            return service->m_threeLineDuration;
    case 4:
            return service->m_fourLineDuration;
    default:
            return service->m_fiveLineDuration;
    }
}

