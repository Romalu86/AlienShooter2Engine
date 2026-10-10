
#include "myerror.h"
#include "texture.h"
#include "engine_globals.h"
#include "mylib.h"
#include "zm/Utils/zVectorOperations.h"
#include "script.h"
#include "player_arcade.h"
#include "irpg.h"
#include "zm/Game/ScriptExec/zScriptExec.h"
#include "zm/IniParser/zIniParser.h"
#include "message.h"
#include "map.h"
#include "input.h"
#include "net.h"
#include "sprite.h"
#include "vid.h"
#include "zm/Game/zItemTaker.h"
#include "dialog_unit.h"
#include "man.h"
#include "zm/Game/Vehicles/zCarControl.h"
#include "zm/Survive/zSurvive.h"
#include "zm/Menu/zShopDlgMenu.h"
#include "zm/Menu/zShopItemInfo.h"
#include "zm/Menu/zHeroInvMenu.h"
#include "zm/Menu/zParamDlgMenu.h"
#include "zm/Menu/zBriefingDlgMenu.h"
#include <new>
#include "graph.h"
#include "region.h"
#include "dialogSrv.h"
#include "sound.h"
#include "constant.h"
#include "zm/Eng/zProfLdr/zProfLdr.h"
#include "zm/Eng/zProfLdr/zProfMngr.h"
#include "zm/Utils/String/zString.h"
#include "zm/Utils/zRefObject.h"
#include "zm/Utils/zLogConsole.h"
#include "zm/Game/zVidCopier.h"
#include "zm/RPGHero/zRPGFrameWork.h"
#include "zm/RPGHero/zRPGParamDialog.h"
#include "zm/Quest/zQuestServer.h"
#include "zm/Menu/zSpriteInventory.h"
void __cdecl GetNew_zRPGFrameWork(iRPGFrameWork** outObject);


class iHeroInvMenu;
class iParamDlgMenu;
void __cdecl GetNew_zHeroInvMenu(iHeroInvMenu** outObject,iRPGFrameWork* framework);
void __cdecl GetNew_zParamDlgMenu(iParamDlgMenu** outObject,iRPGParamDialog* paramDialog,iHeroInvMenu* heroInvMenu);
unsigned int __cdecl RPGImplantMaskedRandom30Bit();


class iShopDlgMenu;
void __cdecl GetNew_zShopDlgMenu(iShopDlgMenu** outObject);

extern "C" void __cdecl MainSwCreateCurScreenShot(void);
static void PlayerArcadePtrVectorInsertFill(PLAYER_ARCADE_PTR_VECTOR& storage, PTR_SPRITE* position, unsigned int count, const PTR_SPRITE& value);


iBriefingDlgMenu* PLAYER_ARCADE::m_pBriefingDlgMenu = 0;
int PLAYER_ARCADE::m_iDeHypnMonsters = 0;

namespace
{
bool radioLastPhrase = false;
bool radioAddedToLog = false;
}

PLAYER_ARCADE_PTR_VECTOR::PLAYER_ARCADE_PTR_VECTOR(unsigned char allocatorValue)
    : begin(0), end(0), capacityEnd(0)
{
    allocatorState[0] = allocatorValue;
}

PLAYER_ARCADE_PTR_VECTOR::~PLAYER_ARCADE_PTR_VECTOR() noexcept(false)
{
    PTR_SPRITE* const last = end;
    for (PTR_SPRITE* entry = begin; entry != last; ++entry)
        entry->~PTR_SPRITE();
    CrtFreePointer(begin);
    begin = 0;
    end = 0;
    capacityEnd = 0;
}

PLAYER_ARCADE_DWORD_VECTOR::PLAYER_ARCADE_DWORD_VECTOR(unsigned char allocatorValue)
    : begin(0), end(0), capacityEnd(0)
{
    allocatorState[0] = allocatorValue;
}

PLAYER_ARCADE_DWORD_VECTOR::~PLAYER_ARCADE_DWORD_VECTOR() noexcept(false)
{
    CrtFreePointer(begin);
    begin = 0;
    end = 0;
    capacityEnd = 0;
}

StaticObjectStorage<MESSAGE> PLAYER_ARCADE::m_oMessage;
unsigned char PLAYER_ARCADE::messageDestructorFlags = 0;

unsigned int __cdecl PLAYER_ARCADE::InitializeMessage()
{
    ::new (static_cast<void*>(m_oMessage.Address())) MESSAGE(5, -1, 0.0f, 0.0f, 4, 5000u);
    return RegisterCrtAtExit(&PLAYER_ARCADE::DestroyMessage);
}

void __cdecl PLAYER_ARCADE::DestroyMessage()
{
    if ((messageDestructorFlags & 1u) != 0u)
        return;
    messageDestructorFlags = static_cast<unsigned char>(messageDestructorFlags | 1u);
    m_oMessage.Get().~MESSAGE();
}

void* PLAYER_ARCADE::QueryInterface(int interfaceId) const
{

    PLAYER_ARCADE* const owner = const_cast<PLAYER_ARCADE*>(this);
    if (interfaceId == 35) return static_cast<zScriptExec*>(owner);
    if (interfaceId == 21) return static_cast<iPlayer*>(owner);
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 101u);
    return 0;
}


int PLAYER_ARCADE::iRelease()
{
    PLAYER* const owner = static_cast<PLAYER*>(this);
    owner->~PLAYER();
    CrtFreePointer(owner);
    return 0;
}


bool PLAYER_ARCADE::IsCurPlayer()
{
    return Map != 0 && Map->Player() == this;
}


SPRITE* PLAYER_ARCADE::Flagman()
{
    return PLAYER::Flagman();
}


int PLAYER_ARCADE::GetDelayForRotateAfterFire()
{
    return 5000;
}


iRPGHero* PLAYER_ARCADE::GetPlayerHero()
{
    return m_refLegs.m_object;
}


int PLAYER_ARCADE::GetArmy()
{
    return static_cast<int>(m_playerIndex);
}


void PLAYER_ARCADE::CopyHeroToArmy(iRPGObj* object, int army)
{
    ScriptExecSubobject()->ProfMngr()->CopyHeroToArmy(object, army);
}

int PLAYER_ARCADE::GetCurTime()
{
    return static_cast<int>(::CurrentTime);
}


int PLAYER_ARCADE::GetCurCadr(int object)
{
    const SPRITE* const sprite = reinterpret_cast<const SPRITE*>(static_cast<unsigned int>(object));
    return sprite != 0 ? sprite->CurrentFrame() : 0;
}

void* PLAYER_ARCADE::ScalarDeletingDestructor(unsigned int flags)
{
    void* self = this;
    this->PLAYER_ARCADE::~PLAYER_ARCADE();
    if (flags&1u) CrtFreePointer(self);
    return self;
}

void PLAYER_ARCADE::OnClipReloading()
{

    PlaySFx(82, false);
}

void PLAYER_ARCADE::OnClipReload()
{

    PlaySFx(83, false);
}

int PLAYER_ARCADE::OnSpriteCreate(SPRITE* sprite)
{
    return OnSpriteAction(sprite, CSI_ACTION_CREATE);
}


int PLAYER_ARCADE::OnSpriteDeath(const SPRITE* sprite)
{
    return OnSpriteAction(sprite, CSI_ACTION_DEATH);
}

void PLAYER_ARCADE::PutMessage(const STRING& text, float x, float y)
{
    m_oMessage.Get().PutToStack(text, x, y);
}


void PLAYER_ARCADE::StopSFx(int, bool)
{
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1653u);
}


void PLAYER_ARCADE::WinLevel()
{
    Map->m_script.SetVariableActionInt(8u, 1);
}

void PLAYER_ARCADE::DrawSecondaryInfo()
{
    iLogConsole* const debugInfo = ::gpLogCons;
    if (debugInfo)
        debugInfo->Render(Graph);
}

void PLAYER_ARCADE::SetCurAmmoDir(int direction)
{
    zScriptExec* const scriptExec = ScriptExecSubobject();
    if (scriptExec->m_curAmmoDir != direction)
    {
        scriptExec->m_curAmmoDir = direction;
        m_parametersDirty = 1u;
    }
}


int PLAYER_ARCADE::GetDirtyState()
{

    if (m_parametersDirty == 0) return 0;
    m_parametersDirty = 0;
    return 1;
}


int PLAYER_ARCADE::GetVidDirectionsNum(int vidIndex)
{
    if (vidIndex<0 || vidIndex >= Map->m_vidCount) return 0;
    VID* const definition = Map->m_vids[static_cast<unsigned int>(vidIndex)];
    return definition ? definition->noDir : 0;
}


int PLAYER_ARCADE::GetNoSprites(int vidIndex, unsigned int army)
{
    VID* definition = 0;
    if (vidIndex >= 0 && vidIndex<Map->m_vidCount)
        definition = Map->m_vids[static_cast<unsigned int>(vidIndex)];
    if (definition == 0)
        definition = ::EmptyVid;
    return static_cast<int>(definition->spriteCountsByPlayer[army]);
}


int PLAYER_ARCADE::GetFaceDir(int value)
{
    VID* vid = 0;
    if (Map->m_vidCount>390)
        vid = Map->m_vids[390];
    if (vid == 0)
        vid = ::EmptyVid;

    const int scaled = BitsToSigned32((static_cast<unsigned int>(value) - 380u) << 8);
    return (scaled/vid->noDir)&0xFF;
}


void PLAYER_ARCADE::ShowInventory(bool show)
{
    iHeroInvMenu* const inventory = zScriptExec::m_pHeroInvMenu;
    if (inventory == 0) return;
    iBaseDlgMenu* const base = inventory->AsBaseDlgMenu();
    base->Show(show);
}


void PLAYER_ARCADE::SetTankCanShot()
{
    iPlayer* const playerInterface = PlayerInterface();
    MAN* flagman = static_cast<MAN*>(playerInterface->Flagman());
    if (flagman == 0) return;
    flagman = static_cast<MAN*>(playerInterface->Flagman());
    flagman->SetTankCanShot();
}


void PLAYER_ARCADE::RemoveFlagmanDialog()
{
    iPlayer* const playerInterface = PlayerInterface();
    MAN* active = static_cast<MAN*>(playerInterface->Flagman());
    if (active == 0) return;
    active = static_cast<MAN*>(playerInterface->Flagman());
    static_cast<DIALOG_UNIT*>(static_cast<UNIT*>(active))->ClearDialogs();
}


int PLAYER_ARCADE::RecalcCurAmmoClip(bool force)
{
    iPlayer* const playerInterface = PlayerInterface();
    MAN* flagman = static_cast<MAN*>(playerInterface->Flagman());
    if (flagman == 0) return 0;

    flagman = static_cast<MAN*>(playerInterface->Flagman());
    return static_cast<int>(flagman->RecalcCurAmmoClipResult(force));
}


void PLAYER_ARCADE::SetFlagmanDialog(int dialogId)
{
    MAN* active = static_cast<MAN*>(PlayerInterface()->Flagman());
    if (active == 0) return;
    active = static_cast<MAN*>(PlayerInterface()->Flagman());
    static_cast<DIALOG_UNIT*>(static_cast<UNIT*>(active))->SetDialog(dialogId);
}

int PLAYER_ARCADE::GetReloadTimer()
{
    iPlayer* const playerInterface = PlayerInterface();
    SPRITE* flagman = playerInterface->Flagman();
    if (flagman == 0) return 0;

    flagman = playerInterface->Flagman();
    SPRITE* weaponSprite = flagman->ChildChain();
    if (weaponSprite == 0) return 0;

    flagman = playerInterface->Flagman();
    weaponSprite = flagman->ChildChain();
    return static_cast<int>(weaponSprite->StateTimer());
}

