#pragma once
#include "basetype.h"
#include "myerror.h"
#include "list.h"
#include "zm/Utils/zStaticStorage.h"

class SPRITE;
class ENGINE;
class STREAM;
class R_POS;

class R_DOT
{
public:
    enum { LinkCapacity = 6 };

    struct LINK
    {
        R_DOT* dot;
        int length;
        int reverseIndex;
        LINK* crossing;
        ANGLE direction;
        unsigned char reserved11[3];
    };

    static int NoStep;
    static int NoStepForNotFound;
    static int MaxPathDots;
    static SPRITE* Target;
    static R_DOT* FindedDot;
    static R_DOT* Goal;
    static int train_length_in_rails;
    static unsigned char* FindedPath;
    static ENGINE* eng;
    static unsigned int command;
    static int weaponrange;
    static unsigned int head_is_head;
    static int repair_in_tail;
    static int repair_in_head;
    static unsigned char CurrentPath[10000];
    static unsigned int returnPathCount;
    static unsigned int returnLinks[100];
    static unsigned int pathLength;
    static unsigned int pathScore;
    static unsigned int pathDistance;

    R_DOT();
    ~R_DOT();
    void Release();
    void SetIfIsBetter(int pathLength, int pathScore, int unused, int* result);
    int FindNewDot(int incomingLinkIndex, ANGLE angle);
    int TryToFindRailsForReturn(int depth, R_DOT* rejectA, R_DOT* rejectB, ENGINE* engine, ANGLE angle);
    void Error(TYPE_ERROR type, char* text, unsigned long detail) const;
    int CanEnginePassTo(int linkIndex, ENGINE* engine);
    void UnLink(R_DOT* dot);
    void Link(R_DOT* dot);
    void SetNearestPos(int x, int y, int z, R_POS* position) const;
    int GetDistance(int x, int y, int z, int linkIndex) const;
    int GetLink(R_DOT* dot) const;
    int GetLink(ANGLE angle) const;
    int GetPos(int x, int y, int z, int linkIndex) const;
    VECTOR2 ScreenXY() const;

    int m_refs;
    unsigned int m_broken;
    unsigned int m_008;
    int m_selectedLink;
    unsigned int m_010;
    int m_armyRailClass;
    int m_linkCount;
    LINK m_links[LinkCapacity];
    int m_bestDepth[LinkCapacity];
    int m_bestScore[LinkCapacity];
    int m_bestAux[LinkCapacity];
    ENGINE* m_attached;
    int m_x;
    int m_y;
    int m_z;
};

template<> LIST<R_DOT*>::~LIST();

class R_MAP
{
public:
    static unsigned int __cdecl InitializeStatic();
    static void __cdecl DestroyStatic();
    R_MAP();
    ~R_MAP();
    R_DOT* CreateDot(float worldX, float worldY, float worldZ);
    R_DOT* GetDot(int x, int y, int z) const;
    R_DOT* GetNearestDot(int x, int y) const;
    R_DOT* GetNearestDot(int x, int y, int z) const;
    void PrepareForFindDot(R_DOT* target, SPRITE* sprite, unsigned int command, ENGINE* engine);
    void AddDotToArray(int x, int y, int maxX, int maxY, int dotIndex);
    void CreateIntersectedDot(R_DOT* firstStart, R_DOT* firstEnd, R_DOT* secondStart, R_DOT* secondEnd);
    void CreateAdditionalDots();
    void DebugDraw();

    int m_minX;
    int m_minY;
    int m_maxX;
    int m_maxY;
    LIST<R_DOT*> m_dots;
};

extern StaticObjectStorage<R_MAP> RailMap;

class R_POS
{
public:
    ANGLE Direct() const;
    int DoStep(R_DOT* targetDot, SPRITE* sprite, ENGINE* engine);
    int NoStepToTarget(R_DOT* target, SPRITE* sprite, unsigned int command, ENGINE* engine);
    R_DOT* Dot2() const;
    void Write(STREAM* stream) const;
    void Read(STREAM* stream);

    R_DOT* m_dot;
    int m_progress;
    unsigned int m_progressFraction;
    int m_linkIndex;
};

