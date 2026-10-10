#include "myerror.h"
#include "list.h"
#include "mylib.h"
#include "runtime/locale_state.h"
#include "runtime/locale_tables.h"
#include "resread.h"
#include "basetype.h"
#include <cstdio>
#include <cstdlib>
#if defined(_WIN32)
#include <io.h>
#else
extern "C" int _fileno(std::FILE*);
extern "C" long _filelength(int);
#endif

RESOURCE::RESOURCE()
{
    m_file = 0;
    m_recordPosition = 0u;
    m_recordSize = 0u;
    m_basePosition = 0u;
    m_archiveEnd = 0u;


    const unsigned int flags = *reinterpret_cast<volatile unsigned int*>(&m_flags)&~1u;
    m_ioState = IO_SEEK;
    m_flags = flags;
}

RESOURCE::RESOURCE(RESOURCE::OPEN_MODE mode, const STRING& fileName, unsigned int type)
{
    const unsigned int flags = *reinterpret_cast<volatile unsigned int*>(&m_flags)&~1u;
    m_file = 0;
    m_recordPosition = 0u;
    m_recordSize = 0u;
    m_basePosition = 0u;
    m_archiveEnd = 0u;
    m_flags = flags;
    m_ioState = IO_SEEK;
    if (mode == OPEN_MODE_APPEND) (void)OpenForAppend(fileName, type);
    else if (mode == OPEN_MODE_WRITE) (void)OpenForWrite(fileName, type);
    else (void)OpenForRead(fileName, type);
}

RESOURCE::~RESOURCE() noexcept(false)
{
    Close();
}

STRING RESOURCE::GetFileName() const
{
    return m_name;
}

int RESOURCE::Open(std::FILE* file, unsigned int type)
{
    if (m_file != 0) Close();
    m_file = file;

    if (m_file == 0)
    {
        STRING subject("file is NULL");
        Error(E_OPEN, subject, 0u);
        return 1;
    }

    m_basePosition = static_cast<unsigned int>(std::ftell(m_file));
    if (Read(&m_signature, 4u) != 0u)
    {
        {
            STRING subject("empty file");
            Error(E_LOAD, subject, 0u);
        }
        Close();
        return 4;
    }

    if (m_signature != ResourceType::Resource && m_signature != ResourceType::Riff)
    {
        {
            STRING subject("resource signature");
            Error(E_INVALID, subject, 0u);
        }
        Close();
        return 2;
    }

    (void)Read(&m_archiveEnd, 4u);
    m_archiveEnd+=m_basePosition+8u;

    const int physicalLength = BitsToSigned32(
        static_cast<unsigned int>(_filelength(_fileno(m_file))));
    if (physicalLength<BitsToSigned32(m_archiveEnd))
    {
        STRING subject("Invalid filelength");
        const unsigned long delta = static_cast<unsigned int>(_filelength(_fileno(m_file)))-m_archiveEnd;
        Error(E_ERROR, subject, delta);
    }

    unsigned int rootType;
    (void)Read(&rootType, 4u);
    if (rootType != type && type != ResourceType::Any)
    {
        m_recordType = rootType;
        {
            STRING subject("resource type");
            Error(E_INVALID, subject, 0u);
        }
        Close();
        return 3;
    }

    (void)GoBegin(ResourceType::Any);
    return 0;
}


void RESOURCE::Close()
{
    if (m_file != 0)
    {
        if ((m_flags&1u) != 0u)
        {
            m_ioState = IO_SEEK;
            (void)std::fseek(m_file, BitsToSigned32(m_basePosition+4u), SEEK_SET);
            m_archiveEnd-=8u+m_basePosition;
            (void)Write(&m_archiveEnd, 4u);
        }
        (void)std::fclose(m_file);
    }
    m_file = 0;
    m_archiveEnd = 0u;
    m_name = "Not opened";
}


