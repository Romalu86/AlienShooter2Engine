#include "basetype.h"
#include "../engine_globals.h"
#include "zm/Utils/zLogConsole.h"

void __cdecl DebugAssert(LOG_TYPE type, char* expression, char* sourceFile, int sourceLine)
{
    iLogConsole* const logger =
        ::gpLogCons;
    if (logger != 0)
    {
        logger->LogStr(type, "Assert(%s) in file %s line %d",
            expression, sourceFile, sourceLine);
    }
}

void __cdecl DebugAssert(int severity, const char* expression, const char* sourceFile, int sourceLine)
{
    DebugAssert(static_cast<LOG_TYPE>(severity), const_cast<char*>(expression),
        const_cast<char*>(sourceFile), sourceLine);
}
