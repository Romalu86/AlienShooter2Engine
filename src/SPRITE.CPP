
#include <new>
#include "engine_globals.h"
#include <cstring>
#include <cstdlib>
#include "myerror.h"
#include "basetype.h"
#include "base_graph.h"
#include "script.h"
#include "unit.h"
#include "engine.h"
#include "sprite.h"
#include "zm/Utils/zStaticStorage.h"

namespace {
SPRITE* childCreationContext = 0;
StaticObjectStorage<LIST<SPRITE*> > interactionSprites;
unsigned char interactionSpritesInitialized = 0;
}

#include "vid.h"
#include "map.h"
#include "sprite_collector.h"
#include "list.h"
template<> void LIST<ACT>::Insert(ACT value);
template<> void LIST<ACT>::InsertFirst(ACT value);
template<> void LIST<ACT>::InsertBefore(int index, ACT value);
template<> void LIST<ACT>::Expand(int requestedCapacity);
template<> int LIST<int>::DeleteNumber(int index);
#include "mylib.h"

#include "resread.h"
#include "graph.h"
#include "region.h"
#include "player.h"
#include "constant.h"
#include "texture.h"
#include "sound.h"
#include "terrain.h"
void  LIST_Dtor(void* self);


static float SpriteScaleRandomByMode(const SPRITE* sprite, float scale)
{
    const VID* const vid = sprite->Vid();
    return (vid->runtimeFlags&0x00001000u) != 0u ? ::Random(scale) : ::Random2(scale);
}

void SPRITE::InvisibleOn()
{
    for (SPRITE* current = this; current; current = current->m_childChain)
        current->m_runtimeFlags |= 0x04000000u;
}

SPRITE* SPRITE::Goal() const
{
    return m_goalSprite;
}

float SPRITE::Random(float scale) const
{
    return (m_vid->runtimeFlags&0x00001000u) != 0u ? ::Random(scale) : ::Random2(scale);
}

SPRITE::~SPRITE() noexcept(false)
{
    VID* const nullVid = ::EmptyVid;

    if (m_vid == nullVid)
    {
        if (m_referenceCount != 0u)
        {
            const STRING subject("noRef for SPRITE with EmptyVid");
            VID* const vid = m_vid;
            const unsigned int references = m_referenceCount;
            const int vidId = vid?vid->nVid:-1;
            const STRING objectFormat("SPRITE [%i](%i,%i,%i)");
            const float y = m_xyz.y;
            const float x = m_xyz.x;
            const int iz = TruncateFloatToInt32(m_xyz.z);
            const int iy = TruncateFloatToInt32(y);
            const int ix = TruncateFloatToInt32(x);
            MYERROR* const sink = ::Error;
            sink->Error(objectFormat, E_INVALID, subject, references, vidId, ix, iy, iz);
        }

        const unsigned int army = (m_runtimeFlags>>17u)&7u;
        VID* const vid = m_vid;
        const unsigned int count = vid->spriteCountsByPlayer[army];
        if (count != 0u) --vid->spriteCountsByPlayer[army];
        return;
    }

    const int destroyFunction = m_vid->scriptFunction[17];
    if (destroyFunction >= 0)
    {
        MAP* const map = Map;
        (void) map->ScriptRun(destroyFunction, this, 0, 0);
    }

    VID* const gridVid = m_vid;
    if ((gridVid->property&0x28u) != 0u && (m_runtimeFlags&0x08000000u) == 0u)
        gridVid->ResetGridZ(this);

    (void) ::Sprites->Delete(this);

    if (BitsToSigned32(m_referenceCount)>1)
        Map->DeletePointerToSprite(this);

    const bool removeFromBuckets =
        BitsToSigned32(m_referenceCount)>1 ||
    (static_cast<int>(m_vid->spriteClass) == 2 && (m_runtimeFlags&0x000E0000u) != 0x00020000u);
    if (removeFromBuckets)
    {
        int iteratorState;
        SPRITE* entry = ::Sprites->FirstUnit(&iteratorState, -1);
        while (entry)
        {
            (void)(entry->DeletePointerToSprite(this), 0u);
            entry = ::Sprites->NextUnit(&iteratorState);
        }
    }

    if (BitsToSigned32(m_referenceCount)>1)
    {
        SPRITE_COLLECTOR* spriteCollector = ::Sprites;
        int index = BitsToSigned32(static_cast<unsigned int>(spriteCollector->allSprites.m_count)-1u);
        while (index >= 0 && index<spriteCollector->allSprites.m_count)
        {
            SPRITE** const table = spriteCollector->allSprites.m_data;
            SPRITE* const entry = table[index];
            if (entry)
            {
                entry->DeletePointerToSprite(this);
                spriteCollector = ::Sprites;
            }
            --index;
        }
    }

    const unsigned int army = (m_runtimeFlags>>17u)&7u;
    VID* const vid = m_vid;
    const unsigned int population = vid->spriteCountsByPlayer[army];
    if (population != 0u) --vid->spriteCountsByPlayer[army];

    SetGoal(0);

    if (m_childBacklink)
    {
        for (; ;)
        {
            SPRITE* const child = m_childChain;
            if (!child) break;
            if (child->m_vid != m_vid->linkVid) break;
            child->~SPRITE();
            CrtFreePointer(child);
        }
        SPRITE* const child = m_childChain;
        m_childBacklink->m_childChain = child;
        if (child) child->m_childBacklink = m_childBacklink;
    }
    else
    {
        while (SPRITE* child = m_childChain)
        {
            child->~SPRITE();
            CrtFreePointer(child);
        }
    }

    const unsigned int remaining = m_referenceCount-1u;
    m_referenceCount = remaining;
    if (remaining != 0u)
    {
        const STRING subject("Reference count non zero after delete");
        VID* const owner = m_vid;
        const unsigned int references = m_referenceCount;
        const int vidId = owner?owner->nVid:-1;
        const STRING objectFormat("SPRITE [%i](%i,%i,%i)");
        const float y = m_xyz.y;
        const float x = m_xyz.x;
        const int iz = TruncateFloatToInt32(m_xyz.z);
        const int iy = TruncateFloatToInt32(y);
        const int ix = TruncateFloatToInt32(x);
        MYERROR* const sink = ::Error;
        sink->Error(objectFormat, E_ERROR, subject, references, vidId, ix, iy, iz);
    }

    if (m_extraData)
    {
        const int category = m_extraData->numberInName;
        if (category >= 0 && category <= 9)
        {
            MAP* const map = Map;
            const unsigned int count = map->m_spriteCountsByNameNumber[category];
            map->m_spriteCountsByNameNumber[category] = count-1u;
        }

    }
    EX_SPRITE_DATA* const extraData = m_extraData;
    if (extraData)
    {
        extraData->nameText.~STRING();
        as2::core::destroyArray(extraData->actionLane, 16u, 4u,
            [](void* element){ LIST_Dtor(element); });
        extraData->items.~LIST<int>();
        CrtFreePointer(extraData);
    }
}

void SPRITE::operator delete(void* pointer)
{
    CrtFreePointer(pointer);
}


int SPRITE::Release()
{
    const unsigned int remaining = m_referenceCount-1u;
    m_referenceCount = remaining;
    const int refs = BitsToSigned32(remaining);
    if (refs>0) return refs;
    if (refs == 0)
    {
        this->~SPRITE();
        CrtFreePointer(this);
        return 0;
    }

    const STRING subject("noRef at Release");
    VID* const vid = m_vid;
    const unsigned int references = m_referenceCount;
    const int vidId = vid?vid->nVid:-1;
    const STRING objectFormat("SPRITE [%i](%i,%i,%i)");
    const int iz = TruncateFloatToInt32(m_xyz.z);
    const int iy = TruncateFloatToInt32(m_xyz.y);
    const int ix = TruncateFloatToInt32(m_xyz.x);
    MYERROR* const sink = ::Error;
    sink->Error(objectFormat, E_INVALID, subject, references, vidId, ix, iy, iz);
    return 0;
}


void SPRITE::DeletePointerToSprite(SPRITE* target)
{
    if (!target) return;
    SPRITE* observer = m_childChain;
    if (observer) observer->DeletePointerToSprite(target);
    if (m_goalSprite == target)
    {
        if (static_cast<unsigned int>(m_currentAnimation) == 8u && (m_runtimeFlags&0x10000u) == 0u)
            (void) SetCommand(4, target->m_xyz);
        else
            (void) SetCommand(0, static_cast<SPRITE*>(0));
    }
    EX_SPRITE_DATA* const extraData = m_extraData;
    if (!extraData) return;
    for (unsigned int lane = 0; lane<4u; ++lane)
    {
        const int count = extraData->actionLane[lane].m_count;
        ACT* const entries = extraData->actionLane[lane].m_data;
        for (int actionIndex = 0; actionIndex<count; ++actionIndex)
        {
            ACT& entry = entries[actionIndex];
            if (static_cast<unsigned int>(entry.arg1) != PtrToU32(target)) continue;
            const unsigned int type = static_cast<unsigned int>(entry.command);
            if (type == ACT_ATTACK || type == ACT_MOVE_TO || type == ACT_LINK_ENGINE ||
                type == ACT_CLASH_ENGINE || type == ACT_FORCELINK_ENGINE || type == ACT_COPY_STACK_TO ||
                type == ACT_FOLLOW || type == ACT_SET_LINK || type == ACT_SET_UPLINK)
            {
                entry.command = ACT_NONE;
                entry.arg1 = 0;
            }
        }
    }
}


int SPRITE::GetFireDamage() const
{
    return 0;
}

void*  LIST_Ctor(void* self)
{
    LIST<ACT>* const list = new (self) LIST<ACT>;
    list->m_data = 0;
    list->m_count = 0;
    list->m_capacity = 0;
    return self;
}


unsigned int __cdecl Sprite_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


GAMMA::GAMMA(GAMMA left, GAMMA right)
{
    for (unsigned int componentIndex = 0; componentIndex<8u; ++componentIndex) value[componentIndex] = SaturatingAddByte(left.value[componentIndex], right.value[componentIndex]);
}

float VECTOR2::Length() const
{
    return static_cast<float>(sqrt(static_cast<double>(x*x+y*y)));
}

VECTOR::VECTOR(const VECTOR& vectorValue) : x(vectorValue.x), y(vectorValue.y), z(vectorValue.z)
{
}


VECTOR::VECTOR(int xValue, int yValue, int zValue)
    : x(static_cast<float>(xValue)), y(static_cast<float>(yValue)), z(static_cast<float>(zValue))
{
}

VECTOR VECTOR::operator+(const VECTOR& vectorValue) const
{
    return VECTOR(x+vectorValue.x, y+vectorValue.y, z+vectorValue.z);
}

VECTOR2 VAbs(const VECTOR2& value)
{
    return VECTOR2(AbsFloat(value.x), AbsFloat(value.y));
}

VECTOR2 ANGLE::RotateY(const VECTOR2& vectorValue) const
{
    const unsigned int index = static_cast<unsigned int>(value)*4u;
    const float resultY = ANGLE(static_cast<unsigned char>((index)/4u)).CosY()*vectorValue.y + ANGLE(static_cast<unsigned char>((index)/4u)).SinY()*vectorValue.x;
    const float resultX = ANGLE(static_cast<unsigned char>((index)/4u)).Cos()*vectorValue.x - ANGLE(static_cast<unsigned char>((index)/4u)).Sin()*vectorValue.y;
    return VECTOR2(resultX, resultY);
}

ANGLE VID::SteppedDirection(ANGLE direction) const
{
    const int directionCount = noDir;
    if (directionCount>1 && (property&0x20000000u) == 0u)
    {
        const int realDirection = const_cast<VID*>(this)->RealDirection(direction.value);
        return ANGLE(static_cast<unsigned char>((realDirection<<8)/directionCount));
    }
    return direction;
}

static int SpriteHpHalfTowardZero(int value)
{
    if (value >= 0) return value/2;
    return -static_cast<int>((0u-static_cast<unsigned int>(value))/2u);
}


void SPRITE::ChangeAnimation(int action)
{
    if (action<0 || action >= 17)
    {
        STRING subject;
        STRING objectFormat;
        subject = "new_animation in ChangeAnimation";
        objectFormat = "SPRITE [%i](%i,%i,%i)";

        VID* const vid = m_vid;
        const int vidId = vid?vid->nVid:-1;
        MYERROR* sink = ::Error;
        sink->Error(objectFormat, E_INVALID, subject, action, vidId, TruncateFloatToInt32(m_xyz.x), TruncateFloatToInt32(m_xyz.y), TruncateFloatToInt32(m_xyz.z));
        return;
    }

    SPRITE* child = m_childChain;
    if (child)
    {
        VID* const childVid = child->Vid();
        const int childAction = child->Animation();
        const bool deathOnly = (childVid->nWeapon != 0 || childAction == 8 || childAction >= 15);
        if (!deathOnly || action == 15 || action == 16)
            child->ChangeAnimation(action);
    }

    const int oldAction = m_currentAnimation;
    if (oldAction == action && oldAction != 8)
        return;

    m_runtimeFlags &= 0xFFFF7FFFu;
    VID* vid = m_vid;
    const unsigned int actionIndex = static_cast<unsigned int>(action);

    if ((action == 15 || action == 16) && vid->noAnimCadr[actionIndex] == 0)
    {
        if (m_currentFrame>m_currentFrameEnd)
            m_currentFrame = m_currentFrameBegin;
        m_currentFrameBegin = m_currentFrame;
        m_currentFrameEnd = m_currentFrame;
    }
    else
    {
        const unsigned char direction = m_direction.value;
        const int frameCount = vid->animationFrameCount[actionIndex];
        int firstFrame = vid->animationBaseFrame[actionIndex];
        m_currentFrameBegin = firstFrame;

        if ((vid->property&0x00002000u) != 0u)
        {
            firstFrame += static_cast<int>(m_extraData->reservedA4)*frameCount;
        }
        else
        {
            firstFrame += vid->RealDirectionWithGraphDetail(direction)*frameCount;
        }
        m_currentFrameBegin = firstFrame;

        const int lastFrame = (vid->noAnimCadr[actionIndex] == 0 && action >= 13)
            ? firstFrame : firstFrame+frameCount-1;
        m_currentFrameEnd = lastFrame;
        m_currentFrame = firstFrame;

        SPRITE* previous = m_childBacklink;
        if (previous && oldAction == 2)
        {
            WEAPON_RUNTIME* const vidInfo = vid->weapon;
            if ((vidInfo->Property&0x00000800u) != 0u)
            {
                const int thisSpan = m_currentFrameEnd-m_currentFrameBegin;
                const int previousSpan = previous->CurrentFrameEnd()-previous->m_currentFrameBegin;
                if (previousSpan == thisSpan)
                {
                    const int synchronizedFrame = m_currentFrame+
                    (previous->CurrentFrame()-previous->m_currentFrameBegin);
                    m_currentFrame = synchronizedFrame;
                }
            }
        }
    }

    child = m_childChain;
    VECTOR* const framePositions = vid->cadrLinkXYZ;
    if (child && framePositions)
    {
        const int frame = m_currentFrame;
        const VECTOR& offset = framePositions[frame];
        VECTOR position;
        position.x = m_xyz.x+offset.x;
        position.y = m_xyz.y+offset.y;
        position.z = m_xyz.z+offset.z;
        (void) child->ChangeCoor(position);
    }

    m_currentAnimation = actionIndex;
    m_lastAnimationTick = ::CurrentTime;
}

void SPRITE::ChangeHp(int hp)
{
    VID* vid = m_vid;

    if (hp <= 0 && vid->maxHp != 0)
    {
        if (m_currentAnimation >= 15) return;
        const unsigned int direction = (m_runtimeFlags>>17)&7u;
        vid->killedByPlayer[direction] = BitsToSigned32(
            static_cast<unsigned int>(vid->killedByPlayer[direction])+1u);

        const int hpDelta = BitsToSigned32(static_cast<unsigned int>(m_hp)-
            static_cast<unsigned int>(hp));
        if (hpDelta>vid->maxHpByPlayer[direction] && vid->childVid[16] != 0)
        {
            this->ChangeAnimation(16);
            return;
        }
        this->ChangeAnimation(15);
        return;
    }

    const unsigned int direction = (m_runtimeFlags>>17)&7u;
    int halfHp = SpriteHpHalfTowardZero(vid->maxHpByPlayer[direction]);
    if (hp>halfHp && m_hp <= halfHp)
        DestroyLink(vid->childVid[13]);

    const unsigned int nextDirection = (m_runtimeFlags>>17)&7u;
    vid = m_vid;
    halfHp = SpriteHpHalfTowardZero(vid->maxHpByPlayer[nextDirection]);
    if (hp <= halfHp && m_hp>halfHp)
    {
        if (vid->noAnimCadr[13] != 0)
            this->ChangeAnimation(13);
        else
            (void) CreateChildFor(13u, 0);
    }

    SPRITE* const linkedSprite = m_childChain;
    m_hp = hp;
    if (!linkedSprite) return;

    VID* const linkedVid = linkedSprite->Vid();
    WEAPON_RUNTIME* const linkedVidInfo = linkedVid->weapon;
    if ((linkedVidInfo->Property&0x4000u) == 0u) return;

    const int baseHp = m_vid->maxHp;
    if (baseHp == 0) return;
    int clampedHp = hp;
    if (clampedHp<0) clampedHp = 0;
    else if (clampedHp>baseHp) clampedHp = baseHp;

    const int scaledNumerator = BitsToSigned32(static_cast<unsigned int>(clampedHp)*255u);
    const int ratio = scaledNumerator/baseHp;
    const unsigned char newDirection = static_cast<unsigned char>(255u-static_cast<unsigned char>(ratio)-static_cast<unsigned char>(linkedVid->directionQuantizationOffset));
    m_childChain->ChangeDirection(ANGLE(newDirection));
}


static EX_SPRITE_DATA* SpriteEnsureExtraData(SPRITE* sprite);

namespace
{

static float CreateChildRandom(const VID* childVid, float scale)
{
    return (childVid->runtimeFlags&0x00001000u) != 0u ? ::Random(scale) : ::Random2(scale);
}

static int CreateChildRandom(const VID* childVid, int maximum)
{
    return (childVid->runtimeFlags&0x00001000u) != 0u ? ::Random(maximum) : ::Random2(maximum);
}

static VECTOR CreateChildOscillation(const VID* parentVid)
{
    VECTOR offset(0.0f, 0.0f, 0.0f);
    if ((parentVid->property&0x00010000u) != 0u)
    {
        const unsigned int phase = (::CurrentTime>>3)&0xFFu;
        offset.z = FSin[phase]*parentVid->topZ;
    }
    return offset;
}

static inline bool CreateChildFloatEqualOrUnordered(const float& left, const float& right)
{
    return FloatEqualOrUnordered(left, right);
}

static int CreateChildAbsSigned(int value)
{
    unsigned int bits = static_cast<unsigned int>(value);
    if (value<0) bits = 0u-bits;
    return static_cast<int>(bits);
}
}

SPRITE* SPRITE::CreateChild()
{
    VID* const childVid = m_vid->childVid[m_currentAnimation];
    const int rawChildCount = m_vid->noChild[m_currentAnimation];
    unsigned int childCountBits = static_cast<unsigned int>(rawChildCount);
    if (rawChildCount<0) childCountBits = 0u-childCountBits;
    int childCount = static_cast<int>(childCountBits);
    if (!childVid || childVid->notCreateAsChild != 0)
        return 0;

    unsigned char steppedDirection = 0;
    if ((childVid->property&0x02000000u) == 0u)
    {
        const SPRITE* directionSource = this;
        if (m_currentAnimation == 7)
        {
            const SPRITE* context = childCreationContext;
            if (context) directionSource = context;
        }
        steppedDirection = m_vid->SteppedDirection(directionSource->m_direction).value;
    }

    int firstOrdinal = 0;
    const WEAPON_RUNTIME* const weapon = m_vid->weapon;
    if (childCount == 2 && (weapon->Property&0x10u) != 0u)
    {
        if ((m_runtimeFlags&0x00800000u) != 0u)
        {
            m_runtimeFlags &= ~0x00008000u;
            firstOrdinal = 1;
        }
        else
        {
            childCount = 1;
        }
        m_runtimeFlags ^= 0x00800000u;
    }

    SPRITE* lastResult = 0;
    for (int ordinal = firstOrdinal; ordinal<childCount; ++ordinal)
    {
        VECTOR spawnPosition = m_xyz;
        spawnPosition.z += m_vid->childZ[m_currentAnimation];

        float projectileBaseX;
        float projectileBaseY;

        const float childX = m_vid->childX[m_currentAnimation];
        const float childY = m_vid->childY[m_currentAnimation];
        const float childZ = m_vid->childZ[m_currentAnimation];
        const float rotateXX = ANGLE(steppedDirection).Cos();
        const float rotateYX = ANGLE(steppedDirection).SinY();
        const float rotateXY = ANGLE(steppedDirection).Sin();
        const float rotateYY = ANGLE(steppedDirection).CosY();

        if (m_vid->nChildVid[m_currentAnimation]<0)
        {
            if (m_vid->spriteClass == 23u &&
                CreateChildFloatEqualOrUnordered(childX, 0.0f) &&
                CreateChildFloatEqualOrUnordered(childY, 0.0f))
            {
                projectileBaseX = 0.0f;
                projectileBaseY = 0.0f;
                const REGION* const regionOwner = static_cast<const REGION*>(this);
                const unsigned int regionFlags = regionOwner->m_fogFlags;
                if ((regionFlags&0x08u) != 0u)
                {
                    const VECTOR2 viewPosition = m_xyz.ViewCoor();
                    const float mapHeight = Map->m_sizeXY.y;
                    const float randomY = CreateChildRandom(childVid, mapHeight);
                    const float mapWidth = Map->m_sizeXY.x;
                    const float randomX = CreateChildRandom(childVid, mapWidth);
                    spawnPosition.x += randomX-viewPosition.x;
                    spawnPosition.y += randomY-viewPosition.y;
                }
                else
                {
                    const float height = regionOwner->m_halfHeight;
                    const float randomY = CreateChildRandom(childVid, height);
                    const float width = regionOwner->m_halfWidth;
                    const float randomX = CreateChildRandom(childVid, width);
                    spawnPosition.x += width*0.5f-randomX;
                    spawnPosition.y += height*0.5f-randomY+m_xyz.z;
                }
            }
            else
            {
                const float localX = childX-CreateChildRandom(childVid, childX+childX);
                const float localY = childY-CreateChildRandom(childVid, childY+childY);
                projectileBaseX = -rotateXX*localX;
                projectileBaseY = -rotateYX*localX;
                spawnPosition.x += projectileBaseX+rotateXY*localY;
                spawnPosition.y += projectileBaseY-rotateYY*localY;
            }
        }
        else if (childCount == 1 && childVid->spriteClass == 8u &&
            CreateChildFloatEqualOrUnordered(childX, 0.0f) &&
            CreateChildFloatEqualOrUnordered(childY, childZ))
        {
            spawnPosition.x += childX;
            spawnPosition.y += childY;
        }
        else if (ordinal == 1)
        {
            projectileBaseX = -rotateXX*childX;
            projectileBaseY = -rotateYX*childX;
            spawnPosition.x += projectileBaseX+rotateXY*childY;
            spawnPosition.y += projectileBaseY-rotateYY*childY;
        }
        else if (ordinal == 2)
        {
            projectileBaseX = 0.0f;
            projectileBaseY = 0.0f;
            spawnPosition.x += rotateXY*childY;
            spawnPosition.y += rotateYY*childY;
        }
        else
        {
            projectileBaseX = rotateXX*childX;
            projectileBaseY = rotateYX*childX;
            spawnPosition.x += projectileBaseX+rotateXY*childY;
            spawnPosition.y += projectileBaseY-rotateYY*childY;
        }

        if (m_currentAnimation == 8 && !m_goalSprite)
        {
            lastResult = 0;
            continue;
        }

        ANGLE childDirection = m_direction;
        if ((childVid->property&0x1u) != 0u)
        {
            childDirection = ANGLE(static_cast<unsigned char>(CreateChildRandom(childVid, 255)));
        }
        else if (m_currentAnimation == 7)
        {
            SPRITE* const context = childCreationContext;
            if (context) childDirection = context->m_direction;
        }
        else if (((m_vid->property | childVid->property)&0x00080000u) != 0u && m_extraData)
        {
            const VECTOR& anchor = m_extraData->cachedPosition;
            const float currentViewY = m_xyz.y-m_xyz.z;
            const float anchorViewY = anchor.y-anchor.z;
            const bool sameX = CreateChildFloatEqualOrUnordered(m_xyz.x, anchor.x);
            const bool sameViewY = CreateChildFloatEqualOrUnordered(currentViewY, anchorViewY);
            if (!sameX || !sameViewY)
            {
                childDirection = Decart2Polar(
                    m_xyz.x-anchor.x,
                    (currentViewY-anchorViewY)*(1.41430664f));
            }
            else
            {
                const unsigned int tableIndex = static_cast<unsigned int>(m_direction.value)*4u;
                const float directionX = ANGLE(static_cast<unsigned char>((tableIndex)/4u)).Sin()*m_speed;
                const float directionY =
                    (ANGLE(static_cast<unsigned char>((tableIndex)/4u)).Cos()*m_speed+m_zSpeed)*
                (-1.41430664f);
                childDirection = Decart2Polar(directionX, directionY);
            }
        }

        if (childVid->spriteClass == 2u)
        {
            SPRITE_COLLECTOR* collector =
                ::Sprites;
            SPRITE* blocker = collector->CanPlace(childVid, spawnPosition, childDirection);
            if (blocker && blocker != this)
            {
                const int collisionScript = childVid->scriptFunction[19];
                if (collisionScript == 0)
                {
                    lastResult = blocker;
                    continue;
                }

                int attempt = 0;
                while (blocker && blocker != this && attempt<3)
                {
                    Map->ScriptRun(collisionScript, this, blocker, 0);
                    collector = ::Sprites;
                    blocker = collector->CanPlace(childVid, spawnPosition, childDirection);
                    if (blocker && blocker != this)
                        ++attempt;
                }
                if (blocker && blocker != this)
                {
                    lastResult = blocker;
                    continue;
                }
            }
        }

        const VECTOR oscillation = CreateChildOscillation(m_vid);
        const VECTOR finalPosition = spawnPosition+oscillation;
        SPRITE* const child = Map->CreateSprite(childVid, finalPosition, childDirection, this);
        lastResult = child;
        if (!child) continue;

        if (m_currentAnimation >= 15 && (m_behave&0x00010000u) != 0u)
        {
            child->m_behave |= 0x00010000u;
            if (m_extraData)
            {
                EX_SPRITE_DATA* const childExtra = SpriteEnsureExtraData(child);
                EX_SPRITE_DATA* const parentExtra = m_extraData;
                for (int lane = 3; lane >= 0; --lane)
                {
                    LIST<ACT>& sourceLane = parentExtra->actionLane[lane];
                    LIST<ACT>& destinationLane = childExtra->actionLane[lane];
                    if (&destinationLane == &sourceLane) continue;
                    if (destinationLane.m_data) CrtFreePointer(destinationLane.m_data);
                    destinationLane.m_count = sourceLane.m_count;
                    destinationLane.m_capacity = sourceLane.m_capacity;
                    destinationLane.m_data = sourceLane.m_data;
                    sourceLane.m_count = 0;
                    sourceLane.m_capacity = 0;
                    sourceLane.m_data = 0;
                }

                bool haveName = false;
                {
                    STRING name;
                    name = GetName();
                    haveName = name.m_value[0] != '\0';
                }
                if (haveName)
                {
                    STRING name;
                    name = GetName();
                    child->SetName(name);
                }

                const int number = m_extraData->numberInName;
                if (number >= 0) child->SetNumberInName(number);
            }
        }

        if ((m_vid->property&0x00800000u) != 0u)
        {
            VECTOR parentScale(1.0f, 1.0f, 1.0f);
            if (m_extraData)
            {
                parentScale = m_extraData->scale;
            }
            const VECTOR childScale(parentScale.x*m_vid->scaleXYZ.x,
                parentScale.y*m_vid->scaleXYZ.y,
                parentScale.z*m_vid->scaleXYZ.z);
            child->SetScale(childScale);

            if (m_currentAnimation >= 15 || m_currentAnimation == 0)
            {
                GAMMA gamma;
                gamma.modulate = 0u;
                gamma.add = 0u;
                if (m_extraData)
                {
                    gamma.modulate = m_extraData->gammaModulate;
                    gamma.add = m_extraData->gammaAdd;
                }
                child->SetGamma(gamma);

                if ((childVid->spriteType&0x08u) != 0u)
                {
                    if (m_extraData)
                    {
                        const int number = m_extraData->numberInName;
                        if (number >= 0) child->SetNumberInName(number);
                    }
                    if (m_extraData)
                    {
                        const LIST<ACT>& lane = m_extraData->actionLane[0];
                        if (lane.m_count != 0)
                        {
                            const ACT* const records = lane.m_data;
                            if (records[lane.m_count-1].command != ACT_STOP_STACK)
                                (void) Action(ACT_COPY_STACK_TO, 0, static_cast<int>(PtrToU32(child)), 0);
                        }
                    }
                }

            }
        }

        if (m_goalSprite && m_currentAnimation == 8)
        {
            SPRITE* const attackTarget = m_goalSprite;
            if ((m_vid->weapon->Property&0x20u) != 0u &&
                attackTarget->m_vid != ::EmptyVid)
            {
                child->Attack(attackTarget);
                child->StartMove();
            }
            else
            {
                const VECTOR2 toTarget(attackTarget->m_xyz.x-m_xyz.x,
                    attackTarget->m_xyz.y-m_xyz.y);
                const float targetSpread = m_vid->weapon->aimRange;
                const float spread = toTarget.NearLength()*targetSpread*
                (0.00499999989f);
                const float span = spread-(-spread);

                VECTOR targetPosition;
                for (int attempt = 0; ; ++attempt)
                {
                    const float offsetY = CreateChildRandom(childVid, span)-spread;
                    const float offsetX = CreateChildRandom(childVid, span)-spread;
                    targetPosition.x = attackTarget->m_xyz.x+projectileBaseX+offsetX;
                    targetPosition.y = attackTarget->m_xyz.y+projectileBaseY+offsetY;
                    targetPosition.z = attackTarget->m_xyz.z;
                    if (attempt >= 5 || !Map->IsDotUnderGround(targetPosition))
                        break;
                }

                void* const storage = CrtNew(104u);
                SPRITE* syntheticAttackTarget = 0;
                if (storage)
                    syntheticAttackTarget = new (storage) SPRITE(targetPosition, ANGLE(0));
                child->Attack(syntheticAttackTarget);
                child->StartMove();
            }

            if (m_vid->deathDamageMax<0)
            {
                child->InsertItem(123456);
                child->InsertItem(TruncateFloatToInt32(m_vid->deathRange));
                child->InsertItem(CreateChildAbsSigned(m_vid->deathDamageMin));
                child->InsertItem(CreateChildAbsSigned(m_vid->deathDamageMax));
                child->InsertItem(BitsToSigned32(static_cast<unsigned int>(TruncateFloatToInt32(m_vid->deathPush))<<10u));
            }
        }

    }

    if (m_currentAnimation == 8 && (m_runtimeFlags&0x00800000u) == 0u)
    {
        m_runtimeFlags |= 0x00010000u;
        const unsigned int command = m_runtimeFlags&0x7Cu;
        if (command == 0x10u || command == 0x14u)
        {
            if (m_goalSprite && m_childBacklink && m_childBacklink->m_goalSprite == m_goalSprite)
            {
                const unsigned int parentCommand = m_childBacklink->m_runtimeFlags&0x7Cu;
                if (parentCommand == 0x10u || parentCommand == 0x14u)
                    m_childBacklink->SetCommand(0, static_cast<SPRITE*>(0));

                if (m_childBacklink->m_vid->spriteClass == 21u &&
                    (m_childBacklink->m_runtimeFlags&0x7Cu) == 0x74u)
                    reinterpret_cast<ENGINE*>(m_childBacklink)->SetCommandToTrain(0x1E, 0, 0, 0);
            }

            SetCommand(0, static_cast<SPRITE*>(0));
            SPRITE* const detector = m_childBacklink ? m_childBacklink : this;
            detector->DetectTact();
        }
    }

    return lastResult;
}

