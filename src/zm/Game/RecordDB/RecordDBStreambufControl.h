#pragma once
#include <stddef.h>
#include <cstdio>
#include "zRecordDB.h"
#include "runtime/locale_tables.h"

struct AS_RECORD_STRING_CONTROL;

struct RecordDBStreambufControl : Msvc6::StreambufData<char>
{

    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    int __thiscall Overflow(int value);
    int __thiscall Underflow();
    int __thiscall Uflow();
    int __thiscall Read(char* destination, int count);
    int __thiscall Write(const char* source, int count);
    int __thiscall SyncDefault();

    void* __thiscall Setbuf(char* buffer, int count);
};

struct RecordDBFilebufControl
{
    RecordDBStreambufControl streambuf;
    void* codecvt;
    unsigned int conversionState3C;
    unsigned int conversionState40;
    AS_RECORD_STRING_CONTROL* conversionBuffer;
    unsigned char ownsFile;
    unsigned char allocatorPadding49[3];
    void* locale;
    std::FILE* file;

    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    int __thiscall Overflow(int value);
    int __thiscall Pbackfail(int value);
    unsigned int __thiscall Underflow();
    int __thiscall Uflow();
    int __thiscall Sync();

    void* __thiscall Setbuf(char* buffer, int count);
    void* __thiscall Seekoff(void* out, int offset, int direction, unsigned int mode);
    void* __thiscall Seekpos(void* out, RecordDBPositionControl position, unsigned int mode);
};


struct RecordDBIosControl
{
    const Msvc6::FacetVirtualTable* vftable;
    unsigned int state;
    unsigned int exceptionMask;
    unsigned int formatFlags;
    unsigned int precision;
    unsigned int width;
    unsigned int reserved18;
    unsigned int reserved1C;
    void* locale;
    unsigned int registration;
    RecordDBStreambufControl* buffer;
    void* tiedStream;
    unsigned char fill;
    unsigned char padding31[3];
};

struct RecordDBIteratorControl
{
    unsigned char failed;
    unsigned char padding01[3];
    RecordDBStreambufControl* stream;
};
namespace Msvc6Runtime
{
    class locale
    {
    public:
        class facet
        {
        public:
            virtual ~facet() noexcept(false);
        };
    };

    class alignas(8) fpos
    {
    public:
        long offset;
        unsigned int padding04;
        long long position;
        int state;

        explicit fpos(long value);
    };

    class basic_streambuf
    {
    public:
        virtual ~basic_streambuf() noexcept(false);
        virtual int overflow(int value);
        virtual int pbackfail(int value);
        virtual int showmanyc();
        virtual int underflow();
        virtual int uflow();
        virtual int xsgetn(char* destination, int count);
        virtual int xsputn(const char* source, int count);
        virtual fpos seekoff(long offset, int direction, int mode);
        virtual fpos seekpos(fpos position, int mode);
        virtual basic_streambuf* setbuf(char* buffer, int count);
        virtual int sync();
    };

    class numpunct : public locale::facet
    {
    public:
        virtual char do_decimal_point() const;
        virtual char do_thousands_sep() const;
        virtual AS_RECORD_STRING_CONTROL do_grouping() const;
        virtual AS_RECORD_STRING_CONTROL do_falsename() const;
        virtual AS_RECORD_STRING_CONTROL do_truename() const;
    };

    class codecvt : public locale::facet
    {
    public:
        virtual bool do_always_noconv() const;
        virtual int do_max_length() const;
        virtual int do_encoding() const;
        virtual int do_in(int& state, const char* first, const char* last, const char*& next,
                          char* destination, char* limit, char*& destinationNext) const;
        virtual int do_out(int& state, const char* first, const char* last, const char*& next,
                           char* destination, char* limit, char*& destinationNext) const;
        virtual int do_length(int& state, const char* first, const char* last, unsigned int maximum) const;
    };

    class ostreambuf_iterator
    {
    public:
        unsigned char failed;
        unsigned char padding01[3];
        basic_streambuf* stream;

        explicit ostreambuf_iterator(basic_streambuf* value)
            : failed(0), padding01
{
    0, 0, 0
}, stream(value)
{
}
    };

