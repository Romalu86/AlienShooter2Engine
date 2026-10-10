#include "locale_tables.h"
#include "locale_operations.h"
#include "crt_locale.h"
#include "zm/Game/RecordDB/RecordDBStreambufControl.h"
#include "basetype.h"
#include "mylib.h"
#include "exports.h"
#include <cstdint>

unsigned int RecordDBBasicIosDestructor(void*);
void RecordDBCompilerAdjustVtableMinus8(void*);
void basic_ostream_Dtor(void*);
void* RecordDBBasicIfstreamScalarDeletingDtor(void*, unsigned int);
void* RecordDBBasicOfstreamScalarDeletingDtor(void*, unsigned int);
void* RecordDBInitCursorState(void*, unsigned int);

namespace Msvc6
{
    namespace
    {
        template<class Character> unsigned int TextLength(const Character* text)
        {
            unsigned int length = 0;
            while (text[length] != 0) ++length;
            return length;
        }

        template<class Character> StringData<Character>* ReturnFacetString(
            StringData<Character>* result, const Character* text)
        {
            // VC6 copies this caller-provided byte when constructing the result allocator.
            result->allocatorState[0] = static_cast<unsigned char>(
                reinterpret_cast<std::uintptr_t>(result) >> 24u);
            StringOperations<Character>::Tidy(result, false);
            return StringOperations<Character>::AssignRange(result, text, TextLength(text));
        }
    }

    template<class Character> Character NumpunctOperations<Character>::DecimalPoint() const
    {
        return this->decimalPoint;
    }
    template<class Character> Character NumpunctOperations<Character>::ThousandsSeparator() const
    {
        return this->thousandsSeparator;
    }
    template<class Character> StringData<char>* NumpunctOperations<Character>::Grouping(StringData<char>* result) const
    {
        return ReturnFacetString<char>(result, this->grouping);
    }
    template<class Character> StringData<Character>* NumpunctOperations<Character>::FalseName(StringData<Character>* result) const
    {
        return ReturnFacetString<Character>(result, this->falseName);
    }
    template<class Character> StringData<Character>* NumpunctOperations<Character>::TrueName(StringData<Character>* result) const
    {
        return ReturnFacetString<Character>(result, this->trueName);
    }

    template<class Character> Character MoneypunctOperations<Character>::DecimalPoint() const
    {
        return this->decimalPoint;
    }
    template<class Character> Character MoneypunctOperations<Character>::ThousandsSeparator() const
    {
        return this->thousandsSeparator;
    }
    template<class Character> StringData<char>* MoneypunctOperations<Character>::Grouping(StringData<char>* result) const
    {
        return ReturnFacetString<char>(result, this->grouping);
    }
    template<class Character> StringData<Character>* MoneypunctOperations<Character>::CurrencySymbol(StringData<Character>* result) const
    {
        return ReturnFacetString<Character>(result, this->currencySymbol);
    }
    template<class Character> StringData<Character>* MoneypunctOperations<Character>::PositiveSign(StringData<Character>* result) const
    {
        return ReturnFacetString<Character>(result, this->positiveSign);
    }
    template<class Character> StringData<Character>* MoneypunctOperations<Character>::NegativeSign(StringData<Character>* result) const
    {
        return ReturnFacetString<Character>(result, this->negativeSign);
    }
    template<class Character> int MoneypunctOperations<Character>::FractionDigits() const
    {
        return this->fractionDigits;
    }
    template<class Character> MoneyPattern* MoneypunctOperations<Character>::PositivePattern(MoneyPattern* result) const
    {
        std::memcpy(result->fields, this->positivePattern, sizeof(result->fields));
        return result;
    }
    template<class Character> MoneyPattern* MoneypunctOperations<Character>::NegativePattern(MoneyPattern* result) const
    {
        std::memcpy(result->fields, this->negativePattern, sizeof(result->fields));
        return result;
    }

    template<class Character> int MessagesOperations<Character>::Open(
        const StringData<char>* catalog, const RecordDBLocaleControl* locale) const
    {
        (void)catalog;
        (void)locale;
        return 0;
    }
    template<class Character> StringData<Character>* MessagesOperations<Character>::Get(
        StringData<Character>* result, int catalog, int set, int message,
        const StringData<Character>* fallback) const
    {
        (void)catalog;
        (void)set;
        if (message == 0) return ReturnFacetString<Character>(result, this->negativeName);
        if (message == 1) return ReturnFacetString<Character>(result, this->positiveName);
        result->allocatorState[0] = fallback->allocatorState[0];
        StringOperations<Character>::Tidy(result, false);
        return StringOperations<Character>::AssignSubstring(result, fallback, 0,
            static_cast<unsigned int>(-1));
    }
    template<class Character> void MessagesOperations<Character>::Close(int catalog) const
    {
        (void)catalog;
    }

    template struct NumpunctOperations<unsigned short>;
    template struct MoneypunctOperations<char>;
    template struct MoneypunctOperations<unsigned short>;
    template struct MessagesOperations<char>;
    template struct MessagesOperations<unsigned short>;

    bool CharCodecvtOperations::AlwaysNoConversion() const { return true; }
    int CharCodecvtOperations::MaximumLength() const { return 1; }
    int CharCodecvtOperations::Encoding() const { return 1; }
    int CharCodecvtOperations::In(unsigned int&, const char* first, const char*,
        const char*& next, char* destination, char*, char*& destinationNext) const
    {
        next = first;
        destinationNext = destination;
        return 3;
    }
    int CharCodecvtOperations::Out(unsigned int&, const char* first, const char*,
        const char*& next, char* destination, char*, char*& destinationNext) const
    {
        next = first;
        destinationNext = destination;
        return 3;
    }
    int CharCodecvtOperations::Length(unsigned int&, const char* first, const char* last,
        unsigned int maximum) const
    {
        const unsigned int remaining = static_cast<unsigned int>(last - first);
        return static_cast<int>(remaining < maximum ? remaining : maximum);
    }
}

// Native RTTI and virtual-call storage.

namespace Msvc6
{
    namespace
    {
        struct NativeTypeDescriptor
        {
            const FacetVirtualTable* vftable;
            char* undecoratedName;
            void* __thiscall ScalarDeletingDestructor(unsigned int flags);
        };
        template<unsigned int Length> struct NativeTypeStorage
        {
            NativeTypeDescriptor type;
            char decoratedName[Length];
        };
        struct NativeBaseDisplacement
        {
            int memberDisplacement;
            int virtualBasePointerDisplacement;
            int virtualBaseDisplacement;
        };
        struct NativeBaseDescriptor
        {
            const NativeTypeDescriptor* type;
            unsigned int containedBases;
            NativeBaseDisplacement displacement;
            unsigned int attributes;
        };
        struct NativeClassHierarchy
        {
            unsigned int signature;
            unsigned int attributes;
            unsigned int baseCount;
            const NativeBaseDescriptor* const* bases;
        };
        struct NativeCompleteObjectLocator
        {
            unsigned int signature;
            unsigned int offset;
            unsigned int constructorDisplacement;
            const NativeTypeDescriptor* type;
            const NativeClassHierarchy* hierarchy;
        };
        template<class Table> struct NativeVirtualTableStorage
        {
            const NativeCompleteObjectLocator* locator;
            Table functions;
        };
        extern NativeVirtualTableStorage<FacetVirtualTable> typeInfoTable;

        NativeTypeStorage<20> charCtypeType = {{&typeInfoTable.functions, 0}, ".?AV?$ctype@D@std@@"};
        NativeTypeStorage<21> ctypeBaseType = {{&typeInfoTable.functions, 0}, ".?AUctype_base@std@@"};
        NativeTypeStorage<23> baseFacetType = {{&typeInfoTable.functions, 0}, ".?AVfacet@locale@std@@"};
        NativeTypeStorage<74> charNumGetType = {{&typeInfoTable.functions, 0}, ".?AV?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@"};
        NativeTypeStorage<74> charNumPutType = {{&typeInfoTable.functions, 0}, ".?AV?$num_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@"};
        NativeTypeStorage<23> charNumpunctType = {{&typeInfoTable.functions, 0}, ".?AV?$numpunct@D@std@@"};
        NativeTypeStorage<24> charCodecvtType = {{&typeInfoTable.functions, 0}, ".?AV?$codecvt@DDH@std@@"};
        NativeTypeStorage<23> codecvtBaseType = {{&typeInfoTable.functions, 0}, ".?AVcodecvt_base@std@@"};
        NativeTypeStorage<22> charCollateType = {{&typeInfoTable.functions, 0}, ".?AV?$collate@D@std@@"};
        NativeTypeStorage<23> charMessagesType = {{&typeInfoTable.functions, 0}, ".?AV?$messages@D@std@@"};
        NativeTypeStorage<24> messagesBaseType = {{&typeInfoTable.functions, 0}, ".?AUmessages_base@std@@"};
        NativeTypeStorage<76> charMoneyGetType = {{&typeInfoTable.functions, 0}, ".?AV?$money_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@"};
        NativeTypeStorage<76> charMoneyPutType = {{&typeInfoTable.functions, 0}, ".?AV?$money_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@"};
        NativeTypeStorage<29> charMoneypunctType = {{&typeInfoTable.functions, 0}, ".?AV?$moneypunct@D$0A@@std@@"};
        NativeTypeStorage<22> charMoneyBaseType = {{&typeInfoTable.functions, 0}, ".?AV?$_Mpunct@D@std@@"};
        NativeTypeStorage<21> moneyBaseType = {{&typeInfoTable.functions, 0}, ".?AUmoney_base@std@@"};
        NativeTypeStorage<28> charMoneypunctIntlType = {{&typeInfoTable.functions, 0}, ".?AV?$moneypunct@D$00@std@@"};
        NativeTypeStorage<75> charTimeGetType = {{&typeInfoTable.functions, 0}, ".?AV?$time_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@"};
        NativeTypeStorage<20> timeBaseType = {{&typeInfoTable.functions, 0}, ".?AUtime_base@std@@"};
        NativeTypeStorage<75> charTimePutType = {{&typeInfoTable.functions, 0}, ".?AV?$time_put@DV?$ostreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@"};
        NativeTypeStorage<20> wideCtypeType = {{&typeInfoTable.functions, 0}, ".?AV?$ctype@G@std@@"};
        NativeTypeStorage<74> wideNumGetType = {{&typeInfoTable.functions, 0}, ".?AV?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@"};
        NativeTypeStorage<74> wideNumPutType = {{&typeInfoTable.functions, 0}, ".?AV?$num_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@"};
        NativeTypeStorage<23> wideNumpunctType = {{&typeInfoTable.functions, 0}, ".?AV?$numpunct@G@std@@"};
        NativeTypeStorage<22> wideCollateType = {{&typeInfoTable.functions, 0}, ".?AV?$collate@G@std@@"};
        NativeTypeStorage<23> wideMessagesType = {{&typeInfoTable.functions, 0}, ".?AV?$messages@G@std@@"};
        NativeTypeStorage<76> wideMoneyGetType = {{&typeInfoTable.functions, 0}, ".?AV?$money_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@"};
        NativeTypeStorage<76> wideMoneyPutType = {{&typeInfoTable.functions, 0}, ".?AV?$money_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@"};
        NativeTypeStorage<29> wideMoneypunctType = {{&typeInfoTable.functions, 0}, ".?AV?$moneypunct@G$0A@@std@@"};
        NativeTypeStorage<22> wideMoneyBaseType = {{&typeInfoTable.functions, 0}, ".?AV?$_Mpunct@G@std@@"};
        NativeTypeStorage<28> wideMoneypunctIntlType = {{&typeInfoTable.functions, 0}, ".?AV?$moneypunct@G$00@std@@"};
        NativeTypeStorage<75> wideTimeGetType = {{&typeInfoTable.functions, 0}, ".?AV?$time_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@"};
        NativeTypeStorage<75> wideTimePutType = {{&typeInfoTable.functions, 0}, ".?AV?$time_put@GV?$ostreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@"};
        NativeTypeStorage<24> wideCodecvtType = {{&typeInfoTable.functions, 0}, ".?AV?$codecvt@GDH@std@@"};
        NativeTypeStorage<25> localeImplType = {{&typeInfoTable.functions, 0}, ".?AV_Locimp@locale@std@@"};
        NativeTypeStorage<52> streambufType = {{&typeInfoTable.functions, 0}, ".?AV?$basic_streambuf@DU?$char_traits@D@std@@@std@@"};
        NativeTypeStorage<50> filebufType = {{&typeInfoTable.functions, 0}, ".?AV?$basic_filebuf@DU?$char_traits@D@std@@@std@@"};
        NativeTypeStorage<16> exceptionType = {{&typeInfoTable.functions, 0}, ".?AVexception@@"};
        NativeTypeStorage<19> badCastType = {{&typeInfoTable.functions, 0}, ".?AVbad_cast@std@@"};
        NativeTypeStorage<22> logicErrorType = {{&typeInfoTable.functions, 0}, ".?AVlogic_error@std@@"};
        NativeTypeStorage<23> lengthErrorType = {{&typeInfoTable.functions, 0}, ".?AVlength_error@std@@"};
        NativeTypeStorage<23> outOfRangeType = {{&typeInfoTable.functions, 0}, ".?AVout_of_range@std@@"};
        NativeTypeStorage<24> runtimeErrorType = {{&typeInfoTable.functions, 0}, ".?AVruntime_error@std@@"};
        NativeTypeStorage<27> iosFailureType = {{&typeInfoTable.functions, 0}, ".?AVfailure@ios_base@std@@"};
        NativeTypeStorage<20> badAllocType = {{&typeInfoTable.functions, 0}, ".?AVbad_alloc@std@@"};
        NativeTypeStorage<19> iosBaseType = {{&typeInfoTable.functions, 0}, ".?AVios_base@std@@"};
        NativeTypeStorage<50> istreamType = {{&typeInfoTable.functions, 0}, ".?AV?$basic_istream@DU?$char_traits@D@std@@@std@@"};
        NativeTypeStorage<46> iosType = {{&typeInfoTable.functions, 0}, ".?AV?$basic_ios@DU?$char_traits@D@std@@@std@@"};
        NativeTypeStorage<50> ostreamType = {{&typeInfoTable.functions, 0}, ".?AV?$basic_ostream@DU?$char_traits@D@std@@@std@@"};
        NativeTypeStorage<16> typeInfoType = {{&typeInfoTable.functions, 0}, ".?AVtype_info@@"};

