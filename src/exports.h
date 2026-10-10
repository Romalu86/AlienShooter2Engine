#ifndef ALIENSHOOTER_EXPORTS_H
#define ALIENSHOOTER_EXPORTS_H

#include "basetype.h"
#include <cstdio>

extern char* crtProgramPath;

void __stdcall GetRegistrationInformation(char* buffer);
void __cdecl SFINIT0_MonsterVidAllocate(void);
void __cdecl SFINIT1_Proc(void);
void __cdecl SFINIT2_Proc(void);
int __cdecl SFLB_GetHeroId(int value);


unsigned int __cdecl Msvc6LengthError();
unsigned int __cdecl Msvc6RangeError();
void __cdecl Msvc6NoMemory();
namespace Msvc6 { std::FILE* __cdecl Fiopen(const char* path, int openMode); }
unsigned int __cdecl Msvc6ToLower(unsigned int value,unsigned int );
unsigned int __cdecl Msvc6ToUpper(unsigned int value,unsigned int );
unsigned long long __cdecl Msvc6LocaleStamp();
unsigned long long __cdecl Msvc6CollateStamp();
unsigned int __cdecl Msvc6ByteToWide(unsigned int value);
unsigned int __cdecl Msvc6GetDays();
unsigned int __cdecl Msvc6GetMonths();
unsigned int __cdecl Msvc6GetTimeNames();
struct RecordDBCtypeDescriptor;
int __cdecl Msvc6MbrToWide(unsigned short* destination, const char* source,
    unsigned int count, unsigned int* state, const RecordDBCtypeDescriptor* descriptor);
unsigned int __cdecl Msvc6GetPos(unsigned int file,unsigned int out32);
unsigned int __cdecl Msvc6SetPos(unsigned int file,unsigned int pos32);
unsigned int __cdecl Msvc6SetVbuf(unsigned int file,unsigned int buffer,unsigned int mode,unsigned int size);
unsigned int __cdecl Msvc6CtypeDescriptor(unsigned int out32);

unsigned int  Msvc6BasicIosClear(void* self,unsigned int state,unsigned int rethrow);
unsigned int  Msvc6BasicIosInit(void* self);
unsigned int  Msvc6BasicIosDestruct(void* self);
unsigned int  Msvc6StreamRegistration(void* self);
unsigned int __cdecl Msvc6ExceptionInFlight();
unsigned int __cdecl Msvc6GlobalLocale();
unsigned int  Msvc6LocaleGetFacet(void* locale,unsigned int id,unsigned int create);
unsigned int  Msvc6LocaleCanSupplyFacet(void* locale);
unsigned int  Msvc6LocaleAddFacet(void* locale,unsigned int facet,unsigned int id,unsigned int replaceName);
unsigned int  Msvc6LocaleConstruct(void* locale,unsigned int );
unsigned int  Msvc6LocaleDestruct(void* locale);

#endif