    class num_put : public locale::facet
    {
    public:
        virtual ostreambuf_iterator do_put(ostreambuf_iterator iterator,
            RecordDBIosControl& ios, char fill, bool value) const;
        virtual ostreambuf_iterator do_put(ostreambuf_iterator iterator,
            RecordDBIosControl& ios, char fill, long value) const;
        virtual ostreambuf_iterator do_put(ostreambuf_iterator iterator,
            RecordDBIosControl& ios, char fill, unsigned long value) const;
        virtual ostreambuf_iterator do_put(ostreambuf_iterator iterator,
            RecordDBIosControl& ios, char fill, double value) const;
        virtual ostreambuf_iterator do_put(ostreambuf_iterator iterator,
            RecordDBIosControl& ios, char fill, long double value) const;
        virtual ostreambuf_iterator do_put(ostreambuf_iterator iterator,
            RecordDBIosControl& ios, char fill, const void* value) const;
    };
}

int RecordDBInitializeVirtualOwners();
int RecordDBInitializeLocaleMakers();

struct RecordDBOstreamSentry
{
    unsigned char ready;
    unsigned char padding01[3];
    void* stream;

    explicit RecordDBOstreamSentry(void* source);
    ~RecordDBOstreamSentry() noexcept(false);
};

struct RecordDBIosStorageNode { RecordDBIosStorageNode* next; };
struct RecordDBIosCallbackNode
{
    RecordDBIosCallbackNode* next;
    int index;
    unsigned int callback;
};
struct RecordDBLocaleControl { void* implementation; };
struct RecordDBLocaleImplControl : Msvc6::FacetData
{
    static RecordDBLocaleImplControl* _Global;
    static RecordDBLocaleImplControl* _Clocptr;
    void** facets;
    unsigned int facetCount;
    unsigned int category;
    unsigned char transparent;
    unsigned char padding15[3];
    AS_RECORD_STRING_CONTROL name;

    RecordDBLocaleImplControl* __thiscall Construct(unsigned int transparentValue);
    RecordDBLocaleImplControl* __thiscall CopyConstruct(const RecordDBLocaleImplControl& source);
    void AddFacet(void* facet, unsigned int id);
    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
};
struct RecordDBCtypeControl : Msvc6::CtypeData
{
    static const short* _Cltab;

    void* __thiscall ScalarDeletingDestructor(unsigned int flags);

    unsigned char* __thiscall ToLowerRange(unsigned char* begin, unsigned char* end);
    unsigned int __thiscall ToLower(unsigned int value);
    unsigned char* __thiscall ToUpperRange(unsigned char* begin, unsigned char* end);
    unsigned int __thiscall ToUpper(unsigned int value);
};
struct RecordDBNumpunctControl : Msvc6::NumpunctData<char>
{

    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    unsigned char __thiscall DecimalPoint() const;
    unsigned char __thiscall ThousandsSeparator() const;
    void* __thiscall Grouping(void* out);
    void* __thiscall FalseName(void* out);
    void* __thiscall TrueName(void* out);
};
struct RecordDBCodecvtControl : Msvc6::LocaleStampData
{
};
struct RecordDBExceptionControl
{
    const Msvc6::ExceptionVirtualTable* vftable;
    char* message;
    unsigned int ownsMessage;

    RecordDBExceptionControl* __thiscall Construct(const char* const& value);
    RecordDBExceptionControl* __thiscall CopyConstruct(const RecordDBExceptionControl& source);
    void __thiscall Destruct();
    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    const char* __thiscall what() const;
};

struct RecordDBBadCastControl : RecordDBExceptionControl
{
    RecordDBBadCastControl* __thiscall Construct(const char* value);
    RecordDBBadCastControl* __thiscall CopyConstruct(const RecordDBBadCastControl& source);
    void __thiscall Destruct();
    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    void __thiscall _Doraise() const;
};

struct RecordDBLogicErrorControl : RecordDBExceptionControl
{
    AS_RECORD_STRING_CONTROL errorText;

    RecordDBLogicErrorControl* __thiscall Construct(const AS_RECORD_STRING_CONTROL& text);
    RecordDBLogicErrorControl* __thiscall CopyConstruct(const RecordDBLogicErrorControl& source);
    void __thiscall Destruct();
    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    const char* __thiscall what() const;
    void __thiscall _Doraise() const;
};

struct RecordDBLengthErrorControl : RecordDBLogicErrorControl
{
    RecordDBLengthErrorControl* __thiscall CopyConstruct(const RecordDBLengthErrorControl& source);
    void __thiscall Destruct();
    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    void __thiscall _Doraise() const;
};

