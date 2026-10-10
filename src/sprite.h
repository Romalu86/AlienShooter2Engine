#pragma once
#include "basetype.h"
#include "list.h"
#include "myerror.h"
#include <stddef.h>

class VID;
class STRING;
class GRAPH;
class GAMMA;
class STREAM;
class SPRITE;
class MAN;


enum SPRITE_ACTION_CODE
{
    ACT_ATTACK = 32,
    ACT_MOVE = 33,
    ACT_MOVE_TO = 34,
    ACT_BUILD_UNIT = 35,
    ACT_PATROL = 36,
    ACT_COOR_ATTACK = 37,
    ACT_RANDOM = 38,
    ACT_STOP = 39,
    ACT_PAUSE = 40,
    ACT_ROTATE = 41,
    ACT_CLEAR_COMMAND = 42,
    ACT_FLAGMAN_TRIGGER = 43,
    ACT_MOVE_ACCURATELY = 44,
    ACT_FOLLOW = 45,
    ACT_STAND_FIRM = 46,
    ACT_NO_ITEMS = 53,
    ACT_ADD_ITEM = 54,
    ACT_DELETE_ITEM = 55,
    ACT_HAVE_ITEM = 56,
    ACT_DELETE_ALL_ITEM = 57,
    ACT_GET_ITEM = 58,
    ACT_GET_ITEM_TYPE = 59,
    ACT_CHANGE_DIRECTION = 60,
    ACT_CHANGE_ANIMATION = 61,
    ACT_CHANGE_VID = 62,
    ACT_CHANGE_COOR = 63,
    ACT_BACKUP_COMMAND = 70,
    ACT_GOTO_STACK = 71,
    ACT_CLEAR_STACK = 72,
    ACT_STOP_STACK = 73,
    ACT_RESTORE_COMMAND = 74,
    ACT_COPY_STACK_TO = 75,
    ACT_NO_STACK = 76,
    ACT_WHILE_NOT_QUEST = 77,
    ACT_SET_QUEST = 78,
    ACT_WHILE_NOT_SCRIPT_VAR = 79,
    ACT_SAVE = 80,
    ACT_RESTORE = 81,
    ACT_GET_BIRTH_TIME = 84,
    ACT_DAMAGE = 85,
    ACT_REPAIR = 86,
    ACT_GET_HP = 87,
    ACT_SET_HP = 88,
    ACT_GET_PERCENT_HP = 89,
    ACT_GET_GOAL = 90,
    ACT_SET_GOAL_COOR = 91,
    ACT_GET_AMMO = 92,
    ACT_ADD_AMMO = 93,
    ACT_GET_BEHAVE = 94,
    ACT_SET_BEHAVE = 95,
    ACT_GET_ARMY = 96,
    ACT_SET_ARMY = 97,
    ACT_SET_INVISIBLE = 98,
    ACT_SET_HASH = 99,
    ACT_GET_BATTLE_RANGE = 100,
    ACT_GET_LINK = 101,
    ACT_SET_LINK = 102,
    ACT_GET_UPLINK = 103,
    ACT_SET_UPLINK = 104,
    ACT_GET_TIMER = 105,
    ACT_SET_TIMER = 106,
    ACT_GET_ZSPEED = 107,
    ACT_SET_ZSPEED = 108,
    ACT_GET_SPEED = 109,
    ACT_SET_SPEED = 110,
    ACT_GET_COMMAND = 111,
    ACT_SET_COMMAND = 112,
    ACT_SET_DEATH_TIMER = 113,
    ACT_GET_DIALOG = 114,
    ACT_SET_DIALOG = 115,
    ACT_SET_FLAGMAN_DIALOG = 116,
    ACT_GET_KILLER_ARMY = 117,
    ACT_SET_GAMMA = 118,
    ACT_SET_OBJECT_ID = 119,
    ACT_SET_TEXT = 120,
    ACT_GET_TEXT_DESC = 121,
    ACT_SET_TEXT_COUNT = 122,
    ACT_SET_FILE = 123,
    ACT_GET_TEXT = 124,
    ACT_SET_TEXT_ROW = 125,
    ACT_GET_GAMMA = 126,
    ACT_SET_TEXTURE_VID = 127,
    ACT_GET_SCALE = 128,
    ACT_SET_SCALE = 129,
    ACT_NEXT_COMMAND = 130,
    ACT_SCRIPT_VAR = 131,
    ACT_UNDO_REMOVE = 132,
    ACT_UNDO_INSERT = 133,
    ACT_DESTROY_UNIT = 134,
    ACT_PLAY_SFX = 135,
    ACT_SCRIPT_EVENT = 136,
    ACT_SET_IMPULSE_SPEED = 137,
    ACT_SET_MAX_SPEED = 138,
    ACT_GET_ACCELERATION = 139,
    ACT_LINK_ENGINE = 150,
    ACT_CLASH_ENGINE = 151,
    ACT_FORCELINK_ENGINE = 152,
    ACT_TRAIN_BEHAVE = 153,
    ACT_FIRST_ENGINE = 154,
    ACT_LAST_ENGINE = 155,
    ACT_NEXT_ENGINE = 156,
    ACT_IS_FIRST = 157,
    ACT_IN_TRAIN = 158,
    ACT_IS_TRAIN = 159,
    ACT_LEGACY_RESTORE = 200,
    ACT_NONE = 255
};

