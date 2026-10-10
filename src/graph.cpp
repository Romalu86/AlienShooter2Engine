
#include "myerror.h"
#include "engine_globals.h"
#include "zm/Utils/zStaticStorage.h"
#include <stdarg.h>
#include <stdio.h>
#if defined(_WIN32)
#define INPUT WIN32_INPUT
#include <windows.h>
#include <mmsystem.h>
#include <d3d9.h>
#include <dshow.h>
#undef INPUT
#endif

#include "mylib.h"
#include "basetype.h"
#include "graph.h"
#include "legacy_d3dx_font.h"
#include "legacy_d3dx_effect.h"
#include "system_font.h"
#include "region.h"
#include "vid.h"
#include "list.h"
#include "texture.h"
#include "registry.h"
#include <malloc.h>
#include <new>
#include <cstdlib>
#include <cstring>
#include "map.h"
#include "sprite_collector.h"
#include "sprite.h"

GRAPH* Graph=0;

COLOR GRAPH::YELLOW;
COLOR GRAPH::LIGHTRED;
COLOR GRAPH::RED;
COLOR GRAPH::LIGHTGREEN;
COLOR GRAPH::GREEN;
COLOR GRAPH::LIGHTBLUE;
COLOR GRAPH::BLUE;
COLOR GRAPH::GRAY;
COLOR GRAPH::WHITE;
COLOR GRAPH::BROWN;
COLOR GRAPH::MAGENTA;
COLOR GRAPH::CYAN;
COLOR GRAPH::BLACK;

namespace {
#if defined(AS2_VENGEANCE)
constexpr char pseudoEffectPath[] = "vid\\pseudo4d.fx";
constexpr char pseudoEffectFileName[] = "pseudo4d.fx";
#else
constexpr char pseudoEffectPath[] = "vid\\pseudo3d.fx";
constexpr char pseudoEffectFileName[] = "pseudo3d.fx";
#endif

StaticObjectStorage<STRING> textureStageDescription;
unsigned char textureStageDescriptionInitialized = 0;
StaticObjectStorage<VECTOR2> savedCameraShift;
unsigned char savedCameraShiftInitialized = 0;
unsigned int loadBarFrame = 0;
unsigned int loadBarLastDrawTime = 0;
unsigned int loadBarMode = 0;
float squallBaseMagnitude = -1.0f;
float rainWindMagnitude = 99999.0f;
float snowWindMagnitude = 99999.0f;
float snowHorizontalSpeed = 0.0f;
unsigned char snowWindDirection = 0;
unsigned char snowInitialized = 0;
float rainHorizontalSpeed = 0.0f;
float rainCameraX = 0.0f;
float rainCameraY = 0.0f;
unsigned char rainWindDirection = 0;
unsigned char rainInitialized = 0;
unsigned int squallStartTime = 0;
int rainParticleCount = 0;
float snowCameraX = 0.0f;
float snowCameraY = 0.0f;
int snowParticleCount = 0;
}


struct ID3DXBuffer : IUnknown
{
    virtual void* __stdcall GetBufferPointer()=0;
    virtual unsigned long __stdcall GetBufferSize()=0;
};





void*  GraphPackRGBA(void*,int,int,int,int);

void*  Vector2IntCtorAbiBridge(void* self,int first,int second);



const STRING operator+(const char* left, const STRING& right)
{
    return STRING(left, right.value);
}

const STRING Int2Str(int value)
{
    char buffer[128];
    const char* const text = reinterpret_cast<const char*>(AddressToUIntPtr(
        CrtItoa(value, PtrToU32(buffer), 10u)));
    return STRING(text);
}

int STRING::Length() const
{
    int length = 0;
    while (value[length] != 0)
        ++length;
    return length;
}



























void __cdecl GraphStaticCleanupNoOpPrimary()
{
}

void GRAPH::DrawFog(const VECTOR2&, const VECTOR2&, int, int, COLOR, unsigned short*, int, int)
{
}

void __cdecl GraphStaticCleanupNoOpSecondary()
{
}

void __cdecl GraphStaticCleanupNoOp()
{
}

void __cdecl GraphStaticCleanupNoOpTertiary()
{
}


VECTOR2 VECTOR2::operator/(float divisor) const
{
    const float scale = (1.0f)/divisor;
    return VECTOR2(scale*x, scale*y);
}

VECTOR::VECTOR(const VECTOR2& xyValue, float zValue)
    : x(xyValue.x), y(xyValue.y), z(zValue)
{
}

void __cdecl GraphReportStaticString()
{
    ::Error->Log("ReloadPalettes");
}

int GAMMA::Blue() const
{
    const unsigned int first = modulate&0xFFu;
    return first? -static_cast<int>(first):static_cast<int>(add&0xFFu);
}

void GRAPH::EndPause()
{
    capabilityFlags &= ~1u;
    movie.Resume();
}

int MOVIE::Update()
{
    if (!mediaEvent)
        return 1;
    long status;
    (void)mediaEvent->WaitForCompletion(0, &status);
    return status == 1?1:0;
}

unsigned int RGB16::rMask = 0xF800u;
unsigned int RGB16::gMask = 0x07E0u;
int RGB16::rShift = 8;
int RGB16::gShift = 3;

void RGB16::SetChannelMasks(bool rgb565)
{
    if (rgb565)
    {
        rMask = 0xF800u;
        gMask = 0x07E0u;
        rShift = 8;
        gShift = 3;
    }
    else
    {
        rMask = 0x7C00u;
        gMask = 0x03E0u;
        rShift = 7;
        gShift = 2;
    }
}

unsigned short RGB16::Pack(unsigned int color)
{
    return static_cast<unsigned short>(((color >> 3) & 0x1Fu)
        | ((color >> (8 - gShift)) & gMask)
        | ((color >> (16 - rShift)) & rMask));
}

unsigned int RGB16::Expand(unsigned short color)
{
    const unsigned int packed = color;
    const unsigned int red = (packed << (16 - rShift)) & 0xFFFF0000u;
    const unsigned int green = (packed << (8 - gShift)) & 0x0000FF00u;
    const unsigned int blue = ((packed & 0x1Fu) | 0xFFE00000u) << 3;
    return red | green | blue;
}

void GRAPH::BeginPause()
{
    capabilityFlags |= 1u;
    ::DrawMenuBar(reinterpret_cast<HWND>(window));
    ::RedrawWindow(reinterpret_cast<HWND>(window), 0, 0, RDW_FRAME);
    movie.Pause();
}

void GRAPH::SetDrawShadow(int enabled)
{
    if (enabled != 0 && graphDetail<5u)
        return;
    if (((deviceState&1u)^static_cast<unsigned int>(enabled)) == 0u)
        return;
    const unsigned int restoreSpecial = sceneState&1u;
    if (restoreSpecial)
        PostTact(1);
    deviceState ^= (deviceState^static_cast<unsigned int>(enabled))&1u;
    InvalidateDeviceObjects();
    RestoreDeviceObjects();
    if (restoreSpecial)
        PreTact();
}

long GRAPH::SetAlphaBlend(unsigned long sourceBlend, unsigned long destinationBlend) const
{
    (void)device->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
    (void)device->SetRenderState(D3DRS_SRCBLEND, static_cast<unsigned int>(sourceBlend));
    return static_cast<long>(device->SetRenderState(D3DRS_DESTBLEND, static_cast<unsigned int>(destinationBlend)));
}



void GRAPH::SetEnvironment(unsigned int mask)
{
    unsigned int flags = environmentFlags;
    if ((mask&0x80000000u) != 0u)
    {
        environmentFlags = flags&~mask;
        return;
    }
    if (mask == 1u || mask == 2u)
        flags &= ~3u;
    if ((mask&0x00000C00u) != 0u)
        flags &= ~0x00000C00u;
    if ((mask&0x0000C000u) != 0u)
        flags &= ~0x0000C000u;
    environmentFlags = flags|mask;
}

void GRAPH::SetWind(int magnitude, ANGLE direction)
{
    const float scale = static_cast<float>(magnitude)*(0.00100000005f);
    windMagnitude = scale;
    windDirection = direction.value;
    const float s = direction.Sin();
    const float c = direction.Cos();
    windX = s*scale;
    windY = -c*scale;
}

void GRAPH::SaveParameters(STREAM* stream) const
{
    (void)stream->Write(&environmentFlags, 4u);
    (void)stream->Write(&gamma, sizeof(gamma));
    unsigned int transient = windDirection;
    (void)stream->Write(&transient, 4u);
    (void)stream->Write(&windMagnitude, 4u);
}

unsigned int*  GraphPackOpaqueRgb(unsigned int* outColor, int red, int green, int blue)
{
    if (red<0)
        red = 0;
    else if (red>255)
        red = 255;
    if (green<0)
        green = 0;
    else if (green>255)
        green = 255;
    if (blue<0)
        blue = 0;
    else if (blue>255)
        blue = 255;
    *outColor = 0xFF000000u|(static_cast<unsigned int>(red)<<16)|(static_cast<unsigned int>(green)<<8)|static_cast<unsigned int>(blue);
    return outColor;
}

void GRAPH::PlayMovie(const STRING& fileName)
{
    const int halfY = TruncateFloatToInt32(screenHeight)/2;
    const int halfX = TruncateFloatToInt32(screenWidth)/2;
    movie.Open(fileName, halfX, halfY);
}

void MOVIE::Open(const STRING& fileName, int x, int y)
{
    static const CLSID filterGraphClass = {0xE436EBB3u, 0x524Fu, 0x11CEu, {0x9Fu, 0x53u, 0x00u, 0x20u, 0xAFu, 0x0Bu, 0xA7u, 0x70u}};
    static const IID graphBuilderInterface = {0x56A868A9u, 0x0AD4u, 0x11CEu, {0xB0u, 0x3Au, 0x00u, 0x20u, 0xAFu, 0x0Bu, 0xA7u, 0x70u}};
    static const IID mediaControlInterface = {0x56A868B1u, 0x0AD4u, 0x11CEu, {0xB0u, 0x3Au, 0x00u, 0x20u, 0xAFu, 0x0Bu, 0xA7u, 0x70u}};
    static const IID mediaEventInterface = {0x56A868B6u, 0x0AD4u, 0x11CEu, {0xB0u, 0x3Au, 0x00u, 0x20u, 0xAFu, 0x0Bu, 0xA7u, 0x70u}};
    static const IID videoWindowInterface = {0x56A868B4u, 0x0AD4u, 0x11CEu, {0xB0u, 0x3Au, 0x00u, 0x20u, 0xAFu, 0x0Bu, 0xA7u, 0x70u}};
    (void)x;
    (void)y;
    Release();
    Graph->FlipToGDISurface();

    int result = static_cast<int>(::CoCreateInstance(filterGraphClass, 0,
        CLSCTX_INPROC_SERVER, graphBuilderInterface, reinterpret_cast<void**>(&graphBuilder)));
    const auto report = [](const char* operation, TYPE_ERROR category, int code)
    {
        const STRING subject(operation);
        if (::Error)
        {
            const STRING owner("MOVIE");
            ::Error->Error(owner, category, subject, static_cast<unsigned long>(code));
        }
    };
    if (result<0)
    {
        report("GraphBuilder", E_CREATE, result);
        return;
    }

    result = static_cast<int>(graphBuilder->QueryInterface(mediaControlInterface,
        reinterpret_cast<void**>(&mediaControl)));
    if (result<0)
    {
        report("MediaControl", E_CREATE, result);
        Release();
        return;
    }
    (void)graphBuilder->QueryInterface(mediaEventInterface, reinterpret_cast<void**>(&mediaEvent));
    wchar_t wideName[1024];
    fileName.ToWideChar(reinterpret_cast<unsigned short*>(wideName), 1024);
    result = static_cast<int>(graphBuilder->RenderFile(wideName, 0));
    if (result<0)
    {
        report("RenderFile", E_INVALID, result);
        Release();
        return;
    }

    (void)graphBuilder->QueryInterface(videoWindowInterface, reinterpret_cast<void**>(&videoWindow));
    (void)videoWindow->put_Owner(static_cast<OAHWND>(Map->DialogHandle()));
    (void)videoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
    GRAPH* const graph = Graph;
    const int top = TruncateFloatToInt32(graph->viewTop);
    const int left = TruncateFloatToInt32(graph->viewLeft);
    const int height = BitsToSigned32(static_cast<unsigned int>(TruncateFloatToInt32(graph->viewBottom))-
        static_cast<unsigned int>(top)+1u);
    const int width = BitsToSigned32(static_cast<unsigned int>(TruncateFloatToInt32(graph->viewRight))-
        static_cast<unsigned int>(left)+1u);
    (void)videoWindow->SetWindowPosition(left, top, width, height);
    (void)mediaControl->Run();
    (void)::SetCapture(reinterpret_cast<HWND>(AddressToUIntPtr(Map->DialogHandle())));
    GRAPH* const cursorGraph = Graph;
    const int cursorY = TruncateFloatToInt32(cursorGraph->screenHeight);
    const int cursorX = TruncateFloatToInt32(cursorGraph->screenWidth);
    (void)::SetCursorPos(cursorX, cursorY);
}

void MOVIE::Release()
{
    (void)::ReleaseCapture();
    if (videoWindow)
        videoWindow->Release();
    videoWindow = 0;
    if (mediaControl)
        mediaControl->Release();
    mediaControl = 0;
    if (mediaEvent)
        mediaEvent->Release();
    mediaEvent = 0;
    if (graphBuilder)
        graphBuilder->Release();
    graphBuilder = 0;
}


template<> LIST<int>::~LIST()
{
    if (m_data)
        CrtFreePointer(m_data);
    m_data = 0;
    m_count = 0;
}


void MOVIE::Pause()
{
    IMediaControl* const object = mediaControl;
    if (object)
        (void)object->Pause();
}

void MOVIE::Resume()
{
    IMediaControl* const object = mediaControl;
    if (object)
        (void)object->Run();
}


void __cdecl InitializeGraphBlackAtStartup()
{
    GRAPH::BLACK.value = 0xFF000000u;
}

void __cdecl InitializeGraphCyanAtStartup()
{
    GRAPH::CYAN.value = 0xFF000000u;
}

void __cdecl InitializeGraphMagentaAtStartup()
{
    GRAPH::MAGENTA.value = 0xFF000000u;
}

void __cdecl InitializeGraphBrownAtStartup()
{
    GRAPH::BROWN.value = 0xFF000000u;
}

void __cdecl InitializeGraphWhiteAtStartup()
{
    GRAPH::WHITE.value = 0xFFFFFFFFu;
}

void __cdecl InitializeGraphGrayAtStartup()
{
    GRAPH::GRAY.value = 0xFF808080u;
}

void __cdecl InitializeGraphBlueAtStartup()
{
    GRAPH::BLUE.value = 0xFF0000FFu;
}

void __cdecl InitializeGraphLightBlueAtStartup()
{
    GRAPH::LIGHTBLUE.value = 0xFF8080FFu;
}

void __cdecl InitializeGraphGreenAtStartup()
{
    GRAPH::GREEN.value = 0xFF00FF00u;
}

void __cdecl InitializeGraphLightGreenAtStartup()
{
    GRAPH::LIGHTGREEN.value = 0xFF80FF80u;
}

void __cdecl InitializeGraphRedAtStartup()
{
    GRAPH::RED.value = 0xFFFF0000u;
}

void __cdecl InitializeGraphLightRedAtStartup()
{
    GRAPH::LIGHTRED.value = 0xFFFF8080u;
}

void __cdecl InitializeGraphYellowAtStartup()
{
    GRAPH::YELLOW.value = 0xFFFFFF00u;
}


void*  GraphPackRGBA(void* self, int alpha, int red, int green, int blue)
{
    unsigned int packedColor = static_cast<unsigned int>(ClampByteInt(alpha));
    packedColor = (packedColor<<8)|static_cast<unsigned int>(ClampByteInt(red));
    packedColor = (packedColor<<8)|static_cast<unsigned int>(ClampByteInt(green));
    packedColor = (packedColor<<8)|static_cast<unsigned int>(ClampByteInt(blue));
    static_cast<COLOR*>(self)->value = packedColor;
    return self;
}

long GRAPH::SetRenderTarget(IDirect3DSurface9* surface, const VECTOR2& leftTop, const VECTOR2& rightBottom, IDirect3DSurface9* depthSurface)
{
    if (surface == savedRenderTarget)
        return 0;
    int result = static_cast<int>(device->SetRenderTarget(0u, surface));
    if (result<0)
        return result;
    if (depthSurface)
    {
        result = static_cast<int>(this->device->SetDepthStencilSurface(depthSurface));
        if (result<0)
            return result;
    }
    SetViewPort(leftTop, rightBottom);
    savedRenderTarget = surface;
    return result;
}



int DD_DRIVER::GetMode(int requestedWidth, int requestedHeight, int bitsPerPixel) const
{
    for (int modeIndex = 0;modeIndex<modeCount;++modeIndex)
    {
        if (width[modeIndex] != static_cast<unsigned int>(requestedWidth) || height[modeIndex] != static_cast<unsigned int>(requestedHeight))
            continue;
        const unsigned int modeFormat = format[modeIndex];
        int formatBitsPerPixel = 0;
        if (modeFormat == D3DFMT_R8G8B8)
            formatBitsPerPixel = 24;
        else if (modeFormat == D3DFMT_A8R8G8B8 || modeFormat == D3DFMT_X8R8G8B8)
            formatBitsPerPixel = 32;
        else if (modeFormat >= D3DFMT_R5G6B5 && modeFormat <= D3DFMT_A4R4G4B4)
            formatBitsPerPixel = 16;
        else if (modeFormat == D3DFMT_P8 || modeFormat == D3DFMT_DXT3 || modeFormat == D3DFMT_DXT5)
            formatBitsPerPixel = 8;
        else if (modeFormat == D3DFMT_DXT1)
            formatBitsPerPixel = 4;
        if (formatBitsPerPixel == bitsPerPixel)
            return modeIndex;
    }
    return -1;
}

unsigned int  GraphHasRegisteredFormatTriple(const GRAPH_INIT* startup, unsigned int width, unsigned int height, unsigned int colorBits)
{
    unsigned int entryIndex = 0u;
    while (startup->allowedColorBits[entryIndex] != 0u && startup->allowedColorBits[entryIndex] != colorBits)
        ++entryIndex;
    if (startup->allowedColorBits[entryIndex] == 0u)
        return 0u;
    entryIndex = 0u;
    while (startup->allowedWidth[entryIndex] != 0u)
    {
        if (startup->allowedWidth[entryIndex] == width && startup->allowedHeight[entryIndex] == height)
            return 1u;
        ++entryIndex;
    }
    return 0u;
}

void __cdecl SetTextArg(STRING* text, unsigned int flags)
{
    STRING& result = *static_cast<STRING*>(text);
    if ((flags&D3DTA_ALPHAREPLICATE) != 0u)
        result += "alp - ";
    if ((flags&D3DTA_COMPLEMENT) != 0u)
        result += "inv - ";
    switch (flags&D3DTA_SELECTMASK)
    {
    case D3DTA_TEXTURE:
        result += "tex";
    break;
    case D3DTA_DIFFUSE:
        result += "dif";
    break;
    case D3DTA_SPECULAR:
        result += "spec";
    break;
    case D3DTA_CURRENT:
        result += "cur";
    break;
    case D3DTA_TFACTOR:
        result += "tfac";
    break;
    default:
        break;
    }
}

