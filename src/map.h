#pragma once
#include "basetype.h"
#include "myerror.h"
#include "mouse.h"
#include "list.h"
#include "mylib.h"
#include "groups.h"
#include "mousetips.h"
#include "input.h"
#include "script.h"
#include "resread.h"
#include "menu.h"

void __cdecl DispatchMapPresentation(int mode);

enum MAP_SCRIPT_EVENT
{
    MAP_EVENT_MAIN = 0,
    MAP_EVENT_TRAIN_NOT_AMMO = 1,
    MAP_EVENT_TRAIN_NOT_POWER = 2,
    MAP_EVENT_TRAIN_DAMAGE = 3,
    MAP_EVENT_TRAIN_CREATED = 4,
    MAP_EVENT_TRAIN_SPLIT = 5,
    MAP_EVENT_TRAIN_DESTROY = 6,
    MAP_EVENT_TRAIN_DESTROY_POWER = 7,
    MAP_EVENT_TRAIN_ARRIVE = 8,
    MAP_EVENT_TRAIN_NOT_ARRIVE = 9,
    MAP_EVENT_TRAIN_ATTACKED = 10,
    MAP_EVENT_DEPO_DESTROY = 11,
    MAP_EVENT_DEPO_BIRTH = 12,
    MAP_EVENT_DEPO_ATTACKED = 13,
    MAP_EVENT_DEPO_FREE = 14,
    MAP_EVENT_BUILDING_CAPTURE = 15,
    MAP_EVENT_MASTER_DESTROY = 16,
    MAP_EVENT_UNKNOWN = 17,
    MAP_EVENT_SUPER_WEAPON_WOUNDED = 18,
    MAP_EVENT_MINE_BLAST = 19,
    MAP_EVENT_MINE_REMOVE = 20,
    MAP_EVENT_ENEMY_LINKED = 21,
    MAP_EVENT_TRAIN_CLASH = 22,
    MAP_EVENT_UNIT_CREATED = 23,
    MAP_EVENT_UNIT_DESTROY = 24
};

extern int EvFunctionNumber[64];

class SPRITE;
class VID;
class STREAM;
class PLAYER;
class RESOURCE;
struct WEAPON_RUNTIME;
struct VECTOR;
struct VECTOR2;
struct HWND__;
class ANGLE;
struct HINSTANCE__;
struct GRAPH_INIT;

class RELATION
{
public:
    RELATION();
    ~RELATION();
    SPRITE* Decode(SPRITE* sprite) const;
    void Insert(SPRITE* first, SPRITE* second);
    void Release();

private:
    LIST<SPRITE*> m_first;
    LIST<SPRITE*> m_second;
};


class MAP
{
    friend class zShopDlgMenu;
    friend class MOUSE;
    friend class MESSAGE;
    friend class PLAYER_ARCADE;
    friend class PLAYER;
    friend class ENGINE;
    friend class SPRITE;
    friend class VID_SURFACE;
    friend class zHeroInvMenu;
    friend class zShopItemInfo;
    friend class zSpriteInventory;
    friend class zParamDlgMenu;
    friend class zCarControl;
    friend class REGION;
public:
    enum { NamedSpriteSlotCount = 10, PlayerSlotCount = 8, VidSlotCount = 4096 };

    MAP(HINSTANCE__* instance, HINSTANCE__* previous, const STRING& command, int show, GRAPH_INIT* graphInit);
    virtual ~MAP() noexcept(false);
    virtual int WorkWndMessage(HWND__* hwnd, unsigned long msg, unsigned long wParam, unsigned long lParam);
    virtual void DeletePointerToSprite(SPRITE* sprite);
    virtual int Tact();
    virtual void DrawSecondaryInfo();
    virtual void Release();
    virtual void Load(STRING name);
    virtual void Save(STRING name);
    virtual SPRITE* CreateSprite(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent);