int RESOURCE::Read(void* buffer, unsigned int count)
{
    if (m_file == 0 || count == 0u) return BitsToSigned32(count);
    if (m_ioState == IO_WRITE)
    {
        m_ioState = IO_SEEK;
        (void)std::fseek(m_file, 0L, SEEK_CUR);
    }
    m_ioState = IO_READ;
    return BitsToSigned32(count-static_cast<unsigned int>(std::fread(buffer, 1u, count, m_file)));
}


int RESOURCE::Write(const void* buffer, unsigned int count)
{
    if (m_file == 0 || count == 0u) return BitsToSigned32(count);
    if (m_ioState == IO_READ)
    {
        m_ioState = IO_SEEK;
        (void)std::fseek(m_file, 0L, SEEK_CUR);
    }
    m_ioState = IO_WRITE;
    return BitsToSigned32(count-static_cast<unsigned int>(std::fwrite(buffer, 1u, count, m_file)));
}


int RESOURCE::OpenForRead(const STRING& fileName, unsigned int type)
{
    if (m_file != 0) Close();

    std::FILE* openedFile;
    {
        STRING openMode("rb");
        const char* const name = fileName.value;
        openedFile = name[0] ? std::fopen(name, openMode.value) : 0;
    }

    if (openedFile == 0)
    {
        Error(E_OPEN, fileName, 0u);
        return 1;
    }

    m_name = fileName;

    return Open(openedFile, type);
}


int RESOURCE::OpenForWrite(const STRING& fileName, unsigned int type)
{
    unsigned int rootSize = 4u;
    if (m_file != 0) Close();

    {
        STRING openMode("wb+");
        const char* const name = fileName.value;
        m_file = name[0]
            ? std::fopen(name, openMode.value)
            : 0;
    }
    if (m_file == 0)
    {
        Error(E_CREATE, fileName, 0u);
        return 1;
    }

    m_basePosition = 0u;
    m_archiveEnd = 12u;
    m_recordType = 0u;
    m_name = fileName;
    m_signature = ResourceType::Resource;

    (void)Write(&m_signature, 4u);
    (void)Write(&rootSize, 4u);
    (void)Write(&type, 4u);
    (void)GoBegin(ResourceType::Any);
    return 0;
}


int RESOURCE::OpenForAppend(const STRING& fileName, unsigned int type)
{
    if (m_file != 0) Close();

    std::FILE* openedFile;
    {
        STRING openMode("rb+");
        const char* const name = fileName.value;
        openedFile = name[0] ? std::fopen(name, openMode.value) : 0;
    }
    m_name = fileName;
    if (openedFile != 0)
        return Open(openedFile, type);
    return OpenForWrite(fileName, type);
}


int RESOURCE::GoBegin(unsigned int type)
{
    if (m_file == 0) return 1;
    m_recordPosition = m_basePosition+4u;
    m_recordSize = 0u;
    return GoNext(type);
}


int RESOURCE::GoNext(unsigned int type)
{
    if (m_file == 0) return 1;

    for (;;)
    {
        const unsigned int alignedSize = (m_recordSize+1u)&~1u;
        const unsigned int nextPosition = m_recordPosition+alignedSize+8u;
        m_ioState = IO_SEEK;
        m_recordPosition = nextPosition;
        (void)std::fseek(m_file, BitsToSigned32(nextPosition), SEEK_SET);

        if (BitsToSigned32(m_recordPosition) >= BitsToSigned32(m_archiveEnd))
        {
            m_ioState = IO_SEEK;
            m_recordPosition-=((m_recordSize+1u)&~1u)+8u;
            const unsigned int rewind = m_recordPosition+
                (m_signature == ResourceType::Resource?24u:8u);
            (void)std::fseek(m_file, BitsToSigned32(rewind), SEEK_SET);
            return 2;
        }

        (void)Read(&m_recordType, 4u);
        (void)Read(&m_recordSize, 4u);
        if (m_signature == ResourceType::Resource)
        {
            (void)Read(&m_recordFlags, 4u);
            if ((m_recordFlags&0x80000000u) != 0u)
            {
                (void)Read(&m_packedSize, 4u);
                (void)Read(&m_recordDataSize, 4u);
            }
            else
            {
                m_recordDataSize = m_recordFlags;
                m_recordFlags = 0u;
            }
            m_dataPosition = static_cast<unsigned int>(std::ftell(m_file));
            (void)Read(&m_subRecordSize, 4u);
        }
        if (type == m_recordType || type == ResourceType::Any) return 0;
    }
}