void PLAYER_ARCADE::SetReloadTimer(int timer)
{
    iPlayer* const playerInterface = PlayerInterface();
    SPRITE* flagman = playerInterface->Flagman();
    if (flagman == 0) return;

    flagman = playerInterface->Flagman();
    SPRITE* weaponSprite = flagman->ChildChain();
    if (weaponSprite == 0) return;

    if (timer<0 || timer >= 20000) timer = 0;

    flagman = playerInterface->Flagman();
    weaponSprite = flagman->ChildChain();
    weaponSprite->SetStateTimer(static_cast<unsigned int>(timer));
}

void PLAYER_ARCADE::SetNumInVolley(int volleyCount)
{
    iPlayer* const playerInterface = PlayerInterface();
    SPRITE* flagman = playerInterface->Flagman();
    if (flagman == 0) return;

    flagman = playerInterface->Flagman();
    SPRITE* weaponSprite = flagman->ChildChain();
    if (weaponSprite == 0) return;

    flagman = playerInterface->Flagman();
    weaponSprite = flagman->ChildChain();
    EX_SPRITE_DATA* const weaponData = weaponSprite->ExtraData();
    if (weaponData != 0) weaponData->tacticalCounter = static_cast<unsigned int>(volleyCount);
}


void PLAYER_ARCADE::InventorySetActive(bool active)
{
    iHeroInvMenu* const inventory = zScriptExec::m_pHeroInvMenu;
    if (inventory != 0)
    {
        iBaseDlgMenu* const base = inventory->AsBaseDlgMenu();
        base->SetInputActive(active);
    }

    iShopDlgMenu* const shopMenu = zScriptExec::m_pShopDlgMenu;
    if (shopMenu != 0)
    {
        iBaseDlgMenu* const base = shopMenu->AsBaseDlgMenu();
        base->SetInputActive(active);
    }
}

int PLAYER_ARCADE::GetHeroPDClr(int army)
{
    PLAYER_ARCADE* const player = static_cast<PLAYER_ARCADE*>(Map->Player(army & 7));
    if (player == 0) return 0;

    iVidCopier* const vidCopier = static_cast<iVidCopier*>(player->RefObjectSubobject()->QueryInterface(35));
    return vidCopier != 0 ? vidCopier->GetHeroColor() : 0;
}

int PLAYER_ARCADE::ScriptRun(int function, const char* script, int scriptArgument1, int scriptArgument2, int scriptArgument3)
{
    MAP* const map = Map;
    if (map == 0 || function<0) return 0;
    return map->ScriptRun(function, script, scriptArgument1, scriptArgument2, scriptArgument3);
}


int PLAYER_ARCADE::ScriptGetNFunction(const char* name)
{
    if (Map == 0) return 0;
    const STRING temporary(name);
    return Map->m_script.GetNFunction(temporary);
}


void PLAYER_ARCADE::GetSpeed(SPRITE* _pSpr, float& speedX, float& speedY)
{
    speedX = 0.0f;
    speedY = 0.0f;
    if (_pSpr == 0) return;

    VID* const definition = _pSpr->Vid();
    if (definition == 0 || definition->spriteClass != 7u)
        DebugAssert(5u, "_pSpr->IsSpriteClass( B_MAN )", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1391u);

    static_cast<zCarControl*>(static_cast<MAN*>(_pSpr))->GetSpeed(speedX, speedY);
}

void PLAYER_ARCADE::ReloadDB()
{
    iLogConsole* const logger = ::gpLogCons;
    (void)logger->LogStr(LT_NOTICE, "Database reloading..");
    ReleaseAll();
    for (unsigned int playerIndex = 0; playerIndex < 8u; ++playerIndex)
        static_cast<PLAYER_ARCADE*>(Map->Player(playerIndex))->CreateAll();
}

int PLAYER_ARCADE::GetNumInVolley()
{
    iPlayer* const playerInterface = PlayerInterface();
    SPRITE* flagman = playerInterface->Flagman();
    if (flagman == 0) return 0;

    flagman = playerInterface->Flagman();
    SPRITE* weaponSprite = flagman->ChildChain();
    if (weaponSprite == 0) return 0;

    flagman = playerInterface->Flagman();
    weaponSprite = flagman->ChildChain();
    EX_SPRITE_DATA* const weaponData = weaponSprite->ExtraData();
    return weaponData != 0 ? static_cast<int>(weaponData->tacticalCounter) : 0;
}


void PLAYER_ARCADE::GetSpeed(float& speedX, float& speedY)
{
    speedX = 0.0f;
    speedY = 0.0f;

    iPlayer* const playerLane = PlayerInterface();
    SPRITE* active = playerLane->Flagman();
    if (active == 0) return;

    active = playerLane->Flagman();
    VID* const definition = active->Vid();
    if (definition->spriteClass == 7u)
    {
        VID* const state = definition->linkVid;
        if (state != 0)
        {
            const WEAPON_RUNTIME* const flags = state->weapon;
            if ((flags->Property&0x800u) != 0u)
                return;
        }
    }

    active = playerLane->Flagman();
    GetSpeed(active, speedX, speedY);
}

bool PLAYER_ARCADE::IsReadyForWeaponChange()
{
    bool ready = true;
    iPlayer* const playerInterface = PlayerInterface();

    SPRITE* flagman = playerInterface->Flagman();
    if (flagman != 0)
    {
        flagman = playerInterface->Flagman();
        VID* const definition = flagman->Vid();
        if (definition != 0 && definition->nVid == 1200)
            ready = false;
    }

    flagman = playerInterface->Flagman();
    SPRITE* weaponSprite = 0;
    if (flagman != 0)
    {
        flagman = playerInterface->Flagman();
        weaponSprite = flagman->ChildChain();
    }

    if (ready && weaponSprite != 0 &&
       weaponSprite->Animation() == 8 &&
       (weaponSprite->RuntimeFlags()&0x00010000u) == 0u)
        return false;

    return ready;
}

int PLAYER_ARCADE::GetHeroPDId(int army)
{
    PLAYER_ARCADE* const player = static_cast<PLAYER_ARCADE*>(Map->Player(army & 7));
    if (player == 0 || ScriptExecSubobject()->m_mapType != 2u) return 0;

    iRPGHero* const hero = player->PlayerInterface()->GetPlayerHero();
    if (hero == 0) return 0;

    iRPGObj* const heroObject = static_cast<iRPGObj*>(hero->QueryInterface(9));
    return heroObject->GetBaseId();
}

void PLAYER_ARCADE::SetHeroPDClr(int army, int color)
{
    iLogConsole* logger = ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "SetHeroPDClr. army %d. color == %d", static_cast<unsigned int>(army), static_cast<unsigned int>(color));

    PLAYER_ARCADE* const player = static_cast<PLAYER_ARCADE*>(Map->Player(army & 7));
    if (player == 0) return;

    iVidCopier* const vidCopier = static_cast<iVidCopier*>(player->RefObjectSubobject()->QueryInterface(35));
    if (vidCopier != 0) (void)vidCopier->SetHeroColor(color);
}


static void AssertArcadeModeDisabled(PLAYER_ARCADE* player, unsigned int line)
{
    if (player->PlayerInterface()->GetArcadeMod_() != 0)
        DebugAssert(4u, "!GetArcadeMod_()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", line);
}

int PLAYER_ARCADE::GetQuestState(int group, int quest)
{
    AssertArcadeModeDisabled(this, 1557u);

    iQuestServer* server = zScriptExec::m_pQuestServer;
    if (server == 0) return 0;

    iRPGQuest* entry = server->GetQuest(group, quest, false, false);
    if (entry == 0)
    {
        iLogConsole* logger = ::gpLogCons;
        (void)logger->LogStr(LT_WARN, "Invalid Quest %d %d is get from scripts!", static_cast<unsigned int>(group), static_cast<unsigned int>(quest));
        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1569u);
        return -1;
    }

    server = zScriptExec::m_pQuestServer;
    return server->GetQuestState(entry) == 1 ? 1 : 0;
}

bool PLAYER_ARCADE::SetQuestState(int group, int quest, int state)
{
    (void)state;
    AssertArcadeModeDisabled(this, 1585u);

    iQuestServer* server = zScriptExec::m_pQuestServer;
    if (server == 0) return false;

    iRPGQuest* entry = server->GetQuest(group, quest, false, false);
    if (entry == 0)
    {
        iLogConsole* logger = ::gpLogCons;
        (void)logger->LogStr(LT_WARN, "Invalid Quest %d %d is set from scripts!", static_cast<unsigned int>(group), static_cast<unsigned int>(quest));
        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1596u);
        return false;
    }

    server = zScriptExec::m_pQuestServer;
    (void)server->SetQuestState(entry, 1, false);

    iLogConsole* logger = ::gpLogCons;
    (void)logger->LogStr(LT_NOTICE, "QS set from map %d - %d - %d", static_cast<unsigned int>(group), static_cast<unsigned int>(quest), 1);
    return true;
}


int PLAYER_ARCADE::GetCarSpeed(SPRITE* sprite)
{
    if (sprite == 0)
        sprite = PlayerInterface()->Flagman();

    float speedX = 0.0f;
    float speedY = 0.0f;
    GetSpeed(sprite, speedY, speedX);
    return TruncatedPlanarDistance(speedX, speedY);
}

void PLAYER_ARCADE::SetRadioEx(int radioId, int recordIndex, bool skipRecordCopy)
{
    zScriptExec* const exec = ScriptExecSubobject();
    iPlayer* const playerInterface = PlayerInterface();
    if (radioId == 0)
    {
        if (radioLastPhrase && radioAddedToLog)
            playerInterface->SendInfoMessage(5, 500);
        exec->m_pendingText.operator = (static_cast<const char*>(0));
        exec->m_radioIconVid = 0u;
        exec->m_radioIconDirection = 0u;
        exec->m_radioFontVid = 0u;
        radioLastPhrase = false;
        exec->m_radioMessageDirty = 1u;
        return;
    }

    zDlgSrv* const manager = ::g_pDlgSrv;
    DIALOG* const dlg = manager->GetDialog(radioId);
    if (!dlg) DebugAssert(5u, "dlg", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1817u);

    if (!skipRecordCopy)
    {
        const STRING& row = dlg->GetPhrase(recordIndex);
        exec->m_pendingText.operator = (row.m_value);
        exec->m_radioIconVid = dlg->m_radio;
        exec->m_radioIconDirection = dlg->m_faceDirection;
        exec->m_radioFontVid = static_cast<unsigned int>(dlg->m_unitVid[dlg->m_radio?1:0]);
    }

    radioAddedToLog = exec->ProfMngr()->AddLog(radioId, recordIndex, true);
    radioLastPhrase = recordIndex == dlg->m_phrases.m_count-1;
    playerInterface->SendInfoMessage(6, 0);
    exec->m_radioMessageDirty = 1u;
}


PLAYER_ARCADE::PLAYER_ARCADE(unsigned int mode)
    : PLAYER(mode), zRefObject(), zScriptExec(),
      m_hypnSprites(static_cast<unsigned char>(mode)),
      m_hypnTimes(static_cast<unsigned char>(mode))
{
    m_parametersDirty = 1u;
    m_prevInventoryAction = 0u;
    m_lastQuestCoordTick = 0u;
    m_lastInventoryTick = 0u;
    m_pCarToStop = 0;

    GetNew_zItemTaker(&m_itemTaker);


    GRAPH* const camera = Graph;
    m_oMessage.Get().m_rowShift = 1;
    m_oMessage.Get().m_baseX = camera->screenWidth-442.0f;
    m_oMessage.Get().m_baseY = camera->screenHeight-120.0f;

    iLogConsole* globalObject = ::gpLogCons;
    if (!globalObject)
    {
        GetNew_zLogConsole(&::gpLogCons);
        globalObject = ::gpLogCons;
    }

    globalObject->SetScrollTextDelay(0);

    CONSTANT* const flags = ::Const;
    if (flags->DebugMode != 0 || g_iZmDebug != 0u)
    {
        globalObject = ::gpLogCons;
        globalObject->Show_(1);
    }

    if (m_playerIndex == 0u)
        gpQuestEvent = QuestEventSubobject();
}

