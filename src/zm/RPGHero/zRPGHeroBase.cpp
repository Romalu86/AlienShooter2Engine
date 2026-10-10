#include "mylib.h"
#include "zRPGHeroBase.h"
#include "zRPGBaseObject.h"
#include "zm/Game/zGroundItem.h"
#include <new>
#include <cstdlib>

#include "basetype.h"


zRPGHeroBase::zRPGHeroBase(OBJ_TYPE typeValue, int paramsCount)
    : zRPGObj(typeValue, paramsCount), m_apChildren(0), m_iChildrenCount(0), m_iChildrenCapacity(0), m_iSRandVal(0)
{
}

void* zRPGHeroBase::QueryInterface(int interfaceId) const
{
    if (interfaceId == 3)
        return const_cast<iRPGHeroBase*>(static_cast<const iRPGHeroBase*>(this));
    return zGroundItem::QueryInterface(interfaceId);
}

int zRPGHeroBase::iRelease()
{
    delete static_cast<zRPGObj*>(this);
    return 0;
}

void zRPGHeroBase::AddChild(iRPGObj* _pObject)
{
    if (!_pObject)
        DebugAssert(5u, "_pObject", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHeroBase.cpp", 54u);
    if (m_iChildrenCount == m_iChildrenCapacity)
    {
        const int oldCapacity = m_iChildrenCapacity;
        m_iChildrenCapacity = AddSigned32Wrap(m_iChildrenCapacity, 8);
        m_apChildren = static_cast<iRef<iRPGObj>**>(std::realloc(             m_apChildren, static_cast<unsigned int>(m_iChildrenCapacity)*                          static_cast<unsigned int>(sizeof(*m_apChildren))));
        for (int childIndex = oldCapacity; childIndex < m_iChildrenCapacity; ++childIndex)
        {
            iRef<iRPGObj>* relation = static_cast<iRef<iRPGObj>*>(                 CrtNew(sizeof(*relation)));
            if (relation)
                ::new(static_cast<void*>(relation)) iRef<iRPGObj>
            {
                0, 0, 0
            }
            ;
            m_apChildren[childIndex] = relation;
        }
    }
    for (int parentIndex = 0; parentIndex < MAX_PARENT_CNT; ++parentIndex)
        if (_pObject == zRPGObj::GetParent(parentIndex))             DebugAssert(4u, "_pObject != GetParent( i )", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHeroBase.cpp", 72u);
    for (int childIndex = 0; childIndex < m_iChildrenCount; ++childIndex)
        if (m_apChildren[childIndex]->m_object == _pObject)             DebugAssert(4u, "(*m_apChildren[ i ]) != _pObject", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHeroBase.cpp", 78u);
    m_apChildren[m_iChildrenCount]->Set(_pObject);
    m_iChildrenCount = AddSigned32Wrap(m_iChildrenCount, 1);
}

void zRPGHeroBase::RemoveChild(iRPGObj* child)
{
    int num = -1;
    for (int index = 0; index < m_iChildrenCount; ++index)
        if (m_apChildren[index]->m_object == child)
    {
        num = index;
        break;
    }
    if (num < 0)
        DebugAssert(4u, "num >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGHeroBase.cpp", 100u);
    for (++num; num < m_iChildrenCount; ++num)
        m_apChildren[num-1]->Set(m_apChildren[num]->m_object);
    m_apChildren[m_iChildrenCount-1]->Set(nullptr);
    m_iChildrenCount = SubSigned32Wrap(m_iChildrenCount, 1);
}

void zRPGHeroBase::RecalcSkills()
{
    zRPGObj::RecalcSkills();
    for (int childIndex = 0; childIndex < m_iChildrenCount; ++childIndex)
        m_apChildren[childIndex]->m_object->RecalcSkills();
}

zRPGHeroBase::~zRPGHeroBase()
{
    if (!m_apChildren)
        return;
    zRPGObj* const parent = static_cast<zRPGObj*>(this);
    while (m_iChildrenCount != 0)
        m_apChildren[0]->m_object->RemoveParent(parent, g_bGlobalRelease);
    for (int index = 0;index<m_iChildrenCapacity;++index)
    {
        iRef<iRPGObj>* const relation = m_apChildren[index];
        if (!relation)
            continue;
        relation->Invalidate();
        std::free(relation);
    }
    std::free(m_apChildren);
}


const char* GetRPGHeroParamName(unsigned int id)
{
    switch (id)
    {
        case RPGHERO_MONEY:
            return "Money";
        case RPGHERO_VIDHEAD:
            return "VidHead";
        case RPGHERO_HEALTH:
            return "Health";
        case RPGHERO_STRENGTH:
            return "Strength";
        case RPGHERO_SPEED:
            return "Speed";
        case RPGHERO_ACCURACY:
            return "Accuracy";
        case RPGHERO_INTELLIGENCE:
            return "Intelligence";
        case RPGHERO_ARMOR:
            return "S_Armor";
        case RPGHERO_GUNS:
            return "Pistols";
        case RPGHERO_SHORTGUNS:
            return "Shotgun";
        case RPGHERO_AUTOGUNS:
            return "Minuguns";
        case RPGHERO_GRANADEGUNS:
            return "Rocket";
        case RPGHERO_ENERGOGUNS:
            return "Energy";
        case RPGHERO_RANK:
            return "Rank";
        case RPGHERO_EXP:
            return "Exp";
        case RPGHERO_PERK_HEALING:
            return "Healing";
        case RPGHERO_PERK_EDUCATION:
            return "Education";
        case RPGHERO_PERK_ADDMONEY:
            return "AddMoney";
        case RPGHERO_PERK_QUICKSHOTRELOAD:
            return "QuickShotReload";
        case RPGHERO_PERK_DARKVISION:
            return "Darkvision";
        case RPGHERO_PERK_VAMPIRE:
            return "Vampire";
        case RPGHERO_PERK_HYPNOSIS:
            return "Hypnosis";
        case RPGHERO_PERK_SECRETSVISION:
            return "SecretsVision";
        case RPGHERO_PERK_QUICKCLIPRELOAD:
            return "QuickClipReload";
        case RPGHERO_PERK_HANDTOHAND:
            return "HandToHand";
        case RPGHERO_SKILLSTOADD:
            return "SkillsToAdd";
        case RPGHERO_DISABLED:
            return "Disabled";
        case RPGHERO_COLOR:
            return "HeroColor";
        case RPGHERO_LIVES:
            return "Lives";
        case RPGHERO_KILLS:
            return "Kills";
        case RPGHERO_PERK_RESISTANCE:
            return "Resistance";
        case RPGHERO_CanShowSurvMenu:
            return "CanShowSurvMenu";
        case RPGHERO_Cheater:
            return "Cheater";
        case RPGHERO_Cheater2:
            return "CheaterSaves";
        default:
            return 0;
    }
}


