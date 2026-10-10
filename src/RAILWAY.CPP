#include "mylib.h"
#include "railway.h"
#include "vid.h"
#include "texture.h"

#include "myerror.h"
#include "basetype.h"
#include "engine.h"
#include "railway.h"
#include "sprite.h"
#include "stream.h"
#include "region.h"
#include "map.h"
#include "graph.h"
#include <new>

namespace
{
int railDotBuckets[100][100][30] = {};
}

StaticObjectStorage<R_MAP> RailMap;
unsigned char R_DOT::CurrentPath[10000] = {};
unsigned int R_DOT::returnPathCount = 0;
unsigned int R_DOT::returnLinks[100] = {};
unsigned int R_DOT::pathLength = 0;
unsigned int R_DOT::pathScore = 0;
unsigned int R_DOT::pathDistance = 0;

unsigned int __cdecl R_MAP::InitializeStatic()
{
    ::new (static_cast<void*>(RailMap.Address())) R_MAP();
    return RegisterCrtAtExit(&R_MAP::DestroyStatic);
}

void __cdecl R_MAP::DestroyStatic()
{
    RailMap.Get().~R_MAP();
}

int R_DOT::NoStep = 65535;
int R_DOT::NoStepForNotFound = 65535;
int R_DOT::MaxPathDots = 0;
SPRITE* R_DOT::Target = 0;
R_DOT* R_DOT::FindedDot = 0;
R_DOT* R_DOT::Goal = 0;
int R_DOT::train_length_in_rails = 0;
unsigned char* R_DOT::FindedPath = 0;
ENGINE* R_DOT::eng = 0;
unsigned int R_DOT::command = 0;
int R_DOT::weaponrange = 0;
unsigned int R_DOT::head_is_head = 0;
int R_DOT::repair_in_tail = 0;
int R_DOT::repair_in_head = 0;

namespace {


static COLOR RailwayColor(unsigned int value)
{
    COLOR color;
    color.value = value;
    return color;
}
}

void R_MAP::DebugDraw()
{
    GRAPH* const graph = Graph;
    const unsigned short attachedColor16 = RGB16::Pack(GRAPH::RED.value);
    const int dotCount = m_dots.m_count;
    R_DOT** const dots = m_dots.m_data;
    for (int nodeIndex = 0;nodeIndex<dotCount;++nodeIndex)
    {
        R_DOT* const node = dots[nodeIndex];
        const VECTOR2 screen = node->ScreenXY();
        if (!(screen.x>-50.0f && screen.y>-50.0f && screen.x<1000.0f && screen.y<1000.0f))
            continue;
        const int dotHeight = node->m_z;
        const COLOR blue = RailwayColor(GRAPH::BLUE.value);
        graph->Line(screen+VECTOR2(-1, 0), screen+VECTOR2(-1, dotHeight), blue);
        graph->Line(screen+VECTOR2(-2, dotHeight), screen+VECTOR2(0, dotHeight), blue);
        unsigned short nodeColor16 = RGB16::Pack( node->m_attached?GRAPH::RED.value:GRAPH::WHITE.value);
        const int linkCount = node->m_linkCount;
        for (int edgeIndex = 0;edgeIndex<linkCount;++edgeIndex)
        {
            R_DOT::LINK& edge = node->m_links[edgeIndex];
            R_DOT* const target = edge.dot;
            unsigned short edgeColor16 = RGB16::Pack(GRAPH::WHITE.value);
            if (node->m_armyRailClass>3 && target->m_armyRailClass>3)
                edgeColor16 = RGB16::Pack(GRAPH::BLACK.value);
            if (node->m_broken != 0u && target->m_broken != 0u)
                edgeColor16 = attachedColor16;
            if (node->m_selectedLink == edgeIndex || target->m_selectedLink == edge.reverseIndex)
                edgeColor16 = RGB16::Pack(0xFFFF8080u);
            graph->Line(screen, target->ScreenXY(), RailwayColor(RGB16::Expand(edgeColor16)));
            R_DOT::LINK* const crossingRecord = edge.crossing;
            if (crossingRecord)
            {
                R_DOT* const crossingDot = crossingRecord->dot;
                graph->Line(screen, crossingDot->ScreenXY(), RailwayColor(GRAPH::GREEN.value));
            }
        }
        const COLOR nodeColor = RailwayColor(RGB16::Expand(nodeColor16));
        graph->Line(screen+VECTOR2(-2, -2), screen+VECTOR2(2, 2), nodeColor);
        graph->Line(screen+VECTOR2(-2, 2), screen+VECTOR2(2, -2), nodeColor);
    }
}

R_DOT::R_DOT()
    : m_refs(0), m_broken(0), m_selectedLink(-1), m_010(0), m_armyRailClass(0),
      m_linkCount(0), m_attached(0)
{
}

R_DOT::~R_DOT()
{
    Release();
}

void R_DOT::Release()
{
    int references = m_refs;
    if (references == 0)
        return;
    --references;
    m_refs = references;
    if (references != 0)
        return;
    m_attached = 0;
    for (int linkIndex = 0;linkIndex<m_linkCount;++linkIndex)
    {
        R_DOT* linkedDot = m_links[linkIndex].dot;
        linkedDot->UnLink(this);
    }
    m_linkCount = 0;
    int registeredDotCount = RailMap.Get().m_dots.m_count;
    if (registeredDotCount != 0)
    {
        R_DOT** const registeredDots = RailMap.Get().m_dots.m_data;
        int dotIndex = registeredDotCount;
        while (dotIndex != 0)
        {
            --dotIndex;
            if (registeredDots[dotIndex] == this)
            {
                if (dotIndex >= 0 && dotIndex<registeredDotCount)
                {
                    --registeredDotCount;
                    RailMap.Get().m_dots.m_count = registeredDotCount;
                    registeredDots[dotIndex] = registeredDots[registeredDotCount];
                }
                break;
            }
        }
    }
    this->~R_DOT();
    CrtFreePointer(this);
}

void R_DOT::SetIfIsBetter(int pathLength, int pathScore, int unused, int* result)
{
    (void)unused;
    R_DOT::FindedDot = this;
    R_DOT::NoStep = static_cast<unsigned int>(pathScore);
    *result = -1;
    unsigned char* const pathBuffer = R_DOT::FindedPath;
    if (pathBuffer && pathLength<2500)
    {
        ENGINE* const pathOwner = R_DOT::eng;
        if (pathOwner)
            pathOwner->m_pathCount = pathLength;
        CopyBytes(R_DOT::FindedPath, &R_DOT::CurrentPath[0], static_cast<unsigned int>(pathLength));
    }
}

