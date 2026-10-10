#pragma once

struct VECTOR2
{
    float x;
    float y;

    VECTOR2()
    {
    }
    VECTOR2(int xValue, int yValue) : x(static_cast<float>(xValue)), y(static_cast<float>(yValue))
    {
    }
    VECTOR2(float xValue, float yValue) : x(xValue), y(yValue)
    {
    }
    VECTOR2(const VECTOR2& value) : x(value.x), y(value.y)
    {
    }

    VECTOR2 operator*(const VECTOR2& value) const;
    VECTOR2 operator*(float scale) const;
    VECTOR2 operator-(const VECTOR2& value) const;
    VECTOR2 operator+(const VECTOR2& value) const;
    VECTOR2 operator/(float divisor) const;
    float NearLength() const;
    float Length() const;
    int operator<(const VECTOR2& value) const;
    int operator>(const VECTOR2& value) const;
    int operator>=(const VECTOR2& value) const;
    int operator<=(const VECTOR2& value) const;
};

struct VECTOR
{
    float x;
    float y;
    float z;

    VECTOR()
    {
    }
    VECTOR(int xValue,int yValue,int zValue);
    VECTOR(float xValue,float yValue,float zValue);
    VECTOR(const VECTOR& value);
    VECTOR(const VECTOR2& xyValue,float zValue);

    VECTOR operator*(float scale) const;
    VECTOR operator-(const VECTOR& value) const;
    VECTOR operator+(const VECTOR& value) const;
    VECTOR2& XY();
    VECTOR2 ViewCoor() const;
    float NearLength() const;
};

class RGB16
{
public:
    static void SetChannelMasks(bool rgb565);
    static unsigned short Pack(unsigned int color);
    static unsigned int Expand(unsigned short color);

private:
    static unsigned int rMask;
    static unsigned int gMask;
    static int rShift;
    static int gShift;
};

class GAMMA
{
public:
    union
    {
        struct { unsigned int modulate; unsigned int add; };
        unsigned char value[8];
    };

    enum GAMMA_CREATE : int;

    GAMMA()
    {
    }
    GAMMA(const GAMMA& source) : modulate(source.modulate), add(source.add)
    {
    }
    GAMMA(GAMMA_CREATE create,unsigned long packed);
    GAMMA(int alpha,int red,int green,int blue);
    GAMMA(GAMMA left,GAMMA right);
    const GAMMA& operator=(const GAMMA& source);
    const GAMMA& SetRed(int value);
    const GAMMA& SetGreen(int value);
    const GAMMA& SetBlue(int value);
    int Blue() const;
};

class ANGLE
{
public:
    unsigned char value;

    ANGLE()
    {
    }
    explicit ANGLE(unsigned char angleValue) : value(angleValue)
    {
    }
    ANGLE(const ANGLE& angleValue);
    explicit ANGLE(const VECTOR2& vectorValue);

    const ANGLE& operator=(const ANGLE& angleValue) { value=angleValue.value; return *this; }
    const ANGLE operator+(const ANGLE& angleValue) const;
    const ANGLE operator-(const ANGLE& angleValue) const;
    const ANGLE GetInversed() const;
    float Sin() const;
    float Cos() const;
    float SinY() const;
    float CosY() const;
    static void InitializeYTables(float scale);
    VECTOR2 RotateY(const VECTOR2& vectorValue) const;

private:
    static float SinTable[256];
    static float CosTable[256];
    static float SinYTable[256];
    static float CosYTable[256];
};

extern float FSin[256];

VECTOR2 VAbs(const VECTOR2& value);

ANGLE Decart2Polar(float x,float y);
ANGLE Decart2Polar(float x,float y,float* length);



extern unsigned char CrtStaticInitializationGuards[4];

int AS2RuntimeInitializeStandalone();


#include <cstdlib>
#include <cstdint>
#include <cstdio>
#if defined(_WIN32)
#include <io.h>
#else
extern "C" int _fileno(std::FILE*);
extern "C" long _filelength(int);
#endif

#include <cstring>
#include <cctype>
#include <ctime>

typedef unsigned int (__cdecl *IosEventCallbackFunction)(unsigned int,unsigned int,unsigned int);

