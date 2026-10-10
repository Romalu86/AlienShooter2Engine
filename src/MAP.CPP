#include <cstdio>
#include <new>
#include <cstring>
#include <direct.h>
#include "railway.h"
#include "texture.h"
#include "engine_globals.h"
#include "exports.h"

#include "myerror.h"
#include "mylib.h"
#include "script.h"
#include "basetype.h"
#include "sprite_collector.h"
#include "sprite.h"
#include "frame.h"
#include "sprite_list.h"
#include "mouse.h"
#include "mousetips.h"

#include "unit.h"
#include "engine.h"
#include "railway.h"
#include "plane.h"
#include "cannon.h"
#include "building.h"
#include "terrain.h"
#include "man.h"
#include "map.h"
#include "groups.h"
#include "input.h"
#include "net.h"
#include "resread.h"
#include "constant.h"
#include "sound.h"
#include "vid.h"
#include "region.h"
#include "zm/Game/Vehicles/zCarControl.h"
#include <math.h>

int EvFunctionNumber[64] = {};
int MapUltraLowDetail = 0;

namespace
{
StaticObjectStorage<ADD_CREATE_SPRITE> mapSpriteCreationContext;
unsigned char mapSpriteCreationInitialized = 0;
StaticObjectStorage<STRING> mapLoadingText;
StaticObjectStorage<ANGLE> mapLoadingDirection;
VID* mapLoadingVid = 0;

const char* mapEventNames[] =
{
    "main", "TrainNotAmmo", "TrainNotPower", "TrainDamage", "TrainCreated",
    "TrainSplit", "TrainDestroy", "TrainDestroyPower", "TrainArrive",
    "???TrainNotArrive", "TrainAttacked", "DepoDestroy", "DepoBirth",
    "DepoAttacked", "DepoFree", "BuildingCapture", "MasterDestroy", "???",
    "???SuperWeaponWounded", "MineBlast", "MineRemove", "EnemyLinked",
    "TrainClash", "UnitCreated", "UnitDestroy", &StringGlobals::EmptyTextCharacter
};

void __cdecl DestroyMapLoadingText()
{
    mapLoadingText.Get().~STRING();
}
}

void __cdecl InitializeMapLoadingDirectionAtStartup()
{
    new (mapLoadingDirection.Address()) ANGLE(0);
}

unsigned int __cdecl InitializeMapLoadingTextAtStartup()
{
    new (mapLoadingText.Address()) STRING;
    return RegisterCrtAtExit(&DestroyMapLoadingText);
}

struct MapOpenFileName
{
    unsigned int size;
    unsigned int ownerWindow;
    void* instance;
    const char* filter;
    char* customFilter;
    unsigned int maximumCustomFilter;
    unsigned int filterIndex;
    char* file;
    unsigned int maximumFile;
    char* fileTitle;
    unsigned int maximumFileTitle;
    const char* initialDirectory;
    const char* title;
    unsigned int flags;
    unsigned short fileOffset;
    unsigned short extensionOffset;
    const char* defaultExtension;
    unsigned int customData;
    unsigned int hook;
    const char* templateName;
};


#include <stdlib.h>
#include <new>
#include "graph.h"
#include "registry.h"
#include "dialogSrv.h"
#include "player_arcade.h"
#include "player.h"
#include "zm/Input/zInputMap.h"
#include "menu.h"
#if defined(_WIN32)
#  define INPUT WIN32_INPUT
#endif
#include <windows.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <shlobj.h>
#if defined(_WIN32)
#  undef INPUT
#endif
#include "runtime/win32_callbacks.h"


void DirectPlayReconcilePlayerRecords(NET* self);
static void __cdecl MapPresentationCallback(int mode);


int g_iZmDebug = 0;

MAP* Map=0;

unsigned long PrevRealCurrentTime = 0;
unsigned long RealCurrentTime = 0;
unsigned long PrevCurrentTime = 0;
unsigned long CurrentTime = 0;

static unsigned long pauseStartTick = 0;
static unsigned long demoRealTick = 0;
static unsigned long demoSimulationTick = 0;
static unsigned long frameRateSampleTick = 0;
static int mouseDisabledBeforeDeactivate = 0;
static int cursorNeedsRefresh = 0;
static int networkRandomMismatchReported = 0;

STRING MAP::WindowsUserPath() const
{
    return STRING(m_windowsUserPath);
}


void MAP::DrawSecondaryInfo()
{
    if ((m_flags&0x00040000u) != 0u)
    {
        GRAPH* const graph = Graph;
        graph->PrintfXY(graph->ViewMin().x, graph->ViewMin().y+1.0f, "%i", static_cast<int>(m_frameRate04));
    }
    if (Const->DebugMode == 0)
        return;
    SPRITE_COLLECTOR* collector;
    if ((m_flags&0x00020000u) != 0u)
    {
        const int vertexTexture = static_cast<int>(VID::useTextureMemory);
        const float soundY = Graph->ViewMin().y;
        const float soundX = Graph->ViewMax().x;
        SOUND* const sound = ::Sound;
        const unsigned int activeSound = static_cast<unsigned int>(sound->GetNoPlayed());
        Graph->PrintfXY(soundX-20.0f, soundY+1.0f, "%2i", activeSound);
        const int textureTime = static_cast<int>(TEXTURE::memoryInUse)/1000000;
        GRAPH* graph = Graph;
        graph->PrintfXY(graph->ViewMin().x, graph->ViewMin().y+20.0f, "%i ver/tex %i/%i,%i(%i) obj/ver/tri %u/%u/%u", ::NoBuildAlpha, VID::useVertexMemory, vertexTexture/1000000, vertexTexture%1000000, textureTime, VID_SURFACE::noObj, VID_SURFACE::noVertex, VID_SURFACE::noTri);
        graph = Graph;
        const int graphTime = static_cast<int>(graph->currentTime);
        const unsigned int shared = VID_SURFACE::noDrawCall[7];
        graph->PrintfXY(graph->ViewMin().x, graph->ViewMin().y+30.0f, "draw[1] %i/%i draw[2] %i/%i  draw[3] %i/%i  draw[4] %i/%i sLight %i,%i", VID_SURFACE::noDrawCall[1], VID_SURFACE::noDrawReal[1], VID_SURFACE::noDrawCall[2], VID_SURFACE::noDrawReal[2], VID_SURFACE::noDrawCall[3], VID_SURFACE::noDrawReal[3], VID_SURFACE::noDrawCall[4]+shared, VID_SURFACE::noDrawReal[4]+shared, graphTime/1000000, graphTime%1000000);
        const VECTOR2 entriesPoint = Graph->ViewMin();
        collector = ::Sprites;
        const int freeGround = collector->GetNoFreeBuildedGround();
        collector = ::Sprites;
        const int debugEntries = collector->DrawNamed();
        Graph->PrintfXY(entriesPoint.x, entriesPoint.y+50.0f, "%i %i", debugEntries, freeGround);
    }
    if ((m_flags&0x00001000u) != 0u && m_groundPrimary && m_groundHeight>1)
    {
        for (int row = 1;row<m_groundHeight;++row)
        {
            GRAPH* graph = Graph;
            for (int column = 1;column<m_groundWidth;++column)
            {
                const short* const first = reinterpret_cast<const short*>(m_groundPrimary);
                const short* const second = reinterpret_cast<const short*>(m_groundSecondary);
                const int index = row*m_groundWidth+column;
                const int z = first[index]>second[index]?first[index]:second[index];
                const int previousZ = first[index-1]>second[index-1]?first[index-1]:second[index-1];
                const VECTOR2 current(static_cast<float>(column*8+4)-m_shiftXY.x, static_cast<float>(row*8-z+4)-m_shiftXY.y);
                const VECTOR2 previous(static_cast<float>(column*8-4)-m_shiftXY.x, static_cast<float>(row*8-previousZ+4)-m_shiftXY.y);
                const bool currentInside = current.x >= graph->ViewMin().x && current.x<graph->ViewMax().x && current.y >= graph->ViewMin().y && current.y<graph->ViewMax().y;
                const bool previousInside = previous.x >= graph->ViewMin().x && previous.x<graph->ViewMax().x && previous.y >= graph->ViewMin().y && previous.y<graph->ViewMax().y;
                if (currentInside || previousInside)
                {
                    COLOR color;
                    color.value = GRAPH::GRAY.value;
                    Graph->Line(current, previous, color);
                    graph = Graph;
                }
            }
        }
    }
    collector = ::Sprites;
    if ((m_flags&0x00010000u) != 0u)
    {
        int index = collector->allSprites.m_count-1;
        for (;;)
        {
            SPRITE* sprite = 0;
            while (index >= 0 && index<collector->allSprites.m_count)
            {
                sprite = collector->allSprites.m_data[index];
                if (sprite)
                    break;
                --index;
            }
            if (!sprite)
                break;
            sprite->DrawRectangle();
            collector = ::Sprites;
            --index;
        }
    }
    if ((m_flags&0x00002000u) != 0u)
    {
        SPRITE* debugSprite = m_menu.SelectedSprite();
        if (!debugSprite)
        {
            const VECTOR2& pickPoint = m_input.MouseXY();
            debugSprite = collector->GetSpriteScr(0x00800000, pickPoint, 0);
        }
        if (debugSprite)
        {
            debugSprite->DrawSecondaryInfo();
        }
        else if (Flagman(m_playerIndex))
        {
            Flagman(m_playerIndex)->DrawSecondaryInfo();
            for (int oy = -17;oy<17;++oy)
            {
                for (int ox = -17;ox<17;++ox)
                {
                    const int centerX = TruncateFloatToInt32(Flagman(m_playerIndex)->X())/8;
                    const int centerY = TruncateFloatToInt32(Flagman(m_playerIndex)->Y())/8;
                    const int gx = centerX+ox;
                    const int gy = centerY+oy;
                    if (gx<0 || gx >= m_groundWidth || gy<0 || gy >= m_groundHeight)
                        continue;
                    const short* const first = reinterpret_cast<const short*>(m_groundPrimary);
                    const short* const second = reinterpret_cast<const short*>(m_groundSecondary);
                    const int index = gy*m_groundWidth+gx;
                    int shade = first[index]>second[index]?first[index]:second[index];
                    shade /= 2;
                    if (shade<0)
                        shade = 0;
                    else if (shade>255)
                        shade = 255;
                    const unsigned int channel = static_cast<unsigned int>(shade);
                    const unsigned int color = 0xFF000000u|(channel<<16u)|(channel<<8u)|channel;
                    const VECTOR2 drawPoint = (Graph->ViewMax()-VECTOR2(100, 50))+VECTOR2(ox, oy)*2.0f;
                    COLOR markerColor;
                    markerColor.value = color;
                    Graph->PutBigPixel(drawPoint, markerColor);
                }
            }
        }
    }
    if ((m_flags&0x00008000u) != 0u)
        m_groups.DrawNumber();
    if ((m_flags&0x00004000u) != 0u)
        RailMap.Get().DebugDraw();
}

void MAP::DeletePointerToSprite(SPRITE* sprite)
{
    m_mouseTips.DeletePointerToSprite(sprite);
    if (m_players[0] != 0)
        for (unsigned int i = 0u;i<8u;++i) m_players[i]->DeletePointerToSprite(sprite);
    m_script.DeletePointerToObject(sprite);
    m_groups.DeletePointerToSprite(sprite);
}

int MAP::WorkWndMessage(HWND__* hwnd, unsigned long msg, unsigned long wParam, unsigned long lParam)
{
    INPUT* const input = &m_input;
    if ((static_cast<unsigned char>(m_flags)&0x10u) != 0u && input->WorkWndMessage(hwnd, msg, wParam, lParam) != 0)
        return 1;
    switch (msg)
    {
        case WM_PAINT:
            ::Error->Log("    WM_PAINT");
        break;
        case WM_ACTIVATEAPP:
        {
            SOUND* const sound = ::Sound;
            MOUSE* const mouse = Mouse;
            if (sound != 0 && mouse != 0)
            {
                if (wParam != 0u)
                {
                    if ((static_cast<unsigned char>(m_flags)&0x10u) == 0u)
                    {
                        INPUT::pauseForceFeedBack = 0u;
                        sound->Resume();
                        if (mouseDisabledBeforeDeactivate == 0)
                            Mouse->Enable();
                        else (void)SetCursor(0);
                    }
                }
                else if ((static_cast<unsigned char>(m_flags)&0x10u) != 0u)
                {
                    input->StopFFB(-1);
                    INPUT::pauseForceFeedBack = 1u;
                    ::Sound->Pause();
                    MOUSE* const currentMouse = Mouse;
                    mouseDisabledBeforeDeactivate = currentMouse->m_enabled == 0u?1:0;
                    currentMouse->Disable();
                }
            }
            const unsigned int active = wParam != 0u?0x10u:0u;
            m_flags = (m_flags&~0x10u)|active;
            cursorNeedsRefresh = 1;
            break;
        }
        case WM_SETCURSOR:
            if ((static_cast<unsigned char>(m_flags)&0x10u) != 0u && cursorNeedsRefresh != 0u)
        {
            cursorNeedsRefresh = 0;
            MOUSE* const mouse = Mouse;
            if (mouse != 0 && mouse->m_enabled == 0u && GetCursor() != 0)
                (void)SetCursor(0);
        }
        break;
        case WM_DESTROY:
            if ((Graph->capabilityFlags&0x40u) == 0u)
        {
            RECT rect;
            (void)GetWindowRect(reinterpret_cast<HWND>(U32ToPtr(m_dialog)), &rect);
            {
                const STRING xName("WindowPositionX");
                ::Registry->SetInt(xName, rect.left);
            }
            {
                const STRING yName("WindowPositionY");
                ::Registry->SetInt(yName, rect.top);
            }
        }
        m_dialog = 0u;
        PostQuitMessage(0);
        break;
        case WM_SYSCOMMAND:
            if ((wParam == SC_SIZE || wParam == SC_MOVE || wParam == SC_MAXIMIZE || wParam == SC_MONITORPOWER) && (Graph->capabilityFlags&0x40u) != 0u) return 1;
        break;
        case WM_ENTERMENULOOP:
            case WM_ENTERSIZEMOVE: Graph->BeginPause();
        if (SOUND* const sound = ::Sound)
            sound->Pause();
        break;
        case WM_EXITMENULOOP:
            case WM_EXITSIZEMOVE: Graph->EndPause();
        if (SOUND* const sound = ::Sound)
            sound->Resume();
        break;
        default:
            break;
    }
    return 0;
}

VECTOR2 MAP::ToScreenScaled(const VECTOR2& value) const
{
    GRAPH* const graph = Graph;
    if ((graph->sceneState&2u) == 0u)
        return VECTOR2(value.x-m_shiftXY.x, value.y-m_shiftXY.y);
    const float offsetX = static_cast<float>(TruncateFloatToInt32(m_shiftXY.x*graph->screenScaleX));
    const float offsetY = static_cast<float>(TruncateFloatToInt32(m_shiftXY.y*graph->screenScaleY));
    return VECTOR2((value.x-graph->viewportOffsetX)*graph->screenScaleX-offsetX, (value.y-graph->viewportOffsetY)*graph->screenScaleY-offsetY);
}

void MAP::SetShiftCoor(const VECTOR2& point, int mode)
{
    GRAPH* graph = Graph;
    if (mode == 2)
    {
        const unsigned int x = static_cast<unsigned int>(TruncateFloatToInt32(point.x));
        const unsigned int y = static_cast<unsigned int>(TruncateFloatToInt32(point.y));
        graph->Effect(2, static_cast<int>(x), static_cast<int>(y), -1);
        return;
    }
    const bool absoluteProjection = (static_cast<unsigned int>(mode)&0x10000000u) != 0u;
    float x = point.x-graph->screenWidth*0.5f;
    float y = point.y-graph->screenHeight*0.5f;
    if (!absoluteProjection)
    {
        x += graph->viewportOffsetX;
        y += graph->viewportOffsetY;
        const float minX = m_cameraMinX-graph->viewLeft;
        const float minY = m_cameraMinY-graph->viewTop;
        if (x<minX)
            x = minX;
        if (y<minY)
            y = minY;
        x -= graph->viewportOffsetX*2.0f;
        y -= graph->viewportOffsetY*2.0f;
        const float maxX = m_cameraMaxX-graph->viewRight;
        const float maxY = m_cameraMaxY-graph->viewBottom;
        if (x>maxX)
            x = maxX;
        if (y>maxY)
            y = maxY;
        x += graph->viewportOffsetX;
        y += graph->viewportOffsetY;
    }
    x = static_cast<float>(TruncateFloatToInt32(x));
    y = static_cast<float>(TruncateFloatToInt32(y));
    if ((m_shiftXY.x != x || m_shiftXY.y != y) && !absoluteProjection)
    {
        const float dx = x-m_shiftXY.x;
        const float dy = y-m_shiftXY.y;
        for (int i = 0;i<m_menu.m_count;++i)
        {
            SPRITE* const sprite = m_menu.m_data[i];
            const VECTOR translated(sprite->X()+dx, sprite->Y()+dy, sprite->Z());
            sprite->ChangeCoor(translated);
        }
        if (Mouse)
        {
            const VECTOR translated(Mouse->X()+dx, Mouse->Y()+dy, Mouse->Z());
            Mouse->ChangeCoor(translated);
        }
    }
    m_shiftXY.x = x;
    m_shiftXY.y = y;
    graph = Graph;
    const float scaleX = graph->screenScaleX;
    const float scaleY = graph->screenScaleY;
    const float screenX = static_cast<float>(TruncateFloatToInt32(x*scaleX));
    const float screenY = static_cast<float>(TruncateFloatToInt32(y*scaleY));
    const float halfSpanX = (graph->viewLeft+graph->viewRight)*0.5f;
    const float halfSpanY = (graph->viewTop+graph->viewBottom)*0.5f;
    const float transform[16] =
    {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, -(screenX/scaleX)-halfSpanX, -(screenY/scaleY)-halfSpanY, 128.0f, 1.0f
    }
    ;
    graph->SetTransformMatrix(static_cast<_D3DTRANSFORMSTATETYPE>(2), reinterpret_cast<const _D3DMATRIX*>(transform), 0);
}

void MAP::SetScrollType(int mode, const VECTOR2& first, const VECTOR2& second)
{
    m_scrollMode = static_cast<unsigned int>(mode);
    if (FloatBitsU32(first.x) == FloatBitsU32(-1000.0f) && FloatBitsU32(first.y) == FloatBitsU32(-1000.0f))
    {
        m_scrollMouseMinX = Graph->screenWidth-(640.0f);
        m_scrollMouseMinY = Graph->screenHeight-(480.0f);
        m_scrollMouseMaxX = 640.0f;
        m_scrollMouseMaxY = 480.0f;
    }
    else
    {
        m_scrollMouseMinX = first.x;
        m_scrollMouseMinY = first.y;
        m_scrollMouseMaxX = second.x;
        m_scrollMouseMaxY = second.y;
    }
}