int SPRITE::DestroyLink(const VID* linkVid)
{
    SPRITE* previous = this;
    SPRITE* linked = previous->m_childChain;
    while (linked)
    {
        if (linked->m_vid == linkVid)
        {
            SPRITE* const next = linked->m_childChain;
            previous->m_childChain = next;
            if (next) next->m_childBacklink = previous;
            linked->m_childChain = 0;
            linked->m_childBacklink = 0;
            linked->~SPRITE();
            CrtFreePointer(linked);
            return 1;
        }
        previous = linked;
        linked = previous->m_childChain;
    }
    return 0;
}

int SPRITE::CreateChildFor(int action, SPRITE* creationContext)
{
    const unsigned int actionIndex = static_cast<unsigned int>(action);
    const unsigned int oldAction = static_cast<unsigned int>(m_currentAnimation);
    m_currentAnimation = actionIndex;

    VID* vid = m_vid;
    if (vid->childVid[actionIndex] != 0)
    {
        childCreationContext = creationContext;
        (void) CreateChild();
        childCreationContext = 0;
    }

    vid = m_vid;
    const unsigned int sfxId = static_cast<unsigned int>(vid->sfx[actionIndex]);
    if (sfxId != 0u) PlaySFX(static_cast<int>(sfxId));

    vid = m_vid;
    const int scriptId = vid->scriptFunction[actionIndex];
    unsigned int result = 0u;
    if (scriptId >= 0 && actionIndex != 14u)
        result = static_cast<unsigned int>(Map->ScriptRun(scriptId, this, creationContext, 0));

    m_currentAnimation = static_cast<int>(oldAction);
    return result;
}
static inline bool SpriteX87EqualOrUnorderedZero(const float& value)
{
    return FloatEqualOrUnordered(value, 0.0f);
}
static inline bool SpriteX87LessOrUnorderedOne(const float& value)
{
    return FloatLessOrUnordered(value, 1.0f);
}
static unsigned char SpriteCircularDirectionDifference(unsigned char firstDirection, unsigned char secondDirection)
{
    const unsigned char forwardDifference = static_cast<unsigned char>(firstDirection-secondDirection);
    const unsigned char reverseDifference = static_cast<unsigned char>(secondDirection-firstDirection);
    return forwardDifference<reverseDifference?forwardDifference:reverseDifference;
}



void SPRITE::ChangeDirection1(ANGLE directionValue)
{
    const unsigned char direction = directionValue.value;
    if (m_direction.value == direction) return;

    VID* vid = m_vid;
    if (vid->noDir != 1 && (vid->property&0x00002000u) == 0u)
    {
        const int frameDelta = m_currentFrame-m_currentFrameBegin;
        const unsigned int action = static_cast<unsigned int>(m_currentAnimation);
        const int baseFrame = vid->animationBaseFrame[action];
        const int frameCount = vid->animationFrameCount[action];
        m_currentFrameBegin = baseFrame;
        const int firstFrame = baseFrame+vid->RealDirectionWithGraphDetail(direction)*frameCount;
        m_currentFrameBegin = firstFrame;
        m_currentFrameEnd = firstFrame+frameCount-1;
        m_currentFrame = firstFrame+frameDelta;
    }
    m_direction.value = direction;
}


void LINKER::LinkRotate(ANGLE direction)
{
    const unsigned char delta = static_cast<unsigned char>(direction.value-m_linkDirection.value);
    const unsigned int tableOffset = static_cast<unsigned int>(delta)*4u;

    double positionX = static_cast<double>(ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Cos())*m_linkOffset.x
    -static_cast<double>(ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Sin())*m_linkOffset.y;
    double positionY = static_cast<double>(ANGLE(static_cast<unsigned char>((tableOffset)/4u)).SinY())*m_linkOffset.x
    +static_cast<double>(ANGLE(static_cast<unsigned char>((tableOffset)/4u)).CosY())*m_linkOffset.y;

    if (m_linkedSprite)
    {
        positionX += m_linkedSprite->X();
        positionY += m_linkedSprite->Y();
    }
    else
    {
        positionX += m_childBacklink->X();
        positionY += m_childBacklink->Y();
    }
    const VECTOR2 position(static_cast<float>(positionX), static_cast<float>(positionY));
    ChangeCoorForLinkRotate(position);
}

void SPRITE::ChangeDirection(ANGLE directionValue)
{
    const unsigned char direction = directionValue.value;
    const unsigned char oldDirection = m_direction.value;
    if (oldDirection == direction) return;

    VID* vid = m_vid;
    if ((vid->property&0x00000100u) != 0u)
    {
        m_direction.value = direction;
        if (CanPlaceWithCrush(m_xyz))
        {
            m_direction.value = oldDirection;
            return;
        }
    }

    vid = m_vid;
    if (vid->noDir != 1 && (vid->property&0x00002000u) == 0u)
    {
        const int frameDelta = m_currentFrame-m_currentFrameBegin;
        const unsigned int action = static_cast<unsigned int>(m_currentAnimation);
        const int baseFrame = vid->animationBaseFrame[action];
        const int frameCount = vid->animationFrameCount[action];
        m_currentFrameBegin = baseFrame;
        const int firstFrame = baseFrame+vid->RealDirectionWithGraphDetail(direction)*frameCount;
        m_currentFrameBegin = firstFrame;
        m_currentFrameEnd = firstFrame+frameCount-1;
        m_currentFrame = firstFrame+frameDelta;
    }

    int propagateLinkedDirection = 1;
    SPRITE* chainParent = this;
    SPRITE* child = chainParent->m_childChain;
    while (child)
    {
        VID* childVid = child->m_vid;
        if (childVid->spriteClass == 12)
        {
            if (reinterpret_cast<LINKER*>(child)->m_linkedSprite == this)
            {
                if ((childVid->property&0x02000000u) == 0u)
                {
                    VID* parentVid = chainParent->m_vid;
                    unsigned char mappedDirection = direction;
                    mappedDirection = parentVid->SteppedDirection(ANGLE(direction)).value;
                    reinterpret_cast<LINKER*>(child)->LinkRotate(ANGLE(static_cast<unsigned char>(mappedDirection)));
                }
                if (SpriteX87EqualOrUnorderedZero(child->m_vid->rotationSpeed))
                    child->ChangeDirection1(ANGLE(direction));
            }
        }
        else
        {
            if ((childVid->property&0x02000000u) == 0u)
            {
                VID* parentVid = chainParent->m_vid;
                if (parentVid->linkVid == childVid)
                {
                    VECTOR* framePositionTable = parentVid->cadrLinkXYZ;
                    if (framePositionTable)
                    {
                        const int frame = chainParent->m_currentFrame;
                        const VECTOR* table = framePositionTable;
                        VECTOR2 position;
                        position.x = table[frame].x+chainParent->m_xyz.x;
                        position.y = table[frame].y+chainParent->m_xyz.y;
                        child->ChangeCoorForLinkRotate(position);
                    }
                    else if (!SpriteX87EqualOrUnorderedZero(parentVid->linkXYZ.x) ||
                        !SpriteX87EqualOrUnorderedZero(parentVid->linkXYZ.y))
                    {
                        unsigned char mappedDirection = direction;
                        mappedDirection = parentVid->SteppedDirection(ANGLE(direction)).value;
                        const unsigned int tableOffset = static_cast<unsigned int>(mappedDirection)*4u;
                        parentVid = chainParent->m_vid;
                        const float localX = parentVid->linkXYZ.x;
                        const float localY = parentVid->linkXYZ.y;
                        VECTOR2 position;
                        position.x = static_cast<float>(
                            static_cast<double>(ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Cos())*localX
                            -static_cast<double>(ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Sin())*localY
                            +chainParent->X());
                        const float offsetY = static_cast<float>(
                            static_cast<double>(ANGLE(static_cast<unsigned char>((tableOffset)/4u)).SinY())*localX
                            +static_cast<double>(ANGLE(static_cast<unsigned char>((tableOffset)/4u)).CosY())*localY);
                        position.y = offsetY+chainParent->Y();
                        child->ChangeCoorForLinkRotate(position);
                    }
                }
            }

            childVid = child->m_vid;
            if (propagateLinkedDirection != 0)
            {
                if (SpriteX87EqualOrUnorderedZero(childVid->rotationSpeed))
                {
                    child->ChangeDirection1(ANGLE(direction));
                }
                else
                {
                    WEAPON_RUNTIME* childInfo = childVid->weapon;
                    if ((childInfo->Property&0x00000800u) == 0u)
                    {
                        propagateLinkedDirection = 0;
                    }
                    else
                    {
                        VID* parentVid = chainParent->m_vid;
                        if (static_cast<unsigned int>(parentVid->noAnimCadr[6]) != 0u)
                        {
                            propagateLinkedDirection = 0;
                        }
                        else if (SpriteCircularDirectionDifference(direction, child->m_direction.value) <= 32u)
                        {
                            propagateLinkedDirection = 0;
                        }
                        else
                        {
                            const unsigned char currentRootDirection = m_direction.value;
                            const unsigned char delta = static_cast<unsigned char>(direction-currentRootDirection);
                            const unsigned char adjusted = static_cast<unsigned char>(child->m_direction.value+delta);
                            child->ChangeDirection1(ANGLE(adjusted));
                        }
                    }
                }
            }
        }

        chainParent = child;
        child = chainParent->m_childChain;
    }

    m_direction.value = direction;

    SPRITE* linked = m_childBacklink;
    if (linked)
    {
        WEAPON_RUNTIME* vidInfo = m_vid->weapon;
        if ((vidInfo->Property&0x00000800u) != 0u)
        {
            const unsigned char diff = SpriteCircularDirectionDifference(linked->m_direction.value, direction);
            if (static_cast<unsigned int>(linked->m_currentAnimation) == 2u && diff<35u)
            {
                if (static_cast<unsigned int>(m_currentAnimation) == 0u) this->ChangeAnimation(2);
            }
            else if (static_cast<unsigned int>(m_currentAnimation) == 2u)
            {
                this->ChangeAnimation(0);
            }
        }
    }

    child = m_childChain;
    if (child)
    {
        VID* childVid = child->m_vid;
        WEAPON_RUNTIME* childInfo = childVid->weapon;
        if ((childInfo->Property&0x00000800u) != 0u)
        {
            const unsigned char diff = SpriteCircularDirectionDifference(child->m_direction.value, direction);
            if (static_cast<unsigned int>(m_currentAnimation) == 2u && diff<35u)
            {
                if (static_cast<unsigned int>(child->m_currentAnimation) == 0u) child->ChangeAnimation(2);
                return;
            }
            if (static_cast<unsigned int>(child->m_currentAnimation) == 2u) child->ChangeAnimation(0);
        }
    }
}

void SPRITE::PlaySFX(int sfxId) const
{
    VID* vid = m_vid;
    float pitch = 1.0f;
    if ((vid->property&0x00800000u) != 0u && vid->spriteClass == 2)
    {
        EX_SPRITE_DATA* source = m_extraData;
        const float scaleY = source?source->scale.y:1.0f;
        pitch = SpriteX87LessOrUnorderedOne(scaleY)?scaleY*(1.10000002f):scaleY;
    }

    GRAPH* camera = Graph;
    MAP* const application = Map;
    const float halfWidth = camera->screenWidth*(0.5f);
    const float halfHeight = camera->screenHeight*(0.5f);

    VECTOR2 position;
    position.x = (m_xyz.x-application->m_shiftXY.x)-halfWidth;
    position.y = ((m_xyz.y-m_xyz.z)-application->m_shiftXY.y)-halfHeight;
    SOUND* const sound = ::Sound;
    sound->PlaySFXFromCoor(sfxId, position, pitch);
}









static EX_SPRITE_DATA* SpriteEnsureExtraData(SPRITE* sprite)
{
    EX_SPRITE_DATA* extraData = sprite->ExData();
    if (extraData) return extraData;
    void* const storage = CrtNew(sizeof(EX_SPRITE_DATA));
    extraData = storage ? new (storage) EX_SPRITE_DATA(sprite) : 0;
    sprite->SetExtraData(extraData);
    return extraData;
}


static float SpriteSourceGroundZ(MAP* map, const VID* vid, const void* position, unsigned char direction)
{
    return map->GetGroundZ(
        vid,
        *static_cast<const VECTOR2*>(position),
        ANGLE(direction));
}
static unsigned int SpriteSourceSelectGroup(VID* vid, SPRITE* parent)
{
    const ADD_CREATE_SPRITE* const overrideState = ::AddCreateSprite;
    if (overrideState) return overrideState->group&7u;
    if (parent) return (parent->RuntimeFlags()>>17u)&7u;
    VID* source = vid;
    VID* const linked = vid->linkVid;
    if (vid->nWeapon == 0 && linked != 0) source = linked;
    WEAPON_RUNTIME* info = source->weapon;
    return info->DefaultArmy&7u;
}
static unsigned int SpriteSourceSelectBehavior(VID* vid)
{
    VID* source = vid;
    VID* const linked = vid->linkVid;
    if (linked != 0 && linked->nWeapon != 0 && linked->childVid[8] != 0) source = linked;
    WEAPON_RUNTIME* info = source->weapon;
    return info->DefaultBehave;
}
static bool SpriteSourceNeedsRuntimeExtra(const VID* vid)
{
    if (vid->actionAuxStateRequired != 0) return true;
    const ADD_CREATE_SPRITE* const state = ::AddCreateSprite;
    if (!state) return false;
    return FloatBitsU32(state->scale.x) != 0x3F800000u || FloatBitsU32(state->scale.y) != 0x3F800000u ||
    FloatBitsU32(state->scale.z) != 0x3F800000u || state->gamma.modulate != 0u || state->gamma.add != 0u;
}


SPRITE::SPRITE(VID* vidObject, const VECTOR& positionValue, ANGLE directionValue, SPRITE* parentObject)
{
    const unsigned int directionByte = static_cast<unsigned int>(directionValue.value);

    m_xyz = positionValue;
    m_vid = vidObject;

    const unsigned int vidFlags = vidObject->property;
    const unsigned char direction = static_cast<unsigned char>(directionByte);
    if ((vidFlags&0x00000200u) != 0u)
    {
        MAP* const map = Map;
        const float ground = SpriteSourceGroundZ(map, vidObject, &m_xyz, direction);
        float z = ground;
        if (parentObject) z += positionValue.z-parentObject->m_xyz.z;
        m_xyz.z = z;
    }

    if ((vidFlags&0x00100000u) != 0u)
    {
        const float yJitter = SpriteScaleRandomByMode(this, 4.0f)-2.0f;
        const float xJitter = SpriteScaleRandomByMode(this, 4.0f)-2.0f;
        m_xyz.x += xJitter;
        m_xyz.y += yJitter;
    }

    unsigned int flags = m_runtimeFlags&0xFF8FFFFFu;
    const unsigned int group = SpriteSourceSelectGroup(vidObject, parentObject);
    flags = (flags&0xFFF1FFFFu) | ((group&7u)<<17u);
    m_runtimeFlags = flags;

    const unsigned int now = ::CurrentTime;
    m_lastAnimationTick = now;
    m_birthTime = now;
    m_referenceCount = 0u;
    m_goalSprite = 0;
    m_childChain = 0;
    m_childBacklink = 0;
    m_behave = SpriteSourceSelectBehavior(vidObject);

    unsigned int groupGridBit = 0u;
    if ((vidFlags&0x00020000u) != 0u)
    {
        MAP* const map = Map;
        const unsigned int active = map->m_playerIndex;
        PLAYER* const record = map->m_players[group];
        if (record->m_relation[active] == 1) groupGridBit = 1u;
    }
    flags = (m_runtimeFlags&0xE07E0380u) | (groupGridBit<<26u);
    m_runtimeFlags = flags;
    m_direction = ANGLE(0u);
    m_stateTimer = 0u;
    m_speed = 0.0f;
    m_zSpeed = 0.0f;
    const unsigned int currentGroup = (flags>>17u)&7u;
    m_hp = vidObject->maxHpByPlayer[currentGroup];

    m_extraData = 0;
    if (parentObject && (vidFlags&0x00800000u) != 0u)
    {
        m_impulseSpeedX = parentObject->m_impulseSpeedX;
        m_impulseSpeedY = parentObject->m_impulseSpeedY;
        m_impulseSpeedZ = parentObject->m_impulseSpeedZ;
    }
    else
    {
        m_impulseSpeedX = 0.0f;
        m_impulseSpeedY = 0.0f;
        m_impulseSpeedZ = 0.0f;
    }

    if (SpriteSourceNeedsRuntimeExtra(vidObject))
    {
        void* const storage = CrtNew(sizeof(EX_SPRITE_DATA));
        m_extraData = storage?new (storage) EX_SPRITE_DATA(this):0;
    }
    static_cast<void>(::AddCreateSprite = 0);

    unsigned int action = 0u;
    if (vidObject->noAnimCadr[0] == 0 && vidObject->noAnimCadr[2] == 0 && vidObject->noAnimCadr[15] != 0)
        action = 15u;
    else if (vidObject->noAnimCadr[14] != 0)
    {
        MAP* const map = Map;
        if ((map->m_flags&0x40u) == 0u) action = 14u;
    }
    m_currentAnimation = static_cast<int>(action);

    int firstFrame = vidObject->animationBaseFrame[action];
    const int frameCount = vidObject->animationFrameCount[action];
    if ((vidFlags&0x00002000u) != 0u)
    {
        EX_SPRITE_DATA* extraData = m_extraData;
        if (parentObject)
        {
            VID* parentVid = parentObject->m_vid;
            if ((parentVid->property&0x00002000u) != 0u)
            {
                EX_SPRITE_DATA* parentExtra = parentObject->m_extraData;
                extraData->reservedA4 = parentExtra->reservedA4;
            }
        }
        firstFrame += frameCount*static_cast<int>(extraData->reservedA4);
    }
    m_currentFrameBegin = firstFrame;
    m_currentFrame = firstFrame;
    m_currentFrameEnd = firstFrame+frameCount-1;
    ChangeDirection(ANGLE(direction));

    if (action == 0u)
    {
        const int range = m_currentFrameEnd-m_currentFrameBegin;
        if (range>0 && (vidFlags&0x01000000u) == 0u && (vidObject->spriteType&0x20u) == 0u)
        {
            const int frameOffset = (vidObject->runtimeFlags&0x00001000u) != 0u
                ? ::Random(range) : ::Random2(range);
            m_currentFrame = BitsToSigned32(static_cast<unsigned int>(m_currentFrame)+
                static_cast<unsigned int>(frameOffset));
        }
    }

    if (vidObject != ::EmptyVid)
    {
        ++m_referenceCount;
        if (vidObject != ::EmptyVid)
        {
            SPRITE_COLLECTOR* collector = ::Sprites;
            collector->Insert(this);
        }
    }

    CreateLink();
    const unsigned int runtimeGroup = (m_runtimeFlags>>17u)&7u;
    m_vid->lastSpriteCountChangeTick = ::RealCurrentTime;
    ++m_vid->spriteCountsByPlayer[runtimeGroup];

    if (m_currentAnimation != 14 && (m_vid->childVid[14] != 0 || m_vid->sfx[14] != 0))
    {
        MAP* const map = Map;
        if ((map->m_flags&0x40u) == 0u)(void) CreateChildFor(14, 0);
    }

    if ((m_vid->property&0x00020000u) != 0u)
    {
        MAP* const map = Map;
        if (((m_runtimeFlags>>17u)&7u) != map->m_playerIndex)
        {
            m_runtimeFlags |= 0x04000000u;
            if (m_childChain) m_childChain->InvisibleOn();
        }
    }

    if ((m_vid->property&0x28u) != 0u && (m_runtimeFlags&0x08000000u) == 0u)
        m_vid->SetGridZ(this);
}

SPRITE::SPRITE(const VECTOR& position, ANGLE direction)
{
    m_xyz = position;
    m_direction = direction;
    m_vid = ::EmptyVid;
    unsigned int flags = m_runtimeFlags&0xFF81FFFFu;
    const unsigned int tick = ::CurrentTime;
    m_lastAnimationTick = tick;
    m_birthTime = tick;
    m_referenceCount = 0u;
    m_childBacklink = 0;
    m_childChain = 0;
    m_goalSprite = 0;
    m_extraData = 0;
    m_behave = 0u;
    m_currentAnimation = 0;
    flags &= 0xFFFFFF83u;
    m_runtimeFlags = flags;

    const unsigned int sequence = static_cast<unsigned int>(m_vid->animationBaseFrame[0]);
    m_currentFrameBegin = static_cast<int>(sequence);
    m_currentFrame = static_cast<int>(sequence);
    m_currentFrameEnd = m_vid->animationFrameCount[0]-1;
    m_vid->lastSpriteCountChangeTick = ::RealCurrentTime;
    const unsigned int army = (flags>>17u)&7u;
    ++m_vid->spriteCountsByPlayer[army];
}


extern "C" void*  ConstructSpriteAtPosition(void* self, const VECTOR* position, unsigned int direction)
{
    new (self) SPRITE(*position, ANGLE(static_cast<unsigned char>(direction)));
    return self;
}

static SPRITE* SpriteCreateActionTarget(const VECTOR& position, ANGLE direction = ANGLE(0))
{
    void* const memory = CrtNew(sizeof(SPRITE));
    if (!memory) return 0;
    try
    {
        return new (memory) SPRITE(position, direction);
    }
    catch (...)
    {
        CrtFreePointer(memory);
        throw;
    }
}

EX_SPRITE_DATA::EX_SPRITE_DATA(const SPRITE* sprite)
{
    const VID* vid = sprite->Vid();

    cachedPosition = sprite->Coor();
    cachedPositionTick = 0u;
    lifetimeRemaining = static_cast<unsigned int>(vid->lifeTime);
    childCadence = 1u;
    effectTimestamp = ::CurrentTime;
    effectCurvePosition = 0.0f;
    gammaModulate = 0u;
    gammaAdd = 0u;
    reserved38 = 0xFFFFFFFFu;
    objectId = -1;
    tacticalCounter = 0u;

    new (&items) LIST<int>;
    items.m_count = 0;
    items.m_capacity = 0;
    items.m_data = 0;
    for (int lane = 0; lane<4; ++lane) LIST_Ctor(&actionLane[lane]);
    actionStackIndex = 0u;
    new (&nameText) STRING;
    numberInName = -1;
    envTextureVid = 0;

    const ADD_CREATE_SPRITE* const overrideState =
        ::AddCreateSprite;
    if (overrideState)
    {
        scale = overrideState->scale;
        gammaModulate = overrideState->gamma.modulate;
        gammaAdd = overrideState->gamma.add;
    }
    else
    {
        scale = VECTOR(1.0f, 1.0f, 1.0f);
    }

    vid = sprite->Vid();
    if ((vid->property&0x00100000u) != 0u)
    {
        if (lifetimeRemaining != 999999u && lifetimeRemaining>4u)
        {
            const int half = static_cast<int>(lifetimeRemaining)/2;
            const bool localRandom = (vid->runtimeFlags&0x00001000u) != 0u;
            const int jitter = localRandom ? Random(half) : Random2(half);
            lifetimeRemaining = lifetimeRemaining-(lifetimeRemaining/4u)+static_cast<unsigned int>(jitter);
        }

        if (::AddCreateSprite == 0)
        {
            vid = sprite->Vid();
            const bool localRandom = (vid->runtimeFlags&0x00001000u) != 0u;
            const double randomValue = localRandom ? Random(0.6f) : Random2(0.6f);
            const double delta = randomValue-static_cast<double>(0.3f);
            const float deltaX = static_cast<float>(delta*static_cast<double>(scale.x));
            const float deltaY = static_cast<float>(delta*static_cast<double>(scale.y));
            scale.x += deltaX;
            scale.y += deltaY;
            scale.z = static_cast<float>(static_cast<double>(scale.z)+delta*static_cast<double>(scale.z));
        }
    }

    vid = sprite->Vid();
    if ((vid->property&0x00002000u) != 0u)
    {
        const int maximum = vid->noDir-1;
        const bool localRandom = (vid->runtimeFlags&0x00001000u) != 0u;
        reservedA4 = static_cast<unsigned int>(localRandom ? Random(maximum) : Random2(maximum));
    }

    vid = sprite->Vid();
    const float minimum = vid->maxSpeed;
    const float maximum = vid->randomSpeed;
    const bool orderedDifferent = (minimum == minimum) && (maximum == maximum) && minimum != maximum;
    if (orderedDifferent)
    {
        const float range = maximum-minimum;
        const bool localRandom = (vid->runtimeFlags&0x00001000u) != 0u;
        maxSpeedOverride = minimum+(localRandom ? Random(range) : Random2(range));
    }
    else maxSpeedOverride = minimum;
}



static inline bool SpriteX87OrderedNonZero(const float& value, const float& zero)
{
    return !FloatEqualOrUnordered(value, zero);
}

static inline bool SpriteX87LessOrUnordered(const float& left, const float& right)
{
    return FloatLessOrUnordered(left, right);
}

void SPRITE::InsertItem(int item)
{
    EX_SPRITE_DATA* const extraData = SpriteEnsureExtraData(this);
    LIST<int>& items = extraData->items;
    int count = items.m_count;
    int capacity = items.m_capacity;
    if (count >= capacity)
    {
        const int newCapacity = capacity*2+4;
        if (newCapacity>capacity)
        {
            int* oldData = items.m_data;
            int* newData = static_cast<int*>(CrtNew(static_cast<unsigned int>(newCapacity)*4u));
            items.m_data = newData;
            if (!newData)
            {
                STRING subject;
                subject = "!!!ERROR!!!::LIST: Not enough memory %i";
                ::Error->LogExit(subject.m_value, newCapacity);
            }
            if (oldData)
            {
                for (int itemIndex = 0; itemIndex<items.m_capacity; ++itemIndex)
                    items.m_data[itemIndex] = oldData[itemIndex];
                CrtFreePointer(oldData);
            }
            items.m_capacity = newCapacity;
        }
    }
    int* data = items.m_data;
    data[items.m_count] = item;
    ++items.m_count;
}

void SPRITE::SetObjectID(int item)
{
    EX_SPRITE_DATA* const extraData = SpriteEnsureExtraData(this);

    extraData->objectId = item;
}


float SPRITE::MaxSpeed() const
{
    const EX_SPRITE_DATA* const extraData = m_extraData;
    VID* const vid = m_vid;
    float speed = extraData?extraData->maxSpeedOverride:vid->maxSpeed;
    if (m_currentAnimation == 6)
    {
        SPRITE* child = m_childChain;
        if (child)
        {
            VID* const childVid = child->Vid();
            WEAPON_RUNTIME* const childInfo = childVid->weapon;
            if ((childInfo->Property&0x00000800u) != 0u)
                speed *= (0.666660011f);
        }
    }
    if ((m_runtimeFlags&0x00004000u) != 0u) speed = -speed;
    return speed;
}

