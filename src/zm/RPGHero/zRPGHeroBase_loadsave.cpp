#include "mylib.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "zRPGHeroBase.h"
#include "zRPGBaseObject.h"
#include "basetype.h"

unsigned int __cdecl RPGImplantMaskedRandom30Bit();

bool zRPGHeroBase::GetParams(const char* token, int length, int& parameterCount, int* parameterIndices) const
{
    parameterCount = 1;
    parameterIndices[0] = -1;
    for (int parameterIndex = 0;parameterIndex<RPGHERO_PARAMS_COUNT;++parameterIndex)
    {
        const char* name = GetRPGHeroParamName(static_cast<unsigned int>(parameterIndex));
        if (name && std::strncmp(name, token, static_cast<unsigned int>(length)) == 0)
        {
            parameterIndices[0] = parameterIndex;
            break;
        }
    }
    return parameterIndices[0] != -1;
}

bool zRPGHeroBase::ParseUnknownCommand(const char* token, int valueCount,
                                       const char** values, iRPGFrameWork* framework)
{
    if (zRPGObj::ParseUnknownCommand(token, valueCount, values, framework))
        return true;
    if (std::strcmp(token, "Srand") != 0)
        return false;
    m_iSRandVal = BitsToSigned32(CrtAtoiText(values[0]));
    return true;
}

bool zRPGHeroBase::SaveToFile(std::FILE* output)
{
    zRPGObj* const object = static_cast<zRPGObj*>(this);
    for (unsigned int id = 0;id<RPGHERO_PARAMS_COUNT;++id)
    {
        const char* const label = GetRPGHeroParamName(id);
        if (!label)
            continue;
        const int value = object->GetSkillValueBase(static_cast<int>(id));
        (void)std::fprintf(output, "%s=%d\n", label, value);
    }
    return true;
}

unsigned int zRPGHeroBase::GetSRandVal(bool initializeIfZero)
{
    if (m_iSRandVal == 0 && initializeIfZero)
        m_iSRandVal = BitsToSigned32(RPGImplantMaskedRandom30Bit());
    return static_cast<unsigned int>(m_iSRandVal);
}


