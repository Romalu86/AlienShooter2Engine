#pragma once
#include "mylib.h"
#include "stream.h"
#include "myerror.h"
#include <stddef.h>
#include <cstdio>

namespace ResourceType
{
enum : unsigned int
{
    Any='A'|('N'<<8)|('Y'<<16)|(' '<<24),
    Resource='R'|('E'<<8)|('S'<<16)|(' '<<24),
    Riff='R'|('I'<<8)|('F'<<16)|('F'<<24),
    Constants='C'|('N'<<8)|('S'<<16)|('T'<<24),
    Map='M'|('A'<<8)|('P'<<16)|(' '<<24),
    Object='O'|('B'<<8)|('J'<<16)|(' '<<24),
    Data='D'|('A'<<8)|('T'<<16)|('A'<<24),
    Demo='D'|('E'<<8)|('M'<<16)|('O'<<24),
    Weapons='W'|('E'<<8)|('A'<<16)|('P'<<24),
    Graphics='G'|('R'<<8)|('P'<<16)|('H'<<24),
    Header='H'|('E'<<8)|('A'<<16)|('D'<<24),
    Grid='G'|('R'<<8)|('I'<<16)|('D'<<24),
    Sprites='S'|('P'<<8)|('R'<<16)|(' '<<24),
    SpriteData='S'|('P'<<8)|('R'<<16)|('D'<<24),
    Player='P'|('L'<<8)|('A'<<16)|('Y'<<24),
    Groups='G'|('R'<<8)|('O'<<16)|('U'<<24),
    Video='V'|('I'<<8)|('D'<<16)|(' '<<24),
    Menu='M'|('E'<<8)|('N'<<16)|('U'<<24),
    SpriteIndices='S'|('P'<<8)|('R'<<16)|('I'<<24),
    Palette='P'|('A'<<8)|('L'<<16)|(' '<<24),
    Frame='C'|('A'<<8)|('D'<<16)|('R'<<24)
};
}

class FILTER;

class RESOURCE : public STREAM {
    friend class MAP;
    enum IO_STATE : unsigned int { IO_WRITE=0, IO_READ=1, IO_SEEK=2 };
public:
    enum OPEN_MODE { OPEN_MODE_READ=0, OPEN_MODE_APPEND=1, OPEN_MODE_WRITE=2 };
    RESOURCE();
    RESOURCE(OPEN_MODE mode,const STRING& fileName,unsigned int type);
    virtual ~RESOURCE() noexcept(false);
    int Open(std::FILE* file,unsigned int type);
    void Close();
    virtual int Read(void* buffer,unsigned int count);
    virtual int Write(const void* buffer,unsigned int count);

    int OpenForRead(const STRING& fileName,unsigned int type);
    int OpenForWrite(const STRING& fileName,unsigned int type);
    int OpenForAppend(const STRING& fileName,unsigned int type);

    int GoBegin(unsigned int type);
    int GoNext(unsigned int type);
    int GoNextSub(unsigned int type);
    int PreAppend(unsigned int type,FILTER* filter=0);
    void PostAppend();
    int Copy(RESOURCE* source,unsigned int type);
    int GetNoSubRes(unsigned int type);
    STRING GetFileName() const;
    int SubLoad(void** outBuffer,FILTER* filter=0);
    int Load(unsigned int type,void** outBuffer,int recordSize);
private:
    void Error(TYPE_ERROR type,const STRING& subject,unsigned long errorCode);
public:

    std::FILE* FileHandle() const
{
    return m_file;
}
    unsigned int PackedSize() const
    {
        return m_packedSize;
    }
    unsigned int RecordSize() const
    {
        return m_recordSize;
    }
    int DataPosition() const
    {
        return static_cast<int>(m_dataPosition);
    }
    int SubRecordSize() const
    {
        return static_cast<int>(m_subRecordSize);
    }
    unsigned int RecordType() const
    {
        return m_recordType;
    }
    void SetIoState(unsigned int state)
    {
        m_ioState = static_cast<IO_STATE>(state);
    }

private:

    unsigned int m_flags;
    IO_STATE     m_ioState;
    STRING       m_name;
    unsigned int m_signature;
    unsigned int m_recordSize;
    unsigned int m_recordPosition;
    unsigned int m_basePosition;
    unsigned int m_archiveEnd;
    unsigned int m_recordFlags;
    unsigned int m_recordDataSize;
    unsigned int m_dataPosition;
    unsigned int m_subRecordSize;
    unsigned int m_packedSize;
    std::FILE*      m_file;
    unsigned int m_recordType;
};

