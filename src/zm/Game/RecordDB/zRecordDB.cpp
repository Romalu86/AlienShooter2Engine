#include <cstdio>
#include "myerror.h"
#include "RecordDBStreambufControl.h"
#include <new>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include "runtime/locale_operations.h"
#include "runtime/locale_state.h"
#include "runtime/crt_locale.h"

#include "zm/Game/RecordDB/zRecordDB.h"
#include "zm/Game/ScriptExec/zScriptExec.h"
#include "mylib.h"
#include "map.h"
#include "resread.h"
#include "basetype.h"
#include "exports.h"
#include "zm/Utils/zVectorOperations.h"


void __cdecl RecordDBConstructRecord(void*, void*);
#include "list.h"
#include "zm/Survive/zSurvive.h"
#include <stdio.h>

void  basic_ifstream_Dtor(void* self);
void  basic_ofstream_Dtor(void* self);
void  basic_filebuf_Dtor(void* self);
void  RecordDBBasicFilebufInit(void* self, unsigned int backing, unsigned int mode);
static void* RecordDBIosBaseFromStream(void* stream);
unsigned int  RecordDBBasicIosDestructor(void* self);
void  numpunct_Dtor(void* self);
void*  RecordDBRefCountReleaseAndReturnIfZero(void* self);
unsigned int  RecordDBBasicFilebufInitCodecvt(void* self);
void*  RecordDBStringAssignSubstring(void* self, void* sourceObject, unsigned int pos, unsigned int count);
unsigned int __cdecl RecordDBBasicIstreamIpfx(void* self, unsigned int noSkip);

const unsigned int AS_RECORD_STRING_CONTROL::npos;

zRecordDB* RecordDatabase = nullptr;

void* zRecordDB::QueryInterface(int interfaceId) const
{
    if (interfaceId == 57)
        return const_cast<zRecordDB*>(this);
    DebugAssert( 5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\RecordDB\\zRecordDB.cpp", 18u);
    return 0;
}

int zRecordDB::iRelease()
{
    if (this != nullptr)
    {
        this->~zRecordDB();
        zRecordDB::operator delete(this);
    }
    return 0;
}

void zRecordDB::operator delete(void* memory)
{
    CrtFreePointer(memory);
}

void vector_Dtor_2(void* self)
{
    AS_RECORD_PTR_VECTOR& records = *static_cast<AS_RECORD_PTR_VECTOR*>(self);
    CrtFreePointer(records.begin);
    records.begin = 0;
    records.end = 0;
    records.capacityEnd = 0;
}


void  vector_Dtor_1(void* self)
{
    AS_RECORD_VECTOR& records = *static_cast<AS_RECORD_VECTOR*>(self);
    AS_RECORD* current = records.begin;
    AS_RECORD* const end = records.end;
    for (;current != end;++current)
        current->~AS_RECORD();
    CrtFreePointer(records.begin);
    records.begin = 0;
    records.end = 0;
    records.capacityEnd = 0;
}

bool zRecordDB::LoadIfNeed(int iType)
{
    if (iType < 0 || iType >= 3)
        DebugAssert(5u, "iType >= 0 && iType <REC_TYPES_CNT", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\RecordDB\\zRecordDB.cpp", 116u);
    if (m_DataFlags[iType])
        return true;
    m_DataFlags[iType] = 1;
    return LoadFromFile(iType);
}

namespace {
template<class Vector>
struct RecordDBTableCleanup
{
    Vector* tables;
    unsigned int remaining;
    void (*destroy)(void*);

    void Clear()
    {
        while (remaining != 0u)
            destroy(&tables[--remaining]);
    }

    ~RecordDBTableCleanup() noexcept(false)
    {
        Clear();
    }
};
}

zRecordDB::~zRecordDB() noexcept(false)
{
    RecordDBTableCleanup<AS_RECORD_VECTOR> data = {m_Data, 3u, vector_Dtor_1};
    RecordDBTableCleanup<AS_RECORD_PTR_VECTOR> output = {m_OutData, 3u, vector_Dtor_2};
    output.Clear();
    data.Clear();
}

zRecordDB::zRecordDB()
{
    for (AS_RECORD_VECTOR& records : m_Data)
        records.begin = records.end = records.capacityEnd = nullptr;
    for (AS_RECORD_PTR_VECTOR& records : m_OutData)
        records.begin = records.end = records.capacityEnd = nullptr;
    for (int tableIndex = 0;tableIndex<3;++tableIndex)
    {
        m_DataFlags[tableIndex] = 0;
        m_OutFlags[tableIndex] = 1;
    }
}

int zRecordDB::GetRecCnt(int iType)
{
    (void)LoadIfNeed(iType);
    BuildDataIfNeed(iType);
    const AS_RECORD_VECTOR& data = m_Data[iType];
    const AS_RECORD_PTR_VECTOR& out = m_OutData[iType];
    const int dataCount = data.begin ? static_cast<int>(data.end-data.begin) : 0;
    const int outCount = out.begin ? static_cast<int>(out.end-out.begin) : 0;
    if (dataCount != outCount)
        DebugAssert(5u, "m_Data[iType].size() == m_OutData[iType].size()", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\RecordDB\\zRecordDB.cpp", 88u);
    return outCount;
}


void*  RecordDBCompilerInitVtable2920(void* self)
{
    static_cast<RecordDBIosControl*>(self)->registration = 0u;
    static_cast<RecordDBIosControl*>(self)->vftable = Msvc6::IosBaseTable();
    return self;
}

void  RecordDBStringEos(void* self, unsigned int index)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);
    storage.size = index;
    storage.data[index] = '\0';
}

int __cdecl Msvc6BasicStringMaxSize()
{
    return -3;
}

void  facet_Dtor(void* self)
{
    static_cast<Msvc6::FacetData*>(self)->vftable = Msvc6::BaseFacetTable();
}

void  basic_ostream_Dtor(void* self)
{
    unsigned char* streamBytes = static_cast<unsigned char*>(self);
    const Msvc6::VirtualBaseTable* const virtualBaseTableAddress = reinterpret_cast<RecordDBStreamVirtualBase*>(streamBytes - 4)->vbtable;
    unsigned int virtualBaseOffset = virtualBaseTableAddress->virtualBaseOffset;
    reinterpret_cast<RecordDBIosControl*>(streamBytes - 4 + virtualBaseOffset)->vftable = Msvc6::OstreamTable();
}

void  RecordDBCompilerAdjustVtableMinus8(void* self)
{
    unsigned char* streamBytes = static_cast<unsigned char*>(self);
    const Msvc6::VirtualBaseTable* const virtualBaseTableAddress = reinterpret_cast<RecordDBStreamVirtualBase*>(streamBytes - 8)->vbtable;
    unsigned int virtualBaseOffset = virtualBaseTableAddress->virtualBaseOffset;
    reinterpret_cast<RecordDBIosControl*>(streamBytes - 8 + virtualBaseOffset)->vftable = Msvc6::IstreamTable();
}

int __cdecl RecordDBBasicStreambufUnderflow()
{
    return -1;
}

void*  RecordDBBasicStreambufSetbuf(void* self, unsigned int, unsigned int)
{
    return self;
}




unsigned char  NumpunctChar_DoDecimalPoint(const void* self)
{
    return static_cast<const RecordDBNumpunctControl*>(self)->decimalPoint;
}

unsigned char  NumpunctChar_DoThousandsSep(const void* self)
{
    return static_cast<const RecordDBNumpunctControl*>(self)->thousandsSeparator;
}

unsigned int __cdecl zRecordDB_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


unsigned int __cdecl zRecordDB_StaticCleanupRegister02()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


unsigned int __cdecl zRecordDB_StaticCleanupRegister03()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


unsigned int __cdecl zRecordDB_StaticCleanupRegister04()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


void  Msvc6BasicStreambufInit(void* self)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);

    storage.getCursorSlot = &storage.getCursor;
    storage.putCursorSlot = &storage.putCursor;
    storage.getBeginSlot = &storage.getBegin;
    storage.getRemainingSlot = &storage.getRemaining;
    storage.putBeginSlot = &storage.putBegin;
    storage.putRemainingSlot = &storage.putRemaining;
    storage.putBegin = 0;
    storage.putCursor = 0;
    storage.putRemaining = 0;
    storage.getBegin = 0;
    storage.getCursor = 0;
    storage.getRemaining = 0;
}

void  Msvc6BasicStreambufInitRanges(void* self,
    unsigned int getBeginSlotAddress,
    unsigned int getCursorSlotAddress,
    unsigned int getRemainingSlotAddress,
    unsigned int putBeginSlotAddress,
    unsigned int putCursorSlotAddress,
    unsigned int putRemainingSlotAddress)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);
    storage.getBeginSlot = reinterpret_cast<char**>(U32ToPtr(getBeginSlotAddress));
    storage.putBeginSlot = reinterpret_cast<char**>(U32ToPtr(putBeginSlotAddress));
    storage.getCursorSlot = reinterpret_cast<char**>(U32ToPtr(getCursorSlotAddress));
    storage.putCursorSlot = reinterpret_cast<char**>(U32ToPtr(putCursorSlotAddress));
    storage.getRemainingSlot = reinterpret_cast<unsigned int*>(U32ToPtr(getRemainingSlotAddress));
    storage.putRemainingSlot = reinterpret_cast<unsigned int*>(U32ToPtr(putRemainingSlotAddress));
}

void  Msvc6BasicStreambufSetg(void* self, unsigned int begin, unsigned int middle, unsigned int end)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);

    *storage.getBeginSlot = reinterpret_cast<char*>(U32ToPtr(begin));
    *storage.getCursorSlot = reinterpret_cast<char*>(U32ToPtr(middle));
    *storage.getRemainingSlot = end-middle;
}

void  Msvc6BasicStreambufSetp(void* self, unsigned int begin, unsigned int end)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);

    *storage.putBeginSlot = reinterpret_cast<char*>(U32ToPtr(begin));
    *storage.putCursorSlot = reinterpret_cast<char*>(U32ToPtr(begin));
    *storage.putRemainingSlot = end-begin;
}


void  RecordDBResetEmbedded5C(void* self)
{
    void* embedded = &static_cast<RecordDBIfstreamControl*>(self)->ios;
    basic_ifstream_Dtor(embedded);
    (void)RecordDBBasicIosDestructor(embedded);
}

void  basic_ofstream_VBaseDtor(void* self)
{
    void* embedded = &static_cast<RecordDBOfstreamControl*>(self)->ios;
    basic_ofstream_Dtor(embedded);
    (void)RecordDBBasicIosDestructor(embedded);
}


void  BasicIos_SetState(void* self, unsigned int mask, unsigned int value)
{


    if (mask == 0u) return;
    unsigned int flags = static_cast<RecordDBIosControl*>(self)->state|mask;
    if (static_cast<RecordDBIosControl*>(self)->buffer == nullptr) flags|=4u;

    Msvc6Runtime::BasicIosClear(self, flags, value);
}


int  RecordDBBasicStreambufUflow(void* self)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);
    const int status = static_cast<int>(static_cast<Msvc6Runtime::basic_streambuf*>(self)->underflow());
    if (status == -1)
        return -1;
    unsigned int* const remaining = storage.getRemainingSlot;
    *remaining = *remaining-1u;
    char** const cursorSlot = storage.getCursorSlot;
    char* const cursor = *cursorSlot;
    *cursorSlot = cursor+1;
    return static_cast<unsigned char>(*cursor);
}


RecordDBExceptionControl* RecordDBExceptionControl::Construct(const char* const& value)
{
    vftable = Msvc6::ExceptionTable();
    const unsigned int count = static_cast<unsigned int>(std::strlen(value))+1u;
    message = static_cast<char*>(CrtNew(count));
    if (message != 0)
        std::strcpy(message, value);
    ownsMessage = 1u;
    return this;
}

RecordDBExceptionControl* RecordDBExceptionControl::CopyConstruct(const RecordDBExceptionControl& source)
{
    vftable = Msvc6::ExceptionTable();
    ownsMessage = source.ownsMessage;
    if (ownsMessage != 0u)
    {
        const unsigned int count = static_cast<unsigned int>(std::strlen(source.message))+1u;
        message = static_cast<char*>(CrtNew(count));
        if (message != 0)
            std::strcpy(message, source.message);
    }
    else
        message = source.message;
    return this;
}

void RecordDBExceptionControl::Destruct()
{
    const unsigned int owned = ownsMessage;
    vftable = Msvc6::ExceptionTable();
    if (owned != 0u)
        CrtFreePointer(message);
}

void* RecordDBExceptionControl::ScalarDeletingDestructor(unsigned int flags)
{
    Destruct();
    if ((flags&1u) != 0u)
        CrtFreePointer(this);
    return this;
}

const char* RecordDBExceptionControl::what() const
{
    return message != 0 ? message : "Unknown exception";
}

RecordDBBadCastControl* RecordDBBadCastControl::CopyConstruct(const RecordDBBadCastControl& source)
{
    RecordDBExceptionControl::CopyConstruct(source);
    vftable = Msvc6::BadCastTable();
    return this;
}

RecordDBLogicErrorControl* RecordDBLogicErrorControl::Construct(const AS_RECORD_STRING_CONTROL& text)
{
    const char* const baseMessage = StringGlobals::EmptyText();
    RecordDBExceptionControl::Construct(baseMessage);
    try
    {
        errorText.allocatorState[0] = text.allocatorState[0];
        Tidy(&errorText, 0u);
        RecordDBStringAssignSubstring(&errorText, const_cast<AS_RECORD_STRING_CONTROL*>(&text),
            0u, AS_RECORD_STRING_CONTROL::npos);
    }
    catch (...)
    {
        RecordDBExceptionControl::Destruct();
        throw;
    }
    vftable = Msvc6::LogicErrorTable();
    return this;
}

RecordDBLogicErrorControl* RecordDBLogicErrorControl::CopyConstruct(const RecordDBLogicErrorControl& source)
{
    RecordDBExceptionControl::CopyConstruct(source);
    try
    {
        errorText.allocatorState[0] = source.errorText.allocatorState[0];
        Tidy(&errorText, 0u);
        RecordDBStringAssignSubstring(&errorText,
            const_cast<AS_RECORD_STRING_CONTROL*>(&source.errorText),
            0u, AS_RECORD_STRING_CONTROL::npos);
    }
    catch (...)
    {
        RecordDBExceptionControl::Destruct();
        throw;
    }
    vftable = Msvc6::LogicErrorTable();
    return this;
}

void RecordDBLogicErrorControl::Destruct()
{
    vftable = Msvc6::LogicErrorTable();
    try
    {
        Tidy(&errorText, 1u);
    }
    catch (...)
    {
        RecordDBExceptionControl::Destruct();
        throw;
    }
    RecordDBExceptionControl::Destruct();
}

void* RecordDBLogicErrorControl::ScalarDeletingDestructor(unsigned int flags)
{
    Destruct();
    if ((flags&1u) != 0u)
        CrtFreePointer(this);
    return this;
}

const char* RecordDBLogicErrorControl::what() const
{
    return errorText.data != 0 ? errorText.data : reinterpret_cast<const char*>(U32ToPtr(PtrToU32(Msvc6::StringEmptyData<char>())));
}

void RecordDBLogicErrorControl::_Doraise() const
{
    RecordDBLogicErrorControl object;
    object.CopyConstruct(*this);
    Msvc6::ThrowException(&object, Msvc6::LogicErrorThrowInfo());
}

RecordDBLengthErrorControl* RecordDBLengthErrorControl::CopyConstruct(const RecordDBLengthErrorControl& source)
{
    RecordDBLogicErrorControl::CopyConstruct(source);
    vftable = Msvc6::LengthErrorTable();
    return this;
}

void RecordDBLengthErrorControl::Destruct()
{
    RecordDBLogicErrorControl::Destruct();
}

void* RecordDBLengthErrorControl::ScalarDeletingDestructor(unsigned int flags)
{
    Destruct();
    if ((flags&1u) != 0u)
        CrtFreePointer(this);
    return this;
}

void RecordDBLengthErrorControl::_Doraise() const
{
    RecordDBLengthErrorControl object;
    object.CopyConstruct(*this);
    Msvc6::ThrowException(&object, Msvc6::LengthErrorThrowInfo());
}

RecordDBOutOfRangeControl* RecordDBOutOfRangeControl::CopyConstruct(const RecordDBOutOfRangeControl& source)
{
    RecordDBLogicErrorControl::CopyConstruct(source);
    vftable = Msvc6::OutOfRangeTable();
    return this;
}

void RecordDBOutOfRangeControl::Destruct()
{
    RecordDBLogicErrorControl::Destruct();
}

void* RecordDBOutOfRangeControl::ScalarDeletingDestructor(unsigned int flags)
{
    Destruct();
    if ((flags&1u) != 0u)
        CrtFreePointer(this);
    return this;
}

void RecordDBOutOfRangeControl::_Doraise() const
{
    RecordDBOutOfRangeControl object;
    object.CopyConstruct(*this);
    Msvc6::ThrowException(&object, Msvc6::OutOfRangeThrowInfo());
}

RecordDBRuntimeErrorControl* RecordDBRuntimeErrorControl::Construct(const AS_RECORD_STRING_CONTROL& text)
{
    const char* const baseMessage = StringGlobals::EmptyText();
    RecordDBExceptionControl::Construct(baseMessage);
    try
    {
        errorText.allocatorState[0] = text.allocatorState[0];
        Tidy(&errorText, 0u);
        RecordDBStringAssignSubstring(&errorText, const_cast<AS_RECORD_STRING_CONTROL*>(&text),
            0u, AS_RECORD_STRING_CONTROL::npos);
    }
    catch (...)
    {
        RecordDBExceptionControl::Destruct();
        throw;
    }
    vftable = Msvc6::RuntimeErrorTable();
    return this;
}

RecordDBRuntimeErrorControl* RecordDBRuntimeErrorControl::CopyConstruct(const RecordDBRuntimeErrorControl& source)
{
    RecordDBExceptionControl::CopyConstruct(source);
    try
    {
        errorText.allocatorState[0] = source.errorText.allocatorState[0];
        Tidy(&errorText, 0u);
        RecordDBStringAssignSubstring(&errorText,
            const_cast<AS_RECORD_STRING_CONTROL*>(&source.errorText),
            0u, AS_RECORD_STRING_CONTROL::npos);
    }
    catch (...)
    {
        RecordDBExceptionControl::Destruct();
        throw;
    }
    vftable = Msvc6::RuntimeErrorTable();
    return this;
}

void RecordDBRuntimeErrorControl::Destruct()
{
    vftable = Msvc6::RuntimeErrorTable();
    try
    {
        Tidy(&errorText, 1u);
    }
    catch (...)
    {
        RecordDBExceptionControl::Destruct();
        throw;
    }
    RecordDBExceptionControl::Destruct();
}

void* RecordDBRuntimeErrorControl::ScalarDeletingDestructor(unsigned int flags)
{
    Destruct();
    if ((flags&1u) != 0u)
        CrtFreePointer(this);
    return this;
}

const char* RecordDBRuntimeErrorControl::what() const
{
    return errorText.data != 0 ? errorText.data : reinterpret_cast<const char*>(U32ToPtr(PtrToU32(Msvc6::StringEmptyData<char>())));
}

void RecordDBRuntimeErrorControl::_Doraise() const
{
    RecordDBRuntimeErrorControl object;
    object.CopyConstruct(*this);
    Msvc6::ThrowException(&object, Msvc6::RuntimeErrorThrowInfo());
}

RecordDBIosFailureControl* RecordDBIosFailureControl::CopyConstruct(const RecordDBIosFailureControl& source)
{
    RecordDBRuntimeErrorControl::CopyConstruct(source);
    vftable = Msvc6::IosFailureTable();
    return this;
}

void RecordDBIosFailureControl::Destruct()
{
    RecordDBRuntimeErrorControl::Destruct();
}

void* RecordDBIosFailureControl::ScalarDeletingDestructor(unsigned int flags)
{
    Destruct();
    if ((flags&1u) != 0u)
        CrtFreePointer(this);
    return this;
}

void RecordDBIosFailureControl::_Doraise() const
{
    RecordDBIosFailureControl object;
    object.CopyConstruct(*this);
    Msvc6::ThrowException(&object, Msvc6::IosFailureThrowInfo());
}

RecordDBBadAllocControl* RecordDBBadAllocControl::Construct(const char* value)
{
    RecordDBExceptionControl::Construct(value);
    vftable = Msvc6::BadAllocTable();
    return this;
}

RecordDBBadAllocControl* RecordDBBadAllocControl::CopyConstruct(const RecordDBBadAllocControl& source)
{
    RecordDBExceptionControl::CopyConstruct(source);
    vftable = Msvc6::BadAllocTable();
    return this;
}

void RecordDBBadAllocControl::Destruct()
{
    vftable = Msvc6::BadAllocTable();
    RecordDBExceptionControl::Destruct();
}

void* RecordDBBadAllocControl::ScalarDeletingDestructor(unsigned int flags)
{
    Destruct();
    if ((flags&1u) != 0u)
        CrtFreePointer(this);
    return this;
}

void RecordDBBadAllocControl::_Doraise() const
{
    RecordDBBadAllocControl object;
    object.CopyConstruct(*this);
    Msvc6::ThrowException(&object, Msvc6::BadAllocThrowInfo());
}

void __cdecl RecordDBBadAllocCleanup()
{
    Msvc6::BadAllocationObject().Destruct();
}



unsigned int  RecordDBBasicFilebufUnderflow(void* self)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);

    char** cursorHolder = storage.streambuf.getCursorSlot;
    const unsigned int cursor = PtrToU32(*cursorHolder);
    if (cursor != 0u)
    {
        const unsigned int* countHolder = storage.streambuf.getRemainingSlot;
        const unsigned int count = *countHolder;
        if (cursor<cursor+count) return static_cast<unsigned int>(ReadGlobalU8(cursor));
    }
    const unsigned int refill = static_cast<Msvc6Runtime::basic_streambuf*>(self)->uflow();
    return static_cast<Msvc6Runtime::basic_streambuf*>(self)->pbackfail(static_cast<int>(refill));
}


void*  RecordDBBasicFilebufSetbuf(void* self, unsigned int first, unsigned int second)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);

    const unsigned int handle = PtrToU32(storage.file);
    if (handle == 0u) return 0;
    const unsigned int status = Msvc6SetVbuf(handle, first, 0u, second);
    return status == 0u?self:0;
}


void  Msvc6LocaleFacetDecref(void* self)
{
    void* owner = static_cast<RecordDBLocaleControl*>(self)->implementation;
    if (!owner) return;
    void* object = RecordDBRefCountReleaseAndReturnIfZero(owner);
    if (object) delete static_cast<Msvc6Runtime::locale::facet*>(object);
}


unsigned int  do_toupper_1(void* self, unsigned int value)
{
    return Msvc6ToUpper(value&0xFFu, PtrToU32(&static_cast<RecordDBCtypeControl*>(self)->descriptor));
}


void*  DU_char_traits_N_4_ostreambuf_iterator(void* self, unsigned int value)
{
    RecordDBIteratorControl& iterator = *static_cast<RecordDBIteratorControl*>(self);
    RecordDBStreambufControl* const stream = iterator.stream;
    unsigned int result = 0xFFFFFFFFu;
    if (stream)
    {
        char** const cursorSlot = stream->putCursorSlot;
        char* const cursor = *cursorSlot;
        const unsigned int address = PtrToU32(cursor);
        if (address != 0u)
        {
            unsigned int* const remainingSlot = stream->putRemainingSlot;
            const unsigned int remaining = *remainingSlot;
            if (address<address+remaining)
            {
                *remainingSlot = remaining-1u;
                *cursorSlot = cursor+1;
                *cursor = static_cast<char>(value);
                result = value&0xFFu;
            }
            else result = reinterpret_cast<Msvc6Runtime::basic_streambuf*>(stream)->overflow(static_cast<int>(value&0xFFu));
        }
        else result = reinterpret_cast<Msvc6Runtime::basic_streambuf*>(stream)->overflow(static_cast<int>(value&0xFFu));
    }
    if (result == 0xFFFFFFFFu)
        iterator.failed = 1u;
    return self;
}


unsigned int  BasicStreambuf_SGetC(void* self)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);
    char** const cursorSlot = storage.getCursorSlot;
    const char* const cursor = *cursorSlot;
    const unsigned int address = PtrToU32(cursor);
    if (address != 0u)
    {
        const unsigned int* const remainingSlot = storage.getRemainingSlot;
        const unsigned int remaining = *remainingSlot;
        if (address<address+remaining)
            return static_cast<unsigned char>(*cursor);
    }
    return static_cast<Msvc6Runtime::basic_streambuf*>(self)->underflow();
}

unsigned int  RecordDBCopyRangeIntoEnd(void* self,
    unsigned int destination,
    unsigned int source)
{
    AS_RECORD_DWORD_VECTOR& storage = *static_cast<AS_RECORD_DWORD_VECTOR*>(self);
    unsigned int previousEndAddress = PtrToU32(storage.end), writeAddress = destination;
    while (source != previousEndAddress)
    {
        WriteGlobalU32(writeAddress, ReadGlobalU32(source));
        source+=4u;
        writeAddress+=4u;
    }
    storage.end = reinterpret_cast<int*>(U32ToPtr(writeAddress));
    return destination;
}