void SPRITE::SetMoveAnimation()
{
    int action = m_currentAnimation;
    if (action >= 15) return;
    if (action == 8 && m_currentFrame <= m_currentFrameEnd) return;

    SPRITE* linked = m_childBacklink;
    const float zero = (0.0f);
    const bool linkedMoving = linked && SpriteX87OrderedNonZero(linked->m_speed, zero);
    const bool selfMoving = SpriteX87OrderedNonZero(m_speed, zero);

    if (!linkedMoving && !selfMoving)
    {
        if (action != 0 && action != 10) ChangeAnimation(0);
        SPRITE* child = m_childChain;
        if (child && child->m_currentAnimation == 2) child->ChangeAnimation(0);
        return;
    }

    if (!selfMoving)
    {
        if (action == 10) return;
        const unsigned char linkedDirection = linked->m_direction.value;
        const unsigned char selfDirection = m_direction.value;
        const unsigned char forward = static_cast<unsigned char>(linkedDirection-selfDirection);
        const unsigned char backward = static_cast<unsigned char>(selfDirection-linkedDirection);
        const unsigned char difference = forward<backward?forward:backward;
        if (linked->m_currentAnimation != 6 && (difference<35u || difference>96u)) ChangeAnimation(2);
        else ChangeAnimation(0);
        return;
    }

    if ((m_runtimeFlags&0x00000400u) == 0u)
    {
        float current = m_speed;
        if (current<0.0f) current = -current;
        float maximum = MaxSpeed();
        if (maximum<0.0f) maximum = -maximum;
        if (maximum*(0.25f) >= current)
        {
            if (action != 1) ChangeAnimation(1);
            return;
        }
    }

    action = m_currentAnimation;
    if (action == 6)
    {
        SPRITE* child = m_childChain;
        if (child)
        {
            VID* const childVid = child->Vid();
            WEAPON_RUNTIME* const childInfo = childVid->weapon;
            if ((childInfo->Property&0x00000800u) != 0u)
            {
                const unsigned char selfDirection = m_direction.value;
                const unsigned char childDirection = child->m_direction.value;
                const unsigned char forward = static_cast<unsigned char>(selfDirection-childDirection);
                const unsigned char backward = static_cast<unsigned char>(childDirection-selfDirection);
                const unsigned char difference = forward<backward?forward:backward;
                if (difference>64u)
                {
                    float current = m_speed;
                    if (current<0.0f) current = -current;
                    float maximum = MaxSpeed();
                    if (maximum<0.0f) maximum = -maximum;
                    if (SpriteX87LessOrUnordered(maximum, current)) m_speed = MaxSpeed();
                    return;
                }
            }
        }
    }

    if (action != 2) ChangeAnimation(2);
}

int SPRITE::PercentHp() const
{
    const SPRITE* source = this;
    const SPRITE* child = m_childChain;
    if (child)
    {
        VID* const owner = m_vid;
        VID* const childOwner = child->m_vid;
        if (childOwner == owner->linkVid &&
            childOwner->nWeapon != 0 && childOwner->childVid[8] != 0)
        {
            const unsigned int army = (child->m_runtimeFlags>>17u)&7u;
            if (childOwner->maxHpByPlayer[army] != 0) source = child;
        }
    }
    VID* const sourceOwner = source->m_vid;
    const unsigned int army = (source->m_runtimeFlags>>17u)&7u;
    const int maxHp = sourceOwner->maxHpByPlayer[army];
    return maxHp?(MultiplySigned32Wrap(source->m_hp, 255)/maxHp):0;
}

int SPRITE::StartMove()
{
    if (MaxSpeed() == 0.0f) return 0;

    m_runtimeFlags &= ~0x00002000u;
    SPRITE* target = m_goalSprite;
    float routeLength = 0.0f;

    if (target)
    {
        if (m_xyz.x == target->m_xyz.x &&
            m_xyz.y == target->m_xyz.y)
            return 0;

        VID* vid = m_vid;
        if (vid->noDir == 1 || (vid->property&0x00000001u) != 0u)
        {
            VECTOR2 delta;
            delta = VECTOR2(target->m_xyz.x-m_xyz.x, target->m_xyz.y-m_xyz.y);
            const ANGLE direction = Decart2Polar(delta.x, delta.y, &routeLength);
            ChangeDirection(direction);
            vid = m_vid;
        }

        const unsigned int spriteTypeFlags = vid->spriteType;
        const unsigned int propertyFlags = vid->property;
        if ((spriteTypeFlags&0x00000200u) != 0u || vid->spriteClass == 5)
        {
            float distance = routeLength;
            if (distance == 0.0f)
            {
                VECTOR2 delta;
                delta.x = target->m_xyz.x-m_xyz.x;
                delta.y = target->m_xyz.y-m_xyz.y;
                distance = delta.Length();
                if (distance == 0.0f) return 0;
            }
            const float dz = target->m_xyz.z-m_xyz.z;
            const float vz = vid->CalculateZSpeed(dz, distance);
            m_zSpeed = vz;
        }
        else if ((propertyFlags&0x00000006u) == 0u)
        {
            const float selfZ = m_xyz.z;
            const float targetZ = target->m_xyz.z;
            if (selfZ<targetZ) m_zSpeed = vid->maxZSpeed;
            else if (selfZ>targetZ) m_zSpeed = -vid->maxZSpeed;
            else m_zSpeed = 0.0f;
        }
    }

    unsigned int flags = m_runtimeFlags;
    const int action = m_currentAnimation;
    if ((flags&0x00000400u) == 0u && action<15 && action != 3)
    {
        VID* vid = m_vid;
        if (m_speed == 0.0f &&
            (vid->noAnimCadr[3] != 0 || vid->childVid[3] != 0 || vid->sfx[3] != 0))
        {
            ChangeAnimation(3);
            vid = m_vid;
            if ((vid->property&0x00100000u) != 0u)
            {
                const int range = m_currentFrameEnd-m_currentFrameBegin;
                int randomValue;
                if ((vid->runtimeFlags&0x00001000u) != 0u)
                    randomValue = ::Random(range);
                else
                    randomValue = Random2(range);
                m_currentFrame += randomValue;
            }
        }
        else
        {
            bool useAction6 = false;
            SPRITE* child = m_childChain;
            if (child)
            {
                VID* const childVid = child->Vid();
                WEAPON_RUNTIME* const childInfo = childVid->weapon;
                VID* const vid = m_vid;
                if ((childInfo->Property&0x00000800u) != 0u && vid->noAnimCadr[6] != 0)
                {
                    const unsigned char selfDir = m_direction.value;
                    const unsigned char childDir = child->m_direction.value;
                    const unsigned char d0 = static_cast<unsigned char>(selfDir-childDir);
                    const unsigned char d1 = static_cast<unsigned char>(childDir-selfDir);
                    const unsigned char d = d0<d1?d0:d1;
                    if (d>64u) useAction6 = true;
                }
            }
            ChangeAnimation(useAction6?6:2);
        }
    }

    VID* vid = m_vid;
    if (vid->acceleration == (999999.0f))
        m_speed = MaxSpeed();

    flags = m_runtimeFlags;
    flags = (flags&0xFCFFFFFFu) | 0x00000400u;
    m_runtimeFlags = flags;
    return 1;
}


void SPRITE::SetNumberInName(int number)
{
    EX_SPRITE_DATA* extraData = m_extraData;
    const int previous = extraData?extraData->numberInName:-1;
    MAP* const map = Map;
    if (previous >= 0 && previous <= 9)
        --map->m_spriteCountsByNameNumber[previous];

    extraData = m_extraData;
    if (!extraData && number >= 0) extraData = SpriteEnsureExtraData(this);
    if (extraData) extraData->numberInName = number;

    if (number >= 0 && number <= 9)
        ++Map->m_spriteCountsByNameNumber[number];
}


void SPRITE::SetName(const STRING& nameValue)
{
    const STRING* name = &nameValue;
    STRING local(*name);
    const char* incoming = name->m_value;

    char* text = local.m_value;
    if (text[0] == 'm' && text[1] >= '0' && text[1] <= '9')
    {
        SetNumberInName(static_cast<int>(text[1]-'0'));
        text[0] = ' ';
        text[1] = ' ';
        local.RemoveBeginChars(" \n\r\t");
        text = local.m_value;
    }

    bool registerNewName = false;
    EX_SPRITE_DATA* extraData = m_extraData;
    if (extraData)
    {
        const char* oldName = extraData->nameText.value;
        if (oldName[0] != '\0' && text[0] == '\0')
        {
            SPRITE_COLLECTOR* registry = ::Sprites;
            registry->DeleteFromNamed(this);
        }
    }

    extraData = m_extraData;
    if ((!extraData || extraData->nameText.value[0] == '\0') && text[0] != '\0')
        registerNewName = true;

    if (!extraData && text[0] != '\0') extraData = SpriteEnsureExtraData(this);
    if (extraData) extraData->nameText = local;

    if (registerNewName)
    {
        SPRITE_COLLECTOR* registry = ::Sprites;
        registry->InsertToNamed(this);
    }
}












static unsigned int SpriteTacticalState(const SPRITE* sprite)
{
    return (sprite->RuntimeFlags()>>7)&7u;
}
static void SpriteSetTacticalState(SPRITE* sprite, unsigned int state)
{
    sprite->SetRuntimeFlags((sprite->RuntimeFlags()&0xFFFFFC7Fu) | ((state&7u)<<7));
}
static inline bool SpriteFloatLessOrUnordered(const float& left, const float& right)
{
    return FloatLessOrUnordered(left, right);
}
static inline bool SpriteFloatGreaterOrdered(const float& left, const float& right)
{
    return FloatGreaterOrdered(left, right);
}
static inline bool SpriteFloatEqualOrUnordered(const float& left, const float& right)
{
    return FloatEqualOrUnordered(left, right);
}
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma intrinsic(fabs)
#endif
static float SpriteFloatAbs(const float& value)
{
    return static_cast<float>(fabs(static_cast<double>(value)));
}
static int SpriteAbsSignedBits(int value)
{
    unsigned int bits = static_cast<unsigned int>(value);
    if (value<0) bits = 0u-bits;
    return static_cast<int>(bits);
}
static int SpriteAttackTactPost(SPRITE* sprite, bool enteredAttackProbe)
{
    SPRITE* previous = sprite->ChildBacklink();
    if (!previous || SpriteTacticalState(sprite) == 4u)
        return static_cast<int>(SpriteTacticalState(sprite));

    VID* const vid = sprite->Vid();
    if (vid->noAnimCadr[10] != 0)
    {
        VID* const previousVid = previous->Vid();
        const int previousType = previousVid->spriteClass;
        if (previousType == 7 || previousType == 2)
        {
            const unsigned int timer = sprite->StateTimer();
            const int action = sprite->Animation();
            if (timer != 0u)
            {
                if (action >= 6 && action != 10) sprite->ChangeAnimation(10);
            }
            else if (action == 10)
            {
                sprite->ChangeAnimation(0);
            }
        }
    }

    previous = sprite->ChildBacklink();
    VID* const previousVid = previous->Vid();
    if (previousVid->spriteClass == 7 || enteredAttackProbe)
        return static_cast<int>(SpriteTacticalState(sprite));

    const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
    SPRITE* const target = sprite->Goal();
    if (target)
    {
        const unsigned char desiredDirection = sprite->DirectionTo(target).value;
        (void) sprite->RotateTact(ANGLE(desiredDirection), static_cast<int>(elapsed)).value;
    }
    else if (sprite->StateTimer() == 0u)
    {
        (void) sprite->RotateTact(ANGLE(previous->Direction().value), static_cast<int>(elapsed)).value;
    }
    return static_cast<int>(SpriteTacticalState(sprite));
}
static SPRITE* SpriteHashFirst(const VECTOR* position, const VID* vid)
{
    SPRITE_COLLECTOR* spriteCollector = ::Sprites;
    const float halfX = vid->halfSizeXY.x;
    const float halfY = vid->halfSizeXY.y;
    return spriteCollector->FirstHashInBox(position->x-halfX, position->y-halfY,
        position->x+halfX, position->y+halfY);
}
static SPRITE* SpriteHashNext()
{
    SPRITE_COLLECTOR* spriteCollector = ::Sprites;
    return spriteCollector->NextHashInBox();
}
static bool SpriteCollisionAt(SPRITE* candidate, const VID* movingVid, const VECTOR* position, unsigned char direction)
{
    return candidate->IsCross(movingVid, *position, ANGLE(direction)) != 0;
}
static bool SpriteType7CollisionIsIgnored(const SPRITE* moving, const SPRITE* candidate)
{
    const VID* const candidateVid = candidate->Vid();
    if (candidateVid->spriteClass != 7) return false;
    if (((candidate->RuntimeFlags()^moving->RuntimeFlags())&0x000E0000u) != 0u) return false;
    const VID* const movingVid = moving->Vid();
    return (movingVid->property&0x40u) == 0u;
}
static WEAPON_RUNTIME* SpriteEffectiveCollisionInfo(const SPRITE* sprite)
{
    VID* const vid = sprite->Vid();
    if (vid->nWeapon == 0)
    {
        SPRITE* const linkedSprite = sprite->ChildChain();
        if (linkedSprite) return linkedSprite->Vid()->weapon;
    }
    return vid->weapon;
}
static void SpriteComputeUnusedTargetDirection(SPRITE* sprite)
{
    VID* const vid = sprite->Vid();
    WEAPON_RUNTIME* const weaponData = vid->weapon;
    if ((weaponData->Property&0x00001100u) != 0x00001100u) return;
    SPRITE* goalSprite = sprite->Goal();
    if (!goalSprite) return;
    const VECTOR2 delta(goalSprite->X()-sprite->X(), goalSprite->Y()-sprite->Y());
    const ANGLE direction(delta);
    (void) direction;
}
static float SpriteClampCrushVelocity(float value)
{
    const float magnitude = value<0.0f?-value:value;

    return magnitude>30.0f?5.0f:value;
}
static void SpriteResolveCrushGlide(SPRITE* moving, SPRITE* candidate,
    float movingMass, float candidateMass, const VID* movingVid)
{
    SPRITE_LAYOUT_ACCESS* const movingLayout = reinterpret_cast<SPRITE_LAYOUT_ACCESS*>(moving);
    SPRITE_LAYOUT_ACCESS* const candidateLayout = reinterpret_cast<SPRITE_LAYOUT_ACCESS*>(candidate);
    const float movingRadius = movingLayout->m_speed;
    const float candidateRadius = candidateLayout->m_speed;
    const double movingBaseX = static_cast<double>(moving->Direction().Sin())*movingRadius;
    const float movingBaseY = (-moving->Direction().Cos())*movingRadius;
    const double candidateInitialBaseX = static_cast<double>(candidate->Direction().Sin())*candidateRadius;
    const float candidateBaseX = static_cast<float>(candidateInitialBaseX);
    const float candidateBaseY = (-candidate->Direction().Cos())*candidateRadius;

    const float movingVX = static_cast<float>(movingBaseX+movingLayout->m_impulseSpeedX);
    const float movingVY = movingBaseY+movingLayout->m_impulseSpeedY;
    const float candidateVX = static_cast<float>(candidateInitialBaseX+candidateLayout->m_impulseSpeedX);
    const float candidateVY = candidateBaseY+candidateLayout->m_impulseSpeedY;
    const float deltaX = candidateVX-movingVX;
    const float deltaY = candidateVY-movingVY;
    const double invMass = 1.0/(static_cast<double>(candidateMass)+movingMass);
    const float movingInvMass = static_cast<float>(invMass);
    const float candidateFactor = (movingVid->weapon->Property&0x00000100u) != 0u?0.8f:0.5f;

    const float movingMomentumX = movingVX*movingMass;
    const float movingMomentumY = movingVY*movingMass;
    const float candidateMomentumX = candidateVX*candidateMass;
    const float candidateMomentumY = candidateVY*candidateMass;

    const double candidateWeight = static_cast<double>(candidateFactor)*movingMass;
    const float candidateWeightedX = static_cast<float>(candidateWeight*deltaX);
    const float candidateSumX = candidateWeightedX+movingMomentumX;
    const float candidateTotalX = candidateSumX+candidateMomentumX;
    const float candidateVelocityX = static_cast<float>(static_cast<double>(candidateTotalX)*invMass);
    const float newCandidateX = candidateVelocityX-candidateBaseX;
    const float newCandidateY = static_cast<float>(
        ((candidateWeight*deltaY+movingMomentumY)+candidateMomentumY)*invMass-candidateBaseY);
    candidateLayout->m_impulseSpeedX = SpriteClampCrushVelocity(newCandidateX);
    candidateLayout->m_impulseSpeedY = SpriteClampCrushVelocity(newCandidateY);

    const float oldMovingX = movingLayout->m_impulseSpeedX;
    const float oldMovingY = movingLayout->m_impulseSpeedY;
    if (SpriteFloatEqualOrUnordered(oldMovingX, 0.0f) &&
        SpriteFloatEqualOrUnordered(oldMovingY, 0.0f)) return;

    const float movingFactor = 0.5f*candidateMass;
    const float newMovingX = static_cast<float>(
        ((static_cast<double>(movingFactor)*deltaX+movingMomentumX)+candidateMomentumX)*movingInvMass-movingBaseX);
    const float movingWeightedY = movingFactor*deltaY;
    const float movingSumY = movingWeightedY+movingMomentumY;
    const float movingTotalY = movingSumY+candidateMomentumY;
    const float movingVelocityY = movingTotalY*movingInvMass;
    const float newMovingY = movingVelocityY-movingBaseY;
    movingLayout->m_impulseSpeedX = SpriteClampCrushVelocity(newMovingX);
    movingLayout->m_impulseSpeedY = SpriteClampCrushVelocity(newMovingY);
}

void SPRITE::SetScale(const VECTOR& scaleValue)
{
    SpriteEnsureExtraData(this);
    const unsigned int spatialMode = static_cast<unsigned int>(m_vid->layer);
    if (spatialMode == 1u || spatialMode == 2u)
        (void) ::Sprites->Delete(this);

    if ((m_vid->property&0x28u) != 0u && (m_runtimeFlags&0x08000000u) == 0u)
        m_vid->ResetGridZ(this);

    m_extraData->scale = scaleValue;

    if ((m_vid->property&0x28u) != 0u && (m_runtimeFlags&0x08000000u) == 0u)
        m_vid->SetGridZ(this);

    const unsigned int spatialModeAfter = static_cast<unsigned int>(m_vid->layer);
    if (spatialModeAfter == 1u || spatialModeAfter == 2u)
        ::Sprites->Insert(this);

    if ((m_vid->property&0x00800000u) == 0u) return;
    for (SPRITE* child = m_childChain; child; child = child->m_childChain)
    {
        const EX_SPRITE_DATA* const currentState = m_extraData;
        const VECTOR parentScale = currentState?currentState->scale:VECTOR(1.0f, 1.0f, 1.0f);
        const VECTOR childScale(parentScale.x*m_vid->scaleXYZ.x,
            parentScale.y*m_vid->scaleXYZ.y,
            parentScale.z*m_vid->scaleXYZ.z);
        child->SetScale(childScale);
    }
}

void SPRITE::SetEnvTexture(VID* textureVid)
{
    EX_SPRITE_DATA* extraData = m_extraData;
    if (!extraData) extraData = SpriteEnsureExtraData(this);

    extraData->envTextureVid = textureVid;
}

void SPRITE::SetGamma(const GAMMA& gammaValue)
{
    const GAMMA* gamma = &gammaValue;
    EX_SPRITE_DATA* extraData = m_extraData;
    const unsigned int packedModulate = gamma->modulate;
    const unsigned int packedAdd = gamma->add;
    if (!extraData && (packedModulate != 0u || packedAdd != 0u)) extraData = SpriteEnsureExtraData(this);
    if (extraData)
    {
        extraData->gammaModulate = packedModulate;
        extraData->gammaAdd = packedAdd;
    }
}

static int ClampPackedSignedGammaComponent(int value);

GAMMA SPRITE::GetGamma() const
{
    GAMMA result;
    const EX_SPRITE_DATA* const extraData = m_extraData;
    if (extraData && (extraData->gammaModulate != 0u || extraData->gammaAdd != 0u))
    {
        result.modulate = extraData->gammaModulate;
        result.add = extraData->gammaAdd;
    }
    else
    {
        const GAMMA& base = m_vid->gammaByPlayer[(m_runtimeFlags>>17)&7u];
        result.modulate = base.modulate;
        result.add = base.add;
    }
    if ((m_vid->runtimeFlags&1u) == 0u) return result;

    const WEAPON_RUNTIME* const info = m_vid->weapon;
    const int* const curves[4] = {info->GammaB, info->GammaG, info->GammaR, info->GammaA};
    int component[4];
    for (unsigned int channel = 0; channel<4u; ++channel)
    {
        const double frame = static_cast<double>(extraData->effectCurvePosition);
        const int frameIndex = TruncateDoubleToInt32(frame);
        const int limit = info->maxColumn;
        const int* const curve = curves[channel];
        if (frameIndex >= limit)
            component[channel] = curve[limit];
        else
        {
            const int lo = curve[frameIndex];
            const int difference = BitsToSigned32(static_cast<unsigned int>(curve[frameIndex+1])-
                static_cast<unsigned int>(lo));
            component[channel] = TruncateDoubleToInt32(
                static_cast<double>(difference)*(frame-static_cast<double>(frameIndex))+static_cast<double>(lo));
        }
    }

    const int alpha = ClampPackedSignedGammaComponent(component[3]);
    GAMMA adjustment;
    adjustment.modulate = alpha<0?static_cast<unsigned int>(-alpha)<<24:0u;
    adjustment.add = alpha >= 0?static_cast<unsigned int>(alpha)<<24:0u;
    adjustment.SetRed(component[2]);
    adjustment.SetGreen(component[1]);
    adjustment.SetBlue(component[0]);
    return GAMMA(result, adjustment);
}

SPRITE* SPRITE::CanPlaceInHash(const VECTOR& positionValue) const
{
    const VECTOR* const position = &positionValue;
    VID* const movingVid = m_vid;
    SPRITE* candidate = SpriteHashFirst(position, movingVid);
    while (candidate)
    {
        if (candidate != this && (candidate->m_behave&0x20u) == 0u)
        {
            VID* const candidateVid = candidate->Vid();
            if ((candidateVid->moveMask&movingVid->moveMask) != 0u &&
                SpriteCollisionAt(candidate, movingVid, position, m_direction.value) &&
                !SpriteType7CollisionIsIgnored(this, candidate))
                return candidate;
        }
        candidate = SpriteHashNext();
    }
    return 0;
}

SPRITE* SPRITE::CanPlace(const VECTOR& positionValue) const
{
    const VECTOR* const position = &positionValue;
    VID* const movingVid = m_vid;
    if (movingVid->moveMask == 0u) return 0;

    MAP* application = Map;
    if (application->CanPlaceInGridZ(movingVid, *position, ANGLE(m_direction.value)) != 0)
        return Mouse;

    SPRITE* candidate = SpriteHashFirst(position, movingVid);
    while (candidate)
    {
        if (candidate != this)
        {
            VID* const candidateVid = candidate->Vid();
            if ((candidateVid->moveMask&movingVid->moveMask) != 0u &&
                ((candidate->Behave()^m_behave)&0x20u) == 0u &&
                SpriteCollisionAt(candidate, movingVid, position, m_direction.value) &&
                !SpriteType7CollisionIsIgnored(this, candidate))
                return candidate;
        }
        candidate = SpriteHashNext();
    }
    return 0;
}

SPRITE* SPRITE::CanPlaceWithCrush(const VECTOR& positionValue)
{
    const VECTOR* const position = &positionValue;
    SPRITE* result = 0;
    VID* movingVid = m_vid;
    if (movingVid->moveMask == 0u) return 0;

    const unsigned char collisionDirection = m_direction.value;
    MAP* application = Map;
    if (application->CanPlaceInGridZ(movingVid, *position, ANGLE(collisionDirection)) != 0)
        return Mouse;

    movingVid = m_vid;
    WEAPON_RUNTIME* movingInfo = SpriteEffectiveCollisionInfo(this);
    const float movingMass = movingInfo->Weight;
    const float movingUpper = movingInfo->Power;
    SpriteComputeUnusedTargetDirection(this);

    movingVid = m_vid;
    SPRITE* candidate = SpriteHashFirst(position, movingVid);
    while (candidate)
    {
        movingVid = m_vid;
        if (candidate != this &&
            SpriteCollisionAt(candidate, movingVid, position, collisionDirection) &&
            !SpriteType7CollisionIsIgnored(this, candidate))
        {
            VID* const candidateVid = candidate->Vid();
            const unsigned int candidateFlags = candidate->Behave();
            const unsigned int movingFlags = m_behave;
            const bool sameCollisionSide = ((candidateFlags^movingFlags)&0x20u) == 0u;
            if (sameCollisionSide) movingVid = m_vid;
            const bool masksOverlap = (candidateVid->moveMask&movingVid->moveMask) != 0u;

            if (sameCollisionSide && masksOverlap)
            {
                const int crushScript = movingVid->scriptFunction[19];
                if (crushScript >= 0)
                {
                    (void) Map->ScriptRun(crushScript, this, candidate, 0);
                    return SpriteCollisionAt(candidate, m_vid, position, m_direction.value)?candidate:0;
                }

                WEAPON_RUNTIME* candidateInfo = SpriteEffectiveCollisionInfo(candidate);
                const float candidateMass = candidateInfo->Weight;
                const int candidateType = candidateVid->spriteClass;
                SPRITE* mapCollision = Mouse;

                if (candidate == mapCollision || SpriteFloatEqualOrUnordered(candidateMass, 999999.0f))
                {
                    m_impulseSpeedX = 0.0f;
                    m_impulseSpeedY = 0.0f;
                    m_impulseSpeedZ = 0.0f;
                }
                else if (!SpriteFloatEqualOrUnordered(movingMass, 999999.0f) &&
                    (candidateType != 7 || movingUpper>candidateMass))
                {
                    SpriteResolveCrushGlide(this, candidate, movingMass, candidateMass, movingVid);
                }

                movingVid = m_vid;
                movingInfo = movingVid->weapon;
                if ((movingInfo->Property&0x00000100u) == 0u)
                    return candidate;

                WEAPON_RUNTIME* movingEffectiveInfo = SpriteEffectiveCollisionInfo(this);
                WEAPON_RUNTIME* candidateEffectiveInfo = SpriteEffectiveCollisionInfo(candidate);
                const float ownReactionMass = movingEffectiveInfo->Weight;
                const float candidateReactionMass = candidateEffectiveInfo->Weight;
                const double numerator = static_cast<double>(SpriteFloatAbs(m_speed))+10.0;
                const int reaction = 1-TruncateDoubleToInt32((numerator*ownReactionMass*(-100.0))/candidateReactionMass);
                (void) candidate->Action(ACT_DAMAGE, reaction, 0, 0);

                if (SpriteFloatLessOrUnordered(ownReactionMass, candidateReactionMass))
                    candidate->m_speed *= 0.5f;

                movingVid = m_vid;
                movingInfo = movingVid->weapon;
                if ((movingInfo->Property&0x00000100u) == 0u || candidate->Animation()<15)
                    result = candidate;
            }
            else
            {
                const bool candidateForcesReaction = (candidateVid->property&0x00004000u) != 0u;
                movingVid = m_vid;
                movingInfo = movingVid->weapon;
                bool react = candidateForcesReaction || (movingInfo->Property&0x00020000u) != 0u;
                if (!react && (candidateFlags&0x20u) != 0u)
                {
                    WEAPON_RUNTIME* effective = SpriteEffectiveCollisionInfo(this);
                    react = !SpriteFloatEqualOrUnordered(effective->Power, 0.0f);
                }
                if (react)
                    (void) candidate->Action(ACT_DAMAGE, 5, 0, 0);
            }
        }
        candidate = SpriteHashNext();
    }
    return result;
}

static int ClampPackedSignedGammaComponent(int value)
{
    if (value < -255) return -255;
    if (value > 255) return 255;
    return value;
}

static const GAMMA& PackedSignedGammaSetComponent(GAMMA* self, int value, unsigned int component)
{
    value = ClampPackedSignedGammaComponent(value);
    self->value[component] = 0u;
    self->value[component+4u] = 0u;
    if (value<0) self->value[component] = static_cast<unsigned char>(-value);
    else self->value[component+4u] = static_cast<unsigned char>(value);
    return *self;
}


const GAMMA& GAMMA::SetRed(int value)
{
    return PackedSignedGammaSetComponent(this, value, 2u);
}


const GAMMA& GAMMA::SetGreen(int value)
{
    return PackedSignedGammaSetComponent(this, value, 1u);
}


const GAMMA& GAMMA::SetBlue(int value)
{
    return PackedSignedGammaSetComponent(this, value, 0u);
}

ANGLE SPRITE::RotateTact(ANGLE desiredDirectionValue, int elapsed)
{
    const unsigned char desiredDirection = desiredDirectionValue.value;
    SPRITE* child = m_childChain;
    if (child)
    {
        VID* const childVid = child->Vid();
        WEAPON_RUNTIME* const childInfo = childVid->weapon;
        VID* const ownVid = m_vid;
        if ((childInfo->Property&0x00000800u) != 0u && ownVid->noAnimCadr[6] != 0 &&
            SpriteCircularDirectionDifference(desiredDirection, m_direction.value)>64u)
            ChangeDirection(ANGLE(static_cast<unsigned char>(m_direction.value+128u)));
    }

    const unsigned char current = m_direction.value;
    if (desiredDirection == current) return ANGLE(0);

    VID* vid = m_vid;
    const float speed = vid->rotationSpeed;
    if (SpriteFloatEqualOrUnordered(speed, (0.0f)))
        return ANGLE(SpriteCircularDirectionDifference(current, desiredDirection));

    if (FloatBitsU32(speed) == 0x497423F0u)
    {
        ChangeDirection(desiredDirectionValue);
        return ANGLE(1);
    }

    int step = TruncateDoubleToInt32(static_cast<double>(elapsed)*speed+(0.5f));
    if (step == 0)
    {
        const unsigned int period = static_cast<unsigned int>(TruncateDoubleToInt32(1.0/static_cast<double>(speed)));
        const unsigned int now = ::CurrentTime;
        const unsigned int periodStart = now-(now%period);
        if (periodStart>now-static_cast<unsigned int>(elapsed)) step = 1;
        else return ANGLE(SpriteCircularDirectionDifference(current, desiredDirection));
    }

    const int direct = current>desiredDirection ? static_cast<int>(current)-static_cast<int>(desiredDirection)
        : static_cast<int>(desiredDirection)-static_cast<int>(current);
    const int wrapped = 256-direct;
    const bool rotateNegative = (current>desiredDirection)?(direct<wrapped):(direct>wrapped);
    const bool rotatePositive = !rotateNegative;
    const int distance = direct<wrapped?direct:wrapped;
    unsigned char proposed;
    if (step >= distance) proposed = desiredDirection;
    else if (rotateNegative) proposed = static_cast<unsigned char>(current-static_cast<unsigned char>(step));
    else proposed = static_cast<unsigned char>(current+static_cast<unsigned char>(step));

    SPRITE* previous = m_childBacklink;
    if (previous)
    {
        WEAPON_RUNTIME* info = vid->weapon;
        VID* const previousVid = previous->Vid();
        if ((info->Property&0x00000800u) != 0u && previousVid->noAnimCadr[6] == 0u)
        {
            const unsigned char previousDirection = previous->Direction().value;
            if (SpriteCircularDirectionDifference(proposed, previousDirection)>32u &&
                SpriteCircularDirectionDifference(desiredDirection, previousDirection)>32u)
            {
                if (rotateNegative) proposed = static_cast<unsigned char>(previousDirection-32u);
                else if (rotatePositive) proposed = static_cast<unsigned char>(previousDirection+32u);
            }
        }
    }

    ChangeDirection(ANGLE(proposed));
    if (proposed == desiredDirection) return ANGLE(0);
    return ANGLE(SpriteCircularDirectionDifference(m_direction.value, desiredDirection));
}