int RESOURCE::GoNextSub(unsigned int type)
{
    if (m_file == 0) return -1;

    m_dataPosition+=m_subRecordSize+4u;
    const unsigned int recordEnd = m_recordPosition+m_recordSize+8u;
    if (BitsToSigned32(m_dataPosition) >= BitsToSigned32(recordEnd))
        return GoNext(type);

    m_ioState = IO_SEEK;
    (void)std::fseek(m_file, BitsToSigned32(m_dataPosition), SEEK_SET);
    (void)Read(&m_subRecordSize, 4u);
    return 0;
}


int RESOURCE::PreAppend(unsigned int type, FILTER* filter)
{
    if (m_file == 0) return -1;
    if (filter) m_recordFlags|=0x100u;

    while (GoNext(ResourceType::Any) == 0)
    {
    }
    if (m_recordType != type)
    {
        m_recordType = type;
        m_archiveEnd+=20u;
        m_recordPosition+=((m_recordSize+1u)&~1u)+8u;
        m_recordDataSize = 0u;
        m_recordFlags = 0u;
        m_recordSize = 12u;
    }

    m_subRecordSize = 0u;
    m_packedSize = 0u;
    m_dataPosition = m_recordPosition+m_recordSize+8u;
    m_ioState = IO_SEEK;
    (void)std::fseek(m_file, BitsToSigned32(m_dataPosition+4u), SEEK_SET);
    return 0;
}


void RESOURCE::PostAppend()
{
    m_flags|=1u;
    m_recordFlags|=0x80000000u;
    ++m_recordDataSize;

    const unsigned int oldDataPosition = m_dataPosition;
    const unsigned int payloadSize = static_cast<unsigned int>(std::ftell(m_file))-oldDataPosition-4u;
    m_subRecordSize = payloadSize;
    m_ioState = IO_SEEK;
    m_recordSize+=payloadSize+4u;
    m_archiveEnd+=payloadSize+4u;


    (void)std::fseek(m_file, BitsToSigned32(oldDataPosition), SEEK_SET);
    m_dataPosition+=m_subRecordSize+4u;
    (void)Write(&m_subRecordSize, 4u);

    m_ioState = IO_SEEK;
    (void)std::fseek(m_file, BitsToSigned32(m_recordPosition), SEEK_SET);
    (void)Write(&m_recordType, 4u);
    (void)Write(&m_recordSize, 4u);
    (void)Write(&m_recordFlags, 4u);
    (void)Write(&m_packedSize, 4u);
    (void)Write(&m_recordDataSize, 4u);
    m_recordFlags&=~0x100u;
}


int RESOURCE::Copy(RESOURCE* source, unsigned int type)
{
    if (m_file == 0) return 1;
    if (source->m_file == 0) return 1;
    if (source->GoBegin(type) != 0) return 0;

    (void)PreAppend(type, 0);
    m_recordDataSize = 0u;
    unsigned int bytes = source->m_recordSize;
    m_recordDataSize = source->m_recordDataSize;
    for (;;)
    {
        void* const buffer = CrtNew(bytes);
        if (buffer == 0) return 1;
        (void)source->Read(buffer, bytes);
        (void)Write(buffer, bytes);
        CrtFreePointer(buffer);
        if (source->GoNext(type) != 0)
        {
            --m_recordDataSize;
            PostAppend();
            return 0;
        }
        const unsigned int sourceCount = source->m_recordDataSize;
        const unsigned int currentCount = m_recordDataSize;
        bytes = source->m_recordSize;
        m_recordDataSize = currentCount+sourceCount;
    }
}


