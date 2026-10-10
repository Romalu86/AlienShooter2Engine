#ifndef AS2_RUNTIME_CRT_LOCALE_H
#define AS2_RUNTIME_CRT_LOCALE_H

struct tm;
struct RecordDBCtypeDescriptor;

// The VC6 locale records used by the engine have their own layout. They do
// not alias the records owned by the compiler's current C runtime.
struct CrtLocaleConvention
{
    const char* decimalPoint;
    const char* thousandsSeparator;
    const char* grouping;
    const char* internationalCurrencySymbol;
    const char* currencySymbol;
    const char* monetaryDecimalPoint;
    const char* monetaryThousandsSeparator;
    const char* monetaryGrouping;
    const char* positiveSign;
    const char* negativeSign;
    signed char internationalFractionDigits;
    signed char fractionDigits;
    unsigned char positiveCurrencyPrecedes;
    unsigned char positiveSpaceSeparates;
    unsigned char negativeCurrencyPrecedes;
    unsigned char negativeSpaceSeparates;
    signed char positiveSignPosition;
    signed char negativeSignPosition;
};

struct CrtTimeNames
{
    const char* abbreviatedDays[7];
    const char* days[7];
    const char* abbreviatedMonths[12];
    const char* months[12];
    const char* am;
    const char* pm;
    const char* shortDate;
    const char* longDate;
    const char* time;
};

namespace Msvc6
{
    extern const long long _Fpz;
    extern int FposInitialState;
    extern int FilebufInitialState;
    const CrtLocaleConvention* GetLocaleConvention();
    void LockTypeInfo();
    void UnlockTypeInfo();
    int MaximumMultibyteLength();
    int WideToMultibyte(char* destination, unsigned short value,
        unsigned int* state, const RecordDBCtypeDescriptor* descriptor);
    int WideToByte(unsigned short value);
    unsigned int FormatTime(char* destination, unsigned int capacity,
        const char* format, const tm* value, const CrtTimeNames* names);
    double ParseFloating(const char* source, char** end, int decimalExponent);
    void PrepareMoneyAmount(double& amount, bool& negative, unsigned int& extraZeros);
}

#endif