unsigned int __cdecl CrtIsAlpha(unsigned int ch);
unsigned int __cdecl CrtIsDigit(unsigned int ch);
unsigned int __cdecl CrtIsAlnum(unsigned int ch);
unsigned int __cdecl CrtIsSpace(unsigned int ch);
static inline unsigned int CrtAtoiText(const char* text)
{
    const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text);
    while (CrtIsSpace(*cursor) != 0u) ++cursor;
    const unsigned char sign = *cursor;
    if (sign == '-' || sign == '+') ++cursor;
    unsigned int value = 0u;
    while (CrtIsDigit(*cursor) != 0u)
    {
        value = value*10u+static_cast<unsigned int>(*cursor)-static_cast<unsigned int>('0');
        ++cursor;
    }
    return sign == '-' ? 0u-value : value;
}

void* __cdecl CrtAllocate(unsigned int size);
void* __cdecl CrtNew(unsigned int size);
static inline unsigned int CrtRealloc(unsigned int memory, unsigned int size)
{
    return static_cast<unsigned int>(reinterpret_cast<unsigned long>(std::realloc(reinterpret_cast<void*>(static_cast<unsigned long>(memory)), size)));
}
static inline unsigned int CrtFree(unsigned int memory)
{
    std::free(reinterpret_cast<void*>(static_cast<unsigned long>(memory)));
    return 0u;
}
static inline void CrtFreePointer(void* memory)
{
    std::free(memory);
}
static inline unsigned int CrtStrncmp(unsigned int leftAddress, unsigned int rightAddress, unsigned int byteCount)
{
    const int comparison = std::strncmp(reinterpret_cast<const char*>(static_cast<unsigned long>(leftAddress)),
        reinterpret_cast<const char*>(static_cast<unsigned long>(rightAddress)), byteCount);
    return comparison<0?0xFFFFFFFFu:(comparison>0?1u:0u);
}

static inline unsigned int CrtStrchr(unsigned int text, unsigned int ch)
{
    const char* result = std::strchr(reinterpret_cast<const char*>(static_cast<unsigned long>(text)), static_cast<int>(ch));
    return static_cast<unsigned int>(reinterpret_cast<unsigned long>(result));
}
static inline unsigned int CrtMemmove(unsigned int destinationAddress, unsigned int sourceAddress, unsigned int byteCount)
{
    return static_cast<unsigned int>(reinterpret_cast<unsigned long>(std::memmove(reinterpret_cast<void*>(static_cast<unsigned long>(destinationAddress)), reinterpret_cast<const void*>(static_cast<unsigned long>(sourceAddress)), byteCount)));
}
unsigned int __cdecl CrtLowercase(unsigned int textAddress);
unsigned int __cdecl CrtUppercase(unsigned int textAddress);
static inline unsigned int CrtItoa(int value, unsigned int output, unsigned int radix)
{
    char* cursor = reinterpret_cast<char*>(static_cast<unsigned long>(output));
    unsigned int remaining = static_cast<unsigned int>(value);
    if (value<0 && radix == 10u)
    {
        *cursor++='-';
        remaining = 0u-remaining;
    }
    char* first = cursor;
    do
    {
        const unsigned int digit = remaining%radix;
        remaining/=radix;
        *cursor++=static_cast<char>(digit>9u?digit+('a'-10):digit+'0');
    }
    while (remaining != 0u);
    *cursor = 0;
    --cursor;
    do
    {
        const char digit = *first;
        *first = *cursor;
        *cursor = digit;
        if (first == cursor) break;
        ++first;
        --cursor;
    }
    while (first<cursor);
    return output;
}
static inline unsigned int CrtExit(unsigned int code)
{
    std::exit(static_cast<int>(code));
    return 0u;
}

