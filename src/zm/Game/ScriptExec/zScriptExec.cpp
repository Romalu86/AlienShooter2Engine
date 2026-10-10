#include "list.h"
#include "../../../engine_globals.h"
#include "basetype.h"
#include "zm/RPGHero/zRPGArmor.h"
#include "exports.h"
int __cdecl SFLB_GetHeroId(int value);

#include "zScriptExec.h"
#include "zm/Menu/zShopDlgMenu.h"
#include "zm/Menu/zParamDlgMenu.h"
#include "zm/Game/ScriptExec/ScriptExecExternalCommand.h"
#include "mylib.h"
#include "zm/Input/zInputMap.h"
#include "myerror.h"
#include "sprite_collector.h"
#include "sprite.h"
#include "vid.h"
#include "zm/Game/zVidCopier.h"
#include "zm/Utils/zLogConsole.h"
#include "zm/Utils/zRefObject.h"
#include "zm/Utils/zStaticStorage.h"
#include "zm/Utils/zTreeOperations.h"
#include "zm/Eng/zProfLdr/zProfLdr.h"
#include "zm/Eng/zProfLdr/zProfMngr.h"
#include "zm/Game/RecordDB/zRecordDB.h"
#include "zm/Survive/zSurvive.h"
#include "zm/Quest/zQuestServer.h"
#include "zm/RPGHero/zRPGQuest.h"
#include "zm/Game/Campaign/zCampaign.h"
#include "zm/Utils/String/zString.h"
#include "zm/IniParser/zIniParser.h"
#include "zm/Inventory/zInventory.h"
#include "zm/Inventory/zInventoryItem.h"
#include "zm/RPGHero/zRPGWeapon.h"
#include "zm/RPGHero/zRPGAmmo.h"
#include <new>

int __cdecl SFLB_GetHeroId(int value)
{
    return AddSigned32Wrap(MultiplySigned32Wrap(value, 3), 25);
}


void* zScriptExec::QueryInterface(int interfaceId) const
{
    if (static_cast<unsigned int>(interfaceId) == 37u)
        return const_cast<zScriptExec*>(this);
    DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 101u);
    return 0;
}
#include "graph.h"
#include "map.h"
#include "net.h"
#include "player.h"
#include "dialogSrv.h"
#include "constant.h"
#include "zm/Menu/zHeroInvMenu.h"
namespace {
static int vidReduceScale[4096];
static char hotKeyCharacter[2];
static int cachedDamageMinimum;
static int cachedDamageMaximum;
static unsigned char damageRangeInitialized;
StaticObjectStorage<iRef<iSurvive> > surviveReference;
StaticObjectStorage<iRef<iCampaign> > campaignReference;
unsigned char destroyedScriptReferenceFlags = 0;
}

iRef<iSurvive>& zScriptExec::m_pSurvive =
    *reinterpret_cast<iRef<iSurvive>*>(&surviveReference.memory);
iRef<iCampaign>& zScriptExec::m_pCampaign =
    *reinterpret_cast<iRef<iCampaign>*>(&campaignReference.memory);

unsigned int __cdecl zScriptExec::RegisterSurviveReference()
{
    ::new (&m_pSurvive) iRef<iSurvive>;
    return RegisterCrtAtExit(&zScriptExec::DestroySurviveReference);
}

unsigned int __cdecl zScriptExec::RegisterCampaignReference()
{
    ::new (&m_pCampaign) iRef<iCampaign>;
    return RegisterCrtAtExit(&zScriptExec::DestroyCampaignReference);
}

void __cdecl zScriptExec::DestroySurviveReference()
{
    if (destroyedScriptReferenceFlags&1u) return;
    destroyedScriptReferenceFlags =
        static_cast<unsigned char>(destroyedScriptReferenceFlags|1u);
    m_pSurvive.~iRef<iSurvive>();
}

void __cdecl zScriptExec::DestroyCampaignReference()
{
    if (destroyedScriptReferenceFlags&2u) return;
    destroyedScriptReferenceFlags =
        static_cast<unsigned char>(destroyedScriptReferenceFlags|2u);
    m_pCampaign.~iRef<iCampaign>();
}

iRPGFrameWork* zScriptExec::m_pRPGFrameWork = 0;
iQuestServer* zScriptExec::m_pQuestServer = 0;
iShopDlgMenu* zScriptExec::m_pShopDlgMenu = 0;
iShopItemInfo* zScriptExec::m_pShopItemInfo = 0;
iHeroInvMenu* zScriptExec::m_pHeroInvMenu = 0;
iParamDlgMenu* zScriptExec::m_pParamDlgMenu = 0;

zScriptExec::~zScriptExec()
{
    iIniParser* parser = ::g_pParser;
    if (parser)
    {
        (void)parser->iRelease();
        ::g_pParser = 0;
    }
}

zScriptExec::zScriptExec()
    : zVidCopier(), m_modeFlags(0), m_loadHeroFromMaxSave(0),
      m_itemTaker(0), m_curAmmoDir(0), m_reloadBarDirection(0),
      m_flagmanControlEnabled(1), m_messageAvailableTick(0), m_messageId(0),
      m_mapType(0), m_radioIconDirection(0), m_radioIconVid(0),
      m_radioFontVid(0), m_radioMessageDirty(0), m_ctrlWheelsCar(0),
      m_lastFlyIconTick(0), m_paramDialog(0), m_showSurvivalHero(0)
{
    if (::g_pProfLdr == nullptr)
    {
        void* loader = CrtNew(sizeof(zProfLdr));
        if (loader)
        {
            try
            {
                loader = new (loader) zProfLdr();
            }
            catch (...)
            {
                CrtFreePointer(loader);
                throw;
            }
        }
        ::g_pProfLdr = static_cast<zProfLdr*>(loader);
    }

    int* const idTable = vidReduceScale;
    for (unsigned int i = 0u;i<4096u;++i)idTable[i] = 0u;

    bool* const enabledMonsterVids = g_abMonsterVid;
    if (enabledMonsterVids)
        for (unsigned int i = 0u;i<4096u;++i) enabledMonsterVids[i] = false;

    if (::g_pParser == nullptr)
    {
        iIniParser* parser;
        GetNew_zIniParser(&parser);
        ::g_pParser = parser;
    }

    if (RecordDatabase == nullptr)
    {
        void* recordDb = CrtNew(sizeof(zRecordDB));
        if (recordDb)
        {
            try
            {
                recordDb = new (recordDb) zRecordDB();
            }
            catch (...)
            {
                CrtFreePointer(recordDb);
                throw;
            }
        }
        RecordDatabase = static_cast<zRecordDB*>(recordDb);
    }
}

void zScriptExec::SetCtrlWheelsCar(SPRITE* sprite)
{
    m_ctrlWheelsCar = sprite;
}

int zScriptExec::GetArcadeMod_() const
{
    return static_cast<int>(m_modeFlags);
}


unsigned int __cdecl ScriptexecZscriptexec_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

zScriptExecIntVector::zScriptExecIntVector() noexcept
{
    begin = end = capacityEnd = 0;
}

zScriptExecIntVector::~zScriptExecIntVector() noexcept(false)
{
    CrtFreePointer(begin);
    begin = end = capacityEnd = 0;
}

zScriptExecByteVector::zScriptExecByteVector() noexcept
    : begin(0), end(0), capacityEnd(0)
{
}

zScriptExecByteVector::~zScriptExecByteVector() noexcept(false)
{
    CrtFreePointer(begin);
    begin = end = capacityEnd = 0;
}

AS_RECORD::AS_RECORD() noexcept
    : m_totalScore(0), m_monsterKills(0), m_elapsedSeconds(0), m_checksum(0), m_cheater(0)
{
    m_playerName.data = m_recordTimestamp.data = 0;
    m_playerName.size = m_playerName.capacity = 0;
    m_recordTimestamp.size = m_recordTimestamp.capacity = 0;
    m_weaponScores.begin = m_weaponScores.end = m_weaponScores.capacityEnd = 0;
    m_weaponKills.begin = m_weaponKills.end = m_weaponKills.capacityEnd = 0;
}

void zScriptExec::SetDeathMatchMode(bool enabled)
{
    m_modeFlags = enabled?(m_modeFlags|2u):(m_modeFlags&~2u);
}

iRPGHero* zScriptExec::GetHeroForShow()
{
    if (m_showSurvivalHero)
    {
        iRPGObj* object = m_refHead.m_object;
        if (object) return static_cast<iRPGHero*>(object->QueryInterface(0));
    }
    return m_refLegs.m_object;
}


void zScriptExec::SwitchHeroCS(bool enabled)
{
    m_showSurvivalHero = static_cast<unsigned char>(enabled);
    if (enabled) CreateContSurvHero();
}





void zScriptExec::ReleaseArmyHero()
{
    ProfMngr()->ReleaseArmyHero();
}


void  Tidy(void* self, unsigned char releasePayload)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);

    if (releasePayload)ReleaseByteRefPayload(PtrToU32(storage.data));
    storage.data = 0;
    storage.size = 0u;
    storage.capacity = 0u;
}

ScriptExecMapNode* ScriptExecMapControl::_Nil = 0;
unsigned int ScriptExecMapControl::_Nilrefs = 0;

static void EraseScriptExecBranch(ScriptExecMapNode* node)
{
    ScriptExecMapNode* current = static_cast<ScriptExecMapNode*>(node);
    while (current != ScriptExecMapControl::_Nil)
    {
        EraseScriptExecBranch(current->right);
        ScriptExecMapNode* const left = current->left;
        CrtFreePointer(current);
        current = left;
    }
}


int zScriptExec::DirFromRPGObj(const iRPGObj* source)
{
    if (!source)return -1;
    const unsigned int key = static_cast<unsigned int>(source->GetVidNum(1));
    const int divisor = GetVidDirectionsNum(static_cast<int>(key));
    if (divisor == 0)return -1;
    const unsigned int raw = static_cast<unsigned int>(source->GetDirNum(1));
    const int numerator = static_cast<int>(raw<<8);
    return (numerator/divisor)&0xFF;
}

static void PreviousScriptExecNode(ScriptExecMapNode*& current)
{
    ScriptExecMapNode* node = current;
    if (node->color == 0u && node->parent->parent == node)
    {
        current = node->right;
        return;
    }
    ScriptExecMapNode* const nil = ScriptExecMapControl::_Nil;
    if (node->left != nil)
    {
        node = node->left;
        while (node->right != nil)node = node->right;
        current = node;
        return;
    }
    ScriptExecMapNode* parent = node->parent;
    while (current == parent->left)
    {
        current = parent;
        parent = parent->parent;
    }
    current = parent;
}

static void NextScriptExecNode(ScriptExecMapNode*& current)
{
    ScriptExecMapNode* node = current;
    ScriptExecMapNode* const nil = ScriptExecMapControl::_Nil;
    if (node->right != nil)
    {
        node = node->right;
        while (node->left != nil)node = node->left;
        current = node;
        return;
    }
    ScriptExecMapNode* parent = node->parent;
    while (current == parent->right)
    {
        current = parent;
        parent = parent->parent;
    }
    if (current->right != parent)current = parent;
}

ScriptExecMapControl::ScriptExecMapControl()
{
    policyState[0] = 0;
    ScriptExecMapNode* candidate = static_cast<ScriptExecMapNode*>(CrtNew(sizeof(ScriptExecMapNode)));
    candidate->parent = 0;
    candidate->color = 1;
    unsigned int staticLock = 0;
    Msvc6Runtime::StaticLockConstruct(&staticLock);
    if (!_Nil)
    {
        _Nil = candidate;
        candidate->left = 0;
        _Nil->right = 0;
        candidate = 0;
    }
    ++_Nilrefs;
    Msvc6Runtime::StaticLockDestruct(&staticLock);
    if (candidate) CrtFreePointer(candidate);

    ScriptExecMapNode* const sentinel = _Nil;
    ScriptExecMapNode* const header = static_cast<ScriptExecMapNode*>(CrtNew(sizeof(ScriptExecMapNode)));
    header->parent = sentinel;
    header->color = 0;
    head = header;
    size = 0;
    header->left = header;
    head->right = head;
}

static void ReleaseRefCountedScriptBuffer(AS_RECORD_STRING_CONTROL& storage)
{
    char* const payload = storage.data;
    if (payload)
    {
        unsigned char* const referenceCountByte = reinterpret_cast<unsigned char*>(payload)-1;
        const unsigned char referenceCount = *referenceCountByte;
        if (referenceCount == 0u || referenceCount == 0xFFu)CrtFreePointer(referenceCountByte);
        else *referenceCountByte = static_cast<unsigned char>(referenceCount-1u);
    }
    storage.data = 0;
    storage.size = 0u;
    storage.capacity = 0u;
}

AS_RECORD::~AS_RECORD() noexcept(false)
{
    AS_RECORD& record = *this;
    CrtFreePointer(record.m_weaponKills.begin);
    record.m_weaponKills.begin = 0;
    record.m_weaponKills.end = 0;
    record.m_weaponKills.capacityEnd = 0;
    CrtFreePointer(record.m_weaponScores.begin);
    record.m_weaponScores.begin = 0;
    record.m_weaponScores.end = 0;
    record.m_weaponScores.capacityEnd = 0;
    ReleaseRefCountedScriptBuffer(record.m_recordTimestamp);
    ReleaseRefCountedScriptBuffer(record.m_playerName);
}