void SPRITE::Stop()
{
    unsigned int flags = m_runtimeFlags;
    if ((flags&0x7Cu) == 0u)
        this->SetGoal(0);

    flags = m_runtimeFlags;
    if ((flags&0x7Cu) == 0x04u)
    {
        if ((flags&0x00002000u) != 0u)
        {
            SPRITE* target = m_goalSprite;
            if (target)
            {
                m_runtimeFlags = flags&0xFFFFDFFFu;
                m_speed = 0.0f;
                (void) ChangeCoor(target->m_xyz);
            }
        }
        (void) SetCommandWithoutLink(0u, 0);
    }

    m_runtimeFlags &= 0xFCFFFBFFu;
    VID* vid = m_vid;
    if (SpriteFloatEqualOrUnordered(vid->slow, (999999.0f)))
        m_speed = 0.0f;
}

int SPRITE::CantMoveBulletTo(const VECTOR& positionValue) const
{
    const VECTOR* const position = &positionValue;
    VID* vid = m_vid;
    VID* projectileVid = vid->childVid[8];
    while (projectileVid && projectileVid->spriteClass == 12)
        projectileVid = projectileVid->childVid[8];
    if (!projectileVid) return 1;

    if (projectileVid->moveMask == 0u && vid->nVid != 1830)
        return 0;

    VECTOR probe;
    probe.x = m_xyz.x;
    probe.y = m_xyz.y;
    probe.z = m_xyz.z+vid->childZ[8];
    MAP* const map = Map;
    return map->IsLineUnderGround(probe, *position);
}

void SPRITE::SetGoal(SPRITE* target)
{
    SPRITE* current = m_goalSprite;
    if (current == target) return;
    if (current) (void) current->Release();
    m_goalSprite = target;
    if (target) ++target->m_referenceCount;
}

void SPRITE::Move(SPRITE* target)
{
    int result = SetCommandWithoutLink(1u, target);
    if (result == 0)
    {
        result = StartMove();
        if (result == 0)
        {
            (void) SetCommand(0u, 0);
            return;
        }
    }

    SPRITE* child = m_childChain;
    if (!child) return;
    VID* const selfVid = m_vid;
    VID* childVid = child->Vid();
    if (childVid != selfVid->linkVid) return;
    if (static_cast<unsigned int>(childVid->nWeapon) == 0u || childVid->childVid[8] == 0) return;
    const unsigned int childCommandBits = child->RuntimeFlags()&0x7Cu;
    if (childCommandBits == 0u || childCommandBits == 0x10u || childCommandBits == 0x14u) return;
    (void) child->SetCommand(0u, 0);
}

void SPRITE::Move(float x, float y, float z)
{
    void* const storage = CrtNew(104u);
    SPRITE* const target = storage?new (storage) SPRITE(VECTOR(x, y, z), ANGLE(0)):0;
    Move(target);
}

int SPRITE::SetCommand(int command, SPRITE* target)
{
    const unsigned int oldCommandBits = m_runtimeFlags&0x7Cu;
    if (oldCommandBits == 0x48u && command != 0x12)
        m_stateTimer = 0u;

    this->SetGoal(target);

    SPRITE* child = m_childChain;
    if (child)
    {
        VID* vid = m_vid;
        VID* childVid = child->Vid();
        if (childVid == vid->linkVid && childVid->nWeapon != 0 && childVid->childVid[8] != 0)
            (void) child->SetCommand(command, target);
    }

    if (command<16 && m_goalSprite == 0)
    {
        m_runtimeFlags &= 0xFFFFFF83u;
        return 1;
    }
    const unsigned int flags = m_runtimeFlags&0xFFFFFF83u;
    m_runtimeFlags = flags | ((static_cast<unsigned int>(command)&31u)<<2);
    return 0;
}


int SPRITE::SetCommandWithoutLink(int command, SPRITE* target)
{
    const unsigned int oldCommandBits = m_runtimeFlags&0x7Cu;
    if (oldCommandBits == 0x48u && command != 0x12)
        m_stateTimer = 0u;

    this->SetGoal(target);
    if (command<16 && m_goalSprite == 0)
    {
        m_runtimeFlags &= 0xFFFFFF83u;
        return 1;
    }
    const unsigned int flags = m_runtimeFlags&0xFFFFFF83u;
    m_runtimeFlags = flags | ((static_cast<unsigned int>(command)&31u)<<2);
    return 0;
}

int SPRITE::AddLinkToLast(SPRITE* link)
{
    if (!link || link->ChildBacklink() != 0) return 1;
    SPRITE* oldPrimary = m_childChain;
    if (oldPrimary)
    {
        oldPrimary->m_childBacklink = 0;
        (void) link->AddLink(oldPrimary);
    }
    m_childChain = link;
    link->m_childBacklink = this;
    return 0;
}


int SPRITE::AddLink(SPRITE* link)
{
    if (!link || link->ChildBacklink() != 0) return 1;
    SPRITE* tail = this;
    while (tail->ChildChain() != 0)
        tail = tail->ChildChain();
    tail->m_childChain = link;
    link->m_childBacklink = tail;
    return 0;
}


void SPRITE::BreakLink()
{
    SPRITE* next = m_childChain;
    if (next) next->m_childBacklink = m_childBacklink;
    SPRITE* previous = m_childBacklink;
    if (previous)
    {
        previous->m_childChain = m_childChain;
        m_childBacklink = 0;
    }
    m_childChain = 0;
}


void SPRITE::AddAction(int actionIndex, int command, int argument1, int argument2, int argument3)
{
    EX_SPRITE_DATA* extraData = SpriteEnsureExtraData(this);
    extraData->actionLane[static_cast<unsigned int>(actionIndex)].InsertFirst(
        ACT{command, argument1, argument2, argument3});
}


void SPRITE::AddActionImmediate(int actionIndex, int command, int argument1, int argument2, int argument3)
{
    EX_SPRITE_DATA* extraData = SpriteEnsureExtraData(this);
    extraData->actionLane[static_cast<unsigned int>(actionIndex)].Insert(
        ACT{command, argument1, argument2, argument3});
}

float SPRITE::Random(VID* vid, float scale) const
{
    if ((vid->runtimeFlags&0x1000u) != 0u) return ::Random(scale);
    return ::Random2(scale);
}

STRING SPRITE::GetTextActions(int actionIndex) const
{
    STRING text;

    const EX_SPRITE_DATA* extraData = m_extraData;
    if (extraData)
    {
        const LIST<ACT>* lane = (&extraData->actionLane[static_cast<unsigned int>(actionIndex)]);
        const ACT* actionRecords = lane->m_data;
        for (int index = 0; index<lane->m_count; ++index)
        {
            const ACT& actionRecord = actionRecords[index];
            STRING formattedAction = Printf("%c%i,%i,%i;", actionRecord.command+0x3C, actionRecord.arg1, actionRecord.arg2, actionRecord.arg3);
            (text += formattedAction);
        }
    }

    return STRING(text);
}

void SPRITE::SetTextActions(int actionIndex, const STRING& textValue)
{
    const STRING* actionText = &textValue;

    STRING remainingActionText;
    if (actionText->m_value[0] != '\0')
    {
        unsigned int actionTextLength = 0;
        while (actionText->m_value[actionTextLength] != '\0') ++actionTextLength;
        const unsigned int actionTextCapacity = (actionTextLength&0xFFFFFFF0u)+16u;
        remainingActionText.m_value = static_cast<char*>(CrtNew(actionTextCapacity));
        for (unsigned int index = 0; index<actionTextLength; ++index) remainingActionText.m_value[index] = actionText->m_value[index];
        remainingActionText.m_value[actionTextLength] = '\0';
    }
    else
    {
        remainingActionText.m_value = StringGlobals::EmptySentinel();
    }

    while (remainingActionText.m_value[0] != '\0')
    {
        int commandCharacter = 0;
        int argument1;
        int argument2;
        int argument3;
        (void) std::sscanf(remainingActionText.m_value, "%c%i,%i,%i", reinterpret_cast<char*>(&commandCharacter), &argument1, &argument2, &argument3);
        AddActionImmediate(actionIndex, commandCharacter-0x3C, argument1, argument2, argument3);

        STRING suffix = remainingActionText.After(";");
        remainingActionText = suffix;
    }
}





static void SpriteAppendItemToKnownExtra(EX_SPRITE_DATA* extraData, int item)
{
    LIST<int>& items = extraData->items;
    int count = items.m_count;
    int capacity = items.m_capacity;
    if (count >= capacity)
    {
        const int newCapacity = capacity*2+4;
        if (newCapacity>capacity)
        {
            int* oldData = items.m_data;
            int* newData = static_cast<int*>(CrtNew(static_cast<unsigned int>(newCapacity)*4u));
            items.m_data = newData;
            if (!newData)
            {
                STRING subject;
                subject = "!!!ERROR!!!::LIST: Not enough memory %i";
                ::Error->LogExit(subject.m_value, newCapacity);
            }
            if (oldData)
            {
                for (int index = 0; index<items.m_capacity; ++index)
                    items.m_data[index] = oldData[index];
                CrtFreePointer(oldData);
            }
            items.m_capacity = newCapacity;
        }
    }
    int* data = items.m_data;
    data[items.m_count] = item;
    ++items.m_count;
}

STRING SPRITE::GetTextItems()
{
    STRING text;

    EX_SPRITE_DATA* const extraData = m_extraData;
    if (extraData)
    {
        const int count = extraData->items.m_count;
        if (count != 0)
        {
            if (count>0)
            {
                int* data = extraData->items.m_data;
                for (int index = 0; index<count; ++index)
                {
                    STRING formatted = Printf("\x01%i", data[index]);
                    text += formatted;
                }
            }
            text += "\x02";
        }
    }

    return text;
}

void SPRITE::SetTextItems(const STRING& textValue)
{
    const STRING* text = &textValue;
    if ((std::strcmp(text->m_value, "\x03") == 0)) return;

    STRING remaining = text->After("\x01");

    EX_SPRITE_DATA* extraData = m_extraData;
    if (!extraData)
    {
        void* const storage = CrtNew(sizeof(EX_SPRITE_DATA));
        extraData = storage ? new (storage) EX_SPRITE_DATA(this) : 0;
        m_extraData = extraData;
    }

    while (!(std::strcmp(remaining.m_value, "\x03") == 0))
    {
        int value;
        (void) std::sscanf(remaining.m_value, "%i", &value);
        SpriteAppendItemToKnownExtra(m_extraData, value);

        STRING next = remaining.After("\x01");
        remaining = next;
    }
}


void SPRITE::AddActionAfterStop(int actionIndex, int command, int argument1, int argument2, int argument3)
{
    EX_SPRITE_DATA* extraData = SpriteEnsureExtraData(this);
    LIST<ACT>* lane = (&extraData->actionLane[static_cast<unsigned int>(actionIndex)]);
    const ACT* actionRecords = lane->m_data;
    if (lane->m_count>0)
    {
        for (int index = lane->m_count-1; index >= 0; --index)
        {
            const ACT& item = actionRecords[index];
            if (item.command == ACT_STOP_STACK && item.arg1 == 0 && item.arg2 == 0 && item.arg3 == 0)
            {
                (lane)->InsertBefore(index+1, ACT{command, argument1, argument2, argument3});
                return;
            }
        }
    }
    AddAction(actionIndex, command, argument1, argument2, argument3);
}


const LIST<ACT>& SPRITE::ActionStack(int actionIndex)
{
    static StaticObjectStorage<LIST<ACT> > s;
    static unsigned char initialized = 0;
    const unsigned char flags = initialized;
    if ((flags&1u) == 0u)
    {
        LIST<ACT>* const list = ::new (s.Address()) LIST<ACT>;
        list->m_data = 0;
        initialized = static_cast<unsigned char>(flags|1u);
        list->m_count = 0;
        list->m_capacity = 0;
        (void)RegisterCrtAtExit([]() { s.Get().~LIST<ACT>(); });
    }
    return m_extraData ? m_extraData->actionLane[actionIndex] : s.Get();
}

int SPRITE::ActionStackHaveCommand(int actionIndex, int command) const
{
    if (!m_extraData) return 0;
    const LIST<ACT>* lane = (&m_extraData->actionLane[static_cast<unsigned int>(actionIndex)]);
    const ACT* actionRecords = lane->m_data;
    for (int index = 0; index<lane->m_count; ++index)
        if (actionRecords[index].command == command) return 1;
    return 0;
}


int SPRITE::Attack(SPRITE* target)
{
    VID* vid = m_vid;
    if (vid->spriteClass == 12 && vid->childVid[8] != 0)
    {
        (void) SetCommand(4u, target);
        (void) CreateChildFor(8u, 0);
        (void) SetCommand(0u, 0);
        return 0;
    }

    bool setAttackCommand = (target == 0);
    if (target)
    {
        VID* const targetVid = target->m_vid;
        if (targetVid == ::EmptyVid)
            setAttackCommand = true;
        else if ((this->CanAttackThisSprite(target) != 0))
            setAttackCommand = true;
        else if (vid->spriteClass == 5)
            setAttackCommand = true;
    }
    if (setAttackCommand) (void) SetCommand(3u, target);
    return 0;
}

void SPRITE::ResetActionStack(int actionIndex)
{
    if (!m_extraData) return;
    LIST<ACT>* lane = (&m_extraData->actionLane[static_cast<unsigned int>(actionIndex)]);
    lane->m_capacity = 0;
    lane->m_count = 0;
    if (lane->m_data) (void)(CrtFreePointer(lane->m_data), 0u);
    lane->m_data = 0;
}


int SPRITE::CanAttackThisSprite(const SPRITE* target) const
{
    if (!target) return 0;
    VID* const selfVid = m_vid;
    if (selfVid->nWeapon != 0 && selfVid->childVid[8] != 0)
    {
        WEAPON_RUNTIME* info = selfVid->weapon;
        VID* const targetVid = target->m_vid;
        if ((targetVid->spriteType&static_cast<unsigned int>(info->SpriteType)) != 0u) return 1;
    }

    SPRITE* child = m_childChain;
    if (!child) return 0;
    VID* const childVid = child->m_vid;
    if (childVid != selfVid->linkVid) return 0;
    if (childVid->nWeapon == 0 || childVid->childVid[8] == 0) return 0;
    WEAPON_RUNTIME* childInfo = childVid->weapon;
    VID* const targetVid = target->m_vid;
    return (targetVid->spriteType&static_cast<unsigned int>(childInfo->SpriteType)) != 0u ? 1 : 0;
}

void SPRITE::PrimitiveTact()
{
    const unsigned int tick = ::CurrentTime;
    const unsigned int action = static_cast<unsigned int>(m_currentAnimation);
    const unsigned int lastTick = m_lastAnimationTick;
    const unsigned int interval = static_cast<unsigned int>(m_vid->frameSpeed[action]);
    if (static_cast<unsigned int>(tick-lastTick)<interval) return;
    const int frame = m_currentFrame+1;
    m_lastAnimationTick = tick;
    m_currentFrame = frame;
    if (frame>m_currentFrameEnd)
        m_currentFrame = m_currentFrameBegin;
}

int SPRITE::AttackTact()
{
    VID* const vid = m_vid;
    if (vid->nWeapon == 0 || vid->childVid[8] == 0)
    {
        SpriteSetTacticalState(this, 0u);
        return 0;
    }

    SPRITE* target = m_goalSprite;
    if (!target)
    {
        SpriteSetTacticalState(this, 0u);
        return SpriteAttackTactPost(this, false);
    }

    if (m_currentAnimation == 8 && (m_runtimeFlags&0x00010000u) == 0u)
    {
        SpriteSetTacticalState(this, 4u);
        return SpriteAttackTactPost(this, false);
    }

    const unsigned int commandBits = m_runtimeFlags&0x7Cu;
    if (commandBits != 0x14u && commandBits != 0x0Cu && commandBits != 0x10u)
    {
        SpriteSetTacticalState(this, 0u);
        return SpriteAttackTactPost(this, false);
    }

    VECTOR2 delta;
    delta.x = target->X()-m_xyz.x;
    delta.y = target->Y()-m_xyz.y;
    WEAPON_RUNTIME* vidInfo = vid->weapon;
    const float closeRange = vidInfo->BattleRange;
    const float preciseRange = vidInfo->DeadZone;
    const float distance = delta.NearLength();

    if (SpriteFloatGreaterOrdered(distance, closeRange))
    {
        const float baseRange = SpriteFloatGreaterOrdered(vidInfo->DetectRange, closeRange)
            ? vidInfo->DetectRange : closeRange;
        const float doubledRange = baseRange+baseRange;
        SpriteSetTacticalState(this, SpriteFloatLessOrUnordered(distance, doubledRange)?2u:3u);
        return SpriteAttackTactPost(this, false);
    }

    if (!SpriteFloatGreaterOrdered(distance, preciseRange))
    {
        SpriteSetTacticalState(this, 2u);
        return SpriteAttackTactPost(this, false);
    }

    if (m_stateTimer >= 5000u)
    {
        SpriteSetTacticalState(this, 4u);
        return SpriteAttackTactPost(this, false);
    }

    const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
    unsigned char desiredDirection = DirectionTo(target).value;
    if (RotateTact(ANGLE(desiredDirection), static_cast<int>(elapsed)).value != 0u && (m_vid->weapon->Property&1u) == 0u)
    {
        SpriteSetTacticalState(this, 0u);
        return SpriteAttackTactPost(this, true);
    }

    VID* const probeVid = m_vid;
    const int attackProbe = Action(ACT_GET_AMMO, 0, 0, 0);
    const int recoilMagnitude = SpriteAbsSignedBits(probeVid->noChild[8]);
    if (attackProbe<recoilMagnitude)
    {
        SpriteSetTacticalState(this, 0u);
        SPRITE* previous = m_childBacklink;
        if (previous && previous->Goal() == m_goalSprite)
            (void) previous->SetCommand(0u, 0);
        else
            (void) SetCommand(0u, 0);
        return SpriteAttackTactPost(this, true);
    }

    SPRITE* previous = m_childBacklink;
    if (!previous || previous->Vid()->spriteClass != 7)
    {
        if (SpriteFloatGreaterOrdered(distance, (115.0f)) &&
            (probeVid->weapon->Property&1u) == 0u &&
            CantMoveBulletTo(m_goalSprite->Coor()) != 0)
        {
            SpriteSetTacticalState(this, 2u);
            return 2;
        }
    }

    (void) Action(ACT_ADD_AMMO, BitsToSigned32(0u-static_cast<unsigned int>(m_vid->noChild[8])), 0, 0);
    ChangeAnimation(8);
    m_runtimeFlags &= 0xFFFEFFFFu;

    EX_SPRITE_DATA* actionStack = SpriteEnsureExtraData(this);
    const unsigned int counterBits = static_cast<unsigned int>(actionStack->tacticalCounter)+1u;
    actionStack->tacticalCounter = BitsToSigned32(counterBits);
    vidInfo = m_vid->weapon;
    if (BitsToSigned32(counterBits) >= vidInfo->clipSize)
    {
        actionStack->tacticalCounter = 0;
        m_stateTimer = vidInfo->fullReloadTime+5000u;
    }
    else
    {
        m_stateTimer = vidInfo->reloadTime+5000u;
    }

    previous = m_childBacklink;
    if (previous)
    {
        SpriteSetTacticalState(previous, 4u);
        VID* const previousVid = previous->Vid();
        const unsigned int previousCommand = previous->RuntimeFlags()&0x7Cu;
        if (previousVid->spriteClass != 7 && previousCommand != 0x04u && previousCommand != 0x20u &&
            previousCommand != 0x28u && (vidInfo->Property&0x00008000u) == 0u)
            previous->Stop();
    }
    else if (m_vid->spriteClass != 7 && (vidInfo->Property&0x00008000u) == 0u)
    {
        Stop();
    }

    SpriteSetTacticalState(this, 4u);
    return 4;
}


void SPRITE::InvisibleOff()
{
    SPRITE* current = this;
    while (current)
    {
        current->m_runtimeFlags &= 0xFBFFFFFFu;
        current = current->m_childChain;
    }
}


template<>
int LIST<int>::DeleteNumber(int index)
{
    if (index<0 || index >= m_count) return 1;
    --m_count;
    m_data[index] = m_data[m_count];
    return 0;
}

void SPRITE::DetectTact()
{
    if (m_currentAnimation >= 14) return;
    const int scriptId = m_vid->scriptFunction[20];
    if (scriptId<0) return;
    (void) Map->ScriptRun(scriptId, this, 0, 0);
}

void SPRITE::Draw()
{
    m_vid->Draw(this);
}

template<> LIST<ACT>::~LIST()
{
    if (m_data) CrtFreePointer(m_data);
    m_data = 0;
    m_count = 0;
}


void  LIST_Dtor(void* self)
{
    static_cast<LIST<ACT>*>(self)->LIST<ACT>::~LIST();
}


void __cdecl DestroySpriteInteractionList()
{
    interactionSprites.Get().LIST<SPRITE*>::~LIST();
}

template<>
void LIST<ACT>::Insert(ACT value)
{
    if (m_count >= m_capacity) Expand(m_capacity*2+4);
    m_data[m_count++] = value;
}


template<>
void LIST<ACT>::InsertFirst(ACT value)
{
    if (m_count >= m_capacity) Expand(m_capacity*2+4);
    for (int index = m_count; index>0; --index) m_data[index] = m_data[index-1];
    ++m_count;
    m_data[0] = value;
}


template<>
void LIST<ACT>::InsertBefore(int index, ACT value)
{
    if (m_count >= m_capacity) Expand(m_capacity*2+4);
    for (int current = m_count; current>index; --current) m_data[current] = m_data[current-1];
    ++m_count;
    m_data[index] = value;
}


template<>
void LIST<ACT>::Expand(int requestedCapacity)
{
    if (requestedCapacity <= m_capacity) return;
    ACT* oldData = m_data;
    ACT* newData = static_cast<ACT*>(CrtNew(static_cast<unsigned int>(requestedCapacity*sizeof(ACT))));
    m_data = newData;
    if (!newData)
    {
        const STRING subject("!!!ERROR!!!::LIST: Not enough memory %i");
        ::Error->LogExit(subject.m_value, requestedCapacity);
    }
    if (oldData)
    {
        for (int index = 0; index<m_capacity; ++index) m_data[index] = oldData[index];
        CrtFreePointer(oldData);
    }
    m_capacity = requestedCapacity;
}

int SPRITE::SetCommand(int command, const VECTOR& source)
{
    void* const storage = CrtNew(104u);
    SPRITE* const target = storage ? new (storage) SPRITE(source, ANGLE(0)) : 0;
    return SetCommand(command, target);
}

void SPRITE::ChangeCoorForLinkRotate(const VECTOR2& target)
{
    const float dx = target.x-m_xyz.x;
    const float dy = target.y-m_xyz.y;
    VID* vid = m_vid;
    const unsigned int flags = vid->property;

    if ((flags&0x02000000u) != 0u) return;
    if ((flags&0x28u) != 0u && (m_runtimeFlags&0x08000000u) == 0u)
        vid->ResetGridZ(this);

    VECTOR position(m_xyz.x+dx, m_xyz.y+dy, m_xyz.z);
    SPRITE_COLLECTOR* collector = ::Sprites;
    collector->ChangeCoor(this, position);

    EX_SPRITE_DATA* extraData = m_extraData;
    if (extraData)
    {
        const unsigned int now = ::CurrentTime;
        if (extraData->cachedPositionTick != now)
        {
            extraData->cachedPositionTick = now;
            extraData->cachedPosition = m_xyz;
        }
    }

    m_xyz.x += dx;
    m_xyz.y += dy;
    vid = m_vid;
    if ((vid->property&0x28u) != 0u && (m_runtimeFlags&0x08000000u) == 0u)
        vid->SetGridZ(this);
}

int SPRITE::MoveTactMapLimit(const VECTOR& point)
{
    unsigned int direction = 0xFFFFFFFFu;
    const float x = point.x;
    const float y = point.y;
    if (SpriteFloatLessOrUnordered(x, 0.0f)) direction = 64u;
    else if (SpriteFloatLessOrUnordered(y, 0.0f)) direction = 128u;
    else
    {
        MAP* game = Map;
        if (x == x && game->m_sizeXY.x == game->m_sizeXY.x && x >= game->m_sizeXY.x) direction = 192u;
        else if (y == y && game->m_sizeXY.y == game->m_sizeXY.y && y >= game->m_sizeXY.y) direction = 0u;
    }
    if (direction == 0xFFFFFFFFu) return 0;
    const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
    (void) RotateTact(ANGLE(static_cast<unsigned char>(direction)), static_cast<int>(elapsed));
    return 1;
}

void SPRITE::ChangeArmy(int army)
{
    const unsigned int oldArmy = (m_runtimeFlags>>17u)&7u;
    const unsigned int newArmy = static_cast<unsigned int>(army)&7u;

    SPRITE_COLLECTOR* collector = ::Sprites;
    collector->ChangeArmy(this, static_cast<int>(newArmy));

    m_runtimeFlags = (m_runtimeFlags&0xFFF1FFFFu) | (newArmy<<17u);
    if (m_childChain) m_childChain->ChangeArmy(static_cast<int>(newArmy));

    const int newMaximum = m_vid->maxHpByPlayer[newArmy];
    const int oldMaximum = m_vid->maxHpByPlayer[oldArmy];
    if (newMaximum != oldMaximum)
        ChangeHp(static_cast<int>(ScaleSignedFixedPoint8(m_hp, oldMaximum, newMaximum)));

    if ((m_vid->property&0x00020000u) != 0u)
    {
        const unsigned int activeArmy = Map->m_playerIndex;
        if (newArmy != activeArmy)
        {
            m_runtimeFlags |= 0x04000000u;
            if (m_childChain) m_childChain->InvisibleOn();
        }
        else
        {
            m_runtimeFlags &= ~0x04000000u;
            if (m_childChain) m_childChain->InvisibleOff();
        }
    }

    if (m_vid->spriteCountsByPlayer[oldArmy] != 0u)
        --m_vid->spriteCountsByPlayer[oldArmy];
    m_vid->lastSpriteCountChangeTick = ::RealCurrentTime;
    ++m_vid->spriteCountsByPlayer[newArmy];

    if ((m_runtimeFlags&0x000E0000u) == 0x00080000u)
    {
        SetNumberInName(-1);
        iQuestEvent* const observer = gpQuestEvent;
        if (observer) (void) observer->OnSpriteDeath(this);
    }
}


void SPRITE::DrawGoalLine()
{
    if (m_goalSprite)
    {
        const unsigned int command = m_runtimeFlags&0x7Cu;
        COLOR color;
        color.value = command == 0x0Cu ? GRAPH::RED.value :
            (command == 0x10u ? 0xFF804040u : GRAPH::GREEN.value);
        SPRITE* const goal = Goal();
        if (goal)
        {
            const VECTOR2 goalScreen = goal->ScreenXY();
            const VECTOR2 selfScreen = ScreenXY();
            Graph->Line(selfScreen, goalScreen, color);
        }
    }
    if (m_childChain && m_childChain->m_goalSprite) m_childChain->DrawGoalLine();
}


void SPRITE::Save(STREAM* stream) const
{
    const unsigned int flags = m_runtimeFlags;
    if (flags&0x00000800u) return;

    const unsigned int direction = (flags>>17)&7u;
    const unsigned int selfValue = PtrToU32(this);
    (void) stream->Write(&selfValue, 4u);
    (void) stream->Write(&m_vid->nVid, 4u);
    (void) stream->Write(&m_xyz.x, 4u);
    (void) stream->Write(&m_xyz.y, 4u);
    (void) stream->Write(&m_xyz.z, 4u);
    const unsigned int stateByte = static_cast<unsigned int>(m_direction.value);
    (void) stream->Write(&stateByte, 4u);
    (void) stream->Write(&direction, 4u);

    const EX_SPRITE_DATA* const gammaData = m_extraData;
    unsigned int gamma[2] = {0u, 0u};
    if (gammaData)
    {
        gamma[0] = gammaData->gammaModulate;
        gamma[1] = gammaData->gammaAdd;
    }
    (void) stream->Write(gamma, 8u);

    const EX_SPRITE_DATA* const scaleData = m_extraData;
    unsigned int scale[3] = {0x3F800000u, 0x3F800000u, 0x3F800000u};
    if (scaleData) std::memcpy(scale, &scaleData->scale, 12u);
    (void) stream->Write(&scale[0], 4u);
    (void) stream->Write(&scale[1], 4u);
    (void) stream->Write(&scale[2], 4u);
}

SPRITE* SPRITE::AskLine(VECTOR* targetValue) const
{
    VECTOR* const target = targetValue;
    VID* movingVid = m_vid;
    if (movingVid->moveMask == 0u) return 0;

    VECTOR position = m_xyz;

    const float dx = target->x-position.x;
    const float dy = target->y-position.y;
    const float dz = target->z-position.z;
    const float ax = dx<0.0f?-dx:dx;
    const float ay = dy<0.0f?-dy:dy;
    const float dominant = ax>ay?ax:ay;
    const float scaled = dominant*0.125f;
    const int steps = static_cast<int>(scaled);
    const float reciprocal = 1.0f/scaled;
    const float stepX = dx*reciprocal;
    const float stepY = dy*reciprocal;
    const float stepZ = dz*reciprocal;

    for (int stepIndex = 0; stepIndex<steps; ++stepIndex)
    {
        MAP* const map = Map;
        if (map->IsDotUnderGround(position) != 0)
        {
            *target = position;
            return Mouse;
        }

        position.x += stepX;
        position.y += stepY;
        position.z += stepZ;

        if (stepIndex != 0)
        {
            for (SPRITE* candidate = SpriteHashFirst(&position, movingVid); candidate; candidate = SpriteHashNext())
            {
                if (candidate == this) continue;
                if (((candidate->m_behave^m_behave)&0x20u) != 0u) continue;
                VID* candidateVid = candidate->m_vid;
                if ((candidateVid->moveMask&movingVid->moveMask) == 0u) continue;
                if (SpriteCollisionAt(candidate, movingVid, &position, m_direction.value))
                {
                    *target = position;
                    return candidate;
                }
            }
        }
    }
    return 0;
}

