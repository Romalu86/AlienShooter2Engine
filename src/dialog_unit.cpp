#include "zm/Utils/zLogConsole.h"
#include "engine_globals.h"
#include "mylib.h"
#include "dialog_unit.h"
#include "terrain.h"
#include "input.h"
#include "sprite.h"
#include "vid.h"
#include "map.h"
#include "sound.h"
#include "dialogSrv.h"
#include "base_graph.h"
#include <new>
#include "basetype.h"

static DIALOG_UNIT* activeSpeechUser = 0;

static void DialogUnitTogglePlayback(DIALOG_UNIT* self, unsigned char enabled, const STRING& text)
{
    if (enabled)
    {
        activeSpeechUser = self;
        (void)::Sound->PlaySpeech(text);
        iLogConsole* logger = ::gpLogCons;
        (void) logger->LogStr(LT_INFO, "PlaySpeech %s", text.m_value);
        return;
    }

    if (activeSpeechUser == self)
    {
        (::Sound)->StopSpeech();
        iLogConsole* logger = ::gpLogCons;
        (void) logger->LogStr(LT_INFO, "StopSpeech");
        activeSpeechUser = 0;
    }
}

DIALOG_UNIT* DIALOG_UNIT::m_pRadioMesUser=0;

DIALOG_UNIT::DIALOG_UNIT()
{
    for (int dialogSlot = 0; dialogSlot<2; ++dialogSlot)
    {
        m_dialog[dialogSlot].dialogId = 0;
        m_dialog[dialogSlot].phraseIndex = 0;
        m_dialog[dialogSlot].phraseStartTime = 0;
        m_dialog[dialogSlot].skipRequested = 0;
        m_dialog[dialogSlot].lastDialogId = 0;
    }
}

DIALOG_UNIT::~DIALOG_UNIT() noexcept(false)
{
    SetRadioMsg(0, 0, false);
    STRING empty;
    DialogUnitTogglePlayback(this, 0u, empty);
}

void DIALOG_UNIT::SetRadioMsg(int dialogKey, int phraseIndex, bool skipRecordCopy)
{
    if (!gpQuestEvent)
        return;

    if (dialogKey)
    {
        m_pRadioMesUser = this;
        gpQuestEvent->SetRadioEx(dialogKey, phraseIndex, skipRecordCopy);
        return;
    }

    if (m_pRadioMesUser == this)
    {
        gpQuestEvent->SetRadioEx(0, 0, skipRecordCopy);
        m_pRadioMesUser = 0;
    }
}

void DIALOG_UNIT::SetDialog(int dialogKey)
{
    DialogSrv* const dialogService = ::g_pDlgSrv;
    if (dialogService->IsDlgNumValid(dialogKey) == 0u)
    {
        const STRING errorText("N dialog in SetDialog");
        Error(E_INVALID, errorText, static_cast<unsigned long>(dialogKey));
        return;
    }

    DIALOG* const dialog = ::g_pDlgSrv->GetDialog(dialogKey);
    const unsigned int dialogSlot = dialog->m_radio?1u:0u;
    DIALOG_UNIT_RECORD& record = m_dialog[dialogSlot];
    const unsigned int currentTick = ::CurrentTime;
    if ((record.dialogId == dialogKey ||
        (record.lastDialogId == dialogKey && currentTick-static_cast<unsigned int>(record.phraseStartTime)<5000u)) &&
        dialogKey != 102000)
        return;

    const int fontVidIndex = dialog->m_unitVid[dialogSlot];
    if (fontVidIndex<0 || fontVidIndex >= Map->VidCount() || Map->StoredVid(fontVidIndex) == 0)
    {
        const STRING errorText("Vid for dialog in SetDialog");
        Error(E_INVALID, errorText, static_cast<unsigned long>(fontVidIndex));
        return;
    }

    ClearDialog(static_cast<DLG_TYPE>(dialogSlot), false);
    record.dialogId = dialogKey;
    record.phraseIndex = 0;
    record.lastDialogId = dialogKey;
    if (ShowCurPhrase(static_cast<DLG_TYPE>(dialogSlot), false))
    {
        if (dialogSlot == 1u) ::g_pDlgSrv->m_activeRadioDialogKey = static_cast<unsigned int>(dialogKey);
    }
    else
    {
        record.dialogId = 0;
    }
}

void DIALOG_UNIT::ClearDialogs()
{
    ClearDialog(static_cast<DLG_TYPE>(0), false);
    ClearDialog(static_cast<DLG_TYPE>(1), false);
}

