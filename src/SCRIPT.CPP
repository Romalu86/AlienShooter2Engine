#include <cstdio>
#include "myerror.h"
#include "basetype.h"


#include "mylib.h"
#include "sprite.h"
#include "map.h"
#include "script.h"
#include "script_exec_dispatch.h"
#include "stream.h"
#include <stdlib.h>
#include <string.h>
#include <new>
#include <intrin.h>

int __cdecl ScriptExecFunc(int command);

namespace {
int parsingArrayArgument = 0;

enum ScriptStackFlags {
    SCRIPT_STACK_STRING      = 0x01,
    SCRIPT_STACK_INT         = 0x02,
    SCRIPT_STACK_ARRAY       = 0x04,
    SCRIPT_STACK_HAS_VALUE   = 0x08,
    SCRIPT_STACK_OBJECT      = 0x10,
    SCRIPT_STACK_DYNAMIC     = 0x20,
    SCRIPT_STACK_WRITE_CHAR  = 0x40
};

enum ScriptOpcode {
    SCRIPT_OP_PUSH_INT = 1,
    SCRIPT_OP_PUSH_STRING = 2,
    SCRIPT_OP_NEGATE = 3,
    SCRIPT_OP_BITNOT = 4,
    SCRIPT_OP_LOGNOT = 5,
    SCRIPT_OP_DIVIDE = 6,
    SCRIPT_OP_MODULO = 7,
    SCRIPT_OP_ADD = 8,
    SCRIPT_OP_SUBTRACT = 9,
    SCRIPT_OP_BIT_XOR = 10,
    SCRIPT_OP_BIT_OR = 11,
    SCRIPT_OP_BIT_AND = 12,
    SCRIPT_OP_EQUAL = 13,
    SCRIPT_OP_LOGIC_OR = 14,
    SCRIPT_OP_GREATER = 15,
    SCRIPT_OP_LESS = 16,
    SCRIPT_OP_GREATER_EQUAL = 17,
    SCRIPT_OP_LESS_EQUAL = 18,
    SCRIPT_OP_MULTIPLY = 19,
    SCRIPT_OP_NOT_EQUAL = 20,
    SCRIPT_OP_LOGIC_AND = 21,
    SCRIPT_OP_SHIFT_RIGHT = 22,
    SCRIPT_OP_SHIFT_LEFT = 23,
    SCRIPT_OP_IF = 24,
    SCRIPT_OP_LINE = 25,
    SCRIPT_OP_POP = 26,
    SCRIPT_OP_JUMP = 28,
    SCRIPT_OP_IFF = 29,
    SCRIPT_OP_CALL = 30,
    SCRIPT_OP_RETURN = 31,
    SCRIPT_OP_POST_INC = 32,
    SCRIPT_OP_POST_DEC = 33,
    SCRIPT_OP_PRE_INC = 34,
    SCRIPT_OP_PRE_DEC = 35,
    SCRIPT_OP_READ = 36,
    SCRIPT_OP_ADDRESS = 37,
    SCRIPT_OP_ASSIGN = 38,
    SCRIPT_OP_COMPOUND = 39,
    SCRIPT_OP_ARRAY_INDEX = 40,
    SCRIPT_OP_CAST_STRING = 41,
    SCRIPT_OP_CAST_INT = 42,
    SCRIPT_OP_CAST_OBJECT = 43,
    SCRIPT_OP_SHORT_CIRCUIT_AND = 44,
    SCRIPT_OP_SHORT_CIRCUIT_OR = 45,
};
}

int SCRIPTSTACK::Int() const
{
    if ((m_flags & 1u) == 0)
        return m_intValue;

    const char* numericText = m_stringValue.m_value;
    const int firstTextChar = static_cast<int>(static_cast<signed char>(numericText[0]));
    if (CrtIsDigit(static_cast<unsigned int>(firstTextChar)) == 0)
    {
        int validNegative = 0;
        if (numericText[0] == '-')
        {
            const int secondTextChar = static_cast<int>(static_cast<signed char>(numericText[1]));
            validNegative = CrtIsDigit(static_cast<unsigned int>(secondTextChar)) != 0;
        }
        if (!validNegative)
        {
            MYERROR* const logger = ::Error;
            (void)logger->Log("!!!ERROR!!!SCRIPT: присваивание переменной int строки '%s'", numericText);
        }
    }

    if ((m_flags & 1u) == 0)
        return m_intValue;
    numericText = m_stringValue.m_value;
    if (numericText[1] != 'x')
        return static_cast<int>(CrtAtoiText(numericText));

    int parsedValue;
    (void)std::sscanf(numericText, "%i", &parsedValue);
    return parsedValue;
}


void SCRIPT::SetNoElement(int elementOrParameterCount)
{

    m_functionList.Record(m_functionList.m_count-1).m_value.m_elementOrParameterCount = elementOrParameterCount;
}

void SCRIPT::AddDataCommand(unsigned char opcode, int operand)
{
    unsigned int cursor = m_bytecodeCursor;
    m_bytecode[cursor] = opcode;
    ++cursor;
    *reinterpret_cast<unsigned int*>(m_bytecode+cursor) = static_cast<unsigned int>(operand);
    m_bytecodeCursor = cursor+4u;
}

void SCRIPT::AddDataCommand(unsigned char opcodeByte)
{
    if (m_errorLatch != 0) return;
    m_bytecode[m_bytecodeCursor] = opcodeByte;
    ++m_bytecodeCursor;
}

int SCRIPT::DeletePointerToObject(void* objectToClear)
{
    const int objectAddressBits = static_cast<int>(PtrToU32(objectToClear));
    int clearedEntryCount = 0;
    for (int i = 0;i<m_stack.m_count;++i)
    {
        SCRIPTSTACK& stackEntry = m_stack.m_data[i];
        if ((stackEntry.m_flags&0x10u) != 0u && stackEntry.m_intValue == objectAddressBits)
        {
            stackEntry.m_intValue = 0;
            stackEntry.m_flags = static_cast<unsigned char>(stackEntry.m_flags&0xEFu);
            ++clearedEntryCount;
        }
    }
    return clearedEntryCount;
}

template<> NAMED_LIST_STRUCT<SCRIPTVAR>::~NAMED_LIST_STRUCT();
template<> NAMED_LIST_STRUCT<STRING>::~NAMED_LIST_STRUCT();


void*  NamedScriptVarVectorDeletingDtor(void* self, unsigned int flags)
{
    return as2::core::vectorDeletingDestructor(self, flags, sizeof(NAMED_LIST_STRUCT<SCRIPTVAR>),
        [](void* item){ static_cast<NAMED_LIST_STRUCT<SCRIPTVAR>*>(item)->~NAMED_LIST_STRUCT<SCRIPTVAR>(); },
        [](void* storage){ CrtFreePointer(storage); });
}

void*  NamedStringVectorDeletingDtor(void* self, unsigned int flags)
{
    return as2::core::vectorDeletingDestructor(self, flags, sizeof(NAMED_LIST_STRUCT<STRING>),
        [](void* item){ static_cast<NAMED_LIST_STRUCT<STRING>*>(item)->~NAMED_LIST_STRUCT<STRING>(); },
        [](void* storage){ CrtFreePointer(storage); });
}

void SCRIPT::slag()
{
    const int expressionStart = m_bytecodeCursor;
    mnog();
    if (m_errorLatch != 0)return;
    for (;;)
    {
        if (Word(const_cast<char*>("*")) != 0)
        {
            mnog();
            SetOperation(expressionStart, SCRIPT_OP_MULTIPLY);
            continue;
        }
        if (Word(const_cast<char*>("/")) != 0)
        {
            mnog();
            SetOperation(expressionStart, SCRIPT_OP_DIVIDE);
            continue;
        }
        if (Word(const_cast<char*>("%")) != 0)
        {
            mnog();
            SetOperation(expressionStart, SCRIPT_OP_MODULO);
            continue;
        }
        return;
    }
}


void SCRIPT::cmpslag()
{
    const int expressionStart = m_bytecodeCursor;
    slag();
    if (m_errorLatch != 0)return;
    for (;;)
    {
        if (Word(const_cast<char*>("+")) != 0)
        {
            slag();
            SetOperation(expressionStart, SCRIPT_OP_ADD);
            continue;
        }
        if (Word(const_cast<char*>("-")) != 0)
        {
            slag();
            SetOperation(expressionStart, SCRIPT_OP_SUBTRACT);
            continue;
        }
        return;
    }
}


void SCRIPT::logicslag()
{
    const int expressionStart = m_bytecodeCursor;
    cmpslag();
    if (m_errorLatch != 0)return;
    for (;;)
    {
        if (Word(const_cast<char*>(">=")) != 0)
        {
            cmpslag();
            AddDataCommand(SCRIPT_OP_GREATER_EQUAL);
            continue;
        }
        if (Word(const_cast<char*>(">>")) != 0)
        {
            cmpslag();
            SetOperation(expressionStart, SCRIPT_OP_SHIFT_RIGHT);
            continue;
        }
        if (Word(const_cast<char*>(">")) != 0)
        {
            cmpslag();
            AddDataCommand(SCRIPT_OP_GREATER);
            continue;
        }
        if (Word(const_cast<char*>("<=")) != 0)
        {
            cmpslag();
            AddDataCommand(SCRIPT_OP_LESS_EQUAL);
            continue;
        }
        if (Word(const_cast<char*>("<<")) != 0)
        {
            cmpslag();
            SetOperation(expressionStart, SCRIPT_OP_SHIFT_LEFT);
            continue;
        }
        if (Word(const_cast<char*>("<")) != 0)
        {
            cmpslag();
            AddDataCommand(SCRIPT_OP_LESS);
            continue;
        }
        if (Word(const_cast<char*>("==")) != 0)
        {
            cmpslag();
            AddDataCommand(SCRIPT_OP_EQUAL);
            continue;
        }
        if (Word(const_cast<char*>("!=")) != 0)
        {
            cmpslag();
            AddDataCommand(SCRIPT_OP_NOT_EQUAL);
            continue;
        }
        return;
    }
}


void SCRIPT::vyragAnd()
{
    const int expressionStart = m_bytecodeCursor;
    logicslag();
    if (m_errorLatch != 0)return;
    while (Word(const_cast<char*>("&")) != 0)
    {
        logicslag();
        SetOperation(expressionStart, SCRIPT_OP_BIT_AND);
    }
}


void SCRIPT::vyragXor()
{
    const int expressionStart = m_bytecodeCursor;
    vyragAnd();
    if (m_errorLatch != 0)return;
    while (Word(const_cast<char*>("^")) != 0)
    {
        vyragAnd();
        SetOperation(expressionStart, SCRIPT_OP_BIT_XOR);
    }
}


void SCRIPT::vyragOr()
{
    const int expressionStart = m_bytecodeCursor;
    vyragXor();
    if (m_errorLatch != 0)return;
    while (Word(const_cast<char*>("|")) != 0)
    {
        vyragXor();
        SetOperation(expressionStart, SCRIPT_OP_BIT_OR);
    }
}


void SCRIPT::vyragCmpAnd()
{
    vyragOr();
    if (m_errorLatch != 0) return;
    while (Word(const_cast<char*>("&&")) != 0)
    {
        m_bytecode[m_bytecodeCursor++] = SCRIPT_OP_SHORT_CIRCUIT_AND;
        const int patch = m_bytecodeCursor;
        m_bytecodeCursor += 4;
        vyragOr();
        m_bytecode[m_bytecodeCursor++] = SCRIPT_OP_LOGIC_AND;
        *reinterpret_cast<int*>(m_bytecode+patch) = m_bytecodeCursor-patch;
    }
}

void SCRIPT::vyrag(int parserMode)
{
    const int oldMode = m_parserMode;
    m_parserMode = parserMode;
    vyragCmpAnd();
    if (m_errorLatch != 0) return;
    while (Word(const_cast<char*>("||")) != 0)
    {
        AddDataCommand(SCRIPT_OP_SHORT_CIRCUIT_OR);
        const int patch = m_bytecodeCursor;
        m_bytecodeCursor += 4;
        vyragCmpAnd();
        m_bytecode[m_bytecodeCursor++] = SCRIPT_OP_LOGIC_OR;
        *reinterpret_cast<int*>(m_bytecode+patch) = m_bytecodeCursor-patch;
    }
    m_parserMode = oldMode;
}


void SCRIPT::vyrag_oper()
{
    vyrag(0);
    if (m_errorLatch != 0)return;
    for (;;)
    {
        AddDataCommand(SCRIPT_OP_LINE, m_sourceLine+1);
        if (Word(const_cast<char*>(",")) == 0u)return;
        vyrag(0);
        if (m_errorLatch != 0)return;
    }
}


SCRIPTSTACK::SCRIPTSTACK(const SCRIPTSTACK& source)
    : m_flags(source.m_flags),
      m_intValue(source.m_intValue),
      m_stringValue(source.m_stringValue)
{
}


void SCRIPTSTACK::Read(STREAM* stream)
{
    (void)stream->Read(&m_flags, 1u);
    if ((m_flags&0x08u) == 0u) return;
    if ((m_flags&0x01u) != 0u)
    {
        m_stringValue.Read(stream);
        const unsigned int stringLength = static_cast<unsigned int>(std::strlen(m_stringValue.m_value));
        for (int characterIndex = BitsToSigned32(stringLength-1u);characterIndex >= 0;--characterIndex)
        {
            char* const decodedText = m_stringValue.m_value;
            decodedText[characterIndex] = static_cast<char>(static_cast<unsigned char>(decodedText[characterIndex])^0x17u);
        }
        return;
    }
    (void)stream->Read(&m_intValue, 4u);
}


SCRIPTSTACK::SCRIPTSTACK()
    : m_flags(0), m_intValue(0), m_stringValue()
{
}

SCRIPTSTACK::SCRIPTSTACK(const STRING& stringValue, const STRING& tag)
    : m_flags(1), m_stringValue(stringValue)
{
    (void)tag;
}

SCRIPTSTACK::SCRIPTSTACK(int integerValue, const STRING& tag)
    : m_flags(2), m_intValue(integerValue), m_stringValue()
{
    (void)tag;
}

SCRIPTSTACK::~SCRIPTSTACK()
{
}

const SCRIPTSTACK& SCRIPTSTACK::operator = (const SCRIPTSTACK& source)
{
    if (this != &source){
        m_flags = static_cast<unsigned char>(source.m_flags&0x7Fu);
        m_intValue = source.m_intValue;
        m_stringValue = source.m_stringValue;
    }
    return *this;
}

template<>
NAMED_LIST_STRUCT<SCRIPTVAR>::NAMED_LIST_STRUCT()
    : m_name()
{

    m_value.m_type = 0;
    m_value.m_functionFlags = 0;
}

template<>
NAMED_LIST_STRUCT<SCRIPTVAR>::~NAMED_LIST_STRUCT()
{
}

template<>
NAMED_LIST_STRUCT<SCRIPTVAR>& NAMED_LIST_STRUCT<SCRIPTVAR>::operator = (const NAMED_LIST_STRUCT<SCRIPTVAR>& source)
{
    m_hash = source.m_hash;
    m_name = source.m_name;
    m_value.m_type = source.m_value.m_type;
    m_value.m_functionFlags = source.m_value.m_functionFlags;

    m_value.m_text = source.m_value.m_text;
    m_value.m_valueOrOffset = source.m_value.m_valueOrOffset;
    m_value.m_parameterStackBase = source.m_value.m_parameterStackBase;
    m_value.m_elementOrParameterCount = source.m_value.m_elementOrParameterCount;
    return *this;
}