void MAP::ControlShiftCoor()
{
    const unsigned int flags = m_scrollMode;
    INPUT* const input = &m_input;
    const float mouseX = input->m_clientMouseXY.x;
    const float mouseY = input->m_clientMouseXY.y;
    GRAPH* const graph = Graph;
    const float edge = 5.0f;
    const float xStep = 0.04f;
    const float yStep = 0.03f;
    const float half = 0.5f;
    const float velocityScale = 0.001f;
    float velocityX = m_velocityX;
    float velocityY = m_velocityY;
    if ((flags&0x21u) == 0u)
    {
        velocityX = 0.0f;
        velocityY = 0.0f;
    }
    else
    {
        const float maxX = Const->MaxScrollSpeedX;
        if (mouseX <= edge && (flags&1u) != 0u)
        {
            if (velocityX>-maxX)
                velocityX -= xStep;
        }
        else if (mouseX >= graph->screenWidth-edge && (flags&1u) != 0u)
        {
            if (velocityX<maxX)
                velocityX += xStep;
        }
        else if (input->GetAction(3u) != 0 && (flags&0x20u) != 0u)
        {
            if (velocityX>-maxX)
                velocityX -= xStep;
        }
        else if (input->GetAction(4u) != 0 && (flags&0x20u) != 0u)
        {
            if (velocityX<maxX)
                velocityX += xStep;
        }
        else velocityX = 0.0f;
        const float maxY = Const->MaxScrollSpeedY;
        if (mouseY <= edge && (flags&1u) != 0u)
        {
            if (velocityY>-maxY)
                velocityY -= yStep;
        }
        else if (mouseY >= graph->screenHeight-edge && (flags&1u) != 0u)
        {
            if (velocityY<maxY)
                velocityY += yStep;
        }
        else if (input->GetAction(1u) != 0 && (flags&0x20u) != 0u)
        {
            if (velocityY>-maxY)
                velocityY -= yStep;
        }
        else if (input->GetAction(2u) != 0 && (flags&0x20u) != 0u)
        {
            if (velocityY<maxY)
                velocityY += yStep;
        }
        else velocityY = 0.0f;
    }
    m_velocityX = velocityX;
    m_velocityY = velocityY;
    const int playerIndex = static_cast<int>(m_playerIndex);
    SPRITE* const player = Flagman(static_cast<unsigned int>(playerIndex));
    if (player && (flags&4u) != 0u)
    {
        const VECTOR2 playerScreen = player->Coor().ViewCoor();
        const VECTOR2 mapShift(Map->m_shiftXY);
        const VECTOR2 relative = playerScreen-mapShift;
        const float scale = m_scrollFollowSpeedScale*velocityScale;
        velocityX = (relative.x-graph->screenWidth*half)*scale;
        velocityY = (relative.y-graph->screenHeight*half)*scale;
        m_velocityX = velocityX;
        m_velocityY = velocityY;
    }
    else if (player && (flags&8u) != 0u && velocityX == 0.0f && velocityY == 0.0f)
    {
        float targetMouseX = mouseX;
        if (targetMouseX<m_scrollMouseMinX)
            targetMouseX = m_scrollMouseMinX;
        else if (targetMouseX>m_scrollMouseMaxX)
            targetMouseX = m_scrollMouseMaxX;
        float targetMouseY = mouseY;
        if (targetMouseY<m_scrollMouseMinY)
            targetMouseY = m_scrollMouseMinY;
        else if (targetMouseY>m_scrollMouseMaxY)
            targetMouseY = m_scrollMouseMaxY;
        float speedX = 0.0f;
        float speedY = 0.0f;
        VID* const playerVid = player->Vid();
        if (playerVid->spriteClass == 7u)
            static_cast<zCarControl*>(static_cast<MAN*>(player))->GetSpeed(speedX, speedY);
        const VECTOR2 playerScreen = player->Coor().ViewCoor();
        const VECTOR2 mapShift(Map->m_shiftXY);
        const VECTOR2 relative = playerScreen-mapShift;
        const float targetX = (relative.x+targetMouseX)*half;
        const float targetY = (relative.y+targetMouseY)*half;
        float lead = static_cast<float>(sqrt(static_cast<double>(speedX*speedX+speedY*speedY)))*0.033333335f;
        if (lead<4.0f)
            lead = 4.0f;
        else if (lead>20.0f)
            lead = 20.0f;
        const float centerX = (m_scrollMouseMinX+m_scrollMouseMaxX)*half;
        const float centerY = (m_scrollMouseMinY+m_scrollMouseMaxY)*half;
        const float scale = m_scrollFollowSpeedScale*velocityScale;
        velocityX = (targetX-centerX)*lead*scale;
        velocityY = (targetY-centerY)*lead*scale;
        m_velocityX = velocityX;
        m_velocityY = velocityY;
    }
    else if (player && (flags&0x10u) != 0u && velocityX == 0.0f && velocityY == 0.0f)
    {
        const VECTOR2 playerScreen = player->Coor().ViewCoor();
        const VECTOR2 target(m_shiftXY.x+playerScreen.x-Map->m_shiftXY.x, m_shiftXY.y+playerScreen.y-Map->m_shiftXY.y);
        SetShiftCoor(target, 0);
        return;
    }
    const unsigned int elapsed = ::CurrentTime-::PrevCurrentTime;
    const VECTOR2 target(graph->screenWidth*half+m_shiftXY.x+static_cast<float>(elapsed)*velocityX, graph->screenHeight*half+m_shiftXY.y+static_cast<float>(elapsed)*velocityY);
    SetShiftCoor(target, 0);
}

SPRITE* MAP::Flagman(unsigned int index) const
{
    PLAYER* const entry = m_players[index&7u];
    return entry->m_flagman.m_value;
}

SPRITE* MAP::SpriteUnderCursor() const
{
    const unsigned int index = m_playerIndex&7u;
    PLAYER* const entry = m_players[index];
    return entry->m_transientSprite.m_value;
}

int MAP::DemoTact()
{
    RESOURCE* const stream = &m_resource;
    INPUT* const input = &m_input;
    unsigned int flags = m_flags;
    if ((flags&0x00000400u) != 0u)
    {
        int demoTime;
        (void)stream->Read(&demoTime, 4u);
        if (demoTime == -1 || input->GetAction(IA_ANY_KEY) != 0)
            goto playback_finished;
        (void)stream->Read(&m_frameRate04, 4u);
        input->Load(stream);
        unsigned int recordedRandom;
        (void)stream->Read(&recordedRandom, 4u);
        const unsigned int liveRandom = static_cast<unsigned int>(Random(static_cast<int>(50000u)));
        if (recordedRandom != liveRandom)
        {
            m_flags |= 0x00800000u;
            STRING subject("Demo synchro error");
            const unsigned int errorTick = ::CurrentTime;
            if (::Error != 0)
            {
                STRING owner("MAP");
                ::Error->Error(owner, E_ERROR, subject, static_cast<int>(errorTick));
            }
        }
        unsigned int previousDemoTime = demoSimulationTick;
        if (static_cast<unsigned int>(demoTime)-previousDemoTime>71u)
        {
            previousDemoTime = static_cast<unsigned int>(demoTime)-71u;
            demoSimulationTick = previousDemoTime;
        }
        const unsigned int targetTime = static_cast<unsigned int>(demoTime);
        unsigned int now = timeGetTime();
        unsigned int elapsedReal = now-demoRealTick;
        unsigned int elapsedDemo = targetTime-demoSimulationTick;
        if (elapsedDemo>elapsedReal)
        {
            now = timeGetTime();
            elapsedReal = now-demoRealTick;
            elapsedDemo = targetTime-demoSimulationTick;
            if (elapsedDemo >= elapsedReal)
            {
                do
                {
                    now = timeGetTime();
                    elapsedReal = now-demoRealTick;
                    elapsedDemo = targetTime-demoSimulationTick;
                }
                while (elapsedDemo >= elapsedReal);
            }
        }
        ::CurrentTime = targetTime;
        demoSimulationTick = targetTime;
        demoRealTick = timeGetTime();
    }
    flags = m_flags;
    if ((flags&0x00000200u) != 0u)
    {
        (void)stream->Write(&::CurrentTime, 4u);
        (void)stream->Write(&m_frameRate04, 4u);
        input->Save(stream);
        unsigned int randomValue = static_cast<unsigned int>(Random(static_cast<int>(50000u)));
        (void)stream->Write(&randomValue, 4u);
    }
    return 1;
    playback_finished:
    {
        STRING name;
        (void)stream->GoNext(ResourceType::Demo);
        stream->m_ioState = RESOURCE::IO_SEEK;
        (void)std::fseek(stream->m_file, BitsToSigned32(stream->m_recordSize), SEEK_CUR);
        name.Read(stream);
        const char* text = name.m_value;
        const char* expected = StringGlobals::EmptyText();
        const char* mapNameCursor = text;
        const char* emptyNameCursor = expected;
        while (*mapNameCursor == *emptyNameCursor && *mapNameCursor)
        {
            ++mapNameCursor;
            ++emptyNameCursor;
        }
        if (static_cast<unsigned char>(*mapNameCursor) == static_cast<unsigned char>(*emptyNameCursor))
            name = "maps\\logo.map";
        stream->Close();
        Mouse->Enable();
        unsigned int newFlags = m_flags;
        newFlags &= ~0x00000400u;
        newFlags |= 0x00000080u;
        m_flags = newFlags;
        m_pendingMapFileName = name;
        input->Tact();
    }
    return 999999u;
}


void __cdecl MapStaticCleanupNoOp()
{
}

void VID::Draw(SPRITE*)
{
}
void VID::DrawShadow(const SPRITE*, const VECTOR&)
{
}
void VID::DrawOnlyZ(const SPRITE*)
{
}
void VID::InvalidateDeviceObjects()
{
}
void VID::RestoreDeviceObjects()
{
}
void VID::Load(RESOURCE*)
{
}
void VID::SetReColorForArmy(COLOR)
{
}
void VID::SetFramesToDevice()
{
}

void VID_SOFTWARE::DrawShadow(const SPRITE*)
{
}

void __cdecl MapCompilerCleanupNoOp()
{
}

void __stdcall MsvcNoOpDestroyRange(unsigned int, unsigned int)
{
}


void MAP::SetFlagman(unsigned int index, SPRITE* sprite) const
{
    m_players[index&7u]->SetFlagman(sprite);
}

const STRING STRING::operator+(const STRING& right) const
{
    return STRING(value, right.value);
}

const STRING STRING::operator+(const char* right) const
{
    return STRING(value, right);
}
int MAP::Tact()
{
    return 0;
}

int MAP::ValidateVid(int index) const
{
    return index >= 0 && index < m_vidCount && m_vids[static_cast<unsigned int>(index)] != 0;
}

STREAM::~STREAM() noexcept(false)
{
}

FSTREAM::FSTREAM() : file(0)
{
}

int FSTREAM::Read(void* buffer, unsigned int count)
{
    const unsigned int consumed = static_cast<unsigned int>(std::fread(buffer, 1u, count, file));
    return BitsToSigned32(count-consumed);
}

int FSTREAM::Write(const void* buffer, unsigned int count)
{
    const unsigned int consumed = static_cast<unsigned int>(std::fwrite(buffer, 1u, count, file));
    return BitsToSigned32(count-consumed);
}

int MAP::IsNetGameStart() const
{
    const NET* const net = reinterpret_cast<const NET*>(::Net);
    return net && (net->m_flags&0x10u) ? 1 : 0;
}

void MAP::SetupProcessorFlags()
{
    (void)CrtControlFp(0x00020000u, 0x00030000u);
}

STRING VID::GetNumberName() const
{
    return Printf("%04i %s", nVid, name.value);
}

int MAP::ScriptRun(int function, const SPRITE* spriteA, const SPRITE* spriteB, int integerArgument)
{
    if (m_flags&0x00100000u)
        return 0;
    SCRIPT* const scriptRuntime = &m_script;
    return scriptRuntime->CallFunction(function, "ppi", static_cast<int>(PtrToU32(spriteA)), static_cast<int>(PtrToU32(spriteB)), integerArgument);
}

int MAP::ScriptEventRun(int eventId, const SPRITE* sprite, int integerArgument1, int integerArgument2)
{
    if (m_flags&0x00100000u)
        return 0;
    SCRIPT* const scriptRuntime = &m_script;
    return scriptRuntime->CallScriptEventFunction(eventId, "pii", static_cast<int>(PtrToU32(sprite)), integerArgument1, integerArgument2);
}

int MAP::ScriptRun(int function, const char* script, int scriptArgument1, int scriptArgument2, int scriptArgument3)
{
    if (m_flags&0x00100000u)
        return 0;
    SCRIPT* const scriptRuntime = &m_script;
    return scriptRuntime->CallFunction(function, script, scriptArgument1, scriptArgument2, scriptArgument3);
}

int MAP::ScriptRun(const STRING& script, const char* name, int scriptArgument1, int scriptArgument2, int scriptArgument3)
{
    if (m_flags&0x00100000u)
        return 0;
    SCRIPT* const scriptRuntime = &m_script;
    return scriptRuntime->CallFunction(script, name, scriptArgument1, scriptArgument2, scriptArgument3);
}

FSTREAM::~FSTREAM()
{
    if (file)
        (void)static_cast<unsigned int>(std::fclose(file));
}

const STRING STRING::BeforeLast(const STRING& delimiter) const
{
    return BeforeLast(delimiter.m_value);
}

void MAP::InvalidateDeviceObjects()
{
    for (int i = 0;i<m_vidCount;++i)
    {
        VID* const storedVid = m_vids[static_cast<unsigned int>(i)];
        if (storedVid)
            storedVid->InvalidateDeviceObjects();
    }
}

template<>
LIST<SPRITE*>::~LIST()
{
    if (m_data)
        CrtFreePointer(m_data);
    m_data = 0;
    m_count = 0;
}

template<>
void LIST<SPRITE*>::Expand(int requestedCapacity)
{
    if (requestedCapacity <= m_capacity)
        return;
    SPRITE** const oldData = m_data;
    SPRITE** const newData = static_cast<SPRITE**>(CrtNew(static_cast<unsigned int>(requestedCapacity*sizeof(SPRITE*))));
    m_data = newData;
    if (!newData)
    {
        STRING format("!!!ERROR!!!::LIST: Not enough memory %i");
        ::Error->LogExit(format, requestedCapacity);
    }
    if (oldData)
    {
        for (int index = 0;index<m_capacity;++index)
            m_data[index] = oldData[index];
        CrtFreePointer(oldData);
    }
    m_capacity = requestedCapacity;
}

RELATION::~RELATION()
{
}

SPRITE* RELATION::Decode(SPRITE* sprite) const
{
    for (int index = m_first.m_count-1;index >= 0;--index)
        if (m_first.m_data[index] == sprite) return m_second.m_data[index];
    return 0;
}

SPRITE* MAP::ReadPointer(STREAM* inputStream) const
{
    SPRITE* savedPointer = reinterpret_cast<SPRITE*>(inputStream);
    (void)inputStream->Read(&savedPointer, 4u);
    if (PtrToU32(savedPointer) == 0xFFFFFFFFu)
        return savedPointer;
    return m_relation.Decode(savedPointer);
}


void MAP::PauseOn()
{
    if ((m_flags&0x20u) == 0u)
    {
        pauseStartTick = ::CurrentTime;
        for (unsigned int i = 0;i<8u;++i)
        {
            PLAYER* const slot = m_players[i];
            slot->m_controlPauseState = 1u;
        }
    }
    m_flags |= 0x20u;
}

void MAP::PauseOff()
{
    if ((m_flags&0x20u) != 0u)
    {
        SPRITE_COLLECTOR* spriteCollector = ::Sprites;
        const int spriteCount = spriteCollector->allSprites.m_count;
        int spriteIndex = static_cast<int>(static_cast<unsigned int>(spriteCount)-1u);
        SPRITE* candidate = 0;
        if (spriteIndex >= 0 && spriteIndex<spriteCount)
        {
            SPRITE* const* const spriteTable = spriteCollector->allSprites.m_data;
            while (spriteIndex >= 0)
            {
                SPRITE* const listedSprite = spriteTable[spriteIndex];
                if (listedSprite != 0)
                {
                    candidate = listedSprite;
                    break;
                }
                --spriteIndex;
            }
        }
        const unsigned int resumeTick = pauseStartTick;
        while (candidate != 0)
        {
            if (candidate->m_lastAnimationTick>resumeTick)
                candidate->m_lastAnimationTick = resumeTick;
            --spriteIndex;
            candidate = 0;
            if (spriteIndex >= 0)
            {
                spriteCollector = ::Sprites;
                const int currentCount = spriteCollector->allSprites.m_count;
                if (spriteIndex<currentCount)
                {
                    SPRITE* const* const spriteTable = spriteCollector->allSprites.m_data;
                    while (spriteIndex >= 0)
                    {
                        SPRITE* const listedSprite = spriteTable[spriteIndex];
                        if (listedSprite != 0)
                        {
                            candidate = listedSprite;
                            break;
                        }
                        --spriteIndex;
                    }
                }
            }
        }
        const unsigned int pausedTick = ::CurrentTime;
        Graph->SetTime(pausedTick, resumeTick);
        const unsigned long resumedTick = pauseStartTick;
        ::CurrentTime = resumedTick;
        ::PrevCurrentTime = resumedTick-10u;
        for (unsigned int i = 0;i<8u;++i)
        {
            PLAYER* const slot = m_players[i];
            slot->m_controlPauseState = 0u;
        }
    }
    m_flags &= ~0x20u;
}


PRIMITIVE::PRIMITIVE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : SPRITE(vid, position, direction, parent)
{
}

PRIMITIVE::~PRIMITIVE()
{
}

void PRIMITIVE::Tact()
{
    PrimitiveTact();
}

void PRIMITIVE::DeletePointerToSprite(SPRITE*)
{
}

void PRIMITIVE::DrawGoalLine()
{
    m_vid->Draw(this);
}


MOUSETIPS::~MOUSETIPS()
{
    Clear();
}


void RELATION::Release()
{
    if (m_first.m_data)
        CrtFreePointer(m_first.m_data);
    m_first.m_data = 0;
    m_first.m_count = 0;
    m_first.m_capacity = 0;
    if (m_second.m_data)
        CrtFreePointer(m_second.m_data);
    m_second.m_data = 0;
    m_second.m_count = 0;
    m_second.m_capacity = 0;
}

