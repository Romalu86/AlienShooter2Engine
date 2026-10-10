#include "zm/Utils/zLogConsole.h"
#include "../../../engine_globals.h"
#include "mylib.h"
#include "zm/Game/Vehicles/zCarControl.h"
#include "script.h"
#include "script_exec.h"
#include "sprite.h"
#include "sprite_collector.h"
#include "input.h"
#include "map.h"
#include "zm/Utils/zRefObject.h"
#include <math.h>

namespace
{
CARTYPE vehicleType = {};
}

int zCarControl::m_iControlCarTact = 0;
int zCarControl::m_iGetVehicleConst = 0;
unsigned char carPointSteeringEnabled = 0;

zCarControl::zCarControl()
    : m_stopState(0), m_reverseMotion(0), m_controlStartTick(0), m_inputGrace(0),
      m_goalNode(0), m_nextGoalNode(0), m_moveToGoalFlag(0), m_bMoveToGoalFlag(0),
      m_reverseSteeringLatch(0), m_previousX(0.0f), m_previousY(0.0f)
{
    InitCar(&vehicleType);
}


zCarControl::~zCarControl() noexcept(false)
{
}


void* zCarControl::QueryInterface(int id) const
{
    if (id == 36) return const_cast<zCarControl*>(this);
    (void)DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\Vehicles\\zCarControl.cpp", 65u);
    return 0;
}