int DIALOG_UNIT::GetDialog() const
{
    return (m_dialog[0].dialogId != 0 || m_dialog[1].dialogId != 0) ? 1 : 0;
}

const VECTOR& DIALOG::Coor() const
{
    return m_coor;
}

void DIALOG_UNIT::ClearDialog(DLG_TYPE type, bool keepRuntimeState)
{
    const unsigned int dialogSlot = static_cast<unsigned int>(type);
    DIALOG_UNIT_RECORD& record = m_dialog[dialogSlot];
    const unsigned int dialogId = static_cast<unsigned int>(record.dialogId);
    if (dialogId == 0u) return;

    DialogSrv* dialogServer = ::g_pDlgSrv;
    DIALOG* const dialog = dialogServer->GetDialog(static_cast<int>(dialogId));

    if (dialogSlot == 0u)
    {
        const unsigned int fontVidId = static_cast<unsigned int>(dialog->m_unitVid[dialog->m_radio?1:0]);
        SPRITE* textSprite = Link();
        while (textSprite)
        {
            if (static_cast<unsigned int>(textSprite->Vid()->nVid) == fontVidId)
            {
                textSprite->BreakLink();
                delete textSprite;
                break;
            }
            textSprite = textSprite->ChildChain();
        }
    }
    else if (!keepRuntimeState)
    {
        SetRadioMsg(0, 0, false);
    }

    const STRING& voiceFile = dialog->GetVoiceFile(record.phraseIndex);
    const char* const voiceFileText = voiceFile.m_value;
    if (voiceFileText[0] != '\0')
    {
        STRING emptyVoiceFile(StringGlobals::EmptyText());
        DialogUnitTogglePlayback(this, 0u, emptyVoiceFile);
    }

    dialogServer = ::g_pDlgSrv;
    if (dialogServer->m_activeRadioDialogKey == static_cast<unsigned int>(record.dialogId))
        dialogServer->m_activeRadioDialogKey = 0u;
    record.phraseStartTime = ::CurrentTime;
    record.skipRequested = 0;
    record.dialogId = 0;
}

void DIALOG_UNIT::DialogTact()
{
    DialogTact(static_cast<DLG_TYPE>(0));
    DialogTact(static_cast<DLG_TYPE>(1));
}

void DIALOG_UNIT::DialogTact(DLG_TYPE type)
{
    const unsigned int dialogSlot = static_cast<unsigned int>(type);
    DIALOG_UNIT_RECORD& record = m_dialog[dialogSlot];
    if (record.dialogId == 0) return;

    DialogSrv* dialogServer = ::g_pDlgSrv;
    const unsigned int activeRadioDialogKey = dialogServer->m_activeRadioDialogKey;
    if (activeRadioDialogKey != 0u && dialogSlot == 1u && activeRadioDialogKey != static_cast<unsigned int>(m_dialog[1].dialogId))
    {
        ClearDialog(type, true);
        return;
    }

    DIALOG* const dialog = dialogServer->GetDialog(record.dialogId);

    const unsigned int previousPhraseTick = static_cast<unsigned int>(record.phraseStartTime);
    const unsigned int phraseIndex = static_cast<unsigned int>(record.phraseIndex);
    MAP* const map = Map;
    if (map->InputState()->GetAction(IA_SKIP_DIALOG) != 0)
    {
        const unsigned int duration = static_cast<unsigned int>(dialog->GetDuration(static_cast<int>(phraseIndex)));
        record.phraseStartTime = ::CurrentTime-duration-1u;
        record.skipRequested = 1;
    }

    const unsigned int duration = static_cast<unsigned int>(dialog->GetDuration(static_cast<int>(phraseIndex)));
    const unsigned int elapsed = ::CurrentTime-previousPhraseTick;
    bool advancePhrase = elapsed>duration;
    if (advancePhrase)
    {
        SOUND* const sound = ::Sound;
        MUSIC* const speech = sound->m_speech;
        if (speech && speech->IsPlaying() != 0 && activeSpeechUser == this)
            advancePhrase = false;
    }
    if (!advancePhrase && record.skipRequested == 0u) return;
    if (BitsToSigned32(static_cast<unsigned int>(record.phraseIndex)+1u) >= dialog->m_phrases.m_count)
    {
        ClearDialog(type, false);
        return;
    }

    const unsigned int nextDuration = static_cast<unsigned int>(dialog->GetDuration(static_cast<int>(phraseIndex)));
    const bool phraseExpired = (::CurrentTime-previousPhraseTick)>nextDuration+100u;
    if (phraseExpired) record.phraseIndex = BitsToSigned32(static_cast<unsigned int>(record.phraseIndex)+1u);
    (void) ShowCurPhrase(type, !phraseExpired);
}

