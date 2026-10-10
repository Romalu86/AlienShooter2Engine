#pragma once

#include "unit.h"
#include "railway.h"
#include "sprite_list.h"

class SPRITE;
class PLAYER;
class R_POS;
class R_DOT;
class SPRITE_LIST;
struct VECTOR;

class ENGINE : public UNIT
{
    friend class R_MAP;
    friend class DEPO;
    friend class TRAIN_INFO;
public:
    enum { PathLinkCapacity = 2500 };
    static unsigned int __cdecl InitializePathDots();
    static void ClearPathDots();

    ENGINE(VID* vid, const VECTOR& position, ANGLE direction, SPRITE* parent = 0);
    virtual ~ENGINE() override;
    virtual void Error(TYPE_ERROR type, const STRING& subject, unsigned long code) const override;
    virtual void MoveTact() override;
    virtual void DeletePointerToSprite(SPRITE* target) override;
    virtual void DrawSecondaryInfo() override;
    virtual int Action(int action, int argument1, int argument2, int argument3) override;
    virtual void DrawGoalLine() override;

    int Repair(int trigger);
    void SetCommandToTrain(int command, SPRITE* targetSprite, R_DOT* firstDot, R_DOT* lastDot);
    void AddAmmoTact();
    int RepairByRepair(ENGINE* engine);
    void RepairTact();
    void ActNextCommandFighter();
    int NeedRepairByRepair();
    int NeedAddAmmo();
    void Stop();
    int TrainWeaponRange();
    void ForceStop();
    void Move(const VECTOR& targetPosition, int mode, int screenCoordinates);
    void ReCalcMoveParameters();

    void CalcCoor();
    int ForceLink(ENGINE* engine);

    int IsRepair() const;
    ENGINE* FirstEngine();
    ENGINE* LastEngine();
    void SetRDot();
    int InTrain(const SPRITE* candidateSprite) const;
    void ReverseTrain();
    ENGINE* GetIntersecting();
    ENGINE* GetBadIntersecting();
    ENGINE* GetForwardIntersecting(int* relation);
    int IsTouch(ENGINE* otherEngine, int strictOverlap) const;
    void ClearDotBusy();
    void SetDotBusy();
    void PullTail(const R_POS& position);
    float Clash(ENGINE* otherEngine, int touchRelation);
    int GetTrainLengthInRails();
    void MT_SpeedProcessing(float* currentSpeed);
    int CanLinkWithEngine(ENGINE* otherEngine);
    int MT_IntersectingProcessing(const R_POS& position, float* currentSpeed);
    void MT_PullCoordinates(R_POS* position, float speed, int movementState);
    void DeleteAttackToEngine();
    void CreatePathDots(R_DOT* dot);
    void CreatePathDots(R_DOT* dot, SPRITE_LIST* list, int mode);
    int IsTailInFindedPath(R_DOT* dot);
    int ReachTheTarget();
    void MoveEngineTact();
    void CheckPrevNextEngine();
    int IsCommandToAllTrain() const;

    ENGINE* NextEngine() const;

private:
    static StaticObjectStorage<SPRITE_LIST> PathDots;
    static unsigned char pathDotsDestructorFlags;
    static void __cdecl DestroyPathDots();
    static int NoStep;
    static int NoStepForNotFound;
    static PLAYER* ActionPlayer(unsigned int army);
    static VID* ActionVidSlot70();
    void RemoveFromPlayer();
    void AddToStateBar();

protected:
    friend class R_DOT;
    friend class R_POS;

    unsigned int m_trainFlags;
    ENGINE* m_prevEngine;
    ENGINE* m_nextEngine;
    SPRITE* m_commandOwner;
    R_DOT* m_commandDot;
    R_DOT* m_pathEnd;
    R_DOT* m_pathStart;
    int m_acceleration;
    float m_targetSpeed;
    unsigned int m_railState;
    int m_repairProgress;
    R_POS m_frontPos;
    R_POS m_backPos;
    int m_stateFC;
    int m_depotBatchNotificationPending;
    float m_previousPositionX;
    float m_previousPositionY;
    float m_previousPositionZ;
    int m_mineDeploymentReady;
    unsigned int m_commandTick;
    int m_depotBatchId;
    unsigned char m_pathLinks[PathLinkCapacity];
    int m_pathCount;
};


class TRAIN_INFO
{
public:

    unsigned int flags00;
    float maxWeaponRange;
    float minWeaponRange;
    float totalDriveScalar;
    float totalLoadScalar;
    float poweredLoadScalar;
    int maxSpeedThousandths;
    int engineCount;
    int currentHp;
    int maxHp;
    int fireDamage;
    int buildTime;
    int ammoEngineCount;
    int ammoCount;
    int ammoCapacity;
    int averageAmmoPercent;

    TRAIN_INFO(const ENGINE* engine);
    void AddEngine(const ENGINE* engine);
    int Acceleration() const;
};

