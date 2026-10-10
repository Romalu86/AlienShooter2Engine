#include "mylib.h"
#include "../../../engine_globals.h"
#include "zCampaign.h"
#include "zm/Survive/zSurvive.h"
#include "../../RPGHero/zRPGBaseObject.h"
#include "myerror.h"
#include "zm/Game/zGroundItem.h"
#include "zm/Utils/zLogConsole.h"
#include <new>
#include <cstring>

bool zCampaign::m__abChangeableParams[50] = {};
bool zCampaign::m__abBaseParams[50] = {};
bool zCampaign::m__abArrayParams[50] = {};

static int SubtractSigned32Wrap(int left, int right)
{
    return BitsToSigned32(static_cast<unsigned int>(left)-static_cast<unsigned int>(right));
}

zCampaign::zCampaign(OBJ_TYPE _eType, iRPGFrameWork* frameWork)
    : zRPGObj(_eType, 1),
      m_bonusCount(0),
      m_frameWork(frameWork)
{
    if (_eType != 12)
        DebugAssert(5, "_eType == TYPE_CAMPAIGN",
            "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\Campaign\\zCampaign.cpp",
            31);
}

zCampaign::~zCampaign()
{
}








bool zCampaign::ParseUnknownCommand(
    const char* name, int valueCount, const char** values, iRPGFrameWork* frameWork)
{
    bool handled = zRPGObj::ParseUnknownCommand(name, valueCount, values, frameWork);
    if (handled)
        return true;

    if (std::strncmp("Bonuses_", name, std::strlen("Bonuses_")) == 0)
    {
        const char* c = std::strchr(name, '_');
        if (c == 0 || c + 1 == 0)
            DebugAssert(3, "c && c + 1",
                "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\Campaign\\zCampaign.cpp", 75);

        const int bonusGroupId = static_cast<int>(CrtAtoiText(c + 1));
        if (m_bonusCount == 0 || bonusGroupId != m_bonusIds[m_bonusCount - 1])
        {
            m_bonusIds[m_bonusCount] = bonusGroupId;
            m_bonusValues[m_bonusCount].count = 0;
            m_randomWeights[m_bonusCount].count = 0;
            ++m_bonusCount;
        }

        if (valueCount > 0)
        {
            for (int i = 0; i < valueCount; ++i)
            {
                int bonusBaseId = 0;
                if (std::strcmp(values[i], "Empty") != 0)
                {
                    iRPGObj* resolved = frameWork->GetObjByNamee(values[i], true);
                    if (resolved != 0)
                        bonusBaseId = resolved->GetBaseId();
                }
                zCampaignValueList& list = m_bonusValues[m_bonusCount - 1];
                list.values[list.count++] = bonusBaseId;
            }
        }
        handled = true;
    }

    if (std::strcmp(name, "Random") == 0)
    {
        if (valueCount > 0)
        {
            zCampaignValueList& list = m_randomWeights[m_bonusCount - 1];
            for (int i = 0; i < valueCount; ++i)
                list.values[list.count++] = static_cast<int>(CrtAtoiText(values[i]));
        }
        return true;
    }

    return handled;
}




void zCampaign::SetDataArrays()
{
    if (m_changeableParams != 0)
        return;

    m_changeableParams = m__abChangeableParams;
    m_baseParams = m__abBaseParams;
    m_arrayParams = m__abArrayParams;
    for (int parameterIndex = 0; parameterIndex != 50; ++parameterIndex)
    {
        m_changeableParams[parameterIndex] = 0;
        m_baseParams[parameterIndex] = 0;
        m_arrayParams[parameterIndex] = 0;
    }
}


bool zCampaign::GetParams(const char*, int, int&, int*) const
{
    return false;
}


int zCampaign::GetBonusBaseId(int bonusGroupId)
{
    int recordIndex = 0;
    while (recordIndex < m_bonusCount && m_bonusIds[recordIndex] != bonusGroupId)
        ++recordIndex;
    if (recordIndex >= m_bonusCount)
        return 0;

    const zCampaignValueList& bonuses = m_bonusValues[recordIndex];
    const zCampaignValueList& weights = m_randomWeights[recordIndex];
    if (bonuses.count != weights.count)
    {
        iLogConsole* logger = ::gpLogCons;
        logger->LogStr(LT_INPTERR,
            "Error in campaign.cfg for vid %d - different number of parameters!", bonusGroupId);
        return 0;
    }

    if (weights.count < 1)
        return 0;

    int totalWeight = 0;
    for (int i = 0; i < weights.count; ++i)
        totalWeight = AddSigned32Wrap(totalWeight, weights.values[i]);
    if (totalWeight <= 0)
        return 0;

    int remainingWeight = Random(SubtractSigned32Wrap(totalWeight, 1));
    for (int i = 0; i < weights.count; ++i)
    {
        remainingWeight = SubtractSigned32Wrap(remainingWeight, weights.values[i]);
        if (remainingWeight < 0)
            return bonuses.values[i];
    }
    return 0;
}




void* zCampaign::QueryInterface(int interfaceId) const
{
    if (interfaceId == 39)
        return static_cast<iCampaign*>(const_cast<zCampaign*>(this));
    return zGroundItem::QueryInterface(interfaceId);
}

int zCampaign::iRelease()
{
    if (this)
    {
        this->~zCampaign();
        CrtFreePointer(this);
    }
    return 0;
}

void zCampaign::InitParams()
{
}


#include "basetype.h"



