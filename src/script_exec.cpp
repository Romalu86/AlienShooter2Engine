#include <cstdio>
#include "engine_globals.h"
#include "zm/Utils/zLogConsole.h"
#include "zm/Utils/zStaticStorage.h"

#include "mylib.h"
#include "basetype.h"
#include "zm/Input/zInputMap.h"
#include "script_exec_dispatch.h"
#include "script.h"
#include "script_exec.h"
#include "myerror.h"
#include "resread.h"
#include "sprite_collector.h"
#include "sprite_list.h"
#include "sprite.h"

#include "map.h"
#include "list.h"
#include "mouse.h"
#include "menu.h"
#include "unit.h"
#include "player_arcade.h"
#include "zm/Game/ScriptExec/zScriptExec.h"
#include "zm/Game/ScriptExec/ScriptExecExternalCommand.h"
#include "base_graph.h"
#include "graph.h"
#include "registry.h"
#include "net.h"
#include "player.h"
#include "input.h"
#include "vid.h"
#include "region.h"
#include "engine.h"
#include "sound.h"
#include "zm/Eng/zProfLdr/zProfLdr.h"
#include <new>
#include <windows.h>
#include <shellapi.h>


void  GRAPH_SetScreenSize(void* self,int width,int height);

namespace
{
    const char* registrationInformation = 0;
    int scriptFileSearch = 0;
    int scriptSpriteIterator = 0;
    int scriptUnitIterator = 0;
    unsigned int scriptUnitMask = 0;
    StaticObjectStorage<STRING> externCommandText;
    unsigned char externCommandTextInitialized = 0;
}

SPRITE* g_pFucking = 0;

void __stdcall GetRegistrationInformation(char* buffer)
{
    registrationInformation = buffer;
}

int PLAYER::RelationTo(unsigned int index) const
{
    return m_relation[index];
}

void PLAYER::SetRelationTo(unsigned int index, int relation)
{
    m_relation[index&7u] = relation;
}

int PLAYER::GetMoney() const
{
    return m_money;
}

void PLAYER::SetMoney(int money)
{
    m_money = money;
}

void PLAYER::SetPlayerControl(int control)
{
    m_controlMode = control;
}


EX_SPRITE_DATA* SPRITE::ExData() const
{
    return m_extraData;
}

int SPRITE::IsSpriteClass(unsigned int spriteClass) const
{
    return m_vid->spriteClass == spriteClass;
}

int SPRITE::IsCommand(unsigned int command) const
{
    return static_cast<int>((m_runtimeFlags>>2)&0x1Fu) == command;
}

void SPRITE::ChangeZCoor(float z)
{
    VECTOR xyz = m_xyz;
    xyz.z = z;
    ChangeCoor(xyz);
}


int TRAIN_INFO::Acceleration() const
{
    const float divisor = totalLoadScalar;
    const float zero = (0.0f);
    if (divisor != divisor || divisor == zero)return 0;
    return TruncateFloatToInt32((totalDriveScalar/divisor)*(8.0f));
}

ENGINE* ENGINE::NextEngine() const
{
    return m_nextEngine;
}







int STRING::IsEmpty() const
{
    return value[0] == 0 ? 1 : 0;
}

int STRING::Int() const
{
    const char* const text = value;
    if (text[1] != 'x')
        return BitsToSigned32(CrtAtoiText(text));
    int result = BitsToSigned32(PtrToU32(this));
    sscanf(text, "%i", &result);
    return result;
}

void STRING::Write(STREAM* stream) const
{
    const char* const text = value;
    const unsigned int length = static_cast<unsigned int>(strlen(text))+1u;
    (void)stream->Write(text, length);
}

void STRING::Write(std::FILE* file) const
{
    const char* const text = value;
    const unsigned int length = static_cast<unsigned int>(strlen(text))+1u;
    (void)std::fwrite(text, length, 1u, file);
}

const STRING STRING::Before(const STRING& delimiter) const
{
    return Before(delimiter.m_value);
}

const STRING STRING::After(const STRING& delimiter) const
{
    return After(delimiter.m_value);
}

char STRING::operator[](int index) const
{
    return m_value[index];
}

std::FILE* FOpen(const STRING& fileName, const STRING& mode)
{
    if (fileName.m_value[0] == 0)
        return 0;
    return std::fopen(fileName.m_value, mode.m_value);
}

CRC32::CRC32(const void* data, unsigned int size)
{
    value = 0u;
    (void)Add(data, size);
}


int VID::PropHide() const
{
    return static_cast<int>((runtimeFlags>>11)&1u);
}

int VID::CanFight() const
{
    return nWeapon != 0 && childVid[8] != 0;
}

int VID::GetMaxHp(unsigned int player) const
{
    return maxHpByPlayer[player&7u];
}

int VID::NoSprites() const
{
    unsigned int sum = 0;
    for (int i = 0;i<PlayerSlotCount;++i) sum += spriteCountsByPlayer[i];
    return static_cast<int>(sum);
}

int VID::Killed(unsigned int player) const
{
    return killedByPlayer[player&7u];
}

int VID::Killed() const
{
    unsigned int sum = 0;
    for (int i = 0;i<PlayerSlotCount;++i) sum += static_cast<unsigned int>(killedByPlayer[i]);
    return static_cast<int>(sum);
}

int VID::ReCaptured(unsigned int player) const
{
    return reCapturedByPlayer[player&7u];
}

int VID::ReCaptured() const
{
    unsigned int sum = 0;
    for (int i = 0;i<PlayerSlotCount;++i) sum += static_cast<unsigned int>(reCapturedByPlayer[i]);
    return static_cast<int>(sum);
}

int VID::PropNotCreateAsChild() const
{
    return notCreateAsChild;
}

void VID::SetPropNotCreateAsChild(int value)
{
    notCreateAsChild = value;
}

int VID::PropBirthAsSmoke() const
{
    return static_cast<int>(property&0x80u);
}


SPRITE* SPRITE_COLLECTOR::FirstActiveSprite(int* index) const
{
    const int count = allSprites.m_count;
    SPRITE** slots = allSprites.m_data;
    *index = count-1;
    while (*index >= 0 && *index<count && slots[*index] == 0)--*index;
    return (*index<0 || *index >= count)?0:slots[*index];
}

SPRITE* SPRITE_COLLECTOR::NextActiveSprite(int* index) const
{
    const int count = allSprites.m_count;
    SPRITE** slots = allSprites.m_data;
    --*index;
    while (*index >= 0 && *index<count && slots[*index] == 0)--*index;
    return (*index<0 || *index >= count)?0:slots[*index];
}


unsigned int NET::IsConnected() const
{
    return (m_flags>>3)&1u;
}

unsigned int NET::IsHost() const
{
    return (m_flags>>1)&1u;
}

unsigned int NET::IsLobby() const
{
    return (m_flags>>2)&1u;
}

unsigned int NET::NoPlayers() const
{
    return m_noPlayers;
}

unsigned long NET::PlayerDPNID(unsigned int index) const
{
    return index<8u ? m_players[index].m_dpnid : 0u;
}


int MAP::PopInt()
{
    const int depth = AddSigned32Wrap(m_script.m_stack.m_count, -1);
    m_script.m_stack.m_count = depth;
    return m_script.m_stack.m_data[depth].Int();
}

const STRING& MAP::PopStr()
{
    const int depth = AddSigned32Wrap(m_script.m_stack.m_count, -1);
    m_script.m_stack.m_count = depth;
    return m_script.m_stack.m_data[depth].String();
}

void MAP::PushInt(int value)
{
    m_script.PushInt(value);
}

void MAP::PushStr(const STRING& value)
{
    m_script.PushStr(value);
}

void MAP::PushSprite(SPRITE* sprite, const STRING& tag)
{
    m_script.PushObject(sprite, tag);
}

float MAP::ToScreenY(float lhs, float rhs) const
{
    return lhs-rhs-m_shiftXY.y;
}

const VECTOR2& MAP::SizeXY() const
{
    return m_sizeXY;
}

PLAYER* MAP::Player(int index) const
{
    return m_players[static_cast<unsigned int>(index)&7u];
}

PLAYER* MAP::Player() const
{
    return m_players[m_playerIndex&7u];
}


int __cdecl ScriptExecFunc(int command)
{

    MAP* const map = Map;
    return map->ExecFunc(command);
}

int SCRIPT::IsLastStackString() const
{
    return m_stack.m_data[m_stack.m_count - 1].m_flags & 1;
}

void __cdecl ScriptExecReleaseGlobalString()
{
    externCommandText.Get().~STRING();
}

const STRING& SCRIPTSTACK::String()
{
    if ((m_flags & 2u) == 0)
        return m_stringValue;

    STRING temporary = Int2Str(m_intValue);
    return m_stringValue = temporary;
}

VID* MAP::PopVid(const char* diagnosticContext)
{
    const int depth = AddSigned32Wrap(m_script.m_stack.m_count, -1);
    m_script.m_stack.m_count = depth;
    const int index = m_script.m_stack.m_data[depth].Int();

    VID* const fallback = ::EmptyVid;
    VID* result = fallback;
    const int objectCount = m_vidCount;
    if (index >= 0 && index<objectCount)
    {
        VID* candidate = m_vids[index];
        if (candidate) result = candidate;
    }

    if (result == fallback)
    {
        MYERROR* const logger = ::Error;
        if (logger && diagnosticContext != StringGlobals::EmptyText() && g_iZmDebug == 0u)
            (void)logger->Log("!!!ERROR!!!SCRIPT: Invalid nvid %s %i", diagnosticContext, index);
    }
    return result;
}


void SCRIPT::PushInt(int value)
{
    const STRING tag(StringGlobals::EmptyText());
    SCRIPTSTACK record(value, tag);
    m_stack.Insert(record);
}


void SCRIPT::PushObject(void* object, const STRING& tag)
{
    SCRIPTSTACK record;
    record.m_flags = static_cast<unsigned char>(object?0x12u:0x02u);
    record.m_intValue = PtrToU32(object);
    (void)tag;
    m_stack.Insert(record);
}


void SCRIPT::PushStr(const STRING& source)
{
    const STRING tag(StringGlobals::EmptyText());
    SCRIPTSTACK record(source, tag);
    m_stack.Insert(record);
}

unsigned int __cdecl SrcScriptExec_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

unsigned char __cdecl InterpolateSignedByteChannel(int from, int to, int factor)
{
    if (from >= 128) from -= 254;
    if (to >= 128) to -= 254;
    const int difference = BitsToSigned32(static_cast<unsigned int>(to)-static_cast<unsigned int>(from));
    const int product = MultiplySigned32Wrap(difference, factor);
    return static_cast<unsigned char>(static_cast<unsigned int>(from)+static_cast<unsigned int>(product/255));
}