unsigned int  RecordDBBasicStreambufSbumpc(void* self)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);
    char** const cursorSlot = storage.getCursorSlot;
    char* const cursor = *cursorSlot;
    const unsigned int address = PtrToU32(cursor);
    if (address != 0u)
    {
        unsigned int* const remainingSlot = storage.getRemainingSlot;
        const unsigned int remaining = *remainingSlot;
        if (address<address+remaining)
        {
            *remainingSlot = remaining-1u;
            *cursorSlot = cursor+1;
            return static_cast<unsigned char>(*cursor);
        }
    }
    return static_cast<Msvc6Runtime::basic_streambuf*>(self)->uflow();
}


const AS_RECORD* zRecordDB::GetRecData(int iType, int index)
{
    if (iType<0 || iType >= 3 || index<0)
        return 0;
    const AS_RECORD_PTR_VECTOR& out = m_OutData[iType];
    const int count = out.begin ? static_cast<int>(out.end-out.begin) : 0;
    return index<count ? out.begin[index] : 0;
}

void*  RecordDBRefCountReleaseAndReturnIfZero(void* self)
{
    unsigned int lockStorage = PtrToU32(self);
    Msvc6Runtime::StaticLockConstruct(&lockStorage);
    MsvcLocaleFacetRef& facet = *static_cast<MsvcLocaleFacetRef*>(self);
    unsigned int references = facet.m_refCount04;
    if (references>0u && references<0xFFFFFFFFu)
    {
        --references;
        facet.m_refCount04 = references;
    }
    void* const result = references == 0u?self:0;
    Msvc6Runtime::StaticLockDestruct(&lockStorage);
    return result;
}


void*  RecordDBInitCursorState(void* self, unsigned int value)
{
    RecordDBPositionControl& position = *static_cast<RecordDBPositionControl*>(self);
    position.offset = BitsToSigned32(value);
    position.position = Msvc6::_Fpz;
    unsigned int lockStorage = value;
    Msvc6Runtime::StaticLockConstruct(&lockStorage);
    position.state = Msvc6::FposInitialState;
    Msvc6Runtime::StaticLockDestruct(&lockStorage);
    return self;
}


unsigned int*  RecordDBCopyAndRetainField20(void* self, unsigned int* out)
{
    MsvcLocaleFacetRef* const locale = static_cast<MsvcLocaleFacetRef*>(
        static_cast<RecordDBIosControl*>(self)->locale);
    *out = PtrToU32(locale);
    unsigned int lockStorage = 0u;
    Msvc6Runtime::StaticLockConstruct(&lockStorage);
    const unsigned int references = locale->m_refCount04;
    if (references<0xFFFFFFFFu)
        locale->m_refCount04 = references+1u;
    Msvc6Runtime::StaticLockDestruct(&lockStorage);
    return out;
}


RecordDBBadCastControl* RecordDBBadCastControl::Construct(const char* value)
{
    RecordDBExceptionControl::Construct(value);
    vftable = Msvc6::BadCastTable();
    return this;
}


unsigned int*  Msvc6BasicStreambufGetloc(void* self, unsigned int* out)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);
    MsvcLocaleFacetRef* const locale = static_cast<MsvcLocaleFacetRef*>(storage.locale);
    *out = PtrToU32(locale);
    unsigned int lockStorage = 0u;
    Msvc6Runtime::StaticLockConstruct(&lockStorage);
    const unsigned int references = locale->m_refCount04;
    if (references<0xFFFFFFFFu)
        locale->m_refCount04 = references+1u;
    Msvc6Runtime::StaticLockDestruct(&lockStorage);
    return out;
}


unsigned char*  CTypeChar_DoToLowerRange(void* self, unsigned char* begin, unsigned char* end)
{
    RecordDBCtypeControl& storage = *static_cast<RecordDBCtypeControl*>(self);
    unsigned char* current = begin;
    while (current != end)
    {
        const int signedByte = static_cast<signed char>(*current);
        *current = static_cast<unsigned char>(Msvc6ToLower(
            static_cast<unsigned int>(signedByte), PtrToU32(&storage.descriptor)));
        ++current;
    }
    return current;
}


unsigned char*  do_toupper_2(void* self, unsigned char* begin, unsigned char* end)
{
    RecordDBCtypeControl& storage = *static_cast<RecordDBCtypeControl*>(self);
    unsigned char* current = begin;
    while (current != end)
    {
        const int signedByte = static_cast<signed char>(*current);
        *current = static_cast<unsigned char>(Msvc6ToUpper(
            static_cast<unsigned int>(signedByte), PtrToU32(&storage.descriptor)));
        ++current;
    }
    return current;
}


void  ctype_Dtor(void* self)
{
    RecordDBCtypeControl& storage = *static_cast<RecordDBCtypeControl*>(self);
    const unsigned int ownsTable = storage.descriptor.ownsTable;
    storage.vftable = Msvc6::CharCtype::GetTable();
    if (ownsTable != 0u)
        (void)CrtFree(PtrToU32(storage.descriptor.table));
    storage.vftable = Msvc6::BaseFacetTable();
}


unsigned int  RecordDBNumpunctGrouping(void* self, unsigned int value)
{
    *reinterpret_cast<AS_RECORD_STRING_CONTROL*>(U32ToPtr(value)) = static_cast<Msvc6Runtime::numpunct*>(self)->do_grouping();
    return value;
}


void __cdecl RecordDBTouchFourStaticInitializers()
{
    unsigned int (__cdecl *const initializers[4])() = {
        zRecordDB_StaticCleanupRegister01, zRecordDB_StaticCleanupRegister02,
        zRecordDB_StaticCleanupRegister03, zRecordDB_StaticCleanupRegister04};
    for (unsigned int initializerIndex = 0;initializerIndex<4u;++initializerIndex)
    {
        const unsigned char guardValue = CrtStaticInitializationGuards[initializerIndex];
        if ((guardValue&1u) == 0u)
            CrtStaticInitializationGuards[initializerIndex] = static_cast<unsigned char>(guardValue|1u);
        (void)initializers[initializerIndex]();
    }
}

void* RecordDBBadCastControl::ScalarDeletingDestructor(unsigned int flags)
{
    Destruct();
    if ((flags&1u) != 0u)
        CrtFreePointer(this);
    return this;
}


void*  RecordDBCtypeScalarDeletingDtor(void* self, unsigned int flags)
{
    ctype_Dtor(self);
    if ((flags&1u) != 0u)
        CrtFreePointer(self);
    return self;
}


void*  Msvc6NumPutScalarDeletingDestructor(void* self, unsigned int flags)
{
    facet_Dtor(self);
    if ((flags&1u) != 0u)
        CrtFreePointer(self);
    return self;
}


void*  RecordDBNumpunctScalarDeletingDtor(void* self, unsigned int flags)
{
    numpunct_Dtor(self);
    if ((flags&1u) != 0u)
        CrtFreePointer(self);
    return self;
}

void  numpunct_Dtor(void* self)
{
    RecordDBNumpunctControl& storage = *static_cast<RecordDBNumpunctControl*>(self);
    char* const grouping = storage.grouping;
    storage.vftable = Msvc6::CharNumpunct::GetTable();
    CrtFreePointer(grouping);
    CrtFreePointer(storage.falseName);
    CrtFreePointer(storage.trueName);
    storage.vftable = Msvc6::BaseFacetTable();
}


void*  RecordDBLocaleFacetScalarDeletingDtor(void* self, unsigned int flags)
{
    static_cast<Msvc6::FacetData*>(self)->vftable = Msvc6::BaseFacetTable();
    if (flags&1u)CrtFreePointer(self);
    return self;
}

unsigned int __stdcall RecordDBBasicStreambufSeekoffDefault(unsigned int object, unsigned int, unsigned int, unsigned int)
{
    (void)RecordDBInitCursorState(U32ToPtr(object), 0xFFFFFFFFu);
    return object;
}

unsigned int __stdcall RecordDBBasicStreambufSeekposDefault(unsigned int object, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int)
{
    (void)RecordDBInitCursorState(U32ToPtr(object), 0xFFFFFFFFu);
    return object;
}


unsigned int  RecordDBBasicIosDestructor(void* self)
{
    static_cast<RecordDBIosControl*>(self)->vftable = Msvc6::IosTable();
    return Msvc6BasicIosDestruct(self);
}


void RecordDBBadCastControl::Destruct()
{
    vftable = Msvc6::BadCastTable();
    RecordDBExceptionControl::Destruct();
}


void*  RecordDBBasicIfstreamScalarDeletingDtor(void* self, unsigned int flags)
{
    void* base = AdjustThisPointer(self, -92);
    basic_ifstream_Dtor(self);
    (void)RecordDBBasicIosDestructor(self);
    if ((flags&1u) != 0u) CrtFreePointer(base);
    return base;
}

void*  Msvc6LocaleDefaultCtor(void* self)
{
    unsigned int tempA = 0u, tempB = 0u;
    static_cast<RecordDBLocaleControl*>(self)->implementation = U32ToPtr(Msvc6Runtime::GlobalLocale());
    Msvc6Runtime::StaticLockConstruct(&tempA);
    RecordDBLocaleImplControl* const shared = RecordDBLocaleImplControl::_Global;
    Msvc6Runtime::StaticLockConstruct(&tempB);
    unsigned int count = shared->references;
    if (count != 0xFFFFFFFFu) shared->references = count+1u;
    Msvc6Runtime::StaticLockDestruct(&tempB);
    Msvc6Runtime::StaticLockDestruct(&tempA);
    return self;
}


void __cdecl RecordDBCtypeTerm()
{
    CrtFreePointer(const_cast<short*>(RecordDBCtypeControl::_Cltab));
}

void*  RecordDBBasicOfstreamScalarDeletingDtor(void* self, unsigned int flags)
{
    basic_ofstream_Dtor(self);
    (void)RecordDBBasicIosDestructor(self);
    void* base = static_cast<unsigned char*>(self)-88;
    if (flags&1u)CrtFreePointer(base);
    return base;
}

void*  RecordDBCodecvtCtor(void* self, unsigned int value)
{
    RecordDBCodecvtControl& storage = *static_cast<RecordDBCodecvtControl*>(self);
    storage.references = value;
    storage.vftable = Msvc6::CharCodecvt::GetTable();
    try
    {
        unsigned char temporary[64];
        Msvc6Runtime::LocaleConstruct(temporary, PtrToU32("C"));
        const CrtUInt64 stamp = Msvc6LocaleStamp();
        storage.localeLow = static_cast<unsigned int>(stamp);
        storage.localeHigh = static_cast<unsigned int>(stamp>>32);
        Msvc6Runtime::LocaleDestruct(temporary);
    }
    catch (...)
    {
        storage.vftable = Msvc6::BaseFacetTable();
        throw;
    }
    return self;
}

void  basic_streambuf_Dtor(void* self)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);

    storage.vftable = Msvc6::StreambufTable();
    void* localeObject = storage.locale;
    if (localeObject)
    {
        unsigned int lockStorage;
        Msvc6Runtime::StaticLockConstruct(&lockStorage);
        unsigned int referenceCount = static_cast<MsvcLocaleFacetRef*>(localeObject)->m_refCount04;
        if (referenceCount>0u && referenceCount<0xFFFFFFFFu)
        {
            --referenceCount;
            static_cast<MsvcLocaleFacetRef*>(localeObject)->m_refCount04 = referenceCount;
        }
        void* unreferencedFacet = static_cast<MsvcLocaleFacetRef*>(localeObject)->m_refCount04 == 0u?localeObject:0;
        Msvc6Runtime::StaticLockDestruct(&lockStorage);
        if (unreferencedFacet)delete static_cast<Msvc6Runtime::locale::facet*>(unreferencedFacet);
    }
}


int  RecordDBBasicStreambufXsgetn(void* self, void* destination, int count)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);
    unsigned char* destinationBytes = static_cast<unsigned char*>(destination);
    int transferredBytes = 0;
    while (count>0)
    {
        char** const cursorSlot = storage.getCursorSlot;
        char* const cursor = *cursorSlot;
        const int bufferedBytes = cursor ? static_cast<int>(*storage.getRemainingSlot) : 0;
        if (cursor != 0 && bufferedBytes>0)
        {
            const int chunkBytes = count<bufferedBytes?count:bufferedBytes;
            CopyBytes(destinationBytes, cursor, static_cast<unsigned int>(chunkBytes));
            destinationBytes+=chunkBytes;
            transferredBytes+=chunkBytes;
            count-=chunkBytes;
            unsigned int* const remainingSlot = storage.getRemainingSlot;
            *remainingSlot = *remainingSlot-static_cast<unsigned int>(chunkBytes);
            char** const updatedCursorSlot = storage.getCursorSlot;
            *updatedCursorSlot = *updatedCursorSlot+chunkBytes;
        }
        else
        {
            const int readCharacter = static_cast<int>(static_cast<Msvc6Runtime::basic_streambuf*>(self)->uflow());
            if (readCharacter == -1)
                break;
            *destinationBytes++=static_cast<unsigned char>(readCharacter);
            ++transferredBytes;
            --count;
        }
    }
    return transferredBytes;
}


int  RecordDBBasicStreambufXsputn(void* self, const void* source, int count)
{
    RecordDBStreambufControl& storage = *static_cast<RecordDBStreambufControl*>(self);
    const unsigned char* sourceBytes = static_cast<const unsigned char*>(source);
    int transferredBytes = 0;
    while (count>0)
    {
        char** const cursorSlot = storage.putCursorSlot;
        char* const cursor = *cursorSlot;
        const int bufferedSpace = cursor ? static_cast<int>(*storage.putRemainingSlot) : 0;
        if (cursor != 0 && bufferedSpace>0)
        {
            const int chunkBytes = count<bufferedSpace?count:bufferedSpace;
            CopyBytes(cursor, sourceBytes, static_cast<unsigned int>(chunkBytes));
            sourceBytes+=chunkBytes;
            transferredBytes+=chunkBytes;
            count-=chunkBytes;
            unsigned int* const remainingSlot = storage.putRemainingSlot;
            *remainingSlot = *remainingSlot-static_cast<unsigned int>(chunkBytes);
            char** const updatedCursorSlot = storage.putCursorSlot;
            *updatedCursorSlot = *updatedCursorSlot+chunkBytes;
        }
        else
        {
            const int writeResult = static_cast<int>(static_cast<Msvc6Runtime::basic_streambuf*>(self)->overflow(static_cast<int>(*sourceBytes)));
            if (writeResult == -1)
                break;
            ++sourceBytes;
            ++transferredBytes;
            --count;
        }
    }
    return transferredBytes;
}


void AS_RECORD_DWORD_VECTOR::CopyConstruct(const AS_RECORD_DWORD_VECTOR& input)
{
    allocatorState[0] = input.allocatorState[0];
    int count = input.begin
        ? static_cast<int>(input.end-input.begin) : 0;
    if (count<0)count = 0;
    begin = static_cast<int*>(CrtNew(static_cast<unsigned int>(count)*sizeof(int)));
    end = VectorOperations::Copy(input.begin, input.end, begin);
    capacityEnd = end;
}

AS_RECORD_DWORD_VECTOR& AS_RECORD_DWORD_VECTOR::operator=(const AS_RECORD_DWORD_VECTOR& other)
{
    if (this != &other)
        VectorOperations::Assign(begin, end, capacityEnd, other.begin, other.end);
    return *this;
}

void AS_RECORD_DWORD_VECTOR::insert(int* position, unsigned int count, const int& value)
{
    VectorOperations::Insert(begin, end, capacityEnd, position, count, value);
}

int* AS_RECORD_DWORD_VECTOR::insert(int* position, const int& value)
{
    const unsigned int index = begin ? static_cast<unsigned int>(position-begin) : 0u;
    insert(position, 1u, value);
    return begin+index;
}

void AS_RECORD_PTR_VECTOR::insert(AS_RECORD** position, unsigned int count, AS_RECORD* const& value)
{
    VectorOperations::Insert(begin, end, capacityEnd, position, count, value);
}


void RecordDBBadCastControl::_Doraise() const
{
    RecordDBBadCastControl exceptionObject;
    exceptionObject.CopyConstruct(*this);
    Msvc6::ThrowException(&exceptionObject, Msvc6::BadCastThrowInfo());
}


void  sentry_Dtor(void* self)
{
    if (Msvc6Runtime::ExceptionInFlight(self))return;
    void* const stream = static_cast<RecordDBOstreamSentry*>(self)->stream;
    const Msvc6::VirtualBaseTable* vbtable = static_cast<RecordDBStreamVirtualBase*>(stream)->vbtable;
    RecordDBIosControl* ios = reinterpret_cast<RecordDBIosControl*>(
        static_cast<unsigned char*>(stream)+vbtable->virtualBaseOffset);
    if ((static_cast<unsigned char>(ios->formatFlags)&2u) == 0u)return;
    unsigned int fail = 0u;
    if ((static_cast<unsigned char>(ios->state)&6u) == 0u)
    {
        if (static_cast<int>(reinterpret_cast<Msvc6Runtime::basic_streambuf*>(ios->buffer)->sync()) == -1)fail = 4u;
    }
    vbtable = static_cast<RecordDBStreamVirtualBase*>(stream)->vbtable;
    ios = reinterpret_cast<RecordDBIosControl*>(
        static_cast<unsigned char*>(stream)+vbtable->virtualBaseOffset);
    if (fail)
    {
        unsigned int flags = ios->state|fail;
        if (ios->buffer == 0)flags|=4u;
        Msvc6Runtime::BasicIosClear(ios, flags, 0u);
    }
}

void*  RecordDBBasicFilebufOpen(void* self, const char* fileName, int openMode)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);
    if (storage.file != 0)return 0;
    std::FILE* const file = Msvc6::Fiopen(fileName, openMode);
    if (file == 0)return 0;
    RecordDBBasicFilebufInit(self, PtrToU32(file), 1u);
    (void)RecordDBBasicFilebufInitCodecvt(self);
    return self;
}

void*  RecordDBCloseFileAndResetBuffers(void* self)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);
    std::FILE* const file = storage.file;
    if (file == 0)return 0;
    if (CrtFileClose(PtrToU32(file)) != 0u)return 0;
    RecordDBBasicFilebufInit(self, 0u, 2u);
    return self;
}

void*  RecordDBStringAppendSubstring(void* self, const void* source, unsigned int offset, unsigned int count)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);
    const AS_RECORD_STRING_CONTROL& sourceStorage = *static_cast<const AS_RECORD_STRING_CONTROL*>(source);

    const unsigned int sourceLength = sourceStorage.size;
    if (sourceLength<offset)(void)Msvc6RangeError();
    unsigned int copyCount = MinUnsigned(sourceLength-offset, count), previousLength = storage.size;
    if (AS_RECORD_STRING_CONTROL::npos-previousLength <= copyCount)(void)Msvc6LengthError();
    if (copyCount == 0u)return self;
    const unsigned int newLength = previousLength+copyCount, payloadAddress = PtrToU32(storage.data);
    if (StringIsShared(payloadAddress) || storage.capacity<newLength)StringReserve(self, newLength);
    unsigned int sourceAddress = PtrToU32(sourceStorage.data);
    if (sourceAddress == 0u)sourceAddress = PtrToU32(Msvc6::StringEmptyData<char>());
    CopyBytes(U32ToPtr(PtrToU32(storage.data)+previousLength), U32ToPtr(sourceAddress+offset), copyCount);
    storage.size = newLength;
    WriteGlobalU8(PtrToU32(storage.data)+newLength, 0u);
    return self;
}


void*  RecordDBStringAppendBytes(void* self, const void* source, unsigned int count)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);

    const unsigned int previousLength = storage.size;
    if (AS_RECORD_STRING_CONTROL::npos-previousLength <= count)(void)Msvc6LengthError();
    if (count == 0u)return self;
    const unsigned int newLength = previousLength+count, payloadAddress = PtrToU32(storage.data);
    if (StringIsShared(payloadAddress) || storage.capacity<newLength)StringReserve(self, newLength);
    CopyBytes(U32ToPtr(PtrToU32(storage.data)+previousLength), source, count);
    storage.size = newLength;
    WriteGlobalU8(PtrToU32(storage.data)+newLength, 0u);
    return self;
}


void*  RecordDBStringAssignBytes(void* self, const void* source, unsigned int count)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);

    if (count>0xFFFFFFFDu)(void)Msvc6LengthError();
    unsigned int payloadAddress = PtrToU32(storage.data);
    if (StringIsShared(payloadAddress))
    {
        if (count == 0u)
        {
            WriteGlobalU8(payloadAddress-1u, static_cast<unsigned char>(ReadGlobalU8(payloadAddress-1u)-1u));
            storage.data = 0;
            storage.size = 0u;
            storage.capacity = 0u;
            return self;
        }
        StringReserve(self, count);
    }
    else if (count == 0u)
    {
        StringReset(self, 1u);
        return self;
    }
    else
    {
        const unsigned int capacity = storage.capacity;
        if (capacity>31u || capacity<count)
        {
            StringReset(self, 1u);
            StringReserve(self, count);
        }
    }
    CopyBytes(storage.data, source, count);
    storage.size = count;
    WriteGlobalU8(PtrToU32(storage.data)+count, 0u);
    return self;
}


void  RecordDBStringFreeze(void* self)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);

    unsigned int payloadAddress = PtrToU32(storage.data);
    if (StringIsShared(payloadAddress))
    {
        const unsigned int length = storage.size;
        if (length>0xFFFFFFFDu)(void)Msvc6LengthError();
        payloadAddress = PtrToU32(storage.data);
        if (StringIsShared(payloadAddress))
        {
            if (length == 0u)
            {
                WriteGlobalU8(payloadAddress-1u, static_cast<unsigned char>(ReadGlobalU8(payloadAddress-1u)-1u));
                storage.data = 0;
                storage.size = 0u;
                storage.capacity = 0u;
            }
            else StringReserve(self, length);
        }
        else if (length == 0u)
        {
            if (payloadAddress)RecordDBStringEos(self, 0u);
        }
        else if (storage.capacity<length)StringReserve(self, length);
    }
    payloadAddress = PtrToU32(storage.data);
    if (payloadAddress)WriteGlobalU8(payloadAddress-1u, 0xFFu);
}


void  RecordDBStringSplit(void* self)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);

    unsigned int oldData = PtrToU32(storage.data);
    if (!StringIsShared(oldData))return;
    WriteGlobalU8(oldData-1u, static_cast<unsigned char>(ReadGlobalU8(oldData-1u)-1u));
    storage.data = 0;
    storage.size = 0u;
    storage.capacity = 0u;
    const unsigned int length = CStrLen32(oldData);
    if (length>0xFFFFFFFDu)(void)Msvc6LengthError();
    if (length == 0u)
    {
        StringReset(self, 0u);
        return;
    }
    StringReserve(self, length);
    CopyBytes(storage.data, U32ToPtr(oldData), length);
    storage.size = length;
    WriteGlobalU8(PtrToU32(storage.data)+length, 0u);
}


unsigned char  RecordDBBasicOstreamOpfx(void* self)
{
    const Msvc6::VirtualBaseTable* const virtualBaseTableAddress = static_cast<RecordDBStreamVirtualBase*>(self)->vbtable;
    int virtualBaseOffset = static_cast<int>(virtualBaseTableAddress->virtualBaseOffset);
    void* iosBase = static_cast<unsigned char*>(self)+virtualBaseOffset;
    if (static_cast<RecordDBIosControl*>(iosBase)->state == 0u)
    {
        void* tied = static_cast<RecordDBIosControl*>(iosBase)->tiedStream;
        if (tied)
        {
            const Msvc6::VirtualBaseTable* tiedVirtualBaseTableAddress = static_cast<RecordDBStreamVirtualBase*>(tied)->vbtable;
            int tiedVirtualBaseOffset = static_cast<int>(tiedVirtualBaseTableAddress->virtualBaseOffset);
            void* tiedIosBase = static_cast<unsigned char*>(tied)+tiedVirtualBaseOffset;
            unsigned int syncFailureState = 0u;
            if ((static_cast<unsigned char>(static_cast<RecordDBIosControl*>(tiedIosBase)->state)&6u) == 0u)
            {
                void* buffer = static_cast<RecordDBIosControl*>(tiedIosBase)->buffer;
                if (static_cast<int>(static_cast<Msvc6Runtime::basic_streambuf*>(buffer)->sync()) == -1)syncFailureState = 4u;
            }
            tiedVirtualBaseTableAddress = static_cast<RecordDBStreamVirtualBase*>(tied)->vbtable;
            tiedVirtualBaseOffset = static_cast<int>(tiedVirtualBaseTableAddress->virtualBaseOffset);
            tiedIosBase = static_cast<unsigned char*>(tied)+tiedVirtualBaseOffset;
            if (syncFailureState)
            {
                unsigned int flags = static_cast<RecordDBIosControl*>(tiedIosBase)->state|syncFailureState;
                if (static_cast<RecordDBIosControl*>(tiedIosBase)->buffer == nullptr)flags|=4u;
                Msvc6Runtime::BasicIosClear(tiedIosBase, flags, 0u);
            }
        }
    }
    virtualBaseOffset = static_cast<int>(static_cast<RecordDBStreamVirtualBase*>(self)->vbtable->virtualBaseOffset);
    iosBase = static_cast<unsigned char*>(self)+virtualBaseOffset;
    return static_cast<RecordDBIosControl*>(iosBase)->state == 0u?1u:0u;
}

