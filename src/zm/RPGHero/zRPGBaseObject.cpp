#include "zRPGBaseObject.h"
#include "basetype.h"
#include "zm/IniParser/zIniParser.h"
#include <new>

int zRPGObj::m_iFormulaHero = 0;
int zRPGObj::m_iFormulaWeapon = 0;
int zRPGObj::m_iFormulaArmor = 0;

int zItem::GetBuyNum() const
{
    return m_buyNumber;
}

int zItem::GetGrndTakeNum() const
{
    return m_buyNumber / 2;
}

bool zItem::HaveInv() const
{
    return m_pInventory != 0;
}

void zItem::GetSize2DBase(int& outWidth, int& outHeight) const
{
    outWidth = m_width;
    outHeight = m_height;
}

int zItem::GetId() const
{
    return m_cost < 0 ? SubSigned32Wrap(0, m_cost) : 0;
}

OBJ_TYPE zItem::GetType() const
{
    return static_cast<OBJ_TYPE>(m_itemType);
}

int zItem::GetShopNum(int shop)
{
    return shop ? m_shopNumber1 : m_shopNumber0;
}

bool zRPGObj::CanHoldObj(const iRPGObj*) const
{
    return false;
}

void zRPGObj::SetClone(int clone)
{
    m_clone = clone;
}

int zRPGObj::GetClone() const
{
    return m_clone;
}

bool zRPGObj::IsArrayProperty(int index)
{
    return m_arrayParams[index];
}

void zRPGObj::SetRecalcAllSkills(bool value)
{
    m_recalcAllSkills = value;
}

bool zRPGObj::GetParams(const char*, int, int&, int*) const
{
    return false;
}

int zRPGObj::GetExtInfoLen() const
{
    return 0;
}

void zRPGObj::BeginArray(int)
{
}

void zRPGObj::SetOwnerFrameWork(iRPGFrameWork* frameWork)
{
    m_pOwnerFW = frameWork;
}

int zRPGObj::GetMaxInvAmount() const
{
    iRPGFrameWork* frameWork = const_cast<zRPGObj*>(this)->GetFW();
    if ((frameWork->GetArcadeMod_()&1) != 0)
        return MultiplySigned32Wrap(GetMaxNumInCell(), 2);
    return GetSkillValue(4);
}

bool zRPGObj::GetDirty()
{
    const bool result = m_dirty != 0;
    m_dirty = 0;
    return result;
}

bool zRPGObj::GetAmmoDirty()
{
    const bool result = m_ammoDirty != 0;
    m_ammoDirty = 0;
    return result;
}

void zRPGObj::SetAmmoDirty()
{
    m_ammoDirty = 1;
}

void zRPGObj::SetDirty()
{
    m_dirty = 1;
}

iRPGFrameWork* zRPGObj::GetFW()
{
    return m_pOwnerFW;
}

zRPGObj::zRPGObj(OBJ_TYPE typeValue, int paramsCount)
    : zGroundItem(typeValue),
      m_iParamsCount(paramsCount), m_recalcAllSkills(0), m_suspendSkillRecalc(0), m_pOwnerFW(0), m_clone(0),
      m_changeableParams(0), m_baseParams(0), m_arrayParams(0), m_baseSkills(0), m_skills(0), m_perkSkill(-1), m_dirty(1), m_ammoDirty(1)
{
    if (m_iParamsCount != 0)
    {
        m_baseSkills = static_cast<int*>(CrtAllocate(static_cast<unsigned int>(m_iParamsCount) * sizeof(*m_baseSkills)));
        m_skills = static_cast<int*>(CrtAllocate(static_cast<unsigned int>(m_iParamsCount) * sizeof(*m_skills)));
    }
    for (int paramIndex = 0; paramIndex < m_iParamsCount; ++paramIndex)
    {
        m_baseSkills[paramIndex] = 0;
        m_skills[paramIndex] = 0;
    }
    for (int parentIndex = 0; parentIndex < MAX_PARENT_CNT; ++parentIndex)
        m_apParentObj[parentIndex] = nullptr;
}

zRPGObj::~zRPGObj()
{
    for (int parentIndex = 0; parentIndex != MAX_PARENT_CNT; ++parentIndex)
    {
        if (m_apParentObj[parentIndex] != nullptr)
            RemoveParent(m_apParentObj[parentIndex], true);
    }
    if (m_baseSkills != nullptr)
    {
        CrtFreePointer(m_baseSkills);
        m_baseSkills = nullptr;
    }
    if (m_skills != nullptr)
    {
        CrtFreePointer(m_skills);
        m_skills = nullptr;
    }
}

#include "myerror.h"

void zRPGObj::AddChild(iRPGObj*)
{
    DebugAssert(5, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 496);
}