template<>
NAMED_LIST_STRUCT<STRING>::NAMED_LIST_STRUCT()
    : m_name(), m_value()
{
}

template<>
NAMED_LIST_STRUCT<STRING>::~NAMED_LIST_STRUCT()
{
}

template<>
NAMED_LIST_STRUCT<STRING>& NAMED_LIST_STRUCT<STRING>::operator = (const NAMED_LIST_STRUCT<STRING>& source)
{
    m_hash = source.m_hash;
    m_name = source.m_name;
    m_value = source.m_value;
    return *this;
}

unsigned int __cdecl Script_StaticCleanupRegister01()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

template<>
int LIST<SCRIPTSTACK>::No() const
{
    return m_count;
}

template<>
int NAMED_LIST<SCRIPTVAR>::GetHash(const char* text)
{
    int hash = 0;
    int position = 1;
    while (*text)
    {
        const int rem = hash%999;
        const int term = MultiplySigned32Wrap(rem+position, static_cast<unsigned char>(*text));
        hash = AddSigned32Wrap(hash, term);
        ++position;
        ++text;
    }
    return hash%100000;
}

template<>
int NAMED_LIST<SCRIPTVAR>::Location(const STRING& name) const
{
    const char* const nameText = name.m_value;
    const int nameHash = GetHash(nameText);
    int recordIndex = BitsToSigned32(static_cast<unsigned int>(this->m_count)-1u);
    if (recordIndex<0) return -1;
    const NAMED_LIST_STRUCT<SCRIPTVAR>* candidateRecord = this->m_data+recordIndex;
    for (;;)
    {
        if (candidateRecord->m_hash == nameHash && std::strcmp(candidateRecord->m_name.m_value, nameText) == 0)
            return recordIndex;
        --recordIndex;
        if (recordIndex<0) return -1;
        --candidateRecord;
    }
}

template<>
void LIST<SCRIPTSTACK>::Expand(int requestedCapacity)
{
    if (requestedCapacity <= m_capacity) return;
    SCRIPTSTACK* const oldData = m_data;

    unsigned char* const allocation = static_cast<unsigned char*>(CrtNew(
        static_cast<unsigned int>(requestedCapacity)*sizeof(SCRIPTSTACK)+4u));
    SCRIPTSTACK* next = 0;
    if (allocation)
    {
        *reinterpret_cast<int*>(allocation) = requestedCapacity;
        next = reinterpret_cast<SCRIPTSTACK*>(allocation+4u);
        for (int i = 0;i<requestedCapacity;++i) new (next+i) SCRIPTSTACK();
    }
    m_data = next;
    if (!next)
    {
        ::Error->LogExit(
            STRING("!!!ERROR!!!::LIST: Not enough memory %i"), requestedCapacity);
    }
    if (oldData)
    {
        for (int i = 0;i<m_capacity;++i) m_data[i] = oldData[i];
        const int allocated = *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(oldData)-4);
        for (int i = allocated-1;i >= 0;--i) oldData[i].~SCRIPTSTACK();
        CrtFreePointer(reinterpret_cast<unsigned char*>(oldData)-4);
    }
    m_capacity = requestedCapacity;
}

template<>
void LIST<NAMED_LIST_STRUCT<SCRIPTVAR> >::Expand(int requestedCapacity)
{
    if (requestedCapacity <= m_capacity) return;
    NAMED_LIST_STRUCT<SCRIPTVAR>* const oldData = m_data;

    unsigned char* const allocation = static_cast<unsigned char*>(CrtNew(
        static_cast<unsigned int>(requestedCapacity)*sizeof(NAMED_LIST_STRUCT<SCRIPTVAR>)+4u));
    NAMED_LIST_STRUCT<SCRIPTVAR>* next = 0;
    if (allocation)
    {
        *reinterpret_cast<int*>(allocation) = requestedCapacity;
        next = reinterpret_cast<NAMED_LIST_STRUCT<SCRIPTVAR>*>(allocation+4u);
        for (int i = 0;i<requestedCapacity;++i) new (next+i) NAMED_LIST_STRUCT<SCRIPTVAR>();
    }
    m_data = next;
    if (!next)
    {
        ::Error->LogExit(
            STRING("!!!ERROR!!!::LIST: Not enough memory %i"), requestedCapacity);
    }
    if (oldData)
    {
        for (int i = 0;i<m_capacity;++i) m_data[i] = oldData[i];
        const int allocated = *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(oldData)-4);
        for (int i = allocated-1;i >= 0;--i) oldData[i].~NAMED_LIST_STRUCT<SCRIPTVAR>();
        CrtFreePointer(reinterpret_cast<unsigned char*>(oldData)-4);
    }
    m_capacity = requestedCapacity;
}

template<>
void LIST<NAMED_LIST_STRUCT<STRING> >::Expand(int requestedCapacity)
{
    if (requestedCapacity <= m_capacity) return;
    NAMED_LIST_STRUCT<STRING>* const oldData = m_data;

    unsigned char* const allocation = static_cast<unsigned char*>(CrtNew(
        static_cast<unsigned int>(requestedCapacity)*sizeof(NAMED_LIST_STRUCT<STRING>)+4u));
    NAMED_LIST_STRUCT<STRING>* next = 0;
    if (allocation)
    {
        *reinterpret_cast<int*>(allocation) = requestedCapacity;
        next = reinterpret_cast<NAMED_LIST_STRUCT<STRING>*>(allocation+4u);
        for (int i = 0;i<requestedCapacity;++i) new (next+i) NAMED_LIST_STRUCT<STRING>();
    }
    m_data = next;
    if (!next)
    {
        ::Error->LogExit(
            STRING("!!!ERROR!!!::LIST: Not enough memory %i"), requestedCapacity);
    }
    if (oldData)
    {
        for (int i = 0;i<m_capacity;++i) m_data[i] = oldData[i];
        const int allocated = *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(oldData)-4);
        for (int i = allocated-1;i >= 0;--i) oldData[i].~NAMED_LIST_STRUCT<STRING>();
        CrtFreePointer(reinterpret_cast<unsigned char*>(oldData)-4);
    }
    m_capacity = requestedCapacity;
}

template<>
void LIST<SCRIPTSTACK>::Insert(SCRIPTSTACK stackEntry)
{
    if (m_count >= m_capacity)
    {
        const int nextCapacity = m_capacity*2+4;
        if (nextCapacity>m_capacity) Expand(nextCapacity);
    }
    SCRIPTSTACK* const destination = m_data+m_count;
    ++m_count;
    *destination = stackEntry;
}

template<>
void LIST<SCRIPTSTACK>::Push(const SCRIPTSTACK& stackEntry)
{
    Insert(stackEntry);
}

template<>
void NAMED_LIST<SCRIPTVAR>::Insert(STRING name, SCRIPTVAR scriptVariable)
{
    if (this->m_count >= this->m_capacity)
        static_cast<LIST<NAMED_LIST_STRUCT<SCRIPTVAR> >&>(*this).Expand(this->m_capacity*2+4);
    NAMED_LIST_STRUCT<SCRIPTVAR>& record = this->m_data[this->m_count];
    record.m_name = name;
    record.m_hash = GetHash(record.m_name.m_value);
    ++this->m_count;
    record.m_value.m_type = scriptVariable.m_type;
    record.m_value.m_functionFlags = scriptVariable.m_functionFlags;
    record.m_value.m_text = scriptVariable.m_text;
    record.m_value.m_valueOrOffset = scriptVariable.m_valueOrOffset;
    record.m_value.m_parameterStackBase = scriptVariable.m_parameterStackBase;
    record.m_value.m_elementOrParameterCount = scriptVariable.m_elementOrParameterCount;
}

template<>
void NAMED_LIST<STRING>::Insert(STRING name, STRING stringValue)
{
    if (this->m_count >= this->m_capacity)
        static_cast<LIST<NAMED_LIST_STRUCT<STRING> >&>(*this).Expand(this->m_capacity*2+4);
    NAMED_LIST_STRUCT<STRING>& record = this->m_data[this->m_count];
    record.m_name = name;
    record.m_hash = NAMED_LIST<SCRIPTVAR>::GetHash(record.m_name.m_value);
    ++this->m_count;
    record.m_value = stringValue;
}

template<>
void LIST<SCRIPTSTACK>::Release()
{
    SCRIPTSTACK* const oldData = m_data;
    m_capacity = 0;
    m_count = 0;
    if (oldData)
    {
        unsigned char* const allocation = reinterpret_cast<unsigned char*>(oldData)-4;
        unsigned int remaining = *reinterpret_cast<const unsigned int*>(allocation);
        while (remaining != 0u)
        {
            --remaining;
            oldData[remaining].~SCRIPTSTACK();
        }
        CrtFreePointer(allocation);
    }
    m_data = 0;
}

template<>
void LIST<NAMED_LIST_STRUCT<SCRIPTVAR> >::Release()
{
    NAMED_LIST_STRUCT<SCRIPTVAR>* const oldData = m_data;
    m_capacity = 0;
    m_count = 0;
    if (oldData)
    {
        unsigned char* const allocation = reinterpret_cast<unsigned char*>(oldData)-4;
        unsigned int remaining = *reinterpret_cast<const unsigned int*>(allocation);
        while (remaining != 0u)
        {
            --remaining;
            oldData[remaining].~NAMED_LIST_STRUCT();
        }
        CrtFreePointer(allocation);
    }
    m_data = 0;
}

template<>
void LIST<NAMED_LIST_STRUCT<STRING> >::Release()
{
    NAMED_LIST_STRUCT<STRING>* const oldData = m_data;
    m_capacity = 0;
    m_count = 0;
    if (oldData)
    {
        unsigned char* const allocation = reinterpret_cast<unsigned char*>(oldData)-4;
        unsigned int remaining = *reinterpret_cast<const unsigned int*>(allocation);
        while (remaining != 0u)
        {
            --remaining;
            oldData[remaining].~NAMED_LIST_STRUCT();
        }
        CrtFreePointer(allocation);
    }
    m_data = 0;
}


int SCRIPT::GetNFunction(const STRING& name) const
{
    if (m_errorLatch != 0) return -1;
    const char* const functionNameText = name.m_value;
    const int functionIndex = m_functionList.Location(name);
    if (functionIndex >= 0 && m_functionList.Record(functionIndex).m_value.m_type == 3u)
        return functionIndex;
    (void)::Error->Log("!!!ERROR!!! SCRIPT Can't find function '%s' in GetNFunc", functionNameText);
    return -1;
}


int SCRIPT::CallScriptEventFunction(int eventSlot, const char* signature, int argument1, int argument2, int argument3)
{
    if (m_errorLatch != 0) return 0;
    if (eventSlot<0 || eventSlot >= 256)
    {
        (void)::Error->Log("!!!ERROR!!! SCRIPT invalidate n_event = %i in CallScriptEventFunction", static_cast<unsigned int>(eventSlot));
        return 0;
    }
    const int functionIndex = m_eventMap[eventSlot];
    if (functionIndex != -1 && m_functionList.Record(functionIndex).m_value.m_type == 3u)
        return CallFunction(functionIndex, signature, argument1, argument2, argument3);
    (void)::Error->Log("!!!ERROR!!! SCRIPT Can't find function ScriptEvent%i in CallScriptEventFunction", static_cast<unsigned int>(eventSlot));
    return 0;
}


template<>
int NAMED_LIST<STRING>::Location(const STRING& name) const
{
    const int hash = NAMED_LIST<SCRIPTVAR>::GetHash(name.m_value);
    const NAMED_LIST_STRUCT<STRING>* const records = m_data;
    for (int i = m_count-1;i >= 0;--i)
        if (records[i].m_hash == hash && std::strcmp(records[i].m_name.m_value, name.m_value) == 0)
            return i;
    return -1;
}


void SCRIPT::Error(TYPE_ERROR type, const STRING& subject, int errorArgument)
{
    {
        STRING detail(subject);
        STRING format("SCRIPT '%s' line %i");
        (void)::Error->Error(
            format, type, detail, static_cast<unsigned int>(errorArgument),
            PtrToU32(m_scriptFileName.m_value), static_cast<unsigned int>(m_sourceLine+1));
    }

    if (!m_sourceCursor) return;
    m_errorLatch = 1;

    char contextWindow[61];
    const char* contextStart = m_sourceCursor-30;
    for (int i = 0;i<60;++i)
    {
        char contextChar = contextStart[i];
        if (contextChar == '\n' || contextChar == '\r' || contextChar == '\t') contextChar = '?';
        contextWindow[i] = contextChar;
    }
    contextWindow[60] = '\0';
    {
        STRING context(contextWindow);
        STRING tag("SCRIPT");
        (void)::Error->Error(
            tag, E_ERROR, context, 0u);
    }
    for (int i = 0;i<60;++i) contextWindow[i] = (i == 30)?'^':' ';
    contextWindow[60] = '\0';
    {
        STRING context(contextWindow);
        STRING tag("SCRIPT");
        (void)::Error->Error(
            tag, E_ERROR, context, 0u);
    }
}