void  StdString_CopyStorage(void* self, unsigned int requested)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);
    unsigned int newCapacity = requested|0x1Fu;
    if (newCapacity>0xFFFFFFFDu)newCapacity = requested;
    unsigned char* allocation;
    try
    {
        const unsigned int allocationSize = newCapacity+2u;
        allocation = static_cast<unsigned char*>(CrtNew(
            BitsToSigned32(allocationSize)<0?0u:allocationSize));
    }
    catch (...)
    {
        newCapacity = requested;
        const unsigned int allocationSize = requested+2u;
        allocation = static_cast<unsigned char*>(CrtNew(
            BitsToSigned32(allocationSize)<0?0u:allocationSize));
    }
    const unsigned int copyLength = storage.size<newCapacity?storage.size:newCapacity;
    if (copyLength)CopyBytes(allocation+1u, storage.data, copyLength);

    char* const oldPayload = storage.data;
    const unsigned int oldLength = storage.size;
    if (oldPayload)
    {
        unsigned char* const oldReferenceCountByte = reinterpret_cast<unsigned char*>(oldPayload)-1u;
        const unsigned char references = *oldReferenceCountByte;
        if (references != 0u && references != 0xFFu)
            *oldReferenceCountByte = static_cast<unsigned char>(references-1u);
        else
            CrtFreePointer(oldReferenceCountByte);
    }
    storage.size = 0u;
    storage.data = reinterpret_cast<char*>(allocation+1u);
    allocation[0] = 0u;
    storage.capacity = newCapacity;
    const unsigned int newLength = oldLength<newCapacity?oldLength:newCapacity;
    storage.size = newLength;
    storage.data[newLength] = 0;
}

ScriptExecMapControl::~ScriptExecMapControl() noexcept(false)
{
    ScriptExecMapNode* const finish = head;
    ScriptExecMapNode* first = finish->left;
    if (size != 0)
    {
        ScriptExecMapNode* node = finish->parent;
        while (node != _Nil)
        {
            EraseScriptExecBranch(node->right);
            ScriptExecMapNode* const next = node->left;
            CrtFreePointer(node);
            node = next;
        }
        head->parent = _Nil;
        size = 0;
        head->left = head;
        head->right = head;
    }
    else
    {
        while (first != finish)
        {
            ScriptExecMapNode* const eraseNode = first;
            NextScriptExecNode(first);
            (void)TreeOperations::Erase(head, size, eraseNode, _Nil);
        }
    }
    CrtFreePointer(head);
    head = 0;
    size = 0;
    unsigned int staticLock = 0;
    ScriptExecMapNode* detachedSentinel = 0;
    Msvc6Runtime::StaticLockConstruct(&staticLock);
    if (--_Nilrefs == 0)
    {
        detachedSentinel = _Nil;
        _Nil = 0;
    }
    Msvc6Runtime::StaticLockDestruct(&staticLock);
    if (detachedSentinel) CrtFreePointer(detachedSentinel);
}

unsigned char  Grow(void* self, unsigned int requested, unsigned char releaseShared)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);
    if (requested>0xFFFFFFFDu)
        (void)Msvc6LengthError();

    char* const payload = storage.data;
    const unsigned char references = payload?*(reinterpret_cast<unsigned char*>(payload)-1u):0u;
    if (payload && references != 0u && references != 0xFFu)
    {
        if (requested != 0u)
        {
            StdString_CopyStorage(self, requested);
            return 1u;
        }
        *(reinterpret_cast<unsigned char*>(payload)-1u) = static_cast<unsigned char>(references-1u);
        storage.data = 0;
        storage.size = 0u;
        storage.capacity = 0u;
        return 0u;
    }

    if (requested == 0u)
    {
        if (releaseShared)
            Tidy(self, 1u);
        else if (payload)
        {
            storage.size = 0u;
            storage.data[0] = 0;
        }
        return 0u;
    }

    if (releaseShared)
    {
        const unsigned int capacity = storage.capacity;
        if (capacity <= 31u && capacity >= requested)
            return 1u;
        Tidy(self, 1u);
        StdString_CopyStorage(self, requested);
        return 1u;
    }

    if (storage.capacity<requested)
        StdString_CopyStorage(self, requested);
    return 1u;
}
void zScriptExec::CreateContSurvHero()
{
    zProfLdr* const profileLoader = ::g_pProfLdr;
    const unsigned int currentId = static_cast<unsigned int>(profileLoader->m_currentProfile);
    iRPGObj* const oldObject = m_refHead.m_object;
    if (oldObject && static_cast<unsigned int>(m_weaponState) == currentId)return;
    m_weaponState = static_cast<int>(currentId);
    if (oldObject) (void)m_pRPGFrameWork->ReleaseObject(oldObject);
    m_refHead.Set(m_pRPGFrameWork->GetNewRPGObj(static_cast<OBJ_TYPE>(TYPE_HERO), false));
    m_refHead.m_object->SetClone(1);

    STRING saveFileName;
    {
        STRING savedName = ::g_pProfLdr->GetMaxLevSaveName(static_cast<SAVE_TYPE>(1));
        saveFileName = savedName;
    }
    iIniParser* const sink = ::g_pParser;
    (void)sink->LoadObjFromFile(m_refHead.m_object, saveFileName.m_value);
}

void zScriptExec::ClearFlyIcons()
{
    for (unsigned int i = 0u;i<4u;++i)
    {
        AS_RECORD_DWORD_VECTOR& vector = m_flyIconDwords[i];
        MsvcNoOpDestroyRange(PtrToU32(vector.begin), PtrToU32(vector.end));
        vector.end = vector.begin;
    }

    for (unsigned int i = 4u;i<6u;++i)
    {
        AS_RECORD_DWORD_VECTOR& vector = m_flyIconDwords[i];
        unsigned int* const newEnd = zScriptExec_DwordRangeCopy(
            reinterpret_cast<const unsigned int*>(vector.end),
            reinterpret_cast<const unsigned int*>(vector.end),
            reinterpret_cast<unsigned int*>(vector.begin));
        MsvcNoOpDestroyRange(PtrToU32(newEnd), PtrToU32(vector.end));
        vector.end = reinterpret_cast<int*>(newEnd);
    }
    MsvcNoOpDestroyRange(PtrToU32(m_flyIconTags.begin), PtrToU32(m_flyIconTags.end));
    m_flyIconTags.end = m_flyIconTags.begin;
}

void zScriptExec::SetSurviveMode(bool enabled, const char* name)
{
    m_modeFlags = enabled?(m_modeFlags|1u):(m_modeFlags&~1u);
    if (!name || !*name)return;

    iQuestServer* const factory = m_pQuestServer;
    iRPGFrameWork* const family = static_cast<iRPGFrameWork*>(factory->QueryInterface(23));
    iRPGObj* const context = family->GetObjByNamee(name, true);
    if (!context)return;

    if (enabled)
    {
        iSurvive* const next = static_cast<iSurvive*>(context->QueryInterface(29));
        m_pSurvive.Set(next);
    }
    else
    {
        iCampaign* const next = static_cast<iCampaign*>(context->QueryInterface(39));
        m_pCampaign.Set(next);
    }
}

void zScriptExec::ExecCommandC(int opcode, const char* textValue, int& resultValue, STRING& resultText)
{
    char* text = const_cast<char*>(textValue);
    resultValue = 0;
    resultText = StringGlobals::EmptyText();

    switch (opcode)
    {
    case EXT_GETINVBYNAME:
        resultValue = -1;
        if (m_refLegs.m_object != 0 && text)
            resultValue = m_refLegs.m_object->GetInvServer()->GetInvByName(text);
        return;

    case EXT_SETUSERNAME:
        if (!text)return;
        {
            iRPGFrameWork* first = m_pRPGFrameWork;
            if (!first)
            {
                DebugAssert(4u, "m_pRPGFrameWork", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1533u);
                first = m_pRPGFrameWork;
            }
            if (first) first->SetPlayerName(text);
            iQuestServer* const second = m_pQuestServer;
            if (second)
            {
                iRPGFrameWork* const target = static_cast<iRPGFrameWork*>(second->QueryInterface(23));
                target->SetPlayerName(text);
            }
        }
        return;

    case EXT_GetIdByName:
        resultValue = -1;
        {
            iRPGFrameWork* const service = m_pRPGFrameWork;
            if (service && text)
            {
                iRPGObj* const found = service->GetObjByNamee(text, true);
                if (found)resultValue = found->GetBaseId();
            }
        }
        return;

    case EXT_SETKEYWAITMODE:
        if (!text)DebugAssert(5u, "var1", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1567u);
        {
            char* first = std::strchr(text, '_');
            if (first)
            {
                char* firstNumber = first+1;
                char* second = std::strchr(firstNumber, '_');
                if (second)
                {
                    *second = 0;
                    const int x1 = BitsToSigned32(CrtAtoiText(firstNumber));
                    const int secondValue = BitsToSigned32(CrtAtoiText(second+1));
                    if (x1<0 || x1 >= zInputMap::ACTION_COUNT)
                        DebugAssert(5u, "x1 >= 0 && x1 < IA_COUNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1579u);
                    zInputMap::m_bKeyWaitAct = static_cast<unsigned int>(x1);
                    zInputMap::m_iKeyWaitNum = secondValue;
                }
            }
        }
        return;

    case EXT_AddHallData:
        {
            unsigned int length = 0u;
            while (text[length])++length;
            AS_RECORD_STRING_CONTROL* const owned = &m_hallRecord.m_playerName;
            if (Grow(owned, length, 1u) != 0u)
            {
                char* const dst = owned->data;
                for (unsigned int i = 0;i<length;++i)dst[i] = text[i];
                owned->size = length;
                dst[length] = 0;
            }
        }
        return;

    default:
        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1585u);
        return;
    }
}



void ScriptExecMapControl::insert(int key, int value)
{
    ScriptExecMapNode* node = head->parent;
    ScriptExecMapNode* parent = head;
    bool left = true;
    while (node != _Nil)
    {
        parent = node;
        left = key<node->key;
        node = left ? node->left : node->right;
    }
    bool insertRequired = policyState[0] != 0u;
    if (!insertRequired)
    {
        if (left)
        {
            if (parent == head->left) insertRequired = true;
            else
                insertRequired = TreeOperations::Predecessor(parent, _Nil)->key<key;
        }
        else
            insertRequired = parent->key<key;
    }
    if (insertRequired)
        (void)TreeOperations::Insert(head, size, node, parent, _Nil,
            [key, value](ScriptExecMapNode* inserted)
            {
                inserted->key = key;
                inserted->value = value;
            },
            [key](const ScriptExecMapNode* existing) { return key<existing->key; });
}

void zScriptExec::AddFlyIcon(int vid, int direction, int count, int x, int y, int z, bool tag)
{
    if (!vid)return;
    int values[6] = {vid, direction, count, x, y, z};
    for (unsigned int i = 0;i<6u;++i)
    {
        AS_RECORD_DWORD_VECTOR& vector = m_flyIconDwords[i];
        vector.insert(vector.end, 1u, values[i]);
    }

    zScriptExecByteVector& bytes = m_flyIconTags;
    unsigned char* const oldEnd = bytes.end;
    if (oldEnd == bytes.capacityEnd)
    {
        const unsigned int old = bytes.begin?static_cast<unsigned int>(oldEnd-bytes.begin):0u;
        const unsigned int newCapacity = old+(old>1u?old:1u);
        const unsigned int allocationSize = BitsToSigned32(newCapacity)<0?0u:newCapacity;
        unsigned char* const newBegin = static_cast<unsigned char*>(CrtNew(allocationSize));
        unsigned char* destination = newBegin;
        for (unsigned char* source = bytes.begin;source != oldEnd;++source, ++destination)
            if (destination)*destination = *source;
        if (destination)*destination = static_cast<unsigned char>(tag);
        ++destination;
        for (unsigned char* source = oldEnd;source != bytes.end;++source, ++destination)
            if (destination)*destination = *source;
        MsvcNoOpDestroyRange(PtrToU32(bytes.begin), PtrToU32(bytes.end));
        CrtFreePointer(bytes.begin);
        bytes.capacityEnd = newBegin+newCapacity;
        const unsigned int previous = bytes.begin?static_cast<unsigned int>(bytes.end-bytes.begin):0u;
        bytes.begin = newBegin;
        bytes.end = newBegin+previous+1u;
        return;
    }
    *oldEnd = static_cast<unsigned char>(tag);
    ++bytes.end;
}