    int ExecFunc(int command);
    float ToScreenX(float value) const;
    float ToScreenY(float value) const;
    VECTOR2 ToScreen(const VECTOR& value) const;
    VECTOR2 ToScreenScaled(const VECTOR2& value) const;
    VECTOR2 FromScreen(const VECTOR2& value) const;
    void SetShiftCoor(const VECTOR2& point, int mode);
    void SetScrollType(int mode, const VECTOR2& first, const VECTOR2& second);
    void ControlShiftCoor();
    void SetKeyboardLocale(const STRING& locale) const;
    STRING FileName() const;
    STRING WindowsUserPath() const;

    STRING OpenSaveDialog(int saveDialog, const char* filter, const char* defaultExtension) const;
    STRING OpenDialog(const char* filter, const char* defaultExtension) const;
    SPRITE* ReadPointer(STREAM* inputStream) const;
    SPRITE* OldLoadSprite(STREAM* inputStream);
    SPRITE* LoadSprite(STREAM* inputStream, int version);
    int ScriptRun(int function, const SPRITE* spriteA, const SPRITE* spriteB, int integerArgument);
    int ScriptEventRun(int eventId, const SPRITE* sprite, int integerArgument1, int integerArgument2);
    int ScriptRun(int function, const char* script, int scriptArgument1, int scriptArgument2, int scriptArgument3);
    int ScriptRun(const STRING& script, const char* name, int scriptArgument1, int scriptArgument2, int scriptArgument3);
    int IsDotUnderGround(const VECTOR& point) const;
    int IsLineUnderGround(const VECTOR& from, const VECTOR& to) const;
    float GetGroundZ(const VECTOR2& point) const;
    float GetGroundZ(const VID* vid, const VECTOR2& point, ANGLE direction) const;
    int CanPlaceInGridZ(const VID* vid, const VECTOR& point, ANGLE direction) const;
    float GetGroundZScr(VECTOR2 point) const;
    void PauseOn();
    void PauseOff();
    void ResetGroundZ();
    void SetGroundZ(const VECTOR& point);
    void SetTempGroundZ(const VECTOR& point);
    void ClearTempGroundZ(const VECTOR& point);
    int ValidateVid(int index) const;
    void DeleteExtraVid();
    void LoadVid(RESOURCE* resource);
    VID* CreateVid(RESOURCE* resource, int nVid);
    void SetFlagman(unsigned int index, SPRITE* sprite) const;
    SPRITE* Flagman(unsigned int index) const;
    SPRITE* SpriteUnderCursor() const;
    STRING GetMouseTipsString() const;
    STRING ScriptVariable(const STRING& expression);
    void SetScriptVariable(const STRING& name, STRING value);
    void RestoreDeviceObjects();
    void InvalidateDeviceObjects();
    int IsNetGameStart() const;
    void CopyVid(VID* dst, VID* src);
    void ExchangeVid(VID* first, VID* second);
    void ReloadVid();
    float ToScreenY(float lhs, float rhs) const;
    const VECTOR2& SizeXY() const;
    PLAYER* Player(int index) const;
    PLAYER* Player() const;
    VID* Vid(int index) const;
    const VECTOR2& ShiftXY() const
    {
        return m_shiftXY;
    }
    INPUT* InputState()
    {
        return &m_input;
    }
    const INPUT* InputState() const
    {
        return &m_input;
    }
    VECTOR2 ClientMouseXY() const
    {
        return m_input.m_clientMouseXY;
    }
    unsigned int State00C(unsigned int index) const
    {
        return m_spriteCountsByNameNumber[index];
    }
    SPRITE* MenuSelectedSprite() const
    {
        return m_menu.SelectedSprite();
    }
    unsigned int Flags() const
    {
        return m_flags;
    }
    void SetNetworkMapName(const STRING& name)
    {
        m_flags |= 0x80u;
        m_pendingMapFileName = name;
    }
    unsigned int DialogHandle() const
    {
        return m_dialog;
    }
    unsigned int PlayerIndex() const
    {
        return m_playerIndex;
    }
    MENU& MenuState()
    {
        return m_menu;
    }
    const MENU& MenuState() const
    {
        return m_menu;
    }
    int VidCount() const
    {
        return m_vidCount;
    }
    VID* StoredVid(int index) const
    {
        return (index >= 0 && index<m_vidCount)?m_vids[static_cast<unsigned int>(index)]:0;
    }

protected:
    int DemoTact();
    int StartTact();
    void SetupProcessorFlags();
    void NetworkTact();
    int PopInt();
    const STRING& PopStr();
    VID* PopVid(const char* diagnosticContext);
    void PushInt(int value);
    void PushStr(const STRING& value);
    void PushSprite(SPRITE* sprite, const STRING& tag);
    void Error(TYPE_ERROR type, const STRING& subject, unsigned long code);

private:
    void UpdateGroundHeightCell(const VECTOR& point, bool temporary, bool clearOnEqual);
    void ScanGroundHeightRect(float minX, float minY, float maxX, float maxY, int& highest, int& accepted) const;
    int CanPlaceGroundHeightRect(const VID* vid, const VECTOR& position, float minX, float minY, float maxX, float maxY, bool steppedHeight) const;
    void LoadWeapon(RESOURCE* resource);

protected:

