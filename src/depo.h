#pragma once

#include "unit.h"

class DEPO : public UNIT
{
public:
    enum { BuildQueueStorageCount = 20 };

    DEPO(VID* vid,const VECTOR& position,ANGLE direction,SPRITE* parent=0);
    virtual ~DEPO();
    virtual void MoveTact();
    virtual int Action(int command,int argument1,int argument2,int argument3);

private:
    void AddUnitToQueue(int unitId);
    void BuildNextUnit();
    int ActionBuildUnit(int x,int y);

    unsigned int   m_depotSerial;
    unsigned short m_queueUnit[BuildQueueStorageCount];
    unsigned char  m_reserved0DC[160];
    unsigned int   m_queueBuildTime[BuildQueueStorageCount];
    unsigned char  m_reserved1CC[320];
    unsigned int   m_queueState[BuildQueueStorageCount];
    unsigned char  m_reserved35C[320];
    int            m_buildQueueIndex;
    int            m_buildQueueCapacity;
    int            m_buildQueueCount;
};

