#pragma once
#include "basetype.h"

class SPRITE_COLLECTOR;
extern SPRITE_COLLECTOR* Sprites;
class VID;
class NET;
extern NET* Net;
extern unsigned long PrevRealCurrentTime;
extern unsigned long RealCurrentTime;
extern unsigned long PrevCurrentTime;
extern unsigned long CurrentTime;
extern int g_iZmDebug;
class iLogConsole;
class SOUND;
class REGISTRY;
class CONSTANT;
class PROFILE;
class MOUSE;
class zProfLdr;
class zDlgSrv;
class iIniParser;
struct ADD_CREATE_SPRITE;

extern VID* EmptyVid;
extern SOUND* Sound;
extern REGISTRY* Registry;
extern CONSTANT* Const;
extern PROFILE* Strings;
extern MOUSE* Mouse;
extern int NoBuildAlpha;
extern ADD_CREATE_SPRITE* AddCreateSprite;
extern zProfLdr* g_pProfLdr;
extern zDlgSrv* g_pDlgSrv;
extern iLogConsole* gpLogCons;
extern iIniParser* g_pParser;
extern bool* g_abMonsterVid;
extern int* g_aiNamedMonstersLast;
extern void (__cdecl *DrawLoadScreenTact)(int);
