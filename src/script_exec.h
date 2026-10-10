#pragma once
#include "mylib.h"


#include "basetype.h"


void __cdecl ScriptExecReleaseGlobalString();

unsigned int SrcScriptExec_StaticCleanupRegister01();
unsigned int __cdecl InterpolatePackedRGB(unsigned int from, unsigned int to, int factor);
unsigned char __cdecl InterpolateSignedByteChannel(int from, int to, int factor);
unsigned int __cdecl StaticAtexitGuard();


class SPRITE;
extern SPRITE* g_pFucking;