int R_DOT::TryToFindRailsForReturn(int depth, R_DOT* rejectA, R_DOT* rejectB, ENGINE* engine, ANGLE angle)
{
    if (!depth)
        return 1;
    const int linkCount = m_linkCount;
    for (int linkIndex = 0;linkIndex<linkCount;++linkIndex)
    {
        R_DOT* linkedDot = m_links[linkIndex].dot;
        if (linkedDot == rejectA || linkedDot == rejectB || !engine)
            continue;
        if (!CanEnginePassTo(linkIndex, engine))
            continue;
        const int reverseLinkIndex = m_links[linkIndex].reverseIndex;
        if (!linkedDot->CanEnginePassTo(reverseLinkIndex, engine))
            continue;
        const ANGLE linkDirection(m_links[linkIndex].direction.value);
        if (AbsDiffU8(angle.value, linkDirection.value)>31u)
            continue;
        const int returnPathLength = static_cast<int>(R_DOT::returnPathCount);
        if (returnPathLength<95)
        {
            R_DOT::returnLinks[static_cast<unsigned int>(returnPathLength)] = static_cast<unsigned int>(linkIndex);
            R_DOT::returnPathCount = static_cast<unsigned int>(returnPathLength+1);
        }
        else Error(E_ERROR, const_cast<char*>("dots_num is large"), static_cast<unsigned long>(returnPathLength));
        if (linkedDot->TryToFindRailsForReturn(depth-1, rejectA, rejectB, engine, linkDirection))
            return 1;
        R_DOT::returnPathCount = R_DOT::returnPathCount-1u;
    }
    return 0;
}

void R_DOT::Error(TYPE_ERROR type, char* text, unsigned long detail) const
{
    const STRING subject(text);
    const STRING objectFormat("R_DOT %i,%i");
    ::Error->Error(objectFormat, type, subject, detail, m_x, m_y);
}

int R_DOT::CanEnginePassTo(int linkIndex, ENGINE* engine)
{
    if (linkIndex<0 || linkIndex >= m_linkCount)
        return 0;
    R_DOT* linkedDot = m_links[linkIndex].dot;
    if (engine)
    {
        const int engineRailClass = static_cast<int>(((engine->m_runtimeFlags>>17u)&7u)+4u);
        const int dotRailClass = m_armyRailClass;
        if (dotRailClass == engineRailClass && linkedDot->m_armyRailClass == dotRailClass)
            return 0;
        ENGINE* attachedEngine = reinterpret_cast<ENGINE*>(linkedDot->m_attached);
        if (attachedEngine && !engine->InTrain(attachedEngine))
        {
            if (R_DOT::command == 26u && attachedEngine->InTrain(R_DOT::Target))
                return 0;
            double attachedSpeed = static_cast<double>(attachedEngine->m_speed);
            if (attachedSpeed<0.0)
                attachedSpeed = -attachedSpeed;
            if (attachedSpeed >= 0.03)
            {
                R_DOT* frontDot = attachedEngine->m_frontPos.m_dot;
                R_DOT* frontNextDot = frontDot ? frontDot->m_links[attachedEngine->m_frontPos.m_linkIndex].dot : 0;
                if (frontNextDot != this)
                    goto pass_common;
            }
            SPRITE* const attachedGoal = attachedEngine->m_goalSprite;
            if (attachedGoal != 0 && attachedGoal == engine->m_goalSprite)
                goto pass_common;
            R_DOT* attachedCommandDot = attachedEngine->m_commandDot;
            if (!attachedCommandDot || attachedCommandDot != engine->m_commandDot)
                return 0;
        }
    }
    pass_common: return linkedDot->m_selectedLink != m_links[linkIndex].reverseIndex;
}

void R_DOT::UnLink(R_DOT* linkedDot)
{
    int linkCount = m_linkCount;
    int removedLinkIndex = 0;
    while (removedLinkIndex<linkCount && m_links[removedLinkIndex].dot != linkedDot)
        ++removedLinkIndex;
    if (removedLinkIndex >= linkCount)
        return;
    if (m_selectedLink == removedLinkIndex)
        m_selectedLink = -1;
    --linkCount;
    m_linkCount = linkCount;
    LINK& removed = m_links[removedLinkIndex];
    const LINK& last = m_links[linkCount];
    removed.dot = last.dot;
    removed.length = last.length;
    removed.reverseIndex = last.reverseIndex;
    removed.crossing = last.crossing;
    removed.direction = last.direction;
    R_DOT* const reverseDot = m_links[removedLinkIndex].dot;
    const int reverseLinkIndex = m_links[removedLinkIndex].reverseIndex;
    reverseDot->m_links[reverseLinkIndex].reverseIndex = removedLinkIndex;
    if (m_selectedLink == linkCount)
        m_selectedLink = removedLinkIndex;
}

void R_DOT::Link(R_DOT* linkedDot)
{
    if (!linkedDot || GetLink(linkedDot) >= 0)
        return;
    const int deltaY = linkedDot->m_y-m_y;
    const int deltaX = linkedDot->m_x-m_x;
    ANGLE direction = Decart2Polar(static_cast<float>(deltaX), static_cast<float>(deltaY));
    const int reverseDy = m_y-linkedDot->m_y;
    const int reverseDx = m_x-linkedDot->m_x;
    const int ySquare = MultiplySigned32Wrap(reverseDy, reverseDy);
    const int scaledY = MultiplySigned32Wrap(ySquare, 9)/4;
    const int xSquare = MultiplySigned32Wrap(reverseDx, reverseDx);
    const unsigned int linkLength = static_cast<unsigned int>(Sqrt(AddSigned32Wrap(xSquare, scaledY)));
    int selfLinkCount = m_linkCount;
    if (selfLinkCount<LinkCapacity)
    {
        LINK& linkRecord = m_links[selfLinkCount];
        m_linkCount = selfLinkCount+1;
        linkRecord.dot = linkedDot;
        linkRecord.length = static_cast<int>(linkLength);
        linkRecord.reverseIndex = linkedDot->m_linkCount;
        linkRecord.crossing = 0;
        linkRecord.direction = direction;
    }
    else (void)::Error->Log("!!!ERROR!!!R_DOT: Too many links in %i,%i,%i", static_cast<unsigned int>(m_x), static_cast<unsigned int>(m_y), static_cast<unsigned int>(m_z));
    direction.value = static_cast<unsigned char>(direction.value-128u);
    int otherLinkCount = linkedDot->m_linkCount;
    if (otherLinkCount<LinkCapacity)
    {
        LINK& linkRecord = linkedDot->m_links[otherLinkCount];
        linkedDot->m_linkCount = otherLinkCount+1;
        linkRecord.dot = this;
        linkRecord.length = static_cast<int>(linkLength);
        linkRecord.reverseIndex = m_linkCount-1;
        linkRecord.crossing = 0;
        linkRecord.direction = direction;
    }
    else (void)::Error->Log("!!!ERROR!!!R_DOT: Too many links2 in %i,%i,%i", static_cast<unsigned int>(linkedDot->m_x), static_cast<unsigned int>(linkedDot->m_y), static_cast<unsigned int>(linkedDot->m_z));
}