STEXT::STEXT(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : FRAME(vid, position, direction, parent), m_renderedText(), m_sourceText()
{
    m_lineOffsets.m_data = 0;
    m_lineOffsets.m_count = 0;
    m_lineOffsets.m_capacity = 0;
    m_cursor = 0;
    m_textHeight = 0;
    m_textWidth = 0;
}

STEXT::~STEXT()
{
}

BUILDED_TERRAIN::BUILDED_TERRAIN(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent)
    : SPRITE(vid, position, direction, parent)
{
    ChangeAnimation(15);
}

BUILDED_TERRAIN::~BUILDED_TERRAIN()
{
}

SCRIPT::SCRIPT()
    : m_scriptFileName()
{
    m_stack.m_count = 0;
    m_stack.m_capacity = 0;
    m_stack.m_data = 0;
    m_functionList.m_count = 0;
    m_functionList.m_capacity = 0;
    m_functionList.m_data = 0;
    m_defineList.m_count = 0;
    m_defineList.m_capacity = 0;
    m_defineList.m_data = 0;
    m_bytecode = 0;
    m_bytecodeCursor = 0;
    m_sourceAllocation = 0;
    m_sourceLine = -1;
    m_allocation38 = 0;
}

RELATION::RELATION()
{
    m_first.m_count = 0;
    m_first.m_capacity = 0;
    m_first.m_data = 0;
    m_second.m_count = 0;
    m_second.m_capacity = 0;
    m_second.m_data = 0;
}

template<>
LIST<SCRIPTSTACK>::~LIST()
{
    if (m_data)
    {
        unsigned char* const allocation = reinterpret_cast<unsigned char*>(m_data)-4;
        const unsigned int count = *reinterpret_cast<unsigned int*>(allocation);
        as2::core::destroyArray(m_data, sizeof(SCRIPTSTACK), count, [](void* item)
        {
            static_cast<SCRIPTSTACK*>(item)->~SCRIPTSTACK();
        }
        );
        CrtFreePointer(allocation);
    }
    m_data = 0;
    m_count = 0;
}

template<>
LIST<NAMED_LIST_STRUCT<SCRIPTVAR> >::~LIST()
{
    if (m_data)
    {
        unsigned char* const allocation = reinterpret_cast<unsigned char*>(m_data)-4;
        const unsigned int count = *reinterpret_cast<unsigned int*>(allocation);
        as2::core::destroyArray(m_data, sizeof(NAMED_LIST_STRUCT<SCRIPTVAR>), count, [](void* item)
        {
            static_cast<NAMED_LIST_STRUCT<SCRIPTVAR>*>(item)->~NAMED_LIST_STRUCT<SCRIPTVAR>();
        }
        );
        CrtFreePointer(allocation);
    }
    m_data = 0;
    m_count = 0;
}

template<>
LIST<NAMED_LIST_STRUCT<STRING> >::~LIST()
{
    if (m_data)
    {
        unsigned char* const allocation = reinterpret_cast<unsigned char*>(m_data)-4;
        const unsigned int count = *reinterpret_cast<unsigned int*>(allocation);
        as2::core::destroyArray(m_data, sizeof(NAMED_LIST_STRUCT<STRING>), count, [](void* item)
        {
            static_cast<NAMED_LIST_STRUCT<STRING>*>(item)->~NAMED_LIST_STRUCT<STRING>();
        }
        );
        CrtFreePointer(allocation);
    }
    m_data = 0;
    m_count = 0;
}


VID* VID_LIGHT::CreateMirror()
{
    void* const memory = static_cast<unsigned char*>(CrtNew(sizeof(VID_LIGHT)));
    if (!memory) return 0;
    try
    {
        return ::new(memory) VID_LIGHT(*this);
    }
    catch (...)
    {
        CrtFreePointer(memory);
        throw;
    }
}


void MAP::UpdateGroundHeightCell(const VECTOR& point, bool temporary, bool clearOnEqual)
{
    const float x = point.x, y = point.y, z = point.z;
    const float zero = (0.0f);
    if (!(x >= zero) || !(y >= zero) || !(x<m_sizeXY.x) || !(y<m_sizeXY.y))
        return;
    const float scale = (-0.125f);
    const int row = static_cast<int>(static_cast<unsigned int>(TruncateFloatToInt32(y*scale))*static_cast<unsigned int>(m_groundWidth));
    const int rawColumn = TruncateFloatToInt32(x);
    const int column = (rawColumn+((rawColumn<0)?7:0))>>3;
    short* const grid = temporary?m_groundSecondary:m_groundPrimary;
    short* cell = grid+(column-row);
    const int zi = TruncateFloatToInt32(z);
    if (clearOnEqual)
    {
        if (static_cast<int>(*cell) == zi)
            *cell = 0;
    }
    else if (static_cast<int>(*cell)<zi)
        *cell = static_cast<short>(TruncateFloatToInt32(z));
}

LINKER::~LINKER()
{
    VID* const target = m_vid->linkVid;
    if (target)
    {
        for (SPRITE* node = m_childChain;node;node = node->ChildChain())
        {
            if (node->Animation()<15 && node->Vid() == target)
            {
                node->ChangeAnimation(15);
                break;
            }
        }
    }
    BreakLink();
}

void MAP::SetGroundZ(const VECTOR& point)
{
    UpdateGroundHeightCell(point, false, false);
}

void MAP::SetTempGroundZ(const VECTOR& point)
{
    UpdateGroundHeightCell(point, true, false);
}

void MAP::ClearTempGroundZ(const VECTOR& point)
{
    UpdateGroundHeightCell(point, true, true);
}

void MAP::RestoreDeviceObjects()
{
    for (int i = 0;i<m_vidCount;++i)
    {
        VID* const observer = m_vids[static_cast<unsigned int>(i)];
        if (observer)
            observer->RestoreDeviceObjects();
    }
    GRAPH* const graph = Graph;
    const float scale = (0.5f);
    const VECTOR2 point( m_shiftXY.x+graph->SizeXY().x*scale, m_shiftXY.y+graph->SizeXY().y*scale);
    SetShiftCoor(point, 0);
}

void RELATION::Insert(SPRITE* first, SPRITE* second)
{
    if (!first)
        return;
    if (m_first.m_count >= m_first.m_capacity)
        m_first.Expand(m_first.m_capacity*2+4);
    m_first.m_data[m_first.m_count++] = first;
    if (m_second.m_count >= m_second.m_capacity)
        m_second.Expand(m_second.m_capacity*2+4);
    m_second.m_data[m_second.m_count++] = second;
}






static int MapAbi_MapGridCoord(float value, float limit, int cells)
{
    if (value<0.0f || value != value)
        return 0;
    if (value >= limit)
        return cells-1;
    return TruncateFloatToInt32(value)/8;
}

int MAP::IsDotUnderGround(const VECTOR& point) const
{
    const int x = MapAbi_MapGridCoord(point.x, m_sizeXY.x, m_groundWidth);
    const int y = MapAbi_MapGridCoord(point.y, m_sizeXY.y, m_groundHeight);
    const int index = y*m_groundWidth+x;
    const short primaryHeight = reinterpret_cast<const short*>(m_groundPrimary)[index];
    if (static_cast<float>(primaryHeight)>point.z)
        return 1;
    const short secondaryHeight = reinterpret_cast<const short*>(m_groundSecondary)[index];
    return static_cast<float>(secondaryHeight)>point.z?1:0;
}

float MAP::GetGroundZ(const VECTOR2& point) const
{
    const int x = MapAbi_MapGridCoord(point.x, m_sizeXY.x, m_groundWidth);
    const int y = MapAbi_MapGridCoord(point.y, m_sizeXY.y, m_groundHeight);
    const int index = y*m_groundWidth+x;
    const short primaryHeight = reinterpret_cast<const short*>(m_groundPrimary)[index];
    const short secondaryHeight = reinterpret_cast<const short*>(m_groundSecondary)[index];
    return static_cast<float>(primaryHeight>secondaryHeight?primaryHeight:secondaryHeight);
}

void MAP::DeleteExtraVid()
{
    void* self = this;
    SPRITE_COLLECTOR* spriteCollector = ::Sprites;
    SPRITE* candidateSprite = spriteCollector->FirstAllSprite();
    while (candidateSprite != 0)
    {
        VID* candidateVid = candidateSprite->Vid();
        if ((candidateVid->storageFlags&2u) != 0u)
        {
            candidateSprite->~SPRITE();
            CrtFreePointer(candidateSprite);
        }
        spriteCollector = ::Sprites;
        candidateSprite = spriteCollector->NextAllSprite();
    }
    int storedVidCount = m_vidCount;
    for (int i = storedVidCount-1;i >= 0;--i)
    {
        VID* storedVid = m_vids[static_cast<unsigned int>(i)];
        if (storedVid != 0 && (storedVid->storageFlags&2u) != 0u)
        {
            storedVid->~VID();
            CrtFreePointer(storedVid);
            m_vids[static_cast<unsigned int>(i)] = 0;
        }
    }
    while (m_vidCount>0)
    {
        const int trailingVidCount = m_vidCount;
        if (m_vids[static_cast<unsigned int>(trailingVidCount-1)] != 0)
            break;
        --m_vidCount;
    }
}

static void MapAbi_ZeroBytes(void* memory, unsigned int byteCount)
{
    unsigned char* destinationBytes = static_cast<unsigned char*>(memory);
    for (unsigned int i = 0;i<byteCount;++i)
        destinationBytes[i] = 0u;
}

void MAP::ResetGroundZ()
{
    void* self = this;
    void* first = m_groundPrimary;
    if (first != 0)
        CrtFreePointer(first);
    void* second = m_groundSecondary;
    if (second != 0)
        CrtFreePointer(second);
    const int width = TruncateFloatToInt32(m_sizeXY.x+(7.0f))/8;
    const int height = TruncateFloatToInt32(m_sizeXY.y+(7.0f))/8;
    m_groundWidth = width;
    m_groundHeight = height;
    const unsigned int bytes = static_cast<unsigned int>(width*height*2);
    first = static_cast<unsigned char*>(CrtNew(bytes));
    m_groundPrimary = static_cast<short*>(first);
    second = static_cast<unsigned char*>(CrtNew(bytes));
    m_groundSecondary = static_cast<short*>(second);
    MapAbi_ZeroBytes(first, bytes);
    MapAbi_ZeroBytes(second, bytes);
}

const STRING FCurrentDirectory()
{
    char buffer[4096];
    const unsigned int length = GetCurrentDirectoryA(4096u, buffer);
    const char* const source = length != 0u ? buffer : StringGlobals::EmptyText();
    return STRING(source);
}

void MAP::ExchangeVid(VID* firstVid, VID* secondVid)
{
    VID* first = firstVid;
    VID* second = secondVid;
    if (first == 0 || second == 0 || first == second)
        return;
    VID* globalObject = ::EmptyVid;
    if (first == globalObject || second == globalObject)
        return;
    const int count = m_vidCount;
    for (int i = 0;i<count;++i)
    {
        VID* storedVid = m_vids[static_cast<unsigned int>(i)];
        if (storedVid == 0)
            continue;
        if (storedVid->linkVid == first)
            storedVid->linkVid = second;
        else if (storedVid->linkVid == second)
            storedVid->linkVid = first;
        for (unsigned int child = 0;child<VID::AnimationSlotCount;++child)
        {
            if (storedVid->childVid[child] == first)
                storedVid->childVid[child] = second;
            else if (storedVid->childVid[child] == second)
                storedVid->childVid[child] = first;
        }
    }
    const unsigned int firstIndex = static_cast<unsigned int>(first->nVid);
    const unsigned int secondIndex = static_cast<unsigned int>(second->nVid);
    m_vids[firstIndex] = second;
    m_vids[secondIndex] = first;
    VID* vidValue = second->exchangedVid;
    second->exchangedVid = first->exchangedVid;
    first->exchangedVid = vidValue;
    const int nVidValue = second->nVid;
    second->nVid = first->nVid;
    first->nVid = nVidValue;
    for (unsigned int i = 0;i<VID::ScriptFunctionCount;++i)
    {
        const int function = second->scriptFunction[i];
        second->scriptFunction[i] = first->scriptFunction[i];
        first->scriptFunction[i] = function;
    }
}

template<>
STRING NAMED_LIST<SCRIPTVAR>::Name(int index) const
{
    if (index >= 0 && index<this->m_count)
        return STRING(this->m_data[index].m_name);
    return STRING(StringGlobals::EmptyText());
}

SCRIPT::~SCRIPT()
{
    Release();
}

LINKER::LINKER(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* linked)
    : SPRITE(vid, position, direction, linked),
      m_linkOffset(0.0f, 0.0f), m_linkDirection(direction), m_linkedSprite(linked)
{
    if (linked)
    {
        linked->AddLink(this);
        m_linkOffset.x = m_xyz.x-linked->Coor().x;
        m_linkOffset.y = (m_xyz.y-linked->Coor().y)*(1.41430664f);
    }
}

STRING STRING::ToLower() const
{
    STRING converted(value);
    (void)CrtLowercase(PtrToU32(converted.value));
    return STRING(converted);
}

STRING MAP::GetMouseTipsString() const
{
    STRING tipsText(StringGlobals::EmptyText());
    STRING selectedTipsText;
    const unsigned int index = m_playerIndex;
    PLAYER* const playerController = m_players[index];
    selectedTipsText = playerController->GetMouseTipsString();
    tipsText = selectedTipsText;
    return tipsText;
}



float MAP::GetGroundZScr(VECTOR2 point) const
{
    const float x = point.x;
    const float y = point.y;
    const float zero = (0.0f);
    const float one = (1.0f);
    const float scale = (0.125f);
    float clampedX = x;
    if (clampedX<zero)
        clampedX = zero;
    else if (clampedX >= m_sizeXY.x)
        clampedX = m_sizeXY.x-one;
    float clampedY = y;
    if (clampedY<zero)
        clampedY = zero;
    else if (clampedY >= m_sizeXY.y)
        clampedY = m_sizeXY.y-one;
    const int baseY = TruncateFloatToInt32(clampedY*scale);
    const int xIndex = TruncateFloatToInt32(clampedX*scale);
    const int width = m_groundWidth;
    const int height = m_groundHeight;
    int row = baseY+32;
    if (row >= height)
        row = height-1;
    int index = width*row+xIndex;
    const short* first = m_groundPrimary;
    const short* second = m_groundSecondary;
    for (;row >= baseY;--row, index -= width)
    {
        const int threshold = (row-baseY)*8;
        if (static_cast<int>(first[index]) >= threshold || static_cast<int>(second[index]) >= threshold)
            return static_cast<float>(threshold);
    }
    return zero;
}

STRING MAP::OpenSaveDialog(int saveDialog, const char* filter, const char* defaultExtension) const
{
    MapOpenFileName ofn =
    {
    }
    ;
    char name[4096];
    for (unsigned int i = 0;i<sizeof(name);++i)
        name[i] = 0;
    name[0] = *reinterpret_cast<const unsigned char*>(StringGlobals::EmptyText());
    ofn.size = 76u;
    ofn.ownerWindow = m_dialog;
    ofn.instance = m_hInstance;
    ofn.filter = filter;
    ofn.filterIndex = 1u;
    ofn.file = name;
    ofn.maximumFile = 4096u;
    ofn.initialDirectory = "maps";
    ofn.flags = saveDialog?0x0008080Cu:0x0008180Cu;
    ofn.defaultExtension = defaultExtension;
    Graph->BeginPause();
    const unsigned int ok = saveDialog ?GetSaveFileNameA(reinterpret_cast<OPENFILENAMEA*>(&ofn)) :GetOpenFileNameA(reinterpret_cast<OPENFILENAMEA*>(&ofn));
    Graph->EndPause();
    if (ok) return STRING(name);
    return STRING();
}

int MAP::IsLineUnderGround(const VECTOR& fromPoint, const VECTOR& toPoint) const
{
    VECTOR point = fromPoint;
    const double dx = static_cast<double>(toPoint.x)-fromPoint.x;
    const float dy = toPoint.y-fromPoint.y;
    const double dz = static_cast<double>(toPoint.z)-fromPoint.z;
    double dominant = fabs(dx);
    const float absoluteY = static_cast<float>(fabs(dy));
    if (!(dominant>absoluteY))
        dominant = absoluteY;
    const double stepExtent = dominant*static_cast<double>(0.0833333358f);
    const int steps = TruncateDoubleToInt32(stepExtent);
    if (steps == 0)
        return IsDotUnderGround(point);
    const double inverseExtent = 1.0/stepExtent;
    const float stepX = static_cast<float>(dx*inverseExtent);
    const float stepY = static_cast<float>(static_cast<double>(dy)*inverseExtent);
    const float stepZ = static_cast<float>(dz*inverseExtent);
    int hit = 0;
    for (int remaining = steps-1;remaining >= 0;--remaining)
    {
        point.x += stepX;
        point.y += stepY;
        point.z += stepZ;
        hit = IsDotUnderGround(point);
        if (hit != 0)
            break;
    }
    if (hit == 0)
        return 0;
    const double absoluteX = fabs(static_cast<double>(point.x)-fromPoint.x);
    const float displacementY = point.y-fromPoint.y;
    const float displacementZ = point.z-fromPoint.z;
    const double absoluteHitY = fabs(displacementY);
    const double absoluteHitZ = fabs(displacementZ);
    const double weight = static_cast<double>(0.398000002f);
    double distance = absoluteX>absoluteHitY ?absoluteX+absoluteHitY*weight:absoluteHitY+absoluteX*weight;
    distance = distance>absoluteHitZ ?distance+absoluteHitZ*weight:absoluteHitZ+distance*weight;
    const int result = TruncateDoubleToInt32(distance);
    return result != 0?result:1;
}

void MAP::SetKeyboardLocale(const STRING& locale) const
{
    for (int index = 0;index<16;++index)
    {
        char layoutNameBuffer[512];
        (void)GetKeyboardLayoutNameA(layoutNameBuffer);
        STRING layoutName(layoutNameBuffer);
        layoutName.RemoveBeginChars("0");
        const char* layoutNameCursor = layoutName.value;
        const char* requestedLocaleCursor = locale.m_value;
        while (*layoutNameCursor == *requestedLocaleCursor && *layoutNameCursor)
        {
            ++layoutNameCursor;
            ++requestedLocaleCursor;
        }
        if ((unsigned char)*layoutNameCursor == (unsigned char)*requestedLocaleCursor)
            break;
        (void)ActivateKeyboardLayout(reinterpret_cast<HKL>(1u), 8u);
    }
}

int MAP::StartTact()
{
    unsigned int flags = m_flags&0xFFBFFFFFu;
    m_flags = flags;
    if ((flags&0x20u) == 0u && (::CurrentTime&7u) == 0u)
    {
        SPRITE_COLLECTOR* const collector = ::Sprites;
        collector->OptimizeForDraw();
    }
    flags = m_flags;
    if ((flags&0x80u) != 0u)
    {
        m_flags = flags&~0x80u;
        Load(m_pendingMapFileName);
    }
    unsigned int firstNow;
    unsigned int previous;
    unsigned int delta;
    do
    {
        firstNow = timeGetTime();
        previous = ::RealCurrentTime;
        delta = firstNow-previous;
    }
    while (delta == 0u);
    ::PrevRealCurrentTime = previous;
    ::RealCurrentTime = timeGetTime();
    ::PrevCurrentTime = ::CurrentTime;
    if (delta>71u)
        delta = 71u;
    const int scaledDelta = TruncateDoubleToInt32(static_cast<double>(delta)*static_cast<double>(m_scale40));
    ::CurrentTime = ::CurrentTime+static_cast<unsigned int>(scaledDelta);
    unsigned int frameCounter = m_frameCounter08+1u;
    m_frameCounter08 = frameCounter;
    const unsigned int now = ::RealCurrentTime;
    if (now-frameRateSampleTick >= 1000u)
    {
        frameRateSampleTick = now;
        m_frameRate04 = frameCounter;
        m_frameCounter08 = 0u;
    }
    if (m_players[0] != 0)
        ControlShiftCoor();
    INPUT* const input = &m_input;
    input->Tact();
    if (input->GetAction(IA_BOSS) != 0)
        m_flags ^= 0x01000000u;
    MSG message;
    while (PeekMessageA(&message, 0, 0u, 0u, PM_REMOVE) != 0)
    {
        if (message.message == WM_QUIT)
            return 1;
        bool handled = false;
        const unsigned int dialog = m_dialog;
        if (dialog != 0u)
        {
            GRAPH* const graph = Graph;
            if ((graph->capabilityFlags&1u) == 0u)
            {
                const unsigned int dialogWindow = m_dialogWindow;
                handled = TranslateAcceleratorA(reinterpret_cast<HWND>(U32ToPtr(dialog)), reinterpret_cast<HACCEL>(U32ToPtr(dialogWindow)), &message) != 0;
            }
        }
        if (!handled)
        {
            (void)TranslateMessage(&message);
            (void)DispatchMessageA(&message);
        }
    }
    if (m_simulationFrameCount == 0u)
        input->Reset();
    return static_cast<int>((m_flags>>24u)&1u);
}

void MAP::LoadWeapon(RESOURCE* resource)
{
    WEAPON_RUNTIME* const previousWeaponData = m_weaponData;
    if (previousWeaponData)
        CrtFreePointer(previousWeaponData);
    m_weaponData = 0;
    void* serializedBuffer = 0;
    const unsigned int weaponCount = static_cast<unsigned int>(resource->Load(ResourceType::Weapons, &serializedBuffer, 0x280u));
    m_weaponCount = static_cast<int>(weaponCount);
    if (weaponCount == 0u || serializedBuffer == 0)
    {
        VID* const emptyVid = ::EmptyVid;
        emptyVid->weapon = m_weaponData;
        return;
    }
    const unsigned int runtimeRecords = PtrToU32(CrtNew(weaponCount*sizeof(WEAPON_RUNTIME)));
    m_weaponData = reinterpret_cast<WEAPON_RUNTIME*>(U32ToPtr(runtimeRecords));
    const WEAPON_SERIALIZED* const serialized = static_cast<const WEAPON_SERIALIZED*>(serializedBuffer);
    const float kSerializedScale = 0.0010000000474974513f;
    const unsigned int kUnscaledSentinel = 0x497423F0u;
    for (unsigned int record = 0u;record<weaponCount;++record)
    {
        WEAPON_RUNTIME& runtimeWeapon = m_weaponData[record];
        const WEAPON_SERIALIZED& serializedWeapon = serialized[record];
        unsigned char* const runtimeBytes = reinterpret_cast<unsigned char*>(&runtimeWeapon);
        const unsigned char* const serializedBytes = reinterpret_cast<const unsigned char*>(&serializedWeapon);
        for (unsigned int i = 0u;i<540u;++i)
            runtimeBytes[i] = serializedBytes[i];
        runtimeWeapon.maxColumn = 7;
        for (unsigned int direction = 0u;direction<8u;++direction)
        {
            runtimeWeapon.Scale[direction] = VECTOR( serializedWeapon.ScaleX[direction], serializedWeapon.ScaleY[direction], serializedWeapon.ScaleZ[direction]);
            runtimeWeapon.Shift[direction] = VECTOR( serializedWeapon.ShiftX[direction], serializedWeapon.ShiftY[direction], serializedWeapon.ShiftZ[direction]);
            runtimeWeapon.DirectionX[direction] = static_cast<unsigned char>(serializedWeapon.DirectionX[direction]);
            runtimeWeapon.DirectionY[direction] = static_cast<unsigned char>(serializedWeapon.DirectionY[direction]);
            runtimeWeapon.DirectionZ[direction] = static_cast<unsigned char>(serializedWeapon.DirectionZ[direction]);
            runtimeWeapon.FrameSpeed[direction] = serializedWeapon.FrameSpeed[direction];
            const union
            {
                float f;
                unsigned int u;
            }
            firstLaneBits =
            {
                serializedWeapon.Speed[direction]
            }
            ;
            const union
            {
                float f;
                unsigned int u;
            }
            secondLaneBits =
            {
                serializedWeapon.ZSpeed[direction]
            }
            ;
            float firstScaledLane = firstLaneBits.f;
            float secondScaledLane = secondLaneBits.f;
            if (firstLaneBits.u != kUnscaledSentinel)
                firstScaledLane *= kSerializedScale;
            if (secondLaneBits.u != kUnscaledSentinel)
                secondScaledLane *= kSerializedScale;
            runtimeWeapon.Speed[direction] = firstScaledLane;
            runtimeWeapon.ZSpeed[direction] = secondScaledLane;
        }
    }
    CrtFreePointer(serializedBuffer);
    VID* const emptyVid = ::EmptyVid;
    emptyVid->weapon = m_weaponData;
}

static void MapReloadVidDiagnostic(const char* subjectText, int category, int errorCode)
{
    STRING subject;
    subject = subjectText;
    MYERROR* sink = ::Error;
    if (sink != 0)
    {
        STRING objectFormat;
        objectFormat = "MAP";
        sink->Error(objectFormat, static_cast<TYPE_ERROR>(category), subject, errorCode);
    }
}

void MAP::ReloadVid()
{
    RESOURCE mapResource;
    RESOURCE* const resource = &mapResource;
    for (int index = 0;index<m_vidCount;++index)
    {
        VID* storedVid = m_vids[static_cast<unsigned int>(index)];
        if (storedVid == 0)
            continue;
        VID* linked = storedVid->exchangedVid;
        if (linked != storedVid)
            ExchangeVid(linked, storedVid);
    }
    STRING* const resourcePath = &m_resourceFileName;
    if (resource->OpenForRead(*resourcePath, ResourceType::Data) != 0)
    {
        MapReloadVidDiagnostic( "resource file", 7, 0);
        return;
    }
    LoadWeapon(resource);
    if (resource->GoBegin(ResourceType::Object) != 0u)
    {
        MapReloadVidDiagnostic( "load 'VID'", 11, 0);
        return;
    }
    do
    {
        int nvid = 0;
        (void)resource->Read(&nvid, 4u);
        if (nvid >= VidSlotCount)
        {
            MapReloadVidDiagnostic( "nvid > MAX_VID", 4, nvid);
        }
        VID* storedVid = m_vids[static_cast<unsigned int>(nvid)];
        if (storedVid != 0 && storedVid->sourceVid == 0)
        {
            VID* parentVid = storedVid->exchangedVid;
            nvid = parentVid->nVid;
            storedVid = m_vids[static_cast<unsigned int>(nvid)];
            storedVid->name.Read( resource);
            storedVid->LoadParameters(resource);
            const int weaponIndex = storedVid->nWeapon;
            WEAPON_RUNTIME* const weaponBase = m_weaponData;
            if (weaponIndex<m_weaponCount)
                storedVid->weapon = weaponBase+static_cast<unsigned int>(weaponIndex);
            else storedVid->weapon = weaponBase;
        }
    }
    while (resource->GoNextSub(ResourceType::Object) == 0);
    for (int index = 0;index<m_vidCount;++index)
    {
        VID* storedVid = m_vids[static_cast<unsigned int>(index)];
        if (storedVid != 0 && storedVid->sourceVid == 0 && (storedVid->storageFlags&2u) == 0u)
            storedVid->SetChildAndLink();
    }
    resource->Close();
}

void MAP::NetworkTact()
{
    NET* net = ::Net;
    if ((net->m_flags&0x48u) != 0u)
    {
        net->Tact();
        net = ::Net;
        m_playerIndex = net->m_localPlayer;
        net = ::Net;
    }
    if ((net->m_flags&0x10u) == 0u || (static_cast<unsigned char>(m_flags)&0x80u) != 0u)
        return;
    SetupProcessorFlags();
    const unsigned int currentIndex = m_playerIndex;
    PLAYER* const current = m_players[currentIndex];
    INPUT* const currentInput = &current->m_input;
    const unsigned char* const mapInput = reinterpret_cast<const unsigned char*>(&m_input);
    unsigned char* const playerInput = reinterpret_cast<unsigned char*>(currentInput);
    for (unsigned int i = 0;i<39u;++i)
        playerInput[i] = mapInput[i];
    m_flags &= 0xFF7FFFFFu;
    net = ::Net;
    if (net->m_noPlayers <= 1u)
        return;
    currentInput->m_networkDeltaTicks = static_cast<unsigned char>(::CurrentTime-::PrevCurrentTime);
    const unsigned char randomState = static_cast<unsigned char>(Random(255));
    m_players[m_playerIndex]->m_input.m_randomSyncValue = randomState;
    m_players[m_playerIndex]->m_input.m_networkControlFlags = *reinterpret_cast<const unsigned char*>(&m_pendingNetworkPauseCommand);
    net = ::Net;
    (void)net->Send(&m_players[m_playerIndex]->m_input, 39u, 0u, -1);
    *reinterpret_cast<unsigned char*>(&m_pendingNetworkPauseCommand) = 0u;
    unsigned int maxDelta = 0u;
    net = ::Net;
    for (unsigned int player = 0;player<8u;++player)
    {
        if (net->m_players[player].m_dpnid == 0u)
            continue;
        if (m_simulationFrameCount<2u)
        {
            m_players[player]->m_input.Reset();
            net = ::Net;
            continue;
        }
        while (net->Receive(&m_players[player]->m_input, 39u, static_cast<int>(player)) != 0)
        {
            Sleep(0u);
            net = ::Net;
            DirectPlayReconcilePlayerRecords(net);
            net = ::Net;
            if ((net->m_flags&0x10u) == 0u)
                return;
            if (net->m_players[player].m_dpnid == 0u)
                break;
        }
        net = ::Net;
        if (net->m_players[player].m_dpnid == 0u)
            continue;
        PLAYER* const controller = m_players[player];
        INPUT* const input = &controller->m_input;
        const unsigned char delta = input->m_networkDeltaTicks;
        if (maxDelta<static_cast<unsigned int>(delta))
            maxDelta = delta;
        const unsigned char state = input->m_networkControlFlags;
        if ((state&3u) != 0u)
        {
            controller->m_controlPauseState = static_cast<unsigned int>((state&3u)-1u);
            net = ::Net;
        }
        if (net->m_players[player].m_dpnid == 0u)
            continue;
        const PLAYER* const firstController = m_players[0];
        if (input->m_randomSyncValue != firstController->m_input.m_randomSyncValue)
        {
            m_flags |= 0x00800000u;
            if (networkRandomMismatchReported == 0u)
            {
                ::Error->Log("Synchro rand error PrevCurrentTime=%u player[%i]", ::PrevCurrentTime, player);
            }
            networkRandomMismatchReported = 1;
        }
        net = ::Net;
    }
    const unsigned int delta = (m_simulationFrameCount<2u)?1u:maxDelta;
    ::CurrentTime = ::PrevCurrentTime+delta;
    for (unsigned int player = 0;player<8u;++player)
    {
        const PLAYER* const controller = m_players[player];
        const unsigned char state = controller->m_input.m_networkControlFlags;
        if ((state&3u) != 0u)
        {
            ::Error->Log("NET Pause(%i) Player[%i] %u", static_cast<unsigned int>((state&3u)-1u), player, ::CurrentTime);
        }
    }
}

SPRITE* MAP::OldLoadSprite(STREAM* inputStream)
{
    SPRITE* persistentId;
    (void)inputStream->Read(&persistentId, 4u);
    if (PtrToU32(persistentId) == 0xFFFFFFFFu)
        return reinterpret_cast<SPRITE*>(U32ToPtr(0xFFFFFFFFu));
    short vidIndex;
    short x;
    short y;
    short z;
    unsigned char direction;
    unsigned char legacyFlags;
    (void)inputStream->Read(&vidIndex, 2u);
    (void)inputStream->Read(&x, 2u);
    (void)inputStream->Read(&y, 2u);
    (void)inputStream->Read(&z, 2u);
    (void)inputStream->Read(&direction, 1u);
    (void)inputStream->Read(&legacyFlags, 1u);
    (void)legacyFlags;
    SPRITE* created = 0;
    bool vidExists = false;
    const int index = static_cast<int>(vidIndex);
    if (index >= 0 && index<m_vidCount)
    {
        VID* baseVid = m_vids[static_cast<unsigned int>(index)];
        if (baseVid != 0)
        {
            vidExists = true;
            VECTOR position;
            position.x = static_cast<float>(x);
            position.y = static_cast<float>(y);
            position.z = static_cast<float>(z);
            created = this->CreateSprite(baseVid, position, ANGLE(static_cast<unsigned char>(direction)), 0);
        }
    }
    if (!vidExists && g_iZmDebug == 0u)
    {
        MYERROR* sink = ::Error;
        if (sink != 0)
        {
            STRING subject;
            STRING owner;
            subject = "sprite, this vid not exist";
            owner = "MAP";
            sink->Error(owner, E_CREATE, subject, index);
        }
    }
    m_relation.Insert( persistentId, created);
    return created;
}

void MAP::CopyVid(VID* dstVid, VID* srcVid)
{
    VID* const dst = dstVid;
    VID* const src = srcVid;
    VID* const nullVid = ::EmptyVid;
    if (dst == src || dst == 0 || src == 0 || dst == nullVid || src == nullVid || (dst->type&0x40u) == 0u || (src->type&0x40u) == 0u)
    {
        STRING subject("Can't copy vid to vid");
        if (::Error != 0)
        {
            STRING owner("MAP");
            ::Error->Error(owner, E_ERROR, subject, 0u);
        }
        return;
    }
    VID* const previousLink = dst->sourceVid;
    SPRITE* staged[256];
    int stagedCount = 0;
    if (previousLink != src)
    {
        for (unsigned int lane = 0;lane<8u;++lane)
        {
            PLAYER* const laneOwner = m_players[lane];
            SPRITE* sprite = laneOwner->m_flagman.m_value;
            while (sprite != 0 && stagedCount<256)
            {
                if (sprite->Vid() == dst)
                    staged[stagedCount++] = sprite;
                sprite = sprite->ChildChain();
            }
        }
        unsigned int expectedCount = 0u;
        for (unsigned int lane = 0;lane<8u;++lane)
            expectedCount += dst->spriteCountsByPlayer[lane];
        if (expectedCount != static_cast<unsigned int>(stagedCount))
        {
            stagedCount = 0;
            SPRITE* sprite = ::Sprites->FirstAllSprite();
            while (sprite != 0 && stagedCount<256)
            {
                if (sprite->Vid() == dst)
                    staged[stagedCount++] = sprite;
                sprite = ::Sprites->NextAllSprite();
            }
        }
        if (stagedCount>0)
        {
            for (int i = 0;i<stagedCount;++i)
                (void)::Sprites->Delete(staged[i]);
        }
    }
    reinterpret_cast<VID_SURFACE*>(dst)->CopyFromVid(reinterpret_cast<VID_SURFACE*>(src));
    if (previousLink != src && stagedCount>0)
    {
        for (int i = 0;i<stagedCount;++i)
        {
            SPRITE* const sprite = staged[i];
            ::Sprites->Insert(sprite);
            const unsigned int state = static_cast<unsigned int>(sprite->Animation());
            (void)sprite->ChangeAnimation(static_cast<int>(state^1u));
            (void)sprite->ChangeAnimation(static_cast<int>(state));
        }
    }
}

static void MapParseObjectLoadListRange(const char* source, int* loadStates, unsigned int stateCountLimit)
{
    std::FILE* file = 0;
    {
        const STRING mode("rt");
        const STRING sourceName(source);
        if (sourceName.value[0] != 0)
            file = std::fopen(sourceName.value, mode.value);
    }
    if (!file)
        return;
    char line[256];
    while ((std::feof(file)?0x10u:0u) == 0u)
    {
        if (std::fgets(line, static_cast<int>(256u), file) == 0 || line[0] == '\0')
            continue;
        unsigned int lineLength = 0u;
        while (line[lineLength] != '\0')
            ++lineLength;
        while (lineLength != 0u && (line[lineLength-1u] == '\n' || line[lineLength-1u] == '\r'))
            line[--lineLength] = '\0';
        char* comment = std::strchr(line, ';');
        if (comment)
            *comment = '\0';
        if (line[0] == '\0')
            continue;
        char* openSquare = std::strchr(line, '[');
        char* closeSquare = std::strchr(line, ']');
        char* openAngle = std::strchr(line, '<');
        char* closeAngle = std::strchr(line, '>');
        char* dash = std::strchr(line, '-');
        if (openAngle && closeAngle && openAngle<closeAngle)
        {
            *closeAngle = '\0';
            MapParseObjectLoadListRange(openAngle+1, loadStates, stateCountLimit);
            continue;
        }
        const int requestedState = (line[0] == '+')?1:0;
        int first = 0, last = 0;
        if (openSquare && closeSquare && dash && openSquare<closeSquare)
        {
            ++openSquare;
            *dash = '\0';
            first = BitsToSigned32(CrtAtoiText(openSquare));
            ++dash;
            *closeSquare = '\0';
            last = BitsToSigned32(CrtAtoiText(dash));
        }
        else
        {
            first = last = BitsToSigned32(CrtAtoiText(line));
        }
        if (first <= last)
        {
            int* stateEntry = loadStates+first;
            for (int index = first;index <= last;++index, ++stateEntry)
                if (index >= 0 && index<static_cast<int>(stateCountLimit))*stateEntry = requestedState;
        }
    }
    (void)static_cast<unsigned int>(std::fclose(file));
}

SPRITE* MAP::LoadSprite(STREAM* inputStream, int version)
{
    SPRITE* persistentId;
    (void)inputStream->Read(&persistentId, 4u);
    if (PtrToU32(persistentId) == 0xFFFFFFFFu)
        return reinterpret_cast<SPRITE*>(U32ToPtr(0xFFFFFFFFu));
    int vidIndex;
    (void)inputStream->Read(&vidIndex, 4u);
    VECTOR position;
    unsigned int serializedValue = PtrToU32(inputStream);
    if (version>9)
    {
        (void)inputStream->Read(&position.x, 4u);
        (void)inputStream->Read(&position.y, 4u);
        (void)inputStream->Read(&position.z, 4u);
    }
    else
    {
        (void)inputStream->Read(&serializedValue, 4u);
        position.x = static_cast<float>(BitsToSigned32(serializedValue));
        (void)inputStream->Read(&serializedValue, 4u);
        position.y = static_cast<float>(BitsToSigned32(serializedValue));
        (void)inputStream->Read(&serializedValue, 4u);
        position.z = static_cast<float>(BitsToSigned32(serializedValue));
    }
    (void)inputStream->Read(&serializedValue, 4u);
    const unsigned char direction = static_cast<unsigned char>(serializedValue);
    if ((mapSpriteCreationInitialized&1u) == 0u)
    {
        ADD_CREATE_SPRITE* const context = new (mapSpriteCreationContext.Address()) ADD_CREATE_SPRITE;
        context->gamma.modulate = 0u;
        mapSpriteCreationInitialized |= 1u;
        context->gamma.add = 0u;
        (void)RegisterCrtAtExit(&MapCompilerCleanupNoOp);
    }
    ADD_CREATE_SPRITE* const creation = mapSpriteCreationContext.Address();
    ::AddCreateSprite = creation;
    (void)inputStream->Read(&creation->group, 4u);
    if (version >= 16)
    {
        GAMMA* const gamma = &::AddCreateSprite->gamma;
        (void)inputStream->Read(gamma, 8u);
        VECTOR* const scale = &::AddCreateSprite->scale;
        (void)inputStream->Read(&scale->x, 4u);
        (void)inputStream->Read(&scale->y, 4u);
        (void)inputStream->Read(&scale->z, 4u);
    }
    else
    {
        GAMMA* const gamma = &::AddCreateSprite->gamma;
        gamma->modulate = 0u;
        gamma->add = 0u;
        VECTOR* const scale = &::AddCreateSprite->scale;
        scale->x = 1.0f;
        scale->y = 1.0f;
        scale->z = 1.0f;
    }
    SPRITE* created = 0;
    bool vidExists = false;
    if (vidIndex >= 0 && vidIndex<m_vidCount)
    {
        VID* vid = m_vids[static_cast<unsigned int>(vidIndex)];
        if (vid != 0)
        {
            vidExists = true;
            created = this->CreateSprite(vid, position, ANGLE(static_cast<unsigned char>(direction)), 0);
        }
    }
    if (!vidExists && g_iZmDebug == 0u)
    {
        MYERROR* sink = ::Error;
        if (sink != 0)
        {
            STRING subject;
            STRING owner;
            subject = "can't load sprite, this vid not exist";
            owner = "MAP";
            sink->Error(owner, E_ERROR, subject, vidIndex);
        }
    }
    ::AddCreateSprite = 0;
    m_relation.Insert( persistentId, created);
    return created;
}

SPRITE* MAP::CreateSprite(VID* definition, const VECTOR& location, ANGLE direction, SPRITE* parentSprite)
{
    if (!definition || definition == ::EmptyVid)
        return 0;
    if ((definition->runtimeFlags&0x20u) != 0u)
        definition = REGION::ConvertVid(definition, location);
    SPRITE* created = 0;
    void* storage = 0;
    try
    {
        switch (definition->spriteClass)
        {
        case 5:
            storage = CrtNew(sizeof(CANNON));
            if (storage)
                created = new(storage) CANNON(definition, location, direction, parentSprite);
            break;
        case 9:
            storage = CrtNew(sizeof(SPRITE));
            if (storage)
                created = new(storage) SPRITE(definition, location, direction, parentSprite);
            break;
        case 2:
            storage = CrtNew(sizeof(UNIT));
            if (storage)
                created = new(storage) UNIT(definition, location, direction, parentSprite);
            break;
        case 4:
            storage = CrtNew(sizeof(PLANE));
            if (storage)
                created = new(storage) PLANE(definition, location, direction, parentSprite);
            break;
        case 10:
            storage = CrtNew(sizeof(FRAME));
            if (storage)
                created = new(storage) FRAME(definition, location, direction, parentSprite);
            break;
        case 12:
            storage = CrtNew(sizeof(LINKER));
            if (storage)
                created = new(storage) LINKER(definition, location, direction, parentSprite);
            break;
        case 19:
            storage = CrtNew(sizeof(STEXT));
            if (storage)
                created = new(storage) STEXT(definition, location, direction, parentSprite);
            break;
        case 6:
            storage = CrtNew(sizeof(PRIMITIVE));
            if (storage)
                created = new(storage) PRIMITIVE(definition, location, direction, parentSprite);
            break;
        case 23:
            storage = CrtNew(sizeof(REGION));
            if (storage)
                created = new(storage) REGION(definition, location, direction, parentSprite);
            break;
        case 7:
            storage = CrtNew(sizeof(MAN));
            if (storage)
                created = new(storage) MAN(definition, location, direction, parentSprite);
            break;
        case 8:
            storage = CrtNew(sizeof(BUILDED_TERRAIN));
            if (storage)
                created = new(storage) BUILDED_TERRAIN(definition, location, direction, parentSprite);
            break;
        case 21:
            storage = CrtNew(sizeof(ENGINE));
            if (storage)
                created = new(storage) ENGINE(definition, location, direction, parentSprite);
            break;
        case 0:
        case 1:
            storage = CrtNew(sizeof(TERRAIN));
            if (storage)
                created = new(storage) TERRAIN(definition, location, direction, parentSprite);
            break;
        default:
            {
                STRING subject("sprite - Behave is invalidate");
                const unsigned int detail = definition->spriteClass;
                if (::Error)
                {
                    STRING owner("MAP");
                    ::Error->Error(owner, E_CREATE, subject, detail);
                }
                return 0;
            }
        }
    }
    catch (...)
    {
        if (storage)
            CrtFreePointer(storage);
        throw;
    }
    if ((m_flags&0x42u) == 0u && created)
    {
        const int function = created->Vid()->scriptFunction[14];
        if (function >= 0)
            (void)ScriptRun(function, created, parentSprite, 0);
    }
    return created;
}


struct MapAbi_GroundScanState
{
    int highest;
    int accepted;
};

static int MapAbi_MapGroundGridIndex(float coordinate, float limit, int cellCount)
{
    if (!(coordinate >= 0.0f))
        return 0u;
    if (!(coordinate<limit))
        return cellCount-1u;
    return TruncateFloatToInt32(coordinate)/8;
}

static void MapAbi_MapGroundAccumulate(int& highest, int& accepted, int sample)
{
    int difference;
    if (sample>highest)
    {
        difference = sample-highest;
        highest = sample;
    }
    else
    {
        difference = highest-sample;
    }
    if (difference<7)
        accepted = highest;
}

void MAP::ScanGroundHeightRect(float minX, float minY, float maxX, float maxY,
    int& highest, int& accepted) const
{
    const int width = m_groundWidth;
    const int height = m_groundHeight;
    const int x0 = MapAbi_MapGroundGridIndex(minX, m_sizeXY.x, width);
    const int x1 = MapAbi_MapGroundGridIndex(maxX, m_sizeXY.x, width);
    const int y0 = MapAbi_MapGroundGridIndex(minY, m_sizeXY.y, height);
    const int y1 = MapAbi_MapGroundGridIndex(maxY, m_sizeXY.y, height);
    if (y0>y1 || x0>x1)
        return;
    const short* const primary = m_groundPrimary;
    const short* const secondary = m_groundSecondary;
    for (int y = y0;y <= y1;++y)
    {
        const unsigned int row = static_cast<unsigned int>(y)*static_cast<unsigned int>(width);
        for (int x = x0;x <= x1;++x)
        {
            MapAbi_MapGroundAccumulate(highest, accepted, static_cast<int>(primary[row+x]));
            MapAbi_MapGroundAccumulate(highest, accepted, static_cast<int>(secondary[row+x]));
        }
    }
}

float MAP::GetGroundZ(const VID* vid, const VECTOR2& position, ANGLE direction) const
{
    const bool directional = (vid->property&0x100u) != 0u;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float halfX;
    float halfY;
    if (directional)
    {
        const double directionRadius = static_cast<double>(vid->realSizeXYZ.y)*0.27000001072883606f;
        offsetX = static_cast<float>(-(directionRadius*direction.Sin()));
        offsetY = static_cast<float>(directionRadius*direction.CosY());
        halfX = vid->realSizeXYZ.x*0.60000002384185791f;
        halfY = vid->realSizeXYZ.y*0.30000001192092896f;
    }
    else
    {
        halfX = vid->sizeXYZ.x*0.5f;
        halfY = vid->sizeXYZ.y*0.5f;
    }
    if (halfX>3.0f)
        halfX -= 3.0f;
    if (halfY>3.0f)
        halfY -= 3.0f;
    MapAbi_GroundScanState scan =
    {
        -999999, -999999
    }
    ;
    const double centerX = static_cast<double>(position.x)+offsetX;
    const float centerY = position.y+offsetY;
    ScanGroundHeightRect(static_cast<float>(centerX-halfX), centerY-halfY, static_cast<float>(centerX+halfX), centerY+halfY, scan.highest, scan.accepted);
    if (directional)
    {
        const double oppositeX = static_cast<double>(position.x)-offsetX;
        const float oppositeY = position.y-offsetY;
        ScanGroundHeightRect(static_cast<float>(oppositeX-halfX), oppositeY-halfY, static_cast<float>(oppositeX+halfX), oppositeY+halfY, scan.highest, scan.accepted);
    }
    return static_cast<float>((scan.accepted != -999999)?scan.accepted:scan.highest);
}

void MAP::Release()
{
    for (int i = 0;i<m_vidCount;++i)
    {
        VID* const vid = m_vids[static_cast<unsigned int>(i)];
        if (!vid)
            continue;
        unsigned int noVidSum = 0u;
        for (unsigned int playerIndex = 0;playerIndex<8u;++playerIndex)
            noVidSum += static_cast<unsigned int>(vid->spriteCountsByPlayer[playerIndex]);
        if (noVidSum == 0)
            continue;
        ::Error->Log("    NoVid[%3i]=%i %i %i %i %s Layer=%i %s VidType=%x", i, static_cast<int>(vid->spriteCountsByPlayer[0]), static_cast<int>(vid->spriteCountsByPlayer[1]), static_cast<int>(vid->spriteCountsByPlayer[2]), static_cast<int>(vid->spriteCountsByPlayer[3]), vid->name.m_value, vid->layer, vid->vidName.m_value, static_cast<unsigned int>(vid->type));
    }
    m_relation.Release();
    m_velocityX = 0.0f;
    m_scale40 = 1.0f;
    m_frameRate04 = 0u;
    m_frameCounter08 = 0u;
    m_velocityY = 0.0f;
    m_flags &= 0xFF7FFFFFu;
    Graph->movie.Release();
    Graph->SetWind(25, ANGLE(200u));
    Graph->SetEnvironment(0xFFFFFFFFu);
    Graph->Effect(11, 0, 0, 0);
    RESOURCE* const replay = &m_resource;
    if ((m_flags&0x400u) != 0u)
    {
        Mouse->Enable();
        replay->Close();
    }
    if ((m_flags&0x200u) != 0u)
    {
        int marker = -1;
        (void)replay->Write(&marker, 4u);
        replay->PostAppend();
        replay->Close();
    }
    m_flags &= ~0x600u;
    const VECTOR2 defaultFirst(-1000.0f, -1000.0f);
    const VECTOR2 defaultSecond(-1000.0f, -1000.0f);
    SetScrollType(1, defaultFirst, defaultSecond);
    ::Error->Log("Player release");
    *reinterpret_cast<unsigned char*>(&m_pendingNetworkPauseCommand) = 0u;
    for (unsigned int i = 0;i<8u;++i)
    {
        PLAYER* const playerController = m_players[i];
        if (playerController)
            playerController->Release();
    }
    ::Error->Log("Sprite release");
    SPRITE_COLLECTOR* const collector = ::Sprites;
    if (collector)
        collector->DeleteAll();
    void* const groupOwner = m_groups.m_next;
    void* const embeddedGroup = &m_groups;
    if (groupOwner && groupOwner != embeddedGroup)
    {
        STRING subject("Incorrect delete groups in DeleteAll()");
        if (::Error)
        {
            STRING objectFormat("MAP");
            ::Error->Error(objectFormat, E_ERROR, subject, 0);
        }
    }
    ::Error->Log("Menu   release");
    SPRITE_LIST* const menuList = &m_menu;
    if (menuList->m_count != 0)
    {
        STRING subject("Menu sprite exist after delete");
        SPRITE* const sprite = menuList->m_data[0];
        int vidNumber = -1;
        VID* const vid = sprite->Vid();
        if (vid)
            vidNumber = vid->nVid;
        STRING objectFormat("SPRITE [%i](%i,%i,%i)");
        const int z = TruncateFloatToInt32(sprite->Z());
        const int y = TruncateFloatToInt32(sprite->Y());
        const int x = TruncateFloatToInt32(sprite->X());
        ::Error->Error(objectFormat, E_ERROR, subject, 0, vidNumber, x, y, z);
        menuList->DeleteAll();
    }
    const unsigned int elapsed = ::CurrentTime-m_tick60;
    const unsigned int averageFps = (elapsed != 0u) ? (m_simulationFrameCount*1000u)/elapsed : 0u;
    ::Error->Log("Average fps=%i", averageFps);
    ::Error->Log("Script release");
    m_script.Release();
    m_simulationFrameCount = 0u;
    DeleteExtraVid();
    for (int i = 0;i<m_vidCount;++i)
    {
        VID* const vid = m_vids[static_cast<unsigned int>(i)];
        if (vid)
            vid->ResetSprites();
    }
    for (unsigned int i = 0;i<64u;++i)
        EvFunctionNumber[i] = static_cast<int>(1000000u+i);
    for (unsigned int i = 0;i<10u;++i)
        m_spriteCountsByNameNumber[i] = 0u;
    VID* const emptyVid = ::EmptyVid;
    unsigned int remaining = 0u;
    for (unsigned int playerIndex = 0;playerIndex<8u;++playerIndex)
        remaining += static_cast<unsigned int>(emptyVid->spriteCountsByPlayer[playerIndex]);
    ::Error->Log("No EmptyVid=%i", remaining);
}

int MAP::CanPlaceGroundHeightRect(const VID* vid, const VECTOR& position,
    float minX, float minY, float maxX, float maxY, bool steppedHeight) const
{
    const int width = m_groundWidth;
    const int height = m_groundHeight;
    const int x0 = MapAbi_MapGroundGridIndex(minX, m_sizeXY.x, width);
    const int x1 = MapAbi_MapGroundGridIndex(maxX, m_sizeXY.x, width);
    const int y0 = MapAbi_MapGroundGridIndex(minY, m_sizeXY.y, height);
    const int y1 = MapAbi_MapGroundGridIndex(maxY, m_sizeXY.y, height);
    const short* const primary = m_groundPrimary;
    const short* const secondary = m_groundSecondary;
    if (steppedHeight)
    {
        MapAbi_GroundScanState scan =
        {
            -999999, -999999
        }
        ;
        for (int y = y0;y <= y1;++y)
        {
            const unsigned int row = static_cast<unsigned int>(y)*static_cast<unsigned int>(width);
            for (int x = x0;x <= x1;++x)
            {
                MapAbi_MapGroundAccumulate(scan.highest, scan.accepted, static_cast<int>(primary[row+x]));
                MapAbi_MapGroundAccumulate(scan.highest, scan.accepted, static_cast<int>(secondary[row+x]));
            }
        }
        const int ground = (scan.accepted != -999999)?scan.accepted:scan.highest;
        const double dz = static_cast<double>(ground)-position.z;
        if (dz>vid->forMoveUpZ)
            return 1;
        if (-dz>vid->forMoveDownZ)
            return 2;
        return 0;
    }
    for (int y = y0;y <= y1;++y)
    {
        const unsigned int row = static_cast<unsigned int>(y)*static_cast<unsigned int>(width);
        for (int x = x0;x <= x1;++x)
        {
            if (static_cast<float>(primary[row+x])>position.z || static_cast<float>(secondary[row+x])>position.z)
                return 1;
        }
    }
    return 0;
}

int MAP::CanPlaceInGridZ(const VID* vid, const VECTOR& position, ANGLE direction) const
{
    if (vid->moveMask == 0u)
        return 0;
    const bool directional = (vid->property&0x100u) != 0u;
    const bool steppedHeight = (vid->property&0x200u) != 0u;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float halfX;
    float halfY;
    if (directional)
    {
        const double radius = static_cast<double>(vid->realSizeXYZ.y)*0.27000001072883606f;
        offsetX = static_cast<float>(-(direction.Sin()*radius));
        offsetY = static_cast<float>(direction.CosY()*radius);
        halfX = vid->realSizeXYZ.x*0.60000002384185791f;
        halfY = vid->realSizeXYZ.y*0.30000001192092896f;
    }
    else
    {
        halfX = vid->sizeXYZ.x*0.5f;
        halfY = vid->sizeXYZ.y*0.5f;
    }
    if (halfX>3.0f)
        halfX -= 3.0f;
    if (halfY>3.0f)
        halfY -= 3.0f;
    const double centerX = static_cast<double>(position.x)+offsetX;
    const float centerY = position.y+offsetY;
    int result = CanPlaceGroundHeightRect(vid, position, static_cast<float>(centerX-halfX), centerY-halfY, static_cast<float>(centerX+halfX), centerY+halfY, steppedHeight);
    if (result != 0 || !directional)
        return result;
    const double oppositeX = static_cast<double>(position.x)-offsetX;
    const float oppositeY = position.y-offsetY;
    return CanPlaceGroundHeightRect(vid, position, static_cast<float>(oppositeX-halfX), oppositeY-halfY, static_cast<float>(oppositeX+halfX), oppositeY+halfY, steppedHeight);
}


void MAP::Save(STRING name)
{
    RESOURCE mapResource;
    RESOURCE* const resource = &mapResource;
    const char* const empty = StringGlobals::EmptyText();
    const char* const requested = name.m_value;
    if (std::strcmp(requested, empty) == 0)
    {
        return;
    }
    STRING* const currentName = &m_currentMapFileName;
    static const char kTemporaryMapName[] = "tmp_del!.map";
    if (std::strcmp(requested, currentName->m_value) == 0)
    {
        STRING temporaryMapName(kTemporaryMapName);
        (void)::MoveFileExA(currentName->m_value, temporaryMapName.m_value, 9u);
        *currentName = kTemporaryMapName;
    }
    if (resource->OpenForWrite(name, ResourceType::Map) != 0)
    {
        MYERROR* const sink = ::Error;
        sink->Window(STRING("Can't open file %s"), name.m_value);
        return;
    }
    bool carryForwardRecords = false;
    for (int index = 0;index<m_vidCount;++index)
    {
        VID* const entry = m_vids[index];
        if (entry && (entry->storageFlags&2u) != 0u)
        {
            carryForwardRecords = true;
            break;
        }
    }
    if (carryForwardRecords)
    {
        RESOURCE previousMapResource;
        RESOURCE* const oldResource = &previousMapResource;
        if (oldResource->OpenForRead(*currentName, ResourceType::Map) == 0)
        {
            (void)resource->Copy(oldResource, ResourceType::Weapons);
            (void)resource->Copy(oldResource, ResourceType::Object);
            oldResource->Close();
        }
        else
        {
            MYERROR* const sink = ::Error;
            sink->Window(STRING("Can't open file '%s', needed for save map"), currentName->m_value);
        }
    }
    (void)resource->PreAppend(ResourceType::Graphics, 0);
    Graph->SaveParameters(reinterpret_cast<STREAM*>(resource));
    resource->PostAppend();
    (void)resource->PreAppend(ResourceType::Header, 0);
    (void)resource->Write(&m_sizeXY.x, 4u);
    (void)resource->Write(&m_sizeXY.y, 4u);
    (void)resource->Write(&m_shiftXY.x, 4u);
    (void)resource->Write(&m_shiftXY.y, 4u);
    (void)resource->Write(&::CurrentTime, 4u);
    const unsigned int version = 19u;
    (void)resource->Write(&version, 4u);
    resource->PostAppend();
    const int gridWidth = m_groundWidth;
    const int gridHeight = m_groundHeight;
    const unsigned short* const grid = reinterpret_cast<const unsigned short*>(m_groundPrimary);
    bool gridUsed = false;
    if (gridHeight>0)
    {
        for (int y = 0;y<gridHeight && !gridUsed;++y)
            for (int x = 0;x<gridWidth;++x) if (grid[static_cast<unsigned int>(y)*static_cast<unsigned int>(gridWidth)+static_cast<unsigned int>(x)] != 0u)
        {
            gridUsed = true;
            break;
        }
        if (gridUsed)
        {
            const unsigned int cells = static_cast<unsigned int>(gridWidth)*static_cast<unsigned int>(gridHeight);
            (void)resource->PreAppend(ResourceType::Grid, 0);
            (void)resource->Write(grid, cells*2u);
            resource->PostAppend();
        }
    }
    (void)resource->PreAppend(ResourceType::Sprites, 0);
    for (SPRITE* sprite = ::Sprites->FirstAllSpriteFrom0();sprite;sprite = ::Sprites->NextAllSpriteFrom0())
    {
        if (sprite->ChildBacklink())
            continue;
        int menuIndex = m_menu.m_count-1;
        while (menuIndex >= 0 && m_menu.m_data[menuIndex] != sprite)
            --menuIndex;
        if (menuIndex<0)
            sprite->Save(resource);
    }
    int spriteSentinel = -1;
    (void)resource->Write(&spriteSentinel, 4u);
    resource->PostAppend();
    for (SPRITE* sprite = ::Sprites->FirstAllSpriteFrom0();sprite;sprite = ::Sprites->NextAllSpriteFrom0())
    {
        if (sprite->ChildBacklink())
            continue;
        int menuIndex = m_menu.m_count-1;
        while (menuIndex >= 0 && m_menu.m_data[menuIndex] != sprite)
            --menuIndex;
        if (menuIndex >= 0)
            continue;
        const unsigned int before = static_cast<unsigned int>(std::ftell(resource->m_file));
        (void)resource->PreAppend(ResourceType::SpriteData, 0);
        (void)resource->Write(&sprite, 4u);
        (void)sprite->Action(ACT_SAVE, static_cast<int>(PtrToU32(resource)), 0, 0);
        const unsigned int after = static_cast<unsigned int>(std::ftell(resource->m_file));
        if (BitsToSigned32(after)>BitsToSigned32(before+5u))
            resource->PostAppend();
    }
    (void)resource->PreAppend(ResourceType::SpriteData, 0);
    (void)resource->Write(&spriteSentinel, 4u);
    resource->PostAppend();
    (void)resource->PreAppend(ResourceType::Player, 0);
    for (unsigned int index = 0;index<8u;++index)
    {
        PLAYER* const player = m_players[index];
        player->Save(reinterpret_cast<STREAM*>(resource));
    }
    resource->PostAppend();
    (void)resource->PreAppend(ResourceType::Groups, 0);
    m_groups.Save(resource);
    resource->PostAppend();
    resource->Close();
    if (std::strcmp(currentName->m_value, kTemporaryMapName) == 0)
    {
        STRING temporaryMapName(kTemporaryMapName);
        (void)CrtRemove(temporaryMapName.m_value);
        (void)currentName->operator = (name);
    }
}




static void MapLoadVidDiagnostic(const char* subjectText, int category, int errorCode)
{
    if (!Error)
        return;
    const STRING owner("MAP");
    const STRING subject(subjectText);
    Error->Error(owner, static_cast<TYPE_ERROR>(category), subject, static_cast<unsigned long>(errorCode));
}

static bool MapLoadVidFileExists(const STRING& fileName)
{
    if (fileName.m_value[0] == 0)
        return false;
    std::FILE* const file = std::fopen(fileName.m_value, "rb");
    if (!file)
        return false;
    (void)std::fclose(file);
    return true;
}

void MAP::LoadVid(RESOURCE* resource)
{
    const unsigned int loadStart = timeGetTime();
    STRING* const loadingText = mapLoadingText.Address();
    PROFILE* const profile = ::Strings;
    if (profile)
    {
        STRING section;
        STRING key;
        STRING fallback;
        section = "strings";
        key = "Loading";
        fallback = "LOADING...";
        STRING localized = (::Strings)->GetString(section, key, fallback);
        *loadingText = localized;
    }
    else
    {
        STRING empty;
        empty = StringGlobals::EmptyText();
        *loadingText = empty;
    }
    if (m_weaponData == 0)
    {
        LoadWeapon(resource);
        const int weaponCount = m_weaponCount;
        if (weaponCount != 0)
        {
            ::Error->Log("LoadWeapon::No=%-5i             sizeof(WEAPON)=%-4i", weaponCount, 572);
        }
    }
    int loadTable[VidSlotCount];
    for (unsigned int i = 0;i<static_cast<unsigned int>(VidSlotCount);++i)
        loadTable[i] = 1;
    MapParseObjectLoadListRange("objects_load_list.txt", loadTable, static_cast<unsigned int>(VidSlotCount));
    if (MapLoadVidFileExists(STRING("vid\\fast_load.vid")) &&
        MapLoadVidFileExists(STRING("vid\\fast_load_data.vid")))
    {
        {
            STRING path("vid\\fast_load.vid");
            m_fastLoadIndex = static_cast<unsigned char*>(FFileMapping(path));
        }
        {
            STRING path("vid\\fast_load_data.vid");
            m_fastLoadData = static_cast<unsigned char*>(FFileMapping(path));
        }
    }
    static const unsigned int kObjectResourceType = ResourceType::Object;
    if (resource->GoBegin(kObjectResourceType) != 0)
    {
        unsigned char* const mappedFastLoad = m_fastLoadIndex;
        if (mappedFastLoad)
            (void)UnmapViewOfFile(mappedFastLoad);
        m_fastLoadIndex = 0;
        return;
    }
    const bool overlayExistingDepot = m_vidCount != 0;
    do
    {
        int nvid = 0;
        (void)resource->Read(&nvid, 4u);
        if (nvid >= VidSlotCount)
        {
            MapLoadVidDiagnostic( "nvid > MAX_VID", 4, nvid);
            continue;
        }
        if (loadTable[nvid] == 0)
            continue;
        VID* previous = m_vids[static_cast<unsigned int>(nvid)];
        if (previous)
        {
            previous->~VID();
            CrtFreePointer(previous);
            m_vids[static_cast<unsigned int>(nvid)] = 0;
            MapLoadVidDiagnostic( "this VID already loaded", 5, nvid);
        }
        VID* created = CreateVid(resource, nvid);
        m_vids[static_cast<unsigned int>(nvid)] = created;
        if (!created)
            continue;
        if (nvid >= m_vidCount)
            m_vidCount = nvid+1;
        if (overlayExistingDepot)
            created->storageFlags |= 2u;
        const int weaponIndex = created->nWeapon;
        WEAPON_RUNTIME* weapon = 0;
        if (weaponIndex<m_weaponCount)
        {
            weapon = m_weaponData+weaponIndex;
        }
        else
        {
            {
                STRING subject;
                subject = "nWeapon > noWeapon";
                VID* const current = m_vids[static_cast<unsigned int>(nvid)];
                current->Error((TYPE_ERROR)10, subject, static_cast<unsigned long>(current->nWeapon));
            }
            weapon = m_weaponData;
        }
        m_vids[static_cast<unsigned int>(nvid)]->weapon = weapon;
        if (nvid == 0)
        {
            mapLoadingVid = m_vids[0];
            DispatchMapPresentation(10u);
        }
        else
        {
            DispatchMapPresentation(0u);
        }
    }
    while (resource->GoNextSub(kObjectResourceType) == 0);
    int maxSizeX = 0;
    int maxSizeY = 0;
    for (int index = 0;index<m_vidCount;++index)
    {
        VID* const vid = m_vids[static_cast<unsigned int>(index)];
        if (!vid)
            continue;
        vid->SetChildAndLink();
        VID* const linked = m_vids[static_cast<unsigned int>(index)];
        const int sx = static_cast<int>(linked->vidSizeX);
        const int sy = static_cast<int>(linked->vidSizeY);
        if (sx>maxSizeX)
            maxSizeX = sx;
        if (sy>maxSizeY)
            maxSizeY = sy;
    }
    for (int index = 0;index<m_vidCount;++index)
    {
        VID* const vid = m_vids[static_cast<unsigned int>(index)];
        if (!vid)
            continue;
        (void)vid->CalcImportantForSync();
        ::Error->Log("vid[%i].sync = %i", index, (m_vids[static_cast<unsigned int>(index)]->runtimeFlags>>12u)&1u);
    }
    const unsigned int loadElapsed = timeGetTime()-loadStart;
    ::Error->Log("LoadVid::No   =%-15i   sizeof(VID)   =%-5i    load time     =%-6ims MaxSizeX, Y     =%i,%i sizeof(SPRITE)=%-8i", m_vidCount, 1344, loadElapsed, maxSizeX, maxSizeY, 104);
    const unsigned int waitStart = timeGetTime();
    DispatchMapPresentation(9u);
    while (timeGetTime()-waitStart<300u)
        DispatchMapPresentation(0u);
    unsigned char* const mappedFastLoad = m_fastLoadIndex;
    if (mappedFastLoad)
        (void)UnmapViewOfFile(mappedFastLoad);
    m_fastLoadIndex = 0;
}

static void MapLoadDebugText(const char* textValue)
{
    if (Const->DebugMode != 0)
    {
        STRING text(textValue);
        Graph->DrawDebugText(text);
    }
}

static void MapLoadErrorLiteral(const char* subjectText, TYPE_ERROR type, unsigned long code)
{
    if (!Error)
        return;
    STRING subject(subjectText);
    const STRING owner("MAP");
    Error->Error(owner, type, subject, code);
}

static void MapLoadErrorNamed(const char* prefix, const STRING& functionName,
                              TYPE_ERROR type, unsigned long code)
{
    MYERROR* const sink = ::Error;
    if (!sink)
        return;
    STRING stem = prefix+functionName;
    STRING subject = stem+"'";
    STRING owner("MAP");
    sink->Error(owner, type, subject, code);
}

void MAP::Load(STRING name)
{
    RESOURCE mapResource;
    RESOURCE* const resource = &mapResource;
    const unsigned int oldGraphTime = ::CurrentTime;
    const int loadingSlotRemaining = Graph->GetEffectState(3);
    SetupProcessorFlags();
    const char* const empty = StringGlobals::EmptyText();
    const char* requested = name.m_value;
    if (std::strcmp(requested, empty) == 0)
    {
        return;
    }
    m_flags |= 0x40u;
    mapLoadingVid = 0;
    MapLoadDebugText("Release previous map");
    Release();
    RESOURCE* const demoResource = &m_resource;
    if (m_resource.m_file == 0 && demoResource->OpenForRead(name, ResourceType::Demo) == 0)
    {
        (void)name.Read(demoResource);
        m_flags |= 0x400u;
        requested = name.m_value;
    }
    if (resource->OpenForRead(name, ResourceType::Map) != 0)
    {
        MYERROR* const sink = ::Error;
        sink->Window(STRING("!!!ERROR!!!LOAD: Invalid map file %s"), name.m_value);
        return;
    }
    Mouse->Disable();
    (void)m_previousMapFileName.operator = (m_currentMapFileName);
    (void)m_currentMapFileName.operator = (name);
    unsigned int randomSeed = 0u;
    if ((m_flags&0x400u) != 0u)
    {
        (void)demoResource->Read(&randomSeed, 4u);
    }
    else
    {
        NET* const network = ::Net;
        if ((network->m_flags&0x10u) != 0u)
            randomSeed = 0x00DB2F7Fu;
        else
        {
            randomSeed = ::timeGetTime();
        }
    }
    srand(static_cast<unsigned int>(randomSeed));
    MapLoadDebugText("Load extra vid");
    const bool currentFormat = (resource->GoBegin(ResourceType::Graphics) == 0);
    int loadingTerrainSpriteCount = 0;
    unsigned int lastHardwareTerrainTag = 0u;
    unsigned int mapVersion = 0u;
    bool containerOk = true;
    if (currentFormat)
    {
        MapLoadDebugText("Load graph parameters");
        Graph->LoadParameters(resource, m_sizeXY);
        if (resource->GoNext(ResourceType::Header) != 0)
        {
            MapLoadErrorLiteral("HEAD", E_SECTION, 0u);
            containerOk = false;
        }
        else
        {
            (void)resource->Read(&m_sizeXY.x, 4u);
            (void)resource->Read(&m_sizeXY.y, 4u);
            (void)resource->Read(&m_shiftXY.x, 4u);
            (void)resource->Read(&m_shiftXY.y, 4u);
            (void)resource->Read(&::CurrentTime, 4u);
            ::PrevCurrentTime = 1u;
            ::CurrentTime = 10u;
            if ((m_flags&0x400u) != 0u)
            {
                (void)demoResource->Read(&::CurrentTime, 4u);
                m_input.Reset();
            }
            Graph->SetTime(oldGraphTime, ::CurrentTime);
            if ((m_flags&0x400u) == 0u && m_resource.m_file != 0)
            {
                m_flags |= 0x200u;
                (void)demoResource->PreAppend(ResourceType::Demo, 0);
                unsigned int textBytes = 1u;
                const char* text = name.m_value;
                while (text[textBytes-1u] != 0)
                    ++textBytes;
                (void)demoResource->Write(text, textBytes);
                (void)demoResource->Write(&randomSeed, 4u);
                (void)demoResource->Write(&::CurrentTime, 4u);
                m_input.Reset();
            }
            m_tick60 = ::CurrentTime;
            (void)resource->Read(&mapVersion, 4u);
            if (static_cast<int>(mapVersion) <= 9)
            {
                m_sizeXY.x = static_cast<float>(*reinterpret_cast<int*>(&m_sizeXY.x));
                m_sizeXY.y = static_cast<float>(*reinterpret_cast<int*>(&m_sizeXY.y));
                m_shiftXY.x = static_cast<float>(*reinterpret_cast<int*>(&m_shiftXY.x));
                m_shiftXY.y = static_cast<float>(*reinterpret_cast<int*>(&m_shiftXY.y));
            }
            NET* const network = ::Net;
            ::Error->Log("CurrentTime   =%-15u   SizeXY = %i, %i Map version   =%-8i NoNetworkPlayer=%i", ::CurrentTime, static_cast<unsigned int>(TruncateFloatToInt32(m_sizeXY.x)), static_cast<unsigned int>(TruncateFloatToInt32(m_sizeXY.y)), mapVersion, network->m_noPlayers);
        }
    }
    else
    {
        if (resource->GoBegin(ResourceType::Header) != 0)
        {
            MapLoadErrorLiteral("HEAD", E_SECTION, 0u);
            containerOk = false;
        }
        else
        {
            int legacyWidth = 0, legacyHeight = 0;
            short legacyX = 0, legacyY = 0;
            (void)resource->Read(&legacyWidth, 4u);
            (void)resource->Read(&legacyHeight, 4u);
            (void)resource->Read(&legacyX, 2u);
            (void)resource->Read(&legacyY, 2u);
            m_sizeXY.x = static_cast<float>(legacyWidth);
            m_sizeXY.y = static_cast<float>(legacyHeight);
            m_shiftXY.x = static_cast<float>(legacyX);
            m_shiftXY.y = static_cast<float>(legacyY);
            (void)resource->Read(&::CurrentTime, 4u);
            m_tick60 = ::CurrentTime;
            ::PrevCurrentTime = ::CurrentTime;
            Graph->SetTime(oldGraphTime, ::CurrentTime);
            (void)resource->Read(&mapVersion, 4u);
            Graph->OldLoadParameters(resource, m_sizeXY);
        }
    }
    if (containerOk)
    {
        SPRITE_COLLECTOR* const oldCollector = ::Sprites;
        if (oldCollector)
        {
            oldCollector->~SPRITE_COLLECTOR();
            CrtFreePointer(oldCollector);
        }
        SPRITE_COLLECTOR* const newCollector = static_cast<SPRITE_COLLECTOR*>(CrtNew(sizeof(SPRITE_COLLECTOR)));
        if (newCollector)
        {
            try
            {
                new(newCollector) SPRITE_COLLECTOR(m_sizeXY);
            }
            catch (...)
            {
                CrtFreePointer(newCollector);
                throw;
            }
        }
        ::Sprites = newCollector;
        m_cameraMinX = 0u;
        std::memcpy(&m_cameraMaxX, &m_sizeXY.x, sizeof(m_cameraMaxX));
        m_cameraMinY = 0u;
        std::memcpy(&m_cameraMaxY, &m_sizeXY.y, sizeof(m_cameraMaxY));
        if (currentFormat)
        {
            MapLoadDebugText("Create new hash table");
            GRAPH* const graph = Graph;
            const VECTOR2 centered(m_shiftXY.x+graph->screenWidth*0.5f, m_shiftXY.y+graph->screenHeight*0.5f);
            SetShiftCoor(centered, 0);
            MapLoadDebugText("Load gridZ");
        }
        ResetGroundZ();
        if (resource->GoNext(ResourceType::Grid) == 0)
        {
            short* const oldGrid = m_groundPrimary;
            if (oldGrid)
                std::free(oldGrid);
            m_groundPrimary = 0;
            const unsigned int gridBytes = static_cast<unsigned int>(resource->SubLoad( reinterpret_cast<void**>(&m_groundPrimary), 0));
            const unsigned int expected = static_cast<unsigned int>(m_groundWidth)* static_cast<unsigned int>(m_groundHeight)*2u;
            if (gridBytes != expected)
            {
                MapLoadErrorLiteral("grid", E_INVALID, gridBytes);
                ResetGroundZ();
            }
            else
            {
                ::Error->Log("Load GridZ from map");
            }
        }
        else
        {
            (void)resource->GoBegin(ResourceType::Any);
        }
        Graph->SetDefaultState();
        if (resource->GoNext(ResourceType::Sprites) != 0)
        {
            MapLoadErrorLiteral("SPR ", E_SECTION, 0u);
            containerOk = false;
        }
        else
        {
            STREAM* const stream = resource;
            if (currentFormat)
            {
                for (;;)
                {
                    SPRITE* sprite = LoadSprite(stream, static_cast<int>(mapVersion));
                    if (sprite == reinterpret_cast<SPRITE*>(AddressToUIntPtr(0xFFFFFFFFu)))
                        break;
                    if (sprite)
                    {
                        VID* const definition = sprite->Vid();
                        if (definition->spriteClass == 8)
                            ++loadingTerrainSpriteCount;
                        const unsigned int terrainTag = definition->layer;
                        if (terrainTag != lastHardwareTerrainTag)
                        {
                            lastHardwareTerrainTag = terrainTag;
                            MapLoadDebugText(terrainTag == 0u?"Load hardware terrain":"Load sprites");
                        }
                    }
                }
            }
            else
            {
                for (;;)
                {
                    SPRITE* sprite = OldLoadSprite(stream);
                    if (sprite == reinterpret_cast<SPRITE*>(AddressToUIntPtr(0xFFFFFFFFu)))
                        break;
                }
            }
        }
    }
    if (containerOk)
    {
        RailMap.Get().CreateAdditionalDots();
        STRING* const globalLoadingText = mapLoadingText.Address();
        if (loadingTerrainSpriteCount>30 && loadingSlotRemaining>0)
        {
            STRING section("strings");
            STRING key("Loading");
            STRING fallback("LOADING...");
            STRING localized = (::Strings)->GetString(section, key, fallback);
            *globalLoadingText = localized;
            if (mapLoadingVid == 0)
                mapLoadingVid = m_vids[0];
            DispatchMapPresentation(10u);
        }
        else *globalLoadingText = StringGlobals::EmptyText();
        MapLoadDebugText("Load data for sprite");
        if (resource->GoNext(ResourceType::SpriteData) != 0)
        {
            MapLoadErrorLiteral("SPRD", E_SECTION, 0u);
            containerOk = false;
        }
        else
        {
            STREAM* const stream = resource;
            for (;;)
            {
                SPRITE* sprite = ReadPointer(stream);
                if (sprite == reinterpret_cast<SPRITE*>(AddressToUIntPtr(0xFFFFFFFFu)))
                    break;
                DispatchMapPresentation(0u);
                if (sprite)
                {
                    (void)sprite->Action(currentFormat?ACT_RESTORE:ACT_LEGACY_RESTORE, static_cast<int>(PtrToU32(resource)), static_cast<int>(mapVersion), 0);
                }
                (void)resource->GoNextSub(ResourceType::SpriteData);
            }
        }
    }
    if (containerOk && currentFormat)
    {
        MapLoadDebugText("Load players info");
        if (resource->GoNext(ResourceType::Player) != 0)
        {
            MapLoadErrorLiteral("PLAY", E_SECTION, 0u);
            containerOk = false;
        }
        else
        {
            for (unsigned int index = 0u;index<8u;++index)
            {
                PLAYER* const player = m_players[index];
                player->Load(resource);
            }
        }
        if (containerOk)
        {
            MapLoadDebugText("Load groups info");
            if (resource->GoNext(ResourceType::Groups) != 0)
            {
                MapLoadErrorLiteral("GROU", E_SECTION, 0u);
                containerOk = false;
            }
            else
            {
                m_groups.Load(resource);
            }
        }
    }
    if (!containerOk)
    {
        return;
    }
    m_flags &= ~0x40u;
    resource->Close();
    m_relation.Release();
    ::Error->Log("Vid    release %i %i", TEXTURE::memoryInUse, VID::MemoryInUse);
    ::Sound->MusicTact();
    STRING baseName = name.BeforeLast(".");
    STRING compiledName = baseName+".lgc";
    STRING debugName = baseName+".lgd";
    bool compiledExists = false;
    bool debugExists = false;
    if (compiledName.m_value[0])
    {
        std::FILE* const file = std::fopen(compiledName.m_value, "rb");
        if (file)
        {
            compiledExists = true;
            (void)static_cast<unsigned int>(std::fclose(file));
        }
    }
    if (!compiledExists && debugName.m_value[0])
    {
        std::FILE* const file = std::fopen(debugName.m_value, "rb");
        if (file)
        {
            debugExists = true;
            (void)static_cast<unsigned int>(std::fclose(file));
        }
    }
    STRING* const scriptName = (!compiledExists && debugExists)?&debugName:&compiledName;
    SCRIPT* const scriptRuntime = &m_script;
    MapLoadDebugText("Load script file");
    (void)scriptRuntime->Load(*scriptName);
    ::Sound->MusicTact();
    MapLoadDebugText("Connect script function with VID");
    for (int functionIndex = 0;functionIndex<m_script.m_functionList.m_count;++functionIndex)
    {
        if (m_script.m_functionList.m_data[functionIndex].m_value.m_type != 3u)
            continue;
        const STRING functionNameValue = m_script.m_functionList.Name(functionIndex);
        const char* const functionName = functionNameValue.m_value;
        for (unsigned int eventIndex = 0u;;++eventIndex)
        {
            const char* const knownName = mapEventNames[eventIndex];
            if (knownName[0] == 0)
                break;
            if (std::strcmp(functionName, knownName) == 0)
                EvFunctionNumber[eventIndex] = functionIndex;
        }
        if (functionName[0] == 'F' && CrtIsDigit(static_cast<unsigned int>(static_cast<unsigned char>(functionName[1]))) != 0u && CrtIsDigit(static_cast<unsigned int>(static_cast<unsigned char>(functionName[2]))) != 0u && CrtIsDigit(static_cast<unsigned int>(static_cast<unsigned char>(functionName[3]))) != 0u)
        {
            unsigned int cursor = 1u;
            int vidIndex = 0;
            while (CrtIsDigit(static_cast<unsigned int>(static_cast<unsigned char>(functionName[cursor]))) != 0u)
            {
                vidIndex = AddSigned32Wrap(MultiplySigned32Wrap(vidIndex, 10), static_cast<int>(functionName[cursor])-'0');
                ++cursor;
            }
            if (functionName[cursor] == '_')
            {
                const char* const suffix = functionName+cursor+1u;
                int animationIndex = 0;
                int expectedParameters = 1;
                bool suffixValid = true;
                if (std::strncmp(suffix, "DAMAGE", 6u) == 0u)
                {
                    animationIndex = 18;
                    expectedParameters = 3;
                }
                else if (std::strncmp(suffix, "DESTROY", 7u) == 0u)
                {
                    animationIndex = 17;
                    expectedParameters = 1;
                }
                else if (std::strncmp(suffix, "COLLISION", 9u) == 0u)
                {
                    animationIndex = 19;
                    expectedParameters = 2;
                }
                else if (std::strncmp(suffix, "DETECT", 6u) == 0u)
                {
                    animationIndex = 20;
                    expectedParameters = 1;
                }
                else if (CrtIsDigit(static_cast<unsigned int>(static_cast<unsigned char>(suffix[0]))) != 0u)
                {
                    if (suffix[1] != 0)
                        animationIndex = (static_cast<int>(suffix[0])-'0')*10+ (static_cast<int>(suffix[1])-'0');
                    else animationIndex = static_cast<int>(suffix[0])-'0';
                }
                else
                {
                    suffixValid = false;
                    MapLoadErrorLiteral("n_animation in event function", E_MISSING, 0u);
                }
                if (suffixValid)
                {
                    const int actualParameters = m_script.m_functionList.m_data[functionIndex].m_value.m_elementOrParameterCount;
                    const bool parameterCountMatches = actualParameters == expectedParameters || ((animationIndex == 7 || animationIndex == 14) && actualParameters == 2);
                    if (!parameterCountMatches)
                    {
                        MapLoadErrorNamed("no parameters in functions '", functionNameValue, E_INVALID, static_cast<unsigned long>(actualParameters));
                    }
                    else if (vidIndex >= 0 && vidIndex<m_vidCount && m_vids[static_cast<unsigned int>(vidIndex)] != 0)
                    {
                        VID* const vid = m_vids[static_cast<unsigned int>(vidIndex)];
                        if (animationIndex >= 0 && animationIndex<20)
                            vid->scriptFunction[static_cast<unsigned int>(animationIndex)] = functionIndex;
                        else
                        {
                            MapLoadErrorNamed("n_animation in functions '", functionNameValue, E_INVALID, static_cast<unsigned long>(animationIndex));
                        }
                    }
                    else if (g_iZmDebug == 0u)
                    {
                        MapLoadErrorNamed("n_vid in functions '", functionNameValue, E_INVALID, static_cast<unsigned long>(vidIndex));
                    }
                }
            }
            else
            {
                MapLoadErrorLiteral("'_' and n_animation in event function", E_MISSING, 0u);
            }
        }
    }
    const unsigned int loadTick = ::timeGetTime();
    ::RealCurrentTime = loadTick;
    for (unsigned int playerIndex = 0u;playerIndex<8u;++playerIndex)
    {
        PLAYER* const player = m_players[playerIndex];
        player->InitForLoadMap();
    }
    MapLoadDebugText("Run scripts for create sprites");
    if ((static_cast<unsigned char>(m_flags)&0x02u) == 0u)
    {
        SPRITE_COLLECTOR* collector = ::Sprites;
        int cursor = collector->allSprites.m_count;
        SPRITE* sprite = collector->allSprites.NextIterateNon0(&cursor);
        while (sprite)
        {
            VID* const definition = sprite->Vid();
            const int createEvent = definition->scriptFunction[14];
            if (createEvent >= 0)
                (void)ScriptRun(createEvent, sprite, static_cast<const SPRITE*>(0), 0);
            collector = ::Sprites;
            sprite = collector->allSprites.NextIterateNon0(&cursor);
        }
    }
    {
        SPRITE_COLLECTOR* collector = ::Sprites;
        int cursor = collector->allSprites.m_count;
        SPRITE* sprite = collector->allSprites.NextIterateNon0(&cursor);
        while (sprite)
        {
            VID* const definition = sprite->Vid();
            if (definition->spriteClass == 21)
                reinterpret_cast<ENGINE*>(sprite)->CheckPrevNextEngine();
            collector = ::Sprites;
            sprite = collector->allSprites.NextIterateNon0(&cursor);
        }
    }
    MOUSE* const mapCollision = Mouse;
    {
        VECTOR collisionPosition;
        collisionPosition.x = m_shiftXY.x+m_input.m_clientMouseXY.x;
        collisionPosition.y = m_shiftXY.y+m_input.m_clientMouseXY.y;
        collisionPosition.z = mapCollision->Coor().z;
        mapCollision->ChangeCoor(collisionPosition);
    }
    {
        if (g_iZmDebug == 0u)
        {
            MapLoadDebugText("Build sprites in terrain");
            m_flags |= 0x100u;
            ::Sprites->BuildAllTerrain();
            m_flags &= ~0x100u;
        }
        else
        {
            ::Sprites->InvalidateDeviceObjects();
            ::Sprites->RestoreDeviceObjects();
        }
    }
    MapLoadDebugText(StringGlobals::EmptyText());
    if (loadingTerrainSpriteCount>30 && loadingSlotRemaining>0)
    {
        const unsigned int waitStart = ::timeGetTime();
        DispatchMapPresentation(9u);
        while (::timeGetTime()-waitStart<300u)
            DispatchMapPresentation(0u);
        Graph->Effect(10, 0, 0, 500);
    }
}

static bool MapResourceTextContains(const STRING& text, const char* fragment)
{
    return std::strstr(text.value, fragment) != 0;
}

static bool MapResourceTextEquals(const char* left, const char* right)
{
    while (*left == *right)
    {
        if (*left == 0)
            return true;
        ++left;
        ++right;
    }
    return false;
}

static void MapResourceBuildExternalPath(RESOURCE* resource, const STRING& source, STRING* output)
{
    STRING normalizedPath = resource->GetFileName().ToLower();
    STRING reducedDirectory;
    if (MapResourceTextContains(normalizedPath, ".map") && MapResourceTextContains(normalizedPath, ":\\"))
    {
        STRING normalizedSourceDirectory = source.BeforeLast("\\").ToLower();
        STRING mapDirectory = normalizedPath.BeforeLast("\\");
        reducedDirectory = mapDirectory.BeforeLast(normalizedSourceDirectory);
    }
    else
    {
        reducedDirectory = StringGlobals::EmptyText();
    }
    *output = reducedDirectory+source;
}

static void MapResourceAssignNumberAndName(VID* vid, unsigned int out32, const STRING& name)
{
    vid->nVid = static_cast<int>(out32);
    vid->name = name;
}

static void MapResourceAssignIdentity(VID* vid, unsigned int out32, const STRING& name, const STRING& source)
{
    MapResourceAssignNumberAndName(vid, out32, name);
    vid->vidName = source;
}

static void MapResourceLogLoadedVid(VID* vid)
{
    if ((vid->type&0x40u) != 0u)
    {
        VID_SURFACE* const surface = static_cast<VID_SURFACE*>(vid);
        const double maxGrid = static_cast<double>(vid->maxXGridZ);
        const double minGrid = static_cast<double>(vid->minXGridZ);
        const unsigned int storage = (vid->storageFlags>>3u)&1u;
        const unsigned int runtime = (vid->runtimeFlags>>7u)&1u;
        const int linked = vid->cadrLinkXYZ != 0 ? 1 : 0;
        const unsigned int mode = surface->surfaceMode;
        const unsigned int formatCode = surface->renderMode;
        const unsigned int state = surface->pixelFormat;
        STRING pixelFormat = GetPixelFormat(static_cast<_D3DFORMAT>(state));
        const int sizeY = static_cast<int>(vid->vidSizeY);
        const int sizeX = static_cast<int>(vid->vidSizeX);
        const unsigned int type = static_cast<unsigned int>(vid->type);
        const STRING numberName = vid->GetNumberName();
        const STRING withSpace = numberName+" ";
        const STRING withSource = withSpace+vid->vidName;
        const STRING format = withSource+" %i, %i, %x, %s(%i) fx=%i mesh=%i LinkDot=%i optActive=%i FastLoad=%i XGridZ[%f-%f]";
        Error->Log(format, sizeX, sizeY, type, pixelFormat.CharPtr(), state, formatCode, mode, linked, runtime, storage, minGrid, maxGrid);
    }
    else
    {
        const unsigned int sync = (vid->runtimeFlags>>12u)&1u;
        const unsigned int runtime = (vid->runtimeFlags>>7u)&1u;
        const unsigned int type = static_cast<unsigned int>(vid->type);
        const int sizeY = static_cast<int>(vid->vidSizeY);
        const int sizeX = static_cast<int>(vid->vidSizeX);
        const STRING numberName = vid->GetNumberName();
        const STRING withSpace = numberName+" ";
        const STRING withSource = withSpace+vid->vidName;
        const STRING format = withSource+" %i, %i, %x optActive=%i sync=%i";
        Error->Log(format, sizeX, sizeY, type, runtime, sync);
    }
}

VID* MAP::CreateVid(RESOURCE* resource, int nVid)
{
    STRING name;
    STRING source;
    VID baseVid;
    baseVid.noCadr = 32000;
    name.Read(resource);
    std::FILE* const file = resource->m_file;
    const unsigned int parameterOffset = static_cast<unsigned int>(std::ftell(file));
    baseVid.LoadParameters(resource);
    source.Read(resource);
    source = source.ToLower();
    resource->m_ioState = RESOURCE::IO_SEEK;
    (void)static_cast<unsigned int>(std::fseek(resource->m_file, BitsToSigned32(parameterOffset), static_cast<int>(0u)));
    VID* loaded = 0;
    const int objectCount = m_vidCount;
    for (int index = 0;index<objectCount;++index)
    {
        VID* const current = m_vids[static_cast<unsigned int>(index)];
        if (!current)
            continue;
        if (MapResourceTextEquals(current->vidName.value, source.value) && (current->type&0x4000u) == 0u)
        {
            loaded = current->CreateMirror();
            MapResourceAssignIdentity(loaded, nVid, name, source);
            loaded->LoadParameters(resource);
            break;
        }
    }
    if (!loaded)
    {
        if (!MapResourceTextContains(source, "."))
        {
            void* const storage = static_cast<unsigned char*>(CrtNew(sizeof(VID_FONT)));
            if (storage)
            {
                try
                {
                    loaded = new(storage) VID_FONT(&baseVid, source);
                }
                catch (...)
                {
                    CrtFreePointer(storage);
                    throw;
                }
            }
            MapResourceAssignNumberAndName(loaded, nVid, name);
            loaded->LoadParameters(resource);
        }
        else if (MapResourceTextContains(source, ".fon") || MapResourceTextContains(source, ".ttf"))
        {
            loaded = 0;
        }
        else if (MapResourceTextContains(source, ".jpg") || MapResourceTextContains(source, ".tiff") || MapResourceTextContains(source, ".gif") || MapResourceTextContains(source, ".png") || MapResourceTextContains(source, ".tga") || MapResourceTextContains(source, ".bmp"))
        {
            void* const storage = static_cast<unsigned char*>(CrtNew(sizeof(VID_SURFACE)));
            if (storage)
            {
                try
                {
                    loaded = new(storage) VID_SURFACE(&baseVid, resource, source);
                }
                catch (...)
                {
                    CrtFreePointer(storage);
                    throw;
                }
            }
            MapResourceAssignNumberAndName(loaded, nVid, name);
        }
        else
        {
            STRING externalPath;
            MapResourceBuildExternalPath(resource, source, &externalPath);
            RESOURCE archiveStorage;
            RESOURCE* const archive = &archiveStorage;
            if (archive->OpenForRead(externalPath, ResourceType::Video) != 0)
            {
                ::Error->Log("LOAD::Can't open file %s", source.value);
                return 0;
            }
            if (archive->GoBegin(ResourceType::Header) != 0)
            {
                ::Error->Log("!!!ERROR!!!VID '%s': Load() not HEAD ", source.value);
            }
            (void)archive->Read(&baseVid.type, 2u);
            const unsigned short headFlags = baseVid.type;
            if ((headFlags&0x40u) != 0u)
            {
                void* const storage = static_cast<unsigned char*>(CrtNew(sizeof(VID_SURFACE)));
                if (storage)
                {
                    try
                    {
                        loaded = new(storage) VID_SURFACE();
                    }
                    catch (...)
                    {
                        CrtFreePointer(storage);
                        throw;
                    }
                }
            }
            else if ((headFlags&0x80u) != 0u)
            {
                void* const storage = static_cast<unsigned char*>(CrtNew(sizeof(VID_LIGHT)));
                if (storage)
                {
                    try
                    {
                        loaded = new(storage) VID_LIGHT();
                    }
                    catch (...)
                    {
                        CrtFreePointer(storage);
                        throw;
                    }
                }
            }
            else
            {
                void* const storage = static_cast<unsigned char*>(CrtNew(sizeof(VID_SOFTWARE)));
                if (storage)
                {
                    try
                    {
                        loaded = new(storage) VID_SOFTWARE();
                    }
                    catch (...)
                    {
                        CrtFreePointer(storage);
                        throw;
                    }
                }
            }
            VID* const vid = loaded;
            vid->nVid = nVid;
            vid->type = headFlags;
            vid->name = name;
            vid->vidName = source;
            (void)archive->Read(&vid->frameSpeedDefault, 2u);
            (void)archive->Read(&vid->noCadr, 2u);
            (void)archive->Read(&vid->vidSizeX, 2u);
            (void)archive->Read(&vid->vidSizeY, 2u);
            vid->buildSizeX = static_cast<float>(vid->vidSizeX);
            vid->buildSizeY = static_cast<float>(vid->vidSizeY);
            vid->LoadParameters(resource);
            vid->Load(archive);
        }
    }
    MapResourceLogLoadedVid(loaded);
    loaded->SetLayer();
    return loaded;
}

static void __cdecl MapPresentationCallback(int mode)
{
    const ANGLE direction(mapLoadingDirection.Get().value);
    VID* const vid = mapLoadingVid;
    const int result = Graph->DrawLoadBar(mapLoadingText.Get(), vid, direction, static_cast<int>(mode));
    if (result != 0)
        ::Sound->MusicTact();
}

void __cdecl DispatchMapPresentation(int mode)
{
    DrawLoadScreenTact(mode);
}


static bool MapCtorContains(const STRING* text, const char* fragment)
{
    return std::strstr(text->value, fragment) != 0;
}

static void MapCtorCurrentDirectory(STRING& out)
{
    out = FCurrentDirectory();
}

static void MapCtorProfileLoad(PROFILE& profile, const STRING& fileName)
{
    (void)profile.Load(fileName);
}

static int MapCtorProfileInt(PROFILE& profile, const char* section, const char* key, int defaultValue)
{
    STRING sectionString(section), keyString(key);
    return profile.GetInt(sectionString, keyString, defaultValue);
}

static void MapCtorProfileString(PROFILE& profile, STRING& out, const char* section, const char* key, const char* defaultValue)
{
    STRING sectionString(section), keyString(key), defaultString(defaultValue);
    out = profile.GetString(sectionString, keyString, defaultString);
}

static int MapCtorRegistryInt(REGISTRY* registry, const char* key, int defaultValue)
{
    STRING keyString(key);
    return registry->GetInt(keyString, defaultValue);
}

static void MapCtorRegistrySetString(REGISTRY* registry, const char* key, const STRING& value)
{
    STRING keyString(key);
    registry->SetString(keyString, value);
}

static void MapCtorMakePath(STRING& out, const STRING& base, const char* suffix)
{
    out = base;
    out += suffix;
}

static void MapCtorCreateDirectory(const STRING& path)
{
    (void)_mkdir(path.m_value);
}

static void MapCtorEnsureUserFolders(const STRING& userRoot)
{
    STRING path;
    MapCtorCreateDirectory(userRoot);
    MapCtorMakePath(path, userRoot, "Saves");
    MapCtorCreateDirectory(path);
    MapCtorMakePath(path, userRoot, "Logs");
    MapCtorCreateDirectory(path);
    MapCtorMakePath(path, userRoot, "Screens");
    MapCtorCreateDirectory(path);
    MapCtorMakePath(path, userRoot, "Send\\images\\cl.css");
    std::FILE* stream = std::fopen(path.value, "rb");
    if (stream)
    {
        (void)static_cast<unsigned int>(std::fclose(stream));
        return;
    }
    MapCtorMakePath(path, userRoot, "Send");
    MapCtorCreateDirectory(path);
    MapCtorMakePath(path, userRoot, "Send\\images");
    MapCtorCreateDirectory(path);
    static const char* const sourceSuffix[3] =
    {
        "\\send\\images\\cl.css", "\\send\\images\\pic.jpg", "\\send\\images\\send.jpg"
    }
    ;
    static const char* const destSuffix[3] =
    {
        "Send\\images\\cl.css", "Send\\images\\pic.jpg", "Send\\images\\send.jpg"
    }
    ;
    for (unsigned int i = 0;i<3u;++i)
    {
        STRING source, destination;
        MapCtorCurrentDirectory(source);
        source += sourceSuffix[i];
        destination = userRoot;
        destination += destSuffix[i];
        (void)CopyFileA(source.value, destination.value, 0);
    }
}

static void MapCtorInitializeClockAndTables()
{
    (void)timeBeginPeriod(1u);
    const unsigned int now = timeGetTime();
    ::RealCurrentTime = now;
    ::PrevRealCurrentTime = now-10u;
    ANGLE::InitializeYTables(0.7070602f);
}

static unsigned int MapCtorFwrite(unsigned int file, const void* data, unsigned int bytes)
{
    return static_cast<unsigned int>(std::fwrite(data, 1u, bytes, reinterpret_cast<std::FILE*>(U32ToPtr(file))));
}

static unsigned int MapCtorBytesPerPixelOrBlock(unsigned int format)
{
    if (format == 0x31545844u)
        return 4u;
    if (format == 0x33545844u || format == 0x35545844u)
        return 8u;
    if (format == 0x29u)
        return 1u;
    if (format >= 0x14u && format <= 0x1Au)
    {
        const unsigned int formatIndex = format-0x14u;
        if (formatIndex == 0u)
            return 3u;
        if (formatIndex <= 2u)
            return 4u;
        return 2u;
    }
    return 0u;
}

static void MapCtorSerializeFastLoadEntry(void* rawEntry, unsigned int indexFile, unsigned int dataFile, const unsigned int cacheStamp[2])
{
    VID_SURFACE* const entry = static_cast<VID_SURFACE*>(rawEntry);
    if (!entry || (entry->type&0x40u) == 0u || (entry->surfaceFlags&0x02u) != 0u)
        return;
    STRING* const path = &entry->vidName;
    unsigned int creation[2] =
    {
        0u, 0u
    }
    , access[2] =
    {
        0u, 0u
    }
    , write[2] =
    {
        0u, 0u
    }
    ;
    (void)FGetFileTime(*path, reinterpret_cast<_FILETIME*>(creation), reinterpret_cast<_FILETIME*>(access), reinterpret_cast<_FILETIME*>(write));
    RESOURCE videoResource(RESOURCE::OPEN_MODE_APPEND, *path, ResourceType::Video);
    RESOURCE* const resource = &videoResource;
    if (static_cast<unsigned int>(resource->GoBegin(ResourceType::Header)) != 0u)
    {
        return;
    }
    std::FILE* const resourceFile = resource->FileHandle();
    const int current = static_cast<int>(static_cast<unsigned int>(std::ftell(resourceFile)));
    const int packedSize = static_cast<int>(resource->PackedSize());
    const int subRecordSize = static_cast<int>(static_cast<unsigned int>(resource->SubRecordSize()));
    if (subRecordSize-current+packedSize+4<34)
    {
        return;
    }
    const int entryKey = entry->nVid;
    VID* cursor = entry;
    if (entry->nextMirror != entry)
    {
        for (;;)
        {
            if (cursor->nVid<entryKey)
                break;
            cursor = cursor->nextMirror;
            if (cursor->nextMirror == entry)
                break;
        }
    }
    if (cursor->nVid<entryKey)
    {
        return;
    }
    MYERROR* const logger = ::Error;
    (void)logger->Log("fast load file %s", path->value);
    const unsigned int dataOffset = static_cast<unsigned int>(std::ftell(reinterpret_cast<std::FILE*>(U32ToPtr(indexFile))));
    (void)static_cast<unsigned int>(std::fseek(resourceFile, BitsToSigned32(22u), static_cast<int>(1u)));
    (void)resource->Write(&dataOffset, 4u);
    (void)resource->Write(cacheStamp, 8u);
    resource->Close();
    (void)FSetFileTime(*path, reinterpret_cast<_FILETIME*>(creation), reinterpret_cast<_FILETIME*>(access), reinterpret_cast<_FILETIME*>(write));
    const unsigned int framesBase = PtrToU32(entry->frames);
    if (!framesBase)
    {
        return;
    }
    const int frameCount = entry->noCadr;
    if ((entry->type&0x0200u) != 0u)
        MapCtorFwrite(dataFile, entry->cadrLinkXYZ, static_cast<unsigned int>(frameCount*12));
    if ((entry->type&0x0008u) != 0u)
        MapCtorFwrite(dataFile, entry->palette, 1024u);
    VID* child = entry;
    if (entry->nextMirror != entry)
    {
        for (;;)
        {
            if ((child->property&0x20u) != 0u)
                break;
            child = child->nextMirror;
            if (child->nextMirror == entry)
                break;
        }
    }
    unsigned int childCountMarker = 0u;
    if ((child->property&0x20u) != 0u)
    {
        childCountMarker = static_cast<unsigned int>(child->noGridZ);
        if (childCountMarker != 0u)
        {
            MapCtorFwrite(dataFile, child->gridZ, childCountMarker*12u);
            const int childFrameCount = child->noCadr;
            MapCtorFwrite(dataFile, child->gridCadrShift, static_cast<unsigned int>(childFrameCount*4));
        }
    }
    MapCtorFwrite(dataFile, &childCountMarker, 4u);
    unsigned int* const offsets = static_cast<unsigned int*>(CrtNew(static_cast<unsigned int>(frameCount*4)));
    const unsigned int pixelBase = PtrToU32(entry->surfaceData);
    const unsigned int format = entry->pixelFormat;
    for (int frame = 0;frame<frameCount;++frame)
    {
        CADR_DATA& cadr = entry->frames[frame];
        offsets[frame] = static_cast<unsigned int>(std::ftell(reinterpret_cast<std::FILE*>(U32ToPtr(dataFile))));
        const unsigned int blockOffset = cadr.frameDataOffset;
        const unsigned int countA = cadr.indexCount;
        const unsigned int countB = cadr.vertexCount;
        MapCtorFwrite(dataFile, U32ToPtr(pixelBase+blockOffset), countA*2u);
        MapCtorFwrite(dataFile, U32ToPtr(pixelBase+blockOffset+countA*2u), countB*4u);
        const unsigned int rowCount = cadr.height;
        if (rowCount != 0u)
        {
            unsigned char* payload = U32ToPtr(pixelBase+blockOffset+(countA+countB*2u)*2u);
            if (format == 0x31545844u || format == 0x33545844u || format == 0x35545844u)
            {
                const unsigned int bits = (format == 0x31545844u)?4u:8u;
                const int width = static_cast<short>(cadr.width);
                const unsigned int bytes = static_cast<unsigned int>((width*static_cast<int>(rowCount)*static_cast<int>(bits)+7)/8);
                MapCtorFwrite(dataFile, payload, bytes);
            }
            else
            {
                const unsigned int unit = MapCtorBytesPerPixelOrBlock(format);
                unsigned char* scan = payload;
                while (ReadU16(scan, 0u) != 0u)
                {
                    MapCtorFwrite(dataFile, scan, 2u);
                    ++scan;
                    unsigned int runs = ReadU8(scan, 0u);
                    ++scan;
                    for (unsigned int r = 0;r<runs;++r)
                    {
                        MapCtorFwrite(dataFile, scan, unit);
                        scan += unit;
                        if ((entry->type&0x2000u) != 0u)
                            ++scan;
                        if ((entry->type&0x0004u) != 0u)
                            scan += 2u;
                    }
                }
                MapCtorFwrite(dataFile, scan, 2u);
            }
        }
    }
    for (int frame = 0;frame<frameCount;++frame)
    {
        CADR_DATA& cadr = entry->frames[frame];
        cadr.CalcMaxMinZ(entry);
        MapCtorFwrite(indexFile, &cadr.indexCount, sizeof(cadr.indexCount));
        MapCtorFwrite(indexFile, &cadr.vertexCount, sizeof(cadr.vertexCount));
        MapCtorFwrite(indexFile, &cadr.width, sizeof(cadr.width));
        MapCtorFwrite(indexFile, &cadr.height, sizeof(cadr.height));
        MapCtorFwrite(indexFile, &cadr.offsetX, sizeof(cadr.offsetX));
        MapCtorFwrite(indexFile, &cadr.offsetY, sizeof(cadr.offsetY));
        MapCtorFwrite(indexFile, &offsets[frame], sizeof(offsets[frame]));
        MapCtorFwrite(indexFile, &cadr.minZ, sizeof(cadr.minZ));
        MapCtorFwrite(indexFile, &cadr.maxZ, sizeof(cadr.maxZ));
    }
    MapCtorFwrite(indexFile, &entry->minXGridZ, sizeof(entry->minXGridZ));
    MapCtorFwrite(indexFile, &entry->maxXGridZ, sizeof(entry->maxXGridZ));
    MapCtorFwrite(indexFile, &entry->minYGridZ, sizeof(entry->minYGridZ));
    MapCtorFwrite(indexFile, &entry->maxYGridZ, sizeof(entry->maxYGridZ));
    (void)(CrtFreePointer(offsets), 0u);
}




MAP::MAP(HINSTANCE__* instance, HINSTANCE__* previous, const STRING& commandText, int show, GRAPH_INIT* init)
    : m_mouseTips()
{
    void* const self = this;
    const unsigned int hInstance = PtrToU32(instance);
    const unsigned int hPrevInstance = PtrToU32(previous);
    const unsigned int nShowCmd = static_cast<unsigned int>(show);
    const unsigned int empty = PtrToU32(StringGlobals::EmptySentinel());
    MAP* const map = this;
    const STRING* const command = &commandText;
    GRAPH_INIT* const graphInit = init;
    PROFILE profile;
    STRING executablePath(crtProgramPath ? crtProgramPath : "");
    STRING fileName = executablePath.AfterLast("\\");
    STRING baseName = fileName.BeforeLast(".");
    STRING configPath;
    MapCtorCurrentDirectory(configPath);
    configPath += "\\";
    configPath += baseName.value;
    configPath += ".cfg";
    MapCtorProfileLoad(profile, configPath);
    if (MapCtorContains(command, ".cfg"))
    {
        STRING commandPath;
        MapCtorCurrentDirectory(commandPath);
        commandPath += "\\";
        commandPath += command->value;
        MapCtorProfileLoad(profile, commandPath);
    }
    char documents[260];
    documents[0] = 0;
    if (SHGetFolderPathA(0, 5, 0, 0u, documents) >= 0)
    {
        map->m_windowsUserPath = documents;
        STRING titleForPath;
        MapCtorProfileString(profile, titleForPath, "common", "Title", "Unknown");
        STRING suffix("\\");
        suffix += titleForPath.value;
        suffix += " Saves\\";
        map->m_windowsUserPath += suffix;
    }
    MapCtorEnsureUserFolders(map->m_windowsUserPath);
    if (::Error == 0)
    {
        STRING logPath;
        MapCtorMakePath(logPath, map->m_windowsUserPath, "Logs");
        MYERROR* const logger = new MYERROR(logPath.value, 1);
        ::Error = logger;
    }
    MYERROR* const logger = ::Error;
    (void)logger->Log("WindowsUserPath is '%s'", map->m_windowsUserPath.m_value);
    const bool createFastLoad = MapCtorContains(command, "createfastload");
    MapCtorInitializeClockAndTables();
    map->m_flags = (map->m_flags&0xFE6A0001u)|0x00220000u;
    map->m_sizeXY.x = 640.0f;
    map->m_sizeXY.y = 480.0f;
    map->m_shiftXY.x = 0.0f;
    map->m_shiftXY.y = 1.0f;
    map->m_groundPrimary = 0;
    map->m_groundSecondary = 0;
    map->m_playerIndex = 0u;
    map->m_weaponData = 0;
    map->m_weaponCount = 0;
    map->m_vidCount = 0;
    map->m_simulationFrameCount = 0u;
    map->m_tick60 = ::CurrentTime;
    map->m_scale40 = 1.0f;
    map->m_scrollFollowSpeedScale = 1.0f;
    map->m_frameRate04 = 0u;
    map->m_frameCounter08 = 0u;
    map->m_scrollMode = 1u;
    map->m_cameraMinX = 0.0f;
    map->m_cameraMinY = 0.0f;
    map->m_cameraMaxX = 640.0f;
    map->m_cameraMaxY = 480.0f;
    map->m_fastLoadIndex = 0;
    map->m_fastLoadData = 0;
    for (unsigned int i = 0;i<static_cast<unsigned int>(VidSlotCount);++i)
        map->m_vids[i] = 0;
    for (unsigned int i = 0;i<8u;++i)
        map->m_players[i] = 0;
    map->m_hInstance = U32ToPtr(hInstance);
    map->ResetGroundZ();
    DrawLoadScreenTact = &MapPresentationCallback;
    MEMORYSTATUS memoryStatus =
    {
    }
    ;
    memoryStatus.dwLength = 32u;
    GlobalMemoryStatus(&memoryStatus);
    (void)logger->Log("Memory:: TotalPhys=%u AvailPhys=%u TotalPageFile=%u AvailPageFile=%u TotalVirtual=%u AvailVirtual=%u", memoryStatus.dwTotalPhys, memoryStatus.dwAvailPhys, memoryStatus.dwTotalPageFile, memoryStatus.dwAvailPageFile, memoryStatus.dwTotalVirtual, memoryStatus.dwAvailVirtual);
    const HRESULT comResult = CoInitializeEx(0, 0u);
    if (comResult<0)
    {
        const unsigned int errorSink = PtrToU32(::Error);
        if (errorSink != 0u)
        {
            STRING object("MAP"), subject("COM");
            (void)reinterpret_cast<MYERROR*>(U32ToPtr(errorSink))->Error( object, E_INIT, subject, static_cast<unsigned int>(comResult));
        }
        return;
    }
    map->m_comObject = 0;
    {
        STRING key("UniqueLog");
        STRING section("game");
        const unsigned int unique = static_cast<unsigned int>(profile.GetInt(section, key, 0));
        map->m_flags = (map->m_flags&~1u)|(unique&1u);
    }
    if (::g_pDlgSrv == 0)
    {
        void* const memory = CrtNew(116u);
        zDlgSrv* dialogs = 0;
        if (memory)
        {
            try
            {
                dialogs = static_cast<zDlgSrv*>(zDlgSrv_Ctor(memory));
            }
            catch (...)
            {
                CrtFreePointer(memory);
                throw;
            }
        }
        ::g_pDlgSrv = dialogs;
    }
    if (PtrToU32(::g_pDlgSrv))
    {
        STRING dialogs;
        MapCtorProfileString(profile, dialogs, "game", "Dialogs", "");
        zDlgSrv_LoadDialogs(reinterpret_cast<unsigned char*>(::g_pDlgSrv), dialogs);
    }
    {
        PROFILE* const fx = new PROFILE();
        STRING fxPath = FCurrentDirectory();
        fxPath += "\\vid\\12345.fx";
        (void)fx->Load(fxPath);
        ::Strings = fx;
    }
    STRING title;
    MapCtorProfileString(profile, title, "common", "Title", "Unknown");
    map->m_windowTitle = title;
    {
        void* const memory = CrtNew(sizeof(REGISTRY));
        REGISTRY* newRegistry = 0;
        if (memory)
        {
            try
            {
                STRING defaultReg("SOFTWARE\\SigmaTeam\\");
                defaultReg += title.value;
                STRING regPath;
                MapCtorProfileString(profile, regPath, "common", "RegPath", defaultReg.value);
                newRegistry = new(memory) REGISTRY(regPath);
            }
            catch (...)
            {
                CrtFreePointer(memory);
                throw;
            }
        }
        ::Registry = newRegistry;
    }
    REGISTRY* const registry = ::Registry;
    {
        void* const memory = CrtNew(sizeof(NET));
        NET* net = 0;
        if (memory)
        {
            try
            {
                STRING netTitle;
                netTitle = map->m_windowTitle;
                net = new(memory) NET(instance, netTitle);
            }
            catch (...)
            {
                CrtFreePointer(memory);
                throw;
            }
        }
        ::Net = net;
    }
    const HWND existing = FindWindowA(baseName.value, map->m_windowTitle.m_value);
    if (existing)
    {
        (void)SetForegroundWindow(existing);
        (void)ShowWindow(existing, 1);
        return;
    }
    map->m_playerIndex = 0u;
    STRING gamePath;
    MapCtorCurrentDirectory(gamePath);
    MapCtorRegistrySetString(registry, "GamePath", gamePath);
    {
        STRING registryLog("Registry path = '");
        registryLog += registry->Path().value;
        registryLog += "'";
        logger->Log(registryLog);
    }
    {
        STRING key("VSync");
        STRING section("graph");
        const int value = profile.GetInt(section, key, 1);
        graphInit->flags |= value != 0?0x02u:0u;
    }
    {
        STRING key("VertexSoftware");
        STRING section("graph");
        const int value = profile.GetInt(section, key, 0);
        graphInit->flags |= value != 0?0x10u:0u;
    }
    {
        STRING key("SaveDemo");
        STRING section("game");
        const int value = profile.GetInt(section, key, 0);
        graphInit->flags |= value != 0?0x08u:0u;
    }
    {
        STRING key("StartDialogIsFull");
        STRING section("game");
        const int value = profile.GetInt(section, key, 0);
        graphInit->flags |= value != 0?0x04u:0u;
    }
    {
        STRING key("Device");
        graphInit->adapter = static_cast<unsigned int>(registry->GetInt(key, static_cast<int>(graphInit->adapter)));
    }
    {
        STRING key("ScreenX");
        STRING section("graph");
        STRING registryKey("ScreenX");
        const int configured = profile.GetInt(section, key, static_cast<int>(graphInit->width));
        graphInit->width = static_cast<unsigned int>(registry->GetInt(registryKey, configured));
    }
    {
        STRING key("ScreenY");
        STRING section("graph");
        STRING registryKey("ScreenY");
        const int configured = profile.GetInt(section, key, static_cast<int>(graphInit->height));
        graphInit->height = static_cast<unsigned int>(registry->GetInt(registryKey, configured));
    }
    {
        STRING key("BPP");
        STRING section("graph");
        STRING registryKey("BPP");
        const int configured = profile.GetInt(section, key, static_cast<int>(graphInit->colorBits));
        graphInit->colorBits = static_cast<unsigned int>(registry->GetInt(registryKey, configured));
    }
    {
        STRING key("FullScreen");
        graphInit->fullScreen = static_cast<unsigned int>(registry->GetInt(key, static_cast<int>(graphInit->fullScreen)));
    }
    const char* source = map->m_windowTitle.value;
    char* destination = graphInit->title;
    while ((*destination++ = *source++) != 0)
    {
    }
    Graph = new GRAPH(graphInit);
    GRAPH* const graph = Graph;
    g_iZmDebug = MapCtorRegistryInt(registry, "zmTEMP", 0);
    MapUltraLowDetail = MapCtorRegistryInt(registry, "zmUltraLow", 0);
    if (g_iZmDebug != 0u && MapCtorContains(command, "secondexe"))
        g_iZmDebug = 2;
    if (MapCtorProfileInt(profile, "game", "StartDialog", 0) != 0)
    {
        const INT_PTR result = DialogBoxParamA(reinterpret_cast<HINSTANCE>(U32ToPtr(hInstance)), "START_DIALOG", 0, &AppStart, 0);
        if (result == 0u)
        {
            return;
        }
    }
    const bool noSystemMenu = MapCtorRegistryInt(registry, "zNoSysMenu", 0) != 0;
    if (hPrevInstance == 0u)
    {
        WNDCLASSA windowClass =
        {
        }
        ;
        windowClass.style = 0x0Bu;
        windowClass.lpfnWndProc = &AppWndProc;
        windowClass.hInstance = reinterpret_cast<HINSTANCE>(U32ToPtr(hInstance));
        windowClass.hIcon = LoadIconA(windowClass.hInstance, "AppIcon");
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(4));
        windowClass.lpszClassName = baseName.value;
        if (RegisterClassA(&windowClass) == 0u)
        {
            return;
        }
    }
    int windowX = 0, windowY = 0;
    if ((graph->capabilityFlags&0x40u) == 0u)
    {
        windowX = MapCtorRegistryInt(registry, "WindowPositionX", 0);
        windowY = MapCtorRegistryInt(registry, "WindowPositionY", 0);
    }
    Map = map;
    unsigned int style = 0x90000000u;
    if ((graph->capabilityFlags&0x40u) == 0u && !noSystemMenu)
        style = 0x90CA0000u;
    const int width = static_cast<int>(graph->screenWidth);
    const int height = static_cast<int>(graph->screenHeight);
    const unsigned int hwnd = PtrToU32(CreateWindowExA(0x00040000u, baseName.value, map->m_windowTitle.m_value, style, windowX, windowY, width, height, 0, 0, reinterpret_cast<HINSTANCE>(U32ToPtr(hInstance)), 0));
    map->m_dialog = hwnd;
    (void)ShowWindow(reinterpret_cast<HWND>(U32ToPtr(hwnd)), static_cast<int>(nShowCmd));
    (void)UpdateWindow(reinterpret_cast<HWND>(U32ToPtr(hwnd)));
    map->m_dialogWindow = PtrToU32(LoadAcceleratorsA(reinterpret_cast<HINSTANCE>(U32ToPtr(hInstance)), "AppAccel"));
    (void)ShowCursor(0);
    if (graph->Init(reinterpret_cast<HWND__*>(U32ToPtr(hwnd))) != 0)
    {
        return;
    }
    (void)graph->SetGraphDetail(MapCtorProfileInt(profile, "graph", "GraphDetail", 10));
    STRING font;
    MapCtorProfileString(profile, font, "graph", "Font", "Arial");
    const int fontX = MapCtorProfileInt(profile, "graph", "FontSizeX", 8);
    const int fontY = MapCtorProfileInt(profile, "graph", "FontSizeY", 0);
    graph->SetFont(font, fontX, fontY);
    {
        RESOURCE mapResource;
        RESOURCE* const resource = &mapResource;
        {
            STRING resourceName;
            MapCtorProfileString(profile, resourceName, "game", "Resource", "objects.res");
            map->m_resourceFileName = resourceName;
        }
        if (static_cast<unsigned int>(resource->OpenForRead(map->m_resourceFileName, ResourceType::Data)) != 0u)
        {
            const unsigned int errorSink = PtrToU32(::Error);
            if (errorSink != 0u)
            {
                STRING object("MAP"), subject("resource file");
                (void)reinterpret_cast<MYERROR*>(U32ToPtr(errorSink))->Error( object, E_OPEN, subject, 0u);
            }
            return;
        }
        {
            void* const soundMemory = CrtNew(sizeof(SOUND));
            SOUND* sound = 0;
            if (soundMemory)
            {
                try
                {
                    const int highQuality = MapCtorRegistryInt(registry, "SoundHighQuality", 0);
                    sound = new(soundMemory) SOUND(reinterpret_cast<HWND__*>(U32ToPtr(hwnd)), resource, highQuality);
                }
                catch (...)
                {
                    CrtFreePointer(soundMemory);
                    throw;
                }
            }
            ::Sound = sound;
            void* const constantsMemory = CrtNew(sizeof(CONSTANT));
            CONSTANT* constants = 0;
            if (constantsMemory)
            {
                try
                {
                    constants = new(constantsMemory) CONSTANT(resource);
                }
                catch (...)
                {
                    CrtFreePointer(constantsMemory);
                    throw;
                }
            }
            Const = constants;
            ::Const = constants;
            {
                STRING key("DebugMode");
                STRING section("game");
                Const->DebugMode = profile.GetInt(section, key, 0);
            }
            {
                STRING key("DrawFPS");
                STRING section("game");
                const unsigned int value = static_cast<unsigned int>(profile.GetInt(section, key, 0));
                map->m_flags = (map->m_flags&~0x00040000u)|((value&1u)<<18u);
            }
            {
                STRING key("DrawPresentation");
                STRING section("game");
                const unsigned int value = static_cast<unsigned int>(profile.GetInt(section, key, 1));
                map->m_flags = (map->m_flags&~0x00080000u)|((value&1u)<<19u);
            }
            {
                STRING key("DrawDebugInfo");
                STRING section("game");
                const unsigned int value = static_cast<unsigned int>(profile.GetInt(section, key, 0));
                map->m_flags = (map->m_flags&~0x00020000u)|((value&1u)<<17u);
            }
        }
        (void)CrtControlFp(0x00020000u, 0x00030000u);
        map->m_cameraMinX = 0.0f;
        map->m_cameraMaxX = map->m_sizeXY.x;
        map->m_cameraMinY = 0.0f;
        map->m_cameraMaxY = map->m_sizeXY.y;
        {
            float point[3] =
            {
                graph->screenHeight*0.5f, graph->screenWidth*0.5f, 0.0f
            }
            ;
            map->SetShiftCoor(VECTOR2(point[0], point[1]), 0);
        }
        {
            void* const gridMemory = CrtNew(sizeof(SPRITE_COLLECTOR));
            SPRITE_COLLECTOR* collector = 0;
            if (gridMemory)
            {
                try
                {
                    collector = new(gridMemory) SPRITE_COLLECTOR(map->m_sizeXY);
                }
                catch (...)
                {
                    CrtFreePointer(gridMemory);
                    throw;
                }
            }
            ::Sprites = collector;
        }
        (void)SetActiveWindow(reinterpret_cast<HWND>(U32ToPtr(hwnd)));
        (void)SetFocus(reinterpret_cast<HWND>(U32ToPtr(hwnd)));
        (void)SetForegroundWindow(reinterpret_cast<HWND>(U32ToPtr(hwnd)));
        (void)map->StartTact();
        if (createFastLoad)
        {
            (void)DeleteFileA("vid\\fast_load.vid");
            (void)DeleteFileA("vid\\fast_load_data.vid");
        }
        map->LoadVid(resource);
        (void)ShowCursor(1);
    }
    (void)SetCursor(0);
    const char* const emptyText = StringGlobals::EmptyText();
    STRING configuredLanguage;
    MapCtorProfileString(profile, configuredLanguage, "common", "Language", emptyText);
    bool languageChanged = false;
    {
        const char* languageCursor = configuredLanguage.value;
        const char* emptyLanguageCursor = emptyText;
        while (*languageCursor && *languageCursor == *emptyLanguageCursor)
        {
            ++languageCursor;
            ++emptyLanguageCursor;
        }
        languageChanged = static_cast<unsigned char>(*languageCursor) != static_cast<unsigned char>(*emptyLanguageCursor);
    }
    if (languageChanged)
    {
        STRING selectedLanguage;
        MapCtorProfileString(profile, selectedLanguage, "common", "Language", "409");
        map->SetKeyboardLocale(selectedLanguage);
    }
    {
        void* const nodeMemory = CrtNew(sizeof(MOUSE));
        MOUSE* node = 0;
        if (nodeMemory)
        {
            try
            {
                const VECTOR2& size = graph->SizeXY();
                const VECTOR position(size.x*0.5f, size.y*0.5f, 0.0f);
                node = new(nodeMemory) MOUSE(::EmptyVid, position, ANGLE(0), 0);
            }
            catch (...)
            {
                CrtFreePointer(nodeMemory);
                throw;
            }
        }
        Mouse = node;
        ::Mouse = Mouse;
        Mouse->Enable();
    }
    {
        STRING bossKey;
        MapCtorProfileString(profile, bossKey, "game", "BossKey", emptyText);
    }
    {
        STRING inputCfg("input.cfg");
        (void)zInputMap::Load(inputCfg.CharPtr());
        map->m_input.Init(reinterpret_cast<HWND__*>(AddressToUIntPtr(hwnd)));
    }
    for (unsigned int i = 0;i<8u;++i)
    {
        void* const memory = CrtNew(sizeof(PLAYER_ARCADE));
        PLAYER_ARCADE* player = 0;
        if (memory)
        {
            try
            {
                player = new(memory) PLAYER_ARCADE(i);
            }
            catch (...)
            {
                CrtFreePointer(memory);
                throw;
            }
        }
        map->m_players[i] = player;
    }
    map->m_cameraMinX = 0.0f;
    map->m_cameraMaxX = map->m_sizeXY.x;
    map->m_cameraMinY = 0.0f;
    map->m_cameraMaxY = map->m_sizeXY.y;
    if (command->value[0] && !MapCtorContains(command, ".cfg") && !MapCtorContains(command, "secondexe"))
        map->m_pendingMapFileName = *command;
    else
    {
        STRING startMap;
        MapCtorProfileString(profile, startMap, "game", "StartMap", "maps\\logo.map");
        map->m_pendingMapFileName = startMap;
    }
    if (createFastLoad)
    {
        const unsigned int indexFile = PtrToU32(std::fopen("vid\\fast_load.vid", "wb+"));
        const unsigned int dataFile = PtrToU32(std::fopen("vid\\fast_load_data.vid", "wb+"));
        unsigned int systemTime[2] =
        {
            0u, 0u
        }
        ;
        GetSystemTimeAsFileTime(reinterpret_cast<FILETIME*>(systemTime));
        MapCtorFwrite(indexFile, systemTime, 8u);
        const int count = m_vidCount;
        for (int i = 0;i<count;++i)
            MapCtorSerializeFastLoadEntry(m_vids[i], indexFile, dataFile, systemTime);
        if (dataFile)
            (void)static_cast<unsigned int>(std::fclose(reinterpret_cast<std::FILE*>(U32ToPtr(dataFile))));
        if (indexFile)
            (void)static_cast<unsigned int>(std::fclose(reinterpret_cast<std::FILE*>(U32ToPtr(indexFile))));
    }
    map->m_flags |= 0x08u;
    (void)logger->Log("sizeof(SPRITE) = %i  sizeof(INPUT) = %i", 104u, 39u);
    return;
}