struct ACT
{
    int command;
    int arg1;
    int arg2;
    int arg3;
};


struct ADD_CREATE_SPRITE
{
    unsigned int group;
    VECTOR scale;
    GAMMA gamma;
};


struct EX_SPRITE_DATA
{
    explicit EX_SPRITE_DATA(const SPRITE* sprite);
    // Members are destroyed explicitly by SPRITE::~SPRITE.
    ~EX_SPRITE_DATA() = delete;
    VECTOR cachedPosition;
    unsigned int cachedPositionTick;
    unsigned int lifetimeRemaining;
    unsigned int childCadence;
    unsigned int effectTimestamp;
    float effectCurvePosition;
    float maxSpeedOverride;
    VECTOR scale;
    unsigned int gammaModulate;
    unsigned int gammaAdd;
    unsigned int reserved38;
    int objectId;
    unsigned int tacticalCounter;
    union { LIST<int> items; };
    union { LIST<ACT> actionLane[4]; };
    unsigned int actionStackIndex;
    union { STRING nameText; };
    int numberInName;
    VID* envTextureVid;
    unsigned int reservedA4;
};

class SPRITE
{
    friend class MAN;
    friend class GRAPH;
    friend class PLAYER;
    friend class MAP;
    friend class VID;
    friend class VID_SURFACE;
    friend class ENGINE;
    friend class TRAIN_INFO;
public:
    SPRITE(const VECTOR& position, ANGLE direction);
    SPRITE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent=0);
    virtual ~SPRITE() noexcept(false);
    static void operator delete(void* pointer);
    const VECTOR& Coor() const;
    SPRITE* Link() const
    {
        return m_childChain;
    }
    void Error(TYPE_ERROR type, const STRING& subject, unsigned long code) const;
    float X() const;
    float Y() const;
    float Z() const;
    VID* Vid() const;
    SPRITE* Goal() const;
    unsigned int Behave() const
    {
        return m_behave;
    }
    unsigned int RuntimeFlags() const
    {
        return m_runtimeFlags;
    }
    void AddReference()
    {
        ++m_referenceCount;
    }
    void SetRuntimeFlags(unsigned int value)
    {
        m_runtimeFlags = value;
    }
    void AddRuntimeFlags(unsigned int value)
    {
        m_runtimeFlags |= value;
    }
    int Animation() const
    {
        return m_currentAnimation;
    }
    unsigned int StateTimer() const
    {
        return m_stateTimer;
    }
    void SetStateTimer(unsigned int value)
    {
        m_stateTimer = value;
    }
    int CurrentFrame() const
    {
        return m_currentFrame;
    }
    int CurrentFrameEnd() const
    {
        return m_currentFrameEnd;
    }
    unsigned int LastAnimationTick() const
    {
        return m_lastAnimationTick;
    }
    SPRITE* ChildChain() const
    {
        return m_childChain;
    }
    SPRITE* ChildBacklink() const
    {
        return m_childBacklink;
    }
    EX_SPRITE_DATA* ExtraData() const
    {
        return m_extraData;
    }
    void SetExtraData(EX_SPRITE_DATA* value)
    {
        m_extraData = value;
    }
    VECTOR2 ScreenXY() const;
    void ChangeAnimation(int animation);
    void InvisibleOn();
    int Random(int maximum) const;
    float Random(float scale) const;
    float Random(VID* vid, float scale) const;
    int Release();
    ANGLE Direction() const;
    int HaveLink() const;
    ANGLE DirectionTo(const SPRITE* target) const;
    VECTOR GetScale() const;
    void SetScale(const VECTOR& scale);
    void SetEnvTexture(VID* textureVid);
    void SetGamma(const GAMMA& gamma);
    GAMMA GetGamma() const;
    SPRITE* CanPlaceInHash(const VECTOR& position) const;
    SPRITE* CanPlace(const VECTOR& position) const;
    SPRITE* CanPlaceWithCrush(const VECTOR& position);
    SPRITE* CanPlaceWithCrushAndGlide(VECTOR* position);
    void Stop();
    int CantMoveBulletTo(const VECTOR& position) const;
    SPRITE* AskLine(VECTOR* target) const;
    SPRITE* SeekEnemy() const;
    ANGLE RotateTact(ANGLE desiredDirection, int elapsed);
    void Rotate(ANGLE direction);
    void RotateForGlide(ANGLE direction, int elapsed);
    STRING GetName() const;
    void SetName(const STRING& name);
    void InsertItem(int item);
    void SetObjectID(int objectId);
    float MaxSpeed() const;
    int StartMove();
    void Move(SPRITE* target);
    void Move(const VECTOR& point);
    void Move(float x, float y, float z);
    void SetNumberInName(int number);
    int SetCommand(int command, SPRITE* target);
    int SetCommand(int command, const VECTOR& target);
    int SetCommandWithoutLink(int command, SPRITE* target);
    int AddLink(SPRITE* link);
    void BreakLink();

    void ChangeHp(int hp);
    void ChangeDirection1(ANGLE direction);
    void ChangeArmy(int army);
    void ChangeDirection(ANGLE direction);
    void ChangeCoor(const VECTOR& position);
    void ChangeCoorForLinkRotate(const VECTOR2& position);
    int DestroyLink(const VID* linkVid);
    void PlaySFX(int sfxId) const;

    void AddAction(int actionIndex, int command, int argument1, int argument2, int argument3);
    void AddActionImmediate(int actionIndex, int command, int argument1, int argument2, int argument3);
    void AddActionAfterStop(int actionIndex, int command, int argument1, int argument2, int argument3);
    const LIST<ACT>& ActionStack(int actionIndex);
    int ActionStackHaveCommand(int actionIndex, int command) const;

    STRING GetTextActions(int actionIndex) const;
    void SetTextActions(int actionIndex, const STRING& text);

    STRING GetTextItems();
    void SetTextItems(const STRING& text);

    int Attack(SPRITE* target);
    int Attack(const VECTOR& target);
    int IsXYCross(const VID* vid, const VECTOR& coor, ANGLE direction) const;
    int IsCross(const VID* vid, const VECTOR& coor, ANGLE direction) const;
    int IsRotateXYCross(const VID* vid, const VECTOR& coor, ANGLE direction) const;
    int GetFireDamage() const;
    int PercentHp() const;
    void SetGoal(SPRITE* target);
    int AddLinkToLast(SPRITE* link);
    void ResetActionStack(int actionIndex);
    int CanAttackThisSprite(const SPRITE* target) const;
    void PrimitiveTact();

    virtual int Action(int opcode, int argument1, int argument2, int argument3);
    virtual void Tact();
    virtual void MoveTact();
    virtual void DeletePointerToSprite(SPRITE* sprite);
    virtual void Draw();
    virtual void DetectTact();
    virtual void DrawSecondaryInfo();
    virtual void DrawGoalLine();
    void DrawRectangle();
    void InvisibleOff();
    void Save(STREAM* stream) const;
    SPRITE* CreateChild();

    EX_SPRITE_DATA* ExData() const;
    int IsSpriteClass(unsigned int spriteClass) const;
    int IsCommand(unsigned int command) const;
    void ChangeZCoor(float z);