void SPRITE::MoveTact()
{
    if (m_vid->movementTactEnabled == 0) return;

    VECTOR candidate = m_xyz;
    MoveTactCalcCoor(&candidate);

    const VECTOR2 currentXY(m_xyz.x, m_xyz.y);
    const VECTOR2 candidateXY(candidate.x, candidate.y);
    const float currentGround = Map->GetGroundZ(currentXY);
    const float candidateGround = Map->GetGroundZ(candidateXY);
    const float moveUpLimit = candidateGround+m_vid->topZ;
    const unsigned int property = m_vid->property;

    const bool candidateNotAboveGround = (candidate.z != candidate.z) || candidate.z <= candidateGround;
    const bool currentAtOrAboveGround = (m_xyz.z == m_xyz.z) && (currentGround == currentGround) && m_xyz.z >= currentGround;
    if ((m_vid->spriteType&0x00000200u) != 0u && (property&0x00000006u) != 0u &&
        candidateNotAboveGround && currentAtOrAboveGround)
    {
        candidate.z = candidateGround+3.0f;
    }
    else
    {
        const bool zOrderedDifferent = (m_xyz.z == m_xyz.z) && (candidate.z == candidate.z) && m_xyz.z != candidate.z;
        const bool limitOrderedNonZero = (moveUpLimit == moveUpLimit) && moveUpLimit != 0.0f;
        if (zOrderedDifferent && (property&0x00000006u) == 0u && limitOrderedNonZero)
        {
            bool clearZSpeed = false;
            if (m_xyz.z<moveUpLimit)
            {
                if (candidate.z == candidate.z && moveUpLimit == moveUpLimit && candidate.z >= moveUpLimit)
                {
                    candidate.z = moveUpLimit;
                    clearZSpeed = true;
                }
            }
            else if (m_xyz.z>moveUpLimit)
            {
                if (SpriteFloatLessOrUnordered(candidate.z, moveUpLimit))
                {
                    candidate.z = moveUpLimit;
                    clearZSpeed = true;
                }
            }
            else if ((property&0x08000000u) == 0u)
            {
                clearZSpeed = true;
            }
            if (clearZSpeed) m_zSpeed = 0.0f;
        }
    }

    if (m_goalSprite && SpriteX87OrderedNonZero(m_speed, 0.0f))
    {
        if ((m_runtimeFlags&0x03000000u) == 0x03000000u)
        {
            Stop();
        }
        else
        {
            unsigned char desired = DirectionTo(m_goalSprite).value;
            if (SpriteFloatLessOrUnordered(m_speed, 0.0f)) desired = static_cast<unsigned char>(desired+128u);
            const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
            (void) RotateTact(ANGLE(desired), static_cast<int>(elapsed));
        }
    }

    const bool xChanged = (m_xyz.x == m_xyz.x) && (candidate.x == candidate.x) && m_xyz.x != candidate.x;
    const bool yChanged = (m_xyz.y == m_xyz.y) && (candidate.y == candidate.y) && m_xyz.y != candidate.y;
    if (xChanged || yChanged)
    {
        if (CanPlaceWithCrush(candidate) != 0)
        {
            m_zSpeed = 0.0f;
            m_speed = 0.0f;
            m_runtimeFlags |= 0x00001000u;
        }
        else
        {
            ChangeCoor(candidate);
        }
    }

    const bool zChanged = (m_xyz.z == m_xyz.z) && (candidate.z == candidate.z) && m_xyz.z != candidate.z;
    if (zChanged)
        ChangeCoor(VECTOR(m_xyz.x, m_xyz.y, candidate.z));
}

void SPRITE::Rotate(ANGLE direction)
{
    void* const allocation = CrtNew(sizeof(SPRITE));
    SPRITE* rotated = allocation?::new(allocation) SPRITE(m_xyz, direction):0;
    (void) SetCommandWithoutLink(7, rotated);
}

void SPRITE::DrawSecondaryInfo()
{
    GRAPH* const graph = Graph;
    float top = graph->ViewYMin();
    const float textX = m_xyz.x+22.0f;
    const int goalVid = m_goalSprite?m_goalSprite->m_vid->nVid:0;
    const int ground = TruncateFloatToInt32(Map->GetGroundZ(VECTOR2(m_xyz.x, m_xyz.y)));
    const int ammo = Action(ACT_GET_AMMO, 0, 0, 0);
    graph->PrintfXY(
        textX, top,
        "Ref%iCmd%1i ani/cdr%-2i/%i hp%i AT%i goal=%i spd=%i,%i tmr%i amm%i b%i mvE%1u%1u %i,%i,%i/%i",
        static_cast<int>(m_referenceCount),
        static_cast<int>((m_runtimeFlags>>2)&0x1Fu),
        m_currentAnimation, m_currentFrame, m_hp,
        static_cast<int>((m_runtimeFlags>>7)&7u), goalVid,
        TruncateFloatToInt32(m_speed*1000.0f), TruncateFloatToInt32(m_zSpeed*1000.0f),
        static_cast<int>(m_stateTimer), ammo, static_cast<int>(m_behave),
        static_cast<unsigned int>((m_runtimeFlags>>24)&1u),
        static_cast<unsigned int>((m_runtimeFlags>>25)&1u),
        TruncateFloatToInt32(m_xyz.x), TruncateFloatToInt32(m_xyz.y), TruncateFloatToInt32(m_xyz.z), ground);

    SPRITE* const child = m_childChain;
    if (child && child->m_vid == m_vid->linkVid)
    {
        top += 12.0f;
        const int childGoalVid = child->m_goalSprite?child->m_goalSprite->m_vid->nVid:0;
        const int childAmmo = child->Action(ACT_GET_AMMO, 0, 0, 0);
        graph->PrintfXY(
            textX, top,
            "Ref%iCmd%1i ani/cdr%-2i/%i hp%i AT%i goal=%i tmr%i amm%i dir%i",
            static_cast<int>(child->m_referenceCount),
            static_cast<int>((child->m_runtimeFlags>>2)&0x1Fu),
            child->m_currentAnimation, child->m_currentFrame, child->m_hp,
            static_cast<int>((child->m_runtimeFlags>>7)&7u), childGoalVid,
            static_cast<int>(child->m_stateTimer), childAmmo,
            static_cast<int>(child->m_direction.value));
    }

    if (m_extraData && m_extraData->actionLane[0].m_count != 0)
    {
        top += 12.0f;
        const LIST<ACT>& lane = m_extraData->actionLane[0];
        STRING commandText = Printf("%i - ", lane.m_count);
        const ACT* const records = lane.m_data;
        for (int index = lane.m_count-1; index >= 0; --index)
        {
            const ACT& action = records[index];
            STRING item = Printf("%i(%i,%i,%i) ", action.command, action.arg1, action.arg2, action.arg3);
            commandText += item;
        }
        COLOR color;
        color.value = 0xFFFFFFFFu;
        graph->PutsXY(graph->ViewXMin()+30.0f, top, commandText, color);
    }

    DrawGoalLine();
}

int SPRITE::IsRotateXYCross(const VID* otherVid, const VECTOR& otherCoor, ANGLE otherDirection) const
{
    const VID* const currentVid = m_vid;

    const bool currentRotates = (currentVid->property&0x100u) != 0u;
    const bool otherRotates = (otherVid->property&0x100u) != 0u;
    if (!currentRotates && !otherRotates) return 1;

    const float selfX = m_xyz.x;
    const float selfY = m_xyz.y;
    const unsigned int currentDirection = static_cast<unsigned int>(m_direction.value);
    const unsigned int otherDirectionIndex = static_cast<unsigned int>(otherDirection.value);

    const float kRotatingRadius = (0.270000011f);
    const float kPairRadius = (0.707060218f);
    const float kHalfX = (0.600000024f);
    const float kHalfY = (0.300000012f);

    const float currentRadius = currentVid->realSizeXYZ.y*kRotatingRadius;
    const float currentOffsetX = -ANGLE(static_cast<unsigned char>(currentDirection)).Sin()*currentRadius;
    const float currentOffsetY = ANGLE(static_cast<unsigned char>(currentDirection)).CosY()*currentRadius;

    if (currentRotates && otherRotates)
    {
        const float otherRadius = otherVid->realSizeXYZ.y*kPairRadius;
        const float otherOffsetX = -ANGLE(static_cast<unsigned char>(otherDirectionIndex)).Sin()*otherRadius;
        const float otherOffsetY = ANGLE(static_cast<unsigned char>(otherDirectionIndex)).CosY()*otherRadius;
        const float extentX = currentVid->realSizeXYZ.x*kHalfX + otherVid->realSizeXYZ.x*kHalfX;
        const float extentY = currentVid->realSizeXYZ.y*kHalfY + otherVid->realSizeXYZ.y*kHalfY;

        const float currentLobeX[2] = {selfX+currentOffsetX, selfX-currentOffsetX};
        const float currentLobeY[2] = {selfY+currentOffsetY, selfY-currentOffsetY};
        const float otherLobeX[2] = {otherCoor.x+otherOffsetX, otherCoor.x-otherOffsetX};
        const float otherLobeY[2] = {otherCoor.y+otherOffsetY, otherCoor.y-otherOffsetY};
        for (unsigned int currentLobeIndex = 0; currentLobeIndex<2u; ++currentLobeIndex)
            for (unsigned int otherLobeIndex = 0; otherLobeIndex<2u; ++otherLobeIndex)
                if (SpriteFloatLessOrUnordered(
                    SpriteFloatAbs(currentLobeX[currentLobeIndex]-otherLobeX[otherLobeIndex]), extentX) &&
                    SpriteFloatLessOrUnordered(
                    SpriteFloatAbs(currentLobeY[currentLobeIndex]-otherLobeY[otherLobeIndex]), extentY))
                    return 1;
        return 0;
    }

    if (currentRotates)
    {
        const float extentX = currentVid->realSizeXYZ.x*kHalfX + otherVid->halfSizeXY.x;
        const float extentY = currentVid->realSizeXYZ.y*kHalfY + otherVid->halfSizeXY.y;
        const float dx0 = SpriteFloatAbs((selfX+currentOffsetX)-otherCoor.x);
        const float dy0 = SpriteFloatAbs((selfY+currentOffsetY)-otherCoor.y);
        if (SpriteFloatLessOrUnordered(dx0, extentX) &&
            SpriteFloatLessOrUnordered(dy0, extentY)) return 1;
        const float dx1 = SpriteFloatAbs((selfX-currentOffsetX)-otherCoor.x);
        const float dy1 = SpriteFloatAbs((selfY-currentOffsetY)-otherCoor.y);
        return (SpriteFloatLessOrUnordered(dx1, extentX) &&
            SpriteFloatLessOrUnordered(dy1, extentY))?1:0;
    }

    const float otherRadius = otherVid->realSizeXYZ.y*kRotatingRadius;
    const float otherOffsetX = -ANGLE(static_cast<unsigned char>(otherDirectionIndex)).Sin()*otherRadius;
    const float otherOffsetY = ANGLE(static_cast<unsigned char>(otherDirectionIndex)).CosY()*otherRadius;
    const float extentX = currentVid->halfSizeXY.x + otherVid->realSizeXYZ.x*kHalfX;
    const float extentY = currentVid->halfSizeXY.y + otherVid->realSizeXYZ.y*kHalfY;
    const float dx0 = SpriteFloatAbs((otherCoor.x+otherOffsetX)-selfX);
    const float dy0 = SpriteFloatAbs((otherCoor.y+otherOffsetY)-selfY);
    if (SpriteFloatLessOrUnordered(dx0, extentX) &&
        SpriteFloatLessOrUnordered(dy0, extentY)) return 1;
    const float dx1 = SpriteFloatAbs((otherCoor.x-otherOffsetX)-selfX);
    const float dy1 = SpriteFloatAbs((otherCoor.y-otherOffsetY)-selfY);
    return (SpriteFloatLessOrUnordered(dx1, extentX) &&
        SpriteFloatLessOrUnordered(dy1, extentY))?1:0;
}

void SPRITE::DrawRectangle()
{
    VID* const vid = m_vid;
    if (vid->spriteClass == 23u)
    {
        DrawSecondaryInfo();
        return;
    }

    const unsigned int flags = vid->spriteType;
    COLOR color;
    if ((flags&2u) != 0u && (static_cast<unsigned char>(vid->property)&0x40u) != 0u &&
        (m_runtimeFlags&0x08000000u) == 0u)
        color.value = GRAPH::BLACK.value;
    else if (vid->layer == 7)
        color.value = GRAPH::WHITE.value;
    else if ((flags&8u) != 0u)
        color.value = GRAPH::GREEN.value;
    else if ((flags&0x10u) != 0u)
        color.value = GRAPH::LIGHTBLUE.value;
    else if ((flags&4u) != 0u)
        color.value = GRAPH::YELLOW.value;
    else if ((flags&0x40u) == 0u && (flags&0x200u) != 0u)
        color.value = GRAPH::GRAY.value;
    else if ((flags&0x40u) == 0u && vid->spriteClass == 10u)
        color.value = GRAPH::WHITE.value;
    else
        color.value = GRAPH::RED.value;

    const VECTOR2 center = ScreenXY();
    Graph->Box(center+vid->halfSizeXY, center-vid->halfSizeXY, color);

    if ((vid->spriteType&0x0Cu) != 0u)
    {
        COLOR verticalColor;
        verticalColor.value = GRAPH::BLUE.value;
        const VECTOR2 vertical(0.0f, vid->sizeXYZ.z);
        Graph->Line(center, center-vertical, verticalColor);
    }

    const VECTOR2 mapView = Map->m_shiftXY;
    Graph->PrintfXY(m_xyz.ViewCoor()-mapView, "%i", vid->nVid);

    if ((vid->property&0x100u) == 0u) return;

    const unsigned int direction = static_cast<unsigned int>(m_direction.value);
    const float radius = vid->realSizeXYZ.y*(0.270000011f);
    const VECTOR2 offset(
        -ANGLE(static_cast<unsigned char>(direction)).Sin()*radius,
        ANGLE(static_cast<unsigned char>(direction)).CosY()*radius);
    const VECTOR2 extent(
        vid->realSizeXYZ.x*(0.600000024f),
        vid->realSizeXYZ.y*(0.300000012f));
    COLOR collisionColor;
    collisionColor.value = GRAPH::YELLOW.value;

    const VECTOR2 firstCenter = center-offset;
    Graph->Box(firstCenter+extent, firstCenter-extent, collisionColor);
    const VECTOR2 secondCenter = center+offset;
    Graph->Box(secondCenter+extent, secondCenter-extent, collisionColor);
}


void SPRITE::CreateLink()
{
    if (!m_vid->linkVid) return;
    if (HaveLink() != 0 || m_vid->linkVid->notCreateAsChild != 0) return;

    VECTOR offset = m_vid->cadrLinkXYZ ? m_vid->cadrLinkXYZ[m_currentFrame] : m_vid->linkXYZ;
    if ((m_vid->linkVid->property&0x02000000u) == 0u)
    {
        const unsigned int direction = static_cast<unsigned int>(m_direction.value);
        const float x = offset.x;
        const float y = offset.y;
        offset.x = x*ANGLE(static_cast<unsigned char>(direction)).Cos()-y*ANGLE(static_cast<unsigned char>(direction)).Sin();
        offset.y = y*ANGLE(static_cast<unsigned char>(direction)).CosY()+x*ANGLE(static_cast<unsigned char>(direction)).SinY();
    }

    const ANGLE direction(m_direction);
    const VECTOR position = Coor()+offset;
    VID* const linkVid = m_vid->linkVid;
    (void) AddLinkToLast(Map->CreateSprite(linkVid, position, direction, 0));
    SPRITE* child = Link();

    if (child)
    {
        const unsigned int army = (m_runtimeFlags>>17u)&7u;
        child->ChangeArmy(static_cast<int>(army));

        child = Link();
        VID* const childVid = child->m_vid;
        if ((childVid->weapon->Property&0x00004000u) != 0u && m_vid->maxHp != 0)
        {
            int hp = m_hp;
            if (hp<0) hp = 0;
            else if (hp>m_vid->maxHp) hp = m_vid->maxHp;
            const int ratio = BitsToSigned32(static_cast<unsigned int>(hp)*255u)/m_vid->maxHp;
            const unsigned char direction = static_cast<unsigned char>(-Link()->m_vid->directionQuantizationOffset-ratio);
            Link()->ChangeDirection(ANGLE(direction));
        }

        if ((m_vid->property&0x00800000u) != 0u)
        {
            VECTOR scale(1.0f, 1.0f, 1.0f);
            if (m_extraData) scale = m_extraData->scale;
            scale.x *= m_vid->scaleXYZ.x;
            scale.y *= m_vid->scaleXYZ.y;
            scale.z *= m_vid->scaleXYZ.z;
            Link()->SetScale(scale);
        }
        return;
    }

    const STRING subject("link");
    VID* const owner = m_vid;
    const int vidId = owner?owner->nVid:-1;
    const STRING objectFormat("SPRITE [%i](%i,%i,%i)");
    const int iz = TruncateFloatToInt32(m_xyz.z);
    const int iy = TruncateFloatToInt32(m_xyz.y);
    const int ix = TruncateFloatToInt32(m_xyz.x);
    MYERROR* const sink = ::Error;
    sink->Error(objectFormat, E_CREATE, subject, 0u, vidId, ix, iy, iz);
}

void SPRITE::RotateForGlide(ANGLE direction, int elapsed)
{
    VID* vid = m_vid;
    const float oldExtentX = vid->forMoveUpZ;
    const float oldExtentY = vid->forMoveDownZ;
    vid->forMoveUpZ = oldExtentX+oldExtentX;
    vid->forMoveDownZ = oldExtentY+oldExtentY;

    const unsigned char requested = direction.value;
    unsigned char glide = requested;
    const float x = m_xyz.x;
    const float y = m_xyz.y;
    const float z = m_xyz.z;
    const float longProbe = 24.0f;
    const float shortProbe = 12.0f;

    VECTOR firstProbe = {x, y, z};
    VECTOR secondProbe = {x, y, z};

    if (static_cast<unsigned char>(requested-113u) <= 30u)
    {
        firstProbe.x = x-longProbe;
        firstProbe.y = y+shortProbe;
        secondProbe.x = x+longProbe;
        secondProbe.y = y+shortProbe;
        if (CanPlace(firstProbe) != 0 && CanPlace(secondProbe) == 0)
            glide = 88u;
        else if (CanPlace(secondProbe) != 0 && CanPlace(firstProbe) == 0)
            glide = 168u;
    }

    else if (static_cast<unsigned char>(requested-16u) <= 224u)
    {
        if (static_cast<unsigned char>(requested+79u) <= 30u)
        {
            firstProbe.x = x-shortProbe;
            firstProbe.y = y+longProbe;
            secondProbe.x = x-shortProbe;
            secondProbe.y = y-longProbe;
            if (CanPlace(firstProbe) != 0 && CanPlace(secondProbe) == 0)
                glide = 216u;
            else if (CanPlace(secondProbe) != 0 && CanPlace(firstProbe) == 0)
                glide = 168u;
        }

        else if (static_cast<unsigned char>(requested-49u) <= 30u)
        {
            firstProbe.x = x+shortProbe;
            firstProbe.y = y+longProbe;
            secondProbe.x = x+shortProbe;
            secondProbe.y = y-longProbe;
            if (CanPlace(firstProbe) != 0 && CanPlace(secondProbe) == 0)
                glide = 40u;
            else if (CanPlace(secondProbe) != 0 && CanPlace(firstProbe) == 0)
                glide = 88u;
        }
    }
    else
    {
        firstProbe.x = x-longProbe;
        firstProbe.y = y-shortProbe;
        secondProbe.x = x+longProbe;
        secondProbe.y = y-shortProbe;
        if (CanPlace(firstProbe) != 0 && CanPlace(secondProbe) == 0)
            glide = 40u;
        else if (CanPlace(secondProbe) != 0 && CanPlace(firstProbe) == 0)
            glide = 216u;
    }

    vid->forMoveUpZ = oldExtentX;
    vid->forMoveDownZ = oldExtentY;

    (void) RotateTact(ANGLE(glide), elapsed);
}

void SPRITE::MoveTactCalcCoor(VECTOR* outPosition)
{
    m_runtimeFlags &= ~0x00001000u;
    outPosition->x = m_xyz.x;
    outPosition->y = m_xyz.y;
    outPosition->z = m_xyz.z;

    const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
    const double elapsedUnsigned = static_cast<double>(elapsed);
    const float impulseDisplacementX = static_cast<float>(elapsedUnsigned*m_impulseSpeedX);
    outPosition->x += impulseDisplacementX;
    outPosition->y = static_cast<float>(static_cast<double>(outPosition->y)+elapsedUnsigned*m_impulseSpeedY);

    CONSTANT* app = ::Const;
    const double damping = elapsedUnsigned*Const->Friction;

    double impulseX = m_impulseSpeedX;
    if (impulseX>0.0f)
    {
        impulseX -= damping;
        if ((impulseX<0.0 || impulseX != impulseX)) impulseX = 0.0f;
    }
    else
    {
        impulseX += damping;
        if (impulseX>0.0f) impulseX = 0.0f;
    }
    m_impulseSpeedX = static_cast<float>(impulseX);

    double impulseY = m_impulseSpeedY;
    if (impulseY>0.0f)
    {
        impulseY -= damping;
        if ((impulseY<0.0 || impulseY != impulseY)) impulseY = 0.0f;
    }
    else
    {
        impulseY += damping;
        if (impulseY>0.0f) impulseY = 0.0f;
    }
    m_impulseSpeedY = static_cast<float>(impulseY);

    VID* vid = m_vid;
    const bool accelerate = (m_runtimeFlags&0x00000400u) != 0u;
    const bool reverseAcceleration = (m_runtimeFlags&0x00004000u) != 0u;
    const unsigned int sentinelBits = 0x497423F0u;

    if (accelerate)
    {
        const double speed = m_speed;
        const float maximum = MaxSpeed();
        if (SpriteFloatAbs(maximum)>SpriteFloatAbs(static_cast<float>(speed)))
        {
            if (FloatBitsU32(vid->acceleration) == sentinelBits)
            {
                m_speed = MaxSpeed();
            }
            else
            {
                const float sign = reverseAcceleration?-1.0f:1.0f;
                const float changed = static_cast<float>(static_cast<double>(speed)+
                    elapsedUnsigned*vid->acceleration*sign);
                m_speed = changed;
                const float limit = MaxSpeed();
                if (!(SpriteFloatAbs(limit)>SpriteFloatAbs(changed)))
                    m_speed = MaxSpeed();
            }
        }
        else if (static_cast<int>(vid->spriteClass) == 5)
        {
            const float current = m_speed;
            const float limit = MaxSpeed();
            if (SpriteFloatLessOrUnordered(limit, current))
            {
                if (FloatBitsU32(vid->slow) == sentinelBits)
                {
                    m_speed = MaxSpeed();
                }
                else
                {
                    const float sign = reverseAcceleration?-1.0f:1.0f;
                    const double elapsedSigned = static_cast<double>(static_cast<int>(elapsed));
                    const float changed = static_cast<float>(static_cast<double>(current)-
                        sign*elapsedSigned*vid->slow);
                    m_speed = changed;
                    const float currentLimit = MaxSpeed();
                    if (SpriteFloatAbs(currentLimit)>SpriteFloatAbs(changed))
                        m_speed = MaxSpeed();
                }
            }
        }
    }
    else
    {
        const float deceleration = vid->slow;
        double speed = m_speed;
        if (FloatBitsU32(deceleration) == sentinelBits && SpriteX87OrderedNonZero(static_cast<float>(speed), 0.0f))
        {
            m_speed = 0.0f;
        }
        else if (speed>0.0f)
        {
            speed -= elapsedUnsigned*deceleration;
            if ((speed<0.0 || speed != speed)) speed = 0.0;
            m_speed = static_cast<float>(speed);
        }
        else if (speed<0.0 || speed != speed)
        {
            speed += elapsedUnsigned*deceleration;
            if (speed>0.0f) speed = 0.0f;
            m_speed = static_cast<float>(speed);
        }
    }

    const double speed = m_speed;
    if (!SpriteFloatEqualOrUnordered(static_cast<float>(speed), 0.0f))
    {
        if (FloatBitsU32(static_cast<float>(speed)) == sentinelBits)
        {
            SPRITE* goal = m_goalSprite;
            if (goal && (m_runtimeFlags&0x03000000u) != 0x03000000u)
            {
                outPosition->x = goal->m_xyz.x;
                outPosition->y = goal->m_xyz.y;
                outPosition->z = goal->m_xyz.z;
                SPRITE* collision = AskLine(outPosition);
                ChangeCoor(*outPosition);
                if (collision) m_runtimeFlags |= 0x00001000u;
                m_runtimeFlags |= 0x03000000u;
                return;
            }
            m_runtimeFlags |= 0x03000000u;
        }
        else
        {
            unsigned char direction = m_direction.value;
            WEAPON_RUNTIME* info = vid->weapon;
            if ((info->Property&0x00001000u) != 0u)
            {
                SPRITE* goal = m_goalSprite;
                if (goal)
                {
                    const float dx = goal->m_xyz.x-m_xyz.x;
                    const float dy = goal->m_xyz.y-m_xyz.y;
                    direction = Decart2Polar(dx, dy).value;
                }
            }
            const unsigned int tableOffset = static_cast<unsigned int>(direction)*4u;
            const double velocityX = static_cast<double>(ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Sin())*speed;
            const float velocityY = static_cast<float>(-ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Cos()*speed);
            const float elapsedForMovement = static_cast<float>(elapsedUnsigned);
            outPosition->x = static_cast<float>(static_cast<double>(outPosition->x)+velocityX*elapsedForMovement);
            const float displacementY = velocityY*elapsedForMovement;
            outPosition->y += displacementY;
        }
    }

    const unsigned int vidFlags = vid->property;
    const double elapsedSigned = static_cast<double>(static_cast<int>(elapsed));
    if ((vidFlags&0x02u) != 0u)
        m_zSpeed = static_cast<float>(static_cast<double>(m_zSpeed)-elapsedSigned*Const->Gravitation);
    else if ((vidFlags&0x04u) != 0u)
        m_zSpeed = static_cast<float>(static_cast<double>(m_zSpeed)-elapsedSigned*Const->Gravitation2);
    outPosition->z = static_cast<float>(static_cast<double>(outPosition->z)+elapsedSigned*m_zSpeed);

    SPRITE* goal = m_goalSprite;
    if (!goal) return;

    const double half = 0.5;
    const float currentX = m_xyz.x;
    const float nextX = outPosition->x;
    const float goalX = goal->m_xyz.x;
    bool reachedX = false;
    if (SpriteFloatLessOrUnordered(currentX, nextX))
    {
        if (!(goalX<static_cast<double>(currentX)-half || goalX != goalX || currentX != currentX) &&
            (goalX <= static_cast<double>(nextX)+half || goalX != goalX || nextX != nextX)) reachedX = true;
    }
    else
    {
        if (!(goalX<static_cast<double>(nextX)-half || goalX != goalX || nextX != nextX) &&
            (goalX <= static_cast<double>(currentX)+half || goalX != goalX || currentX != currentX)) reachedX = true;
    }
    if (reachedX) m_runtimeFlags |= 0x01000000u;

    const float currentY = m_xyz.y;
    const float nextY = outPosition->y;
    const float goalY = goal->m_xyz.y;
    bool reachedY = false;
    if (SpriteFloatLessOrUnordered(currentY, nextY))
    {
        if (!(goalY<static_cast<double>(currentY)-half || goalY != goalY || currentY != currentY) &&
            (goalY <= static_cast<double>(nextY)+half || goalY != goalY || nextY != nextY)) reachedY = true;
    }
    else
    {
        if (!(goalY<static_cast<double>(nextY)-half || goalY != goalY || nextY != nextY) &&
            (goalY <= static_cast<double>(currentY)+half || goalY != goalY || currentY != currentY)) reachedY = true;
    }
    if (reachedY) m_runtimeFlags |= 0x02000000u;
}

