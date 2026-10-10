
#include "sprite.h"
#include "texture.h"
#include "engine_globals.h"
#include "sound.h"
#include "myerror.h"
#include "map.h"
#include "mousetips.h"
#include "mouse.h"
#include "graph.h"
#include "menu.h"
#include "region.h"
#include "mylib.h"
#include "input.h"
#include "zm/Input/zInputMap.h"
#include "balloon.h"
#include "building.h"
#include "creature.h"
#include "robot.h"
#include "depo.h"
#include "rail.h"
#include "resread.h"
#include "vid.h"
#include "unit.h"
#include "engine.h"
#include "registry.h"
#include "dialogSrv.h"
#include "sprite_list.h"
#include "sprite_collector.h"
#include "player_arcade.h"
#include "net.h"
#include "script.h"
#include "basetype.h"
#include "constant.h"
#include <new>
#pragma push_macro("INPUT")
#undef INPUT
#define INPUT WIN32_INPUT
#include <windows.h>
#pragma pop_macro("INPUT")

MAP_STEAM::MAP_STEAM(HINSTANCE__* instance, HINSTANCE__* previousInstance, const STRING& commandLine, int showCommand, GRAPH_INIT* graphInit)
    : MAP(instance, previousInstance, commandLine, showCommand, graphInit)
{
    MAP_STEAM* const map = this;
    unsigned int mapFlags = map->m_flags;
    if ((mapFlags&8u) == 0u)
        return;
    map->m_debugFlags &= 0xFFFFFFF8u;
    CRC32 resourceCrc;
    resourceCrc.value = 0u;
    const unsigned int resourceCheckResult = static_cast<unsigned int>(resourceCrc.CheckListFromFile( STRING("maps\\check_list.lst")));
    mapFlags = map->m_flags&0xFDFFFFFFu;
    if (resourceCheckResult == 0u)
        mapFlags |= 0x02000000u;
    map->m_flags = mapFlags;
    (void)::Error->Log("Check right resource = %i %u", (mapFlags>>25)&1u, 0u);
    if ((*reinterpret_cast<const unsigned char*>(&graphInit->flags)&8u) != 0u)
    {
        STRING demoFile("game.dem");
        (void)map->m_resource.OpenForWrite(demoFile, ResourceType::Demo);
    }
    map->Load(map->m_pendingMapFileName);
    return;
}

MAP_STEAM::~MAP_STEAM() noexcept(false)
{
}

void MAP_STEAM::Release()
{
    MAP_STEAM* const map = this;
    map->m_flags &= ~4u;
    ENGINE::ClearPathDots();
    map->m_mouseTips.Clear();
    map->MAP::Release();
}

void MAP_STEAM::DrawSecondaryInfo()
{
    void* const self = this;
    MAP::DrawSecondaryInfo();
    unsigned char debugFlags = static_cast<unsigned char>(static_cast<MAP_STEAM*>(self)->m_debugFlags);
    SPRITE_COLLECTOR* collector;
    if ((debugFlags&2u) != 0)
    {
        collector = ::Sprites;
        SPRITE* sprite = collector->FirstUnit(&collector->reserved2EC, -1);
        while (sprite)
        {
            if (!sprite->ChildBacklink())
            {
                const VECTOR2& viewShift = Map->ShiftXY();
                const VECTOR2 screenPosition( static_cast<float>(static_cast<double>(sprite->X())-viewShift.x), static_cast<float>(static_cast<double>(sprite->Y())-sprite->Z()-viewShift.y));
                Graph->PrintfXY(screenPosition, "%i", reinterpret_cast<const SPRITE_LAYOUT_ACCESS*>(sprite)->m_hp);
            }
            collector = ::Sprites;
            sprite = collector->NextUnit(&collector->reserved2EC);
        }
    }
    debugFlags = static_cast<unsigned char>(static_cast<MAP_STEAM*>(self)->m_debugFlags);
    if ((debugFlags&4u) != 0)
    {
        collector = ::Sprites;
        SPRITE* sprite = collector->FirstUnit(&collector->reserved2EC, -1);
        while (sprite)
        {
            sprite->DrawGoalLine();
            collector = ::Sprites;
            sprite = collector->NextUnit(&collector->reserved2EC);
        }
    }
}