PLAYER_ARCADE::~PLAYER_ARCADE() noexcept(false)
{
    ReleaseAll();
    if (m_itemTaker)
    {
        (void)m_itemTaker->iRelease();
        m_itemTaker = 0;
    }
    iLogConsole* const globalObj = ::gpLogCons;
    if (globalObj)
    {
        (void)globalObj->iRelease();
        static_cast<void>(::gpLogCons = 0);
    }

}


void PLAYER_ARCADE::DeletePointerToSprite(SPRITE* sprite)
{
    m_oMessage.Get().DeletePointerToSprite(sprite);
    PLAYER::DeletePointerToSprite(sprite);
    iParamDlgMenu* const paramMenu = zScriptExec::m_pParamDlgMenu;
    if (paramMenu) paramMenu->DeletePointerToSprite(sprite);
    iShopDlgMenu* const shopMenu = zScriptExec::m_pShopDlgMenu;
    if (shopMenu)
    {
        iBaseDlgMenu* const base = shopMenu->AsBaseDlgMenu();
        base->DeletePointerToSprite(sprite);
    }
    iShopItemInfo* const shopInfo = zScriptExec::m_pShopItemInfo;
    if (shopInfo) shopInfo->DeletePointerToSprite(sprite);
    iHeroInvMenu* const heroMenu = zScriptExec::m_pHeroInvMenu;
    if (heroMenu)
    {
        iBaseDlgMenu* const base = heroMenu->AsBaseDlgMenu();
        base->DeletePointerToSprite(sprite);
    }
    iBriefingDlgMenu* const briefingMenu = m_pBriefingDlgMenu;
    if (briefingMenu) briefingMenu->DeletePointerToSprite(sprite);
    if (m_pCarToStop == sprite) m_pCarToStop = 0;
    m_itemTaker->DeletePointerToSprite(sprite);
    unsigned int index = 0;
    while (m_hypnSprites.begin && index<static_cast<unsigned int>(m_hypnSprites.end-m_hypnSprites.begin))
    {
        if (m_hypnSprites.begin[index].m_value != sprite)
        {
            ++index;
            continue;
        }
        const unsigned int count = static_cast<unsigned int>(m_hypnSprites.end-m_hypnSprites.begin);
        for (unsigned int i = index+1;i<count;++i)m_hypnSprites.begin[i-1] = m_hypnSprites.begin[i];
        PTR_SPRITE* const removedEntry = m_hypnSprites.end-1;
        removedEntry->~PTR_SPRITE();
        --m_hypnSprites.end;
        const unsigned int timeCount = static_cast<unsigned int>(m_hypnTimes.end-m_hypnTimes.begin);
        for (unsigned int i = index+1;i<timeCount;++i)m_hypnTimes.begin[i-1] = m_hypnTimes.begin[i];
        --m_hypnTimes.end;
    }
}


void PLAYER_ARCADE::SetFlagman(SPRITE* newReference)
{
    iPlayer* const player = PlayerInterface();
    SPRITE* previous = player->Flagman();

    if (newReference)
        newReference->AddReference();

    if (m_flagman.m_value)
        m_flagman.m_value->Release();
    m_flagman.m_value = newReference;
    m_moveHistoryCount = 0u;

    if (previous && player->Flagman() != previous)
    {
        VID* const link = previous->Vid();
        const unsigned int id = static_cast<unsigned int>(link->nVid);
        if (id == 1206u || id == 3000u)
            m_pCarToStop = previous;
    }

    if (player->Flagman() != 0)
    {
        SPRITE* current = player->Flagman();
        VID* link = current->Vid();
        bool permitted = true;
        if (link->spriteClass == 7u)
        {
            VID* const route = link->linkVid;
            if (route)
            {
                WEAPON_RUNTIME* const cfg = route->weapon;
                if ((cfg->Property&0x800u) != 0u)
                    permitted = false;
            }
        }

        if (permitted)
        {
            current = player->Flagman();
            link = current->Vid();
            const unsigned int id = static_cast<unsigned int>(link->nVid);
            if (id == 1206u || id == 3000u)
            {
                SPRITE* const sprite = player->Flagman();
                current = player->Flagman();
                static_cast<zCarControl*>(static_cast<MAN*>(current))->InitializeCarControl(sprite);
            }
        }
    }

    if (previous && player->Flagman() != previous)
    {
        static_cast<MAN*>(previous)->SetPlayer(0, 0);
        SPRITE* const current = player->Flagman();
        if (current)
        {
            const unsigned int id = static_cast<unsigned int>(current->Vid()->nVid);
            if (id == 3010u || id == 3040u)
                goto retain_previous_hero;
        }
        static_cast<MAN*>(previous)->SetRPGHero(0);
    }
retain_previous_hero:

    if (player->Flagman() != 0)
    {
        iRPGFrameWork* const manager = zScriptExec::m_pRPGFrameWork;
        SPRITE* current = player->Flagman();
        static_cast<MAN*>(current)->SetPlayer(player, manager);

        current = player->Flagman();
        if (current)
        {
            const unsigned int id = static_cast<unsigned int>(current->Vid()->nVid);
            if (id == 3010u || id == 3040u)
                goto change_parameters;
        }

        {
            iRPGHero* const hero = m_refLegs.m_object;
            current = player->Flagman();
            static_cast<MAN*>(current)->SetRPGHero(hero);
        }
change_parameters:
        ChangeParameters();
    }
}


void PLAYER_ARCADE::PlaySFx(int effectId, bool positioned)
{
    if (!positioned)
    {
        if (PlayerInterface()->IsCurPlayer())
            ::Sound->PlaySFX(effectId, 0, 0, 1.0f);
        return;
    }

    SPRITE* object = PlayerInterface()->Flagman();
    if (object == 0) return;

    object = PlayerInterface()->Flagman();
    const float objectX = object->X();
    object = PlayerInterface()->Flagman();
    const float objectY = object->Y();

    GRAPH* const camera = Graph;
    const float scale = 0.5f;
    const float cameraX = camera->screenWidth*scale;
    const float cameraY = camera->screenHeight*scale;
    const VECTOR2& mapShift = Map->ShiftXY();

    VECTOR2 position;
    position.x = static_cast<float>(TruncateFloatToInt32(objectX))-cameraX-mapShift.x;
    position.y = static_cast<float>(TruncateFloatToInt32(objectY))-cameraY-mapShift.y;
    ::Sound->PlaySFXFromCoor(effectId, position, 1.0f);
}


float MAP::ToScreenY(float value) const
{
    return value-m_shiftXY.y;
}


static void PlayerArcadeDestroyEntryRange(PTR_SPRITE* begin, PTR_SPRITE* end)
{
    while (begin != end)
    {
        begin->~PTR_SPRITE();
        ++begin;
    }
}

int PLAYER_ARCADE::OnSpriteAction(const SPRITE* sprite, CSI_ACTION action)
{
    if (PlayerInterface()->GetArcadeMod_() != 0) return 0;
    if (!zScriptExec::m_pQuestServer || !sprite) return 0;

    EX_SPRITE_DATA* const extraData = sprite->ExData();
    if (extraData && extraData->nameText.value[0])
    {
        STRING name = sprite->GetName();
        iQuestServer* const quests = zScriptExec::m_pQuestServer;
        quests->CheckStateItemName(action, name.value);
    }

    const int vidIndex = sprite->Vid()->nVid;
    iQuestServer* const quests = zScriptExec::m_pQuestServer;
    quests->CheckStateItemVid(action, vidIndex);
    return 0;
}


void PLAYER_ARCADE::ReleaseAll()
{
    iParamDlgMenu* const paramMenu = zScriptExec::m_pParamDlgMenu;
    if (paramMenu)
    {
        paramMenu->iRelease();
        zScriptExec::m_pParamDlgMenu = 0;
    }

    iPlayer* const playerInterface = PlayerInterface();
    zScriptExec* const scriptExec = ScriptExecSubobject();
    if (zScriptExec::m_pShopDlgMenu != 0 && playerInterface->IsCurPlayer())
    {
        iShopDlgMenu* const shopMenu = zScriptExec::m_pShopDlgMenu;
        shopMenu->iRelease();
        zScriptExec::m_pShopDlgMenu = 0;
    }

    iShopItemInfo* const shopInfo = zScriptExec::m_pShopItemInfo;
    if (shopInfo)
    {
        shopInfo->iRelease();
        zScriptExec::m_pShopItemInfo = 0;
    }
    iHeroInvMenu* const heroMenu = zScriptExec::m_pHeroInvMenu;
    if (heroMenu)
    {
        heroMenu->iRelease();
        zScriptExec::m_pHeroInvMenu = 0;
    }

    if (scriptExec->m_paramDialog)
    {
        scriptExec->m_paramDialog->iRelease();
        scriptExec->m_paramDialog = 0;
    }

    iBriefingDlgMenu* const briefingMenu = m_pBriefingDlgMenu;
    if (briefingMenu)
    {
        briefingMenu->iRelease();
        m_pBriefingDlgMenu = 0;
    }
    iRPGFrameWork* const framework = zScriptExec::m_pRPGFrameWork;
    if (framework)
    {
        framework->iRelease();
        zScriptExec::m_pRPGFrameWork = 0;
    }
    iQuestServer* const quests = zScriptExec::m_pQuestServer;
    if (quests)
    {
        quests->iRelease();
        zScriptExec::m_pQuestServer = 0;
    }
}

void PLAYER_ARCADE::SetInventoryCoord(int inventorySlot, float deltaX, float deltaY)
{
    int currentSlot = inventorySlot;
    for (;;)
    {
        const float screenScale = 0.5f;
        GRAPH* const graph = Graph;
        const float centerX = graph->screenWidth * screenScale;
        const float centerY = graph->screenHeight * screenScale;

        if (m_refLegs.m_object == 0)
            CheckRPGHeroChange(false);

        iRPGHero* hero = m_refLegs.m_object;
        iInvServer* inventoryServer = hero->GetInvServer();
        const float screenY = centerY + deltaY;
        const float screenX = centerX + deltaX;
        inventoryServer->SetInventoryCoord(currentSlot, screenX, screenY);
        if (currentSlot != 15) return;

        hero = m_refLegs.m_object;
        inventoryServer = hero->GetInvServer();
        iInventory* const inventory = inventoryServer->GetInv(currentSlot);
        iInventory2D* const grid = static_cast<iInventory2D*>(inventory->QueryInterface(17));
        (void)grid->GetCellSizeX();
        const int rowStep = BitsToSigned32(static_cast<unsigned int>(grid->GetCellSizeY()) + 14u);
        SetInventoryCoord(16, deltaX, static_cast<float>(static_cast<double>(deltaY) + rowStep));
        const int secondRowStep = BitsToSigned32(static_cast<unsigned int>(rowStep) * 2u);
        SetInventoryCoord(17, deltaX, static_cast<float>(static_cast<double>(deltaY) + secondRowStep));
        const int thirdRowStep = BitsToSigned32(static_cast<unsigned int>(rowStep) * 3u);
        currentSlot = 18;
        deltaY = static_cast<float>(static_cast<double>(deltaY) + thirdRowStep);
    }
}
#include "basetype.h"


