#include "zm/Utils/zLogConsole.h"
#include "zm/Utils/zStaticStorage.h"
#include "zm/Utils/zVectorOperations.h"
#include "../../engine_globals.h"
#include "basetype.h"
#include "mylib.h"
#include "zItemTaker.h"
#include "myerror.h"
#include "resread.h"
#include "../../sprite.h"
#include "../../sprite_collector.h"
#include "../../vid.h"
#include <new>
#include <cstring>
#include "map.h"
#include "zm/Utils/String/zString.h"
#include "zm/Menu/zSpriteInventory.h"
#include "zm/Quest/zQuestServer.h"


namespace
{
StaticObjectStorage<PTR_SPRITE[500]> netResponseSlots;
unsigned char netResponseSlotsDestroyed = 0;
}

PTR_SPRITE (&zItemTaker::m_aNetRespItems)[500] =
    *reinterpret_cast<PTR_SPRITE(*)[500]>(&netResponseSlots.memory);

int zItemTaker::m_iNetRespCnt = 0;
bool zItemTaker::m_bNeedVidDirty = true;
int zItemTaker::m_aiNeededVid[4096];


PTR_SPRITE::PTR_SPRITE()
{
    m_value = 0;
}


void zItemTaker::SetRPGFrameWork(iRPGFrameWork* frameWork)
{
    m_pRPGFrameWork = frameWork;
}


void zItemTaker::SetQuestServer(iQuestServer* questServer)
{
    m_pQuestServer = questServer;
}


void zItemTaker::SetPlayer(iPlayer* player)
{
    m_pPlayer = player;
}


void zItemTaker::DeletePointerToSprite(SPRITE* spriteToClear)
{
    int i = 0;
    if (m_iNetRespCnt <= 0)
        return;

    PTR_SPRITE* slot = m_aNetRespItems;
    do
    {
        if (slot->m_value == spriteToClear)
        {
            SPRITE* previousSprite = slot->m_value;
            if (previousSprite != 0)
                previousSprite->Release();
            slot->m_value = 0;
        }
        ++i;
        ++slot;

    }
    while (i < m_iNetRespCnt);
}


void zItemTaker::SetNetRespItem(int _iNum, SPRITE* value)
{
    int& count = m_iNetRespCnt;
    const int requiredCount = AddSigned32Wrap(_iNum, 1);

    if (count < requiredCount)
    {
        PTR_SPRITE* slot = m_aNetRespItems + count;
        int remaining = AddSigned32Wrap(requiredCount, MultiplySigned32Wrap(-1, count));
        do
        {
            SPRITE* old = slot->m_value;
            if (old != 0)
                old->Release();
            slot->m_value = 0;
            ++slot;
            remaining = AddSigned32Wrap(remaining, -1);
        }
        while (remaining != 0);
        count = requiredCount;
    }

    PTR_SPRITE* target = m_aNetRespItems + _iNum;
    if (target->m_value != 0)
    {
        DebugAssert(5,
                              "m_aNetRespItems[_iNum] == NULL",
                              "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp",
                              664);
    }

    if (value != 0)
        value->AddReference();

    SPRITE* old = target->m_value;
    if (old != 0)
        old->Release();

    target->m_value = value;
}


SPRITE* zItemTaker::GetNetRespItem(int _iNum)
{
    if (_iNum >= m_iNetRespCnt)
    {
        DebugAssert(5,
                              "_iNum < m_iNetRespCnt",
                              "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp",
                              672);
    }
    return m_aNetRespItems[_iNum].m_value;
}


void zItemTaker::SetItemTakeDist(float distance)
{
    m_itemTakeDist = distance;
}








int zItemTaker::iRelease()
{
    delete this;
    return 0;
}


unsigned int __cdecl zItemTaker::RegisterNetResponseSlots()
{
    as2::core::constructArray(m_aNetRespItems, sizeof(PTR_SPRITE), 500u,
        [](void* item) { ::new (item) PTR_SPRITE; });
    return RegisterCrtAtExit(&zItemTaker::DestroyNetResponseSlots);
}