bool DIALOG_UNIT::ShowCurPhrase(DLG_TYPE type, bool force)
{
    const unsigned int dialogSlot = static_cast<unsigned int>(type);
    DIALOG_UNIT_RECORD& record = m_dialog[dialogSlot];
    const unsigned int phraseIndex = static_cast<unsigned int>(record.phraseIndex);
    const unsigned int dialogKey = static_cast<unsigned int>(record.dialogId);
    DialogSrv* const dialogService = ::g_pDlgSrv;
    DIALOG* const dialog = dialogService->GetDialog(static_cast<int>(dialogKey));

    if (!force)
    {
        record.phraseStartTime = ::CurrentTime;
        record.skipRequested = 0;
        const unsigned int sfxId = static_cast<unsigned int>(dialog->GetSFX(static_cast<int>(phraseIndex)));
        if (sfxId) PlaySFX(static_cast<int>(sfxId));
    }

    const STRING& voiceFile = dialog->GetVoiceFile(static_cast<int>(phraseIndex));
    STRING speechFile(voiceFile);
    const char* speechFileText = speechFile.m_value;
    if (speechFileText[0] != 0 && !force)
        DialogUnitTogglePlayback(this, 1u, speechFile);

    bool suppressPhraseText = false;
    if (::g_pDlgSrv->m_showDialogSubtitles == 0u)
    {
        const STRING& gateVoiceFile = dialog->GetVoiceFile(static_cast<int>(phraseIndex));
        const char* gateVoiceFileText = gateVoiceFile.m_value;
        suppressPhraseText = gateVoiceFileText[0] != 0;
    }

    bool result = false;
    if (dialog->m_radio != 0u)
    {
        if (gpQuestEvent)
        {
            if (force) SetRadioMsg(0, 0, false);
            else SetRadioMsg(static_cast<int>(dialogKey), static_cast<int>(phraseIndex), suppressPhraseText);
            result = true;
        }
        return result;
    }

    STRING phraseText;
    if (!force)
    {
        const STRING& phrase = dialog->GetPhrase(static_cast<int>(phraseIndex));
        phraseText = phrase;
    }
    else
    {
        STRING emptyPhraseText(StringGlobals::EmptyText());
        phraseText = emptyPhraseText;
    }

    const unsigned int fontVidId = static_cast<unsigned int>(dialog->m_unitVid[dialog->m_radio?1:0]);
    if (!suppressPhraseText || force)
    {
        SPRITE* textSprite = Link();
        while (textSprite)
        {
            if (static_cast<unsigned int>(textSprite->Vid()->nVid) == fontVidId)
            {
                (void) textSprite->Action(ACT_SET_TEXT, PtrToU32(&phraseText), 0, 0);
                result = true;
                break;
            }
            textSprite = textSprite->ChildChain();
        }

        if (!result && phraseIndex == 0u)
        {
            void* const* const creatorVtable = *reinterpret_cast<void* const* const*>(Map);
            unsigned char direction = 255u;
            (void) ANGLE_Ctor(&direction, 0u);
            const VECTOR offset(0, 200, 200);
            const VECTOR& dialogCoor = dialog->Coor();
            const VECTOR& unitCoor = Coor();
            const VECTOR spawnCoor = (unitCoor+dialogCoor)+offset;
            VID* const fontVid = Map->Vid(static_cast<int>(fontVidId));
            typedef SPRITE* (__thiscall *CreateSpriteCall)(MAP*, VID*, const VECTOR&, ANGLE, SPRITE*);
            const CreateSpriteCall createSprite = reinterpret_cast<CreateSpriteCall>(creatorVtable[8]);
            SPRITE* createdTextSprite = createSprite(Map, fontVid, spawnCoor, ANGLE(static_cast<unsigned char>(direction)), 0);
            if (createdTextSprite)
            {
                (void) createdTextSprite->Action(ACT_SET_BEHAVE, 5, 0, 0);
                (void) AddLinkToLast(createdTextSprite);
                (void) createdTextSprite->Action(ACT_SET_TEXT, PtrToU32(&phraseText), 0, 0);
                result = true;
            }
        }
    }

    return result;
}