        const NativeBaseDescriptor charCtypeBase = {&charCtypeType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor ctypeBaseBase = {&ctypeBaseType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor baseFacetBase = {&baseFacetType.type, 0, {0, -1, 0}, 0};
        const NativeBaseDescriptor charNumGetBase = {&charNumGetType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor charNumPutBase = {&charNumPutType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor charNumpunctBase = {&charNumpunctType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor charCodecvtBase = {&charCodecvtType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor codecvtBaseBase = {&codecvtBaseType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor charCollateBase = {&charCollateType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor charMessagesBase = {&charMessagesType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor messagesBaseBase = {&messagesBaseType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor charMoneyGetBase = {&charMoneyGetType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor charMoneyPutBase = {&charMoneyPutType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor charMoneypunctBase = {&charMoneypunctType.type, 3, {0, -1, 0}, 0};
        const NativeBaseDescriptor charMoneyBaseBase = {&charMoneyBaseType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor moneyBaseBase = {&moneyBaseType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor charMoneypunctIntlBase = {&charMoneypunctIntlType.type, 3, {0, -1, 0}, 0};
        const NativeBaseDescriptor charTimeGetBase = {&charTimeGetType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor timeBaseBase = {&timeBaseType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor charTimePutBase = {&charTimePutType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideCtypeBase = {&wideCtypeType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideNumGetBase = {&wideNumGetType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideNumPutBase = {&wideNumPutType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideNumpunctBase = {&wideNumpunctType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideCollateBase = {&wideCollateType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideMessagesBase = {&wideMessagesType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideMoneyGetBase = {&wideMoneyGetType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideMoneyPutBase = {&wideMoneyPutType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideMoneypunctBase = {&wideMoneypunctType.type, 3, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideMoneyBaseBase = {&wideMoneyBaseType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideMoneypunctIntlBase = {&wideMoneypunctIntlType.type, 3, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideTimeGetBase = {&wideTimeGetType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideTimePutBase = {&wideTimePutType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor wideCodecvtBase = {&wideCodecvtType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor localeImplBase = {&localeImplType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor streambufBase = {&streambufType.type, 0, {0, -1, 0}, 0};
        const NativeBaseDescriptor filebufBase = {&filebufType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor exceptionBase = {&exceptionType.type, 0, {0, -1, 0}, 0};
        const NativeBaseDescriptor badCastBase = {&badCastType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor logicErrorBase = {&logicErrorType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor lengthErrorBase = {&lengthErrorType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor outOfRangeBase = {&outOfRangeType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor runtimeErrorBase = {&runtimeErrorType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor iosFailureBase = {&iosFailureType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor badAllocBase = {&badAllocType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor iosBaseBase = {&iosBaseType.type, 0, {0, -1, 0}, 0};
        const NativeBaseDescriptor istreamBase = {&istreamType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor iosVirtualBase = {&iosType.type, 1, {0, 0, 4}, 16};
        const NativeBaseDescriptor iosBaseVirtualBase = {&iosBaseType.type, 0, {0, 0, 4}, 0};
        const NativeBaseDescriptor ostreamBase = {&ostreamType.type, 2, {0, -1, 0}, 0};
        const NativeBaseDescriptor iosBase = {&iosType.type, 1, {0, -1, 0}, 0};
        const NativeBaseDescriptor typeInfoBase = {&typeInfoType.type, 0, {0, -1, 0}, 0};

        const NativeBaseDescriptor* const charCtypeBases[] = {&charCtypeBase, &ctypeBaseBase, &baseFacetBase};
        const NativeClassHierarchy charCtypeHierarchy = {0, 0, 3, charCtypeBases};
        const NativeCompleteObjectLocator charCtypeLocator = {0, 0, 0, &charCtypeType.type, &charCtypeHierarchy};
        NativeVirtualTableStorage<CharCtypeVirtualTable> charCtypeTable = {&charCtypeLocator, {}};
        const NativeBaseDescriptor* const charNumGetBases[] = {&charNumGetBase, &baseFacetBase};
        const NativeClassHierarchy charNumGetHierarchy = {0, 0, 2, charNumGetBases};
        const NativeCompleteObjectLocator charNumGetLocator = {0, 0, 0, &charNumGetType.type, &charNumGetHierarchy};
        NativeVirtualTableStorage<NumGetVirtualTable<char>> charNumGetTable = {&charNumGetLocator, {}};
        const NativeBaseDescriptor* const charNumPutBases[] = {&charNumPutBase, &baseFacetBase};
        const NativeClassHierarchy charNumPutHierarchy = {0, 0, 2, charNumPutBases};
        const NativeCompleteObjectLocator charNumPutLocator = {0, 0, 0, &charNumPutType.type, &charNumPutHierarchy};
        NativeVirtualTableStorage<NumPutVirtualTable<char>> charNumPutTable = {&charNumPutLocator, {}};
        const NativeBaseDescriptor* const charNumpunctBases[] = {&charNumpunctBase, &baseFacetBase};
        const NativeClassHierarchy charNumpunctHierarchy = {0, 0, 2, charNumpunctBases};
        const NativeCompleteObjectLocator charNumpunctLocator = {0, 0, 0, &charNumpunctType.type, &charNumpunctHierarchy};
        NativeVirtualTableStorage<NumpunctVirtualTable<char>> charNumpunctTable = {&charNumpunctLocator, {}};
        const NativeBaseDescriptor* const charCodecvtBases[] = {&charCodecvtBase, &codecvtBaseBase, &baseFacetBase};
        const NativeClassHierarchy charCodecvtHierarchy = {0, 0, 3, charCodecvtBases};
        const NativeCompleteObjectLocator charCodecvtLocator = {0, 0, 0, &charCodecvtType.type, &charCodecvtHierarchy};
        NativeVirtualTableStorage<CodecvtVirtualTable<char>> charCodecvtTable = {&charCodecvtLocator, {}};
        const NativeBaseDescriptor* const charCollateBases[] = {&charCollateBase, &baseFacetBase};
        const NativeClassHierarchy charCollateHierarchy = {0, 0, 2, charCollateBases};
        const NativeCompleteObjectLocator charCollateLocator = {0, 0, 0, &charCollateType.type, &charCollateHierarchy};
        NativeVirtualTableStorage<CollateVirtualTable<char>> charCollateTable = {&charCollateLocator, {}};
        const NativeBaseDescriptor* const charMessagesBases[] = {&charMessagesBase, &messagesBaseBase, &baseFacetBase};
        const NativeClassHierarchy charMessagesHierarchy = {0, 0, 3, charMessagesBases};
        const NativeCompleteObjectLocator charMessagesLocator = {0, 0, 0, &charMessagesType.type, &charMessagesHierarchy};
        NativeVirtualTableStorage<MessagesVirtualTable<char>> charMessagesTable = {&charMessagesLocator, {}};
        const NativeBaseDescriptor* const charMoneyGetBases[] = {&charMoneyGetBase, &baseFacetBase};
        const NativeClassHierarchy charMoneyGetHierarchy = {0, 0, 2, charMoneyGetBases};
        const NativeCompleteObjectLocator charMoneyGetLocator = {0, 0, 0, &charMoneyGetType.type, &charMoneyGetHierarchy};
        NativeVirtualTableStorage<MoneyGetVirtualTable<char>> charMoneyGetTable = {&charMoneyGetLocator, {}};
        const NativeBaseDescriptor* const charMoneyPutBases[] = {&charMoneyPutBase, &baseFacetBase};
        const NativeClassHierarchy charMoneyPutHierarchy = {0, 0, 2, charMoneyPutBases};
        const NativeCompleteObjectLocator charMoneyPutLocator = {0, 0, 0, &charMoneyPutType.type, &charMoneyPutHierarchy};
        NativeVirtualTableStorage<MoneyPutVirtualTable<char>> charMoneyPutTable = {&charMoneyPutLocator, {}};
        const NativeBaseDescriptor* const charMoneypunctBases[] = {&charMoneypunctBase, &charMoneyBaseBase, &moneyBaseBase, &baseFacetBase};
        const NativeClassHierarchy charMoneypunctHierarchy = {0, 0, 4, charMoneypunctBases};
        const NativeCompleteObjectLocator charMoneypunctLocator = {0, 0, 0, &charMoneypunctType.type, &charMoneypunctHierarchy};
        NativeVirtualTableStorage<MoneypunctVirtualTable<char>> charMoneypunctTable = {&charMoneypunctLocator, {}};
        const NativeBaseDescriptor* const charMoneypunctIntlBases[] = {&charMoneypunctIntlBase, &charMoneyBaseBase, &moneyBaseBase, &baseFacetBase};
        const NativeClassHierarchy charMoneypunctIntlHierarchy = {0, 0, 4, charMoneypunctIntlBases};
        const NativeCompleteObjectLocator charMoneypunctIntlLocator = {0, 0, 0, &charMoneypunctIntlType.type, &charMoneypunctIntlHierarchy};
        NativeVirtualTableStorage<MoneypunctVirtualTable<char>> charMoneypunctIntlTable = {&charMoneypunctIntlLocator, {}};
        const NativeBaseDescriptor* const charTimeGetBases[] = {&charTimeGetBase, &timeBaseBase, &baseFacetBase};
        const NativeClassHierarchy charTimeGetHierarchy = {0, 0, 3, charTimeGetBases};
        const NativeCompleteObjectLocator charTimeGetLocator = {0, 0, 0, &charTimeGetType.type, &charTimeGetHierarchy};
        NativeVirtualTableStorage<TimeGetVirtualTable<char>> charTimeGetTable = {&charTimeGetLocator, {}};
        const NativeBaseDescriptor* const charTimePutBases[] = {&charTimePutBase, &timeBaseBase, &baseFacetBase};
        const NativeClassHierarchy charTimePutHierarchy = {0, 0, 3, charTimePutBases};
        const NativeCompleteObjectLocator charTimePutLocator = {0, 0, 0, &charTimePutType.type, &charTimePutHierarchy};
        NativeVirtualTableStorage<TimePutVirtualTable<char>> charTimePutTable = {&charTimePutLocator, {}};
        const NativeBaseDescriptor* const wideCtypeBases[] = {&wideCtypeBase, &ctypeBaseBase, &baseFacetBase};
        const NativeClassHierarchy wideCtypeHierarchy = {0, 0, 3, wideCtypeBases};
        const NativeCompleteObjectLocator wideCtypeLocator = {0, 0, 0, &wideCtypeType.type, &wideCtypeHierarchy};
        NativeVirtualTableStorage<WideCtypeVirtualTable> wideCtypeTable = {&wideCtypeLocator, {}};
        const NativeBaseDescriptor* const wideNumGetBases[] = {&wideNumGetBase, &baseFacetBase};
        const NativeClassHierarchy wideNumGetHierarchy = {0, 0, 2, wideNumGetBases};
        const NativeCompleteObjectLocator wideNumGetLocator = {0, 0, 0, &wideNumGetType.type, &wideNumGetHierarchy};
        NativeVirtualTableStorage<NumGetVirtualTable<unsigned short>> wideNumGetTable = {&wideNumGetLocator, {}};
        const NativeBaseDescriptor* const wideNumPutBases[] = {&wideNumPutBase, &baseFacetBase};
        const NativeClassHierarchy wideNumPutHierarchy = {0, 0, 2, wideNumPutBases};
        const NativeCompleteObjectLocator wideNumPutLocator = {0, 0, 0, &wideNumPutType.type, &wideNumPutHierarchy};
        NativeVirtualTableStorage<NumPutVirtualTable<unsigned short>> wideNumPutTable = {&wideNumPutLocator, {}};
        const NativeBaseDescriptor* const wideNumpunctBases[] = {&wideNumpunctBase, &baseFacetBase};
        const NativeClassHierarchy wideNumpunctHierarchy = {0, 0, 2, wideNumpunctBases};
        const NativeCompleteObjectLocator wideNumpunctLocator = {0, 0, 0, &wideNumpunctType.type, &wideNumpunctHierarchy};
        NativeVirtualTableStorage<NumpunctVirtualTable<unsigned short>> wideNumpunctTable = {&wideNumpunctLocator, {}};
        const NativeBaseDescriptor* const wideCollateBases[] = {&wideCollateBase, &baseFacetBase};
        const NativeClassHierarchy wideCollateHierarchy = {0, 0, 2, wideCollateBases};
        const NativeCompleteObjectLocator wideCollateLocator = {0, 0, 0, &wideCollateType.type, &wideCollateHierarchy};
        NativeVirtualTableStorage<CollateVirtualTable<unsigned short>> wideCollateTable = {&wideCollateLocator, {}};
        const NativeBaseDescriptor* const wideMessagesBases[] = {&wideMessagesBase, &messagesBaseBase, &baseFacetBase};
        const NativeClassHierarchy wideMessagesHierarchy = {0, 0, 3, wideMessagesBases};
        const NativeCompleteObjectLocator wideMessagesLocator = {0, 0, 0, &wideMessagesType.type, &wideMessagesHierarchy};
        NativeVirtualTableStorage<MessagesVirtualTable<unsigned short>> wideMessagesTable = {&wideMessagesLocator, {}};
        const NativeBaseDescriptor* const wideMoneyGetBases[] = {&wideMoneyGetBase, &baseFacetBase};
        const NativeClassHierarchy wideMoneyGetHierarchy = {0, 0, 2, wideMoneyGetBases};
        const NativeCompleteObjectLocator wideMoneyGetLocator = {0, 0, 0, &wideMoneyGetType.type, &wideMoneyGetHierarchy};
        NativeVirtualTableStorage<MoneyGetVirtualTable<unsigned short>> wideMoneyGetTable = {&wideMoneyGetLocator, {}};
        const NativeBaseDescriptor* const wideMoneyPutBases[] = {&wideMoneyPutBase, &baseFacetBase};
        const NativeClassHierarchy wideMoneyPutHierarchy = {0, 0, 2, wideMoneyPutBases};
        const NativeCompleteObjectLocator wideMoneyPutLocator = {0, 0, 0, &wideMoneyPutType.type, &wideMoneyPutHierarchy};
        NativeVirtualTableStorage<MoneyPutVirtualTable<unsigned short>> wideMoneyPutTable = {&wideMoneyPutLocator, {}};
        const NativeBaseDescriptor* const wideMoneypunctBases[] = {&wideMoneypunctBase, &wideMoneyBaseBase, &moneyBaseBase, &baseFacetBase};
        const NativeClassHierarchy wideMoneypunctHierarchy = {0, 0, 4, wideMoneypunctBases};
        const NativeCompleteObjectLocator wideMoneypunctLocator = {0, 0, 0, &wideMoneypunctType.type, &wideMoneypunctHierarchy};
        NativeVirtualTableStorage<MoneypunctVirtualTable<unsigned short>> wideMoneypunctTable = {&wideMoneypunctLocator, {}};
        const NativeBaseDescriptor* const wideMoneypunctIntlBases[] = {&wideMoneypunctIntlBase, &wideMoneyBaseBase, &moneyBaseBase, &baseFacetBase};
        const NativeClassHierarchy wideMoneypunctIntlHierarchy = {0, 0, 4, wideMoneypunctIntlBases};
        const NativeCompleteObjectLocator wideMoneypunctIntlLocator = {0, 0, 0, &wideMoneypunctIntlType.type, &wideMoneypunctIntlHierarchy};
        NativeVirtualTableStorage<MoneypunctVirtualTable<unsigned short>> wideMoneypunctIntlTable = {&wideMoneypunctIntlLocator, {}};
        const NativeBaseDescriptor* const wideTimeGetBases[] = {&wideTimeGetBase, &timeBaseBase, &baseFacetBase};
        const NativeClassHierarchy wideTimeGetHierarchy = {0, 0, 3, wideTimeGetBases};
        const NativeCompleteObjectLocator wideTimeGetLocator = {0, 0, 0, &wideTimeGetType.type, &wideTimeGetHierarchy};
        NativeVirtualTableStorage<TimeGetVirtualTable<unsigned short>> wideTimeGetTable = {&wideTimeGetLocator, {}};
        const NativeBaseDescriptor* const wideTimePutBases[] = {&wideTimePutBase, &timeBaseBase, &baseFacetBase};
        const NativeClassHierarchy wideTimePutHierarchy = {0, 0, 3, wideTimePutBases};
        const NativeCompleteObjectLocator wideTimePutLocator = {0, 0, 0, &wideTimePutType.type, &wideTimePutHierarchy};
        NativeVirtualTableStorage<TimePutVirtualTable<unsigned short>> wideTimePutTable = {&wideTimePutLocator, {}};
        const NativeBaseDescriptor* const wideCodecvtBases[] = {&wideCodecvtBase, &codecvtBaseBase, &baseFacetBase};
        const NativeClassHierarchy wideCodecvtHierarchy = {0, 0, 3, wideCodecvtBases};
        const NativeCompleteObjectLocator wideCodecvtLocator = {0, 0, 0, &wideCodecvtType.type, &wideCodecvtHierarchy};
        NativeVirtualTableStorage<CodecvtVirtualTable<unsigned short>> wideCodecvtTable = {&wideCodecvtLocator, {}};
        const NativeBaseDescriptor* const charMoneyBaseBases[] = {&charMoneyBaseBase, &moneyBaseBase, &baseFacetBase};
        const NativeClassHierarchy charMoneyBaseHierarchy = {0, 0, 3, charMoneyBaseBases};
        const NativeCompleteObjectLocator charMoneyBaseLocator = {0, 0, 0, &charMoneyBaseType.type, &charMoneyBaseHierarchy};
        NativeVirtualTableStorage<FacetVirtualTable> charMoneyBaseTable = {&charMoneyBaseLocator, {}};
        const NativeBaseDescriptor* const wideMoneyBaseBases[] = {&wideMoneyBaseBase, &moneyBaseBase, &baseFacetBase};
        const NativeClassHierarchy wideMoneyBaseHierarchy = {0, 0, 3, wideMoneyBaseBases};
        const NativeCompleteObjectLocator wideMoneyBaseLocator = {0, 0, 0, &wideMoneyBaseType.type, &wideMoneyBaseHierarchy};
        NativeVirtualTableStorage<FacetVirtualTable> wideMoneyBaseTable = {&wideMoneyBaseLocator, {}};
        const NativeBaseDescriptor* const baseFacetBases[] = {&baseFacetBase};
        const NativeClassHierarchy baseFacetHierarchy = {0, 0, 1, baseFacetBases};
        const NativeCompleteObjectLocator baseFacetLocator = {0, 0, 0, &baseFacetType.type, &baseFacetHierarchy};
        NativeVirtualTableStorage<FacetVirtualTable> baseFacetTable = {&baseFacetLocator, {}};
        const NativeBaseDescriptor* const localeImplBases[] = {&localeImplBase, &baseFacetBase};
        const NativeClassHierarchy localeImplHierarchy = {0, 0, 2, localeImplBases};
        const NativeCompleteObjectLocator localeImplLocator = {0, 0, 0, &localeImplType.type, &localeImplHierarchy};
        NativeVirtualTableStorage<FacetVirtualTable> localeImplTable = {&localeImplLocator, {}};
        const NativeBaseDescriptor* const streambufBases[] = {&streambufBase};
        const NativeClassHierarchy streambufHierarchy = {0, 0, 1, streambufBases};
        const NativeCompleteObjectLocator streambufLocator = {0, 0, 0, &streambufType.type, &streambufHierarchy};
        NativeVirtualTableStorage<StreambufVirtualTable> streambufTable = {&streambufLocator, {}};
        const NativeBaseDescriptor* const filebufBases[] = {&filebufBase, &streambufBase};
        const NativeClassHierarchy filebufHierarchy = {0, 0, 2, filebufBases};
        const NativeCompleteObjectLocator filebufLocator = {0, 0, 0, &filebufType.type, &filebufHierarchy};
        NativeVirtualTableStorage<StreambufVirtualTable> filebufTable = {&filebufLocator, {}};
        const NativeBaseDescriptor* const exceptionBases[] = {&exceptionBase};
        const NativeClassHierarchy exceptionHierarchy = {0, 0, 1, exceptionBases};
        const NativeCompleteObjectLocator exceptionLocator = {0, 0, 0, &exceptionType.type, &exceptionHierarchy};
        NativeVirtualTableStorage<ExceptionVirtualTable> exceptionTable = {&exceptionLocator, {}};
        const NativeBaseDescriptor* const badCastBases[] = {&badCastBase, &exceptionBase};
        const NativeClassHierarchy badCastHierarchy = {0, 0, 2, badCastBases};
        const NativeCompleteObjectLocator badCastLocator = {0, 0, 0, &badCastType.type, &badCastHierarchy};
        NativeVirtualTableStorage<RaisedExceptionVirtualTable> badCastTable = {&badCastLocator, {}};
        const NativeBaseDescriptor* const logicErrorBases[] = {&logicErrorBase, &exceptionBase};
        const NativeClassHierarchy logicErrorHierarchy = {0, 0, 2, logicErrorBases};
        const NativeCompleteObjectLocator logicErrorLocator = {0, 0, 0, &logicErrorType.type, &logicErrorHierarchy};
        NativeVirtualTableStorage<RaisedExceptionVirtualTable> logicErrorTable = {&logicErrorLocator, {}};
        const NativeBaseDescriptor* const lengthErrorBases[] = {&lengthErrorBase, &logicErrorBase, &exceptionBase};
        const NativeClassHierarchy lengthErrorHierarchy = {0, 0, 3, lengthErrorBases};
        const NativeCompleteObjectLocator lengthErrorLocator = {0, 0, 0, &lengthErrorType.type, &lengthErrorHierarchy};
        NativeVirtualTableStorage<RaisedExceptionVirtualTable> lengthErrorTable = {&lengthErrorLocator, {}};
        const NativeBaseDescriptor* const outOfRangeBases[] = {&outOfRangeBase, &logicErrorBase, &exceptionBase};
        const NativeClassHierarchy outOfRangeHierarchy = {0, 0, 3, outOfRangeBases};
        const NativeCompleteObjectLocator outOfRangeLocator = {0, 0, 0, &outOfRangeType.type, &outOfRangeHierarchy};
        NativeVirtualTableStorage<RaisedExceptionVirtualTable> outOfRangeTable = {&outOfRangeLocator, {}};
        const NativeBaseDescriptor* const runtimeErrorBases[] = {&runtimeErrorBase, &exceptionBase};
        const NativeClassHierarchy runtimeErrorHierarchy = {0, 0, 2, runtimeErrorBases};
        const NativeCompleteObjectLocator runtimeErrorLocator = {0, 0, 0, &runtimeErrorType.type, &runtimeErrorHierarchy};
        NativeVirtualTableStorage<RaisedExceptionVirtualTable> runtimeErrorTable = {&runtimeErrorLocator, {}};
        const NativeBaseDescriptor* const iosFailureBases[] = {&iosFailureBase, &runtimeErrorBase, &exceptionBase};
        const NativeClassHierarchy iosFailureHierarchy = {0, 0, 3, iosFailureBases};
        const NativeCompleteObjectLocator iosFailureLocator = {0, 0, 0, &iosFailureType.type, &iosFailureHierarchy};
        NativeVirtualTableStorage<RaisedExceptionVirtualTable> iosFailureTable = {&iosFailureLocator, {}};
        const NativeBaseDescriptor* const badAllocBases[] = {&badAllocBase, &exceptionBase};
        const NativeClassHierarchy badAllocHierarchy = {0, 0, 2, badAllocBases};
        const NativeCompleteObjectLocator badAllocLocator = {0, 0, 0, &badAllocType.type, &badAllocHierarchy};
        NativeVirtualTableStorage<RaisedExceptionVirtualTable> badAllocTable = {&badAllocLocator, {}};
        const NativeBaseDescriptor* const iosBaseBases[] = {&iosBaseBase};
        const NativeClassHierarchy iosBaseHierarchy = {0, 0, 1, iosBaseBases};
        const NativeCompleteObjectLocator iosBaseLocator = {0, 0, 0, &iosBaseType.type, &iosBaseHierarchy};
        NativeVirtualTableStorage<FacetVirtualTable> iosBaseTable = {&iosBaseLocator, {}};
        const NativeBaseDescriptor* const istreamBases[] = {&istreamBase, &iosVirtualBase, &iosBaseVirtualBase};
        const NativeClassHierarchy istreamHierarchy = {0, 0, 3, istreamBases};
        const NativeCompleteObjectLocator istreamLocator = {0, 8, 0, &istreamType.type, &istreamHierarchy};
        NativeVirtualTableStorage<FacetVirtualTable> istreamTable = {&istreamLocator, {}};
        const NativeBaseDescriptor* const ostreamBases[] = {&ostreamBase, &iosVirtualBase, &iosBaseVirtualBase};
        const NativeClassHierarchy ostreamHierarchy = {0, 0, 3, ostreamBases};
        const NativeCompleteObjectLocator ostreamLocator = {0, 4, 0, &ostreamType.type, &ostreamHierarchy};
        NativeVirtualTableStorage<FacetVirtualTable> ostreamTable = {&ostreamLocator, {}};
        const NativeBaseDescriptor* const iosBases[] = {&iosBase, &iosBaseBase};
        const NativeClassHierarchy iosHierarchy = {0, 0, 2, iosBases};
        const NativeCompleteObjectLocator iosLocator = {0, 0, 0, &iosType.type, &iosHierarchy};
        NativeVirtualTableStorage<FacetVirtualTable> iosTable = {&iosLocator, {}};
        const NativeBaseDescriptor* const typeInfoBases[] = {&typeInfoBase};
        const NativeClassHierarchy typeInfoHierarchy = {0, 0, 1, typeInfoBases};
        const NativeCompleteObjectLocator typeInfoLocator = {0, 0, 0, &typeInfoType.type, &typeInfoHierarchy};
        NativeVirtualTableStorage<FacetVirtualTable> typeInfoTable = {&typeInfoLocator, {}};

        // These two native stream tables do not have complete-object locators.
        FacetVirtualTable ifstreamTable = {};
        FacetVirtualTable ofstreamTable = {};

        typedef void* (__thiscall *CopyException)(void*, const void*);
        typedef void (__thiscall *DestroyException)(void*);
        struct NativeCatchableType
        {
            unsigned int attributes;
            const NativeTypeDescriptor* type;
            NativeBaseDisplacement displacement;
            unsigned int objectSize;
            CopyException copy;
        };
        struct NativeCatchableTypeArray { unsigned int count; };
        template<unsigned int Count> struct NativeCatchableTypeStorage
        {
            NativeCatchableTypeArray array;
            const NativeCatchableType* types[Count];
        };
    }

    struct NativeThrowInfo
    {
        unsigned int attributes;
        DestroyException unwind;
        void* forwardCompatibility;
        const NativeCatchableTypeArray* catchableTypes;
    };

    namespace
    {
        NativeCatchableType badCastCatchable = {0, &badCastType.type, {0, -1, 0}, 12, 0};
        NativeCatchableType exceptionCatchable = {0, &exceptionType.type, {0, -1, 0}, 12, 0};
        NativeCatchableType logicErrorCatchable = {0, &logicErrorType.type, {0, -1, 0}, 28, 0};
        NativeCatchableType lengthErrorCatchable = {0, &lengthErrorType.type, {0, -1, 0}, 28, 0};
        NativeCatchableType outOfRangeCatchable = {0, &outOfRangeType.type, {0, -1, 0}, 28, 0};
        NativeCatchableType runtimeErrorCatchable = {0, &runtimeErrorType.type, {0, -1, 0}, 28, 0};
        NativeCatchableType iosFailureCatchable = {0, &iosFailureType.type, {0, -1, 0}, 28, 0};
        NativeCatchableType badAllocCatchable = {0, &badAllocType.type, {0, -1, 0}, 12, 0};
        const NativeCatchableTypeStorage<2> badCastCatchables = {{2}, {&badCastCatchable, &exceptionCatchable}};
        NativeThrowInfo badCastThrowInfo = {0, 0, 0, &badCastCatchables.array};
        const NativeCatchableTypeStorage<2> logicErrorCatchables = {{2}, {&logicErrorCatchable, &exceptionCatchable}};
        NativeThrowInfo logicErrorThrowInfo = {0, 0, 0, &logicErrorCatchables.array};
        const NativeCatchableTypeStorage<3> lengthErrorCatchables = {{3}, {&lengthErrorCatchable, &logicErrorCatchable, &exceptionCatchable}};
        NativeThrowInfo lengthErrorThrowInfo = {0, 0, 0, &lengthErrorCatchables.array};
        const NativeCatchableTypeStorage<3> outOfRangeCatchables = {{3}, {&outOfRangeCatchable, &logicErrorCatchable, &exceptionCatchable}};
        NativeThrowInfo outOfRangeThrowInfo = {0, 0, 0, &outOfRangeCatchables.array};
        const NativeCatchableTypeStorage<2> runtimeErrorCatchables = {{2}, {&runtimeErrorCatchable, &exceptionCatchable}};
        NativeThrowInfo runtimeErrorThrowInfo = {0, 0, 0, &runtimeErrorCatchables.array};
        const NativeCatchableTypeStorage<3> iosFailureCatchables = {{3}, {&iosFailureCatchable, &runtimeErrorCatchable, &exceptionCatchable}};
        NativeThrowInfo iosFailureThrowInfo = {0, 0, 0, &iosFailureCatchables.array};
        const NativeCatchableTypeStorage<2> badAllocCatchables = {{2}, {&badAllocCatchable, &exceptionCatchable}};
        NativeThrowInfo badAllocThrowInfo = {0, 0, 0, &badAllocCatchables.array};

        void* NativeTypeDescriptor::ScalarDeletingDestructor(unsigned int flags)
        {
            vftable = &typeInfoTable.functions;
            LockTypeInfo();
            if (undecoratedName != 0) CrtFreePointer(undecoratedName);
            UnlockTypeInfo();
            if ((flags & 1u) != 0u) CrtFreePointer(this);
            return this;
        }

        struct StreamDestructorOperations
        {
            void* __thiscall DeleteIosBase(unsigned int flags)
            {
                Msvc6BasicIosDestruct(this);
                if ((flags & 1u) != 0u) CrtFreePointer(this);
                return this;
            }
            void* __thiscall DeleteIos(unsigned int flags)
            {
                RecordDBBasicIosDestructor(this);
                if ((flags & 1u) != 0u) CrtFreePointer(this);
                return this;
            }
            void* __thiscall DeleteIstream(unsigned int flags)
            {
                void* const complete = reinterpret_cast<unsigned char*>(this) - 8;
                RecordDBCompilerAdjustVtableMinus8(this);
                RecordDBBasicIosDestructor(this);
                if ((flags & 1u) != 0u) CrtFreePointer(complete);
                return complete;
            }
            void* __thiscall DeleteOstream(unsigned int flags)
            {
                void* const complete = reinterpret_cast<unsigned char*>(this) - 4;
                basic_ostream_Dtor(this);
                RecordDBBasicIosDestructor(this);
                if ((flags & 1u) != 0u) CrtFreePointer(complete);
                return complete;
            }
            void* __thiscall DeleteIfstream(unsigned int flags)
            {
                return RecordDBBasicIfstreamScalarDeletingDtor(this, flags);
            }
            void* __thiscall DeleteOfstream(unsigned int flags)
            {
                return RecordDBBasicOfstreamScalarDeletingDtor(this, flags);
            }
        };

        struct StreambufDefaultOperations : RecordDBStreambufControl
        {
            RecordDBPositionControl* __thiscall SeekOffset(RecordDBPositionControl* result,
                int, int, unsigned int)
            {
                RecordDBInitCursorState(result, static_cast<unsigned int>(-1));
                return result;
            }
            RecordDBPositionControl* __thiscall SeekPosition(RecordDBPositionControl* result,
                RecordDBPositionControl position, unsigned int mode)
            {
                return SeekOffset(result, position.offset, 0, mode);
            }
        };
    }

    const VirtualBaseTable IstreamVirtualBase = {0, 8};
    const VirtualBaseTable OstreamVirtualBase = {0, 4};
    const VirtualBaseTable IfstreamVirtualBase = {0, 92};
    const VirtualBaseTable OfstreamVirtualBase = {0, 88};

    const FacetVirtualTable* CharCtype::GetTable() { return &charCtypeTable.functions; }
    const FacetVirtualTable* CharNumGet::GetTable() { return &charNumGetTable.functions; }
    const FacetVirtualTable* CharNumPut::GetTable() { return &charNumPutTable.functions; }
    const FacetVirtualTable* CharNumpunct::GetTable() { return &charNumpunctTable.functions; }
    const FacetVirtualTable* CharCodecvt::GetTable() { return &charCodecvtTable.functions; }
    const FacetVirtualTable* CharCollate::GetTable() { return &charCollateTable.functions; }
    const FacetVirtualTable* CharMessages::GetTable() { return &charMessagesTable.functions; }
    const FacetVirtualTable* CharMoneyGet::GetTable() { return &charMoneyGetTable.functions; }
    const FacetVirtualTable* CharMoneyPut::GetTable() { return &charMoneyPutTable.functions; }
    const FacetVirtualTable* CharMoneypunct::GetTable() { return &charMoneypunctTable.functions; }
    const FacetVirtualTable* CharMoneypunctIntl::GetTable() { return &charMoneypunctIntlTable.functions; }
    const FacetVirtualTable* CharTimeGet::GetTable() { return &charTimeGetTable.functions; }
    const FacetVirtualTable* CharTimePut::GetTable() { return &charTimePutTable.functions; }
    const FacetVirtualTable* WideCtype::GetTable() { return &wideCtypeTable.functions; }
    const FacetVirtualTable* WideNumGet::GetTable() { return &wideNumGetTable.functions; }
    const FacetVirtualTable* WideNumPut::GetTable() { return &wideNumPutTable.functions; }
    const FacetVirtualTable* WideNumpunct::GetTable() { return &wideNumpunctTable.functions; }
    const FacetVirtualTable* WideCollate::GetTable() { return &wideCollateTable.functions; }
    const FacetVirtualTable* WideMessages::GetTable() { return &wideMessagesTable.functions; }
    const FacetVirtualTable* WideMoneyGet::GetTable() { return &wideMoneyGetTable.functions; }
    const FacetVirtualTable* WideMoneyPut::GetTable() { return &wideMoneyPutTable.functions; }
    const FacetVirtualTable* WideMoneypunct::GetTable() { return &wideMoneypunctTable.functions; }
    const FacetVirtualTable* WideMoneypunctIntl::GetTable() { return &wideMoneypunctIntlTable.functions; }
    const FacetVirtualTable* WideTimeGet::GetTable() { return &wideTimeGetTable.functions; }
    const FacetVirtualTable* WideTimePut::GetTable() { return &wideTimePutTable.functions; }
    const FacetVirtualTable* WideCodecvt::GetTable() { return &wideCodecvtTable.functions; }
    const FacetVirtualTable* CharMoneyBase::GetTable() { return &charMoneyBaseTable.functions; }
    const FacetVirtualTable* WideMoneyBase::GetTable() { return &wideMoneyBaseTable.functions; }
    const FacetVirtualTable* BaseFacetTable() { return &baseFacetTable.functions; }
    const FacetVirtualTable* LocaleImplTable() { return &localeImplTable.functions; }
    const FacetVirtualTable* IosBaseTable() { return &iosBaseTable.functions; }
    const FacetVirtualTable* IosTable() { return &iosTable.functions; }
    const FacetVirtualTable* IstreamTable() { return &istreamTable.functions; }
    const FacetVirtualTable* OstreamTable() { return &ostreamTable.functions; }
    const FacetVirtualTable* IfstreamTable() { return &ifstreamTable; }
    const FacetVirtualTable* OfstreamTable() { return &ofstreamTable; }
    const ExceptionVirtualTable* ExceptionTable() { return &exceptionTable.functions; }
    const ExceptionVirtualTable* BadCastTable() { return &badCastTable.functions; }
    const ExceptionVirtualTable* LogicErrorTable() { return &logicErrorTable.functions; }
    const ExceptionVirtualTable* LengthErrorTable() { return &lengthErrorTable.functions; }
    const ExceptionVirtualTable* OutOfRangeTable() { return &outOfRangeTable.functions; }
    const ExceptionVirtualTable* RuntimeErrorTable() { return &runtimeErrorTable.functions; }
    const ExceptionVirtualTable* IosFailureTable() { return &iosFailureTable.functions; }
    const ExceptionVirtualTable* BadAllocTable() { return &badAllocTable.functions; }
    const StreambufVirtualTable* StreambufTable() { return &streambufTable.functions; }
    const StreambufVirtualTable* FilebufTable() { return &filebufTable.functions; }
    const NativeThrowInfo* BadCastThrowInfo() { return &badCastThrowInfo; }
    const NativeThrowInfo* LogicErrorThrowInfo() { return &logicErrorThrowInfo; }
    const NativeThrowInfo* LengthErrorThrowInfo() { return &lengthErrorThrowInfo; }
    const NativeThrowInfo* OutOfRangeThrowInfo() { return &outOfRangeThrowInfo; }
    const NativeThrowInfo* RuntimeErrorThrowInfo() { return &runtimeErrorThrowInfo; }
    const NativeThrowInfo* IosFailureThrowInfo() { return &iosFailureThrowInfo; }
    const NativeThrowInfo* BadAllocThrowInfo() { return &badAllocThrowInfo; }

    void InitializeLocaleVirtualTables()
    {
        typeInfoTable.functions.destroy = NativeMethod<decltype(typeInfoTable.functions.destroy)>(&NativeTypeDescriptor::ScalarDeletingDestructor);
        exceptionTable.functions.destroy = NativeMethod<decltype(exceptionTable.functions.destroy)>(&RecordDBExceptionControl::ScalarDeletingDestructor);
        exceptionTable.functions.what = NativeMethod<decltype(exceptionTable.functions.what)>(&RecordDBExceptionControl::what);
        exceptionCatchable.copy = NativeMethod<decltype(exceptionCatchable.copy)>(&RecordDBExceptionControl::CopyConstruct);
        badCastTable.functions.destroy = NativeMethod<decltype(badCastTable.functions.destroy)>(&RecordDBBadCastControl::ScalarDeletingDestructor);
        badCastTable.functions.what = NativeMethod<decltype(badCastTable.functions.what)>(&RecordDBExceptionControl::what);
        badCastTable.functions.raise = NativeMethod<decltype(badCastTable.functions.raise)>(&RecordDBBadCastControl::_Doraise);
        badCastCatchable.copy = NativeMethod<decltype(badCastCatchable.copy)>(&RecordDBBadCastControl::CopyConstruct);
        badCastThrowInfo.unwind = NativeMethod<decltype(badCastThrowInfo.unwind)>(&RecordDBBadCastControl::Destruct);
        logicErrorTable.functions.destroy = NativeMethod<decltype(logicErrorTable.functions.destroy)>(&RecordDBLogicErrorControl::ScalarDeletingDestructor);
        logicErrorTable.functions.what = NativeMethod<decltype(logicErrorTable.functions.what)>(&RecordDBLogicErrorControl::what);
        logicErrorTable.functions.raise = NativeMethod<decltype(logicErrorTable.functions.raise)>(&RecordDBLogicErrorControl::_Doraise);
        logicErrorCatchable.copy = NativeMethod<decltype(logicErrorCatchable.copy)>(&RecordDBLogicErrorControl::CopyConstruct);
        logicErrorThrowInfo.unwind = NativeMethod<decltype(logicErrorThrowInfo.unwind)>(&RecordDBLogicErrorControl::Destruct);
        lengthErrorTable.functions.destroy = NativeMethod<decltype(lengthErrorTable.functions.destroy)>(&RecordDBLengthErrorControl::ScalarDeletingDestructor);
        lengthErrorTable.functions.what = NativeMethod<decltype(lengthErrorTable.functions.what)>(&RecordDBLogicErrorControl::what);
        lengthErrorTable.functions.raise = NativeMethod<decltype(lengthErrorTable.functions.raise)>(&RecordDBLengthErrorControl::_Doraise);
        lengthErrorCatchable.copy = NativeMethod<decltype(lengthErrorCatchable.copy)>(&RecordDBLengthErrorControl::CopyConstruct);
        lengthErrorThrowInfo.unwind = NativeMethod<decltype(lengthErrorThrowInfo.unwind)>(&RecordDBLengthErrorControl::Destruct);
        outOfRangeTable.functions.destroy = NativeMethod<decltype(outOfRangeTable.functions.destroy)>(&RecordDBOutOfRangeControl::ScalarDeletingDestructor);
        outOfRangeTable.functions.what = NativeMethod<decltype(outOfRangeTable.functions.what)>(&RecordDBLogicErrorControl::what);
        outOfRangeTable.functions.raise = NativeMethod<decltype(outOfRangeTable.functions.raise)>(&RecordDBOutOfRangeControl::_Doraise);
        outOfRangeCatchable.copy = NativeMethod<decltype(outOfRangeCatchable.copy)>(&RecordDBOutOfRangeControl::CopyConstruct);
        outOfRangeThrowInfo.unwind = NativeMethod<decltype(outOfRangeThrowInfo.unwind)>(&RecordDBOutOfRangeControl::Destruct);
        runtimeErrorTable.functions.destroy = NativeMethod<decltype(runtimeErrorTable.functions.destroy)>(&RecordDBRuntimeErrorControl::ScalarDeletingDestructor);
        runtimeErrorTable.functions.what = NativeMethod<decltype(runtimeErrorTable.functions.what)>(&RecordDBRuntimeErrorControl::what);
        runtimeErrorTable.functions.raise = NativeMethod<decltype(runtimeErrorTable.functions.raise)>(&RecordDBRuntimeErrorControl::_Doraise);
        runtimeErrorCatchable.copy = NativeMethod<decltype(runtimeErrorCatchable.copy)>(&RecordDBRuntimeErrorControl::CopyConstruct);
        runtimeErrorThrowInfo.unwind = NativeMethod<decltype(runtimeErrorThrowInfo.unwind)>(&RecordDBRuntimeErrorControl::Destruct);
        iosFailureTable.functions.destroy = NativeMethod<decltype(iosFailureTable.functions.destroy)>(&RecordDBIosFailureControl::ScalarDeletingDestructor);
        iosFailureTable.functions.what = NativeMethod<decltype(iosFailureTable.functions.what)>(&RecordDBRuntimeErrorControl::what);
        iosFailureTable.functions.raise = NativeMethod<decltype(iosFailureTable.functions.raise)>(&RecordDBIosFailureControl::_Doraise);
        iosFailureCatchable.copy = NativeMethod<decltype(iosFailureCatchable.copy)>(&RecordDBIosFailureControl::CopyConstruct);
        iosFailureThrowInfo.unwind = NativeMethod<decltype(iosFailureThrowInfo.unwind)>(&RecordDBIosFailureControl::Destruct);
        badAllocTable.functions.destroy = NativeMethod<decltype(badAllocTable.functions.destroy)>(&RecordDBBadAllocControl::ScalarDeletingDestructor);
        badAllocTable.functions.what = NativeMethod<decltype(badAllocTable.functions.what)>(&RecordDBExceptionControl::what);
        badAllocTable.functions.raise = NativeMethod<decltype(badAllocTable.functions.raise)>(&RecordDBBadAllocControl::_Doraise);
        badAllocCatchable.copy = NativeMethod<decltype(badAllocCatchable.copy)>(&RecordDBBadAllocControl::CopyConstruct);
        badAllocThrowInfo.unwind = NativeMethod<decltype(badAllocThrowInfo.unwind)>(&RecordDBBadAllocControl::Destruct);
        iosBaseTable.functions.destroy = NativeMethod<decltype(iosBaseTable.functions.destroy)>(&StreamDestructorOperations::DeleteIosBase);
        iosTable.functions.destroy = NativeMethod<decltype(iosTable.functions.destroy)>(&StreamDestructorOperations::DeleteIos);
        istreamTable.functions.destroy = NativeMethod<decltype(istreamTable.functions.destroy)>(&StreamDestructorOperations::DeleteIstream);
        ostreamTable.functions.destroy = NativeMethod<decltype(ostreamTable.functions.destroy)>(&StreamDestructorOperations::DeleteOstream);
        ifstreamTable.destroy = NativeMethod<decltype(ifstreamTable.destroy)>(&StreamDestructorOperations::DeleteIfstream);
        ofstreamTable.destroy = NativeMethod<decltype(ofstreamTable.destroy)>(&StreamDestructorOperations::DeleteOfstream);
        streambufTable.functions.destroy = NativeMethod<decltype(streambufTable.functions.destroy)>(&RecordDBStreambufControl::ScalarDeletingDestructor);
        streambufTable.functions.overflow = NativeMethod<decltype(streambufTable.functions.overflow)>(&RecordDBStreambufControl::Overflow);
        streambufTable.functions.putbackFailure = NativeMethod<decltype(streambufTable.functions.putbackFailure)>(&RecordDBStreambufControl::Overflow);
        streambufTable.functions.available = NativeMethod<decltype(streambufTable.functions.available)>(&RecordDBStreambufControl::SyncDefault);
        streambufTable.functions.underflow = NativeMethod<decltype(streambufTable.functions.underflow)>(&RecordDBStreambufControl::Underflow);
        streambufTable.functions.uflow = NativeMethod<decltype(streambufTable.functions.uflow)>(&RecordDBStreambufControl::Uflow);
        streambufTable.functions.read = NativeMethod<decltype(streambufTable.functions.read)>(&RecordDBStreambufControl::Read);
        streambufTable.functions.write = NativeMethod<decltype(streambufTable.functions.write)>(&RecordDBStreambufControl::Write);
        streambufTable.functions.setBuffer = NativeMethod<decltype(streambufTable.functions.setBuffer)>(&RecordDBStreambufControl::Setbuf);
        streambufTable.functions.sync = NativeMethod<decltype(streambufTable.functions.sync)>(&RecordDBStreambufControl::SyncDefault);
        streambufTable.functions.seekOffset = NativeMethod<decltype(streambufTable.functions.seekOffset)>(&StreambufDefaultOperations::SeekOffset);
        streambufTable.functions.seekPosition = NativeMethod<decltype(streambufTable.functions.seekPosition)>(&StreambufDefaultOperations::SeekPosition);
        filebufTable.functions.destroy = NativeMethod<decltype(filebufTable.functions.destroy)>(&RecordDBFilebufControl::ScalarDeletingDestructor);
        filebufTable.functions.overflow = NativeMethod<decltype(filebufTable.functions.overflow)>(&RecordDBFilebufControl::Overflow);
        filebufTable.functions.putbackFailure = NativeMethod<decltype(filebufTable.functions.putbackFailure)>(&RecordDBFilebufControl::Pbackfail);
        filebufTable.functions.available = NativeMethod<decltype(filebufTable.functions.available)>(&RecordDBStreambufControl::SyncDefault);
        filebufTable.functions.underflow = NativeMethod<decltype(filebufTable.functions.underflow)>(&RecordDBFilebufControl::Underflow);
        filebufTable.functions.uflow = NativeMethod<decltype(filebufTable.functions.uflow)>(&RecordDBFilebufControl::Uflow);
        filebufTable.functions.read = NativeMethod<decltype(filebufTable.functions.read)>(&RecordDBStreambufControl::Read);
        filebufTable.functions.write = NativeMethod<decltype(filebufTable.functions.write)>(&RecordDBStreambufControl::Write);
        filebufTable.functions.setBuffer = NativeMethod<decltype(filebufTable.functions.setBuffer)>(&RecordDBFilebufControl::Setbuf);
        filebufTable.functions.sync = NativeMethod<decltype(filebufTable.functions.sync)>(&RecordDBFilebufControl::Sync);
        filebufTable.functions.seekOffset = NativeMethod<decltype(filebufTable.functions.seekOffset)>(&RecordDBFilebufControl::Seekoff);
        filebufTable.functions.seekPosition = NativeMethod<decltype(filebufTable.functions.seekPosition)>(&RecordDBFilebufControl::Seekpos);
        baseFacetTable.functions.destroy = NativeMethod<decltype(baseFacetTable.functions.destroy)>(&MsvcLocaleFacetRef::DeleteFacet);
        localeImplTable.functions.destroy = NativeMethod<decltype(localeImplTable.functions.destroy)>(&RecordDBLocaleImplControl::ScalarDeletingDestructor);
        charCtypeTable.functions.destroy = NativeMethod<decltype(charCtypeTable.functions.destroy)>(&RecordDBCtypeControl::ScalarDeletingDestructor);
        charNumpunctTable.functions.destroy = NativeMethod<decltype(charNumpunctTable.functions.destroy)>(&RecordDBNumpunctControl::ScalarDeletingDestructor);
        charCodecvtTable.functions.destroy = NativeMethod<decltype(charCodecvtTable.functions.destroy)>(&MsvcLocaleFacetRef::DeleteNumPut);
        charNumPutTable.functions.destroy = NativeMethod<decltype(charNumPutTable.functions.destroy)>(&MsvcLocaleFacetRef::DeleteNumPut);
        charCtypeTable.functions.lowerRange = NativeMethod<decltype(charCtypeTable.functions.lowerRange)>(&RecordDBCtypeControl::ToLowerRange);
        charCtypeTable.functions.lower = NativeMethod<decltype(charCtypeTable.functions.lower)>(&RecordDBCtypeControl::ToLower);
        charCtypeTable.functions.upperRange = NativeMethod<decltype(charCtypeTable.functions.upperRange)>(&RecordDBCtypeControl::ToUpperRange);
        charCtypeTable.functions.upper = NativeMethod<decltype(charCtypeTable.functions.upper)>(&RecordDBCtypeControl::ToUpper);
        charNumpunctTable.functions.decimalPoint = NativeMethod<decltype(charNumpunctTable.functions.decimalPoint)>(&RecordDBNumpunctControl::DecimalPoint);
        charNumpunctTable.functions.thousandsSeparator = NativeMethod<decltype(charNumpunctTable.functions.thousandsSeparator)>(&RecordDBNumpunctControl::ThousandsSeparator);
        charNumpunctTable.functions.grouping = NativeMethod<decltype(charNumpunctTable.functions.grouping)>(&RecordDBNumpunctControl::Grouping);
        charNumpunctTable.functions.falseName = NativeMethod<decltype(charNumpunctTable.functions.falseName)>(&RecordDBNumpunctControl::FalseName);
        charNumpunctTable.functions.trueName = NativeMethod<decltype(charNumpunctTable.functions.trueName)>(&RecordDBNumpunctControl::TrueName);
        charNumGetTable.functions.getPointer = NativeMethod<decltype(charNumGetTable.functions.getPointer)>(&NumGetOperations<char>::GetPointer);
        charNumGetTable.functions.getLongDouble = NativeMethod<decltype(charNumGetTable.functions.getLongDouble)>(&NumGetOperations<char>::GetLongDouble);
        charNumGetTable.functions.getDouble = NativeMethod<decltype(charNumGetTable.functions.getDouble)>(&NumGetOperations<char>::GetDouble);
        charNumGetTable.functions.getFloat = NativeMethod<decltype(charNumGetTable.functions.getFloat)>(&NumGetOperations<char>::GetFloat);
        charNumGetTable.functions.getUnsignedLong = NativeMethod<decltype(charNumGetTable.functions.getUnsignedLong)>(&NumGetOperations<char>::GetUnsignedLong);
        charNumGetTable.functions.getLong = NativeMethod<decltype(charNumGetTable.functions.getLong)>(&NumGetOperations<char>::GetLong);
        charNumGetTable.functions.getUnsignedInt = NativeMethod<decltype(charNumGetTable.functions.getUnsignedInt)>(&NumGetOperations<char>::GetUnsignedInt);
        charNumGetTable.functions.getUnsignedShort = NativeMethod<decltype(charNumGetTable.functions.getUnsignedShort)>(&NumGetOperations<char>::GetUnsignedShort);
        charNumGetTable.functions.getBoolean = NativeMethod<decltype(charNumGetTable.functions.getBoolean)>(&NumGetOperations<char>::GetBoolean);
        charNumPutTable.functions.putPointer = NativeMethod<decltype(charNumPutTable.functions.putPointer)>(&NumPutOperations<char>::PutPointer);
        charNumPutTable.functions.putLongDouble = NativeMethod<decltype(charNumPutTable.functions.putLongDouble)>(&NumPutOperations<char>::PutLongDouble);
        charNumPutTable.functions.putDouble = NativeMethod<decltype(charNumPutTable.functions.putDouble)>(&NumPutOperations<char>::PutDouble);
        charNumPutTable.functions.putUnsignedLong = NativeMethod<decltype(charNumPutTable.functions.putUnsignedLong)>(&NumPutOperations<char>::PutUnsignedLong);
        charNumPutTable.functions.putLong = NativeMethod<decltype(charNumPutTable.functions.putLong)>(&NumPutOperations<char>::PutLong);
        charNumPutTable.functions.putBoolean = NativeMethod<decltype(charNumPutTable.functions.putBoolean)>(&NumPutOperations<char>::PutBoolean);
        charCodecvtTable.functions.alwaysNoConversion = NativeMethod<decltype(charCodecvtTable.functions.alwaysNoConversion)>(&CharCodecvtOperations::AlwaysNoConversion);
        charCodecvtTable.functions.maxLength = NativeMethod<decltype(charCodecvtTable.functions.maxLength)>(&CharCodecvtOperations::MaximumLength);
        charCodecvtTable.functions.encoding = NativeMethod<decltype(charCodecvtTable.functions.encoding)>(&CharCodecvtOperations::Encoding);
        charCodecvtTable.functions.in = NativeMethod<decltype(charCodecvtTable.functions.in)>(&CharCodecvtOperations::In);
        charCodecvtTable.functions.out = NativeMethod<decltype(charCodecvtTable.functions.out)>(&CharCodecvtOperations::Out);
        charCodecvtTable.functions.length = NativeMethod<decltype(charCodecvtTable.functions.length)>(&CharCodecvtOperations::Length);
        charCollateTable.functions.compare = NativeMethod<decltype(charCollateTable.functions.compare)>(&CollateOperations<char>::Compare);
        charCollateTable.functions.transform = NativeMethod<decltype(charCollateTable.functions.transform)>(&CollateOperations<char>::Transform);
        charCollateTable.functions.hash = NativeMethod<decltype(charCollateTable.functions.hash)>(&CollateOperations<char>::Hash);
        charMessagesTable.functions.open = NativeMethod<decltype(charMessagesTable.functions.open)>(&MessagesOperations<char>::Open);
        charMessagesTable.functions.get = NativeMethod<decltype(charMessagesTable.functions.get)>(&MessagesOperations<char>::Get);
        charMessagesTable.functions.close = NativeMethod<decltype(charMessagesTable.functions.close)>(&MessagesOperations<char>::Close);
        charMoneyGetTable.functions.getString = NativeMethod<decltype(charMoneyGetTable.functions.getString)>(&MoneyGetOperations<char>::GetString);
        charMoneyGetTable.functions.getNumber = NativeMethod<decltype(charMoneyGetTable.functions.getNumber)>(&MoneyGetOperations<char>::GetValue);
        charMoneyPutTable.functions.putString = NativeMethod<decltype(charMoneyPutTable.functions.putString)>(&MoneyPutOperations<char>::PutString);
        charMoneyPutTable.functions.putNumber = NativeMethod<decltype(charMoneyPutTable.functions.putNumber)>(&MoneyPutOperations<char>::PutValue);
        charMoneypunctTable.functions.decimalPoint = NativeMethod<decltype(charMoneypunctTable.functions.decimalPoint)>(&MoneypunctOperations<char>::DecimalPoint);
        charMoneypunctTable.functions.thousandsSeparator = NativeMethod<decltype(charMoneypunctTable.functions.thousandsSeparator)>(&MoneypunctOperations<char>::ThousandsSeparator);
        charMoneypunctTable.functions.grouping = NativeMethod<decltype(charMoneypunctTable.functions.grouping)>(&MoneypunctOperations<char>::Grouping);
        charMoneypunctTable.functions.currencySymbol = NativeMethod<decltype(charMoneypunctTable.functions.currencySymbol)>(&MoneypunctOperations<char>::CurrencySymbol);
        charMoneypunctTable.functions.positiveSign = NativeMethod<decltype(charMoneypunctTable.functions.positiveSign)>(&MoneypunctOperations<char>::PositiveSign);
        charMoneypunctTable.functions.negativeSign = NativeMethod<decltype(charMoneypunctTable.functions.negativeSign)>(&MoneypunctOperations<char>::NegativeSign);
        charMoneypunctTable.functions.fractionDigits = NativeMethod<decltype(charMoneypunctTable.functions.fractionDigits)>(&MoneypunctOperations<char>::FractionDigits);
        charMoneypunctTable.functions.positivePattern = NativeMethod<decltype(charMoneypunctTable.functions.positivePattern)>(&MoneypunctOperations<char>::PositivePattern);
        charMoneypunctTable.functions.negativePattern = NativeMethod<decltype(charMoneypunctTable.functions.negativePattern)>(&MoneypunctOperations<char>::NegativePattern);
        charMoneypunctIntlTable.functions.decimalPoint = NativeMethod<decltype(charMoneypunctIntlTable.functions.decimalPoint)>(&MoneypunctOperations<char>::DecimalPoint);
        charMoneypunctIntlTable.functions.thousandsSeparator = NativeMethod<decltype(charMoneypunctIntlTable.functions.thousandsSeparator)>(&MoneypunctOperations<char>::ThousandsSeparator);
        charMoneypunctIntlTable.functions.grouping = NativeMethod<decltype(charMoneypunctIntlTable.functions.grouping)>(&MoneypunctOperations<char>::Grouping);
        charMoneypunctIntlTable.functions.currencySymbol = NativeMethod<decltype(charMoneypunctIntlTable.functions.currencySymbol)>(&MoneypunctOperations<char>::CurrencySymbol);
        charMoneypunctIntlTable.functions.positiveSign = NativeMethod<decltype(charMoneypunctIntlTable.functions.positiveSign)>(&MoneypunctOperations<char>::PositiveSign);
        charMoneypunctIntlTable.functions.negativeSign = NativeMethod<decltype(charMoneypunctIntlTable.functions.negativeSign)>(&MoneypunctOperations<char>::NegativeSign);
        charMoneypunctIntlTable.functions.fractionDigits = NativeMethod<decltype(charMoneypunctIntlTable.functions.fractionDigits)>(&MoneypunctOperations<char>::FractionDigits);
        charMoneypunctIntlTable.functions.positivePattern = NativeMethod<decltype(charMoneypunctIntlTable.functions.positivePattern)>(&MoneypunctOperations<char>::PositivePattern);
        charMoneypunctIntlTable.functions.negativePattern = NativeMethod<decltype(charMoneypunctIntlTable.functions.negativePattern)>(&MoneypunctOperations<char>::NegativePattern);
        charTimeGetTable.functions.dateOrder = NativeMethod<decltype(charTimeGetTable.functions.dateOrder)>(&TimeGetOperations<char>::DateOrder);
        charTimeGetTable.functions.getTime = NativeMethod<decltype(charTimeGetTable.functions.getTime)>(&TimeGetOperations<char>::GetTime);
        charTimeGetTable.functions.getDate = NativeMethod<decltype(charTimeGetTable.functions.getDate)>(&TimeGetOperations<char>::GetDate);
        charTimeGetTable.functions.getWeekday = NativeMethod<decltype(charTimeGetTable.functions.getWeekday)>(&TimeGetOperations<char>::GetWeekday);
        charTimeGetTable.functions.getMonth = NativeMethod<decltype(charTimeGetTable.functions.getMonth)>(&TimeGetOperations<char>::GetMonth);
        charTimeGetTable.functions.getYear = NativeMethod<decltype(charTimeGetTable.functions.getYear)>(&TimeGetOperations<char>::GetYear);
        charTimePutTable.functions.put = NativeMethod<decltype(charTimePutTable.functions.put)>(&TimePutOperations<char>::Put);
        wideNumGetTable.functions.getPointer = NativeMethod<decltype(wideNumGetTable.functions.getPointer)>(&NumGetOperations<unsigned short>::GetPointer);
        wideNumGetTable.functions.getLongDouble = NativeMethod<decltype(wideNumGetTable.functions.getLongDouble)>(&NumGetOperations<unsigned short>::GetLongDouble);
        wideNumGetTable.functions.getDouble = NativeMethod<decltype(wideNumGetTable.functions.getDouble)>(&NumGetOperations<unsigned short>::GetDouble);
        wideNumGetTable.functions.getFloat = NativeMethod<decltype(wideNumGetTable.functions.getFloat)>(&NumGetOperations<unsigned short>::GetFloat);
        wideNumGetTable.functions.getUnsignedLong = NativeMethod<decltype(wideNumGetTable.functions.getUnsignedLong)>(&NumGetOperations<unsigned short>::GetUnsignedLong);
        wideNumGetTable.functions.getLong = NativeMethod<decltype(wideNumGetTable.functions.getLong)>(&NumGetOperations<unsigned short>::GetLong);
        wideNumGetTable.functions.getUnsignedInt = NativeMethod<decltype(wideNumGetTable.functions.getUnsignedInt)>(&NumGetOperations<unsigned short>::GetUnsignedInt);
        wideNumGetTable.functions.getUnsignedShort = NativeMethod<decltype(wideNumGetTable.functions.getUnsignedShort)>(&NumGetOperations<unsigned short>::GetUnsignedShort);
        wideNumGetTable.functions.getBoolean = NativeMethod<decltype(wideNumGetTable.functions.getBoolean)>(&NumGetOperations<unsigned short>::GetBoolean);
        wideNumPutTable.functions.putPointer = NativeMethod<decltype(wideNumPutTable.functions.putPointer)>(&NumPutOperations<unsigned short>::PutPointer);
        wideNumPutTable.functions.putLongDouble = NativeMethod<decltype(wideNumPutTable.functions.putLongDouble)>(&NumPutOperations<unsigned short>::PutLongDouble);
        wideNumPutTable.functions.putDouble = NativeMethod<decltype(wideNumPutTable.functions.putDouble)>(&NumPutOperations<unsigned short>::PutDouble);
        wideNumPutTable.functions.putUnsignedLong = NativeMethod<decltype(wideNumPutTable.functions.putUnsignedLong)>(&NumPutOperations<unsigned short>::PutUnsignedLong);
        wideNumPutTable.functions.putLong = NativeMethod<decltype(wideNumPutTable.functions.putLong)>(&NumPutOperations<unsigned short>::PutLong);
        wideNumPutTable.functions.putBoolean = NativeMethod<decltype(wideNumPutTable.functions.putBoolean)>(&NumPutOperations<unsigned short>::PutBoolean);
        wideNumpunctTable.functions.decimalPoint = NativeMethod<decltype(wideNumpunctTable.functions.decimalPoint)>(&NumpunctOperations<unsigned short>::DecimalPoint);
        wideNumpunctTable.functions.thousandsSeparator = NativeMethod<decltype(wideNumpunctTable.functions.thousandsSeparator)>(&NumpunctOperations<unsigned short>::ThousandsSeparator);
        wideNumpunctTable.functions.grouping = NativeMethod<decltype(wideNumpunctTable.functions.grouping)>(&NumpunctOperations<unsigned short>::Grouping);
        wideNumpunctTable.functions.falseName = NativeMethod<decltype(wideNumpunctTable.functions.falseName)>(&NumpunctOperations<unsigned short>::FalseName);
        wideNumpunctTable.functions.trueName = NativeMethod<decltype(wideNumpunctTable.functions.trueName)>(&NumpunctOperations<unsigned short>::TrueName);
        wideCodecvtTable.functions.alwaysNoConversion = NativeMethod<decltype(wideCodecvtTable.functions.alwaysNoConversion)>(&WideCodecvtOperations::AlwaysNoConversion);
        wideCodecvtTable.functions.maxLength = NativeMethod<decltype(wideCodecvtTable.functions.maxLength)>(&WideCodecvtOperations::MaximumLength);
        wideCodecvtTable.functions.encoding = NativeMethod<decltype(wideCodecvtTable.functions.encoding)>(&WideCodecvtOperations::Encoding);
        wideCodecvtTable.functions.in = NativeMethod<decltype(wideCodecvtTable.functions.in)>(&WideCodecvtOperations::In);
        wideCodecvtTable.functions.out = NativeMethod<decltype(wideCodecvtTable.functions.out)>(&WideCodecvtOperations::Out);
        wideCodecvtTable.functions.length = NativeMethod<decltype(wideCodecvtTable.functions.length)>(&WideCodecvtOperations::Length);
        wideCtypeTable.functions.classifyRange = NativeMethod<decltype(wideCtypeTable.functions.classifyRange)>(&WideCtypeOperations::ClassifyRange);
        wideCtypeTable.functions.classify = NativeMethod<decltype(wideCtypeTable.functions.classify)>(&WideCtypeOperations::Classify);
        wideCtypeTable.functions.scanIs = NativeMethod<decltype(wideCtypeTable.functions.scanIs)>(&WideCtypeOperations::ScanIs);
        wideCtypeTable.functions.scanNot = NativeMethod<decltype(wideCtypeTable.functions.scanNot)>(&WideCtypeOperations::ScanNot);
        wideCtypeTable.functions.lowerRange = NativeMethod<decltype(wideCtypeTable.functions.lowerRange)>(&WideCtypeOperations::ToLowerRange);
        wideCtypeTable.functions.lower = NativeMethod<decltype(wideCtypeTable.functions.lower)>(&WideCtypeOperations::ToLower);
        wideCtypeTable.functions.upperRange = NativeMethod<decltype(wideCtypeTable.functions.upperRange)>(&WideCtypeOperations::ToUpperRange);
        wideCtypeTable.functions.upper = NativeMethod<decltype(wideCtypeTable.functions.upper)>(&WideCtypeOperations::ToUpper);
        wideCtypeTable.functions.widenRange = NativeMethod<decltype(wideCtypeTable.functions.widenRange)>(&WideCtypeOperations::WidenRange);
        wideCtypeTable.functions.widen = NativeMethod<decltype(wideCtypeTable.functions.widen)>(&WideCtypeOperations::Widen);
        wideCtypeTable.functions.narrowRange = NativeMethod<decltype(wideCtypeTable.functions.narrowRange)>(&WideCtypeOperations::NarrowRange);
        wideCtypeTable.functions.narrow = NativeMethod<decltype(wideCtypeTable.functions.narrow)>(&WideCtypeOperations::Narrow);
        wideCollateTable.functions.compare = NativeMethod<decltype(wideCollateTable.functions.compare)>(&CollateOperations<unsigned short>::Compare);
        wideCollateTable.functions.transform = NativeMethod<decltype(wideCollateTable.functions.transform)>(&CollateOperations<unsigned short>::Transform);
        wideCollateTable.functions.hash = NativeMethod<decltype(wideCollateTable.functions.hash)>(&CollateOperations<unsigned short>::Hash);
        wideMessagesTable.functions.open = NativeMethod<decltype(wideMessagesTable.functions.open)>(&MessagesOperations<unsigned short>::Open);
        wideMessagesTable.functions.get = NativeMethod<decltype(wideMessagesTable.functions.get)>(&MessagesOperations<unsigned short>::Get);
        wideMessagesTable.functions.close = NativeMethod<decltype(wideMessagesTable.functions.close)>(&MessagesOperations<unsigned short>::Close);
        wideMoneyGetTable.functions.getString = NativeMethod<decltype(wideMoneyGetTable.functions.getString)>(&MoneyGetOperations<unsigned short>::GetString);
        wideMoneyGetTable.functions.getNumber = NativeMethod<decltype(wideMoneyGetTable.functions.getNumber)>(&MoneyGetOperations<unsigned short>::GetValue);
        wideMoneyPutTable.functions.putString = NativeMethod<decltype(wideMoneyPutTable.functions.putString)>(&MoneyPutOperations<unsigned short>::PutString);
        wideMoneyPutTable.functions.putNumber = NativeMethod<decltype(wideMoneyPutTable.functions.putNumber)>(&MoneyPutOperations<unsigned short>::PutValue);
        wideMoneypunctTable.functions.decimalPoint = NativeMethod<decltype(wideMoneypunctTable.functions.decimalPoint)>(&MoneypunctOperations<unsigned short>::DecimalPoint);
        wideMoneypunctTable.functions.thousandsSeparator = NativeMethod<decltype(wideMoneypunctTable.functions.thousandsSeparator)>(&MoneypunctOperations<unsigned short>::ThousandsSeparator);
        wideMoneypunctTable.functions.grouping = NativeMethod<decltype(wideMoneypunctTable.functions.grouping)>(&MoneypunctOperations<unsigned short>::Grouping);
        wideMoneypunctTable.functions.currencySymbol = NativeMethod<decltype(wideMoneypunctTable.functions.currencySymbol)>(&MoneypunctOperations<unsigned short>::CurrencySymbol);
        wideMoneypunctTable.functions.positiveSign = NativeMethod<decltype(wideMoneypunctTable.functions.positiveSign)>(&MoneypunctOperations<unsigned short>::PositiveSign);
        wideMoneypunctTable.functions.negativeSign = NativeMethod<decltype(wideMoneypunctTable.functions.negativeSign)>(&MoneypunctOperations<unsigned short>::NegativeSign);
        wideMoneypunctTable.functions.fractionDigits = NativeMethod<decltype(wideMoneypunctTable.functions.fractionDigits)>(&MoneypunctOperations<unsigned short>::FractionDigits);
        wideMoneypunctTable.functions.positivePattern = NativeMethod<decltype(wideMoneypunctTable.functions.positivePattern)>(&MoneypunctOperations<unsigned short>::PositivePattern);
        wideMoneypunctTable.functions.negativePattern = NativeMethod<decltype(wideMoneypunctTable.functions.negativePattern)>(&MoneypunctOperations<unsigned short>::NegativePattern);
        wideMoneypunctIntlTable.functions.decimalPoint = NativeMethod<decltype(wideMoneypunctIntlTable.functions.decimalPoint)>(&MoneypunctOperations<unsigned short>::DecimalPoint);
        wideMoneypunctIntlTable.functions.thousandsSeparator = NativeMethod<decltype(wideMoneypunctIntlTable.functions.thousandsSeparator)>(&MoneypunctOperations<unsigned short>::ThousandsSeparator);
        wideMoneypunctIntlTable.functions.grouping = NativeMethod<decltype(wideMoneypunctIntlTable.functions.grouping)>(&MoneypunctOperations<unsigned short>::Grouping);
        wideMoneypunctIntlTable.functions.currencySymbol = NativeMethod<decltype(wideMoneypunctIntlTable.functions.currencySymbol)>(&MoneypunctOperations<unsigned short>::CurrencySymbol);
        wideMoneypunctIntlTable.functions.positiveSign = NativeMethod<decltype(wideMoneypunctIntlTable.functions.positiveSign)>(&MoneypunctOperations<unsigned short>::PositiveSign);
        wideMoneypunctIntlTable.functions.negativeSign = NativeMethod<decltype(wideMoneypunctIntlTable.functions.negativeSign)>(&MoneypunctOperations<unsigned short>::NegativeSign);
        wideMoneypunctIntlTable.functions.fractionDigits = NativeMethod<decltype(wideMoneypunctIntlTable.functions.fractionDigits)>(&MoneypunctOperations<unsigned short>::FractionDigits);
        wideMoneypunctIntlTable.functions.positivePattern = NativeMethod<decltype(wideMoneypunctIntlTable.functions.positivePattern)>(&MoneypunctOperations<unsigned short>::PositivePattern);
        wideMoneypunctIntlTable.functions.negativePattern = NativeMethod<decltype(wideMoneypunctIntlTable.functions.negativePattern)>(&MoneypunctOperations<unsigned short>::NegativePattern);
        wideTimeGetTable.functions.dateOrder = NativeMethod<decltype(wideTimeGetTable.functions.dateOrder)>(&TimeGetOperations<unsigned short>::DateOrder);
        wideTimeGetTable.functions.getTime = NativeMethod<decltype(wideTimeGetTable.functions.getTime)>(&TimeGetOperations<unsigned short>::GetTime);
        wideTimeGetTable.functions.getDate = NativeMethod<decltype(wideTimeGetTable.functions.getDate)>(&TimeGetOperations<unsigned short>::GetDate);
        wideTimeGetTable.functions.getWeekday = NativeMethod<decltype(wideTimeGetTable.functions.getWeekday)>(&TimeGetOperations<unsigned short>::GetWeekday);
        wideTimeGetTable.functions.getMonth = NativeMethod<decltype(wideTimeGetTable.functions.getMonth)>(&TimeGetOperations<unsigned short>::GetMonth);
        wideTimeGetTable.functions.getYear = NativeMethod<decltype(wideTimeGetTable.functions.getYear)>(&TimeGetOperations<unsigned short>::GetYear);
        wideTimePutTable.functions.put = NativeMethod<decltype(wideTimePutTable.functions.put)>(&TimePutOperations<unsigned short>::Put);
    }
}
