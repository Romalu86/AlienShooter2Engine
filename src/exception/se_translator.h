#pragma once

#include "basetype.h"

struct _EXCEPTION_POINTERS;

class se_translator
{
public:
    se_translator();
    ~se_translator();
    se_translator(const se_translator&) = delete;
    se_translator& operator=(const se_translator&) = delete;

    class exception
    {
        friend class se_translator;
    public:
        ~exception();
        _EXCEPTION_POINTERS* ExceptionPointers() const
        {
            return m_exceptionPointers;
        }
    protected:
        explicit exception(_EXCEPTION_POINTERS* pointers)
            : m_exceptionPointers(pointers)
        {
        }
        exception(const exception& source);
        _EXCEPTION_POINTERS* m_exceptionPointers;
    };

#define AS2_SE_EXCEPTION_CLASS(name) \
    class name : public exception { \
        friend class se_translator; \
    public: \
        name(const name& source) = default; \
        ~name() = default; \
    protected: \
        explicit name(_EXCEPTION_POINTERS* pointers); \
    }

    AS2_SE_EXCEPTION_CLASS(access_violation);
    AS2_SE_EXCEPTION_CLASS(no_memory);
    AS2_SE_EXCEPTION_CLASS(datatype_misalignment);
    AS2_SE_EXCEPTION_CLASS(breakpoint);
    AS2_SE_EXCEPTION_CLASS(single_step);
    AS2_SE_EXCEPTION_CLASS(array_bounds_exceeded);
    AS2_SE_EXCEPTION_CLASS(flt_denormal_operand);
    AS2_SE_EXCEPTION_CLASS(flt_divide_by_zero);
    AS2_SE_EXCEPTION_CLASS(flt_inexact_result);
    AS2_SE_EXCEPTION_CLASS(flt_invalid_operation);
    AS2_SE_EXCEPTION_CLASS(flt_overflow);
    AS2_SE_EXCEPTION_CLASS(flt_stack_check);
    AS2_SE_EXCEPTION_CLASS(flt_underflow);
    AS2_SE_EXCEPTION_CLASS(int_divide_by_zero);
    AS2_SE_EXCEPTION_CLASS(int_overflow);
    AS2_SE_EXCEPTION_CLASS(priv_instruction);
    AS2_SE_EXCEPTION_CLASS(in_page_error);
    AS2_SE_EXCEPTION_CLASS(illegal_instruction);
    AS2_SE_EXCEPTION_CLASS(noncontinuable_exception);
    AS2_SE_EXCEPTION_CLASS(stack_overflow);
    AS2_SE_EXCEPTION_CLASS(invalid_disposition);
    AS2_SE_EXCEPTION_CLASS(guard_page);
    AS2_SE_EXCEPTION_CLASS(invalid_handle);
    AS2_SE_EXCEPTION_CLASS(microsoft_cpp);
    AS2_SE_EXCEPTION_CLASS(zm_random_synchro);

#undef AS2_SE_EXCEPTION_CLASS

    static void __cdecl translator(unsigned int code, _EXCEPTION_POINTERS* pointers);
    static const char* __cdecl name(unsigned int code);

private:
    typedef void (__cdecl *translator_function)(unsigned int, _EXCEPTION_POINTERS*);
    translator_function m_previous;
};

