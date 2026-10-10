
#pragma once
#include <stddef.h>
#include "mylib.h"
#include "basetype.h"
#include "myerror.h"
#include "weapon_runtime.h"
class RESOURCE;
class GAMMA;
class SPRITE;
class COLOR;
struct CADR_DATA;
class TEXTURE;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;
struct ID3DXFont;
struct IUnknown;
enum _D3DFORMAT : int;
void __cdecl VidStaticRangeAInit();
void __cdecl VidStaticRangeBInit();
unsigned int Vid_StaticCleanupRegister01();

class VID
{
public:
    VID();

    virtual VID* CreateMirror();
    virtual ~VID() noexcept(false);
    virtual void Draw(SPRITE* sprite);
    virtual void DrawShadow(const SPRITE* sprite, const VECTOR& offset);
    virtual void DrawOnlyZ(const SPRITE* sprite);
    virtual void InvalidateDeviceObjects();
    virtual void RestoreDeviceObjects();
    virtual void Load(RESOURCE* resource);
    virtual void SetReColorForArmy(COLOR color);

    virtual void SetFramesToDevice();
    virtual void SetLayer();

    void ResetSprites();
    void Error(TYPE_ERROR type, const STRING& subject, unsigned long code) const;
    void SetChildAndLink();
    unsigned int CalcImportantForSync();
    void LoadParameters(RESOURCE* resource);
    void SetGamma(const GAMMA& gamma, int slot);
    void SetGridZ(const SPRITE* sprite);
    void ResetGridZ(const SPRITE* sprite);
    float CalculateZSpeed(float numerator, float denominator) const;
    void SetPropHide(int hide);
    void SetPropHideWithoutLink(int hide);
    int GetFireDamageMax() const;
    int GetBuildTime() const;
    int GetMaxAmmo() const;
    STRING GetNumberName() const;
    void SetHpCoeff(unsigned int direction, int hpCoeff);
    void SetMaxHp(unsigned int direction, int hp);

    int RealDirection(unsigned char direction);
    ANGLE SteppedDirection(ANGLE direction) const;
    int RealDirectionWithGraphDetail(unsigned char direction);


    int PropHide() const;
    int CanFight() const;
    int GetMaxHp(unsigned int player) const;
    int NoSprites(int player) const;
    int NoSprites() const;
    int Killed(unsigned int player) const;
    int Killed() const;
    int ReCaptured(unsigned int player) const;
    int ReCaptured() const;
    int PropNotCreateAsChild() const;
    void SetPropNotCreateAsChild(int value);
    int PropBirthAsSmoke() const;

    enum { AnimationSlotCount = 17, PlayerSlotCount = 8, ScriptFunctionCount = 21 };

    static VECTOR2 viewMin;
    static VECTOR2 viewMax;
    static int MemoryInUse;
    static unsigned int useTextureMemory;
    static unsigned int useVertexMemory;
    static int optDrawFlagmanBehindWalls;
    static CADR_DATA* lastUse;

    int          nVid;
    STRING       name;
    unsigned int spriteType;
    unsigned int spriteClass;
    unsigned int property;
    unsigned int moveMask;
    VECTOR       sizeXYZ;
    int          maxHp;
    float        maxSpeed;
    float        randomSpeed;
    float        maxZSpeed;
    float        randomZSpeed;
    float        acceleration;
    float        slow;
    float        rotationSpeed;
    int          nWeapon;
    float        deathRange;
    int          deathDamageMin;
    int          deathDamageMax;
    float        deathPush;
    VECTOR       linkXYZ;
    int          nLinkVid;
    VID*         linkVid;
    float        topZ;
    float        forMoveUpZ;
    float        forMoveDownZ;
    int          lifeTime;
    int          noDir;
    int          noAnimCadr[AnimationSlotCount];
    int          sfx[AnimationSlotCount];
    int          frameSpeed[AnimationSlotCount];
    float        childX[AnimationSlotCount];
    float        childY[AnimationSlotCount];
    float        childZ[AnimationSlotCount];
    int          nChildVid[AnimationSlotCount];
    VID*         childVid[AnimationSlotCount];
    int          noChild[AnimationSlotCount];
    GAMMA gammaRaw;
    VECTOR       scaleXYZ;
    STRING       vidName;
    unsigned short type;
    unsigned short frameSpeedDefault;
    short        noCadr;
    short        vidSizeX;
    short        vidSizeY;
    short        reserved30A;
    float        buildSizeX;
    float        buildSizeY;
    VECTOR*      cadrLinkXYZ;
    int          animationBaseFrame[AnimationSlotCount];
    int          animationFrameCount[AnimationSlotCount];
    VECTOR2      halfSizeXY;
    VECTOR       realSizeXYZ;
    int          layer;
    int          directionQuantizationOffset;
    int          spriteLimit;
    int          spriteLimitByPlayer[PlayerSlotCount];
    unsigned int spriteCountsByPlayer[PlayerSlotCount];
    int          killedByPlayer[PlayerSlotCount];
    int          reCapturedByPlayer[PlayerSlotCount];
    int          maxHpByPlayer[PlayerSlotCount];
    GAMMA gammaByPlayer[PlayerSlotCount];
    int          scriptFunction[ScriptFunctionCount];
    unsigned int lastSpriteCountChangeTick;
    unsigned int lastRenderUseTick;
    WEAPON_RUNTIME* weapon;
    VID*         nextMirror;
    VID*         exchangedVid;
    VID*         sourceVid;
    float        minXGridZ;
    float        maxXGridZ;
    float        minYGridZ;
    float        maxYGridZ;
    int          noGridZ;
    VECTOR*      gridZ;
    int*         gridCadrShift;
    int          movementTactEnabled;
    int          actionAuxStateRequired;
    unsigned int runtimeFlags;
    int          notCreateAsChild;
    unsigned int storageFlags;
    unsigned int runtimeBackend;
};