float GRAPH::SizeX() const
{
    return screenWidth;
}

float GRAPH::SizeY() const
{
    return screenHeight;
}

SPRITE* MAP_STEAM::CreateSprite(VID* definition, const VECTOR& location, ANGLE direction, SPRITE* parent)
{
    if (!definition || definition == ::EmptyVid)
        return 0;
    if ((definition->runtimeFlags&0x20u) != 0u)
        definition = REGION::ConvertVid(definition, location);
    const int spriteLimit = definition->spriteLimit;
    if (spriteLimit >= 0)
    {
        unsigned int spriteCount = 0u;
        for (unsigned int playerIndex = 0;playerIndex<8u;++playerIndex)
            spriteCount += static_cast<unsigned int>(definition->spriteCountsByPlayer[playerIndex]);
        if (BitsToSigned32(spriteCount) >= spriteLimit)
            return 0;
    }
    const unsigned int spriteClass = definition->spriteClass;
    unsigned int allocationBytes;
    switch (spriteClass)
    {
        case 3u: allocationBytes = sizeof(BUILDING); break;
        case 20u: allocationBytes = sizeof(CIV_ROBOT); break;
        case 22u: allocationBytes = sizeof(RAIL); break;
        case 24u: allocationBytes = sizeof(DEPO); break;
        case 25u: allocationBytes = sizeof(CREATURE); break;
        case 26u: allocationBytes = sizeof(BALLOON); break;
        default: return MAP::CreateSprite(definition, location, direction, parent);
    }
    void* const storage = CrtNew(allocationBytes);
    if (!storage)
        return 0;

    SPRITE* createdSprite = 0;
    try
    {
        switch (spriteClass)
        {
            case 3u: createdSprite = new (storage) BUILDING(definition, location, direction, parent); break;
            case 20u: createdSprite = new (storage) CIV_ROBOT(definition, location, direction, parent); break;
            case 22u: createdSprite = new (storage) RAIL(definition, location, direction, parent); break;
            case 24u: createdSprite = new (storage) DEPO(definition, location, direction, parent); break;
            case 25u: createdSprite = new (storage) CREATURE(definition, location, direction, parent); break;
            case 26u: createdSprite = new (storage) BALLOON(definition, location, direction, parent); break;
        }
    }
    catch (...)
    {
        CrtFreePointer(storage);
        throw;
    }
    if (!createdSprite)
        return 0;
    if ((m_flags&0x40u) == 0u)
    {
        const int scriptFunction = createdSprite->Vid()->scriptFunction[14];
        if (scriptFunction >= 0)
            (void)ScriptRun(scriptFunction, createdSprite, parent, 0);
    }
    return createdSprite;
}

static void MapSteamWaitFrame()
{
    ::CurrentTime = ::PrevCurrentTime;
    (void)::WaitMessage();
}

void MAP_STEAM::MapSteamAssignMapFileName(MAP_STEAM* self)
{
    const STRING fileName = reinterpret_cast<MAP*>(self)->FileName();
    self->m_flags |= 0x80u;
    self->m_pendingMapFileName = fileName;
}