void zCarControl::CheckGoalIsReached()
{
    const double dx = fabs(static_cast<double>(m_goalX)-m_physicsX);
    const double dy = fabs(static_cast<double>(m_goalY)-m_physicsY);
    const float metric = static_cast<float>((dx*dx+dy*dy)*(10000.0f));

    if (m_bMoveToGoalFlag)
        DebugAssert(4u, "!m_bMoveToGoalFlag", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\Vehicles\\zCarControl.cpp", 897u);
    if (!(metric >= (28900.0f)))
        m_moveToGoalFlag = 0;
}
#include "graph.h"

void zCarControl::ControlVehicle(INPUT* inputState, SPRITE* _pFlagman)
{
    if (!_pFlagman) DebugAssert(4u, "_pFlagman", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\Vehicles\\zCarControl.cpp", 1026u);
    if (!inputState) m_moveToGoalFlag = 0;
    if (m_moveToGoalFlag == 0)
    {
        m_bMoveToGoalFlag = 0;
        m_goalNode = 0;
        m_nextGoalNode = 0;
        int index = 0;
        SPRITE* first = FindNextGoal(_pFlagman, _pFlagman, 1200.0f, index);
        m_goalNode = first;
        if (first)
        {
            SPRITE* second = FindNextGoal(_pFlagman, first, 1200.0f, index);
            m_nextGoalNode = second;
            if (second && index<16) (void)FindNextGoal(_pFlagman, first, 1200.0f, index);
            first = m_goalNode;
            if (first)
            {
                MoveToWorldCoord(first->X(), first->Y());
                m_bMoveToGoalFlag = 1;
            }
        }
    }
    if (inputState)
    {
        const bool active = inputState->GetAction(1u) != 0 || inputState->GetAction(2u) != 0 || inputState->GetAction(4u) != 0 || inputState->GetAction(3u) != 0;
        if (active && (_pFlagman->RuntimeFlags()&0x7Cu) == 4u) (void)_pFlagman->SetCommandWithoutLink(0, 0);
    }
    const unsigned int delta = ::CurrentTime-::PrevCurrentTime;
    const float scaled = (float)delta*(0.00100000005f);
    ProcessInput(inputState, scaled);
}

void zCarControl::ResetScriptFunctions()
{
    m_iControlCarTact = 0;
    m_iGetVehicleConst = 0;
}

void zCarControl::StopCarControl()
{
    m_controlStartTick = 0u;
}

void zCarControl::MoveToWorldCoord(float worldX, float worldY)
{
    m_bMoveToGoalFlag = 0u;
    m_goalX = worldX*0.00999999978f;
    m_goalY = worldY*(-0.00999999978f);
}



float zCarControl::RadianFromDir(int direction)
{
    return static_cast<float>(direction)*(0.0245436933f);
}





void zCarControl::InitCar(CARTYPE* owner)
{
    m_carType = owner;
    m_physicsX = 0.0f;
    m_physicsY = 0.0f;
    m_physicsZ = 0.0f;
    m_heading = 0.0f;
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
    m_yawVelocity = 0.0f;
    m_steering = 0.0f;
    m_lateralOffset = 0.0f;
    m_forwardInput = 0u;
    m_reverseInput = 0u;
    m_brakeInput = 0u;
}

void zCarControl::GetSpeed(float& x, float& y)
{
    const float scale = (100.0f);
    x = m_velocityX*scale;
    y = m_velocityY*scale;
}




void zCarControl::InitializeCarControl(SPRITE* value)
{
    InitCartypes();
    ResetPhysicsCoordinates(value);
    m_inputGrace = 1u;
    m_controlStartTick = ::CurrentTime;
}


void zCarControl::PhysicsTactEx(float dt, bool active)
{
    PhysicsTact(dt, active);
}
#include "basetype.h"


int zCarControl::iRelease()
{
    delete this;
    return 0;
}





VEC2 zCarControl::GetGoalCoord()
{
    VEC2 result = {0.0f, 0.0f};
    if (m_moveToGoalFlag != 0u || m_bMoveToGoalFlag != 0u)
    {
        result.x = m_goalX*(100.0f);
        result.y = m_goalY*(-100.0f);
    }
    return result;
}

VEC2 zCarControl::GetNextNodeCoord()
{
    VEC2 result = {0.0f, 0.0f};
    if (m_bMoveToGoalFlag != 0u)
    {
        SPRITE* const source = m_nextGoalNode;
        if (source)
        {
            result.x = source->X();
            result.y = source->Y();
        }
    }
    return result;
}


void zCarControl::ResetPhysicsCoordinates(SPRITE* source)
{
    m_physicsX = source->X()*(0.00999999978f);
    m_physicsY = source->Y()*(-0.00999999978f);
    m_heading = RadianFromDir(static_cast<int>(source->Direction().value));
}

int zCarControl::DirFromRadian(float radians)
{
    int phase = TruncateDoubleToInt32(floor(static_cast<double>(radians)*40.7436638f+0.5f));
    if (phase >= 256)
        phase = BitsToSigned32(static_cast<unsigned int>(phase)-((static_cast<unsigned int>(phase)>>8u)<<8u));
    if (phase<0)
        phase = BitsToSigned32(static_cast<unsigned int>(phase)+(((255u-static_cast<unsigned int>(phase))>>8u)<<8u));
    return phase;
}

void zCarControl::ControlWheels(SPRITE* actor, int wheel, bool moving, bool reverse)
{
    int callback = m_iControlCarTact;
    if (callback == 0u)
    {
        const STRING functionName("ControlWheels_cpp");
        callback = Map->m_script.GetNFunction(functionName);
        m_iControlCarTact = callback;
    }

    if (callback != 0u)
    {
        unsigned int mode = moving?1u:0u;
        if (reverse) mode += 2u;
        g_pFucking = actor;
        Map->ScriptRun(callback,
                       "%d%d",
                       wheel, static_cast<int>(mode), 0);
        g_pFucking = 0;
    }
}


void zCarControl::ControlScreenScrolling(SPRITE* actor)
{
    if (m_brakeInput != 0u)
        return;
    if (m_velocityX == 0.0f && m_velocityY == 0.0f)
        return;

    const float angle = static_cast<float>(static_cast<double>(actor->Direction().value)*(0.0245436933f)+(1.57079637f));
    GRAPH* const graph = Graph;
    const double halfWidth = static_cast<double>(graph->screenWidth)*(0.200000003f);
    const float halfHeight = graph->screenHeight*(0.200000003f);
    const double cosineValue = cos(static_cast<double>(angle));
    const float cosine = static_cast<float>(cosineValue);

    double xOffset;
    double radial;
    if (fabs(cosineValue)<0.001)
    {
        xOffset = 0.0f;
        radial = halfHeight;
    }
    else
    {
        const double tangent = tan(static_cast<double>(angle));
        const double width2 = halfWidth*halfWidth;
        const double height2 = static_cast<double>(halfHeight)*halfHeight;
        const double xMagnitude = sqrt((height2*width2)/(tangent*tangent*width2+height2));
        radial = xMagnitude/cosine;
        xOffset = (cosine<0.0f)?-xMagnitude:xMagnitude;
    }

    const double sine = sin(static_cast<double>(angle));
    float yOffset = static_cast<float>(sqrt(static_cast<double>(radial*radial-xOffset*xOffset)));
    if (!(sine>0.0f))
        yOffset = -yOffset;

    const double centerX = xOffset+static_cast<double>(graph->screenWidth)*(0.5f);
    const float centerY = static_cast<float>(static_cast<double>(yOffset)+static_cast<double>(graph->screenHeight)*(0.5f));
    const float margin = (10.0f);
    const VECTOR2 first(static_cast<float>(centerX-margin), centerY-margin);
    const VECTOR2 second(static_cast<float>(centerX+margin), centerY+margin);
    Map->SetScrollType(8, first, second);
    Map->m_scrollFollowSpeedScale = 2.0f;
}


SPRITE* zCarControl::FindNextGoal(SPRITE* headingOwner, SPRITE* origin, float searchRadius, int& outAngleDiff)
{
    outAngleDiff = 0;
    const bool extended = (headingOwner != origin);
    SPRITE* best = 0;
    float bestDistance = 999999.0f;
    SPRITE* candidate = 0;
    const VECTOR2& originXY = const_cast<VECTOR&>(origin->Coor()).XY();

    for (;;)
    {
        SPRITE_COLLECTOR* const collector = ::Sprites;
        candidate = collector->FindNearestSprite(1366, originXY, searchRadius, candidate);
        if (candidate == 0)
            break;
        if (candidate == origin)
            continue;

        const VECTOR2 delta(candidate->X()-origin->X(),
                            candidate->Y()-origin->Y());
        const float distance = delta.Length();
        if (!(distance>(260.0f)) && !extended)
            continue;

        const ANGLE direction(delta);
        unsigned char a = static_cast<unsigned char>(headingOwner->Direction().value-direction.value);
        unsigned char b = static_cast<unsigned char>(direction.value-headingOwner->Direction().value);
        const unsigned int diff = (a<b)?a:b;

        bool directionAccepted = false;
        if (diff<20u && distance>(300.0f))
            directionAccepted = true;
        else if (diff<40u)
            directionAccepted = true;
        else if (extended && diff <= 74u)
            directionAccepted = true;

        if (!directionAccepted || !(distance<bestDistance))
            continue;

        best = candidate;
        bestDistance = distance;
        outAngleDiff = static_cast<int>(diff);
    }
    return best;
}

void zCarControl::InitCartypes()
{
    int parameterReader = m_iGetVehicleConst;
    if (parameterReader == 0u)
    {
        {
            const STRING readerName("GetVehicleConst");
            parameterReader = Map->m_script.GetNFunction(readerName);
            m_iGetVehicleConst = parameterReader;
        }
        if (m_iGetVehicleConst == 0u)
            (void)DebugAssert(4u, "m_iGetVehicleConst", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\Vehicles\\zCarControl.cpp", 131u);
    }

    CARTYPE& carType = vehicleType;
    const char* readerEntry = "%d%d";
    int value;

    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 8, 0);
    carType.m_maxForward = static_cast<float>(static_cast<double>(value)*(0.00999999978f));

    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 6, 0);
    carType.m_mass = (float)value;
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 7, 0);
    carType.m_inertia = (float)value;
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 2, 0);
    carType.m_rearStiffness = (float)value;
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 3, 0);
    carType.m_frontStiffness = (float)value;
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 4, 0);
    carType.m_tyreLimit = (float)value;
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 0, 0);
    carType.m_drag = (float)value;
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 1, 0);
    carType.m_rollingResistance = (float)value;
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 5, 0);
    carType.m_engineForce = (float)value;
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 9, 0);
    carType.m_frontArm = (float)value;
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 10, 0);
    carType.m_rearArm = (float)value;
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 11, 0);
    carType.m_maxWheel = static_cast<float>(static_cast<double>(value)*(0.0174532924f));
    value = Map->ScriptRun(m_iGetVehicleConst, readerEntry, 1206, 12, 0);
    carType.m_dynamicWheelLimit = -1.0f;
    carType.m_steeringRate = static_cast<float>(static_cast<double>(value)*(0.0174532924f));

    const float scale = (0.00100000005f);
    carType.m_rearStiffness *= scale;
    carType.m_frontStiffness *= scale;
    carType.m_tyreLimit *= scale;
    carType.m_drag *= scale;
    carType.m_rollingResistance *= scale;
    carType.m_frontArm *= scale;
    const double rearArm = static_cast<double>(carType.m_rearArm)*scale;
    carType.m_rearArm = static_cast<float>(rearArm);
    carType.m_wheelbase = static_cast<float>(
        rearArm+carType.m_frontArm);
}