class VID_SOFTWARE : public VID
{
public:
    VID_SOFTWARE();
    VID_SOFTWARE(VID_SOFTWARE& source);
    virtual ~VID_SOFTWARE();
    virtual void Load(RESOURCE* resource);


    virtual void DrawShadow(const SPRITE* sprite);
private:
    virtual VID* CreateMirror();

public:
    unsigned int*  frameOffsets;
    unsigned int   allocatedBytes;
    unsigned char* mappedFileBase;
    unsigned char  paletteTables[9216];
    unsigned char* mappedPalette;
};

class VID_LIGHT : public VID
{
public:
    VID_LIGHT();
    VID_LIGHT(VID_LIGHT& source);
    virtual ~VID_LIGHT();
    virtual void Load(RESOURCE* resource);
    virtual void SetLayer();
    virtual void Draw(SPRITE* sprite);
private:

    virtual VID* CreateMirror();
public:

    unsigned int lightDataBytes;
    void*        lightData;
};

template<class T> class LIST;
class VID_FONT : public VID
{
public:
    VID_FONT(VID* source, const STRING& name);
    VID_FONT(VID_FONT& source);
    virtual ~VID_FONT();
    virtual void InvalidateDeviceObjects();
    virtual void RestoreDeviceObjects();
    virtual void SetLayer();
    virtual void Draw(SPRITE* sprite);
    void DrawTextA(int x, int y, const STRING& text, int textLength, int firstLine, int lastLine,
                   COLOR color, const LIST<int>& lineOffsets) const;
    void CalcTextSize(const STRING& text, int* outMaxWidth, int* outHeight, int* outLineCount,
                      LIST<int>* lineOffsets, unsigned long alignFlags) const;
private:
    virtual VID* CreateMirror();

public:
    ID3DXFont* fontBackend;
};

class VID_SURFACE : public VID
{
public:
    static unsigned int noObj;
    static unsigned int noVertex;
    static unsigned int noTri;
    static unsigned int noDrawShadow;
    static unsigned int noDrawReal[16];
    static unsigned int noDrawCall[16];
    VID_SURFACE();
    VID_SURFACE(VID* source, RESOURCE* resource, const STRING& name);
    virtual ~VID_SURFACE();
    void CopyGraphFromVid(VID_SURFACE* source);
    void CopyFromVid(VID_SURFACE* source);
    virtual void Load(RESOURCE* resource);
    virtual void SetReColorForArmy(COLOR color);
    virtual void InvalidateDeviceObjects();
    virtual void RestoreDeviceObjects();
    virtual void Draw(SPRITE* sprite);
    virtual void DrawOnlyZ(const SPRITE* sprite);
    virtual void DrawShadow(const SPRITE* sprite, const VECTOR& offset);
    virtual void SetFramesToDevice();
    virtual void SetLayer();
private:
    virtual VID* CreateMirror();
    void ReleaseVideoMemory();

public:
    _D3DFORMAT   pixelFormat;
    int           surfaceMode;
    unsigned int  renderMode;
    unsigned char* surfaceData;
    unsigned int* palette;
    unsigned int* recolorPalette;
    unsigned int  surfaceFlags;
    CADR_DATA*    frames;
    IUnknown*     surfaceBackend;
};



struct CADR_DATA
{
    unsigned int lastUseTick;
    CADR_DATA* lruPrevious;
    CADR_DATA* lruNext;
    unsigned short indexCount;
    unsigned short vertexCount;
    unsigned short width;
    unsigned short height;
    unsigned short offsetX;
    unsigned short offsetY;
    unsigned int frameDataOffset;
    float maxZ;
    float minZ;
    TEXTURE* textureObject;
    IDirect3DVertexBuffer9* vertexBuffer;
    IDirect3DIndexBuffer9* indexBuffer;

    CADR_DATA();
    ~CADR_DATA() noexcept(false);
    void SetTextureToDevice(VID_SURFACE* surface, int finalizeArg);
    void CalcMaxMinZ(const VID_SURFACE* surface);
    void SetVertexToDevice(VID_SURFACE* surface);
    void ReleaseVideoMemory();
};



void* __cdecl InitializeVidRootAtStartup();
void* __cdecl VidInitializeGlobalRoot();