void zScriptExec::GetHotKey(STRING& hotKeyText, int actionIndex, int bindingIndex)
{
    if (actionIndex <= 0 || actionIndex >= zInputMap::ACTION_COUNT)
    {
        DebugAssert(5,
            "var1 > 0 && var1 < IA_COUNT",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp",
            2314);
    }

    const InpAction binding = zInputMap::GetHotKey(
        static_cast<unsigned int>(actionIndex), bindingIndex);

    const unsigned int inputMode = binding.inputMode;
    const int inputValue = static_cast<int>(binding.inputValue);
    PROFILE* const profile =
        ::Strings;

    if (inputMode != IM_KEYDOWN)
    {
        const char* const modeName = zInputMap::InpMsgToString(inputMode);
        if (!modeName)
            return;

        STRING defaultValue;
        STRING key;
        STRING section;
        defaultValue = StringGlobals::EmptyText();
        key = modeName;
        section = "menu";

        STRING localized = profile->GetString(section, key, defaultValue);
        hotKeyText = localized;

        return;
    }

    const char* const keyName = zInputMap::KeyToString(static_cast<unsigned int>(inputValue), false);
    if (!keyName)
    {
        (void)std::sprintf(hotKeyCharacter, "%c",
            static_cast<unsigned int>(inputValue));
        hotKeyText = hotKeyCharacter;
        return;
    }

    STRING key;
    key = "VK_";
    key += keyName;

    STRING defaultValue;
    STRING section;
    defaultValue = StringGlobals::EmptyText();
    section = "menu";

    STRING localized = profile->GetString(section, key, defaultValue);
    key = localized;

    if (key.IsEmpty() != 0)
        hotKeyText = keyName;
    else
        hotKeyText = key;

}