void MAP_STEAM::MapSteamProcessDebugHotkey(MAP_STEAM* self, INPUT* input)
{
    const unsigned int debugCommand = static_cast<const INPUT*>(input)->GetAction(72u);
    switch (debugCommand)
    {
        case 66u:
            if ((self->m_flags&0x20u) != 0u) reinterpret_cast<MAP*>(self)->PauseOff();
        else reinterpret_cast<MAP*>(self)->PauseOn();
        break;
        case 71u:
            self->m_debugFlags ^= 0x4u;
        break;
        case 72u:
            self->m_debugFlags ^= 0x2u;
        break;
        case 73u:
        {
            const unsigned int mapFlags = self->m_flags;
            const unsigned int stateBit13 = (mapFlags>>13)&1u;
            const unsigned int stateBit17 = (mapFlags>>17)&1u;
            const unsigned int nextState = (stateBit17-(stateBit13<<1)+1u)&3u;
            const unsigned int encodedState = ((((nextState>>1)&1u)|((nextState&1u)<<4))<<13);
            self->m_flags = (mapFlags&0xFFFDDFFFu)|encodedState;
            break;
        }
        case 74u:
        {
            GRAPH* const graph = Graph;
            graph->SetDrawShadow((graph->deviceState&1u) == 0u?1:0);
            break;
        }
        case 76u:
        {
            GRAPH* graph = Graph;
            graph->deviceState ^= 0x2u;
            break;
        }
        case 77u:
        {
            GRAPH* const graph = Graph;
            graph->SetDrawMesh((graph->capabilityFlags&0x8000u) == 0u?1u:0u);
            break;
        }
        case 78u:
            self->m_flags ^= 0x00100000u;
        break;
        case 79u:
            self->m_flags ^= 0x00010000u;
        self->m_flags ^= 0x00001000u;
        break;
        case 80u:
            self->m_debugFlags ^= 0x1u;
        break;
        default:
            break;
    }
}

bool MAP_STEAM::MapSteamFrameMayContinue(MAP_STEAM* self, INPUT* input, bool configEnabled)
{
    const bool frameActive = ((self->m_flags&0x10u) != 0u) || ((::Net->m_flags&0x20u) != 0u);
    if (frameActive)
    {
        GRAPH* graph = Graph;
        if ((graph->capabilityFlags&1u) == 0u)
        {
            if (!configEnabled)
                return true;
            if ((self->m_debugFlags&1u) == 0u)
                return true;
            if (static_cast<const INPUT*>(input)->GetAction(72u) == 112u)
                return true;
        }
    }
    MapSteamWaitFrame();
    return false;
}