void zCarControl::MoveVehicle(SPRITE* _pFlagman, bool scrollScreen)
{
    if (m_controlStartTick == 0u)
        return;
    if (!_pFlagman)
        DebugAssert(5u, "_pFlagman", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\Vehicles\\zCarControl.cpp", 1095u);


    ResetPhysicsCoordinates(_pFlagman);
    const float savedX = m_physicsX;
    const float savedY = m_physicsY;
    const float savedZ = m_physicsZ;
    const float savedAngle = m_heading;

    float dt = static_cast<float>(::CurrentTime-::PrevCurrentTime)*(0.00100000005f);
    const float zero = (0.0f);
    if (dt>zero)
    {
        const float fixedStep = (0.000709999993f);
        while (dt >= fixedStep)
        {
            PhysicsTactEx(fixedStep, true);
            dt -= fixedStep;
            if (!(dt>zero))
                break;
        }
        if (dt>zero)
        {
            if (dt<(0.000142000004f))
                dt = (0.000142000004f);
            PhysicsTactEx(dt, true);
        }
    }

    VECTOR requested(m_physicsX*(100.0f), m_physicsY*(-100.0f), 0.0f);

    bool outsideWorld = true;
    MAP* const world = Map;
    if (requested.x >= zero && requested.y >= zero &&
       requested.x<world->m_sizeXY.x && requested.y<world->m_sizeXY.y)
        outsideWorld = false;


    const unsigned int direction = (unsigned int)DirFromRadian(m_heading);
    (void)_pFlagman->ChangeDirection(ANGLE(static_cast<unsigned char>(direction&0xFFu)));

    const float vx = m_localLongitudinal;
    const float vy = m_localLateral;
    const float reverseScale = m_reverseMotion?(-1.0f):(1.0f);
    const float squaredSpeed = static_cast<float>(static_cast<double>(vx)*vx+static_cast<double>(vy)*vy);
    reinterpret_cast<SPRITE_LAYOUT_ACCESS*>(_pFlagman)->m_speed = static_cast<float>(sqrt(static_cast<double>(squaredSpeed))*reverseScale*(0.00200000009f));

    bool rejected = outsideWorld;
    if (!rejected)
    {

        rejected = _pFlagman->CanPlaceWithCrushAndGlide(&requested) != 0;
        if (!rejected && requested.z <= (20.0f))
        {
            (void)_pFlagman->ChangeCoor(requested);
            const unsigned int correctedDir = (unsigned int)DirFromRadian(m_heading);
            (void)_pFlagman->ChangeDirection(ANGLE(static_cast<unsigned char>(correctedDir&0xFFu)));

            m_physicsZ = requested.z;
            m_physicsX = requested.x*(0.00999999978f);
            m_physicsY = requested.y*(-0.00999999978f);
            if (m_moveToGoalFlag)
                CheckGoalIsReached();
        }
        else
            rejected = true;
    }

    if (rejected)
    {
        m_physicsX = savedX;
        m_physicsY = savedY;
        m_physicsZ = savedZ;
        m_heading = savedAngle;
        m_velocityX = 0.0f;
        m_velocityY = 0.0f;
        m_yawVelocity = 0.0f;


        if (g_iZmDebug != 0u)
        {
            VECTOR debugPoint(savedX*(100.0f), savedY*(-100.0f), savedZ);
            const unsigned int buf = _pFlagman->CanPlaceWithCrushAndGlide(&debugPoint) != 0;
            iLogConsole* const logger = ::gpLogCons;
            const unsigned int actorDir = _pFlagman->Direction().value;
            const unsigned int oldDir = (unsigned int)DirFromRadian(savedAngle);
            (void)logger->LogStr(LT_INFO, "PrevDir = %d. Sprite dir = %d", oldDir, actorDir);
            if (buf)
                DebugAssert(4u, "buf == NULL", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\Vehicles\\zCarControl.cpp", 1182u);
        }
        m_moveToGoalFlag = 0u;
    }

    int wheel = TruncateDoubleToInt32(static_cast<double>(m_steering)*(127.323952f));
    if (wheel>100)wheel = 100;
    else if (wheel<-100)wheel = -100;

    const bool moving = (m_brakeInput == 0u && (m_velocityX != zero || m_velocityY != zero));
    ControlWheels(_pFlagman, wheel, moving, m_stopState != 0u);
    if (scrollScreen)
        ControlScreenScrolling(_pFlagman);
}

void zCarControl::ProcessInput(INPUT* input, float dt)
{
    unsigned int buttons = 0u;

    if (input)
    {


        if (m_inputGrace != 0u)
        {
            const bool directionalHeld = input->GetAction(1u) != 0 || input->GetAction(2u) != 0 ||
                                       input->GetAction(3u) != 0 || input->GetAction(4u) != 0;
            if (directionalHeld && (::CurrentTime-m_controlStartTick) <= 500u)
                return;
            m_inputGrace = 0u;
        }

        if (input->GetAction(6u) != 0)
        {
            MoveToWorldCoord(input->MouseXY().x, input->MouseXY().y);
            m_moveToGoalFlag = 1u;
        }

        if (input->GetAction(1u) != 0) buttons |= 0x01u;
        if (input->GetAction(2u) != 0) buttons |= 0x02u;
        if (input->GetAction(3u) != 0) buttons |= 0x04u;
        if (input->GetAction(4u) != 0) buttons |= 0x08u;

        if (carPointSteeringEnabled != 0)
        {
            float targetX = m_physicsX*100.0f;
            float targetY = m_physicsY*-100.0f;
            const float oldX = targetX;
            const float oldY = targetY;
            buttons = 0u;
            if (input->GetAction(3u) != 0) targetX -= 200.0f;
            else if (input->GetAction(4u) != 0) targetX += 200.0f;
            if (input->GetAction(1u) != 0) targetY -= 200.0f;
            else if (input->GetAction(2u) != 0) targetY += 200.0f;
            if (targetX != oldX || targetY != oldY)
            {
                MoveToWorldCoord(targetX, targetY);
                m_moveToGoalFlag = 1u;
            }
        }

        if (m_moveToGoalFlag != 0u)
        {
            if (buttons != 0u) m_moveToGoalFlag = 0u;
            else buttons = 0u;
        }
    }

    CARTYPE* carType = m_carType;
    carType->m_dynamicWheelLimit = -1.0f;

    if (m_moveToGoalFlag != 0u || (m_bMoveToGoalFlag != 0u && buttons == 1u))
    {
        const double dx = static_cast<double>(m_goalX)-m_physicsX;
        const double dy = static_cast<double>(m_goalY)-m_physicsY;
        const double initialAngle = (static_cast<double>(1.5707963705062866f)-m_heading)-atan2(dy, dx);
        float angle = static_cast<float>(initialAngle);
        const float distanceDy = static_cast<float>(dy);
        const float distance2 = static_cast<float>(dx*dx+static_cast<double>(distanceDy)*distanceDy);
        float cosAngle = static_cast<float>(cos(initialAngle));

        if (!(cos(2.0)<cosAngle) && distance2<400.0f && (buttons&1u) == 0u)
            buttons |= 2u;
        else
            buttons |= 1u;

        if ((buttons&2u) != 0u)
        {
            const double reverseAngle = static_cast<double>(angle)+3.1415927410125732f;
            angle = static_cast<float>(reverseAngle);
            cosAngle = static_cast<float>(cos(reverseAngle));
        }

        const float cos7 = (float)cos(0.12217305103937785);
        const double cos2 = cos(0.03490658476948738);
        bool applySteering = false;

        if (cos7<cosAngle && cos2 >= cosAngle)
        {
            carType->m_dynamicWheelLimit = 0.00261799409054219722747802734375f;
            applySteering = true;
        }
        else if (cos7>cosAngle)
        {
            bool allowSteer = true;
            if (m_yawVelocity != 0.0f && cos(0.19198622306187949)<cosAngle)
            {
                const double predicted = static_cast<double>(angle)-static_cast<double>(m_yawVelocity)*0.3499999940395355f;
                if (cos(predicted)<cos7)
                    allowSteer = false;
            }
            if (m_bMoveToGoalFlag != 0u && m_localLongitudinal>200.0f && static_cast<double>(distance2)*10000.0f<2500.0f)
                allowSteer = false;
            applySteering = allowSteer;
        }

        if (applySteering)
        {
            const double steerSign = sin(static_cast<double>(angle));
            if ((buttons&1u) != 0u)
                buttons |= (steerSign<0.0f)?0x04u:0x08u;
            else if ((buttons&2u) != 0u)
                buttons |= (steerSign >= 0.0f)?0x04u:0x08u;
        }
    }

    if ((buttons&0x01u) != 0u) m_forwardInput = 100u;
    else m_forwardInput = 0u;
    if ((buttons&0x02u) != 0u)
    {
        m_reverseInput = 100u;
        m_forwardInput = 0u;
    }
    else m_reverseInput = 0u;

    if ((buttons&0x20u) != 0u) m_lateralOffset = -0.1f;
    else if ((buttons&0x40u) != 0u) m_lateralOffset = 0.1f;
    else m_lateralOffset = 0.0f;

    double wheel = m_steering;
    const float steeringRate = carType->m_steeringRate;
    if ((buttons&0x04u) != 0u) wheel -= static_cast<double>(dt)*steeringRate;
    else if ((buttons&0x08u) != 0u) wheel += static_cast<double>(dt)*steeringRate;
    else
    {
        double decay = steeringRate;
        if (m_localLongitudinal>200.0f) decay *= 5.0f;
        const double step = static_cast<double>(dt)*decay;
        if (wheel>0.0f)
        {
            wheel -= step;
            if (wheel<0.0f) wheel = 0.0f;
        }
        else if (wheel<0.0f)
        {
            wheel += step;
            if (wheel>0.0f) wheel = 0.0f;
        }
    }

    wheel = static_cast<float>(wheel);
    const float maxWheel = carType->m_maxWheel;
    if (wheel < -maxWheel) wheel = -maxWheel;
    if (wheel >  maxWheel) wheel = maxWheel;
    const float dynamicLimit = carType->m_dynamicWheelLimit;
    if (dynamicLimit >= 0.0f)
    {
        if (wheel < -dynamicLimit) wheel = -dynamicLimit;
        if (wheel >  dynamicLimit) wheel = dynamicLimit;
    }
    m_steering = static_cast<float>(wheel);
}

void zCarControl::PhysicsTact(float dt, bool active)
{
    (void)active;

    
    const float zero = (0.0f);
    const float epsilon = (9.99999997e-07f);


    m_localLongitudinal = zero;
    m_localLateral = zero;

    if (m_velocityX == zero && m_velocityY == zero &&
       m_forwardInput == 0u && m_reverseInput == 0u)
    {
        m_yawVelocity = zero;
        m_stopState = 0u;
        m_reverseMotion = 0u;
        return;
    }

    CARTYPE* const carType = m_carType;
    const float heading = m_heading;
    const float sinHeading = (float)sin((double)heading);
    const double initialCosHeading = cos(static_cast<double>(heading));
    const float cosHeading = static_cast<float>(initialCosHeading);


    double longitudinal = initialCosHeading*m_velocityY+static_cast<double>(sinHeading)*m_velocityX;
    float lateral = static_cast<float>(static_cast<double>(cosHeading)*m_velocityX-static_cast<double>(sinHeading)*m_velocityY);
    if (fabs((double)longitudinal)<(double)epsilon) longitudinal = zero;
    if (fabs((double)lateral)<(double)epsilon) lateral = zero;

    double halfYawVelocity = static_cast<double>(carType->m_wheelbase)*m_yawVelocity*0.5f;
    if (fabs((double)halfYawVelocity)<(double)epsilon) halfYawVelocity = zero;

    float yawSlip = zero;
    if (longitudinal != zero)
        yawSlip = (float)atan2((double)halfYawVelocity, fabs((double)longitudinal));
    if (fabs((double)yawSlip)<(double)epsilon) yawSlip = zero;

    double initialBodySlip = zero;
    if (longitudinal != zero)
        initialBodySlip = atan2(static_cast<double>(lateral), fabs(longitudinal));
    if (fabs(initialBodySlip)<static_cast<double>(epsilon)) initialBodySlip = zero;
    const float bodySlip = static_cast<float>(initialBodySlip);

    const unsigned int reverseInput = m_reverseInput;
    const unsigned int forwardInput = m_forwardInput;
    const bool inputPresent = (reverseInput != 0u || forwardInput != 0u);
    const bool movingBackwardUnderReverseInput = (longitudinal<zero && reverseInput != 0u);

    m_reverseMotion = movingBackwardUnderReverseInput?1u:0u;
    m_stopState = ((longitudinal>zero && reverseInput != 0u) || !inputPresent)?1u:0u;

    if (longitudinal>zero)
        m_reverseSteeringLatch = 0u;
    if (movingBackwardUnderReverseInput)
        m_reverseSteeringLatch = 1u;
    const bool reverseSteering = (movingBackwardUnderReverseInput || m_reverseSteeringLatch != 0u);

    const float steering = m_steering;
    double frontSlip;
    if (reverseSteering)
        frontSlip = initialBodySlip+steering+yawSlip;
    else
        frontSlip = initialBodySlip+yawSlip-steering;
    float rearSlip = bodySlip-yawSlip;

    if (fabs((double)frontSlip)<(double)epsilon) frontSlip = zero;
    if (fabs((double)rearSlip)<(double)epsilon) rearSlip = zero;

    const float normalHalf = static_cast<float>(static_cast<double>(carType->m_mass)*4.9000001f);
    const float tyreLimit = carType->m_tyreLimit;

    double frontForce = frontSlip*carType->m_frontStiffness;
    if (frontForce>tyreLimit) frontForce = tyreLimit;
    if (frontForce<-tyreLimit) frontForce = -tyreLimit;
    frontForce *= normalHalf;
    if (fabs((double)frontForce)<(double)epsilon) frontForce = zero;

    double rearForce = static_cast<double>(rearSlip)*carType->m_rearStiffness;
    if (m_brakeInput != 0u)
        rearForce *= (1.5f);
    if (rearForce>tyreLimit) rearForce = tyreLimit;
    if (rearForce<-tyreLimit) rearForce = -tyreLimit;
    rearForce *= normalHalf;
    if (fabs((double)rearForce)<(double)epsilon) rearForce = zero;

    const float dragCoefficient = carType->m_drag;
    float rollingResistance = carType->m_rollingResistance;
    if (movingBackwardUnderReverseInput)
        rollingResistance *= (5.0f);

    double longitudinalForce = -(fabs(longitudinal)*dragCoefficient+rollingResistance)*longitudinal;
    const float lateralDrag = static_cast<float>(-(fabs(static_cast<double>(lateral))*dragCoefficient+rollingResistance)*lateral);

    const float mass = carType->m_mass;
    if (m_brakeInput != 0u)
    {
        if (longitudinal>zero)
            longitudinalForce -= static_cast<double>(mass)*5.0f;
        else if (longitudinal<zero)
            longitudinalForce += static_cast<double>(mass)*5.0f;
    }
    else if (reverseInput != 0u)
    {
        if (longitudinal>zero)
            longitudinalForce -= static_cast<double>(mass)*10.0f;
        else
            longitudinalForce -= static_cast<double>(mass)*5.0f;
    }
    else if (!inputPresent)
    {
        if (longitudinal>zero)
            longitudinalForce -= static_cast<double>(mass)*5.0f;
        else if (longitudinal<zero)
            longitudinalForce += static_cast<double>(mass)*5.0f;
    }

    if (m_brakeInput == 0u)
        longitudinalForce += static_cast<double>(static_cast<int>(forwardInput))*carType->m_engineForce;

    const float totalLongitudinalForce = static_cast<float>(longitudinalForce);
    const float lateralForce = static_cast<float>(cos(static_cast<double>(steering))*frontForce+lateralDrag+rearForce);
    const double initialYawTorque = frontForce*carType->m_frontArm-rearForce*carType->m_rearArm;
    float yawTorque = static_cast<float>(initialYawTorque);
    if (fabs(initialYawTorque)<static_cast<double>(epsilon)) yawTorque = zero;

    double longitudinalAcceleration = static_cast<double>(totalLongitudinalForce)/mass;
    float lateralAcceleration = static_cast<float>(static_cast<double>(lateralForce)/mass);
    if (fabs((double)longitudinalAcceleration)<(double)epsilon) longitudinalAcceleration = zero;
    if (fabs((double)lateralAcceleration)<(double)epsilon) lateralAcceleration = zero;

    const double initialAngularAcceleration = static_cast<double>(yawTorque)/carType->m_inertia;
    float angularAcceleration = static_cast<float>(initialAngularAcceleration);
    if (fabs(initialAngularAcceleration)<static_cast<double>(epsilon)) angularAcceleration = zero;

    longitudinal += longitudinalAcceleration*dt;
    lateral = static_cast<float>(static_cast<double>(lateral)+static_cast<double>(lateralAcceleration)*dt);
    const double integratedYawVelocity = static_cast<double>(m_yawVelocity)+static_cast<double>(angularAcceleration)*dt;
    m_yawVelocity = static_cast<float>(integratedYawVelocity);

    const float maxForward = carType->m_maxForward;
    double speedScale = 1.0;
    if (longitudinal>maxForward)
        speedScale = longitudinal/maxForward;
    else
    {
        const double maxReverse = static_cast<double>(maxForward)*(-0.333333343f);
        if (longitudinal<maxReverse)
            speedScale = longitudinal/maxReverse;
    }
    if (speedScale != 1.0f)
    {
        longitudinal /= speedScale;
        lateral = static_cast<float>(static_cast<double>(lateral)/speedScale);
        m_yawVelocity = static_cast<float>(integratedYawVelocity/speedScale);
    }

    const bool oldXNonNegative = (m_velocityX >= zero);
    const bool oldYNonNegative = (m_velocityY >= zero);

    const float longitudinalWorldX = static_cast<float>(longitudinal*sinHeading);
    const float longitudinalWorldY = static_cast<float>(longitudinal*cosHeading);
    const double worldX = static_cast<double>(longitudinalWorldX)+static_cast<double>(lateral)*cosHeading;
    const double worldY = static_cast<double>(longitudinalWorldY)-static_cast<double>(lateral)*sinHeading;
    const bool newXNonNegative = (worldX >= zero);
    const bool newYNonNegative = (worldY >= zero);

    m_velocityX = static_cast<float>(worldX);
    m_velocityY = static_cast<float>(worldY);
    if (fabs((double)worldX)<(double)epsilon) m_velocityX = zero;
    if (fabs(static_cast<double>(m_velocityY))<static_cast<double>(epsilon)) m_velocityY = zero;
    if (fabs((double)m_yawVelocity)<(double)epsilon) m_yawVelocity = zero;

    m_heading = static_cast<float>(static_cast<double>(m_heading)+static_cast<double>(dt)*m_yawVelocity);

    bool stopMotion = false;
    if (!inputPresent && (oldXNonNegative != oldYNonNegative || newXNonNegative != newYNonNegative))
    {
        if (fabs((double)m_velocityX)<(double)(0.000699999975f) &&
           fabs((double)m_velocityY)<(double)(0.000699999975f) &&
           fabs((double)m_yawVelocity)<(double)(0.00150000001f))
            stopMotion = true;
    }

    const double displacedLateral = static_cast<double>(lateral)+m_lateralOffset;
    const float nextX = static_cast<float>(static_cast<double>(m_physicsX)+(displacedLateral*cosHeading+longitudinalWorldX)*dt);
    const double nextY = static_cast<double>(m_physicsY)+(static_cast<double>(longitudinalWorldY)-displacedLateral*sinHeading)*dt;
    m_physicsX = nextX;
    m_physicsY = static_cast<float>(nextY);

    if (!inputPresent)
    {
        const float previousX = m_previousX;
        const float previousY = m_previousY;
        if (previousX != zero || previousY != zero)
        {
            if (fabs(static_cast<double>(previousX)-nextX)<(double)(9.99999975e-06f) &&
               fabs(static_cast<double>(previousY)-nextY)<(double)(9.99999975e-06f) &&
               fabs((double)m_yawVelocity)<(double)(0.00150000001f))
                stopMotion = true;
        }
        m_previousX = nextX;
        m_previousY = static_cast<float>(nextY);
    }
    else
    {
        m_previousX = zero;
        m_previousY = zero;
    }

    if (stopMotion)
    {
        m_velocityX = zero;
        m_velocityY = zero;
    }

    if (m_velocityX != zero || m_velocityY != zero)
    {
        m_localLongitudinal = static_cast<float>(longitudinal*100.0f);
        m_localLateral = static_cast<float>(static_cast<double>(lateral)*100.0f);
    }
}