void PLAYER_ARCADE::SetHeroPDId(int army, int heroId, bool force)
{
    if (heroId == 0) return;

    iLogConsole* logger = ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "SetHeroPDId. army %d. id == %d. _bForceSet = %d", static_cast<unsigned int>(army), static_cast<unsigned int>(heroId), force?1u:0u);

    if (army == PlayerInterface()->GetArmy() && !force) return;

    PLAYER_ARCADE* const player = static_cast<PLAYER_ARCADE*>(Map->Player(army & 7));
    if (player == 0) return;

    iRPGFrameWork* const framework =
        zScriptExec::m_pRPGFrameWork;
    iRPGObj* const hero = framework->GetItemById(heroId);
    if (hero == 0)
        DebugAssert(4u, "hero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1903u);

    if (!hero->IsBaseObject() || hero->GetGameObj() != 0)
        DebugAssert(4u, "hero->IsBaseObject() && hero->GetType() == TYPE_HERO", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1904u);
    if (hero == 0) return;

    zScriptExec* const scriptExec = player->ScriptExecSubobject();
    const int heroColor = scriptExec->GetHeroColor();
    scriptExec->ReleaseArmyHero();
    player->PlayerInterface()->CopyHeroToArmy(hero, heroColor);

    logger = ::gpLogCons;
    (void)logger->LogStr(LT_INFO, "m_pArmyHero - PLAYER_ARCADE. %s", hero->GetName());

    iRPGObj* const armyHero = scriptExec->ProfMngr()->GetArmyHero(false, true, false);
    scriptExec->m_paramDialog->SetHeroExternal(armyHero);
    static_cast<iVidCopier*>(scriptExec)->CheckRPGHeroChange(true);
}

void PLAYER_ARCADE::GetStrIniString(iString* _pString, const char* sectionName, const char* keyName)
{
    if (!_pString) DebugAssert(4u, "_pString", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1700u);

    const STRING key(keyName);
    const STRING defaultValue("");
    const STRING section(sectionName);
    PROFILE* const profile = ::Strings;
    const STRING result = profile->GetString(section, key, defaultValue);
    _pString->operator = (result.value);
}


void PLAYER_ARCADE::AddHypnMonst(SPRITE* sprite, int timeOffset)
{
    {
        PTR_SPRITE reference;
        reference.m_value = sprite;
        if (sprite) sprite->AddReference();
        PlayerArcadePtrVectorInsertFill(m_hypnSprites, m_hypnSprites.end, 1u, reference);
    }
    unsigned int expiry = static_cast<unsigned int>(PlayerInterface()->GetCurTime()) + static_cast<unsigned int>(timeOffset);
    VectorOperations::Insert(m_hypnTimes.begin, m_hypnTimes.end,
        m_hypnTimes.capacityEnd, m_hypnTimes.end, 1u, expiry);
}

void PLAYER_ARCADE::DeHypnMonsters()
{
    for (unsigned int index = 0; m_hypnSprites.begin && index < static_cast<unsigned int>(m_hypnSprites.end - m_hypnSprites.begin); ++index)
    {
        const unsigned int* const expiryTimes = m_hypnTimes.begin;
        const int now = PlayerInterface()->GetCurTime();
        if (now <= BitsToSigned32(expiryTimes[index])) continue;

        if (!m_iDeHypnMonsters)
        {
            {
                const STRING functionName("DeHypnotize");
                m_iDeHypnMonsters = Map->m_script.GetNFunction(functionName);
            }
            if (!m_iDeHypnMonsters)
                DebugAssert(4u, "m_iDeHypnMonsters", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1861u);
        }
        if (m_iDeHypnMonsters)
        {
            SPRITE* const sprite = m_hypnSprites.begin[index].m_value;
            (void)Map->ScriptRun(m_iDeHypnMonsters, "%p", static_cast<int>(PtrToU32(sprite)), 0, 0);
        }

        PTR_SPRITE* const last = m_hypnSprites.end;
        for (PTR_SPRITE* next = m_hypnSprites.begin + index + 1u; next != last; ++next)
            next[-1] = *next;
        m_hypnSprites.end[-1].~PTR_SPRITE();
        --m_hypnSprites.end;

        const unsigned int* const lastTime = m_hypnTimes.end;
        for (unsigned int* next = m_hypnTimes.begin + index + 1u; next != lastTime; ++next)
            next[-1] = *next;
        --m_hypnTimes.end;
    }
}

void PLAYER_ARCADE::DrawCarDebug()
{
    if ((Map->m_flags & 0x2000u) == 0u) return;
    iPlayer* const player = PlayerInterface();
    SPRITE* hero = player->Flagman();
    if (!hero) return;
    hero = player->Flagman();
    if (hero->Vid()->spriteClass != 7u) return;
    hero = player->Flagman();
    if (!hero) return;
    zCarControl* const carControl = static_cast<zCarControl*>(static_cast<MAN*>(hero));

    const VEC2 goal = carControl->GetGoalCoord();
    if (!FloatEqualOrUnordered(goal.x, 0.0f) || !FloatEqualOrUnordered(goal.y, 0.0f))
    {
        const COLOR color = GRAPH::GREEN;
        VECTOR2 goalScreen;
        goalScreen.y = Map->ToScreenY(goal.y);
        goalScreen.x = Map->ToScreenX(goal.x);
        const VECTOR2 heroScreen = hero->ScreenXY();
        Graph->Line(heroScreen, goalScreen, color);
    }

    const VEC2 nextNode = carControl->GetNextNodeCoord();
    if (!FloatEqualOrUnordered(nextNode.x, 0.0f) || !FloatEqualOrUnordered(nextNode.y, 0.0f))
    {
        const COLOR color = GRAPH::BLUE;
        VECTOR2 nextScreen;
        nextScreen.y = Map->ToScreenY(nextNode.y);
        nextScreen.x = Map->ToScreenX(nextNode.x);
        VECTOR2 goalScreen;
        goalScreen.y = Map->ToScreenY(goal.y);
        goalScreen.x = Map->ToScreenX(goal.x);
        Graph->Line(goalScreen, nextScreen, color);
    }
}


static void PlayerArcadePtrVectorInsertFill(PLAYER_ARCADE_PTR_VECTOR& storage, PTR_SPRITE* position, unsigned int count, const PTR_SPRITE& value)
{
    const unsigned int available = storage.end ? static_cast<unsigned int>(storage.capacityEnd - storage.end) : 0u;
    if (available < count)
    {
        const unsigned int size = storage.begin ? static_cast<unsigned int>(storage.end - storage.begin) : 0u;
        const unsigned int newCapacity = size + MaxUnsigned(size, count);
        const unsigned int allocationCount = BitsToSigned32(newCapacity) < 0 ? 0u : newCapacity;
        PTR_SPRITE* const newBegin = static_cast<PTR_SPRITE*>(CrtNew(allocationCount * sizeof(PTR_SPRITE)));
        PTR_SPRITE* destination = newBegin;
        for (PTR_SPRITE* source = storage.begin; source != position; ++source, ++destination)
            if (destination) ::new (static_cast<void*>(destination)) PTR_SPRITE(*source);
        for (unsigned int index = 0; index < count; ++index, ++destination)
            if (destination) ::new (static_cast<void*>(destination)) PTR_SPRITE(value);
        PTR_SPRITE* const last = storage.end;
        for (PTR_SPRITE* source = position; source != last; ++source, ++destination)
            if (destination) ::new (static_cast<void*>(destination)) PTR_SPRITE(*source);
        PlayerArcadeDestroyEntryRange(storage.begin, storage.end);
        CrtFreePointer(storage.begin);
        storage.capacityEnd = newBegin + newCapacity;
        const unsigned int liveSize = storage.begin ? static_cast<unsigned int>(storage.end - storage.begin) : 0u;
        storage.begin = newBegin;
        storage.end = newBegin + liveSize + count;
        return;
    }

    const unsigned int tailSize = storage.end ? static_cast<unsigned int>(storage.end - position) : 0u;
    if (tailSize < count)
    {
        PTR_SPRITE* destination = position + count;
        PTR_SPRITE* const last = storage.end;
        for (PTR_SPRITE* source = position; source != last; ++source, ++destination)
            if (destination) ::new (static_cast<void*>(destination)) PTR_SPRITE(*source);
        destination = storage.end;
        const unsigned int extra = count - static_cast<unsigned int>(storage.end - position);
        for (unsigned int index = 0; index < extra; ++index, ++destination)
            if (destination) ::new (static_cast<void*>(destination)) PTR_SPRITE(value);
        for (PTR_SPRITE* entry = position; entry != storage.end; ++entry)
            *entry = value;
        storage.end += count;
    }
    else if (count)
    {
        PTR_SPRITE* destination = storage.end;
        PTR_SPRITE* const last = storage.end;
        for (PTR_SPRITE* source = last - count; source != last; ++source, ++destination)
            if (destination) ::new (static_cast<void*>(destination)) PTR_SPRITE(*source);
        PTR_SPRITE* source = storage.end - count;
        destination = storage.end;
        while (source != position)
            *--destination = *--source;
        for (PTR_SPRITE* entry = position; entry != position + count; ++entry)
            *entry = value;
        storage.end += count;
    }
}


void PLAYER_ARCADE::SendInfoMessage(int messageId, int timeOffset)
{
    STRING key;
    STRING message;

    switch (messageId)
    {
    case 4:


        if (PlayerInterface()->GetArcadeMod_() != 0) return;
        key = "AutoTakeNotEnoughSpace";
        break;
    case 1:
        key = "CantBuy";
        break;
    case 2:
        key = "InvNotEnoughSpace";
        break;
    case 3:
        key = "NotEnoughMoney";
        break;
    case 5:
        key = "LogAdd";
        break;
    case 6:
        message = StringGlobals::EmptyText();
        break;
    default:
        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1742u);
        break;
    }

    if (messageId != 6)
    {
        STRING section;
        section = "messages";
        STRING localized;
        PROFILE* const profile = ::Strings;
        localized = profile->GetString(section, key, key);
        message = localized;
    }

    if (messageId == 5)
    {
        STRING extra;
        zScriptExec* const scriptOwner = ScriptExecSubobject();
        for (unsigned int i = 0;i<3u;++i)
        {
            extra = StringGlobals::EmptyText();
            scriptOwner->GetHotKey(extra, 26, static_cast<int>(i));
            if (extra.value[0] != 0)
            {
                message += " ";
                message += extra;
                break;
            }
        }
    }

    zScriptExec* const scriptExec = ScriptExecSubobject();
    scriptExec->m_debugText.operator = (message.value);
    scriptExec->m_messageAvailableTick = static_cast<unsigned int>(PlayerInterface()->GetCurTime())+
                     static_cast<unsigned int>(timeOffset);
    scriptExec->m_messageId = static_cast<unsigned int>(messageId);

}