void R_DOT::SetNearestPos(int x, int y, int z, R_POS* position) const
{
    const int linkCount = m_linkCount;
    if (linkCount == 0)
        return;
    int bestDistance = 65535;
    for (int linkIndex = 0;linkIndex<linkCount;++linkIndex)
    {
        R_DOT* linkedDot = m_links[linkIndex].dot;
        const int neighborLinkCount = linkedDot->m_linkCount;
        for (int neighborLinkIndex = 0;neighborLinkIndex<neighborLinkCount;++neighborLinkIndex)
        {
            const int distance = linkedDot->GetDistance(x, y, z, neighborLinkIndex);
            if (distance<bestDistance)
            {
                bestDistance = distance;
                position->m_dot = linkedDot;
                position->m_linkIndex = neighborLinkIndex;
            }
        }
    }
    R_DOT* linkedDot = position->m_dot;
    const int selectedLinkIndex = position->m_linkIndex;
    int projectedProgress = linkedDot->GetPos(x, y, z, selectedLinkIndex);
    position->m_progress = projectedProgress;
    if (projectedProgress<0)
        position->m_progress = 0;
    linkedDot = position->m_dot;
    const int maximumProgress = linkedDot?linkedDot->m_links[position->m_linkIndex].length:0;
    if (position->m_progress >= maximumProgress)
    {
        const int selectedLinkLength = linkedDot?linkedDot->m_links[position->m_linkIndex].length:0;
        position->m_progress = selectedLinkLength-1;
    }
}

int R_DOT::GetDistance(int x, int y, int z, int linkIndex) const
{
    const LINK& linkRecord = m_links[linkIndex];
    R_DOT* linkedDot = linkRecord.dot;
    const int projectedProgress = GetPos(x, y, z, linkIndex);
    if (projectedProgress >= 0 && projectedProgress<linkRecord.length)
    {
        const int pointDeltaX = BitsToSigned32(static_cast<unsigned int>(x)-static_cast<unsigned int>(m_x));
        const int pointDeltaY = BitsToSigned32(static_cast<unsigned int>(y)-static_cast<unsigned int>(m_y));
        const int pointDeltaZ = BitsToSigned32(static_cast<unsigned int>(z)-static_cast<unsigned int>(m_z));
        const int edgeDeltaX = BitsToSigned32(static_cast<unsigned int>(linkedDot->m_x)-static_cast<unsigned int>(m_x));
        const int edgeDeltaY = BitsToSigned32(static_cast<unsigned int>(linkedDot->m_y)-static_cast<unsigned int>(m_y));
        const int edgeDeltaZ = BitsToSigned32(static_cast<unsigned int>(linkedDot->m_z)-static_cast<unsigned int>(m_z));
        const int crossX = BitsToSigned32(static_cast<unsigned int>(MultiplySigned32Wrap(pointDeltaY, edgeDeltaZ))-static_cast<unsigned int>(MultiplySigned32Wrap(pointDeltaZ, edgeDeltaY)));
        const int crossY = BitsToSigned32(static_cast<unsigned int>(MultiplySigned32Wrap(pointDeltaZ, edgeDeltaX))-static_cast<unsigned int>(MultiplySigned32Wrap(pointDeltaX, edgeDeltaZ)));
        const int crossZ = BitsToSigned32(static_cast<unsigned int>(MultiplySigned32Wrap(pointDeltaX, edgeDeltaY))-static_cast<unsigned int>(MultiplySigned32Wrap(pointDeltaY, edgeDeltaX)));
        const int edgeSquare = AddSigned32Wrap(AddSigned32Wrap(MultiplySigned32Wrap(edgeDeltaX, edgeDeltaX), MultiplySigned32Wrap(edgeDeltaY, edgeDeltaY)), MultiplySigned32Wrap(edgeDeltaZ, edgeDeltaZ));
        const int crossSquare = AddSigned32Wrap(AddSigned32Wrap(MultiplySigned32Wrap(crossX, crossX), MultiplySigned32Wrap(crossY, crossY)), MultiplySigned32Wrap(crossZ, crossZ));
        const int edgeLength = static_cast<int>(static_cast<unsigned int>(Sqrt(edgeSquare)));
        const int crossLength = static_cast<int>(static_cast<unsigned int>(Sqrt(crossSquare)));
        return crossLength/edgeLength;
    }
    const int midpointDeltaX = BitsToSigned32(static_cast<unsigned int>(AddSigned32Wrap(linkedDot->m_x, m_x)/2)-static_cast<unsigned int>(x));
    const int midpointDeltaY = BitsToSigned32(static_cast<unsigned int>(AddSigned32Wrap(linkedDot->m_y, m_y)/2)-static_cast<unsigned int>(y));
    const int midpointDeltaZ = AddSigned32Wrap(BitsToSigned32(static_cast<unsigned int>(AddSigned32Wrap(linkedDot->m_z, m_z)/2)-static_cast<unsigned int>(z)), 4);
    const int midpointSquare = AddSigned32Wrap(AddSigned32Wrap(MultiplySigned32Wrap(midpointDeltaX, midpointDeltaX), MultiplySigned32Wrap(midpointDeltaY, midpointDeltaY)), MultiplySigned32Wrap(midpointDeltaZ, midpointDeltaZ));
    return static_cast<int>(static_cast<unsigned int>(Sqrt(midpointSquare)));
}

int R_DOT::GetLink(R_DOT* linkedDot) const
{
    const int linkCount = m_linkCount;
    for (int linkIndex = 0;linkIndex<linkCount;++linkIndex)
        if (m_links[linkIndex].dot == linkedDot) return linkIndex;
    return -1;
}

int R_DOT::GetLink(ANGLE angle) const
{
    const int linkCount = m_linkCount;
    int bestLinkIndex = 0;
    for (int linkIndex = 1;linkIndex<linkCount;++linkIndex)
    {
        const unsigned char bestDirection = m_links[bestLinkIndex].direction.value;
        const unsigned char candidateDirection = m_links[linkIndex].direction.value;
        const unsigned int candidateTurn = static_cast<unsigned char>(angle.value-candidateDirection)<static_cast<unsigned char>(candidateDirection-angle.value) ? static_cast<unsigned char>(angle.value-candidateDirection) : static_cast<unsigned char>(candidateDirection-angle.value);
        const unsigned int bestTurn = static_cast<unsigned char>(angle.value-bestDirection)<static_cast<unsigned char>(bestDirection-angle.value) ? static_cast<unsigned char>(angle.value-bestDirection) : static_cast<unsigned char>(bestDirection-angle.value);
        if (candidateTurn<bestTurn)
            bestLinkIndex = linkIndex;
    }
    return bestLinkIndex;
}

int R_DOT::GetPos(int x, int y, int z, int linkIndex) const
{
    (void)z;
    const unsigned int angleIndex = static_cast<unsigned char>(m_links[linkIndex].direction.value-64u);
    const int scaledDeltaY = (y-m_y)*3;
    const int deltaX = x-m_x;
    const float projectedProgress = static_cast<float>(scaledDeltaY)*ANGLE(static_cast<unsigned char>(angleIndex)).Sin()*(0.5f) +static_cast<float>(deltaX)*ANGLE(static_cast<unsigned char>(angleIndex)).Cos();
    return TruncateFloatToInt32(projectedProgress);
}

template<> LIST<R_DOT*>::~LIST()
{
    if (m_data)
        CrtFreePointer(m_data);
    m_data = 0;
    m_count = 0;
}

R_MAP::R_MAP()
    : m_minX(10000), m_minY(10000), m_maxX(0), m_maxY(0)
{
    m_dots.m_data = 0;
    m_dots.m_count = 0;
    m_dots.m_capacity = 0;
}

