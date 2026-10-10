#include "mylib.h"
#include <ctime>
#include <cstring>
#include "zRPGImplant.h"
#include "zRPGBaseObject.h"
#include "zm/Utils/String/zString.h"
#include "basetype.h"

bool zRPGImplant::m__abArrayParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGImplant::m__abBaseParams[MAX_RPGPARAMSCOUNT] = {};
bool zRPGImplant::m__abChangeableParams[MAX_RPGPARAMSCOUNT] = {};

int iRPGImplant::g_iSrandVal = 0;
unsigned int iRPGImplant::generationSequence = 0;
bool iRPGImplant::sb_FromGround = false;
bool iRPGImplant::sb_FromQuestServ = false;


zRPGImplant::zRPGImplant(iTextDB* textDb)
    : zRPGHeroBase(static_cast<OBJ_TYPE>(TYPE_IMPLANT), RPGIMPLANT_PARAMS_COUNT), m_pTextDB(textDb)
{
}

zRPGImplant::~zRPGImplant()
{
}

void* zRPGImplant::QueryInterface(int interfaceId) const
{
    if (interfaceId == 8)
        return const_cast<zRPGImplant*>(this);
    return zRPGHeroBase::QueryInterface(interfaceId);
}

int zRPGImplant::iRelease()
{
    this->~zRPGImplant();
    CrtFreePointer(this);
    return 0;
}

void zRPGImplant::SetDataArrays()
{
    if (m_changeableParams)
        return;
    m_changeableParams = m__abChangeableParams;
    m_baseParams = m__abBaseParams;
    m_arrayParams = m__abArrayParams;
    for (int index = 0; index < MAX_RPGPARAMSCOUNT; ++index)
    {
        m_changeableParams[index] = false;
        m_baseParams[index] = false;
        m_arrayParams[index] = false;
    }
}

void zRPGImplant::CopyParametersFrom(iRPGObj* source)
{
    zRPGObj::CopyParametersFrom(source);
    iRPGHeroBase* const sourceHeroBase =
        static_cast<iRPGHeroBase*>(source->QueryInterface(3));
    SetSRandVal(sourceHeroBase->GetSRandVal(false));
    GenerateRandomSkills();
    GenerateName();
}

int zRPGImplant::GetExtInfoLen() const
{
    int count = 0;
    for (int slot = 0;slot<RPGHERO_FOR_IMPLANTS_PARAMS_COUNT;++slot)
        if (GetSkillValue(slot) != 0) ++count;
    return count;
}

void zRPGImplant::OutputExtInfoStr(const char* token, int length, char*& ioCursor, int ordinal) const
{
    int i = 0;
    for (;i<RPGHERO_FOR_IMPLANTS_PARAMS_COUNT;++i)
    {
        if (GetSkillValue(i) != 0)
            ordinal = AddSigned32Wrap(ordinal, -1);
        if (ordinal<0)
            break;
    }
    if (i >= RPGHERO_FOR_IMPLANTS_PARAMS_COUNT)
        (void)DebugAssert(5u, "i < RPGHERO_FOR_IMPLANTS_PARAMS_COUNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGImplant.cpp", 86u);
    if (std::strncmp("ParamVal", token, static_cast<unsigned int>(length)) == 0)
    {
        zString formatted;
        (void)formatted.Format("+%d", GetSkillValue(i));
        const char* text = formatted;
        const std::size_t copyLength = std::strlen(text)+1;
        std::memcpy(ioCursor, text, copyLength);
        text = formatted;
        const std::size_t textLength = std::strlen(text);
        ioCursor += textLength;
    }
    if (std::strncmp("ParamName", token, static_cast<unsigned int>(length)) == 0)
    {
        const char* text = m_pTextDB->GetStringValue(RPG_HEROSKILL_TEXT, i);
        if (text)
        {
            const std::size_t copyLength = std::strlen(text)+1;
            std::memcpy(ioCursor, text, copyLength);
            const std::size_t textLength = std::strlen(text);
            ioCursor += textLength;
        }
    }
}

