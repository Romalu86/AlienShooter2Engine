#pragma once

#include <cstdio>

#include <stddef.h>
#include "../Game/zGroundItem.h"
#include "../Utils/String/zString.h"

#if !defined(_MSC_VER) && !defined(__thiscall)
#  define __thiscall
#endif

class iRPGObj;
class iRPGFrameWork;
class zInventory;

class zRPGObj : public zGroundItem
{
public:
    enum { MAX_PARENT_CNT = 4 };

    int m_iParamsCount;
    unsigned char m_recalcAllSkills;
    unsigned char m_suspendSkillRecalc;
    unsigned char m_skillFlagsPadding[2];
    iRPGFrameWork* m_pOwnerFW;
    int m_clone;
    bool* m_changeableParams;
    bool* m_baseParams;
    bool* m_arrayParams;
    int* m_baseSkills;
    int* m_skills;
    zString m_name;
    zString m_stringValues[STR_PARAM_CNT];
    int m_perkSkill;
    unsigned char m_dirty;
    unsigned char m_ammoDirty;
    unsigned char m_dirtyPadding[2];
    iRPGObj* m_apParentObj[MAX_PARENT_CNT];

    zRPGObj(OBJ_TYPE typeValue, int paramsCount);
    ~zRPGObj() override;

    bool SetSkillValue(int _iParamNum, int value) override;
    bool IncSkillValue(int _iParamNum, int delta) override;
    int GetSkillValue(int _iParamNum) const override;
    int GetSkillValueBase(int _iParamNum) const override;
    void SetCharValue(int stringParam, const char* value) override;
    const char* GetCharValue(int _Param) const override;
    void RecalcSkills() override;
    bool GetParams(const char* token, int length, int& count, int* indices) const override;
    void CopyParametersFrom(iRPGObj* _BaseObj) override;
    void SetPerkSkill(int value) override;
    int GetPerkSkill() override;
    bool GetDirty() override;
    bool GetAmmoDirty() override;
    void AddParent(iRPGObj* _pParentObj) override;
    void RemoveParent(iRPGObj* _pParentObj, bool noRefresh) override;
    void AddChild(iRPGObj* child) override;
    void RemoveChild(iRPGObj* child) override;
    int GetExtInfoLen() const override;
    void OutputExtInfoStr(const char* prefix, int mode, char*& out, int flags) const override;
    void Init() override;
    int Damage(int value) override;
    void BeginArray(int index) override;
    bool ParseUnknownCommand(const char* name, int count, const char** values, iRPGFrameWork* frameWork) override;
    bool ParseUnknownCommand(const char* name, int count, const int* values) override;
    bool IsArrayProperty(int index) override;
    void SetRecalcAllSkills(bool value) override;
    void SetOwnerFrameWork(iRPGFrameWork* frameWork) override;
    iRPGFrameWork* GetFW() override;
    bool IfIncreasedByImplant(int paramIndex) override;
    bool CanHoldObj(const iRPGObj* object) const override;
    bool SaveToFile(std::FILE* output) override;
    void SetClone(int clone) override;
    int GetClone() const override;
    bool ItemIsNotReal() override;
    iRPGObj* GetObjCopy() override;
    void SetName(const char* value, bool notifyOwner) override;
    const char* GetName() const override;
    void SetAmmoDirty() override;
    void AddItem(iRPGObj* item) override;
    zInventory* GetBPInv() override;
    int GetMaxInvAmount() const override;

    virtual void OnSkillChange(int paramIndex, int oldValue, int newValue);
    virtual void OnBaseSkillChange(int paramIndex, int oldValue, int value);
    virtual void InitParams();
    virtual void IncreasedByImplant(int paramIndex, bool increased);
    virtual void SetDataArrays();

    void SetDirty();
    static void ResetScriptFunctions();
    iRPGObj* GetParent(int _iNum);

    bool CantExeed200(int paramIndex);
    void SetChangeableParam(int _iParamNum);
    void SetBaseParam(int _iParamNum);
    void SetArrayParam(int _iParamNum);
    void RecalcSkills(bool initializing);
    void RecalcSkill(int paramIndex, bool initializing);
    int GetAddonSkillValueFromScript(int _iParamNum);

private:
    static int m_iFormulaHero;
    static int m_iFormulaWeapon;
    static int m_iFormulaArmor;
};

typedef zRPGObj zRPGBaseObject;