bool zScriptExec::ExecCommandA(
    int commandValue, int argument1, int argument2, int argument3, int& resultValueRef, STRING& resultText)
{
    zScriptExec* self = this;
    const unsigned int command = static_cast<unsigned int>(commandValue);
    const unsigned int argument1Bits = static_cast<unsigned int>(argument1);
    const unsigned int argument2Bits = static_cast<unsigned int>(argument2);
    const unsigned int argument3Bits = static_cast<unsigned int>(argument3);


    resultValueRef = 0;
    int resultValue = 0;
    resultText = StringGlobals::EmptyText();
    if (command>static_cast<unsigned int>(EXT_SETPERKSKILL))
    {
        DebugAssert(4,
            "0",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 2284);
        return false;
    }

    switch (command)
    {
    case EXT_PROC_SHOWINVENTORY:
        {
        const unsigned int enabled = (argument1Bits != 0u)?1u:0u;
        static_cast<iVidCopier*>(self)->ShowInventory(enabled != 0u);
        static_cast<iVidCopier*>(self)->InventorySetActive(enabled != 0u);
        break;
    }
    case EXT_GETITEMNUM:
        {
        iRPGHero* owner = m_refLegs.m_object;
        const iRPGObj* selected = owner->GetEquip(static_cast<int>(argument1Bits));
        if (!selected) break;
        if (argument2Bits == 0u)
            {
                resultValue = BitsToSigned32(static_cast<unsigned int>(selected->GetNumInCell()));
            }
        else
            {
            iInvServer* context = m_refLegs.m_object->GetInvServer();
            const unsigned int key = static_cast<unsigned int>(selected->GetBaseId());
            resultValue = BitsToSigned32(static_cast<unsigned int>(context->GetObjectCnt(static_cast<int>(key))));
        }
        break;
    }
    case EXT_GETDIRTYSTATE:
        resultValue = BitsToSigned32(static_cast<unsigned int>(static_cast<iVidCopier*>(self)->GetDirtyState()));
        break;
    case EXT_GETARMORDIR:
        {
        iRPGHero* owner = m_refLegs.m_object;
        const iRPGObj* source = owner->GetArmor();
        resultValue = BitsToSigned32(static_cast<unsigned int>(DirFromRPGObj(source)));
        break;
    }
    case EXT_GETWEAPONID:
        {
        iRPGHero* owner = m_refLegs.m_object;
        iRPGObj* child = owner->GetCurWpn();
        if (child) resultValue = BitsToSigned32(static_cast<unsigned int>(child->GetExtType()));
        break;
    }
    case EXT_GETARMORID:
        {
        iRPGHero* context = GetHeroForShow();
        const iRPGObj* child = context->GetArmor();
        if (child) resultValue = BitsToSigned32(static_cast<unsigned int>(child->GetExtType()));
        break;
    }
    case EXT_GETHEROID:
        {

        iRPGObj* const owner = self->m_refBody.m_object;
        resultValue = BitsToSigned32(static_cast<unsigned int>(owner->GetExtType()));
        break;
    }
    case EXT_IFHASITEM:
        {
        iRPGHero* owner = m_refLegs.m_object;
        iInvServer* child = owner->GetInvServer();
        const unsigned int raw = static_cast<unsigned int>(child->GetEquipEx(static_cast<int>(argument1Bits), 1) != 0);
        resultValue = BitsToSigned32((raw&0xFFu)?1u:0u);
        break;
    }
    case EXT_GETITEMDISABLED:
        {
        iRPGHero* owner = m_refLegs.m_object;
        iRPGObj* selected = 0;
        selected = (argument1Bits == 0xFFFFFFFFu)?owner->GetCurWpn():const_cast<iRPGObj*>(owner->GetEquip(static_cast<int>(argument1Bits)));
        if (selected)
            {
            iInvKeeper* const context = static_cast<iInvKeeper*>(m_refLegs.m_object->QueryInterface(1));
            resultValue = BitsToSigned32(context->CanHoldObj(selected)?0u:1u);
        }
        break;
    }
    case EXT_GETRELOADBARDIR:
        resultValue = BitsToSigned32(self->m_reloadBarDirection);
        break;
    case EXT_INVENTORYSETACTIVE:
        DebugAssert(4, "0",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 2021);
        break;
    case EXT_ADDSKILL:
        {
        if (argument1Bits == static_cast<unsigned int>(RPGHERO__HP))
            {
            iRPGHero* owner = m_refLegs.m_object;
            (void)owner->AddHp(static_cast<int>(argument2Bits), argument3Bits != 0u);
        }
            else if (argument1Bits == static_cast<unsigned int>(RPGHERO_EXP))
            {
            iRPGHero* owner = m_refLegs.m_object;
            owner->AddExperience(static_cast<int>(argument2Bits), argument3Bits != 0u);
        }
            else if (argument3Bits != 0u && argument1Bits == static_cast<unsigned int>(RPGHERO_MONEY))
            {
            iRPGHero* owner = m_refLegs.m_object;
            (void)owner->AddMoney(static_cast<int>(argument2Bits), false);
        }
            else
            {
            iRPGObj* const owner = m_refBody.m_object;
            (void)owner->IncSkillValue(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits));
        }
        break;
    }
    case EXT_GETCURAMMOBARDIR:
        if (argument1Bits != 0u) (void)static_cast<iVidCopier*>(self)->RecalcCurAmmoClip(false);
        resultValue = BitsToSigned32(static_cast<unsigned int>(self->m_curAmmoDir));
        break;
    case EXT_GETPERKSKILL:
        {
        iRPGHero* context = GetHeroForShow();
        if (!context)
            {
                resultValue = BitsToSigned32(0u);
                break;
            }
        iRPGObj* typed = static_cast<iRPGObj*>(GetHeroForShow()->QueryInterface(9));
        resultValue = BitsToSigned32(static_cast<unsigned int>(typed->GetPerkSkill()));
        break;
    }
    case EXT_SETPERKSKILL:
        m_paramDialog->SetPerkSkill(static_cast<int>(argument1Bits));

    case EXT_GETITEMVIDBYID:
        {
        iRPGFrameWork* registry = m_pRPGFrameWork;
        iRPGObj* selected = registry->GetItemById(static_cast<int>(argument1Bits));
        if (!selected) break;
        if (argument2Bits == 0u)
            {
            resultValue = BitsToSigned32(static_cast<unsigned int>(selected->GetVidNum(0)));
            break;
        }
        if (argument2Bits == 1u)
            {
            resultValue = BitsToSigned32(static_cast<unsigned int>(selected->GetVidNum(2)));
            break;
        }
        if (argument2Bits == 2u)
            {
            resultValue = BitsToSigned32(static_cast<unsigned int>(selected->GetDirNum(2)));
            break;
        }
        if (argument2Bits != 3u) break;

        resultValue = BitsToSigned32(argument1Bits);
        if (static_cast<unsigned int>(selected->GetGameObj()) == 6u && m_refLegs.m_object &&
           static_cast<unsigned int>(selected->GetSkillValue(RPGEQUIP_ADDEXISTINGAMMO)) == 2u)
            {
            iRPGObj* child = m_refLegs.m_object->GetCurWpn();
            if (!child) break;
            zRPGWeapon* typed = static_cast<zRPGWeapon*>(child->QueryInterface(50));
            zRPGAmmo* ammo = typed->GetAmmo();
            iRPGObj* buf = ammo->m_pEquipOwner;
            if (buf)
                {
                resultValue = BitsToSigned32(static_cast<unsigned int>(buf->GetBaseId()));
                break;
            }
            if (static_cast<zRPGObj*>(ammo)->IsInfinite()) break;
            DebugAssert(4, "buf || ammo->IsInfinite()",
                "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1715);
            break;
        }

        if (static_cast<unsigned int>(selected->GetVidNum(0)) == 1682u)
            {


            if (m_refLegs.m_object->GetArmor() == 0) resultValue = BitsToSigned32(0u);
        }
        break;
    }
    case EXT_ADDITEMBYBASEID:
        {
        iRPGHero* owner = m_refLegs.m_object;
        if (!owner) break;
        iRPGFrameWork* registry = m_pRPGFrameWork;
        iRPGObj* selected = registry->GetItemById(static_cast<int>(argument1Bits));
        if (!selected)
            {
            DebugAssert(4, "0",
                "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 2254);
            break;
        }
        const unsigned int acceptedRaw = static_cast<unsigned int>(m_refLegs.m_object->PutItem(selected));
        if ((acceptedRaw&1u) == 0u)
            {
            iLogConsole* logger = ::gpLogCons;
            (void)logger->LogStr(LT_INFO, "EXT_ADDITEMBYBASEID: was not put %d", acceptedRaw);
        }
        if (static_cast<unsigned int>(selected->GetGameObj()) != 2u) break;
        zRPGWeapon* typed = static_cast<zRPGWeapon*>(selected->QueryInterface(50));
        zRPGAmmo* payload = typed->GetAmmo();
        iRPGObj* item = static_cast<zRPGObj*>(payload);
        if (item->IsInfinite()) break;
        iRPGObj* current = item->GetObjCopy();
        const int value = current->GetGrndTakeNum();
        current->SetNumInCell(value);
        (void)m_refLegs.m_object->PutItem(current);
        break;
    }
    case EXT_DISABLEFLAGMANCONTROL:
        {
        self->m_flagmanControlEnabled = static_cast<unsigned char>((argument1Bits == 0u)?1u:0u);
        iLogConsole* logger = ::gpLogCons;
        (void)logger->LogStr(LT_NOTICE, "Flagman control changed: %d", argument1Bits);
        break;
    }
    case EXT_GETFLYITEMVID:
        {
        if (argument1Bits == 0u)
            {
            const unsigned int sentinel = 0xFFFFFDD5u;
            unsigned int value = sentinel;
            AS_RECORD_DWORD_VECTOR& vector = m_flyIconDwords[0];
            if (vector.begin != 0 && vector.end != vector.begin)
                {
                value = static_cast<unsigned int>(*vector.begin);
                if (value != sentinel)
                    {
                    for (int* source = vector.begin+1;source != vector.end;++source)
                        source[-1] = *source;
                    --vector.end;
                    self->m_lastFlyIconTick = ::CurrentTime;
                }
            }
            resultValue = BitsToSigned32(value);
        }
            else
            {
            zScriptExecByteVector& vector = m_flyIconTags;
            if (vector.begin != 0 && vector.end != vector.begin)
                {
                const unsigned char value = *vector.begin;
                for (unsigned char* source = vector.begin+1;source != vector.end;++source)
                    source[-1] = *source;
                --vector.end;
                resultValue = value != 0u?1:0;
            }
        }
        break;
    }
    case EXT_GetCurDroneId:
        {
        iRPGHero* owner = m_refLegs.m_object;
        if (!owner) break;
        iRPGObj* replacement = const_cast<iRPGObj*>(owner->GetEquip(8));
        self->m_selectedEquipmentRef.Set(replacement);
        iRPGObj* current = self->m_selectedEquipmentRef.m_object;
        if (current) resultValue = BitsToSigned32(static_cast<unsigned int>(current->GetExtType()));
        break;
    }
    case EXT_SELECTBESTWEAPON:
        {
        iRPGHero* owner = m_refLegs.m_object;
        if (owner) owner->SelectBestWeapon();
        break;
    }
    case EXT_GETITEMDIR:
        {
        iRPGHero* context = GetHeroForShow();
        const iRPGObj* selected = context->GetEquip(static_cast<int>(argument1Bits));
        resultValue = BitsToSigned32(static_cast<unsigned int>(DirFromRPGObj(selected)));
        break;
    }
    case EXT_GETITEMVID:
        {
        iRPGHero* context = GetHeroForShow();
        const iRPGObj* selected = context->GetEquip(static_cast<int>(argument1Bits));
        resultValue = BitsToSigned32(selected?static_cast<unsigned int>(selected->GetVidNum(1)):0xFFFFFFFFu);
        break;
    }
    case EXT_GETHEROFACEDIR:
        {
        iRPGHero* context = GetHeroForShow();
        iRPGObj* typed = static_cast<iRPGObj*>(context->QueryInterface(9));
        const int value = typed->GetSkillValue(RPGHERO_VIDHEAD);
        resultValue = BitsToSigned32(static_cast<unsigned int>(static_cast<iVidCopier*>(self)->GetFaceDir(value)));
        break;
    }
    case EXT_GETITEMDIRTY:
        {
        iRPGFrameWork* registry = m_pRPGFrameWork;
        iRPGObj* hero = registry->GetItemById(static_cast<int>(argument1Bits));
        if (!hero)
            DebugAssert(5, "hero",
                "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 2141);

        iRPGHero* const typed = static_cast<iRPGHero*>(hero->QueryInterface(0));
        resultValue = BitsToSigned32(static_cast<unsigned int>(typed->GetItemDirty(static_cast<int>(argument2Bits))));
        break;
    }
    case EXT_GETINVSIZEX:
        {
        iRPGHero* const owner = m_refLegs.m_object;
        if (!owner) break;
        iInventory* const selected = owner->GetInvServer()->GetInv(static_cast<int>(argument1Bits));
        iInventory2D* const metric = static_cast<iInventory2D*>(selected->QueryInterface(17));
        int first = static_cast<int>(argument2Bits), second = static_cast<int>(argument3Bits);
        metric->GetSizeInCells(first, second);
        const unsigned int x = static_cast<unsigned int>(metric->GetCellSizeX());
        (void)metric->GetCellSizeY();
        resultValue = BitsToSigned32(x*static_cast<unsigned int>(first));
        break;
    }
    case EXT_GETINVSIZEY:
        {
        iRPGHero* const owner = m_refLegs.m_object;
        if (!owner) break;
        iInventory* const selected = owner->GetInvServer()->GetInv(static_cast<int>(argument1Bits));
        iInventory2D* const metric = static_cast<iInventory2D*>(selected->QueryInterface(17));
        int first = static_cast<int>(argument2Bits), second = static_cast<int>(argument3Bits);
        metric->GetSizeInCells(second, first);
        (void)metric->GetCellSizeX();
        const unsigned int y = static_cast<unsigned int>(metric->GetCellSizeY());
        resultValue = BitsToSigned32(y*static_cast<unsigned int>(first));
        break;
    }
    case EXT_AUTOTAKINGDIST:
        {
        self->m_itemTaker->SetItemTakeDist(static_cast<float>(static_cast<int>(argument1Bits)));
        break;
    }
    case EXT_GETITEMID:
        {
        iRPGHero* owner = m_refLegs.m_object;
        const iRPGObj* child = owner->GetEquip(static_cast<int>(argument1Bits));
        if (child) resultValue = BitsToSigned32(static_cast<unsigned int>(child->GetBaseId()));
        break;
    }
    case EXT_GETMESSAGEDIRTY:
        {
        iPlayer* const player = static_cast<iPlayer*>(self);
        const int limit = player->GetCurTime();
        if (static_cast<int>(self->m_messageAvailableTick) <= limit)resultValue = BitsToSigned32(self->m_messageId);
        break;
    }
    case EXT_REMOVEFLAGMANDIALOG:
        {
        static_cast<iVidCopier*>(self)->RemoveFlagmanDialog();
        break;
    }
    case EXT_GETMESSAGE:
        {
        if (self->m_messageId != 0u)
            {
            const char* text = self->m_debugText.m_pBuf;
            resultText = text?text:StringGlobals::EmptyText();
            self->m_debugText.operator = (StringGlobals::EmptyText());
            self->m_messageId = 0u;
            self->m_messageAvailableTick = 0u;
        }
        resultValue = BitsToSigned32(0u);
        resultValueRef = resultValue;
        return true;
    }
    case EXT_GETCARSPEED:
        resultValue = BitsToSigned32(static_cast<unsigned int>(static_cast<iVidCopier*>(self)->GetCarSpeed(0)));
        break;
    case EXT_PREVHERO:
        m_paramDialog->PrevHero();
        break;
    case EXT_NEXTHERO:
        m_paramDialog->NextHero();
        break;
    case EXT_CHANGESEX:
        m_paramDialog->ChangeCurSex();
        break;
    case EXT_PREVCOLOR:
        (void)self->PrevHeroColor();
        break;
    case EXT_NEXTCOLOR:
        (void)self->NextHeroColor();
        break;
    case EXT_PDADDSKILL:
        {
        if (!m_paramDialog)
            {
            DebugAssert(5, "m_pRPGParamDlg",
                "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1775);
        }
        m_paramDialog->AddSkill(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits));
        break;
    }
    case EXT_PDCANINCSKILL:
        resultValue = BitsToSigned32(m_paramDialog->CanIncSkill(static_cast<int>(argument1Bits))?1u:0u);
        break;
    case EXT_PDCANDECSKILL:
        resultValue = BitsToSigned32(m_paramDialog->CanDecSkill(static_cast<int>(argument1Bits))?1u:0u);
        break;
    case EXT_PDASSIGNSKILLS:
        m_paramDialog->AssignSkills();
        break;
    case EXT_PDREMOVESKILLINCREASE:
        m_paramDialog->RemoveSkillIncrease();
        break;
    case EXT_GETRADIOMSGDIRTY:
        {
        const unsigned int value = self->m_radioMessageDirty?1u:0u;
        self->m_radioMessageDirty = 0u;
        resultValue = BitsToSigned32(value);
        break;
    }
    case EXT_GETRADIOMSG:
        {
        zString* const pendingText = &self->m_pendingText;
        if (!pendingText->IsEmpty())
            {
            const char* text = static_cast<const char*>(*pendingText);
            resultText = text;
        }
        resultValue = BitsToSigned32(0u);
        resultValueRef = resultValue;
        return true;
    }
    case EXT_GETRADIOICONVID:
        resultValue = BitsToSigned32(self->m_radioIconVid);
        break;
    case EXT_GETRADIOICONDIR:
        resultValue = BitsToSigned32(self->m_radioIconDirection);
        break;
    case EXT_GETRADIOFONTVID:
        resultValue = BitsToSigned32(self->m_radioFontVid);
        break;
    case EXT_SETHERODAMAGECOEFF:
        {
        iRPGHero* owner = m_refLegs.m_object;
        if (owner)
            {
            const float value = static_cast<float>(static_cast<double>(static_cast<int>(argument1Bits))*static_cast<double>(0.01f));
            owner->SetDamageCoeff(value);
        }
        break;
    }
    case EXT_REMOVEITEMIFHAS:
        {
        const unsigned int selector = (argument1Bits == 1u)?1u:3u;
        iRPGHero* owner = m_refLegs.m_object;
        iInvServer* context = owner->GetInvServer();
        iRPGObj* selected = context->GetEquipEx(static_cast<int>(argument1Bits), static_cast<int>(selector));
        if (selected)
            {
            (void)selected->AddNumInCell(-1);
            if (argument1Bits == 8u) static_cast<iVidCopier*>(self)->SetTankCanShot();
        }
        break;
    }
    case EXT_FIRSTHERO:
        m_paramDialog->FirstHero(1);
        break;
    case EXT_RESETHEROHP:
        {
        iRPGHero* owner = m_refLegs.m_object;
        (void)owner->SetHpToMax();
        break;
    }
    case EXT_OnLevelWin:
        {
        iRPGHero* owner = m_refLegs.m_object;
        if (!owner)
            DebugAssert(5, "m_pRPGHero",
                "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1803u);

        iRPGObj* profile = static_cast<iRPGObj*>(m_refLegs.m_object->QueryInterface(9));
        iPlayer* const player = static_cast<iPlayer*>(self);
        const bool enabled = ((player->GetArcadeMod_()&1) != 0) || argument1Bits != 0u;
        iQuestServer* const questServer = player->IsCurPlayer()?m_pQuestServer:0;

        self->ProfMngr()->OnLevelWin(
            profile,
            questServer,
            enabled);
        break;
    }
    case EXT_GETFLYITEMDIR:
    case EXT_GETFLYITEMCNT:
    case EXT_GETFLYITEMX:
    case EXT_GETFLYITEMY:
    case EXT_GETFLYITEMZ:
        {
        AS_RECORD_DWORD_VECTOR& vector = m_flyIconDwords[command-EXT_GETFLYITEMVID];
        if (vector.begin != 0 && vector.end != vector.begin)
            {
            const int value = *vector.begin;
            for (int* source = vector.begin+1;source != vector.end;++source)
                source[-1] = *source;
            --vector.end;
            resultValue = value;
        }
        break;
    }
    case EXT_SETSKILL:
        {
        iRPGObj* const owner = m_refBody.m_object;
        if (owner) (void)owner->SetSkillValue(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits));
        break;
    }
    case EXT_REPAIRARMOR:
        {
        iRPGHero* owner = m_refLegs.m_object;
        if (!owner) break;
        const iRPGObj* child = owner->GetArmor();
        if (!child) break;
        iRPGArmor* typed = static_cast<iRPGArmor*>(child->QueryInterface(51));
        (void)typed->Repair(99999999);
        break;
    }
    case EXT_GetAddWpnCnt:
        {
        iSurvive* owner = m_pSurvive.m_object;
        if (owner) resultValue = BitsToSigned32(static_cast<unsigned int>(owner->GetAddWpnCnt(static_cast<int>(argument1Bits))));
        break;
    }
    case EXT_GetAddWpnId:
        {
        iSurvive* owner = m_pSurvive.m_object;
        if (owner)
            {
            resultValue = BitsToSigned32(static_cast<unsigned int>(owner->GetAddWpnId(
                static_cast<int>(argument1Bits), static_cast<int>(argument2Bits),
                m_refLegs.m_object, m_pRPGFrameWork)));
        }
        break;
    }
    case EXT_ADDHYPNMONSTER:
        static_cast<iVidCopier*>(self)->AddHypnMonst(reinterpret_cast<SPRITE*>(U32ToPtr(argument1Bits)), static_cast<int>(argument2Bits));
        break;
    case EXT_SetPlayerMenuType:
        {
        m_paramDialog->SetPlayerMenuType(static_cast<int>(argument1Bits));
        iPlayer* const player = static_cast<iPlayer*>(self);
        if (m_pParamDlgMenu != 0 && player->IsCurPlayer())
            {
            iParamDlgMenu* const observer =
                m_pParamDlgMenu;
            observer->SetRPGParamDialog(m_paramDialog);
        }
        resultValue = BitsToSigned32(0u);
        break;
    }
    case EXT_SetCurDroneHp:
        {
        iRPGObj* const owner = self->m_selectedEquipmentRef.m_object;
        if (owner) (void)owner->SetSkillValue(RPGEQUIP_DroneHp, static_cast<int>(argument1Bits));
        break;
    }
    case Ext_SwitchToStandFirm:
        {
        iRPGHero* owner = m_refLegs.m_object;
        if (!owner) break;
        if (argument1Bits >= 1u && argument1Bits <= 3u)
            {
            (void)owner->SwitchToMode(static_cast<iRPGHero::M_MODE>(argument1Bits-1u));
        }
            else
            {
            DebugAssert(5, "0",
                "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 2183);
        }
        break;
    }
    case Ext_GetGamObjType:
        {
        iRPGFrameWork* registry = m_pRPGFrameWork;
        iRPGObj* selected = registry->GetItemById(static_cast<int>(argument1Bits));
        if (selected) resultValue = BitsToSigned32(static_cast<unsigned int>(selected->GetType()));
        break;
    }
    case EXT_AddItemsToBackPack:
        {
        iRPGFrameWork* registry = m_pRPGFrameWork;
        iRPGObj* selected = registry->GetItemById(static_cast<int>(argument1Bits));
        if (!selected) break;
        iRPGObj* const targetObject = selected->GetObjCopy();
        (void)self->m_itemTaker->CreateGroundSprite(targetObject);

        iRPGHero* owner = m_refLegs.m_object;
        iRPGObj* anchor = owner->GetCurWpn();
        iLogConsole* logger = ::gpLogCons;
        const char* const anchorName = anchor->GetName();
        (void)logger->LogStr(LT_INFO, "Cur wpn %s", anchorName);

        anchor = m_refLegs.m_object->GetCurWpn();
        for (unsigned int index = 0u;index<19u;++index)
            {
            iInvServer* context = m_refLegs.m_object->GetInvServer();
            iInventory* node = context->GetInv(static_cast<int>(index));
            zInventory* inventory = static_cast<zInventory*>(node->QueryInterface(46));

            while (inventory->m_apItems.begin != 0 && inventory->m_apItems.end != inventory->m_apItems.begin)
                {
                zItem* candidate = inventory->GetItem(0);
                (void)inventory->TakeItem(candidate);
                const unsigned int firstType = static_cast<unsigned int>(candidate->GetGameObj());
                bool attach = (firstType == 2u && candidate == anchor);


                if (!attach) attach = (candidate->GetGameObj() == 3);
                if (attach)
                    {
                    targetObject->AddItem(candidate);
                    logger = ::gpLogCons;
                    const int detail = candidate->GetNumInCell();
                    const char* name = candidate->GetName();
                    (void)logger->LogStr(LT_INFO, "Item added %s %d", name, detail);
                }
            }
        }
        break;
    }
    default:


        DebugAssert(4, "0",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 2284);
        break;
    }
    resultValueRef = resultValue;
    return false;
}

static int ScriptExecCartesianAngle(int x, int y)
{

    const double angle = atan2(static_cast<double>(y), static_cast<double>(x));
    return TruncateDoubleToInt32(angle*static_cast<double>(40.7436638f));
}

static int ScriptExecCartesianLength(int x, int y)
{
    const double fx = static_cast<double>(x);
    const double fy = static_cast<double>(y);
    return TruncateDoubleToInt32(sqrt(fx*fx+fy*fy));
}

static int ScriptExecFtolSubFloat(float value, float subtractValue)
{
    return TruncateDoubleToInt32(static_cast<double>(value)-static_cast<double>(subtractValue));
}

static int ScriptExecFtolFloatMinusIntMinusFloat(float value, int integerValue, float floatValue)
{
    return TruncateDoubleToInt32((static_cast<double>(value)-static_cast<double>(integerValue))-static_cast<double>(floatValue));
}

static int ScriptExecFtolFloatMinusRoundedIntMinusFloat(float value, int integerValue, float floatValue)
{

    const float roundedInteger = static_cast<float>(integerValue);
    return TruncateDoubleToInt32((static_cast<double>(value)-static_cast<double>(roundedInteger))-static_cast<double>(floatValue));
}

static int ScriptExecFtolIntPlusFloat(int integerValue, float floatValue)
{
    return TruncateDoubleToInt32(static_cast<double>(integerValue)+static_cast<double>(floatValue));
}

static int ScriptExecFtolTwoIntsPlusFloat(int first, int second, float floatValue)
{

    const float roundedFirst = static_cast<float>(first);
    const float roundedSecond = static_cast<float>(second);
    return TruncateDoubleToInt32(static_cast<double>(roundedFirst)+(static_cast<double>(roundedSecond)+static_cast<double>(floatValue)));
}

static unsigned int ScriptExecVectorDwordCount(const AS_RECORD_DWORD_VECTOR& vectorOwner)
{
    if (!vectorOwner.begin)return 0u;
    return static_cast<unsigned int>(vectorOwner.end-vectorOwner.begin);
}

static void ScriptExecAppendHallDword(AS_RECORD_DWORD_VECTOR* vectorOwner, unsigned int value)
{

    const int localValue = BitsToSigned32(value);
    vectorOwner->insert(vectorOwner->end, 1u, localValue);
}

static void ScriptExecAssignFormattedHallValue(STRING& outString, const char* format, unsigned int value)
{
    const STRING formatted = Printf(format, value);
    outString = formatted;
}

int ScriptExecMapControl::find(int key) const
{
    ScriptExecMapNode* const header = head;
    ScriptExecMapNode* const nil = ScriptExecMapControl::_Nil;
    ScriptExecMapNode* candidate = header;
    ScriptExecMapNode* node = header->parent;
    const int signedKey = key;
    while (node != nil)
    {
        if (node->key<signedKey)node = node->right;
        else
        {
            candidate = node;
            node = node->left;
        }
    }
    if (candidate != header && signedKey >= candidate->key)return candidate->value;
    return -777;
}

bool zScriptExec::ExecCommand(
    int commandValue,
    int argument1,
    int argument2,
    int argument3,
    int& intResultRef,
    STRING& stringResultRef,
    SPRITE*& specialResultRef)
{
    zScriptExec* self = this;
    const unsigned int command = static_cast<unsigned int>(commandValue);
    const unsigned int argument1Bits = static_cast<unsigned int>(argument1);
    const unsigned int argument2Bits = static_cast<unsigned int>(argument2);
    const unsigned int argument3Bits = static_cast<unsigned int>(argument3);

    unsigned int intResult = 0u;
    bool returnsString = false;
    intResultRef = 0;
    stringResultRef = StringGlobals::EmptyText();
    specialResultRef = 0;

    switch (command)
    {
    case EXT_GETSKILL:
    {
        if (argument3Bits != 0u)
            {
            iRPGHero* const object = GetHeroForShow();
            iRPGObj* const typedObject = static_cast<iRPGObj*>(object->QueryInterface(9));
            const int type = typedObject->GetExtType();
            intResult = static_cast<unsigned int>(m_pRPGFrameWork->GetSkillValue(type, static_cast<int>(argument2Bits)));
        }
            else
            {
            intResult = static_cast<unsigned int>(m_pRPGFrameWork->GetSkillValue(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits)));
        }
        break;
    }
    case EXT_GETSKILLBASE:
    {
        iRPGFrameWork* objectRegistry = m_pRPGFrameWork;
        iRPGObj* object = objectRegistry->GetItemById(static_cast<int>(argument1Bits));
        if (object) intResult = static_cast<unsigned int>(object->GetSkillValueBase(static_cast<int>(argument2Bits)));
        break;
    }
    case EXT_GETMONSTERVID:
    case EXT_GETMONSTERCOUNT:
    case EXT_GETMONSTERTYPESCNT:
    case EXT_GetBossSize:
    case EXT_GetBossHpCoeff:
    case EXT_IsBossPeriod:
    case EXT_GETBONUSID:
    case EXT_GetBossColor:
    case EXT_GetScoresCoeff:
    {
        iSurvive* periodOwner = m_pSurvive.m_object;
        if (periodOwner)
            {
            iSurvive* selectedObject = 0;
            int selectedValue = 0;
            (void)periodOwner->GetIntrval(static_cast<int>(argument1Bits), selectedObject, selectedValue, 0);
            if (selectedObject)
                {
                switch (command)
                    {
                case EXT_GETMONSTERVID:
                    intResult = static_cast<unsigned int>(selectedObject->GetMonstersVid(selectedValue, static_cast<int>(argument2Bits)));
                    break;
                case EXT_GETMONSTERCOUNT:
                    intResult = static_cast<unsigned int>(selectedObject->GetMonstersCount(selectedValue, static_cast<int>(argument2Bits)));
                    break;
                case EXT_GETMONSTERTYPESCNT:
                    intResult = static_cast<unsigned int>(selectedObject->GetMonsterVidsCnt(selectedValue));
                    break;
                case EXT_GetBossSize:
                    intResult = static_cast<unsigned int>(selectedObject->GetBossSize());
                    break;
                case EXT_GetBossHpCoeff:
                    intResult = static_cast<unsigned int>(selectedObject->GetBossHpCoeff());
                    break;
                case EXT_IsBossPeriod:
                    intResult = selectedObject->IsBossPeriod(selectedValue)?1u:0u;
                    break;
                case EXT_GETBONUSID:
                    intResult = static_cast<unsigned int>(selectedObject->GetPeriodBonusId(selectedValue));
                    break;
                case EXT_GetBossColor:
                    intResult = static_cast<unsigned int>(selectedObject->GetBossColor(static_cast<int>(argument2Bits)));
                    break;
                case EXT_GetScoresCoeff:
                    intResult = static_cast<unsigned int>(selectedObject->GetScoresCoeff());
                    break;
                default:
                    break;
                }
            }
        }
        break;
    }
    case EXT_GETQUESTNAME:
    {
        iQuestServer* questOwner = m_pQuestServer;
        iRPGQuest* quest = questOwner->GetQuest(static_cast<int>(argument1Bits+1u), static_cast<int>(argument2Bits), true, true);
        const char* text = m_pQuestServer->GetSubQuestName(quest);
        if (text) stringResultRef = text;
        returnsString = true;
        break;
    }
    case EXT_IFINCREASEDBYIMPLANT:
    {
        iRPGObj* object = 0;
        if (argument3Bits != 0u)
            {
            iRPGHero* hero = GetHeroForShow();
            object = static_cast<iRPGObj*>(hero->QueryInterface(9));
        }
            else
            {
            iRPGFrameWork* objectRegistry = m_pRPGFrameWork;
            object = objectRegistry->GetItemById(static_cast<int>(argument1Bits));
        }
        if (object) intResult = object->IfIncreasedByImplant(static_cast<int>(argument2Bits))?1u:0u;
        break;
    }
    case EXT_GETCOLORLINEDIR:
    {
        unsigned int absoluteDirection = 0u;
        iRPGHero* hero = GetHeroForShow();
        if (hero)
            {
            iRPGObj* object = static_cast<iRPGObj*>(GetHeroForShow()->QueryInterface(9));
            absoluteDirection = static_cast<unsigned int>(object->GetSkillValue(RPGHERO_COLOR));
        }
        const int directionCount = static_cast<iVidCopier*>(self)->GetVidDirectionsNum(739);
        if (directionCount != 0)
            {
            const int scaledDirection = static_cast<int>(absoluteDirection<<8);
            intResult = static_cast<unsigned int>(scaledDirection/directionCount)&0xFFu;
        }
        break;
    }
    case EXT_RESETQUESTSYSTEM:
    {
        iPlayer* const player = static_cast<iPlayer*>(self);
        if (!player->IsCurPlayer())
            DebugAssert(4u, "this->IsCurPlayer()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 700u);
        iQuestServer* questOwner = m_pQuestServer;
        if (questOwner) questOwner->PushCurState(0);
        break;
    }
    case EXT_FIND_ID_FOR_SPRITE:
    {
        iItemTaker* const spriteIndex = self->m_itemTaker;
        iRPGFrameWork* const objectRegistry = m_pRPGFrameWork;
        if (spriteIndex && objectRegistry)
            {
            SPRITE* const sprite = reinterpret_cast<SPRITE*>(U32ToPtr(argument1Bits));
            const unsigned int nVid = static_cast<unsigned int>(sprite->Vid()->nVid);
            if (!spriteIndex->IsNeededVid(static_cast<int>(nVid)))
                DebugAssert(4u, "m_pItemTaker->IsNeededVid( nVid )", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 849u);
            self->m_itemTaker->FindIdForSprite(sprite);
        }
        break;
    }
    case EXT_GETINFOTEXT:
    {
        iRPGFrameWork* const objectRegistry = m_pRPGFrameWork;
        iTextDB* const textOwner = static_cast<iTextDB*>(objectRegistry->QueryInterface(28));
        const char** const record = textOwner->GetExtInfo(0, static_cast<int>(argument1Bits), static_cast<int>(argument2Bits));
        if (record != 0)
            {
            const char* const text = textOwner->GetSpeciallyDecodedString(record, -1);
            stringResultRef = text;
        }
        returnsString = true;
        break;
    }
    case EXT_GETINVNAME:
    {
        iRPGHero* const hero = m_refLegs.m_object;
        if (hero)
            {
            iInventory* const item = hero->GetInvServer()->GetInv(static_cast<int>(argument1Bits));
            stringResultRef = item->GetName();
        }
        returnsString = true;
        break;
    }
    case EXT_ResetAllQuests:
    {
        iQuestServer* questOwner = m_pQuestServer;
        if (!questOwner)
            {
            DebugAssert(5u, "m_pQuestServer", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1046u);
            questOwner = m_pQuestServer;
        }
        questOwner->ResetAllQuests();
        break;
    }
    case EXT_CopyVid:
    {
        VID* const destinationVid = Map->Vid(static_cast<int>(argument1Bits));
        VID* const sourceVid = Map->Vid(static_cast<int>(argument2Bits));
        Map->CopyVid(destinationVid, sourceVid);
        break;
    }
    case EXT_GetSQuestData:
    {
        iQuestServer* questOwner = m_pQuestServer;
        iRPGQuest* quest = questOwner->GetQuest(static_cast<int>(argument1Bits+1u), static_cast<int>(argument2Bits), true, true);
        if (quest)
            {
            if (argument3Bits == 0u)
                {
                intResult = static_cast<unsigned int>(quest->GetSubQNum());
            }
                else if (argument3Bits == 1u)
                {
                questOwner = m_pQuestServer;
                if (questOwner) intResult = questOwner->InCurLevel(quest)?1u:0u;
            }
        }
        break;
    }
    case EXT_SETINVULNERABLE:
    {
        const unsigned int army = argument1Bits;
        const unsigned int baseHeroId = static_cast<unsigned int>(g_iHeroIdForInvuln);
        const unsigned int mappedHeroId = static_cast<unsigned int>(SFLB_GetHeroId(static_cast<int>(army)));
        const unsigned int heroId = baseHeroId+mappedHeroId-army*2u;
        iRPGFrameWork* objectRegistry = m_pRPGFrameWork;
        iRPGObj* hero = objectRegistry->GetItemById(static_cast<int>(heroId));
        if (!hero || hero->GetGameObj() != 0)
            DebugAssert(5u, "hero && hero->GetType() == TYPE_HERO", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 234u);
        iRPGHero* invulnerabilityOwner = static_cast<iRPGHero*>(hero->QueryInterface(0));
        invulnerabilityOwner->SetInvulnerable(static_cast<int>(argument2Bits)>0);
        break;
    }
    case EXT_GETCHARSKILL:
    {
        iRPGFrameWork* objectRegistry = m_pRPGFrameWork;
        iTextDB* skillService = objectRegistry->AsTextDB();
        iRPGObj* object = 0;
        if (argument3Bits != 0u)
            {
            iRPGHero* hero = GetHeroForShow();
            object = static_cast<iRPGObj*>(hero->QueryInterface(9));
        }
            else
            {
            object = objectRegistry->GetItemById(static_cast<int>(argument1Bits));
        }
        const char* text = 0;
        if (object)
            {
            if (argument2Bits == static_cast<unsigned int>(RPGHERO_FULLNAME)) text = objectRegistry->GetItemName(object);
            else text = skillService->GetFormattedSkill(object, static_cast<int>(argument2Bits));
        }
        if (text) stringResultRef = text;
        returnsString = true;
        break;
    }
    case EXT_GETPERIODLENGTH:
    {
        iSurvive* periodOwner = m_pSurvive.m_object;
        if (!periodOwner) break;
        iSurvive* selectedObject = 0;
        int selectedValue = 0;
        const unsigned int selectorValue = static_cast<unsigned int>(periodOwner->GetIntrval(static_cast<int>(argument1Bits), selectedObject, selectedValue, 0));
        if (selectedObject)
            {
            if (argument2Bits == 0u) intResult = static_cast<unsigned int>(selectedObject->GetPeriodLength(selectedValue));
            if (argument2Bits == 1u)
                {
                    intResult = selectorValue;
                    break;
                }
            if (argument2Bits == 2u) intResult = static_cast<unsigned int>(selectedObject->GetPauseLength(selectedValue));
            if (argument2Bits == 4u)
                {
                zSurvive* typed = static_cast<zSurvive*>(selectedObject->QueryInterface(53));
                intResult = static_cast<unsigned int>(typed->GetWithoutMenu(selectedValue));
            }
            if (argument2Bits == 5u)
                {
                intResult = selectorValue;
                for (int i = 0;i<static_cast<int>(selectedValue);++i)
                    {
                    intResult += static_cast<unsigned int>(selectedObject->GetPauseLength(i));
                    intResult += static_cast<unsigned int>(selectedObject->GetPeriodLength(i));
                }
            }
            if (argument2Bits == 6u)
                {
                intResult = 0u;
                const int count = selectedObject->GetPerCnt();
                for (int i = 0;i<count;++i)
                    {
                    intResult += static_cast<unsigned int>(selectedObject->GetPauseLength(i));
                    intResult += static_cast<unsigned int>(selectedObject->GetPeriodLength(i));
                }
            }
        }
        if (argument2Bits == 3u)
            {
            if (!selectedObject || selectedValue == 0xFFFFFFFFu)
                {
                    intResult = 1u;
                    break;
                }
            zSurvive* typed = static_cast<zSurvive*>(selectedObject->QueryInterface(53));
            const unsigned int count = static_cast<unsigned int>(static_cast<iSurvive*>(typed)->GetPerCnt());
            if (static_cast<unsigned int>(selectedValue) == count-1u && static_cast<zSurvive*>(selectedObject->QueryInterface(53))->GetNextInt() == 0) intResult = 1u;
            else intResult = 0u;
        }
        break;
    }
    case EXT_GETQUESTSSTATE:
    {
        iPlayer* const player = static_cast<iPlayer*>(self);
        if (player->GetArcadeMod_() == 0)
            {
            iQuestServer* questOwner = m_pQuestServer;
            iRPGQuest* quest = questOwner->GetQuest(static_cast<int>(argument1Bits+1u), static_cast<int>(argument2Bits), argument3Bits == 0u, true);
            if (quest) intResult = static_cast<unsigned int>(m_pQuestServer->GetQuestState(quest));
        }
        break;
    }
    case EXT_SPRITEHASNAME:
    {
        SPRITE* const sprite = reinterpret_cast<SPRITE*>(U32ToPtr(argument1Bits));
        if (sprite->Vid()->spriteClass == 19u)
            {
            if (sprite->Action(ACT_GET_TEXT, 0, 0, 0) != 0) intResult = 1u;
        }
            else
            {
            EX_SPRITE_DATA* const linked = sprite->ExtraData();
            if (linked && *linked->nameText.value != '\0') intResult = 1u;
        }
        break;
    }
    case EXT_GETTEXTHEIGHT:
        if (argument1Bits != 0u) intResult = ReadGlobalU32(argument1Bits+120u);
        break;
    case EXT_SETQUESTSTATE:
    {
        iQuestServer* questOwner = m_pQuestServer;
        iRPGQuest* quest = questOwner->GetQuest(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits), false, false);
        if (quest) intResult = m_pQuestServer->SetQuestState(quest, static_cast<int>(argument3Bits), false)?1u:0u;
        iLogConsole* logger = ::gpLogCons;
        (void)logger->LogStr(LT_NOTICE, "qs from EXT_SETQUESTSTATE %d %d", argument1Bits, argument2Bits);
        break;
    }
    case EXT_GETREALVAL:
    {
        VID* damageRangeVid = ::EmptyVid;
        if (Map->VidCount()>1402)
            {
            VID* const candidate = Map->StoredVid(1402);
            if (candidate != 0) damageRangeVid = candidate;
        }
        unsigned char cacheFlags = damageRangeInitialized;
        if ((cacheFlags&1u) == 0u)
            {
            cacheFlags = static_cast<unsigned char>(cacheFlags|1u);
            damageRangeInitialized = cacheFlags;
            cachedDamageMinimum = damageRangeVid?damageRangeVid->deathDamageMin:0;
        }
        if ((cacheFlags&2u) == 0u)
            {
            cacheFlags = static_cast<unsigned char>(cacheFlags|2u);
            damageRangeInitialized = cacheFlags;
            cachedDamageMaximum = damageRangeVid?damageRangeVid->deathDamageMax:0;
        }
        if (argument1Bits == 0u) intResult = static_cast<unsigned int>(cachedDamageMinimum);
        else if (argument1Bits == 1u) intResult = static_cast<unsigned int>(cachedDamageMaximum);
        else DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1006u);
        break;
    }
    case EXT_GetPeriodNum:
    {
        iSurvive* periodOwner = m_pSurvive.m_object;
        if (!periodOwner)
            {
                intResult = 0u;
                break;
            }
        iSurvive* selectedObject = 0;
        int selectedValue = 0;
        (void)periodOwner->GetIntrval(static_cast<int>(argument1Bits), selectedObject, selectedValue, 0);
        intResult = static_cast<unsigned int>(selectedValue);
        if (selectedObject)
            {
            zSurvive* typed = static_cast<zSurvive*>(selectedObject->QueryInterface(53));
            intResult = static_cast<unsigned int>(typed->GetAbsPeriodNum(selectedValue));
        }
        break;
    }
    case EXT_bShowInvGrnd:
    {
        iRPGHero* const hero = m_refLegs.m_object;
        if (!hero) break;
        iInvServer* const inventoryList = hero->GetInvServer();
        iInventory* const item = inventoryList->GetInv(19);
        zInventory* const typed = static_cast<zInventory*>(item->QueryInterface(46));
        if (typed->m_apItems.begin != 0 && typed->m_apItems.end != typed->m_apItems.begin)
            {
            intResult = 1u;
            break;
        }
        if (inventoryList->GetTakenObj() != 0) intResult = 1u;
        break;
    }
    case EXT_GetSurvInt:
    {
        returnsString = (argument2Bits == 0u);
        stringResultRef = StringGlobals::EmptyText();
        iSurvive* periodOwner = m_pSurvive.m_object;
        if (!periodOwner) break;
        iSurvive* selectedObject = 0;
        int selectedValue = 0;
        int alternateValue = 0;
        intResult = static_cast<unsigned int>(periodOwner->GetIntrval(static_cast<int>(argument1Bits), selectedObject, selectedValue, &alternateValue));
        if (!selectedObject) break;
        if (returnsString)
            {
            iRPGObj* typed = static_cast<iRPGObj*>(selectedObject->QueryInterface(9));
            const char* text = typed->GetName();
            if (text) stringResultRef = text;
        }
            else
            {
            intResult = static_cast<unsigned int>(alternateValue);
        }
        break;
    }
    case Ext_Monst2Create:
    {
        iSurvive* periodOwner = m_pSurvive.m_object;
        if (!periodOwner) break;
        const int timeValue = static_cast<int>(argument1Bits);
        const int seconds = timeValue/1000;
        const int milliseconds = timeValue%1000;
        iSurvive* selectedObject = 0;
        int selectedValue = 0;
        const unsigned int periodBase = static_cast<unsigned int>(periodOwner->GetIntrval(seconds, selectedObject, selectedValue, 0));
        if (selectedObject)
            {
            const unsigned int localTime = periodBase*1000u+static_cast<unsigned int>(milliseconds);
            zSurvive* typed = static_cast<zSurvive*>(selectedObject->QueryInterface(53));
            intResult = static_cast<unsigned int>(typed->GetMonst2Create(selectedValue, static_cast<int>(localTime), static_cast<int>(argument2Bits)));
        }
        break;
    }
    case EXT_GETSPRITENAME:
    {
        SPRITE* const sprite = reinterpret_cast<SPRITE*>(U32ToPtr(argument1Bits));
        EX_SPRITE_DATA* const linked = sprite->ExtraData();
        if (linked && *linked->nameText.value != '\0')
            {
            STRING builtName = sprite->GetName();
            stringResultRef = builtName;
        }
        returnsString = true;
        break;
    }
    case EXT_RK_SAVE:
    {
        zProfLdr* const profileState = ::g_pProfLdr;
        STRING path = profileState->GetCurSaveName2(static_cast<SAVE_TYPE>(3), false);
        (void)zInputMap::Save(path.m_value);
        break;
    }
    case EXT_RK_CANCEL:
    {
        zProfLdr* const profileState = ::g_pProfLdr;
        STRING path = profileState->GetCurSaveName2(static_cast<SAVE_TYPE>(3), false);
        const unsigned int parsed = zInputMap::Load(path.m_value)?1u:0u;
        if (parsed == 0u) (void)zInputMap::Load("input.cfg");
        break;
    }
    case EXT_RK_INIT:
    {
        zProfLdr* const profileState = ::g_pProfLdr;
        STRING path = profileState->GetCurSaveName2(static_cast<SAVE_TYPE>(3), false);
        (void)zInputMap::Load(path.m_value);
        break;
    }
    case EXT_SetCurProf:
    {
        zProfLdr* const profileState = ::g_pProfLdr;
        if (static_cast<unsigned int>(profileState->m_currentProfile) != argument1Bits)
            {
            self->ProfMngr()->ReleaseArmyHero();
        }
        profileState->SetCurProf(static_cast<int>(argument1Bits));
        STRING path = profileState->GetCurSaveName2(static_cast<SAVE_TYPE>(3), false);
        (void)zInputMap::Load(path.m_value);
        break;
    }
    case EXT_LEVELMONEY:
    {
        iQuestServer* const questOwner = m_pQuestServer;
        if (questOwner && argument1Bits != 0xFFFFFFFFu)
            {
            STRING levelText = Printf("Lvl_%.2d", argument1Bits);
            iRPGFrameWork* const levelService = static_cast<iRPGFrameWork*>(m_pQuestServer->QueryInterface(23));
            iRPGObj* const levelRecord = levelService->GetObjByNamee(levelText.m_value, true);
            if (levelRecord) intResult = static_cast<unsigned int>(levelRecord->GetSkillValue(RPGSURVIVE_PERIODS));
        }
        break;
    }
    case EXT_GetLogFaceDir:
    {
        zProfMngr* logOwner = self->ProfMngr();
        if (argument2Bits == 0u)
            {
            intResult = logOwner->GetLogFaceDir(static_cast<int>(argument1Bits));
            break;
        }
        const int timeValue = static_cast<int>(logOwner->GetRealLogNum(static_cast<int>(argument1Bits)));
        intResult = static_cast<unsigned int>(timeValue);
        const int seconds = timeValue/1000;
        const int milliseconds = timeValue%1000;
        {
            const STRING formatted = milliseconds == 0
                ? Printf("%d", static_cast<unsigned int>(seconds))
                : Printf("%d_%d", static_cast<unsigned int>(seconds), static_cast<unsigned int>(milliseconds));
            stringResultRef = formatted;
        }
        returnsString = true;
        break;
    }
    case EXT_SETGAMMAITEMDISABLED:
    {
        iRPGFrameWork* objectRegistry = m_pRPGFrameWork;
        objectRegistry->SetGammaItemDisabled(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits), static_cast<int>(argument3Bits));
        break;
    }
    case EXT_GETCURCADR:
        intResult = static_cast<unsigned int>(static_cast<iVidCopier*>(self)->GetCurCadr(static_cast<int>(argument1Bits)));
        break;
    case EXT_SETVIDREDUCESCALE:
        if (static_cast<int>(argument1Bits) >= 4096)
            DebugAssert(5u, "var1 < Z_MAX_VID_NUM", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 476u);
        vidReduceScale[argument1] = argument2;
        break;
    case EXT_GETVIDREDUCESCALE:
        if (static_cast<int>(argument1Bits) >= 4096)
            DebugAssert(5u, "var1 < Z_MAX_VID_NUM", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 482u);
        intResult = static_cast<unsigned int>(vidReduceScale[argument1]);
        break;
    case EXT_GETQUESTSNUM:
    {
        iPlayer* const player = static_cast<iPlayer*>(self);
        if (player->GetArcadeMod_() == 0)
            {
            iQuestServer* questOwner = m_pQuestServer;
            intResult = static_cast<unsigned int>(questOwner->GetQuestNum());
        }
        break;
    }
    case EXT_GETQUESTSDIRTYSTATE:
    {
        iQuestServer* questOwner = m_pQuestServer;
        intResult = questOwner->GetDirtyState()?1u:0u;
        break;
    }
    case EXT_GETSUBQUESTSNUM:
    {
        iPlayer* const player = static_cast<iPlayer*>(self);
        if (player->GetArcadeMod_() == 0)
            {
            iQuestServer* questOwner = m_pQuestServer;
            intResult = static_cast<unsigned int>(questOwner->GetSubQuestNum(static_cast<int>(argument1Bits+1u)));
        }
        break;
    }
    case EXT_CONVERFROMCARTESIAN:
        if (argument3Bits == 1u)
            intResult = static_cast<unsigned int>(ScriptExecCartesianAngle(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits)));
        else if (argument3Bits == 2u)
            intResult = static_cast<unsigned int>(ScriptExecCartesianLength(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits)));
        else
            DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 498u);
        break;
    case EXT_KILLOBJECTS:
    {
        self->m_itemTaker->KillObjects(static_cast<int>(argument1Bits));
        break;
    }
    case EXT_GETSHOPITEMSNUM:
    {
        iShopDlgMenu* const shop = m_pShopDlgMenu;
        intResult = static_cast<unsigned int>(shop->GetItemsNum());
        break;
    }
    case EXT_GETSHOWNSHOPITEMSNUM:
    {
        iShopDlgMenu* const shop = m_pShopDlgMenu;
        intResult = static_cast<unsigned int>(shop->GetListItemsNum());
        break;
    }
    case EXT_GETFIRSTSHOWNITEM:
    {
        iShopDlgMenu* const shop = m_pShopDlgMenu;
        intResult = static_cast<unsigned int>(shop->GetFirstShownItemNum());
        break;
    }
    case EXT_SHOPLISTPAGEUP:
    {
        iShopDlgMenu* const shop = m_pShopDlgMenu;
        shop->PageUp();
        break;
    }
    case EXT_SHOPLISTPAGEDOWN:
    {
        iShopDlgMenu* const shop = m_pShopDlgMenu;
        shop->PageDown();
        break;
    }
    case EXT_GETABSDIR:
    {
        const int directionCount = static_cast<iVidCopier*>(self)->GetVidDirectionsNum(static_cast<int>(argument1Bits));
        if (directionCount != 0)
            {
            const int scaledDirection = static_cast<int>(argument2Bits<<8);
            intResult = static_cast<unsigned int>(scaledDirection/directionCount)&0xFFu;
        }
        break;
    }
    case EXT_GETRANDBONUSID:
    {
        iCampaign* bonusOwner = m_pCampaign.m_object;
        if (bonusOwner) intResult = static_cast<unsigned int>(bonusOwner->GetBonusBaseId(static_cast<int>(argument1Bits)));
        break;
    }
    case EXT_SETLEVELNUM:
    {
        iShopDlgMenu* const shop = m_pShopDlgMenu;
        shop->SetCurLevel(static_cast<int>(argument1Bits));
        iRPGImplant::generationSequence += argument1Bits;
        break;
    }
    case EXT_GET_MONST_GENERATORS_CNT:
    {
        unsigned int generatorCounts[4096];
        for (unsigned int i = 0u;i<4096u;++i)generatorCounts[i] = 0u;
        SPRITE_COLLECTOR* const spriteIterator =
            ::Sprites;
        SPRITE* sprite = spriteIterator->FirstAllSprite();
        while (sprite)
            {
            if ((sprite->RuntimeFlags()&0x000E0000u) != 0x000C0000u)
                {
                unsigned int cumulativeGeneratorMatches = 0u;
                for (unsigned int blockOffset = 0u;blockOffset<64u;blockOffset += 16u)
                    {
                    EX_SPRITE_DATA* generatorOwner = sprite->ExtraData();
                    if (!generatorOwner)continue;
                    const unsigned int laneIndex = blockOffset>>4u;
                    unsigned int listCount = static_cast<unsigned int>(generatorOwner->actionLane[laneIndex].m_count);
                    if (listCount == 0u)continue;
                    const ACT* const listData = generatorOwner->actionLane[laneIndex].m_data;
                    if (static_cast<unsigned int>(listData[listCount-1u].command) == static_cast<unsigned int>(ACT_STOP_STACK))continue;

                    const int spriteX = TruncateFloatToInt32(sprite->X());
                    const int spriteY = TruncateFloatToInt32(sprite->Y());
                    listCount = static_cast<unsigned int>(
                        sprite->ActionStack(static_cast<int>(laneIndex)).m_count);
                    unsigned int commandFlags = 0u;
                    for (unsigned int remaining = listCount;remaining != 0u;--remaining)
                        {
                        const ACT& record =
                            sprite->ActionStack(static_cast<int>(laneIndex)).m_data[remaining-1u];
                        const unsigned int kind = static_cast<unsigned int>(record.command);
                        const int value = record.arg1;
                        if (kind == static_cast<unsigned int>(ACT_BUILD_UNIT))
                            {
                            if ((commandFlags&4u) == 0u)
                                {
                                commandFlags |= 1u;
                                if (value>0)
                                    {
                                    const bool* const enabledTable = g_abMonsterVid;
                                    if (enabledTable[value])
                                        {
                                        ++generatorCounts[static_cast<unsigned int>(value)];
                                        ++cumulativeGeneratorMatches;
                                    }
                                }
                            }
                        }
                            else if (kind == static_cast<unsigned int>(ACT_GOTO_STACK))
                            {
                            if ((commandFlags&4u) == 0u)commandFlags |= 2u;
                        }
                            else if (kind == static_cast<unsigned int>(ACT_STOP_STACK))
                            {
                            commandFlags |= 4u;
                        }
                    }
                    if ((commandFlags&3u) != 3u)
                        {
                        intResult += cumulativeGeneratorMatches;
                    }
                        else
                        {
                        iLogConsole* logger = ::gpLogCons;
                        VID* const objectRef = sprite->Vid();
                        (void)logger->LogStr(LT_INFO, "zVSTAT: cycled generator! %d %d %d", static_cast<unsigned int>(spriteX), static_cast<unsigned int>(spriteY), static_cast<unsigned int>(objectRef->nVid));
                        (void)sprite->Action(ACT_ADD_ITEM, -666, 0, 0);
                    }
                }
                if (cumulativeGeneratorMatches == 0u)
                    {
                    VID* const objectRef = sprite->Vid();
                    const unsigned int objectId = static_cast<unsigned int>(objectRef->nVid);
                    const bool* const enabledTable = g_abMonsterVid;
                    if (enabledTable[objectId])cumulativeGeneratorMatches = 666u;
                }
                if (cumulativeGeneratorMatches != 0u)
                    {
                    iLogConsole* logger = ::gpLogCons;
                    VID* const objectRef = sprite->Vid();
                    (void)logger->LogStr(LT_INFO, "zVSTAT:scriptgenerator. vid %d. Coord -  %d, %d, %d. num = %d", static_cast<unsigned int>(objectRef->nVid), static_cast<unsigned int>(TruncateFloatToInt32(sprite->X())), static_cast<unsigned int>(TruncateFloatToInt32(sprite->Y())), static_cast<unsigned int>(TruncateFloatToInt32(sprite->Z())), cumulativeGeneratorMatches);
                }
            }
            sprite = spriteIterator->NextAllSprite();
        }
        iLogConsole* logger = ::gpLogCons;
        for (unsigned int generatorId = 0u;generatorId<4096u;++generatorId)
            {
            if (generatorCounts[generatorId] != 0u)
                (void)logger->LogStr(LT_INFO, "zVSTAT_MMONSTERSCNT++, vid: %d - %d", generatorId, generatorCounts[generatorId]);
        }
        break;
    }
    case EXT_SETALLQUESTSUNACTUAL:
    {
        iQuestServer* questOwner = m_pQuestServer;
        questOwner->SetAllQuestsUnActual();
        break;
    }
    case EXT_GETBRIEFINGTEXT:
        DebugAssert(2u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 872u);
        break;
    case EXT_GETSURVIVEMODE:
    {
        iPlayer* const player = static_cast<iPlayer*>(self);
        intResult = static_cast<unsigned int>(player->GetArcadeMod_())&1u;
        break;
    }
    case EXT_FORDEBUG:
        if (argument1Bits == 1u)
        {
            zString* const debugList = &self->m_debugText;
            debugList->operator = ("aaaaaaaaaaaaaaaaaa");
            self->m_messageAvailableTick = 0u;
            self->m_messageId = 1u;
        }
        else if (argument1Bits == 2u)
        {
            DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 906u);
        }
        break;
    case EXT_GETDRAWSHADOW:
        intResult = Graph->deviceState&1u;
        break;
    case EXT_SETDRAWSHADOW:
    {
        Graph->SetDrawShadow(BitsToSigned32(argument1Bits));
        break;
    }
    case EXT_GETHOTKEY:
        GetHotKey(stringResultRef, static_cast<int>(argument1Bits), static_cast<int>(argument2Bits));
        returnsString = true;
        break;
    case EXT_GETKEYWAITMODE:
        intResult = zInputMap::m_bKeyWaitAct != 65535u?1u:0u;
        break;
    case EXT_RK_DEFAULT:
        (void)zInputMap::Load("input.cfg");
        break;
    case EXT_SET_TEXT_DELAY:
        ::g_pDlgSrv->m_textDurationPercent = argument1Bits;
        break;
    case EXT_SHOPSCROLL:
    {
        iShopDlgMenu* const shop = m_pShopDlgMenu;
        shop->PageScroll(static_cast<int>(argument1Bits));
        break;
    }
    case EXT_SaveOptToFile:
    {
        zProfLdr* const profileState = ::g_pProfLdr;
        (void)profileState->ExpRegToFile(true);
        break;
    }
    case EXT_DELETEPROFILE:
    {
        zProfLdr* const profileState = ::g_pProfLdr;
        (void)profileState->DeleteProfile2(static_cast<int>(argument1Bits));
        break;
    }
    case EXT_SetCtrlWheelsCar:
        static_cast<iVidCopier*>(self)->SetCtrlWheelsCar(reinterpret_cast<SPRITE*>(U32ToPtr(argument1Bits)));
        break;
    case EXT_GetLogText:
    {
        zProfMngr* logOwner = self->ProfMngr();
        const char* text = logOwner->GetLogText(static_cast<int>(argument1Bits));
        stringResultRef = text;
        returnsString = true;
        break;
    }
    case EXT_GetLogsNum:
    {
        zProfMngr* logOwner = self->ProfMngr();
        intResult = logOwner->GetLogsNum();
        break;
    }
    case EXT_RefreshShop:
    {
        iShopDlgMenu* const shop = m_pShopDlgMenu;
        shop->RefreshAll();
        iHeroInvMenu* const heroMenu = m_pHeroInvMenu;
        heroMenu->InitOnShow();
        break;
    }
    case EXT_SET_TEXT_DELAY_STRS13:
    {
        DialogSrv* const dialogOwner = ::g_pDlgSrv;
        dialogOwner->m_oneLineDuration = argument1Bits;
        dialogOwner->m_twoLineDuration = argument2Bits;
        dialogOwner->m_threeLineDuration = argument3Bits;
        break;
    }
    case EXT_SET_TEXT_DELAY_STRS45:
    {
        DialogSrv* const dialogOwner = ::g_pDlgSrv;
        dialogOwner->m_fourLineDuration = argument1Bits;
        dialogOwner->m_fiveLineDuration = argument2Bits;
        break;
    }
    case EXT_PlaceTip:
    {
        SPRITE* const sprite = reinterpret_cast<SPRITE*>(U32ToPtr(argument1Bits));
        if (!sprite)DebugAssert(5u, "var1", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1085u);
        if (sprite->Action(ACT_GET_TEXT, 0, 0, 0) == 0)break;
        const VECTOR2& camera = Map->ShiftXY();
        int screenX = ScriptExecFtolSubFloat(sprite->X(), camera.x);
        const int depth = TruncateFloatToInt32(sprite->Z());
        int screenY = ScriptExecFtolFloatMinusRoundedIntMinusFloat(sprite->Y(), depth, camera.y);
        GRAPH* const graph = Graph;
        const int maxX = ScriptExecFtolFloatMinusIntMinusFloat(graph->viewRight,
            static_cast<int>(ReadU32(sprite, 116u)), (10.0f));
        if (screenX>maxX)screenX = maxX;
        if (screenX<10)screenX = 10;
        if (screenY<15)screenY = 15;
        const int worldX = ScriptExecFtolIntPlusFloat(screenX, camera.x);
        const int worldY = ScriptExecFtolTwoIntsPlusFloat(depth, screenY, camera.y);
        (void)sprite->Action(ACT_CHANGE_COOR, worldX, worldY, depth);
        break;
    }
    case EXT_GetProgVer:
        intResult = g_iZmDebug;
        break;
    case EXT_ReloadDB:
        static_cast<iVidCopier*>(self)->ReloadDB();
        break;
    case EXT_GetQuestAccomplished:
    {
        iQuestServer* questOwner = m_pQuestServer;
        iRPGQuest* quest = questOwner->GetQuest(static_cast<int>(argument1Bits+1u), static_cast<int>(argument2Bits), true, true);
        if (quest)
            {
            iRPGObj* typedQuest = static_cast<iRPGObj*>(quest->QueryInterface(9));
            intResult = static_cast<unsigned int>(typedQuest->GetSkillValue(RPGQUEST_ACCOMPLISHED));
        }
        break;
    }
    case EXT_ShowItemInfoById:
    {
        iRPGObj* object = 0;
        if (static_cast<int>(argument1Bits)>0)
            {
            iRPGFrameWork* objectRegistry = m_pRPGFrameWork;
            object = objectRegistry->GetItemById(static_cast<int>(argument1Bits));
        }
        iHeroInvMenu* const heroMenu = m_pHeroInvMenu;
        heroMenu->ControlItemInfo(object, Map->InputState());
        break;
    }
    case Ext_ToggleLogCons:
    {
        iLogConsole* const logger = ::gpLogCons;
        logger->ToggleShow();
        break;
    }
    case EXT_GetNamedMonst:
        if (static_cast<int>(argument1Bits) >= 0 && static_cast<int>(argument1Bits) <= 9)
        {
            intResult = Map->State00C(argument1Bits);
        }
        break;
    case EXT_GetCurProf:
    {
        zProfLdr* const profileState = ::g_pProfLdr;
        intResult = static_cast<unsigned int>(profileState->m_currentProfile);
        break;
    }
    case EXT_GetMaxLevNum:
    {
        const unsigned int profileId = argument1Bits;
        const unsigned int survivalMode = argument2Bits != 0u?1u:0u;
        zProfLdr* const profileState = ::g_pProfLdr;
        intResult = profileState->GetMaxLevNum(static_cast<int>(profileId), survivalMode != 0u);
        break;
    }
    case EXT_GetMenuUnderCursor:
    {
        intResult = PtrToU32(Map->MenuSelectedSprite());
        break;
    }
    case EXT_GetBlinkStatebar:
    {
        iRPGHero* const hero = m_refLegs.m_object;
        if (hero) intResult = hero->GetUseMedTime() != 0?1u:0u;
        break;
    }
    case EXT_SetHeroPDId:
    {
        iPlayer* const player = static_cast<iPlayer*>(self);
        player->SetHeroPDId(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits), argument3Bits != 0u);
        break;
    }
    case EXT_GetHeroPDId:
    {
        iPlayer* const player = static_cast<iPlayer*>(self);
        intResult = static_cast<unsigned int>(player->GetHeroPDId(static_cast<int>(argument1Bits)));
        break;
    }
    case EXT_SetHeroPDClr:
    {
        iPlayer* const player = static_cast<iPlayer*>(self);
        player->SetHeroPDClr(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits));
        break;
    }
    case EXT_GetHeroPDClr:
    {
        iPlayer* const player = static_cast<iPlayer*>(self);
        intResult = static_cast<unsigned int>(player->GetHeroPDClr(static_cast<int>(argument1Bits)));
        break;
    }
    case EXT_SetMainMenu:
        self->m_loadHeroFromMaxSave = 1u;
        break;
    case Ext_SetFulldamagedArmorCost:
        zRPGArmor::m_iFullDmgCost = BitsToSigned32(argument1Bits);
        break;
    case Ext_GetObjectId:
        if (argument1Bits != 0u)
        {
            SPRITE* const object = reinterpret_cast<SPRITE*>(U32ToPtr(argument1Bits));
            EX_SPRITE_DATA* const linked = object->ExtraData();
            intResult = linked?static_cast<unsigned int>(linked->objectId):0xFFFFFFFFu;
        }
        break;
    case EXT_SetNetRespItem:
    {
        self->m_itemTaker->SetNetRespItem(static_cast<int>(argument1Bits), reinterpret_cast<SPRITE*>(U32ToPtr(argument2Bits)));
        break;
    }
    case EXT_GetNetRespItem:
    {
        specialResultRef = self->m_itemTaker->GetNetRespItem(static_cast<int>(argument1Bits));
        break;
    }
    case EXT_SwitchHeroCS:
        SwitchHeroCS(argument1Bits != 0u);
        break;
    case EXT_SetSurvNetCoeff:
    {
        const unsigned int coefficientSelector = argument1Bits;
        const unsigned int coefficientValue = argument2Bits;
        if (coefficientSelector == SNC_MONSTCNT) zSurvive::ms_MonstCntCoeff = BitsToSigned32(coefficientValue);
        else if (coefficientSelector == SNC_BONUSCNT) zSurvive::ms_EmptyBonusWeightCoeff = BitsToSigned32(coefficientValue);
        else DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1305u);
        break;
    }
    case EXT_GetHallRecCnt:
    {
        const unsigned int hallType = argument1Bits;
        zRecordDB* const hallOfFame = RecordDatabase;
        intResult = hallOfFame->GetRecCnt(static_cast<int>(hallType));
        break;
    }
    case EXT_GetHallData:
    {
        const int hallType = static_cast<int>(argument1Bits);
        const int recordIndex = static_cast<int>(argument2Bits);
        const int selector = static_cast<int>(argument3Bits);
        zRecordDB* const hallOfFame = RecordDatabase;
        const AS_RECORD* buf = hallOfFame->GetRecData(static_cast<int>(argument1Bits), static_cast<int>(argument2Bits));
        if (!buf)
            {
            DebugAssert(5u, "buf", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1319u);
            iLogConsole* logger = ::gpLogCons;
            (void)logger->LogStr(LT_ERR, "error in GetRecData %d, %d", static_cast<unsigned int>(hallType), static_cast<unsigned int>(recordIndex));
            break;
        }
        const unsigned int weaponScores = ScriptExecVectorDwordCount(buf->m_weaponScores);
        const unsigned int monsterCounts = ScriptExecVectorDwordCount(buf->m_weaponKills);
        const unsigned int pairCount = weaponScores>monsterCounts?weaponScores:monsterCounts;
        returnsString = true;
        if (selector == 7)
            {
            returnsString = false;
            intResult = buf->m_cheater != 0u?1u:0u;
        }
            else if (selector == 5 || selector == 6)
            {
            const char* text = selector == 5?buf->m_playerName.data:buf->m_recordTimestamp.data;
            if (!text)text = "";
            stringResultRef = text;
        }
            else if (selector <= 3)
            {
            if (selector<0)DebugAssert(5u, "var3 >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1345u);
            if (selector == 2)
                {
                    returnsString = false;
                    intResult = static_cast<unsigned int>(buf->m_elapsedSeconds);
                }
            else if (selector == 3) ScriptExecAssignFormattedHallValue(stringResultRef, "%u", buf->m_checksum);
            else
                {
                const unsigned int value = ReadU32(buf, 16u+static_cast<unsigned int>(selector)*4u);
                ScriptExecAssignFormattedHallValue(stringResultRef, "%d", value);
            }
        }
            else if (selector == 8)
            {
            returnsString = false;
            intResult = weaponScores;
        }
            else if (selector >= 9 && selector<static_cast<int>(9u+pairCount*2u))
            {
            returnsString = false;
            const unsigned int delta = static_cast<unsigned int>(selector-9);
            const unsigned int index = delta>>1;
            if ((delta&1u) == 0u)
                {
                if (index<weaponScores)intResult = static_cast<unsigned int>(buf->m_weaponScores.begin[index]);
            }
                else
                {
                if (index<monsterCounts)intResult = static_cast<unsigned int>(buf->m_weaponKills.begin[index]);
            }
        }
            else
            {
            DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1382u);
            returnsString = false;
        }
        break;
    }
    case EXT_AddHallData:
        if (argument1Bits == 0u)
        {
            self->m_hallRecord.m_totalScore = static_cast<int>(argument2Bits);
            self->m_hallRecord.m_weaponScores.end = self->m_hallRecord.m_weaponScores.begin;
            self->m_hallRecord.m_weaponKills.end = self->m_hallRecord.m_weaponKills.begin;
        }
        else if (argument1Bits == 1u)
        {
            self->m_hallRecord.m_monsterKills = static_cast<int>(argument2Bits);
            self->m_hallRecord.m_elapsedSeconds = static_cast<int>(argument3Bits);
        }
        else if (argument1Bits == 2u)
        {
            ScriptExecAppendHallDword(&self->m_hallRecord.m_weaponScores, argument2Bits);
            ScriptExecAppendHallDword(&self->m_hallRecord.m_weaponKills, argument3Bits);
        }
        else if (argument1Bits == 3u)
        {
            self->m_hallRecord.m_cheater = argument3Bits != 0u?1u:0u;
            zRecordDB* const hallOfFame = RecordDatabase;
            hallOfFame->AddRecord(static_cast<int>(argument2Bits), self->m_hallRecord);
        }
        break;
    case EXT_ClearHallData:
    {
        const int hallType = static_cast<int>(argument1Bits);
        if (hallType >= 0 && hallType<3)
            {
            zRecordDB* const hallOfFame = RecordDatabase;
            hallOfFame->ClearTable(hallType);
        }
        break;
    }
    case EXT_PopPushLifeTIme:
    {
        ScriptExecMapControl& lifetimeValues = m_lifetimeValues;
        if (argument1Bits == 1u)
            {
            lifetimeValues.insert(BitsToSigned32(argument2Bits), BitsToSigned32(argument3Bits));
        }
            else if (argument1Bits == 2u)
            {
            intResult = static_cast<unsigned int>(lifetimeValues.find(BitsToSigned32(argument2Bits)));
        }
            else
            {
            DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1443u);
        }
        break;
    }
    case EXT_IsDebugMode:
    {
        intResult = static_cast<unsigned int>(::Const->DebugMode);
        break;
    }
    case Ext_ShowDialogsSub:
    {
        DialogSrv* const dialogOwner = ::g_pDlgSrv;
        dialogOwner->m_showDialogSubtitles = argument1Bits != 0u?1u:0u;
        break;
    }
    case EXT_CheatHappen:
    {
        iRPGObj* const eventSink = m_refBody.m_object;
        if (eventSink) (void)eventSink->IncSkillValue(RPGHERO_Cheater, 1);
        break;
    }
    case EXT_RandomNetDbg:
    {
        const unsigned int maxValue = argument1Bits;
        const unsigned int networkDebugId = argument2Bits;
        networkTransmissionId = networkDebugId;
        intResult = static_cast<unsigned int>(Random(static_cast<int>(maxValue)));
        networkTransmissionId = 0u;
        break;
    }
    case EXT_GetNameById:
    {
        returnsString = true;
        iRPGFrameWork* objectRegistry = m_pRPGFrameWork;
        const unsigned int objectId = argument1Bits;
        if (objectRegistry && objectId != 0u)
            {
            iRPGObj* object = objectRegistry->GetItemById(static_cast<int>(objectId));
            if (object)
                {
                const char* name = object->GetName();
                stringResultRef = name;
            }
        }
        break;
    }
    case EXT_GetStrVal:
    {
        returnsString = true;
        const unsigned int stringValueId = argument1Bits;
        const unsigned int stringValue = argument2Bits;
        iRPGFrameWork* objectRegistry = m_pRPGFrameWork;
        iTextDB* stringService = objectRegistry->AsTextDB();
        const char* text = stringService->GetStringValue(static_cast<int>(stringValueId), static_cast<int>(stringValue));
        if (text) stringResultRef = text;
        break;
    }
    default:
        DebugAssert(4u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\ScriptExec\\zScriptExec.cpp", 1487u);

        break;
    }

    intResultRef = BitsToSigned32(intResult);
    return returnsString?1u:0u;
}

unsigned int* __cdecl zScriptExec_DwordRangeCopy(
    const unsigned int* first,const unsigned int* last,unsigned int* destination);
unsigned int* __cdecl zScriptExec_DwordRangeCopy(
    const unsigned int* sourceBegin, const unsigned int* sourceEnd, unsigned int* destination)
{
    unsigned int* writeCursor = destination;
    while (sourceBegin != sourceEnd)
    {
        *writeCursor = *sourceBegin;
        ++sourceBegin;
        ++writeCursor;
    }
    return writeCursor;
}