void MAP_STEAM::MapSteamApplyIndexedGamma(MAP_STEAM* self)
{
    const unsigned int armyMask = 0x000E0000u;
    SPRITE* sprite = self->Flagman(self->m_playerIndex);
    if (sprite)
    {
        sprite = self->Flagman(self->m_playerIndex);
        if ((sprite->RuntimeFlags()&armyMask) == 0u)
        {
            CONSTANT* settings = ::Const;
            if (static_cast<unsigned int>(settings->SelectUnitGamma) != 0u)
            {
                GAMMA gamma(static_cast<GAMMA::GAMMA_CREATE>(0), static_cast<unsigned int>(settings->SelectUnitGamma));
                self->Flagman(self->m_playerIndex)->SetGamma(gamma);
                sprite = self->Flagman(self->m_playerIndex);
                SPRITE* child = sprite->ChildChain();
                if (child && child->Vid() == sprite->Vid()->linkVid)
                {
                    settings = ::Const;
                    gamma = GAMMA(static_cast<GAMMA::GAMMA_CREATE>(0), static_cast<unsigned int>(settings->SelectUnitGamma));
                    sprite = self->Flagman(self->m_playerIndex);
                    sprite->ChildChain()->SetGamma(gamma);
                }
            }
        }
    }
    sprite = self->SpriteUnderCursor();
    if (sprite && (self->m_flags&0x00200000u) != 0u)
    {
        CONSTANT* settings = ::Const;
        if (static_cast<unsigned int>(settings->LightedUnitGamma) != 0u || static_cast<unsigned int>(settings->AttackUnitGamma) != 0u)
        {
            sprite = self->SpriteUnderCursor();
            settings = ::Const;
            const unsigned int gammaValue = ((sprite->RuntimeFlags()&armyMask) == 0x00020000u)?static_cast<unsigned int>(settings->AttackUnitGamma):static_cast<unsigned int>(settings->LightedUnitGamma);
            const GAMMA gamma(static_cast<GAMMA::GAMMA_CREATE>(0), gammaValue);
            self->SpriteUnderCursor()->SetGamma(gamma);
            sprite = self->SpriteUnderCursor();
            SPRITE* child = sprite->ChildChain();
            if (child && child->Vid() == sprite->Vid()->linkVid)
            {
                sprite = self->SpriteUnderCursor();
                sprite->ChildChain()->SetGamma(gamma);
            }
        }
    }
    sprite = self->Flagman(self->m_playerIndex);
    if (sprite)
    {
        sprite = self->Flagman(self->m_playerIndex);
        if ((sprite->RuntimeFlags()&armyMask) == 0u)
        {
            CONSTANT* settings = ::Const;
            if (static_cast<unsigned int>(settings->SelectUnitGamma) != 0u)
            {
                const GAMMA gamma(static_cast<GAMMA::GAMMA_CREATE>(0), 0);
                self->Flagman(self->m_playerIndex)->SetGamma(gamma);
                sprite = self->Flagman(self->m_playerIndex);
                SPRITE* child = sprite->ChildChain();
                if (child && child->Vid() == sprite->Vid()->linkVid)
                {
                    sprite = self->Flagman(self->m_playerIndex);
                    const GAMMA childGamma(static_cast<GAMMA::GAMMA_CREATE>(0), 0);
                    sprite->ChildChain()->SetGamma(childGamma);
                }
            }
        }
    }
    sprite = self->SpriteUnderCursor();
    if (sprite)
    {
        CONSTANT* settings = ::Const;
        if (static_cast<unsigned int>(settings->LightedUnitGamma) != 0u || static_cast<unsigned int>(settings->AttackUnitGamma) != 0u)
        {
            const GAMMA gamma(static_cast<GAMMA::GAMMA_CREATE>(0), 0);
            self->SpriteUnderCursor()->SetGamma(gamma);
            sprite = self->SpriteUnderCursor();
            SPRITE* child = sprite->ChildChain();
            if (child && child->Vid() == sprite->Vid()->linkVid)
            {
                sprite = self->SpriteUnderCursor();
                const GAMMA childGamma(static_cast<GAMMA::GAMMA_CREATE>(0), 0);
                sprite->ChildChain()->SetGamma(childGamma);
            }
        }
    }
}

void MAP_STEAM::MapSteamTactControllers(MAP_STEAM* self)
{
    MAP_STEAM* const map = self;
    if ((map->m_flags&0x00100000u) != 0u)
        return;
    NET* network = ::Net;
    const bool replayInput = (network->m_flags&0x10u) != 0u;
    for (unsigned int playerIndex = 0u;playerIndex<8u;++playerIndex)
    {
        PLAYER* const controller = map->m_players[playerIndex];
        INPUT* const controlInput = replayInput?&controller->m_input:&map->m_input;
        controller->Control(controlInput);
    }
}

void MAP_STEAM::MapSteamTactObjects(MAP_STEAM* self)
{
    MAP_STEAM* const map = self;
    if ((map->m_flags&0x20u) != 0u)
    {
        int spriteIndex = map->m_menu.m_count-1;
        while (spriteIndex >= 0)
        {
            SPRITE* const sprite = map->m_menu.m_data[spriteIndex];
            sprite->Tact();
            --spriteIndex;
        }
        return;
    }
    SPRITE_COLLECTOR* collector = ::Sprites;
    int spriteCount = collector->allSprites.m_count;
    int spriteIndex = spriteCount-1;
    SPRITE* sprite = 0;
    while (spriteIndex >= 0 && spriteIndex<spriteCount)
    {
        sprite = collector->allSprites.m_data[spriteIndex];
        if (sprite)
            break;
        --spriteIndex;
    }
    while (sprite)
    {
        sprite->Tact();
        collector = ::Sprites;
        --spriteIndex;
        if (spriteIndex<0)
            break;
        spriteCount = collector->allSprites.m_count;
        if (spriteIndex >= spriteCount)
            break;
        sprite = 0;
        while (spriteIndex >= 0)
        {
            spriteCount = collector->allSprites.m_count;
            if (spriteIndex >= spriteCount)
                break;
            sprite = collector->allSprites.m_data[spriteIndex];
            if (sprite)
                break;
            --spriteIndex;
        }
    }
}