int SCRIPT::skipempty2()
{


    if (m_errorLatch != 0) return 1;

    int skipDepth = 0;
    int commentState = 0;

    while (m_sourceCursor<m_sourceEnd)
    {
        if (m_errorLatch != 0) break;

        if (commentState != 0)
        {
            const unsigned char sourceChar = static_cast<unsigned char>(*m_sourceCursor);
            if (commentState == 1)
            {
                if (sourceChar == '\n') commentState = 0;
            }
            else if (sourceChar == '/' && m_sourceCursor[-1] == '*')
            {
                commentState = 0;
            }
            ++m_sourceCursor;
            if (sourceChar == '\n')++m_sourceLine;
            continue;
        }

        unsigned char sourceChar = static_cast<unsigned char>(*m_sourceCursor);
        if ((CrtIsAlpha(static_cast<unsigned int>(static_cast<signed char>(sourceChar))) != 0u || sourceChar == '_') && m_parseMode == 0)
        {
            char* const tokenStart = m_sourceCursor;
            int tokenLength = 0;
            char token[4096];
            for (;;)
            {
                const unsigned char ch = static_cast<unsigned char>(tokenStart[tokenLength]);
                if (CrtIsAlnum(static_cast<unsigned int>(static_cast<signed char>(ch))) == 0u && ch != '_')
                    break;
                if (tokenLength<4095)
                {
                    token[tokenLength] = static_cast<char>(ch);
                }
                else
                {
                    STRING text("Very long name");
                    Error(E_ERROR, text, 0);
                }
                ++tokenLength;
            }

            token[tokenLength] = '\0';
            int defineIndex;
            {
                STRING tokenString(token);
                defineIndex = m_defineList.Location(tokenString);
            }
            if (defineIndex >= 0)
            {
                const NAMED_LIST_STRUCT<STRING>* const records = m_defineList.m_data;
                const char* const replacement = records[defineIndex].m_value.m_value;
                unsigned int replacementLength = 0u;
                while (replacement[replacementLength]) ++replacementLength;
                m_sourceCursor = tokenStart+tokenLength-replacementLength;

                __movsd(reinterpret_cast<unsigned long*>(m_sourceCursor),
                        reinterpret_cast<const unsigned long*>(replacement), replacementLength>>2u);
                const unsigned int copied = replacementLength&~3u;
                __movsb(reinterpret_cast<unsigned char*>(m_sourceCursor+copied),
                        reinterpret_cast<const unsigned char*>(replacement+copied), replacementLength&3u);

                sourceChar = static_cast<unsigned char>(*m_sourceCursor);
            }
        }

        if (sourceChar == '#')
        {
            int handledDirective = 0;
            if (std::strncmp(m_sourceCursor, "#ifdef", 6) == 0)
            {
                handledDirective = 1;
                m_sourceCursor+=6;
                ++m_conditionalDepth;
                if (skipDepth == 0)
                {
                    STRING name;
                    m_parseMode = 1;
                    GetName(&name);
                    m_parseMode = 0;
                    if (m_functionList.Location(name)<0 && m_defineList.Location(name)<0)
                        skipDepth = m_conditionalDepth;
                }
            }
            else if (std::strncmp(m_sourceCursor, "#ifndef", 7) == 0)
            {
                handledDirective = 1;
                m_sourceCursor+=7;
                ++m_conditionalDepth;
                if (skipDepth == 0)
                {
                    STRING name;
                    m_parseMode = 1;
                    GetName(&name);
                    m_parseMode = 0;
                    if (m_functionList.Location(name) >= 0 || m_defineList.Location(name) >= 0)
                        skipDepth = m_conditionalDepth;
                }
            }
            else if (std::strncmp(m_sourceCursor, "#endif", 6) == 0)
            {
                handledDirective = 1;
                m_sourceCursor+=6;
                if (skipDepth == m_conditionalDepth)skipDepth = 0;
                --m_conditionalDepth;
                if (m_conditionalDepth<0)
                {
                    STRING text("#endif without #ifdef");
                    Error(E_ERROR, text, 0);
                }
            }
            else if (std::strncmp(m_sourceCursor, "#else", 5) == 0)
            {
                handledDirective = 1;
                m_sourceCursor+=5;
                if (skipDepth == 0)
                {
                    if (m_conditionalDepth>0)skipDepth = m_conditionalDepth;
                }
                else if (skipDepth == m_conditionalDepth)
                {
                    skipDepth = 0;
                }
                if (m_conditionalDepth <= 0)
                {
                    STRING text("#else without #ifdef");
                    Error(E_ERROR, text, 0);
                }
            }
            if (handledDirective)
                sourceChar = static_cast<unsigned char>(*m_sourceCursor);
        }

        if (sourceChar == '/' && m_sourceCursor[1] == '/')
        {
            commentState = 1;
        }
        else if (sourceChar == '/' && m_sourceCursor[1] == '*')
        {
            commentState = 2;
        }
        else if (sourceChar == '?')
        {
            STRING text("?: not supported in this version");
            Error(E_ERROR, text, 0);
        }
        else if (skipDepth == 0 && CrtIsSpace(static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(sourceChar)))) == 0u && *m_sourceCursor != 0)
        {
            return 0;
        }

        sourceChar = static_cast<unsigned char>(*m_sourceCursor);
        ++m_sourceCursor;
        if (sourceChar == '\n')++m_sourceLine;
    }

    if (m_conditionalDepth>0)
    {
        STRING text("#ifdef without #endif");
        Error(E_ERROR, text, m_conditionalDepth);
    }
    return 1;
}


void SCRIPT::skipempty()
{
    if (m_errorLatch != 0) return;
    if (skipempty2() != 0)
    {
        STRING text("End of file");
        Error(E_ERROR, text, 0);
    }
}

int SCRIPT::WordEnd(char* token)
{
    if (m_errorLatch != 0) return 0;
    if (Word(token) != 0) return 1;
    STRING missing(token);
    Error(E_MISSING, missing, 0);
    return 0;
}


int SCRIPT::GetVariableActionInt(unsigned int actionIndex)
{
    if (m_errorLatch != 0) return 0;
    if (actionIndex<256u)
    {
        const int stackIndex = m_variableMap[actionIndex];
        if (stackIndex >= 0 && stackIndex<m_stack.m_count) return m_stack.m_data[stackIndex].Int();
    }
    STRING text = Printf("variable for Get Action%i", actionIndex);
    Error(E_MISSING, text, 0);
    return 0;
}


void SCRIPT::SetVariableActionInt(unsigned int actionIndex, int integerValue)
{
    if (m_errorLatch != 0) return;
    if (actionIndex<256u)
    {
        const int stackIndex = m_variableMap[actionIndex];
        if (stackIndex >= 0 && stackIndex<m_stack.m_count)
        {
            m_stack.m_data[stackIndex].m_intValue = integerValue;
            m_stack.m_data[stackIndex].m_flags = 2u;
            return;
        }
    }
    STRING text = Printf("variable for Set Action%i", actionIndex);
    Error(E_MISSING, text, 0);
}

int SCRIPT::Word(char* token)
{
    unsigned int tokenLength = 0u;
    while (token[tokenLength]) ++tokenLength;
    skipempty();
    if (m_errorLatch != 0) return 0;
    const char* const sourceStart = m_sourceCursor;
    if (CrtStrncmp(PtrToU32(sourceStart), PtrToU32(token), tokenLength) != 0u) return 0;

    if (CrtStrncmp(PtrToU32(sourceStart), PtrToU32(token), tokenLength) != 0u) return 0;

    const signed char firstTokenChar = static_cast<signed char>(token[0]);
    if (CrtIsAlpha(firstTokenChar) != 0u || firstTokenChar == '#')
    {
        const signed char followingSourceChar = static_cast<signed char>(sourceStart[tokenLength]);
        if (CrtIsAlnum(followingSourceChar) != 0u || followingSourceChar == '_') return 0;
    }
    if (tokenLength == 1u && sourceStart[1] == token[0] && CrtStrchr(PtrToU32("-+|&="), firstTokenChar) != 0u) return 0;

    m_sourceCursor+=tokenLength;
    (void)skipempty2();
    return 1;
}

void SCRIPT::LoadVar(STREAM* stream)
{
    for (int index = 0;index<m_stack.m_count;++index)
        m_stack.m_data[index].Read(stream);
    if (stream)
    {
        (void)stream->Read(&m_functionList.m_count, 4u);
        m_functionList.Expand(m_functionList.m_count);
        for (int index = 0;index<m_functionList.m_count;++index)
        {
            m_functionList.Record(index).m_name.Read(stream);
            SCRIPTVAR* const variable = &m_functionList.Record(index).m_value;
            (void)stream->Read(variable, sizeof(SCRIPTVAR));
        }
    }
    for (int index = 0;index<m_functionList.m_count;++index)
    {
        m_functionList.Record(index).m_value.m_text.m_value = StringGlobals::EmptySentinel();
        m_functionList.Record(index).m_value.m_text.Read(stream);
    }
}

int SCRIPT::GetInt()
{
    if (m_errorLatch != 0) return 0;
    skipempty();
    const unsigned int expressionStart = static_cast<unsigned int>(m_bytecodeCursor);
    vyrag(1);
    if (m_bytecode[expressionStart] != 1u || static_cast<unsigned int>(m_bytecodeCursor)-expressionStart != 5u)
    {
        STRING message("constant int value");
        Error(E_INVALID, message, 0);
    }
    const unsigned int literalPosition = static_cast<unsigned int>(m_bytecodeCursor)-5u;
    m_bytecodeCursor = static_cast<int>(literalPosition);
    return *reinterpret_cast<const int*>(m_bytecode+literalPosition+1u);
}

void SCRIPT::Release()
{
    void* const allocation = m_allocation38;
    m_currentFunction = 0;
    if (allocation)
    {
        CrtFreePointer(allocation);
        m_allocation38 = 0;
    }
    if (m_bytecode)CrtFreePointer(m_bytecode);
    void* const sourceAllocation = m_sourceAllocation;
    m_bytecode = 0;
    if (sourceAllocation)CrtFreePointer(sourceAllocation);
    m_sourceAllocation = 0;

    m_stack.Release();
    static_cast<LIST<NAMED_LIST_STRUCT<SCRIPTVAR> >&>(m_functionList).Release();
    static_cast<LIST<NAMED_LIST_STRUCT<STRING> >&>(m_defineList).Release();

    m_bytecodeCursor = 0;
    m_conditionalDepth = 0;
    m_sourceCursor = 0;
    m_parseMode = 0;
    m_errorLatch = 0;
    m_parserMode = 0;
    m_fallbackFunction = -1;
    for (int i = 0;i<256;++i)m_variableMap[i] = -1;
    for (int i = 0;i<256;++i)m_eventMap[i] = -1;
}

int SCRIPT::LoadLGC(const STRING& fileName)
{
    FSTREAM stream;
    {
        STRING mode("rb");
        if (fileName.m_value[0] != 0)
            stream.file = std::fopen(fileName.m_value, mode.m_value);
    }

    Release();
    m_scriptFileName = fileName;
    if (!stream.file)
    {
        STRING detail(StringGlobals::EmptyText());
        Error(E_OPEN, detail, 0);
        return 1;
    }

    const unsigned int sourceSize = static_cast<unsigned int>(_filelength(BitsToSigned32(static_cast<unsigned int>(_fileno(stream.file)))));
    m_bytecode = static_cast<unsigned char*>(CrtNew(256000u));
    if (!m_bytecode)
    {
        STRING detail("data");
        Error(E_MEMORY, detail, 0);
        (void)CrtExit(1u);
    }

    m_sourceAllocation = static_cast<unsigned char*>(CrtNew(sourceSize+4116u));
    if (!m_sourceAllocation)
    {
        STRING detail("ini");
        Error(E_MEMORY, detail, 0);
        (void)CrtExit(1u);
    }

    m_sourceCursor = reinterpret_cast<char*>(m_sourceAllocation)+4066u;
    m_sourceEnd = m_sourceCursor+sourceSize;
    (void)std::fread(m_sourceCursor, 1u, sourceSize, stream.file);

    if (m_stack.m_capacity<128) m_stack.Expand(128);
    if (m_functionList.m_capacity<128)
        static_cast<LIST<NAMED_LIST_STRUCT<SCRIPTVAR> >&>(m_functionList).Expand(128);

    m_sourceLine = 0;
    int status = func();
    while (status == 0 && m_errorLatch == 0) status = func();

    (void)::Error->Log("LoadScript::ByteCode=%i varNo=%i DefineNo=%i stackNo=%i", static_cast<unsigned int>(m_bytecodeCursor), static_cast<unsigned int>(m_functionList.m_count), static_cast<unsigned int>(m_defineList.m_count), static_cast<unsigned int>(m_stack.m_count));

    static_cast<LIST<NAMED_LIST_STRUCT<STRING> >&>(m_defineList).Release();

    if (m_bytecodeCursor != 0)
    {
        if (m_bytecodeCursor>256000)
        {
            STRING detail("byte code size");
            Error(E_MEMORY, detail, m_bytecodeCursor);
        }
        unsigned char* exact = static_cast<unsigned char*>(CrtNew(static_cast<unsigned int>(m_bytecodeCursor)));
        if (!exact)
        {
            STRING detail("tmp");
            Error(E_MEMORY, detail, 0);
            (void)CrtExit(1u);
        }
        const unsigned int bytes = static_cast<unsigned int>(m_bytecodeCursor);
        __movsd(reinterpret_cast<unsigned long*>(exact), reinterpret_cast<const unsigned long*>(m_bytecode), bytes>>2);
        __movsb(exact+(bytes&~3u), m_bytecode+(bytes&~3u), bytes&3u);
        CrtFreePointer(m_bytecode);
        m_bytecode = exact;
    }
    else
    {
        Release();
    }

    if (m_sourceAllocation)
    {
        CrtFreePointer(m_sourceAllocation);
        m_sourceAllocation = 0;
    }

    if (m_errorLatch != 0)
    {
        Release();
        return 1;
    }

    for (int i = 0;i<m_functionList.m_count;++i)
    {
        const NAMED_LIST_STRUCT<SCRIPTVAR>& record = m_functionList.Record(i);
        if (record.m_value.m_type == 3u && (record.m_value.m_functionFlags&1u) != 0u && (record.m_value.m_functionFlags&2u) == 0u)
        {
            STRING name = m_functionList.Name(i);
            (void)::Error->Log("!!!ERROR!!!SCRIPT: function %s() not return value", name.m_value);
        }
    }
    return 0;
}

int SCRIPT::Load(const STRING& fileName)
{
    unsigned int stackCount = static_cast<unsigned int>(m_stack.m_count);
    FSTREAM stream;
    {
        STRING mode("rb");
        if (fileName.m_value[0] != 0)
            stream.file = std::fopen(fileName.m_value, mode.m_value);
    }

    Release();
    m_scriptFileName = fileName;
    if (!stream.file)
    {
        STRING detail(StringGlobals::EmptyText());
        Error(E_OPEN, detail, 0);
        return 1;
    }

    (void)std::fread(&stackCount, 1u, 4u, stream.file);
    if ((stackCount&0xFF000000u) != 0u)
        return LoadLGC(m_scriptFileName);

    if (m_stack.m_capacity<128) m_stack.Expand(128);
    m_stack.m_count = static_cast<int>(stackCount);
    if (m_stack.m_count>m_stack.m_capacity) m_stack.Expand(static_cast<int>(stackCount));
    LoadVar(&stream);

    (void)std::fread(&m_bytecodeCursor, 1u, 4u, stream.file);
    m_bytecode = static_cast<unsigned char*>(CrtNew(static_cast<unsigned int>(m_bytecodeCursor)));
    if (!m_bytecode)
    {
        STRING detail("data2");
        Error(E_MEMORY, detail, 0);
        (void)CrtExit(1u);
    }
    (void)stream.Read(m_bytecode, static_cast<unsigned int>(m_bytecodeCursor));

    STRING mainName("main");
    m_fallbackFunction = m_functionList.Location(mainName);
    return 0;
}


int SCRIPT::CallFunction(const STRING& name, const char* signature, int argument1, int argument2, int argument3)
{
    if (m_errorLatch != 0) return 0;
    const char* const functionNameText = name.m_value;
    const int functionIndex = m_functionList.Location(name);
    if (functionIndex<0 || m_functionList.Record(functionIndex).m_value.m_type != 3u)
    {
        (void)::Error->Log("!!!ERROR!!! SCRIPT Can't find function '%s' in CallFunction", functionNameText);
        return 0;
    }
    return CallFunction(functionIndex, signature, argument1, argument2, argument3);
}


void SCRIPT::GetLine(STRING* lineText)
{
    skipempty();
    if (m_errorLatch != 0) return;

    char lineBuffer[4096];
    int lineLength = 0;
    while (*m_sourceCursor != '\n' && *m_sourceCursor != '\r')
    {
        if (lineLength<4095) lineBuffer[lineLength] = *m_sourceCursor;
        else
        {
            STRING message("Very long line");
            Error(E_ERROR, message, 0);
        }
        ++m_sourceCursor;
        ++lineLength;
    }

    lineBuffer[lineLength] = '\0';
    *lineText = lineBuffer;
    {
        STRING beforeCommentText = lineText->Before("//");
        *lineText = beforeCommentText;
    }
    lineText->RemoveEndChars(" \n\r\t");
    skipempty();
}

void SCRIPT::SetVariableStr(const STRING& nameObject, const STRING& sourceObject)
{
    const STRING* variableExpression = &nameObject;
    const STRING* sourceString = &sourceObject;
    STRING variableName = variableExpression->Before("[");
    const int recordIndex = m_functionList.Location(variableName);
    if (recordIndex >= 0)
    {
        SCRIPTVAR& variable = m_functionList.m_data[recordIndex].m_value;
        if (variable.m_type == 1u)
        {
            STRING suffix = variableExpression->After("[");
            int elementIndex = 0;
            if (suffix.m_value[1] == 'x')
                (void)std::sscanf(suffix.m_value, "%i", &elementIndex);
            else
                elementIndex = static_cast<int>(CrtAtoiText(suffix.m_value));
            SCRIPTSTACK& destination = m_stack.m_data[variable.m_valueOrOffset+elementIndex];
            destination.m_stringValue = *sourceString;
            destination.m_flags = 1u;
            return;
        }
    }
    (void)::Error->Log("!!!ERROR!!! SCRIPT Can't find variable '%s' in SetVariableStr", variableExpression->m_value);
}