static void SpriteChangeCoorEnsureInteractionList()
{
    unsigned char flags = interactionSpritesInitialized;
    if ((flags&1u) != 0u) return;
    LIST<SPRITE*>* const list = new (interactionSprites.Address()) LIST<SPRITE*>;
    list->m_data = 0;
    interactionSpritesInitialized = static_cast<unsigned char>(flags | 1u);
    list->m_count = 0;
    list->m_capacity = 0;
    (void) RegisterCrtAtExit(&DestroySpriteInteractionList);
}
static void SpriteChangeCoorResetInteractionList()
{
    SpriteChangeCoorEnsureInteractionList();
    interactionSprites.Get().m_count = 0;
    if (interactionSprites.Get().m_capacity<0)
        interactionSprites.Get().Expand(0);
}
static void SpriteChangeCoorAppendInteraction(SPRITE* sprite)
{
    int count = interactionSprites.Get().m_count;
    const int capacity = interactionSprites.Get().m_capacity;
    if (count >= capacity)
    {
        interactionSprites.Get().Expand(AddSigned32Wrap(MultiplySigned32Wrap(capacity, 2), 4));
        count = interactionSprites.Get().m_count;
    }
    interactionSprites.Get().m_data[count] = sprite;
    interactionSprites.Get().m_count = AddSigned32Wrap(interactionSprites.Get().m_count, 1);
}
static SPRITE* SpriteChangeCoorInteractionAt(unsigned int index)
{
    return interactionSprites.Get().m_data[index];
}
static unsigned char SpriteChangeCoorImpulseDirection(const SPRITE* root, WEAPON_RUNTIME* rootInfo)
{
    if ((rootInfo->Property&0x00001000u) != 0u)
    {
        SPRITE* target = root->Goal();
        if (target)
        {
            const VECTOR2 delta(target->X()-root->X(), target->Y()-root->Y());
            return ANGLE(delta).value;
        }
    }
    return root->Direction().value;
}
static void SpriteChangeCoorCollectInteractions(SPRITE* movingSprite)
{
    SpriteChangeCoorResetInteractionList();
    SPRITE_COLLECTOR* collector = ::Sprites;
    VID* const nodeVid = movingSprite->Vid();

    const float x = movingSprite->X();
    const float y = movingSprite->Y();
    SPRITE* candidate = collector->FirstHashInBox(
        x-nodeVid->sizeXYZ.x, y-nodeVid->sizeXYZ.y,
        x+nodeVid->sizeXYZ.x, y+nodeVid->sizeXYZ.y);
    while (candidate)
    {
        if (candidate != movingSprite)
        {
            VID* const candidateVid = candidate->Vid();
            const bool overlap =
                SpriteFloatLessOrUnordered(x+nodeVid->minXGridZ, candidate->X()+candidateVid->sizeXYZ.x) &&
            x+nodeVid->maxXGridZ>candidate->X()-candidateVid->sizeXYZ.x &&
            SpriteFloatLessOrUnordered(y+nodeVid->minYGridZ, candidate->Y()+candidateVid->sizeXYZ.y) &&
            y+nodeVid->maxYGridZ>candidate->Y()-candidateVid->sizeXYZ.y;
            if (overlap)
            {
                MAP* world = Map;
                if (world->CanPlaceInGridZ(candidateVid, candidate->Coor(), candidate->Direction()) != 0)
                    SpriteChangeCoorAppendInteraction(candidate);
            }
        }
        candidate = collector->NextHashInBox();
    }
}
static void SpriteChangeCoorProcessInteractions(SPRITE* rootSprite, SPRITE* movingSprite, const VECTOR& delta)
{
    VID* const nodeVid = movingSprite->Vid();
    WEAPON_RUNTIME* movingWeaponData = nodeVid->weapon;
    const unsigned int nodeFlags = movingWeaponData->Property;
    if ((nodeFlags&0x00000200u) == 0u)
    {
        if ((nodeFlags&0x00000100u) == 0u) return;
        if (!SpriteX87OrderedNonZero(delta.x, 0.0f) && !SpriteX87OrderedNonZero(delta.y, 0.0f) &&
            !SpriteX87OrderedNonZero(delta.z, 0.0f)) return;
    }

    SpriteChangeCoorEnsureInteractionList();
    VID* const rootVid = rootSprite->Vid();
    WEAPON_RUNTIME* rootWeaponData = rootVid->weapon;
    float impulseX = 0.0f, impulseY = 0.0f;
    if ((rootWeaponData->Property&0x00000100u) != 0u)
    {
        const unsigned char direction = SpriteChangeCoorImpulseDirection(rootSprite, rootWeaponData);
        const unsigned int tableOffset = static_cast<unsigned int>(direction)*4u;
        const float speed = reinterpret_cast<SPRITE_LAYOUT_ACCESS*>(rootSprite)->m_speed;
        impulseX = ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Sin()*speed*0.8999999762f;
        impulseY = -ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Cos()*speed*0.8999999762f;
    }

    SpriteChangeCoorCollectInteractions(movingSprite);
    int index = interactionSprites.Get().m_count-1;
    while (index >= 0)
    {
        SPRITE* candidate = SpriteChangeCoorInteractionAt(static_cast<unsigned int>(index));
        if (!candidate) break;
        VID* candidateVid = candidate->Vid();
        bool runScript = false;

        const float nodeStrength = SpriteEffectiveCollisionInfo(movingSprite)->Power;
        const float candidateMass = SpriteEffectiveCollisionInfo(candidate)->Weight;
        if (SpriteFloatLessOrUnordered(nodeStrength, candidateMass))
        {
            runScript = true;
        }
        else if ((nodeFlags&0x00000200u) != 0u)
        {
            VECTOR pushed;
            pushed.x = candidate->X()+delta.x;
            pushed.y = candidate->Y()+delta.y;
            pushed.z = candidate->Z()+delta.z;
            if (candidate->CanPlaceWithCrushAndGlide(&pushed) == 0)
            {
                candidate->ChangeCoor(pushed);
                candidateVid = candidate->Vid();
                if (candidateVid->noAnimCadr[2] == 0u && candidate->Animation() != 2)
                    (void) candidate->ChangeAnimation(2);
            }
            else
            {
                runScript = true;
            }
        }
        else if ((nodeFlags&0x00000100u) != 0u)
        {
            reinterpret_cast<SPRITE_LAYOUT_ACCESS*>(candidate)->m_impulseSpeedX += impulseX;
            reinterpret_cast<SPRITE_LAYOUT_ACCESS*>(candidate)->m_impulseSpeedY += impulseY;

            const float rootMass = SpriteEffectiveCollisionInfo(rootSprite)->Weight;
            const float reactionMass = SpriteEffectiveCollisionInfo(candidate)->Weight;
            const float speed = reinterpret_cast<SPRITE_LAYOUT_ACCESS*>(rootSprite)->m_speed;
            const float magnitude = SpriteFloatAbs(speed)+10.0f;
            const int reaction = TruncateFloatToInt32((magnitude*rootMass*100.0f)/reactionMass);
            (void) candidate->Action(ACT_DAMAGE, reaction, 0, 0);
            if (SpriteFloatLessOrUnordered(rootMass, reactionMass))
                reinterpret_cast<SPRITE_LAYOUT_ACCESS*>(candidate)->m_speed *= 0.5f;
        }

        candidateVid = candidate->Vid();
        if ((candidateVid->property&0x00004000u) != 0u)
            (void) candidate->Action(ACT_DAMAGE, 5, 0, 0);

        if (runScript)
        {
            const int scriptId = nodeVid->scriptFunction[19];
            if (scriptId >= 0)
                (void) Map->ScriptRun(scriptId, movingSprite, candidate, 0);
        }

        unsigned int count = static_cast<unsigned int>(interactionSprites.Get().m_count);
        if (index>static_cast<int>(count)) index = static_cast<int>(count);
        --index;
    }
}

void SPRITE::ChangeCoor(const VECTOR& target)
{
    VECTOR delta;
    delta.x = target.x-m_xyz.x;
    delta.y = target.y-m_xyz.y;
    delta.z = target.z-m_xyz.z;

    SPRITE* chainSprite = this;
    while (chainSprite)
    {
        VID* const vid = chainSprite->Vid();
        const bool collisionTracked = ((vid->property&0x28u) != 0u) &&
        ((chainSprite->RuntimeFlags()&0x08000000u) == 0u);
        if (collisionTracked)
            vid->ResetGridZ(this);

        VECTOR next;
        next.x = chainSprite->X()+delta.x;
        next.y = chainSprite->Y()+delta.y;
        next.z = chainSprite->Z()+delta.z;
        SPRITE_COLLECTOR* spriteCollector = ::Sprites;
        spriteCollector->ChangeCoor(chainSprite, next);

        EX_SPRITE_DATA* const previousExtraData = chainSprite->m_extraData;
        if (previousExtraData)
        {
            const unsigned int tick = ::CurrentTime;
            if (previousExtraData->cachedPositionTick != tick)
            {
                previousExtraData->cachedPositionTick = tick;
                previousExtraData->cachedPosition = chainSprite->m_xyz;
            }
        }

        chainSprite->m_xyz.x += delta.x;
        chainSprite->m_xyz.y += delta.y;
        chainSprite->m_xyz.z += delta.z;

        if (collisionTracked)
        {
            vid->SetGridZ(this);
            SpriteChangeCoorProcessInteractions(this, chainSprite, delta);
        }
        chainSprite = chainSprite->m_childChain;
    }
}

SPRITE* SPRITE::SeekEnemy() const
{
    SPRITE* linked = m_childChain;
    if (linked)
    {
        VID* const selfVid = m_vid;
        VID* const linkedVid = linked->m_vid;
        if (linkedVid == selfVid->linkVid &&
            linkedVid->nWeapon != 0 && linkedVid->childVid[8] != 0)
            return linked->SeekEnemy();
    }

    VID* const selfVid = m_vid;
    WEAPON_RUNTIME* const info = selfVid->weapon;
    const float searchRadius = info->DetectRange;
    const float minDistance = info->DeadZone;
    const float preferredDistance = info->BattleRange;
    const unsigned int targetMask = static_cast<unsigned int>(info->SpriteType);
    const unsigned int infoFlags = info->Property;

    if (SpriteFloatEqualOrUnordered(searchRadius, (0.0f)) || targetMask == 0u)
        return 0;

    if (Map->IsNetGameStart() != 0 && (infoFlags&0x08u) != 0u &&
        (selfVid->runtimeFlags&0x00001000u) == 0u)
    {
        STRING subject;
        STRING objectFormat;
        subject = "(synchro)P_RandomTarget, not propImportantForSync";
        objectFormat = "SPRITE [%i](%i,%i,%i)";
        MYERROR* const sink = ::Error;
        sink->Error(objectFormat, E_ERROR, subject, 0, selfVid->nVid, TruncateFloatToInt32(m_xyz.x), TruncateFloatToInt32(m_xyz.y), TruncateFloatToInt32(m_xyz.z));
    }

    float bestDistance = searchRadius+(1.0f);
    bool bestPreferred = false;
    SPRITE* best = 0;

    SPRITE_COLLECTOR* const collector = ::Sprites;
    const unsigned int selfLane = (m_runtimeFlags>>17u)&7u;
    PLAYER* const laneOwner = Map->m_players[selfLane];

    for (unsigned int lane = 0u; lane<8u; ++lane)
    {
        if (laneOwner->RelationTo(lane) != 1 && (infoFlags&0x80u) == 0u)
            continue;

        int cursor = 0;
        SPRITE* candidate = collector->FirstUnit(&cursor, static_cast<int>(lane));
        while (candidate)
        {
            VID* const candidateVid = candidate->m_vid;
            bool reject = false;
            if ((candidateVid->spriteType&targetMask) == 0u) reject = true;
            if (!reject && candidateVid->maxHp == 0) reject = true;
            if (!reject && (candidateVid->property&0x00020000u) != 0u) reject = true;
            if (!reject && (m_runtimeFlags&0x08000000u) != 0u) reject = true;
            if (!reject && (candidateVid->spriteType&0x10u) != 0u && candidate->m_childBacklink != 0) reject = true;

            if (!reject && (infoFlags&0x02u) != 0u)
            {
                const VECTOR2 delta(candidate->m_xyz.x-m_xyz.x,
                    candidate->m_xyz.y-m_xyz.y);
                const ANGLE direction(delta);
                if (SpriteCircularDirectionDifference(direction.value, m_direction.value) >= 32u)
                    reject = true;
            }

            float distance = 0.0f;
            bool preferred = false;
            if (!reject)
            {
                VECTOR2 delta;
                delta.x = candidate->m_xyz.x-m_xyz.x;
                delta.y = candidate->m_xyz.y-m_xyz.y;
                distance = delta.NearLength();
                if (SpriteFloatGreaterOrdered(distance, searchRadius) ||
                    SpriteFloatLessOrUnordered(distance, minDistance))
                    reject = true;
                else if (!SpriteFloatGreaterOrdered(distance, preferredDistance))
                {
                    if ((infoFlags&0x01u) != 0u)
                        preferred = true;
                    else
                    {
                        const VECTOR& targetPosition = candidate->m_xyz;
                        preferred = (CantMoveBulletTo(targetPosition) == 0);
                    }
                }
            }

            if (!reject && best)
            {
                if (preferred != bestPreferred)
                {
                    if (!preferred) reject = true;
                }
                else
                {
                    SPRITE* candidateEffective = candidate;
                    if (candidateVid->nWeapon == 0)
                    {
                        SPRITE* const cLinked = candidate->m_childChain;
                        if (cLinked && cLinked->m_vid->nWeapon != 0)
                            candidateEffective = cLinked;
                    }

                    SPRITE* bestEffective = best;
                    VID* const bestVid = best->m_vid;
                    if (bestVid->nWeapon == 0)
                    {
                        SPRITE* const bLinked = best->m_childChain;
                        if (bLinked && bLinked->m_vid->nWeapon != 0)
                            bestEffective = bLinked;
                    }

                    WEAPON_RUNTIME* const cInfo = candidateEffective->m_vid->weapon;
                    WEAPON_RUNTIME* const bInfo = bestEffective->m_vid->weapon;
                    const int candidatePriority = cInfo->EnemyRating;
                    const int bestPriority = bInfo->EnemyRating;
                    if (candidatePriority<bestPriority)
                        reject = true;
                    else if (candidatePriority == bestPriority)
                    {
                        bool takeForDirection = false;
                        if ((infoFlags&0x04u) != 0u)
                        {
                            const float dx = candidate->m_xyz.x-m_xyz.x;
                            const float dy = candidate->m_xyz.y-m_xyz.y;
                            const ANGLE direction = Decart2Polar(dx, dy);
                            takeForDirection = SpriteCircularDirectionDifference(
                                direction.value, m_direction.value)<32u;
                        }
                        if (!takeForDirection && !SpriteFloatLessOrUnordered(distance, bestDistance))
                            reject = true;
                    }
                }
            }

            if (!reject)
            {
                if ((infoFlags&0x08u) != 0u)
                {
                    int (*const randomFunction)(int) = (selfVid->runtimeFlags&0x1000u)
                        ?static_cast<int (*)(int)>(&::Random):static_cast<int (*)(int)>(&::Random2);
                    if (randomFunction(2) == 0)
                        return candidate;
                }
                best = candidate;
                bestDistance = distance;
                bestPreferred = preferred;
            }

            candidate = collector->NextUnit(&cursor);
        }
    }
    return best;
}

static float SpriteCallWorldHeight(const VID* vid, const VECTOR* position, unsigned char direction)
{
    return Map->GetGroundZ(
        vid,
        *reinterpret_cast<const VECTOR2*>(position),
        ANGLE(direction));
}
static float SpriteGlideMetric(float dx, float dy)
{
    dx = SpriteFloatAbs(dx);
    dy = SpriteFloatAbs(dy);

    return dx>dy ? dx+dy*0.3980000019f : dy+dx*0.3980000019f;
}
SPRITE* SPRITE::CanPlaceWithCrushAndGlide(VECTOR* position)
{
    VID* vid = m_vid;
    if (vid->moveMask == 0u) return 0;

    const VECTOR& current = m_xyz;
    const auto resetGlideVelocity = [this]()
    {
        m_zSpeed = 0.0f;
        m_impulseSpeedX = 0.0f;
        m_impulseSpeedY = 0.0f;
        m_impulseSpeedZ = 0.0f;
    };
    const auto dampGlideVelocity = [this]()
    {
        m_impulseSpeedX *= 0.5f;
        m_impulseSpeedY *= 0.5f;
        m_impulseSpeedZ *= 0.5f;
    };
    const unsigned char direction = m_direction.value;
    if (SpriteFloatEqualOrUnordered(position->x, current.x) &&
        SpriteFloatEqualOrUnordered(position->y, current.y) &&
        SpriteFloatEqualOrUnordered(position->z, current.z))
        return 0;

    SPRITE* collision = CanPlaceWithCrush(*position);
    vid = m_vid;

    if (collision != 0)
    {
        const int crushScript = vid->scriptFunction[19];
        if (crushScript >= 0)
        {
            SPRITE* scriptObject = (collision == Mouse)?0:collision;
            if (Map->ScriptRun(crushScript, this, scriptObject, 0) != 0u)
            {
                *position = current;
                return 0;
            }
        }

        vid = m_vid;
        const bool runMapWalk = (collision == Mouse &&
            Map->CanPlaceInGridZ(vid, current, ANGLE(direction)) != 0);
        if (runMapWalk)
        {
            CONSTANT* const gameState = ::Const;
            const unsigned int vidType = static_cast<unsigned int>(vid->spriteClass);
            if (gameState->DebugMode != 0 && (vidType == 2u || vidType == 7u))
            {
                GAMMA gamma;
                gamma.modulate = 0u;
                gamma.add = 0u;
                gamma.SetRed(-255);
                gamma.SetGreen(-255);
                gamma.SetBlue(255);
                SetGamma(gamma);
            }

            VECTOR step = {0.0f, 0.0f, 0.0f};
            SPRITE* linked = m_childChain;
            if (linked != 0)
            {
                const unsigned char linkedDirection = linked->m_direction.value;
                const unsigned int tableOffset = 4u*static_cast<unsigned int>(linkedDirection);
                step.x = ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Sin();
                step.y = -ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Cos();
            }
            else
            {
                step.x = (position->x-current.x)*32.0f;
                step.y = (position->y-current.y)*32.0f;
                step.z = 0.0f;
            }

            if (SpriteFloatEqualOrUnordered(step.x, 0.0f) &&
                SpriteFloatEqualOrUnordered(step.y, 0.0f) &&
                SpriteFloatEqualOrUnordered(step.z, 0.0f))
                return collision;
            {
                position->x = current.x+step.x;
                position->y = current.y+step.y;
                position->z = current.z+step.z;

                while (SpriteFloatLessOrUnordered(
                    SpriteGlideMetric(current.x-position->x, current.y-position->y), 32.0f))
                {
                    const unsigned int stillBlocked = static_cast<unsigned int>(Map->CanPlaceInGridZ(
                        vid, *position, ANGLE(direction)));
                    if (stillBlocked == 0u)
                        break;
                    position->x += step.x;
                    position->y += step.y;
                    position->z += step.z;
                }

                if (SpriteFloatLessOrUnordered(
                    SpriteGlideMetric(current.x-position->x, current.y-position->y), 32.0f))
                {
                    SPRITE* hashCollision = CanPlaceInHash(*position);
                    if (hashCollision == 0)
                        return 0;

                    if (hashCollision != Mouse)
                    {
                        const VECTOR2 delta(hashCollision->m_xyz.x-current.x,
                            hashCollision->m_xyz.y-current.y);
                        const ANGLE impulseDirection(delta);
                        const unsigned int tableOffset = 4u*static_cast<unsigned int>(impulseDirection.value);
                        hashCollision->m_impulseSpeedX =
                            ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Sin()*20.0f;
                        hashCollision->m_impulseSpeedY =
                            -ANGLE(static_cast<unsigned char>((tableOffset)/4u)).Cos()*20.0f;
                    }
                    *position = current;
                    resetGlideVelocity();
                    return hashCollision;
                }

                return collision;
            }
        }

        if (!SpriteFloatEqualOrUnordered(position->y, current.y))
        {
            VECTOR probe = {current.x, position->y, position->z};
            if (CanPlace(probe) == 0)
            {
                position->x = current.x;
                dampGlideVelocity();
                collision = 0;
            }
        }
        if (collision != 0 && !SpriteFloatEqualOrUnordered(position->x, current.x))
        {
            VECTOR probe = {position->x, current.y, position->z};
            if (CanPlace(probe) == 0)
            {
                position->y = current.y;
                dampGlideVelocity();
                collision = 0;
            }
        }
        if (collision != 0 && collision != Mouse &&
            !SpriteFloatEqualOrUnordered(position->z, current.z))
        {
            VECTOR probe = {position->x, position->y, current.z};
            if (CanPlace(probe) == 0)
            {
                position->z = current.z;
                m_zSpeed = 0.0f;
                dampGlideVelocity();
                collision = 0;
            }
        }

        if (collision != 0 && collision != Mouse)
        {
            const unsigned int collisionFlags = collision->Behave();
            const unsigned int selfFlags = m_behave;
            vid = m_vid;
            VID* const collisionVid = collision->Vid();
            const unsigned int commonMask = collisionVid->moveMask&vid->moveMask;
            const bool compatible = ((collisionFlags^selfFlags)&0x20u) == 0u && commonMask != 0u;
            const bool overlap = compatible &&
            collision->IsCross(vid, current, ANGLE(direction)) != 0;
            const bool mapClear = overlap &&
            Map->CanPlaceInGridZ(vid, *position, ANGLE(direction)) == 0;
            if (mapClear)
            {
                const bool highAction = (collision->Animation() >= 15 || m_currentAnimation >= 15);
                CONSTANT* gameState = ::Const;
                const bool effectsEnabled = gameState->DebugMode != 0;
                if (highAction || !effectsEnabled)
                {
                    dampGlideVelocity();
                    collision = 0;
                }
                else
                {
                    GAMMA gamma;
                    gamma.modulate = 0u;
                    gamma.add = 0u;
                    gamma.SetRed(255);
                    gamma.SetGreen(-255);
                    gamma.SetBlue(-255);
                    collision->SetGamma(gamma);

                    gamma.modulate = 0u;
                    gamma.add = 0u;
                    gamma.SetRed(255);
                    gamma.SetGreen(-255);
                    gamma.SetBlue(-255);
                    SetGamma(gamma);
                    return collision;
                }
            }
        }

        if (collision != 0)
        {
            *position = current;
            resetGlideVelocity();
            return collision;
        }
    }

    vid = m_vid;
    if ((vid->property&0x00000200u) != 0u)
    {
        const float ground = SpriteCallWorldHeight(vid, position, m_direction.value);
        vid = m_vid;
        if (ground>position->z || (vid->property&0x00000006u) == 0u)
        {
            position->z = ground;
            m_zSpeed = 0.0f;

            SPRITE* hashCollision = CanPlaceInHash(*position);
            if (hashCollision != 0)
            {
                WEAPON_RUNTIME* info = m_vid->weapon;
                if ((info->Property&0x00000100u) != 0u && hashCollision->m_currentAnimation >= 15)
                    return 0;
                *position = current;
                return hashCollision;
            }
        }
    }
    return 0;
}

namespace {
static int SpriteTruncFloatToInt(float value)
{
    if (value != value || value<-2147483648.0f || value >= 2147483648.0f)
        return static_cast<int>(0x80000000u);
    return static_cast<int>(value);
}

static int SpriteRandomInclusiveForVid(const VID* vid, int maximum)
{
    if (maximum<0) return 0;

    return (vid && (vid->runtimeFlags&0x1000u) != 0u)
        ? Random(maximum)
        : Random2(maximum);
}

static unsigned int SpriteAnimationCadence(const SPRITE* sprite, const VID* childVid)
{
    const unsigned int direction = static_cast<unsigned int>(sprite->Direction().value);
    const float speed = reinterpret_cast<const SPRITE_LAYOUT_ACCESS*>(sprite)->m_speed;
    const float axisX = ANGLE(static_cast<unsigned char>(direction)).Sin()*speed;
    const float axisY = -ANGLE(static_cast<unsigned char>(direction)).Cos()*speed -
    (reinterpret_cast<const SPRITE_LAYOUT_ACCESS*>(sprite)->m_zSpeed-childVid->maxZSpeed);

    unsigned int cadence = 30000u;
    if (!CreateChildFloatEqualOrUnordered(axisX, 0.0f) &&
        !CreateChildFloatEqualOrUnordered(axisY, 0.0f))
    {
        const float tx = SpriteFloatAbs(childVid->sizeXYZ.x/axisX);
        const float ty = SpriteFloatAbs(childVid->sizeXYZ.y/axisY);
        cadence = static_cast<unsigned int>(SpriteTruncFloatToInt(tx>ty?tx:ty));
    }
    return cadence;
}

static bool SpriteSfxRepeatEveryFrame(int sfxId)
{
    const SOUND* const sound = ::Sound;
    SFX* const records = sound->m_sfx;
    if (!records || sfxId<0 || sfxId>sound->m_sfxCount) return false;
    const SFX& record = records[static_cast<unsigned int>(sfxId)];
    return record.m_bufferCount != 0u && (record.Property & P_LOOP) != 0u;
}

static void SpriteUpdateEffectCurve(SPRITE* sprite, unsigned int now)
{
    VID* const vid = sprite->Vid();
    if ((vid->runtimeFlags&0x1Fu) == 0u) return;

    EX_SPRITE_DATA* const extraData = sprite->ExData();
    const WEAPON_RUNTIME* const weapon = vid->weapon;
    const unsigned int elapsed = now-extraData->effectTimestamp;
    const unsigned int refresh = weapon->Period;

    bool haveCurveDuration = false;
    unsigned int duration = refresh;
    unsigned int curveElapsed = elapsed;

    if (refresh != 999999u)
    {
        haveCurveDuration = duration != 0u;
    }
    else
    {
        const int animation = sprite->Animation();
        const SPRITE_LAYOUT_ACCESS* const layout = reinterpret_cast<const SPRITE_LAYOUT_ACCESS*>(sprite);
        const int frameBegin = layout->m_currentFrameBegin;
        const int frameEnd = sprite->CurrentFrameEnd();
        if (animation >= 15 && frameEnd != frameBegin)
        {
            const unsigned int frameSpeed = static_cast<unsigned int>(vid->frameSpeed[animation]);
            duration = (static_cast<unsigned int>(frameEnd)-static_cast<unsigned int>(frameBegin)+1u)*frameSpeed;
            haveCurveDuration = duration != 0u;
            const unsigned int sinceFrameTick = now-layout->m_lastAnimationTick;
            const unsigned int frameOffset = static_cast<unsigned int>(sprite->CurrentFrame())-
            static_cast<unsigned int>(frameBegin);
            if (sinceFrameTick>frameSpeed)
                curveElapsed = (frameOffset+1u)*frameSpeed;
            else
                curveElapsed = frameOffset*frameSpeed-layout->m_lastAnimationTick+now;
        }
        else if (extraData->lifetimeRemaining != 999999u)
        {
            duration = extraData->lifetimeRemaining+curveElapsed;
            haveCurveDuration = duration != 0u;
        }
    }

    if (haveCurveDuration)
    {
        int segment = SpriteTruncFloatToInt(extraData->effectCurvePosition)+1;
        const int count = weapon->maxColumn;
        const float elapsedF = static_cast<float>(curveElapsed);
        const float durationF = static_cast<float>(duration);
        while (segment<count &&
            !SpriteFloatGreaterOrdered(weapon->Time[segment]*durationF, elapsedF))
            ++segment;

        const int previous = segment-1;
        const float segmentStartTimeFactor = weapon->Time[previous];
        const float segmentEndTimeFactor = weapon->Time[previous+1];
        extraData->effectCurvePosition = ((elapsedF-segmentStartTimeFactor*durationF)/((segmentEndTimeFactor-segmentStartTimeFactor)*durationF))+
        static_cast<float>(previous);
    }

    if (refresh != 999999u && elapsed>refresh) extraData->effectTimestamp = now;

    if ((vid->runtimeFlags&0x10u) != 0u)
    {
        const float curve = extraData->effectCurvePosition;
        const int index = SpriteTruncFloatToInt(curve);
        const int count = weapon->maxColumn;
        if (index >= count) extraData->maxSpeedOverride = weapon->Speed[count];
        else
        {
            const float left = weapon->Speed[index];
            const float right = weapon->Speed[index+1];
            extraData->maxSpeedOverride = left+(right-left)*(curve-static_cast<float>(index));
        }
        extraData->maxSpeedOverride += vid->maxSpeed;
    }
}
}