int MAP_STEAM::Tact()
{
    MAP_STEAM* const self = this;
    const int startResult = StartTact();
    if (startResult != 0)
    {
        return 1;
    }
    NetworkTact();
    CONSTANT* settings = ::Const;
    const bool debugModeEnabled = static_cast<unsigned int>(settings->DebugMode) != 0u;
    INPUT* input = &m_input;
    if (debugModeEnabled)
    {
        if (g_iZmDebug != 0u && input->GetAction(33u) != 0u && input->GetAction(32u) != 0u)
            MapSteamAssignMapFileName(self);
        MapSteamProcessDebugHotkey(self, input);
    }
    const bool frameMayContinue = MapSteamFrameMayContinue(self, input, debugModeEnabled);
    if (!frameMayContinue)
    {
        return 0;
    }
    GRAPH* graph = Graph;
    const unsigned int mapFlags = m_flags;
    int graphFrameRequired = ((((mapFlags&0x10u) != 0u) || ((graph->capabilityFlags&0x40u) == 0u)) && graph->movie.graphBuilder == 0)?1:0;
    if (graphFrameRequired && graph->PreTact() != 0)
    {
        return 0;
    }
    ++m_simulationFrameCount;
    const unsigned int demoTactResult = static_cast<unsigned int>(DemoTact());
    if (demoTactResult == 999999u)
    {
        graph = Graph;
        graph->PostTact(1);
        return 0;
    }
    int drawFrame = (demoTactResult != 0u && graphFrameRequired != 0)?1:0;
    m_menu.Control(input);
    (void)ScriptRun(-1, static_cast<const SPRITE*>(0), static_cast<const SPRITE*>(0), 0);
    graph = Graph;
    if (((m_flags&0x00400000u) != 0u && (graph->capabilityFlags&0x40u) != 0u) || (drawFrame != 0 && graph->movie.graphBuilder != 0))
    {
        drawFrame = 0;
        graphFrameRequired = 0;
    }
    MapSteamApplyIndexedGamma(self);
    MapSteamTactControllers(self);
    MapSteamTactObjects(self);
    graph = Graph;
    graph->Tact(drawFrame);
    if ((m_flags&0x00100000u) == 0u)
        m_mouseTips.Tact(input);
    unsigned int capturedMapFlags = m_flags;
    if ((capturedMapFlags&0x20u) == 0u && (capturedMapFlags&0x00080000u) != 0u && (::RealCurrentTime&0x400u) != 0u)
    {
        graph = Graph;
        const float textX = graph->SizeX()*(0.5f)-(200.0f);
        COLOR color;
        color.value = 0xFFFFFFFFu;
        graph->PutsXY(textX, 24.0f, "Presentation version. Not for sale!", color);
    }
    if (drawFrame != 0)
    {
        for (unsigned int playerIndex = 0u;playerIndex<8u;++playerIndex)
            m_players[playerIndex]->DrawSecondaryInfo();
        DrawSecondaryInfo();
        if ((::RealCurrentTime&0x400u) != 0u && m_script.m_bytecode == 0)
        {
            graph = Graph;
            const float textY = graph->SizeY()*(0.5f);
            const float textX = graph->SizeX()*(0.5f)-(100.0f);
            COLOR color;
            color.value = GRAPH::RED.value;
            graph->PutsXY(textX, textY, "Error in script file", color);
        }
    }
    if (graphFrameRequired != 0)
    {
        graph = Graph;
        graph->PostTact(1);
    }
    ::Sound->Tact();
    return 0;
}

STRING MAP::FileName() const
{
    return m_currentMapFileName;
}

extern "C" unsigned int __stdcall DialogProcAdapter(unsigned int,unsigned int,unsigned int,unsigned int);
#if defined(_MSC_VER) && defined(_M_IX86)

#pragma comment(linker, "/alternatename:DialogProcAdapter=_DialogProcAdapter@16")
#endif