unsigned int RecordDBGetOrAssignGlobalSequenceId(Msvc6::LocaleId* self)
{
    unsigned int guard = 0u;
    Msvc6Runtime::StaticLockConstruct(&guard);
    if (self->value == 0u)
    {
        const unsigned int next = static_cast<unsigned int>(Msvc6::LocaleId::_Id_cnt) + 1u;
        Msvc6::LocaleId::_Id_cnt = BitsToSigned32(next);
        self->value = next;
    }
    const unsigned int result = self->value;
    Msvc6Runtime::StaticLockDestruct(&guard);
    return result;
}


void*  RecordDBStringAppendFill(void* self, unsigned int count, unsigned int value)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);

    const unsigned int oldLength = storage.size;
    const unsigned int maxLength = AS_RECORD_STRING_CONTROL::npos;
    if (maxLength-oldLength <= count)(void)Msvc6LengthError();
    if (count == 0u)return self;
    const unsigned int newLength = oldLength+count;
    if (newLength>0xFFFFFFFDu)(void)Msvc6LengthError();
    unsigned int payloadAddress = PtrToU32(storage.data);
    unsigned int shared = 0u;
    unsigned char referenceCount = 0u;
    if (payloadAddress != 0u)
    {
        referenceCount = ReadGlobalU8(payloadAddress-1u);
        shared = (referenceCount != 0u && referenceCount != 0xFFu)?1u:0u;
    }
    if (shared != 0u)
    {
        if (newLength == 0u)
        {
            WriteGlobalU8(payloadAddress-1u, static_cast<unsigned char>(referenceCount-1u));
            storage.data = 0;
            storage.size = 0u;
            storage.capacity = 0u;
            return self;
        }
        StdString_CopyStorage(self, newLength);
    }
    else
    {
        if (newLength == 0u)
        {
            if (payloadAddress != 0u)RecordDBStringEos(self, 0u);
            return self;
        }
        if (storage.capacity<newLength)StdString_CopyStorage(self, newLength);
    }
    payloadAddress = PtrToU32(storage.data);
    unsigned char* fillBytes = U32ToPtr(payloadAddress+oldLength);
    const unsigned char fill = static_cast<unsigned char>(value);
    for (unsigned int fillIndex = 0;fillIndex<count;++fillIndex)fillBytes[fillIndex] = fill;
    storage.size = newLength;
    WriteGlobalU8(payloadAddress+newLength, 0u);
    return self;
}


void  RecordDBBasicFilebufInit(void* self, unsigned int backing, unsigned int mode)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);
    storage.ownsFile = static_cast<unsigned char>(mode == 1u);
    if (mode == 0u)
    {
        void* const current = storage.locale;
        if (current)
        {
            void* const object = RecordDBRefCountReleaseAndReturnIfZero(current);
            if (object)
                delete static_cast<Msvc6Runtime::locale::facet*>(object);
        }
        (void)Msvc6LocaleDefaultCtor(&storage.locale);
        storage.conversionBuffer = 0;
    }
    storage.streambuf.getBeginSlot = &storage.streambuf.getBegin;
    storage.streambuf.getCursorSlot = &storage.streambuf.getCursor;
    storage.streambuf.putCursorSlot = &storage.streambuf.putCursor;
    storage.streambuf.putBeginSlot = &storage.streambuf.putBegin;
    storage.streambuf.getRemainingSlot = &storage.streambuf.getRemaining;
    storage.streambuf.putRemainingSlot = &storage.streambuf.putRemaining;
    storage.streambuf.putBegin = 0;
    storage.streambuf.putCursor = 0;
    storage.streambuf.putRemaining = 0u;
    storage.streambuf.getBegin = 0;
    storage.streambuf.getCursor = 0;
    storage.streambuf.getRemaining = 0u;
    if (backing != 0u && storage.ownsFile == 0u)
    {
        Msvc6BasicStreambufInitRanges(self, backing+8u, backing, backing+4u,
            backing+8u, backing, backing+4u);
    }
    storage.file = reinterpret_cast<std::FILE*>(U32ToPtr(backing));
    storage.conversionState40 = static_cast<unsigned int>(Msvc6::FilebufInitialState);
    storage.conversionState3C = static_cast<unsigned int>(Msvc6::FilebufInitialState);
    storage.codecvt = 0;
}

static char* RecordDBDupCStringExact(const char* source)
{
    unsigned int length = 0u;
    while (source[length] != 0)++length;
    char* output = static_cast<char*>(CrtNew(length+1u));
    for (unsigned int byteIndex = 0;byteIndex <= length;++byteIndex)output[byteIndex] = source[byteIndex];
    return output;
}


void*  RecordDBNumpunctCtor(void* self, unsigned int references)
{
    RecordDBNumpunctControl& storage = *static_cast<RecordDBNumpunctControl*>(self);
    storage.references = references;
    storage.vftable = Msvc6::CharNumpunct::GetTable();
    try
    {
        unsigned char localeObject[64];
        Msvc6Runtime::LocaleConstruct(localeObject, PtrToU32("C"));
        try
        {
            const CrtLocaleConvention* const convention = Msvc6::GetLocaleConvention();
            storage.decimalPoint = convention->decimalPoint[0];
            storage.thousandsSeparator = convention->thousandsSeparator[0];
            storage.grouping = RecordDBDupCStringExact(convention->grouping);
            storage.falseName = RecordDBDupCStringExact("false");
            storage.trueName = RecordDBDupCStringExact("true");
        }
        catch (...)
        {
            Msvc6Runtime::LocaleDestruct(localeObject);
            throw;
        }
        Msvc6Runtime::LocaleDestruct(localeObject);
    }
    catch (...)
    {
        storage.vftable = Msvc6::BaseFacetTable();
        throw;
    }
    return self;
}

void*  RecordDBNumPutCtor(void* self, unsigned int references)
{
    Msvc6::FacetData& storage = *static_cast<Msvc6::FacetData*>(self);
    storage.references = references;
    storage.vftable = Msvc6::CharNumPut::GetTable();
    try
    {
        unsigned char localeObject[64];
        Msvc6Runtime::LocaleConstruct(localeObject, PtrToU32("C"));
        Msvc6Runtime::LocaleDestruct(localeObject);
    }
    catch (...)
    {
        storage.vftable = Msvc6::BaseFacetTable();
        throw;
    }
    return self;
}

void*  RecordDBBasicOstreamCtor(void* self, unsigned int backing,
    unsigned int registerStream, unsigned int initialize, unsigned int constructVirtualBase)
{
    RecordDBStreamVirtualBase& stream = *static_cast<RecordDBStreamVirtualBase*>(self);
    if (constructVirtualBase)
    {
        stream.vbtable = &Msvc6::OstreamVirtualBase;
        RecordDBIosControl* const standalone = reinterpret_cast<RecordDBIosControl*>(
            static_cast<unsigned char*>(self)+4u);
        standalone->registration = 0u;
        standalone->vftable = Msvc6::IosTable();
    }
    RecordDBIosControl* const ios = reinterpret_cast<RecordDBIosControl*>(
        static_cast<unsigned char*>(self)+stream.vbtable->virtualBaseOffset);
    ios->vftable = Msvc6::OstreamTable();
    try
    {
      if (static_cast<unsigned char>(initialize) != 0u)
        {
        ios->buffer = reinterpret_cast<RecordDBStreambufControl*>(U32ToPtr(backing));
        ios->tiedStream = 0;
        ios->fill = 32u;
        Msvc6Runtime::BasicIosInit(ios);
        if (ios->buffer == 0)
            Msvc6Runtime::BasicIosClear(ios, ios->state|4u, 0u);
        if (static_cast<unsigned char>(registerStream) != 0u)
            Msvc6Runtime::RegisterStream(ios);
    }
      }
    catch (...)
    {
        if (constructVirtualBase)
            (void)RecordDBBasicIosDestructor(static_cast<unsigned char*>(self)+4u);
        throw;
      }
    return self;
}

void*  RecordDBBasicIstreamCtor(void* self, unsigned int backing,
    unsigned int registerStream, unsigned int constructVirtualBase)
{
    RecordDBStreamControl& stream = *static_cast<RecordDBStreamControl*>(self);
    if (constructVirtualBase)
    {
        stream.vbtable = &Msvc6::IstreamVirtualBase;
        stream.ios.registration = 0u;
        stream.ios.vftable = Msvc6::IosTable();
    }
    stream.count = 0u;
    RecordDBIosControl* const ios = reinterpret_cast<RecordDBIosControl*>(
        static_cast<unsigned char*>(self)+stream.vbtable->virtualBaseOffset);
    ios->vftable = Msvc6::IstreamTable();
    ios->buffer = reinterpret_cast<RecordDBStreambufControl*>(U32ToPtr(backing));
    ios->tiedStream = 0;
    ios->fill = 32u;
    try
    {
      Msvc6Runtime::BasicIosInit(ios);
    if (ios->buffer == 0)
        Msvc6Runtime::BasicIosClear(ios, ios->state|4u, 0u);
    if (static_cast<unsigned char>(registerStream) != 0u)
        Msvc6Runtime::RegisterStream(ios);
    }
    catch (...)
    {
        if (constructVirtualBase)
            (void)RecordDBBasicIosDestructor(static_cast<unsigned char*>(self)+8u);
        throw;
    }
    return self;
}
static void RecordDBReleaseLocaleRef(unsigned int localeOwner);

static void* RecordDBCopyTaggedString(const char* source, void* out, unsigned char tag)
{
    AS_RECORD_STRING_CONTROL& output = *static_cast<AS_RECORD_STRING_CONTROL*>(out);
    output.allocatorState[0] = tag;
    output.data = 0;
    output.size = 0u;
    output.capacity = 0u;
    const char* sourceBytes = source;
    unsigned length = CStringLength(sourceBytes);
    if (!length)
    {
        Tidy(out, 1u);
        return out;
    }
    StdString_CopyStorage(out, length);
    char* destinationBytes = (char*)output.data;
    if (destinationBytes)
    {
        for (unsigned byteIndex = 0;byteIndex<length;++byteIndex)destinationBytes[byteIndex] = sourceBytes[byteIndex];
        destinationBytes[length] = 0;
    }
    output.size = length;
    return out;
}

void*  RecordDBNumpunctDoGrouping(void* self, void* out)
{
    return RecordDBCopyTaggedString(static_cast<RecordDBNumpunctControl*>(self)->grouping, out, static_cast<unsigned char>(PtrToU32(out)));
}

void*  RecordDBNumpunctDoFalseName(void* self, void* out)
{
    return RecordDBCopyTaggedString(static_cast<RecordDBNumpunctControl*>(self)->falseName, out, static_cast<unsigned char>(PtrToU32(out)));
}

void*  RecordDBNumpunctDoTrueName(void* self, void* out)
{
    return RecordDBCopyTaggedString(static_cast<RecordDBNumpunctControl*>(self)->trueName, out, static_cast<unsigned char>(PtrToU32(out)));
}


void*  RecordDBVectorRecordErase(void* self, void* first, void* last)
{
    AS_RECORD_VECTOR& storage = *static_cast<AS_RECORD_VECTOR*>(self);
    AS_RECORD* destination = static_cast<AS_RECORD*>(first);
    AS_RECORD* source = static_cast<AS_RECORD*>(last);
    AS_RECORD* const oldEnd = storage.end;
    while (source != oldEnd)
    {
        *destination = *source;
        ++destination;
        ++source;
    }
    for (AS_RECORD* record = destination;record != oldEnd;++record)
        record->~AS_RECORD();
    storage.end = destination;
    return first;
}

void zRecordDB::ClearTable(int iType)
{
    (void)LoadIfNeed(iType);
    AS_RECORD_VECTOR& slot = m_Data[iType];
    for (AS_RECORD* recordCursor = slot.begin;recordCursor != slot.end;++recordCursor)
        recordCursor->~AS_RECORD();
    slot.end = slot.begin;
    m_OutFlags[iType] = 1u;
    (void)SaveToFile(iType);
}

void*  RecordDBCtypeCharCtor(void* self, unsigned int table32, unsigned int deleteTable, unsigned int refs)
{
    RecordDBCtypeControl& storage = *static_cast<RecordDBCtypeControl*>(self);
    storage.references = refs;
    storage.vftable = Msvc6::CharCtype::GetTable();
    try
    {
        unsigned int outerLock;
        Msvc6Runtime::StaticLockConstruct(&outerLock);
        try
        {
            unsigned char localeObject[64];
            Msvc6Runtime::LocaleConstruct(localeObject, PtrToU32("C"));
            try
            {
                unsigned int descriptorLock;
                Msvc6Runtime::StaticLockConstruct(&descriptorLock);
                RecordDBCtypeDescriptor descriptor;
                const unsigned int descriptor32 = Msvc6CtypeDescriptor(PtrToU32(&descriptor));
                storage.descriptor = *reinterpret_cast<const RecordDBCtypeDescriptor*>(U32ToPtr(descriptor32));
                if (RecordDBCtypeControl::_Cltab == 0)
                {
                    RecordDBCtypeControl::_Cltab = reinterpret_cast<const short*>(storage.descriptor.table);
                    (void)RegisterCrtAtExit(&RecordDBCtypeTerm);
                    storage.descriptor.ownsTable = 0u;
                }
                Msvc6Runtime::StaticLockDestruct(&descriptorLock);
            }
            catch (...)
            {
                Msvc6Runtime::LocaleDestruct(localeObject);
                throw;
            }
            Msvc6Runtime::LocaleDestruct(localeObject);
            if (storage.descriptor.ownsTable != 0u)
            {
                (void)CrtFree(PtrToU32(storage.descriptor.table));
                storage.descriptor.ownsTable = 0u;
            }
            if (table32 == 0u)
                storage.descriptor.table = reinterpret_cast<unsigned short*>(const_cast<short*>(RecordDBCtypeControl::_Cltab));
            else
            {
                storage.descriptor.table = reinterpret_cast<unsigned short*>(U32ToPtr(table32));
                storage.descriptor.ownsTable = deleteTable&0xFFu;
            }
        }
        catch (...)
        {
            Msvc6Runtime::StaticLockDestruct(&outerLock);
            throw;
        }
        Msvc6Runtime::StaticLockDestruct(&outerLock);
    }
    catch (...)
    {
        storage.vftable = Msvc6::BaseFacetTable();
        throw;
    }
    return self;
}

static void RecordDBRetainLocaleRef(unsigned int localeOwner)
{
    if (!localeOwner) return;
    unsigned int lockStorage = 0u;
    Msvc6Runtime::StaticLockConstruct(&lockStorage);
    const unsigned int count = reinterpret_cast<MsvcLocaleFacetRef*>(U32ToPtr(localeOwner))->m_refCount04;
    if (count<0xFFFFFFFFu) reinterpret_cast<MsvcLocaleFacetRef*>(U32ToPtr(localeOwner))->m_refCount04 = count+1u;
    Msvc6Runtime::StaticLockDestruct(&lockStorage);
}

static unsigned int RecordDBAddCodecvtFacetCore(unsigned int resultOut32, unsigned int localeOwner, unsigned int facet32)
{
    unsigned int localLocale = localeOwner;
    unsigned int lockStorage;
    Msvc6Runtime::StaticLockConstruct(&lockStorage);
    try
    {
        const unsigned int facetId = RecordDBGetOrAssignGlobalSequenceId(&Msvc6::FacetState<Msvc6::CharCodecvt>::id);
        const unsigned int resultObject = Msvc6Runtime::LocaleAddFacet(&localLocale, facet32, facetId, 2u);
        const unsigned int resultOwner = ReadGlobalU32(resultObject);
        WriteGlobalU32(resultOut32, resultOwner);
        RecordDBRetainLocaleRef(resultOwner);
    }
    catch (...)
    {
        Msvc6Runtime::StaticLockDestruct(&lockStorage);
        RecordDBReleaseLocaleRef(localLocale);
        throw;
    }
    Msvc6Runtime::StaticLockDestruct(&lockStorage);
    RecordDBReleaseLocaleRef(localLocale);
    return resultOut32;
}

static void RecordDBDestroyStreamVariant(void* streamThis, unsigned int vbOffset, const Msvc6::FacetVirtualTable* interimVbaseVt, unsigned int finalBackOffset, const Msvc6::FacetVirtualTable* finalSubVt)
{
    unsigned char* const ios = static_cast<unsigned char*>(streamThis);
    RecordDBFilebufControl* const filebuf = reinterpret_cast<RecordDBFilebufControl*>(ios-84u);
    unsigned char* const owner = ios-vbOffset;
    const Msvc6::VirtualBaseTable* const vbtable = reinterpret_cast<RecordDBStreamVirtualBase*>(owner)->vbtable;
    reinterpret_cast<RecordDBIosControl*>(owner + vbtable->virtualBaseOffset)->vftable = interimVbaseVt;
    try
    {
        basic_filebuf_Dtor(filebuf);
    }
    catch (...)
    {
        unsigned char* const streamBase = reinterpret_cast<unsigned char*>(filebuf)-finalBackOffset;
        const Msvc6::VirtualBaseTable* const finalTable = reinterpret_cast<RecordDBStreamVirtualBase*>(streamBase)->vbtable;
        reinterpret_cast<RecordDBIosControl*>(streamBase + finalTable->virtualBaseOffset)->vftable = finalSubVt;
        throw;
    }
    unsigned char* const streamBase = reinterpret_cast<unsigned char*>(filebuf)-finalBackOffset;
    const Msvc6::VirtualBaseTable* const finalTable = reinterpret_cast<RecordDBStreamVirtualBase*>(streamBase)->vbtable;
    reinterpret_cast<RecordDBIosControl*>(streamBase + finalTable->virtualBaseOffset)->vftable = finalSubVt;
}

void  basic_ifstream_Dtor(void* self)
{
    RecordDBDestroyStreamVariant(self, 92u, Msvc6::IfstreamTable(), 8u, Msvc6::IstreamTable());
}

void  basic_ofstream_Dtor(void* self)
{
    RecordDBDestroyStreamVariant(self, 88u, Msvc6::OfstreamTable(), 4u, Msvc6::OstreamTable());
}

void zRecordDB::BuildDataIfNeed(int iType)
{
    const unsigned int index = static_cast<unsigned int>(iType);
    if (m_OutFlags[index] == 0u)
        return;
    m_OutFlags[index] = 0u;

    AS_RECORD_VECTOR& data = m_Data[index];
    AS_RECORD_PTR_VECTOR& out = m_OutData[index];
    const int count = data.begin ? static_cast<int>(data.end-data.begin) : 0;
    out.end = out.begin;

    for (int recordIndex = 0;recordIndex<count;++recordIndex)
    {
        AS_RECORD* record = data.begin+recordIndex;
        out.insert(out.end, 1u, record);
    }

    if (count <= 1)
        return;

    int heap = count-1;
    while (heap>0)
    {
        int best = heap;
        AS_RECORD** const list = out.begin;
        for (int recordIndex = 0;recordIndex<heap;++recordIndex)
        {
            const AS_RECORD* lhs = list[recordIndex];
            const AS_RECORD* rhs = list[best];
            if (lhs->m_totalScore<rhs->m_totalScore ||
               (lhs->m_totalScore == rhs->m_totalScore && lhs->m_monsterKills<rhs->m_monsterKills))
                best = recordIndex;
        }
        AS_RECORD* swappedRecord = list[heap];
        list[heap] = list[best];
        list[best] = swappedRecord;
        --heap;
    }
}


AS_RECORD::AS_RECORD(const AS_RECORD& other)
{

    m_playerName.allocatorState[0] = other.m_playerName.allocatorState[0];
    m_playerName.data = 0;
    m_playerName.size = 0u;
    m_playerName.capacity = 0u;
    (void)RecordDBStringAssignSubstring(&m_playerName,
        const_cast<AS_RECORD_STRING_CONTROL*>(&other.m_playerName), 0u,
        AS_RECORD_STRING_CONTROL::npos);

    try
    {
        m_totalScore = other.m_totalScore;
        m_monsterKills = other.m_monsterKills;
        m_elapsedSeconds = other.m_elapsedSeconds;
        m_checksum = other.m_checksum;

        m_recordTimestamp.allocatorState[0] = other.m_recordTimestamp.allocatorState[0];
        m_recordTimestamp.data = 0;
        m_recordTimestamp.size = 0u;
        m_recordTimestamp.capacity = 0u;
        (void)RecordDBStringAssignSubstring(&m_recordTimestamp,
            const_cast<AS_RECORD_STRING_CONTROL*>(&other.m_recordTimestamp), 0u,
            AS_RECORD_STRING_CONTROL::npos);
        try
        {
            m_weaponScores.CopyConstruct(other.m_weaponScores);
            try
            {
                m_weaponKills.CopyConstruct(other.m_weaponKills);
                m_cheater = other.m_cheater;
            }
            catch (...)
            {
                CrtFreePointer(m_weaponScores.begin);
                m_weaponScores.begin = 0;
                m_weaponScores.end = 0;
                m_weaponScores.capacityEnd = 0;
                throw;
            }
        }
        catch (...)
        {
            Tidy(&m_recordTimestamp, 1u);
            throw;
        }
    }
    catch (...)
    {
        Tidy(&m_playerName, 1u);
        throw;
    }
}

void __cdecl RecordDBConstructRecord(void* destination, void* source)
{
    if (destination)
        new (destination) AS_RECORD(*static_cast<const AS_RECORD*>(source));
}
void  basic_filebuf_Dtor(void* self)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);
    storage.streambuf.vftable = Msvc6::FilebufTable();
    try
    {
        try
        {
            if (storage.ownsFile != 0u && storage.file != 0)
                (void)RecordDBCloseFileAndResetBuffers(self);
            AS_RECORD_STRING_CONTROL* const block = storage.conversionBuffer;
            if (block)
            {
                Tidy(block, 1u);
                CrtFreePointer(block);
            }
        }
        catch (...)
        {
            Msvc6LocaleFacetDecref(&storage.locale);
            throw;
        }
        Msvc6LocaleFacetDecref(&storage.locale);
    }
    catch (...)
    {
        basic_streambuf_Dtor(self);
        throw;
    }
    basic_streambuf_Dtor(self);
}

int  RecordDBBasicFilebufPbackfail(void* self, int value)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);

    char** cursorSlot = storage.streambuf.getCursorSlot;
    unsigned int cursor = cursorSlot?PtrToU32(*cursorSlot):0u;
    char** beginSlot = storage.streambuf.getBeginSlot;
    const unsigned int begin = beginSlot?PtrToU32(*beginSlot):0u;
    if (cursor != 0u && begin<cursor)
    {
        if (value == -1 || static_cast<unsigned int>(ReadGlobalU8(cursor-1u)) == static_cast<unsigned int>(value))
        {
            unsigned int* countSlot = storage.streambuf.getRemainingSlot;
            *countSlot = *countSlot+1u;
            *cursorSlot = reinterpret_cast<char*>(U32ToPtr(cursor-1u));
            return value == -1?0:value;
        }
    }

    const unsigned int stream = PtrToU32(storage.file);
    if (stream == 0u || value == -1) return -1;

    if (storage.codecvt == nullptr)
    {
        const int rc = BitsToSigned32(CrtFileUngetC(static_cast<unsigned int>(value)&0xFFu, stream));
        return rc == -1?-1:value;
    }

    AS_RECORD_STRING_CONTROL* buffer = storage.conversionBuffer;
    unsigned int remaining = buffer->size;
    if (remaining == 0u) return -1;

    unsigned int data = PtrToU32(buffer->data);
    if (data != 0u)
    {
        const unsigned char marker = ReadGlobalU8(data-1u);
        if (marker != 0u && marker != 0xFFu)
            (void)Grow(buffer, remaining, 0u);
    }
    data = PtrToU32(buffer->data);
    if (data != 0u) WriteGlobalU8(data-1u, 0xFFu);

    unsigned int p = data+remaining;
    while (remaining != 0u)
    {
        --p;
        const int rc = BitsToSigned32(CrtFileUngetC(ReadGlobalU8(p), stream));
        if (rc == -1) break;
        --remaining;
    }
    if (remaining != 0u) return -1;

    buffer = storage.conversionBuffer;
    const unsigned int oldData = PtrToU32(buffer->data);
    if (oldData != 0u)
    {
        const unsigned char marker = ReadGlobalU8(oldData-1u);
        if (marker != 0u && marker != 0xFFu)
        {
            Tidy(buffer, 1u);
            unsigned int length = 0u;
            while (ReadGlobalU8(oldData+length) != 0u) ++length;
            (void)RecordDBStringAssignBytes(buffer, U32ToPtr(oldData), length);
        }
    }

    const unsigned int size = buffer->size;
    unsigned int trim = AS_RECORD_STRING_CONTROL::npos;
    if (size<trim) trim = size;
    if (trim != 0u)
    {
        data = PtrToU32(buffer->data);
        (void)CrtMemmove(data, data+trim, size-trim);
        const unsigned int newSize = size-trim;
        if (Grow(buffer, newSize, 0u) != 0u)
        {
            buffer->size = newSize;
            data = PtrToU32(buffer->data);
            WriteGlobalU8(data+newSize, 0u);
        }
    }
    storage.conversionState40 = storage.conversionState3C;
    return value;
}