R_MAP::~R_MAP()
{
}

extern "C" void*  ConstructRailDotAtAddress(void* self)
{
    new(self) R_DOT();
    return self;
}
extern "C" void  DestroyRailDotAtAddress(void* self)
{
    static_cast<R_DOT*>(self)->~R_DOT();
}
extern "C" void*  ConstructRailMapAtAddress(void* self)
{
    new(self) R_MAP();
    return self;
}
extern "C" void  DestroyRailMapAtAddress(void* self)
{
    static_cast<R_MAP*>(self)->~R_MAP();
}

R_DOT* R_MAP::CreateDot(float worldX, float worldY, float worldZ)
{
    const int x = TruncateFloatToInt32(worldX);
    const int y = TruncateFloatToInt32(worldY);
    const int z = TruncateFloatToInt32(worldZ);
    R_DOT* existingDot = GetDot(x, y, z);
    if (existingDot)
    {
        ++existingDot->m_refs;
        return existingDot;
    }
    const unsigned int dotStorageAddress = PtrToU32(CrtNew(236u));
    R_DOT* createdDot = dotStorageAddress ? new(U32ToPtr(dotStorageAddress)) R_DOT() : 0;
    if (!createdDot)
    {
        ::Error->LogExit(STRING("!!!R_MAP::CreateDot- Not enough memory"));
    }
    createdDot->m_x = x;
    createdDot->m_y = y;
    createdDot->m_z = z;
    bool alreadyPresent = false;
    for (int dotIndex = m_dots.m_count-1;dotIndex >= 0;--dotIndex)
        if (m_dots.m_data[dotIndex] == createdDot)
    {
        alreadyPresent = true;
        break;
    }
    if (!alreadyPresent)
    {
        if (m_dots.m_count >= m_dots.m_capacity)
        {
            const int oldCapacity = m_dots.m_capacity;
            const int newCapacity = AddSigned32Wrap(MultiplySigned32Wrap(oldCapacity, 2), 4);
            if (newCapacity>oldCapacity)
            {
                R_DOT** const oldDots = m_dots.m_data;
                m_dots.m_data = static_cast<R_DOT**>(CrtNew(static_cast<unsigned int>(newCapacity)*4u));
                if (!m_dots.m_data)
                    ReportAllocationFailure(static_cast<unsigned int>(newCapacity));
                if (oldDots)
                {
                    for (int copyIndex = 0;copyIndex<oldCapacity;++copyIndex)
                        m_dots.m_data[copyIndex] = oldDots[copyIndex];
                    CrtFreePointer(oldDots);
                }
                m_dots.m_capacity = newCapacity;
            }
        }
        m_dots.m_data[m_dots.m_count++] = createdDot;
    }
    if (x>m_maxX)
        m_maxX = x;
    else if (x<m_minX)
        m_minX = x;
    if (y>m_maxY)
        m_maxY = y;
    else if (y<m_minY)
        m_minY = y;
    ++createdDot->m_refs;
    return createdDot;
}

R_DOT* R_MAP::GetDot(int x, int y, int z) const
{
    for (int dotIndex = 0;dotIndex<m_dots.m_count;++dotIndex)
    {
        R_DOT* const candidateDot = m_dots.m_data[dotIndex];
        if (AbsInt(candidateDot->m_x-x) <= 1 && AbsInt(candidateDot->m_y-y) <= 1 && candidateDot->m_z == z)
            return candidateDot;
    }
    return 0;
}

R_DOT* R_MAP::GetNearestDot(int x, int y) const
{
    int bestDistance = 268435455;
    R_DOT* nearestDot = 0;
    for (int dotIndex = 0;dotIndex<m_dots.m_count;++dotIndex)
    {
        R_DOT* const candidateDot = m_dots.m_data[dotIndex];
        if (candidateDot->m_linkCount <= 0)
            continue;
        const int deltaX = candidateDot->m_x-x;
        const int screenDeltaY = (candidateDot->m_y-candidateDot->m_z)-y;
        const int distance = AddSigned32Wrap(MultiplySigned32Wrap(deltaX, deltaX), MultiplySigned32Wrap(screenDeltaY, screenDeltaY));
        if (distance<bestDistance)
        {
            bestDistance = distance;
            nearestDot = candidateDot;
        }
    }
    return nearestDot;
}

R_DOT* R_MAP::GetNearestDot(int x, int y, int z) const
{
    int bestDistance = 268435455;
    R_DOT* nearestDot = 0;
    for (int dotIndex = 0;dotIndex<m_dots.m_count;++dotIndex)
    {
        R_DOT* const candidateDot = m_dots.m_data[dotIndex];
        if (candidateDot->m_linkCount <= 0)
            continue;
        const int deltaX = candidateDot->m_x-x;
        const int deltaY = candidateDot->m_y-y;
        const int deltaZ = z-candidateDot->m_z;
        const int distance = AddSigned32Wrap(AddSigned32Wrap(MultiplySigned32Wrap(deltaX, deltaX), MultiplySigned32Wrap(deltaY, deltaY)), MultiplySigned32Wrap(deltaZ, deltaZ));
        if (distance<bestDistance)
        {
            bestDistance = distance;
            nearestDot = candidateDot;
        }
    }
    return nearestDot;
}

void R_MAP::PrepareForFindDot(R_DOT* targetDot, SPRITE* targetSprite, unsigned int command, ENGINE* engine)
{
    R_DOT::NoStep = 65535u;
    R_DOT::NoStepForNotFound = 65535u;
    R_DOT::MaxPathDots = 65535u;
    R_DOT::Goal = targetDot;
    R_DOT::FindedDot = 0;
    R_DOT::eng = engine;
    int targetDistanceLimit = 0;
    if (engine)
    {
        engine->m_pathCount = 0;
        const unsigned int trainRailLength = static_cast<unsigned int>(engine->GetTrainLengthInRails())+1u;
        R_DOT::FindedPath = engine->m_pathLinks;
        R_DOT::train_length_in_rails = trainRailLength;
        ENGINE* firstEngine = engine->FirstEngine();
        R_DOT::repair_in_head = static_cast<unsigned int>(firstEngine->IsRepair());
        ENGINE* lastEngine = engine->LastEngine();
        R_DOT::repair_in_tail = static_cast<unsigned int>(lastEngine->IsRepair());
        if (command == 28u || command == 29u)
        {
            const int weaponRange = engine->TrainWeaponRange();
            const unsigned int trainLength = R_DOT::train_length_in_rails;
            targetDistanceLimit = BitsToSigned32(static_cast<unsigned int>(weaponRange)-150u-trainLength*20u);
            if (targetDistanceLimit <= 20)
                targetDistanceLimit = 20;
        }
    }
    else
    {
        R_DOT::train_length_in_rails = 0u;
        R_DOT::FindedPath = 0;
        R_DOT::repair_in_head = 0u;
        R_DOT::repair_in_tail = 0u;
    }
    R_DOT::command = command;
    R_DOT::weaponrange = static_cast<unsigned int>(targetDistanceLimit);
    R_DOT::Target = targetSprite;
    R_DOT::head_is_head = 0u;
    for (int dotIndex = m_dots.m_count-1;dotIndex >= 0;--dotIndex)
    {
        R_DOT* const dot = m_dots.m_data[dotIndex];
        for (int linkIndex = 0;linkIndex<dot->m_linkCount;++linkIndex)
        {
            dot->m_bestDepth[linkIndex] = 268435455;
            dot->m_bestScore[linkIndex] = 268435455;
            dot->m_bestAux[linkIndex] = 268435455;
        }
    }
}