MAP::~MAP() noexcept(false)
{
    void* const self = this;
    MAP* const owner = this;
    MOUSETIPS* const mouseTips = &owner->m_mouseTips;
    mouseTips->Clear();
    if (::Sprites != 0)
        static_cast<MAP*>(self)->Release();
    const unsigned int comObject = PtrToU32(owner->m_comObject);
    if (comObject != 0u)
        (void)DeleteObject(reinterpret_cast<HGDIOBJ>(U32ToPtr(comObject)));
    if (SPRITE_COLLECTOR* const collector = ::Sprites)
    {
        collector->~SPRITE_COLLECTOR();
        CrtFreePointer(collector);
        ::Sprites = 0;
    }
    if (Mouse)
    {
        MOUSE* const mouse = Mouse;
        mouse->~MOUSE();
        CrtFreePointer(mouse);
        Mouse = 0;
        ::Mouse = 0;
    }
    for (unsigned int i = 0u;i<8u;++i)
    {
        PLAYER* const player = owner->m_players[i];
        delete player;
    }
    if (zDlgSrv* const dialogs = ::g_pDlgSrv)
    {
        (void)zDlgSrv_DeletingDtor(dialogs, 1u);
        ::g_pDlgSrv = 0;
    }
    if (NET* const network = ::Net)
    {
        network->~NET();
        CrtFreePointer(network);
    }
    if (PROFILE* const profile = ::Strings)
    {
        delete profile;
    }
    if (SOUND* const sound = ::Sound)
    {
        sound->~SOUND();
        CrtFreePointer(sound);
    }
    if (CONSTANT* const constants = ::Const)
        CrtFreePointer(constants);
    if (REGISTRY* const registry = ::Registry)
    {
        registry->~REGISTRY();
        CrtFreePointer(registry);
    }
    for (int index = owner->m_vidCount-1;index >= 0;--index)
    {
        VID* const vid = owner->m_vids[static_cast<unsigned int>(index)];
        if (vid)
        {
            vid->~VID();
            CrtFreePointer(vid);
            owner->m_vids[static_cast<unsigned int>(index)] = 0;
        }
    }
    owner->m_vidCount = 0;
    (void)::Error->Log("Vid    release (vid mem = %i)", VID::MemoryInUse);
    unsigned char* const mapped0 = owner->m_fastLoadIndex;
    if (mapped0)
        (void)UnmapViewOfFile(mapped0);
    unsigned char* const mapped1 = owner->m_fastLoadData;
    if (mapped1)
        (void)UnmapViewOfFile(mapped1);
    delete Graph;
    CrtFreePointer(owner->m_groundPrimary);
    CrtFreePointer(owner->m_groundSecondary);
    CrtFreePointer(owner->m_weaponData);
    CoUninitialize();
    (void)timeEndPeriod(1u);
    if ((static_cast<unsigned char>(owner->m_flags)&1u) != 0u)
    {
        STRING message("end successful");
        ::Error->LogExit(message);
    }
    if (MYERROR* const logger = ::Error)
    {
        delete logger;
    }
}