int  RecordDBBasicFilebufOverflow(void* self, int value)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);
    if (value == -1)return 0;
    char* cursor = *storage.streambuf.putCursorSlot;
    if (cursor && PtrToU32(cursor)<PtrToU32(cursor)+*storage.streambuf.putRemainingSlot)
    {
        --*storage.streambuf.putRemainingSlot;
        cursor = *storage.streambuf.putCursorSlot;
        *storage.streambuf.putCursorSlot = cursor+1;
        *cursor = static_cast<char>(value);
        return value;
    }
    unsigned int file = PtrToU32(storage.file);
    if (file == 0u)return -1;
    if (storage.codecvt == 0)
        return BitsToSigned32(CrtFilePutC(static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(value))), file)) == -1 ? -1 : value;

    const char byte = static_cast<char>(value);
    AS_RECORD_STRING_CONTROL* buffer = storage.conversionBuffer;
    const unsigned int maximum = AS_RECORD_STRING_CONTROL::npos;
    RecordDBStringSplit(buffer);
    const unsigned int trim = MinUnsigned(buffer->size, maximum);
    if (trim != 0u)
    {
        (void)CrtMemmove(PtrToU32(buffer->data), PtrToU32(buffer->data)+trim, buffer->size-trim);
        const unsigned int remaining = buffer->size-trim;
        if (Grow(buffer, remaining, 0u))
        {
            buffer->size = remaining;
            buffer->data[remaining] = 0;
        }
    }
    buffer = storage.conversionBuffer;
    if (AS_RECORD_STRING_CONTROL::npos-buffer->size <= 8u)(void)Msvc6LengthError();
    const unsigned int enlarged = buffer->size+8u;
    if (Grow(buffer, enlarged, 0u))
    {
        char* const added = buffer->data+buffer->size;
        *reinterpret_cast<unsigned int*>(added) = 0u;
        *reinterpret_cast<unsigned int*>(added+4) = 0u;
        buffer->size = enlarged;
        buffer->data[enlarged] = 0;
    }
    buffer = storage.conversionBuffer;
    RecordDBStringFreeze(buffer);
    char* const end = buffer->data ? buffer->data+buffer->size : 0;
    buffer = storage.conversionBuffer;
    RecordDBStringFreeze(buffer);
    char* const begin = buffer->data;
    const char* inputNext;
    char* outputNext;
    const int result = static_cast<Msvc6Runtime::codecvt*>(storage.codecvt)->do_out(
        *reinterpret_cast<int*>(&storage.conversionState40), &byte, &byte+1, inputNext,
        begin, end, outputNext);
    if (result == 3)
        return BitsToSigned32(CrtFilePutC(static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(byte))), PtrToU32(storage.file))) == -1 ? -1 : value;
    if (result != 0 && (result != 1 || inputNext == &byte))return -1;
    buffer = storage.conversionBuffer;
    RecordDBStringFreeze(buffer);
    const unsigned int count = PtrToU32(outputNext)-PtrToU32(buffer->data);
    buffer = storage.conversionBuffer;
    RecordDBStringFreeze(buffer);
    return CrtFileWrite(PtrToU32(buffer->data), 1u, count, PtrToU32(storage.file)) == count
        ? value : -1;
}


int  RecordDBBasicFilebufUflow(void* self)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);
    char* cursor = *storage.streambuf.getCursorSlot;
    if (cursor && PtrToU32(cursor)<PtrToU32(cursor)+*storage.streambuf.getRemainingSlot)
    {
        --*storage.streambuf.getRemainingSlot;
        cursor = *storage.streambuf.getCursorSlot;
        *storage.streambuf.getCursorSlot = cursor+1;
        return static_cast<unsigned char>(*cursor);
    }
    unsigned int file = PtrToU32(storage.file);
    if (file == 0u)return -1;
    if (storage.codecvt == 0)
    {
        const int value = BitsToSigned32(CrtFileGetC(file));
        return value == -1 ? -1 : value&0xFF;
    }
    storage.conversionState3C = storage.conversionState40;
    AS_RECORD_STRING_CONTROL* buffer = storage.conversionBuffer;
    const unsigned int maximum = AS_RECORD_STRING_CONTROL::npos;
    RecordDBStringSplit(buffer);
    const unsigned int trim = MinUnsigned(buffer->size, maximum);
    if (trim != 0u)
    {
        (void)CrtMemmove(PtrToU32(buffer->data), PtrToU32(buffer->data)+trim, buffer->size-trim);
        const unsigned int remaining = buffer->size-trim;
        if (Grow(buffer, remaining, 0u))
        {
            buffer->size = remaining;
            buffer->data[remaining] = 0;
        }
    }
    for (;;)
    {
        const int value = BitsToSigned32(CrtFileGetC(PtrToU32(storage.file)));
        if (value == -1)return -1;
        buffer = storage.conversionBuffer;
        if (AS_RECORD_STRING_CONTROL::npos-buffer->size <= 1u)(void)Msvc6LengthError();
        const unsigned int enlarged = buffer->size+1u;
        if (Grow(buffer, enlarged, 0u))
        {
            buffer->data[buffer->size] = static_cast<char>(value);
            buffer->size = enlarged;
            buffer->data[enlarged] = 0;
        }
        storage.conversionState40 = storage.conversionState3C;
        buffer = storage.conversionBuffer;
        RecordDBStringFreeze(buffer);
        const char* const end = buffer->data ? buffer->data+buffer->size : 0;
        buffer = storage.conversionBuffer;
        RecordDBStringFreeze(buffer);
        const char* const begin = buffer->data;
        const char* inputNext;
        char byte;
        char* outputNext;
        const int result = static_cast<Msvc6Runtime::codecvt*>(storage.codecvt)->do_in(
            *reinterpret_cast<int*>(&storage.conversionState40), begin, end, inputNext,
            &byte, &byte+1, outputNext);
        if (result == 0)return static_cast<unsigned char>(byte);
        if (result != 1 && result != 3)return -1;
        if (result == 1 || storage.conversionBuffer->size<1u)continue;
        buffer = storage.conversionBuffer;
        if (buffer->data)
        {
            const unsigned char references = static_cast<unsigned char>(buffer->data[-1]);
            if (references != 0u && references != 0xFFu)(void)Grow(buffer, buffer->size, 0u);
        }
        if (buffer->data)buffer->data[-1] = static_cast<char>(0xFFu);
        return static_cast<unsigned char>(buffer->data[0]);
    }
}


void  RecordDBVectorRecordInsertFill(void*, AS_RECORD*, unsigned int, const AS_RECORD&)                                                                                                              ;


void*  RecordDBVectorRecordInsert(void* self, void* position, void* record)
{
    AS_RECORD_VECTOR& storage = *static_cast<AS_RECORD_VECTOR*>(self);
    const int index = BitsToSigned32(PtrToU32(position)-PtrToU32(storage.begin))/84;
    RecordDBVectorRecordInsertFill(self, static_cast<AS_RECORD*>(position), 1u,
        *static_cast<const AS_RECORD*>(record));
    return storage.begin+index;
}


void*  RecordDBBasicFilebufCtor(void* self, void* legacyFile)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);
    (void)Msvc6LocaleDefaultCtor(&storage.streambuf.locale);
    storage.streambuf.vftable = Msvc6::StreambufTable();
    Msvc6BasicStreambufInit(self);
    try
    {
        Msvc6BasicStreambufSetg(self, 0u, 0u, 0u);
    }
    catch (...)
    {
        Msvc6LocaleFacetDecref(&storage.streambuf.locale);
        throw;
    }
    try
    {
        (void)Msvc6LocaleDefaultCtor(&storage.locale);
        storage.streambuf.vftable = Msvc6::FilebufTable();
        storage.ownsFile = 0u;
        try
        {
            Msvc6LocaleFacetDecref(&storage.locale);
            (void)Msvc6LocaleDefaultCtor(&storage.locale);
            storage.conversionBuffer = 0;
            Msvc6BasicStreambufInit(self);
            Msvc6BasicStreambufSetp(self, 0u, 0u);
            Msvc6BasicStreambufSetg(self, 0u, 0u, 0u);
            const unsigned int file = PtrToU32(legacyFile);
            if (file != 0u && storage.ownsFile == 0u)
                Msvc6BasicStreambufInitRanges(self, file+8u, file, file+4u,
                    file+8u, file, file+4u);
            storage.file = static_cast<std::FILE*>(legacyFile);
            storage.conversionState40 = static_cast<unsigned int>(Msvc6::FilebufInitialState);
            storage.conversionState3C = static_cast<unsigned int>(Msvc6::FilebufInitialState);
            storage.codecvt = 0;
        }
        catch (...)
        {
            Msvc6LocaleFacetDecref(&storage.locale);
            throw;
        }
    }
    catch (...)
    {
        basic_streambuf_Dtor(self);
        throw;
    }
    return self;
}

RecordDBOfstreamControl::RecordDBOfstreamControl(const char* fileName, int openMode)
{
    vbtable = &Msvc6::OfstreamVirtualBase;
    (void)RecordDBCompilerInitVtable2920(&ios);
    ios.vftable = Msvc6::IosTable();
    try
    {
        (void)RecordDBBasicOstreamCtor(this, PtrToU32(&filebuf), 0u, 1u, 0u);
        try
        {
            (void)RecordDBBasicFilebufCtor(&filebuf, 0);
            try
            {
                static_cast<RecordDBIosControl*>(RecordDBIosBaseFromStream(this))->vftable = Msvc6::OfstreamTable();
                if (RecordDBBasicFilebufOpen(&filebuf, fileName, openMode) == 0)
                    BasicIos_SetState(RecordDBIosBaseFromStream(this), 2u, 0u);
            }
            catch (...)
            {
                basic_filebuf_Dtor(&filebuf);
                throw;
            }
        }
        catch (...)
        {
            basic_ostream_Dtor(&filebuf);
            throw;
        }
    }
    catch (...)
    {
        (void)RecordDBBasicIosDestructor(&ios);
        throw;
    }
}

RecordDBOfstreamControl::~RecordDBOfstreamControl() noexcept(false)
{
    basic_ofstream_VBaseDtor(this);
}

void RecordDBOfstreamControl::Close()
{
    if (RecordDBCloseFileAndResetBuffers(&filebuf) == 0)
        BasicIos_SetState(RecordDBIosBaseFromStream(this), 2u, 0u);
}

unsigned int RecordDBOfstreamControl::State() const
{
    const unsigned char* const owner = reinterpret_cast<const unsigned char*>(this);
    return reinterpret_cast<const RecordDBIosControl*>(owner+vbtable->virtualBaseOffset)->state;
}

RecordDBIfstreamControl::RecordDBIfstreamControl(const char* fileName, int openMode)
{
    vbtable = &Msvc6::IfstreamVirtualBase;
    (void)RecordDBCompilerInitVtable2920(&ios);
    ios.vftable = Msvc6::IosTable();
    try
    {
        (void)RecordDBBasicIstreamCtor(this, PtrToU32(&filebuf), 0u, 0u);
        try
        {
            (void)RecordDBBasicFilebufCtor(&filebuf, 0);
            try
            {
                static_cast<RecordDBIosControl*>(RecordDBIosBaseFromStream(this))->vftable = Msvc6::IfstreamTable();
                if (RecordDBBasicFilebufOpen(&filebuf, fileName, openMode) == 0)
                    BasicIos_SetState(RecordDBIosBaseFromStream(this), 2u, 0u);
            }
            catch (...)
            {
                basic_filebuf_Dtor(&filebuf);
                throw;
            }
        }
        catch (...)
        {
            RecordDBCompilerAdjustVtableMinus8(&filebuf);
            throw;
        }
    }
    catch (...)
    {
        (void)RecordDBBasicIosDestructor(&ios);
        throw;
    }
}

RecordDBIfstreamControl::~RecordDBIfstreamControl() noexcept(false)
{
    RecordDBResetEmbedded5C(this);
}

void RecordDBIfstreamControl::Close()
{
    if (RecordDBCloseFileAndResetBuffers(&filebuf) == 0)
        BasicIos_SetState(RecordDBIosBaseFromStream(this), 2u, 0u);
}

unsigned int RecordDBIfstreamControl::State() const
{
    const unsigned char* const owner = reinterpret_cast<const unsigned char*>(this);
    return reinterpret_cast<const RecordDBIosControl*>(owner+vbtable->virtualBaseOffset)->state;
}

void* __cdecl ProfileUseCtypeFacet(void* localeOwner, void* unused, unsigned char createIfMissing);

void RecordDBIfstreamControl::PrepareLineRead()
{
    unsigned int localeOwner;
    (void)RecordDBCopyAndRetainField20(RecordDBIosBaseFromStream(this), &localeOwner);
    try
    {
        (void)ProfileUseCtypeFacet(&localeOwner, 0, 1u);
    }
    catch (...)
    {
        Msvc6LocaleFacetDecref(&localeOwner);
        throw;
    }
    Msvc6LocaleFacetDecref(&localeOwner);
}

RecordDBLineString::RecordDBLineString()
{
    Tidy(this, 0u);
}

RecordDBLineString::RecordDBLineString(const char* text)
{
    Tidy(this, 0u);
    RecordDBStringAssignBytes(this, text, static_cast<unsigned int>(std::strlen(text)));
}

RecordDBLineString::RecordDBLineString(const char* text, unsigned char allocatorValue)
{
    allocatorState[0] = allocatorValue;
    Tidy(this, 0u);
    RecordDBStringAssignBytes(this, text, static_cast<unsigned int>(std::strlen(text)));
}


RecordDBLineString::RecordDBLineString(const AS_RECORD_STRING_CONTROL& source)
{
    allocatorState[0] = source.allocatorState[0];
    Tidy(this, 0u);
    (void)RecordDBStringAssignSubstring(this,
        const_cast<AS_RECORD_STRING_CONTROL*>(&source), 0u, AS_RECORD_STRING_CONTROL::npos);
}

RecordDBLineString::~RecordDBLineString() noexcept(false)
{
    Tidy(this, 1u);
}

const char* RecordDBLineString::Data() const
{
    return data ? data : reinterpret_cast<const char*>(U32ToPtr(PtrToU32(Msvc6::StringEmptyData<char>())));
}




void*  RecordDBStringAssignSubstring(void* self, void* sourceObject, unsigned int pos, unsigned int count)
{
    AS_RECORD_STRING_CONTROL& storage = *static_cast<AS_RECORD_STRING_CONTROL*>(self);
    const AS_RECORD_STRING_CONTROL& source = *static_cast<const AS_RECORD_STRING_CONTROL*>(sourceObject);
    if (pos>source.size)(void)Msvc6RangeError();
    count = MinUnsigned(count, source.size-pos);
    if (self == sourceObject)
    {
        const unsigned int first = pos+count;
        if (storage.size<first)(void)Msvc6RangeError();
        RecordDBStringSplit(self);
        const unsigned int tail = MinUnsigned(storage.size-first, AS_RECORD_STRING_CONTROL::npos);
        if (tail != 0u)
        {
            const unsigned int data = PtrToU32(storage.data);
            (void)CrtMemmove(data+first, data+first+tail, storage.size-first-tail);
            const unsigned int length = storage.size-tail;
            if (Grow(self, length, 0u))RecordDBStringEos(self, length);
        }
        RecordDBStringSplit(self);
        const unsigned int prefix = MinUnsigned(storage.size, pos);
        if (prefix != 0u)
        {
            const unsigned int data = PtrToU32(storage.data);
            (void)CrtMemmove(data, data+prefix, storage.size-prefix);
            const unsigned int length = storage.size-prefix;
            if (Grow(self, length, 0u))RecordDBStringEos(self, length);
        }
        return self;
    }
    if (count != 0u && count == source.size)
    {
        unsigned int data = PtrToU32(source.data);
        if (data == 0u)data = PtrToU32(Msvc6::StringEmptyData<char>());
        if (ReadGlobalU8(data-1u)<254u)
        {
            Tidy(self, 1u);
            data = PtrToU32(source.data);
            if (data == 0u)data = PtrToU32(Msvc6::StringEmptyData<char>());
            storage.data = reinterpret_cast<char*>(U32ToPtr(data));
            storage.size = source.size;
            storage.capacity = source.capacity;
            WriteGlobalU8(data-1u, static_cast<unsigned char>(ReadGlobalU8(data-1u)+1u));
            return self;
        }
    }
    if (count>static_cast<unsigned int>(Msvc6BasicStringMaxSize()))
        (void)Msvc6LengthError();
    const unsigned int oldData = PtrToU32(storage.data);
    if (StringIsShared(oldData))
    {
        if (count == 0u)
        {
            WriteGlobalU8(oldData-1u, static_cast<unsigned char>(ReadGlobalU8(oldData-1u)-1u));
            Tidy(self, 0u);
            return self;
        }
        StdString_CopyStorage(self, count);
    }
    else if (count == 0u)
    {
        Tidy(self, 1u);
        return self;
    }
    else if (storage.capacity>31u || storage.capacity<count)
    {
        Tidy(self, 1u);
        StdString_CopyStorage(self, count);
    }
    unsigned int data = PtrToU32(source.data);
    if (data == 0u)data = PtrToU32(Msvc6::StringEmptyData<char>());
    CopyAbsoluteBytes(PtrToU32(storage.data), data+pos, count);
    RecordDBStringEos(self, count);
    return self;
}

AS_RECORD& AS_RECORD::operator=(const AS_RECORD& other)
{
    (void)RecordDBStringAssignSubstring(&m_playerName,
        const_cast<AS_RECORD_STRING_CONTROL*>(&other.m_playerName), 0u,
        AS_RECORD_STRING_CONTROL::npos);
    m_totalScore=other.m_totalScore;
    m_monsterKills=other.m_monsterKills;
    m_elapsedSeconds=other.m_elapsedSeconds;
    m_checksum=other.m_checksum;
    (void)RecordDBStringAssignSubstring(&m_recordTimestamp,
        const_cast<AS_RECORD_STRING_CONTROL*>(&other.m_recordTimestamp), 0u,
        AS_RECORD_STRING_CONTROL::npos);
    m_weaponScores = other.m_weaponScores;
    m_weaponKills = other.m_weaponKills;
    m_cheater=other.m_cheater;
    return *this;
}

unsigned int __cdecl RecordDBGetlineDelimited(void* stream, void* outString, unsigned int delimiter)
{
    AS_RECORD_STRING_CONTROL& output = *static_cast<AS_RECORD_STRING_CONTROL*>(outString);
    unsigned int state = 0u;
    bool extracted = false;
    RecordDBStringSplit(outString);
    const unsigned int erased = MinUnsigned(output.size, 0xFFFFFFFFu);
    if (erased != 0u)
    {
        const unsigned int data = PtrToU32(output.data);
        (void)CrtMemmove(data, data+erased, output.size-erased);
        const unsigned int length = output.size-erased;
        if (Grow(outString, length, 0u))RecordDBStringEos(outString, length);
    }
    const auto ios = [stream]()->RecordDBIosControl*
    {
        const Msvc6::VirtualBaseTable* const vbtable = static_cast<RecordDBStreamVirtualBase*>(stream)->vbtable;
        return reinterpret_cast<RecordDBIosControl*>(static_cast<unsigned char*>(stream)+
            vbtable->virtualBaseOffset);
    };
    if (RecordDBBasicIstreamIpfx(stream, 1u) != 0u)
    {
        try
        {
            void* buffer = ios()->buffer;
            unsigned int ch = BasicStreambuf_SGetC(buffer);
            for (;;)
            {
                if (ch == 0xFFFFFFFFu)
                {
                    state|=1u;
                    break;
                }
                if (static_cast<unsigned char>(ch) == static_cast<unsigned char>(delimiter))
                {
                    extracted = true;
                    buffer = ios()->buffer;
                    if (RecordDBBasicStreambufSbumpc(buffer) != 0xFFFFFFFFu)
                        (void)BasicStreambuf_SGetC(buffer);
                    break;
                }
                if (output.size >= 0xFFFFFFFDu)
                {
                    state|=2u;
                    break;
                }
                if (AS_RECORD_STRING_CONTROL::npos-output.size <= 1u)
                    (void)Msvc6LengthError();
                const unsigned int length = output.size+1u;
                if (Grow(outString, length, 0u))
                {
                    output.data[output.size] = static_cast<char>(ch);
                    RecordDBStringEos(outString, length);
                }
                extracted = true;
                buffer = ios()->buffer;
                const unsigned int consumed = RecordDBBasicStreambufSbumpc(buffer);
                ch = consumed == 0xFFFFFFFFu?0xFFFFFFFFu:BasicStreambuf_SGetC(buffer);
            }
        }
        catch (...)
        {
            RecordDBIosControl* const base = ios();
            unsigned int flags = base->state|4u;
            if (base->buffer == 0)flags|=4u;
            Msvc6Runtime::BasicIosClear(base, flags, 1u);
        }
    }
    if (!extracted)state|=2u;
    RecordDBIosControl* const base = ios();
    if (state != 0u)
    {
        unsigned int flags = base->state|state;
        if (base->buffer == 0)flags|=4u;
        Msvc6Runtime::BasicIosClear(base, flags, 0u);
    }
    return PtrToU32(stream);
}


void zRecordDB::AddRecord(int iType, const AS_RECORD& source)
{
    m_DataFlags[iType] = 0;
    (void)LoadIfNeed(iType);

    AS_RECORD record(source);
    {
        STRING timeText = GetTimeString();
        STRING dateText = GetDateString();
        STRING withDash = dateText+" - ";
        STRING timestamp = withDash+timeText;
        const char* const stamp = timestamp.value;
        const unsigned int stampLength = CStringLength(stamp);
        if (Grow(&record.m_recordTimestamp, stampLength, 1u))
        {
            std::memcpy(record.m_recordTimestamp.data, stamp, stampLength);
            RecordDBStringEos(&record.m_recordTimestamp, stampLength);
        }
    }

    record.m_checksum = static_cast<unsigned int>(record.GetHash(2));
    AS_RECORD_VECTOR& slot = m_Data[iType];
    (void)RecordDBVectorRecordInsert(&slot, slot.end, &record);
    m_OutFlags[iType] = 1;
    (void)SaveToFile(iType);

}
static void* RecordDBIosBaseFromOstream(void* stream)
{
    const Msvc6::VirtualBaseTable* const vtable = static_cast<RecordDBStreamVirtualBase*>(stream)->vbtable;
    const int adjust = static_cast<int>(vtable->virtualBaseOffset);
    return static_cast<unsigned char*>(stream)+adjust;
}

static void RecordDBSetOstreamState(void* ios, unsigned int bits, unsigned int rethrow)
{
    unsigned int state = static_cast<RecordDBIosControl*>(ios)->state|bits;
    if (static_cast<RecordDBIosControl*>(ios)->buffer == nullptr) state|=4u;
    Msvc6Runtime::BasicIosClear(ios, state, rethrow);
}

RecordDBOstreamSentry::RecordDBOstreamSentry(void* source)
    : ready(RecordDBBasicOstreamOpfx(source)), stream(source)
{
}

RecordDBOstreamSentry::~RecordDBOstreamSentry() noexcept(false)
{
    sentry_Dtor(this);
}

static int RecordDBOstreamPutFill(RecordDBIosControl* ios)
{
    RecordDBStreambufControl* const buffer = ios->buffer;
    const unsigned char fill = ios->fill;
    char* const cursor = *buffer->putCursorSlot;
    const unsigned int address = PtrToU32(cursor);
    if (cursor && address<address+*buffer->putRemainingSlot)
    {
        --*buffer->putRemainingSlot;
        char* const destination = *buffer->putCursorSlot;
        *buffer->putCursorSlot = destination+1;
        *destination = static_cast<char>(fill);
        return fill;
    }
    return static_cast<int>(reinterpret_cast<Msvc6Runtime::basic_streambuf*>(buffer)->overflow(static_cast<int>(fill)));
}


void* __cdecl N_6std(void* stream, const char* text)
{
    const unsigned int length = CStringLength(text);
    const int width = BitsToSigned32(static_cast<RecordDBIosControl*>(
        RecordDBIosBaseFromOstream(stream))->width);
    unsigned int padding = width>0 && static_cast<unsigned int>(width)>length
        ? static_cast<unsigned int>(width)-length : 0u;
    unsigned int state = 0u;
    RecordDBOstreamSentry sentry(stream);
    if (!sentry.ready)state = 4u;
    else
    {
        try
        {
            if ((static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(stream))->formatFlags&0x1C0u) != 0x40u)
            {
                while (padding != 0u)
                {
                    if (RecordDBOstreamPutFill(static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(stream))) == -1)
                    {
                        state|=4u;
                        break;
                    }
                    --padding;
                }
            }
            if (state == 0u)
            {
                RecordDBIosControl* const ios = static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(stream));
                if (reinterpret_cast<Msvc6Runtime::basic_streambuf*>(ios->buffer)->xsputn(text, static_cast<int>(length)) != length)state = 4u;
                else
                {
                    while (padding != 0u)
                    {
                        if (RecordDBOstreamPutFill(static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(stream))) == -1)
                        {
                            state|=4u;
                            break;
                        }
                        --padding;
                    }
                }
            }
            static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(stream))->width = 0u;
        }
        catch (...)
        {
            RecordDBSetOstreamState(RecordDBIosBaseFromOstream(stream), 4u, 1u);
        }
    }
    if (state != 0u)RecordDBSetOstreamState(RecordDBIosBaseFromOstream(stream), state, 0u);
    return stream;
}