void SCRIPT::SetOperation(int expressionStart, int opcode)
{
    if (m_errorLatch != 0)return;
    unsigned char* bytecode = m_bytecode;
    const unsigned char leftOperandType = bytecode[expressionStart];
    const unsigned int cursor = static_cast<unsigned int>(m_bytecodeCursor);
    if (leftOperandType == SCRIPT_OP_PUSH_INT && bytecode[expressionStart+5u] == SCRIPT_OP_PUSH_INT && cursor-expressionStart == 10u)
    {
        int* lhs = reinterpret_cast<int*>(bytecode+expressionStart+1u);
        const int rhs = *reinterpret_cast<const int*>(bytecode+expressionStart+6u);
        switch (opcode)
        {
            case SCRIPT_OP_DIVIDE:
                *lhs /= rhs;
            break;
            case SCRIPT_OP_MODULO:
                *lhs %= rhs;
            break;
            case SCRIPT_OP_ADD:
                *lhs = AddSigned32Wrap(*lhs, rhs);
            break;
            case SCRIPT_OP_SUBTRACT:
                *lhs = BitsToSigned32(static_cast<unsigned int>(*lhs)-static_cast<unsigned int>(rhs));
            break;
            case SCRIPT_OP_BIT_XOR:
                *lhs ^= rhs;
            break;
            case SCRIPT_OP_BIT_OR:
                *lhs |= rhs;
            break;
            case SCRIPT_OP_BIT_AND:
                *lhs &= rhs;
            break;
            case SCRIPT_OP_MULTIPLY:
                *lhs = MultiplySigned32Wrap(*lhs, rhs);
            break;
            case SCRIPT_OP_SHIFT_RIGHT:
                *lhs >>= (rhs&31);
            break;
            case SCRIPT_OP_SHIFT_LEFT:
                *lhs = static_cast<int>(static_cast<unsigned int>(*lhs) << (rhs&31));
            break;
            default:
                break;
        }

        m_bytecodeCursor = BitsToSigned32(static_cast<unsigned int>(m_bytecodeCursor)-5u);
        return;
    }

    if (leftOperandType == SCRIPT_OP_PUSH_STRING)
    {
        const char* leftString = reinterpret_cast<const char*>(bytecode+expressionStart+1u);
        unsigned int leftStringLength = 0u;
        while (leftString[leftStringLength])++leftStringLength;
        const unsigned int secondTypeOffset = expressionStart+leftStringLength+2u;
        if (bytecode[secondTypeOffset] == SCRIPT_OP_PUSH_STRING)
        {
            const char* rightString = reinterpret_cast<const char*>(bytecode+secondTypeOffset+1u);
            unsigned int rightStringLength = 0u;
            while (rightString[rightStringLength])++rightStringLength;
            if (cursor-expressionStart == leftStringLength+rightStringLength+4u)
            {
                if (opcode == SCRIPT_OP_ADD)
                {
                    (void)CrtMemmove(PtrToU32(bytecode+expressionStart+leftStringLength+1u), PtrToU32(rightString), rightStringLength+1u);
                }
                m_bytecodeCursor = BitsToSigned32(static_cast<unsigned int>(m_bytecodeCursor)-2u);
                return;
            }
        }
    }

    AddDataCommand(static_cast<unsigned char>(opcode));
}


int SCRIPT::GetName(STRING* outString)
{
    if (m_errorLatch != 0) return 0;
    skipempty();
    if (m_errorLatch != 0) return 0;

    char nameBuffer[4096];
    unsigned int nameLength = 0u;
    for (;;)
    {
        const signed char ch = static_cast<signed char>(*m_sourceCursor);
        if (!(CrtIsAlnum(ch) != 0u || ch == '_')) break;
        if (nameLength<4095u) nameBuffer[nameLength] = static_cast<char>(ch);
        else
        {
            STRING message("Very long name");
            Error(E_ERROR, message, 0);
        }
        ++m_sourceCursor;
        ++nameLength;
    }
    nameBuffer[nameLength] = '\0';
    *outString = nameBuffer;
    if (nameLength == 0u)
    {
        STRING message("name");
        Error(E_INVALID, message, 0);
    }
    skipempty();
    return static_cast<int>(nameLength);
}


STRING SCRIPT::GetString()
{
    skipempty();
    if (m_errorLatch != 0)
        return STRING(StringGlobals::EmptyText());

    const unsigned int expressionStart = static_cast<unsigned int>(m_bytecodeCursor);
    vyrag(1);
    const unsigned char literalOpcode = m_bytecode[expressionStart];
    unsigned int stringBytes = 0u;
    if (literalOpcode == 2u)
    {
        stringBytes = 1u;
        while (m_bytecode[expressionStart+stringBytes] != 0u)++stringBytes;
    }
    const unsigned int expressionByteCount = static_cast<unsigned int>(m_bytecodeCursor)-expressionStart;
    if (literalOpcode != 2u || expressionByteCount != stringBytes+1u)
    {
        STRING message("constant string value");
        Error(E_INVALID, message, 0);
    }

    const unsigned int literalPosition = static_cast<unsigned int>(m_bytecodeCursor)-(stringBytes+1u);
    m_bytecodeCursor = static_cast<int>(literalPosition);
    return STRING(reinterpret_cast<const char*>(m_bytecode+literalPosition+1u));
}

STRING SCRIPT::GetVariableStr(const STRING& expression)
{
    STRING variableName = expression.Before("[");
    const int recordIndex = m_functionList.Location(variableName);

    if (recordIndex >= 0 && m_functionList.Record(recordIndex).m_value.m_type == 1u)
    {
        STRING indexText = expression.After("[");
        int elementIndex = 0;
        if (indexText.m_value[1] == 'x')
            (void)std::sscanf(indexText.m_value, "%i", &elementIndex);
        else
            elementIndex = static_cast<int>(CrtAtoiText(indexText.m_value));

        const NAMED_LIST_STRUCT<SCRIPTVAR>& variable = m_functionList.Record(recordIndex);
        if (elementIndex<variable.m_value.m_elementOrParameterCount)
        {
            SCRIPTSTACK& stackEntry = m_stack.m_data[variable.m_value.m_valueOrOffset+elementIndex];
            if ((stackEntry.m_flags&2u) != 0u)
            {
                stackEntry.m_stringValue = Printf("%i", stackEntry.m_intValue);
            }
            return STRING(stackEntry.m_stringValue);
        }
    }

    (void)::Error->Log("!!!ERROR!!! SCRIPT Can't find variable '%s' in GetVariableString", expression.m_value);
    return STRING();
}


int SCRIPT::PutString(char* outText)
{
    if (m_errorLatch != 0) return 0;
    char* writeCursor = outText;
    if (*m_sourceCursor != '\"') return 0;

    ++m_sourceCursor;
    while (*m_sourceCursor != '\"' && m_sourceCursor<m_sourceEnd)
    {
        unsigned char sourceChar = static_cast<unsigned char>(*m_sourceCursor);
        if (sourceChar == '\\')
        {
            const unsigned char escapedChar = static_cast<unsigned char>(m_sourceCursor[1]);
            if (escapedChar == '\r' && m_sourceCursor[2] == '\n')
            {
                m_sourceCursor += 2;
                ++m_sourceLine;
            }
            else if (escapedChar == '\n')
            {
                ++m_sourceCursor;
                ++m_sourceLine;
            }
            else if (CrtIsDigit(static_cast<unsigned int>(static_cast<signed char>(escapedChar))) != 0u &&
                      CrtIsDigit(static_cast<unsigned int>(static_cast<signed char>(m_sourceCursor[2]))) != 0u &&
                      CrtIsDigit(static_cast<unsigned int>(static_cast<signed char>(m_sourceCursor[3]))) != 0u)
            {
                const unsigned char firstOctalDigit = static_cast<unsigned char>(m_sourceCursor[1]-'0');
                const unsigned char secondOctalDigit = static_cast<unsigned char>(m_sourceCursor[2]-'0');
                const unsigned char thirdOctalDigit = static_cast<unsigned char>(m_sourceCursor[3]-'0');
                *writeCursor++ = static_cast<char>((firstOctalDigit<<6)+(secondOctalDigit<<3)+thirdOctalDigit);
                m_sourceCursor += 3;
            }
            else if (escapedChar == 'n')
            {
                *writeCursor++ = '\n';
                ++m_sourceCursor;
            }
            else if (escapedChar == 'r')
            {
                *writeCursor++ = '\r';
                ++m_sourceCursor;
            }
            else if (escapedChar == 't')
            {
                *writeCursor++ = '\t';
                ++m_sourceCursor;
            }
            else if (escapedChar == '\'' || escapedChar == '\\' || escapedChar == 0 || escapedChar == '\"')
            {
                *writeCursor++ = static_cast<char>(escapedChar);
                ++m_sourceCursor;
            }
            else
            {
                STRING detail("symbol after '\\'");
                Error(E_UNKNOWN, detail, 0);
                ++m_sourceCursor;
                *writeCursor++ = *m_sourceCursor;
            }
        }
        else
        {
            *writeCursor++ = static_cast<char>(sourceChar);
        }
        ++m_sourceCursor;
    }

    char* closingQuote = m_sourceCursor;
    ++m_sourceCursor;
    if (closingQuote >= m_sourceEnd)
    {
        STRING detail("End of file");
        Error(E_ERROR, detail, 0);
    }
    *writeCursor++ = '\0';
    return static_cast<int>(writeCursor-outText);
}

void SCRIPTSTACK::BinarOperator(int operation, const SCRIPTSTACK& rhs)
{

    if ((rhs.m_flags&1u) != 0u && (m_flags&1u) != 0u)
    {
        if (operation == SCRIPT_OP_ADD)
        {
            m_stringValue += rhs.m_stringValue;
            m_flags = 1u;
            return;
        }
        if (operation == SCRIPT_OP_SUBTRACT)
        {
            STRING empty(StringGlobals::EmptyText());
            m_stringValue.Replace(rhs.m_stringValue.m_value, empty.m_value);
            m_flags = 1u;
            return;
        }
        if (operation == SCRIPT_OP_EQUAL)
        {
            m_intValue = std::strcmp(m_stringValue.m_value, rhs.m_stringValue.m_value) == 0?1:0;
            m_flags = 2u;
            return;
        }
        if (operation == SCRIPT_OP_NOT_EQUAL)
        {
            m_intValue = std::strcmp(m_stringValue.m_value, rhs.m_stringValue.m_value) != 0?1:0;
            m_flags = 2u;
            return;
        }
    }

    const int rhsValue = rhs.Int();
    if ((m_flags&1u) != 0u)
    {
        const char* const text = m_stringValue.m_value;
        if (text[1] != 'x') m_intValue = BitsToSigned32(CrtAtoiText(text));
        else
        {
            int parsed;
            (void)std::sscanf(text, "%i", &parsed);
            m_intValue = parsed;
        }
    }

    switch (operation)
    {
        case SCRIPT_OP_DIVIDE:
            m_intValue = rhsValue != 0?m_intValue/rhsValue:0x0FFFFFFF;
        break;
        case SCRIPT_OP_MODULO:
            m_intValue %= rhsValue;
        break;
        case SCRIPT_OP_ADD:
            m_intValue = AddSigned32Wrap(m_intValue, rhsValue);
        break;
        case SCRIPT_OP_SUBTRACT:
            m_intValue = BitsToSigned32(static_cast<unsigned int>(m_intValue)-static_cast<unsigned int>(rhsValue));
        break;
        case SCRIPT_OP_BIT_XOR:
            m_intValue ^= rhsValue;
        break;
        case SCRIPT_OP_BIT_OR:
            m_intValue |= rhsValue;
        break;
        case SCRIPT_OP_BIT_AND:
            m_intValue &= rhsValue;
        break;
        case SCRIPT_OP_EQUAL:
            m_intValue = (m_intValue == rhsValue)?1:0;
        break;
        case SCRIPT_OP_LOGIC_OR:
            m_intValue = (m_intValue != 0 || rhsValue != 0)?1:0;
        break;
        case SCRIPT_OP_GREATER:
            m_intValue = (m_intValue>rhsValue)?1:0;
        break;
        case SCRIPT_OP_LESS:
            m_intValue = (m_intValue<rhsValue)?1:0;
        break;
        case SCRIPT_OP_GREATER_EQUAL:
            m_intValue = (m_intValue >= rhsValue)?1:0;
        break;
        case SCRIPT_OP_LESS_EQUAL:
            m_intValue = (m_intValue <= rhsValue)?1:0;
        break;
        case SCRIPT_OP_MULTIPLY:
            m_intValue = MultiplySigned32Wrap(m_intValue, rhsValue);
        break;
        case SCRIPT_OP_NOT_EQUAL:
            m_intValue = (m_intValue != rhsValue)?1:0;
        break;
        case SCRIPT_OP_LOGIC_AND:
            m_intValue = (m_intValue != 0 && rhsValue != 0)?1:0;
        break;
        case SCRIPT_OP_SHIFT_RIGHT:
            m_intValue >>= (rhsValue&31);
        break;
        case SCRIPT_OP_SHIFT_LEFT:
            m_intValue = static_cast<int>(static_cast<unsigned int>(m_intValue)<<(rhsValue&31));
        break;
        default:
            (void)::Error->Log("!!!ERROE!!!SCRIPT::Unknown Binary command %i", static_cast<unsigned int>(operation));
            break;
    }
    m_flags = 2u;
}