void GRAPH::SetScreenScale(const VECTOR& scale)
{
    screenScaleX = scale.x;
    screenScaleY = scale.y;
    screenScaleZ = scale.z;
    const double inverseX = 1.0/static_cast<double>(screenScaleX);
    const float inverseY = 1.0f/screenScaleY;
    const double factorX = 1.0-inverseX;
    const float factorY = 1.0f-inverseY;
    const double halfWidth = static_cast<double>(screenWidth)*0.5;
    const float halfHeight = screenHeight*0.5f;
    viewportOffsetX = static_cast<float>(halfWidth*factorX);
    viewportOffsetY = halfHeight*factorY;
    const bool unitScale = FloatBitsU32(scale.x) == 0x3F800000u &&
                         FloatBitsU32(scale.y) == 0x3F800000u &&
                         FloatBitsU32(scale.z) == 0x3F800000u;
    sceneState = (sceneState&~2u)|(unitScale?0u:2u);
    SetViewPort(*reinterpret_cast<const VECTOR2*>(&viewLeft),
                *reinterpret_cast<const VECTOR2*>(&viewRight));
}

void GRAPH::LoadParameters(STREAM* stream, const VECTOR2& viewport)
{
    (void)viewport;
    GAMMA loadedGamma;
    loadedGamma.modulate = 0;
    loadedGamma.add = 0;
    (void)stream->Read(&environmentFlags, 4u);
    (void)stream->Read(&loadedGamma, sizeof(loadedGamma));
    SetGamma(loadedGamma);
    unsigned int direction = 0u;
    (void)stream->Read(&direction, 4u);
    windDirection = static_cast<unsigned char>(direction);
    (void)stream->Read(&windMagnitude, 4u);
    const ANGLE directionAngle(windDirection);
    const float value = windMagnitude;
    const float x = directionAngle.Sin();
    const float y = directionAngle.Cos();
    windX = x*value;
    windY = -y*value;
}

void GRAPH::Line(const VECTOR2& first, const VECTOR2& second, COLOR color) const
{
    struct Vertex { float x, y, z, rhw; unsigned int color; };
    const Vertex quad[2] = {
        {first.x, first.y, 0.99999988079071044921875f, 1.0f, color.value},
        {second.x, second.y, 0.99999988079071044921875f, 1.0f, color.value}
    };
    (void)device->SetTexture(0u, 0);
    (void)device->SetRenderState(D3DRS_ALPHATESTENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    (void)device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
    DrawPrimitive(D3DPT_LINELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad, 20u, 2);
}

void GRAPH::Line(float x0, float y0, float x1, float y1, COLOR color) const
{
    Line(VECTOR2(x0, y0), VECTOR2(x1, y1), color);
}



void GRAPH::SaveScreenShot(const STRING& fileName, int left, int top, int right, int bottom) const
{
    RECT rectangle = {left, top, right, bottom};
    const long code = D3DXSaveSurfaceToFileA(fileName.value,
        static_cast<D3DXIMAGE_FILEFORMAT>(0), renderTarget, 0, &rectangle);
    ::Error->Log("SaveScreenShot(%s) = %i", fileName.value, code);
}

void GRAPH::PrintfXY(const VECTOR2& position, const char* format, ...)
{
    char text[1024];
    va_list args;
    va_start(args, format);
    vsprintf(text, format, args);
    va_end(args);
    COLOR color;
    color.value = GRAPH::GREEN.value;
    PutsXY(position, text, color);
}

void GRAPH::PrintfXY(float x, float y, const char* format, ...)
{
    char text[1024];
    va_list args;
    va_start(args, format);
    vsprintf(text, format, args);
    va_end(args);
    COLOR color;
    color.value = GRAPH::GREEN.value;
    PutsXY(x, y, text, color);
}



void GRAPH::PutBigPixel(const VECTOR2& point, COLOR color) const
{
    struct Vertex
    {
        float x, y, z, rhw;
        unsigned int diffuse;
    };
    const float right = point.x+2.0f;
    const float bottom = point.y+2.0f;
    Vertex vertices[4] = {
        {
            point.x, point.y, 0.99999988079071044921875f, 1.0f, color.value
        },
        {right, point.y, 0.99999988079071044921875f, 1.0f, color.value},
        {right, bottom, 0.99999988079071044921875f, 1.0f, color.value},
        {point.x, bottom, 0.99999988079071044921875f, 1.0f, color.value}
    };
    (void)device->SetTexture(0u, 0);
    (void)device->SetRenderState(D3DRS_ALPHATESTENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, vertices, sizeof(Vertex), 4);
}

void GRAPH::SetScreenSize(int width, int height)
{
    const float widthValue = static_cast<float>(width);
    if (!(widthValue<screenWidth) && !(widthValue>screenWidth))
    {
        const double heightValue = static_cast<double>(height);
        if (!(heightValue<static_cast<double>(screenHeight)) &&
           !(heightValue>static_cast<double>(screenHeight)))return;
    }
    const unsigned int wasActive = sceneState&1u;
    if (wasActive)
        (void)PostTact(1);
    const float heightValue = static_cast<float>(height);
    screenWidth = widthValue;
    screenHeight = heightValue;
    const double inverseX = 1.0/static_cast<double>(screenScaleX);
    const float inverseY = 1.0f/screenScaleY;
    const double factorX = 1.0-inverseX;
    const float factorY = 1.0f-inverseY;
    const double halfWidth = static_cast<double>(screenWidth)*0.5;
    const float halfHeight = screenHeight*0.5f;
    viewportOffsetX = static_cast<float>(halfWidth*factorX);
    viewportOffsetY = halfHeight*factorY;
    const VECTOR2 viewMin(0.0f, 0.0f);
    SetViewPort(viewMin, *reinterpret_cast<const VECTOR2*>(&screenWidth));
    InvalidateDeviceObjects();
    RestoreDeviceObjects();
    if (wasActive)
        (void)PreTact();
}

void  GRAPH_SetScreenSize(void* self, int width, int height)
{
    static_cast<GRAPH*>(self)->SetScreenSize(width, height);
}

void GRAPH::ShadowBar(const VECTOR2& leftTop, const VECTOR2& rightBottom, int color)
{
    struct Vertex
    {
        float x, y, z, rhw;
        unsigned int diffuse, specular;
    };
    const unsigned int packed = static_cast<unsigned int>(color);
    Vertex vertices[4] = {
        {
            leftTop.x, leftTop.y, 0.99999988079071044921875f, 1.0f, packed, 0xFFFFFFFFu
        },
        {rightBottom.x, leftTop.y, 0.99999988079071044921875f, 1.0f, packed, 0xFFFFFFFFu},
        {rightBottom.x, rightBottom.y, 0.99999988079071044921875f, 1.0f, packed, 0xFFFFFFFFu},
        {leftTop.x, rightBottom.y, 0.99999988079071044921875f, 1.0f, packed, 0xFFFFFFFFu}
    };
    (void)device->SetTexture(0u, 0);
    (void)device->SetRenderState(D3DRS_ALPHATESTENABLE, 0u);
    (void)SetAlphaBlend(D3DBLEND_ZERO, D3DBLEND_INVSRCCOLOR);
    (void)device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, vertices, sizeof(Vertex), 4);
}

void GRAPH::DrawDebugText(const STRING& text, ...)
{
    statusText = text;
    (void)PreTact();

    const VECTOR2 rightBottom = ViewMin()+VECTOR2(800, 30);
    const VECTOR2 leftTop = ViewMin()+VECTOR2(200, 5);
    COLOR black;
    black.value = 0xFF000000u;
    Bar(leftTop, rightBottom, black);

    COLOR green;
    green.value = GRAPH::GREEN.value;
    const VECTOR2 textOrigin = ViewMin()+VECTOR2(200, 5);
    PutsXY(textOrigin, text, green);
    PostTact(1);
}

void GRAPH::PutsXY(float x, float y, const char* text, COLOR color)
{
    ID3DXFont* const backend = static_cast<ID3DXFont*>(fontBackend);
    if (!backend)
        return;
    const bool insideX = !(x>screenWidth);
    unsigned int length = 0;
    while (text[length])
        ++length;
    if (!insideX)
        return;
    if (y>screenHeight)
        return;
    const double right = static_cast<double>(BitsToSigned32(length))*fontWidth+x+10.0f;
    if (!(right >= 0.0f))
        return;
    const double bottom = static_cast<double>(y)+fontHeight+10.0f;
    if (!(bottom >= 0.0f))
        return;
    RECT rect;
    rect.left = TruncateFloatToInt32(x);
    rect.top = TruncateFloatToInt32(y);
    rect.right = BitsToSigned32(static_cast<unsigned int>(rect.left)+1u);
    rect.bottom = BitsToSigned32(static_cast<unsigned int>(rect.top)+1u);
    (void)backend->DrawTextA(text, BitsToSigned32(length), &rect, DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX, color.value);
}

GAMMA::GAMMA(int alpha, int red, int green, int blue)
    : modulate(0u), add(0u)
{
    const int channels[4] = {alpha, red, green, blue};
    const unsigned int shifts[4] = {24u, 16u, 8u, 0u};
    for (int channel = 0;channel<4;++channel)
    {
        int value = channels[channel];
        if (value<-255)
            value = -255;
        if (value>255)
            value = 255;
        if (value<0)
            modulate |= static_cast<unsigned int>(-value)<<shifts[channel];
        else add |= static_cast<unsigned int>(value)<<shifts[channel];
    }
}

void GRAPH::DrawSquall()
{
    GRAPH* const graph = this;
    void* const world = Map;
    if ((Map->Flags()&0x20u) != 0u)
        return;

    float global = squallBaseMagnitude;
    const unsigned int flags = graph->environmentFlags;
    bool updateVector = false;

    if ((flags&0x80u) != 0u)
    {
        if (global == -1.0f)
        {
            global = graph->windMagnitude;
            squallBaseMagnitude = global;
        }

        const unsigned int now = ::CurrentTime;
        const unsigned int old = squallStartTime;
        const unsigned int delta = now-old;
        if (delta <= 2048u)
            graph->windMagnitude = global+static_cast<float>(delta)*global*(0.001953125f);
        else if (delta <= 4096u)
            graph->windMagnitude = global+static_cast<float>(4096u-delta)*global*(0.001953125f);
        else
        {
            graph->environmentFlags = flags&0xFFFFFF7Fu;
            graph->windMagnitude = global;
            squallBaseMagnitude = -1.0f;
        }
        updateVector = true;
    }
    else
    {
        squallStartTime = ::CurrentTime;
        if (global != -1.0f)
        {
            graph->windMagnitude = global;
            updateVector = true;
        }
        squallBaseMagnitude = -1.0f;
    }

    if (updateVector)
    {
        const ANGLE directionAngle(graph->windDirection);
        const float scale = graph->windMagnitude;
        const float x = directionAngle.Sin();
        const float y = -directionAngle.Cos();
        graph->windX = x*scale;
        graph->windY = y*scale;
    }
}

void GRAPH::LightBar(const VECTOR2& leftTop, const VECTOR2& rightBottom, unsigned int color)
{
    struct Vertex
    {
        float x, y, z, rhw;
        unsigned int diffuse, specular;
    };
    Vertex vertices[4] = {
        {
            leftTop.x, leftTop.y, 0.99999988079071044921875f, 1.0f, color, 0xFFFFFFFFu
        },
        {rightBottom.x, leftTop.y, 0.99999988079071044921875f, 1.0f, color, 0xFFFFFFFFu},
        {rightBottom.x, rightBottom.y, 0.99999988079071044921875f, 1.0f, color, 0xFFFFFFFFu},
        {leftTop.x, rightBottom.y, 0.99999988079071044921875f, 1.0f, color, 0xFFFFFFFFu}
    };
    (void)device->SetTexture(0u, 0);
    (void)device->SetRenderState(D3DRS_SPECULARENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ALPHATESTENABLE, 0u);
    (void)device->SetRenderState(D3DRS_SPECULARENABLE, 0u);
    (void)SetAlphaBlend(D3DBLEND_DESTCOLOR, D3DBLEND_ONE);
    (void)device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    (void)device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
    DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, vertices, sizeof(Vertex), 4);
}



void GRAPH::SetGamma(const GAMMA& gamma)
{
    this->gamma.modulate = gamma.modulate;
    this->gamma.add = gamma.add;
}

void GRAPH::OldLoadParameters(STREAM* stream, const VECTOR2& viewport)
{
    (void)viewport;
    (void)stream->Read(&environmentFlags, 4u);
    unsigned int packed = 0u;
    (void)stream->Read(&packed, 4u);

    const GAMMA gamma(static_cast<GAMMA::GAMMA_CREATE>(0), packed);
    SetGamma(gamma);

    unsigned char mode = 0u;
    short value = 0;
    (void)stream->Read(&mode, 1u);
    (void)stream->Read(&value, 2u);
    SetWind(static_cast<int>(value), ANGLE(static_cast<unsigned char>(mode)));
}

void GRAPH::ClearScreen(COLOR color)
{
    IDirect3DDevice9* const device = this->device;
    const int hr = static_cast<int>(device->Clear(0u, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color.value, 0.0f, 0u));
    if (hr >= 0)
        return;

    const STRING subject("Can't clear screen");
    if (::Error)
    {
        const STRING object("GRAPH");
        ::Error->Error(object, E_ERROR, subject, static_cast<unsigned long>(hr));
    }
}

STRING DD_DRIVER::GetModeDesctription(int modeIndex) const
{
    const unsigned int count = static_cast<unsigned int>(modeCount);
    const unsigned int firstFormat = format[0];
    bool allFormatsEqual = true;
    for (int scanModeIndex = static_cast<int>(count)-1;scanModeIndex >= 1;--scanModeIndex)
    {
        if (format[scanModeIndex] != firstFormat)
        {
            allFormatsEqual = false;
            break;
        }
    }

    const unsigned int modeWidth = width[modeIndex];
    const unsigned int modeHeight = height[modeIndex];
    if (allFormatsEqual)
    {
        return Printf("%i x %i", modeWidth, modeHeight);
    }

    const unsigned int modeFormat = format[modeIndex];
    unsigned int bitsPerPixel = 0u;
    switch (modeFormat)
    {
    case D3DFMT_R8G8B8:
        bitsPerPixel = 24u;
    break;
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        bitsPerPixel = 32u;
    break;
    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A4R4G4B4:
        bitsPerPixel = 16u;
    break;
    case D3DFMT_P8:
    case D3DFMT_DXT3:
    case D3DFMT_DXT5:
        bitsPerPixel = 8u;
        break;
    case D3DFMT_DXT1:
        bitsPerPixel = 4u;
        break;
    default:
        break;
    }
    return Printf("%i x %i x %ibpp", modeWidth, modeHeight, bitsPerPixel);
}

void GRAPH::DrawPrimitive(_D3DPRIMITIVETYPE primitiveType, unsigned long vertexShader,
    const void* vertexData, unsigned int vertexStride, int vertexCount) const
{
    int primitiveCount = vertexCount;
    if (primitiveType == D3DPT_TRIANGLESTRIP || primitiveType == D3DPT_TRIANGLEFAN)
        primitiveCount = BitsToSigned32(static_cast<unsigned int>(vertexCount)-2u);
    else if (primitiveType == D3DPT_LINELIST)
        primitiveCount = vertexCount/2;
    else if (primitiveType == D3DPT_TRIANGLELIST)
        primitiveCount = vertexCount/3;

    (void)device->SetFVF(vertexShader);
    const int hr = static_cast<int>(this->device->DrawPrimitiveUP(
        primitiveType, static_cast<unsigned int>(primitiveCount), vertexData, vertexStride));
    if (hr >= 0)
        return;

    const STRING subject("DrawPrimitiveUP");
    if (::Error)
    {
        const STRING object("GRAPH");
        ::Error->Error(object, E_ERROR, subject, static_cast<unsigned long>(hr));
    }
}

MOVIE::~MOVIE()
{
    Release();
}

GRAPH::~GRAPH()
{
    movie.Release();

    if (pseudo3DEffect)
    {
        pseudo3DEffect->Release();
        pseudo3DEffect = 0;
    }
    if (fontBackend)
    {
        static_cast<ID3DXFont*>(fontBackend)->Release();
        fontBackend = 0;
    }

    if (device)
        device->SetTexture(0, 0);

    if (depthSurface)
    {
        depthSurface->Release();
        depthSurface = 0;
    }

    if (lightSampleTexture)
    {
        delete lightSampleTexture;
        lightSampleTexture = 0;
    }
    if (hiBuffer)
    {
        delete hiBuffer;
        hiBuffer = 0;
    }

    if (lightDepthSurface)
    {
        lightDepthSurface->Release();
        lightDepthSurface = 0;
    }

    if (lightRenderTexture)
    {
        delete lightRenderTexture;
        lightRenderTexture = 0;
    }

    if (device)
    {
        const unsigned int refs = device->Release();
        device = 0;
        Error->Log("d3dDevice release %i", refs);
    }

    if (renderTarget)
    {
        const unsigned int refs = renderTarget->Release();
        renderTarget = 0;
        Error->LogTmp(STRING("backBuffer release %i"), refs);
    }

    if (direct3D)
    {
        const unsigned int refs = direct3D->Release();
        direct3D = 0;
        Error->Log("d3d release %i", refs);
    }
}

void GRAPH::SetFont(const STRING& requestedName, int requestedWidth, int requestedHeight)
{
    fontName = requestedName;
    if (fontBackend)
    {
        (void)static_cast<ID3DXFont*>(fontBackend)->Release();
        fontBackend = 0;
    }

    LOGFONTA descriptor;
    HDC const dc = ::GetDC(0);
    descriptor.lfHeight = BitsToSigned32(0u-static_cast<unsigned int>(
        ::MulDiv(requestedHeight, ::GetDeviceCaps(dc, LOGPIXELSY), 72)));
    descriptor.lfWidth = BitsToSigned32(0u-static_cast<unsigned int>(
        ::MulDiv(requestedWidth, ::GetDeviceCaps(dc, LOGPIXELSX), 72)));
    descriptor.lfEscapement = 0;
    descriptor.lfOrientation = 0;
    descriptor.lfWeight = FW_NORMAL;
    descriptor.lfItalic = 0;
    descriptor.lfUnderline = 0;
    descriptor.lfStrikeOut = 0;
    descriptor.lfCharSet = DEFAULT_CHARSET;
    descriptor.lfOutPrecision = OUT_DEFAULT_PRECIS;
    descriptor.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    descriptor.lfQuality = ANTIALIASED_QUALITY;
    descriptor.lfPitchAndFamily = FIXED_PITCH;
    std::strcpy(descriptor.lfFaceName, requestedName.value);
    (void)::ReleaseDC(0, dc);

    const int result = static_cast<int>(D3DXCreateFontIndirect(Graph->device, &descriptor, &fontBackend));
    if (result<0)
    {
        STRING operation = "SetFont( font "+requestedName;
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_CREATE, operation, static_cast<unsigned long>(result));
        }
    }
    fontWidth = static_cast<float>(requestedWidth);
    fontHeight = static_cast<float>(requestedHeight);
}

