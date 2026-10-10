#include "myerror.h"
#include "engine_globals.h"
#include "exports.h"
#include "basetype.h"
#include "mylib.h"
#include <windows.h>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

MYERROR* Error=0;

namespace {
STRING& MyErrorGlobalLogPath()
{
    static STRING path;
    return path;
}

const char* const errorCategories[15]={
    "0x%X Couldn't lock %s",
    "0x%X Couldn't copy %s",
    "%i There was not enough memory for %s",
    "0x%X Couldn't create the %s",
    "0x%X Invalid %s",
    "0x%X Load %s",
    "0x%X Save %s",
    "0x%X Couldn't open '%s'",
    "0x%X Couldn't set the %s",
    "0x%X Couldn't get the %s",
    "%i %s",
    "0x%X Section can't found (%s)",
    "0x%X Unable initialize %s",
    "%i Missing %s",
    "%i Unknownn %s"
};
}

MYERROR::MYERROR(const char* baseLogDirectory, int timestampedFile)
    : hwnd(0)
{
    std::strcpy(filename, baseLogDirectory);
    STRING& logPath = MyErrorGlobalLogPath();

    {
        STRING timeText = GetTimeString();
        STRING dateText = GetDateString();
        STRING errorStem = "\\error"+dateText;
        STRING stemWithSpace = errorStem+" ";
        STRING timestampedName = stemWithSpace+timeText;
        STRING timestampedLog = timestampedName+".log";
        logPath = timestampedLog;
    }

    logPath.Replace(":", "h");
    logPath.Replace(":", "m");
    {
        STRING base(filename);
        STRING prefixed = base+logPath;
        logPath = prefixed;
    }

    {
        if (timestampedFile == 0)
        {
            STRING base(filename);
            STRING fixedLog = base+"\\error.log";
            logPath = fixedLog;
        }
        STRING mode(timestampedFile != 0 ? "wt" : "at");
        file = logPath.value[0] != 0 ? std::fopen(logPath.value, mode.value) : 0;
    }

    STRING bannerTime = GetTimeString();
    STRING bannerDate = GetDateString();
    const char* const version = crtProgramPath;
    Log(
        "----< %s %s >----< %s (%s %s) >----",
        bannerDate.value, bannerTime.value, version, "Mar 14 2007", "17:03:02");
}

MYERROR::~MYERROR()
{
    if (file != 0)
        std::fclose(file);
    file = 0;

    STRING& logPath = MyErrorGlobalLogPath();
    bool moveErrorLog;
    {
        STRING path(logPath);
        STRING afterLogs = path.After("Logs\\");
        moveErrorLog = std::strcmp(afterLogs.value, "error.log") != 0;
        if (moveErrorLog)
        {
            STRING path(logPath);
            STRING baseName = path.AfterLast("\\");
            moveErrorLog = std::strcmp(baseName.value, "error.log") != 0;
        }
    }
    if (!moveErrorLog)
        return;

    {
        STRING base(filename);
        STRING target = base+"\\error.log";
        (void)std::remove(target.value);
    }
    bool moved;
    {
        STRING base(filename);
        STRING target = base+"\\error.log";
        moved = MoveFileExA(logPath.value, target.value,
                         MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH) != 0;
    }
    if (moved)
        return;

    {
        STRING target("error.log");
        (void)std::remove(target.value);
    }
    {
        STRING target("error.log");
        (void)MoveFileExA(logPath.value, target.value,
                        MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    }
}

int __cdecl MYERROR::Window(STRING text, ...)
{
    char buffer[4096];
    va_list args;
    va_start(args, text);
    std::vsprintf(buffer, text.m_value, args);
    va_end(args);

    if (file != 0)
    {
        std::fputs(buffer, file);
        std::fputs("\n", file);
        std::fflush(file);
    }
    if (hwnd == 0)
        hwnd = GetForegroundWindow();
    const int result = MessageBoxA(hwnd, buffer, "Error", MB_OK);
    return result;
}

void __cdecl MYERROR::LogTmp(STRING text, ...)
{
    if (file == 0)
    {
        return;
    }
    char buffer[4096];
    va_list args;
    va_start(args, text);
    std::vsprintf(buffer, text.m_value, args);
    va_end(args);
    const long position = std::ftell(file);
    std::fputs(buffer, file);
    std::fputs("\n", file);
    std::fflush(file);
    std::fseek(file, position, SEEK_SET);
}

void __cdecl MYERROR::Log(STRING text, ...)
{
    if (file == 0)
    {
        return;
    }
    char buffer[4096];
    va_list args;
    va_start(args, text);
    std::vsprintf(buffer, text.m_value, args);
    va_end(args);
    std::fputs(buffer, file);
    std::fputs("\n", file);
    std::fflush(file);
}

void __cdecl MYERROR::LogExit(STRING text, ...)
{
    char buffer[4096];
    va_list args;
    va_start(args, text);
    std::vsprintf(buffer, text.m_value, args);
    va_end(args);

    Log(buffer);
    if (file != 0)
        std::fclose(file);
    file = 0;
    std::exit(1);
}

void __cdecl MYERROR::Error(const STRING& objectFormat, TYPE_ERROR type,
                            const STRING& subject, unsigned long errorCode, ...)
{
    char buffer[4096];
    {
        STRING timeText = GetTimeString();
        std::sprintf(buffer, "!!!ERROR %s/%u!!!", timeText.m_value,
                     ::CurrentTime);
    }

    va_list args;
    va_start(args, errorCode);
    std::vsprintf(buffer+std::strlen(buffer), objectFormat.m_value, args);
    va_end(args);
    std::strcat(buffer, ": ");
    if (static_cast<unsigned int>(type) <= E_UNKNOWN)
        std::strcat(buffer, errorCategories[static_cast<unsigned int>(type)]);

    Log(buffer, errorCode, subject.m_value);
}


void __cdecl MYERROR::Log(const char* format, ...)
{
    FILE* const stream = file;
    if (stream != 0)
    {
        char text[4096];
        va_list args;
        va_start(args, format);
        std::vsprintf(text, format, args);
        va_end(args);
        std::fputs(text, file);
        std::fputs("\n", file);
        std::fflush(file);
    }
}

void __cdecl MYERROR::LogExit(const char* format, ...)
{
    char text[4096];
    va_list args;
    va_start(args, format);
    std::vsprintf(text, format, args);
    va_end(args);

    Log(text);

    FILE* const stream = file;
    if (stream != 0)
        std::fclose(stream);
    file = 0;
    std::exit(1);
}
