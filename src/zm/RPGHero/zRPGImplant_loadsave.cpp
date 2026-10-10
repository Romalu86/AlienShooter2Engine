#include "mylib.h"
#include <cstring>

#include "zRPGImplant.h"
#include "basetype.h"

bool zRPGImplant::GetParams(const char* token, int length, int& parameterCount, int* parameterIndices) const
{
    parameterCount = 1;
    parameterIndices[0] = -1;
    const unsigned int tokenLength = static_cast<unsigned int>(length);
    if (std::strncmp("Level", token, tokenLength) == 0)
        parameterIndices[0] = RPGIMPLANT_LEVEL;
    else if (std::strncmp("Class", token, tokenLength) == 0)
        parameterIndices[0] = RPGIMPLANT_CLASS;
    else if (std::strncmp("NumSkill", token, tokenLength) == 0)
        parameterIndices[0] = RPGIMPLANT_NUMSKILL;
    else if (std::strncmp("MinMax", token, tokenLength) == 0)
    {
        parameterIndices[0] = RPGIMPLANT_ADDSKILLMIN;
        parameterIndices[1] = RPGIMPLANT_ADDSKILLMAX;
        parameterCount = 2;
    }
    else
        (void)zRPGHeroBase::GetParams(token, length, parameterCount, parameterIndices);
    return parameterIndices[0] != -1;
}