iRPGObj* zItemTaker::NextItemToTake()
{
    m_takeIndex = AddSigned32Wrap(m_takeIndex, 1);
    if (!m_itemsBegin)
        return 0;
    const unsigned int itemStorageBytes = PtrToU32(m_itemsEnd)-PtrToU32(m_itemsBegin);
    const unsigned int itemCount = (itemStorageBytes>>2)|((itemStorageBytes&0x80000000u)?0xC0000000u:0u);
    return static_cast<unsigned int>(m_takeIndex)<itemCount ? m_itemsBegin[m_takeIndex] : 0;
}


bool zItemTaker::IsItemFarToTake(iRPGObj* object)
{
    SPRITE* const sprite = object->GetGrndSpr();
    return !sprite || IsSpriteFarToTake(sprite);
}


void* zItemTaker::QueryInterface(int interfaceId) const
{
    if (interfaceId == 32)
        return const_cast<iItemTaker*>(static_cast<const iItemTaker*>(this));
    return zRefObject::QueryInterface(interfaceId);
}


void __cdecl zItemTaker::DestroyNetResponseSlots()
{
    unsigned char& g = netResponseSlotsDestroyed;
    if ((g&1u) == 0u)
    {
        g = (unsigned char)(g|1u);
        as2::core::destroyArray(m_aNetRespItems, sizeof(PTR_SPRITE), 500u, [](void* item){ static_cast<PTR_SPRITE*>(item)->~PTR_SPRITE(); });
    }
}


SPRITE* zItemTaker::CurSpriteToTake()
{
    const unsigned int spriteStorageBytes = PtrToU32(m_spritesEnd)-PtrToU32(m_spritesBegin);
    const unsigned int spriteCount = m_spritesBegin ?
        (spriteStorageBytes>>2)|((spriteStorageBytes&0x80000000u)?0xC0000000u:0u) : 0u;
    return static_cast<unsigned int>(m_takeIndex)<spriteCount ? m_spritesBegin[m_takeIndex] : 0;
}