void* __cdecl RecordDBOstreamWriteChar(void* stream, unsigned char value)
{
    unsigned int state = 0u;
    RecordDBOstreamSentry sentry(stream);
    if (sentry.ready)
    {
        const int width = BitsToSigned32(static_cast<RecordDBIosControl*>(
            RecordDBIosBaseFromOstream(stream))->width);
        unsigned int padding = width>1?static_cast<unsigned int>(width-1):0u;
        try
        {
            if ((static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(stream))->formatFlags&0x1C0u) != 0x40u)
            {
                while (state == 0u && padding != 0u)
                {
                    if (RecordDBOstreamPutFill(static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(stream))) == -1)state = 4u;
                    --padding;
                }
            }
            if (state == 0u)
            {
                RecordDBStreambufControl* const buffer = static_cast<RecordDBIosControl*>(
                    RecordDBIosBaseFromOstream(stream))->buffer;
                char* const cursor = *buffer->putCursorSlot;
                const unsigned int address = PtrToU32(cursor);
                int written;
                if (cursor && address<address+*buffer->putRemainingSlot)
                {
                    --*buffer->putRemainingSlot;
                    char* const destination = *buffer->putCursorSlot;
                    *buffer->putCursorSlot = destination+1;
                    *destination = static_cast<char>(value);
                    written = value;
                }
                else
                {
                    written = static_cast<int>(reinterpret_cast<Msvc6Runtime::basic_streambuf*>(buffer)->overflow(value));
                }
                if (written == -1)state = 4u;
            }
            while (state == 0u && padding != 0u)
            {
                if (RecordDBOstreamPutFill(static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(stream))) == -1)state = 4u;
                --padding;
            }
        }
        catch (...)
        {
            RecordDBSetOstreamState(RecordDBIosBaseFromOstream(stream), 4u, 1u);
        }
    }
    static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(stream))->width = 0u;
    if (state != 0u)RecordDBSetOstreamState(RecordDBIosBaseFromOstream(stream), state, 0u);
    return stream;
}

void* __cdecl RecordDBBuildHallOfFamePath(void* outString, int iType)
{
    RecordDBLineString path;
    path.allocatorState[0] = static_cast<unsigned char>(iType);

    {
        STRING basePath = Map->WindowsUserPath();
        const char* const base = basePath.value;
        const unsigned int length = CStringLength(base);
        if (Grow(&path, length, 1u))
        {
            std::memcpy(path.data, base, length);
            RecordDBStringEos(&path, length);
        }
    }

    const char* suffix = 0;
    if (iType == 0)
        suffix = "Saves\\HallOfFame1.txt";
    else if (iType == 1)
        suffix = "Saves\\HallOfFame2.txt";
    else if (iType == 2)
        suffix = "Saves\\HallOfFame3.txt";
    else
        DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\RecordDB\\zRecordDB.cpp", 56u);

    if (suffix)
        (void)RecordDBStringAppendBytes(&path, suffix, CStringLength(suffix));

    AS_RECORD_STRING_CONTROL& output = *static_cast<AS_RECORD_STRING_CONTROL*>(outString);
    output.allocatorState[0] = path.allocatorState[0];
    output.data = 0;
    output.size = 0;
    output.capacity = 0;
    (void)RecordDBStringAssignSubstring(&output, &path, 0u, AS_RECORD_STRING_CONTROL::npos);
    return outString;
}

static void RecordDBReleaseLocaleRef(unsigned int localeOwner)
{
    if (!localeOwner) return;
    void* dead = RecordDBRefCountReleaseAndReturnIfZero(U32ToPtr(localeOwner));
    if (dead) delete static_cast<Msvc6Runtime::locale::facet*>(dead);
}

static const Msvc6::CtypeData* RecordDBAcquireCtypeFacet(void* ios, unsigned int* localeOwnerOut)
{
    RecordDBCopyAndRetainField20(ios, localeOwnerOut);
    const RecordDBLocaleControl locale = {U32ToPtr(*localeOwnerOut)};
    try
    {
        return Msvc6::LocaleFacetUse<Msvc6::CharCtype>::Get(locale);
    }
    catch (...)
    {
        RecordDBReleaseLocaleRef(*localeOwnerOut);
        throw;
    }
}

static void* RecordDBIosBaseFromStream(void* stream)
{
    const Msvc6::VirtualBaseTable* const vft = static_cast<RecordDBStreamVirtualBase*>(stream)->vbtable;
    const int delta = static_cast<int>(vft->virtualBaseOffset);
    return static_cast<unsigned char*>(stream)+delta;
}

unsigned int __cdecl RecordDBBasicIstreamIpfx(void* self, unsigned int noSkip)
{
    void* iosBase = RecordDBIosBaseFromStream(self);

    if (static_cast<RecordDBIosControl*>(iosBase)->state != 0u)
    {
        BasicIos_SetState(iosBase, 2u, 0u);
        return 0u;
    }

    void* tied = static_cast<RecordDBIosControl*>(iosBase)->tiedStream;
    if (tied)
    {
        void* tieIos = RecordDBIosBaseFromStream(tied);
        unsigned int fail = 0u;
        if ((static_cast<unsigned char>(static_cast<RecordDBIosControl*>(tieIos)->state)&6u) == 0u)
        {
            void* tieBuffer = static_cast<RecordDBIosControl*>(tieIos)->buffer;
            if (static_cast<int>(static_cast<Msvc6Runtime::basic_streambuf*>(tieBuffer)->sync()) == -1)
                fail = 4u;
        }
        BasicIos_SetState(RecordDBIosBaseFromStream(tied), fail, 0u);
    }
    iosBase = RecordDBIosBaseFromStream(self);
    if (static_cast<unsigned char>(noSkip) != 0u || (static_cast<RecordDBIosControl*>(iosBase)->formatFlags&1u) == 0u)
    {
        if (static_cast<RecordDBIosControl*>(iosBase)->state == 0u)return 1u;
        BasicIos_SetState(iosBase, 2u, 0u);
        return 0u;
    }

    unsigned int localeOwner;
    const Msvc6::CtypeData* const ctype = RecordDBAcquireCtypeFacet(RecordDBIosBaseFromStream(self), &localeOwner);
    RecordDBReleaseLocaleRef(localeOwner);

    try
    {
        void* streambuf = static_cast<RecordDBIosControl*>(RecordDBIosBaseFromStream(self))->buffer;
        unsigned int character = BasicStreambuf_SGetC(streambuf);
        while (character != 0xFFFFFFFFu)
        {
            const unsigned short mask = ctype->descriptor.table[character & 0xFFu];
            if ((mask&0x48u) == 0u)
                break;
            streambuf = static_cast<RecordDBIosControl*>(RecordDBIosBaseFromStream(self))->buffer;
            character = RecordDBBasicStreambufSbumpc(streambuf);
            if (character == 0xFFFFFFFFu)
                break;
            character = BasicStreambuf_SGetC(streambuf);
        }
    }
    catch (...)
    {
        BasicIos_SetState(RecordDBIosBaseFromStream(self), 4u, 1u);
    }

    iosBase = RecordDBIosBaseFromStream(self);
    if (static_cast<RecordDBIosControl*>(iosBase)->state == 0u)
        return 1u;
    BasicIos_SetState(iosBase, 2u, 0u);
    return 0u;
}

unsigned int  RecordDBBasicFilebufInitCodecvt(void* self)
{
    RecordDBFilebufControl& storage = *static_cast<RecordDBFilebufControl*>(self);
    unsigned int sourceLocale;
    (void)Msvc6BasicStreambufGetloc(self, &sourceLocale);
    const Msvc6::LocaleStampData* facet;
    const RecordDBLocaleControl source = {U32ToPtr(sourceLocale)};
    try
    {
        facet = Msvc6::LocaleFacetUse<Msvc6::CharCodecvt>::Get(source);
    }
    catch (...)
    {
        RecordDBReleaseLocaleRef(sourceLocale);
        throw;
    }
    storage.codecvt = const_cast<Msvc6::LocaleStampData*>(facet);
    RecordDBReleaseLocaleRef(sourceLocale);

    unsigned int argumentOwner = PtrToU32(storage.locale);
    RecordDBRetainLocaleRef(argumentOwner);
    unsigned int resultOwner;
    (void)RecordDBAddCodecvtFacetCore(PtrToU32(&resultOwner), argumentOwner, PtrToU32(storage.codecvt));
    try
    {
        const unsigned int oldOwner = PtrToU32(storage.locale);
        if (resultOwner != oldOwner)
        {
            RecordDBReleaseLocaleRef(oldOwner);
            storage.locale = U32ToPtr(resultOwner);
            RecordDBRetainLocaleRef(resultOwner);
        }
    }
    catch (...)
    {
        RecordDBReleaseLocaleRef(resultOwner);
        throw;
    }
    RecordDBReleaseLocaleRef(resultOwner);
    if (static_cast<Msvc6Runtime::codecvt*>(storage.codecvt)->do_always_noconv() != 0u)
        storage.codecvt = 0;

    if (storage.conversionBuffer == 0)
    {
        AS_RECORD_STRING_CONTROL* const buffer = static_cast<AS_RECORD_STRING_CONTROL*>(CrtNew(16u));
        if (buffer)
            Tidy(buffer, 0u);
        storage.conversionBuffer = buffer;
    }
    return 0u;
}

int AS_RECORD::GetHash(int mode)
{
    RecordDBLineString serialized(m_playerName);
    (void)RecordDBStringAppendFill(&serialized, 1u, ' ');
    (void)RecordDBStringAppendSubstring(
        &serialized, &m_recordTimestamp, 0u, AS_RECORD_STRING_CONTROL::npos);

    const int* const scalarValues[3] = {&m_totalScore, &m_monsterKills, &m_elapsedSeconds};
    for (unsigned int scalar = 0;scalar<3u;++scalar)
    {
        (void)RecordDBStringAppendFill(&serialized, 1u, ' ');
        STRING number = Int2Str(*scalarValues[scalar]);
        const char* const text = number.value;
        (void)RecordDBStringAppendBytes(&serialized, text, CStringLength(text));
    }

    const AS_RECORD_DWORD_VECTOR* vectors[2] = {&m_weaponScores, &m_weaponKills};
    for (unsigned int vectorIndex = 0;vectorIndex<2u;++vectorIndex)
    {
        const AS_RECORD_DWORD_VECTOR& values = *vectors[vectorIndex];
        for (unsigned int i = 0;values.begin &&
            i<static_cast<unsigned int>(values.end-values.begin);++i)
        {
            (void)RecordDBStringAppendFill(&serialized, 1u, ' ');
            STRING number = Int2Str(values.begin[i]);
            const char* const text = number.value;
            (void)RecordDBStringAppendBytes(&serialized, text, CStringLength(text));
        }
    }

    const char* const data = serialized.Data();
    const unsigned int length = serialized.size;
    unsigned int result = 0u;
    if (mode == 2)
        result = static_cast<unsigned int>(zutl::GetHashNb(data));
    else if (mode == 1)
    {
        CRC32 crc;
        crc.value = 0u;
        result = crc.Add(data, length);
    }
    else if (mode == 0)
        result = zutl::GetHashPJW(data);
    else
        DebugAssert(5u, "0", "D:\\zm\\gamedev\\AlienShooter\\code\\src\\zm\\Game\\RecordDB\\zRecordDB.cpp", 302u);

    return static_cast<int>(result);
}

void  RecordDBVectorRecordInsertFill(void* self, AS_RECORD* position,
    unsigned int count, const AS_RECORD& value)
{
    AS_RECORD_VECTOR& storage = *static_cast<AS_RECORD_VECTOR*>(self);
    const unsigned int available = static_cast<unsigned int>(
        BitsToSigned32(PtrToU32(storage.capacityEnd)-PtrToU32(storage.end))/84);
    if (available<count)
    {
        const unsigned int oldSize = storage.begin?static_cast<unsigned int>(
            BitsToSigned32(PtrToU32(storage.end)-PtrToU32(storage.begin))/84):0u;
        const unsigned int newCapacity = oldSize+(oldSize>count?oldSize:count);
        const unsigned int allocationCount = BitsToSigned32(newCapacity)<0?0u:newCapacity;
        AS_RECORD* const allocation = static_cast<AS_RECORD*>(CrtNew(allocationCount*84u));
        AS_RECORD* destination = allocation;
        for (AS_RECORD* source = storage.begin;source != position;++source, ++destination)
            RecordDBConstructRecord(destination, source);
        AS_RECORD* fill = destination;
        for (unsigned int i = 0u;i<count;++i, ++fill)
            RecordDBConstructRecord(fill, const_cast<AS_RECORD*>(&value));
        destination+=count;
        AS_RECORD* const suffixEnd = storage.end;
        for (AS_RECORD* source = position;source != suffixEnd;++source, ++destination)
            RecordDBConstructRecord(destination, source);
        AS_RECORD* const destroyEnd = storage.end;
        for (AS_RECORD* source = storage.begin;source != destroyEnd;++source)
            source->~AS_RECORD();
        CrtFreePointer(storage.begin);
        storage.capacityEnd = allocation+newCapacity;
        const unsigned int retained = storage.begin?static_cast<unsigned int>(
            BitsToSigned32(PtrToU32(storage.end)-PtrToU32(storage.begin))/84):0u;
        storage.begin = allocation;
        storage.end = allocation+(retained+count);
        return;
    }
    const unsigned int tail = static_cast<unsigned int>(
        BitsToSigned32(PtrToU32(storage.end)-PtrToU32(position))/84);
    if (tail<count)
    {
        AS_RECORD* const oldEnd = storage.end;
        AS_RECORD* destination = position+count;
        for (AS_RECORD* source = position;source != oldEnd;++source, ++destination)
            RecordDBConstructRecord(destination, source);
        AS_RECORD* fill = storage.end;
        const unsigned int remaining = count-static_cast<unsigned int>(
            BitsToSigned32(PtrToU32(storage.end)-PtrToU32(position))/84);
        for (unsigned int i = 0u;i<remaining;++i, ++fill)
            RecordDBConstructRecord(fill, const_cast<AS_RECORD*>(&value));
        AS_RECORD* const assignEnd = storage.end;
        for (AS_RECORD* current = position;current != assignEnd;++current)
            *current = value;
        storage.end+=count;
    }
    else if (count != 0u)
    {
        AS_RECORD* const oldEnd = storage.end;
        AS_RECORD* destination = oldEnd;
        for (AS_RECORD* source = oldEnd-count;source != oldEnd;++source, ++destination)
            RecordDBConstructRecord(destination, source);
        AS_RECORD* source = storage.end-count;
        destination = storage.end;
        while (source != position)
        {
            --source;
            --destination;
            *destination = *source;
        }
        for (AS_RECORD* current = position;current != position+count;++current)
            *current = value;
        storage.end+=count;
    }
}

bool zRecordDB::SaveToFile(int iType)
{
    {
        RecordDBLineString path;
        (void)RecordDBBuildHallOfFamePath(&path, iType);
        zutl::MoveToTrash(path.Data());
    }

    RecordDBLineString path;
    (void)RecordDBBuildHallOfFamePath(&path, iType);
    RecordDBOfstreamControl output(path.Data(), 18);
    Tidy(&path, 1u);
    if ((output.State()&6u) != 0u)
        return false;

    (void)N_6std(N_6std(&output, "FileVersion"), "\n");
    (void)N_6std(U32ToPtr(Msvc6BasicOstreamInsertInt(&output, 105)), "\n");

    const AS_RECORD_VECTOR& records = m_Data[iType];
    for (unsigned int recordIndex = 0;records.begin &&
        recordIndex<static_cast<unsigned int>(records.end-records.begin);++recordIndex)
    {
        const AS_RECORD& record = records.begin[recordIndex];
        (void)N_6std(&output, record.m_playerName.data ? record.m_playerName.data : "");
        (void)N_6std(&output, "\n");
        (void)N_6std(&output, record.m_recordTimestamp.data ? record.m_recordTimestamp.data : "");
        (void)N_6std(&output, "\n");
        (void)N_6std(U32ToPtr(Msvc6BasicOstreamInsertInt(&output, record.m_totalScore)), "\n");
        (void)N_6std(U32ToPtr(Msvc6BasicOstreamInsertInt(&output, record.m_monsterKills)), "\n");
        (void)N_6std(U32ToPtr(Msvc6BasicOstreamInsertInt(&output, record.m_elapsedSeconds)), "\n");
        (void)N_6std(U32ToPtr(Msvc6BasicOstreamInsertInt(&output, static_cast<int>(record.m_checksum))), "\n");
        (void)N_6std(U32ToPtr(Msvc6BasicOstreamInsertInt(&output, record.m_cheater)), "\n");

        const AS_RECORD_DWORD_VECTOR* const vectors[2] = {&record.m_weaponScores, &record.m_weaponKills};
        for (unsigned int vectorIndex = 0;vectorIndex<2u;++vectorIndex)
        {
            const AS_RECORD_DWORD_VECTOR& values = *vectors[vectorIndex];
            const unsigned int count = values.begin
                ? static_cast<unsigned int>(values.end-values.begin) : 0u;
            (void)N_6std(U32ToPtr(Msvc6BasicOstreamInsertUnsignedInt(&output, count)), "\n");
            for (unsigned int element = 0;values.begin &&
                element<static_cast<unsigned int>(values.end-values.begin);++element)
            {
                (void)N_6std(U32ToPtr(Msvc6BasicOstreamInsertInt(&output, values.begin[element])), "\n");
            }
        }
    }

    output.Close();
    return true;
}


static const Msvc6::FacetData* RecordDBAcquireNumPutFacetExact(void* ios, unsigned int* localeOwnerOut)
{
    RecordDBCopyAndRetainField20(ios, localeOwnerOut);
    const RecordDBLocaleControl locale = {U32ToPtr(*localeOwnerOut)};
    try
    {
        return Msvc6::LocaleFacetUse<Msvc6::CharNumPut>::Get(locale);
    }
    catch (...)
    {
        RecordDBReleaseLocaleRef(*localeOwnerOut);
        throw;
    }
}

static unsigned int RecordDBBasicOstreamInsertInteger(void* self, unsigned int value, bool isUnsigned)
{
    unsigned int stateBits = 0u;
    RecordDBOstreamSentry sentry(self);
    if (sentry.ready)
    {
        void* ios = RecordDBIosBaseFromOstream(self);
        unsigned int localeOwner = 0u;
        const Msvc6::FacetData* const facet = RecordDBAcquireNumPutFacetExact(ios, &localeOwner);
        RecordDBReleaseLocaleRef(localeOwner);
        try
        {
            ios = RecordDBIosBaseFromOstream(self);
            RecordDBIosControl* const fields = static_cast<RecordDBIosControl*>(ios);
            const char fill = static_cast<char>(fields->fill);
            Msvc6::OutputIterator<char> iterator;
            iterator.failed = 0;
            iterator.stream = fields->buffer;
            Msvc6::OutputIterator<char> result;
            const Msvc6::NumPutVirtualTable<char>* const table =
                static_cast<const Msvc6::NumPutVirtualTable<char>*>(facet->vftable);
            const Msvc6::OutputIterator<char>* const returned = isUnsigned
                ? table->putUnsignedLong(facet, &result, iterator, fields, fill, static_cast<unsigned long>(value))
                : table->putLong(facet, &result, iterator, fields, fill, static_cast<long>(BitsToSigned32(value)));
            if (returned->failed != 0u)stateBits = 4u;
        }
        catch (...)
        {
            RecordDBSetOstreamState(RecordDBIosBaseFromOstream(self), 4u, 1u);
        }
    }
    if (stateBits != 0u)RecordDBSetOstreamState(RecordDBIosBaseFromOstream(self), stateBits, 0u);
    return PtrToU32(self);
}

unsigned int  Msvc6BasicOstreamInsertInt(void* self, int value)
{
    return RecordDBBasicOstreamInsertInteger(self, static_cast<unsigned int>(value), false);
}

unsigned int  Msvc6BasicOstreamInsertUnsignedInt(void* self, unsigned int value)
{
    return RecordDBBasicOstreamInsertInteger(self, value, true);
}

RecordDBOfstreamControl& RecordDBOfstreamControl::operator<<(const char* text)
{
    return *static_cast<RecordDBOfstreamControl*>(N_6std(this, text));
}

RecordDBOfstreamControl& RecordDBOfstreamControl::operator<<(int value)
{
    (void)Msvc6BasicOstreamInsertInt(this, value);
    return *this;
}

RecordDBOfstreamControl& RecordDBOfstreamControl::operator<<(unsigned long value)
{
    (void)Msvc6BasicOstreamInsertUnsignedInt(this, value);
    return *this;
}

RecordDBOfstreamControl& RecordDBOfstreamControl::operator<<(const void* address)
{
    unsigned int stateBits = 0u;
    RecordDBOstreamSentry sentry(this);
    if (sentry.ready)
    {
        void* fields = RecordDBIosBaseFromOstream(this);
        unsigned int localeOwner = 0u;
        const Msvc6::FacetData* const facet = RecordDBAcquireNumPutFacetExact(fields, &localeOwner);
        RecordDBReleaseLocaleRef(localeOwner);
        try
        {
            RecordDBIosControl& current = *static_cast<RecordDBIosControl*>(
                RecordDBIosBaseFromOstream(this));
            Msvc6::OutputIterator<char> iterator;
            iterator.failed = 0;
            iterator.stream = current.buffer;
            Msvc6::OutputIterator<char> result;
            const Msvc6::OutputIterator<char>* const returned =
                static_cast<const Msvc6::NumPutVirtualTable<char>*>(facet->vftable)->putPointer(
                    facet, &result, iterator, &current, static_cast<char>(current.fill), address);
            if (returned->failed != 0u) stateBits = 4u;
        }
        catch (...)
        {
            RecordDBSetOstreamState(RecordDBIosBaseFromOstream(this), 4u, 1u);
        }
    }
    if (stateBits != 0u) RecordDBSetOstreamState(RecordDBIosBaseFromOstream(this), stateBits, 0u);
    return *this;
}

RecordDBOfstreamControl& RecordDBOfstreamControl::put(char value)
{
    unsigned int stateBits = 0u;
    RecordDBOstreamSentry sentry(this);
    if (!sentry.ready) stateBits = 4u;
    else
    {
        try
        {
            RecordDBStreambufControl* const buffer = static_cast<RecordDBIosControl*>(
                RecordDBIosBaseFromOstream(this))->buffer;
            char* const cursor = *buffer->putCursorSlot;
            const unsigned int address = PtrToU32(cursor);
            int written;
            if (cursor && address < address + *buffer->putRemainingSlot)
            {
                --*buffer->putRemainingSlot;
                char* const destination = *buffer->putCursorSlot;
                *buffer->putCursorSlot = destination + 1;
                *destination = value;
                written = static_cast<unsigned char>(value);
            }
            else
            {
                written = reinterpret_cast<Msvc6Runtime::basic_streambuf*>(buffer)->overflow(
                    static_cast<unsigned char>(value));
            }
            if (written == -1) stateBits = 4u;
        }
        catch (...)
        {
            RecordDBSetOstreamState(RecordDBIosBaseFromOstream(this), 4u, 1u);
        }
    }
    if (stateBits != 0u) RecordDBSetOstreamState(RecordDBIosBaseFromOstream(this), stateBits, 0u);
    return *this;
}

RecordDBOfstreamControl& RecordDBOfstreamControl::flush()
{
    RecordDBIosControl* const fields = static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(this));
    if ((fields->state & 6u) == 0u &&
        reinterpret_cast<Msvc6Runtime::basic_streambuf*>(fields->buffer)->sync() == -1)
    {
        RecordDBSetOstreamState(RecordDBIosBaseFromOstream(this), 4u, 0u);
    }
    return *this;
}

unsigned int RecordDBOfstreamControl::setf(unsigned int flags, unsigned int mask)
{
    RecordDBIosControl& fields = *static_cast<RecordDBIosControl*>(RecordDBIosBaseFromOstream(this));
    const unsigned int previous = fields.formatFlags;
    fields.formatFlags = (previous & ~mask) | (flags & mask & 32767u);
    return previous;
}