unsigned int __cdecl InterpolatePackedRGB(unsigned int from, unsigned int to, int factor)
{
    const unsigned int r = InterpolateSignedByteChannel(
        static_cast<int>(from & 0xFFu), static_cast<int>(to & 0xFFu), factor);
    const unsigned int g = InterpolateSignedByteChannel(
        static_cast<int>((from >> 8) & 0xFFu), static_cast<int>((to >> 8) & 0xFFu), factor);
    const unsigned int b = InterpolateSignedByteChannel(
        static_cast<int>((from >> 16) & 0xFFu), static_cast<int>((to >> 16) & 0xFFu), factor);
    return r | (g << 8) | (b << 16);
}

GAMMA::GAMMA(GAMMA_CREATE create, unsigned long packed)
{
    (void)create;
    modulate = 0u;
    add = 0u;
    const unsigned int encoded = static_cast<unsigned int>(packed);
    // The packed format carries the preceding channel's sign bit into
    // the next channel's low bit. These masks are intentionally not byte lanes.
    const unsigned int magnitudeMasks[4] = {0x0000007Fu, 0x00007F80u, 0x007F8000u, 0xFF800000u};
    const unsigned int signMasks[4] = {0x00000080u, 0x00008000u, 0x00800000u, 0x80000000u};
    for (int channel = 0;channel<4;++channel)
    {
        if ((encoded&signMasks[channel]) != 0u)
            add |= ((~encoded)&magnitudeMasks[channel])<<1;
        else
            modulate |= (encoded&magnitudeMasks[channel])<<1;
    }
}

unsigned int __cdecl StaticAtexitGuard()
{
    unsigned char flags = CrtStaticInitializationGuards[0];
    if ((flags & 1u) == 0)
        CrtStaticInitializationGuards[0] = static_cast<unsigned char>(flags | 1u);
    return SrcScriptExec_StaticCleanupRegister01();
}


void MAP::Error(TYPE_ERROR type, const STRING& subject, unsigned long code)
{
    if (!::Error)return;
    const STRING objectFormat("MAP");
    ::Error->Error(objectFormat, type, subject, code);
}

static inline bool ScriptExternCStringEqual(const char* lhs, const char* rhs)
{
    while (*lhs == *rhs)
    {
        if (*lhs == '\0')return true;
        ++lhs;
        ++rhs;
    }
    return false;
}

static inline int ScriptExternTrig1024(int angle, bool cosine)
{
    const unsigned char index = static_cast<unsigned char>(angle);
    const float value = cosine
        ? ANGLE
    {
        index
    }
    .Cos()
        : ANGLE
    {
        index
    }
    .Sin();
    return TruncateFloatToInt32(value*1024.0f);
}

static inline int ScriptExternToScreenY(const MAP* map, int y, int z)
{
    return TruncateFloatToInt32(map->ToScreenY(static_cast<float>(y), static_cast<float>(z)));
}

static inline void ScriptExternSetCursor(int type)
{
    MOUSE* cursor = Mouse;
    if (type == -1)
    {
        cursor->Disable();
        return;
    }
    if (type == 256)
    {
        cursor->HardwareOn();
        return;
    }
    if (type == 257)
    {
        cursor->HardwareOff();
        return;
    }
    if (cursor->Enabled() == 0u)
        cursor->Enable();
    cursor->ChangeAnimation(type);
}

static inline unsigned int ScriptExternIsPlayMusic()
{
    const SOUND* const sound = ::Sound;
    MUSIC* const music = sound->m_music;
    if (!music)return 0u;
    if (music->IsPlaying() != 0u)return 1u;
    if (sound->m_musicMode != 0)return 1u;
    const unsigned char* left = reinterpret_cast<const unsigned char*>(sound->m_musicName.value);
    const unsigned char* right = reinterpret_cast<const unsigned char*>(StringGlobals::EmptyText());
    while (*left == *right)
    {
        if (*left == 0u)return 0u;
        ++left;
        ++right;
    }
    return 1u;
}

static inline unsigned int ScriptExternGetGamma()
{
    const GRAPH* const graph = Graph;
    const unsigned int gamma = graph->gamma.modulate;
    const unsigned int control = graph->gamma.add;
    unsigned int result = (gamma>>1)&0x7F7F7F7Fu;
    if ((control&0x000000FFu) != 0u)
    {
        unsigned int lane = (~control)&0x000000FEu;
        lane |= 0x00000100u;
        result |= (lane>>1);
    }
    if ((control&0x0000FF00u) != 0u)
    {
        unsigned int lane = (~control)&0x0000FE00u;
        lane |= 0x00010000u;
        result |= (lane>>1);
    }
    if ((control&0x00FF0000u) != 0u)
    {
        unsigned int lane = (~control)&0x00FE0000u;
        lane |= 0x01000000u;
        result |= (lane>>1);
    }
    if ((control&0xFF000000u) != 0u)
    {
        unsigned int lane = (~control)>>1;
        lane &= 0xFF000000u;
        lane |= 0x80000000u;
        result |= lane;
    }
    return result;
}