void SPRITE::Tact()
{
    unsigned int previousFrameClock = ::PrevCurrentTime;
    unsigned int now = ::CurrentTime;
    const int entryAnimation = m_currentAnimation;

    SpriteUpdateEffectCurve(this, now);

    if (!m_childBacklink)
    {
        const unsigned int savedTick = m_lastAnimationTick;
        m_lastAnimationTick = previousFrameClock;
        MoveTact();
        m_lastAnimationTick = savedTick;
    }

    previousFrameClock = ::PrevCurrentTime;
    now = ::CurrentTime;
    VID* vid = m_vid;
    VID* childVid = vid->childVid[m_currentAnimation];
    if (childVid)
    {
        if (vid->spriteClass == 23u &&
            CreateChildFloatEqualOrUnordered(vid->childX[m_currentAnimation], 0.0f) &&
            CreateChildFloatEqualOrUnordered(vid->childY[m_currentAnimation], 0.0f))
        {
            const REGION* const region = static_cast<const REGION*>(this);
            const bool applicationSized = (region->m_fogFlags&8u) != 0u;
            MAP* const map = Map;
            const float width = applicationSized?map->m_sizeXY.x:region->m_halfWidth;
            const float height = applicationSized?map->m_sizeXY.y:region->m_halfHeight;
            const int tileCount = SpriteTruncFloatToInt(
                width*height/childVid->sizeXYZ.x/childVid->sizeXYZ.y);
            if (tileCount != 0 && now != previousFrameClock)
            {
                const unsigned int divisor = 1000u/(now-previousFrameClock)/
                static_cast<unsigned int>(tileCount);
                if (Random2(static_cast<int>(divisor)) == 0)
                    (void) CreateChild();
            }
        }
        else if ((childVid->property&0x80u) != 0u)
        {
            EX_SPRITE_DATA* const aux = m_extraData;
            if (now-now%aux->childCadence>previousFrameClock)
            {
                unsigned int cadence = SpriteAnimationCadence(this, childVid);
                if (m_currentAnimation == 8 &&
                    (vid->weapon->Property&0x10u) != 0u)
                    cadence >>= 1;
                if (cadence>30000u) cadence = 30000u;
                if (cadence == 0u) cadence = 1u;
                aux->childCadence = cadence;
                (void) CreateChild();
            }
        }
    }

    if ((m_runtimeFlags&0x7Cu) == 0x1Cu)
    {
        bool keepCommand = false;
        if (m_goalSprite)
        {
            now = ::CurrentTime;
            previousFrameClock = ::PrevCurrentTime;
            const ANGLE desired = m_goalSprite->Direction();
            const ANGLE rotated = RotateTact(desired, BitsToSigned32(now-previousFrameClock));
            keepCommand = rotated.value != 0;
        }
        if (!keepCommand) SetCommandWithoutLink(0, 0);
    }

    if (m_stateTimer != 0u)
    {
        now = ::CurrentTime;
        previousFrameClock = ::PrevCurrentTime;
        const unsigned int elapsed = now-previousFrameClock;
        if (elapsed >= m_stateTimer)
        {
            m_stateTimer = 0u;
            if ((m_runtimeFlags&0x7Cu) == 0x48u)
            {
                m_runtimeFlags &= ~0x10000000u;
                SetCommand(0, static_cast<SPRITE*>(0));
            }
        }
        else m_stateTimer -= elapsed;
    }

    vid = m_vid;
    if ((vid->runtimeFlags&0x100u) != 0u && m_currentAnimation<15)
        (void) AttackTact();
    vid = m_vid;

    {
        const int sfxId = vid->sfx[m_currentAnimation];
        if (sfxId != 0 && (m_runtimeFlags&0x8000u) == 0u && !SpriteSfxRepeatEveryFrame(sfxId))
        {
            m_runtimeFlags |= 0x8000u;
            PlaySFX(sfxId);
        }
    }

    const bool firstAttackFrameFastPath =
        (m_runtimeFlags&0x10000u) == 0u && m_currentAnimation == 8 &&
    m_currentFrame == m_currentFrameBegin;
    if (!firstAttackFrameFastPath)
    {
        now = ::CurrentTime;
        if (m_birthTime == now) return;
        vid = m_vid;
        const unsigned int frameInterval = static_cast<unsigned int>(vid->frameSpeed[m_currentAnimation]);
        if (static_cast<unsigned int>(now-m_lastAnimationTick)<frameInterval) return;

        if (entryAnimation != m_currentAnimation && (vid->type&0x40u) != 0u &&
            static_cast<VID_SURFACE*>(vid)->frames != 0)
            return;
    }

    for (; ;)
    {
        const int animation = m_currentAnimation;
        vid = m_vid;
        const int eventId = vid->scriptFunction[animation];
        if (eventId<0 || (m_currentFrame != m_currentFrameBegin && (vid->property&0x10u) == 0u)) break;
        const int result = Map->ScriptRun(eventId, this, static_cast<const SPRITE*>(0), 0);
        if (result != 0) return;
        if (animation == m_currentAnimation) break;
    }

    vid = m_vid;

    {
        const int sfxId = vid->sfx[m_currentAnimation];
        if (sfxId != 0 && SpriteSfxRepeatEveryFrame(sfxId)) PlaySFX(sfxId);
    }

    vid = m_vid;
    childVid = vid->childVid[m_currentAnimation];
    SPRITE* createdChild = 0;
    if (childVid && (childVid->property&0x80u) == 0u &&
        (m_currentAnimation != 8 || (m_runtimeFlags&0x10000u) == 0u))
    {
        bool create = false;
        if (m_currentAnimation == 8 && (m_runtimeFlags&0x10000u) == 0u &&
            (vid->weapon->Property&0x10u) != 0u)
        {
            const int midpoint = BitsToSigned32(static_cast<unsigned int>(m_currentFrameEnd)+
                static_cast<unsigned int>(m_currentFrameBegin)+1u)/2;
            if (m_currentFrame == midpoint) create = true;
            else
            {
                const int trigger = (vid->property&0x40000u)?m_currentFrameEnd:m_currentFrameBegin;
                create = m_currentFrame == trigger;
            }
        }
        else if ((vid->property&0x10u) != 0u) create = true;
        else
        {
            const int trigger = (vid->property&0x40000u)?m_currentFrameEnd:m_currentFrameBegin;
            create = m_currentFrame == trigger;
        }
        if (create) createdChild = CreateChild();
    }

    vid = m_vid;
    unsigned int detectCadence;
    if (m_childChain && m_childChain->m_vid == vid->linkVid &&
        m_childChain->m_vid->nWeapon != 0 && m_childChain->m_vid->childVid[8] != 0)
        detectCadence = m_childChain->m_vid->weapon->DetectPeriod;
    else
        detectCadence = vid->weapon->DetectPeriod;

    if (detectCadence != 999999u)
    {
        bool runDetect = true;
        if (detectCadence != 0u)
        {
            now = ::CurrentTime;
            const unsigned int alignedNow = now-now%detectCadence;
            if (alignedNow>m_lastAnimationTick) runDetect = false;
        }
        if (runDetect && m_currentFrame != m_currentFrameEnd &&
            (vid->weapon->Property&0x400u) != 0u)
            runDetect = false;
        if (runDetect) DetectTact();
    }

    m_currentFrame = BitsToSigned32(static_cast<unsigned int>(m_currentFrame)+1u);
    vid = m_vid;

    if (m_currentAnimation >= 15)
    {
        const bool deathFrameComplete =
            m_currentFrame>m_currentFrameEnd || vid->noAnimCadr[m_currentAnimation] == 0;
        if (deathFrameComplete)
        {
            m_currentFrame = m_currentFrameEnd;

            const int* payload = 0;
            bool usePayload = false;
            if (m_extraData && m_extraData->items.m_count == 5)
            {
                payload = m_extraData->items.m_data;
                usePayload = payload[0] == 123456;
            }

            if (vid->deathDamageMax>0 || usePayload)
            {
                if (Map->IsNetGameStart() != 0 && (vid->runtimeFlags&0x1000u) == 0u)
                {
                    STRING subject;
                    STRING objectFormat;
                    subject = "(synchro)no damage, not propImportantForSync";
                    objectFormat = "SPRITE [%i](%i,%i,%i)";
                    MYERROR* sink = ::Error;
                    sink->Error(objectFormat, E_ERROR, subject, 0, vid?vid->nVid:-1, TruncateFloatToInt32(m_xyz.x), TruncateFloatToInt32(m_xyz.y), TruncateFloatToInt32(m_xyz.z));
                }

                vid = m_vid;
                payload = 0;
                usePayload = false;
                if (m_extraData && m_extraData->items.m_count == 5)
                {
                    payload = m_extraData->items.m_data;
                    usePayload = payload[0] == 123456;
                }
                float deathRange;
                int damageMin;
                int damageMax;
                float impulseScale;
                if (usePayload)
                {
                    deathRange = static_cast<float>(payload[1]);
                    damageMin = payload[2];
                    damageMax = payload[3];

                    impulseScale = static_cast<float>(payload[4]/1024);
                }
                else
                {
                    deathRange = vid->deathRange;
                    damageMin = vid->deathDamageMin;
                    damageMax = vid->deathDamageMax;
                    impulseScale = vid->deathPush;
                }

                int damage = damageMin;
                if (usePayload || damageMax != damageMin)
                {
                    const int span = BitsToSigned32(static_cast<unsigned int>(damageMax)-
                        static_cast<unsigned int>(damageMin));
                    const int randomDamage = (vid->runtimeFlags&0x1000u) != 0u
                        ? ::Random(span) : ::Random2(span);
                    damage = BitsToSigned32(static_cast<unsigned int>(damage)+
                        static_cast<unsigned int>(randomDamage));
                }

                vid = m_vid;
                const float sourceVertical = SpriteFloatGreaterOrdered(20.0f, vid->sizeXYZ.z)
                    ?20.0f:vid->sizeXYZ.z;
                const float extentX = deathRange+vid->halfSizeXY.x;
                const float extentY = deathRange+vid->halfSizeXY.y;
                SPRITE_COLLECTOR* const collector =
                    ::Sprites;

                for (SPRITE* candidate = collector->FirstHashInBox(
                    m_xyz.x-extentX, m_xyz.y-extentY, m_xyz.x+extentX, m_xyz.y+extentY);
                    candidate; candidate = ::Sprites->NextHashInBox())
                {
                    if (candidate == this || candidate->m_currentAnimation >= 15 || candidate == createdChild)
                        continue;

                    VID* const candidateVid = candidate->m_vid;
                    float candidateWeaponScalar = 0.0f;
                    if (candidateVid->maxHp == 0)
                    {
                        if (!SpriteX87OrderedNonZero(impulseScale, 0.0f)) continue;
                        if (candidateVid->nWeapon == 0 && candidate->m_childChain)
                            candidateWeaponScalar = candidate->m_childChain->m_vid->weapon->Weight;
                        else
                            candidateWeaponScalar = candidateVid->weapon->Weight;
                        if (!SpriteX87OrderedNonZero(candidateWeaponScalar, 999999.0f)) continue;
                    }

                    if ((m_vid->property&0x80000000u) != 0u)
                    {
                        const unsigned int sourceArmy = (m_runtimeFlags>>17)&7u;
                        const unsigned int candidateArmy = (candidate->m_runtimeFlags>>17)&7u;
                        PLAYER* const player = Map->m_players[sourceArmy];
                        if (player->RelationTo(candidateArmy) == 2) continue;
                    }

                    const float thresholdX = deathRange+vid->halfSizeXY.x+candidateVid->halfSizeXY.x;
                    const float thresholdY = deathRange+vid->halfSizeXY.y+candidateVid->halfSizeXY.y;
                    const float thresholdZ = sourceVertical+candidateVid->sizeXYZ.z;
                    if (!SpriteFloatLessOrUnordered(SpriteFloatAbs(m_xyz.x-candidate->m_xyz.x), thresholdX) ||
                        !SpriteFloatLessOrUnordered(SpriteFloatAbs(m_xyz.y-candidate->m_xyz.y), thresholdY) ||
                        !SpriteFloatLessOrUnordered(SpriteFloatAbs(m_xyz.z-candidate->m_xyz.z), thresholdZ))
                        continue;

                    const float candidateTop = candidate->m_xyz.z+candidateVid->sizeXYZ.z;
                    const VECTOR2 midpoint((m_xyz.x+candidate->m_xyz.x)*0.5f,
                        (m_xyz.y+candidate->m_xyz.y)*0.5f);
                    if (SpriteFloatLessOrUnordered(candidateTop, Map->GetGroundZ(midpoint))) continue;
                    const VECTOR2 nearSource((m_xyz.x*3.0f+candidate->m_xyz.x)*0.25f,
                        (m_xyz.y*3.0f+candidate->m_xyz.y)*0.25f);
                    if (SpriteFloatLessOrUnordered(candidateTop, Map->GetGroundZ(nearSource))) continue;
                    const VECTOR2 nearCandidate((candidate->m_xyz.x*3.0f+m_xyz.x)*0.25f,
                        (candidate->m_xyz.y*3.0f+m_xyz.y)*0.25f);
                    if (SpriteFloatLessOrUnordered(candidateTop, Map->GetGroundZ(nearCandidate))) continue;

                    if (candidateVid->maxHp != 0)
                    {
                        if ((m_vid->property&0x04000000u) != 0u &&
                            SpriteX87OrderedNonZero(deathRange, 0.0f))
                        {
                            const VECTOR2 delta(candidate->m_xyz.x-m_xyz.x, candidate->m_xyz.y-m_xyz.y);
                            const float distance = delta.NearLength();
                            if (SpriteFloatLessOrUnordered(distance, deathRange))
                            {
                                const int scaledDamage = SpriteTruncFloatToInt(
                                    static_cast<float>(damage)*(1.0f-(distance/deathRange)*0.5f));
                                (void) candidate->Action(ACT_DAMAGE, scaledDamage,
                                    static_cast<int>(PtrToU32(this)), 0);
                            }
                        }
                        else
                        {
                            (void) candidate->Action(ACT_DAMAGE, damage,
                                static_cast<int>(PtrToU32(this)), 0);
                        }
                    }

                    if (!SpriteX87OrderedNonZero(impulseScale, 0.0f)) continue;
                    {
                        VID* const impulseVid = candidate->m_vid;
                        if (impulseVid->nWeapon == 0 && candidate->m_childChain)
                            candidateWeaponScalar = candidate->m_childChain->m_vid->weapon->Weight;
                        else
                            candidateWeaponScalar = impulseVid->weapon->Weight;
                    }

                    ANGLE impulseDirection = m_direction;
                    if ((m_vid->property&0x04000000u) != 0u)
                        impulseDirection = Decart2Polar(candidate->m_xyz.x-m_xyz.x, candidate->m_xyz.y-m_xyz.y);

                    const unsigned int directionIndex = static_cast<unsigned int>(impulseDirection.value)*4u;
                    const float impulseX = ANGLE(static_cast<unsigned char>((directionIndex)/4u)).Sin()*impulseScale;
                    const float impulseY = -ANGLE(static_cast<unsigned char>((directionIndex)/4u)).Cos()*impulseScale;
                    const float massFactor = 1.0f/(candidateWeaponScalar+15.0f);
                    candidate->m_impulseSpeedX += impulseX*massFactor;
                    candidate->m_impulseSpeedY += impulseY*massFactor;
                }
            }

            vid = m_vid;

            if (vid->spriteClass == 8u)
            {
                if (vid->noAnimCadr[0] != 0 || vid->noAnimCadr[2] != 0 || vid->noAnimCadr[15] == 0)
                {
                    ::Sprites
                    ->DeleteFromActive(this);
                    return;
                }
            }
            this->~SPRITE();
            CrtFreePointer(this);
            return;
        }
    }

    if (SpriteFloatLessOrUnordered(m_xyz.z, -10000.0f))
    {
        {
            STRING subject;
            STRING objectFormat;
            subject = "z coor";
            objectFormat = "SPRITE [%i](%i,%i,%i)";
            MYERROR* sink = ::Error;
            sink->Error(objectFormat, E_INVALID, subject, TruncateFloatToInt32(m_xyz.z), vid?vid->nVid:-1, TruncateFloatToInt32(m_xyz.x), TruncateFloatToInt32(m_xyz.y), TruncateFloatToInt32(m_xyz.z));
        }
        this->~SPRITE();
        CrtFreePointer(this);
        return;
    }

    bool finalizeCommand = false;
    bool leaveToTail = false;
    const unsigned int command = m_runtimeFlags&0x7Cu;
    if (command == 0x44u || command == 0x48u)
    {
        if (m_currentFrame>m_currentFrameEnd) finalizeCommand = true;
        else leaveToTail = true;
    }
    else if (m_currentFrame <= m_currentFrameEnd && (vid->property&0x10u) == 0u)
    {
        leaveToTail = true;
    }
    else
    {
        EX_SPRITE_DATA* extraData = m_extraData;
        if (!extraData || (command != 0u && command != 0x28u && command != 0x20u))
        {
            (void) Action(ACT_NEXT_COMMAND, 0, 0, 0);
            leaveToTail = true;
        }
        else
        {
            bool poppedAny = false;
            bool stopInterpreter = false;
            for (int laneSwitch = 0; laneSwitch<4 && !stopInterpreter; ++laneSwitch)
            {
                extraData = m_extraData;
                extraData->actionStackIndex = (extraData->actionStackIndex+1u)&3u;
                LIST<ACT>* lane =
                    (&extraData->actionLane[extraData->actionStackIndex]);

                while (lane->m_count>0)
                {
                    const ACT* const data = lane->m_data;
                    if (data[lane->m_count-1].command == ACT_STOP_STACK) break;

                    const ACT action = data[lane->m_count-1];
                    --lane->m_count;
                    poppedAny = true;

                    bool conditional = false;
                    bool conditionFulfilled = false;
                    if (action.command == ACT_FLAGMAN_TRIGGER)
                    {
                        conditional = true;
                        if (Map->Flagman(Map->m_playerIndex))
                        {
                            SPRITE* const flagman = Map->Flagman(Map->m_playerIndex);
                            const VECTOR2 delta(static_cast<float>(action.arg1)-flagman->m_xyz.x,
                                static_cast<float>(action.arg2)-flagman->m_xyz.y);
                            conditionFulfilled = SpriteFloatLessOrUnordered(
                                delta.NearLength(), static_cast<float>(action.arg3));
                        }
                    }
                    else if (action.command == ACT_WHILE_NOT_QUEST)
                    {
                        conditional = true;
                        PLAYER* const player = Map->m_players[0];
                        const int result = player->GetQuestState(action.arg1, action.arg2);
                        if (result == -1)
                        {
                            (void) ::Error->Log("SPRITE::Invalid quest number %i_%i! sprite[%i] coor: %i %i", static_cast<unsigned int>(action.arg1), static_cast<unsigned int>(action.arg2), static_cast<unsigned int>(vid?vid->nVid:-1), static_cast<unsigned int>(TruncateFloatToInt32(m_xyz.x)), static_cast<unsigned int>(TruncateFloatToInt32(m_xyz.y)));
                        }
                        conditionFulfilled = result>0;
                    }
                    else if (action.command == ACT_WHILE_NOT_SCRIPT_VAR)
                    {
                        conditional = true;
                        SCRIPT* const script = &Map->m_script;
                        conditionFulfilled =
                            script->GetVariableActionInt(static_cast<unsigned int>(action.arg1)) == action.arg2;
                    }

                    if (conditional)
                    {
                        extraData = m_extraData;
                        lane = (&extraData->actionLane[extraData->actionStackIndex]);
                        if (conditionFulfilled) continue;

                        ++lane->m_count;
                        if (lane->m_count>lane->m_capacity)
                            (lane)->Expand(lane->m_count);
                        finalizeCommand = true;
                        stopInterpreter = true;
                        break;
                    }

                    if (action.command >= 17)
                    {
                        if (m_currentFrame>m_currentFrameEnd)
                        {
                            const int animation = m_currentAnimation;
                            if ((animation >= 7 && animation<15 && animation != 10) ||
                                (animation == 2 && SpriteFloatEqualOrUnordered(m_speed, 0.0f)))
                                ChangeAnimation(0);
                        }
                        if (action.command == ACT_DESTROY_UNIT && action.arg1 == -1)
                        {
                            this->~SPRITE();
                            CrtFreePointer(this);
                            return;
                        }
                        (void) Action(action.command,
                            action.arg1, action.arg2, action.arg3);
                        leaveToTail = true;
                        stopInterpreter = true;
                        break;
                    }

                    SPRITE* animationTarget = this;
                    if (action.arg1>0 && action.arg1<Map->m_vidCount &&
                        Map->m_vids[action.arg1] != 0)
                    {
                        SPRITE_COLLECTOR* const collector =
                            ::Sprites;
                        if (action.arg2 == 0 && action.arg3 == 0)
                        {
                            const VECTOR2 origin(0.0f, 0.0f);
                            animationTarget = collector->FindNearestSprite(
                                action.arg1, origin, 20000.0f, 0);
                        }
                        else
                        {
                            const VECTOR2 screenPoint(static_cast<float>(action.arg2),
                                static_cast<float>(action.arg3));
                            animationTarget = collector->GetSpriteScr(action.arg1, screenPoint, 0);
                        }
                    }
                    if (animationTarget) animationTarget->ChangeAnimation(action.command);
                    leaveToTail = true;
                    stopInterpreter = true;
                    break;
                }
            }

            if (!stopInterpreter)
            {
                if (poppedAny) leaveToTail = true;
                else
                {
                    (void) Action(ACT_NEXT_COMMAND, 0, 0, 0);
                    leaveToTail = true;
                }
            }
        }
    }

    if (finalizeCommand && !leaveToTail)
    {
        const unsigned int currentCommand = m_runtimeFlags&0x7Cu;
        if (currentCommand != 0x28u && currentCommand != 0x20u)
            SetMoveAnimation();
        else
            (void) Action(ACT_NEXT_COMMAND, 0, 0, 0);

        if ((m_runtimeFlags&0x7Cu) == 0x48u && (m_runtimeFlags&0x10000000u) != 0u)
        {
            if (Action(ACT_GET_DIALOG, 0, 0, 0) == 0)
            {
                if (!Map->Flagman(0) || Map->Flagman(0)->Action(ACT_GET_DIALOG, 0, 0, 0) == 0)
                {
                    m_runtimeFlags &= ~0x10000000u;
                    SetCommand(0, static_cast<SPRITE*>(0));
                }
            }
        }
    }

    if (m_extraData && m_extraData->lifetimeRemaining != 999999u && m_currentAnimation<15)
    {
        now = ::CurrentTime;
        const unsigned int elapsed = now-m_lastAnimationTick;
        if (elapsed >= m_extraData->lifetimeRemaining) ChangeAnimation(15);
        else m_extraData->lifetimeRemaining -= elapsed;
    }

    m_lastAnimationTick = ::CurrentTime;
    if (m_currentFrame>m_currentFrameEnd) m_currentFrame = m_currentFrameBegin;

    vid = m_vid;
    if (m_childChain && vid->cadrLinkXYZ)
    {
        const VECTOR position = m_xyz+vid->cadrLinkXYZ[m_currentFrame];
        m_childChain->ChangeCoor(position);
    }

    vid = m_vid;
    if ((vid->property&0x28u) != 0u && (m_runtimeFlags&0x08000000u) == 0u && m_extraData)
    {
        if (m_extraData->reserved38 != static_cast<unsigned int>(m_currentFrame) ||
            static_cast<unsigned int>(::CurrentTime-m_birthTime)<2000u)
        {
            vid->ResetGridZ(this);
            m_vid->SetGridZ(this);
        }
    }
}

namespace
{

static void SpriteActionFreeIntList(LIST<int>& list)
{
    if (list.m_data) CrtFreePointer(list.m_data);
    list.m_data = 0;
    list.m_count = 0;
    list.m_capacity = 0;
}

static void SpriteActionCopyIntList(LIST<int>& destination, const LIST<int>& source)
{
    if (&destination == &source) return;
    void* const oldData = destination.m_data;
    destination.m_capacity = 0;
    destination.m_count = 0;
    if (oldData) CrtFreePointer(oldData);
    destination.m_data = 0;
    destination.m_count = source.m_count;
    destination.m_capacity = source.m_capacity;

    destination.m_data = static_cast<int*>(CrtNew(static_cast<unsigned int>(destination.m_capacity)*4u));
    if (!destination.m_data)
    {
        const int failedCapacity = destination.m_capacity;
        const STRING format("!!!ERROR!!!::LIST: Not enough memory for = %i");
        ::Error->LogExit(format.m_value, failedCapacity);
    }

    for (int itemIndex = 0; itemIndex<destination.m_count; ++itemIndex)
        destination.m_data[itemIndex] = source.m_data[itemIndex];
}

static void SpriteActionEnsureTemporaryIntList(LIST<int>& list)
{
    if (list.m_count <= list.m_capacity) return;
    const int requested = list.m_count;
    void* oldData = list.m_data;
    list.m_data = static_cast<int*>(CrtNew(static_cast<unsigned int>(requested)*4u));
    if (!list.m_data)
    {
        const STRING format("!!!ERROR!!!::LIST: Not enough memory %i");
        ::Error->LogExit(format.m_value, requested);
    }
    if (oldData)
    {
        const int* const src = static_cast<const int*>(oldData);
        for (int itemIndex = 0; itemIndex<list.m_capacity; ++itemIndex) list.m_data[itemIndex] = src[itemIndex];
    }
    if (oldData) (void)(CrtFreePointer(oldData), 0u);
    list.m_capacity = requested;
}

static bool SpriteActionRecordHasSpritePointer(unsigned int opcode)
{
    return opcode == ACT_ATTACK || opcode == ACT_MOVE_TO || opcode == ACT_FOLLOW ||
    opcode == ACT_COPY_STACK_TO || opcode == ACT_SET_LINK || opcode == ACT_SET_UPLINK ||
    opcode == ACT_LINK_ENGINE || opcode == ACT_CLASH_ENGINE || opcode == ACT_FORCELINK_ENGINE;
}

static float SpriteBuildCoordinate(const SPRITE* sprite, float ownerCoordinate, int rawCoordinate)
{
    if (rawCoordinate >= 0) return static_cast<float>(rawCoordinate);
    const int radius = BitsToSigned32(0u-static_cast<unsigned int>(rawCoordinate));
    const int randomPart = SpriteRandomInclusiveForVid(sprite->Vid(), radius);
    const int doubledRandom = BitsToSigned32(static_cast<unsigned int>(randomPart)*2u);
    return static_cast<float>(static_cast<double>(ownerCoordinate)-
        static_cast<double>(static_cast<float>(rawCoordinate))-static_cast<double>(doubledRandom));
}

static void SpriteCopyBuildVisualState(const SPRITE* owner, SPRITE* created)
{
    if ((owner->Vid()->property&0x00800000u) == 0u) return;
    const EX_SPRITE_DATA* extraData = owner->ExData();
    const VECTOR scale = extraData?extraData->scale:VECTOR(1.0f, 1.0f, 1.0f);
    created->SetScale(scale);

    extraData = owner->ExData();
    GAMMA gamma;
    gamma.modulate = extraData?extraData->gammaModulate:0u;
    gamma.add = extraData?extraData->gammaAdd:0u;
    created->SetGamma(gamma);
    extraData = owner->ExData();
    if (extraData && extraData->numberInName >= 0) created->SetNumberInName(extraData->numberInName);
}

}