bool zRecordDB::LoadFromFile(int iType)
{
    RecordDBLineString path;
    (void)RecordDBBuildHallOfFamePath(&path, iType);
    RecordDBIfstreamControl input(path.Data(), 1);
    Tidy(&path, 1u);
    if ((input.State()&6u) != 0u)
        return false;

    AS_RECORD_VECTOR& records = m_Data[iType];
    (void)RecordDBVectorRecordErase(&records, records.begin, records.end);
    RecordDBLineString line;
    AS_RECORD record;
    bool complete = true;
    const auto readLine = [&input, &line]() -> bool
    {
        input.PrepareLineRead();
        (void)RecordDBGetlineDelimited(&input, &line, '\n');
        return (input.State()&6u) == 0u;
    };

    (void)readLine();
    int version = 100;
    if (std::strcmp(line.Data(), "FileVersion") == 0)
    {
        complete = readLine();
        if (complete)
        {
            version = BitsToSigned32(CrtAtoiText(line.Data()));
            complete = readLine();
        }
    }

    while (complete)
    {
        (void)RecordDBStringAssignSubstring(&record.m_playerName, &line, 0u,
            AS_RECORD_STRING_CONTROL::npos);
        complete = readLine();
        if (!complete)
            break;
        (void)RecordDBStringAssignSubstring(&record.m_recordTimestamp, &line, 0u,
            AS_RECORD_STRING_CONTROL::npos);

        int* const scalarFields[3] = {&record.m_totalScore, &record.m_monsterKills, &record.m_elapsedSeconds};
        for (unsigned int scalar = 0;scalar<3u;++scalar)
        {
            complete = readLine();
            if (!complete)
                break;
            *scalarFields[scalar] = BitsToSigned32(CrtAtoiText(line.Data()));
        }
        if (!complete)
            break;
        complete = readLine();
        if (!complete)
            break;
        record.m_checksum = CrtAtoiText(line.Data());

        if (version >= 105)
        {
            complete = readLine();
            if (!complete)
                break;
            record.m_cheater = CrtAtoiText(line.Data()) != 0u;
        }

        const unsigned int storedChecksum = record.m_checksum;
        AS_RECORD_DWORD_VECTOR* const vectors[2] = {&record.m_weaponScores, &record.m_weaponKills};
        for (int vectorIndex = 0;vectorIndex<2;++vectorIndex)
        {
            if (version<101+vectorIndex)
                continue;
            AS_RECORD_DWORD_VECTOR& values = *vectors[vectorIndex];
            (void)RecordDBCopyRangeIntoEnd(&values, PtrToU32(values.begin), PtrToU32(values.end));
            complete = readLine();
            if (!complete)
                break;
            const int count = BitsToSigned32(CrtAtoiText(line.Data()));
            for (int element = 0;element<count;++element)
            {
                complete = readLine();
                if (!complete)
                    break;
                const int value = BitsToSigned32(CrtAtoiText(line.Data()));
                (void)values.insert(values.end, value);
            }
            if (!complete)
                break;
        }
        if (!complete)
            break;

        bool hashMatches = storedChecksum == static_cast<unsigned int>(-666);
        if (!hashMatches)
            hashMatches = storedChecksum == static_cast<unsigned int>(record.GetHash(2));
        if (!hashMatches && version<103)
        {
            hashMatches = storedChecksum == static_cast<unsigned int>(record.GetHash(0));
            if (hashMatches)
                record.m_checksum = static_cast<unsigned int>(record.GetHash(2));
        }
        if (!hashMatches && version<104)
        {
            hashMatches = storedChecksum == static_cast<unsigned int>(record.GetHash(1));
            if (hashMatches)
                record.m_checksum = static_cast<unsigned int>(record.GetHash(2));
        }
        if (hashMatches)
            (void)RecordDBVectorRecordInsert(&records, records.end, &record);
        if (!readLine())
            break;
    }
    if (complete)
        input.Close();

    return complete;
}


static int RecordDBBasicStreambufOverflow(void*, int)
{
    return -1;
}

static int RecordDBBasicFilebufSync(void* self)
{
    const unsigned int file = PtrToU32(static_cast<RecordDBFilebufControl*>(self)->file);
    if (file != 0u && BitsToSigned32(CrtFileFlush(file))<0)
        return -1;
    return 0;
}

static void* RecordDBBasicStreambufScalarDeletingDtor(void* self, unsigned int flags)
{
    basic_streambuf_Dtor(self);
    if (flags&1u)CrtFreePointer(self);
    return self;
}

static void* RecordDBBasicFilebufScalarDeletingDtor(void* self, unsigned int flags)
{
    basic_filebuf_Dtor(self);
    if (flags&1u)CrtFreePointer(self);
    return self;
}

static void RecordDBLocaleImplDtor(void* self)
{
    RecordDBLocaleImplControl& impl = *static_cast<RecordDBLocaleImplControl*>(self);
    impl.vftable = Msvc6::LocaleImplTable();
    unsigned int guard;
    Msvc6Runtime::StaticLockConstruct(&guard);
    try
    {
        unsigned int remaining = impl.facetCount;
        while (remaining != 0u)
        {
            void* const value = impl.facets[--remaining];
            if (value)
            {
                void* const dead = RecordDBRefCountReleaseAndReturnIfZero(value);
                if (dead)delete static_cast<Msvc6Runtime::locale::facet*>(dead);
            }
        }
        CrtFreePointer(impl.facets);
    }
    catch (...)
    {
        Msvc6Runtime::StaticLockDestruct(&guard);
        Tidy(&impl.name, 1u);
        impl.vftable = Msvc6::BaseFacetTable();
        throw;
    }
    Msvc6Runtime::StaticLockDestruct(&guard);
    Tidy(&impl.name, 1u);
    impl.vftable = Msvc6::BaseFacetTable();
}

static void* RecordDBLocaleImplScalarDeletingDtor(void* self, unsigned int flags)
{
    RecordDBLocaleImplDtor(self);
    if (flags&1u)CrtFreePointer(self);
    return self;
}

void* RecordDBStreambufControl::ScalarDeletingDestructor(unsigned int flags)
{
    return RecordDBBasicStreambufScalarDeletingDtor(this, flags);
}

int RecordDBStreambufControl::Overflow(int value)
{
    return RecordDBBasicStreambufOverflow(this, value);
}

int RecordDBStreambufControl::Underflow()
{
    return RecordDBBasicStreambufUnderflow();
}

int RecordDBStreambufControl::Uflow()
{
    return RecordDBBasicStreambufUflow(this);
}

int RecordDBStreambufControl::Read(char* destination, int count)
{
    return RecordDBBasicStreambufXsgetn(this, destination, count);
}

int RecordDBStreambufControl::Write(const char* source, int count)
{
    return RecordDBBasicStreambufXsputn(this, source, count);
}

int RecordDBStreambufControl::SyncDefault()
{
    return 0;
}

void* RecordDBFilebufControl::ScalarDeletingDestructor(unsigned int flags)
{
    return RecordDBBasicFilebufScalarDeletingDtor(this, flags);
}

int RecordDBFilebufControl::Overflow(int value)
{
    return RecordDBBasicFilebufOverflow(this, value);
}

int RecordDBFilebufControl::Pbackfail(int value)
{
    return RecordDBBasicFilebufPbackfail(this, value);
}

unsigned int RecordDBFilebufControl::Underflow()
{
    return RecordDBBasicFilebufUnderflow(this);
}

int RecordDBFilebufControl::Uflow()
{
    return RecordDBBasicFilebufUflow(this);
}

int RecordDBFilebufControl::Sync()
{
    return RecordDBBasicFilebufSync(this);
}

void* MsvcLocaleFacetRef::DeleteFacet(unsigned int flags)
{
    return RecordDBLocaleFacetScalarDeletingDtor(this, flags);
}

void* RecordDBLocaleImplControl::ScalarDeletingDestructor(unsigned int flags)
{
    return RecordDBLocaleImplScalarDeletingDtor(this, flags);
}

void* RecordDBNumpunctControl::ScalarDeletingDestructor(unsigned int flags)
{
    return RecordDBNumpunctScalarDeletingDtor(this, flags);
}

unsigned char RecordDBNumpunctControl::DecimalPoint() const
{
    return NumpunctChar_DoDecimalPoint(this);
}

unsigned char RecordDBNumpunctControl::ThousandsSeparator() const
{
    return NumpunctChar_DoThousandsSep(this);
}

void* RecordDBNumpunctControl::Grouping(void* out)
{
    return RecordDBNumpunctDoGrouping(this, out);
}

void* RecordDBNumpunctControl::FalseName(void* out)
{
    return RecordDBNumpunctDoFalseName(this, out);
}

void* RecordDBNumpunctControl::TrueName(void* out)
{
    return RecordDBNumpunctDoTrueName(this, out);
}

void* MsvcLocaleFacetRef::DeleteNumPut(unsigned int flags)
{
    return Msvc6NumPutScalarDeletingDestructor(this, flags);
}



void* RecordDBCtypeControl::ScalarDeletingDestructor(unsigned int flags)
{
    return RecordDBCtypeScalarDeletingDtor(this, flags);
}


void* RecordDBStreambufControl::Setbuf(char* buffer, int count)
{
    return RecordDBBasicStreambufSetbuf(this, PtrToU32(buffer), static_cast<unsigned int>(count));
}

void* RecordDBFilebufControl::Setbuf(char* buffer, int count)
{
    return RecordDBBasicFilebufSetbuf(this, PtrToU32(buffer), static_cast<unsigned int>(count));
}

void* RecordDBFilebufControl::Seekoff(void* out, int offset, int direction, unsigned int mode)
{
    (void)mode;
    const unsigned int handle = PtrToU32(file);
    long long position;
    if (handle == 0u ||
       CrtFileSeek(handle, static_cast<unsigned int>(offset), static_cast<unsigned int>(direction)) != 0u ||
       Msvc6GetPos(PtrToU32(file), PtrToU32(&position)) != 0u)
    {
        (void)RecordDBInitCursorState(out, 0xFFFFFFFFu);
        return out;
    }
    const unsigned int state = conversionState40;
    RecordDBPositionControl& result = *static_cast<RecordDBPositionControl*>(out);
    result.position = position;
    result.state = BitsToSigned32(state);
    result.offset = 0;
    return out;
}

void* RecordDBFilebufControl::Seekpos(void* out, RecordDBPositionControl requested,
    unsigned int mode)
{
    (void)mode;
    long long position = requested.position;
    const int offset = requested.offset;
    const unsigned int state = static_cast<unsigned int>(requested.state);
    const unsigned int handle = PtrToU32(file);
    if (handle == 0u || Msvc6SetPos(handle, PtrToU32(&position)) != 0u ||
       (offset != 0 && CrtFileSeek(PtrToU32(file), static_cast<unsigned int>(offset), 1u) != 0u) ||
       Msvc6GetPos(PtrToU32(file), PtrToU32(&position)) != 0u)
    {
        (void)RecordDBInitCursorState(out, 0xFFFFFFFFu);
        return out;
    }

    AS_RECORD_STRING_CONTROL* const buffer = conversionBuffer;
    if (buffer)
    {
        conversionState40 = state;
        unsigned int payload = PtrToU32(buffer->data);
        const unsigned int maximum = AS_RECORD_STRING_CONTROL::npos;
        if (payload != 0u)
        {
            const unsigned char references = ReadGlobalU8(payload-1u);
            if (references != 0u && references != 0xFFu)
            {
                const unsigned int previousPayload = payload;
                Tidy(buffer, 1u);
                unsigned int length = 0u;
                while (ReadGlobalU8(previousPayload+length))++length;
                if (static_cast<unsigned char>(Grow(buffer, length, 1u)) != 0u)
                {
                    payload = PtrToU32(buffer->data);
                    CopyBytes(U32ToPtr(payload), U32ToPtr(previousPayload), length);
                    RecordDBStringEos(buffer, length);
                }
            }
        }
        const unsigned int size = buffer->size;
        const unsigned int trim = MinUnsigned(size, maximum);
        if (trim != 0u)
        {
            payload = PtrToU32(buffer->data);
            (void)CrtMemmove(payload, payload+trim, size-trim);
            const unsigned int remaining = buffer->size-trim;
            if (static_cast<unsigned char>(Grow(buffer, remaining, 0u)) != 0u)
            {
                buffer->size = remaining;
                buffer->data[remaining] = 0;
            }
        }
    }

    const unsigned int resultingState = conversionState40;
    RecordDBPositionControl& result = *static_cast<RecordDBPositionControl*>(out);
    result.state = BitsToSigned32(resultingState);
    result.position = position;
    result.offset = 0;
    return out;
}

unsigned char* RecordDBCtypeControl::ToLowerRange(unsigned char* begin, unsigned char* end)
{
    return CTypeChar_DoToLowerRange(this, begin, end);
}

unsigned int RecordDBCtypeControl::ToLower(unsigned int value)
{
    return Msvc6ToLower(value&0xFFu, PtrToU32(&descriptor));
}

unsigned char* RecordDBCtypeControl::ToUpperRange(unsigned char* begin, unsigned char* end)
{
    return do_toupper_2(this, begin, end);
}

unsigned int RecordDBCtypeControl::ToUpper(unsigned int value)
{
    return do_toupper_1(this, value);
}


int RecordDBInitializeVirtualOwners()
{
#if defined(_WIN32)
    Msvc6::InitializeLocaleVirtualTables();
    if (!RecordDBInitializeLocaleMakers()) return 0;
#endif
    return 1;
}




namespace Msvc6
{
namespace
{
    template<class Character> struct CharacterConversion;
    template<> struct CharacterConversion<char>
    {
        static char Widen(char value) { return value; }
        static char Narrow(char value) { return value; }
        static bool IsEnd(int value) { return value == -1; }
        static int UnsignedValue(char value) { return static_cast<unsigned char>(value); }
        static int CollationValue(char value) { return static_cast<signed char>(value); }
    };
    template<> struct CharacterConversion<unsigned short>
    {
        static unsigned short Widen(char value)
        {
            return static_cast<unsigned short>(Msvc6ByteToWide(
                static_cast<unsigned int>(static_cast<signed char>(value))));
        }
        static char Narrow(unsigned short value) { return static_cast<char>(WideToByte(value)); }
        static bool IsEnd(int value) { return static_cast<unsigned short>(value) == 65535u; }
        static int UnsignedValue(unsigned short value) { return value; }
        static int CollationValue(unsigned short value) { return value; }
    };

    template<class Character> struct FacetSelection;
    template<> struct FacetSelection<char>
    {
        typedef CharNumpunct Numpunct;
        typedef CharCtype Ctype;
        typedef CharMoneypunct Moneypunct;
        typedef CharMoneypunctIntl MoneypunctIntl;
    };
    template<> struct FacetSelection<unsigned short>
    {
        typedef WideNumpunct Numpunct;
        typedef WideCtype Ctype;
        typedef WideMoneypunct Moneypunct;
        typedef WideMoneypunctIntl MoneypunctIntl;
    };

    class RetainedLocale
    {
        bool active;
    public:
        RecordDBLocaleControl value;
        explicit RetainedLocale(RecordDBIosControl& ios) : active(false)
        {
            unsigned int owner;
            RecordDBCopyAndRetainField20(&ios, &owner);
            value.implementation = U32ToPtr(owner);
            active = true;
        }
        void Release()
        {
            if (active)
            {
                active = false;
                RecordDBReleaseLocaleRef(PtrToU32(value.implementation));
            }
        }
        ~RetainedLocale() noexcept(false) { Release(); }
        RetainedLocale(const RetainedLocale&) = delete;
        RetainedLocale& operator=(const RetainedLocale&) = delete;
    };

    template<class Character> class TemporaryString
    {
    public:
        StringData<Character> value;
        explicit TemporaryString(unsigned char allocator = 0)
        {
            value.allocatorState[0] = allocator;
            StringOperations<Character>::Tidy(&value, false);
        }
        ~TemporaryString() noexcept(false) { StringOperations<Character>::Tidy(&value, true); }
        TemporaryString(const TemporaryString&) = delete;
        TemporaryString& operator=(const TemporaryString&) = delete;
        const Character* Data() const
        {
            return value.data ? value.data : StringEmptyData<Character>();
        }
        void Append(unsigned int count, Character character)
        {
            StringOperations<Character>::Append(&value, count, character);
        }
    };

    template<class Character> Character* StringElement(StringData<Character>& value, unsigned int index)
    {
        if (value.data != 0 && index <= value.size)
        {
            StringOperations<Character>::Freeze(&value);
            return value.data + index;
        }
        return StringEmptyData<Character>();
    }

    template<class Character> int StreamCharacter(StreambufData<Character>* stream, bool advance)
    {
        Character* const cursor = *stream->getCursorSlot;
        const unsigned int address = PtrToU32(cursor);
        if (cursor && address < address + *stream->getRemainingSlot * sizeof(Character))
        {
            if (advance)
            {
                --*stream->getRemainingSlot;
                *stream->getCursorSlot = cursor + 1;
            }
            return CharacterConversion<Character>::UnsignedValue(*cursor);
        }
        return advance ? stream->vftable->uflow(stream) : stream->vftable->underflow(stream);
    }

    template<class Character> Character ReadIterator(InputIterator<Character>& iterator)
    {
        if (!iterator.cached)
        {
            if (iterator.stream)
            {
                const int character = StreamCharacter(iterator.stream, false);
                if (!CharacterConversion<Character>::IsEnd(character))
                    iterator.value = static_cast<Character>(character);
                else
                    iterator.stream = 0;
            }
            iterator.cached = 1;
        }
        return iterator.value;
    }

    template<class Character> void AdvanceIterator(InputIterator<Character>& iterator)
    {
        if (iterator.stream && !CharacterConversion<Character>::IsEnd(
                StreamCharacter(iterator.stream, true)))
            iterator.cached = 0;
        else
        {
            iterator.stream = 0;
            iterator.cached = 1;
        }
    }

    template<class Character> bool EqualIterator(InputIterator<Character>& first,
        InputIterator<Character>& last)
    {
        if (!first.cached) ReadIterator(first);
        if (!last.cached) ReadIterator(last);
        return (first.stream == 0) == (last.stream == 0);
    }

    template<class Character> InputIterator<Character>* CopyIterator(
        InputIterator<Character>* destination, const InputIterator<Character>& source)
    {
        std::memcpy(destination, &source, sizeof(source));
        return destination;
    }

    template<class Character> void PutIterator(OutputIterator<Character>& iterator, Character character)
    {
        int result = -1;
        if (iterator.stream)
        {
            StreambufData<Character>* const stream = iterator.stream;
            Character* const cursor = *stream->putCursorSlot;
            const unsigned int address = PtrToU32(cursor);
            if (cursor && address < address + *stream->putRemainingSlot * sizeof(Character))
            {
                --*stream->putRemainingSlot;
                *stream->putCursorSlot = cursor + 1;
                *cursor = character;
                result = CharacterConversion<Character>::UnsignedValue(character);
            }
            else
                result = stream->vftable->overflow(stream,
                    CharacterConversion<Character>::UnsignedValue(character));
        }
        if (CharacterConversion<Character>::IsEnd(result)) iterator.failed = 1;
    }

    template<class Character> OutputIterator<Character>* CopyIterator(
        OutputIterator<Character>* destination, const OutputIterator<Character>& source)
    {
        std::memcpy(destination, &source, sizeof(source));
        return destination;
    }

    template<class Character> void PutRange(OutputIterator<Character>& iterator,
        const Character* first, unsigned int count)
    {
        for (unsigned int i = 0; i != count; ++i) PutIterator(iterator, first[i]);
    }

    template<class Character> void PutFill(OutputIterator<Character>& iterator,
        Character character, unsigned int count)
    {
        while (count != 0)
        {
            PutIterator(iterator, character);
            --count;
        }
    }

    template<class Character> const NumpunctData<Character>* UseNumpunct(const RecordDBLocaleControl& locale)
    {
        return LocaleFacetUse<typename FacetSelection<Character>::Numpunct>::Get(locale);
    }
    template<class Character> const NumpunctVirtualTable<Character>* NumpunctTableOf(
        const NumpunctData<Character>* facet)
    {
        return static_cast<const NumpunctVirtualTable<Character>*>(facet->vftable);
    }

    bool IsDecimalDigit(char character, const RecordDBLocaleControl& locale)
    {
        const CtypeData* const facet = LocaleFacetUse<CharCtype>::Get(locale);
        return (facet->descriptor.table[static_cast<unsigned char>(character)] & 4u) != 0;
    }

    template<class Character> void ConsumeSign(char*& output,
        InputIterator<Character>& first, InputIterator<Character>& last)
    {
        if (!EqualIterator(first, last))
        {
            if (ReadIterator(first) == CharacterConversion<Character>::Widen('+'))
                *output++ = '+';
            else if (ReadIterator(first) == CharacterConversion<Character>::Widen('-'))
                *output++ = '-';
            else
                return;
            AdvanceIterator(first);
        }
    }

    template<class Character> int ReadIntegerField(char* output,
        InputIterator<Character>& first, InputIterator<Character>& last,
        unsigned int flags, const RecordDBLocaleControl& locale)
    {
        const Character zero = CharacterConversion<Character>::Widen('0');
        const NumpunctData<Character>* const facet = UseNumpunct<Character>(locale);
        TemporaryString<char> grouping;
        NumpunctTableOf(facet)->grouping(facet, &grouping.value);
        const Character separator = NumpunctTableOf(facet)->thousandsSeparator(facet);
        char* cursor = output;
        ConsumeSign(cursor, first, last);
        const unsigned int baseFlags = flags & 0xE00u;
        int base = baseFlags == 0x400u ? 8 : baseFlags == 0x800u ? 16 : baseFlags ? 10 : 0;
        bool hasDigit = false;
        bool hasSignificantDigit = false;
        if (!EqualIterator(first, last) && ReadIterator(first) == zero)
        {
            hasDigit = true;
            AdvanceIterator(first);
            if (!EqualIterator(first, last)
                && (ReadIterator(first) == CharacterConversion<Character>::Widen('x')
                    || ReadIterator(first) == CharacterConversion<Character>::Widen('X'))
                && (base == 0 || base == 16))
            {
                hasDigit = false;
                base = 16;
                AdvanceIterator(first);
            }
            else if (base == 0)
                base = 8;
        }
        const unsigned int digitCount = base == 8 ? 8 : (base == 0 || base == 10) ? 10 : 22;
        static const char digits[] = "0123456789abcdefABCDEF";
        TemporaryString<char> groups;
        groups.Append(1u, hasDigit ? 1 : 0);
        unsigned int group = 0;
        if (!EqualIterator(first, last))
        {
            do
            {
                *cursor = CharacterConversion<Character>::Narrow(ReadIterator(first));
                if (std::memchr(digits, *cursor, digitCount))
                {
                    if ((hasSignificantDigit || *cursor != '0') && cursor < output + 31)
                    {
                        ++cursor;
                        hasSignificantDigit = true;
                    }
                    hasDigit = true;
                    if (*StringElement<char>(groups.value, group) != 127)
                        ++*StringElement<char>(groups.value, group);
                }
                else
                {
                    if (*StringElement<char>(groups.value, group) == 0 || separator == 0
                        || ReadIterator(first) != separator)
                        break;
                    groups.Append(1u, 0);
                    ++group;
                }
                AdvanceIterator(first);
            } while (!EqualIterator(first, last));
            if (group != 0)
            {
                if (static_cast<signed char>(*StringElement<char>(groups.value, group)) > 0)
                    ++group;
                else
                    hasDigit = false;
            }
        }
        const char* pattern = grouping.Data();
        // VC6's grouping loop decrements on both sides of the comparison.
        // Keep its unsigned counter and terminal-group rules, including that step.
        while (hasDigit && group > 0 && *pattern != 127)
        {
            --group;
            if ((group != 0 && *pattern != *StringElement<char>(groups.value, group))
                || (group == 0 && static_cast<signed char>(*pattern)
                    < static_cast<signed char>(*StringElement<char>(groups.value, group))))
                hasDigit = false;
            else if (static_cast<signed char>(pattern[1]) > 0)
                ++pattern;
            --group;
        }
        if (!hasDigit) cursor = output;
        else if (!hasSignificantDigit) *cursor++ = '0';
        *cursor = '\0';
        return base;
    }

    template<class Character> int ReadFloatingField(char* output,
        InputIterator<Character>& first, InputIterator<Character>& last,
        const RecordDBLocaleControl& locale)
    {
        const Character zero = CharacterConversion<Character>::Widen('0');
        const NumpunctData<Character>* const facet = UseNumpunct<Character>(locale);
        char* cursor = output;
        ConsumeSign(cursor, first, last);
        bool hasDigit = false;
        while (!EqualIterator(first, last) && ReadIterator(first) == zero)
        {
            hasDigit = true;
            AdvanceIterator(first);
        }
        if (hasDigit) *cursor++ = '0';
        int significantDigits = 0;
        int decimalExponent = 0;
        while (!EqualIterator(first, last))
        {
            *cursor = CharacterConversion<Character>::Narrow(ReadIterator(first));
            if (!IsDecimalDigit(*cursor, locale)) break;
            if (significantDigits < 36) { ++cursor; ++significantDigits; }
            else ++decimalExponent;
            hasDigit = true;
            AdvanceIterator(first);
        }
        if (!EqualIterator(first, last)
            && ReadIterator(first) == NumpunctTableOf(facet)->decimalPoint(facet))
        {
            *cursor++ = GetLocaleConvention()->decimalPoint[0];
            AdvanceIterator(first);
        }
        if (significantDigits == 0)
        {
            while (!EqualIterator(first, last) && ReadIterator(first) == zero)
            {
                --decimalExponent;
                hasDigit = true;
                AdvanceIterator(first);
            }
            if (decimalExponent < 0) { *cursor++ = '0'; ++decimalExponent; }
        }
        while (!EqualIterator(first, last))
        {
            *cursor = CharacterConversion<Character>::Narrow(ReadIterator(first));
            if (!IsDecimalDigit(*cursor, locale)) break;
            if (significantDigits < 36) { ++cursor; ++significantDigits; }
            hasDigit = true;
            AdvanceIterator(first);
        }
        if (hasDigit && !EqualIterator(first, last)
            && (ReadIterator(first) == CharacterConversion<Character>::Widen('e')
                || ReadIterator(first) == CharacterConversion<Character>::Widen('E')))
        {
            *cursor++ = 'e';
            AdvanceIterator(first);
            hasDigit = false;
            ConsumeSign(cursor, first, last);
            while (!EqualIterator(first, last) && ReadIterator(first) == zero)
            {
                hasDigit = true;
                AdvanceIterator(first);
            }
            if (hasDigit) *cursor++ = '0';
            int exponentDigits = 0;
            while (!EqualIterator(first, last))
            {
                *cursor = CharacterConversion<Character>::Narrow(ReadIterator(first));
                if (!IsDecimalDigit(*cursor, locale)) break;
                if (exponentDigits < 8) { ++cursor; ++exponentDigits; }
                hasDigit = true;
                AdvanceIterator(first);
            }
        }
        if (!hasDigit) cursor = output;
        *cursor = '\0';
        return decimalExponent;
    }

