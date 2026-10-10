#pragma once
#include "locale_tables.h"

namespace Msvc6
{
    template<class Character> struct NumpunctOperations : NumpunctData<Character>
    {
        Character __thiscall DecimalPoint() const;
        Character __thiscall ThousandsSeparator() const;
        StringData<char>* __thiscall Grouping(StringData<char>*) const;
        StringData<Character>* __thiscall FalseName(StringData<Character>*) const;
        StringData<Character>* __thiscall TrueName(StringData<Character>*) const;
    };
    template<class Character> struct MoneypunctOperations : MoneypunctData<Character>
    {
        Character __thiscall DecimalPoint() const;
        Character __thiscall ThousandsSeparator() const;
        StringData<char>* __thiscall Grouping(StringData<char>*) const;
        StringData<Character>* __thiscall CurrencySymbol(StringData<Character>*) const;
        StringData<Character>* __thiscall PositiveSign(StringData<Character>*) const;
        StringData<Character>* __thiscall NegativeSign(StringData<Character>*) const;
        int __thiscall FractionDigits() const;
        MoneyPattern* __thiscall PositivePattern(MoneyPattern*) const;
        MoneyPattern* __thiscall NegativePattern(MoneyPattern*) const;
    };
    template<class Character> struct MessagesOperations : MessagesData<Character>
    {
        int __thiscall Open(const StringData<char>*, const RecordDBLocaleControl*) const;
        StringData<Character>* __thiscall Get(StringData<Character>*, int, int, int, const StringData<Character>*) const;
        void __thiscall Close(int) const;
    };
    struct WideCtypeOperations : CtypeData
    {
        const unsigned short* __thiscall ClassifyRange(const unsigned short*, const unsigned short*, unsigned short*) const;
        bool __thiscall Classify(unsigned short mask, unsigned short value) const;
        const unsigned short* __thiscall ScanIs(unsigned short mask, const unsigned short*, const unsigned short*) const;
        const unsigned short* __thiscall ScanNot(unsigned short mask, const unsigned short*, const unsigned short*) const;
        const unsigned short* __thiscall ToLowerRange(unsigned short*, const unsigned short*) const;
        unsigned short __thiscall ToLower(unsigned short) const;
        const unsigned short* __thiscall ToUpperRange(unsigned short*, const unsigned short*) const;
        unsigned short __thiscall ToUpper(unsigned short) const;
        const char* __thiscall WidenRange(const char*, const char*, unsigned short*) const;
        unsigned short __thiscall Widen(char) const;
        const unsigned short* __thiscall NarrowRange(const unsigned short*, const unsigned short*, char, char*) const;
        char __thiscall Narrow(unsigned short, char) const;
    };
    struct CharCodecvtOperations : LocaleStampData
    {
        bool __thiscall AlwaysNoConversion() const;
        int __thiscall MaximumLength() const;
        int __thiscall Encoding() const;
        int __thiscall In(unsigned int&, const char*, const char*, const char*&, char*, char*, char*&) const;
        int __thiscall Out(unsigned int&, const char*, const char*, const char*&, char*, char*, char*&) const;
        int __thiscall Length(unsigned int&, const char*, const char*, unsigned int) const;
    };
    struct WideCodecvtOperations : LocaleStampData
    {
        bool __thiscall AlwaysNoConversion() const;
        int __thiscall MaximumLength() const;
        int __thiscall Encoding() const;
        int __thiscall In(unsigned int&, const char*, const char*, const char*&, unsigned short*, unsigned short*, unsigned short*&) const;
        int __thiscall Out(unsigned int&, const unsigned short*, const unsigned short*, const unsigned short*&, char*, char*, char*&) const;
        int __thiscall Length(unsigned int&, const unsigned short*, const unsigned short*, unsigned int) const;
    };
    template<class Character> struct CollateOperations : LocaleStampData
    {
        int __thiscall Compare(const Character*, const Character*, const Character*, const Character*) const;
        StringData<Character>* __thiscall Transform(StringData<Character>*, const Character*, const Character*) const;
        long __thiscall Hash(const Character*, const Character*) const;
    };
    template<class Character> struct NumGetOperations : FacetData
    {
        InputIterator<Character>* __thiscall GetPointer(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, void*&) const;
        InputIterator<Character>* __thiscall GetLongDouble(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, long double&) const;
        InputIterator<Character>* __thiscall GetDouble(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, double&) const;
        InputIterator<Character>* __thiscall GetFloat(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, float&) const;
        InputIterator<Character>* __thiscall GetUnsignedLong(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, unsigned long&) const;
        InputIterator<Character>* __thiscall GetLong(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, long&) const;
        InputIterator<Character>* __thiscall GetUnsignedInt(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, unsigned int&) const;
        InputIterator<Character>* __thiscall GetUnsignedShort(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, unsigned short&) const;
        InputIterator<Character>* __thiscall GetBoolean(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, bool&) const;
    };
    template<class Character> struct NumPutOperations : FacetData
    {
        OutputIterator<Character>* __thiscall PutPointer(OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl&, Character, const void*) const;
        OutputIterator<Character>* __thiscall PutLongDouble(OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl&, Character, long double) const;
        OutputIterator<Character>* __thiscall PutDouble(OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl&, Character, double) const;
        OutputIterator<Character>* __thiscall PutUnsignedLong(OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl&, Character, unsigned long) const;
        OutputIterator<Character>* __thiscall PutLong(OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl&, Character, long) const;
        OutputIterator<Character>* __thiscall PutBoolean(OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl&, Character, bool) const;
    };
    template<class Character> struct MoneyGetOperations : FacetData
    {
        InputIterator<Character>* __thiscall GetString(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, bool, RecordDBIosControl&, unsigned int&, StringData<Character>&) const;
        InputIterator<Character>* __thiscall GetValue(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, bool, RecordDBIosControl&, unsigned int&, long double&) const;
    };
    template<class Character> struct MoneyPutOperations : FacetData
    {
        OutputIterator<Character>* __thiscall PutString(OutputIterator<Character>*, OutputIterator<Character>, bool, RecordDBIosControl&, Character, const StringData<Character>&) const;
        OutputIterator<Character>* __thiscall PutValue(OutputIterator<Character>*, OutputIterator<Character>, bool, RecordDBIosControl&, Character, long double) const;
    };
    template<class Character> struct TimeGetOperations : TimeGetData<Character>
    {
        int __thiscall DateOrder() const;
        InputIterator<Character>* __thiscall GetTime(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, tm*) const;
        InputIterator<Character>* __thiscall GetDate(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, tm*) const;
        InputIterator<Character>* __thiscall GetWeekday(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, tm*) const;
        InputIterator<Character>* __thiscall GetMonth(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, tm*) const;
        InputIterator<Character>* __thiscall GetYear(InputIterator<Character>*, InputIterator<Character>, InputIterator<Character>, RecordDBIosControl&, unsigned int&, tm*) const;
    };
    template<class Character> struct TimePutOperations : TimePutData
    {
        OutputIterator<Character>* __thiscall Put(OutputIterator<Character>*, OutputIterator<Character>, RecordDBIosControl&, const tm*, char, char) const;
    };
}
