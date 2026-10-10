#pragma once

#include "locale_facets.h"

struct RecordDBLocaleControl;
struct RecordDBIosControl;
struct RecordDBBadAllocControl;

namespace Msvc6
{
    // Each locale ID stores one unsigned word.
    // _Id_cnt supplies the shared sequence for newly assigned IDs.
    struct LocaleId
    {
        unsigned int value;
        static int _Id_cnt;
    };

    template<class Tag> struct FacetDataType;
    template<> struct FacetDataType<CharCtype> { typedef CtypeData Type; };
    template<> struct FacetDataType<CharNumGet> { typedef FacetData Type; };
    template<> struct FacetDataType<CharNumPut> { typedef FacetData Type; };
    template<> struct FacetDataType<CharNumpunct> { typedef NumpunctData<char> Type; };
    template<> struct FacetDataType<CharCodecvt> { typedef LocaleStampData Type; };
    template<> struct FacetDataType<CharCollate> { typedef LocaleStampData Type; };
    template<> struct FacetDataType<CharMessages> { typedef MessagesData<char> Type; };
    template<> struct FacetDataType<CharMoneyGet> { typedef FacetData Type; };
    template<> struct FacetDataType<CharMoneyPut> { typedef FacetData Type; };
    template<> struct FacetDataType<CharMoneypunct> { typedef MoneypunctData<char> Type; };
    template<> struct FacetDataType<CharMoneypunctIntl> { typedef MoneypunctData<char> Type; };
    template<> struct FacetDataType<CharTimeGet> { typedef TimeGetData<char> Type; };
    template<> struct FacetDataType<CharTimePut> { typedef TimePutData Type; };
    template<> struct FacetDataType<WideCtype> { typedef CtypeData Type; };
    template<> struct FacetDataType<WideNumGet> { typedef FacetData Type; };
    template<> struct FacetDataType<WideNumPut> { typedef FacetData Type; };
    template<> struct FacetDataType<WideNumpunct> { typedef NumpunctData<unsigned short> Type; };
    template<> struct FacetDataType<WideCodecvt> { typedef LocaleStampData Type; };
    template<> struct FacetDataType<WideCollate> { typedef LocaleStampData Type; };
    template<> struct FacetDataType<WideMessages> { typedef MessagesData<unsigned short> Type; };
    template<> struct FacetDataType<WideMoneyGet> { typedef FacetData Type; };
    template<> struct FacetDataType<WideMoneyPut> { typedef FacetData Type; };
    template<> struct FacetDataType<WideMoneypunct> { typedef MoneypunctData<unsigned short> Type; };
    template<> struct FacetDataType<WideMoneypunctIntl> { typedef MoneypunctData<unsigned short> Type; };
    template<> struct FacetDataType<WideTimeGet> { typedef TimeGetData<unsigned short> Type; };
    template<> struct FacetDataType<WideTimePut> { typedef TimePutData Type; };

    // Each facet has a distinct ID, a non-owning lookup cache,
    // and an owner retained until shutdown. These are separate
    // static objects with independent lifetimes.
    template<class Tag> struct FacetState
    {
        static LocaleId id;
        static const FacetData* _Psave;
        static typename FacetDataType<Tag>::Type* _Facsav;
    };

    template<class Tag> struct LocaleFacetUse
    {
        static const typename FacetDataType<Tag>::Type* Get(const RecordDBLocaleControl& locale);
    };

    extern RecordDBLocaleControl ClassicLocale;
    extern RecordDBIosControl* RegisteredStreams[9];
    extern unsigned char StreamRegistrations[9];

    // First-use construction and the native atexit callback own this object.
    // Accessing its storage does not construct it or register another cleanup.
    RecordDBBadAllocControl& BadAllocationObject();
}