    template<class Character> int MatchLocaleText(InputIterator<Character>& first,
        InputIterator<Character>& last, unsigned int count, const Character* names)
    {
        const Character delimiter = names[0];
        for (const Character* current = names; *current != 0; ++current)
            if (*current == delimiter) ++count;
        TemporaryString<char> matched;
        matched.Append(count, 0);
        int result = -2;
        unsigned int position = 1;
        for (;;)
        {
            bool anyMatch = false;
            unsigned int offset = 0;
            for (unsigned int index = 0; index < count; ++index)
            {
                while (names[offset] != 0 && names[offset] != delimiter) ++offset;
                const signed char previous = static_cast<signed char>(*StringElement<char>(matched.value, index));
                if (previous != 0)
                    offset += previous;
                else
                {
                    offset += position;
                    if (names[offset] == 0 || names[offset] == delimiter)
                    {
                        *StringElement<char>(matched.value, index) = static_cast<char>(position < 127u ? position : 127u);
                        result = static_cast<int>(index);
                    }
                    else if (!EqualIterator(first, last) && ReadIterator(first) == names[offset])
                        anyMatch = true;
                    else
                        *StringElement<char>(matched.value, index) = static_cast<char>(position < 127u ? position : 127u);
                }
            }
            if (!anyMatch || EqualIterator(first, last)) break;
            ++position;
            AdvanceIterator(first);
            result = -1;
        }
        return result;
    }

    template<class Character, class Value> InputIterator<Character>* GetFloating(
        InputIterator<Character>* output, InputIterator<Character> first,
        InputIterator<Character> last, RecordDBIosControl& ios, unsigned int& state, Value& value)
    {
        errno = 0;
        char field[84];
        char* end;
        Value converted;
        {
            RetainedLocale locale(ios);
            const int exponent = ReadFloatingField(field, first, last, locale.value);
            converted = static_cast<Value>(ParseFloating(field, &end, exponent));
        }
        if (EqualIterator(first, last)) state |= 1u;
        if (end == field || errno != 0) state |= 2u;
        else value = converted;
        return CopyIterator(output, first);
    }

    template<class Character, class Value> InputIterator<Character>* GetSmallUnsigned(
        InputIterator<Character>* output, InputIterator<Character> first,
        InputIterator<Character> last, RecordDBIosControl& ios, unsigned int& state,
        Value& value, unsigned long maximum)
    {
        errno = 0;
        char field[32];
        int base;
        {
            RetainedLocale locale(ios);
            base = ReadIntegerField(field, first, last, ios.formatFlags, locale.value);
        }
        char* end;
        const char* start = field + (field[0] == '-' ? 1 : 0);
        unsigned long converted = std::strtoul(start, &end, base);
        if (EqualIterator(first, last)) state |= 1u;
        if (end == start || errno != 0 || converted > maximum) state |= 2u;
        else
        {
            if (field[0] == '-') converted = 0u - converted;
            value = static_cast<Value>(converted);
        }
        return CopyIterator(output, first);
    }

    template<class Character> unsigned int ReadRangedInteger(InputIterator<Character>& first,
        InputIterator<Character>& last, int minimum, int maximum, int& value)
    {
        char field[32];
        char* cursor = field;
        ConsumeSign(cursor, first, last);
        bool hasDigit = false;
        const Character zero = CharacterConversion<Character>::Widen('0');
        while (!EqualIterator(first, last) && ReadIterator(first) == zero)
        {
            hasDigit = true;
            AdvanceIterator(first);
        }
        if (hasDigit) *cursor++ = '0';
        while (!EqualIterator(first, last))
        {
            const char digit = CharacterConversion<Character>::Narrow(ReadIterator(first));
            if (digit < '0' || digit > '9') break;
            if (cursor < field + 31) *cursor++ = digit;
            hasDigit = true;
            AdvanceIterator(first);
        }
        if (!hasDigit) cursor = field;
        *cursor = '\0';
        errno = 0;
        char* end;
        const long converted = std::strtol(field, &end, 10);
        unsigned int state = EqualIterator(first, last) ? 1u : 0u;
        if (end == field || errno != 0 || converted < minimum || converted > maximum)
            state |= 2u;
        else value = static_cast<int>(converted);
        return state;
    }
}

const unsigned short* WideCtypeOperations::ClassifyRange(const unsigned short* first,
    const unsigned short* last, unsigned short* masks) const
{
    for (; first != last; ++first, ++masks)
    {
        const char value = static_cast<const WideCtypeVirtualTable*>(vftable)->narrow(this, *first, 0);
        *masks = descriptor.table[static_cast<signed char>(value)];
    }
    return last;
}
bool WideCtypeOperations::Classify(unsigned short mask, unsigned short value) const
{
    const char byte = static_cast<const WideCtypeVirtualTable*>(vftable)->narrow(this, value, 0);
    return (mask & descriptor.table[static_cast<signed char>(byte)]) != 0;
}
const unsigned short* WideCtypeOperations::ScanIs(unsigned short mask,
    const unsigned short* first, const unsigned short* last) const
{
    for (; first != last; ++first)
        if (static_cast<const WideCtypeVirtualTable*>(vftable)->classify(this, mask, *first)) break;
    return first;
}
const unsigned short* WideCtypeOperations::ScanNot(unsigned short mask,
    const unsigned short* first, const unsigned short* last) const
{
    for (; first != last; ++first)
        if (!static_cast<const WideCtypeVirtualTable*>(vftable)->classify(this, mask, *first)) break;
    return first;
}
unsigned short WideCtypeOperations::ToLower(unsigned short value) const
{
    const char byte = static_cast<const WideCtypeVirtualTable*>(vftable)->narrow(this, value, 0);
    const char lower = static_cast<char>(Msvc6ToLower(
        static_cast<unsigned int>(static_cast<signed char>(byte)), PtrToU32(&descriptor)));
    return static_cast<const WideCtypeVirtualTable*>(vftable)->widen(this, lower);
}
const unsigned short* WideCtypeOperations::ToLowerRange(unsigned short* first,
    const unsigned short* last) const
{
    for (; first != last; ++first)
        *first = static_cast<unsigned short>(Msvc6ToLower(*first, PtrToU32(&descriptor)));
    return last;
}
unsigned short WideCtypeOperations::ToUpper(unsigned short value) const
{
    const char byte = static_cast<const WideCtypeVirtualTable*>(vftable)->narrow(this, value, 0);
    const char upper = static_cast<char>(Msvc6ToUpper(
        static_cast<unsigned int>(static_cast<signed char>(byte)), PtrToU32(&descriptor)));
    return static_cast<const WideCtypeVirtualTable*>(vftable)->widen(this, upper);
}
const unsigned short* WideCtypeOperations::ToUpperRange(unsigned short* first,
    const unsigned short* last) const
{
    for (; first != last; ++first)
        *first = static_cast<unsigned short>(Msvc6ToUpper(*first, PtrToU32(&descriptor)));
    return last;
}
unsigned short WideCtypeOperations::Widen(char value) const
{
    return CharacterConversion<unsigned short>::Widen(value);
}
const char* WideCtypeOperations::WidenRange(const char* first, const char* last,
    unsigned short* destination) const
{
    for (; first != last; ++first, ++destination)
        *destination = CharacterConversion<unsigned short>::Widen(*first);
    return last;
}
char WideCtypeOperations::Narrow(unsigned short value, char) const
{
    return static_cast<char>(WideToByte(value));
}
const unsigned short* WideCtypeOperations::NarrowRange(const unsigned short* first,
    const unsigned short* last, char, char* destination) const
{
    for (; first != last; ++first, ++destination)
        *destination = static_cast<char>(WideToByte(*first));
    return last;
}
bool WideCodecvtOperations::AlwaysNoConversion() const { return false; }
int WideCodecvtOperations::MaximumLength() const { return 2; }
int WideCodecvtOperations::Encoding() const { return 0; }
int WideCodecvtOperations::In(unsigned int& state, const char* first, const char* last,
    const char*& next, unsigned short* destination, unsigned short* limit,
    unsigned short*& destinationNext) const
{
    next = first;
    destinationNext = destination;
    int result = first == last ? 0 : 1;
    while (next != last && destinationNext != limit)
    {
        int count = Msvc6MbrToWide(destinationNext, next, static_cast<unsigned int>(last - next),
            &state, reinterpret_cast<const RecordDBCtypeDescriptor*>(&localeLow));
        if (count == -2) { next = last; break; }
        if (count == -1) return 2;
        if (count == 0) count = static_cast<int>(std::strlen(next)) + 1;
        next += count;
        ++destinationNext;
        result = 0;
    }
    return result;
}
int WideCodecvtOperations::Out(unsigned int& state, const unsigned short* first,
    const unsigned short* last, const unsigned short*& next, char* destination,
    char* limit, char*& destinationNext) const
{
    next = first;
    destinationNext = destination;
    int result = first == last ? 0 : 1;
    while (next != last && destinationNext != limit)
    {
        int count;
        if (limit - destinationNext >= MaximumMultibyteLength())
        {
            count = WideToMultibyte(destinationNext, *next, &state,
                reinterpret_cast<const RecordDBCtypeDescriptor*>(&localeLow));
            if (count <= 0) return 2;
        }
        else
        {
            char converted[2];
            const unsigned int savedState = state;
            count = WideToMultibyte(converted, *next, &state,
                reinterpret_cast<const RecordDBCtypeDescriptor*>(&localeLow));
            if (count <= 0) return 2;
            if (count > limit - destinationNext) { state = savedState; break; }
            std::memcpy(destinationNext, converted, count);
        }
        destinationNext += count;
        ++next;
        result = 0;
    }
    return result;
}
int WideCodecvtOperations::Length(unsigned int& state, const unsigned short* first,
    const unsigned short* last, unsigned int maximum) const
{
    const unsigned short* current = first;
    while (current != last && maximum != 0)
    {
        char converted[2];
        const int count = WideToMultibyte(converted, *current, &state,
            reinterpret_cast<const RecordDBCtypeDescriptor*>(&localeLow));
        if (count <= 0 || static_cast<unsigned int>(count) > maximum) break;
        maximum -= count;
        ++current;
    }
    return static_cast<int>(current - first);
}

template<class Character> int CollateOperations<Character>::Compare(const Character* first,
    const Character* last, const Character* otherFirst, const Character* otherLast) const
{
    for (; first != last && otherFirst != otherLast; ++first, ++otherFirst)
    {
        const int left = CharacterConversion<Character>::CollationValue(*first);
        const int right = CharacterConversion<Character>::CollationValue(*otherFirst);
        if (left < right) return -1;
        if (left > right) return 1;
    }
    return first != last ? 1 : otherFirst != otherLast ? -1 : 0;
}
template<class Character> long CollateOperations<Character>::Hash(const Character* first,
    const Character* last) const
{
    unsigned int hash = 0;
    for (; first != last; ++first)
        hash = ((hash << 8) | (hash >> 24)) + CharacterConversion<Character>::CollationValue(*first);
    return BitsToSigned32(hash);
}
template<class Character> StringData<Character>* CollateOperations<Character>::Transform(
    StringData<Character>* output, const Character* first, const Character* last) const
{
    TemporaryString<Character> text;
    const unsigned int count = static_cast<unsigned int>(last - first);
    text.Append(count, 0);
    StringOperations<Character>::Freeze(&text.value);
    if (count != 0) std::memcpy(text.value.data, first, count * sizeof(Character));
    StringOperations<Character>::Erase(&text.value, count, AS_RECORD_STRING_CONTROL::npos);
    output->allocatorState[0] = text.value.allocatorState[0];
    StringOperations<Character>::Tidy(output, false);
    return StringOperations<Character>::AssignSubstring(output, &text.value, 0, AS_RECORD_STRING_CONTROL::npos);
}

template<class Character> InputIterator<Character>* NumGetOperations<Character>::GetLong(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl& ios, unsigned int& state, long& value) const
{
    errno = 0;
    char field[32];
    char* end;
    long converted;
    {
        RetainedLocale locale(ios);
        const int base = ReadIntegerField(field, first, last, ios.formatFlags, locale.value);
        converted = std::strtol(field, &end, base);
    }
    if (EqualIterator(first, last)) state |= 1u;
    if (end == field || errno != 0) state |= 2u;
    else value = converted;
    return CopyIterator(output, first);
}
template<class Character> InputIterator<Character>* NumGetOperations<Character>::GetUnsignedLong(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl& ios, unsigned int& state, unsigned long& value) const
{
    errno = 0;
    char field[32];
    char* end;
    unsigned long converted;
    {
        RetainedLocale locale(ios);
        const int base = ReadIntegerField(field, first, last, ios.formatFlags, locale.value);
        converted = std::strtoul(field, &end, base);
    }
    if (EqualIterator(first, last)) state |= 1u;
    if (end == field || errno != 0) state |= 2u;
    else value = converted;
    return CopyIterator(output, first);
}
template<class Character> InputIterator<Character>* NumGetOperations<Character>::GetUnsignedInt(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl& ios, unsigned int& state, unsigned int& value) const
{
    return GetSmallUnsigned(output, first, last, ios, state, value, 4294967295ul);
}
template<class Character> InputIterator<Character>* NumGetOperations<Character>::GetUnsignedShort(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl& ios, unsigned int& state, unsigned short& value) const
{
    return GetSmallUnsigned(output, first, last, ios, state, value, 65535ul);
}
template<class Character> InputIterator<Character>* NumGetOperations<Character>::GetFloat(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl& ios, unsigned int& state, float& value) const
{
    return GetFloating(output, first, last, ios, state, value);
}
template<class Character> InputIterator<Character>* NumGetOperations<Character>::GetDouble(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl& ios, unsigned int& state, double& value) const
{
    return GetFloating(output, first, last, ios, state, value);
}
template<class Character> InputIterator<Character>* NumGetOperations<Character>::GetLongDouble(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl& ios, unsigned int& state, long double& value) const
{
    return GetFloating(output, first, last, ios, state, value);
}
template<class Character> InputIterator<Character>* NumGetOperations<Character>::GetPointer(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl& ios, unsigned int& state, void*& value) const
{
    errno = 0;
    char field[32];
    char* end;
    unsigned long converted;
    {
        RetainedLocale locale(ios);
        const int base = ReadIntegerField(field, first, last, 0x800u, locale.value);
        converted = std::strtoul(field, &end, base);
    }
    if (EqualIterator(first, last)) state |= 1u;
    if (end == field || errno != 0) state |= 2u;
    if ((state & 2u) == 0) value = U32ToPtr(static_cast<unsigned int>(converted));
    return CopyIterator(output, first);
}
template<class Character> InputIterator<Character>* NumGetOperations<Character>::GetBoolean(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl& ios, unsigned int& state, bool& value) const
{
    int result = -1;
    if ((ios.formatFlags & 0x4000u) != 0)
    {
        const NumpunctData<Character>* facet;
        {
            RetainedLocale locale(ios);
            facet = UseNumpunct<Character>(locale.value);
        }
        TemporaryString<Character> names;
        names.Append(1, 0);
        {
            TemporaryString<Character> name;
            NumpunctTableOf(facet)->falseName(facet, &name.value);
            StringOperations<Character>::AppendSubstring(&names.value, &name.value, 0u, AS_RECORD_STRING_CONTROL::npos);
        }
        names.Append(1, 0);
        {
            TemporaryString<Character> name;
            NumpunctTableOf(facet)->trueName(facet, &name.value);
            StringOperations<Character>::AppendSubstring(&names.value, &name.value, 0u, AS_RECORD_STRING_CONTROL::npos);
        }
        result = MatchLocaleText(first, last, 2u, names.Data());
    }
    else
    {
        errno = 0;
        char field[32];
        char* end;
        unsigned long converted;
        {
            RetainedLocale locale(ios);
            const int base = ReadIntegerField(field, first, last, ios.formatFlags, locale.value);
            converted = std::strtoul(field, &end, base);
        }
        if (end != field && errno == 0 && converted <= 1u) result = static_cast<int>(converted);
    }
    if (EqualIterator(first, last)) state |= 1u;
    if (result < 0) state |= 2u;
    else value = result != 0;
    return CopyIterator(output, first);
}

template<class Character> int TimeGetOperations<Character>::DateOrder() const { return 2; }
template<class Character> InputIterator<Character>* TimeGetOperations<Character>::GetTime(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl&, unsigned int& state, tm* value) const
{
    state |= ReadRangedInteger(first, last, 0, 23, value->tm_hour);
    if (state == 0 && ReadIterator(first) == CharacterConversion<Character>::Widen(':'))
    {
        AdvanceIterator(first);
        state |= ReadRangedInteger(first, last, 0, 59, value->tm_min);
    }
    else state |= 2u;
    if (state == 0 && ReadIterator(first) == CharacterConversion<Character>::Widen(':'))
    {
        AdvanceIterator(first);
        state |= ReadRangedInteger(first, last, 0, 59, value->tm_sec);
    }
    else state |= 2u;
    return CopyIterator(output, first);
}
template<class Character> InputIterator<Character>* TimeGetOperations<Character>::GetDate(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl& ios, unsigned int& state, tm* value) const
{
    InputIterator<Character> next;
    const InputIterator<Character>* returned = static_cast<const TimeGetVirtualTable<Character>*>(this->vftable)
        ->getMonth(this, &next, first, last, &ios, &state, value);
    CopyIterator(&first, *returned);
    if (state == 0 && ReadIterator(first) == CharacterConversion<Character>::Widen(' '))
    {
        AdvanceIterator(first);
        state |= ReadRangedInteger(first, last, 1, 31, value->tm_mday);
    }
    else state |= 2u;
    bool yearFollows = false;
    if (state == 0 && ReadIterator(first) == CharacterConversion<Character>::Widen(','))
    {
        AdvanceIterator(first);
        if (!EqualIterator(first, last) && ReadIterator(first) == CharacterConversion<Character>::Widen(' '))
        {
            AdvanceIterator(first);
            yearFollows = true;
        }
    }
    if (yearFollows)
    {
        returned = static_cast<const TimeGetVirtualTable<Character>*>(this->vftable)
            ->getYear(this, &next, first, last, &ios, &state, value);
        CopyIterator(&first, *returned);
    }
    else state |= 2u;
    if (EqualIterator(first, last)) state |= 1u;
    return CopyIterator(output, first);
}
template<class Character> InputIterator<Character>* TimeGetOperations<Character>::GetWeekday(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl&, unsigned int& state, tm* value) const
{
    const int match = MatchLocaleText(first, last, 0u, this->days);
    if (match < 0) state |= 2u;
    else value->tm_wday = match / 2;
    return CopyIterator(output, first);
}
template<class Character> InputIterator<Character>* TimeGetOperations<Character>::GetMonth(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl&, unsigned int& state, tm* value) const
{
    const int match = MatchLocaleText(first, last, 0u, this->months);
    if (match < 0) state |= 2u;
    else value->tm_mon = match / 2;
    return CopyIterator(output, first);
}
template<class Character> InputIterator<Character>* TimeGetOperations<Character>::GetYear(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    RecordDBIosControl&, unsigned int& state, tm* value) const
{
    int year;
    state |= ReadRangedInteger(first, last, 0, 2035, year);
    if ((state & 2u) == 0)
    {
        if (year >= 1900) year -= 1900;
        else if (year > 135) state |= 2u;
    }
    if ((state & 2u) == 0) value->tm_year = year;
    return CopyIterator(output, first);
}
template<class Character> OutputIterator<Character>* TimePutOperations<Character>::Put(
    OutputIterator<Character>* output, OutputIterator<Character> iterator,
    RecordDBIosControl&, const tm* value, char format, char modifier) const
{
    char conversion[] = "!%x\0";
    if (modifier == 0) conversion[2] = format;
    else { conversion[2] = modifier; conversion[3] = format; }
    TemporaryString<char> text;
    unsigned int increment = 16u;
    unsigned int count;
    for (;;)
    {
        text.Append(increment, '\0');
        const unsigned int capacity = text.value.size;
        const CrtTimeNames* names = static_cast<const CrtTimeNames*>(this->timeNames);
        StringOperations<char>::Freeze(&text.value);
        count = FormatTime(text.value.data, capacity, conversion, value, names);
        if (count != 0) break;
        increment += increment;
    }
    StringOperations<char>::Freeze(&text.value);
    const char* cursor = text.value.data;
    while (--count != 0)
        PutIterator(iterator, CharacterConversion<Character>::Widen(*++cursor));
    return CopyIterator(output, iterator);
}

namespace
{
    template<class Character> void PutAscii(OutputIterator<Character>& iterator,
        const char* first, unsigned int count)
    {
        for (unsigned int index = 0; index != count; ++index)
            PutIterator(iterator, CharacterConversion<Character>::Widen(first[index]));
    }
    template<class Character> OutputIterator<Character>* PutIntegerField(
        OutputIterator<Character>* output, OutputIterator<Character> iterator,
        RecordDBIosControl& ios, Character fill, char* text, unsigned int count)
    {
        unsigned int prefix = text[0] == '+' || text[0] == '-' ? 1u
            : text[0] == '0' && (text[1] == 'x' || text[1] == 'X') ? 2u : 0u;
        const NumpunctData<Character>* facet;
        {
            RetainedLocale locale(ios);
            facet = UseNumpunct<Character>(locale.value);
        }
        TemporaryString<char> grouping;
        NumpunctTableOf(facet)->grouping(facet, &grouping.value);
        const Character separator = NumpunctTableOf(facet)->thousandsSeparator(facet);
        bool inserted = false;
        const char* pattern = grouping.Data();
        unsigned int position = count;
        while (static_cast<signed char>(*pattern) > 0 && *pattern != 127)
        {
            const unsigned int group = static_cast<unsigned int>(static_cast<signed char>(*pattern));
            if (group >= position - prefix) break;
            position -= group;
            std::memmove(text + position + 1, text + position, count - position + 1);
            ++count;
            text[position] = ',';
            if (static_cast<signed char>(pattern[1]) > 0) ++pattern;
            inserted = true;
        }
        const int width = BitsToSigned32(ios.width);
        unsigned int padding = width > 0 && static_cast<unsigned int>(width) > count ? width - count : 0u;
        const unsigned int adjustment = ios.formatFlags & 0x1C0u;
        if (adjustment != 0x40u)
        {
            if (adjustment == 0x100u)
            {
                PutAscii(iterator, text, prefix);
                text += prefix;
                count -= prefix;
            }
            PutFill(iterator, fill, padding);
            padding = 0;
        }
        if (inserted)
        {
            while (count != 0)
            {
                const unsigned int chunk = static_cast<unsigned int>(std::strcspn(text, ","));
                PutAscii(iterator, text, chunk);
                text += chunk;
                count -= chunk;
                if (count == 0) break;
                if (separator != 0) PutIterator(iterator, separator);
                ++text;
                --count;
            }
        }
        else PutAscii(iterator, text, count);
        ios.width = 0;
        PutFill(iterator, fill, padding);
        return CopyIterator(output, iterator);
    }

    template<class Character> OutputIterator<Character>* PutFloatingField(
        OutputIterator<Character>* output, OutputIterator<Character> iterator,
        RecordDBIosControl& ios, Character fill, const char* text,
        unsigned int trailingZeros, unsigned int count)
    {
        const int width = BitsToSigned32(ios.width);
        unsigned int padding = width > 0 && static_cast<unsigned int>(width) > count + trailingZeros
            ? width - count - trailingZeros : 0u;
        const unsigned int adjustment = ios.formatFlags & 0x1C0u;
        if (adjustment != 0x40u)
        {
            if (adjustment == 0x100u && count != 0 && (*text == '+' || *text == '-'))
            {
                PutIterator(iterator, CharacterConversion<Character>::Widen(*text));
                ++text;
                --count;
            }
            PutFill(iterator, fill, padding);
            padding = 0;
        }
        const char* decimal = static_cast<const char*>(std::memchr(text, GetLocaleConvention()->decimalPoint[0], count));
        if (decimal)
        {
            const NumpunctData<Character>* facet;
            {
                RetainedLocale locale(ios);
                facet = UseNumpunct<Character>(locale.value);
            }
            const unsigned int before = static_cast<unsigned int>(decimal - text);
            PutAscii(iterator, text, before);
            PutIterator(iterator, NumpunctTableOf(facet)->decimalPoint(facet));
            text += before + 1;
            count -= before + 1;
        }
        const char* exponent = static_cast<const char*>(std::memchr(text, 'e', count));
        if (exponent)
        {
            const unsigned int before = static_cast<unsigned int>(exponent - text);
            PutAscii(iterator, text, before);
            PutFill(iterator, CharacterConversion<Character>::Widen('0'), trailingZeros);
            trailingZeros = 0;
            PutIterator(iterator, CharacterConversion<Character>::Widen((ios.formatFlags & 4u) ? 'E' : 'e'));
            text += before + 1;
            count -= before + 1;
        }
        PutAscii(iterator, text, count);
        PutFill(iterator, CharacterConversion<Character>::Widen('0'), trailingZeros);
        ios.width = 0;
        PutFill(iterator, fill, padding);
        return CopyIterator(output, iterator);
    }

