#pragma once
#include <ctime>
#include <cstring>
#include "locale_facets.h"
#include "locale_strings.h"

struct RecordDBIosControl;
struct RecordDBLocaleControl;
namespace Msvc6Runtime { class fpos; }
using RecordDBPositionControl = Msvc6Runtime::fpos;

namespace Msvc6
{
    // Explicit x86 VC6 virtual-call ABI. The complete-object locator, when present,
    // occupies the pointer immediately before the first function slot.
    typedef void* (__thiscall *DeletingDestructor)(void*, unsigned int);
    struct FacetVirtualTable { DeletingDestructor destroy; };

    // These bound methods are nonvirtual x86 members with zero this adjustment.
    // Their entry pointer is the first field of the compiler's member pointer.
    template<class Function, class Method> Function NativeMethod(Method method)
    {
        Function entry;
        std::memcpy(&entry, &method, sizeof(entry));
        return entry;
    }
    template<class Owner> void BindFacetDestructor(const FacetVirtualTable* table,
        void* (__thiscall Owner::*method)(unsigned int))
    {
        const_cast<FacetVirtualTable*>(table)->destroy = NativeMethod<DeletingDestructor>(method);
    }
    inline void DeleteFacet(void* object)
    {
        if (object != 0)
        {
            FacetData* const facet = static_cast<FacetData*>(object);
            facet->vftable->destroy(object, 1u);
        }
    }

    struct StreambufVirtualTable;
    template<class Character> struct StreambufData
    {
        const StreambufVirtualTable* vftable;
        Character* getBegin;
        Character* putBegin;
        Character** getBeginSlot;
        Character** putBeginSlot;
        Character* getCursor;
        Character* putCursor;
        Character** getCursorSlot;
        Character** putCursorSlot;
        unsigned int getRemaining;
        unsigned int putRemaining;
        unsigned int* getRemainingSlot;
        unsigned int* putRemainingSlot;
        void* locale;
    };
    template<class Character> struct InputIterator
    {
        StreambufData<Character>* stream;
        unsigned char cached;
        Character value;
    };
    template<class Character> struct OutputIterator
    {
        unsigned char failed;
        StreambufData<Character>* stream;
    };
    struct MoneyPattern { unsigned char fields[4]; };

    struct CharCtypeVirtualTable : FacetVirtualTable
    {
        unsigned char* (__thiscall *lowerRange)(const void*, unsigned char*, unsigned char*);
        unsigned char (__thiscall *lower)(const void*, unsigned char);
        unsigned char* (__thiscall *upperRange)(const void*, unsigned char*, unsigned char*);
        unsigned char (__thiscall *upper)(const void*, unsigned char);
    };
    struct WideCtypeVirtualTable : FacetVirtualTable
    {
        const unsigned short* (__thiscall *classifyRange)(const void*, const unsigned short*, const unsigned short*, unsigned short*);
        bool (__thiscall *classify)(const void*, unsigned short, unsigned short);
        const unsigned short* (__thiscall *scanIs)(const void*, unsigned short, const unsigned short*, const unsigned short*);
        const unsigned short* (__thiscall *scanNot)(const void*, unsigned short, const unsigned short*, const unsigned short*);
        const unsigned short* (__thiscall *lowerRange)(const void*, unsigned short*, const unsigned short*);
        unsigned short (__thiscall *lower)(const void*, unsigned short);
        const unsigned short* (__thiscall *upperRange)(const void*, unsigned short*, const unsigned short*);
        unsigned short (__thiscall *upper)(const void*, unsigned short);
        const char* (__thiscall *widenRange)(const void*, const char*, const char*, unsigned short*);
        unsigned short (__thiscall *widen)(const void*, char);
        const unsigned short* (__thiscall *narrowRange)(const void*, const unsigned short*, const unsigned short*, char, char*);
        char (__thiscall *narrow)(const void*, unsigned short, char);
    };