int RESOURCE::GetNoSubRes(unsigned int type)
{
    if (m_file == 0) return 0;
    unsigned int total = 0u;
    if (GoBegin(type) != 0) return 0;
    do
    {
        total+=m_recordDataSize;
    }
    while (GoNext(type) == 0);
    return static_cast<int>(total);
}


void RESOURCE::Error(TYPE_ERROR type, const STRING& subject, unsigned long errorCode)
{
    if (::Error == 0) return;
    STRING objectFormat("RES '%s' '%.4s'");
    ::Error->Error(objectFormat, type, subject, errorCode,
        m_name.value,
        reinterpret_cast<const char*>(&m_recordType));
}

int RESOURCE::SubLoad(void** outBuffer, FILTER* filter)
{
    (void)filter;

    if (m_file == 0)
    {
        STRING subject("file not opened");
        Error(E_LOAD, subject, 0u);
        return 0;
    }

    if (static_cast<int>(m_subRecordSize) <= 0)
    {
        STRING subject("SubLoad");
        Error(E_SECTION, subject, 0u);
        return 0;
    }

    void* const payload = CrtNew(m_subRecordSize);
    *outBuffer = payload;
    if (payload == 0)
    {
        STRING subject("Subload data");
        const unsigned long size = m_subRecordSize;
        Error(E_MEMORY, subject, size);
        return 0;
    }

    if (Read(payload, m_subRecordSize) != 0)
    {
        STRING subject("Subload");
        Error(E_LOAD, subject, 0u);
    }
    return static_cast<int>(m_subRecordSize);
}


int RESOURCE::Load(unsigned int type, void** outBuffer, int recordSize)
{
    if (m_file == 0)
    {
        {
            STRING subject("file not opened");
            Error(E_LOAD, subject, 0u);
        }
        (void)std::exit(1);
    }

    const int count = GetNoSubRes(type);
    if (count == 0)
    {
        {
            STRING subject("Load");
            Error(E_SECTION, subject, type);
        }
        (void)std::exit(1);
    }

    (void)GoBegin(type);
    if (*outBuffer == 0)
    {
        *outBuffer = CrtNew(static_cast<unsigned int>(count)*static_cast<unsigned int>(recordSize));
    }
    else
    {
        STRING subject("Already loaded");
        Error(E_LOAD, subject, 0u);
    }

    if (*outBuffer == 0)
    {
        ::Error->LogExit(STRING("ResLoad::type=%.4s no_sub=%i Not enough Memory"),
            reinterpret_cast<const char*>(&type), count);
    }

    if (count>0)
    {
        int remaining = count;
        unsigned int offset = 0u;
        do
        {
            (void)Read(static_cast<unsigned char*>(*outBuffer)+offset, static_cast<unsigned int>(recordSize));
            (void)GoNextSub(type);
            offset+=static_cast<unsigned int>(recordSize);
            --remaining;
        }
        while (remaining != 0);
    }
    return count;
}



class ResourceFacetLock
{
public:
    ResourceFacetLock()
{
    Msvc6Runtime::StaticLockConstruct(&m_storage);
}
    ~ResourceFacetLock()
    {
        Msvc6Runtime::StaticLockDestruct(&m_storage);
    }
private:
    unsigned int m_storage;
};

static void RetainStaticRef(MsvcLocaleFacetRef* value)
{
    value->Retain();
}
static void ReleaseStaticRef(MsvcLocaleFacetRef* value)
{
    MsvcLocaleFacetRef* released;
    {
        ResourceFacetLock lock;
        unsigned int referenceCount = value->m_refCount04;
        if (referenceCount>0u && referenceCount<0xFFFFFFFFu)
        {
            --referenceCount;
            value->m_refCount04 = referenceCount;
        }
        released = value->m_refCount04 == 0u?value:0;
    }
    if (released) Msvc6::DeleteFacet(released);
}