R_DOT* R_POS::Dot2() const
{
    R_DOT* currentDot = m_dot;
    if (!currentDot)
        return 0;
    const unsigned int linkIndex = static_cast<unsigned int>(m_linkIndex);
    return currentDot->m_links[linkIndex].dot;
}

int R_POS::NoStepToTarget(R_DOT* targetDot, SPRITE* targetSprite, unsigned int command, ENGINE* engine)
{
    if (!engine || (!targetDot && !targetSprite))
        return 65536;
    RailMap.Get().PrepareForFindDot(targetDot, targetSprite, command, engine);
    R_DOT* currentDot = m_dot;
    const unsigned int linkIndex = static_cast<unsigned int>(m_linkIndex);
    const unsigned char direction = currentDot?currentDot->m_links[linkIndex].direction.value:0u;
    R_DOT* nextDot = Dot2();
    const int reverseLinkIndex = nextDot->GetLink(currentDot);
    nextDot = Dot2();
    const int selectedLinkIndex = nextDot->FindNewDot(reverseLinkIndex, ANGLE(direction));
    if (selectedLinkIndex >= 0)
    {
        R_DOT* const nextSegmentDot = currentDot?currentDot->m_links[linkIndex].dot:0;
        const unsigned char childDirection = currentDot?currentDot->m_links[linkIndex].direction.value:0u;
        const unsigned char stepDirection = nextSegmentDot->m_links[selectedLinkIndex].direction.value;
        if (AbsDiffU8(childDirection, stepDirection)>31u && R_DOT::FindedDot != 0)
        {
            R_DOT* const pathDot = currentDot?currentDot->m_links[linkIndex].dot:0;
            if (engine->IsTailInFindedPath(pathDot) != 0)
            {
                const int pathScore = static_cast<int>(R_DOT::NoStep);
                R_DOT::NoStep = static_cast<unsigned int>(-pathScore);
            }
        }
    }
    return static_cast<int>(R_DOT::NoStep);
}

void R_POS::Write(STREAM* stream) const
{
    stream->Write(&m_dot->m_x, 2u);
    stream->Write(&m_dot->m_y, 2u);
    stream->Write(&m_dot->m_z, 2u);
    stream->Write(&Dot2()->m_x, 2u);
    stream->Write(&Dot2()->m_y, 2u);
    stream->Write(&Dot2()->m_z, 2u);
    unsigned int packed = static_cast<unsigned int>(m_progress)<<16;
    stream->Write(&packed, 4u);
}

void R_POS::Read(STREAM* stream)
{
    short x, y, z;
    stream->Read(&x, 2u);
    stream->Read(&y, 2u);
    stream->Read(&z, 2u);
    m_dot = RailMap.Get().GetDot(x, y, z);
    stream->Read(&x, 2u);
    stream->Read(&y, 2u);
    stream->Read(&z, 2u);
    if (m_dot)
    {
        R_DOT* second = RailMap.Get().GetDot(x, y, z);
        m_linkIndex = m_dot->GetLink(second);
    }
    if (m_linkIndex<0)
        (void)::Error->Log("!!!ERROR!!!RAIL: Read error");
    stream->Read(&m_progress, 4u);
    m_progress >>= 16;
}


void R_MAP::AddDotToArray(int x, int y, int maxX, int maxY, int dotIndex)
{
    if (x >= maxX || y >= maxY || x<0 || y<0)
        return;
    int* const bucket = railDotBuckets[x][y];
    const unsigned int bucketCount = static_cast<unsigned int>(bucket[0])+1u;
    bucket[0] = BitsToSigned32(bucketCount);
    if (BitsToSigned32(bucketCount)<30)
    {
        bucket[bucketCount] = dotIndex;
        return;
    }
    {
        const STRING subject("AddDotToArray() a[x][y][0] >= ARR_SIZE");
        const STRING objectFormat("R_MAP");
        ::Error->Error(objectFormat, E_ERROR, subject, 0u);
    }
    bucket[0] = SubSigned32Wrap(bucket[0], 1);
}

static bool RailBetweenInclusive(int value, int first, int second)
{
    return first<second ? value >= first && value <= second : value >= second && value <= first;
}

void R_MAP::CreateIntersectedDot(R_DOT* firstStart, R_DOT* firstEnd, R_DOT* secondStart, R_DOT* secondEnd)
{
    if (firstStart == secondStart || firstStart == secondEnd || firstEnd == secondStart || firstEnd == secondEnd)
        return;
    if (firstStart->m_bestDepth[0] || firstEnd->m_bestDepth[0] || secondStart->m_bestDepth[0] || secondEnd->m_bestDepth[0])
        return;
    const int firstStartX = firstStart->m_x, firstStartY = firstStart->m_y;
    const int firstEndX = firstEnd->m_x, firstEndY = firstEnd->m_y;
    const int secondStartX = secondStart->m_x, secondStartY = secondStart->m_y;
    const int secondEndX = secondEnd->m_x, secondEndY = secondEnd->m_y;
    const int firstDeltaX = SubSigned32Wrap(firstEndX, firstStartX);
    const int firstDeltaY = SubSigned32Wrap(firstEndY, firstStartY);
    const int secondDeltaX = SubSigned32Wrap(secondEndX, secondStartX);
    const int secondDeltaY = SubSigned32Wrap(secondEndY, secondStartY);
    const int firstLineConstant = SubSigned32Wrap(MultiplySigned32Wrap(firstEndX, firstStartY), MultiplySigned32Wrap(firstEndY, firstStartX));
    const int secondLineConstant = SubSigned32Wrap(MultiplySigned32Wrap(secondEndX, secondStartY), MultiplySigned32Wrap(secondEndY, secondStartX));
    const int denominator = SubSigned32Wrap(MultiplySigned32Wrap(firstDeltaX, secondDeltaY), MultiplySigned32Wrap(firstDeltaY, secondDeltaX));
    if (denominator == 0)
        return;
    const int intersectionNumeratorX = SubSigned32Wrap(MultiplySigned32Wrap(secondDeltaX, firstLineConstant), MultiplySigned32Wrap(firstDeltaX, secondLineConstant));
    const double intersectionX = static_cast<double>(intersectionNumeratorX)/static_cast<double>(denominator);
    double intersectionY = 0.0;
    if (firstDeltaX != 0)
        intersectionY = (static_cast<double>(firstLineConstant)+static_cast<double>(firstDeltaY)*intersectionX)/static_cast<double>(firstDeltaX);
    else if (secondDeltaX != 0)
        intersectionY = (static_cast<double>(secondLineConstant)+static_cast<double>(secondDeltaY)*intersectionX)/static_cast<double>(secondDeltaX);
    else return;
    const int intersectionIntX = static_cast<int>(intersectionX);
    const int intersectionIntY = static_cast<int>(intersectionY);
    if (!RailBetweenInclusive(intersectionIntX, firstStartX, firstEndX) || !RailBetweenInclusive(intersectionIntY, firstStartY, firstEndY) || !RailBetweenInclusive(intersectionIntX, secondStartX, secondEndX) || !RailBetweenInclusive(intersectionIntY, secondStartY, secondEndY))
        return;
    const int firstForwardIndex = firstStart->GetLink(firstEnd);
    const int firstReverseIndex = firstEnd->GetLink(firstStart);
    const int secondForwardIndex = secondStart->GetLink(secondEnd);
    const int secondReverseIndex = secondEnd->GetLink(secondStart);
    if (firstForwardIndex<0 || firstReverseIndex<0 || secondForwardIndex<0 || secondReverseIndex<0)
        return;
    R_DOT::LINK* const firstLink = &firstStart->m_links[static_cast<unsigned int>(firstForwardIndex)];
    R_DOT::LINK* const secondLink = &secondStart->m_links[static_cast<unsigned int>(secondForwardIndex)];
    firstStart->m_links[firstForwardIndex].crossing = secondLink;
    firstEnd->m_links[firstReverseIndex].crossing = secondLink;
    secondStart->m_links[secondForwardIndex].crossing = firstLink;
    secondEnd->m_links[secondReverseIndex].crossing = firstLink;
}