int MAP::ExecFunc(int command)
{
    unsigned char guard = externCommandTextInitialized;
    if ((guard&1u) == 0u)
    {
        ::new (externCommandText.Address()) STRING;
        externCommandTextInitialized = static_cast<unsigned char>(guard|1u);
        (void)RegisterCrtAtExit(&ScriptExecReleaseGlobalString);
    }
    if (command<65u || command>255u)
    {
        MYERROR* sink = ::Error;
        sink->Log("!!!ERROR!!!LOGIC: Unknown extern Function %i", command);
        return 0u;
    }

    SCRIPT* stack = &m_script;
    SPRITE_COLLECTOR* const world = ::Sprites;

    switch (command)
    {
    case SCRIPT_EXT_CreateSprite:
        {
        const unsigned int parent = static_cast<unsigned int>(PopInt());
        const unsigned int direction = static_cast<unsigned int>(PopInt());
        const int z = PopInt();
        const int y = PopInt();
        const int x = PopInt();
        const int nvid = PopInt();
        VID* const resource = Vid(nvid);
        if (resource == ::EmptyVid)
        {
            MYERROR* const sink = ::Error;
            if (sink && g_iZmDebug == 0u)
                sink->Log("!!!ERROR!!!SCRIPT: Invalid nvid %s %i", "for CreateSprite()", nvid);
        }
        if (resource != ::EmptyVid)
        {
            const STRING tag("return CreateSprite");
            const ANGLE angle(static_cast<unsigned char>(direction));
            const float positionZ = static_cast<float>(z);
            const float positionY = static_cast<float>(y);
            const float positionX = static_cast<float>(x);
            const VECTOR position(positionX, positionY, positionZ);
            SPRITE* const result = CreateSprite(resource, position, angle,
                reinterpret_cast<SPRITE*>(U32ToPtr(parent)));
            stack->PushObject(result, tag);
        }
        else stack->PushInt(0);
        break;
    }
    case SCRIPT_EXT_Flagman:
        {
        const STRING tag("return Flagman");
        const int index = PopInt();
        SPRITE* const result = Flagman(static_cast<unsigned int>(index));
        stack->PushObject(result, tag);
        break;
    }
    case SCRIPT_EXT_FirstUnit:
        {
        const int lane = PopInt();
        const unsigned int requiredMask = static_cast<unsigned int>(PopInt());
        scriptUnitMask = requiredMask;
        int* const cursor = &scriptUnitIterator;
        SPRITE* candidate = ::Sprites->FirstUnit(cursor, lane);
        while (candidate)
        {
            if ((candidate->Vid()->spriteType&scriptUnitMask) != 0u)break;
            candidate = ::Sprites->NextUnit(cursor);
        }
        const STRING tag("return FirstUnit");
        stack->PushObject(candidate, tag);
        break;
    }
    case SCRIPT_EXT_NextUnit:
        {
        int* const cursor = &scriptUnitIterator;
        SPRITE* candidate;
        for (;;)
        {
            candidate = ::Sprites->NextUnit(cursor);
            if (!candidate || (candidate->Vid()->spriteType&scriptUnitMask) != 0u)break;
        }
        const STRING tag("return NextUnit");
        stack->PushObject(candidate, tag);
        break;
    }
    case SCRIPT_EXT_GetSprite:
        {
        SPRITE* const previous = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        const int y = PopInt();
        const int x = PopInt();
        const int type = PopInt();
        const STRING tag("return GetSprite");
        SPRITE* const result = ::Sprites->GetSprite(
            type, static_cast<float>(x), static_cast<float>(y), previous);
        stack->PushObject(result, tag);
        break;
    }
    case SCRIPT_EXT_GetSpriteScr:
        {
        const int y = PopInt();
        const int x = PopInt();
        const int type = PopInt();
        const STRING tag("return GetSpriteScr");
        SPRITE_COLLECTOR* const collector = ::Sprites;
        const VECTOR2 screenPoint = {static_cast<float>(x), static_cast<float>(y)};
        SPRITE* const result = collector->GetSpriteScr(type, screenPoint, 0);
        stack->PushObject(result, tag);
        break;
    }
    case SCRIPT_EXT_FindNearestSprite:
        {
        SPRITE* const previous = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        const int radius = PopInt();
        const int y = PopInt();
        const int x = PopInt();
        const int type = PopInt();
        const STRING tag("return FindNearestSprite");
        const VECTOR2 point = {static_cast<float>(x), static_cast<float>(y)};
        const float distance = static_cast<float>(radius);
        SPRITE* const result = ::Sprites->FindNearestSprite(
            type, point, distance, previous);
        stack->PushObject(result, tag);
        break;
    }
    case SCRIPT_EXT_FirstInBox:
        {
        const float maxY = static_cast<float>(PopInt());
        const float maxX = static_cast<float>(PopInt());
        const float minY = static_cast<float>(PopInt());
        const float minX = static_cast<float>(PopInt());
        const STRING tag("return FirstInBox");
        SPRITE* const result = ::Sprites->FirstHashInBox(minX, minY, maxX, maxY);
        stack->PushObject(result, tag);
        break;
    }
    case SCRIPT_EXT_NextInBox:
        {
        const STRING tag("return NextInBox");
        SPRITE* const result = ::Sprites->NextHashInBox();
        stack->PushObject(result, tag);
        break;
    }
    case SCRIPT_EXT_FirstSprite:
        {
        const STRING tag("return FirstSprite");
        SPRITE_COLLECTOR* const collector = ::Sprites;
        scriptSpriteIterator = collector->allSprites.m_count;
        int* const cursor = &scriptSpriteIterator;
        SPRITE* const result = collector->allSprites.NextIterateNon0(cursor);
        stack->PushObject(result, tag);
        break;
    }
    case SCRIPT_EXT_NextSprite:
        {
        const STRING tag("return NextSprite");
        SPRITE_COLLECTOR* const collector = ::Sprites;
        int* const cursor = &scriptSpriteIterator;
        SPRITE* const result = collector->allSprites.NextIterateNon0(cursor);
        stack->PushObject(result, tag);
        break;
    }
    case SCRIPT_EXT_Action:
        {
        const unsigned int var3 = static_cast<unsigned int>(PopInt());
        const unsigned int var2 = static_cast<unsigned int>(PopInt());
        const unsigned int var1 = static_cast<unsigned int>(PopInt());
        const unsigned int action = static_cast<unsigned int>(PopInt());
        SPRITE* const unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        void* const cursorObject = Mouse;
        if (!unit || unit == cursorObject)
            {
            stack->PushInt(0);
            break;
        }
        if (static_cast<int>(action)<17)
            {
            unit->ChangeAnimation(static_cast<int>(action));
            stack->PushInt(0);
            break;
        }
        if (action == ACT_MOVE || action == ACT_ATTACK || action == ACT_PATROL || action == ACT_MOVE_TO ||
           action == ACT_LINK_ENGINE || action == ACT_CLASH_ENGINE)
            {
            VID* const owner = unit->Vid();
            if (owner->spriteClass == 21u && (unit->RuntimeFlags()&0x7Cu) == 0x68u)
                {
                stack->PushInt(0);
                break;
            }
        }
        const unsigned int result = static_cast<unsigned int>(unit->Action(static_cast<int>(action), static_cast<int>(var1), static_cast<int>(var2), static_cast<int>(var3)));
        if (action == ACT_GET_TEXT_DESC)
            {
            stack->PushStr(
                *reinterpret_cast<const STRING*>(U32ToPtr(result)));
            break;
        }
        if (result != 0u && (action == ACT_GET_GOAL || action == ACT_NEXT_ENGINE || action == ACT_LAST_ENGINE || action == ACT_FIRST_ENGINE ||
           action == ACT_GET_LINK || action == ACT_GET_UPLINK))
            {
            const STRING tag("return Action");
            stack->PushObject(U32ToPtr(result), tag);
            break;
        }
        stack->PushInt(result);
        break;
    }
    case SCRIPT_EXT_SizeTo:
        {
        const int y = PopInt();
        const int x = PopInt();
        SPRITE* const unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        if (!unit)
            {
            stack->PushInt(60000);
            break;
        }
        const VECTOR2 target(static_cast<float>(x), static_cast<float>(y));
        const VECTOR2 delta = target-*reinterpret_cast<const VECTOR2*>(&unit->Coor());
        const float distance = delta.NearLength();
        stack->PushInt(TruncateFloatToInt32(distance));
        break;
    }
    case SCRIPT_EXT_AddCommand:
        {
        const unsigned int var3 = static_cast<unsigned int>(PopInt());
        const unsigned int var2 = static_cast<unsigned int>(PopInt());
        const unsigned int var1 = static_cast<unsigned int>(PopInt());
        const unsigned int action = static_cast<unsigned int>(PopInt());
        SPRITE* const unit = reinterpret_cast<SPRITE*>(
            U32ToPtr(static_cast<unsigned int>(PopInt())));
        if (unit) unit->AddActionAfterStop(0u, action, var1, var2, var3);
        break;
    }
    case SCRIPT_EXT_GetVid:
        {
        SPRITE* unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        unsigned int nvid = 0u;
        if (unit)
            {
            VID* const owner = unit->Vid();
            nvid = static_cast<unsigned int>(owner->nVid);
        }
        stack->PushInt(nvid);
        break;
    }
    case SCRIPT_EXT_Destroy:
        {
        SPRITE* unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        void* const invalid = Mouse;
        if (unit && unit != invalid)
            delete unit;
        break;
    }
    case SCRIPT_EXT_GetX:
        {
        SPRITE* unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        const int x = unit?TruncateFloatToInt32(unit->X()):0;
        stack->PushInt(x);
        break;
    }
    case SCRIPT_EXT_GetY:
        {
        SPRITE* unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        const int y = unit?TruncateFloatToInt32(unit->Y()):0;
        stack->PushInt(y);
        break;
    }
    case SCRIPT_EXT_GetZ:
        {
        SPRITE* unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        const int z = unit?TruncateFloatToInt32(unit->Z()):0;
        stack->PushInt(z);
        break;
    }
    case SCRIPT_EXT_GetDirection:
        {
        SPRITE* unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        const unsigned int direction = unit?static_cast<unsigned int>(unit->Direction().value):0u;
        stack->PushInt(direction);
        break;
    }
    case SCRIPT_EXT_GetAnimation:
        {
        SPRITE* unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        const unsigned int animation = unit?static_cast<unsigned int>(unit->Animation()):0u;
        stack->PushInt(animation);
        break;
    }
    case SCRIPT_EXT_DirectionTo:
        {
        const int y = PopInt();
        const int x = PopInt();
        SPRITE* const unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        if (!unit)
            {
            stack->PushInt(0);
            break;
        }
        const VECTOR2 target = {static_cast<float>(x), static_cast<float>(y)};
        VECTOR2 delta;
        delta = target - *reinterpret_cast<const VECTOR2*>(&unit->Coor());
        const ANGLE direction(delta);
        stack->PushInt(static_cast<unsigned int>(direction.value));
        break;
    }
    case SCRIPT_EXT_GetCommands:
        {
        SPRITE* const unit = reinterpret_cast<SPRITE*>(
            U32ToPtr(static_cast<unsigned int>(PopInt())));
        if (!unit)
            {
            const STRING empty(StringGlobals::EmptyText());
            stack->PushStr(empty);
            break;
        }
        const STRING actions = unit->GetTextActions(0);
        const STRING items = unit->GetTextItems();
        const STRING combined = items+actions;
        stack->PushStr(combined);
        break;
    }
    case SCRIPT_EXT_SetCommands:
        {
        STRING* const scratch = externCommandText.Address();
        *scratch = PopStr();
        SPRITE* const unit = reinterpret_cast<SPRITE*>(
            U32ToPtr(static_cast<unsigned int>(PopInt())));
        if (!unit) break;
        unit->SetTextItems(*scratch);
        const char* const commandSeparator = "\002";
        if (!ScriptExternCStringEqual(scratch->m_value, commandSeparator))
            {
            STRING actions = scratch->After(commandSeparator);
            *scratch = actions;
        }
        unit->SetTextActions(0, *scratch);
        break;
    }
    case SCRIPT_EXT_MenuSpriteUnderCursor:
        {
        stack->PushInt(PtrToU32(m_menu.SelectedSprite()));
        break;
    }
    case SCRIPT_EXT_MenuFind:
        {
        const int screenY = PopInt();
        const int screenX = PopInt();
        const int requestedDirection = PopInt();
        const int nvid = PopInt();
        VID* const resource = static_cast<VID*>(Vid(nvid));
        if (resource == ::EmptyVid)
        {
            MYERROR* const sink = ::Error;
            if (sink && g_iZmDebug == 0u)
                sink->Log("!!!ERROR!!!SCRIPT: Invalid nvid %s %i", "for MenuFind", nvid);
        }
        SPRITE* best = 0;
        if (resource != ::EmptyVid)
        {
            unsigned int activeFrames = 0u;
            for (unsigned int lane = 0u;lane<8u;++lane)activeFrames += resource->spriteCountsByPlayer[lane];
            if (activeFrames != 0u)
            {
                const float targetX = static_cast<float>(screenX);
                const float targetY = static_cast<float>(screenY);
                for (int i = 0;i<m_menu.m_count;++i)
                {
                    SPRITE* const candidate = m_menu.m_data[i];
                    if (candidate->Vid() != resource)continue;
                    if (requestedDirection != 999999 &&
                       candidate->Vid()->RealDirection(candidate->Direction().value) != requestedDirection)
                    {
                        if (requestedDirection != 999000+static_cast<int>(m_menu.m_data[i]->Direction().value))continue;
                    }
                    if (best)
                    {
                        const VECTOR2 candidatePosition = Map->ToScreen(m_menu.m_data[i]->Coor());
                        const double dx = ::fabs(static_cast<double>(candidatePosition.x)-targetX);
                        const double dy = ::fabs(static_cast<double>(candidatePosition.y)-targetY);
                        const float candidateDistance = static_cast<float>(dx>dy?dx+dy*0.398000002f:dy+dx*0.398000002f);
                        const VECTOR2 bestPosition = Map->ToScreen(best->Coor());
                        const double bestDX = ::fabs(static_cast<double>(bestPosition.x)-targetX);
                        const double bestDY = ::fabs(static_cast<double>(bestPosition.y)-targetY);
                        const double bestDistance = bestDX>bestDY?bestDX+bestDY*0.398000002f:bestDY+bestDX*0.398000002f;
                        if (candidateDistance >= bestDistance)continue;
                    }
                    best = m_menu.m_data[i];
                }
            }
        }
        const STRING tag("return MenuFind");
        stack->PushObject(best, tag);
        break;
    }
    case SCRIPT_EXT_MenuLoad:
        {
        STRING* fileName = const_cast<STRING*>(&PopStr());
        (void)m_menu.Load(*fileName);
        break;
    }
    case SCRIPT_EXT_MenuRelease:
        {
        STRING* const scratch = externCommandText.Address();
        *scratch = PopStr();
        const char* const emptyName = StringGlobals::EmptyText();
        MENU* const menu = &m_menu;
        if (ScriptExternCStringEqual(scratch->value, emptyName))
            menu->DeleteAll();
        else
            (void)menu->DeleteFromFile(*scratch);
        break;
    }
    case SCRIPT_EXT_MenuNVidUnderCursor:
        {
        stack->PushInt(m_menu.NVidUnderCursor());
        break;
    }
    case SCRIPT_EXT_MenuNDirUnderCursor:
        {
        stack->PushInt(m_menu.NDirUnderCursor());
        break;
    }
    case SCRIPT_EXT_MenuAction:
        {
        const unsigned int var3 = static_cast<unsigned int>(PopInt());
        const unsigned int var2 = static_cast<unsigned int>(PopInt());
        const unsigned int var1 = static_cast<unsigned int>(PopInt());
        const unsigned int action = static_cast<unsigned int>(PopInt());
        const int requestedDirection = PopInt();
        const int nvid = PopInt();
        VID* const resource = Vid(nvid);
        if (resource == ::EmptyVid)
            {
            MYERROR* const sink = ::Error;
            if (sink && g_iZmDebug == 0u)
                sink->Log("!!!ERROR!!!SCRIPT: Invalid nvid %s %i", "for MenuAction", nvid);
        }
        if (resource != ::EmptyVid)
            {
            for (int i = 0;i<m_menu.m_count;++i)
                {
                SPRITE* const child = m_menu.m_data[i];
                if (child->Vid() != resource)continue;
                if (requestedDirection != 999999 &&
                   child->Vid()->RealDirection(child->Direction().value) != requestedDirection)
                    {
                    if (requestedDirection != 999000+static_cast<int>(m_menu.m_data[i]->Direction().value))continue;
                }
                if (static_cast<int>(action)<17)
                    (void)m_menu.m_data[i]->ChangeAnimation(static_cast<int>(action));
                else
                    (void)m_menu.m_data[i]->Action(static_cast<int>(action), static_cast<int>(var1), static_cast<int>(var2), static_cast<int>(var3));
            }
        }
        break;
    }
    case SCRIPT_EXT_MenuCreate:
        {
        const int z = PopInt();
        const int y = PopInt();
        const int yPlusZ = BitsToSigned32(static_cast<unsigned int>(y)+static_cast<unsigned int>(z));
        const int x = PopInt();
        const int requestedDirection = PopInt();
        const int nvid = PopInt();
        VID* const resource = Vid(nvid);
        if (resource == ::EmptyVid)
            {
            MYERROR* const sink = ::Error;
            if (sink && g_iZmDebug == 0u)
                sink->Log("!!!ERROR!!!SCRIPT: Invalid nvid %s %i", "for MenuCreate", nvid);
        }
        if (resource == ::EmptyVid)
            {
            stack->PushInt(0);
            break;
        }
        const STRING tag("return MenuCreate");
        const int shiftedDirection = BitsToSigned32(static_cast<unsigned int>(requestedDirection)<<8);
        const ANGLE direction(static_cast<unsigned char>(shiftedDirection/resource->noDir));
        const float positionZ = static_cast<float>(z);
        const float positionY = static_cast<float>(yPlusZ);
        const float positionX = static_cast<float>(x);
        const VECTOR position(positionX, positionY, positionZ);
        SPRITE* const created = Map->CreateSprite(resource, position, direction, 0);
        stack->PushObject(created, tag);
        break;
    }
    case SCRIPT_EXT_MenuLClick:
        {
        const STRING tag("return MenuLClick");
        const unsigned int result = (m_menu.ControlFlags()&0x01u) != 0u ? PtrToU32(m_menu.SelectedSprite()) : 0u;
        stack->PushObject(U32ToPtr(result), tag);
        break;
    }
    case SCRIPT_EXT_MenuRClick:
        {
        const STRING tag("return MenuRClick");
        const unsigned int result = (m_menu.ControlFlags()&0x02u) != 0u ? PtrToU32(m_menu.SelectedSprite()) : 0u;
        stack->PushObject(U32ToPtr(result), tag);
        break;
    }
    case SCRIPT_EXT_MenuFindNamed:
        {
        const STRING tag("return MenuFindNamed");
        STRING* name = const_cast<STRING*>(&PopStr());
        const unsigned int result = PtrToU32(m_menu.SpriteWithName(*name));
        stack->PushObject(U32ToPtr(result), tag);
        break;
    }
    case SCRIPT_EXT_Load:
        {
        STRING* fileName = const_cast<STRING*>(&PopStr());
        m_flags |= 0x80u;
        m_pendingMapFileName = *fileName;
        break;
    }
    case SCRIPT_EXT_Save:
        {
        STRING* const source = const_cast<STRING*>(&PopStr());
        Save(*source);
        break;
    }
    case SCRIPT_EXT_SaveDemo:
        {
        if (m_resource.m_file == 0)
            {
            STRING* fileName = const_cast<STRING*>(&PopStr());
            (void)m_resource.OpenForWrite(*fileName, ResourceType::Demo);
        }
        break;
    }
    case SCRIPT_EXT_Exit:
        {
        (void)PopStr();
        (void)::PostMessageA(reinterpret_cast<HWND>(U32ToPtr(m_dialog)), 0x10u, 0u, 0u);
        break;
    }
    case SCRIPT_EXT_Exec:
        {
        CRC32 crc;
        crc.value = 0u;
        const STRING path(PopStr());
        const STRING parameter(PopStr());
        STRING* const commandText = externCommandText.Address();
        *commandText = PopStr();

        bool allowed = (m_flags&0x02000000u) != 0u;
        if (allowed)
            allowed = (crc.CheckListFromFile(STRING("maps\\check_list.lst")) == 0);

        MYERROR* const sink = ::Error;
        if (allowed)
            {
            sink->Log("    Exec '%s'", commandText->value);
            m_flags |= 0x00400000u;
            (void)::ShellExecuteA(0, 0, commandText->value, parameter.value, path.value, 5);
            stack->PushInt(0);
        }
            else
            {
            sink->Log("    can't Exec '%s' because check file crc error", commandText->value);
            stack->PushInt(1);
        }
        break;
    }
    case SCRIPT_EXT_Pause:
        {
        const int pause = PopInt();
        NET* const session = ::Net;
        NET* const network = session;
        const bool networkSession = (network->m_flags&0x10u) != 0u;
        if (pause != 0)
            {
            if (networkSession)
                *reinterpret_cast<unsigned char*>(&m_pendingNetworkPauseCommand) = 2u;
            else
                PauseOn();
            MOUSE* const cursor = Mouse;
            cursor->ChangeAnimation(0);
            (void)::Sound->PauseSpeech();
        }
            else
            {
            if (networkSession)
                *reinterpret_cast<unsigned char*>(&m_pendingNetworkPauseCommand) = 1u;
            else
                PauseOff();
            (void)::Sound->ResumeSpeech();
        }
        break;
    }
    case SCRIPT_EXT_MessageText:
        {
        const int y = PopInt();
        const int x = PopInt();
        STRING* text = const_cast<STRING*>(&PopStr());
        const unsigned int playerIndex = m_playerIndex;
        PLAYER* const player = m_players[playerIndex];
        player->PutMessage(*text, static_cast<float>(x), static_cast<float>(y));
        break;
    }
    case SCRIPT_EXT_SetScrollType:
        {
        const int y1 = PopInt();
        const int x1 = PopInt();
        const int y0 = PopInt();
        const int x0 = PopInt();
        const VECTOR2 second(static_cast<float>(x1), static_cast<float>(y1));
        const VECTOR2 first(static_cast<float>(x0), static_cast<float>(y0));
        const int type = PopInt();
        SetScrollType(type, first, second);
        break;
    }
    case SCRIPT_EXT_SetScrollBox:
        {
        const int y1 = PopInt();
        const int x1 = PopInt();
        const int y0 = PopInt();
        const int x0 = PopInt();
        const float right = static_cast<float>(x1);
        const float bottom = static_cast<float>(y1);
        const float left = static_cast<float>(x0);
        const float top = static_cast<float>(y0);
        m_cameraMaxY = bottom;
        m_cameraMinX = left;
        m_cameraMinY = top;
        m_cameraMaxX = right;
        break;
    }
    case SCRIPT_EXT_SetStateBar:
        {
        const int flag = PopInt();
        const unsigned int playerIndex = m_playerIndex;
        PLAYER* const player = m_players[playerIndex];
        if (flag != 0) player->StateBarOn();
        else player->StateBarOff();
        break;
    }
    case SCRIPT_EXT_GetPreviousMapName:
        {
        stack->PushStr(
            m_previousMapFileName);
        break;
    }
    case SCRIPT_EXT_GetASProtectUserName:
        {
        const char* const cached = registrationInformation;
        if (cached && *cached)
            {
            const STRING result(cached);
            stack->PushStr(result);
        }
            else
            {
            char value[256];
            (void)::GetEnvironmentVariableA("TYPE", value, 256u);
            value[255] = 0;
            (void)CrtLowercase(PtrToU32(value));
            const char* const accepted = "purchased";
            const char* const text = ScriptExternCStringEqual(value, accepted) ? accepted :
                StringGlobals::EmptyText();
            const STRING result(text);
            stack->PushStr(result);
        }
        break;
    }
    case SCRIPT_EXT_GetMapName:
        {
        const STRING mapName = FileName();
        PushStr(mapName);
        break;
    }
    case SCRIPT_EXT_SetPosition:
        {
        const int y = PopInt();
        const int x = PopInt();
        const VECTOR2 position(static_cast<float>(x), static_cast<float>(y));
        SetShiftCoor(position, 0);
        break;
    }
    case SCRIPT_EXT_GetScrollType:
        {
        stack->PushInt(m_scrollMode);
        break;
    }
    case SCRIPT_EXT_SetLocale:
        {
        STRING* codePage = const_cast<STRING*>(&PopStr());
        SetKeyboardLocale(*codePage);
        break;
    }
    case SCRIPT_EXT_SetInputType:
        {
        const int type = PopInt();
        m_input.SetInputType(type);
        break;
    }
    case SCRIPT_EXT_GetInputAction:
        {
        int playerIndex = PopInt();
        const int inputAction = PopInt();
        NET* const session = ::Net;
        unsigned int result = 0u;
        if ((session->m_flags&0x10u) != 0u)
            {
            if (playerIndex<0 || playerIndex >= 8)
                playerIndex = static_cast<int>(m_playerIndex);
            PLAYER* const army = m_players[playerIndex];
            result = army->m_input.GetAction(static_cast<unsigned int>(inputAction));
        }
            else
            {
            result = m_input.GetAction(static_cast<unsigned int>(inputAction));
        }
        stack->PushInt(result);
        break;
    }
    case SCRIPT_EXT_SetInputActionMode:
        {
        const int num = PopInt();
        const int inputAction = PopInt();
        zInputMap::m_bKeyWaitAct = static_cast<unsigned int>(inputAction);
        zInputMap::m_iKeyWaitNum = num;
        break;
    }
    case SCRIPT_EXT_SetCursor:
        {
        ScriptExternSetCursor(PopInt());
        break;
    }
    case SCRIPT_EXT_RemoveInputAction:
        {
        int playerIndex = PopInt();
        const int inputAction = PopInt();
        NET* session = ::Net;
        if ((session->m_flags&0x10u) != 0u)
            {
            if (playerIndex<0 || playerIndex >= 8)
                playerIndex = static_cast<int>(m_playerIndex);
            PLAYER* const army = m_players[playerIndex];
            army->m_input.RemoveEvent(static_cast<unsigned int>(inputAction), 0u);
        }
            else
            {
            m_input.RemoveEvent(static_cast<unsigned int>(inputAction), 0u);
        }
        break;
    }
    case SCRIPT_EXT_SetCursorPosition:
        {
        const int screenY = PopInt();
        const int screenX = PopInt();
        MOUSE* const cursor = Mouse;
        (void)cursor->Action(ACT_CHANGE_COOR, screenX, screenY, 0);
        break;
    }
    case SCRIPT_EXT_SaveInput:
        {
        STRING* fileName = const_cast<STRING*>(&PopStr());
        (void)zInputMap::Save(fileName->CharPtr());
        break;
    }
    case SCRIPT_EXT_LoadInput:
        {
        STRING* fileName = const_cast<STRING*>(&PopStr());
        (void)zInputMap::Load(fileName->CharPtr());
        break;
    }
    case SCRIPT_EXT_SetSpeechVolume:
        {
        ::Sound->VolumeSpeech(PopInt());
        break;
    }
    case SCRIPT_EXT_SetSoundVolume:
        {
        ::Sound->VolumeSound(PopInt());
        break;
    }
    case SCRIPT_EXT_SetMusicVolume:
        {
        ::Sound->VolumeMusic(PopInt());
        break;
    }
    case SCRIPT_EXT_PlaySFX:
        {
        ::Sound->PlaySFX(PopInt(), 0, 0, 1.0f);
        break;
    }
    case SCRIPT_EXT_StopSFX:
        {
        ::Sound->StopSFX(PopInt());
        break;
    }
    case SCRIPT_EXT_StopMusic:
        {
        ::Sound->StopMusic(static_cast<unsigned int>(PopInt()));
        break;
    }
    case SCRIPT_EXT_PlaySFXFromCoor:
        {
        const int y = PopInt();
        const int x = PopInt();
        const GRAPH* const graphOwner = Graph;
        const float half = (0.5f);
        const VECTOR2 centered = {static_cast<float>(x)-graphOwner->screenWidth*half,
                                static_cast<float>(y)-graphOwner->screenHeight*half};
        VECTOR2 relative;
        relative = centered-m_shiftXY;
        const int nSfx = PopInt();
        ::Sound->PlaySFXFromCoor(nSfx, relative, 1.0f);
        break;
    }
    case SCRIPT_EXT_PlayMusicFile:
        {
        const int changeTime = PopInt();
        const int loop = PopInt();
        STRING* fileName = const_cast<STRING*>(&PopStr());
        ::Sound->FadeAndPlayFile(*fileName, loop, static_cast<unsigned long>(changeTime));
        break;
    }
    case SCRIPT_EXT_Effect:
        {
        const int duration = PopInt();
        const int var2 = PopInt();
        const int var1 = PopInt();
        const int effect = PopInt();
        Graph->Effect(effect, var1, var2, duration);
        break;
    }
    case SCRIPT_EXT_SetEnvironment:
        {
        Graph->SetEnvironment(static_cast<unsigned int>(PopInt()));
        break;
    }
    case SCRIPT_EXT_SetGraphDetail:
        {
        stack->PushInt(Graph->SetGraphDetail(PopInt()));
        break;
    }
    case SCRIPT_EXT_SetGamma:
        {
        const unsigned int packed = static_cast<unsigned int>(PopInt());
        const GAMMA gamma(static_cast<GAMMA::GAMMA_CREATE>(0), packed);
        Graph->SetGamma(gamma);
        break;
    }
    case SCRIPT_EXT_SetWind:
        {
        const unsigned int direction = static_cast<unsigned int>(static_cast<unsigned char>(PopInt()));
        const int wind = PopInt();
        Graph->SetWind(wind, ANGLE(static_cast<unsigned char>(direction)));
        break;
    }
    case SCRIPT_EXT_PlayMovie:
        {
        Graph->PlayMovie(PopStr());
        break;
    }
    case SCRIPT_EXT_IsPlayMovie:
        {
        stack->PushInt((Graph->movie.graphBuilder != 0));
        break;
    }
    case SCRIPT_EXT_StopMovie:
        {
        Graph->movie.Release();
        break;
    }
    case SCRIPT_EXT_IsPlayMusic:
        {
        stack->PushInt(ScriptExternIsPlayMusic());
        break;
    }
    case SCRIPT_EXT_CountGamma:
        {
        const int time = PopInt();
        const unsigned int gamma2 = static_cast<unsigned int>(PopInt());
        const unsigned int gamma1 = static_cast<unsigned int>(PopInt());
        stack->PushInt(InterpolatePackedRGB(gamma1, gamma2, time));
        break;
    }
    case SCRIPT_EXT_GetGamma:
        {
        stack->PushInt(ScriptExternGetGamma());
        break;
    }
    case SCRIPT_EXT_GetEffectState:
        {
        stack->PushInt(Graph->GetEffectState(PopInt()));
        break;
    }
    case SCRIPT_EXT_ScreenX:
        {
        const GRAPH* const graph = Graph;
        stack->PushInt(TruncateFloatToInt32(graph->screenWidth));
        break;
    }
    case SCRIPT_EXT_ScreenY:
        {
        const GRAPH* const graph = Graph;
        stack->PushInt(TruncateFloatToInt32(graph->screenHeight));
        break;
    }
    case SCRIPT_EXT_SetGraphScreen:
        {
        const int sizeY = PopInt();
        const int sizeX = PopInt();
        GRAPH_SetScreenSize(Graph, sizeX, sizeY);
        break;
    }
    case SCRIPT_EXT_GetGraphDetail:
        {
        const GRAPH* const graph = Graph;
        PushInt(graph->graphDetail);
        break;
    }
    case SCRIPT_EXT_PlaySpeech:
        {
        const STRING& source = PopStr();
        (void)::Sound->PlaySpeech(source);
        break;
    }
    case SCRIPT_EXT_StopSpeech:
        {
        ::Sound->StopSpeech();
        break;
    }
    case SCRIPT_EXT_Log:
        {
        STRING* const source = const_cast<STRING*>(&PopStr());
        STRING formatted = "    "+(*source);

        MYERROR* const logger = ::Error;
        logger->Log(STRING(formatted.CharPtr()));
        break;
    }
    case SCRIPT_EXT_Random_:
        {
        stack->PushInt(Random(PopInt()));
        break;
    }
    case SCRIPT_EXT_ChangeZUnit:
        {
        const int z = PopInt();
        VID* const owner = PopVid("for ChangeZUnit");
        VID* const invalid = ::EmptyVid;
        if (owner != invalid)
            {
            SPRITE_COLLECTOR* const manager = ::Sprites;
            int cursor = 0;
            SPRITE* unit = manager->FirstActiveSprite(&cursor);
            while (unit != 0)
                {
                if (unit->Vid() == owner)
                    unit->ChangeZCoor(static_cast<float>(z));
                unit = manager->NextActiveSprite(&cursor);
            }
        }
        break;
    }
    case SCRIPT_EXT_GetTime:
        {
        PushInt(::CurrentTime);
        break;
    }
    case SCRIPT_EXT_GetGroundZ:
        {
        const int y = PopInt();
        const int x = PopInt();
        const VECTOR2 point = {static_cast<float>(x), static_cast<float>(y)};
        const float height = GetGroundZ(point);
        stack->PushInt(TruncateFloatToInt32(height));
        break;
    }
    case SCRIPT_EXT_CanPlace:
        {
        const int z = PopInt();
        const int y = PopInt();
        const int x = PopInt();
        const VID* const owner = PopVid("for CanPlace");
        const STRING tag("return CanPlace");
        const ANGLE direction(0);
        SPRITE_COLLECTOR* const collector = ::Sprites;
        SPRITE* const result = collector->CanPlace(owner, static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(z), direction);
        PushSprite(result, tag);
        break;
    }
    case SCRIPT_EXT_GetVidData:
        {
        const int type = PopInt();
        const char* const diagnostic = (type == 0)
            ? StringGlobals::EmptyText()
            : "for GetVid";
        VID* const object = PopVid(diagnostic);
        VID* const invalid = ::EmptyVid;
        if (object == invalid)
            {
            stack->PushInt(0);
            break;
        }
        const VID* const vid = static_cast<const VID*>(object);

        if (type >= 160 && type<177)
            {
            VID* const child = vid->childVid[type-160];
            stack->PushInt(child ? child->nVid : 0);
            break;
        }
        if (type >= 192 && type<209)
            {
            stack->PushInt(vid->noChild[type-192]);
            break;
        }

        switch (type)
            {
        case 0:

            stack->PushInt(1);
            break;
        case 3:
                {
            WEAPON_RUNTIME* const runtime = static_cast<VID*>(object)->weapon;
            stack->PushInt(TruncateFloatToInt32(runtime->BattleRange));
            break;
        }
        case 7:
            PushStr(vid->name);
            break;
        case 29:
            stack->PushInt(static_cast<const VID*>(object)->Killed());
            break;
        case 30:
            case 31: case 32: case 33: case 34: case 35: case 36: case 37:
            stack->PushInt(
                static_cast<const VID*>(object)->Killed(static_cast<unsigned int>(type-30)));
            break;
        case 49:
            stack->PushInt(static_cast<const VID*>(object)->NoSprites());
            break;
        case 50:
            case 51: case 52: case 53: case 54: case 55: case 56: case 57:
            stack->PushInt(
                static_cast<VID*>(object)->NoSprites(type-50));
            break;
        case 69:
            stack->PushInt(vid->maxHp);
            break;
        case 70:
            case 71: case 72: case 73: case 74: case 75: case 76: case 77:
            stack->PushInt(
                static_cast<const VID*>(object)->GetMaxHp(static_cast<unsigned int>(type-70)));
            break;
        case 110:
            stack->PushInt(vid->spriteType);
            break;
        case 111:
            stack->PushInt(vid->spriteClass);
            break;
        case 112:
            case 114:
                {
            const float value = (type == 112)?vid->maxSpeed:vid->maxZSpeed;
            if (FloatBitsU32(value) == 0x497423F0u) stack->PushInt(999999);
            else stack->PushInt(TruncateFloatToInt32(value*1000.0f));
            break;
        }
        case 113:
            stack->PushInt(vid->lifeTime);
            break;
        case 117:
            stack->PushInt(vid->noDir);
            break;
        case 118:
            stack->PushInt(vid->moveMask);
            break;
        case 119:
                {
            WEAPON_RUNTIME* const runtime = static_cast<VID*>(object)->weapon;
            stack->PushInt(runtime->BuildTime);
            break;
        }
        case 120:
            stack->PushInt(
                static_cast<const VID*>(object)->PropHide());
            break;
        case 121:
            stack->PushInt(
                static_cast<const VID*>(object)->PropNotCreateAsChild());
            break;
        case 122:
            stack->PushInt(vid->frameSpeedDefault);
            break;
        case 123:
                {
            stack->PushInt(vid->linkVid ? vid->linkVid->nVid : 0);
            break;
        }
        case 124:
            stack->PushInt(vid->deathDamageMin);
            break;
        case 125:
            stack->PushInt(vid->deathDamageMax);
            break;
        case 126:
            case 127:
                {
            WEAPON_RUNTIME* const runtime = static_cast<VID*>(object)->weapon;
            const float parameter = (type == 126)?runtime->aimRange:runtime->DetectRange;
            stack->PushInt(TruncateFloatToInt32(parameter));
            break;
        }
        case 128:
            stack->PushInt(static_cast<const VID*>(object)->GetMaxAmmo());
            break;
        case 132:
            stack->PushInt((vid->runtimeFlags>>12u)&1u);
            break;
        case 133:
            const_cast<VID*>(vid)->SetFramesToDevice();
            stack->PushInt(1);
            break;
        case 229:
            stack->PushInt(static_cast<const VID*>(object)->ReCaptured());
            break;
        case 230:
            case 231: case 232: case 233: case 234: case 235: case 236: case 237:
            stack->PushInt(
                static_cast<const VID*>(object)->ReCaptured(static_cast<unsigned int>(type-230)));
            break;
        case 238:
                {
            stack->PushInt(vid->sourceVid ? vid->sourceVid->nVid : 0);
            break;
        }
        case 239:
            case 240: case 241:
                {
            const float value = (type == 239)?vid->sizeXYZ.x:(type == 240)?vid->sizeXYZ.y:vid->sizeXYZ.z;
            stack->PushInt(TruncateFloatToInt32(value));
            break;
        }
        default:
                {
            STRING label;
            label = "GetVid type";
            Error(E_UNKNOWN, label, static_cast<unsigned int>(type));
            break;
        }
        }
        break;
    }
    case SCRIPT_EXT_SetVidData:
        {
        const int value = PopInt();
        const int type = PopInt();
        VID* const object = PopVid(
            "for SetVid");
        VID* const invalid = ::EmptyVid;
        if (object == invalid) break;
        VID* const vid = static_cast<VID*>(object);

        switch (type)
            {
        case 29:
            for (unsigned int i = 0;i<4u;++i) vid->killedByPlayer[i] = value;
            break;
        case 30:
            case 31: case 32: case 33: case 34: case 35: case 36: case 37:
            vid->killedByPlayer[type-30] = value;
            break;
        case 69:
            vid->maxHp = value;
            break;
        case 70:
            case 71: case 72: case 73: case 74: case 75: case 76: case 77:
            static_cast<VID*>(object)->SetMaxHp(static_cast<unsigned int>(type-70), value);
            break;
        case 90:
            case 91: case 92: case 93: case 94: case 95: case 96: case 97:
            static_cast<VID*>(object)->SetHpCoeff(static_cast<unsigned int>(type-90), value);
            break;
        case 112:
                {
            const float scale = (value == 999999)
                ? (999999.0f)
                : static_cast<float>(value)*(0.00100000005f);
            vid->maxSpeed = scale;
            int cursor = 0;
            SPRITE* unit = world->FirstActiveSprite(&cursor);
            while (unit)
                    {
                if (unit->Vid() == object)
                        {
                    EX_SPRITE_DATA* const linked = unit->ExData();
                    if (linked)
                            {
                        EX_SPRITE_DATA* const linkedAgain = unit->ExData();
                        linkedAgain->maxSpeedOverride = vid->maxSpeed;
                    }
                }
                unit = world->NextActiveSprite(&cursor);
            }
            break;
        }
        case 113:
            if (vid->nVid != 0)
                {
                vid->lifeTime = value;
                vid->actionAuxStateRequired = 1;
            }
            break;
        case 114:
                {
            const float scale = (value == 999999)
                ? (999999.0f)
                : static_cast<float>(value)*(0.00100000005f);
            vid->maxZSpeed = scale;
            break;
        }
        case 116:
                {
            if (ValidateVid(value) != 0u)
                    {
                VID* const source = Vid(value);
                ExchangeVid(static_cast<VID*>(object), source);
            }
                    else
                    {
                STRING label;
                label = "SetVid exchangevid";
                Error(E_INVALID, label, static_cast<unsigned int>(value));
            }
            break;
        }
        case 118:
            vid->moveMask = static_cast<unsigned int>(value);
            break;
        case 119:
                {
            WEAPON_RUNTIME* const runtime = static_cast<VID*>(object)->weapon;
            runtime->BuildTime = static_cast<unsigned int>(value);
            break;
        }
        case 120:
            static_cast<VID*>(object)->SetPropHideWithoutLink(value);
            break;
        case 121:
            static_cast<VID*>(object)->SetPropNotCreateAsChild(value);
            break;
        case 122:
            vid->frameSpeedDefault = static_cast<unsigned short>(value);
            for (unsigned int i = 0;i<VID::AnimationSlotCount;++i) vid->frameSpeed[i] = value;
            break;
        case 123:
            if (ValidateVid(value) != 0u)
                vid->linkVid = Vid(value);
            else vid->linkVid = 0;
            break;
        case 124:
            vid->deathDamageMin = value;
            break;
        case 125:
            vid->deathDamageMax = value;
            break;
        case 126:
                {
            WEAPON_RUNTIME* const runtime = static_cast<VID*>(object)->weapon;
            runtime->aimRange = static_cast<float>(value);
            break;
        }
        case 127:
                {
            WEAPON_RUNTIME* const runtime = static_cast<VID*>(object)->weapon;
            runtime->DetectRange = static_cast<float>(value);
            break;
        }
        case 128:
                {
            VID* const vidObject = static_cast<VID*>(object);
            WEAPON_RUNTIME* runtime = vidObject->weapon;
            VID* const linked = vidObject->linkVid;
            if (linked && linked->CanFight() != 0) runtime = linked->weapon;
            runtime->MaxAmmo = value;
            break;
        }
        case 229:
                {
            COLOR color;
            color.value = static_cast<unsigned int>(value);
            vid->SetReColorForArmy(color);
            break;
        }
        case 238:
            if (ValidateVid(value) != 0u)
                {
                VID* const source = Vid(value);
                ExchangeVid(static_cast<VID*>(object), source);
            }
                else
                {
                STRING label;
                label = "SetVid copy_vid";
                Error(E_INVALID, label, static_cast<unsigned int>(value));
            }
            break;
        case 239:
            vid->sizeXYZ.x = static_cast<float>(value);
            break;
        case 240:
            vid->sizeXYZ.y = static_cast<float>(value);
            break;
        case 241:
            vid->sizeXYZ.z = static_cast<float>(value);
            break;
        default:
                {
            if (type >= 160 && type<177)
                    {
                const unsigned int id = (value<0)?static_cast<unsigned int>(-value):static_cast<unsigned int>(value);
                const unsigned int slot = static_cast<unsigned int>(type-160);
                if (value == 0)
                        {
                    vid->nChildVid[slot] = 0;
                    vid->childVid[slot] = 0;
                    break;
                }
                if (ValidateVid((int)id) != 0u)
                        {
                    vid->nChildVid[slot] = value;
                    VID* const child = Vid((int)id);
                    vid->childVid[slot] = child;
                    if (child->PropBirthAsSmoke() != 0)
                        vid->actionAuxStateRequired |= 1u;
                }
                        else
                        {
                    STRING label;
                    label = "SetVid child";
                    Error(E_INVALID, label, static_cast<unsigned int>(value));
                }
                break;
            }
            if (type >= 10 && type<18)
                    {
                const GAMMA gamma(static_cast<GAMMA::GAMMA_CREATE>(0), static_cast<unsigned long>(value));
                vid->SetGamma(gamma, type-10);
                break;
            }
            if (type >= 192 && type<209)
                    {
                vid->noChild[type-192] = value;
                break;
            }
            STRING label;
            label = "SetVid type";
            Error(E_UNKNOWN, label, static_cast<unsigned int>(type));
            break;
        }
        }
        break;
    }
    case SCRIPT_EXT_Sin:
        {
        stack->PushInt(ScriptExternTrig1024(PopInt(), false));
        break;
    }
    case SCRIPT_EXT_Cos:
        {
        stack->PushInt(ScriptExternTrig1024(PopInt(), true));
        break;
    }
    case SCRIPT_EXT_MapSizeX:
        {
        const VECTOR2& mapSize = SizeXY();
        PushInt(TruncateFloatToInt32(mapSize.x));
        break;
    }
    case SCRIPT_EXT_MapSizeY:
        {
        const VECTOR2& mapSize = SizeXY();
        PushInt(TruncateFloatToInt32(mapSize.y));
        break;
    }
    case SCRIPT_EXT_Genocide:
        {
        VID* const owner = PopVid("for Genocide");
        VID* const invalid = ::EmptyVid;
        if (owner != invalid)
            {
            SPRITE_COLLECTOR* const manager = ::Sprites;
            int cursor = 0;
            SPRITE* unit = manager->FirstActiveSprite(&cursor);
            while (unit != 0)
                {
                if (unit->Vid() == owner)
                    delete unit;
                unit = manager->NextActiveSprite(&cursor);
            }
        }
        break;
    }
    case SCRIPT_EXT_ReplaceUnit:
        {
        VID* const replacementOwner = PopVid(
            "for Replace Unit 2");
        VID* const sourceOwner = PopVid(
            "for Replace Unit 1");
        VID* const invalid = ::EmptyVid;
        if (replacementOwner == invalid || sourceOwner == invalid)break;
        SPRITE_COLLECTOR* const manager = ::Sprites;
        int cursor = 0;
        SPRITE* unit = manager->FirstActiveSprite(&cursor);
        while (unit)
            {
            if (unit->Vid() == sourceOwner)
                {
                const unsigned char direction = unit->Direction().value;
                const VECTOR& position = unit->Coor();
                (void)this->CreateSprite(static_cast<VID*>(replacementOwner), position, ANGLE(static_cast<unsigned char>(direction)), 0);
                delete unit;
            }
            unit = manager->NextActiveSprite(&cursor);
        }
        break;
    }
    case SCRIPT_EXT_ReloadVid:
        {
        ReloadVid();
        break;
    }
    case SCRIPT_EXT_ToScreenX:
        {
        const float projected = ToScreenX(static_cast<float>(PopInt()));
        stack->PushInt(TruncateFloatToInt32(projected));
        break;
    }
    case SCRIPT_EXT_ToScreenY:
        {
        const int z = PopInt();
        const int y = PopInt();
        stack->PushInt(ScriptExternToScreenY(this, y, z));
        break;
    }
    case SCRIPT_EXT_LogToConsole:
        {
        (void)Map->Player();
        ::gpLogCons->LogStr(LT_NOTICE, PopStr().CharPtr());
        break;
    }
    case SCRIPT_EXT_Random2:
        {
        stack->PushInt(Random2(PopInt()));
        break;
    }
    case SCRIPT_EXT_IsLineUnderGround:
        {
        const int z1 = PopInt();
        const int y1 = PopInt();
        const int x1 = PopInt();
        const int z0 = PopInt();
        const int y0 = PopInt();
        const int x0 = PopInt();
        const VECTOR from(x0, y0, z0);
        const VECTOR to(x1, y1, z1);
        stack->PushInt(IsLineUnderGround(from, to));
        break;
    }
    case SCRIPT_EXT_GetDateTimeString:
        {
        STRING timeText = GetTimeString();
        STRING dateText = GetDateString();
        STRING withSpace = dateText+" ";
        STRING result = withSpace+timeText;
        PushStr(result);
        break;
    }
    case SCRIPT_EXT_FWrite:
        {
        STRING* source = const_cast<STRING*>(&PopStr());
        STRING* const scratch = externCommandText.Address();
        *scratch = *source;
        const unsigned int file = static_cast<unsigned int>(PopInt());
        if (file != 0u)
            {
            scratch->Write(reinterpret_cast<std::FILE*>(U32ToPtr(file)));
            (void)static_cast<unsigned int>(std::fseek(reinterpret_cast<std::FILE*>(U32ToPtr(file)), BitsToSigned32(0xFFFFFFFFu), static_cast<int>(1u)));
            (void)static_cast<unsigned int>(std::fputs("\n", reinterpret_cast<std::FILE*>(U32ToPtr(file))));
        }
        break;
    }
    case SCRIPT_EXT_FRead:
        {
        const unsigned int file = static_cast<unsigned int>(PopInt());
        STRING* const scratch = externCommandText.Address();
        const unsigned int flags = m_flags;
        if ((flags&0x400u) != 0u)
            scratch->Read(static_cast<STREAM*>(&m_resource));
        else if (file != 0u)
            scratch->Read(reinterpret_cast<std::FILE*>(U32ToPtr(file)));
        if ((m_flags&0x200u) != 0u)
            scratch->Write(static_cast<STREAM*>(&m_resource));
        PushStr(*scratch);
        break;
    }
    case SCRIPT_EXT_FOpen:
        {
        STRING* source = const_cast<STRING*>(&PopStr());
        STRING* const scratch = externCommandText.Address();
        *scratch = *source;
        if ((m_flags&0x400u) != 0u)
            {
            PushInt(0);
            break;
        }
        STRING mode;
        mode = "rt+";
        const unsigned int file = PtrToU32(FOpen(*scratch, mode));
        if (file == 0u)Error(E_OPEN, *scratch, 0u);
        PushInt(file);
        break;
    }
    case SCRIPT_EXT_FClose:
        {
        const unsigned int file = static_cast<unsigned int>(PopInt());
        if (file != 0u)
            (void)static_cast<unsigned int>(std::fclose(reinterpret_cast<std::FILE*>(U32ToPtr(file))));
        break;
    }
    case SCRIPT_EXT_FCreate:
        {
        STRING* source = const_cast<STRING*>(&PopStr());
        STRING* const scratch = externCommandText.Address();
        *scratch = *source;
        if ((m_flags&0x400u) != 0u)
            {
            PushInt(0);
            break;
        }
        STRING mode;
        mode = "wt+";
        const unsigned int file = PtrToU32(FOpen(*scratch, mode));
        PushInt(file);
        break;
    }
    case SCRIPT_EXT_FEof:
        {
        const unsigned int file = static_cast<unsigned int>(PopInt());
        const unsigned int value = file == 0u ? 1u : (std::feof(reinterpret_cast<std::FILE*>(U32ToPtr(file)))?0x10u:0u);
        PushInt(value);
        break;
    }
    case SCRIPT_EXT_SaveFolder:
        {
        STRING folder = WindowsUserPath();
        PushStr(folder);
        break;
    }
    case SCRIPT_EXT_charat:
        {
        const int index = PopInt();
        STRING* source = const_cast<STRING*>(&PopStr());
        STRING* const scratch = externCommandText.Address();
        *scratch = *source;
        const signed char value = static_cast<signed char>((*scratch)[index]);
        stack->PushInt(static_cast<int>(value));
        break;
    }
    case SCRIPT_EXT_itoa:
        {
        const int value = PopInt();
        STRING text = Int2Str(value);
        stack->PushStr(text);
        break;
    }
    case SCRIPT_EXT_Printf:
        {
        if (stack->IsLastStackString() != 0)
            {
            STRING value;
            value = PopStr();
            STRING* format = const_cast<STRING*>(&PopStr());
            const STRING formatted = Printf(format->value, value.value);
            PushStr(formatted);
        }
            else
            {
            const int value = PopInt();
            STRING* format = const_cast<STRING*>(&PopStr());
            const STRING formatted = Printf(format->value, value);
            PushStr(formatted);
        }
        break;
    }
    case SCRIPT_EXT_strlen:
        {
        STRING* source = const_cast<STRING*>(&PopStr());
        stack->PushInt(source->Length());
        break;
    }
    case SCRIPT_EXT_ToLower:
        {
        const STRING& source = PopStr();
        const unsigned int text = PtrToU32(source.CharPtr());
        char* const converted = ::CharLowerA(reinterpret_cast<char*>(U32ToPtr(text)));
        STRING temporary;
        temporary = converted;
        PushStr(temporary);
        break;
    }
    case SCRIPT_EXT_ToUpper:
        {
        const STRING& source = PopStr();
        const unsigned int text = PtrToU32(source.CharPtr());
        char* const converted = ::CharUpperA(reinterpret_cast<char*>(U32ToPtr(text)));
        STRING temporary;
        temporary = converted;
        PushStr(temporary);
        break;
    }
    case SCRIPT_EXT_ToBase64:
        {
        const int addCode = PopInt();
        STRING* source = const_cast<STRING*>(&PopStr());
        STRING encoded = source->ToBase64(addCode);
        PushStr(encoded);
        break;
    }
    case SCRIPT_EXT_IsAlpha:
        {
        const unsigned int value = static_cast<unsigned int>(::IsCharAlphaA(static_cast<char>(PopInt())));
        PushInt(value);
        break;
    }
    case SCRIPT_EXT_GetString:
        {
        STRING* const keyword = externCommandText.Address();
        *keyword = PopStr();
        STRING fallback;
        fallback = StringGlobals::EmptyText();
        STRING* section = const_cast<STRING*>(&PopStr());
        PROFILE* strings = ::Strings;
        STRING result = strings->GetString(*section, *keyword, fallback);
        PushStr(result);
        break;
    }
    case SCRIPT_EXT_Crc32:
        {
        STRING* source = const_cast<STRING*>(&PopStr());
        STRING* const scratch = externCommandText.Address();
        *scratch = *source;
        const unsigned int length = static_cast<unsigned int>(scratch->Length());
        const CRC32 crc(scratch->value, length);
        PushInt(crc.value);
        break;
    }
    case SCRIPT_EXT_StrBefore:
        {
        STRING* delimiter = const_cast<STRING*>(&PopStr());
        STRING* const scratch = externCommandText.Address();
        *scratch = *delimiter;
        const STRING result = PopStr().Before(*scratch);
        PushStr(result);
        break;
    }
    case SCRIPT_EXT_StrAfter:
        {
        STRING* delimiter = const_cast<STRING*>(&PopStr());
        STRING* const scratch = externCommandText.Address();
        *scratch = *delimiter;
        const STRING result = PopStr().After(*scratch);
        PushStr(result);
        break;
    }
    case SCRIPT_EXT_StrSubStr:
        {
        const int end = PopInt();
        const int begin = PopInt();
        STRING* const scratch = externCommandText.Address();
        *scratch = PopStr();
        const STRING result = scratch->SubString(
            static_cast<unsigned int>(begin), static_cast<unsigned int>(end));
        PushStr(result);
        break;
    }
    case SCRIPT_EXT_GetReg:
        {
        zProfLdr* const g_pProfLdr = ::g_pProfLdr;
        if (g_pProfLdr->GetLoadState() == 0u)
            DebugAssert(4u, "g_pProfLdr->GetLoadState()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\script_exec.cpp", 880u);

        const STRING fallbackValue(PopStr());
        STRING* const valueName = externCommandText.Address();
        *valueName = PopStr();
        REGISTRY registry(PopStr());

        const unsigned int flags = m_flags;
        if ((flags&0x400u) != 0u)
            {
            valueName->Read(
                static_cast<STREAM*>(&m_resource));
        }
            else
            {
            STRING temporary = registry.GetString(*valueName, fallbackValue);
            *valueName = temporary;
        }
        if ((m_flags&0x200u) != 0u)
            valueName->Write(static_cast<STREAM*>(&m_resource));
        PushStr(*valueName);
        break;
    }
    case SCRIPT_EXT_SetReg:
        {
        if (::g_pProfLdr->GetLoadState() == 0u)
            DebugAssert(4u, "g_pProfLdr->GetLoadState()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\script_exec.cpp", 893u);

        const STRING value(PopStr());
        STRING* const name = externCommandText.Address();
        *name = PopStr();
        REGISTRY registry(PopStr());
        if ((m_flags&0x400u) == 0u)
            registry.SetString(*name, value);
        break;
    }
    case SCRIPT_EXT_DelReg:
        {
        const STRING name(PopStr());
        REGISTRY registry(PopStr());
        if ((m_flags&0x400u) == 0u)
            registry.Delete(name);
        break;
    }
    case SCRIPT_EXT_GetDefaultRegPath:
        {
        REGISTRY* registry = ::Registry;
        PushStr(registry->Path());
        break;
    }
    case SCRIPT_EXT_FirstFile:
        {
        STRING* fileMask = const_cast<STRING*>(&PopStr());
        STRING result = FFirstFile(
            &scriptFileSearch, *fileMask, 0);
        PushStr(result);
        break;
    }
    case SCRIPT_EXT_NextFile:
        {
        STRING result = FNextFile(
            &scriptFileSearch, 0);
        PushStr(result);
        break;
    }
    case SCRIPT_EXT_GetRelation:
        {
        const int army2 = PopInt();
        const int army1 = PopInt();
        PLAYER* const army = Player(army1);
        stack->PushInt(army->RelationTo(static_cast<unsigned int>(army2)));
        break;
    }
    case SCRIPT_EXT_SetRelation:
        {
        const int relation = PopInt();
        const int army2 = PopInt();
        const int army1 = PopInt();
        PLAYER* const army = Player(army1);
        army->SetRelationTo(static_cast<unsigned int>(army2), relation);
        break;
    }
    case SCRIPT_EXT_GetMoney:
        {
        const int armyIndex = PopInt();
        PLAYER* const army = Player(armyIndex);
        stack->PushInt(army->GetMoney());
        break;
    }
    case SCRIPT_EXT_SetMoney:
        {
        const int money = PopInt();
        const int armyIndex = PopInt();
        PLAYER* const army = Player(armyIndex);
        army->SetMoney(money);
        break;
    }
    case SCRIPT_EXT_SetFlagman:
        {
        const int unit = PopInt();
        const int army = PopInt();
        SetFlagman(static_cast<unsigned int>(army), reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(unit))));
        break;
    }
    case SCRIPT_EXT_SetPlayerControl:
        {
        const int controlType = PopInt();
        const int armyIndex = PopInt();
        PLAYER* const army = m_players[armyIndex];
        army->SetPlayerControl(controlType);
        break;
    }
    case SCRIPT_EXT_NetProperty:
        {

        STRING* const scratch = externCommandText.Address();
        *scratch = PopStr();
        const int type = PopInt();
        NET* const session = ::Net;

        if (type >= 50 && type <= 57)
            {
            session->SetChatColor(type-50, static_cast<unsigned long>(scratch->Int()));
        }
        if (type >= 60 && type <= 67)
            {
            stack->PushInt(
                static_cast<unsigned int>(DirectPlayQueryPeerMetric(session, type-60)));
        }
        if (type >= 30 && type <= 37)
            {
            stack->PushInt(
                session->PlayerDPNID(static_cast<unsigned int>(type-30)) != 0u ? 1u : 0u);
        }

        if (type >= 20 && type <= 29)
            {
            STRING text = session->GetPlayerName(type-20);
            PushStr(text);
            break;
        }

        switch (type)
            {
        case 1:
            stack->PushInt(session->NoPlayers());
            break;
        case 2:
                {
            STRING text = session->GetGameList(scratch->Int());
            PushStr(text);
            break;
        }
        case 4:
            stack->PushInt(session->IsLobby());
            break;
        case 5:
            stack->PushInt(session->IsConnected());
            break;
        case 6:
            stack->PushInt(session->IsHost());
            break;
        case 7:
            stack->PushInt(session->GetNoGame());
            break;
        case 8:
            stack->PushInt(PlayerIndex());
            break;
        case 9:
                {
            session->SetPlayerName(*scratch);
            stack->PushInt(0);
            break;
        }
        case 10:
                {
            session->StartHostSearch(*scratch);
            stack->PushInt(0);
            break;
        }
        case 11:
                {
            STRING text = session->GetPlayerNameForHost(scratch->Int());
            PushStr(text);
            break;
        }
        case 40:
                {
            session->SendChat(*scratch);
            break;
        }
        case 41:
                {
            STRING text = session->GetChat(scratch->Int());
            PushStr(text);
            break;
        }
        case 130:
                {
            const int hr = static_cast<int>(session->GameCreate(*scratch));
            stack->PushInt(hr<0 ? 1u : 0u);
            break;
        }
        case 131:
                {
            const int hr = static_cast<int>(session->GameJoin(scratch->Int()));
            stack->PushInt(hr<0 ? 1u : 0u);
            break;
        }
        case 132:
                {
            const int hr = static_cast<int>(session->GameJoinByName(*scratch));
            stack->PushInt(hr<0 ? 1u : 0u);
            break;
        }
        case 133:
            (void)session->GameDisconnect();
            break;
        case 134:
                {
            session->GameStart(*scratch);
            break;
        }
        case 135:
            session->GameEnd();
            break;
        case 136:
            session->NetworkOn();
            break;
        case 137:
            session->NetworkOff();
            break;
        case 138:
            session->DestroyPlayer(scratch->Int());
            break;
        default:
            break;
        }
        break;
    }
    case SCRIPT_EXT_NetSend:
        {
        STRING* const sendBuffer = externCommandText.Address();
        *sendBuffer = PopStr();
        const int type = PopInt();
        NET* const session = ::Net;
        session->SendString(type, *sendBuffer);
        break;
    }
    case SCRIPT_EXT_NetReceive:
        {
        const int player = PopInt();
        const int type = PopInt();
        NET* const session = ::Net;
        STRING received = session->ReceiveString(type, player);
        STRING* const scratch = externCommandText.Address();
        *scratch = received;
        PushStr(*scratch);
        break;
    }
    case SCRIPT_EXT_TrainProperty:
        {

        const int selector = PopInt();
        SPRITE* const unit = reinterpret_cast<SPRITE*>(U32ToPtr(static_cast<unsigned int>(PopInt())));
        if (unit == 0 ||
           unit->IsSpriteClass(21u) == 0)
            {
            stack->PushInt(0);
            break;
        }

        TRAIN_INFO stats(reinterpret_cast<const ENGINE*>(unit));
        switch (selector)
            {
        case 1:
            stack->PushInt(stats.maxSpeedThousandths);
        break;
        case 2:
            stack->PushInt(stats.fireDamage);
        break;
        case 3:
            stack->PushInt(static_cast<unsigned int>(
                MultiplySigned32Wrap(stats.currentHp, 100)/stats.maxHp));
            break;
        case 4:
            stack->PushInt(stats.currentHp);
        break;
        case 5:
            stack->PushInt(stats.averageAmmoPercent);
        break;
        case 6:
            stack->PushInt(stats.Acceleration());
        break;
        case 7:
            stack->PushInt(stats.buildTime);
        break;
        case 8:
            stack->PushInt(0);
        break;
        case 9:
                {
            for (ENGINE* node = reinterpret_cast<ENGINE*>(unit)->FirstEngine();node;node = node->NextEngine())
                    {
                const unsigned int value = reinterpret_cast<const SPRITE*>(node)->IsCommand(0u);
                if (value == 0u)stack->PushInt(value);
            }
            stack->PushInt(0);
            break;
        }
        case 10:
            stack->PushInt(stats.ammoCount);
        break;
        case 11:
            stack->PushInt(stats.ammoCapacity);
        break;
        default:
            stack->PushInt(0);
        break;
        }
        break;
    }
    case SCRIPT_EXT_SendCommand:
        {
        const unsigned int var3 = static_cast<unsigned int>(PopInt());
        const unsigned int var2 = static_cast<unsigned int>(PopInt());
        const unsigned int var1 = static_cast<unsigned int>(PopInt());
        const unsigned int scriptCommand = static_cast<unsigned int>(PopInt());

        if (scriptCommand == EXT_SomethingStrange)
            {
            if (!g_pFucking)
                DebugAssert(4u, "g_pFucking", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\script_exec.cpp", 1116u);
            STRING tag;
            tag = "EXT_SomethingStrange";
            PushSprite(g_pFucking, tag);
            break;
        }

        STRING textResult;
        textResult = "";
        int intResult = 0;
        SPRITE* specialResult = 0;
        MAP* const map = Map;
        PLAYER* const player = map->Player();
        zScriptExec* const exec = static_cast<zScriptExec*>(static_cast<PLAYER_ARCADE*>(player));
        const bool returnsString = exec->ExecCommand(
            static_cast<int>(scriptCommand), static_cast<int>(var1), static_cast<int>(var2), static_cast<int>(var3),
            intResult, textResult, specialResult);
        if (specialResult != 0)
            {
            STRING tag;
            tag = "EXT_SomethingStrange2";
            PushSprite(specialResult, tag);
        }
            else if (returnsString)
            {
            PushStr(textResult);
        }
            else
            {
            stack->PushInt(intResult);
        }
        break;
    }
    case SCRIPT_EXT_SendCommandC:
        {
        STRING textArgument;
        textArgument = PopStr();
        const int scriptCommand = PopInt();
        STRING CharRet;
        CharRet = "";
        int intResult = 0;
        MAP* const map = Map;
        PLAYER* const player = map->Player();
        static_cast<zScriptExec*>(static_cast<PLAYER_ARCADE*>(player))->ExecCommandC(
                scriptCommand, textArgument.value, intResult, CharRet);
        if (!CharRet.IsEmpty())
            DebugAssert(4u, "CharRet.IsEmpty()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\script_exec.cpp", 1199u);
        if (!CharRet.IsEmpty())
            PushStr(CharRet);
        else
            stack->PushInt(intResult);
        break;
    }
    case SCRIPT_EXT_SendCommandA:
        {
        const unsigned int var3 = static_cast<unsigned int>(PopInt());
        const unsigned int var2 = static_cast<unsigned int>(PopInt());
        const unsigned int var1 = static_cast<unsigned int>(PopInt());
        const unsigned int scriptCommand = static_cast<unsigned int>(PopInt());
        int nArmy = PopInt();

        if (scriptCommand == EXT_GETINPUTACTION)
            {
            INPUT* inputState;
            if (nArmy == -2)
                {
                DebugAssert(4u, "nArmy != -2", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\script_exec.cpp", 1153u);
                inputState = &m_input;
            }
                else if (nArmy >= 0 && nArmy<8)
                {
                PLAYER* const player = m_players[nArmy];
                inputState = &player->m_input;
            }
                else
                {
                inputState = &m_input;
            }
            if (static_cast<int>(var1) <= 0 || var1 >= 32u)
                DebugAssert(5u, "var1 > 0 && var1 < IA_COUNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\script_exec.cpp", 1156u);
            stack->PushInt(inputState->GetAction(var1));
            break;
        }

        STRING textResult;
        textResult = "";
        int IntRet = 0;
        if (nArmy == -1) nArmy = static_cast<int>(m_playerIndex);
        int firstArmy = nArmy;
        int lastArmy = nArmy;
        if (nArmy == -2)
            {
                firstArmy = 0;
                lastArmy = 7;
            }

        bool ReturnString = false;
        for (int index = firstArmy;index <= lastArmy;++index)
            {
            PLAYER* const player = Map->Player(static_cast<int>(index));
            zScriptExec* const exec = static_cast<zScriptExec*>(static_cast<PLAYER_ARCADE*>(player));
            ReturnString = exec->ExecCommandA(
                static_cast<int>(scriptCommand), static_cast<int>(var1), static_cast<int>(var2), static_cast<int>(var3),
                IntRet, textResult);
        }
        if (nArmy == -2 && (ReturnString || IntRet != 0u))
            DebugAssert(4u, "nArmy != -2 || !ReturnString && IntRet == 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\script_exec.cpp", 1178u);
        if (ReturnString)
            PushStr(textResult);
        else
            stack->PushInt(IntRet);
        break;
    }
    case SCRIPT_EXT_SetInventoryCoord:
        {
        const int deltaY = PopInt();
        const int deltaX = PopInt();
        const unsigned int inventorySlot = static_cast<unsigned int>(PopInt());
        MAP* const map = Map;
        PLAYER_ARCADE* const player = static_cast<PLAYER_ARCADE*>(
            map->Player());
        player->SetInventoryCoord(static_cast<int>(inventorySlot),
                                  static_cast<float>(deltaX),
                                  static_cast<float>(deltaY));
        break;
    }
    case SCRIPT_EXT_SetMapType:
        {
        const unsigned int deathMatch = PopInt() != 0 ? 1u : 0u;
        STRING campaignName;
        campaignName = PopStr();
        const unsigned int surviveMode = PopInt() != 0 ? 1u : 0u;
        const unsigned int mapType = static_cast<unsigned int>(PopInt());
        for (unsigned int army = 0u;army<8u;++army)
            {
            PLAYER* const player = Map->Player(static_cast<int>(army));
            static_cast<PLAYER_ARCADE*>(player)->SetCurMapType(
                static_cast<int>(mapType), surviveMode != 0u, campaignName, deathMatch != 0u);
        }
        break;
    }
    default:
        {

        MYERROR* sink = ::Error;
        sink->Log("!!!ERROR!!!LOGIC: Unknown extern Function %i", command);
        break;
    }
    }
    return 0u;
}





int VID::NoSprites(int player) const
{
    return static_cast<int>(spriteCountsByPlayer[player]);
}

extern "C" void*  ConstructRegistryAtAddress(void* self, const STRING* source)
{
    ::new(self) REGISTRY(*source);
    return self;
}
extern "C" void*  ConstructCrc32AtAddress(void* self, const void* data, unsigned int size)
{
    ::new(self) CRC32(data, size);
    return self;
}