    char* IntegerFormat(char* output, char conversion, unsigned int flags)
    {
        char* cursor = output;
        *cursor++ = '%';
        if (flags & 0x20u) *cursor++ = '+';
        if (flags & 8u) *cursor++ = '#';
        *cursor++ = 'l';
        const unsigned int base = flags & 0xE00u;
        *cursor++ = base == 0x400u ? 'o' : base == 0x800u ? ((flags & 4u) ? 'X' : 'x') : conversion;
        *cursor = '\0';
        return output;
    }
    char* FloatingFormat(char* output, char modifier, unsigned int flags)
    {
        char* cursor = output;
        *cursor++ = '%';
        if (flags & 0x20u) *cursor++ = '+';
        if (flags & 0x10u) *cursor++ = '#';
        *cursor++ = '.';
        *cursor++ = '*';
        if (modifier) *cursor++ = modifier;
        const unsigned int mode = flags & 0x3000u;
        *cursor++ = mode == 0x2000u ? 'f' : mode == 0x1000u ? 'e' : 'g';
        *cursor = '\0';
        return output;
    }
    template<class Character, class Value> OutputIterator<Character>* PutFloating(
        OutputIterator<Character>* output, OutputIterator<Character> iterator,
        RecordDBIosControl& ios, Character fill, Value value, char modifier)
    {
        int precision = BitsToSigned32(ios.precision);
        if (precision <= 0 && (ios.formatFlags & 0x2000u) == 0) precision = 6;
        const int renderedPrecision = precision > 36 ? 36 : precision;
        char format[8];
        char text[76];
        const int count = std::sprintf(text, FloatingFormat(format, modifier, ios.formatFlags), renderedPrecision, value);
        return PutFloatingField(output, iterator, ios, fill, text,
            static_cast<unsigned int>(precision - renderedPrecision), static_cast<unsigned int>(count));
    }
}

template<class Character> OutputIterator<Character>* NumPutOperations<Character>::PutLong(
    OutputIterator<Character>* output, OutputIterator<Character> iterator,
    RecordDBIosControl& ios, Character fill, long value) const
{
    char format[8];
    char text[64];
    const int count = std::sprintf(text, IntegerFormat(format, 'd', ios.formatFlags), value);
    return PutIntegerField(output, iterator, ios, fill, text, static_cast<unsigned int>(count));
}
template<class Character> OutputIterator<Character>* NumPutOperations<Character>::PutUnsignedLong(
    OutputIterator<Character>* output, OutputIterator<Character> iterator,
    RecordDBIosControl& ios, Character fill, unsigned long value) const
{
    char format[8];
    char text[64];
    const int count = std::sprintf(text, IntegerFormat(format, 'u', ios.formatFlags), value);
    return PutIntegerField(output, iterator, ios, fill, text, static_cast<unsigned int>(count));
}
template<class Character> OutputIterator<Character>* NumPutOperations<Character>::PutDouble(
    OutputIterator<Character>* output, OutputIterator<Character> iterator,
    RecordDBIosControl& ios, Character fill, double value) const
{
    return PutFloating(output, iterator, ios, fill, value, 0);
}
template<class Character> OutputIterator<Character>* NumPutOperations<Character>::PutLongDouble(
    OutputIterator<Character>* output, OutputIterator<Character> iterator,
    RecordDBIosControl& ios, Character fill, long double value) const
{
    return PutFloating(output, iterator, ios, fill, value, 'L');
}
template<class Character> OutputIterator<Character>* NumPutOperations<Character>::PutPointer(
    OutputIterator<Character>* output, OutputIterator<Character> iterator,
    RecordDBIosControl& ios, Character fill, const void* value) const
{
    char text[68];
    const unsigned int count = static_cast<unsigned int>(std::sprintf(text, "%p", value));
    const int width = BitsToSigned32(ios.width);
    unsigned int padding = width > 0 && width > static_cast<int>(count) ? width - count : 0;
    if ((ios.formatFlags & 0x1C0u) != 0x40u)
    {
        PutFill(iterator, fill, padding);
        padding = 0;
    }
    PutAscii(iterator, text, count);
    ios.width = 0;
    PutFill(iterator, fill, padding);
    return CopyIterator(output, iterator);
}
template<class Character> OutputIterator<Character>* NumPutOperations<Character>::PutBoolean(
    OutputIterator<Character>* output, OutputIterator<Character> iterator,
    RecordDBIosControl& ios, Character fill, bool value) const
{
    const NumpunctData<Character>* facet;
    {
        RetainedLocale locale(ios);
        facet = UseNumpunct<Character>(locale.value);
    }
    TemporaryString<Character> text;
    if ((ios.formatFlags & 0x4000u) == 0)
        text.Append(1u, CharacterConversion<Character>::Widen(value ? '1' : '0'));
    else
    {
        TemporaryString<Character> name;
        if (value) NumpunctTableOf(facet)->trueName(facet, &name.value);
        else NumpunctTableOf(facet)->falseName(facet, &name.value);
        StringOperations<Character>::AssignSubstring(&text.value, &name.value, 0, AS_RECORD_STRING_CONTROL::npos);
    }
    const int width = BitsToSigned32(ios.width);
    unsigned int padding = width > 0 && static_cast<unsigned int>(width) > text.value.size
        ? width - text.value.size : 0;
    if ((ios.formatFlags & 0x1C0u) != 0x40u)
    {
        PutFill(iterator, fill, padding);
        padding = 0;
    }
    PutRange(iterator, text.Data(), text.value.size);
    ios.width = 0;
    PutFill(iterator, fill, padding);
    return CopyIterator(output, iterator);
}

namespace
{
    bool ClassifyMoney(const CtypeData* facet, char character, unsigned short mask)
    {
        return (facet->descriptor.table[static_cast<unsigned char>(character)] & mask) != 0;
    }
    bool ClassifyMoney(const CtypeData* facet, unsigned short character, unsigned short mask)
    {
        return static_cast<const WideCtypeVirtualTable*>(facet->vftable)->classify(facet, mask, character);
    }
    template<class Character> const MoneypunctData<Character>* AcquireMoneyPunctuation(
        RecordDBIosControl& ios, bool international)
    {
        RetainedLocale locale(ios);
        if (international)
            return LocaleFacetUse<typename FacetSelection<Character>::MoneypunctIntl>::Get(locale.value);
        return LocaleFacetUse<typename FacetSelection<Character>::Moneypunct>::Get(locale.value);
    }
    template<class Character> const CtypeData* AcquireCharacterType(RecordDBIosControl& ios)
    {
        RetainedLocale locale(ios);
        return LocaleFacetUse<typename FacetSelection<Character>::Ctype>::Get(locale.value);
    }
    template<class Character> const MoneypunctVirtualTable<Character>* MoneyTableOf(
        const MoneypunctData<Character>* facet)
    {
        return static_cast<const MoneypunctVirtualTable<Character>*>(facet->vftable);
    }
    template<class Character> void InsertStringFill(StringData<Character>& text,
        unsigned int position, unsigned int count, Character value)
    {
        if (position > text.size) Msvc6RangeError();
        if (AS_RECORD_STRING_CONTROL::npos - text.size <= count) Msvc6LengthError();
        if (count != 0)
        {
            const unsigned int size = text.size + count;
            if (StringOperations<Character>::Grow(&text, size, false))
            {
                std::memmove(text.data + position + count, text.data + position,
                    (text.size - position) * sizeof(Character));
                for (unsigned int index = 0; index < count; ++index) text.data[position + index] = value;
                text.size = size;
                text.data[size] = 0;
            }
        }
    }

    template<class Character> bool MatchSignStart(const MoneypunctData<Character>* facet,
        InputIterator<Character>& first, bool negative)
    {
        TemporaryString<Character> sizeTest;
        if (negative) MoneyTableOf(facet)->negativeSign(facet, &sizeTest.value);
        else MoneyTableOf(facet)->positiveSign(facet, &sizeTest.value);
        if (sizeTest.value.size == 0) return false;
        TemporaryString<Character> firstTest;
        if (negative) MoneyTableOf(facet)->negativeSign(facet, &firstTest.value);
        else MoneyTableOf(facet)->positiveSign(facet, &firstTest.value);
        const Character character = *StringElement<Character>(firstTest.value, 0);
        return character == ReadIterator(first);
    }

    template<class Character> StringData<Character>* ReadMoneyField(StringData<Character>* output,
        InputIterator<Character>& first, InputIterator<Character>& last,
        bool international, RecordDBIosControl& ios)
    {
        const MoneypunctData<Character>* const facet = AcquireMoneyPunctuation<Character>(ios, international);
        const CtypeData* const ctype = AcquireCharacterType<Character>(ios);
        bool failed = false;
        bool negative = false;
        TemporaryString<Character> sign;
        TemporaryString<Character> digits;
        MoneyPattern pattern;
        MoneyTableOf(facet)->negativePattern(facet, &pattern);
        for (unsigned int part = 0; part < 4 && !failed; ++part)
        {
            switch (pattern.fields[part])
            {
            case 'v':
            {
                int fractionCount = 0;
                const int fractionDigits = MoneyTableOf(facet)->fractionDigits(facet);
                TemporaryString<char> grouping;
                MoneyTableOf(facet)->grouping(facet, &grouping.value);
                if (static_cast<signed char>(grouping.Data()[0]) <= 0)
                {
                    while (!EqualIterator(first, last) && ClassifyMoney(ctype, ReadIterator(first), 4u))
                    {
                        InputIterator<Character> previous;
                        CopyIterator(&previous, first);
                        AdvanceIterator(first);
                        digits.Append(1u, ReadIterator(previous));
                    }
                }
                else
                {
                    const Character separator = MoneyTableOf(facet)->thousandsSeparator(facet);
                    TemporaryString<char> groups;
                    groups.Append(1u, 0);
                    unsigned int group = 0;
                    if (!EqualIterator(first, last))
                    {
                        do
                        {
                            if (ClassifyMoney(ctype, ReadIterator(first), 4u))
                            {
                                digits.Append(1u, ReadIterator(first));
                                if (*StringElement<char>(groups.value, group) != 127)
                                    ++*StringElement<char>(groups.value, group);
                            }
                            else
                            {
                                if (*StringElement<char>(groups.value, group) == 0 || ReadIterator(first) != separator)
                                    break;
                                groups.Append(1u, 0);
                                ++group;
                            }
                            AdvanceIterator(first);
                        } while (!EqualIterator(first, last));
                        if (group != 0)
                        {
                            if (static_cast<signed char>(*StringElement<char>(groups.value, group)) > 0) ++group;
                            else failed = true;
                        }
                    }
                    const char* rule = grouping.Data();
                    while (!failed && group > 0 && *rule != 127)
                    {
                        --group;
                        if ((group != 0 && *rule != *StringElement<char>(groups.value, group))
                            || (group == 0 && static_cast<signed char>(*rule)
                                < static_cast<signed char>(*StringElement<char>(groups.value, group))))
                            failed = true;
                        else if (static_cast<signed char>(rule[1]) > 0) ++rule;
                        --group;
                    }
                }
                if (failed) break;
                if (!EqualIterator(first, last)
                    && ReadIterator(first) == MoneyTableOf(facet)->decimalPoint(facet))
                {
                    AdvanceIterator(first);
                    while (!EqualIterator(first, last) && ClassifyMoney(ctype, ReadIterator(first), 4u))
                    {
                        if (fractionCount < fractionDigits)
                        {
                            digits.Append(1u, ReadIterator(first));
                            ++fractionCount;
                        }
                        AdvanceIterator(first);
                    }
                }
                if (digits.value.size == 0) failed = true;
                else while (fractionCount < fractionDigits)
                {
                    digits.Append(1u, CharacterConversion<Character>::Widen('0'));
                    ++fractionCount;
                }
                break;
            }
            case '+':
                if (!EqualIterator(first, last))
                {
                    if (MatchSignStart(facet, first, false))
                    {
                        AdvanceIterator(first);
                        TemporaryString<Character> completeSign;
                        MoneyTableOf(facet)->positiveSign(facet, &completeSign.value);
                        StringOperations<Character>::AssignSubstring(&sign.value, &completeSign.value, 0, AS_RECORD_STRING_CONTROL::npos);
                    }
                    else if (MatchSignStart(facet, first, true))
                    {
                        AdvanceIterator(first);
                        TemporaryString<Character> completeSign;
                        MoneyTableOf(facet)->negativeSign(facet, &completeSign.value);
                        StringOperations<Character>::AssignSubstring(&sign.value, &completeSign.value, 0, AS_RECORD_STRING_CONTROL::npos);
                        negative = true;
                    }
                }
                if (sign.value.size == 1)
                    StringOperations<Character>::Erase(&sign.value, 0, AS_RECORD_STRING_CONTROL::npos);
                break;
            case '$':
            {
                TemporaryString<Character> symbol;
                MoneyTableOf(facet)->currencySymbol(facet, &symbol.value);
                if ((ios.formatFlags & 8u) == 0
                    && ((part == 3 && sign.value.size == 0) || EqualIterator(first, last)
                        || ReadIterator(first) != symbol.Data()[0]))
                    StringOperations<Character>::Erase(&symbol.value, 0, AS_RECORD_STRING_CONTROL::npos);
                StringOperations<Character>::Freeze(&symbol.value);
                Character* current = symbol.value.data;
                while (!EqualIterator(first, last))
                {
                    StringOperations<Character>::Freeze(&symbol.value);
                    Character* const end = symbol.value.data ? symbol.value.data + symbol.value.size : 0;
                    if (current == end || ReadIterator(first) != *current) break;
                    ++current;
                    AdvanceIterator(first);
                }
                StringOperations<Character>::Freeze(&symbol.value);
                if (current != (symbol.value.data ? symbol.value.data + symbol.value.size : 0)) failed = true;
                break;
            }
            default:
                if (part != 3)
                    while (!EqualIterator(first, last) && ClassifyMoney(ctype, ReadIterator(first), 0x48u))
                        AdvanceIterator(first);
                break;
            }
        }
        if (!failed && sign.value.size != 0)
        {
            StringOperations<Character>::Freeze(&sign.value);
            Character* current = sign.value.data;
            while (!EqualIterator(first, last))
            {
                ++current;
                StringOperations<Character>::Freeze(&sign.value);
                Character* const end = sign.value.data ? sign.value.data + sign.value.size : 0;
                if (current == end || ReadIterator(first) != *current) break;
                AdvanceIterator(first);
            }
            StringOperations<Character>::Freeze(&sign.value);
            if (current != (sign.value.data ? sign.value.data + sign.value.size : 0)) failed = true;
        }
        if (failed) StringOperations<Character>::Erase(&digits.value, 0, AS_RECORD_STRING_CONTROL::npos);
        else if (negative) InsertStringFill(digits.value, 0u, 1u, CharacterConversion<Character>::Widen('-'));
        output->allocatorState[0] = digits.value.allocatorState[0];
        StringOperations<Character>::Tidy(output, false);
        return StringOperations<Character>::AssignSubstring(output, &digits.value, 0, AS_RECORD_STRING_CONTROL::npos);
    }

    template<class Character> OutputIterator<Character>* PutMoneyField(OutputIterator<Character>* output,
        OutputIterator<Character> iterator, bool international, RecordDBIosControl& ios,
        Character fill, bool negative, const StringData<Character>& source)
    {
        // The native core receives its basic_string by value. Its COW lifetime
        // remains distinct from the caller's normalized number.
        TemporaryString<Character> digits(source.allocatorState[0]);
        StringOperations<Character>::AssignSubstring(&digits.value, &source, 0, AS_RECORD_STRING_CONTROL::npos);
        const MoneypunctData<Character>* const facet = AcquireMoneyPunctuation<Character>(ios, international);
        const int fractionDigits = MoneyTableOf(facet)->fractionDigits(facet);
        const unsigned int fraction = static_cast<unsigned int>(fractionDigits);
        TemporaryString<char> grouping;
        MoneyTableOf(facet)->grouping(facet, &grouping.value);
        if (fraction < digits.value.size && static_cast<signed char>(grouping.Data()[0]) > 0)
        {
            const Character separator = MoneyTableOf(facet)->thousandsSeparator(facet);
            const char* rule = grouping.Data();
            unsigned int position = digits.value.size - fraction;
            while (static_cast<signed char>(*rule) > 0 && *rule != 127
                && static_cast<unsigned int>(static_cast<signed char>(*rule)) < position)
            {
                position -= static_cast<unsigned int>(static_cast<signed char>(*rule));
                InsertStringFill(digits.value, position, 1u, separator);
                if (static_cast<signed char>(rule[1]) > 0) ++rule;
            }
        }
        TemporaryString<Character> sign;
        MoneyPattern pattern;
        if (negative)
        {
            MoneyTableOf(facet)->negativePattern(facet, &pattern);
            TemporaryString<Character> selected;
            MoneyTableOf(facet)->negativeSign(facet, &selected.value);
            StringOperations<Character>::AssignSubstring(&sign.value, &selected.value, 0, AS_RECORD_STRING_CONTROL::npos);
        }
        else
        {
            MoneyTableOf(facet)->positivePattern(facet, &pattern);
            TemporaryString<Character> selected;
            MoneyTableOf(facet)->positiveSign(facet, &selected.value);
            StringOperations<Character>::AssignSubstring(&sign.value, &selected.value, 0, AS_RECORD_STRING_CONTROL::npos);
        }
        TemporaryString<Character> symbol;
        if (ios.formatFlags & 8u)
        {
            TemporaryString<Character> selected;
            MoneyTableOf(facet)->currencySymbol(facet, &selected.value);
            StringOperations<Character>::AssignSubstring(&symbol.value, &selected.value, 0, AS_RECORD_STRING_CONTROL::npos);
        }
        bool internalSpace = false;
        unsigned int count = 0;
        for (unsigned int part = 0; part < 4; ++part)
        {
            switch (pattern.fields[part])
            {
            case 'v': count += digits.value.size + (fraction != 0 ? 1u : 0u)
                + (digits.value.size <= fraction ? fraction - digits.value.size + 1u : 0u); break;
            case '+': count += sign.value.size; break;
            case '$': count += symbol.value.size; break;
            case ' ': internalSpace = true; break;
            }
        }
        const int width = BitsToSigned32(ios.width);
        unsigned int padding = width > 0 && static_cast<unsigned int>(width) > count ? width - count : 0u;
        const unsigned int adjustment = ios.formatFlags & 0x1C0u;
        if (adjustment != 0x40u && (adjustment != 0x100u || !internalSpace))
        {
            PutFill(iterator, fill, padding);
            padding = 0;
        }
        for (unsigned int part = 0; part < 4; ++part)
        {
            switch (pattern.fields[part])
            {
            case 'v':
                if (fraction == 0)
                {
                    StringOperations<Character>::Freeze(&digits.value);
                    PutRange(iterator, digits.value.data, digits.value.size);
                }
                else if (digits.value.size <= fraction)
                {
                    PutIterator(iterator, CharacterConversion<Character>::Widen('0'));
                    PutIterator(iterator, MoneyTableOf(facet)->decimalPoint(facet));
                    PutFill(iterator, CharacterConversion<Character>::Widen('0'), fraction - digits.value.size);
                    StringOperations<Character>::Freeze(&digits.value);
                    PutRange(iterator, digits.value.data, digits.value.size);
                }
                else
                {
                    StringOperations<Character>::Freeze(&digits.value);
                    PutRange(iterator, digits.value.data, digits.value.size - fraction);
                    PutIterator(iterator, MoneyTableOf(facet)->decimalPoint(facet));
                    StringOperations<Character>::Freeze(&digits.value);
                    PutRange(iterator, digits.value.data + digits.value.size - fraction, fraction);
                }
                break;
            case '+':
                if (sign.value.size != 0)
                {
                    StringOperations<Character>::Freeze(&sign.value);
                    PutIterator(iterator, sign.value.data[0]);
                }
                break;
            case '$':
                StringOperations<Character>::Freeze(&symbol.value);
                PutRange(iterator, symbol.value.data, symbol.value.size);
                break;
            case ' ':
                if (adjustment == 0x100u)
                {
                    PutFill(iterator, fill, padding);
                    padding = 0;
                }
                break;
            }
        }
        if (sign.value.size > 1)
        {
            StringOperations<Character>::Freeze(&sign.value);
            PutRange(iterator, sign.value.data + 1, sign.value.size - 1);
        }
        ios.width = 0;
        PutFill(iterator, fill, padding);
        return CopyIterator(output, iterator);
    }

}

template<class Character> InputIterator<Character>* MoneyGetOperations<Character>::GetString(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    bool international, RecordDBIosControl& ios, unsigned int& state, StringData<Character>& value) const
{
    TemporaryString<Character> digits;
    ReadMoneyField(&digits.value, first, last, international, ios);
    if (EqualIterator(first, last)) state |= 1u;
    if (digits.value.size == 0) state |= 2u;
    else StringOperations<Character>::AssignSubstring(&value, &digits.value, 0, AS_RECORD_STRING_CONTROL::npos);
    return CopyIterator(output, first);
}
template<class Character> InputIterator<Character>* MoneyGetOperations<Character>::GetValue(
    InputIterator<Character>* output, InputIterator<Character> first, InputIterator<Character> last,
    bool international, RecordDBIosControl& ios, unsigned int& state, long double& value) const
{
    TemporaryString<Character> digits;
    ReadMoneyField(&digits.value, first, last, international, ios);
    if (EqualIterator(first, last)) state |= 1u;
    if (digits.value.size == 0) state |= 2u;
    else
    {
        TemporaryString<char> converted;
        if (converted.value.capacity < digits.value.size)
            StringOperations<char>::Grow(&converted.value, digits.value.size, false);
        for (unsigned int index = 0; index < digits.value.size; ++index)
            converted.Append(1u, CharacterConversion<Character>::Narrow(*StringElement<Character>(digits.value, index)));
        const char* start = converted.Data();
        char* end;
        errno = 0;
        const double result = ParseFloating(start, &end, 0);
        if (end == start || errno != 0) state |= 2u;
        else value = result;
    }
    return CopyIterator(output, first);
}
template<class Character> OutputIterator<Character>* MoneyPutOperations<Character>::PutString(
    OutputIterator<Character>* output, OutputIterator<Character> iterator, bool international,
    RecordDBIosControl& ios, Character fill, const StringData<Character>& value) const
{
    const CtypeData* const ctype = AcquireCharacterType<Character>(ios);
    const Character* start = value.data ? value.data : StringEmptyData<Character>();
    const bool negative = *start == CharacterConversion<Character>::Widen('-');
    if (negative) ++start;
    unsigned int count = 0;
    while (ClassifyMoney(ctype, start[count], 4u)) ++count;
    TemporaryString<Character> digits;
    StringOperations<Character>::AssignRange(&digits.value, start, count);
    if (count == 0) digits.Append(1u, CharacterConversion<Character>::Widen('0'));
    OutputIterator<Character> result;
    const OutputIterator<Character>* returned = PutMoneyField(&result, iterator, international, ios, fill, negative, digits.value);
    return CopyIterator(output, *returned);
}
template<class Character> OutputIterator<Character>* MoneyPutOperations<Character>::PutValue(
    OutputIterator<Character>* output, OutputIterator<Character> iterator, bool international,
    RecordDBIosControl& ios, Character fill, long double value) const
{
    double amount = static_cast<double>(value);
    bool negative;
    unsigned int extraZeros;
    PrepareMoneyAmount(amount, negative, extraZeros);
    TemporaryString<Character> digits;
    char formatted[40];
    const int count = std::sprintf(formatted, "%.0Lf", static_cast<long double>(amount));
    for (int index = 0; index < count; ++index)
        digits.Append(1u, CharacterConversion<Character>::Widen(formatted[index]));
    digits.Append(extraZeros, CharacterConversion<Character>::Widen('0'));
    OutputIterator<Character> result;
    const OutputIterator<Character>* returned = PutMoneyField(&result, iterator, international, ios, fill, negative, digits.value);
    return CopyIterator(output, *returned);
}

template struct CollateOperations<char>;
template struct CollateOperations<unsigned short>;
template struct NumGetOperations<char>;
template struct NumGetOperations<unsigned short>;
template struct NumPutOperations<char>;
template struct NumPutOperations<unsigned short>;
template struct MoneyGetOperations<char>;
template struct MoneyGetOperations<unsigned short>;
template struct MoneyPutOperations<char>;
template struct MoneyPutOperations<unsigned short>;
template struct TimeGetOperations<char>;
template struct TimeGetOperations<unsigned short>;
template struct TimePutOperations<char>;
template struct TimePutOperations<unsigned short>;
}