void SCRIPT::IntVar(TYPE_VAR type)
{
    int count = 1;
    unsigned char rawFlags = 0;
    STRING name;
    skipempty();
    if (m_errorLatch != 0)return;

    if (*m_sourceCursor == '*')
    {
        rawFlags = SCRIPT_STACK_DYNAMIC;
        ++m_sourceCursor;
    }

    GetName(&name);

    if (m_functionList.Location(name) >= 0)
    {
        STRING message = Printf("int redefinition '%s'", name.m_value);
        STRING detail(message);
        Error(E_ERROR, detail, 0);
    }
    if (m_errorLatch != 0)return;

    if (std::strncmp(name.m_value, "Action", 6) == 0)
    {
        int valid;
        {
            STRING suffix = name.After("Action");
            const char first = suffix.m_value[0];
            valid = (CrtIsDigit(static_cast<unsigned int>(static_cast<signed char>(first))) != 0u) ||
                (first == '-' && CrtIsDigit(static_cast<unsigned int>(static_cast<signed char>(suffix.m_value[1]))) != 0u);
        }
        if (valid)
        {
            int index;
            {
                STRING suffix = name.After("Action");
                if (suffix.m_value[1] == 'x')
                    (void)std::sscanf(suffix.m_value, "%i", &index);
                else
                    index = static_cast<int>(CrtAtoiText(suffix.m_value));
            }
            if (static_cast<unsigned int>(index)<256u)
                m_variableMap[index] = m_stack.No();
        }
    }

    SCRIPTVAR variable;
    variable.m_type = 1u;
    variable.m_functionFlags = 0u;
    variable.m_valueOrOffset = m_stack.No();
    m_functionList.Insert(name, variable);

    if (Word(const_cast<char*>("[")))
    {
        rawFlags = static_cast<unsigned char>(rawFlags|SCRIPT_STACK_ARRAY);
        if (*m_sourceCursor == ']')count = 999999;
        else count = GetInt();
        WordEnd(const_cast<char*>("]"));
    }

    int initialized = 0;
    if (Word(const_cast<char*>("=")))
    {
        if ((rawFlags&SCRIPT_STACK_ARRAY) != 0u)
        {
            WordEnd(const_cast<char*>("{"));
            for (;;)
            {
                if (type == TYPE_VAR_RUNTIME_LOCAL)
                {
                    {
                        STRING elementSuffix = Printf("[%i]", initialized);
                        STRING elementTag = name+elementSuffix;
                        SCRIPTSTACK value(0, elementTag);
                        value.m_flags = static_cast<unsigned char>(rawFlags|0xC2u);
                        m_stack.Insert(value);
                    }
                    vyrag(1);
                    const int stackIndex = m_functionList.Record(m_functionList.m_count-1).m_value.m_valueOrOffset+initialized;
                    AddDataCommand(SCRIPT_OP_ASSIGN, stackIndex);
                    AddDataCommand(SCRIPT_OP_LINE, m_sourceLine);
                }
                else
                {
                    STRING elementSuffix = Printf("[%i]", initialized);
                    STRING elementTag = name+elementSuffix;
                    const int intValue = GetInt();
                    SCRIPTSTACK value(intValue, elementTag);
                    value.m_flags = static_cast<unsigned char>(rawFlags|0x8Au);
                    m_stack.Insert(value);
                }

                const int previous = initialized;
                ++initialized;
                if (previous >= count)
                {
                    STRING message("too many initializers");
                    Error(E_ERROR, message, 0);
                }

                if (Word(const_cast<char*>(",")) != 0 && m_errorLatch == 0)
                    continue;
                WordEnd(const_cast<char*>("}"));
                break;
            }
            if (count == 999999)count = initialized;
        }
        else
        {
            if (type == TYPE_VAR_RUNTIME_LOCAL)
            {
                {
                    SCRIPTSTACK value(0, name);
                    value.m_flags = static_cast<unsigned char>(rawFlags|0xC2u);
                    m_stack.Insert(value);
                }
                vyrag(1);
                const int stackIndex = m_functionList.Record(m_functionList.m_count-1).m_value.m_valueOrOffset;
                AddDataCommand(SCRIPT_OP_ASSIGN, stackIndex);
                AddDataCommand(SCRIPT_OP_LINE, m_sourceLine);
            }
            else
            {
                const int intValue = GetInt();
                SCRIPTSTACK value(intValue, name);
                value.m_flags = static_cast<unsigned char>(rawFlags|0x8Au);
                m_stack.Insert(value);
            }
            initialized = 1;
        }
    }
    else if (count == 999999)
    {
        STRING message("for [] need initialisation");
        Error(E_ERROR, message, 0);
    }

    while (initialized<count)
    {
        rawFlags = static_cast<unsigned char>(rawFlags|0xC2u);
        {
            STRING elementSuffix = Printf("[%i]", initialized);
            STRING elementTag = name+elementSuffix;
            SCRIPTSTACK value(0, elementTag);
            value.m_flags = rawFlags;
            m_stack.Insert(value);
        }
        ++initialized;
    }

    SetNoElement(count);
}

void SCRIPT::StringVar(TYPE_VAR type)
{
    int count = 1;
    unsigned char rawFlags = 0;
    STRING name;
    skipempty();
    if (m_errorLatch != 0)return;

    if (*m_sourceCursor == '*')
    {
        rawFlags = SCRIPT_STACK_DYNAMIC;
        ++m_sourceCursor;
    }

    GetName(&name);
    if (m_functionList.Location(name) >= 0)
    {
        STRING message = Printf("string redefinition '%s'", name.m_value);
        Error(E_ERROR, message, 0);
    }
    if (m_errorLatch != 0)return;

    SCRIPTVAR variable;
    variable.m_type = 1u;
    variable.m_functionFlags = 0u;
    variable.m_valueOrOffset = m_stack.No();
    m_functionList.Insert(name, variable);

    if (Word(const_cast<char*>("[")))
    {
        rawFlags = static_cast<unsigned char>(rawFlags|SCRIPT_STACK_ARRAY);
        if (*m_sourceCursor == ']')count = 999999;
        else count = GetInt();
        WordEnd(const_cast<char*>("]"));
    }

    int initialized = 0;
    if (Word(const_cast<char*>("=")))
    {
        if ((rawFlags&SCRIPT_STACK_ARRAY) != 0u)
        {
            WordEnd(const_cast<char*>("{"));
            for (;;)
            {
                if (type == TYPE_VAR_RUNTIME_LOCAL)
                {
                    {
                        STRING empty(StringGlobals::EmptyText());
                        STRING elementSuffix = Printf("[%i]", initialized);
                        STRING elementTag = name+elementSuffix;
                        SCRIPTSTACK value(empty, elementTag);
                        value.m_flags = static_cast<unsigned char>(rawFlags|0xC1u);
                        m_stack.Insert(value);
                    }
                    vyrag(1);
                    const int stackIndex = m_functionList.Record(m_functionList.m_count-1).m_value.m_valueOrOffset+initialized;
                    AddDataCommand(SCRIPT_OP_ASSIGN, stackIndex);
                    AddDataCommand(SCRIPT_OP_LINE, m_sourceLine);
                }
                else
                {
                    STRING elementSuffix = Printf("[%i]", initialized);
                    STRING elementTag = name+elementSuffix;
                    STRING stringValue = GetString();
                    SCRIPTSTACK value(stringValue, elementTag);
                    value.m_flags = static_cast<unsigned char>(rawFlags|0x89u);
                    m_stack.Insert(value);
                }

                const int previous = initialized;
                ++initialized;
                if (previous >= count)
                {
                    STRING message("too many initializers");
                    Error(E_ERROR, message, 0);
                }

                if (Word(const_cast<char*>(",")) != 0 && m_errorLatch == 0)
                    continue;
                WordEnd(const_cast<char*>("}"));
                break;
            }
            if (count == 999999)count = initialized;
        }
        else
        {
            if (type == TYPE_VAR_RUNTIME_LOCAL)
            {
                {
                    STRING empty(StringGlobals::EmptyText());
                    SCRIPTSTACK value(empty, name);
                    value.m_flags = static_cast<unsigned char>(rawFlags|0xC1u);
                    m_stack.Insert(value);
                }
                vyrag(1);
                const int stackIndex = m_functionList.Record(m_functionList.m_count-1).m_value.m_valueOrOffset;
                AddDataCommand(SCRIPT_OP_ASSIGN, stackIndex);
                AddDataCommand(SCRIPT_OP_LINE, m_sourceLine);
            }
            else
            {
                STRING stringValue = GetString();
                SCRIPTSTACK value(stringValue, name);
                value.m_flags = static_cast<unsigned char>(rawFlags|0x89u);
                m_stack.Insert(value);
            }
            initialized = 1;
        }
    }
    else if (count == 999999)
    {
        STRING message("for [] need initialisation");
        Error(E_ERROR, message, 0);
    }

    while (initialized<count)
    {
        rawFlags = static_cast<unsigned char>(rawFlags|0xC1u);
        {
            STRING empty(StringGlobals::EmptyText());
            STRING elementSuffix = Printf("[%i]", initialized);
            STRING elementTag = name+elementSuffix;
            SCRIPTSTACK value(empty, elementTag);
            value.m_flags = rawFlags;
            m_stack.Insert(value);
        }
        ++initialized;
    }

    SetNoElement(count);
}

