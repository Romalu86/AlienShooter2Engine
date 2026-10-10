#include "exception/se_translator.h"
#include "mylib.h"
#include <windows.h>
#include <cstdlib>
#include <eh.h>

se_translator::se_translator()
    : m_previous(_set_se_translator(&translator))
{
}

se_translator::~se_translator()
{
    (void)_set_se_translator(m_previous);
}

se_translator::exception::exception(const exception& source)
    : m_exceptionPointers(source.m_exceptionPointers)
{
}

se_translator::exception::~exception()
{
}

#define AS2_DEFINE_SE_EXCEPTION_CTOR(name) \
    se_translator::name::name(_EXCEPTION_POINTERS* pointers) \
        : exception(pointers) \
    { \
    }

AS2_DEFINE_SE_EXCEPTION_CTOR(access_violation)
AS2_DEFINE_SE_EXCEPTION_CTOR(no_memory)
AS2_DEFINE_SE_EXCEPTION_CTOR(datatype_misalignment)
AS2_DEFINE_SE_EXCEPTION_CTOR(breakpoint)
AS2_DEFINE_SE_EXCEPTION_CTOR(single_step)
AS2_DEFINE_SE_EXCEPTION_CTOR(array_bounds_exceeded)
AS2_DEFINE_SE_EXCEPTION_CTOR(flt_denormal_operand)
AS2_DEFINE_SE_EXCEPTION_CTOR(flt_divide_by_zero)
AS2_DEFINE_SE_EXCEPTION_CTOR(flt_inexact_result)
AS2_DEFINE_SE_EXCEPTION_CTOR(flt_invalid_operation)
AS2_DEFINE_SE_EXCEPTION_CTOR(flt_overflow)
AS2_DEFINE_SE_EXCEPTION_CTOR(flt_stack_check)
AS2_DEFINE_SE_EXCEPTION_CTOR(flt_underflow)
AS2_DEFINE_SE_EXCEPTION_CTOR(int_divide_by_zero)
AS2_DEFINE_SE_EXCEPTION_CTOR(int_overflow)
AS2_DEFINE_SE_EXCEPTION_CTOR(priv_instruction)
AS2_DEFINE_SE_EXCEPTION_CTOR(in_page_error)
AS2_DEFINE_SE_EXCEPTION_CTOR(illegal_instruction)
AS2_DEFINE_SE_EXCEPTION_CTOR(noncontinuable_exception)
AS2_DEFINE_SE_EXCEPTION_CTOR(stack_overflow)
AS2_DEFINE_SE_EXCEPTION_CTOR(invalid_disposition)
AS2_DEFINE_SE_EXCEPTION_CTOR(guard_page)
AS2_DEFINE_SE_EXCEPTION_CTOR(invalid_handle)
AS2_DEFINE_SE_EXCEPTION_CTOR(microsoft_cpp)
AS2_DEFINE_SE_EXCEPTION_CTOR(zm_random_synchro)

#undef AS2_DEFINE_SE_EXCEPTION_CTOR

void __cdecl se_translator::translator(unsigned int code, _EXCEPTION_POINTERS* pointers)
{
    (void)code;
    const unsigned int exceptionCode =
        static_cast<unsigned int>(pointers->ExceptionRecord->ExceptionCode);
    switch (exceptionCode)
    {
    case EXCEPTION_BREAKPOINT:
        throw breakpoint(pointers);
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        throw datatype_misalignment(pointers);
    case EXCEPTION_GUARD_PAGE:
        throw guard_page(pointers);
    case EXCEPTION_SINGLE_STEP:
        throw single_step(pointers);
    case EXCEPTION_IN_PAGE_ERROR:
        throw in_page_error(pointers);
    case EXCEPTION_ACCESS_VIOLATION:
        throw access_violation(pointers);
    case EXCEPTION_INVALID_HANDLE:
        throw invalid_handle(pointers);
    case STATUS_NO_MEMORY:
        throw no_memory(pointers);
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        throw array_bounds_exceeded(pointers);
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        throw illegal_instruction(pointers);
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        throw noncontinuable_exception(pointers);
    case EXCEPTION_INVALID_DISPOSITION:
        throw invalid_disposition(pointers);
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        throw flt_denormal_operand(pointers);
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        throw flt_divide_by_zero(pointers);
    case EXCEPTION_FLT_INEXACT_RESULT:
        throw flt_inexact_result(pointers);
    case EXCEPTION_FLT_INVALID_OPERATION:
        throw flt_invalid_operation(pointers);
    case EXCEPTION_FLT_OVERFLOW:
        throw flt_overflow(pointers);
    case EXCEPTION_FLT_STACK_CHECK:
        throw flt_stack_check(pointers);
    case EXCEPTION_FLT_UNDERFLOW:
        throw flt_underflow(pointers);
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        throw int_divide_by_zero(pointers);
    case EXCEPTION_INT_OVERFLOW:
        throw int_overflow(pointers);
    case EXCEPTION_PRIV_INSTRUCTION:
        throw priv_instruction(pointers);
    case EXCEPTION_STACK_OVERFLOW:
        throw stack_overflow(pointers);
    case 0xE06D7363u:
        throw microsoft_cpp(pointers);
    case 0xE0000666u:
        throw zm_random_synchro(pointers);
    default:
        throw exception(pointers);
    }
}


const char* __cdecl se_translator::name(unsigned int code)
{
    switch (code)
    {
    case EXCEPTION_GUARD_PAGE:
        return "Guard Page";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return "Datatype Misalignment";
    case EXCEPTION_BREAKPOINT:
        return "Breakpoint";
    case EXCEPTION_SINGLE_STEP:
        return "Single Step";
    case EXCEPTION_ACCESS_VIOLATION:
        return "Access Violation";
    case EXCEPTION_IN_PAGE_ERROR:
        return "In Page Error";
    case EXCEPTION_INVALID_HANDLE:
        return "Invalid Handle";
    case STATUS_NO_MEMORY:
        return "No Memory";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "Illegal Instruction";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        return "Noncontinuable Exception";
    case EXCEPTION_INVALID_DISPOSITION:
        return "Invalid Disposition";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        return "Array Bounds Exceeded";
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        return "Float Denormal Operand";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return "Float Divide by Zero";
    case EXCEPTION_FLT_INEXACT_RESULT:
        return "Float Inexact Result";
    case EXCEPTION_FLT_INVALID_OPERATION:
        return "Float Invalid Operation";
    case EXCEPTION_FLT_OVERFLOW:
        return "Float Overflow";
    case EXCEPTION_FLT_STACK_CHECK:
        return "Float Stack Check";
    case EXCEPTION_FLT_UNDERFLOW:
        return "Float Underflow";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "Integer Divide by Zero";
    case EXCEPTION_INT_OVERFLOW:
        return "Integer Overflow";
    case EXCEPTION_PRIV_INSTRUCTION:
        return "Privileged Instruction";
    case EXCEPTION_STACK_OVERFLOW:
        return "Stack Overflow";
    case 0xE06D7363u:
        return "Microsoft C++ Exception";
    default:
    {
        static char codeText[] = "0x000000000";
        return _itoa(static_cast<int>(code), codeText + 2, 16);
    }
    }
}