protected:
    int AttackTact();
    int MoveTactMapLimit(const VECTOR& position);
    void MoveTactCalcCoor(VECTOR* outPosition);
    int CreateChildFor(int action, SPRITE* creationContext);
    void SetMoveAnimation();
    void CreateLink();

    friend class SPRITE_COLLECTOR;
    friend class R_DOT;
    friend class R_POS;

    VECTOR       m_xyz;
    unsigned int m_referenceCount;
    SPRITE*      m_goalSprite;
    SPRITE*      m_childChain;
    SPRITE*      m_childBacklink;
    int          m_currentAnimation;
    unsigned int m_stateTimer;
    int          m_hp;
    EX_SPRITE_DATA* m_extraData;
    int          m_currentFrameBegin;
    int          m_currentFrame;
    int          m_currentFrameEnd;
    unsigned int m_lastAnimationTick;
    unsigned int m_birthTime;
    VID*         m_vid;
    float        m_impulseSpeedX;
    float        m_impulseSpeedY;
    float        m_impulseSpeedZ;
    float        m_speed;
    float        m_zSpeed;
    unsigned int m_behave;
    unsigned int m_runtimeFlags;
    ANGLE        m_direction;
    unsigned char m_reserved65[3];
};

struct PTR_SPRITE
{
    SPRITE* m_value;
    PTR_SPRITE();
    ~PTR_SPRITE() noexcept(false);
};

