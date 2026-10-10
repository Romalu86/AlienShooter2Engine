#pragma once

struct RecordDBCtypeDescriptor
{
    unsigned int localeLow;
    unsigned int localeHigh;
    unsigned short* table;
    unsigned int ownsTable;
};

// Physical facet data shared by the VC6-compatible owners and their operations.

namespace Msvc6
{
    struct FacetVirtualTable;
    const FacetVirtualTable* BaseFacetTable();
    const FacetVirtualTable* LocaleImplTable();

    struct FacetData
    {
        const FacetVirtualTable* vftable;
        unsigned int references;
    };

    typedef ::RecordDBCtypeDescriptor CtypeDescriptor;

    struct CtypeData : FacetData
    {
        CtypeDescriptor descriptor;
    };

    struct LocaleStampData : FacetData
    {
        unsigned int localeLow;
        unsigned int localeHigh;
    };

    template<class Character> struct NumpunctData : FacetData
    {
        char* grouping;
        Character decimalPoint;
        Character thousandsSeparator;
        Character* falseName;
        Character* trueName;
    };

    template<class Character> struct MoneypunctData : FacetData
    {
        char* grouping;
        Character decimalPoint;
        Character thousandsSeparator;
        Character* currencySymbol;
        Character* positiveSign;
        Character* negativeSign;
        int fractionDigits;
        unsigned char positivePattern[4];
        unsigned char negativePattern[4];
        unsigned char international;
    };

    template<class Character> struct MessagesData : FacetData
    {
        Character* negativeName;
        Character* positiveName;
    };

    template<class Character> struct TimeGetData : FacetData
    {
        Character* days;
        Character* months;
    };

    struct TimePutData : FacetData
    {
        void* timeNames;
    };

    struct CharCtype { static const FacetVirtualTable* GetTable(); };
    struct CharNumGet { static const FacetVirtualTable* GetTable(); };
    struct CharNumPut { static const FacetVirtualTable* GetTable(); };
    struct CharNumpunct { static const FacetVirtualTable* GetTable(); };
    struct CharCodecvt { static const FacetVirtualTable* GetTable(); };
    struct CharCollate { static const FacetVirtualTable* GetTable(); };
    struct CharMessages { static const FacetVirtualTable* GetTable(); };
    struct CharMoneyGet { static const FacetVirtualTable* GetTable(); };
    struct CharMoneyPut { static const FacetVirtualTable* GetTable(); };
    struct CharMoneypunct { static const FacetVirtualTable* GetTable(); };
    struct CharMoneypunctIntl { static const FacetVirtualTable* GetTable(); };
    struct CharMoneyBase { static const FacetVirtualTable* GetTable(); };
    struct CharTimeGet { static const FacetVirtualTable* GetTable(); };
    struct CharTimePut { static const FacetVirtualTable* GetTable(); };
    struct WideCtype { static const FacetVirtualTable* GetTable(); };
    struct WideNumGet { static const FacetVirtualTable* GetTable(); };
    struct WideNumPut { static const FacetVirtualTable* GetTable(); };
    struct WideNumpunct { static const FacetVirtualTable* GetTable(); };
    struct WideCodecvt { static const FacetVirtualTable* GetTable(); };
    struct WideCollate { static const FacetVirtualTable* GetTable(); };
    struct WideMessages { static const FacetVirtualTable* GetTable(); };
    struct WideMoneyGet { static const FacetVirtualTable* GetTable(); };
    struct WideMoneyPut { static const FacetVirtualTable* GetTable(); };
    struct WideMoneypunct { static const FacetVirtualTable* GetTable(); };
    struct WideMoneypunctIntl { static const FacetVirtualTable* GetTable(); };
    struct WideMoneyBase { static const FacetVirtualTable* GetTable(); };
    struct WideTimeGet { static const FacetVirtualTable* GetTable(); };
    struct WideTimePut { static const FacetVirtualTable* GetTable(); };
}