struct RecordDBOutOfRangeControl : RecordDBLogicErrorControl
{
    RecordDBOutOfRangeControl* __thiscall CopyConstruct(const RecordDBOutOfRangeControl& source);
    void __thiscall Destruct();
    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    void __thiscall _Doraise() const;
};

struct RecordDBRuntimeErrorControl : RecordDBExceptionControl
{
    AS_RECORD_STRING_CONTROL errorText;

    RecordDBRuntimeErrorControl* __thiscall Construct(const AS_RECORD_STRING_CONTROL& text);
    RecordDBRuntimeErrorControl* __thiscall CopyConstruct(const RecordDBRuntimeErrorControl& source);
    void __thiscall Destruct();
    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    const char* __thiscall what() const;
    void __thiscall _Doraise() const;
};

struct RecordDBIosFailureControl : RecordDBRuntimeErrorControl
{
    RecordDBIosFailureControl* __thiscall CopyConstruct(const RecordDBIosFailureControl& source);
    void __thiscall Destruct();
    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    void __thiscall _Doraise() const;
};

struct RecordDBBadAllocControl : RecordDBExceptionControl
{
    RecordDBBadAllocControl* __thiscall Construct(const char* value);
    RecordDBBadAllocControl* __thiscall CopyConstruct(const RecordDBBadAllocControl& source);
    void __thiscall Destruct();
    void* __thiscall ScalarDeletingDestructor(unsigned int flags);
    void __thiscall _Doraise() const;
};

void __cdecl RecordDBBadAllocCleanup();


struct RecordDBStreamVirtualBase { const Msvc6::VirtualBaseTable* vbtable; };
struct RecordDBStreamControl
{
    const Msvc6::VirtualBaseTable* vbtable;
    unsigned int count;
    RecordDBIosControl ios;
};
struct RecordDBIfstreamControl
{
    const Msvc6::VirtualBaseTable* vbtable;
    unsigned int count;
    RecordDBFilebufControl filebuf;
    RecordDBIosControl ios;

    RecordDBIfstreamControl(const char* fileName, int openMode);
    ~RecordDBIfstreamControl() noexcept(false);
    void Close();
    unsigned int State() const;
    void PrepareLineRead();
    RecordDBIfstreamControl(const RecordDBIfstreamControl&)=delete;
    RecordDBIfstreamControl& operator=(const RecordDBIfstreamControl&)=delete;
};
struct RecordDBOfstreamControl
{
    const Msvc6::VirtualBaseTable* vbtable;
    RecordDBFilebufControl filebuf;
    RecordDBIosControl ios;

    RecordDBOfstreamControl(const char* fileName, int openMode);
    ~RecordDBOfstreamControl() noexcept(false);
    void Close();
    unsigned int State() const;
    enum FormatFlag
    {
        right = 128,
        decimal = 512,
        hexadecimal = 2048,
        adjustmentField = 448,
        baseField = 3584
    };
    RecordDBOfstreamControl& operator<<(const char* text);
    RecordDBOfstreamControl& operator<<(const void* address);
    RecordDBOfstreamControl& operator<<(int value);
    RecordDBOfstreamControl& operator<<(unsigned long value);
    RecordDBOfstreamControl& put(char value);
    RecordDBOfstreamControl& flush();
    unsigned int setf(unsigned int flags, unsigned int mask);
    RecordDBOfstreamControl(const RecordDBOfstreamControl&)=delete;
    RecordDBOfstreamControl& operator=(const RecordDBOfstreamControl&)=delete;
};

struct RecordDBLineString : AS_RECORD_STRING_CONTROL
{
    RecordDBLineString();
    explicit RecordDBLineString(const char* text);
    explicit RecordDBLineString(const AS_RECORD_STRING_CONTROL& source);
    RecordDBLineString(const char* text, unsigned char allocatorValue);
    ~RecordDBLineString() noexcept(false);
    const char* Data() const;
    RecordDBLineString(const RecordDBLineString&)=delete;
    RecordDBLineString& operator=(const RecordDBLineString&)=delete;
};

void*  RecordDBStringAssignBytes(void* self, const void* source, unsigned int count);
unsigned int  Msvc6BasicOstreamInsertInt(void* stream, int value);
unsigned int  Msvc6BasicOstreamInsertUnsignedInt(void* stream, unsigned int value);
void* __cdecl N_6std(void* stream, const char* text);
void* __cdecl RecordDBOstreamWriteChar(void* stream, unsigned char value);
unsigned int __cdecl RecordDBGetlineDelimited(void* stream, void* output, unsigned int delimiter);