    template<class Character> struct NumGetVirtualTable : FacetVirtualTable
    {
        InputIterator<Character>* (__thiscall *getPointer)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, void**);
        InputIterator<Character>* (__thiscall *getLongDouble)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, long double*);
        InputIterator<Character>* (__thiscall *getDouble)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, double*);
        InputIterator<Character>* (__thiscall *getFloat)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, float*);
        InputIterator<Character>* (__thiscall *getUnsignedLong)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, unsigned long*);
        InputIterator<Character>* (__thiscall *getLong)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, long*);
        InputIterator<Character>* (__thiscall *getUnsignedInt)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, unsigned int*);
        InputIterator<Character>* (__thiscall *getUnsignedShort)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, unsigned short*);
        InputIterator<Character>* (__thiscall *getBoolean)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, bool*);
    };
    template<class Character> struct NumPutVirtualTable : FacetVirtualTable
    {
        OutputIterator<Character>* (__thiscall *putPointer)(const void*, OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl*, Character, const void*);
        OutputIterator<Character>* (__thiscall *putLongDouble)(const void*, OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl*, Character, long double);
        OutputIterator<Character>* (__thiscall *putDouble)(const void*, OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl*, Character, double);
        OutputIterator<Character>* (__thiscall *putUnsignedLong)(const void*, OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl*, Character, unsigned long);
        OutputIterator<Character>* (__thiscall *putLong)(const void*, OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl*, Character, long);
        OutputIterator<Character>* (__thiscall *putBoolean)(const void*, OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl*, Character, bool);
    };
    template<class Character> struct NumpunctVirtualTable : FacetVirtualTable
    {
        Character (__thiscall *decimalPoint)(const void*);
        Character (__thiscall *thousandsSeparator)(const void*);
        StringData<char>* (__thiscall *grouping)(const void*, StringData<char>*);
        StringData<Character>* (__thiscall *falseName)(const void*, StringData<Character>*);
        StringData<Character>* (__thiscall *trueName)(const void*, StringData<Character>*);
    };
    template<class Character> struct CodecvtVirtualTable : FacetVirtualTable
    {
        bool (__thiscall *alwaysNoConversion)(const void*);
        int (__thiscall *maxLength)(const void*);
        int (__thiscall *encoding)(const void*);
        int (__thiscall *in)(const void*, int*, const char*, const char*, const char**, Character*, Character*, Character**);
        int (__thiscall *out)(const void*, int*, const Character*, const Character*, const Character**, char*, char*, char**);
        int (__thiscall *length)(const void*, int*, const Character*, const Character*, unsigned int);
    };
    template<class Character> struct CollateVirtualTable : FacetVirtualTable
    {
        int (__thiscall *compare)(const void*, const Character*, const Character*, const Character*, const Character*);
        StringData<Character>* (__thiscall *transform)(const void*, StringData<Character>*, const Character*, const Character*);
        long (__thiscall *hash)(const void*, const Character*, const Character*);
    };
    template<class Character> struct MessagesVirtualTable : FacetVirtualTable
    {
        int (__thiscall *open)(const void*, const StringData<char>*, const RecordDBLocaleControl*);
        StringData<Character>* (__thiscall *get)(const void*, StringData<Character>*, int, int, int, const StringData<Character>*);
        void (__thiscall *close)(const void*, int);
    };
    template<class Character> struct MoneyGetVirtualTable : FacetVirtualTable
    {
        InputIterator<Character>* (__thiscall *getString)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, bool, RecordDBIosControl*, unsigned int*, StringData<Character>*);
        InputIterator<Character>* (__thiscall *getNumber)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, bool, RecordDBIosControl*, unsigned int*, long double*);
    };
    template<class Character> struct MoneyPutVirtualTable : FacetVirtualTable
    {
        OutputIterator<Character>* (__thiscall *putString)(const void*, OutputIterator<Character>*, OutputIterator<Character>, bool, RecordDBIosControl*, Character, const StringData<Character>*);
        OutputIterator<Character>* (__thiscall *putNumber)(const void*, OutputIterator<Character>*, OutputIterator<Character>, bool, RecordDBIosControl*, Character, long double);
    };
    template<class Character> struct MoneypunctVirtualTable : FacetVirtualTable
    {
        Character (__thiscall *decimalPoint)(const void*);
        Character (__thiscall *thousandsSeparator)(const void*);
        StringData<char>* (__thiscall *grouping)(const void*, StringData<char>*);
        StringData<Character>* (__thiscall *currencySymbol)(const void*, StringData<Character>*);
        StringData<Character>* (__thiscall *positiveSign)(const void*, StringData<Character>*);
        StringData<Character>* (__thiscall *negativeSign)(const void*, StringData<Character>*);
        int (__thiscall *fractionDigits)(const void*);
        MoneyPattern* (__thiscall *positivePattern)(const void*, MoneyPattern*);
        MoneyPattern* (__thiscall *negativePattern)(const void*, MoneyPattern*);
    };
    template<class Character> struct TimeGetVirtualTable : FacetVirtualTable
    {
        int (__thiscall *dateOrder)(const void*);
        InputIterator<Character>* (__thiscall *getTime)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, std::tm*);
        InputIterator<Character>* (__thiscall *getDate)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, std::tm*);
        InputIterator<Character>* (__thiscall *getWeekday)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, std::tm*);
        InputIterator<Character>* (__thiscall *getMonth)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, std::tm*);
        InputIterator<Character>* (__thiscall *getYear)(const void*, InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl*, unsigned int*, std::tm*);
    };
    template<class Character> struct TimePutVirtualTable : FacetVirtualTable
    {
        OutputIterator<Character>* (__thiscall *put)(const void*, OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl*, const std::tm*, char, char);
    };

    struct ExceptionVirtualTable
    {
        DeletingDestructor destroy;
        const char* (__thiscall *what)(const void*);
    };
    struct RaisedExceptionVirtualTable : ExceptionVirtualTable
    {
        void (__thiscall *raise)(const void*);
    };
    struct StreambufVirtualTable
    {
        DeletingDestructor destroy;
        int (__thiscall *overflow)(void*, int);
        int (__thiscall *putbackFailure)(void*, int);
        int (__thiscall *available)(void*);
        int (__thiscall *underflow)(void*);
        int (__thiscall *uflow)(void*);
        int (__thiscall *read)(void*, char*, int);
        int (__thiscall *write)(void*, const char*, int);
        RecordDBPositionControl* (__thiscall *seekOffset)(void*, RecordDBPositionControl*, int, int, unsigned int);
        RecordDBPositionControl* (__thiscall *seekPosition)(void*, RecordDBPositionControl*, RecordDBPositionControl, unsigned int);
        void* (__thiscall *setBuffer)(void*, char*, int);
        int (__thiscall *sync)(void*);
    };

    const ExceptionVirtualTable* ExceptionTable();
    const ExceptionVirtualTable* BadCastTable();
    const ExceptionVirtualTable* LogicErrorTable();
    const ExceptionVirtualTable* LengthErrorTable();
    const ExceptionVirtualTable* OutOfRangeTable();
    const ExceptionVirtualTable* RuntimeErrorTable();
    const ExceptionVirtualTable* IosFailureTable();
    const ExceptionVirtualTable* BadAllocTable();
    const StreambufVirtualTable* StreambufTable();
    const StreambufVirtualTable* FilebufTable();
    const FacetVirtualTable* IosBaseTable();
    const FacetVirtualTable* IosTable();
    const FacetVirtualTable* IstreamTable();
    const FacetVirtualTable* OstreamTable();
    const FacetVirtualTable* IfstreamTable();
    const FacetVirtualTable* OfstreamTable();

    struct VirtualBaseTable { int offsetToTop; int virtualBaseOffset; };
    extern const VirtualBaseTable IstreamVirtualBase;
    extern const VirtualBaseTable OstreamVirtualBase;
    extern const VirtualBaseTable IfstreamVirtualBase;
    extern const VirtualBaseTable OfstreamVirtualBase;

    struct NativeThrowInfo;
    const NativeThrowInfo* BadCastThrowInfo();
    const NativeThrowInfo* LogicErrorThrowInfo();
    const NativeThrowInfo* LengthErrorThrowInfo();
    const NativeThrowInfo* OutOfRangeThrowInfo();
    const NativeThrowInfo* RuntimeErrorThrowInfo();
    const NativeThrowInfo* IosFailureThrowInfo();
    const NativeThrowInfo* BadAllocThrowInfo();
    void ThrowException(void* object, const NativeThrowInfo* information);

    void InitializeLocaleVirtualTables();
}