void R_MAP::CreateAdditionalDots()
{
    int gridX = (m_maxX/150)*2+10;
    int gridY = (m_maxY/150)*2+10;
    if (gridX >= 100)
        gridX = 100;
    if (gridY >= 100)
        gridY = 100;
    for (int x = 0;x<gridX;++x)
        for (int y = 0;y<gridY;++y) railDotBuckets[x][y][0] = 0;
    const int dotCount = m_dots.m_count;
    R_DOT** const dotArray = m_dots.m_data;
    for (int dotIndex = 0;dotIndex<dotCount;++dotIndex)
    {
        R_DOT* candidateDot = dotArray[dotIndex];
        candidateDot->m_bestDepth[0] = 0;
        const int cellX = candidateDot->m_x/75;
        const int cellY = candidateDot->m_y/75;
        AddDotToArray(cellX, cellY, gridX, gridY, dotIndex);
        AddDotToArray(cellX-1, cellY, gridX, gridY, dotIndex);
        AddDotToArray(cellX, cellY-1, gridX, gridY, dotIndex);
        AddDotToArray(cellX-1, cellY-1, gridX, gridY, dotIndex);
    }
    for (int x = 0;x<gridX;++x)
    {
        for (int y = 0;y<gridY;++y)
        {
            const int* const bucket = railDotBuckets[x][y];
            const int bucketCount = bucket[0];
            if (bucketCount<2)
                continue;
            for (int bucketEntryIndex = 2;bucketEntryIndex <= bucketCount;++bucketEntryIndex)
            {
                const unsigned int previousDotIndex = static_cast<unsigned int>(bucket[bucketEntryIndex-1]);
                const unsigned int currentDotIndex = static_cast<unsigned int>(bucket[bucketEntryIndex]);
                R_DOT* previousDot = dotArray[previousDotIndex];
                R_DOT* currentDot = dotArray[currentDotIndex];
                const int previousLinkCount = previousDot->m_linkCount;
                const int currentLinkCount = currentDot->m_linkCount;
                for (int previousLinkIndex = 0;previousLinkIndex<previousLinkCount;++previousLinkIndex)
                {
                    R_DOT* previousLinkedDot = previousDot->m_links[previousLinkIndex].dot;
                    for (int currentLinkIndex = 0;currentLinkIndex<currentLinkCount;++currentLinkIndex)
                    {
                        R_DOT* currentLinkedDot = currentDot->m_links[currentLinkIndex].dot;
                        CreateIntersectedDot(previousDot, previousLinkedDot, currentDot, currentLinkedDot);
                    }
                }
            }
        }
    }
}

static float RailDistanceMetric(float deltaX, float deltaY)
{
    if (deltaX<0.0f)
        deltaX = -deltaX;
    if (deltaY<0.0f)
        deltaY = -deltaY;
    const float diagonalWeight = 0.398f;
    return deltaX>deltaY ? deltaX+deltaY*diagonalWeight : deltaY+deltaX*diagonalWeight;
}

static int RailAbsoluteInt(int value)
{
    const unsigned int sign = static_cast<unsigned int>(value>>31);
    return static_cast<int>((static_cast<unsigned int>(value)^sign)-sign);
}
static int RailIntegerDistanceMetric(int firstX, int firstY, int secondX, int secondY)
{
    const int deltaX = RailAbsoluteInt(SubSigned32Wrap(firstX, secondX));
    const int deltaY = RailAbsoluteInt(SubSigned32Wrap(firstY, secondY));
    const int largerDelta = deltaX>deltaY?deltaX:deltaY, smallerDelta = deltaX>deltaY?deltaY:deltaX;
    return AddSigned32Wrap(largerDelta, smallerDelta/2);
}
static int RailDirectionDifference(unsigned char firstDirection, unsigned char secondDirection)
{
    const unsigned int forwardTurn = static_cast<unsigned char>(firstDirection-secondDirection);
    const unsigned int reverseTurn = static_cast<unsigned char>(secondDirection-firstDirection);
    return static_cast<int>(forwardTurn<reverseTurn?forwardTurn:reverseTurn);
}

static void RailSaveFallbackPath(void* candidateDot, int& result)
{
    const unsigned int pathScore = R_DOT::pathScore;
    R_DOT::FindedDot = static_cast<R_DOT*>(candidateDot);
    R_DOT::NoStepForNotFound = pathScore;
    unsigned char* const pathBuffer = R_DOT::FindedPath;
    const unsigned int pathLength = R_DOT::pathLength;
    if (pathBuffer && BitsToSigned32(pathLength)<2500)
    {
        ENGINE* const pathOwner = R_DOT::eng;
        if (pathOwner)
            WriteU32(pathOwner, 2784u, pathLength);
        CopyBytes(pathBuffer, &R_DOT::CurrentPath[0], pathLength);
    }
    result = -1;
}
static bool RailCrossingObjectPasses(void* attachedSprite, const SPRITE* engine)
{
    if (!attachedSprite)
        return true;
    float attachedSpeed = static_cast<SPRITE_LAYOUT_ACCESS*>(attachedSprite)->m_speed;
    if (attachedSpeed<0.0f)
        attachedSpeed = -attachedSpeed;
    if (static_cast<double>(attachedSpeed) >= 0.03)
        return true;
    return static_cast<ENGINE*>(attachedSprite)->InTrain(engine) != 0;
}