void SCRIPT::mnog()
{
    if (m_errorLatch != 0)return;

    int castOpcode = 0;
    STRING name;
    for (;;)
    {
        int unaryOpcode = 0;
        int variableOpcode = SCRIPT_OP_READ;
        int compoundOpcode = 0;

        if (Word(const_cast<char*>("(int)"))) castOpcode = SCRIPT_OP_CAST_INT;
        else if (Word(const_cast<char*>("(string)"))) castOpcode = SCRIPT_OP_CAST_STRING;
        else if (Word(const_cast<char*>("(sprite)"))) castOpcode = SCRIPT_OP_CAST_OBJECT;

        if (Word(const_cast<char*>("-")))unaryOpcode = SCRIPT_OP_NEGATE;
        else if (Word(const_cast<char*>("+")))unaryOpcode = 0;
        else if (Word(const_cast<char*>("~")))unaryOpcode = SCRIPT_OP_BITNOT;
        else if (Word(const_cast<char*>("!")))unaryOpcode = SCRIPT_OP_LOGNOT;

        if (Word(const_cast<char*>("--")))variableOpcode = SCRIPT_OP_PRE_DEC;
        else if (Word(const_cast<char*>("++")))variableOpcode = SCRIPT_OP_PRE_INC;
        else if (Word(const_cast<char*>("&")))variableOpcode = SCRIPT_OP_ADDRESS;

        const auto emitTail = [this, &unaryOpcode, castOpcode]()
        {

            if (unaryOpcode != 0)
            {
                m_bytecode[m_bytecodeCursor] = static_cast<unsigned char>(unaryOpcode);
                ++m_bytecodeCursor;
            }
            if (castOpcode != 0)
            {
                m_bytecode[m_bytecodeCursor] = static_cast<unsigned char>(castOpcode);
                ++m_bytecodeCursor;
            }
        };

        const signed char sourceChar = *reinterpret_cast<const signed char*>(m_sourceCursor);
        if (CrtIsDigit(static_cast<unsigned int>(sourceChar)) != 0u)
        {
            GetName(&name);
            int value = SCRIPT_OP_READ;
            (void)std::sscanf(name.m_value, "%i", &value);
            if (unaryOpcode == SCRIPT_OP_NEGATE)
            {
                value = BitsToSigned32(0u-static_cast<unsigned int>(value));
                unaryOpcode = 0;
            }
            else if (unaryOpcode == SCRIPT_OP_BITNOT)
            {
                value = ~value;
                unaryOpcode = 0;
            }
            else if (unaryOpcode == SCRIPT_OP_LOGNOT)
            {
                value = (value == 0)?1:0;
                unaryOpcode = 0;
            }
            AddDataCommand(SCRIPT_OP_PUSH_INT, value);
            emitTail();
            return;
        }

        if (*m_sourceCursor == '"')
        {
            AddDataCommand(SCRIPT_OP_PUSH_STRING);
            m_bytecodeCursor+=PutString(reinterpret_cast<char*>(m_bytecode+m_bytecodeCursor));
            emitTail();
            return;
        }

        if (*m_sourceCursor == '\'')
        {
            ++m_sourceCursor;
            const int value = static_cast<int>(*reinterpret_cast<const signed char*>(m_sourceCursor));
            AddDataCommand(SCRIPT_OP_PUSH_INT, value);
            ++m_sourceCursor;
            if (*m_sourceCursor != '\'')
            {
                STRING errorText("second '");
                Error(E_MISSING, errorText, 0);
            }
            ++m_sourceCursor;
            emitTail();
            return;
        }

        if (Word(const_cast<char*>("sizeof")))
        {
            if (!Word(const_cast<char*>("(")))
            {
                STRING errorText("'(' for sizeof");
                Error(E_MISSING, errorText, 0);
            }

            if (Word(const_cast<char*>("int")) || Word(const_cast<char*>("string")))
            {
                AddDataCommand(SCRIPT_OP_PUSH_INT, 4);
            }
            else
            {
                GetName(&name);
                const int index = m_functionList.Location(name);
                if (index >= 0 && m_functionList.Record(index).m_value.m_type == 1u)
                    AddDataCommand(SCRIPT_OP_PUSH_INT, MultiplySigned32Wrap(m_functionList.Record(index).m_value.m_elementOrParameterCount, 4));
                else
                {
                    STRING errorText("sizeof parameter");
                    Error(E_INVALID, errorText, 0);
                }
            }
            WordEnd(const_cast<char*>(")"));
            emitTail();
            return;
        }

        if (Word(const_cast<char*>("static")))
        {
            if (Word(const_cast<char*>("int")))
            {
                do
                {
                    IntVar(TYPE_VAR_STATIC_LOCAL);
                }
                while (Word(const_cast<char*>(",")) && m_errorLatch == 0);
            }
            else if (Word(const_cast<char*>("string")))
            {
                do
                {
                    StringVar(TYPE_VAR_STATIC_LOCAL);
                }
                while (Word(const_cast<char*>(",")) && m_errorLatch == 0);
            }
            else
            {
                STRING errorText("static variable");
                Error(E_INVALID, errorText, 0);
            }
            emitTail();
            return;
        }

        if (Word(const_cast<char*>("int")))
        {
            do
            {
                IntVar(TYPE_VAR_RUNTIME_LOCAL);
            }
            while (Word(const_cast<char*>(",")) && m_errorLatch == 0);
            emitTail();
            return;
        }
        if (Word(const_cast<char*>("string")))
        {
            do
            {
                StringVar(TYPE_VAR_RUNTIME_LOCAL);
            }
            while (Word(const_cast<char*>(",")) && m_errorLatch == 0);
            emitTail();
            return;
        }

        if (Word(const_cast<char*>("return")))
        {
            vyrag(1);
            AddDataCommand(SCRIPT_OP_RETURN);
            if (m_currentFunction != 0)
                m_currentFunction->m_functionFlags|=2u;
            emitTail();
            return;
        }

        if (Word(const_cast<char*>("(")))
        {
            vyrag(1);
            WordEnd(const_cast<char*>(")"));
            emitTail();
            return;
        }

        if (CrtIsAlpha(static_cast<unsigned int>(*reinterpret_cast<const signed char*>(m_sourceCursor))) != 0u)
        {
            GetName(&name);
            int found = m_functionList.Location(name);
            if (found<0)
            {
                if (*m_sourceCursor == ':')
                {
                    ++m_sourceCursor;
                    SCRIPTVAR value;
                    value.m_type = 7u;
                    value.m_functionFlags = 0u;
                    value.m_text = STRING();
                    value.m_valueOrOffset = m_bytecodeCursor;
                    m_functionList.Insert(name, value);
                    continue;
                }
                {
                    STRING message = Printf("Undeclared identifier '%s'", name.m_value);
                    Error(E_ERROR, message, 0);
                }
                emitTail();
                return;
            }

            const unsigned char kind = m_functionList.Record(found).m_value.m_type;
            if (kind == 8u)
            {
                if (*m_sourceCursor != ':')
                {
                    {
                        STRING message = Printf("Incorrect use label '%s'", name.m_value);
                        Error(E_ERROR, message, 0);
                    }
                    emitTail();
                    return;
                }
                ++m_sourceCursor;
                m_functionList.Record(found).m_value.m_type = 7u;
                const int patch = m_functionList.Record(found).m_value.m_valueOrOffset;
                *reinterpret_cast<int*>(m_bytecode+patch) = m_bytecodeCursor-patch;
                m_functionList.Record(found).m_value.m_valueOrOffset = m_bytecodeCursor;
                continue;
            }
            if (kind == 7u)
            {
                {
                    const char* const format = (*m_sourceCursor == ':')
                        ?"Label redefinition '%s'":"Incorrect use label '%s'";
                    STRING message = Printf(format, name.m_value);
                    Error(E_ERROR, message, 0);
                }
                emitTail();
                return;
            }
            if (kind == 6u)
            {
                emitTail();
                return;
            }

            if (kind == 2u)
            {
                WordEnd(const_cast<char*>("("));
                int parsed = 0;
                if (!Word(const_cast<char*>(")")))
                {
                    while (m_errorLatch == 0 && parsed <= m_functionList.Record(found).m_value.m_elementOrParameterCount)
                    {
                        SCRIPTSTACK& parameter = m_stack.m_data[m_functionList.Record(found).m_value.m_parameterStackBase+parsed];
                        if ((parameter.m_flags&SCRIPT_STACK_DYNAMIC) != 0u)
                            parsingArrayArgument = 1;
                        vyrag(1);
                        (void)Word(const_cast<char*>(","));
                        parsingArrayArgument = 0;
                        ++parsed;
                        if (Word(const_cast<char*>(")")))break;
                    }
                }
                while (parsed<m_functionList.Record(found).m_value.m_elementOrParameterCount)
                {
                    SCRIPTSTACK& def = m_stack.m_data[m_functionList.Record(found).m_value.m_parameterStackBase+parsed];
                    if ((def.m_flags&SCRIPT_STACK_HAS_VALUE) == 0u)break;
                    AddDataCommand(SCRIPT_OP_READ, m_functionList.Record(found).m_value.m_parameterStackBase+parsed);
                    ++parsed;
                }
                if (parsed != m_functionList.Record(found).m_value.m_elementOrParameterCount || m_errorLatch != 0)
                {
                    m_errorLatch = 1;
                    STRING errorText("extern function parameters number");
                    Error(E_INVALID, errorText, 0);
                    return;
                }
                AddDataCommand(static_cast<unsigned char>(m_functionList.Record(found).m_value.m_valueOrOffset));
                if (m_parserMode != 0)m_functionList.Record(found).m_value.m_functionFlags|=1u;
                emitTail();
                return;
            }

            if (kind == 3u)
            {
                WordEnd(const_cast<char*>("("));
                int parsed = 0;
                if (!Word(const_cast<char*>(")")))
                {
                    while (m_errorLatch == 0 && parsed <= m_functionList.Record(found).m_value.m_elementOrParameterCount)
                    {
                        SCRIPTSTACK& parameter = m_stack.m_data[m_functionList.Record(found).m_value.m_parameterStackBase+parsed];
                        if ((parameter.m_flags&SCRIPT_STACK_DYNAMIC) != 0u)
                            parsingArrayArgument = 1;
                        vyrag(1);
                        (void)Word(const_cast<char*>(","));
                        parsingArrayArgument = 0;
                        AddDataCommand(SCRIPT_OP_ASSIGN, m_functionList.Record(found).m_value.m_parameterStackBase+parsed);
                        AddDataCommand(SCRIPT_OP_POP);
                        ++parsed;
                        if (Word(const_cast<char*>(")")))break;
                    }
                }
                while (parsed<m_functionList.Record(found).m_value.m_elementOrParameterCount)
                {
                    SCRIPTSTACK& def = m_stack.m_data[m_functionList.Record(found).m_value.m_parameterStackBase+parsed];
                    if ((def.m_flags&SCRIPT_STACK_HAS_VALUE) == 0u)break;
                    if ((def.m_flags&SCRIPT_STACK_STRING) != 0u)
                    {
                        AddDataCommand(SCRIPT_OP_PUSH_STRING);
                        const unsigned int len = static_cast<unsigned int>(std::strlen(def.m_stringValue.m_value))+1u;
                        std::memcpy(m_bytecode+m_bytecodeCursor, def.m_stringValue.m_value, len);
                        m_bytecodeCursor+=static_cast<int>(len);
                    }
                    else
                    {
                        AddDataCommand(SCRIPT_OP_PUSH_INT, def.m_intValue);
                    }
                    AddDataCommand(SCRIPT_OP_ASSIGN, m_functionList.Record(found).m_value.m_parameterStackBase+parsed);
                    AddDataCommand(SCRIPT_OP_POP);
                    ++parsed;
                }
                if (parsed != m_functionList.Record(found).m_value.m_elementOrParameterCount || m_errorLatch != 0)
                {
                    m_errorLatch = 1;
                    STRING errorText("function parameters number");
                    Error(E_INVALID, errorText, 0);
                    return;
                }


                AddDataCommand(SCRIPT_OP_CALL, found);
                if (m_parserMode != 0)m_functionList.Record(found).m_value.m_functionFlags|=1u;
                emitTail();
                return;
            }

            if (kind == 4u)
            {
                AddDataCommand(SCRIPT_OP_PUSH_STRING);
                const char* text = m_functionList.Record(found).m_value.m_text.m_value;
                const unsigned int len = static_cast<unsigned int>(std::strlen(text))+1u;
                std::memcpy(m_bytecode+m_bytecodeCursor, text, len);
                m_bytecodeCursor+=static_cast<int>(len);
                emitTail();
                return;
            }
            if (kind == 5u)
            {
                AddDataCommand(SCRIPT_OP_PUSH_INT, m_functionList.Record(found).m_value.m_valueOrOffset);
                emitTail();
                return;
            }

            if (kind == 1u)
            {
                unsigned char* savedIndex = 0;
                int savedIndexSize = 0;

                if (Word(const_cast<char*>("[")))
                {
                    const int indexStart = m_bytecodeCursor;
                    if ((m_stack.m_data[m_functionList.Record(found).m_value.m_valueOrOffset].m_flags&(SCRIPT_STACK_STRING|SCRIPT_STACK_ARRAY|SCRIPT_STACK_DYNAMIC)) == 0u)
                    {
                        STRING errorText("[] for not array");
                        Error(E_ERROR, errorText, 0);
                    }
                    vyrag(1);
                    AddDataCommand(SCRIPT_OP_ARRAY_INDEX);
                    savedIndexSize = m_bytecodeCursor-indexStart;
                    savedIndex = static_cast<unsigned char*>(CrtNew(static_cast<unsigned int>(savedIndexSize)));
                    if (!savedIndex)Error(E_MEMORY, name, 0);
                    std::memcpy(savedIndex, m_bytecode+indexStart, static_cast<unsigned int>(savedIndexSize));
                    m_bytecodeCursor = indexStart;
                    WordEnd(const_cast<char*>("]"));
                }

                int opcode = variableOpcode;
                if (Word(const_cast<char*>("=")))
                {
                    if (Word(const_cast<char*>("{")))
                    {
                        if ((m_stack.m_data[m_functionList.Record(found).m_value.m_valueOrOffset].m_flags&(SCRIPT_STACK_ARRAY|SCRIPT_STACK_DYNAMIC)) == 0u)
                        {
                            STRING errorText("= {} for not array");
                            Error(E_ERROR, errorText, 0);
                        }
                        int element = 0;
                        do
                        {
                            vyrag(1);
                            AddDataCommand(SCRIPT_OP_ASSIGN, m_functionList.Record(found).m_value.m_valueOrOffset+element);
                            AddDataCommand(SCRIPT_OP_LINE, m_sourceLine);
                            ++element;
                        }
                        while (Word(const_cast<char*>(",")));
                        WordEnd(const_cast<char*>("}"));
                        opcode = variableOpcode;
                    }
                    else
                    {
                        vyrag(1);
                        opcode = SCRIPT_OP_ASSIGN;
                    }
                }
                else if (Word(const_cast<char*>("+=")))
                {
                    vyrag(1);
                    opcode = SCRIPT_OP_COMPOUND;
                    compoundOpcode = SCRIPT_OP_ADD;
                }
                else if (Word(const_cast<char*>("-=")))
                {
                    vyrag(1);
                    opcode = SCRIPT_OP_COMPOUND;
                    compoundOpcode = SCRIPT_OP_SUBTRACT;
                }
                else if (Word(const_cast<char*>("/=")))
                {
                    vyrag(1);
                    opcode = SCRIPT_OP_COMPOUND;
                    compoundOpcode = SCRIPT_OP_DIVIDE;
                }
                else if (Word(const_cast<char*>("*=")))
                {
                    vyrag(1);
                    opcode = SCRIPT_OP_COMPOUND;
                    compoundOpcode = SCRIPT_OP_MULTIPLY;
                }
                else if (Word(const_cast<char*>("%=")))
                {
                    vyrag(1);
                    opcode = SCRIPT_OP_COMPOUND;
                    compoundOpcode = SCRIPT_OP_MODULO;
                }
                else if (Word(const_cast<char*>("&=")))
                {
                    vyrag(1);
                    opcode = SCRIPT_OP_COMPOUND;
                    compoundOpcode = SCRIPT_OP_BIT_AND;
                }
                else if (Word(const_cast<char*>("|=")))
                {
                    vyrag(1);
                    opcode = SCRIPT_OP_COMPOUND;
                    compoundOpcode = SCRIPT_OP_BIT_OR;
                }
                else if (Word(const_cast<char*>("^=")))
                {
                    vyrag(1);
                    opcode = SCRIPT_OP_COMPOUND;
                    compoundOpcode = SCRIPT_OP_BIT_XOR;
                }
                else if (Word(const_cast<char*>("<<=")))
                {
                    vyrag(1);
                    opcode = SCRIPT_OP_COMPOUND;
                    compoundOpcode = SCRIPT_OP_SHIFT_LEFT;
                }
                else if (Word(const_cast<char*>(">>=")))
                {
                    vyrag(1);
                    opcode = SCRIPT_OP_COMPOUND;
                    compoundOpcode = SCRIPT_OP_SHIFT_RIGHT;
                }
                else if (Word(const_cast<char*>("++")))opcode = SCRIPT_OP_POST_INC;
                else if (Word(const_cast<char*>("--")))opcode = SCRIPT_OP_POST_DEC;

                if (savedIndexSize != 0)
                {
                    std::memcpy(m_bytecode+m_bytecodeCursor, savedIndex, static_cast<unsigned int>(savedIndexSize));
                    m_bytecodeCursor+=savedIndexSize;
                    CrtFreePointer(savedIndex);
                }
                else if ((m_stack.m_data[m_functionList.Record(found).m_value.m_valueOrOffset].m_flags&(SCRIPT_STACK_ARRAY|SCRIPT_STACK_DYNAMIC)) != 0u)
                {
                    if (opcode == SCRIPT_OP_POST_INC || opcode == SCRIPT_OP_POST_DEC ||
                       opcode == SCRIPT_OP_PRE_INC || opcode == SCRIPT_OP_PRE_DEC)
                    {
                        STRING errorText("Increment or decrement for array");
                        Error(E_ERROR, errorText, 0);
                    }
                    if (opcode != SCRIPT_OP_READ || parsingArrayArgument == 0)
                    {
                        STRING errorText("operation for array");
                        Error(E_INVALID, errorText, opcode);
                    }
                }

                AddDataCommand(static_cast<unsigned char>(opcode), m_functionList.Record(found).m_value.m_valueOrOffset);
                if (opcode == SCRIPT_OP_COMPOUND)
                {
                    m_bytecode[m_bytecodeCursor] = static_cast<unsigned char>(compoundOpcode);
                    ++m_bytecodeCursor;
                }
                emitTail();
                return;
            }


            emitTail();
            return;
        }

        if (unaryOpcode != 0)
        {
            {
                STRING errorText("error symbol");
                Error(E_ERROR, errorText, 0);
            }
            emitTail();
            return;
        }
        if (castOpcode != 0)
        {
            {
                STRING errorText("error change type");
                Error(E_ERROR, errorText, 0);
            }
            emitTail();
            return;
        }

        bool errorSymbol;
        {
            STRING symbols(".$#@`");
            errorSymbol = std::strchr(symbols.m_value, *m_sourceCursor) != 0;
        }
        if (errorSymbol)
        {
            STRING errorText("error symbol");
            Error(E_ERROR, errorText, 0);
            return;
        }
        bool validEnd;
        {
            STRING symbols(";)");
            validEnd = std::strchr(symbols.m_value, *m_sourceCursor) != 0;
        }
        if (!validEnd)
        {
            STRING errorText("syntax error");
            Error(E_ERROR, errorText, 0);
        }
        return;
    }
}