unsigned int __cdecl CrtFileGetC(unsigned int stream);
unsigned int __cdecl CrtFileClose(unsigned int stream);
unsigned int __cdecl CrtFileOpen(unsigned int path,unsigned int mode);
unsigned int __cdecl CrtFileSeek(unsigned int stream,unsigned int offset,unsigned int origin);
unsigned int __cdecl CrtFileFlush(unsigned int stream);
unsigned int __cdecl CrtFileWrite(unsigned int input,unsigned int size,unsigned int count,unsigned int stream);
unsigned int __cdecl CrtFilePutC(unsigned int ch,unsigned int stream);
unsigned int __cdecl CrtFileUngetC(unsigned int ch,unsigned int stream);
int __cdecl CrtRemove(const char* path);
typedef void (__cdecl *CrtExitCallback)();
int __cdecl RegisterCrtAtExitCallback(CrtExitCallback callback);
void __cdecl MapStaticCleanupNoOp();
extern "C" void __cdecl MylibNoOp(int);
unsigned int __cdecl CrtStaticLockConstruct(unsigned int self);
void __cdecl CrtStaticLockCleanup();
unsigned int __cdecl CrtStaticLockDestruct(unsigned int self);
unsigned int __cdecl CrtControlFp(unsigned int newControl,unsigned int mask);
struct _finddata32_t;
intptr_t __cdecl CrtFindFirst(const char* path, _finddata32_t* data);
int __cdecl CrtFindClose(intptr_t handle);
static inline unsigned long AddressToUIntPtr(unsigned int addressBits)
{
    return static_cast<unsigned long>(addressBits);
}
static inline unsigned int PtrToU32(const void* pointer){
#if defined(_MSC_VER) && defined(_M_IX86)
 return reinterpret_cast<unsigned int>(pointer);
#else
 return static_cast<unsigned int>(reinterpret_cast<unsigned long>(pointer));
#endif
}
static inline unsigned char* U32ToPtr(unsigned int addressBits)
{
    return reinterpret_cast<unsigned char*>(AddressToUIntPtr(addressBits));
}

namespace StringGlobals
{
extern char EmptyTextCharacter;
inline const char* EmptyText()
{
    return &EmptyTextCharacter;
}

char* EmptySentinel();
}

static inline unsigned int ReadGlobalU32(unsigned int address)
{
    return *reinterpret_cast<unsigned int*>(AddressToUIntPtr(address));
}
static inline unsigned char ReadGlobalU8(unsigned int address)
{
    return *reinterpret_cast<unsigned char*>(AddressToUIntPtr(address));
}
static inline void WriteGlobalU32(unsigned int address, unsigned int value)
{
    *reinterpret_cast<unsigned int*>(AddressToUIntPtr(address)) = value;
}
static inline void WriteGlobalU8(unsigned int address, unsigned char value)
{
    *reinterpret_cast<unsigned char*>(AddressToUIntPtr(address)) = value;
}

static inline unsigned int RegisterCrtAtExit(CrtExitCallback callback)
{
    return static_cast<unsigned int>(RegisterCrtAtExitCallback(callback));
}


static inline unsigned int MinUnsigned(unsigned int left, unsigned int right)
{
    return left<right?left:right;
}
static inline unsigned int FloatBitsU32(float value)
{
    union Bits
    {
        float f;
        unsigned int u;
    }
    bits;
    bits.f = value;
    return bits.u;
}
static inline void CopyBytes(void* destination, const void* source, unsigned int count)
{
    unsigned char* destinationBytes = static_cast<unsigned char*>(destination);
    const unsigned char* sourceBytes = static_cast<const unsigned char*>(source);
    for (unsigned int byteIndex = 0;byteIndex<count;++byteIndex)destinationBytes[byteIndex] = sourceBytes[byteIndex];
}

static inline unsigned int ReadU32(const void* source, unsigned int byteOffset)
{
    return *reinterpret_cast<const unsigned int*>(static_cast<const unsigned char*>(source)+byteOffset);
}
static inline int ReadS32(const void* source, unsigned int byteOffset)
{
    return *reinterpret_cast<const int*>(static_cast<const unsigned char*>(source)+byteOffset);
}
static inline unsigned short ReadU16(const void* source, unsigned int byteOffset)
{
    return *reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(source)+byteOffset);
}

static inline unsigned char ReadU8(const void* source, unsigned int byteOffset)
{
    return *reinterpret_cast<const unsigned char*>(static_cast<const unsigned char*>(source)+byteOffset);
}
static inline float ReadF32(const void* source, unsigned int byteOffset)
{
    return *reinterpret_cast<const float*>(static_cast<const unsigned char*>(source)+byteOffset);
}
static inline void WriteU32(void* destination, unsigned int byteOffset, unsigned int value)
{
    *reinterpret_cast<unsigned int*>(static_cast<unsigned char*>(destination)+byteOffset) = value;
}