int R_DOT::FindNewDot(int incomingLinkIndex, ANGLE angle)
{
    const unsigned int savedReturnDirection = R_DOT::head_is_head;
    int result = -2;
    const int maximumPathLength = static_cast<int>(R_DOT::MaxPathDots);
    int pathLength = static_cast<int>(R_DOT::pathLength);
    if (pathLength >= maximumPathLength)
    {
        R_DOT::head_is_head = savedReturnDirection;
        return result;
    }
    if (incomingLinkIndex >= 0)
    {
        m_bestDepth[incomingLinkIndex] = pathLength;
        m_bestScore[incomingLinkIndex] = static_cast<int>(R_DOT::pathScore);
        m_bestAux[incomingLinkIndex] = static_cast<int>(R_DOT::pathDistance);
    }
    bool publishPrimary = (this == R_DOT::Goal);
    SPRITE* target = R_DOT::Target;
    ENGINE* attached = static_cast<ENGINE*>(m_attached);
    if (!publishPrimary && target && attached && attached->InTrain(target) != 0)
    {
        const int searchCommand = static_cast<int>(R_DOT::command);
        if (searchCommand != 26)
        {
            publishPrimary = true;
        }
        else if (incomingLinkIndex >= 0)
        {
            attached = static_cast<ENGINE*>(m_attached);
            if (attached->m_prevEngine == 0)
            {
                if (attached->m_frontPos.Dot2() == m_links[incomingLinkIndex].dot || attached->m_frontPos.Dot2() == this)
                    publishPrimary = true;
            }
            if (!publishPrimary)
            {
                if (attached->m_nextEngine != 0)
                {
                    R_DOT::head_is_head = savedReturnDirection;
                    return -2;
                }
                if (attached->m_backPos.Dot2() == m_links[incomingLinkIndex].dot || attached->m_backPos.Dot2() == this)
                    publishPrimary = true;
                else
                {
                    R_DOT::head_is_head = savedReturnDirection;
                    return -2;
                }
            }
        }
    }
    if (publishPrimary)
    {
        this->SetIfIsBetter(pathLength, static_cast<int>(R_DOT::pathScore), static_cast<int>(R_DOT::weaponrange), &result);
    }
    const int searchCommand = static_cast<int>(R_DOT::command);
    if ((searchCommand == 28 || searchCommand == 29) && target)
    {
        bool eligible = true;
        attached = static_cast<ENGINE*>(m_attached);
        ENGINE* searchEngine = R_DOT::eng;
        if (attached)
        {
            if (!searchEngine || searchEngine->InTrain(attached) == 0)
                eligible = false;
        }
        if (eligible)
        {
            const int targetDistance = TruncateFloatToInt32(RailDistanceMetric(target->m_xyz.x-static_cast<float>(m_x), target->m_xyz.y-static_cast<float>(m_y)));
            if (targetDistance <= static_cast<int>(R_DOT::weaponrange) && static_cast<int>(R_DOT::NoStep)>static_cast<int>(R_DOT::pathScore))
            {
                this->SetIfIsBetter(pathLength, static_cast<int>(R_DOT::pathScore), static_cast<int>(R_DOT::weaponrange), &result);
            }
        }
    }
    if (static_cast<int>(R_DOT::NoStep) >= 65535)
    {
        R_DOT* goalDot = R_DOT::Goal;
        if (goalDot)
        {
            R_DOT* bestDot = R_DOT::FindedDot;
            bool better = !bestDot;
            if (bestDot)
            {
                const int previousDistance = RailIntegerDistanceMetric(goalDot->m_x, goalDot->m_y, bestDot->m_x, bestDot->m_y);
                const int candidateDistance = RailIntegerDistanceMetric(goalDot->m_x, goalDot->m_y, m_x, m_y);
                better = (candidateDistance<previousDistance) || (bestDot == this && static_cast<int>(R_DOT::NoStepForNotFound)>static_cast<int>(R_DOT::pathScore));
            }
            if (better)
                RailSaveFallbackPath(this, result);
        }
        target = R_DOT::Target;
        if (target)
        {
            R_DOT* bestDot = R_DOT::FindedDot;
            bool better = !bestDot;
            if (bestDot)
            {
                const float previousDistance = RailDistanceMetric(target->m_xyz.x-static_cast<float>(bestDot->m_x), target->m_xyz.y-static_cast<float>(bestDot->m_y));
                const float candidateDistance = RailDistanceMetric(target->m_xyz.x-static_cast<float>(m_x), target->m_xyz.y-static_cast<float>(m_y));
                better = (candidateDistance<previousDistance) || (bestDot == this && static_cast<int>(R_DOT::NoStepForNotFound)>static_cast<int>(R_DOT::pathScore));
            }
            if (better)
                RailSaveFallbackPath(this, result);
        }
    }
    int incomingTraversable = 1;
    ENGINE* searchEngine = R_DOT::eng;
    if (incomingLinkIndex >= 0 && searchEngine)
    {
        R_DOT* previous = m_links[incomingLinkIndex].dot;
        incomingTraversable = previous->CanEnginePassTo(m_links[incomingLinkIndex].reverseIndex, searchEngine);
    }
    if (static_cast<int>(R_DOT::pathScore) >= static_cast<int>(R_DOT::NoStep))
    {
        R_DOT::head_is_head = savedReturnDirection;
        return result;
    }
    pathLength = AddSigned32Wrap(pathLength, 1);
    int score = AddSigned32Wrap(static_cast<int>(R_DOT::pathScore), 1);
    R_DOT::pathLength = static_cast<unsigned int>(pathLength);
    R_DOT::pathScore = static_cast<unsigned int>(score);
    const int linkCount = m_linkCount;
    const unsigned char incomingDirection = angle.value;
    for (int linkIndex = 0;linkIndex<linkCount;++linkIndex)
    {
        if (!incomingTraversable)
        {
            if (pathLength != 1 || incomingLinkIndex != linkIndex)
                continue;
        }
        else if (incomingLinkIndex == linkIndex && pathLength >= 3)
        {
            continue;
        }
        R_DOT* neighbor = m_links[linkIndex].dot;
        const int reverseLinkIndex = m_links[linkIndex].reverseIndex;
        if (incomingLinkIndex >= 0 && neighbor->m_bestScore[reverseLinkIndex] <= static_cast<int>(R_DOT::pathScore))
            continue;
        R_DOT::head_is_head = savedReturnDirection;
        int turnPenalty = 0;
        int returnPathFound = 1;
        if (incomingLinkIndex >= 0 && RailDirectionDifference(incomingDirection, m_links[linkIndex].direction.value)>31)
        {
            turnPenalty = 1;
            const int currentDepth = static_cast<int>(R_DOT::pathLength);
            searchEngine = R_DOT::eng;
            const bool shortStopped = (currentDepth <= 1 && searchEngine && searchEngine->m_speed == (0.0f));
            if (!shortStopped && incomingLinkIndex != linkIndex)
            {
                const int trainRailLength = static_cast<int>(R_DOT::train_length_in_rails);
                if (trainRailLength != 0)
                {
                    R_DOT::returnPathCount = 0u;
                    returnPathFound = this->TryToFindRailsForReturn(trainRailLength, m_links[incomingLinkIndex].dot, neighbor, searchEngine, ANGLE(incomingDirection));
                }
            }
            R_DOT::head_is_head = R_DOT::head_is_head^1u;
        }
        const int linkLength = m_links[linkIndex].length;
        R_DOT::pathDistance = static_cast<unsigned int>(AddSigned32Wrap(static_cast<int>(R_DOT::pathDistance), linkLength));
        if (turnPenalty)
        {
            R_DOT::pathScore = static_cast<unsigned int>(AddSigned32Wrap(static_cast<int>(R_DOT::pathScore), static_cast<int>(R_DOT::train_length_in_rails)));
        }
        bool traversable = true;
        R_DOT::LINK* crossingLink = m_links[linkIndex].crossing;
        if (crossingLink)
        {
            R_DOT* crossingOwner = crossingLink->dot;
            const int crossingReverseIndex = crossingLink->reverseIndex;
            R_DOT* crossingDot = crossingOwner->m_links[crossingReverseIndex].dot;
            void* attachedSprite = crossingOwner->m_attached;
            searchEngine = R_DOT::eng;
            if (!RailCrossingObjectPasses(attachedSprite, searchEngine))
                traversable = false;
            if (traversable && !RailCrossingObjectPasses(crossingDot->m_attached, searchEngine))
                traversable = false;
        }
        if (returnPathFound && traversable)
        {
            const int currentDepth = static_cast<int>(R_DOT::pathLength);
            if (currentDepth-1<2500)
                R_DOT::CurrentPath[static_cast<unsigned int>(currentDepth-1)] = m_links[linkIndex].direction.value;
            const int recursiveResult = neighbor->FindNewDot(reverseLinkIndex, ANGLE(m_links[linkIndex].direction.value));
            if (recursiveResult >= -1)
                result = linkIndex;
        }
        R_DOT::pathDistance = static_cast<unsigned int>(SubSigned32Wrap(static_cast<int>(R_DOT::pathDistance), linkLength));
        if (turnPenalty)
        {
            R_DOT::pathScore = static_cast<unsigned int>(SubSigned32Wrap(static_cast<int>(R_DOT::pathScore), static_cast<int>(R_DOT::train_length_in_rails)));
        }
    }
    R_DOT::pathLength = static_cast<unsigned int>(SubSigned32Wrap(static_cast<int>(R_DOT::pathLength), 1));
    R_DOT::pathScore = static_cast<unsigned int>(SubSigned32Wrap(static_cast<int>(R_DOT::pathScore), 1));
    R_DOT::head_is_head = savedReturnDirection;
    return result;
}