int zRPGImplant::ImplantRand()
{
    int seed = g_iSrandVal;
    if (!seed)
    {
        DebugAssert(4u, "g_iSrandVal", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGImplant.cpp", 132u);
        seed = g_iSrandVal;
    }
    const int quotient = seed / 44488;
    const int remainder = seed % 44488;
    int next = AddSigned32Wrap(MultiplySigned32Wrap(remainder, 48271),
                                     MultiplySigned32Wrap(quotient, -3399));
    if (next<0)
        next = AddSigned32Wrap(next, 2147483647);
    g_iSrandVal = next;
    return next;
}

void zRPGImplant::GenerateCost()
{
    iRPGFrameWork* framework = GetFW();
    iRPGObj* templt = framework->GetObjByNamee("ImplantSkillsCost", true);
    if (!templt)
        DebugAssert(5u, "templt", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGImplant.cpp", 294u);
    int addedCost = 0;
    for (unsigned int skillIndex = 0; skillIndex < RPGHERO_FOR_IMPLANTS_PARAMS_COUNT; ++skillIndex)
    {
        int skillValue = GetSkillValue(skillIndex);
        if (skillValue>0)
            addedCost = AddSigned32Wrap(addedCost, MultiplySigned32Wrap(templt->GetSkillValue(skillIndex), skillValue));
    }
    m_cost = AddSigned32Wrap(m_cost, addedCost);
}

bool zRPGImplant::SetSkillValue(int skill, int value)
{
    const bool result = zRPGObj::SetSkillValue(skill, value);
    if (GetAddMoney() == 16 && skill == RPGIMPLANT_CLASS)
    {
        int extType = 16;
        if (value == 1)
            extType = 6;
        else if (value == 2)
            extType = 7;
        else if (value == 3)
            extType = 8;
        m_gameObjectExtType = extType;
    }
    return result;
}

int iRPGImplant::GetImplantSeed()
{
    return g_iSrandVal;
}

int iRPGImplant::SeedImplantRand(int value)
{
    g_iSrandVal = value;
    return value;
}


unsigned int __cdecl RPGImplantMaskedRandom30Bit()
{
    return static_cast<unsigned int>(_time32(0))&0x3FFFFFFFu;
}


void zRPGImplant::GenerateRandomSkills()
{
    iRPGFrameWork* framework = GetFW();
    if (!framework)
        DebugAssert(5u, "GetFW()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGImplant.cpp", 153u);
    if (GetSkillValue(RPGIMPLANT_CLASS) == 0)
        return;
    const int requestedRandomCount = GetSkillValue(RPGIMPLANT_NUMSKILL);
    if (requestedRandomCount == 0)
        return;
    bool ownsSeed = false;
    if (g_iSrandVal == 0)
    {
        if (!sb_FromGround && !sb_FromQuestServ)
            DebugAssert(5u, "iRPGImplant::sb_FromGround || iRPGImplant::sb_FromQuestServ", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGImplant.cpp", 169u);
        ++generationSequence;
        ownsSeed = true;
        const unsigned int sourceSeed = GetSRandVal(true);
        (void)iRPGImplant::SeedImplantRand(BitsToSigned32(sourceSeed + generationSequence));
    }
    SetSRandVal(static_cast<unsigned int>(g_iSrandVal));
    (void)SetSkillValue(RPGIMPLANT_NUMSKILL, 0);
    int emptyCount = 0;
    for (int skillIndex = 1; skillIndex < RPGHERO_FOR_IMPLANTS_PARAMS_COUNT; ++skillIndex)
        if (GetSkillValue(skillIndex) == 0)
            ++emptyCount;
    int weights[RPGHERO_FOR_IMPLANTS_PARAMS_COUNT];
    for (int skillIndex = 0; skillIndex < RPGHERO_FOR_IMPLANTS_PARAMS_COUNT; ++skillIndex)
        weights[skillIndex] = 0;
    zString keyString;
    (void)keyString.Format("RandItems_%d", GetSkillValue(RPGIMPLANT_CLASS));
    framework = GetFW();
    iRPGObj* const weightTable =
        framework->GetObjByNamee(static_cast<const char*>(keyString), true);
    int positiveWeightCount = 0;
    if (weightTable)
    {
        for (int skillIndex = 0; skillIndex < RPGHERO_FOR_IMPLANTS_PARAMS_COUNT; ++skillIndex)
        {
            weights[skillIndex] = weightTable->GetSkillValue(skillIndex);
            if (weights[skillIndex]>0)
                ++positiveWeightCount;
        }
    }
    int eligibleCount = emptyCount;
    if (positiveWeightCount<eligibleCount)
        eligibleCount = positiveWeightCount;
    int randomCount = requestedRandomCount;
    if (randomCount>eligibleCount)
        randomCount = eligibleCount;
    const int rangeMin = GetSkillValue(RPGIMPLANT_ADDSKILLMIN);
    const int rangeMax = GetSkillValue(RPGIMPLANT_ADDSKILLMAX);
    while (randomCount != 0)
    {
        int skillIndex;
        do
        {
            skillIndex = ImplantRand()%RPGHERO_FOR_IMPLANTS_PARAMS_COUNT;
        }
        while (GetSkillValue(skillIndex) != 0 ||
               skillIndex == 0 || weights[skillIndex] == 0);
        const int randomValue = ImplantRand();
        const int rangeSpan = AddSigned32Wrap(SubSigned32Wrap(rangeMax, rangeMin), 1);
        const int skillValue = AddSigned32Wrap(rangeMin, randomValue%rangeSpan);
        (void)SetSkillValue(skillIndex, skillValue);
        randomCount = AddSigned32Wrap(randomCount, -1);
    }
    for (int skillIndex = 0; skillIndex < RPGHERO_FOR_IMPLANTS_PARAMS_COUNT; ++skillIndex)
    {
        if (GetSkillValue(skillIndex) == -1)
        {
            const int randomValue = ImplantRand();
            const int rangeSpan = AddSigned32Wrap(SubSigned32Wrap(rangeMax, rangeMin), 1);
            const int skillValue = AddSigned32Wrap(rangeMin, randomValue%rangeSpan);
            (void)SetSkillValue(skillIndex, skillValue);
        }
    }
    GenerateCost();
    if (ownsSeed)
        (void)iRPGImplant::SeedImplantRand(0);
}

void zRPGImplant::GenerateName()
{
    zString SkillName;
    zString FullName;
    FullName = m_pTextDB->GetStringValue(RPG_HEROSKILL_TEXT, 0);
    if (FullName.IsEmpty())
    {
        if (static_cast<const char*>(FullName) == 0)
            DebugAssert(4u, "FullName", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGImplant.cpp", 262u);
        return;
    }
    FullName += ' ';
    for (int skillIndex = 0; skillIndex < RPGHERO_FOR_IMPLANTS_PARAMS_COUNT; ++skillIndex)
    {
        const int val = GetSkillValue(skillIndex);
        if (val<0)
            DebugAssert(4u, "val >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGImplant.cpp", 272u);
        if (val>0)
        {
            SkillName = m_pTextDB->GetStringValue(RPG_HEROSKILL_TEXT, skillIndex);
            if (SkillName.IsEmpty())
                DebugAssert(4u, "!SkillName.IsEmpty()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGImplant.cpp", 277u);
            FullName += SkillName.GetAt(0u);
            const char* const numberText = SkillName.Format("%d", val);
            FullName += numberText;
        }
    }
    SetCharValue(RPG_FULLNAME, static_cast<const char*>(FullName));
}