int SPRITE::Action(int opcode, int argument1, int argument2, int argument3)
{
    switch (static_cast<unsigned int>(opcode)&0xFFu)
    {
    case 9:
            if (m_currentAnimation != 8) ChangeAnimation(9);
        return 0;

    case ACT_ATTACK:
            (void) Attack(reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1))));
        return 0;
    case ACT_MOVE:
            Move(SpriteCreateActionTarget(VECTOR(static_cast<float>(argument1), static_cast<float>(argument2), static_cast<float>(argument3))));
        return 0;
    case ACT_MOVE_TO:
            Move(reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1))));
        return 0;
    case ACT_STOP:
            Stop();
        if (argument1) m_speed = 0.0f;
        if (argument2) (void) SetCommand(0, static_cast<SPRITE*>(0));
        return 0;
    case ACT_RANDOM:
        {
            if (Map->IsNetGameStart() != 0 && (m_vid->runtimeFlags&0x00001000u) == 0u)
                SPRITE::Error(E_ERROR, STRING("(synchro)in ACT_RANDOM, not propImportantForSync"), static_cast<unsigned long>(0));

            if (SpriteRandomInclusiveForVid(m_vid, 4) == 0)
            {
                ChangeAnimation(0);
                return 0;
            }
            if (SpriteRandomInclusiveForVid(m_vid, 4) == 0)
            {
                ChangeAnimation(12);
                return 0;
            }

            const unsigned int elapsed = ::CurrentTime-m_lastAnimationTick;
            unsigned char rotated = 0;
            const ANGLE quarterTurn(64u);
            if (m_currentAnimation == 4 && SpriteRandomInclusiveForVid(m_vid, 2) != 0)
            {
                const ANGLE direction = m_direction-quarterTurn;
                rotated = RotateTact(direction, static_cast<int>(elapsed)).value;
                return 0;
            }
            if (m_currentAnimation == 5 && SpriteRandomInclusiveForVid(m_vid, 2) != 0)
            {
                const ANGLE direction = m_direction+quarterTurn;
                rotated = RotateTact(direction, static_cast<int>(elapsed)).value;
                return 0;
            }
            if (SpriteRandomInclusiveForVid(m_vid, 3) != 0) return 0;
            const ANGLE direction = SpriteRandomInclusiveForVid(m_vid, 1) != 0
                ?(m_direction-quarterTurn):(m_direction+quarterTurn);
            rotated = RotateTact(direction, static_cast<int>(elapsed)).value;
            return 0;
        }
    case ACT_PAUSE:
        {
            if (SpriteX87OrderedNonZero(m_speed, (0.0f))) Stop();
            if (Map->IsNetGameStart() != 0 && (m_vid->runtimeFlags&0x00001000u) == 0u)
                SPRITE::Error(E_ERROR, STRING("(synchro)in ACT_PAUSE, not propImportantForSync"), static_cast<unsigned long>(0));
            const int delay = argument1+SpriteRandomInclusiveForVid(m_vid, argument2);
            m_runtimeFlags &= ~0x10000000u;
            if (delay != 0 || (m_runtimeFlags&0x7Cu) == 0x48u)
            {
                m_stateTimer = static_cast<unsigned int>(delay);
                (void) SetCommand(delay != 0?0x12:0, static_cast<SPRITE*>(0));
            }
            if (argument3 == 1) m_runtimeFlags |= 0x10000000u;
            return 0;
        }
    case ACT_ROTATE:
            Rotate(ANGLE(static_cast<unsigned char>(argument1)));
        return 0;
    case ACT_MOVE_ACCURATELY:
            Move(SpriteCreateActionTarget(VECTOR(static_cast<float>(argument1), static_cast<float>(argument2), static_cast<float>(argument3))));
        m_runtimeFlags |= 0x00002000u;
        return 0;
    case ACT_FOLLOW:
        {
            SPRITE* target = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1)));
            if (!target)
                target = Map->Flagman(0);
            (void) SetCommandWithoutLink(8, target);
            return 0;
        }
    case ACT_STAND_FIRM:
            (void) SetCommandWithoutLink(10, SpriteCreateActionTarget(
            VECTOR(static_cast<float>(argument1), static_cast<float>(argument2), static_cast<float>(argument3))));
        return 0;
    case ACT_BUILD_UNIT:
        {
            int createNvid = argument1;
            if (createNvid == 0)
                createNvid = Action(ACT_GET_ITEM_TYPE, 8, 0, 0);

            MAP* map = Map;
            VID* createVid = 0;
            if (createNvid>0 && createNvid<map->m_vidCount)
                createVid = map->m_vids[createNvid];
            if (!createVid)
            {
                SPRITE::Error(E_INVALID, STRING("var1 in ACT_BUILD_UNIT"), static_cast<unsigned long>(createNvid));
                return 0;
            }

            const bool useOwnerCoordinates = argument2 == 0 && argument3 == 0;
            const float createX = useOwnerCoordinates?m_xyz.x:SpriteBuildCoordinate(this, m_xyz.x, argument2);
            const float createY = useOwnerCoordinates?m_xyz.y:SpriteBuildCoordinate(this, m_xyz.y, argument3);

            if (Map->IsNetGameStart() != 0)
            {
                MAP* syncMap = Map;
                VID* syncVid = 0;
                if (createNvid >= 0 && createNvid<syncMap->m_vidCount)
                    syncVid = syncMap->m_vids[createNvid];
                if (!syncVid)
                    syncVid = ::EmptyVid;
                if (((syncVid->runtimeFlags^m_vid->runtimeFlags)&0x00001000u) != 0u)
                    SPRITE::Error(E_CREATE, STRING("(synchro)in ACT_BUILD_UNIT разные propImportantForSync"), static_cast<unsigned long>(createNvid));
            }

            SPRITE* created = Map->CreateSprite(createVid, VECTOR(createX, createY, m_xyz.z), m_direction, this);
            if (!created) return 0;

            if (ActionStackHaveCommand(0, ACT_STOP_STACK))
                (void) Action(ACT_COPY_STACK_TO, 0, static_cast<int>(PtrToU32(created)), 0);

            SpriteCopyBuildVisualState(this, created);

            if (createVid->spriteClass == 2u && createVid->scriptFunction[19] != 0)
            {
                for (int attempt = 0; attempt<3; ++attempt)
                {
                    SPRITE_COLLECTOR* collector = ::Sprites;
                    SPRITE* collision = collector->CanPlace(createVid, created->Coor(), created->Direction());
                    if (collision && collision != created)
                        (void) Map->ScriptRun(createVid->scriptFunction[19], created, collision, 0);
                }
            }
            return 0;
        }
    case ACT_COOR_ATTACK:
        {
            VID* metricVid = m_vid;
            if (m_childChain && m_childChain->m_vid == m_vid->linkVid &&
                m_childChain->m_vid->nWeapon != 0 && m_childChain->m_vid->childVid[8] != 0)
                metricVid = m_childChain->m_vid;

            const int weaponType = metricVid->weapon->SpriteType;
            const float x = static_cast<float>(argument1);
            float y = static_cast<float>(argument2);
            float z = 0.0f;
            MAP* map = Map;
            if (weaponType == 0x10)
            {
                z = map->GetGroundZ(VECTOR2(x, y+80.0f))+80.0f;
                y = static_cast<float>(argument2+TruncateFloatToInt32(z));
            }
            else
            {
                z = map->GetGroundZ(VECTOR2(x, y))+19.0f;
                y = static_cast<float>(argument2+TruncateFloatToInt32(z)-19);
            }
            (void) SetCommand(4, SpriteCreateActionTarget(VECTOR(x, y, z)));
            return 0;
        }
    case ACT_DESTROY_UNIT:
        {
            if (argument1 == -1)
            {
                this->~SPRITE();
                CrtFreePointer(this);
                return 0;
            }
            MAP* map = Map;
            VID* requested = 0;
            if (argument1 >= 0 && argument1<map->m_vidCount)
                requested = map->m_vids[argument1];
            if (argument1 <= 0 || !requested)
            {
                SPRITE::Error(E_INVALID, STRING("var1 in ACT_DESTROY_UNIT"), static_cast<unsigned long>(argument1));
                return 0;
            }
            if (argument2 == 0 && argument3 == 0)
            {
                const VECTOR2 origin(0.0f, 0.0f);
                for (; ;)
                {
                    SPRITE_COLLECTOR* collector = ::Sprites;
                    SPRITE* hit = collector->FindNearestSprite(argument1, origin, 40000.0f, 0);
                    if (!hit) break;
                    hit->~SPRITE();
                    CrtFreePointer(hit);
                }
            }
            else
            {
                const VECTOR2 point(static_cast<float>(argument2), static_cast<float>(argument3));
                SPRITE_COLLECTOR* collector = ::Sprites;
                SPRITE* hit = collector->GetSpriteScr(argument1, point, 0);
                if (hit)
                {
                    hit->~SPRITE();
                    CrtFreePointer(hit);
                }
            }
            return 0;
        }

    case ACT_NO_ITEMS:
            return m_extraData ? m_extraData->items.m_count : 0;
    case ACT_ADD_ITEM:
            InsertItem(argument1);
        return 0;
    case ACT_DELETE_ITEM:
            if (m_extraData)
        {
            LIST<int>& items = m_extraData->items;
            int index = items.m_count-1;
            int* values = items.m_data;
            while (index >= 0 && values[index] != argument1) --index;
            if (items.DeleteNumber(index) == 0) return 1;
        }
        return 0;
    case ACT_HAVE_ITEM:
            if (m_extraData)
        {
            const LIST<int>& items = m_extraData->items;
            const int* values = items.m_data;
            for (int index = items.m_count-1; index >= 0; --index)
                if (values[index] == argument1) return 1;
        }
        return 0;
    case ACT_DELETE_ALL_ITEM:
            if (m_extraData)
        {
            LIST<int>& items = m_extraData->items;
            items.m_count = 0;
            items.m_capacity = 0;
            if (items.m_data) (void)(CrtFreePointer(items.m_data), 0u);
            items.m_data = 0;
        }
        return 0;
    case ACT_GET_ITEM:
            if (m_extraData && argument1 >= 0 && argument1<m_extraData->items.m_count)
            return m_extraData->items.m_data[argument1];
        return 0;
    case ACT_GET_ITEM_TYPE:
        {
            const unsigned int mask = argument1?static_cast<unsigned int>(argument1):0x00FFFFFFu;
            if (!m_extraData || m_extraData->items.m_count <= 0) return -1;
            MAP* map = Map;
            const int* items = m_extraData->items.m_data;
            int matches = 0;
            for (int itemIndex = 0; itemIndex<m_extraData->items.m_count; ++itemIndex)
            {
                VID* itemVid = 0;
                const int itemNvid = items[itemIndex];
                if (itemNvid >= 0 && itemNvid<map->m_vidCount)
                    itemVid = map->m_vids[itemNvid];
                if (!itemVid) itemVid = ::EmptyVid;
                if ((itemVid->spriteType&mask) != 0u) ++matches;
            }
            if (matches <= 0) return -1;

            if (map->IsNetGameStart() != 0 && (m_vid->runtimeFlags&0x00001000u) == 0u)
                SPRITE::Error(E_ERROR, STRING("(synchro)in ACT_GET_ITEM_TYPE, not propImportantForSync"), static_cast<unsigned long>(0));

            int selected = SpriteRandomInclusiveForVid(m_vid, matches-1);
            const int itemCount = m_extraData->items.m_count;
            items = m_extraData->items.m_data;
            map = Map;
            for (int itemIndex = 0; itemIndex<itemCount; ++itemIndex)
            {
                VID* itemVid = 0;
                const int itemNvid = items[itemIndex];
                if (itemNvid >= 0 && itemNvid<map->m_vidCount)
                    itemVid = map->m_vids[itemNvid];
                if (!itemVid) itemVid = ::EmptyVid;
                if ((itemVid->spriteType&mask) == 0u) continue;
                if (selected-- == 0) return items[itemIndex];
            }
            return -1;
        }

    case ACT_CHANGE_DIRECTION:
            ChangeDirection(ANGLE(static_cast<unsigned char>(argument1)));
        return 0;
    case ACT_CHANGE_ANIMATION:
            ChangeAnimation(argument1);
        return 0;
    case ACT_CHANGE_VID:
        {
            MAP* map = Map;
            VID* nextVid = 0;
            if (argument1 >= 0 && argument1<map->m_vidCount)
                nextVid = map->m_vids[argument1];
            if (!nextVid || m_vid->nVid == argument1) return 0;

            VID* oldVid = m_vid;
            if (this != Mouse &&
                oldVid->spriteClass != nextVid->spriteClass)
                SPRITE::Error(E_INVALID, STRING("ACT_CHANGE_VID"), static_cast<unsigned long>(argument1));

            for (VID* link = m_vid->linkVid; link; link = link->linkVid)
                (void) DestroyLink(link);

            m_runtimeFlags &= ~0x00800000u;
            ::Sprites->Delete(this);

            const unsigned int army = (m_runtimeFlags>>17u)&7u;
            VID* const populationVid = m_vid;
            if (populationVid->spriteCountsByPlayer[army] != 0u)
                --populationVid->spriteCountsByPlayer[army];

            VID* replacementVid = 0;
            MAP* currentMap = Map;
            if (argument1<currentMap->m_vidCount)
                replacementVid = currentMap->m_vids[argument1];
            if (!replacementVid)
                replacementVid = ::EmptyVid;
            m_vid = replacementVid;
            m_vid->lastSpriteCountChangeTick = ::RealCurrentTime;
            ++m_vid->spriteCountsByPlayer[army];

            const int requestedAnimation = argument2<0?m_currentAnimation:argument2;
            const ANGLE savedDirection = m_direction;
            m_direction = ANGLE(0);
            m_currentAnimation = 0;
            m_currentFrameBegin = 0;
            m_currentFrame = 0;
            m_currentFrameEnd = m_vid->animationFrameCount[0]-1;

            if (!m_extraData)
            {
                if (m_vid->actionAuxStateRequired != 0)
                    m_extraData = SpriteEnsureExtraData(this);
            }
            else
            {
                m_extraData->lifetimeRemaining = static_cast<unsigned int>(m_vid->lifeTime);
                m_extraData->maxSpeedOverride = m_vid->maxSpeed;
            }

            ::Sprites->Insert(this);
            CreateLink();
            ChangeAnimation(requestedAnimation);
            ChangeDirection(savedDirection);
            return 0;
        }
    case ACT_CHANGE_COOR:
            ChangeCoor(VECTOR(static_cast<float>(argument1), static_cast<float>(argument2), static_cast<float>(argument3)));
        return 0;

    case ACT_GOTO_STACK:
            if (m_extraData)
        {
            LIST<ACT>& lane = m_extraData->actionLane[m_extraData->actionStackIndex];
            lane.m_count = argument1+1;
            if (lane.m_count>lane.m_capacity) lane.Expand(lane.m_count);
            m_currentFrame = m_currentFrameEnd;
        }
        return 0;
    case ACT_CLEAR_STACK:
            if (m_extraData)
        {
            LIST<ACT>& lane = m_extraData->actionLane[argument1];
            lane.m_count = 0;
            lane.m_capacity = 0;
            if (lane.m_data) (void)(CrtFreePointer(lane.m_data), 0u);
            lane.m_data = 0;
        }
        return 0;
    case ACT_RESTORE_COMMAND:
        {
            SPRITE* target = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1)));
            const int restoredCommand = (static_cast<unsigned int>(opcode)>>8)&0xFFu;
            (void) SetCommand(restoredCommand, target);
            if (target) (void) target->Release();
            return 0;
        }
    case ACT_BACKUP_COMMAND:
        {
            const int backupOpcode = static_cast<int>((((m_runtimeFlags>>2)&31u)<<8) | ACT_RESTORE_COMMAND);
            AddActionImmediate(0, backupOpcode, static_cast<int>(PtrToU32(m_goalSprite)), 0, 0);
            if (m_goalSprite) ++m_goalSprite->m_referenceCount;
            return 0;
        }
    case ACT_COPY_STACK_TO:
        {
            SPRITE* target = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument2)));
            if (!target || target == this) return 0;

            int laneIndex = (argument1<0)?3:0;
            for (; laneIndex >= 0; --laneIndex)
            {
                target->ResetActionStack(laneIndex);
                if (!m_extraData) continue;

                for (int index = 0; m_extraData && index<m_extraData->actionLane[laneIndex].m_count; ++index)
                {
                    const ACT& action = m_extraData->actionLane[laneIndex].m_data[index];
                    if (action.command == ACT_STOP_STACK) break;
                    target->AddActionImmediate(laneIndex, action.command, action.arg1, action.arg2, action.arg3);
                }
            }
            return 0;
        }
    case ACT_NO_STACK:
            return m_extraData ? m_extraData->actionLane[argument1].m_count : 0;
    case ACT_UNDO_REMOVE:
        {
            m_runtimeFlags |= 0x00000800u;
            for (SPRITE* sprite = this; sprite; sprite = sprite->m_childChain)
                (void) ::Sprites->Delete(sprite);
            return 0;
        }
    case ACT_UNDO_INSERT:
        {
            m_runtimeFlags &= ~0x00000800u;
            for (SPRITE* sprite = this; sprite; sprite = sprite->m_childChain)
                ::Sprites->Insert(sprite);
            return 0;
        }

    case ACT_SAVE:
        {
            STREAM* const stream = reinterpret_cast<STREAM*>(U32ToPtr(static_cast<unsigned int>(argument1)));
            (void) stream->Write(&m_behave, 4u);

            int laneCount = 0;
            if (m_extraData)
            {
                for (int laneIndex = 0; laneIndex<4 && m_extraData; ++laneIndex)
                {
                    LIST<ACT>& lane = m_extraData->actionLane[laneIndex];
                    if (lane.m_count == 1 && lane.m_data[0].command == ACT_STOP_STACK)
                    {
                        void* const oldData = lane.m_data;
                        lane.m_capacity = 0;
                        lane.m_count = 0;
                        if (oldData) CrtFreePointer(oldData);
                        lane.m_data = 0;
                    }
                    if (m_extraData->actionLane[laneIndex].m_count != 0) laneCount = 4;
                }
            }

            (void) stream->Write(&laneCount, 4u);
            for (int laneIndex = 0; laneIndex<laneCount; ++laneIndex)
            {
                LIST<ACT>& lane = m_extraData->actionLane[laneIndex];
                (void) stream->Write(&lane.m_count, 4u);
                (void) stream->Write(lane.m_data, static_cast<unsigned int>(lane.m_count)*16u);
            }

            LIST<int> temporary;
            temporary.m_count = 0;
            temporary.m_capacity = 0;
            temporary.m_data = 0;
            if (m_extraData) SpriteActionCopyIntList(temporary, m_extraData->items);
            (void) stream->Write(&temporary.m_count, 4u);
            (void) stream->Write(temporary.m_data, static_cast<unsigned int>(temporary.m_count)*4u);
            SpriteActionFreeIntList(temporary);

            STRING name;
            name = GetName();
            const char* text = name.m_value;
            (void) stream->Write(text, static_cast<unsigned int>(std::strlen(text))+1u);
            return 0;
        }

    case ACT_RESTORE:
        {
            STREAM* const stream = reinterpret_cast<STREAM*>(U32ToPtr(static_cast<unsigned int>(argument1)));
            const int version = argument2;
            if (version >= 19) (void) stream->Read(&m_behave, 4u);

            int laneCount = 0;
            if (version >= 17) (void) stream->Read(&laneCount, 4u);
            if (version<18) laneCount = 1;

            if (laneCount != 0 && !m_extraData)
                m_extraData = SpriteEnsureExtraData(this);

            for (int laneIndex = 0; laneIndex<laneCount; ++laneIndex)
            {
                LIST<ACT>& lane = m_extraData->actionLane[laneIndex];
                (void) stream->Read(&lane.m_count, 4u);
                if (lane.m_count>lane.m_capacity) lane.Expand(lane.m_count);
                (void) stream->Read(lane.m_data, static_cast<unsigned int>(lane.m_count)*16u);

                for (int index = m_extraData->actionLane[laneIndex].m_count-1; index >= 0; --index)
                {
                    LIST<ACT>& currentLane = m_extraData->actionLane[laneIndex];
                    ACT* const records = currentLane.m_data;
                    ACT& record = records[index];
                    if (version<14 && record.command == ACT_SET_COMMAND) record.command = ACT_SET_DEATH_TIMER;

                    const unsigned int recordOpcode = static_cast<unsigned int>(record.command);
                    if (SpriteActionRecordHasSpritePointer(recordOpcode))
                    {
                        const unsigned int savedPointer = static_cast<unsigned int>(record.arg1);
                        const unsigned int resolved = PtrToU32(Map->m_relation.Decode(reinterpret_cast<SPRITE*>(U32ToPtr(savedPointer))));
                        record.arg1 = static_cast<int>(resolved);
                        ACT& currentRecord = m_extraData->actionLane[laneIndex].m_data[index];
                        if (currentRecord.arg1 == 0 && argument3 == 999) currentRecord.arg1 = static_cast<int>(savedPointer);
                    }
                    else if (laneIndex != 0 && index>0 &&
                        record.command == ACT_STOP_STACK && records[index-1].command == ACT_STOP_STACK && index<currentLane.m_count)
                    {
                        const int newCount = currentLane.m_count-1;
                        currentLane.m_count = newCount;
                        record = records[newCount];
                    }
                }
            }

            if (version >= 12)
            {
                LIST<int> temporary;
                temporary.m_count = 0;
                temporary.m_capacity = 0;
                temporary.m_data = 0;

                (void) stream->Read(&temporary.m_count, 4u);
                SpriteActionEnsureTemporaryIntList(temporary);
                (void) stream->Read(temporary.m_data, static_cast<unsigned int>(temporary.m_count)*4u);

                if (temporary.m_count != 0)
                {
                    if (!m_extraData)
                        m_extraData = SpriteEnsureExtraData(this);
                    SpriteActionCopyIntList(m_extraData->items, temporary);
                }
                SpriteActionFreeIntList(temporary);
            }

            if (version >= 17)
            {
                STRING name;
                (void) name.Read(stream);
                SetName(name);
            }
            return 0;
        }

    case ACT_GET_BIRTH_TIME:
            return static_cast<int>(m_birthTime);
    case ACT_GET_HP:
            return m_hp;
    case ACT_SET_HP:
            if (argument1 == 0 && argument2 != 0)
        {
            const unsigned int army = (m_runtimeFlags>>17)&7u;
            const int maximumHp = m_vid->maxHpByPlayer[army];
            int hp = BitsToSigned32(static_cast<unsigned int>(maximumHp)*static_cast<unsigned int>(argument2))/100;
            if (hp <= 1) hp = 1;
            ChangeHp(hp);
        }
        else ChangeHp(argument1);
        return 0;
    case ACT_GET_PERCENT_HP:
            return (PercentHp()*100)/255;
    case ACT_GET_BATTLE_RANGE:
        {
            if (m_childChain && m_childChain->m_vid == m_vid->linkVid &&
                m_childChain->m_vid->nWeapon != 0 && m_childChain->m_vid->childVid[8] != 0)
            {
                const float parentRange = m_vid->weapon->BattleRange;
                if (SpriteX87EqualOrUnorderedZero(parentRange))
                    return TruncateFloatToInt32(m_childChain->m_vid->weapon->BattleRange);
            }
            return TruncateFloatToInt32(m_vid->weapon->BattleRange);
        }
    case ACT_DAMAGE:
        {
            const unsigned int army = (m_runtimeFlags>>17)&7u;
            const int maximumHp = m_vid->maxHpByPlayer[army];
            if (m_hp >= maximumHp && argument1<0) return 1;
            if (m_currentAnimation >= 15) return 0;

            SPRITE* source = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument2)));
            int scriptResult = 0;
            const int damageScript = m_vid->scriptFunction[18];
            if (damageScript >= 0)
                scriptResult = Map->ScriptRun(damageScript, this, source, argument1);
            if (scriptResult == 0)
            {
                if (m_vid->maxHp != 0)
                    ChangeHp(BitsToSigned32(static_cast<unsigned int>(m_hp)-static_cast<unsigned int>(argument1)));
                const unsigned int currentArmy = (m_runtimeFlags>>17)&7u;
                const int currentMaximumHp = m_vid->maxHpByPlayer[currentArmy];
                if (m_hp>currentMaximumHp && argument1<0) m_hp = currentMaximumHp;
            }
            if (argument1 <= 0) return 0;

            if (scriptResult == 0)
            {
                if (m_vid->noAnimCadr[7] != 0u && (m_currentAnimation == 0 || m_currentAnimation == 2))
                    ChangeAnimation(7);
                else
                    (void) CreateChildFor(7, source);
            }
            if (source && m_currentAnimation >= 15)
            {
                const unsigned int killerArmy = (source->m_runtimeFlags>>17)&7u;
                m_runtimeFlags = (m_runtimeFlags&0xFF8FFFFFu) | (killerArmy<<20);
            }
            return 0;
        }
    case ACT_REPAIR:
        {
            (void) DestroyLink(m_vid->childVid[13]);
            const unsigned int army = (m_runtimeFlags>>17)&7u;
            m_hp = m_vid->maxHpByPlayer[army];
            if (m_childChain && m_childChain->m_vid == m_vid->linkVid)
                (void) m_childChain->Action(ACT_REPAIR, 0, 0, 0);
            else
                CreateLink();
            return 0;
        }
    case ACT_GET_GOAL:
            return static_cast<int>(PtrToU32(m_goalSprite));
    case ACT_SET_GOAL_COOR:
            SetGoal(SpriteCreateActionTarget(VECTOR(static_cast<float>(argument1), static_cast<float>(argument2), static_cast<float>(argument3))));
        return 0;
    case ACT_GET_BEHAVE:
            return static_cast<int>(m_behave);
    case ACT_SET_BEHAVE:
            m_behave = static_cast<unsigned int>(argument1);
        return 0;
    case ACT_SET_TEXTURE_VID:
        {
            MAP* map = Map;
            VID* textureVid = 0;
            if (argument1 >= 0 && argument1<map->m_vidCount)
                textureVid = map->m_vids[argument1];
            if (textureVid) SetEnvTexture(textureVid);
            return 0;
        }
    case ACT_GET_ARMY:
            return static_cast<int>((m_runtimeFlags>>17)&7u);
    case ACT_SET_ARMY:
            ChangeArmy(argument1);
        return 0;
    case ACT_SET_INVISIBLE:
        {
            if (argument1)
            {
                m_runtimeFlags |= 0x04000000u;
                if (m_childChain) m_childChain->InvisibleOn();
            }
            else
            {
                m_runtimeFlags &= ~0x04000000u;
                if (m_childChain) m_childChain->InvisibleOff();
            }

            if ((m_vid->property&0x40u) != 0u && (m_runtimeFlags&0x08000000u) == 0u)
                (void) ::Sprites->DeleteFromHash(this);
            if ((m_vid->property&0x28u) != 0u && (m_runtimeFlags&0x08000000u) == 0u)
                m_vid->ResetGridZ(this);

            m_runtimeFlags = (m_runtimeFlags&~0x08000000u) | (argument2?0x08000000u:0u);

            if ((m_vid->property&0x40u) != 0u && (m_runtimeFlags&0x08000000u) == 0u)
                ::Sprites->InsertToHash(this);
            if ((m_vid->property&0x28u) != 0u && (m_runtimeFlags&0x08000000u) == 0u)
                m_vid->SetGridZ(this);
            return 0;
        }

    case ACT_GET_LINK:
            return static_cast<int>(PtrToU32(m_childChain));
    case ACT_SET_LINK:
            (void) AddLinkToLast(reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1))));
        return 0;
    case ACT_GET_UPLINK:
            return static_cast<int>(PtrToU32(m_childBacklink));
    case ACT_SET_UPLINK:
            if (argument1)
            (void) reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument1)))->AddLinkToLast(this);
        return 0;

    case ACT_GET_TIMER:
            return static_cast<int>(m_stateTimer);
    case ACT_SET_TIMER:
            m_stateTimer = static_cast<unsigned int>(argument1);
        return 0;
    case ACT_GET_ZSPEED:
            return TruncateFloatToInt32(m_zSpeed*1000.0f);
    case ACT_SET_ZSPEED:
            m_zSpeed = static_cast<float>(argument1)*0.001f;
        return 0;
    case ACT_GET_SPEED:
            return TruncateFloatToInt32(m_speed*1000.0f);
    case ACT_SET_SPEED:
            if (SpriteX87EqualOrUnorderedZero(m_speed) && argument1>0 && !m_goalSprite) (void) StartMove();
        m_speed = static_cast<float>(argument1)*0.001f;
        return 0;

    case ACT_GET_AMMO:
        case ACT_ADD_AMMO:
            return m_childBacklink ? m_childBacklink->Action(opcode, argument1, argument2, argument3) : 0;

    case ACT_GET_COMMAND:
            return static_cast<int>((m_runtimeFlags>>2)&31u);
    case ACT_SET_COMMAND:
            (void) SetCommand(argument1, reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(argument2))));
        return 0;
    case ACT_SET_DEATH_TIMER:
        {
            EX_SPRITE_DATA* extraData = SpriteEnsureExtraData(this);
            extraData->lifetimeRemaining = static_cast<unsigned int>(argument1);
            return 0;
        }
    case ACT_GET_DIALOG:
            return 0;
    case ACT_SET_FLAGMAN_DIALOG:
        {
            if (Map->Flagman(Map->m_playerIndex))
                (void) Map->Flagman(Map->m_playerIndex)->Action(ACT_SET_DIALOG, argument1, argument2, 0);
            return 0;
        }
    case ACT_GET_KILLER_ARMY:
            return static_cast<int>((m_runtimeFlags>>20)&7u);
    case ACT_SET_OBJECT_ID:
            SetObjectID(argument1);
        return 0;
    case ACT_SET_GAMMA:
        {
            const unsigned int encoded = static_cast<unsigned int>(argument1);
            GAMMA gamma;
            gamma.modulate = 0u;
            gamma.add = 0u;
            const unsigned int channelMasks[4] = {0x7Fu, 0x7F80u, 0x7F8000u, 0xFF800000u};
            for (unsigned int channel = 0; channel<4u; ++channel)
            {
                const unsigned int shift = channel*8u;
                if ((encoded&(0x80u<<shift)) != 0u)
                    gamma.add |= ((~encoded)&channelMasks[channel])<<1u;
                else
                    gamma.modulate |= (encoded&channelMasks[channel])<<1u;
            }
            SetGamma(gamma);
            return 0;
        }
    case ACT_GET_GAMMA:
        {
            if (!m_extraData) return 0;
            const unsigned int modulate = m_extraData->gammaModulate;
            const unsigned int add = m_extraData->gammaAdd;
            unsigned int channel = 0u;
            if (argument1 == 1) channel = 2u;
            else if (argument1 == 2) channel = 1u;
            else if (argument1 == 3) channel = 0u;
            else if (argument1 == 4) channel = 3u;
            else
            {
                unsigned int encoded = (modulate>>1)&0x7F7F7F7Fu;
                for (unsigned int index = 0; index<4u; ++index)
                {
                    const unsigned int shift = index*8u;
                    const unsigned int value = (add>>shift)&0xFFu;
                    if (value)
                    {
                        const unsigned int signedMagnitude = 0x80u | (((~value)&0xFEu)>>1);
                        encoded |= signedMagnitude<<shift;
                    }
                }
                return static_cast<int>(encoded);
            }
            const unsigned int shift = channel*8u;
            const unsigned int negative = (modulate>>shift)&0xFFu;
            if (negative) return -static_cast<int>(negative);
            return static_cast<int>((add>>shift)&0xFFu);
        }
    case ACT_NEXT_COMMAND:
            if (m_currentAnimation<15)
        {
            if (!m_childBacklink) (void) MaxSpeed();
            SetMoveAnimation();
        }
        return 0;

    case ACT_GET_SCALE:
            if (!m_extraData) return 1000;
        if (argument1 == 0) return TruncateFloatToInt32(m_extraData->scale.x*1000.0f);
        if (argument1 == 1) return TruncateFloatToInt32(m_extraData->scale.y*1000.0f);
        return TruncateFloatToInt32(m_extraData->scale.z*1000.0f);
    case ACT_SET_SCALE:
        {
            // Preserve the original single precision multiplier before widening it.
            const double multiplier = 0.0010000000474974513;
            SetScale(VECTOR(static_cast<float>(static_cast<double>(argument1)*multiplier),
                static_cast<float>(static_cast<double>(static_cast<float>(argument2))*multiplier),
                static_cast<float>(static_cast<double>(static_cast<float>(argument3))*multiplier)));
            return 0;
        }
    case ACT_SET_QUEST:
        {
            MAP* const map = Map;
            PLAYER* const player = map->m_players[0];
            if (player->SetQuestState(argument1, argument2, argument3))
                return 0;
            const int nvid = m_vid->nVid;
            (void) ::Error->Log("SPRITE::Invalid quest number %i_%i! sprite[%i] coor: %i %i", static_cast<unsigned int>(argument1), static_cast<unsigned int>(argument2), static_cast<unsigned int>(nvid), static_cast<unsigned int>(TruncateFloatToInt32(m_xyz.x)), static_cast<unsigned int>(TruncateFloatToInt32(m_xyz.y)));
            return 0;
        }
    case ACT_SCRIPT_VAR:
        {
            int value = argument2;
            if (argument3 == 1)
                value = BitsToSigned32(static_cast<unsigned int>(value)+
                    static_cast<unsigned int>(Map->m_script.GetVariableActionInt(static_cast<unsigned int>(argument1))));
            Map->m_script.SetVariableActionInt(static_cast<unsigned int>(argument1), value);
            return 0;
        }
    case ACT_SCRIPT_EVENT:
            return Map->ScriptEventRun(argument1, this, argument2, argument3);
    case ACT_PLAY_SFX:
            PlaySFX(argument1);
        return 0;
    case ACT_SET_IMPULSE_SPEED:
            m_impulseSpeedX = static_cast<float>(argument1)*0.001f;
        m_impulseSpeedY = static_cast<float>(argument2)*0.001f;
        m_impulseSpeedZ = static_cast<float>(argument3)*0.001f;
        return 0;
    case ACT_GET_ACCELERATION:
        {
            const float slow = m_vid->slow;
            if ((m_runtimeFlags&0x00000400u) != 0u)
            {
                const float maximum = AbsFloat(MaxSpeed());
                const float current = AbsFloat(m_speed);
                if (maximum>current)
                {
                    if (m_vid->acceleration == 999999.0f) return 999999;
                    return TruncateFloatToInt32(m_vid->acceleration*1000000.0f);
                }
                if (m_vid->spriteClass == 5u && SpriteX87LessOrUnordered(MaxSpeed(), m_speed))
                {
                    if (slow == 999999.0f) return -999999;
                    return TruncateFloatToInt32(slow*-1000000.0f);
                }
                return 0;
            }
            if (SpriteX87EqualOrUnorderedZero(m_speed)) return 0;
            if (slow == 999999.0f) return -999999;
            return TruncateFloatToInt32(slow*-1000000.0f);
        }

    case ACT_SET_MAX_SPEED:
        {
            EX_SPRITE_DATA* extraData = SpriteEnsureExtraData(this);
            extraData->maxSpeedOverride = static_cast<float>(argument1)*0.001f;
            return 0;
        }

    case ACT_LEGACY_RESTORE:
            return 0;
    case 201:
            ChangeAnimation(argument1);
        ChangeDirection(ANGLE(static_cast<unsigned char>(argument2)));
        return 0;
    case 202:
            return m_currentAnimation;
    case 203:
            ChangeCoor(VECTOR(static_cast<float>(argument1), static_cast<float>(argument2), m_xyz.z));
        return 0;
    case 204:
            ChangeCoor(VECTOR(m_xyz.x, m_xyz.y, static_cast<float>(argument1)));
        return 0;

    default:

            SPRITE::Error(E_ERROR, STRING("Action() have not this act"), static_cast<unsigned long>(opcode));
        return 0;
    }
}