STRING PLAYER_ARCADE::GetMouseTipsString() const
{
    STRING text;
    STRING result;

    MAP* const game = Map;
    SPRITE* sprite = game?game->m_menu.SelectedSprite():0;
    if (sprite == 0) sprite = m_transientSprite.m_value;

    const unsigned int mode = ScriptExecSubobject()->m_mapType;
    const bool specialMode = (mode == 3u || mode == 4u);
    bool found = false;
    unsigned int vidNumber = 0u;
    unsigned int direction = 0u;
    iRPGFrameWork* const framework = zScriptExec::m_pRPGFrameWork;

    if (framework != 0 && sprite != 0)
    {
        bool allowItemTips = true;
        if (mode == 2u || mode == 7u)
        {
            iRPGParamDialog* const paramDialog = ScriptExecSubobject()->m_paramDialog;
            allowItemTips = (paramDialog->GetPlayerMenuType() == 1);
        }
        if (allowItemTips)
        {
            VID* const definition = sprite->Vid();
            vidNumber = static_cast<unsigned int>(definition->nVid);
            if (vidNumber == 712u || vidNumber == 713u || vidNumber == 721u)
            {
                direction = static_cast<unsigned int>(definition->RealDirection(sprite->Direction().value));

                zString itemKey;
                const char** textEntry = 0;
                iTextDB* const textDB = framework->AsTextDB();

                if (specialMode && vidNumber == 721u)
                {
                    (void)itemKey.Format("%d_%d", vidNumber, direction);
                    const char* const itemKeyText = static_cast<const char*>(itemKey);
                    textEntry = textDB->GetExtInfo(1, itemKeyText);
                }
                else
                {
                    iRPGParamDialog* const paramDialog = ScriptExecSubobject()->m_paramDialog;
                    const bool survivalMenu = (paramDialog->GetPlayerMenuType() == 1);
                    const bool survivalPlayer = (static_cast<unsigned int>(PlayerInterface()->GetArcadeMod_())&1u) != 0u;
                    if (survivalMenu || survivalPlayer)
                    {
                        (void)itemKey.Format("%d_%d_surv", vidNumber, direction);
                        const char* const itemKeyText = static_cast<const char*>(itemKey);
                        textEntry = textDB->GetExtInfo(0, itemKeyText);
                    }
                    else if (!specialMode)
                    {
                        textEntry = textDB->GetExtInfo(0, static_cast<int>(vidNumber), static_cast<int>(direction));
                    }
                }

                if (textEntry != 0 && text.value[0] == '\0')
                {
                    const char* const itemText = textDB->GetSpeciallyDecodedString(textEntry, -1);
                    if (itemText != 0)
                    {
                        text = itemText;
                        found = true;
                    }
                }
                else if (textEntry == 0 && specialMode && (vidNumber == 712u || vidNumber == 713u))
                {

                    const char* const fallback = textDB->GetFirstString(static_cast<int>(vidNumber), static_cast<int>(direction));
                    if (fallback != 0) text = fallback;
                }
            }
        }
    }

    if (!found)
    {
        iHeroInvMenu* const inventoryMenu = zScriptExec::m_pHeroInvMenu;
        if (inventoryMenu != 0)
        {
            iBaseDlgMenu* const inventoryDialog = inventoryMenu->AsBaseDlgMenu();
            if (inventoryDialog->IsVivible())
            {
                const VECTOR2& mousePosition = game->InputState()->ClientMouseXY();
                const char* const mouseText = inventoryMenu->GetMouseTipsString(mousePosition.x, mousePosition.y);
                if (mouseText != 0) text = mouseText;
            }
        }
    }

    if (text.value[0] != '\0') result = text;
    return result;
}

void PLAYER_ARCADE::OnAddSkill(int skillType, int value)
{
    STRING titleKey;
    STRING unitKey;
    STRING title;
    STRING unit;
    STRING message;
    STRING valueText;

    if (skillType == RPGHERO_EXP)
    {
        titleKey = "ExpAdded";
        unitKey = "ExpUnit";
    }
    else if (skillType == RPGHERO_MONEY)
    {
        titleKey = "MoneyAdded";
        unitKey = "MoneyUnit";
    }

    if (titleKey.value[0] != '\0')
    {
        STRING section;
        section = "menu";
        PROFILE* const profile = ::Strings;
        STRING localizedTitle = profile->GetString(section, titleKey, titleKey);
        title = localizedTitle;
        STRING localizedUnit = profile->GetString(section, unitKey, unitKey);
        unit = localizedUnit;

        message = title;
        message += " : ";
        message += "<Font=03/>";

        char number[32];
        number[0] = '\0';
        const char* const converted = reinterpret_cast<const char*>(AddressToUIntPtr(
            CrtItoa(static_cast<unsigned int>(value), PtrToU32(number), 10u)));
        valueText = converted;

        if (unit.value[0] != '\0')
        {
            STRING unitSection;
            unitSection = "menu";
            STRING localizedValueUnit = profile->GetString(unitSection, unit, unit);
            valueText += localizedValueUnit;
        }

        if (skillType == RPGHERO_EXP)
        {
            message += "+";
            message += valueText;
            message += " ";
            message += unit;
        }
        else if (skillType == RPGHERO_MONEY)
        {
            message += unit;
            message += valueText;
        }
    }

    zScriptExec* const scriptExec = static_cast<zScriptExec*>(this);
    if (skillType == RPGHERO_EXP)
    {
        scriptExec->AddFlyIcon(2214, 0, value, 0, 0, 0, false);
    }
    else if (skillType == RPGHERO_MONEY)
    {
        scriptExec->AddFlyIcon(2213, 13, -value, 0, 0, 0, false);
    }
}