void GRAPH::SetDefaultState() const
{

    (void)device->SetTextureStageState(0u, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    (void)device->SetTextureStageState(0u, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    (void)device->SetTextureStageState(0u, D3DTSS_COLOROP, D3DTOP_MODULATE);
    (void)device->SetTextureStageState(0u, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    (void)device->SetTextureStageState(0u, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    (void)device->SetTextureStageState(0u, D3DTSS_ALPHAOP, D3DTOP_MODULATE);


    (void)device->SetSamplerState(0u, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    (void)device->SetSamplerState(0u, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    (void)device->SetSamplerState(0u, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    (void)device->SetSamplerState(0u, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    (void)device->SetSamplerState(0u, D3DSAMP_MIPFILTER, D3DTEXF_NONE);


    (void)device->SetTextureStageState(1u, D3DTSS_COLOROP, D3DTOP_DISABLE);
    (void)device->SetTextureStageState(1u, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    (void)device->SetSamplerState(1u, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    (void)device->SetSamplerState(1u, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    (void)device->SetSamplerState(1u, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    (void)device->SetSamplerState(1u, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    (void)device->SetSamplerState(1u, D3DSAMP_MIPFILTER, D3DTEXF_NONE);


    (void)device->SetRenderState(D3DRS_SPECULARENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0u);
    (void)device->SetRenderState(D3DRS_FOGENABLE, 0u);
    (void)device->SetRenderState(D3DRS_DITHERENABLE, 0u);
    (void)device->SetRenderState(D3DRS_LOCALVIEWER, 0u);
    (void)device->SetRenderState(D3DRS_LIGHTING, 0u);
    (void)device->SetRenderState(D3DRS_ALPHATESTENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    (void)device->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
    (void)device->SetRenderState(D3DRS_SCISSORTESTENABLE, 0u);
    (void)device->SetRenderState(D3DRS_STENCILENABLE, 0u);
}

void GRAPH::SetTransformMatrix(_D3DTRANSFORMSTATETYPE mode, const _D3DMATRIX* matrix, const GAMMA* gamma) const
{
    if (vertexShaderVersion >= 0x100u)
    {
        if (mode == D3DTS_WORLD)
        {
            ID3DXEffect* effect = pseudo3DEffect;
            (void)effect->SetMatrix(worldViewMatrixParameter, reinterpret_cast<const D3DXMATRIX*>(matrix));

            const float byteToUnitScale = (0.00392156886f);
            float gammaChannels[4];

            unsigned int packedChannels = ~gamma->modulate;
            gammaChannels[0] = static_cast<float>((packedChannels>>16)&0xFFu)*byteToUnitScale;
            gammaChannels[1] = static_cast<float>((packedChannels>>8)&0xFFu)*byteToUnitScale;
            const D3DXHANDLE modulateHandle = diffuseParameter;
            effect = pseudo3DEffect;
            gammaChannels[2] = static_cast<float>(packedChannels&0xFFu)*byteToUnitScale;
            gammaChannels[3] = static_cast<float>(packedChannels>>24)*byteToUnitScale;
            (void)effect->SetVector(modulateHandle, reinterpret_cast<const D3DXVECTOR4*>(gammaChannels));

            packedChannels = gamma->add&0x00FFFFFFu;
            gammaChannels[0] = static_cast<float>((packedChannels>>16)&0xFFu)*byteToUnitScale;
            gammaChannels[1] = static_cast<float>((packedChannels>>8)&0xFFu)*byteToUnitScale;
            const D3DXHANDLE addHandle = specularParameter;
            effect = pseudo3DEffect;
            gammaChannels[2] = static_cast<float>(packedChannels&0xFFu)*byteToUnitScale;
            gammaChannels[3] = static_cast<float>(packedChannels>>24)*byteToUnitScale;
            (void)effect->SetVector(addHandle, reinterpret_cast<const D3DXVECTOR4*>(gammaChannels));
            return;
        }
        if (mode == D3DTS_VIEW)
        {
            ID3DXEffect* const effect = pseudo3DEffect;
            (void)effect->SetMatrix(viewMatrixParameter, reinterpret_cast<const D3DXMATRIX*>(matrix));
            return;
        }
        if (mode == D3DTS_PROJECTION)
        {
            ID3DXEffect* const effect = pseudo3DEffect;
            (void)effect->SetMatrix(projectionMatrixParameter, reinterpret_cast<const D3DXMATRIX*>(matrix));
            return;
        }
        return;
    }

    const int hr = static_cast<int>(device->SetTransform(
        mode, matrix));
    if (hr >= 0)
        return;

    const STRING subject("Transform matrix");
    if (::Error)
    {
        const STRING object("GRAPH");
        ::Error->Error(object, E_SET, subject, static_cast<unsigned long>(hr));
    }
}



static const char* GraphTextureOpName(unsigned int value)
{
    switch (value)
    {
    case D3DTOP_DISABLE:
        return "dis";
    case D3DTOP_SELECTARG1:
        return "sel1";
    case D3DTOP_SELECTARG2:
        return "sel2";
    case D3DTOP_MODULATE:
        return "mod";
    case D3DTOP_BLENDTEXTUREALPHA:
        return "tex_alpha";
    default:
        return "unknown";
    }
}

void DestroyTextureStageDescription()
{
    textureStageDescription.Address()->~STRING();
}

const STRING& GetTextureState()
{
    unsigned char& initialized = textureStageDescriptionInitialized;
    STRING& text = textureStageDescription.Get();
    if ((initialized&1u) == 0u)
    {
        initialized |= 1u;
        ::new (static_cast<void*>(&text)) STRING("Op = ");
        (void)RegisterCrtAtExit(&DestroyTextureStageDescription);
    }

    DWORD value;
    (void)Graph->device->GetTextureStageState(0u, D3DTSS_COLOROP, &value);
    text += GraphTextureOpName(value);
    text += " Arg1 = ";
    (void)Graph->device->GetTextureStageState(0u, D3DTSS_COLORARG1, &value);
    SetTextArg(&text, value);
    text += " Arg2 = ";
    (void)Graph->device->GetTextureStageState(0u, D3DTSS_COLORARG2, &value);
    SetTextArg(&text, value);
    text += " AOp = ";
    (void)Graph->device->GetTextureStageState(0u, D3DTSS_ALPHAOP, &value);
    text += GraphTextureOpName(value);
    text += " Arg1 = ";
    (void)Graph->device->GetTextureStageState(0u, D3DTSS_ALPHAARG1, &value);
    SetTextArg(&text, value);
    text += " Arg2 = ";
    (void)Graph->device->GetTextureStageState(0u, D3DTSS_ALPHAARG2, &value);
    SetTextArg(&text, value);
    return text;
}

static unsigned int GraphViewportFloatToDword(float value)
{
    return static_cast<unsigned int>(TruncateFloatToInt32(value));
}

void GRAPH::SetViewPort(const VECTOR2& leftTop, const VECTOR2& rightBottom)
{
    const float firstX = leftTop.x;
    IDirect3DDevice9* const viewportDevice = device;
    viewLeft = firstX;
    viewTop = leftTop.y;
    viewRight = rightBottom.x;
    viewBottom = rightBottom.y;
    if (!viewportDevice)
        return;

    D3DVIEWPORT9 viewport;
    viewport.X = static_cast<unsigned int>(TruncateFloatToInt32(viewLeft));
    viewport.Y = static_cast<unsigned int>(TruncateFloatToInt32(viewTop));
    viewport.Width = static_cast<unsigned int>(TruncateDoubleToInt32(static_cast<double>(viewRight)-viewLeft));
    viewport.Height = static_cast<unsigned int>(TruncateDoubleToInt32(static_cast<double>(viewBottom)-viewTop));
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;

    const int hr = static_cast<int>(viewportDevice->SetViewport(&viewport));
    if (hr != 0)
    {
        const STRING subject("viewport");
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_SET, subject, static_cast<unsigned long>(hr));
        }
    }

    const double twiceScaleX = static_cast<double>(screenScaleX)+static_cast<double>(screenScaleX);
    const float twiceScaleY = screenScaleY+screenScaleY;
    float projection[16];
    projection[1] = projection[2] = projection[3] = projection[4] = 0.0f;
    projection[6] = projection[7] = projection[8] = projection[9] = 0.0f;
    projection[11] = projection[12] = projection[13] = projection[14] = 0.0f;
    projection[15] = 1.0f;
    projection[0] = static_cast<float>(twiceScaleX/static_cast<double>(viewport.Width));
    projection[5] = static_cast<float>(-(static_cast<double>(twiceScaleY)/static_cast<double>(viewport.Height)));
    projection[10] = (1.0f/(viewport.MaxZ-viewport.MinZ))*(0.000122070312f);
    SetTransformMatrix(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX*>(projection), 0);
}

GRAPH::GRAPH(const GRAPH_INIT* startup)
{
    for (unsigned int i = 0;i<8u;++i)
    {
        adapters[i].description[0] = 0;
        adapters[i].modeCount = 0;
        adapters[i].flags &= ~0x0Fu;
    }

    effectGamma.modulate = 0;
    effectGamma.add = 0;
    gamma.modulate = 0;
    gamma.add = 0;
    const unsigned int configFlags = startup->flags;
    capabilityFlags &= ~0x00000180u;
    capabilityFlags |= (configFlags&0x01u)<<8;
    capabilityFlags |= (configFlags&0x02u)<<6;
    capabilityFlags &= 0xFFFF3FEEu;
    capabilityFlags |= configFlags&0x10u;
    deviceState |= 3u;
    sceneState &= ~1u;

    direct3D = 0;
    device = 0;
    fontBackend = 0;
    lightSampleTexture = 0;
    hiBuffer = 0;
    depthSurface = 0;
    renderTarget = 0;
    lightDepthSurface = 0;
    lightRenderTexture = 0;
    pseudo3DEffect = 0;
    environmentFlags = 0;

    Effect(0, 0, 0, -1);
    viewLeft = 0.0f;
    viewTop = 0.0f;
    viewRight = 0.0f;
    viewBottom = 0.0f;

    direct3D = ::Direct3DCreate9(D3D_SDK_VERSION);
    if (!direct3D)
    {
        Error->Window(STRING("Can't create Direct3D9"));
        return;
    }

    adapterCount = 0;
    for (int adapter = 0;adapter<static_cast<int>(direct3D->GetAdapterCount());
        adapter = adapterCount)
    {
        adapters[adapter].FillParameters(adapter, direct3D, startup);
        ++adapterCount;
    }

    selectedAdapter = 0;
    screenWidth = static_cast<float>(startup->width);
    screenHeight = static_cast<float>(startup->height);
    graphDetail = 8u;
    screenScaleX = 1.0f;
    screenScaleY = 1.0f;
    screenScaleZ = 1.0f;
    viewportOffsetX = 0;
    viewportOffsetY = 0;
    sceneState &= ~2u;

    capabilityFlags = (capabilityFlags&~2u)|((startup->colorBits == 32u)?2u:0u);
    capabilityFlags = (capabilityFlags&~0x40u)|(startup->fullScreen?0x40u:0u);
    if (adapterCount <= 0)
        selectedAdapter = 0;
    runtimeE18 = 0;
}

static void GraphReportFlipFailure(const char* text, TYPE_ERROR type)
{
    if (!Error)
        return;
    STRING owner("GRAPH");
    STRING operation(text);
    Error->Error(owner, type, operation, 0u);
}

void GRAPH::FlipToGDISurface()
{
    if ((capabilityFlags&0x40u) == 0u)
        return;

    PostTact(1);
    const HWND hwnd = reinterpret_cast<HWND>(window);
    const HDC dc = ::GetDC(hwnd);
    if (!dc)
        GraphReportFlipFailure("DC in FlipToGDI", E_GET);

    const COLORREF pixel = ::SetPixel(dc, 0, 0, 0x00FFFFFFu);
    if (pixel == CLR_INVALID)
        GraphReportFlipFailure("pixel in FlipToGDI", E_SET);

    ::Sleep(150u);
    COLORREF current = ::GetPixel(dc, 0, 0);
    int attempts = 0;
    while (current == 0x00FFFFFFu && attempts<8)
    {
        (void)PreTact();
        PostTact(1);
        ::Sleep(150u);
        current = ::GetPixel(dc, 0, 0);
        ++attempts;
    }
    ::ReleaseDC(hwnd, dc);
}

void GRAPH::PostTact(int presentFrame)
{
    if ((this->sceneState&1u) == 0u)
        return;

    const int endHr = static_cast<int>(this->device->EndScene());
    if (endHr != 0)
    {
        const STRING subject("3dEndScene for PostTact");
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_ERROR, subject, static_cast<unsigned long>(endHr));
        }
    }

    if (presentFrame && this->effectStart[6] == 0u && this->effectStart[7] == 0u)
    {
        RECT source = {
            static_cast<LONG>(GraphViewportFloatToDword(this->viewLeft)),
            static_cast<LONG>(GraphViewportFloatToDword(this->viewTop)),
            static_cast<LONG>(GraphViewportFloatToDword(this->viewRight)),
            static_cast<LONG>(GraphViewportFloatToDword(this->viewBottom))
        };
        RECT destination = source;

        if ((this->capabilityFlags&0x40u) == 0u)
        {
            RECT clientRect;
            RECT windowRect;
            ::GetClientRect(reinterpret_cast<HWND>(window), &clientRect);
            ::ClientToScreen(reinterpret_cast<HWND>(window), reinterpret_cast<POINT*>(&clientRect));
            ::GetWindowRect(reinterpret_cast<HWND>(window), &windowRect);
            const unsigned int dx = static_cast<unsigned int>(windowRect.left)-static_cast<unsigned int>(clientRect.left);
            const unsigned int dy = static_cast<unsigned int>(windowRect.top)-static_cast<unsigned int>(clientRect.top);
            destination.left = BitsToSigned32(static_cast<unsigned int>(destination.left)+dx);
            destination.right = BitsToSigned32(static_cast<unsigned int>(destination.right)+dx);
            destination.top = BitsToSigned32(static_cast<unsigned int>(destination.top)+dy);
            destination.bottom = BitsToSigned32(static_cast<unsigned int>(destination.bottom)+dy);
        }

        const int presentHr = static_cast<int>(this->device->Present(&source, &destination, 0, 0));
        if (presentHr != 0)
        {
            const STRING subject("Present");
            if (::Error)
            {
                const STRING object("GRAPH");
                ::Error->Error(object, E_INVALID, subject, static_cast<unsigned long>(presentHr));
            }
        }
    }

    this->sceneState &= ~1u;
    return;
}

namespace {
struct GraphWeatherVertex
{
    float x,y,z,rhw;
    unsigned int color;
};
struct GraphRainParticle { GraphWeatherVertex vertex[2]; };
struct GraphSnowParticle { GraphWeatherVertex vertex[6]; };

static GraphRainParticle rainParticles[250];
static GraphSnowParticle snowParticles[1000];

static void GraphRotateSnowParticle(GraphSnowParticle& particle)
{
    const float half = (0.5f);
    const float middleX = (particle.vertex[1].x+particle.vertex[0].x)*half;
    const float middleY = (particle.vertex[1].y+particle.vertex[0].y)*half;
    for (int vertexIndex = 0;vertexIndex<6;++vertexIndex)
    {
        const float oldX = particle.vertex[vertexIndex].x;
        const float oldY = particle.vertex[vertexIndex].y;
        particle.vertex[vertexIndex].x = middleX+oldY-middleY;
        particle.vertex[vertexIndex].y = oldX+middleY-middleX;
    }
}
}

void GRAPH::DrawRain()
{
    unsigned char guard = rainInitialized;
    if ((guard&1u) == 0u)
    {
        rainInitialized = static_cast<unsigned char>(guard|1u);
        (void)RegisterCrtAtExit(&GraphStaticCleanupNoOpSecondary);
        guard = rainInitialized;
    }
    if ((guard&2u) == 0u)
    {
        rainInitialized = static_cast<unsigned char>(guard|2u);
        (void)RegisterCrtAtExit(&GraphStaticCleanupNoOp);
    }

    unsigned int renderFlags = environmentFlags;
    const unsigned int mode = renderFlags&0x0C00u;
    if (mode == 0u)
    {
        rainParticleCount = 0u;
        return;
    }

    if ((Map->Flags()&0x20u) != 0u)
        return;

    const unsigned char direction = windDirection;
    const float speed = windMagnitude;
    if (rainWindDirection != direction || rainWindMagnitude != speed)
    {
        const float oldWind = rainHorizontalSpeed;
        const float newWind = ANGLE(direction).Sin()
            *speed*(1000.0f);
        rainHorizontalSpeed = newWind;

        int count = rainParticleCount;
        if (count>0)
        {
            const float deltaWind = newWind-oldWind;
            GraphRainParticle* const particles = rainParticles;
            for (int i = 0;i<count;++i)
            {
                GraphWeatherVertex& first = particles[i].vertex[0];
                GraphWeatherVertex& second = particles[i].vertex[1];
                second.x += (second.y-first.y)*deltaWind*(0.00499999989f);
            }
        }
        rainWindDirection = direction;
        rainWindMagnitude = speed;
    }

    int count = rainParticleCount;
    if (mode == 0x0C00u)
        count = 250;
    else if (mode == 0x0800u)
    {
        if (count<250)
            ++count;
        else
        {
            count = 250;
            SetEnvironment(0x0C00u);
        }
    }
    else if (mode == 0x0400u)
    {
        if (count>0)
            --count;
        else
        {
            renderFlags &= ~0x0C00u;
            environmentFlags = renderFlags;
            count = 0;
        }
    }
    rainParticleCount = count;

    GraphRainParticle* const particles = rainParticles;
    const int elapsed = static_cast<int>(::CurrentTime-::PrevCurrentTime);
    int respawned = 0;
    for (int i = 0;i<count;++i)
    {
        GraphWeatherVertex& first = particles[i].vertex[0];
        GraphWeatherVertex& second = particles[i].vertex[1];
        const bool active = first.color != 0u && first.y<viewBottom &&
            first.z >= (0.015625f);
        if (active)
        {
            const float travel = (second.z-first.z)*static_cast<float>(elapsed)*(200.0f);
            float xTravel = rainHorizontalSpeed*travel*(0.00499999989f);
            xTravel -= Map->ShiftXY().x-rainCameraX;

            const float minimum = (0.0f);
            const float width = screenWidth;
            if (xTravel+first.x<minimum)
                xTravel += width;
            if (xTravel+first.x>width)
                xTravel -= width;
            const float zTravel = travel*(0.000122070312f);
            first.x += xTravel;
            second.x += xTravel;
            first.y += travel;
            second.y += travel;
            first.z -= zTravel;
            second.z -= zTravel;
        }
        else
        {
            ++respawned;
            const float length = static_cast<float>(Random2(50))+(15.0f);
            const float windTail = rainHorizontalSpeed*length*(-0.00499999989f);
            const float x = Random2(viewRight+(100.0f))-
                (50.0f);
            const float yLimit = (respawned<50)?40.0f:viewBottom;
            const float y = Random2(yLimit);
            const float z = (length+(10.0f))*(0.00122070312f)+
                (0.015625f);
            first.x = x;
            first.y = y;
            first.z = z;
            first.rhw = 1.0f;
            first.color = 0x70E0E0FFu;
            second.x = x+windTail;
            second.y = y-length;
            second.z = z+length*(0.000122070312f);
            second.rhw = 1.0f;
            second.color = 0x308080FFu;
        }
    }

    rainCameraX = Map->ShiftXY().x;
    rainCameraY = Map->ShiftXY().y;

    IDirect3DDevice9* const device = this->device;
    (void)device->SetRenderState(D3DRS_ALPHATESTENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
    (void)device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
    (void)device->SetTexture(0u, 0);
    (void)SetAlphaBlend(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
    DrawPrimitive(D3DPT_LINELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, particles, sizeof(GraphWeatherVertex),
        rainParticleCount*2);
}

int GRAPH::PreTact()
{
    if ((sceneState&1u) != 0u)
        return 0;
    const unsigned int cooperative = static_cast<unsigned int>(device->TestCooperativeLevel());
    if (cooperative == D3DERR_DEVICELOST)
    {
        ReLoadPalettes();
        const STRING subject("device lost");
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_ERROR, subject, 0u);
        }
        return 1;
    }
    if (cooperative == D3DERR_DEVICENOTRESET)
    {
        {
            const STRING subject("device not reset");
            if (::Error)
            {
                const STRING object("GRAPH");
                ::Error->Error(object, E_INVALID, subject, 0u);
            }
        }
        InvalidateDeviceObjects();
        RestoreDeviceObjects();
    }
    else if (cooperative != D3D_OK)
        return 3;
    const int beginHr = static_cast<int>(this->device->BeginScene());
    if (beginHr != D3D_OK)
    {
        const STRING subject("3dBeginScene for PreTact");
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_SET, subject, static_cast<unsigned long>(beginHr));
        }
        return 4;
    }
    sceneState |= 1u;
    return 0;
}

int GRAPH::DrawLoadBar(STRING text, VID* vid, ANGLE direction, int mode)
{
    const unsigned int previousTime = ::CurrentTime;
    int result = 0;

    const unsigned int now = timeGetTime();
    static_cast<void>(::CurrentTime = now);

    if (mode != 0)
    {
        loadBarMode = static_cast<unsigned int>(mode);
        Effect(mode, 0, 0, mode == 10?700:300);
    }

    const unsigned int frameInterval = vid ? static_cast<unsigned int>(vid->frameSpeedDefault) : 32u;
    if (now-loadBarLastDrawTime>frameInterval)
    {
        if (vid || !text.IsEmpty())
        {
            (void)PreTact();
            COLOR clearColor;
            clearColor.value = 0xFF000000u;
            ClearScreen(clearColor);

            if (vid)
            {
                MAP* const map = Map;
                const VECTOR2 viewport(screenWidth, screenHeight);
                const VECTOR2 mapOrigin = map->ShiftXY();
                const VECTOR2 viewMaximum = viewport+mapOrigin;
                VID::viewMin.x = mapOrigin.x;
                VID::viewMin.y = mapOrigin.y;
                VID::viewMax.x = viewMaximum.x;
                VID::viewMax.y = viewMaximum.y;

                const VECTOR2 halfViewport = viewport/2.0f;
                const VECTOR2 worldXY = map->FromScreen(halfViewport);
                const VECTOR worldPosition(worldXY, 0.0f);
                SPRITE source(vid, worldPosition, direction, 0);

                unsigned int frame = loadBarFrame;
                if (static_cast<int>(frame) >= vid->noCadr)
                    frame = 0u;
                loadBarFrame = frame+1u;
                source.m_currentFrame = static_cast<int>(frame);
                vid->Draw(&source);
            }

            if (!text.IsEmpty())
            {
                MAP* const map = Map;
                VID* const fallbackFont = ::EmptyVid;
                VID* selectedFont = fallbackFont;
                if (map && map->VidCount()>2)
                {
                    VID* const candidate = map->StoredVid(2);
                    if (candidate)
                        selectedFont = candidate;
                }

                if (selectedFont != fallbackFont && selectedFont->spriteClass == 19u)
                {
                    LIST<int> lineOffsets;
                    lineOffsets.m_count = 0;
                    lineOffsets.m_capacity = 0;
                    lineOffsets.m_data = 0;
                    int maxWidth = 0;
                    int textHeight = 0;
                    int lineCount = 0;

                    reinterpret_cast<VID_FONT*>(selectedFont)->CalcTextSize(
                        text, &maxWidth, &textHeight, &lineCount,
                        &lineOffsets, 9u);

                    COLOR color;
                    color.value = selectedFont->gammaRaw.add;
                    VID* const drawVid = map ? map->Vid(2) : 0;
                    if (drawVid)
                    {
                        const int x = TruncateFloatToInt32(screenWidth)/2+10;
                        const int y = TruncateFloatToInt32(screenHeight)/2+100;
                        reinterpret_cast<VID_FONT*>(drawVid)->DrawTextA(
                            x, y, text, text.Length(), 0, 5, color,
                            lineOffsets);
                    }

                }
            }

            DrawEffect(1);

            COLOR statusColor;
            statusColor.value = GRAPH::GREEN.value;
            PutsXY(200.0f, viewTop+(5.0f), statusText, statusColor);
            PostTact(1);
        }

        loadBarLastDrawTime = ::CurrentTime;
        result = 1;
    }

    const unsigned int activeEffect = loadBarMode;
    if (activeEffect != 0u)
    {
        const int remaining = GetEffectState(static_cast<int>(activeEffect));
        if (remaining>98 || remaining<0)
        {
            result = 2;
            loadBarMode = 0u;
        }
    }

    static_cast<void>(::CurrentTime = previousTime);
    return result;
}

void GRAPH::Effect(int modeValue, int widthValue, int heightValue, int flagsValue)
{
    void* const self = this;
    GRAPH* const graph = this;
    const unsigned int mode = static_cast<unsigned int>(modeValue);
    const unsigned int width = static_cast<unsigned int>(widthValue);
    const unsigned int height = static_cast<unsigned int>(heightValue);
    const unsigned int flags = static_cast<unsigned int>(flagsValue);
    if (mode >= 16u)
        return;

    if (mode == 3u || mode == 9u || mode == 10u)
    {
        graph->effectStart[10] = 0u;
        graph->effectStart[9] = 0u;
        graph->effectStart[3] = 0u;
    }

    unsigned int duration = flags;
    if (duration == 0xFFFFFFFFu)
    {
        if (mode == 1u || mode == 3u)
            duration = 2304u;
        else if (mode == 2u)
            duration = 512u;
        else if (mode == 9u)
            duration = 1280u;
        else duration = 1024u;
    }

    graph->effectStart[mode] = duration?::CurrentTime:0u;
    graph->effectDuration[mode] = duration;
    graph->effectWidth[mode] = width;
    graph->effectHeight[mode] = height;

    if (mode == 0u)
    {
        for (unsigned int i = 0u;i<16u;++i)
            graph->effectStart[i] = 0u;
        return;
    }

    if (mode == 5u)
    {
        IDirect3DSurface9* const depth = graph->depthSurface;
        if (depth)
        {
            graph->effectStart[10] = 0u;
            graph->effectStart[9] = 0u;
            graph->effectStart[3] = 0u;
            IDirect3DDevice9* const device = graph->device;
            IDirect3DSurface9* const renderTarget = graph->renderTarget;
            const int hr = static_cast<int>(device->GetRenderTargetData(renderTarget, depth));
            if (hr<0)
            {
                const STRING operation("for EFF_ALPHAAPPEAR");
                if (::Error)
                {
                    const STRING owner("GRAPH");
                    ::Error->Error(owner, E_COPY, operation, static_cast<unsigned long>(hr));
                }
            }
        }
        return;
    }

    if (mode == 2u)
    {
        void* const world = Map;
        if (world)
        {
            const float scale = (0.5f);
            graph->effectCenterX = graph->screenWidth*scale+Map->ShiftXY().x;
            graph->effectCenterY = graph->screenHeight*scale+Map->ShiftXY().y;
        }
        return;
    }

    if (mode == 11u)
    {
        effectGamma.modulate = gamma.modulate;
        effectGamma.add = gamma.add;
        return;
    }

    if (mode == 12u)
    {
        graph->effectScreenScale[0] = graph->screenScaleX;
        graph->effectScreenScale[1] = graph->screenScaleY;
        graph->effectScreenScale[2] = graph->screenScaleZ;
    }
    return;
}

void GRAPH::InvalidateDeviceObjects()
{
    if (Map)
        Map->InvalidateDeviceObjects();
    SPRITE_COLLECTOR* const collector = ::Sprites;
    if (collector)
        collector->InvalidateDeviceObjects();

    if (lightRenderTexture)
    {
        delete lightRenderTexture;
        lightRenderTexture = 0;
    }
    if (lightSampleTexture)
    {
        delete lightSampleTexture;
        lightSampleTexture = 0;
    }

    if (this->renderTarget)
    {
        this->renderTarget->Release();
        this->renderTarget = 0;
    }
    if (this->depthSurface)
    {
        this->depthSurface->Release();
        this->depthSurface = 0;
    }
    if (this->lightDepthSurface)
    {
        this->lightDepthSurface->Release();
        this->lightDepthSurface = 0;
    }

    (void)this->device->SetTexture(0u, 0);
    (void)this->device->SetTexture(1u, 0);
    (void)this->device->SetTexture(2u, 0);

    if (this->fontBackend)
        (void)this->fontBackend->OnLostDevice();
    (void)this->pseudo3DEffect->OnLostDevice();

    this->present.backBufferWidth = GraphViewportFloatToDword(this->screenWidth);
    this->present.backBufferHeight = GraphViewportFloatToDword(this->screenHeight);

    D3DPRESENT_PARAMETERS present;
    std::memcpy(&present, &this->present, sizeof(present));

    if ((this->capabilityFlags&0x40u) != 0u)
    {
        if (this->graphDetail >= 8u)
        {
            this->desktopFormat = D3DFMT_X8R8G8B8;
            this->backBufferFormat = this->present.backBufferFormat;
            this->depthFormat = this->present.autoDepthStencilFormat;
        }
        else if (this->graphDetail >= 5u)
        {
            this->depthFormat = this->present.autoDepthStencilFormat;
            this->desktopFormat = D3DFMT_X8R8G8B8;
            this->backBufferFormat = this->present.backBufferFormat;
        }
        else
        {
            this->depthFormat = D3DFMT_D16;
            this->backBufferFormat = D3DFMT_R5G6B5;
            this->desktopFormat = D3DFMT_R5G6B5;
            present.BackBufferFormat = D3DFMT_R5G6B5;
            present.AutoDepthStencilFormat = D3DFMT_D16;
        }

        if ((this->deviceState&1u) != 0u)
        {
            const HRESULT check = this->direct3D->CheckDepthStencilMatch(
                this->selectedAdapter, D3DDEVTYPE_HAL,
                static_cast<D3DFORMAT>(this->desktopFormat),
                static_cast<D3DFORMAT>(this->backBufferFormat), D3DFMT_D24S8);
            if (check != D3D_OK)
                this->deviceState &= ~1u;
        }

        if ((this->deviceState&1u) == 0u)
        {
            const HRESULT check = this->direct3D->CheckDepthStencilMatch(
                this->selectedAdapter, D3DDEVTYPE_HAL,
                static_cast<D3DFORMAT>(this->desktopFormat),
                static_cast<D3DFORMAT>(this->backBufferFormat), D3DFMT_D16);
            if (check == D3D_OK)
            {
                this->depthFormat = D3DFMT_D16;
                present.AutoDepthStencilFormat = D3DFMT_D16;
            }
        }
    }

    const int resetHr = static_cast<int>(this->device->Reset(&present));

    {
        const STRING depthName = GetPixelFormat(static_cast<D3DFORMAT>(static_cast<unsigned int>(present.AutoDepthStencilFormat)));
        const STRING desktopName = GetPixelFormat(static_cast<D3DFORMAT>(desktopFormat));
        const STRING colorName = GetPixelFormat(static_cast<D3DFORMAT>(backBufferFormat));
        ::Error->Log("Selected display mode %.0fx%.0f %s desktop %s zbuffer %s", static_cast<double>(screenWidth), static_cast<double>(screenHeight), colorName.value, desktopName.value, depthName.value);
    }
    if (resetHr<0)
    {
        const STRING subject("can't reset device");
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_ERROR, subject, static_cast<unsigned long>(resetHr));
        }
    }
}

void DD_DRIVER::FillParameters(int adapterIndex, IDirect3D9* d3d, const GRAPH_INIT* startup)
{

    D3DADAPTER_IDENTIFIER9 identifier;
    D3DDISPLAYMODE mode;
    D3DCAPS9 caps;
    d3d->GetAdapterIdentifier(adapterIndex, 0u, &identifier);
    d3d->GetAdapterDisplayMode(adapterIndex, &mode);

    strncpy(description, identifier.Description, sizeof(description));
    Error->Log(" %s", identifier.Description);

    desktopFormat = static_cast<unsigned int>(mode.Format);
    modeCount = 0;

    int modeIndex = static_cast<int>(d3d->GetAdapterModeCount(adapterIndex, D3DFMT_X8R8G8B8))-1;
    for (;modeIndex >= 0;--modeIndex)
    {
        d3d->EnumAdapterModes(adapterIndex, D3DFMT_X8R8G8B8, static_cast<unsigned int>(modeIndex), &mode);

        if (mode.Format == D3DFMT_X8R8G8B8 || mode.Format == D3DFMT_A8R8G8B8)
        {
            if (!GraphHasRegisteredFormatTriple(startup, mode.Width, mode.Height, 32u))
                continue;
        }

        int bitsPerPixel = 0;
        if (mode.Format == D3DFMT_R8G8B8)
            bitsPerPixel = 24;
        else if (mode.Format == D3DFMT_A8R8G8B8 || mode.Format == D3DFMT_X8R8G8B8)
            bitsPerPixel = 32;
        else if (mode.Format >= D3DFMT_R5G6B5 && mode.Format <= D3DFMT_A4R4G4B4)
            bitsPerPixel = 16;
        else if (mode.Format == D3DFMT_P8 || mode.Format == D3DFMT_DXT3 || mode.Format == D3DFMT_DXT5)
            bitsPerPixel = 8;
        else if (mode.Format == D3DFMT_DXT1)
            bitsPerPixel = 4;

        if (GetMode(static_cast<int>(mode.Width), static_cast<int>(mode.Height), bitsPerPixel) >= 0)
            continue;

        const int index = modeCount;
        width[index] = mode.Width;
        height[index] = mode.Height;

        unsigned int selectedDepthFormat = 0u;
        const D3DFORMAT depthCandidates[4] = {D3DFMT_D16, D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24S8};
        for (unsigned int i = 0;i<4u;++i)
        {
            if (d3d->CheckDepthStencilMatch(adapterIndex, D3DDEVTYPE_HAL, mode.Format, mode.Format, depthCandidates[i]) == D3D_OK)
            {
                selectedDepthFormat = static_cast<unsigned int>(depthCandidates[i]);
                break;
            }
        }
        depthFormat[modeCount] = selectedDepthFormat;
        format[modeCount] = static_cast<unsigned int>(mode.Format);
        ++modeCount;

        STRING depthName = GetPixelFormat(static_cast<D3DFORMAT>(selectedDepthFormat));
        STRING colorName = GetPixelFormat(static_cast<D3DFORMAT>(static_cast<unsigned int>(mode.Format)));
        Error->Log("   Enum display modes %ix%i %s %s", mode.Width, mode.Height, colorName.value, depthName.value);
    }

    d3d->GetDeviceCaps(adapterIndex, D3DDEVTYPE_HAL, &caps);
    textureMemory = 16000000;
    flags = (flags&0xFFFFFFF0u)|1u;
    pixelShaderVersion = caps.PixelShaderVersion&0xFFFFu;
    
}

void GRAPH::DrawSnow()
{
    unsigned char guard = snowInitialized;
    if ((guard&1u) == 0u)
    {
        snowInitialized = static_cast<unsigned char>(guard|1u);
        (void)RegisterCrtAtExit(&GraphStaticCleanupNoOpTertiary);
    }

    unsigned int renderFlags = environmentFlags;
    const unsigned int mode = renderFlags&0xC000u;
    if (mode == 0u)
    {
        snowParticleCount = 0u;
        return;
    }

    if ((Map->Flags()&0x20u) != 0u)
        return;

    const unsigned char direction = windDirection;
    const float speed = windMagnitude;
    if (snowWindDirection != direction || snowWindMagnitude != speed)
    {
        const float wind = ANGLE(direction).Sin()
            *speed*(1000.0f);
        snowHorizontalSpeed = wind;
        snowWindDirection = direction;
        snowWindMagnitude = speed;
    }

    int count = snowParticleCount;
    if (mode == 0xC000u)
        count = 1000;
    else if (mode == 0x8000u)
    {
        if (count<1000)
            count += 4;
        else
        {
            count = 1000;
            SetEnvironment(0xC000u);
        }
    }
    else if (mode == 0x4000u)
    {
        if (count>0)
            count -= 4;
        else
        {
            renderFlags &= ~0xC000u;
            environmentFlags = renderFlags;
            count = 0;
        }
    }
    snowParticleCount = count;

    GraphSnowParticle* const particles = snowParticles;
    const int elapsed = static_cast<int>(::CurrentTime-::PrevCurrentTime);
    const float currentNegCameraX = -Map->ShiftXY().x;
    const float currentNegCameraY = -Map->ShiftXY().y;
    for (int i = 0;i<count;++i)
    {
        GraphSnowParticle& particle = particles[i];
        GraphWeatherVertex& first = particle.vertex[0];
        const bool active = first.color != 0u && first.z >= (0.015625f);
        if (active)
        {
            const float travel = (particle.vertex[1].z-first.z-(0.000122070312f))
                *static_cast<float>(elapsed)*(150.0f);
            float xTravel = (snowHorizontalSpeed*travel+(50.0f)
                -static_cast<float>(Random2(100)))*(0.0199999996f)
                -(snowCameraX-currentNegCameraX);
            float yTravel = travel-(snowCameraY-currentNegCameraY);

            const float minimum = (0.0f);
            const float width = screenWidth;
            const float height = screenHeight;
            if (xTravel+first.x<minimum)
                xTravel += width;
            if (xTravel+first.x>width)
                xTravel -= width;
            if (yTravel+first.y<minimum)
                yTravel += height;
            if (yTravel+first.y>height)
                yTravel -= height;

            const float zTravel = travel*(0.000122070312f);
            for (int vertex = 0;vertex<6;++vertex)
            {
                particle.vertex[vertex].x += xTravel;
                particle.vertex[vertex].y += yTravel;
                particle.vertex[vertex].z -= zTravel;
            }
        }
        else
        {
            const float radius = static_cast<float>(Random2(2))+(2.0f);
            const float centerX = Random2(viewRight+(100.0f))-
                (50.0f);
            const float centerY = Random2(viewBottom);
            const float z = screenHeight*(0.000122070312f)+(0.015625f);
            const float halfRadius = radius*(0.5f);
            const float zTail = z+radius*(0.000122070312f);
            const unsigned int color = 0x80FFFFFFu;
            particle.vertex[0] = {centerX-radius, centerY, z, 1.0f, color};
            particle.vertex[1] = {centerX+radius, centerY, zTail, 1.0f, color};
            particle.vertex[2] = {centerX-halfRadius, centerY-radius, z, 1.0f, color};
            particle.vertex[3] = {centerX+halfRadius, centerY+radius, z, 1.0f, color};
            particle.vertex[4] = {centerX-halfRadius, centerY+radius, z, 1.0f, color};
            particle.vertex[5] = {centerX+halfRadius, centerY-radius, z, 1.0f, color};
        }
    }

    snowCameraX = currentNegCameraX;
    snowCameraY = currentNegCameraY;
    for (int i = 0;i<count;++i)
        if (Random2(4) == 0)
            GraphRotateSnowParticle(particles[i]);

    IDirect3DDevice9* const device = this->device;
    (void)device->SetRenderState(D3DRS_ALPHATESTENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
    (void)device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
    (void)device->SetTexture(0u, 0);
    (void)SetAlphaBlend(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
    DrawPrimitive(D3DPT_LINELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, particles, sizeof(GraphWeatherVertex),
        snowParticleCount*2);
}
static int GraphDisplayFormatBits(unsigned int format);

static int GraphRegistryGetInt(const char* keyText, int defaultValue)
{
    const STRING key(keyText);
    REGISTRY* const registry = ::Registry;
    return registry->GetInt(key, defaultValue);
}

static void GraphDiagnostic(TYPE_ERROR category, const STRING& subject, int errorCode)
{
    if (!Error)
        return;
    STRING objectFormat("GRAPH");
    Error->Error(objectFormat, category, subject, static_cast<unsigned int>(errorCode));
}

static void GraphDiagnosticLiteral(TYPE_ERROR category, const char* subjectText, int errorCode)
{
    STRING subject(subjectText);
    GraphDiagnostic(category, subject, errorCode);
}



static void GraphLogSelectedMode(const GRAPH* graph)
{
    STRING depth = GetPixelFormat(static_cast<D3DFORMAT>(graph->depthFormat));
    STRING desktop = GetPixelFormat(static_cast<D3DFORMAT>(graph->desktopFormat));
    STRING backBuffer = GetPixelFormat(static_cast<D3DFORMAT>(graph->backBufferFormat));
    if (Error)
        Error->Log("Selected display mode %.0fx%.0f %s desktop %s zbuffer %s", static_cast<double>(graph->screenWidth), static_cast<double>(graph->screenHeight), backBuffer.value, desktop.value, depth.value);
}

static int GraphCreatePseudo3DEffect(GRAPH* graph, const char* path, bool logSuccess, STRING& subject)
{
    ID3DXBuffer* errors = 0;
    const int hr = static_cast<int>(D3DXCreateEffectFromFileA(
        graph->device, path, 0, 0, 0, 0, &graph->pseudo3DEffect, &errors));

    if (errors)
    {
        const STRING message(static_cast<const char*>(errors->GetBufferPointer()));
        const STRING combined = "Effect:\n"+message;
        subject = combined;
    }
    else
    {
        const STRING description("Effect");
        subject = description;
    }

    if (errors)
        (void)errors->Release();

    if (hr<0)
        GraphDiagnostic(E_CREATE, subject, hr);
    else if (logSuccess)
        Error->Log(subject);
    return hr;
}

static void GraphLogTextureStage()
{
    ::Error->Log(GetTextureState());
}

static void GraphBuildCapsText(GRAPH* graph)
{
    STRING caps;
    caps = "";
    const unsigned int flags = graph->capabilityFlags;
    if (flags&0x08u)
        caps += "NONGDI ";
    caps += (flags&0x10u)?"SOFTWARE ":"HARDWARE ";
    if (flags&0x04u)
        caps += "AGP ";
    if ((flags&0x40u) == 0u)
        caps += "WINDOWED ";
    caps += (flags&0x20u)?"TRIPLEBUFFER ":"DOUBLEBUFFER ";
    if (flags&0x00000200u)
        caps += "DOTPRODUCT3 ";
    if ((flags&0x00000400u) == 0u)
        caps += "NOTMODULATE2X ";
    if (flags&0x02u)
        caps += "COLOR32 ";
    if (flags&0x80u)
        caps += "VSYNC ";
    if (flags&0x00002000u)
        caps += "STENCILTWOSIDED ";
    if (flags&0x00000800u)
        caps += "COLORWRITEENABLE ";

    caps += Printf("PIXELSHADER=%3x ", graph->pixelShaderVersion);
    caps += Printf("VERTEXSHADER=%3x ", graph->vertexShaderVersion);
    caps += Printf("MaxStreams=%i ", graph->maxStreams);
    caps += Printf("MaxLights=%i ", graph->maxLights);

    MYERROR* const logger = ::Error;
    if (logger)
        logger->Log("caps=%s", caps.value);
    graph->ReLoadPalettes();
}

void GRAPH::RestoreDeviceObjects()
{
    const int backBufferHr = static_cast<int>(device->GetBackBuffer(
        0u, 0u, D3DBACKBUFFER_TYPE_MONO, &renderTarget));
    if (backBufferHr != 0)
    {
        const STRING subject("BackBuffer");
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_GET, subject, static_cast<unsigned long>(backBufferHr));
        }
    }
    savedRenderTarget = renderTarget;

    if (graphDetail <= 5u)
    {
        TEXTURE* const sample = new TEXTURE(64, 64, D3DFMT_A8R8G8B8, 3u);
        lightSampleTexture = sample;
        if (sample->m_texture == 0 && sample->m_cpuData == 0)
        {
            const STRING subject("lightSample");
            if (::Error)
            {
                const STRING object("GRAPH");
                ::Error->Error(object, E_CREATE, subject, 0u);
            }
        }
        else
        {
            int pitch;
            unsigned char* const bits = sample->Lock(&pitch, 0);
            for (int y = 0;y<64;++y)
            {
                const double fy = (static_cast<double>(y)-32.0f)*0.0441970825f;
                const double fy2 = fy*fy;
                for (int x = 0;x<64;++x)
                {
                    const double fx = (static_cast<double>(x)-32.0f)*0.03125f;
                    double value = ((0.200000003f/(fx*fx+fy2+0.0399999991f))-0.200000003f)*256.0f*0.200000003f;
                    if (!(value >= 0.0f))
                        value = 0.0f;
                    else if (value>255.0f)
                        value = 255.0f;
                    const unsigned int channel = static_cast<unsigned int>(TruncateDoubleToInt32(value));
                    const unsigned int rgba = channel|(channel<<8)|(channel<<16)|(channel<<24);
                    const unsigned int offset = static_cast<unsigned int>(y)*static_cast<unsigned int>(pitch)+
                        static_cast<unsigned int>(x)*4u;
                    *reinterpret_cast<unsigned int*>(bits+offset) = rgba;
                }
            }
            lightSampleTexture->UnLock();
        }
    }

    const unsigned int detail = graphDetail;
    if (detail != 0u)
    {
        const int width = TruncateFloatToInt32(screenWidth);
        const int height = TruncateFloatToInt32(screenHeight);
        TEXTURE* texture;
        if (detail >= 8u)
        {
            texture = new TEXTURE(width, height, D3DFMT_A8R8G8B8, 2u);
        }
        else
        {
            unsigned int format = backBufferFormat;
            if (detail<5u)
            {
                const HRESULT check = direct3D->CheckDepthStencilMatch(
                    selectedAdapter, D3DDEVTYPE_HAL, static_cast<D3DFORMAT>(desktopFormat),
                    D3DFMT_R5G6B5, static_cast<D3DFORMAT>(depthFormat));
                if (check == D3D_OK)
                    format = D3DFMT_R5G6B5;
            }
            if ((deviceState&1u) != 0u && graphDetail >= 5u)
                texture = new TEXTURE(width, height, static_cast<D3DFORMAT>(format), 2u);
            else
                texture = new TEXTURE(256, 256, static_cast<D3DFORMAT>(format), 2u);
        }
        lightRenderTexture = texture;
        if (texture->m_texture == 0 && texture->m_cpuData == 0)
        {
            {
                const STRING subject("render target");
                if (::Error)
                {
                    const STRING object("GRAPH");
                    ::Error->Error(object, E_CREATE, subject, 0u);
                }
            }
            if (lightRenderTexture)
            {
                delete lightRenderTexture;
                lightRenderTexture = 0;
            }
        }
        else
        {
            const STRING formatName = GetPixelFormat(static_cast<D3DFORMAT>(texture->m_format));
            ::Error->Log("Create renderTarget (%i, %i, %s)", lightRenderTexture->m_width, lightRenderTexture->m_height, formatName.value);
        }
    }

    if (fontBackend)
        (void)static_cast<ID3DXFont*>(fontBackend)->OnResetDevice();
    (void)pseudo3DEffect->OnResetDevice();

    char drawLight[128];
    std::memcpy(drawLight, "DrawLight", 10u);
    std::memset(drawLight+10, 0, 118u);
    std::strcat(drawLight, graphDetail == 9u?"20":"10");
    drawLightTechnique = pseudo3DEffect->GetTechniqueByName(drawLight);
    drawBackWithZTechnique = pseudo3DEffect->GetTechniqueByName("DrawBackWithZ");
    drawVolumeFogTechnique = pseudo3DEffect->GetTechniqueByName("DrawVolumeFog");
    drawObjectWithoutShaderTechnique = pseudo3DEffect->GetTechniqueByName("DrawObjectWithoutShader");
    drawObjectTechnique = pseudo3DEffect->GetTechniqueByName("DrawObject");
    drawObjectWithZTechnique = pseudo3DEffect->GetTechniqueByName("DrawObjectWithZ");
    renderTargetTextureParameter = pseudo3DEffect->GetParameterByName(0, "RenderTargetTexture");
    lightColorParameter = pseudo3DEffect->GetParameterByName(0, "LightColor");
    zCoeffParameter = pseudo3DEffect->GetParameterByName(0, "ZCoeff");
    texShiftParameter = pseudo3DEffect->GetParameterByName(0, "TexShift");
    texSizeParameter = pseudo3DEffect->GetParameterByName(0, "TexSize");
    diffuseParameter = pseudo3DEffect->GetParameterByName(0, "Diffuse");
    specularParameter = pseudo3DEffect->GetParameterByName(0, "Specular");
    worldViewMatrixParameter = pseudo3DEffect->GetParameterByName(0, "mWorldView");
    viewMatrixParameter = pseudo3DEffect->GetParameterByName(0, "mView");
    projectionMatrixParameter = pseudo3DEffect->GetParameterByName(0, "mProjection");

    IDirect3DDevice9* const tempDevice = device;
    const D3DFORMAT tempFormat = static_cast<D3DFORMAT>(backBufferFormat);
    const unsigned int targetHeight = static_cast<unsigned int>(TruncateFloatToInt32(screenHeight));
    const unsigned int targetWidth = static_cast<unsigned int>(TruncateFloatToInt32(screenWidth));
    const int tempHr = static_cast<int>(tempDevice->CreateOffscreenPlainSurface(
        targetWidth, targetHeight, tempFormat, D3DPOOL_SYSTEMMEM, &depthSurface, 0));
    if (tempHr != 0)
    {
        const STRING subject("tempBuffer");
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_CREATE, subject, static_cast<unsigned long>(tempHr));
        }
    }

    IDirect3DDevice9* const clearDevice = device;
    runtimeE18 = 0;
    (void)clearDevice->Clear(0u, 0, D3DCLEAR_STENCIL, 0u, 0.0f, 0u);
    const int depthHr = static_cast<int>(device->CreateDepthStencilSurface(
        256u, 256u, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0u, TRUE, &lightDepthSurface, 0));
    if (depthHr != 0)
    {
        const STRING subject("depthBuffer256x256");
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_CREATE, subject, static_cast<unsigned long>(depthHr));
        }
    }

    const bool sameX = !(viewLeft<viewRight) && !(viewLeft>viewRight);
    const bool sameY = !(viewTop<viewBottom) && !(viewTop>viewBottom);
    if (sameX && sameY)
    {
        if ((capabilityFlags&0x40u) != 0u && (capabilityFlags&0x100u) == 0u)
        {
            const VECTOR2 minimum = {0.0f, 0.0f};
            SetViewPort(minimum, *reinterpret_cast<const VECTOR2*>(&screenWidth));
        }
        else
        {
            RECT client, windowRect;
            (void)::GetWindowRect(reinterpret_cast<HWND>(window), &windowRect);
            (void)::GetClientRect(reinterpret_cast<HWND>(window), &client);
            (void)::ClientToScreen(reinterpret_cast<HWND>(window), reinterpret_cast<POINT*>(&client));
            (void)::ClientToScreen(reinterpret_cast<HWND>(window), reinterpret_cast<POINT*>(&client.right));
            VECTOR2 minimum, maximum;
            maximum.x = static_cast<float>(BitsToSigned32(static_cast<unsigned int>(client.right)-static_cast<unsigned int>(windowRect.left)));
            maximum.y = static_cast<float>(BitsToSigned32(static_cast<unsigned int>(client.bottom)-static_cast<unsigned int>(windowRect.top)));
            minimum.x = static_cast<float>(BitsToSigned32(static_cast<unsigned int>(client.left)-static_cast<unsigned int>(windowRect.left)));
            minimum.y = static_cast<float>(BitsToSigned32(static_cast<unsigned int>(client.top)-static_cast<unsigned int>(windowRect.top)));
            SetViewPort(minimum, maximum);
        }
    }
    else
    {
        SetViewPort(*reinterpret_cast<const VECTOR2*>(&viewLeft),
            *reinterpret_cast<const VECTOR2*>(&viewRight));
    }

    ::Error->Log("SetViewPort (%.0f,%.0f) - (%.0f,%.0f)", static_cast<double>(viewLeft), static_cast<double>(viewTop), static_cast<double>(viewRight), static_cast<double>(viewBottom));
    const int conversionHr = static_cast<int>(direct3D->CheckDeviceFormatConversion(
        0u, D3DDEVTYPE_HAL, static_cast<D3DFORMAT>(backBufferFormat),
        static_cast<D3DFORMAT>(builtPixelFormat)));
    if (conversionHr<0)
    {
        ::Error->Log("Slow type for GetRenderTargetRect()");
        capabilityFlags &= ~0x1000u;
    }
    else capabilityFlags |= 0x1000u;

    SetDefaultState();
    if (Map)
        Map->RestoreDeviceObjects();
    SPRITE_COLLECTOR* const collector = ::Sprites;
    if (collector)
        collector->RestoreDeviceObjects();
    state50 = device->GetAvailableTextureMem()>>1;
}

void GRAPH::Tact(int mode)
{
    struct TextureVertex
    {
        float x, y, z, rhw;
        unsigned int diffuse;
        unsigned int specular;
        float u, v;
    };
    struct ColorVertex
    {
        float x, y, z, rhw;
        unsigned int diffuse;
    };
    struct LitVertex
    {
        float x, y, z, rhw;
        unsigned int diffuse;
        unsigned int specular;
    };

    currentTime = 0u;
    for (unsigned int i = 0;i<16u;++i)
    {
        static_cast<void>(VID_SURFACE::noDrawReal[i] = 0u);
    }
    for (unsigned int i = 0;i<16u;++i)
        static_cast<void>(VID_SURFACE::noDrawCall[i] = 0u);
    static_cast<void>(VID_SURFACE::noObj = 0u);
    static_cast<void>(VID_SURFACE::noTri = 0u);
    static_cast<void>(VID_SURFACE::noVertex = 0u);
    static_cast<void>(VID_SURFACE::noDrawShadow = 0u);
    static_cast<void>(::NoBuildAlpha = 0u);

    const auto renderState = [&](unsigned int state, unsigned int value)
    {
        (void)this->device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(state), value);
    };
    const auto setTexture = [&](unsigned int stage, IDirect3DTexture9* texture)
    {
        (void)this->device->SetTexture(stage, texture);
    };
    const auto textureStageState = [&](unsigned int stage, unsigned int state, unsigned int value)
    {
        (void)this->device->SetTextureStageState(stage, static_cast<D3DTEXTURESTAGESTATETYPE>(state), value);
    };
    const auto samplerState = [&](unsigned int stage, unsigned int state, unsigned int value)
    {
        (void)this->device->SetSamplerState(stage, static_cast<D3DSAMPLERSTATETYPE>(state), value);
    };
    const auto reportTargetError = [](const char* subjectText, int hr)
    {
        const STRING subject(subjectText);
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_SET, subject, static_cast<unsigned long>(hr));
        }
    };
    const auto drawMouseSpriteChain = []()
    {
        MOUSE* const mouse = Mouse;
        SPRITE* sprite = mouse;
        if (mouse->HardwareCursorEnabled() != 0u)
            return;
        VID* const owner = sprite->Vid();
        if ((owner->property&0x00008000u) == 0u)
            return;
        while (sprite)
        {
            if ((sprite->RuntimeFlags()&0x04000000u) == 0u)
                sprite->Draw();
            sprite = sprite->ChildChain();
        }
    };

    if (mode != 0)
    {
        if ((savedCameraShiftInitialized&1u) == 0u)
        {
            savedCameraShiftInitialized |= 1u;
            ::new (static_cast<void*>(savedCameraShift.Address())) VECTOR2;
            (void)RegisterCrtAtExit(&GraphStaticCleanupNoOpPrimary);
        }

        const bool cameraShake = ((*reinterpret_cast<unsigned char*>(&environmentFlags)&4u) != 0u) &&
            ((Map->Flags()&32u) == 0u);
        if (cameraShake)
        {
            const float savedX = Map->ShiftXY().x;
            const float savedY = Map->ShiftXY().y;
            VECTOR2* const savedShift = savedCameraShift.Address();
            savedShift->x = savedX;
            savedShift->y = savedY;
            const float randomX = Random2(8.0f)-4.0f;
            const float randomY = Random2(8.0f)-4.0f;
            const float halfWidth = screenWidth*0.5f;
            const float originX = halfWidth+savedCameraShift.Get().x;
            const double originY = static_cast<double>(screenHeight)*0.5+savedCameraShift.Get().y;
            const VECTOR2 shifted(originX+randomY, static_cast<float>(originY+randomX));
            Map->SetShiftCoor(shifted, 0x10000000);
        }

        const VECTOR2 minPoint(
            Map->ShiftXY().x+viewportOffsetX,
            Map->ShiftXY().y+viewportOffsetY);
        const VECTOR2 maxPoint(
            Map->ShiftXY().x+screenWidth-viewportOffsetX,
            Map->ShiftXY().y+screenHeight-viewportOffsetY);
        unsigned int detail = graphDetail;
        if (detail<3u)
        {
            (void)Graph->device->SetRenderState(D3DRS_SCISSORTESTENABLE, 0u);
        }
        else
        {
            IDirect3DSurface9* surface = renderTarget;
            if (detail >= 8u)
                surface = lightRenderTexture->GetSurface();
            ::Sprites->DrawBuilded(minPoint, maxPoint, surface);
        }

        ::Sprites->DrawLayer(3, minPoint, maxPoint);
        ::Sprites->DrawLayer(4, minPoint, maxPoint);
        ::Sprites->DrawLayer(5, minPoint, maxPoint);
        renderState(D3DRS_SHADEMODE, 1u);

        const float z = 0.99999988079071044921875f;
        detail = graphDetail;

        if (detail >= 8u)
        {
            const int hr = SetRenderTarget(
                renderTarget, *reinterpret_cast<const VECTOR2*>(&viewLeft),
                *reinterpret_cast<const VECTOR2*>(&viewRight), 0);
            if (hr<0)
                reportTargetError("rendet target for draw light", hr);

            TEXTURE* const texture = lightRenderTexture;
            const double textureWidth = static_cast<double>(static_cast<int>(texture->m_width));
            const double textureHeight = static_cast<double>(static_cast<int>(texture->m_height));
            const float width = screenWidth;
            const float height = screenHeight;
            const float u0 = static_cast<float>(0.5/textureWidth);
            const float v0 = static_cast<float>(0.5/textureHeight);
            const float u1 = static_cast<float>((static_cast<double>(width)+0.5)/textureWidth);
            const float v1 = static_cast<float>((static_cast<double>(height)+0.5)/textureHeight);
            const unsigned int diffuse = ~gamma.modulate;
            const unsigned int specular = gamma.add;
            TextureVertex vertex[4] = {
                {
                    0.0f, 0.0f, z, 1.0f, diffuse, specular, u0, v0
                },
                {width, 0.0f, z, 1.0f, diffuse, specular, u1, v0},
                {width, height, z, 1.0f, diffuse, specular, u1, v1},
                {0.0f, height, z, 1.0f, diffuse, specular, u0, v1}
            };
            setTexture(0u, texture->m_texture);
            samplerState(0u, D3DSAMP_MINFILTER, D3DTEXF_POINT);
            samplerState(0u, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
            textureStageState(0u, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            textureStageState(0u, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            renderState(D3DRS_SPECULARENABLE, specular != 0u?1u:0u);
            renderState(D3DRS_ALPHATESTENABLE, 0u);
            renderState(D3DRS_ALPHABLENDENABLE, 0u);
            renderState(D3DRS_ZWRITEENABLE, 0u);
            renderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
            DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1, vertex, 32u, 4);
            if ((*reinterpret_cast<unsigned char*>(&deviceState)&2u) != 0u)
                ::Sprites->DrawLayer(7, minPoint, maxPoint);
        }
        else if (detail >= 3u)
        {
            const VECTOR2 savedViewMin(viewLeft, viewTop);
            const VECTOR2 savedViewMax(viewRight, viewBottom);
            VECTOR2 lightSize;
            if ((*reinterpret_cast<unsigned char*>(&deviceState)&1u) != 0u && detail >= 5u)
            {
                lightSize.x = screenWidth;
                lightSize.y = screenHeight;
            }
            else
            {
                TEXTURE* const texture = lightRenderTexture;
                lightSize.x = static_cast<float>(static_cast<int>(texture->m_width));
                lightSize.y = static_cast<float>(static_cast<int>(texture->m_height));
            }
            const VECTOR2 zero(0.0f, 0.0f);
            IDirect3DSurface9* const lightSurface = lightRenderTexture->GetSurface();
            int hr = SetRenderTarget(lightSurface, zero, lightSize, 0);
            if (hr<0)
                reportTargetError("rendet target for draw light", hr);

            const unsigned int complement = ~gamma.modulate;
            const unsigned int add = gamma.add;
            unsigned int lightColor = 0xFFFFFFFFu;
            if (gamma.modulate != 0u || add != 0u)
            {
                const int red = ((((static_cast<int>((complement>>16)&0xFFu)+1)*255)>>8)+
                    static_cast<int>((add>>16)&0xFFu));
                const int green = ((((static_cast<int>((complement>>8)&0xFFu)+1)*255)>>8)+
                    static_cast<int>((add>>8)&0xFFu));
                const int blue = ((((static_cast<int>(complement&0xFFu)+1)*255)>>8)+
                    static_cast<int>(add&0xFFu));
                GraphPackOpaqueRgb(&lightColor, red, green, blue);
            }
            const int clearBlue = TruncateDoubleToInt32(static_cast<double>(lightColor&0xFFu)*0.3f);
            const int clearGreen = TruncateDoubleToInt32(static_cast<double>((lightColor>>8)&0xFFu)*0.3f);
            const int clearRed = TruncateDoubleToInt32(static_cast<double>((lightColor>>16)&0xFFu)*0.3f);
            unsigned int clearColor;
            GraphPackOpaqueRgb(&clearColor, clearRed, clearGreen, clearBlue);
            (void)this->device->Clear(0u, 0, D3DCLEAR_TARGET, clearColor, 0.0f, 0u);

            if ((*reinterpret_cast<unsigned char*>(&deviceState)&2u) != 0u)
                ::Sprites->DrawLayer(7, minPoint, maxPoint);

            hr = SetRenderTarget(renderTarget, savedViewMin, savedViewMax, 0);
            if (hr<0)
                reportTargetError("rendet target for draw light2", hr);

            TEXTURE* const texture = lightRenderTexture;
            const double textureWidth = static_cast<double>(static_cast<int>(texture->m_width));
            const double textureHeight = static_cast<double>(static_cast<int>(texture->m_height));
            const float width = screenWidth;
            const float height = screenHeight;
            const float u0 = static_cast<float>(0.5/textureWidth);
            const float v0 = static_cast<float>(0.5/textureHeight);
            const float u1 = static_cast<float>((static_cast<double>(lightSize.x)+0.5)/textureWidth);
            const float v1 = static_cast<float>((static_cast<double>(lightSize.y)+0.5)/textureHeight);
            const unsigned int specular = gamma.add;
            TextureVertex textureQuad[4] = {
                {
                    0.0f, 0.0f, z, 1.0f, 0xFFFFFFFFu, specular, u0, v0
                },
                {width, 0.0f, z, 1.0f, 0xFFFFFFFFu, specular, u1, v0},
                {width, height, z, 1.0f, 0xFFFFFFFFu, specular, u1, v1},
                {0.0f, height, z, 1.0f, 0xFFFFFFFFu, specular, u0, v1}
            };
            setTexture(0u, texture->m_texture);
            samplerState(0u, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            samplerState(0u, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            textureStageState(0u, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            textureStageState(0u, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
            textureStageState(0u, D3DTSS_COLOROP, D3DTOP_MODULATE);
            (void)SetAlphaBlend(D3DBLEND_DESTCOLOR, D3DBLEND_SRCCOLOR);
            renderState(D3DRS_SPECULARENABLE, specular != 0u?1u:0u);
            renderState(D3DRS_ALPHATESTENABLE, 0u);
            renderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
            renderState(D3DRS_ZWRITEENABLE, 0u);
            DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1, textureQuad, 32u, 4);
            setTexture(0u, 0u);

            const float colorWidth = screenWidth;
            const float colorHeight = screenHeight;
            ColorVertex colorQuad[4] = {
                {
                    0.0f, 0.0f, z, 1.0f, 0xFFFFFFFFu
                },
                {colorWidth, 0.0f, z, 1.0f, 0xFFFFFFFFu},
                {colorWidth, colorHeight, z, 1.0f, 0xFFFFFFFFu},
                {0.0f, colorHeight, z, 1.0f, 0xFFFFFFFFu}
            };
            (void)SetAlphaBlend(D3DBLEND_DESTCOLOR, D3DBLEND_ONE);
            renderState(D3DRS_SPECULARENABLE, 0u);
            DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, colorQuad, 20u, 4);
        }
        else
        {
            const float x0 = viewLeft;
            const float y0 = viewTop;
            const float x1 = viewRight;
            const float y1 = viewBottom;
            const unsigned int diffuse = ~gamma.modulate;
            const unsigned int specular = gamma.add;
            LitVertex quad[4] = {
                {
                    x0, y0, z, 1.0f, diffuse, specular
                },
                {x1, y0, z, 1.0f, diffuse, specular},
                {x1, y1, z, 1.0f, diffuse, specular},
                {x0, y1, z, 1.0f, diffuse, specular}
            };
            setTexture(0u, 0u);
            (void)SetAlphaBlend(D3DBLEND_ZERO, D3DBLEND_SRCCOLOR);
            renderState(D3DRS_SPECULARENABLE, specular != 0u?1u:0u);
            renderState(D3DRS_ALPHATESTENABLE, 0u);
            renderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
            renderState(D3DRS_ZWRITEENABLE, 0u);
            DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, quad, 24u, 4);
            ::Sprites->DrawLayer(7, minPoint, maxPoint);
        }

        renderState(D3DRS_SHADEMODE, 2u);
        ::Sprites->DrawLayer(8, minPoint, maxPoint);
        DrawSnow();
        DrawRain();

        if ((environmentFlags&4u) != 0u && (Map->Flags()&32u) == 0u)
        {
            const float halfHeight = screenHeight*0.5f;
            const VECTOR2 restored(
                static_cast<float>(static_cast<double>(screenWidth)*0.5+savedCameraShift.Get().x),
                halfHeight+savedCameraShift.Get().y);
            Map->SetShiftCoor(restored, 0x10000000);
        }

        const VECTOR2 layer9Max(Map->ShiftXY().x+screenWidth, Map->ShiftXY().y+screenHeight);
        ::Sprites->DrawLayer(9, Map->ShiftXY(), layer9Max);
        drawMouseSpriteChain();
        renderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
    }

    DrawSquall();
    if (movie.graphBuilder != 0 && movie.Update() != 0)
        movie.Release();
    renderState(D3DRS_SPECULARENABLE, 0u);
    DrawEffect(mode);
}

void GRAPH::DrawLight(const VECTOR& position, const VECTOR2& size, COLOR color)
{
    struct FixedVertex
    {
        float x, y, z, rhw;
        unsigned int diffuse;
        float u, v;
    };
    struct ShaderVertex
    {
        float x, y, z, rhw, u, v, lightX, lightY, lightZ, intensity;
    };

    currentTime += static_cast<unsigned int>(TruncateDoubleToInt32(static_cast<double>(size.x)*size.y))<<2;

    const auto renderState = [&](unsigned int state, unsigned int value)
    {
        (void)this->device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(state), value);
    };
    const auto setTexture = [&](unsigned int stage, IDirect3DTexture9* texture)
    {
        (void)this->device->SetTexture(stage, texture);
    };
    const auto textureStageState = [&](unsigned int stage, unsigned int state, unsigned int value)
    {
        (void)this->device->SetTextureStageState(stage, static_cast<D3DTEXTURESTAGESTATETYPE>(state), value);
    };
    const auto samplerState = [&](unsigned int stage, unsigned int state, unsigned int value)
    {
        (void)this->device->SetSamplerState(stage, static_cast<D3DSAMPLERSTATETYPE>(state), value);
    };

    renderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
    const unsigned int detail = graphDetail;

    if (detail == 0u)
    {
        const float centerY = position.y-position.z;
        const VECTOR2 low = Map->ToScreenScaled(VECTOR2(position.x-size.x, centerY-size.y));
        const float highCenterY = position.y-position.z;
        const VECTOR2 high = Map->ToScreenScaled(VECTOR2(position.x+size.x, highCenterY+size.y));
        FixedVertex vertex[4];
        const float z = 0.99999988079071044921875f;
        vertex[0] = {low.x, low.y, z, 1.0f, color.value, 0.0f, 0.0f};
        vertex[1] = {high.x, low.y, z, 1.0f, color.value, 1.0f, 0.0f};
        vertex[2] = {high.x, high.y, z, 1.0f, color.value, 1.0f, 1.0f};
        vertex[3] = {low.x, high.y, z, 1.0f, color.value, 0.0f, 1.0f};

        TEXTURE* const texture = lightSampleTexture;
        setTexture(0u, texture->m_texture);
        renderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        renderState(D3DRS_ZWRITEENABLE, 0u);
        renderState(D3DRS_SPECULARENABLE, 0u);
        (void)SetAlphaBlend(D3DBLEND_DESTCOLOR, D3DBLEND_ONE);
        samplerState(0u, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        samplerState(0u, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        textureStageState(0u, D3DTSS_COLOROP, D3DTOP_MODULATE);
        textureStageState(0u, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        textureStageState(0u, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, vertex, 28u, 4);
    }
    else if (detail <= 5u)
    {
        TEXTURE* const sizeTexture = lightRenderTexture;
        const float textureHeight = static_cast<float>(static_cast<int>(sizeTexture->m_height));
        const float scaleX = static_cast<float>(static_cast<double>(static_cast<int>(sizeTexture->m_width))/screenWidth);
        const float scaleY = textureHeight/screenHeight;
        const float centerY = position.y-position.z;
        const VECTOR2 lowProjected = Map->ToScreenScaled(
            VECTOR2(position.x-size.x, centerY-size.y+10.0f));
        const VECTOR2 low(lowProjected.x*scaleX, lowProjected.y*scaleY);
        const float highCenterY = position.y-position.z;
        const VECTOR2 highProjected = Map->ToScreenScaled(
            VECTOR2(position.x+size.x, highCenterY+size.y+10.0f));
        const VECTOR2 high(highProjected.x*scaleX, highProjected.y*scaleY);

        const double range = 10.0/static_cast<double>(size.y);
        const double attenuation = static_cast<double>(0.2f)/(range*range+static_cast<double>(0.2f));
        const int blueValue = TruncateDoubleToInt32((color.value&0xFFu)*attenuation);
        const int greenValue = TruncateDoubleToInt32(((color.value>>8)&0xFFu)*attenuation);
        const int redValue = TruncateDoubleToInt32(((color.value>>16)&0xFFu)*attenuation);
        unsigned int packed;
        GraphPackOpaqueRgb(&packed, redValue, greenValue, blueValue);
        int alpha = static_cast<int>((packed>>16)&0xFFu);
        const int green = static_cast<int>((packed>>8)&0xFFu);
        const int blue = static_cast<int>(packed&0xFFu);
        if (green>alpha)
            alpha = green;
        if (blue>alpha)
            alpha = blue;
        alpha = ClampByteInt(alpha);
        packed = (packed&0x00FFFFFFu)|(static_cast<unsigned int>(alpha)<<24);

        TEXTURE* const texture = lightSampleTexture;
        const float halfU = static_cast<float>(0.5/static_cast<double>(static_cast<int>(texture->m_width)));
        const double halfV = 0.5/static_cast<double>(static_cast<int>(texture->m_height));
        const double baseZ = (static_cast<double>(position.z)+178.0)*0.0001220703125;
        const double nearZ = baseZ+0.015869140625;
        FixedVertex vertex[4];
        vertex[0] = {low.x, low.y, static_cast<float>(nearZ), 1.0f, packed, halfU, static_cast<float>(halfV)};
        vertex[1] = {high.x, low.y, static_cast<float>(nearZ), 1.0f, packed, 1.0f+halfU, static_cast<float>(halfV)};
        vertex[2] = {high.x, high.y, static_cast<float>(baseZ), 1.0f, packed, 1.0f+halfU, static_cast<float>(1.0+halfV)};
        vertex[3] = {low.x, high.y, static_cast<float>(baseZ), 1.0f, packed, halfU, static_cast<float>(1.0+halfV)};

        setTexture(0u, texture->m_texture);
        renderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        renderState(D3DRS_ZWRITEENABLE, 0u);
        renderState(D3DRS_ALPHATESTENABLE, 1u);
        renderState(D3DRS_ALPHAREF, graphDetail<5u?3u:2u);
        renderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
        renderState(D3DRS_SPECULARENABLE, 0u);
        (void)SetAlphaBlend(D3DBLEND_ONE, D3DBLEND_ONE);
        textureStageState(0u, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        textureStageState(0u, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        textureStageState(0u, D3DTSS_COLOROP, D3DTOP_MODULATE);
        textureStageState(0u, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        textureStageState(0u, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        textureStageState(0u, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        samplerState(0u, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        samplerState(0u, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, vertex, 28u, 4);
    }
    else if (detail >= 8u)
    {
        ID3DXEffect* effect = pseudo3DEffect;
        (void)effect->SetTechnique(drawLightTechnique);
        TEXTURE* texture = lightRenderTexture;
        effect = pseudo3DEffect;
        (void)effect->SetTexture(renderTargetTextureParameter, texture->m_texture);

        const float inv255 = 0.003921568859368563f;
        float lightColor[4] = {
            static_cast<float>(static_cast<double>((color.value>>16)&0xFFu)*inv255*0.2f),
            static_cast<float>((color.value>>8)&0xFFu)*inv255*0.2f,
            static_cast<float>(color.value&0xFFu)*inv255*0.2f,
            0.2f};
        effect = pseudo3DEffect;
        (void)effect->SetVector(lightColorParameter, reinterpret_cast<const D3DXVECTOR4*>(lightColor));
        float lightRange[4] = {0.0f, 512.0f/size.y, -512.0f/size.y, 0.0f};
        effect = pseudo3DEffect;
        (void)effect->SetVector(zCoeffParameter, reinterpret_cast<const D3DXVECTOR4*>(lightRange));

        const float centerY = position.y-position.z;
        const VECTOR2 low = Map->ToScreenScaled(VECTOR2(position.x-size.x, centerY-size.y));
        const float highCenterY = position.y-position.z;
        const VECTOR2 high = Map->ToScreenScaled(VECTOR2(position.x+size.x, highCenterY+size.y));
        texture = lightRenderTexture;
        const double texWidth = static_cast<double>(static_cast<int>(texture->m_width));
        const double texHeight = static_cast<double>(static_cast<int>(texture->m_height));
        const float minXHalf = low.x+0.5f;
        const double minYHalf = static_cast<double>(low.y)+0.5;
        const float maxXHalf = high.x+0.5f;
        const float maxYHalf = high.y+0.5f;
        ShaderVertex vertex[4];

        if (detail == 8u)
        {
            const float zRatio = position.z/size.y;
            const double deviceZ = (static_cast<double>(position.z)+328.0)*0.0001220703125;
            const float negativeRatio = -zRatio;
            const float shiftedRatio = zRatio+0.5f;
            const float inverseRatio = 1.0f-zRatio;
            vertex[0] = {low.x, low.y, static_cast<float>(deviceZ), 1.0f, static_cast<float>(minXHalf/texWidth), static_cast<float>(minYHalf/texHeight), 0.0f, static_cast<float>(negativeRatio), static_cast<float>(shiftedRatio), 0.2f};
            vertex[1] = {high.x, low.y, static_cast<float>(deviceZ), 1.0f, static_cast<float>(maxXHalf/texWidth), static_cast<float>(minYHalf/texHeight), 1.0f, static_cast<float>(negativeRatio), static_cast<float>(shiftedRatio), 0.2f};
            vertex[2] = {high.x, high.y, static_cast<float>(deviceZ), 1.0f, static_cast<float>(maxXHalf/texWidth), static_cast<float>(maxYHalf/texHeight), 1.0f, static_cast<float>(inverseRatio), static_cast<float>(shiftedRatio), 0.2f};
            vertex[3] = {low.x, high.y, static_cast<float>(deviceZ), 1.0f, static_cast<float>(minXHalf/texWidth), static_cast<float>(maxYHalf/texHeight), 0.0f, static_cast<float>(inverseRatio), static_cast<float>(shiftedRatio), 0.2f};
            renderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        }
        else
        {
            const double zRatio = static_cast<double>(position.z)/size.y;
            const double deviceZ = (static_cast<double>(position.z)+278.0)*0.0001220703125;
            const double lowerPlane = -1.414306640625-zRatio;
            const double upperPlane = 1.414306640625-zRatio;
            vertex[0] = {low.x, low.y, static_cast<float>(deviceZ), 1.0f, static_cast<float>(minXHalf/texWidth), static_cast<float>(minYHalf/texHeight), -1.0f, static_cast<float>(lowerPlane), static_cast<float>(zRatio), 0.2f};
            vertex[1] = {high.x, low.y, static_cast<float>(deviceZ), 1.0f, static_cast<float>(maxXHalf/texWidth), static_cast<float>(minYHalf/texHeight), 1.0f, static_cast<float>(lowerPlane), static_cast<float>(zRatio), 0.2f};
            vertex[2] = {high.x, high.y, static_cast<float>(deviceZ), 1.0f, static_cast<float>(maxXHalf/texWidth), static_cast<float>(maxYHalf/texHeight), 1.0f, static_cast<float>(upperPlane), static_cast<float>(zRatio), 0.2f};
            vertex[3] = {low.x, high.y, static_cast<float>(deviceZ), 1.0f, static_cast<float>(minXHalf/texWidth), static_cast<float>(maxYHalf/texHeight), -1.0f, static_cast<float>(upperPlane), static_cast<float>(zRatio), 0.2f};
            renderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
        }

        renderState(D3DRS_ZWRITEENABLE, 0u);
        (void)SetAlphaBlend(D3DBLEND_ONE, D3DBLEND_ONE);
        renderState(D3DRS_ALPHATESTENABLE, 0u);
        unsigned int passCount = 0u;
        effect = pseudo3DEffect;
        (void)effect->Begin(&passCount, 0u);
        for (unsigned int pass = 0;pass<passCount;++pass)
        {
            effect = pseudo3DEffect;
            (void)effect->BeginPass(pass);
            DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE4(1), vertex, 40u, 4);
            (void)effect->EndPass();
        }
        effect = pseudo3DEffect;
        (void)effect->End();
    }

    renderState(D3DRS_STENCILENABLE, 0u);
}

namespace {

static unsigned int GraphGrayRgb(unsigned int level)
{
    level &= 0xFFu;
    return level|(level<<8)|(level<<16);
}

static unsigned int GraphScaleRgb(unsigned int color, unsigned int scale)
{
    const unsigned int r = (((color>>16)&0xFFu)*scale)>>8;
    const unsigned int g = (((color>>8)&0xFFu)*scale)>>8;
    const unsigned int b = ((color&0xFFu)*scale)>>8;
    return (r<<16)|(g<<8)|b;
}

static int GraphGammaChannel(const GAMMA& gamma, unsigned int shift)
{
    if (shift == 0u)
        return gamma.Blue();
    const unsigned int negative = (gamma.modulate>>shift)&0xFFu;
    return negative? -static_cast<int>(negative):static_cast<int>((gamma.add>>shift)&0xFFu);
}

static void GraphDrawGrayOverlay(GRAPH* graph, unsigned int level)
{
    graph->ShadowBar(VECTOR2(0.0f, 0.0f), *reinterpret_cast<const VECTOR2*>(&graph->screenWidth), BitsToSigned32(GraphGrayRgb(level)));
}
}

STRING GetPixelFormat(_D3DFORMAT format)
{
    const char* text = 0;
    switch (format)
    {
    case D3DFMT_R8G8B8:
        text = "R8G8B8";
    break;
    case D3DFMT_A8R8G8B8:
        text = "A8R8G8B8";
    break;
    case D3DFMT_X8R8G8B8:
        text = "X8R8G8B8";
    break;
    case D3DFMT_R5G6B5:
        text = "R5G6B5";
    break;
    case D3DFMT_X1R5G5B5:
        text = "X1R5G5B5";
    break;
    case D3DFMT_A1R5G5B5:
        text = "A1R5G5B5";
    break;
    case D3DFMT_A4R4G4B4:
        text = "A4R4G4B4";
    break;
    case D3DFMT_A8:
        text = "A8";
    break;
    case D3DFMT_A8P8:
        text = "A8P8";
    break;
    case D3DFMT_P8:
        text = "P8";
    break;
    case D3DFMT_L8:
        text = "L8";
    break;
    case D3DFMT_A8L8:
        text = "A8L8";
    break;
    case D3DFMT_A4L4:
        text = "A4L4";
    break;
    case D3DFMT_V8U8:
        text = "V8U8";
    break;
    case D3DFMT_D16_LOCKABLE:
        text = "D16_LOCKABLE";
    break;
    case D3DFMT_D32:
        text = "D32";
    break;
    case D3DFMT_D15S1:
        text = "D15S1";
    break;
    case D3DFMT_D24S8:
        text = "D24S8";
    break;
    case D3DFMT_D24X8:
        text = "D24X8";
    break;
    case D3DFMT_D16:
        text = "D16";
    break;
    case D3DFMT_VERTEXDATA:
        text = "VERTEXDATA";
    break;
    case D3DFMT_INDEX16:
        text = "INDEX16";
    break;
    case D3DFMT_INDEX32:
        text = "INDEX32";
    break;
    case D3DFMT_DXT1:
        text = "DXT1";
    break;
    case D3DFMT_DXT2:
        text = "DXT2";
    break;
    case D3DFMT_DXT3:
        text = "DXT3";
    break;
    case D3DFMT_DXT4:
        text = "DXT4";
    break;
    case D3DFMT_DXT5:
        text = "DXT5";
    break;
    default:
        text = "Unknown";
    break;
    }
    return STRING(text);
}

struct GraphDialogItemRef
{
    HWND window;
    unsigned int controlId;
};

static unsigned int GraphDialogSend(const void* item, unsigned int message, unsigned int wParam, unsigned int lParam)
{
    const GraphDialogItemRef* const ref = static_cast<const GraphDialogItemRef*>(item);
    return static_cast<unsigned int>(::SendDlgItemMessageA(
        ref->window, static_cast<int>(ref->controlId), message,
        static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam)));
}

static int GraphDisplayFormatBits(unsigned int format)
{
    if (format == D3DFMT_R8G8B8)
        return 24;
    if (format == D3DFMT_A8R8G8B8 || format == D3DFMT_X8R8G8B8)
        return 32;
    if (format >= D3DFMT_R5G6B5 && format <= D3DFMT_A4R4G4B4)
        return 16;
    if (format == D3DFMT_P8 || format == D3DFMT_DXT3 || format == D3DFMT_DXT5)
        return 8;
    if (format == D3DFMT_DXT1)
        return 4;
    return 0;
}

int GRAPH::Init(HWND__* windowHandle)
{
    GRAPH* const graph = this;
    graph->windDirection = 220u;
    graph->windMagnitude = 0.02f;

    {
        const STRING key("ScreenX");
        const int value = TruncateFloatToInt32(graph->screenWidth);
        REGISTRY* const registry = ::Registry;
        registry->SetInt(key, value);
    }
    {
        const STRING key("ScreenY");
        const int value = TruncateFloatToInt32(graph->screenHeight);
        REGISTRY* const registry = ::Registry;
        registry->SetInt(key, value);
    }
    {
        const STRING key("BPP");
        const int value = (graph->capabilityFlags&2u)?32:16;
        REGISTRY* const registry = ::Registry;
        registry->SetInt(key, value);
    }
    {
        const STRING key("Device");
        const int value = static_cast<int>(graph->selectedAdapter);
        REGISTRY* const registry = ::Registry;
        registry->SetInt(key, value);
    }
    {
        const STRING key("FullScreen");
        const int value = static_cast<int>((graph->capabilityFlags>>6u)&1u);
        REGISTRY* const registry = ::Registry;
        registry->SetInt(key, value);
    }

    DD_DRIVER* const adapter = &graph->adapters[graph->selectedAdapter];
    unsigned int flags = graph->capabilityFlags;
    flags = (flags&~0x08u)|((adapter->flags&2u)<<2u);
    graph->capabilityFlags = flags;
    if ((adapter->flags&2u) != 0u)
        Error->Log("zm_nongdi");

    flags = graph->capabilityFlags&~0x08u;
    graph->capabilityFlags = flags;
    const bool tripleRequested = GraphRegistryGetInt("TripleBuffer", 1) != 0;
    const bool tripleAllowed = tripleRequested && ((flags&0x40u) != 0u) && ((flags&0x100u) == 0u);
    flags = (flags&~0x20u)|(tripleAllowed?0x20u:0u);
    if ((adapter->flags&1u) == 0u)
        flags |= 0x40u;
    graph->capabilityFlags = flags;

    if ((flags&0x40u) == 0u)
    {
        const int desktopWidth = ::GetSystemMetrics(SM_CXSCREEN);
        const int desktopHeight = ::GetSystemMetrics(SM_CYSCREEN);
        if (static_cast<float>(desktopWidth)<graph->screenWidth)
            graph->screenWidth = static_cast<float>(desktopWidth);
        if (static_cast<float>(desktopHeight)<graph->screenHeight)
            graph->screenHeight = static_cast<float>(desktopHeight);
        const bool desktop32 = GraphDisplayFormatBits(adapter->desktopFormat) == 32;
        const bool selected32 = (graph->capabilityFlags&2u) != 0u;
        if (desktop32 != selected32)
            graph->capabilityFlags = (graph->capabilityFlags&~2u)|(desktop32?2u:0u);
    }

    const int rawBudget = adapter->textureMemory;
    const int halfBudget = (rawBudget-(rawBudget<0?-1:0))/2;
    const double tripleBytes = static_cast<double>(graph->screenWidth)*
        static_cast<double>(graph->screenHeight)*2.0*3.0;
    if (tripleBytes >= static_cast<double>(halfBudget))
        graph->capabilityFlags &= ~0x20u;
    graph->capabilityFlags &= ~0x20u;

    graph->window = windowHandle;
    memset(&graph->present, 0, sizeof(graph->present));

    flags = graph->capabilityFlags;
    if (flags&0x40u)
    {
        graph->desktopFormat = D3DFMT_X8R8G8B8;
        graph->backBufferFormat = D3DFMT_A8R8G8B8;
    }
    else
    {
        graph->desktopFormat = adapter->desktopFormat;
        graph->backBufferFormat = (adapter->desktopFormat == D3DFMT_X8R8G8B8)?D3DFMT_A8R8G8B8:adapter->desktopFormat;
    }

    graph->present.backBufferFormat = graph->backBufferFormat;
    const D3DFORMAT depthCandidates[3] = {D3DFMT_D24S8, D3DFMT_D16, D3DFMT_D24X8};
    unsigned int depthIndex = 0u;
    for (;depthIndex<3u;++depthIndex)
    {
        const HRESULT hr = graph->direct3D->CheckDepthStencilMatch(
            graph->selectedAdapter, D3DDEVTYPE_HAL, static_cast<D3DFORMAT>(graph->desktopFormat),
            static_cast<D3DFORMAT>(graph->backBufferFormat), depthCandidates[depthIndex]);
        if (hr == D3D_OK)
        {
            graph->depthFormat = static_cast<unsigned int>(depthCandidates[depthIndex]);
            break;
        }
    }
    if (depthIndex == 3u)
        graph->depthFormat = 0u;
    if (graph->depthFormat != D3DFMT_D24S8)
        graph->deviceState &= ~1u;

    flags = graph->capabilityFlags;
    graph->present.backBufferCount = 1u;
    graph->present.multiSampleType = D3DMULTISAMPLE_NONE;
    graph->present.multiSampleQuality = 0u;
    graph->present.swapEffect = D3DSWAPEFFECT_COPY;
    graph->present.deviceWindow = graph->window;
    graph->present.windowed = (flags&0x40u)?FALSE:TRUE;
    graph->present.enableAutoDepthStencil = TRUE;
    graph->present.autoDepthStencilFormat = graph->depthFormat;
    graph->present.flags = 0u;
    graph->present.fullScreenRefreshRate = 0u;
    graph->present.presentationInterval = (flags&0x80u)?D3DPRESENT_INTERVAL_ONE:D3DPRESENT_INTERVAL_IMMEDIATE;
    graph->present.backBufferWidth = static_cast<unsigned int>(TruncateFloatToInt32(graph->screenWidth));
    graph->present.backBufferHeight = static_cast<unsigned int>(TruncateFloatToInt32(graph->screenHeight));

    GraphLogSelectedMode(graph);

    unsigned int behavior = (graph->capabilityFlags&0x10u)?0x20u:0x40u;
    int deviceHr = static_cast<int>(graph->direct3D->CreateDevice(
        graph->selectedAdapter, D3DDEVTYPE_HAL, reinterpret_cast<HWND>(graph->window), behavior,
        reinterpret_cast<D3DPRESENT_PARAMETERS*>(&graph->present), &graph->device));
    if (deviceHr != D3D_OK)
    {
        graph->capabilityFlags |= 0x10u;
        deviceHr = static_cast<int>(graph->direct3D->CreateDevice(
            graph->selectedAdapter, D3DDEVTYPE_HAL, reinterpret_cast<HWND>(graph->window), 0x20u,
            reinterpret_cast<D3DPRESENT_PARAMETERS*>(&graph->present), &graph->device));
    }
    if (deviceHr != D3D_OK)
    {
        char subject[64];
        sprintf(subject, "3dDevice%u", graph->selectedAdapter);
        GraphDiagnostic(E_CREATE, STRING(subject), deviceHr);
        return 1;
    }

    STRING effectSubject;
    int effectHr = GraphCreatePseudo3DEffect(graph, pseudoEffectPath, true, effectSubject);
    if (effectHr<0)
    {
        STRING fallbackSubject;
        effectHr = GraphCreatePseudo3DEffect(graph, pseudoEffectFileName, false, fallbackSubject);
        if (effectHr<0)
            return 1;
    }

    D3DCAPS9 caps = {};
    const int capsHr = static_cast<int>(graph->device->GetDeviceCaps(&caps));
    if (capsHr != 0)
    {
        GraphDiagnosticLiteral(E_GET, "Caps", capsHr);
        return 1;
    }

    unsigned int capabilityBits = ((caps.TextureOpCaps&D3DTEXOPCAPS_MODULATE2X)<<1u)|(caps.StencilCaps&D3DSTENCILCAPS_TWOSIDED);
    capabilityBits = (capabilityBits<<1u)|(caps.PrimitiveMiscCaps&D3DPMISCCAPS_COLORWRITEENABLE);
    const unsigned int secondary = ((caps.TextureOpCaps>>4u)&0x80000u)|(caps.DevCaps&D3DDEVCAPS_TEXTURENONLOCALVIDMEM);
    capabilityBits = (capabilityBits<<4u)|(secondary>>10u);
    graph->capabilityFlags = (graph->capabilityFlags&0xFFFFD1FBu)|capabilityBits;
    graph->vertexShaderVersion = caps.VertexShaderVersion&0xFFFFu;
    graph->pixelShaderVersion = caps.PixelShaderVersion&0xFFFFu;
    graph->maxStreams = (caps.DevCaps2&D3DDEVCAPS2_STREAMOFFSET)?caps.MaxStreams:1u;
    graph->maxLights = caps.MaxActiveLights;

    TEXTURE::SetDeviceCaps(graph->device);
    graph->builtPixelFormat = D3DFMT_R5G6B5;
    {
        STRING built = GetPixelFormat(static_cast<D3DFORMAT>(graph->builtPixelFormat));
        if (Error)
            Error->Log("BuildedPixelFormat = %s", built.value);
    }

    (void)graph->RestoreDeviceObjects();
    GraphLogTextureStage();
    graph->SetDefaultState();
    GraphLogTextureStage();

    TEXTURE* const memory = static_cast<TEXTURE*>(CrtNew(36u));
    graph->hiBuffer = memory?new(memory) TEXTURE(256, 256, D3DFMT_R5G6B5, 0u):0;
    if (graph->hiBuffer->m_texture == 0 && graph->hiBuffer->m_cpuData == 0)
    {
        STRING subject("hiBuffer");
        GraphDiagnostic(E_CREATE, subject, 0);
        return 1;
    }

    COLOR clearColor;
    clearColor.value = 0xFF000000u;
    graph->ClearScreen(clearColor);
    graph->SetFont(STRING("Courier"), 7, 8);
    RGB16::SetChannelMasks(graph->hiBuffer->m_format == D3DFMT_R5G6B5);
    GraphBuildCapsText(graph);
    return 0;
}

int GRAPH::SetGraphDetail(unsigned int requestedDetail)
{
    GRAPH* const graph = this;
    if (requestedDetail == graph->graphDetail)
        return 0;

    const unsigned int restoreSpecial = graph->sceneState&1u;
    if (restoreSpecial)
        (void)graph->PostTact(1);
    graph->graphDetail = 0xFFu;

    unsigned int detail = requestedDetail;
    unsigned int capability = TEXTURE::capsCondSizePow2;
    if (detail == 8u &&
       graph->vertexShaderVersion >= 0x100u &&
       graph->pixelShaderVersion >= 0x100u &&
       (graph->capabilityFlags&0x800u) != 0u &&
       graph->present.autoDepthStencilFormat == D3DFMT_D24S8 && capability != 0u)
    {
        (void)::Error->Log("GRAPH::SetGraphDetail: HIGH");
        graph->graphDetail = 8u;
    }
    else
    {
        for (;;)
        {
            if (detail <= 9u)
            {
                switch (detail)
                {
                case 0u:
                    (void)::Error->Log("GRAPH::SetGraphDetail: LOWEST");
                    graph->graphDetail = 0u;
                    break;
                case 3u:
                    (void)::Error->Log("GRAPH::SetGraphDetail: LOW");
                    graph->graphDetail = 3u;
                    break;
                case 5u:
                    if (graph->present.autoDepthStencilFormat == D3DFMT_D24S8 && capability != 0u)
                    {
                        (void)::Error->Log("GRAPH::SetGraphDetail: MEDIUM");
                        graph->graphDetail = 5u;
                    }
                    break;
                case 9u:
                    if (graph->vertexShaderVersion >= 0x100u &&
                       graph->pixelShaderVersion >= 0x200u &&
                       (graph->capabilityFlags&0x800u) != 0u &&
                       graph->present.autoDepthStencilFormat == D3DFMT_D24S8 && capability != 0u)
                    {
                        (void)::Error->Log("GRAPH::SetGraphDetail: HIGHEST");
                        graph->graphDetail = 9u;
                    }
                    break;
                default:
                    break;
                }
            }

            if (graph->graphDetail != 0xFFu)
                break;
            capability = TEXTURE::capsCondSizePow2;
            --detail;
        }
    }

    (void)graph->InvalidateDeviceObjects();
    (void)graph->RestoreDeviceObjects();
    if (graph->graphDetail>0u && graph->lightRenderTexture == 0)
    {
        (void)::Error->Log("GRAPH::SetGraphDetail: LOWEST");
        graph->graphDetail = 0u;
    }
    if (restoreSpecial)
        (void)graph->PreTact();
    return 0;
}

void GRAPH::PutsXY(const VECTOR2& position, const char* text, COLOR color)
{
    PutsXY(position.x, position.y, text, color);
}

void GRAPH::UpdateStartupDialog(DIALOG_COMBO_BOX* adapterCombo, DIALOG_COMBO_BOX* resolutionCombo, DIALOG_BUTTON* capabilityOptionButton)
{
    if (GraphDialogSend(adapterCombo, CB_GETCOUNT, 0u, 0u) != 0u)
    {
        const unsigned int selection = GraphDialogSend(adapterCombo, CB_GETCURSEL, 0u, 0u);
        selectedAdapter = GraphDialogSend(adapterCombo, CB_GETITEMDATA, selection, 0u);
    }
    else
    {
        if (capabilityOptionButton)
            (void)GraphDialogSend(capabilityOptionButton, BM_SETCHECK, (capabilityFlags>>6u)&1u, 0u);
        (void)GraphDialogSend(adapterCombo, CB_RESETCONTENT, 0u, 0u);
        for (int adapter = 0;adapter<adapterCount;++adapter)
        {
            unsigned int item;
            {
                const char* const description = adapters[adapter].description;
                STRING label(description);
                item = GraphDialogSend(adapterCombo, CB_ADDSTRING, 0u, PtrToU32(label.m_value));
                if (item != 0xFFFFFFFFu && item != 0xFFFFFFFEu)
                    (void)GraphDialogSend(adapterCombo, CB_SETITEMDATA, item, static_cast<unsigned int>(adapter));
            }
            if (static_cast<unsigned int>(adapter) == selectedAdapter)
                (void)GraphDialogSend(adapterCombo, CB_SETCURSEL, item, 0u);
        }
    }

    if (GraphDialogSend(resolutionCombo, CB_GETCOUNT, 0u, 0u) != 0u)
    {
        const unsigned int selection = GraphDialogSend(resolutionCombo, CB_GETCURSEL, 0u, 0u);
        const unsigned int packed = GraphDialogSend(resolutionCombo, CB_GETITEMDATA, selection, 0u);
        screenWidth = static_cast<float>(packed&0x7FFFu);
        screenHeight = static_cast<float>(BitsToSigned32(packed)>>16);
        capabilityFlags = (capabilityFlags&~2u)|((packed>>14u)&2u);
    }

    const int desiredBits = (capabilityFlags&2u) != 0u?32:16;
    const int height = TruncateFloatToInt32(screenHeight);
    const int width = TruncateFloatToInt32(screenWidth);
    if (adapters[selectedAdapter].GetMode(width, height, desiredBits)<0)
    {
        screenWidth = static_cast<float>(BitsToSigned32(adapters[selectedAdapter].width[0]));
        screenHeight = static_cast<float>(BitsToSigned32(adapters[selectedAdapter].height[0]));
        const unsigned int bit = GraphDisplayFormatBits(adapters[selectedAdapter].format[0]) == 32?2u:0u;
        capabilityFlags = (capabilityFlags&~2u)|bit;
    }

    if (capabilityOptionButton)
    {
        bool fullscreenAvailable = false;
        if ((adapters[selectedAdapter].flags&1u) != 0u &&
           ((capabilityFlags>>1u)&1u) == (GraphDisplayFormatBits(adapters[selectedAdapter].desktopFormat) == 32?1u:0u))
        {
            const int desktopWidth = ::GetSystemMetrics(SM_CXSCREEN);
            if (static_cast<float>(desktopWidth) >= screenWidth)
            {
                const int desktopHeight = ::GetSystemMetrics(SM_CYSCREEN);
                fullscreenAvailable = static_cast<float>(desktopHeight) >= screenHeight;
            }
        }
        if (!fullscreenAvailable)
            (void)GraphDialogSend(capabilityOptionButton, BM_SETCHECK, 1u, 0u);
        const GraphDialogItemRef* const ref = reinterpret_cast<const GraphDialogItemRef*>(capabilityOptionButton);
        (void)::EnableWindow(::GetDlgItem(ref->window, static_cast<int>(ref->controlId)), fullscreenAvailable?TRUE:FALSE);
        const bool checked = GraphDialogSend(capabilityOptionButton, BM_GETCHECK, 0u, 0u) == 1u;
        capabilityFlags = (capabilityFlags&~0x40u)|(checked?0x40u:0u);
    }

    (void)GraphDialogSend(resolutionCombo, CB_RESETCONTENT, 0u, 0u);
    for (int modeIndex = 0;modeIndex<adapters[selectedAdapter].modeCount;++modeIndex)
    {
        unsigned int item;
        {
            const unsigned int index = static_cast<unsigned int>(modeIndex);
            DD_DRIVER* const record = &adapters[selectedAdapter];
            const int bits = GraphDisplayFormatBits(record->format[index]);
            const unsigned int packed = record->width[index]|(record->height[index]<<16u)|(bits == 32?0x8000u:0u);
            STRING label = record->GetModeDesctription(static_cast<int>(index));
            item = GraphDialogSend(resolutionCombo, CB_ADDSTRING, 0u, PtrToU32(label.m_value));
            if (item != 0xFFFFFFFFu && item != 0xFFFFFFFEu)
                (void)GraphDialogSend(resolutionCombo, CB_SETITEMDATA, item, packed);
        }
        const DD_DRIVER* const record = &adapters[selectedAdapter];
        const unsigned int index = static_cast<unsigned int>(modeIndex);
        const float modeWidth = static_cast<float>(BitsToSigned32(record->width[index]));
        const float modeHeight = static_cast<float>(BitsToSigned32(record->height[index]));
        if (!(modeWidth<screenWidth) && !(modeWidth>screenWidth) &&
           !(modeHeight<screenHeight) && !(modeHeight>screenHeight) &&
           ((capabilityFlags>>1u)&1u) == (GraphDisplayFormatBits(record->format[index]) == 32?1u:0u))
            (void)GraphDialogSend(resolutionCombo, CB_SETCURSEL, item, 0u);
    }
}

void GRAPH::ReLoadPalettes()
{
    (void)GraphReportStaticString();
}

void GRAPH::DrawShadowToStencil(const VECTOR& position, const VECTOR2& leftTop, const VECTOR2& rightBottom)
{


    if ((*reinterpret_cast<unsigned char*>(&deviceState)&1u) == 0u || graphDetail<5u)
        return;

    if (runtimeE18 == 0u)
    {
        runtimeE18 = 1u;
        (void)device->Clear(0u, 0, D3DCLEAR_STENCIL, 0u, 0.0f, 0u);
    }

    const VECTOR2 highWorld(position.x+leftTop.x, position.y-position.z+leftTop.y);
    const VECTOR2 highScreen = Map->ToScreenScaled(highWorld);
    const float highY = !(highScreen.y >= viewBottom)?highScreen.y:viewBottom;
    const float highX = !(highScreen.x >= viewRight)?highScreen.x:viewRight;
    const VECTOR2 lowWorld(position.x-leftTop.x, position.y-position.z-leftTop.y);
    const VECTOR2 lowScreen = Map->ToScreenScaled(lowWorld);
    const float lowY = lowScreen.y>viewTop?lowScreen.y:viewTop;
    const float lowX = lowScreen.x>viewLeft?lowScreen.x:viewLeft;
    int scissor[4];
    scissor[0] = TruncateFloatToInt32(lowX);
    scissor[2] = TruncateFloatToInt32(highX);
    scissor[1] = TruncateFloatToInt32(lowY);
    scissor[3] = TruncateFloatToInt32(highY);

    const int scissorHr = static_cast<int>(device->SetScissorRect(
        reinterpret_cast<const RECT*>(scissor)));
    if (scissorHr<0)
    {
        const STRING subject("scissor rect for shadow");
        if (::Error)
        {
            const STRING object("GRAPH");
            ::Error->Error(object, E_SET, subject, static_cast<unsigned long>(scissorHr));
        }
    }

    const auto renderState = [&](unsigned int state, unsigned int value)
    {
        (void)device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(state), value);
    };
    const auto textureStageState = [&](unsigned int stage, unsigned int state, unsigned int value)
    {
        (void)device->SetTextureStageState(stage, static_cast<D3DTEXTURESTAGESTATETYPE>(state), value);
    };
    const auto samplerState = [&](unsigned int stage, unsigned int state, unsigned int value)
    {
        (void)device->SetSamplerState(stage, static_cast<D3DSAMPLERSTATETYPE>(state), value);
    };

    renderState(D3DRS_SCISSORTESTENABLE, 1u);
    renderState(D3DRS_STENCILENABLE, 1u);
    renderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    renderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
    renderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    const unsigned int stencilRef = runtimeE18;
    runtimeE18 = static_cast<unsigned char>(stencilRef+1u);
    renderState(D3DRS_STENCILREF, stencilRef);
    renderState(D3DRS_STENCILMASK, 0xFFFFFFFFu);
    renderState(D3DRS_STENCILWRITEMASK, 0xFFFFFFFFu);
    renderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
    renderState(D3DRS_COLORWRITEENABLE, 0u);
    textureStageState(0u, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    textureStageState(0u, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    textureStageState(0u, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    samplerState(0u, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    samplerState(0u, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    renderState(D3DRS_ALPHABLENDENABLE, 0u);
    renderState(D3DRS_ALPHATESTENABLE, 1u);
    renderState(D3DRS_ALPHAREF, 0u);
    renderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    renderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
    renderState(D3DRS_ZWRITEENABLE, 0u);
    renderState(D3DRS_SPECULARENABLE, 0u);
    (void)device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

    SPRITE_COLLECTOR* const collector = ::Sprites;
    collector->DrawShadowInLayer(15, position, leftTop, rightBottom);

    renderState(D3DRS_SPECULARENABLE, 0u);
    renderState(D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL);
    renderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    renderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    renderState(D3DRS_SCISSORTESTENABLE, 0u);
}

void GRAPH::SetTime(unsigned long oldTime, unsigned long newTime)
{
    for (unsigned int effectIndex = 0;effectIndex<16u;++effectIndex)
        if (effectStart[effectIndex] != 0u)
            effectStart[effectIndex] = effectStart[effectIndex]-static_cast<unsigned int>(oldTime)+static_cast<unsigned int>(newTime);
}

void GRAPH::DrawEffect(int drawEffects)
{
    GRAPH* const graph = this;
    const unsigned int now = ::CurrentTime;


    unsigned int start = graph->effectStart[5];
    if (start != 0u)
    {
        const unsigned int duration = graph->effectDuration[5];
        const unsigned int elapsed = now-start;
        if (elapsed >= duration)
        {
            graph->effectStart[5] = 0u;
        }
        else
        {
            (void)graph->SetAlphaBlend(D3DBLEND_INVSRCALPHA, D3DBLEND_SRCALPHA);
            IDirect3DDevice9* device = graph->device;
            (void)device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
            device = graph->device;
            (void)device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
            const unsigned int alpha = (elapsed<<8)/duration;
            const float left = graph->viewLeft, top = graph->viewTop;
            const float right = graph->viewRight, bottom = graph->viewBottom;
            for (float y = top;y<bottom;y += 256.0f)
            {
                const float tileH = (y+256.0f<bottom)?256.0f:(bottom-y);
                for (float x = left;x<right;x += 256.0f)
                {
                    const float tileW = (x+256.0f<right)?256.0f:(right-x);
                    RECT destination = {
                        TruncateFloatToInt32(x-left), TruncateFloatToInt32(y-top),
                        TruncateFloatToInt32(x+tileW-left), TruncateFloatToInt32(y+tileH-top)};
                    RECT source = {0, 0, TruncateFloatToInt32(tileW), TruncateFloatToInt32(tileH)};
                    POINT origin = {0, 0};
                    if (drawEffects != 0u)
                    {
                        const unsigned int copyResult = static_cast<unsigned int>(graph->hiBuffer->CopyFromSurface(graph->depthSurface, &destination, &origin));
                        if (copyResult == 0u)
                        {
                            unsigned int color = 0u;
                            (void)GraphPackRGBA(&color, alpha, 255u, 255u, 255u);
                            GAMMA gamma;
                            gamma.modulate = ~color;
                            gamma.add = 0xFF000000u;
                            (void)graph->hiBuffer->Draw(&destination, &source, gamma);
                        }
                    }
                }
            }
        }
    }

    start = graph->effectStart[2];
    if (start != 0u)
    {
        const unsigned int elapsed = now-start;
        const unsigned int duration = graph->effectDuration[2];
        const float targetX = static_cast<float>(static_cast<int>(graph->effectWidth[2]));
        const float targetY = static_cast<float>(static_cast<int>(graph->effectHeight[2]));
        float point[2];
        if (elapsed>duration)
        {
            point[0] = targetX;
            point[1] = targetY;
            Map->SetShiftCoor(VECTOR2(point[0], point[1]), 0);
            graph->effectStart[2] = 0u;
        }
        else
        {
            const float t = static_cast<float>(elapsed)/static_cast<float>(duration);
            const float sx = graph->effectCenterX, sy = graph->effectCenterY;
            point[0] = (targetX-sx)*t+sx;
            point[1] = (targetY-sy)*t+sy;
            Map->SetShiftCoor(VECTOR2(point[0], point[1]), 0);
        }
    }


    start = graph->effectStart[1];
    if (start != 0u)
    {
        const unsigned int duration = graph->effectDuration[1];
        const unsigned int elapsed = now-start;
        if (elapsed >= duration)
        {
            graph->effectStart[1] = 0u;
        }
        else if (drawEffects != 0u)
        {
            const unsigned int ninth = duration/9u;
            const unsigned int scale = (elapsed<ninth)
                ? (((elapsed*9u)<<8)/duration)
                : (((duration-elapsed)<<8)/(duration-ninth));
            const unsigned int color = GraphScaleRgb(graph->effectWidth[1], scale);
            for (unsigned int i = 0;i<3u;++i)
                LightBar(VECTOR2(0.0f, 0.0f), *reinterpret_cast<const VECTOR2*>(&screenWidth), color);
        }
    }


    start = graph->effectStart[3];
    if (start != 0u)
    {
        const unsigned int duration = graph->effectDuration[3];
        const unsigned int elapsed = now-start;
        if (elapsed >= duration)
        {
            graph->effectStart[3] = 0u;
        }
        else if (drawEffects != 0u)
        {
            const unsigned int riseEnd = (duration*4u)/9u;
            const unsigned int plateauEnd = (duration*5u)/9u;
            unsigned int level = 255u;
            if (elapsed<riseEnd)
                level = (2304u*elapsed)/(duration*4u);
            else if (elapsed>plateauEnd)
                level = (2304u*(duration-elapsed))/(duration*4u);
            GraphDrawGrayOverlay(this, level);
        }
    }


    start = graph->effectStart[9];
    if (start != 0u)
    {
        const unsigned int duration = graph->effectDuration[9];
        const unsigned int elapsed = now-start;
        if (elapsed >= duration)
        {
            graph->effectStart[9] = 0u;
        }
        else if (drawEffects != 0u)
        {
            const unsigned int fourFifths = (duration*4u)/5u;
            const unsigned int level = (elapsed >= fourFifths)?255u:(1280u*elapsed)/(duration*4u);
            GraphDrawGrayOverlay(this, level);
        }
    }

    start = graph->effectStart[10];
    if (start != 0u)
    {
        const unsigned int duration = graph->effectDuration[10];
        const unsigned int elapsed = now-start;
        if (elapsed >= duration)
        {
            graph->effectStart[10] = 0u;
        }
        else if (drawEffects != 0u)
        {
            const unsigned int level = (255u*(duration-elapsed))/duration;
            GraphDrawGrayOverlay(this, level);
        }
    }

    start = graph->effectStart[11];
    if (start != 0u)
    {
        const unsigned int duration = graph->effectDuration[11];
        const unsigned int elapsed = now-start;
        const GAMMA target(static_cast<GAMMA::GAMMA_CREATE>(0), graph->effectWidth[11]);
        if (elapsed >= duration)
        {
            graph->SetGamma(target);
            graph->effectStart[11] = 0u;
        }
        else
        {
            const float t = static_cast<float>(static_cast<double>(elapsed)/BitsToSigned32(duration));
            GAMMA begin;
            begin.modulate = graph->effectGamma.modulate;
            begin.add = graph->effectGamma.add;
            int interpolatedChannels[4];
            const unsigned int shifts[4] = {24u, 16u, 8u, 0u};
            for (unsigned int i = 0;i<4u;++i)
            {
                const int startChannel = GraphGammaChannel(begin, shifts[i]);
                const int targetChannel = GraphGammaChannel(target, shifts[i]);
                const double delta = static_cast<double>(targetChannel-startChannel)*t;
                interpolatedChannels[i] = TruncateDoubleToInt32(static_cast<double>(startChannel)+
                    (i == 3u?static_cast<double>(static_cast<float>(delta)):delta));
            }
            const GAMMA interpolated(interpolatedChannels[0], interpolatedChannels[1], interpolatedChannels[2], interpolatedChannels[3]);
            graph->SetGamma(interpolated);
        }
    }

    start = graph->effectStart[12];
    if (start != 0u)
    {
        const unsigned int duration = graph->effectDuration[12];
        const unsigned int elapsed = now-start;
        const float target[3] = {
            static_cast<float>(static_cast<int>(graph->effectWidth[12]))*0.01f,
            static_cast<float>(static_cast<int>(graph->effectHeight[12]))*0.01f,
            static_cast<float>(static_cast<int>(graph->effectHeight[12]))*0.01f};
        if (elapsed >= duration)
        {
            (void)SetScreenScale(VECTOR(target[0], target[1], target[2]));
            SPRITE_COLLECTOR* const collector = ::Sprites;
            collector->RestoreDeviceObjects();
            graph->effectStart[12] = 0u;
            return;
        }
        const float t = static_cast<float>(elapsed)/static_cast<float>(duration);
        float scale[3];
        for (unsigned int i = 0;i<3u;++i)
        {
            const float begin = graph->effectScreenScale[i];
            scale[i] = (target[i]-begin)*t+begin;
        }
        (void)SetScreenScale(VECTOR(scale[0], scale[1], scale[2]));
        ::Sprites->RestoreDeviceObjects();
        return;
    }
    return;
}

int GRAPH::GetEffectState(int effect) const
{
    if (effect <= 0 || effect >= 16)
        return -1;
    const unsigned int start = effectStart[effect];
    if (start == 0u)
        return -1;
    const unsigned int numerator = ::CurrentTime*100u-start*100u;
    return BitsToSigned32(numerator/effectDuration[effect]);
}

void GRAPH::Box(const VECTOR2& leftTop, const VECTOR2& rightBottom, COLOR color) const
{
    Line(leftTop.x, leftTop.y, rightBottom.x, leftTop.y, color);
    Line(leftTop.x, rightBottom.y, rightBottom.x, rightBottom.y, color);
    Line(leftTop.x, leftTop.y, leftTop.x, rightBottom.y, color);
    Line(rightBottom.x, leftTop.y, rightBottom.x, rightBottom.y, color);
}

void GRAPH::Bar(const VECTOR2& leftTop, const VECTOR2& rightBottom, COLOR color)
{
    struct Vertex
    {
        float x, y, z, rhw;
        unsigned int diffuse, specular;
    };
    Vertex vertices[4] = {
        {
            leftTop.x, leftTop.y, 0.99999988079071044921875f, 1.0f, color.value, 0xFFFFFFFFu
        },
        {rightBottom.x, leftTop.y, 0.99999988079071044921875f, 1.0f, color.value, 0xFFFFFFFFu},
        {rightBottom.x, rightBottom.y, 0.99999988079071044921875f, 1.0f, color.value, 0xFFFFFFFFu},
        {leftTop.x, rightBottom.y, 0.99999988079071044921875f, 1.0f, color.value, 0xFFFFFFFFu}
    };
    (void)device->SetTexture(0u, 0);
    if ((color.value&0xFF000000u) == 0xFF000000u)
        (void)device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0u);
    else
        (void)SetAlphaBlend(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
    (void)device->SetRenderState(D3DRS_SPECULARENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ALPHATESTENABLE, 0u);
    (void)device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    (void)device->SetRenderState(D3DRS_ZWRITEENABLE, 0u);
    DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, vertices, sizeof(Vertex), 4);
}

void GRAPH::SetDrawMesh(unsigned int enabled)
{
    capabilityFlags = (capabilityFlags&~0x8000u)|((enabled&1u)<<15);
    if ((capabilityFlags&0x8000u) != 0u)
    {
        (void)device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
        return;
    }
    (void)device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    (void)device->SetTextureStageState(0u, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    (void)device->SetTextureStageState(0u, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
}

void* GraphCompilerPair00::StorePairAt00(int first, int second)
{
    m_dword00 = first;
    m_dword04 = second;
    return this;
}

float GRAPH::ViewYMin() const
{
    return viewTop;
}

const VECTOR2& GRAPH::ViewMin() const
{
    return *reinterpret_cast<const VECTOR2*>(&viewLeft);
}

VECTOR2 MAP::FromScreen(const VECTOR2& value) const
{
    return VECTOR2(m_shiftXY.x+value.x, m_shiftXY.y+value.y);
}

VECTOR2 VECTOR2::operator*(float scale) const
{
    return VECTOR2(x*scale, y*scale);
}