    unsigned int m_frameRate04;
    unsigned int m_frameCounter08;
    unsigned int m_spriteCountsByNameNumber[NamedSpriteSlotCount];
    unsigned char* m_fastLoadIndex;
    unsigned char* m_fastLoadData;
    unsigned int m_flags;
    float m_scale40;
    STRING m_windowTitle;
    STRING m_windowsUserPath;
    STRING m_currentMapFileName;
    STRING m_pendingMapFileName;
    STRING m_previousMapFileName;
    STRING m_resourceFileName;
    unsigned int m_simulationFrameCount;
    unsigned int m_tick60;
    VECTOR2 m_sizeXY;
    unsigned int m_scrollMode;
    float m_scrollMouseMinX;
    float m_scrollMouseMinY;
    float m_scrollMouseMaxX;
    float m_scrollMouseMaxY;
    float m_scrollFollowSpeedScale;
    float m_cameraMinX;
    float m_cameraMaxX;
    float m_cameraMinY;
    float m_cameraMaxY;
    float m_velocityX;
    float m_velocityY;
    VECTOR2 m_shiftXY;
    INPUT m_input;
    unsigned char m_0CB;
    SCRIPT m_script;
    RESOURCE m_resource;
    RELATION m_relation;
    short* m_groundPrimary;
    short* m_groundSecondary;
    int m_groundWidth;
    int m_groundHeight;
    void* m_hInstance;
    unsigned int m_dialog;
    unsigned int m_dialogWindow;
    void* m_comObject;
    unsigned int m_playerIndex;
    PLAYER* m_players[PlayerSlotCount];
    unsigned int m_pendingNetworkPauseCommand;
    MENU m_menu;
    GROUPS m_groups;
    int m_weaponCount;
    WEAPON_RUNTIME* m_weaponData;
    int m_vidCount;
    VID* m_vids[VidSlotCount];
    MOUSETIPS m_mouseTips;
};



class MAP_STEAM : public MAP
{
public:
    MAP_STEAM(HINSTANCE__* instance, HINSTANCE__* previous, const STRING& command, int show, GRAPH_INIT* graphInit);
    ~MAP_STEAM() noexcept(false) override;
    int Tact() override;
    void DrawSecondaryInfo() override;
    void Release() override;
    SPRITE* CreateSprite(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent) override;

private:
    static void MapSteamAssignMapFileName(MAP_STEAM*);
    static void MapSteamProcessDebugHotkey(MAP_STEAM*, INPUT*);
    static bool MapSteamFrameMayContinue(MAP_STEAM*, INPUT*, bool);
    static void MapSteamApplyIndexedGamma(MAP_STEAM*);
    static void MapSteamTactControllers(MAP_STEAM*);
    static void MapSteamTactObjects(MAP_STEAM*);

protected:
    unsigned int m_debugFlags;
};


extern MAP* Map;

extern MOUSE* Mouse;

void __cdecl MapStaticCleanupNoOp();
void __cdecl MapCompilerCleanupNoOp();
void __stdcall MsvcNoOpDestroyRange(unsigned int, unsigned int);