void PLAYER_ARCADE::CheckRPGHeroChange(bool force)
{
    zScriptExec* const scriptExec = static_cast<zScriptExec*>(this);
    iPlayer* const playerInterface = static_cast<iPlayer*>(scriptExec);
    iRPGObj* CurHero = 0;

    const unsigned int mode = scriptExec->m_mapType;
    if ((mode == 7u || mode == 2u || mode == 6u) &&
        playerInterface->IsCurPlayer() != 0u)
    {
        CurHero = scriptExec->m_paramDialog->GetCurHero();
        if (CurHero == 0)
            DebugAssert(4u, "CurHero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1325u);
    }

    const bool isCurrentPlayer = playerInterface->IsCurPlayer() != 0u;
    NET* const session = ::Net;
    bool DontCreateHero = false;
    if (!isCurrentPlayer &&
        (session->m_flags & 8u) == 0u)
    {
        DontCreateHero = true;
    }
    else if ((session->m_flags & 8u) != 0u)
    {
        const unsigned int army = m_playerIndex;
        if (army >= 8u || session->m_players[army].m_dpnid == 0u)
            DontCreateHero = true;
    }

    if (CurHero == 0)
    {
        const bool loadFromMaxSave = scriptExec->m_loadHeroFromMaxSave != 0u;
        const unsigned int survivalFlag =
            static_cast<unsigned int>(playerInterface->GetArcadeMod_()) & 1u;
        CurHero = scriptExec->ProfMngr()->GetArmyHero(
            survivalFlag != 0u,
            DontCreateHero,
            loadFromMaxSave);
        if (CurHero == 0 && !DontCreateHero)
            DebugAssert(5u, "CurHero || DontCreateHero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 1334u);
    }

    iRef<iRPGObj>& heroObjectRef = scriptExec->m_refBody;
    iRPGObj* currentHeroObject = heroObjectRef.m_object;
    if (currentHeroObject == CurHero && !force)
    {
        iRPGHero* const hero = scriptExec->m_refLegs.m_object;
        hero->SetPlayer(playerInterface);
        scriptExec->m_refLegs.m_object->SetItemTaker(scriptExec->m_itemTaker);
        return;
    }

    int savedColor = -1;
    if (currentHeroObject != 0)
    {
        if (currentHeroObject->IsBaseObject() ||
            (((::Net->m_flags & 8u) != 0u) && scriptExec->m_mapType == 2u))
            savedColor = heroObjectRef.m_object->GetSkillValue(RPGHERO_COLOR);
    }

    heroObjectRef.Set(CurHero);
    currentHeroObject = heroObjectRef.m_object;
    iRPGHero* const heroInterface =
        static_cast<iRPGHero*>(currentHeroObject->QueryInterface(0));

    iRef<iRPGHero>& heroRef = scriptExec->m_refLegs;
    if (heroRef.m_object != 0)
        heroRef.m_refObject->RemObjectRef(
            reinterpret_cast<iRef<iRefObject>*>(&heroRef));
    if (heroInterface == 0)
    {
        heroRef.m_object = 0;
        heroRef.m_refObject = 0;
    }
    else
    {
        heroRef.m_object = heroInterface;
        heroRef.m_refObject =
            static_cast<zRefObject*>(heroInterface->QueryInterface(2));
    }
    if (heroRef.m_object != 0)
        heroRef.m_refObject->AddObjectRef(
            reinterpret_cast<iRef<iRefObject>*>(&heroRef));

    if (savedColor != -1)
    {
        currentHeroObject = heroObjectRef.m_object;
        currentHeroObject->SetSkillValue(RPGHERO_COLOR, savedColor);
    }

    if (playerInterface->IsCurPlayer() != 0u)
    {
        iHeroInvMenu* const heroMenu =
            zScriptExec::m_pHeroInvMenu;
        if (heroMenu != 0)
            heroMenu->SetRPGHero(heroRef.m_object);

        iShopDlgMenu* const shopMenu =
            zScriptExec::m_pShopDlgMenu;
        if (shopMenu != 0)
        {
            shopMenu->SetRPGHero(heroRef.m_object);
            iInvServer* const server = heroRef.m_object->GetInvServer();
            zScriptExec::m_pShopDlgMenu->SetInvServer(server);
        }

        iParamDlgMenu* const paramMenu =
            zScriptExec::m_pParamDlgMenu;
        if (paramMenu != 0)
            paramMenu->SetInvHero(heroRef.m_object);
    }

    if (scriptExec->m_paramDialog != 0)
        scriptExec->m_paramDialog->SetHeroExternal(heroObjectRef.m_object);

    iRPGHero* const hero = heroRef.m_object;
    iInvServer* inventoryServer = hero->GetInvServer();
    inventoryServer->SetParent(
        zScriptExec::m_pHeroInvMenu);

    inventoryServer = heroRef.m_object->GetInvServer();
    inventoryServer->SetPlayer(playerInterface);
    heroRef.m_object->SetPlayer(playerInterface);
    heroRef.m_object->SetItemTaker(scriptExec->m_itemTaker);

    ChangeParameters();

    iLogConsole* const logger = ::gpLogCons;
    currentHeroObject = heroObjectRef.m_object;
    (void)logger->LogStr(LT_INFO, "CheckRPGHeroChange. %s. army - %d.", currentHeroObject->GetName(), m_playerIndex);
}

void PLAYER_ARCADE::SetCurMapType(
    int mapType, bool surviveMode, const STRING& campaignName, bool deathMatch)
{
    zScriptExec* const scriptExec = ScriptExecSubobject();
    iPlayer* const playerInterface = PlayerInterface();
    const unsigned int oldMode = m_mapType;
    m_mapType = static_cast<unsigned int>(mapType);

    NET* session = ::Net;
    if ((session->m_flags&0x08u) == 0u)
    {
        if (!playerInterface->IsCurPlayer())
            return;
        session = ::Net;
    }

    if ((session->m_flags&0x08u) != 0u)
    {
        const unsigned int playerSlot = m_playerIndex;
        if (playerSlot >= 8u)
            return;
        if (session->m_players[playerSlot].m_dpnid == 0u)
            return;
    }

    if (m_playerIndex == 0u && (scriptExec->m_mapType == 7u || scriptExec->m_mapType == 1u))
    {
        if (oldMode != 7u && oldMode != 1u && oldMode != 0u)
            scriptExec->ReloadDB();
        iRPGParamDialog* const paramDialog = m_paramDialog;
        paramDialog->FirstHero(-1);
    }

    iHeroInvMenu* const inventoryMenu =
        zScriptExec::m_pHeroInvMenu;
    inventoryMenu->SetShopDlgMenu((scriptExec->m_mapType == 3u || scriptExec->m_mapType == 4u)
        ? zScriptExec::m_pShopDlgMenu : 0);
    zScriptExec::m_pHeroInvMenu->SetShopItemInfo((scriptExec->m_mapType == 3u || scriptExec->m_mapType == 4u)
        ? zScriptExec::m_pShopItemInfo : 0);

    iParamDlgMenu* const paramMenu =
        zScriptExec::m_pParamDlgMenu;
    if (scriptExec->m_mapType == 2u)
        paramMenu->RefershHeroScreen();
    else if (scriptExec->m_mapType == 7u)
        paramMenu->RefreshPerkScreen();

    if (oldMode == 3u || oldMode == 4u ||
       (oldMode == 6u && scriptExec->m_mapType != 2u) ||
       (oldMode == 2u && scriptExec->m_mapType == 5u))
    {
        iRPGParamDialog* const paramDialog = m_paramDialog;
        paramDialog->AssignSkills();
    }

    CheckRPGHeroChange(true);
    ChangeParameters();

    iRPGHero* heroInterface = GetPlayerHero();
    if (!heroInterface)
        DebugAssert(5u, "m_pRPGHero", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 944u);
    heroInterface->SetAutoChangeWeapons(true);

    scriptExec->SetSurviveMode(
        surviveMode, campaignName.m_value);
    scriptExec->SetDeathMatchMode(deathMatch);

    heroInterface = GetPlayerHero();
    if (heroInterface)
    {
        iRPGObj* const hero = static_cast<iRPGObj*>(heroInterface->QueryInterface(9));
        hero->RecalcSkills();

        iInvServer* const inventoryServer = GetPlayerHero()->GetInvServer();
        iInventory* const inv = inventoryServer->GetInv(0);
        if (!inv)
            DebugAssert(5u, "inv", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 963u);

        bool inventoryEnabled = (static_cast<unsigned int>(playerInterface->GetArcadeMod_())&2u) != 0u;
        if (!inventoryEnabled)
            inventoryEnabled = (static_cast<unsigned int>(playerInterface->GetArcadeMod_())&1u) != 0u;
        const unsigned int objectType = inventoryEnabled ? 3u : 14u;
        const unsigned int activeInventory = inventoryEnabled ? 1u : 0u;
        inv->SetObjectType(static_cast<OBJ_TYPE>(objectType), static_cast<OBJ_EXTTYPE>(16), activeInventory != 0u);

        iLogConsole* const logger = ::gpLogCons;
        (void)logger->LogStr(LT_INFO, "zInventory::SetObjectType %d %d %d", objectType, 16u, activeInventory);
        inv->SetUnlimited(activeInventory != 0u);
    }

    const bool arcadeDisabled = static_cast<unsigned int>(playerInterface->GetArcadeMod_()) == 0u;
    if (oldMode != 4u && (scriptExec->m_mapType == 3u || scriptExec->m_mapType == 5u))
    {
        session = ::Net;
        if ((session->m_flags&0x08u) == 0u)
        {
            heroInterface = GetPlayerHero();
            const unsigned int heroFlags = static_cast<unsigned int>(playerInterface->GetArcadeMod_())&1u;
            iRPGObj* const hero = static_cast<iRPGObj*>(heroInterface->QueryInterface(9));
            iQuestServer* const questServer = arcadeDisabled
                ? zScriptExec::m_pQuestServer : 0;
            scriptExec->ProfMngr()->OnLevelBegin(
                hero, questServer, heroFlags != 0u);
        }
        (void)GetPlayerHero()->SetHpToMax();
        if (arcadeDisabled)
        {
            iQuestServer* const questServer = zScriptExec::m_pQuestServer;
            (void)questServer->OnBeginNewLevel();
        }
    }

    if (scriptExec->m_mapType != 1u)
        m_weaponState = -1;
}

void PLAYER_ARCADE::OnAutoTakeItem(iRPGObj* _pItem, int itemCount, int worldX, int worldY, int worldZ)
{
    if (_pItem == 0) DebugAssert(5u, "_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 743u);

    STRING message;
    STRING section;
    STRING key;
    section = "items";
    key = "Message";
    PROFILE* const profile = ::Strings;
    STRING localized = profile->GetString(section, key, key);
    message = localized;

    if (_pItem->GetGameObj() == 2)
    {
        STRING weaponKey = Printf("Weapon%d", _pItem->GetSkillValue(RPGWEAPON_CLASSS));
        STRING weaponName = profile->GetString(section, weaponKey, weaponKey);
        if (weaponName.value[0] != '\0')
        {
            message += weaponName;
            message += " ";
        }
    }

    message += "<Font=03/>";
    iRPGFrameWork* const framework = zScriptExec::m_pRPGFrameWork;
    const char* const itemName = framework->GetItemName(_pItem);
    if (itemName != 0) message += itemName;

    if (_pItem->GetId() == 0)
    {
        unsigned int iconVid = static_cast<unsigned int>(_pItem->GetVidNum(2));
        unsigned int iconDirection = static_cast<unsigned int>(_pItem->GetDirNum(2));
        unsigned int iconValue = static_cast<unsigned int>(itemCount);

        if (iconVid == 0u && _pItem->GetGameObj() == 6)
        {

            iRPGEquip* const equip = static_cast<iRPGEquip*>(_pItem->QueryInterface(5));
            iRPGObj* const addedAmmo = equip->GetAddAmmo();
            if (addedAmmo != 0)
            {
                iconVid = static_cast<unsigned int>(addedAmmo->GetVidNum(2));
                iconDirection = static_cast<unsigned int>(addedAmmo->GetDirNum(2));
                iconValue = static_cast<unsigned int>(addedAmmo->GetBuyNum());
            }
        }

        if (_pItem->ItemIsNotReal())
        {
            const unsigned int addedHp = static_cast<unsigned int>(_pItem->GetSkillValue(RPGEQUIP_ADDHP));
            if (addedHp != 0u)
            {
                iconVid = 2214u;
                iconDirection = 1u;
                iconValue = addedHp;
            }
        }

        unsigned int cannotHoldItem = 0u;
        if (_pItem != 0)
        {
            iRPGHero* const hero = m_refLegs.m_object;
            iInvKeeper* const inventoryKeeper = static_cast<iInvKeeper*>(hero->QueryInterface(1));
            if (!inventoryKeeper->CanHoldObj(_pItem)) cannotHoldItem = 1u;
        }

        zScriptExec* const scriptExec = static_cast<zScriptExec*>(this);
        scriptExec->AddFlyIcon(
            static_cast<int>(iconVid), static_cast<int>(iconDirection), static_cast<int>(iconValue),
            worldX, worldY, worldZ, cannotHoldItem != 0u);
    }

}


void PLAYER_ARCADE::ChangeParameters()
{

    m_parametersDirty = 1u;
    if (m_refBody.m_object->GetSkillValue(RPGHERO_DISABLED) != 0) return;

    NET* session = ::Net;
    iPlayer* const playerInterface = PlayerInterface();
    if ((session->m_flags&8u) == 0u)
    {
        if (playerInterface->IsCurPlayer() == 0u) return;
        session = ::Net;
    }
    if ((session->m_flags&8u) != 0u)
    {
        const unsigned int army = m_playerIndex;
        if (army >= 8u || session->m_players[army].m_dpnid == 0u) return;
    }

    const unsigned int mode = ScriptExecSubobject()->m_mapType;
    iRPGObj* weapon = 0;
    if (mode == 3u || mode == 4u)
    {
        iInvServer* const inventory = m_refLegs.m_object->GetInvServer();
        weapon = const_cast<iRPGObj*>(inventory->GetBestWeapon(true));
    }
    else if (mode == 2u || mode == 7u)
    {
        iInvServer* const inventory = m_refLegs.m_object->GetInvServer();
        weapon = const_cast<iRPGObj*>(inventory->GetBestWeapon(false));
    }
    else
    {
        weapon = m_refLegs.m_object->GetCurWpn();
    }

    const unsigned int sex = static_cast<unsigned int>(m_refBody.m_object->GetSkillValue(RPGHERO_SEX));
    unsigned int armorType = 0u;
    unsigned int weaponType = 0u;
    iRPGObj* const currentArmor = const_cast<iRPGObj*>(m_refLegs.m_object->GetArmor());
    if (currentArmor != 0)
    {
        iInvKeeper* const keeper = static_cast<iInvKeeper*>(m_refLegs.m_object->QueryInterface(1));
        if (keeper->CanHoldObj(currentArmor))
        {
            armorType = static_cast<unsigned int>(currentArmor->GetSkillValue(RPGARMOR_ANIMTYPE));
            if (static_cast<int>(armorType)<0 || armorType>3u)
            {
                armorType = 0u;
                const char* const weaponName = weapon->GetName();
                iLogConsole* const liveLogger = ::gpLogCons;
                (void)liveLogger->LogStr(LT_INPTERR, "Invalid ArmorType %i for weapon %s", 0u, weaponName);
            }
        }
    }

    if (weapon != 0)
    {
        weaponType = static_cast<unsigned int>(weapon->GetSkillValue(RPGWEAPON_TRUNKANIM));
        if (static_cast<int>(weaponType)<0 || weaponType>9u)
        {
            weaponType = 0u;
            const char* const weaponName = weapon->GetName();
            iLogConsole* const liveLogger = ::gpLogCons;
            (void)liveLogger->LogStr(LT_INPTERR, "Invalid WeaponType %i for weapon %s", 0u, weaponName);
        }
    }

    m_iArmy = static_cast<int>(m_playerIndex);
    const int speed = m_refBody.m_object->GetSkillValue(RPGHERO_SPEED);
    iRPGObj* const renderController = m_refBody.m_object;
    zScriptExec* const scriptExec = ScriptExecSubobject();
    iVidCopier* const vidCopier = static_cast<iVidCopier*>(scriptExec);
    const int maxHp = renderController->GetSkillValue(RPGHERO_MAXHP);
    (void)vidCopier->SetLegsVid(
        static_cast<int>(sex), static_cast<int>(armorType), maxHp, speed);

    SPRITE* active = playerInterface->Flagman();
    if (active != 0)
    {
        active = playerInterface->Flagman();
        VID* const definition = active->Vid();
        if (definition->spriteClass == 7u)
        {
            VID* const nested = definition->linkVid;
            if (nested != 0)
            {
                WEAPON_RUNTIME* const flagsTable = nested->weapon;
                if ((flagsTable->Property&0x800u) != 0u)
                {
                    active = playerInterface->Flagman();
                    EX_SPRITE_DATA* data = active->ExtraData();
                    if (data != 0)
                    {
                        active = playerInterface->Flagman();
                        data = active->ExtraData();
                        const float value = static_cast<zVidCopier*>(scriptExec)->GetEngineSpeed(speed);
                        data->maxSpeedOverride = value;
                    }
                }
            }
        }
    }

    (void)vidCopier->SetBodyVid(
        static_cast<int>(sex), static_cast<int>(weaponType), static_cast<int>(armorType));
    iRPGObj* const headController = m_refBody.m_object;
    const int headVid = headController->GetSkillValue(RPGHERO_VIDHEAD);
    (void)vidCopier->SetHeadVid(
        static_cast<int>(sex), headVid, static_cast<int>(armorType));

    if (vidCopier->SetWeaponProperties(weapon))
    {
        if (weapon == 0)
        {
            const unsigned int army = m_playerIndex;
            iLogConsole* const logger = ::gpLogCons;
            (void)logger->LogStr(LT_INFO, "Wpn's prs:army=%d. no weapon.", army);
        }
        else
        {
            const char* heroName = m_refBody.m_object->GetName();
            if (!heroName) heroName = StringGlobals::EmptyText();
            const unsigned int aimRange = static_cast<unsigned int>(weapon->GetSkillValue(RPGWEAPON_AIMRANGE));
            const unsigned int reloadFull = static_cast<unsigned int>(weapon->GetSkillValue(RPGWEAPON_RELOADFULL));
            const unsigned int reload = static_cast<unsigned int>(weapon->GetSkillValue(RPGWEAPON_RELOAD));
            const unsigned int damageRadius = static_cast<unsigned int>(weapon->GetSkillValue(RPGWEAPON_DAMAGERADIUS));
            const unsigned int damageMax = static_cast<unsigned int>(weapon->GetSkillValue(RPGWEAPON_DAMAGEMAX));
            const unsigned int damageMin = static_cast<unsigned int>(weapon->GetSkillValue(RPGWEAPON_DAMAGEMIN));
            const char* const weaponName = weapon->GetName();
            iLogConsole* const liveLogger = ::gpLogCons;
            const unsigned int army = m_playerIndex;
            (void)liveLogger->LogStr(LT_INFO, "%s: Wpn's prs:army=%d. %s. dmin=%d,dmax=%d,drad=%d,rld=%d,rldf=%d,arng=%d", heroName, army, weaponName, damageMin, damageMax, damageRadius, reload, reloadFull, aimRange);
        }
    }

    iRPGObj* const colorController = m_refBody.m_object;
    const int heroColor = colorController->GetSkillValue(RPGHERO_COLOR);
    (void)vidCopier->SetHeroColor(heroColor);
}

void PLAYER_ARCADE::CreateAll()
{
    zScriptExec* const scriptExec = ScriptExecSubobject();
    iPlayer* const playerInterface = PlayerInterface();
    (void)static_cast<zVidCopier*>(scriptExec)->ReadColors();

    zRPGFrameWork* framework = static_cast<zRPGFrameWork*>(zScriptExec::m_pRPGFrameWork);
    if (!framework)
    {
        GetNew_zRPGFrameWork(&zScriptExec::m_pRPGFrameWork);
        framework = static_cast<zRPGFrameWork*>(zScriptExec::m_pRPGFrameWork);
        framework->SetPlayer(playerInterface);

        iIniParser* const iniParser = ::g_pParser;
        iniParser->SetGlobalFrameWork(
            zScriptExec::m_pRPGFrameWork);
        ::g_pParser->SetReadFrameWork(
            zScriptExec::m_pRPGFrameWork);

        if (iRPGImplant::GetImplantSeed() != 0)
            DebugAssert(5u, "iRPGImplant::GetImplantSeed() == 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 182u);

        const unsigned int implantSeed = RPGImplantMaskedRandom30Bit();
        (void)iRPGImplant::SeedImplantRand(BitsToSigned32(implantSeed));

        static const char* const kRpgTextFiles[] = {
            "text\\skills_text.txt",
            "Weapon.cfg",
            "text\\ranks.txt",
            "text\\Heroes.txt",
            "text\\Weapon.txt",
            "text\\Skils.txt",
            "text\\MouseTips.txt"
        };
        for (unsigned int fileIndex = 0;fileIndex<sizeof(kRpgTextFiles)/sizeof(kRpgTextFiles[0]);++fileIndex)
        {
            (void)::g_pParser->AddIniFile(kRpgTextFiles[fileIndex]);
        }
        (void)iRPGImplant::SeedImplantRand(0);
    }
    else
    {
        framework->ResetScriptFunctions();
    }

    framework = static_cast<zRPGFrameWork*>(zScriptExec::m_pRPGFrameWork);
    iRPGObj* heroObject = framework->FirstObject(34, 0, 0);
    while (heroObject)
    {
        if (heroObject->GetGameObj() != 0)
            DebugAssert(5u, "hero->GetType() == TYPE_HERO", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 202u);

        iRPGHero* const heroInterface = static_cast<iRPGHero*>(heroObject->QueryInterface(0));
        heroInterface->SetInvulnerable(false);
        framework = static_cast<zRPGFrameWork*>(zScriptExec::m_pRPGFrameWork);
        heroObject = framework->NextObject();
    }
    scriptExec->ProfMngr()->m_pRPGFrameWork =
        zScriptExec::m_pRPGFrameWork;

    zQuestServer* questServer = static_cast<zQuestServer*>(zScriptExec::m_pQuestServer);
    if (!questServer)
    {
        GetNew_zQuestServer(&zScriptExec::m_pQuestServer);
        questServer = static_cast<zQuestServer*>(zScriptExec::m_pQuestServer);
        iRPGFrameWork* const questInterface =
            static_cast<iRPGFrameWork*>(questServer->QueryInterface(23));
        questInterface->SetPlayer(playerInterface);

        iIniParser* const iniParser = ::g_pParser;
        iniParser->SetGlobalFrameWork(
            zScriptExec::m_pRPGFrameWork);
        ::g_pParser->SetReadFrameWork(questInterface);
        (void)::g_pParser->AddIniFile("levels.cfg");
        (void)::g_pParser->AddIniFile("text\\mission.txt");
        (void)::g_pParser->AddIniFile("Survive.cfg");
        (void)::g_pParser->AddIniFile("campaign.cfg");

        iItemTaker* const itemTaker = m_itemTaker;
        questServer = static_cast<zQuestServer*>(zScriptExec::m_pQuestServer);
        itemTaker->SetQuestServer(static_cast<iQuestServer*>(questServer));
    }

    if (m_playerIndex == 0u)
    {
        (void)zScriptExec::m_pRPGFrameWork->CheckHashesDebug();
        questServer = static_cast<zQuestServer*>(zScriptExec::m_pQuestServer);
        (void)static_cast<iRPGFrameWork*>(questServer->QueryInterface(23))->CheckHashesDebug();
    }

    iRPGParamDialog** const paramDialogSlot = &m_paramDialog;
    if (!*paramDialogSlot)
        GetNew_zRPGParamDialog(paramDialogSlot);
    else
        (*paramDialogSlot)->ResetScriptFunctions();
    (*paramDialogSlot)->SetRPGFrameWork(
        zScriptExec::m_pRPGFrameWork);

    iShopItemInfo* shopInfo = zScriptExec::m_pShopItemInfo;
    if (!shopInfo)
    {
        GetNew_zShopItemInfo(&zScriptExec::m_pShopItemInfo);
        shopInfo = zScriptExec::m_pShopItemInfo;
        shopInfo->SetRPGFrameWork(
            zScriptExec::m_pRPGFrameWork);

        GRAPH* const graph = Graph;
        shopInfo = zScriptExec::m_pShopItemInfo;
        const float screenX = graph->screenWidth*(0.5f)+(5000.0f);
        const float screenY = graph->screenHeight*(0.5f)+(5000.0f);
        shopInfo->SetScreenCoordinates(screenX, screenY);
    }

    iHeroInvMenu* inventoryMenu = zScriptExec::m_pHeroInvMenu;
    if (!inventoryMenu)
    {
        GetNew_zHeroInvMenu(&zScriptExec::m_pHeroInvMenu,
            zScriptExec::m_pRPGFrameWork);
        inventoryMenu = zScriptExec::m_pHeroInvMenu;
        inventoryMenu->SetItemTaker(m_itemTaker);
    }
    else
    {
        inventoryMenu->ReloadAll();
    }

    zParamDlgMenu* paramMenu = static_cast<zParamDlgMenu*>(zScriptExec::m_pParamDlgMenu);
    if (!paramMenu)
    {
        if (playerInterface->IsCurPlayer())
        {
            GetNew_zParamDlgMenu(&zScriptExec::m_pParamDlgMenu, *paramDialogSlot,
                zScriptExec::m_pHeroInvMenu);
            paramMenu = static_cast<zParamDlgMenu*>(zScriptExec::m_pParamDlgMenu);
            paramMenu->SetRPGFrameWork(
                zScriptExec::m_pRPGFrameWork);
        }
    }
    else
    {
        paramMenu->ReloadAll();
    }
    paramMenu = static_cast<zParamDlgMenu*>(zScriptExec::m_pParamDlgMenu);
    if (paramMenu && playerInterface->IsCurPlayer())
        zScriptExec::m_pParamDlgMenu->SetRPGParamDialog(*paramDialogSlot);

    iShopDlgMenu* shopMenu = zScriptExec::m_pShopDlgMenu;
    if (!shopMenu)
    {
        GetNew_zShopDlgMenu(
            &zScriptExec::m_pShopDlgMenu);
        shopMenu = zScriptExec::m_pShopDlgMenu;
    }
    else
    {
        shopMenu->ReloadAll();
    }
    zScriptExec::m_pShopDlgMenu->SetRPGFrameWork(
        zScriptExec::m_pRPGFrameWork);
    zScriptExec::m_pShopDlgMenu->SetShopItemInfo(
        zScriptExec::m_pShopItemInfo);
    zScriptExec::m_pShopDlgMenu->SetHeroInvMenu(
        zScriptExec::m_pHeroInvMenu);
    if (playerInterface->IsCurPlayer())
        zScriptExec::m_pShopDlgMenu->ReleaseListOfObjects();

    (*paramDialogSlot)->SetPlayeR(playerInterface);
    if (playerInterface->IsCurPlayer())
        zScriptExec::m_pShopDlgMenu->SetPlayer(playerInterface);

    zBriefingDlgMenu* briefingDialog = static_cast<zBriefingDlgMenu*>(m_pBriefingDlgMenu);
    if (!briefingDialog)
    {
        GetNew_zBriefingDlgMenu(&m_pBriefingDlgMenu);
        briefingDialog = static_cast<zBriefingDlgMenu*>(m_pBriefingDlgMenu);
        questServer = static_cast<zQuestServer*>(zScriptExec::m_pQuestServer);
        iRPGFrameWork* const questInterface =
            static_cast<iRPGFrameWork*>(questServer->QueryInterface(23));
        m_pBriefingDlgMenu->SetRPGFrameWork(questInterface);
    }
    else
    {
        briefingDialog->ReloadAll();
    }

    CheckRPGHeroChange(true);
    iItemTaker* const itemTaker = m_itemTaker;
    if (itemTaker)
    {
        itemTaker->SetRPGFrameWork(
            zScriptExec::m_pRPGFrameWork);
        m_itemTaker->SetPlayer(playerInterface);
    }
}

void PLAYER_ARCADE::InitForLoadMap()
{
    g_iHeroIdForInvuln = -25;
    m_loadHeroFromMaxSave = 0u;
    iRPGImplant::generationSequence = 0;
    zCarControl::ResetScriptFunctions();
    m_iDeHypnMonsters = 0;


    PTR_SPRITE* const firstHypnotized = m_hypnSprites.begin;
    PlayerArcadeDestroyEntryRange(firstHypnotized, m_hypnSprites.end);
    m_hypnSprites.end = firstHypnotized;

    m_hypnTimes.end = m_hypnTimes.begin;

    zSurvive::ResetMonst2Create();

    if (Map->m_script.m_bytecode != 0)
    {
        if (Const->DebugMode != 0)
        {
            STRING message("Load cfg-files");
            Graph->DrawDebugText(message);
        }
        (void)::g_pProfLdr->ExpFileToReg(true);
        ScriptExecSubobject()->ClearFlyIcons();

        iPlayer* const playerInterface = PlayerInterface();
        if (playerInterface->IsCurPlayer())
        {
            for (unsigned int vidIndex = 0; vidIndex < 4096u; ++vidIndex)
                g_abMonsterVid[vidIndex] = false;

            const int functionId = static_cast<int>(playerInterface->ScriptGetNFunction("GetMonsterVidList"));
            if (functionId>0)
            {
                const int count = static_cast<int>(playerInterface->ScriptRun(functionId, "%d", -1, 0, 0));
                for (int index = 0;index<count;++index)
                {
                    const int vidNumber = static_cast<int>(playerInterface->ScriptRun(functionId, "%d", index, 0, 0));
                    if (vidNumber >= 0 && vidNumber<4096)
                        g_abMonsterVid[vidNumber] = true;
                    else
                        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 455u);
                }
            }
        }
        CreateAll();
        if (m_playerIndex == 0u)
        {
            m_itemTaker->DeleteOldItems();
            m_itemTaker->CreateIdsForSprites();
        }

        int* const namedMonsterCounts = g_aiNamedMonstersLast;
        if (namedMonsterCounts)
        {
            for (unsigned int index = 0u;index<10u;++index)
                g_aiNamedMonstersLast[index] = BitsToSigned32(Map->m_spriteCountsByNameNumber[index]);
        }

        if (Const->DebugMode != 0)
        {
            STRING message("Finish loading of cfg-files");
            Graph->DrawDebugText(message);
        }
    }
}

void PLAYER_ARCADE::Control(INPUT* input)
{
    MAP* const game = Map;
    if (game->m_script.m_bytecode == 0) return;

    NET* session = ::Net;
    iPlayer* const playerInterface = PlayerInterface();
    if ((session->m_flags&8u) == 0u)
    {
        if (playerInterface->IsCurPlayer() == 0u) return;
        session = ::Net;
    }
    if ((session->m_flags&8u) != 0u)
    {
        const unsigned int army = m_playerIndex;
        if (army >= 8u || session->m_players[army].m_dpnid == 0u) return;
    }
    if (static_cast<unsigned int>(m_controlMode) == 0u) return;

    if (input->GetAction(IA_SCREENSHOT) != 0u && playerInterface->IsCurPlayer() != 0u)
        MainSwCreateCurScreenShot();

    unsigned int mode = ScriptExecSubobject()->m_mapType;
    if (mode == 0u) return;
    zScriptExec* const scriptExec = ScriptExecSubobject();
    if (mode == 1u) scriptExec->CheckRPGHeroChange(false);
    mode = scriptExec->m_mapType;

    if (mode == 2u || mode == 7u)
    {
        iParamDlgMenu* const modeUi = zScriptExec::m_pParamDlgMenu;
        modeUi->ControlPlayerMenu(input, mode == 7u);
        mode = scriptExec->m_mapType;
        if (mode == 2u) zScriptExec::m_pParamDlgMenu->ControlHeroScreen(input->ClientMouseXY().x);
        else if (mode == 7u) zScriptExec::m_pParamDlgMenu->ControlPerkScreen();
        scriptExec->CheckRPGHeroChange(false);
    }
    else if (mode == 6u)
    {
        if (playerInterface->IsCurPlayer() != 0u)
        {
            iBriefingDlgMenu* const ui = m_pBriefingDlgMenu;
            ui->Control(input);
        }
    }
    else if (mode == 3u || mode == 4u)
    {
        if (playerInterface->IsCurPlayer() != 0u)
        {
            if (input->GetAction(IM_LBUTTONCLICK) != 0u ||
               input->GetAction(IM_RBUTTONCLICK) != 0u)
            {


                zString* const selector = &ScriptExecSubobject()->m_debugText;
                if (!selector->IsEmpty())
                    *selector = "";
            }
            iShopDlgMenu* const shopUi = zScriptExec::m_pShopDlgMenu;
            shopUi->AsBaseDlgMenu()->Control(input);
            iHeroInvMenu* const heroUi = zScriptExec::m_pHeroInvMenu;
            heroUi->AsBaseDlgMenu()->Control(input);
            iShopItemInfo* const itemUi = zScriptExec::m_pShopItemInfo;
            itemUi->Control(input);
        }
    }
    else if (mode == 5u)
    {
        DeHypnMonsters();
        if (playerInterface->IsCurPlayer() != 0u)
        {
            iHeroInvMenu* const ui = zScriptExec::m_pHeroInvMenu;
            ui->AsBaseDlgMenu()->Control(input);
        }

        if (m_controlPauseState == 0u)
        {
            iRPGHero* hero = GetPlayerHero();
            if (hero != 0) hero->Control(input);

            const unsigned int now = ::CurrentTime;
            const unsigned int lastInventoryTick = m_lastInventoryTick;
            bool update = (lastInventoryTick == 0u);
            if (lastInventoryTick != 0u && now-lastInventoryTick>200u)
            {
                m_lastInventoryTick = now;
                update = (now != 0u);
            }
            if (update)
            {
                hero = GetPlayerHero();
                if (hero != 0)
                {
                    iInvServer* const inventory = hero->GetInvServer();
                    if (inventory != 0)
                        GetPlayerHero()->GetInvServer()->ControlGrndInventory(::CurrentTime);
                }
            }

            SPRITE* active = playerInterface->Flagman();
            if (active != 0)
            {
                active = playerInterface->Flagman();
                ScriptExecSubobject()->m_reloadBarDirection = static_cast<unsigned int>(static_cast<MAN*>(active)->GetReloadBarDir());
            }

            active = 0;
            if (static_cast<unsigned int>(playerInterface->GetArcadeMod_()) == 0u && zScriptExec::m_pQuestServer != 0u &&
               (active = playerInterface->Flagman()) != 0u)
            {
                const unsigned int lastQuestTick = m_lastQuestCoordTick;
                bool updateQuestCoordinates = (lastQuestTick == 0u);
                if (lastQuestTick != 0u && ::CurrentTime-lastQuestTick>100u)
                {
                    m_lastQuestCoordTick = ::CurrentTime;
                    updateQuestCoordinates = (::CurrentTime != 0u);
                }
                if (updateQuestCoordinates)
                {
                    active = playerInterface->Flagman();
                    iQuestServer* const questServer = zScriptExec::m_pQuestServer;
                    const SPRITE* const activeSprite = active;
                    const int x = TruncateFloatToInt32(activeSprite->X());
                    const int y = TruncateFloatToInt32(activeSprite->Y());
                    questServer->CheckFlagmanCoord(x, y);
                }
            }

            if (static_cast<unsigned int>(playerInterface->GetArcadeMod_()) == 0u && zScriptExec::m_pQuestServer != 0u &&
               g_aiNamedMonstersLast != 0)
            {
                for (unsigned int monsterIndex = 0;monsterIndex<10u;++monsterIndex)
                {
                    int* const namedMonsterCounts = g_aiNamedMonstersLast;
                    const unsigned int monsterCount = Map->m_spriteCountsByNameNumber[monsterIndex];
                    if (static_cast<unsigned int>(namedMonsterCounts[monsterIndex]) != monsterCount)
                    {
                        namedMonsterCounts[monsterIndex] = BitsToSigned32(monsterCount);
                        if (monsterCount == 0u)
                        {
                            iLogConsole* const logger = ::gpLogCons;
                            (void)logger->LogStr(LT_NOTICE, "QS. NamedMonsters %d death", monsterIndex);
                            iQuestServer* const questServer = zScriptExec::m_pQuestServer;
                            questServer->CheckNamedMonsters(static_cast<int>(monsterIndex));
                        }
                    }
                }
            }
        }
    }

    iRPGObj* const heroObject = m_refBody.m_object;
    if (heroObject != 0)
    {
        if (heroObject->GetDirty())
        {
            ChangeParameters();
            m_parametersDirty = 1u;
        }
        else if (m_refBody.m_object->GetAmmoDirty()) m_parametersDirty = 1u;
    }

    PlayerSubobject()->GroupsTact();
    m_oMessage.Get().Tact();

    if (((Map->m_flags&0x2000u) != 0u || g_iZmDebug != 0u) &&
       playerInterface->IsCurPlayer() != 0u && input->GetAction(IA_SHOW_DEBUG_CONSOLE) != 0u)
    {
        iLogConsole* const logger = ::gpLogCons;
        logger->ToggleShow();
    }
    {
        iLogConsole* const logger = ::gpLogCons;
        logger->Tact();
    }

    SPRITE* active = playerInterface->Flagman();
    if (active == 0u) return;
    active = playerInterface->Flagman();
    VID* definition = active->Vid();
    if (definition->spriteClass != 7u) return;

    active = playerInterface->Flagman();
    static_cast<MAN*>(active)->RecalcCurAmmoClip(false);
    if (input->GetAction(IA_RELOAD_WPN) != 0u && m_controlPauseState == 0u)
    {
        active = playerInterface->Flagman();
        static_cast<MAN*>(active)->ReloadAmmo();
    }

    if (input->GetAction(IA_ATTACK_COORD) != 0u && m_refLegs.m_object != 0 && m_controlPauseState == 0u)
    {
        active = playerInterface->Flagman();
        definition = active->Vid();
        const unsigned int classId = static_cast<unsigned int>(definition->nVid);
        if (classId == 1203u)
        {
            if (m_prevInventoryAction == 0u)
            {
                active = playerInterface->Flagman();
                if (static_cast<MAN*>(active)->GetAmmoCnt() == 0)
                    playerInterface->PlaySFx(121, false);
            }
        }
        else
        {
            iRPGHero* const hero = GetPlayerHero();
            if (hero->CheckIfNoAmmo() && m_refBody.m_object->GetDirty())
            {
                ChangeParameters();
                m_parametersDirty = 1u;
            }
        }
    }

    if (ScriptExecSubobject()->m_flagmanControlEnabled != 0u && m_controlPauseState == 0u)
    {
        active = playerInterface->Flagman();
        definition = active->Vid();
        const unsigned int classId = static_cast<unsigned int>(definition->nVid);
        if (classId != 1206u && classId != 3000u)
        {
            m_controlMode = classId == 1200u?4:2;
            PlayerSubobject()->ControlFlagman(input);
        }
        else
        {
            const unsigned int savedControlMode = static_cast<unsigned int>(m_controlMode);
            m_controlMode = 3;
            PlayerSubobject()->ControlFlagman(input);
            m_controlMode = static_cast<int>(savedControlMode);
            SPRITE* const carSprite = playerInterface->Flagman();
            active = playerInterface->Flagman();
            static_cast<zCarControl*>(static_cast<MAN*>(active))->ControlVehicle(
                input, carSprite);
            DrawCarDebug();
        }
    }
    else
    {
        active = playerInterface->Flagman();
        active->Stop();
    }

    SPRITE* carToStop = m_pCarToStop;
    if (carToStop != 0)
    {
        definition = carToStop->Vid();
        if (definition->spriteClass != 7u)
            DebugAssert(5u, "m_pCarToStop->IsSpriteClass( B_MAN )", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\player_arcade.cpp", 711u);
        carToStop = m_pCarToStop;
        static_cast<zCarControl*>(static_cast<MAN*>(carToStop))->ControlVehicle(
            0, carToStop);
        if (scriptExec->GetCarSpeed(m_pCarToStop) == 0)
        {
            static_cast<zCarControl*>(static_cast<MAN*>(m_pCarToStop))->StopCarControl();
            m_pCarToStop->ChangeAnimation(10);
            m_pCarToStop = 0;
        }
    }

    if (m_ctrlWheelsCar != 0 && playerInterface->IsCurPlayer() != 0u)
        zCarControl::ControlWheels(m_ctrlWheelsCar, 0, false, false);

    m_prevInventoryAction = input->GetAction(IM_LBUTTONCLICK);
}

float MAP::ToScreenX(float value) const
{
    return value-m_shiftXY.x;
}