int R_POS::DoStep(R_DOT* targetDot, SPRITE* targetSprite, ENGINE* engine)
{
    R_DOT* previousDot = m_dot;
    const int previousLinkIndex = m_linkIndex;
    const unsigned char previousDirection = (previousDot?previousDot->m_links[previousLinkIndex].direction.value:0u);
    const int previousLinkLength = (previousDot?previousDot->m_links[previousLinkIndex].length:0);
    int remainingProgress = m_progress;
    if (remainingProgress>previousLinkLength)
    {
        remainingProgress -= previousLinkLength;
        m_progress = remainingProgress;
    }
    m_dot = previousDot?previousDot->m_links[previousLinkIndex].dot:0;
    if (!targetDot && !targetSprite)
    {
        m_linkIndex = -1;
    }
    else
    {
        const unsigned int searchCommand = (engine->m_runtimeFlags>>2u)&0x1Fu;
        RailMap.Get().PrepareForFindDot(targetDot, targetSprite, searchCommand, engine);
        float targetDistance;
        if (targetDot)
        {
            targetDistance = RailDistanceMetric(engine->m_xyz.x-static_cast<float>(targetDot->m_x), engine->m_xyz.y-static_cast<float>(targetDot->m_y));
        }
        else
        {
            targetDistance = RailDistanceMetric(engine->m_xyz.x-targetSprite->m_xyz.x, engine->m_xyz.y-targetSprite->m_xyz.y);
        }
        R_DOT::MaxPathDots = static_cast<unsigned int>(TruncateFloatToInt32(targetDistance*0.1f));
        R_DOT* currentDot = m_dot;
        int previousToCurrentIndex = previousDot->GetLink(currentDot);
        const unsigned char approachAngle = (previousDot?previousDot->m_links[previousToCurrentIndex].direction.value:0u);
        int currentToPreviousIndex = currentDot->GetLink(previousDot);
        int selectedLinkIndex = currentDot->FindNewDot(currentToPreviousIndex, ANGLE(approachAngle));
        m_linkIndex = selectedLinkIndex;
        if (R_DOT::NoStep == 65535u)
        {
            RailMap.Get().PrepareForFindDot(targetDot, targetSprite, searchCommand, engine);
            previousToCurrentIndex = previousDot->GetLink(currentDot);
            const unsigned char retryAngle = (previousDot?previousDot->m_links[previousToCurrentIndex].direction.value:0u);
            currentToPreviousIndex = currentDot->GetLink(previousDot);
            selectedLinkIndex = currentDot->FindNewDot(currentToPreviousIndex, ANGLE(retryAngle));
            m_linkIndex = selectedLinkIndex;
        }
    }
    R_DOT* currentDot = m_dot;
    int selectedLinkIndex = m_linkIndex;
    if (selectedLinkIndex >= 0)
    {
        const unsigned char selectedAngle = (currentDot?currentDot->m_links[selectedLinkIndex].direction.value:0u);
        const unsigned int forwardTurn = static_cast<unsigned char>(previousDirection-selectedAngle);
        const unsigned int reverseTurn = static_cast<unsigned char>(selectedAngle-previousDirection);
        const unsigned int turn = forwardTurn<reverseTurn?forwardTurn:reverseTurn;
        if (turn>31u)
        {
            R_DOT* nextDot = currentDot?currentDot->m_links[selectedLinkIndex].dot:0;
            selectedLinkIndex = currentDot->GetLink(ANGLE(static_cast<unsigned char>(previousDirection)));
            m_linkIndex = selectedLinkIndex;
            const unsigned int trainRailLength = R_DOT::train_length_in_rails;
            if (trainRailLength)
            {
                R_DOT::returnPathCount = 0u;
                const unsigned int returnPathFound = static_cast<unsigned int>(currentDot->TryToFindRailsForReturn(static_cast<int>(trainRailLength), previousDot, nextDot, engine, ANGLE(static_cast<unsigned char>(previousDirection))));
                if (!returnPathFound)
                    R_DOT::NoStep = 65535u;
                else m_linkIndex = static_cast<int>(R_DOT::returnLinks[0]);
            }
            if (engine && R_DOT::FindedDot && engine->IsTailInFindedPath(currentDot))
            {
                const int pathScore = static_cast<int>(R_DOT::NoStep);
                R_DOT::NoStep = static_cast<unsigned int>(-pathScore);
            }
        }
    }
    else
    {
        m_linkIndex = currentDot->GetLink(ANGLE(static_cast<unsigned char>(previousDirection)));
    }
    currentDot = m_dot;
    selectedLinkIndex = m_linkIndex;
    const int selectedLinkLength = (currentDot?currentDot->m_links[selectedLinkIndex].length:0);
    if (m_progress>selectedLinkLength)
        m_progress = selectedLinkLength;
    return static_cast<int>(R_DOT::NoStep);
}