void zRPGObj::RemoveChild(iRPGObj*)
{
    DebugAssert(5, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 504);
}

void zRPGObj::OutputExtInfoStr(const char*, int, char*&, int) const
{
    DebugAssert(5, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 529);
}

void zRPGObj::OnSkillChange(int, int, int)
{
    SetDirty();
}

int zRPGObj::Damage(int)
{
    DebugAssert(5, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 564);
    return 0;
}

iRPGObj* zRPGObj::GetParent(int _iNum)
{
    if (_iNum>MAX_PARENT_CNT)
        DebugAssert(5, "_iNum <= MAX_PARENT_CNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 642);
    return m_apParentObj[_iNum];
}

void zRPGObj::IncreasedByImplant(int, bool)
{
    DebugAssert(5, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 651);
}

bool zRPGObj::IfIncreasedByImplant(int)
{
    DebugAssert(5, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 660);
    return 0;
}

bool zRPGObj::ItemIsNotReal()
{
    bool unrealEquipment = false;
    const int objectType = GetGameObj();
    if (objectType == TYPE_EQUIP)
        unrealEquipment = GetSkillValue(objectType) != 0;
    return GetId() != 0 || unrealEquipment;
}

iRPGObj* zRPGObj::GetObjCopy()
{
    if (!m_pOwnerFW)
        DebugAssert(5, "m_pOwnerFW", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 683);
    return m_pOwnerFW->GetObjCopy(this);
}

void zRPGObj::SetDataArrays()
{
}

void zRPGObj::InitParams()
{
}

void zRPGObj::Init()
{
    SetDataArrays();
    InitParams();
    RecalcSkills(true);
}

bool zRPGObj::SetSkillValue(int _iParamNum, int value)
{
    if (_iParamNum < 0 || _iParamNum >= m_iParamsCount)
        DebugAssert(5,             "_iParamNum >= 0 && _iParamNum < m_iParamsCount",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 115);
    const int oldValue = m_baseSkills[_iParamNum];
    if (oldValue == value && !m_arrayParams[_iParamNum])
        return false;
    OnBaseSkillChange(_iParamNum, oldValue, value);
    return m_baseSkills[_iParamNum] != oldValue;
}

int zRPGObj::GetSkillValue(int _iParamNum) const
{
    if (_iParamNum < 0 || _iParamNum >= m_iParamsCount)
        DebugAssert(5,             "_iParamNum >= 0 && _iParamNum < m_iParamsCount",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 173);
    return m_skills[_iParamNum];
}

int zRPGObj::GetSkillValueBase(int _iParamNum) const
{
    if (_iParamNum < 0 || _iParamNum >= m_iParamsCount)
        DebugAssert(5,             "_iParamNum >= 0 && _iParamNum < m_iParamsCount",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 182);
    return m_baseSkills[_iParamNum];
}

void zRPGObj::SetCharValue(int stringParam, const char* value)
{
    m_stringValues[stringParam] = value;
    SetDirty();
}

const char* zRPGObj::GetCharValue(int _Param) const
{
    if (_Param<0 || _Param >= STR_PARAM_CNT)
        DebugAssert(5, "_Param >= 0 && _Param < STR_PARAM_CNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 200);
    if (m_stringValues[_Param].IsEmpty())
        return 0;
    return static_cast<const char*>(m_stringValues[_Param]);
}

void zRPGObj::RecalcSkills()
{
    RecalcSkills(false);
}

void zRPGObj::SetChangeableParam(int _iParamNum)
{
    if (_iParamNum < 0 || _iParamNum >= m_iParamsCount)
        DebugAssert(5,             "_iParamNum >= 0 && _iParamNum < m_iParamsCount",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 289);
    m_changeableParams[_iParamNum] = true;
}

void zRPGObj::SetBaseParam(int _iParamNum)
{
    if (_iParamNum < 0 || _iParamNum >= m_iParamsCount)
        DebugAssert(5,             "_iParamNum >= 0 && _iParamNum < m_iParamsCount",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 297);
    m_baseParams[_iParamNum] = true;
}

void zRPGObj::SetArrayParam(int _iParamNum)
{
    if (_iParamNum < 0 || _iParamNum >= m_iParamsCount)
        DebugAssert(5,             "_iParamNum >= 0 && _iParamNum < m_iParamsCount",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 305);
    m_arrayParams[_iParamNum] = true;
}

void zRPGObj::SetPerkSkill(int value)
{
    const int oldValue = m_perkSkill;
    if (oldValue == value)
        return;
    if (oldValue >= 0)
        (void)IncSkillValue(oldValue, -10);
    m_perkSkill = value;
    if (value >= 0)
        (void)IncSkillValue(value, 10);
    SetDirty();
}

int zRPGObj::GetPerkSkill()
{
    const int currentPerk = m_perkSkill;
    if (GetGameObj() != TYPE_HERO)
        return currentPerk;
    if (currentPerk >= 0 && GetSkillValue(currentPerk) != 0)
        return currentPerk;
    for (int perk = RPGHERO_PERK_HEALING; perk <= RPGHERO_PERK_HANDTOHAND; ++perk)
        if (GetSkillValue(perk) > 0)             return perk;
    return currentPerk;
}

void zRPGObj::RemoveParent(iRPGObj* _pParentObj, bool noRefresh)
{
    if (!_pParentObj)
        DebugAssert(4, "_pParentObj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 472);
    int index = 0;
    while (index<MAX_PARENT_CNT && m_apParentObj[index] != _pParentObj)
        ++index;
    if (index == MAX_PARENT_CNT)
        DebugAssert(4, "ret", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 485);
    else
    {
        m_apParentObj[index]->RemoveChild(this);
        m_apParentObj[index] = 0;
    }
    if (!noRefresh)
        RecalcSkills();
}

void zRPGObj::OnBaseSkillChange(int paramIndex, int, int value)
{
    m_baseSkills[paramIndex] = value;
    if (m_suspendSkillRecalc == 0)
    {
        if (m_baseParams[paramIndex])
        {
            RecalcSkills();
            SetDirty();
            return;
        }
        RecalcSkill(paramIndex, false);
    }
    SetDirty();
}

void zRPGObj::SetName(const char* value, bool notifyOwner)
{
    if (IsBaseObject() && notifyOwner)
        m_pOwnerFW->AddBaseName(value, this);
    m_name = value;
}

void zRPGObj::RecalcSkills(bool initializing)
{
    SetRecalcAllSkills(true);
    for (int paramIndex = 0; paramIndex < m_iParamsCount; ++paramIndex)
        RecalcSkill(paramIndex, initializing);
    SetRecalcAllSkills(false);
}

void zRPGObj::RecalcSkill(int paramIndex, bool initializing)
{
    const int oldValue = m_skills[paramIndex];
    const int delta = GetAddonSkillValueFromScript(paramIndex);
    int newValue = AddSigned32Wrap(m_baseSkills[paramIndex], delta);
    if (CantExeed200(paramIndex) && delta > 0 && newValue > 200)
        newValue = 200;
    if (oldValue == newValue && initializing == m_arrayParams[paramIndex])
        return;
    m_skills[paramIndex] = newValue;
    OnSkillChange(paramIndex, oldValue, newValue);
}

void zRPGObj::CopyParametersFrom(iRPGObj* _BaseObj)
{
    const int sourceType = _BaseObj->GetGameObj();
    const int ownType = GetGameObj();
    if (sourceType != ownType)
        DebugAssert(5, "_BaseObj->GetType() == this->GetType()",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 262);
    m_name = _BaseObj->GetName();
    zGroundItem* const sourceGround =         static_cast<zGroundItem*>(_BaseObj->QueryInterface(33));
    zGroundItem::CopyParametersFrom(sourceGround);
    int paramsCount = m_iParamsCount;
    m_suspendSkillRecalc = 1;
    for (int paramIndex = 0; paramIndex < paramsCount; ++paramIndex)
    {
        SetSkillValue(paramIndex, _BaseObj->GetSkillValueBase(paramIndex));
        paramsCount = m_iParamsCount;
    }
    m_suspendSkillRecalc = 0;
    RecalcSkills();
    for (int stringParam = 0; stringParam < STR_PARAM_CNT; ++stringParam)
    {
        SetCharValue(stringParam, _BaseObj->GetCharValue(stringParam));
    }
    SetDirty();
}

void zRPGObj::AddParent(iRPGObj* _pParentObj)
{
    if (!_pParentObj)
        DebugAssert(4, "_pParentObj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 450);
    int EmptyParent = -1;
    for (int parentIndex = 0; parentIndex < MAX_PARENT_CNT; ++parentIndex)
    {
        if (m_apParentObj[parentIndex] == _pParentObj)
            DebugAssert(4, "m_apParentObj[ i ] != _pParentObj", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 455);
        if (EmptyParent == -1 && m_apParentObj[parentIndex] == nullptr)
            EmptyParent = parentIndex;
    }
    if (EmptyParent<0)
        DebugAssert(5, "EmptyParent >= 0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 459);
    m_apParentObj[EmptyParent] = _pParentObj;
    _pParentObj->AddChild(this);
    RecalcSkills();
}

void zRPGObj::AddItem(iRPGObj*)
{
    DebugAssert(4, "0",         "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.h", 243);
}

zInventory* zRPGObj::GetBPInv()
{
    DebugAssert(4, "0",         "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.h", 244);
    return 0;
}

void zRPGObj::ResetScriptFunctions()
{
    m_iFormulaHero = 0;
    m_iFormulaWeapon = 0;
    m_iFormulaArmor = 0;
}

int zRPGObj::GetAddonSkillValueFromScript(int _iParamNum)
{
    if (_iParamNum<0 || _iParamNum >= m_iParamsCount)
        DebugAssert(5, "_iParamNum >= 0 && _iParamNum < m_iParamsCount",             "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 314);
    if ((m_recalcAllSkills == 0 || GetGameObj() != TYPE_HERO) && !m_changeableParams[_iParamNum])
        return 0;
    if (m_iFormulaHero == 0)
    {
        const int heroFormula = m_pOwnerFW->ScriptGetNFunction("GetAddonSkillFormulaForHero");
        m_iFormulaHero = heroFormula;
    }
    if (m_iFormulaWeapon == 0)
    {
        const int weaponFormula = m_pOwnerFW->ScriptGetNFunction("GetAddonSkillFormulaForWeapon");
        m_iFormulaWeapon = weaponFormula;
    }
    if (m_iFormulaArmor == 0)
    {
        const int armorFormula = m_pOwnerFW->ScriptGetNFunction("GetAddonSkillFormulaForArmor");
        m_iFormulaArmor = armorFormula;
    }
    const int type = GetGameObj();
    int formula = -1;
    if (type == TYPE_HERO)
        formula = m_iFormulaHero;
    else if (type == TYPE_WEAPON)
        formula = m_iFormulaWeapon;
    else if (type == TYPE_ARMOR)
        formula = m_iFormulaArmor;
    else return 0;
    if (formula == -1)
        return 0;
    const int branchType = GetGameObj();
    if (branchType == TYPE_WEAPON || branchType == TYPE_ARMOR)
    {
        if (!m_changeableParams[_iParamNum])
            return 0;
        const int parentValue = m_apParentObj[0]             ? static_cast<int>(m_apParentObj[0]->GetExtType()) : -1;
        const int key = static_cast<int>(GetExtType());
        return m_pOwnerFW->ScriptRun(formula, "%d%d%d", key, parentValue, _iParamNum);
    }
    if (branchType != TYPE_HERO)
        return 0;
    int result = 0;
    if (m_changeableParams[_iParamNum])
    {
        const int key = static_cast<int>(GetExtType());
        result = m_pOwnerFW->ScriptRun(formula, "%d%d", key, _iParamNum, 0);
    }
    int parentSum = 0;
    for (int parentIndex = 0; parentIndex < MAX_PARENT_CNT; ++parentIndex)
    {
        iRPGObj* parent = m_apParentObj[parentIndex];
        if (!parent)
            continue;
        bool include = CanHoldObj(parent);
        if (parent->GetGameObj() == TYPE_IMPLANT && parent->GetSkillValue(RPGIMPLANT_ENABLED) != 0 &&            LoadingHeroParameters)
            include = true;
        if (include)
            parentSum = AddSigned32Wrap(parentSum, parent->GetSkillValue(_iParamNum));
    }
    result = AddSigned32Wrap(result, parentSum);
    IncreasedByImplant(_iParamNum, parentSum != 0);
    return result;
}


const char* zRPGObj::GetName() const
{
    return static_cast<const char*>(m_name);
}

bool zRPGObj::IncSkillValue(int _iParamNum, int delta)
{
    if (_iParamNum<0 || _iParamNum >= m_iParamsCount)
        DebugAssert(5u, "_iParamNum >= 0 && _iParamNum < m_iParamsCount", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\RPGHero\\zRPGBaseObject.cpp", 150u);
    const int requestedValue = AddSigned32Wrap(m_baseSkills[_iParamNum], delta);
    const int scriptBonus = GetAddonSkillValueFromScript(_iParamNum);
    int effectiveValue = AddSigned32Wrap(requestedValue, scriptBonus);
    if (CantExeed200(_iParamNum) && scriptBonus>0 && effectiveValue>200)
        effectiveValue = 200;
    if (delta != 0 && effectiveValue >= 0)
        return SetSkillValue(_iParamNum, requestedValue);
    return false;
}

unsigned int __cdecl zRPGBaseObject_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

bool zRPGObj::CantExeed200(int paramIndex)
{
    if (GetGameObj() != TYPE_HERO)
        return false;
    return paramIndex < RPGHERO_FOR_IMPLANTS_PARAMS_COUNT ||         (paramIndex >= RPGHERO_PERK_HEALING && paramIndex <= RPGHERO_PERK_RESISTANCE);
}

#if defined(_MSC_VER) && defined(_M_IX86)
#include "basetype.h"
#endif