void SCRIPT::oper(int* breakPatchList)
{
    if (m_errorLatch != 0)return;

    const int iff = Word(const_cast<char*>("iff"));
    if (iff || Word(const_cast<char*>("if")))
    {
        WordEnd(const_cast<char*>("("));
        vyrag(1);
        WordEnd(const_cast<char*>(")"));

        AddDataCommand(static_cast<unsigned char>(iff?SCRIPT_OP_IFF:SCRIPT_OP_IF));
        const int firstPatch = m_bytecodeCursor;
        m_bytecodeCursor += 4;
        oper(breakPatchList);
        *reinterpret_cast<int*>(m_bytecode+firstPatch) = m_bytecodeCursor-firstPatch;

        if (Word(const_cast<char*>("else")) == 0)return;

        *reinterpret_cast<int*>(m_bytecode+firstPatch) += 5;
        AddDataCommand(SCRIPT_OP_JUMP);
        const int elsePatch = m_bytecodeCursor;
        m_bytecodeCursor += 4;
        oper(breakPatchList);
        *reinterpret_cast<int*>(m_bytecode+elsePatch) = m_bytecodeCursor-elsePatch;
        return;
    }

    if (Word(const_cast<char*>("while")))
    {
        int localBreaks[129];
        localBreaks[0] = 0;
        for (int i = 1;i<128;++i)localBreaks[i] = 0;

        WordEnd(const_cast<char*>("("));
        const int conditionStart = m_bytecodeCursor;
        vyrag(1);
        WordEnd(const_cast<char*>(")"));

        AddDataCommand(SCRIPT_OP_IF);
        const int exitPatch = m_bytecodeCursor;
        m_bytecodeCursor += 4;
        oper(localBreaks);

        AddDataCommand(SCRIPT_OP_JUMP);
        const int backPatch = m_bytecodeCursor;
        *reinterpret_cast<int*>(m_bytecode+backPatch) = conditionStart-backPatch;
        m_bytecodeCursor += 4;

        *reinterpret_cast<int*>(m_bytecode+exitPatch) = m_bytecodeCursor-exitPatch;
        for (int i = 0;localBreaks[i] != 0;++i)
            *reinterpret_cast<int*>(m_bytecode+localBreaks[i]) = m_bytecodeCursor-localBreaks[i];
        return;
    }

    if (Word(const_cast<char*>("do")))
    {
        const int bodyStart = m_bytecodeCursor;
        int localBreaks[129];
        localBreaks[0] = 0;
        for (int i = 1;i<128;++i)localBreaks[i] = 0;

        oper(localBreaks);
        WordEnd(const_cast<char*>("while"));
        WordEnd(const_cast<char*>("("));
        vyrag(1);
        WordEnd(const_cast<char*>(")"));
        AddDataCommand(SCRIPT_OP_LOGNOT);
        AddDataCommand(SCRIPT_OP_IF);

        const int loopPatch = m_bytecodeCursor;
        *reinterpret_cast<int*>(m_bytecode+loopPatch) = bodyStart-loopPatch;
        m_bytecodeCursor += 4;
        for (int i = 0;localBreaks[i] != 0;++i)
            *reinterpret_cast<int*>(m_bytecode+localBreaks[i]) = m_bytecodeCursor-localBreaks[i];
        return;
    }

    if (Word(const_cast<char*>("for")))
    {
        int localBreaks[129];
        localBreaks[0] = 0;
        for (int i = 1;i<128;++i)localBreaks[i] = 0;

        WordEnd(const_cast<char*>("("));
        vyrag_oper();
        WordEnd(const_cast<char*>(";"));

        const int conditionStart = m_bytecodeCursor;
        vyrag(1);
        WordEnd(const_cast<char*>(";"));

        m_bytecode[m_bytecodeCursor] = SCRIPT_OP_IF;
        ++m_bytecodeCursor;
        const int exitPatch = m_bytecodeCursor;
        m_bytecodeCursor += 4;

        m_bytecode[m_bytecodeCursor] = SCRIPT_OP_JUMP;
        ++m_bytecodeCursor;
        const int bodyEntryPatch = m_bytecodeCursor;
        m_bytecodeCursor += 4;
        const int incrementStart = m_bytecodeCursor;

        vyrag_oper();
        WordEnd(const_cast<char*>(")"));

        m_bytecode[m_bytecodeCursor] = SCRIPT_OP_JUMP;
        ++m_bytecodeCursor;
        const int conditionBackPatch = m_bytecodeCursor;
        *reinterpret_cast<int*>(m_bytecode+conditionBackPatch) = conditionStart-conditionBackPatch;
        m_bytecodeCursor += 4;
        *reinterpret_cast<int*>(m_bytecode+bodyEntryPatch) = m_bytecodeCursor-bodyEntryPatch;

        oper(localBreaks);

        m_bytecode[m_bytecodeCursor] = SCRIPT_OP_JUMP;
        ++m_bytecodeCursor;
        const int bodyBackPatch = m_bytecodeCursor;
        *reinterpret_cast<int*>(m_bytecode+bodyBackPatch) = incrementStart-bodyBackPatch;
        m_bytecodeCursor += 4;
        *reinterpret_cast<int*>(m_bytecode+exitPatch) = m_bytecodeCursor-exitPatch;

        for (int i = 0;localBreaks[i] != 0;++i)
            *reinterpret_cast<int*>(m_bytecode+localBreaks[i]) = m_bytecodeCursor-localBreaks[i];
        return;
    }

    if (Word(const_cast<char*>("break")))
    {
        WordEnd(const_cast<char*>(";"));
        if (breakPatchList == 0)
        {
            STRING message("'break' without loop");
            Error(E_ERROR, message, 0);
        }

        int i = 0;
        while (breakPatchList[i] != 0)
        {
            if (i >= 128)
            {
                STRING message("Too many 'break'");
                Error(E_ERROR, message, 0);
            }
            ++i;
        }

        m_bytecode[m_bytecodeCursor] = SCRIPT_OP_JUMP;
        ++m_bytecodeCursor;
        breakPatchList[i] = m_bytecodeCursor;
        m_bytecodeCursor += 4;
        breakPatchList[i+1] = 0;
        return;
    }

    if (Word(const_cast<char*>("goto")))
    {
        STRING name;
        GetName(&name);
        int index = m_functionList.Location(name);
        int inserted = 0;
        if (index<0)
        {
            SCRIPTVAR newLabel;
            newLabel.m_type = 8u;
            newLabel.m_functionFlags = 0u;
            newLabel.m_valueOrOffset = m_bytecodeCursor+1;
            m_functionList.Insert(name, newLabel);
            index = m_functionList.m_count-1;
            inserted = 1;
        }

        if (!inserted)
        {
            if (m_functionList.Record(index).m_value.m_type == 8u)
            {
                STRING first = "second use undefined label '"+name;
                STRING message = first+"'";
                Error(E_ERROR, message, 0);
            }
            else if (m_functionList.Record(index).m_value.m_type != 7u)
            {
                STRING first = "'"+name;
                STRING message = first+"' is not label";
                Error(E_ERROR, message, 0);
            }
        }

        m_bytecode[m_bytecodeCursor] = SCRIPT_OP_JUMP;
        ++m_bytecodeCursor;
        const int patch = m_bytecodeCursor;
        *reinterpret_cast<int*>(m_bytecode+patch) = m_functionList.Record(index).m_value.m_valueOrOffset-patch;
        m_bytecodeCursor += 4;
        return;
    }

    if (Word(const_cast<char*>("{")))
    {
        const int savedFunctions = m_functionList.m_count;
        if (Word(const_cast<char*>("}")) == 0)
        {
            for (;;)
            {
                if (m_errorLatch != 0)break;
                oper(breakPatchList);
                if (Word(const_cast<char*>("}")) != 0)break;
            }
        }

        if (savedFunctions <= 0)
        {
            m_functionList.Release();
        }
        else if (savedFunctions<m_functionList.m_count)
        {
            m_functionList.m_count = savedFunctions;
        }
        return;
    }

    vyrag_oper();
    WordEnd(const_cast<char*>(";"));
}

int SCRIPT::func()
{
    DispatchMapPresentation(0u);
    SCRIPTVAR function;
    function.m_type = 0u;
    function.m_functionFlags = 0u;
    STRING name;
    if (skipempty2() != 0)return 1;
    if (m_errorLatch != 0)return 1;


    if (m_sourceCursor[0] == '#' && m_sourceCursor[1] == 'd' && m_sourceCursor[2] == 'e' &&
       m_sourceCursor[3] == 'f' && m_sourceCursor[4] == 'i' && m_sourceCursor[5] == 'n' &&
       m_sourceCursor[6] == 'e')
    {
        const int sourceLine = m_sourceLine;
        m_sourceCursor+=7;
        m_parseMode = 1;
        GetName(&name);
        m_parseMode = 0;
        {
            STRING value;
            if (sourceLine == m_sourceLine)GetLine(&value);

            const int old = m_defineList.Location(name);
            if (old >= 0)m_defineList.Record(old).m_value = value;
            else m_defineList.Insert(name, value);
        }
        return skipempty2();
    }

    if (m_sourceCursor[0] == '#' && m_sourceCursor[1] == 'u' && m_sourceCursor[2] == 'n' &&
       m_sourceCursor[3] == 'd' && m_sourceCursor[4] == 'e' && m_sourceCursor[5] == 'f')
    {
        m_sourceCursor+=6;
        m_parseMode = 1;
        GetName(&name);
        m_parseMode = 0;
        const int found = m_defineList.Location(name);
        if (found >= 0)
        {
            --m_defineList.m_count;
            if (found<m_defineList.m_count)
            {
                for (int i = found;i<m_defineList.m_count;++i)
                    m_defineList.m_data[i] = m_defineList.m_data[i+1];
            }
            if (m_defineList.m_count == 0)
                static_cast<LIST<NAMED_LIST_STRUCT<STRING> >&>(m_defineList).Release();
        }
        else
        {
            STRING detail("#undef parameters");
            Error(E_INVALID, detail, 0);
        }
        return skipempty2();
    }

    if (Word(const_cast<char*>("#include")))
    {
        {
            const STRING savedFile = m_scriptFileName;
            char path[1024];
            int pathLength = 0;

            const char first = *m_sourceCursor;
            if (first != '\"' && first != '<')
            {
                STRING detail("include file name");
                Error(E_MISSING, detail, 0);
            }
            else
            {
                ++m_sourceCursor;
                for (;;)
                {
                    const char ch = *m_sourceCursor;
                    if (ch == '\"' || ch == '>')break;
                    if (m_sourceCursor<m_sourceEnd)
                    {
                        path[pathLength++] = ch;
                        ++m_sourceCursor;
                    }
                    else
                    {
                        STRING detail("End of file");
                        Error(E_ERROR, detail, 0);
                    }
                }
                path[pathLength] = '\0';
                ++m_sourceCursor;
            }

            std::FILE* const includeFile = std::fopen(path, "rb");
            if (includeFile == 0)
            {
                m_errorLatch = 1;
                STRING detail(path);
                Error(E_OPEN, detail, 0);
                return 1;
            }

            const unsigned int sourceSize = static_cast<unsigned int>(_filelength(BitsToSigned32(static_cast<unsigned int>(_fileno(includeFile)))));
            char* const savedCursor = m_sourceCursor;
            char* const savedEnd = m_sourceEnd;
            void* const savedAllocation = m_sourceAllocation;
            const int savedLine = m_sourceLine;
            const int savedConditional = m_conditionalDepth;

            m_sourceAllocation = static_cast<unsigned char*>(CrtNew(sourceSize+4096u));
            if (!m_sourceAllocation)
            {
                STRING detail("include");
                Error(E_MEMORY, detail, 0);
            }

            m_sourceCursor = reinterpret_cast<char*>(m_sourceAllocation)+4066u;
            m_sourceEnd = m_sourceCursor+sourceSize;
            (void)std::fread(m_sourceCursor, sourceSize, 1u, includeFile);
            m_sourceLine = 0;
            m_scriptFileName = path;
            m_conditionalDepth = 0;

            int status = func();
            while (status == 0 && m_errorLatch == 0)status = func();

            (void)std::fclose(includeFile);
            CrtFreePointer(m_sourceAllocation);
            m_sourceAllocation = savedAllocation;
            m_sourceEnd = savedEnd;
            m_sourceCursor = savedCursor;
            m_sourceLine = savedLine;
            m_conditionalDepth = savedConditional;
            m_scriptFileName = savedFile;
        }
        return skipempty2();
    }

    if (Word(const_cast<char*>("extern")))
    {
        const int savedFunctions = m_functionList.m_count;
        GetName(&name);
        const int found = m_functionList.Location(name);
        if (found >= 0)
        {
            STRING detail("function redefinition");
            Error(E_ERROR, detail, 0);
        }

        function.m_type = 2u;
        function.m_parameterStackBase = m_stack.m_count;
        WordEnd(const_cast<char*>("("));
        for (;;)
        {
            if (Word(const_cast<char*>("int")))IntVar(TYPE_VAR_PARAMETER);
            else if (Word(const_cast<char*>("string")))StringVar(TYPE_VAR_PARAMETER);
            if (Word(const_cast<char*>(",")) != 0 && m_errorLatch == 0)continue;
            break;
        }
        WordEnd(const_cast<char*>(")"));
        function.m_elementOrParameterCount = m_stack.m_count-function.m_parameterStackBase;
        function.m_valueOrOffset = GetInt();
        if (CrtIsDigit(static_cast<unsigned int>(static_cast<signed char>(m_sourceCursor[-1]))) == 0u)
        {
            STRING detail("extern function code");
            Error(E_MISSING, detail, 0);
        }

        if (savedFunctions <= 0)
            static_cast<LIST<NAMED_LIST_STRUCT<SCRIPTVAR> >&>(m_functionList).Release();
        else if (savedFunctions<m_functionList.m_count)
            m_functionList.m_count = savedFunctions;

        m_functionList.Insert(name, function);
        WordEnd(const_cast<char*>(";"));
        return skipempty2();
    }

    if (Word(const_cast<char*>("static")))
    {
        if (Word(const_cast<char*>("int")))
        {
            do
            {
                IntVar(TYPE_VAR_STATIC_GLOBAL);
            }
            while (Word(const_cast<char*>(",")) != 0 && m_errorLatch == 0);
            WordEnd(const_cast<char*>(";"));
            return skipempty2();
        }
        if (Word(const_cast<char*>("string")))
        {
            do
            {
                StringVar(TYPE_VAR_STATIC_GLOBAL);
            }
            while (Word(const_cast<char*>(",")) != 0 && m_errorLatch == 0);
            WordEnd(const_cast<char*>(";"));
            return skipempty2();
        }
        STRING detail("static variable");
        Error(E_INVALID, detail, 0);
        return skipempty2();
    }

    if (Word(const_cast<char*>("int")))
    {
        do
        {
            IntVar(TYPE_VAR_GLOBAL);
        }
        while (Word(const_cast<char*>(",")) != 0 && m_errorLatch == 0);
        WordEnd(const_cast<char*>(";"));
        return skipempty2();
    }
    if (Word(const_cast<char*>("string")))
    {
        do
        {
            StringVar(TYPE_VAR_GLOBAL);
        }
        while (Word(const_cast<char*>(",")) != 0 && m_errorLatch == 0);
        WordEnd(const_cast<char*>(";"));
        return skipempty2();
    }

    const int savedFunctions = m_functionList.m_count;
    GetName(&name);

    function.m_type = 3u;
    function.m_valueOrOffset = m_bytecodeCursor;
    function.m_parameterStackBase = m_stack.m_count;

    WordEnd(const_cast<char*>("("));
    int existing = m_functionList.Location(name);
    if (existing >= 0)m_currentFunction = &m_functionList.Record(existing).m_value;
    else m_currentFunction = &function;

    int parameter = 0;
    for (;;)
    {
        if (Word(const_cast<char*>("int")))
        {
            IntVar(TYPE_VAR_PARAMETER);
            if (existing >= 0)
            {
                NAMED_LIST_STRUCT<SCRIPTVAR>& declaration = m_functionList.Record(m_functionList.m_count-1);
                declaration.m_value.m_valueOrOffset = m_functionList.Record(existing).m_value.m_parameterStackBase+parameter;
            }
        }
        else if (Word(const_cast<char*>("string")))
        {
            StringVar(TYPE_VAR_PARAMETER);
            if (existing >= 0)
            {
                NAMED_LIST_STRUCT<SCRIPTVAR>& declaration = m_functionList.Record(m_functionList.m_count-1);
                declaration.m_value.m_valueOrOffset = m_functionList.Record(existing).m_value.m_parameterStackBase+parameter;
            }
        }
        ++parameter;
        if (Word(const_cast<char*>(",")) != 0 && m_errorLatch == 0)continue;
        break;
    }
    WordEnd(const_cast<char*>(")"));
    function.m_elementOrParameterCount = m_stack.m_count-function.m_parameterStackBase;

    if (existing >= 0)
    {
        const SCRIPTVAR& old = m_functionList.Record(existing).m_value;
        if (old.m_elementOrParameterCount != function.m_elementOrParameterCount || old.m_valueOrOffset != -1)
        {
            m_errorLatch = 1;
            STRING detail("function redefinition");
            Error(E_ERROR, detail, 0);
            return 1;
        }
        if (function.m_parameterStackBase <= 0)m_stack.Release();
        else if (function.m_parameterStackBase<m_stack.m_count)m_stack.m_count = function.m_parameterStackBase;
    }

    if (Word(const_cast<char*>(";")))
    {
        function.m_valueOrOffset = -1;
    }
    else
    {
        if (existing >= 0)m_functionList.Record(existing).m_value.m_valueOrOffset = function.m_valueOrOffset;
        WordEnd(const_cast<char*>("{"));
        while (Word(const_cast<char*>("}")) == 0 && m_errorLatch == 0)oper(0);
        AddDataCommand(SCRIPT_OP_RETURN);
    }

    if (savedFunctions <= 0)
        static_cast<LIST<NAMED_LIST_STRUCT<SCRIPTVAR> >&>(m_functionList).Release();
    else if (savedFunctions<m_functionList.m_count)
        m_functionList.m_count = savedFunctions;

    if (existing >= 0)return skipempty2();

    m_functionList.Insert(name, function);
    if (strcmp(name.m_value, "main") == 0)
    {
        m_fallbackFunction = m_functionList.m_count-1;
        return skipempty2();
    }

    const char* const eventPrefix = "ScriptEvent";
    if (CrtStrncmp(PtrToU32(name.m_value), PtrToU32(eventPrefix), static_cast<unsigned int>(std::strlen(eventPrefix))) == 0u)
    {
        int valid;
        {
            STRING suffix = name.After(eventPrefix);
            const signed char first = static_cast<signed char>(suffix.m_value[0]);
            valid = (CrtIsDigit(static_cast<unsigned int>(first)) != 0u) ||
                (first == '-' && CrtIsDigit(static_cast<unsigned int>(static_cast<signed char>(suffix.m_value[1]))) != 0u);
        }
        if (valid)
        {
            int event;
            {
                STRING suffix = name.After(eventPrefix);
                if (suffix.m_value[1] == 'x')
                    (void)std::sscanf(suffix.m_value, "%i", &event);
                else
                    event = static_cast<int>(CrtAtoiText(suffix.m_value));
            }

            if (function.m_elementOrParameterCount != 3)
            {
                m_errorLatch = 1;
                STRING detail("ScriptEvent must have 3 parameters");
                Error(E_ERROR, detail, 0);
                return 1;
            }
            if (static_cast<unsigned int>(event) >= 256u)
            {
                STRING detail("too big number in function name");
                Error(E_ERROR, detail, 0);
            }
            else
            {
                m_eventMap[event] = m_functionList.m_count-1;
            }
        }
    }
    return skipempty2();
}

