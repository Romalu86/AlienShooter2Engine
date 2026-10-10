#pragma once
#include "mylib.h"
#include <stddef.h>
class SPRITE;
class INPUT;
struct CARTYPE {
    float m_wheelbase;
    float m_frontArm;
    float m_rearArm;
    float m_mass;
    float m_inertia;
    float m_maxForward;
    float m_rearStiffness;
    float m_frontStiffness;
    float m_tyreLimit;
    float m_drag;
    float m_rollingResistance;
    float m_engineForce;
    float m_maxWheel;
    float m_dynamicWheelLimit;
    float m_steeringRate;
};

struct VEC2 { float x; float y; };


class zCarControl {
public:
    zCarControl();
    virtual void* QueryInterface(int id) const;
    virtual int iRelease();
    virtual ~zCarControl() noexcept(false);

private:
    static int m_iControlCarTact;
    static int m_iGetVehicleConst;

    CARTYPE* m_carType;
    float m_physicsX;
    float m_physicsY;
    float m_physicsZ;
    float m_heading;
    float m_velocityX;
    float m_velocityY;
    float m_yawVelocity;
    float m_localLongitudinal;
    float m_localLateral;
    float m_steering;
    float m_lateralOffset;
    unsigned int m_forwardInput;
    unsigned int m_reverseInput;
    unsigned int m_brakeInput;
    unsigned char m_stopState;
    unsigned char m_reverseMotion;
    unsigned char m_pad42[2];
    unsigned int m_controlStartTick;
    unsigned char m_inputGrace;
    unsigned char m_pad49[3];
    float m_goalX;
    float m_goalY;
    SPRITE* m_goalNode;
    SPRITE* m_nextGoalNode;
    unsigned char m_moveToGoalFlag;
    unsigned char m_bMoveToGoalFlag;
    unsigned char m_reverseSteeringLatch;
    unsigned char m_pad5F;
    float m_previousX;
    float m_previousY;

public:
    static void ResetScriptFunctions();
    void InitializeCarControl(SPRITE* sprite);
    void StopCarControl();
    void MoveToWorldCoord(float x,float y);
    VEC2 GetGoalCoord();
    VEC2 GetNextNodeCoord();
    void GetSpeed(float& x,float& y);
    void ControlVehicle(INPUT* input,SPRITE* _pFlagman);
    void MoveVehicle(SPRITE* _pFlagman,bool scrollScreen);
    static void ControlWheels(SPRITE* sprite,int wheel,bool moving,bool reverse);

private:
    void InitCartypes();
    void InitCar(CARTYPE* type);
    void PhysicsTact(float dt,bool active);
    void ProcessInput(INPUT* input,float dt);
    void ResetPhysicsCoordinates(SPRITE* sprite);
    float RadianFromDir(int direction);
    int DirFromRadian(float radians);
    void CheckGoalIsReached();
    void ControlScreenScrolling(SPRITE* sprite);
    SPRITE* FindNextGoal(SPRITE* headingOwner,SPRITE* origin,float radius,int& angleDiff);
    void PhysicsTactEx(float dt,bool active);
};
#include "basetype.h"