class PRIMITIVE : public SPRITE
{
public:
    PRIMITIVE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent=0);
    virtual ~PRIMITIVE();
    virtual void Tact();
    virtual void DeletePointerToSprite(SPRITE* sprite);
    virtual void DrawGoalLine();
};

class LINKER : public SPRITE
{
    friend class SPRITE;
public:
    LINKER(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* linked=0);
    virtual ~LINKER();
    void LinkRotate(ANGLE direction);

private:
    VECTOR2 m_linkOffset;
    ANGLE   m_linkDirection;
    unsigned char m_linkPad[3];
    SPRITE* m_linkedSprite;
};

class BUILDED_TERRAIN : public SPRITE
{
public:
    BUILDED_TERRAIN(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent=0);
    virtual ~BUILDED_TERRAIN();
};

struct SPRITE_LAYOUT_ACCESS : SPRITE
{
    using SPRITE::m_xyz; using SPRITE::m_referenceCount; using SPRITE::m_goalSprite;
    using SPRITE::m_childChain; using SPRITE::m_childBacklink;
    using SPRITE::m_currentAnimation; using SPRITE::m_stateTimer; using SPRITE::m_hp;
    using SPRITE::m_extraData; using SPRITE::m_currentFrameBegin; using SPRITE::m_currentFrame;
    using SPRITE::m_currentFrameEnd; using SPRITE::m_lastAnimationTick; using SPRITE::m_birthTime;
    using SPRITE::m_vid; using SPRITE::m_impulseSpeedX; using SPRITE::m_impulseSpeedY;
    using SPRITE::m_impulseSpeedZ; using SPRITE::m_speed; using SPRITE::m_zSpeed;
    using SPRITE::m_behave; using SPRITE::m_runtimeFlags; using SPRITE::m_direction;
};