bool zItemTaker::DeleteGroundSprite(iRPGObj* _pItem, bool removeSprite)
{
    if (!_pItem)
        DebugAssert(5u, "_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 133u);
    SPRITE* const sprite = _pItem->GetGrndSpr();
    const bool result = DeleteGroundSprite(sprite, removeSprite);
    _pItem->SetGrndSpr(0);
    return result;
}


bool zItemTaker::IsNeededVid(int _iVidNum)
{
    if (_iVidNum<0 || _iVidNum >= 4096)
        DebugAssert(5u, "_iVidNum >= 0 && _iVidNum < Z_MAX_VID_NUM", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 261u);
    if (m_aiNeededVid[_iVidNum] == -1)
        m_aiNeededVid[_iVidNum] = m_pRPGFrameWork->FirstObject(7, _iVidNum, 0) ? 1 : 0;
    return m_aiNeededVid[_iVidNum] == 1;
}


void __cdecl GetNew_zItemTaker(iItemTaker** outObject)
{
    zItemTaker* object = static_cast<zItemTaker*>(CrtNew(72u));
    if (object)
        object = new (object) zItemTaker();
    *outObject = object ? static_cast<iItemTaker*>(object) : 0;
}


zItemTaker::zItemTaker()
    : zRefObject(),
      m_itemsBegin(0), m_itemsEnd(0), m_itemsCapacityEnd(0),
      m_spritesBegin(0), m_spritesEnd(0), m_spritesCapacityEnd(0),
      m_takeIndex(0), m_itemTakeDist(0.0f),
      m_pPlayer(0), m_pRPGFrameWork(0), m_pQuestServer(0)
{
    const unsigned char hi = static_cast<unsigned char>(PtrToU32(this) >> 24);
    m_itemsVectorTag = hi;
    m_spritesVectorTag = hi;
    if (m_bNeedVidDirty)
    {
        for (unsigned int i = 0;i<4096;++i)
            m_aiNeededVid[i] = -1;
    }
    m_bNeedVidDirty = false;
}


zItemTaker::~zItemTaker()
{
    CrtFreePointer(m_spritesBegin);
    m_spritesBegin = 0;
    m_spritesEnd = 0;
    m_spritesCapacityEnd = 0;

    CrtFreePointer(m_itemsBegin);
    m_itemsBegin = 0;
    m_itemsEnd = 0;
    m_itemsCapacityEnd = 0;
}


void zItemTaker::GetGrndSpriteCoor(SPRITE* sprite, int& outX, int& outY, int& outZ)
{
    if (!sprite)
    {
        DebugAssert(5u, "sprite", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 612u);
        outX = 0;
        outY = 0;
        outZ = 0;
        return;
    }
    const VECTOR& position = sprite->Coor();
    outX = TruncateFloatToInt32(position.x);
    outY = TruncateFloatToInt32(position.y);
    outZ = TruncateFloatToInt32(position.z);
}

void zItemTaker::CreateIdsForSprites()
{
    SPRITE_COLLECTOR* manager = ::Sprites;
    int spriteIndex = manager->allSprites.m_count-1;

    for (;;)
    {
        SPRITE* item = 0;
        while (spriteIndex >= 0)
        {
            const int spriteCount = manager->allSprites.m_count;
            if (spriteIndex >= spriteCount) break;
            if (manager->allSprites.m_data[spriteIndex] != 0)
            {
                item = manager->allSprites.m_data[spriteIndex];
                break;
            }
            --spriteIndex;
        }

        if (item == 0) return;
        CreateIdForSprite(item);
        manager = ::Sprites;
        --spriteIndex;
    }
}


iRPGObj* zItemTaker::SetNewItemToSprite(iRPGObj* _pBaseObj, SPRITE* _pSprite)
{
    if (!_pBaseObj)
        DebugAssert(5u, "_pBaseObj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 529u);
    if (!_pSprite)
        DebugAssert(5u, "_pSprite", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 530u);

    if (_pBaseObj->IsBaseObject())
    {
        _pBaseObj = _pBaseObj->GetObjCopy();
        if (_pBaseObj->GetGameObj() == 3 && m_pPlayer && (m_pPlayer->GetArcadeMod_()&2))
        {
            const int randomDropCount = _pBaseObj->GetSkillValue(RPGAMMO_DMGRND);
            if (randomDropCount)
                _pBaseObj->SetNumInCell(randomDropCount);
        }
    }

    _pBaseObj->SetGrndSpr(_pSprite);
    _pSprite->SetObjectID(static_cast<int>(_pBaseObj->GetExtType()));
    return _pBaseObj;
}


bool zItemTaker::DeleteGroundSprite(SPRITE* _pSprite, bool removeMode)
{
    if (!_pSprite)
    {
        DebugAssert(5u, "_pSprite", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 144u);
        return false;
    }

    EX_SPRITE_DATA* const extra = _pSprite->ExData();
    if (extra && *extra->nameText.value != '\0' && m_pQuestServer)
    {
        STRING text = _pSprite->GetName();
        m_pQuestServer->CheckStateItemName(static_cast<CSI_ACTION>(0), text.m_value);
    }

    if (removeMode)
        _pSprite->ChangeAnimation(16);
    else
        delete _pSprite;
    return true;
}

void zItemTaker::DeleteOldItems()
{
    DebugLog("before");
    int responseCount = m_iNetRespCnt;
    for (int i = 0;i<responseCount;++i)
    {
        PTR_SPRITE& slot = m_aNetRespItems[i];
        if (slot.m_value)
            (void)slot.m_value->Release();
        slot.m_value = 0;
        responseCount = m_iNetRespCnt;
    }
    m_iNetRespCnt = 0;

    iRPGObj* item = m_pRPGFrameWork->FirstObject(3, -1, -1);
    while (item)
    {
        const bool active = item->GetGrndSpr() != 0;
        const bool special = item->IsBaseObject();
        if (active)
        {
            item->SetGrndSpr(0);
            if (!special)
                (void)m_pRPGFrameWork->ReleaseObject(item);
        }
        else if (!special && item->GetNumInCell() == 0)
        {
            (void)m_pRPGFrameWork->ReleaseObject(item);
        }
        item = m_pRPGFrameWork->NextObject();
    }
    DebugLog("after");
}


void zItemTaker::FindIdForSprite(SPRITE* sprite)
{
    if (!m_pRPGFrameWork)
        DebugAssert(5u, "m_pRPGFrameWork", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 323u);
    VID* const vid = sprite->Vid();
    const int vidId = vid->nVid;
    const unsigned int vidDirection = static_cast<unsigned int>(vid->RealDirection(sprite->Direction().value));
    iRPGObj* object = m_pRPGFrameWork->FirstObject(13, vidId, static_cast<int>(vidDirection));
    int matchingBaseIds[100];
    int matchingIdCount = 0;
    while (object)
    {
        matchingBaseIds[matchingIdCount++] = object->GetBaseId();
        if (m_aiNeededVid[vidId] == -1)
            m_aiNeededVid[vidId] = 1;
        object = m_pRPGFrameWork->NextObject();
    }
    if (matchingIdCount <= 0)
        return;
    int selectedBaseId = matchingBaseIds[0];
    if (matchingIdCount>1)
    {
        const unsigned int selectedIdIndex = static_cast<unsigned int>(Random(matchingIdCount-1));
        selectedBaseId = matchingBaseIds[selectedIdIndex];
    }
    sprite->SetObjectID(selectedBaseId);
}


bool zItemTaker::AddObjectToSprite(SPRITE* sprite, iRPGObj* _pObject, bool registerMessage)
{
    if (!_pObject)
        DebugAssert(4u, "_pObject", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 409u);
    if (!_pObject)
        return false;

    (void)_pObject->GetBaseId();
    const int objectId = _pObject->GetBaseId();
    if (registerMessage)
        (void)sprite->Action(ACT_ADD_ITEM, objectId, 0, 0);
    else
        sprite->SetObjectID(objectId);

    zString tag;
    tag = registerMessage ? "added" : "set";
    VID* const vid = sprite->Vid();
    iLogConsole* const log = ::gpLogCons;
    (void)log->LogStr(LT_INFO, "Item %s was %s to sprite with vid %d", _pObject->GetName(), static_cast<const char*>(tag), vid->nVid);
    return true;
}


bool zItemTaker::IsSpriteFarToTake(SPRITE* candidate)
{
    SPRITE* current = m_pPlayer->Flagman();
    if (!current || !candidate)
        return true;
    current = m_pPlayer->Flagman();
    const double dx = static_cast<double>(candidate->X())-current->X();
    const float dy = static_cast<float>(static_cast<double>(candidate->Y())-current->Y());
    const double ax = dx<0.0 ? -dx : dx;
    const double ay = dy<0.0f ? -static_cast<double>(dy) : static_cast<double>(dy);
    const double distance = ax>ay ? ax+ay*0.398000002f : ay+ax*0.398000002f;
    return distance>static_cast<double>(m_itemTakeDist);
}

void zItemTaker::DebugLog(const char* reportArg)
{
    unsigned int total = 0, active = 0, flagged = 0, passive = 0;
    iRPGObj* item = m_pRPGFrameWork->FirstObject(3, -1, -1);
    while (item)
    {
        ++total;
        if (item->GetGrndSpr()) ++active;
        if (item->IsBaseObject()) ++flagged;
        if (!item->IsBaseObject() && !item->HaveInv()) ++passive;
        item = m_pRPGFrameWork->NextObject();
    }
    iLogConsole* sink = ::gpLogCons;
    (void)sink->LogStr(static_cast<LOG_TYPE>(0u), "%s items in RPGFW: total=%d, base=%d withSpr=%d notBaseWithoutInv=%d" , reportArg, total, flagged, active, passive);
}

void zItemTaker::KillObjects(int handle)
{
    zString wanted;
    wanted.Format("h%d", handle);
    const char* const wantedName = static_cast<const char*>(wanted);
    iLogConsole* const logger = ::gpLogCons;
    logger->LogStr(LT_INFO, "KillObjects %s", wantedName);

    SPRITE_COLLECTOR* const manager = ::Sprites;
    SPRITE* sprite = manager->FirstAllSprite();
    while (sprite)
    {
        EX_SPRITE_DATA* const extra = sprite->ExData();
        if (extra && *extra->nameText.value)
        {
            bool matches;
            {
                const char* const wantedName = static_cast<const char*>(wanted);
                STRING actualName = sprite->GetName();
                matches = CompareCStrings(actualName.m_value, wantedName) == 0;
            }
            if (matches)
                delete sprite;
        }
        sprite = ::Sprites->NextAllSprite();
    }
}

bool zItemTaker::AddNameToSprite(SPRITE* _pSprite, const char* _pString)
{
    EX_SPRITE_DATA* const objectMeta = _pString ? _pSprite->ExData() : 0;
    const char* const objectName = objectMeta ? objectMeta->nameText.value : 0;
    if (!_pString || !objectMeta || !*objectName)
        DebugAssert(5u, "_pString && _pSprite->HaveName()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 353u);

    const unsigned int objectId = static_cast<unsigned int>(_pSprite->Vid()->nVid);
    const bool applyAll = m_pRPGFrameWork->GetGrndObject(static_cast<int>(objectId), -1) == 0;

    iRPGObj* candidates[50];
    unsigned int ItemsNum = 0u;
    const unsigned int len = CStringLength(_pString);
    unsigned int tokenStart = 0u;
    for (unsigned int pos = 0u;pos <= len;++pos)
    {
        if (pos != len && _pString[pos] != ' ')continue;
        if (pos>tokenStart)
        {
            char token[200];
            const unsigned int n = pos-tokenStart;
            for (unsigned int i = 0;i<n;++i)token[i] = _pString[tokenStart+i];
            token[n] = 0;
            iRPGObj* const candidate = m_pRPGFrameWork->GetObjByNamee(token, true);
            if (candidate)candidates[ItemsNum++] = candidate;
        }
        tokenStart = pos+1u;
    }

    unsigned char result = 0u;
    if (applyAll)
    {
        for (unsigned int i = 0;i<ItemsNum;++i)
            result = (unsigned char)(result | (unsigned char)AddObjectToSprite(_pSprite, candidates[i], true));
        return result;
    }
    if (ItemsNum == 0u)
    {
        iLogConsole* log = ::gpLogCons;
        (void)log->LogStr(static_cast<LOG_TYPE>(2u), "zItemTaker: no items for %s", _pString);
        return result;
    }
    const int num = (int)static_cast<unsigned int>(Random(static_cast<int>(ItemsNum-1u)));
    if (num >= (int)ItemsNum) DebugAssert(5u, "num < ItemsNum", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 396u);
    return AddObjectToSprite(_pSprite, candidates[num], false);
}

void zItemTaker::CreateIdForSprite(SPRITE* _pSprite)
{
    if (!_pSprite)
        DebugAssert(5u, "_pSprite", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 277u);

    STRING ownedName;
    EX_SPRITE_DATA* const extra = _pSprite->ExData();
    if (extra && *extra->nameText.value)
    {
        STRING temporaryName = _pSprite->GetName();
        ownedName = temporaryName;
    }

    const char* name = ownedName.m_value;
    if (name[0] != 'h' && name[0] != 'm')
    {
        char buffer[500];
        char* replacement = 0;
        const unsigned int length = CStringLength(name);
        if (static_cast<int>(length)>0)
        {
            for (unsigned int i = 0u;i <= length;++i)
                buffer[i] = name[i];

            char* const open = std::strchr(buffer, '[');
            char* const close = std::strchr(buffer, ']');
            if (open && close && open<close)
            {
                *open = '\0';
                *close = '\0';
                if (open>buffer && open[-1] == ' ')
                    open[-1] = '\0';
                replacement = open+1;
            }
            ownedName = buffer;
            name = ownedName.m_value;
        }

        if (CStringLength(name)>0u)
            (void)AddNameToSprite(_pSprite, name);
        else
            FindIdForSprite(_pSprite);

        if (replacement)
        {
            STRING replacementText(replacement);
            _pSprite->SetName(replacementText);
        }
    }
}

bool zItemTaker::CreateGroundSprite(iRPGObj* _pItem) const
{
    if (!_pItem)
        DebugAssert(5u, "_pItem", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 74u);
    if (!m_pPlayer->Flagman())
        return false;
    if (_pItem->GetGrndSpr())
        return false;

    int radius = AddSigned32Wrap(TruncateFloatToInt32(m_itemTakeDist), -10);
    if (radius <= 0)
    {
        DebugAssert(3u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 84u);
        radius = 40;
    }
    int kind = _pItem->GetVidNum(0);
    if (!Map->ValidateVid(kind))
        kind = 1602;
    else if (kind == 1679)
        radius = 0;

    float x = m_pPlayer->Flagman()->X();
    float y = m_pPlayer->Flagman()->Y();
    const float z = m_pPlayer->Flagman()->Z()+3.0f;
    if (radius)
    {
        const float half = static_cast<float>(radius)*0.5f;
        const int rx = static_cast<int>(static_cast<unsigned int>(Random(radius)));
        x = static_cast<float>(static_cast<double>(rx)-half+x);
        const int ry = static_cast<int>(static_cast<unsigned int>(Random(radius)));
        y = static_cast<float>(static_cast<double>(ry)-half+y);
    }
    int variant = _pItem->GetDirNum(0);
    if (variant == -1)
        variant = static_cast<int>(static_cast<unsigned int>(Random(255)));

    const ANGLE direction(static_cast<unsigned char>(variant));
    VECTOR position(x, y, z);
    VID* const resource = Map->Vid(kind);
    SPRITE* const s = Map->CreateSprite(resource, position, direction, 0);
    if (!s)
    {
        DebugAssert(4u, "s", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 115u);
        return true;
    }
    s->SetObjectID(static_cast<int>(_pItem->GetExtType()));
    _pItem->SetGrndSpr(s);
    _pItem->SetThrowTime(BitsToSigned32(::CurrentTime));
    return true;
}





iRPGObj* zItemTaker::FirstItemToTake()
{
    SPRITE* playerPosition = m_pPlayer->Flagman();
    if (!playerPosition)
        return 0;

    playerPosition = m_pPlayer->Flagman();
    if (playerPosition->Vid()->nVid == 3040)
        return 0;

    playerPosition = m_pPlayer->Flagman();
    const float playerX = playerPosition->X();
    playerPosition = m_pPlayer->Flagman();
    const float playerY = playerPosition->Y();
    (void)m_pPlayer->Flagman();

    m_itemsEnd = m_itemsBegin;
    m_spritesEnd = m_spritesBegin;

    const float distance = m_itemTakeDist;
    SPRITE_COLLECTOR* const collector =
        ::Sprites;
    SPRITE* sprite = collector->FirstHashInBox(
        playerX-distance, playerY-distance,
        playerX+distance, playerY+distance);

    while (sprite)
    {
        iRPGObj* obj = 0;
        if (sprite->Animation()<15 && !IsSpriteFarToTake(sprite))
        {
            EX_SPRITE_DATA* const spriteData = sprite->ExData();
            if (spriteData)
            {
                const int objectId = spriteData->objectId;
                if (objectId != -1)
                {
                    obj = m_pRPGFrameWork->GetItemById(objectId);
                    if (obj && obj->HaveInv())
                        obj = 0;
                }
            }
        }

        if (obj)
        {
            const bool directTake = obj->GetVidNum(1) != 0;
            if (directTake || obj->ItemIsNotReal())
            {
                if (obj->IsBaseObject() && obj->GetGrndSpr())
                    (void)DebugAssert(4u, "!obj->IsBaseObject() || !obj->GetGrndSpr()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\zItemTaker.cpp", 505u);

                iRPGImplant::sb_FromGround = true;
                if (!obj->ItemIsNotReal())
                    obj = SetNewItemToSprite(obj, sprite);
                iRPGImplant::sb_FromGround = false;

                VectorOperations::Insert(m_itemsBegin, m_itemsEnd, m_itemsCapacityEnd,
                    m_itemsEnd, 1u, obj);
                VectorOperations::Insert(m_spritesBegin, m_spritesEnd, m_spritesCapacityEnd,
                    m_spritesEnd, 1u, sprite);
            }
        }
        sprite = ::Sprites->NextHashInBox();
    }

    m_takeIndex = -1;
    return NextItemToTake();
}