int SCRIPT::CallFunction(int function, const char* signature, int argument1, int argument2, int argument3)
{

    if (m_errorLatch != 0 || m_bytecode == 0)return 0;

    if (function<0)function = m_fallbackFunction;
    if (function<0 || function >= m_functionList.m_count)
    {
        ::Error->Log("!!!ERROR!!! SCRIPT Call unexisted function %i", function);
        return 0;
    }

    if (m_functionList.Record(function).m_value.m_type != 3)
    {
        const STRING functionName = m_functionList.Name(function);
        ::Error->Log("!!!ERROR!!!SCRIPT: Call unexisted function %s()", functionName.m_value);
        return 0;
    }

    const int savedStack = m_stack.m_count;
    PushInt(savedStack);
    PushInt(m_bytecodeCursor);

    int frameBase = m_stack.m_count;
    int cursor = m_functionList.Record(function).m_value.m_valueOrOffset;
    int controlAnchor = cursor;
    int arrayActive = 0;
    int arrayOffset = 0;
    int result = 0;
    int trackedPointer = 0x7FFFFFFF;

    const int rawArgs[3] = {argument1, argument2, argument3};
    static const char* const kStringSuffix[3] = {"(str const", "(..., str const", "(..., ...,  str const"};
    static const char* const kPointerSuffix[3] = {"(obj const", "(..., obj const", "(..., ...,  obj const"};
    static const char* const kIntegerSuffix[3] = {"(int const", "(..., int const", "(..., ...,  int const"};
    for (int i = 0;i<3 && i<m_functionList.Record(function).m_value.m_elementOrParameterCount;++i)
    {
        const char kind = signature[i];
        const char* const suffix = (kind == 's')?kStringSuffix[i]:((kind == 'p')?kPointerSuffix[i]:kIntegerSuffix[i]);
        const STRING functionName = m_functionList.Name(function);
        const STRING diagnostic = functionName+suffix;
        SCRIPTSTACK temporary(rawArgs[i], diagnostic);
        if ((kind == 's' || kind == 'p') && rawArgs[i] != 0)
            temporary.m_flags |= SCRIPT_STACK_OBJECT;
        m_stack.m_data[m_functionList.Record(function).m_value.m_parameterStackBase+i] = temporary;
    }

    if (signature[0] == 'p')trackedPointer = argument1;
    else if (signature[1] == 'p')trackedPointer = argument2;
    else if (signature[2] == 'p')trackedPointer = argument3;

    m_instructionTraceAddresses[0] = PtrToU32(m_allocation38);

    while (cursor<m_bytecodeCursor)
    {
        m_instructionTraceAddresses[1] = m_instructionTraceAddresses[0];
        m_instructionTraceAddresses[0] = PtrToU32(m_allocation38)+static_cast<unsigned int>(cursor)*4u;

        if (m_stack.m_count<frameBase)
        {
            ::Error->Log("!!!ERROR!!!SCRIPT: '%s' stack error %i", "pop, but not push", cursor);
        }

        const unsigned char opcode = m_bytecode[cursor];

        if (opcode >= 6u && opcode <= 23u)
        {
            ++cursor;
            SCRIPTSTACK& lhs = m_stack.m_data[m_stack.m_count-2];
            SCRIPTSTACK& rhs = m_stack.m_data[m_stack.m_count-1];
            lhs.BinarOperator(static_cast<int>(opcode), rhs);
            --m_stack.m_count;
            continue;
        }

        if (opcode >= 32u && opcode <= 39u)
        {
            ++cursor;
            const int baseIndex = *reinterpret_cast<const int*>(m_bytecode+cursor);
            SCRIPTSTACK& base = m_stack.m_data[baseIndex];
            int targetIndex = baseIndex;
            if ((base.m_flags&0x20u) != 0u && arrayActive != 0)targetIndex = base.m_intValue;
            targetIndex += arrayOffset;
            SCRIPTSTACK& target = m_stack.m_data[targetIndex];

            switch (opcode)
            {
            case SCRIPT_OP_POST_INC:
            case SCRIPT_OP_POST_DEC:
                {
                m_stack.Insert(target);

                SCRIPTSTACK& currentTarget = m_stack.m_data[targetIndex];
                currentTarget.m_flags = static_cast<unsigned char>(currentTarget.m_flags&0xEFu);
                if ((currentTarget.m_flags&0x22u) != 0u)
                    currentTarget.m_intValue = AddSigned32Wrap(currentTarget.m_intValue, (opcode == 32u)?1:-1);
                cursor += 4;
                break;
            }

            case SCRIPT_OP_PRE_INC:
            case SCRIPT_OP_PRE_DEC:
                target.m_flags = static_cast<unsigned char>(target.m_flags&0xEFu);
                if ((target.m_flags&0x22u) != 0u)
                    target.m_intValue = AddSigned32Wrap(target.m_intValue, (opcode == 34u)?1:-1);
                m_stack.Insert(target);
                cursor += 4;
                break;

            case SCRIPT_OP_READ:
                if (arrayActive == 0 && (target.m_flags&0x04u) != 0u)PushInt(baseIndex);
                else m_stack.Insert(target);
                cursor += 4;
                break;

            case SCRIPT_OP_ADDRESS:
                if ((target.m_flags&0x01u) != 0u)PushInt(PtrToU32(&target.m_stringValue));
                else PushInt(PtrToU32(&target.m_intValue));
                cursor += 4;
                break;

            case SCRIPT_OP_ASSIGN:
                {
                SCRIPTSTACK& source = m_stack.m_data[m_stack.m_count-1];
                if ((base.m_flags&0x01u) != 0u)
                    {
                    if (arrayActive != 0 && (base.m_flags&0x04u) == 0u)
                        {
                        base.m_stringValue.m_value[arrayOffset] = static_cast<char>(source.Int());
                        cursor += 4;
                        break;
                    }
                    target.m_flags = static_cast<unsigned char>(target.m_flags&0xBFu);
                    if ((source.m_flags&0x02u) != 0u)
                        {
                        STRING text = Int2Str(source.m_intValue);
                        source.m_stringValue = text;
                    }
                    m_stack.m_data[targetIndex].m_stringValue = source.m_stringValue;
                }
                    else
                    {
                    target.m_flags = static_cast<unsigned char>(target.m_flags&0xAFu);
                    target.m_intValue = source.Int();
                    if ((source.m_flags&0x10u) != 0u)target.m_flags = static_cast<unsigned char>(target.m_flags|0x10u);
                }
                cursor += 4;
                break;
            }

            case SCRIPT_OP_COMPOUND:
                {
                SCRIPTSTACK& source = m_stack.m_data[--m_stack.m_count];
                const unsigned char operation = m_bytecode[cursor+4];
                target.BinarOperator(static_cast<int>(operation), source);
                m_stack.Insert(m_stack.m_data[targetIndex]);
                cursor += 5;
                break;
            }
            }

            arrayActive = 0;
            arrayOffset = 0;
            continue;
        }

        ++cursor;
        switch (opcode)
        {
        case SCRIPT_OP_PUSH_INT:
            {
            {
                const STRING tag("int const");
                const SCRIPTSTACK value(*reinterpret_cast<const int*>(m_bytecode+cursor), tag);
                m_stack.Push(value);
            }
            cursor += 4;
            break;
        }

        case SCRIPT_OP_PUSH_STRING:
            {
            {
                const STRING tag("str const");
                const STRING text(reinterpret_cast<const char*>(m_bytecode+cursor));
                const SCRIPTSTACK value(text, tag);
                m_stack.Push(value);
            }
            cursor += static_cast<int>(std::strlen(reinterpret_cast<const char*>(m_bytecode+cursor)))+1;
            break;
        }

        case SCRIPT_OP_NEGATE:
            {
            SCRIPTSTACK& top = m_stack.m_data[m_stack.m_count-1];
            top.m_intValue = BitsToSigned32(0u-static_cast<unsigned int>(top.Int()));
            top.m_flags = 2u;
            break;
        }
        case SCRIPT_OP_BITNOT:
            {
            SCRIPTSTACK& top = m_stack.m_data[m_stack.m_count-1];
            top.m_intValue = ~top.Int();
            top.m_flags = 2u;
            break;
        }
        case SCRIPT_OP_LOGNOT:
            {
            SCRIPTSTACK& top = m_stack.m_data[m_stack.m_count-1];
            top.m_intValue = (top.Int() == 0)?1:0;
            top.m_flags = 2u;
            break;
        }

        case SCRIPT_OP_IF:
            {
            const int condition = m_stack.m_data[--m_stack.m_count].Int();
            if (condition == 0)cursor += *reinterpret_cast<const int*>(m_bytecode+cursor);
            else cursor += 4;
            if (m_stack.m_count-frameBase>1)
                {
                ::Error->Log("!!!ERROR!!!SCRIPT: '%s' stack error %i", "if", cursor);
            }
            controlAnchor = cursor;
            m_stack.m_count = frameBase;
            if (frameBase>m_stack.m_capacity)m_stack.Expand(frameBase);
            break;
        }

        case SCRIPT_OP_LINE:
            {
            if (m_stack.m_count-frameBase>1)
                {
                STRING subject("; in line ");
                STRING line = Int2Str(*reinterpret_cast<const int*>(m_bytecode+cursor));
                subject += line.m_value;
                ::Error->Log("!!!ERROR!!!SCRIPT: '%s' stack error %i", subject.m_value, cursor);
            }
            cursor += 4;
            controlAnchor = cursor;
            m_stack.m_count = frameBase;
            if (frameBase>m_stack.m_capacity)m_stack.Expand(frameBase);
            break;
        }

        case SCRIPT_OP_POP:
            --m_stack.m_count;
            break;

        case SCRIPT_OP_JUMP:
            cursor += *reinterpret_cast<const int*>(m_bytecode+cursor);
            break;

        case SCRIPT_OP_IFF:
            {
            const int condition = m_stack.m_data[--m_stack.m_count].Int();
            if (condition != 0)
                {
                m_bytecode[controlAnchor] = SCRIPT_OP_JUMP;
                *reinterpret_cast<int*>(m_bytecode+controlAnchor+1) =
                    *reinterpret_cast<const int*>(m_bytecode+cursor)-controlAnchor+cursor-1;
                cursor += 4;
            }
                else
                {
                cursor += *reinterpret_cast<const int*>(m_bytecode+cursor);
            }
            if (m_stack.m_count-frameBase>1)
                {
                ::Error->Log("!!!ERROR!!!SCRIPT: '%s' stack error %i", "iff", cursor);
            }
            controlAnchor = cursor;
            m_stack.m_count = frameBase;
            if (frameBase>m_stack.m_capacity)m_stack.Expand(frameBase);
            break;
        }

        case SCRIPT_OP_CALL:
            {
            PushInt(frameBase);
            PushInt(cursor+4);
            frameBase = m_stack.m_count;
            const int calledFunction = *reinterpret_cast<const int*>(m_bytecode+cursor);
            cursor = m_functionList.Record(calledFunction).m_value.m_valueOrOffset;
            controlAnchor = cursor;
            if (cursor<0)
                {
                STRING format("!!!ERROR!!!SCRIPT: Undefined function %s()");
                ::Error->LogExit(format,
                    m_functionList.Record(calledFunction).m_name.m_value);
            }
            break;
        }

        case SCRIPT_OP_RETURN:
            {
            if (m_stack.m_count-frameBase>1)
                {
                ::Error->Log("!!!ERROR!!!SCRIPT: '%s' stack error %i", "return", cursor);
            }

            if (m_stack.m_count>frameBase)
            {
                const SCRIPTSTACK returned(m_stack.m_data[--m_stack.m_count]);
                m_stack.m_count = frameBase;
                if (frameBase>m_stack.m_capacity)m_stack.Expand(frameBase);
                cursor = m_stack.m_data[--m_stack.m_count].Int();
                frameBase = m_stack.m_data[--m_stack.m_count].Int();
                controlAnchor = cursor;
                m_stack.Insert(returned);
                if (cursor >= m_bytecodeCursor && result == 0 && (returned.m_flags&SCRIPT_STACK_STRING) == 0u)
                    result = returned.Int();
            }
            else
            {
                m_stack.m_count = frameBase;
                if (frameBase>m_stack.m_capacity)m_stack.Expand(frameBase);
                cursor = m_stack.m_data[--m_stack.m_count].Int();
                frameBase = m_stack.m_data[--m_stack.m_count].Int();
                controlAnchor = cursor;
            }
            break;
        }

        case SCRIPT_OP_ARRAY_INDEX:
            arrayOffset = m_stack.m_data[--m_stack.m_count].Int();
            arrayActive = 1;
            break;

        case SCRIPT_OP_CAST_STRING:
            {
            SCRIPTSTACK& top = m_stack.m_data[m_stack.m_count-1];
            if ((top.m_flags&0x02u) != 0u)
                {
                STRING text = Int2Str(top.m_intValue);
                top.m_stringValue = text;
            }
            SCRIPTSTACK& converted = m_stack.m_data[m_stack.m_count-1];
            converted.m_stringValue = top.m_stringValue;
            converted.m_flags = SCRIPT_STACK_STRING;
            break;
        }

        case SCRIPT_OP_CAST_INT:
            {
            SCRIPTSTACK& top = m_stack.m_data[m_stack.m_count-1];
            top.m_intValue = top.Int();
            top.m_flags = 2u;
            break;
        }

        case SCRIPT_OP_CAST_OBJECT:
            {
            SCRIPTSTACK& top = m_stack.m_data[m_stack.m_count-1];
            top.m_intValue = top.Int();
            top.m_flags = static_cast<unsigned char>(top.m_intValue != 0?0x12u:0x02u);
            break;
        }

        case SCRIPT_OP_SHORT_CIRCUIT_AND:
            {
            SCRIPTSTACK& top = m_stack.m_data[m_stack.m_count-1];
            if (top.Int() == 0)cursor += *reinterpret_cast<const int*>(m_bytecode+cursor);
            else
                {
                    cursor += 4;
                    arrayActive = 0;
                    arrayOffset = 0;
                }
            break;
        }

        case SCRIPT_OP_SHORT_CIRCUIT_OR:
            {
            SCRIPTSTACK& top = m_stack.m_data[m_stack.m_count-1];
            if (top.Int() != 0)cursor += *reinterpret_cast<const int*>(m_bytecode+cursor);
            else
                {
                    cursor += 4;
                    arrayActive = 0;
                    arrayOffset = 0;
                }
            break;
        }

        default:
            if (opcode == SCRIPT_EXT_Destroy)
            {
                const int top = m_stack.m_data[m_stack.m_count-1].Int();
                if (top == trackedPointer)result = 1;
            }
            (void)ScriptExecFunc(static_cast<int>(opcode));
            break;
        }
    }

    m_stack.m_count = savedStack;
    if (savedStack>m_stack.m_capacity)m_stack.Expand(savedStack);
    return result;
}