void MsvcLocaleFacetRef::Retain()
{


    unsigned int staticLock = 0u;
    Msvc6Runtime::StaticLockConstruct(&staticLock);
    if (m_refCount04 != 0xFFFFFFFFu) ++m_refCount04;
    Msvc6Runtime::StaticLockDestruct(&staticLock);
}

void RecordDBTidyCtypeFacet()
{
    ResourceFacetLock lock;
    MsvcLocaleFacetRef* const value = reinterpret_cast<MsvcLocaleFacetRef*>(
        Msvc6::FacetState<Msvc6::CharCtype>::_Facsav);
    ReleaseStaticRef(value);
    Msvc6::FacetState<Msvc6::CharCtype>::_Facsav = 0;
}

void RecordDBTidyNumPutFacet()
{
    ResourceFacetLock lock;
    MsvcLocaleFacetRef* const value = reinterpret_cast<MsvcLocaleFacetRef*>(
        Msvc6::FacetState<Msvc6::CharNumPut>::_Facsav);
    ReleaseStaticRef(value);
    Msvc6::FacetState<Msvc6::CharNumPut>::_Facsav = 0;
}

void RecordDBTidyNumpunctFacet()
{
    ResourceFacetLock lock;
    MsvcLocaleFacetRef* const value = reinterpret_cast<MsvcLocaleFacetRef*>(
        Msvc6::FacetState<Msvc6::CharNumpunct>::_Facsav);
    ReleaseStaticRef(value);
    Msvc6::FacetState<Msvc6::CharNumpunct>::_Facsav = 0;
}

void RecordDBTidyCodecvtFacet()
{
    ResourceFacetLock lock;
    MsvcLocaleFacetRef* const value = reinterpret_cast<MsvcLocaleFacetRef*>(
        Msvc6::FacetState<Msvc6::CharCodecvt>::_Facsav);
    ReleaseStaticRef(value);
    Msvc6::FacetState<Msvc6::CharCodecvt>::_Facsav = 0;
}

MsvcLocaleFacetRef* RecordDBSaveCtypeFacet(MsvcLocaleFacetRef* value)
{
    ResourceFacetLock lock;
    Msvc6::FacetState<Msvc6::CharCtype>::_Facsav = reinterpret_cast<Msvc6::CtypeData*>(value);
    RetainStaticRef(value);
    (void)RegisterCrtAtExit(RecordDBTidyCtypeFacet);
    return value;
}

MsvcLocaleFacetRef* RecordDBSaveNumPutFacet(MsvcLocaleFacetRef* value)
{
    ResourceFacetLock lock;
    Msvc6::FacetState<Msvc6::CharNumPut>::_Facsav = reinterpret_cast<Msvc6::FacetData*>(value);
    RetainStaticRef(value);
    (void)RegisterCrtAtExit(RecordDBTidyNumPutFacet);
    return value;
}

MsvcLocaleFacetRef* RecordDBSaveNumpunctFacet(MsvcLocaleFacetRef* value)
{
    ResourceFacetLock lock;
    Msvc6::FacetState<Msvc6::CharNumpunct>::_Facsav = reinterpret_cast<Msvc6::NumpunctData<char>*>(value);
    RetainStaticRef(value);
    (void)RegisterCrtAtExit(RecordDBTidyNumpunctFacet);
    return value;
}

MsvcLocaleFacetRef* RecordDBSaveCodecvtFacet(MsvcLocaleFacetRef* value)
{
    ResourceFacetLock lock;
    Msvc6::FacetState<Msvc6::CharCodecvt>::_Facsav = reinterpret_cast<Msvc6::LocaleStampData*>(value);
    RetainStaticRef(value);
    (void)RegisterCrtAtExit(RecordDBTidyCodecvtFacet);
    return value;
}