static inline void WriteF32(void* destination, unsigned int byteOffset, float value)
{
    *reinterpret_cast<float*>(static_cast<unsigned char*>(destination)+byteOffset) = value;
}
static inline unsigned int AbsDiffU8(unsigned char left, unsigned char right)
{
    unsigned int forwardDistance = static_cast<unsigned int>(static_cast<unsigned char>(left-right)), reverseDistance = static_cast<unsigned int>(static_cast<unsigned char>(right-left));
    return forwardDistance<reverseDistance?forwardDistance:reverseDistance;
}

static inline float ReadAbsoluteF32(unsigned int address)
{
    return *reinterpret_cast<const float*>(AddressToUIntPtr(address));
}

static inline int BitsToSigned32(unsigned int value)
{
    union Bits
    {
        unsigned int u;
        int s;
    }
    bits;
    bits.u = value;
    return bits.s;
}
static inline unsigned int Signed32ToBits(int value)
{
    union Bits
    {
        int s;
        unsigned int u;
    }
    bits;
    bits.s = value;
    return bits.u;
}
static inline int AddSigned32Wrap(int left, int right)
{
    return BitsToSigned32(static_cast<unsigned int>(left)+static_cast<unsigned int>(right));
}
static inline int SubSigned32Wrap(int left, int right)
{
    return BitsToSigned32(static_cast<unsigned int>(left)-static_cast<unsigned int>(right));
}
static inline int MultiplySigned32Wrap(int left, int right)
{
    return BitsToSigned32(static_cast<unsigned int>(left)*static_cast<unsigned int>(right));
}


static inline void FreeOwnedStringAddress(unsigned int address)
{
    if (address && address != PtrToU32(StringGlobals::EmptySentinel()))CrtFreePointer(U32ToPtr(address));
}
static inline unsigned int CStringLength(const char* text)
{
    unsigned int length = 0u;
    if (text)while (text[length])++length;
    return length;
}
static inline bool CStringEqual(const char* left, const char* right)
{
    if (!left || !right)return left == right;
    while (*left && *left == *right)
    {
        ++left;
        ++right;
    }
    return *left == *right;
}

void __cdecl DebugAssert(int severity,const char* expression,const char* sourceFile,int sourceLine);

static inline int AbsInt(int value)
{
    return value<0?-value:value;
}
static inline float AbsFloat(float value)
{
    return value<0.0f?-value:value;
}
static inline int ClampByteInt(int value)
{
    return value<0?0:(value>255?255:value);
}





#include <stddef.h>



#include <cstddef>
#include <cstdint>

namespace as2 { namespace core {

template<class Constructor>
inline void constructArray(void* first, std::uint32_t elementSize, std::uint32_t count, Constructor constructor)
{
    auto* bytes = static_cast<unsigned char*>(first);
    for (std::uint32_t i = 0; i < count; ++i)
        constructor(bytes + static_cast<std::size_t>(i) * elementSize);
}

template<class Destructor>
inline void destroyArray(void* first, std::uint32_t elementSize, std::uint32_t count, Destructor destructor)
{
    auto* bytes = static_cast<unsigned char*>(first);
    while (count != 0u)
    {
        --count;
        destructor(bytes + static_cast<std::size_t>(count) * elementSize);
    }
}

template<class Destructor, class Deallocator>
inline void* vectorDeletingDestructor(void* self, std::uint32_t flags, std::uint32_t elementSize, Destructor destructor, Deallocator deallocate)
{
    if ((flags & 0xFFu) & 2u)
    {
        auto* first = static_cast<unsigned char*>(self);
        auto* header = first - sizeof(std::uint32_t);
        const std::uint32_t count = *reinterpret_cast<std::uint32_t*>(header);
        destroyArray(first, elementSize, count, destructor);
        if ((flags & 0xFFu) & 1u)
            deallocate(header);
        return header;
    }

    destructor(self);
    if ((flags & 0xFFu) & 1u)
        deallocate(self);
    return self;
}

} }

#include <stddef.h>

class zLogConsole;

void __cdecl DebugAssert(int severity,
                                  const char* expression,
                                  const char* sourceFile,
                                  int sourceLine);

