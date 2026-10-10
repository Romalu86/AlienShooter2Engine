
#include <new>
#include <cstdlib>
#include <limits>
#include <cmath>
#include "engine_globals.h"
#include <clocale>
#include "zm/Game/RecordDB/zRecordDB.h"
#include "zm/Survive/zSurvive.h"
#include "zm/Game/ScriptExec/zScriptExec.h"
#include "zm/Utils/zStaticStorage.h"
#include "mylib.h"
#include "zm/Game/RecordDB/RecordDBStreambufControl.h"
#include "basetype.h"
void  Msvc6LocaleFacetDecref(void* self);
void*  Msvc6LocaleDefaultCtor(void* self);
void*  RecordDBStringAssignBytes(void* self, const void* source, unsigned int count);
#include "input.h"
#include "registry.h"
#include "zm/IniParser/zIniParser.h"
#if defined(_WIN32)
#  define INPUT WIN32_INPUT
#endif
#include <windows.h>
#include <d3d9.h>
#include <dinput.h>
#include <mmsystem.h>
#if defined(_WIN32)
#  undef INPUT
#endif
#include "runtime/win32_callbacks.h"
#include "runtime/crt_locale.h"
#include "runtime/locale_state.h"
#include "runtime/locale_tables.h"
#include <stdarg.h>
#include <float.h>
#if defined(_MSC_VER) && defined(_M_IX86)
#include <xmmintrin.h>
#include <isa_availability.h>
extern "C" int __isa_available;
#endif
#include <malloc.h>
#include <io.h>
#include <errno.h>
#include "map.h"
#include "zm/Game/zItemTaker.h"
#include "player_arcade.h"
#include "rail.h"
#include "engine.h"
#include "sprite.h"
#include "sprite_collector.h"
#include "sprite_list.h"
#include "exports.h"
#include "graph.h"
#include "texture.h"
#include "system_font.h"
#include "legacy_d3dx_effect.h"
#include "net.h"
#include "constant.h"
#include "zm/Input/zInputMap.h"
#include "exception/se_translator.h"
void __cdecl DialogGlobalStringCleanup();
void __cdecl DialogServiceGlobalStringCleanup();

namespace {
GRAPH_INIT graphStartup = {
    "", {800u, 1024u}, {600u, 768u}, {32u},
    0u, 0u, 800u, 600u, 32u, 1u
};
char startupResourceName[256] = {};
char startupResourceLength[256] = "0";
}

const long long Msvc6::_Fpz = 0;
int Msvc6::FposInitialState = 0;
int Msvc6::FilebufInitialState = 0;

unsigned char CrtStaticInitializationGuards[4] = {};
char* crtProgramPath = 0;

VID* EmptyVid = 0;
SOUND* Sound = 0;
REGISTRY* Registry = 0;
PROFILE* Strings = 0;
int NoBuildAlpha = 0;
ADD_CREATE_SPRITE* AddCreateSprite = 0;
zProfLdr* g_pProfLdr = 0;
zDlgSrv* g_pDlgSrv = 0;
iLogConsole* gpLogCons = 0;
iIniParser* g_pParser = 0;
bool* g_abMonsterVid = 0;
int* g_aiNamedMonstersLast = 0;
void (__cdecl *DrawLoadScreenTact)(int) = &MylibNoOp;

void __cdecl SFINIT0_MonsterVidAllocate(void)
{
    g_abMonsterVid = static_cast<bool*>(
        VirtualAlloc(0, 4096u, MEM_COMMIT, PAGE_READWRITE));
}


void __cdecl SFINIT1_Proc(void)
{
    g_aiNamedMonstersLast = static_cast<int*>(
        VirtualAlloc(0, 10u*sizeof(int), MEM_COMMIT, PAGE_READWRITE));
}


void __cdecl SFINIT2_Proc(void)
{
    IniLineBuffer = static_cast<char*>(VirtualAlloc(0, 1000u, MEM_COMMIT, PAGE_READWRITE));
}

class DIALOG_ITEM
{
public:
    HWND window;
    unsigned int controlId;
    STRING GetText() const;
};

class DIALOG_TEXT : public DIALOG_ITEM
{
public:
    operator STRING() const;
};

STRING DIALOG_ITEM::GetText() const
{
    char buffer[32000];
    *reinterpret_cast<unsigned int*>(buffer) = 0u;
    GetDlgItemTextA(window, static_cast<int>(controlId), buffer, 32000);
    STRING result = {StringEmptySentinel()};
    result = buffer;
    return result;
}

DIALOG_TEXT::operator STRING() const
{
    return GetText();
}

STRING MAP::OpenDialog(const char* filter, const char* defaultExtension) const
{
    return OpenSaveDialog(0, filter, defaultExtension);
}

namespace {

StaticObjectStorage<STRING> synchroErrorLogFileNameStorage;
StaticObjectStorage<STRING> dialogPasswordStorage;
unsigned char dialogPasswordInitializationState = 0;

struct MainSwDialogItemRef
{
    HWND window;
    unsigned int controlId;
};

static STRING& MainSwDialogPassword()
{
    STRING* const password = dialogPasswordStorage.Address();
    const unsigned char guard = dialogPasswordInitializationState;
    if ((guard&1u) == 0u)
    {
        ::new (password) STRING();
        dialogPasswordInitializationState = static_cast<unsigned char>(guard|1u);
        (void)RegisterCrtAtExit(&DialogGlobalStringCleanup);
    }
    return *password;
}

static STRING MainSwReadDialogText(HWND window, unsigned int controlId)
{
    DIALOG_TEXT item;
    item.window = window;
    item.controlId = controlId;
    return static_cast<STRING>(item);
}

static void MainSwSyncStartupDialog(HWND window)
{
    GRAPH* const graph = Graph;
    MainSwDialogItemRef device = {window, 1000u};
    MainSwDialogItemRef mode = {window, 1001u};
    MainSwDialogItemRef fullscreen = {window, 1005u};
    graph->UpdateStartupDialog(
        reinterpret_cast<DIALOG_COMBO_BOX*>(&device),
        reinterpret_cast<DIALOG_COMBO_BOX*>(&mode),
        reinterpret_cast<DIALOG_BUTTON*>(&fullscreen));
}



static void MainSwHideStartControls(HWND window)
{
    if ((graphStartup.flags&4u) != 0u) return;
    static const unsigned int controlIds[] = {1005u, 1006u, 1007u, 1008u, 1009u, 1010u, 1012u};
    for (unsigned int controlIndex = 0u;controlIndex<sizeof(controlIds)/sizeof(controlIds[0]);++controlIndex)
    {
        HWND control = GetDlgItem(window, static_cast<int>(controlIds[controlIndex]));
        ShowWindow(control, SW_HIDE);
    }
}

static void MainSwRefreshPasswordGate(HWND window)
{
    {
        STRING entered = MainSwReadDialogText(window, 1013u);
        const bool matches = strcmp(MainSwDialogPassword().value, entered.value) == 0;
        EnableWindow(GetDlgItem(window, 1017), matches?TRUE:FALSE);
    }

    const bool passwordEmpty = strcmp(MainSwDialogPassword().value,
        StringGlobals::EmptyText()) == 0;
    ShowWindow(GetDlgItem(window, 1015), passwordEmpty?SW_HIDE:SW_SHOW);
    ShowWindow(GetDlgItem(window, 1016), passwordEmpty?SW_SHOW:SW_HIDE);
}

static STRING MainSwRegistryString(const char* key, const char* fallback)
{
    STRING keyText = {StringEmptySentinel()};
    STRING fallbackText = {StringEmptySentinel()};
    keyText = key;
    fallbackText = fallback;
    STRING result = ::Registry->GetString(keyText, fallbackText);
    return result;
}

static void MainSwRegistrySetString(const char* key, const STRING& value)
{
    STRING keyText = {StringEmptySentinel()};
    keyText = key;
    ::Registry->SetString(keyText, value);
}

}

INT_PTR __stdcall AppStart(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    (void)lParam;
    (void)MainSwDialogPassword();
    MainSwRefreshPasswordGate(hwnd);

    if (message == WM_INITDIALOG)
    {
        SetWindowTextA(hwnd, graphStartup.title);
        MainSwHideStartControls(hwnd);

        {
            STRING storedPassword = MainSwRegistryString(
                "Password",
                StringGlobals::EmptyText());
            (MainSwDialogPassword() = storedPassword, &MainSwDialogPassword());
        }

        {
            const STRING key("SoundHighQuality");
            const int highQuality = ::Registry->GetInt(key, 1);
            SendDlgItemMessageA(hwnd, 1012u, BM_SETCHECK, highQuality?BST_CHECKED:BST_UNCHECKED, 0);
        }
        SendDlgItemMessageA(hwnd, 1010u, BM_SETCHECK, BST_UNCHECKED, 0);

        MainSwSyncStartupDialog(hwnd);
        {
            const STRING label("Green Blood");
            SendDlgItemMessageA(hwnd, 1017u, CB_ADDSTRING, 0,
                static_cast<LPARAM>(PtrToU32(label.value)));
        }
        {
            const STRING label("Red Blood");
            SendDlgItemMessageA(hwnd, 1017u, CB_ADDSTRING, 0,
                static_cast<LPARAM>(PtrToU32(label.value)));
        }
        {
            const STRING key("Blood");
            const int blood = ::Registry->GetInt(key, 1);
            SendDlgItemMessageA(hwnd, 1017u, CB_SETCURSEL, static_cast<WPARAM>(blood), 0);
        }
        if (SendDlgItemMessageA(hwnd, 1017u, CB_GETCURSEL, 0, 0) != 0)
        {
            (MainSwDialogPassword() = StringGlobals::EmptyText(), &MainSwDialogPassword());
            SetDlgItemTextA(hwnd, 1013u, MainSwDialogPassword().value);
        }
        return TRUE;
    }

    if (message != WM_COMMAND) return FALSE;

    const unsigned int controlId = wParam&0xFFFFu;
    const unsigned int notification = (wParam>>16u)&0xFFFFu;
    if (controlId == 1000u || controlId == 1001u)
    {
        if (notification == CBN_SELENDOK) MainSwSyncStartupDialog(hwnd);
        return FALSE;
    }

    if (controlId == IDOK)
    {
        MainSwSyncStartupDialog(hwnd);
        graphStartup.flags =
            SendDlgItemMessageA(hwnd, 1010u, BM_GETCHECK, 0, 0) == BST_CHECKED ? 8u : 0u;
        {
            const STRING key("SoundHighQuality");
            const int highQuality = SendDlgItemMessageA(hwnd, 1012u, BM_GETCHECK, 0, 0) == BST_CHECKED?1:0;
            ::Registry->SetInt(key, highQuality);
        }
        {
            const STRING key("Blood");
            const int blood = static_cast<int>(SendDlgItemMessageA(hwnd, 1017u, CB_GETCURSEL, 0, 0));
            ::Registry->SetInt(key, blood);
        }

        bool passwordChanged;
        {
            STRING entered = MainSwReadDialogText(hwnd, 1013u);
            STRING stored = MainSwRegistryString(
                "Password",
                StringGlobals::EmptyText());
            passwordChanged = strcmp(stored.value, entered.value) != 0;
        }
        if (passwordChanged)
        {
            STRING enteredForWrite = MainSwReadDialogText(hwnd, 1013u);
            MainSwRegistrySetString("Password", enteredForWrite);
        }
        EndDialog(hwnd, 1);
        return FALSE;
    }

    if (controlId == IDCANCEL)
    {
        EndDialog(hwnd, 0);
        return FALSE;
    }

    if (controlId == 1013u)
    {
        bool passwordChanged = false;
        {
            STRING entered = MainSwReadDialogText(hwnd, 1013u);
            if (strcmp(entered.value, StringGlobals::EmptyText()) != 0)
            {
                STRING enteredForCompare = MainSwReadDialogText(hwnd, 1013u);
                passwordChanged = strcmp(MainSwDialogPassword().value, enteredForCompare.value) != 0;
            }
        }
        if (passwordChanged)
            SendDlgItemMessageA(hwnd, 1017u, CB_SETCURSEL, 0, 0);
        return FALSE;
    }

    if (controlId == 1017u && notification == CBN_SELENDOK &&
       SendDlgItemMessageA(hwnd, 1017u, CB_GETCURSEL, 0, 0) != 0)
    {
        (MainSwDialogPassword() = StringGlobals::EmptyText(), &MainSwDialogPassword());
        SetDlgItemTextA(hwnd, 1013u, MainSwDialogPassword().value);
    }
    return FALSE;
}



extern "C" void __cdecl MainSwCreateCurScreenShot(void)
{
    STRING timeText = GetTimeString();
    STRING dateText = GetDateString();

    STRING screensAndDate = "\\Screens\\"+dateText;
    STRING withSpace = screensAndDate+" ";
    STRING withTime = withSpace+timeText;
    STRING fileName = withTime+".bmp";

    (void)fileName.Replace(
        ":",
        "h");
    (void)fileName.Replace(
        ":",
        "m");
    (void)fileName.Replace(
        ":",
        "s");

    GRAPH* const graph = Graph;
    MAP* const app = Map;
    STRING appPath = app->WindowsUserPath();
    STRING fullPath = appPath+fileName;

    const int width = TruncateFloatToInt32(graph->screenWidth);
    const int height = TruncateFloatToInt32(graph->screenHeight);
    graph->SaveScreenShot(fullPath, 0, 0, width, height);

}


LRESULT __stdcall AppWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MAP* app = Map;
    const unsigned int lParam32 = static_cast<unsigned int>(lParam);

    if (app->WorkWndMessage(hwnd, message, wParam, static_cast<unsigned long>(lParam32)) != 0)
        return 1;

    if (message != WM_COMMAND)
    {
        return ::DefWindowProcA(hwnd, message, wParam, lParam);
    }

    const unsigned int command = wParam&0xFFFFu;
    if (command == 0x9C42u)
    {
        CONSTANT* const constants = ::Const;
        if (constants != 0 && constants->DebugMode != 0)
        {
            Map->Save(STRING("current.sav"));
        }
        return 0;
    }

    if (command == 0x9C41u)
    {
        CONSTANT* const constants = ::Const;
        if (constants != 0 && constants->DebugMode != 0)
        {
            NET* const session = ::Net;
            if (session != 0)session->NetworkOff();
            Map->Load(Map->OpenDialog(
                "Map Files\0*.map\0Save Files\0*.sav\0Demo Files\0*.dem\0All Files\0*.*\0",
                "map"));
        }
        return 0;
    }

    return 0;
}

static void RunCrtAtExitCallbacks();

enum class FatalExceptionDetail
{
    Address,
    AccessViolation,
    Allocation,
    RandomSynchronization
};

static void WriteFatalException(const STRING& fileName, const se_translator::exception& exception,
    FatalExceptionDetail detail)
{
    RecordDBOfstreamControl output(fileName.value, 2);
    const EXCEPTION_RECORD& record = *exception.ExceptionPointers()->ExceptionRecord;
    const char* access = "read";
    unsigned long extra = 0;
    if (detail == FatalExceptionDetail::AccessViolation)
    {
        if (record.ExceptionInformation[0] != 0) access = "write";
        extra = static_cast<unsigned long>(record.ExceptionInformation[1]);
    }
    else if (detail == FatalExceptionDetail::Allocation)
    {
        extra = static_cast<unsigned long>(record.ExceptionInformation[0]);
    }
    const void* const address = record.ExceptionAddress;
    output << se_translator::name(record.ExceptionCode) << " at 0x";
    output.setf(RecordDBOfstreamControl::hexadecimal, RecordDBOfstreamControl::baseField);
    output << address;
    if (detail == FatalExceptionDetail::AccessViolation)
    {
        output << ", thread attempts to " << access << " at 0x";
        output.setf(RecordDBOfstreamControl::right, RecordDBOfstreamControl::adjustmentField);
        output << extra;
    }
    else if (detail == FatalExceptionDetail::Allocation)
    {
        output << ", unable to allocate ";
        output.setf(RecordDBOfstreamControl::decimal, RecordDBOfstreamControl::baseField);
        output << static_cast<int>(extra) << " bytes";
    }
    output.put('\n').flush();
    if (detail == FatalExceptionDetail::AccessViolation ||
        detail == FatalExceptionDetail::Allocation ||
        detail == FatalExceptionDetail::RandomSynchronization)
    {
        output << "stack trace : ";
        output.put('\n').flush();
    }
    output.Close();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    if (!AS2RuntimeInitializeStandalone()) return 0;

    SFINIT0_MonsterVidAllocate();
    SFINIT1_Proc();
    SFINIT2_Proc();

    STRING& logName = synchroErrorLogFileNameStorage.Get();
    {
        STRING timeText = GetTimeString();
        STRING dateText = GetDateString();
        STRING prefix = "error"+dateText;
        STRING withSpace = prefix+" ";
        STRING withTime = withSpace+timeText;
        logName = withTime;
    }
    (void)logName.Replace(":", "h");
    (void)logName.Replace(":", "m");

    STRING fatalLogPath;
    {
        STRING path = logName+"_fatal.log";
        fatalLogPath = path;
    }
    se_translator translator;
    try
    {

        char* const resourceName = startupResourceName;
        char* const resourceLengthText = startupResourceLength;
        ::LoadStringA(hInstance, 1u, resourceName, 256);
        ::LoadStringA(hInstance, 2u, resourceLengthText, 256);

        const int expectedLength = BitsToSigned32(CrtAtoiText(resourceLengthText));
        bool resourceMatches = true;
        if (expectedLength>0)
        {
            std::FILE* const resource = std::fopen(resourceName, "rb");
            if (resource)
            {
                const unsigned int actualLength = static_cast<unsigned int>(_filelength(_fileno(resource)));
                resourceMatches = (actualLength == static_cast<unsigned int>(expectedLength));
                if (resourceMatches)(void)std::fclose(resource);
            }
            else resourceMatches = false;
        }
        if (!resourceMatches)
        {
            ::MessageBoxA(0, "Invalid game version", "Error", 0u);
            return 0;
        }

        void* const appStorage = CrtNew(sizeof(MAP_STEAM));
        if (appStorage)
        {
            bool appConstructed = false;
            try
            {
                STRING command(lpCmdLine);
                MAP_STEAM* const app = new(appStorage) MAP_STEAM(
                    hInstance, hPrevInstance, command, nShowCmd,
                    &graphStartup);
                appConstructed = true;
                Map = static_cast<MAP*>(app);
            }
            catch (...)
            {
                if (!appConstructed) CrtFreePointer(appStorage);
                throw;
            }
        }
        else Map = 0;

        {
            STRING savePath = Map->WindowsUserPath();
            STRING logsFolder = savePath+"Logs\\";
            STRING fullLogName = logsFolder+logName;
            logName = fullLogName;
        }
        {
            STRING path = logName+"_fatal.log";
            fatalLogPath = path;
        }

        MAP* live = Map;
        if (live && (live->Flags()&8u) != 0u)
        {
            int done;
            do
            {
                done = live->Tact();
                live = Map;
            }
            while (done == 0);
        }
        live = Map;
        delete live;
    }
    catch (const se_translator::access_violation& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::AccessViolation);
    }
    catch (const se_translator::no_memory& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Allocation);
    }
    catch (const se_translator::datatype_misalignment& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::breakpoint& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::single_step& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::array_bounds_exceeded& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::flt_denormal_operand& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::flt_divide_by_zero& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::flt_inexact_result& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::flt_invalid_operation& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::flt_overflow& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::flt_stack_check& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::flt_underflow& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::int_divide_by_zero& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::int_overflow& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::priv_instruction& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::in_page_error& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::illegal_instruction& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::noncontinuable_exception& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::stack_overflow& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::invalid_disposition& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::guard_page& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::invalid_handle& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::microsoft_cpp& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::Address);
    }
    catch (const se_translator::zm_random_synchro& exception)
    {
        WriteFatalException(fatalLogPath, exception, FatalExceptionDetail::RandomSynchronization);
    }

    ::VirtualFree(g_abMonsterVid, 0u, MEM_RELEASE);
    ::VirtualFree(g_aiNamedMonstersLast, 0u, MEM_RELEASE);
    ::VirtualFree(IniLineBuffer, 0u, MEM_RELEASE);
    return 0;
}




#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <time.h>
#include <stdexcept>
#if defined(_MSC_VER)
#include <eh.h>
#endif

void __cdecl InitializeMapLoadingDirectionAtStartup();
unsigned int __cdecl InitializeMapLoadingTextAtStartup();
void __cdecl RefreshQuarterTickAtStartup();
extern "C" unsigned int __cdecl MylibStaticCleanupGuard();
unsigned int __cdecl NetHostListStaticInit();
void* __cdecl InitializeVidRootAtStartup();
void __cdecl InitializeVidFirstRangeAtStartup();
void __cdecl InitializeVidSecondRangeAtStartup();
void __cdecl DialogSrvRegisterStaticCleanupSet();
void __cdecl InitializeGraphBlackAtStartup();
void __cdecl InitializeGraphCyanAtStartup();
void __cdecl InitializeGraphMagentaAtStartup();
void __cdecl InitializeGraphBrownAtStartup();
void __cdecl InitializeGraphBlueAtStartup();
void __cdecl InitializeGraphGrayAtStartup();
void __cdecl InitializeGraphGreenAtStartup();
void __cdecl InitializeGraphLightBlueAtStartup();
void __cdecl InitializeGraphLightGreenAtStartup();
void __cdecl InitializeGraphLightRedAtStartup();
void __cdecl InitializeGraphRedAtStartup();
void __cdecl InitializeGraphWhiteAtStartup();
void __cdecl InitializeGraphYellowAtStartup();
static unsigned int __cdecl MainSwGlobalStringStaticInit();
unsigned int __cdecl ManStaticInitGlobalColorPalette();
unsigned int __cdecl StaticAtexitGuard();
unsigned int __cdecl BaseTexture_StaticCleanupGuard01();
unsigned int __cdecl BaseTexture_StaticCleanupGuard02();
unsigned int __cdecl BaseTexture_StaticCleanupGuard03();
unsigned int __cdecl BaseTexture_StaticCleanupGuard04();
unsigned int __cdecl BaseTexture_StaticCleanupGuard05();
void __cdecl RecordDBTouchFourStaticInitializers();
void __cdecl zIniParser_RegisterSharedCleanupOnce();
void __cdecl zItemInfo_RegisterSharedCleanupOnce();

namespace {

typedef int (__cdecl *CrtNewHandler)(unsigned int);
typedef void (__cdecl *CrtExitFunction)(int);
typedef void (__cdecl *CrtBannerFunction)();
typedef HWND (WINAPI *CrtGetActiveWindowFunction)();
typedef HWND (WINAPI *CrtGetLastActivePopupFunction)(HWND);
typedef int (WINAPI *CrtMessageBoxFunction)(HWND, const char*, const char*, unsigned int);

CrtNewHandler crtNewHandler = 0;
int crtNewMode = 0;
CrtExitFunction crtExit = &_exit;
CrtBannerFunction crtRuntimeBanner = 0;
CrtMessageBoxFunction crtMessageBox = 0;
CrtGetActiveWindowFunction crtGetActiveWindow = 0;
CrtGetLastActivePopupFunction crtGetLastActivePopup = 0;
int crtErrorMode = 0;
int crtApplicationType = 2;
unsigned int crtCharacterMappingApi = 0;
unsigned int crtCharacterTypeApi = 0;
char crtModulePath[MAX_PATH] = {};

// VC6 stream records are distinct from the host CRT's opaque FILE objects.
struct CrtFileRecord
{
    char* pointer;
    int count;
    char* base;
    int flags;
    int file;
    int characterBuffer;
    int bufferSize;
    char* temporaryName;
};

char crtInputBuffer[4096] = {};
CrtFileRecord crtInitialStreams[20] = {
    {crtInputBuffer, 0, crtInputBuffer, 257, 0, 0, 4096, 0},
    {0, 0, 0, 2, 1, 0, 0, 0},
    {0, 0, 0, 2, 2, 0, 0, 0}
};
int crtStreamCount = 0;
CrtFileRecord** crtStreamTable = 0;

}

#if defined(_MSC_VER) && defined(_M_IX86)


unsigned int __cdecl CrtControlFp(unsigned int newControl, unsigned int mask)
{

    unsigned int x87Control = 0u;
    (void)__control87_2(newControl, mask&0xFFF7FFFFu, &x87Control, 0);
    return x87Control;
}
#else
unsigned int __cdecl CrtControlFp(unsigned int, unsigned int)
{
    return 0u;
}
#endif

static int __cdecl CrtCallNewHandler(unsigned int size)
{
    const CrtNewHandler handler = crtNewHandler;
    if (handler == 0)return 0;
    return handler(size) != 0 ? 1 : 0;
}

static void* __cdecl CrtAllocateWithNewHandler(unsigned int size, unsigned int mode)
{
    if (size>0xFFFFFFE0u)return 0;
    for (;;)
    {
        void* const memory = std::malloc(size);
        if (memory != 0 || mode == 0u)return memory;
        if (CrtCallNewHandler(size) == 0)return 0;
    }
}

void* __cdecl CrtNew(unsigned int size)
{
    return CrtAllocateWithNewHandler(size, 1u);
}

void* __cdecl CrtAllocate(unsigned int size)
{
    return CrtAllocateWithNewHandler(size, crtNewMode);
}


unsigned int __cdecl CrtFileGetC(unsigned int streamAddress)
{
    std::FILE* const stream = reinterpret_cast<std::FILE*>(U32ToPtr(streamAddress));
    return static_cast<unsigned int>(std::fgetc(stream));
}
unsigned int __cdecl CrtFileClose(unsigned int streamAddress)
{
    std::FILE* const stream = reinterpret_cast<std::FILE*>(U32ToPtr(streamAddress));
    return static_cast<unsigned int>(std::fclose(stream));
}
unsigned int __cdecl CrtFileOpen(unsigned int pathAddress, unsigned int modeAddress)
{
    return PtrToU32(std::fopen(reinterpret_cast<const char*>(U32ToPtr(pathAddress)),
        reinterpret_cast<const char*>(U32ToPtr(modeAddress))));
}


unsigned int __cdecl CrtFileSeek(unsigned int streamAddress, unsigned int byteOffset, unsigned int origin)
{
    std::FILE* const stream = reinterpret_cast<std::FILE*>(U32ToPtr(streamAddress));
    return static_cast<unsigned int>(std::fseek(stream, BitsToSigned32(byteOffset), BitsToSigned32(origin)));
}
unsigned int __cdecl CrtFileFlush(unsigned int streamAddress)
{
    std::FILE* const stream = reinterpret_cast<std::FILE*>(U32ToPtr(streamAddress));
    return static_cast<unsigned int>(std::fflush(stream));
}
unsigned int __cdecl CrtFileWrite(unsigned int inputAddress, unsigned int elementSize, unsigned int elementCount, unsigned int streamAddress)
{
    std::FILE* const stream = reinterpret_cast<std::FILE*>(U32ToPtr(streamAddress));
    const unsigned int requestedBytes = elementSize*elementCount;
    const unsigned int transferredBytes = static_cast<unsigned int>(std::fwrite(U32ToPtr(inputAddress), 1u, requestedBytes, stream));
    if (requestedBytes == 0u) return 0u;
    return transferredBytes == requestedBytes?elementCount:transferredBytes/elementSize;
}
unsigned int __cdecl CrtFilePutC(unsigned int character, unsigned int streamAddress)
{
    std::FILE* const stream = reinterpret_cast<std::FILE*>(U32ToPtr(streamAddress));
    return static_cast<unsigned int>(std::fputc(BitsToSigned32(character), stream));
}

unsigned int __cdecl CrtFileUngetC(unsigned int character, unsigned int streamAddress)
{
    std::FILE* const stream = reinterpret_cast<std::FILE*>(U32ToPtr(streamAddress));
    return static_cast<unsigned int>(std::ungetc(BitsToSigned32(character), stream));
}

static void SetCrtFileError(DWORD systemError)
{
    struct ErrorMapping
    {
        DWORD systemError;
        int error;
    };
    static const ErrorMapping mappings[] =
    {
        {ERROR_INVALID_FUNCTION, EINVAL},
        {ERROR_FILE_NOT_FOUND, ENOENT},
        {ERROR_PATH_NOT_FOUND, ENOENT},
        {ERROR_TOO_MANY_OPEN_FILES, EMFILE},
        {ERROR_ACCESS_DENIED, EACCES},
        {ERROR_INVALID_HANDLE, EBADF},
        {ERROR_ARENA_TRASHED, ENOMEM},
        {ERROR_NOT_ENOUGH_MEMORY, ENOMEM},
        {ERROR_INVALID_BLOCK, ENOMEM},
        {ERROR_BAD_ENVIRONMENT, E2BIG},
        {ERROR_BAD_FORMAT, ENOEXEC},
        {ERROR_INVALID_ACCESS, EINVAL},
        {ERROR_INVALID_DATA, EINVAL},
        {ERROR_INVALID_DRIVE, ENOENT},
        {ERROR_CURRENT_DIRECTORY, EACCES},
        {ERROR_NOT_SAME_DEVICE, EXDEV},
        {ERROR_NO_MORE_FILES, ENOENT},
        {ERROR_LOCK_VIOLATION, EACCES},
        {ERROR_BAD_NETPATH, ENOENT},
        {ERROR_NETWORK_ACCESS_DENIED, EACCES},
        {ERROR_BAD_NET_NAME, ENOENT},
        {ERROR_FILE_EXISTS, EEXIST},
        {ERROR_CANNOT_MAKE, EACCES},
        {ERROR_FAIL_I24, EACCES},
        {ERROR_INVALID_PARAMETER, EINVAL},
        {ERROR_NO_PROC_SLOTS, EAGAIN},
        {ERROR_DRIVE_LOCKED, EACCES},
        {ERROR_BROKEN_PIPE, EPIPE},
        {ERROR_DISK_FULL, ENOSPC},
        {ERROR_INVALID_TARGET_HANDLE, EBADF},
        {ERROR_INVALID_HANDLE, EINVAL},
        {ERROR_WAIT_NO_CHILDREN, ECHILD},
        {ERROR_CHILD_NOT_COMPLETE, ECHILD},
        {ERROR_DIRECT_ACCESS_HANDLE, EBADF},
        {ERROR_NEGATIVE_SEEK, EINVAL},
        {ERROR_SEEK_ON_DEVICE, EACCES},
        {ERROR_DIR_NOT_EMPTY, ENOTEMPTY},
        {ERROR_NOT_LOCKED, EACCES},
        {ERROR_BAD_PATHNAME, ENOENT},
        {ERROR_MAX_THRDS_REACHED, EAGAIN},
        {ERROR_LOCK_FAILED, EACCES},
        {ERROR_ALREADY_EXISTS, EEXIST},
        {ERROR_FILENAME_EXCED_RANGE, ENOENT},
        {ERROR_NESTING_NOT_ALLOWED, EAGAIN},
        {ERROR_NOT_ENOUGH_QUOTA, ENOMEM}
    };

    (void)_set_doserrno(systemError);
    for (const ErrorMapping& mapping : mappings)
    {
        if (mapping.systemError == systemError)
        {
            errno = mapping.error;
            return;
        }
    }
    if (systemError >= ERROR_WRITE_PROTECT && systemError <= ERROR_SHARING_BUFFER_EXCEEDED)
        errno = EACCES;
    else if (systemError >= ERROR_INVALID_STARTING_CODESEG && systemError <= ERROR_INFLOOP_IN_RELOC_CHAIN)
        errno = ENOEXEC;
    else
        errno = EINVAL;
}

int __cdecl CrtRemove(const char* path)
{
    const DWORD error = DeleteFileA(path) ? ERROR_SUCCESS : GetLastError();
    if (error == ERROR_SUCCESS) return 0;
    SetCrtFileError(error);
    return -1;
}

unsigned int __cdecl Msvc6LengthError()
{
    RecordDBLineString text("string too long");
    RecordDBLengthErrorControl object;
    object.Construct(text);
    object.vftable = Msvc6::LengthErrorTable();
    Msvc6::ThrowException(&object, Msvc6::LengthErrorThrowInfo());
    return 0u;
}

unsigned int __cdecl Msvc6RangeError()
{
    RecordDBLineString text("invalid string position");
    RecordDBOutOfRangeControl object;
    object.Construct(text);
    object.vftable = Msvc6::OutOfRangeTable();
    Msvc6::ThrowException(&object, Msvc6::OutOfRangeThrowInfo());
    return 0u;
}

class CrtStaticLockGuard
{
    unsigned int storage;

public:
    CrtStaticLockGuard()
    {
        (void)CrtStaticLockConstruct(PtrToU32(&storage));
    }

    ~CrtStaticLockGuard() noexcept(false)
    {
        (void)CrtStaticLockDestruct(PtrToU32(&storage));
    }

    CrtStaticLockGuard(const CrtStaticLockGuard&) = delete;
    CrtStaticLockGuard& operator=(const CrtStaticLockGuard&) = delete;
};

RecordDBLocaleImplControl* RecordDBLocaleImplControl::_Global = 0;
RecordDBLocaleImplControl* RecordDBLocaleImplControl::_Clocptr = 0;
const short* RecordDBCtypeControl::_Cltab = 0;

namespace Msvc6
{
int LocaleId::_Id_cnt = 0;
RecordDBLocaleControl ClassicLocale = {};
RecordDBIosControl* RegisteredStreams[9] = {};
unsigned char StreamRegistrations[9] = {};

template<class Tag> LocaleId FacetState<Tag>::id = {};
template<class Tag> const FacetData* FacetState<Tag>::_Psave = 0;
template<class Tag> typename FacetDataType<Tag>::Type* FacetState<Tag>::_Facsav = 0;
template struct FacetState<CharCtype>;
template struct FacetState<CharNumGet>;
template struct FacetState<CharNumPut>;
template struct FacetState<CharNumpunct>;
template struct FacetState<CharCodecvt>;
template struct FacetState<CharCollate>;
template struct FacetState<CharMessages>;
template struct FacetState<CharMoneyGet>;
template struct FacetState<CharMoneyPut>;
template struct FacetState<CharMoneypunct>;
template struct FacetState<CharMoneypunctIntl>;
template struct FacetState<CharTimeGet>;
template struct FacetState<CharTimePut>;
template struct FacetState<WideCtype>;
template struct FacetState<WideNumGet>;
template struct FacetState<WideNumPut>;
template struct FacetState<WideNumpunct>;
template struct FacetState<WideCollate>;
template struct FacetState<WideMessages>;
template struct FacetState<WideMoneyGet>;
template struct FacetState<WideMoneyPut>;
template struct FacetState<WideMoneypunct>;
template struct FacetState<WideMoneypunctIntl>;
template struct FacetState<WideTimeGet>;
template struct FacetState<WideTimePut>;
template struct FacetState<WideCodecvt>;

RecordDBBadAllocControl& BadAllocationObject()
{
    static StaticObjectStorage<RecordDBBadAllocControl> storage;
    return storage.Get();
}
}

static unsigned char LocaleBadAllocationInitialized = 0;

RecordDBLocaleImplControl* RecordDBLocaleImplControl::Construct(unsigned int transparentValue)
{
    references = 1u;
    vftable = Msvc6::BaseFacetTable();
    transparent = static_cast<unsigned char>(transparentValue);
    facets = 0;
    facetCount = 0u;
    category = 0u;
    name.allocatorState[0] = static_cast<unsigned char>(transparentValue>>24u);
    try
    {
        Tidy(&name, 0u);
        (void)RecordDBStringAssignBytes(&name, "*", 1u);
    }
    catch (...)
    {
        vftable = Msvc6::BaseFacetTable();
        throw;
    }
    vftable = Msvc6::LocaleImplTable();
    return this;
}

void* RecordDBRefCountReleaseAndReturnIfZero(void* self);

static void __cdecl Msvc6GlobalLocaleCleanup()
{
    CrtStaticLockGuard lock;
    RecordDBLocaleImplControl* const implementation = RecordDBLocaleImplControl::_Global;
    if (implementation)
    {
        void* const unreferenced = RecordDBRefCountReleaseAndReturnIfZero(implementation);
        if (unreferenced) Msvc6::DeleteFacet(unreferenced);
    }
}

static unsigned int Msvc6LocaleImpl()
{
    CrtStaticLockGuard lock;
    if (RecordDBLocaleImplControl::_Global == 0)
    {
        RecordDBLocaleImplControl* const allocation = static_cast<RecordDBLocaleImplControl*>(
            CrtNew(40u));
        RecordDBLocaleImplControl* implementation = 0;
        try
        {
            if (allocation) implementation = allocation->Construct(0u);
        }
        catch (...)
        {
            CrtFreePointer(allocation);
            throw;
        }
        RecordDBLocaleImplControl::_Global = implementation;
        (void)RegisterCrtAtExit(&Msvc6GlobalLocaleCleanup);
        RecordDBLocaleImplControl::_Global->category = 63u;
        AS_RECORD_STRING_CONTROL* const name = &RecordDBLocaleImplControl::_Global->name;
        (void)RecordDBStringAssignBytes(name, "C", 1u);
        RecordDBLocaleImplControl* const classic = RecordDBLocaleImplControl::_Global;
        RecordDBLocaleImplControl::_Clocptr = classic;
        reinterpret_cast<MsvcLocaleFacetRef*>(classic)->Retain();
        Msvc6::ClassicLocale.implementation = RecordDBLocaleImplControl::_Clocptr;
    }
    return PtrToU32(RecordDBLocaleImplControl::_Global);
}

unsigned int Msvc6BasicIosClear(void* self, unsigned int state, unsigned int rethrow)
{
    RecordDBIosControl& ios = *static_cast<RecordDBIosControl*>(self);
    ios.state = state&7u;
    const unsigned int raised = ios.exceptionMask&ios.state;
    if (raised != 0u)
    {
        if (static_cast<unsigned char>(rethrow) != 0u)
            throw;
        const char* const message = (raised&4u) != 0u ? "ios::badbit set" :
            ((raised&2u) != 0u ? "ios::failbit set" : "ios::eofbit set");
        RecordDBLineString text(message,
            static_cast<unsigned char>(rethrow>>24));
        RecordDBIosFailureControl object;
        object.Construct(text);
        object.vftable = Msvc6::IosFailureTable();
        Msvc6::ThrowException(&object, Msvc6::IosFailureThrowInfo());
    }
    return 0u;
}

unsigned int  Msvc6StreamRegistration(void* self)
{
    unsigned int lockStorage;
    (void)CrtStaticLockConstruct(PtrToU32(&lockStorage));
    RecordDBIosControl& storage = *static_cast<RecordDBIosControl*>(self);
    while (storage.registration<8u)
    {
        const RecordDBIosControl* const current = Msvc6::RegisteredStreams[storage.registration];
        if (current == 0 || current == &storage)break;
        ++storage.registration;
    }
    Msvc6::RegisteredStreams[storage.registration] = &storage;
    const unsigned int slot = storage.registration;
    Msvc6::StreamRegistrations[slot] = static_cast<unsigned char>(Msvc6::StreamRegistrations[slot]+1u);
    return CrtStaticLockDestruct(PtrToU32(&lockStorage));
}

unsigned int  Msvc6BasicIosInit(void* self)
{
    void* const locale = U32ToPtr(PtrToU32(self)+32u);
    if (locale) (void)Msvc6LocaleDefaultCtor(locale);
    static_cast<RecordDBIosControl*>(self)->exceptionMask = 0u;
    static_cast<RecordDBIosControl*>(self)->formatFlags = 0x201u;
    static_cast<RecordDBIosControl*>(self)->precision = 6u;
    static_cast<RecordDBIosControl*>(self)->width = 0u;
    static_cast<RecordDBIosControl*>(self)->reserved18 = 0u;
    static_cast<RecordDBIosControl*>(self)->reserved1C = 0u;
    return Msvc6BasicIosClear(self, 0u, 0u);
}

unsigned int __cdecl Msvc6ExceptionInFlight()
{
    return 0u;
}

unsigned int __cdecl Msvc6GlobalLocale()
{
    return Msvc6LocaleImpl();
}

unsigned int  Msvc6LocaleGetFacet(void* locale, unsigned int id, unsigned int create)
{
    unsigned int lockStorage;
    (void)CrtStaticLockConstruct(PtrToU32(&lockStorage));
    const RecordDBLocaleImplControl* const impl = static_cast<const RecordDBLocaleImplControl*>(
        static_cast<const RecordDBLocaleControl*>(locale)->implementation);
    unsigned int facet = 0u;
    if (id<impl->facetCount)
        facet = PtrToU32(impl->facets[id]);
    if (facet == 0u && static_cast<unsigned char>(create) != 0u && impl->transparent != 0u)
    {
        const RecordDBLocaleImplControl* const global = RecordDBLocaleImplControl::_Global;
        if (id<global->facetCount)
            facet = PtrToU32(global->facets[id]);
    }
    (void)CrtStaticLockDestruct(PtrToU32(&lockStorage));
    return facet;
}

unsigned int  Msvc6LocaleCanSupplyFacet(void* locale)
{
    unsigned int lockStorage;
    (void)CrtStaticLockConstruct(PtrToU32(&lockStorage));
    const RecordDBLocaleImplControl* const impl = static_cast<const RecordDBLocaleImplControl*>(
        static_cast<const RecordDBLocaleControl*>(locale)->implementation);
    const RecordDBLocaleImplControl* const classic = RecordDBLocaleImplControl::_Clocptr;
    const bool available = impl == classic ||
        (impl->transparent != 0u && RecordDBLocaleImplControl::_Global == classic);
    (void)CrtStaticLockDestruct(PtrToU32(&lockStorage));
    return available?1u:0u;
}

void __cdecl Msvc6NoMemory()
{
    RecordDBBadAllocControl& stored = Msvc6::BadAllocationObject();
    const unsigned char guard = LocaleBadAllocationInitialized;
    if ((guard&1u) == 0u)
    {
        LocaleBadAllocationInitialized = static_cast<unsigned char>(guard|1u);
        stored.Construct("bad allocation");
        (void)RegisterCrtAtExit(&RecordDBBadAllocCleanup);
    }
    RecordDBBadAllocControl object;
    object.CopyConstruct(stored);
    Msvc6::ThrowException(&object, Msvc6::BadAllocThrowInfo());
}
void* RecordDBStringAssignSubstring(void* self, void* sourceObject, unsigned int pos, unsigned int count);
RecordDBLocaleImplControl* Msvc6MakeLocale(void* localeInfo, unsigned int categoryMask,
    RecordDBLocaleImplControl* target, const RecordDBLocaleControl* sourceLocale);

class Msvc6LocaleInfoScope
{
    AS_RECORD_STRING_CONTROL strings[4];

public:
    explicit Msvc6LocaleInfoScope(const char* name)
    {
        (void)Msvc6LocaleConstruct(strings, PtrToU32(name));
    }

    ~Msvc6LocaleInfoScope() noexcept(false)
    {
        (void)Msvc6LocaleDestruct(strings);
    }

    void* Data() { return strings; }
    Msvc6LocaleInfoScope(const Msvc6LocaleInfoScope&) = delete;
    Msvc6LocaleInfoScope& operator=(const Msvc6LocaleInfoScope&) = delete;
};

RecordDBLocaleImplControl* RecordDBLocaleImplControl::CopyConstruct(const RecordDBLocaleImplControl& source)
{
    references = 1u;
    vftable = Msvc6::BaseFacetTable();
    facets = 0;
    facetCount = source.facetCount;
    category = source.category;
    transparent = source.transparent;
    name.allocatorState[0] = source.name.allocatorState[0];
    try
    {
        Tidy(&name, 0u);
        (void)RecordDBStringAssignSubstring(&name,
            const_cast<AS_RECORD_STRING_CONTROL*>(&source.name), 0u, AS_RECORD_STRING_CONTROL::npos);
    }
    catch (...)
    {
        vftable = Msvc6::BaseFacetTable();
        throw;
    }
    vftable = Msvc6::LocaleImplTable();
    try
    {
        CrtStaticLockGuard outerLock;
        if (&source == RecordDBLocaleImplControl::_Clocptr)
        {
            Msvc6LocaleInfoScope localeInfo("C");
            (void)Msvc6MakeLocale(localeInfo.Data(), 63u, this, 0);
        }
        else
        {
            CrtStaticLockGuard innerLock;
            if (facetCount>0u)
            {
                facets = static_cast<void**>(CrtAllocate(facetCount*sizeof(void*)));
                if (facets == 0) Msvc6NoMemory();
                unsigned int remaining = facetCount;
                while (remaining>0u)
                {
                    void* const facet = source.facets[--remaining];
                    facets[remaining] = facet;
                    if (facet) static_cast<MsvcLocaleFacetRef*>(facet)->Retain();
                }
            }
        }
    }
    catch (...)
    {
        Tidy(&name, 1u);
        vftable = Msvc6::BaseFacetTable();
        throw;
    }
    return this;
}

void RecordDBLocaleImplControl::AddFacet(void* facet, unsigned int id)
{
    CrtStaticLockGuard lock;
    if (facetCount <= id)
    {
        unsigned int count = id+1u;
        if (count<32u) count = 32u;
        void** const grown = reinterpret_cast<void**>(U32ToPtr(
            CrtRealloc(PtrToU32(facets), count*4u)));
        if (grown == 0) Msvc6NoMemory();
        facets = grown;
        while (facetCount<count)
        {
            facets[facetCount] = 0;
            ++facetCount;
        }
    }
    static_cast<MsvcLocaleFacetRef*>(facet)->Retain();
    void* const previous = facets[id];
    if (previous)
    {
        void* const unreferenced = RecordDBRefCountReleaseAndReturnIfZero(previous);
        if (unreferenced) Msvc6::DeleteFacet(unreferenced);
    }
    facets[id] = facet;
}

unsigned int Msvc6LocaleAddFacet(void* locale, unsigned int facet, unsigned int id, unsigned int replaceName)
{
    RecordDBLocaleControl& holder = *static_cast<RecordDBLocaleControl*>(locale);
    if (static_cast<RecordDBLocaleImplControl*>(holder.implementation)->references>1u)
    {
        (void)RecordDBRefCountReleaseAndReturnIfZero(holder.implementation);
        RecordDBLocaleImplControl* const allocation = static_cast<RecordDBLocaleImplControl*>(
            CrtNew(40u));
        RecordDBLocaleImplControl* copy = 0;
        try
        {
            if (allocation)
                copy = allocation->CopyConstruct(*static_cast<const RecordDBLocaleImplControl*>(holder.implementation));
        }
        catch (...)
        {
            CrtFreePointer(allocation);
            throw;
        }
        holder.implementation = copy;
    }
    static_cast<RecordDBLocaleImplControl*>(holder.implementation)->AddFacet(U32ToPtr(facet), id);
    if (replaceName != 0u)
        (void)RecordDBStringAssignBytes(
            &static_cast<RecordDBLocaleImplControl*>(holder.implementation)->name, "*", 1u);
    return PtrToU32(locale);
}

unsigned int  Msvc6LocaleConstruct(void* locale, unsigned int sourceName)
{
    AS_RECORD_STRING_CONTROL* const strings = static_cast<AS_RECORD_STRING_CONTROL*>(locale);
    for (unsigned int index = 0u;index<4u;++index)
    {
        strings[index].allocatorState[0] = static_cast<unsigned char>(sourceName>>24u);
        Tidy(&strings[index], 0u);
    }
    try
    {
        const char* const previous = std::setlocale(LC_ALL, 0);
        (void)RecordDBStringAssignBytes(&strings[2], previous, static_cast<unsigned int>(std::strlen(previous)));
        const char* selected = 0;
        if (sourceName != 0u)
            selected = std::setlocale(LC_ALL, reinterpret_cast<const char*>(U32ToPtr(sourceName)));
        if (selected == 0)
            selected = "*";
        (void)RecordDBStringAssignBytes(&strings[3], selected, static_cast<unsigned int>(std::strlen(selected)));
    }
    catch (...)
    {
        Tidy(&strings[3], 1u);
        Tidy(&strings[2], 1u);
        Tidy(&strings[1], 1u);
        Tidy(&strings[0], 1u);
        throw;
    }
    return PtrToU32(locale);
}

unsigned int  Msvc6LocaleDestruct(void* locale)
{
    AS_RECORD_STRING_CONTROL* const strings = static_cast<AS_RECORD_STRING_CONTROL*>(locale);
    unsigned int remaining = 3u;
    try
    {
        if (strings[2].size>0u)
        {
            const char* const previous = strings[2].data?strings[2].data:"";
            (void)std::setlocale(LC_ALL, previous);
        }
        Tidy(&strings[3], 1u);
        remaining = 2u;
        Tidy(&strings[2], 1u);
        remaining = 1u;
        Tidy(&strings[1], 1u);
        remaining = 0u;
        Tidy(&strings[0], 1u);
    }
    catch (...)
    {
        while (remaining != 0u)
        {
            --remaining;
            Tidy(&strings[remaining], 1u);
        }
        throw;
    }
    return 0u;
}

unsigned int RecordDBGetOrAssignGlobalSequenceId(Msvc6::LocaleId* self);
void __cdecl RecordDBCtypeTerm();
void* RecordDBCtypeCharCtor(void* self, unsigned int table, unsigned int ownsTable, unsigned int references);
void* RecordDBNumPutCtor(void* self, unsigned int references);
void* RecordDBNumpunctCtor(void* self, unsigned int references);
void* RecordDBCodecvtCtor(void* self, unsigned int references);

namespace {

template<class Character> struct Msvc6LocaleCharacters;

template<> struct Msvc6LocaleCharacters<char>
{
    static char Convert(char value) { return value; }
    static char* Duplicate(const char* source)
    {
        const unsigned int count = static_cast<unsigned int>(std::strlen(source))+1u;
        char* const result = static_cast<char*>(CrtNew(count));
        for (unsigned int index = 0u;index<count;++index) result[index] = source[index];
        return result;
    }
};

template<> struct Msvc6LocaleCharacters<unsigned short>
{
    static unsigned short Convert(char value)
    {
        return static_cast<unsigned short>(Msvc6ByteToWide(
            static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(value)))));
    }
    static unsigned short* Duplicate(const char* source)
    {
        const unsigned int count = static_cast<unsigned int>(std::strlen(source))+1u;
        unsigned short* const result = static_cast<unsigned short*>(CrtNew(count*2u));
        for (unsigned int index = 0u;index<count;++index) result[index] = Convert(source[index]);
        return result;
    }
};

static const char* Msvc6LocaleDays(void* localeInfo)
{
    AS_RECORD_STRING_CONTROL& value = static_cast<AS_RECORD_STRING_CONTROL*>(localeInfo)[0];
    char* const text = reinterpret_cast<char*>(U32ToPtr(Msvc6GetDays()));
    if (text)
    {
        (void)RecordDBStringAssignBytes(&value, text, static_cast<unsigned int>(std::strlen(text)));
        CrtFreePointer(text);
    }
    if (value.size != 0u)
        return value.data?value.data:"";
    return ":Sun:Sunday:Mon:Monday:Tue:Tuesday:Wed:Wednesday:Thu:Thursday:Fri:Friday:Sat:Saturday";
}

static const char* Msvc6LocaleMonths(void* localeInfo)
{
    AS_RECORD_STRING_CONTROL& value = static_cast<AS_RECORD_STRING_CONTROL*>(localeInfo)[1];
    char* const text = reinterpret_cast<char*>(U32ToPtr(Msvc6GetMonths()));
    if (text)
    {
        (void)RecordDBStringAssignBytes(&value, text, static_cast<unsigned int>(std::strlen(text)));
        CrtFreePointer(text);
    }
    if (value.size != 0u)
        return value.data?value.data:"";
    return ":Jan:January:Feb:February:Mar:March:Apr:April:May:May:Jun:June:Jul:July:Aug:August:Sep:September:Oct:October:Nov:November:Dec:December";
}

template<class Table> struct Msvc6EmptyLocaleFacet : Msvc6::FacetData
{
    typedef Table Selector;

    void Begin() { references = 0u; vftable = Table::GetTable(); }
    void Initialize(void*) {}
    void AbortConstruction() { vftable = Msvc6::BaseFacetTable(); }
    void Destruct() { vftable = Msvc6::BaseFacetTable(); }
    void* __thiscall ScalarDeletingDestructor(unsigned int flags)
    {
        Destruct();
        if ((flags&1u) != 0u) CrtFreePointer(this);
        return this;
    }
};

template<class Table, bool Conversion> struct Msvc6StampedLocaleFacet : Msvc6::LocaleStampData
{
    typedef Table Selector;

    void Begin() { references = 0u; vftable = Table::GetTable(); }
    void Initialize(void*)
    {
        const unsigned long long stamp = Conversion?Msvc6LocaleStamp():Msvc6CollateStamp();
        localeLow = static_cast<unsigned int>(stamp);
        localeHigh = static_cast<unsigned int>(stamp>>32u);
    }
    void AbortConstruction() { vftable = Msvc6::BaseFacetTable(); }
    void Destruct() { vftable = Msvc6::BaseFacetTable(); }
    void* __thiscall ScalarDeletingDestructor(unsigned int flags)
    {
        Destruct();
        if ((flags&1u) != 0u) CrtFreePointer(this);
        return this;
    }
};

template<class Table, bool SharedTable> struct Msvc6CtypeLocaleFacet : Msvc6::CtypeData
{
    typedef Table Selector;

    void Begin() { references = 0u; vftable = Table::GetTable(); }
    void Initialize(void*)
    {
        if (SharedTable)
        {
            unsigned int lockStorage;
            (void)CrtStaticLockConstruct(PtrToU32(&lockStorage));
            Msvc6::CtypeDescriptor captured;
            descriptor = *reinterpret_cast<const Msvc6::CtypeDescriptor*>(
                U32ToPtr(Msvc6CtypeDescriptor(PtrToU32(&captured))));
            if (RecordDBCtypeControl::_Cltab == 0)
            {
                RecordDBCtypeControl::_Cltab = reinterpret_cast<const short*>(descriptor.table);
                (void)RegisterCrtAtExit(&RecordDBCtypeTerm);
                descriptor.ownsTable = 0u;
            }
            (void)CrtStaticLockDestruct(PtrToU32(&lockStorage));
        }
        else
        {
            Msvc6::CtypeDescriptor captured;
            descriptor = *reinterpret_cast<const Msvc6::CtypeDescriptor*>(
                U32ToPtr(Msvc6CtypeDescriptor(PtrToU32(&captured))));
        }
    }
    void AbortConstruction() { vftable = Msvc6::BaseFacetTable(); }
    void Destruct()
    {
        vftable = Table::GetTable();
        if (descriptor.ownsTable != 0u) CrtFreePointer(descriptor.table);
        vftable = Msvc6::BaseFacetTable();
    }
    void* __thiscall ScalarDeletingDestructor(unsigned int flags)
    {
        Destruct();
        if ((flags&1u) != 0u) CrtFreePointer(this);
        return this;
    }
};

template<class Character, class Table> struct Msvc6NumpunctLocaleFacet : Msvc6::NumpunctData<Character>
{
    typedef Table Selector;
    typedef Msvc6::NumpunctData<Character> Data;
    using Data::vftable;
    using Data::references;
    using Data::grouping;
    using Data::decimalPoint;
    using Data::thousandsSeparator;
    using Data::falseName;
    using Data::trueName;

    void Begin() { references = 0u; vftable = Table::GetTable(); }
    void Initialize(void*)
    {
        const CrtLocaleConvention& convention = *Msvc6::GetLocaleConvention();
        decimalPoint = Msvc6LocaleCharacters<Character>::Convert(*convention.decimalPoint);
        thousandsSeparator = Msvc6LocaleCharacters<Character>::Convert(*convention.thousandsSeparator);
        grouping = Msvc6LocaleCharacters<char>::Duplicate(convention.grouping);
        falseName = Msvc6LocaleCharacters<Character>::Duplicate("false");
        trueName = Msvc6LocaleCharacters<Character>::Duplicate("true");
    }
    void AbortConstruction() { vftable = Msvc6::BaseFacetTable(); }
    void Destruct()
    {
        vftable = Table::GetTable();
        CrtFreePointer(grouping);
        CrtFreePointer(falseName);
        CrtFreePointer(trueName);
        vftable = Msvc6::BaseFacetTable();
    }
    void* __thiscall ScalarDeletingDestructor(unsigned int flags)
    {
        Destruct();
        if ((flags&1u) != 0u) CrtFreePointer(this);
        return this;
    }
};

template<class Character, class Table> struct Msvc6MessagesLocaleFacet : Msvc6::MessagesData<Character>
{
    typedef Table Selector;
    typedef Msvc6::MessagesData<Character> Data;
    using Data::vftable;
    using Data::references;
    using Data::negativeName;
    using Data::positiveName;

    void Begin() { references = 0u; vftable = Table::GetTable(); }
    void Initialize(void*)
    {
        negativeName = Msvc6LocaleCharacters<Character>::Duplicate("no");
        positiveName = Msvc6LocaleCharacters<Character>::Duplicate("yes");
    }
    void AbortConstruction() { vftable = Msvc6::BaseFacetTable(); }
    void Destruct()
    {
        vftable = Table::GetTable();
        CrtFreePointer(negativeName);
        CrtFreePointer(positiveName);
        vftable = Msvc6::BaseFacetTable();
    }
    void* __thiscall ScalarDeletingDestructor(unsigned int flags)
    {
        Destruct();
        if ((flags&1u) != 0u) CrtFreePointer(this);
        return this;
    }
};

template<class Character, class Table> struct Msvc6TimeGetLocaleFacet : Msvc6::TimeGetData<Character>
{
    typedef Table Selector;
    typedef Msvc6::TimeGetData<Character> Data;
    using Data::vftable;
    using Data::references;
    using Data::days;
    using Data::months;

    void Begin() { references = 0u; vftable = Table::GetTable(); }
    void Initialize(void* localeInfo)
    {
        days = Msvc6LocaleCharacters<Character>::Duplicate(Msvc6LocaleDays(localeInfo));
        months = Msvc6LocaleCharacters<Character>::Duplicate(Msvc6LocaleMonths(localeInfo));
    }
    void AbortConstruction() { vftable = Msvc6::BaseFacetTable(); }
    void Destruct()
    {
        vftable = Table::GetTable();
        CrtFreePointer(days);
        CrtFreePointer(months);
        vftable = Msvc6::BaseFacetTable();
    }
    void* __thiscall ScalarDeletingDestructor(unsigned int flags)
    {
        Destruct();
        if ((flags&1u) != 0u) CrtFreePointer(this);
        return this;
    }
};

struct CrtTimeNamesOwner
{
    void* pointer;
    CrtTimeNamesOwner():pointer(U32ToPtr(Msvc6GetTimeNames())) {}
    ~CrtTimeNamesOwner() noexcept(false) { CrtFreePointer(pointer); }
};

template<class Table> struct Msvc6TimePutLocaleFacet : Msvc6::TimePutData
{
    typedef Table Selector;

    void Begin() { references = 0u; timeNames = 0; vftable = Table::GetTable(); }
    void Initialize(void*)
    {
        CrtTimeNamesOwner captured;
        timeNames = captured.pointer;
        captured.pointer = 0;
    }
    void AbortConstruction()
    {
        CrtFreePointer(timeNames);
        vftable = Msvc6::BaseFacetTable();
    }
    void Destruct()
    {
        vftable = Table::GetTable();
        CrtFreePointer(timeNames);
        vftable = Msvc6::BaseFacetTable();
    }
    void* __thiscall ScalarDeletingDestructor(unsigned int flags)
    {
        Destruct();
        if ((flags&1u) != 0u) CrtFreePointer(this);
        return this;
    }
};

static const char MoneyDefaultPattern[4] = { '$', '+', 'v', 'x' };
static const char MoneyPatterns[80] =
{
    '+', 'v', '$', 'x',
    '+', 'v', '$', 'x',
    'v', '$', '+', 'x',
    'v', '+', '$', 'x',
    'v', '$', '+', 'x',
    '+', '$', 'v', 'x',
    '+', '$', 'v', 'x',
    '$', 'v', '+', 'x',
    '+', '$', 'v', 'x',
    '$', '+', 'v', 'x',
    '+', 'v', ' ', '$',
    '+', 'v', ' ', '$',
    'v', ' ', '$', '+',
    'v', '+', ' ', '$',
    'v', ' ', '$', '+',
    '+', '$', ' ', 'v',
    '+', '$', ' ', 'v',
    '$', ' ', 'v', '+',
    '+', '$', ' ', 'v',
    '$', ' ', '+', 'v'
};

template<class Character, class Table, class BaseTable, bool International>
struct Msvc6MoneypunctLocaleFacet : Msvc6::MoneypunctData<Character>
{
    typedef Table Selector;
    typedef Msvc6::MoneypunctData<Character> Data;
    using Data::vftable;
    using Data::references;
    using Data::grouping;
    using Data::decimalPoint;
    using Data::thousandsSeparator;
    using Data::currencySymbol;
    using Data::positiveSign;
    using Data::negativeSign;
    using Data::fractionDigits;
    using Data::positivePattern;
    using Data::negativePattern;
    using Data::international;

    void Begin()
    {
        references = 0u;
        international = International?1u:0u;
        vftable = BaseTable::GetTable();
    }
    void Pattern(unsigned char* output, unsigned char space, unsigned char precedes, signed char position)
    {
        const char* source = MoneyDefaultPattern;
        if (international == 0u && space <= 1u && precedes <= 1u && position >= 0 && position <= 4)
            source = MoneyPatterns+(space != 0u?40u:0u)+(precedes != 0u?20u:0u)+
                static_cast<unsigned int>(position);
        std::memcpy(output, source, sizeof(MoneyDefaultPattern));
    }
    void Initialize(void*)
    {
        const CrtLocaleConvention& convention = *Msvc6::GetLocaleConvention();
        decimalPoint = Msvc6LocaleCharacters<Character>::Convert(*convention.monetaryDecimalPoint);
        thousandsSeparator = Msvc6LocaleCharacters<Character>::Convert(*convention.monetaryThousandsSeparator);
        grouping = Msvc6LocaleCharacters<char>::Duplicate(convention.monetaryGrouping);
        currencySymbol = Msvc6LocaleCharacters<Character>::Duplicate(international != 0u?
            convention.internationalCurrencySymbol:convention.currencySymbol);
        positiveSign = Msvc6LocaleCharacters<Character>::Duplicate(
            convention.positiveSignPosition >= 0 && convention.positiveSignPosition <= 4?
            convention.positiveSign:StringGlobals::EmptyText());
        negativeSign = Msvc6LocaleCharacters<Character>::Duplicate(
            convention.negativeSignPosition >= 0 && convention.negativeSignPosition <= 4?
            convention.negativeSign:"-");
        fractionDigits = international != 0u?convention.internationalFractionDigits:convention.fractionDigits;
        if (fractionDigits<0 || fractionDigits>=127) fractionDigits = 0;
        Pattern(positivePattern, convention.positiveSpaceSeparates,
            convention.positiveCurrencyPrecedes, convention.positiveSignPosition);
        Pattern(negativePattern, convention.negativeSpaceSeparates,
            convention.negativeCurrencyPrecedes, convention.negativeSignPosition);
    }
    void AbortConstruction() { vftable = Msvc6::BaseFacetTable(); }
    void Destruct()
    {
        vftable = BaseTable::GetTable();
        CrtFreePointer(grouping);
        CrtFreePointer(currencySymbol);
        CrtFreePointer(positiveSign);
        CrtFreePointer(negativeSign);
        vftable = Msvc6::BaseFacetTable();
    }
    void* __thiscall ScalarDeletingDestructor(unsigned int flags)
    {
        Destruct();
        if ((flags&1u) != 0u) CrtFreePointer(this);
        return this;
    }
};

template<class Storage, unsigned int Category, bool GuardFreshConstruction = true>
struct Msvc6LocaleFacet
{
    typedef Storage Owner;
    typedef typename Storage::Selector Tag;
    typedef typename Msvc6::FacetDataType<Tag>::Type Data;
    typedef Msvc6::FacetState<Tag> State;
    enum : unsigned int { Mask = Category };
    enum { GuardFresh = GuardFreshConstruction };
};

typedef Msvc6LocaleFacet<Msvc6CtypeLocaleFacet<Msvc6::CharCtype, true>,
    2u> RecordDBCharCtypeFacet;
typedef Msvc6LocaleFacet<Msvc6EmptyLocaleFacet<Msvc6::CharNumGet>,
    8u, false> RecordDBCharNumGetFacet;
typedef Msvc6LocaleFacet<Msvc6EmptyLocaleFacet<Msvc6::CharNumPut>,
    8u, false> RecordDBCharNumPutFacet;
typedef Msvc6LocaleFacet<Msvc6NumpunctLocaleFacet<char, Msvc6::CharNumpunct>,
    8u> RecordDBCharNumpunctFacet;
typedef Msvc6LocaleFacet<Msvc6StampedLocaleFacet<Msvc6::CharCodecvt, true>,
    2u, false> RecordDBCharCodecvtFacet;
typedef Msvc6LocaleFacet<Msvc6StampedLocaleFacet<Msvc6::CharCollate, false>,
    1u, false> RecordDBCharCollateFacet;
typedef Msvc6LocaleFacet<Msvc6MessagesLocaleFacet<char, Msvc6::CharMessages>,
    32u> RecordDBCharMessagesFacet;
typedef Msvc6LocaleFacet<Msvc6EmptyLocaleFacet<Msvc6::CharMoneyGet>,
    4u, false> RecordDBCharMoneyGetFacet;
typedef Msvc6LocaleFacet<Msvc6EmptyLocaleFacet<Msvc6::CharMoneyPut>,
    4u, false> RecordDBCharMoneyPutFacet;
typedef Msvc6LocaleFacet<Msvc6MoneypunctLocaleFacet<char, Msvc6::CharMoneypunct, Msvc6::CharMoneyBase, false>,
    8u> RecordDBCharMoneypunctFacet;
typedef Msvc6LocaleFacet<Msvc6MoneypunctLocaleFacet<char, Msvc6::CharMoneypunctIntl, Msvc6::CharMoneyBase, true>,
    8u> RecordDBCharMoneypunctIntlFacet;
typedef Msvc6LocaleFacet<Msvc6TimeGetLocaleFacet<char, Msvc6::CharTimeGet>,
    16u> RecordDBCharTimeGetFacet;
typedef Msvc6LocaleFacet<Msvc6TimePutLocaleFacet<Msvc6::CharTimePut>,
    16u> RecordDBCharTimePutFacet;
typedef Msvc6LocaleFacet<Msvc6CtypeLocaleFacet<Msvc6::WideCtype, false>,
    2u, false> RecordDBWideCtypeFacet;
typedef Msvc6LocaleFacet<Msvc6EmptyLocaleFacet<Msvc6::WideNumGet>,
    8u, false> RecordDBWideNumGetFacet;
typedef Msvc6LocaleFacet<Msvc6EmptyLocaleFacet<Msvc6::WideNumPut>,
    8u, false> RecordDBWideNumPutFacet;
typedef Msvc6LocaleFacet<Msvc6NumpunctLocaleFacet<unsigned short, Msvc6::WideNumpunct>,
    8u> RecordDBWideNumpunctFacet;
typedef Msvc6LocaleFacet<Msvc6StampedLocaleFacet<Msvc6::WideCollate, false>,
    1u, false> RecordDBWideCollateFacet;
typedef Msvc6LocaleFacet<Msvc6MessagesLocaleFacet<unsigned short, Msvc6::WideMessages>,
    32u> RecordDBWideMessagesFacet;
typedef Msvc6LocaleFacet<Msvc6EmptyLocaleFacet<Msvc6::WideMoneyGet>,
    4u, false> RecordDBWideMoneyGetFacet;
typedef Msvc6LocaleFacet<Msvc6EmptyLocaleFacet<Msvc6::WideMoneyPut>,
    4u, false> RecordDBWideMoneyPutFacet;
typedef Msvc6LocaleFacet<Msvc6MoneypunctLocaleFacet<unsigned short, Msvc6::WideMoneypunct, Msvc6::WideMoneyBase, false>,
    8u> RecordDBWideMoneypunctFacet;
typedef Msvc6LocaleFacet<Msvc6MoneypunctLocaleFacet<unsigned short, Msvc6::WideMoneypunctIntl, Msvc6::WideMoneyBase, true>,
    8u> RecordDBWideMoneypunctIntlFacet;
typedef Msvc6LocaleFacet<Msvc6TimeGetLocaleFacet<unsigned short, Msvc6::WideTimeGet>,
    16u> RecordDBWideTimeGetFacet;
typedef Msvc6LocaleFacet<Msvc6TimePutLocaleFacet<Msvc6::WideTimePut>,
    16u> RecordDBWideTimePutFacet;
typedef Msvc6LocaleFacet<Msvc6StampedLocaleFacet<Msvc6::WideCodecvt, true>,
    2u, false> RecordDBWideCodecvtFacet;


template<class Storage> static void Msvc6FinishLocaleFacet(Storage*) {}

template<class Character, class Table, class BaseTable, bool International>
static void Msvc6FinishLocaleFacet(Msvc6MoneypunctLocaleFacet<Character, Table, BaseTable, International>* storage)
{
    storage->vftable = Table::GetTable();
}

template<class Storage> static void Msvc6ConstructDefaultFacet(Storage* storage)
{
    storage->Begin();
    try
    {
        {
            Msvc6LocaleInfoScope localeInfo("C");
            storage->Initialize(localeInfo.Data());
        }
        Msvc6FinishLocaleFacet(storage);
    }
    catch (...)
    {
        storage->AbortConstruction();
        throw;
    }
}

template<class Table, bool Conversion>
static void Msvc6ConstructDefaultFacet(Msvc6StampedLocaleFacet<Table, Conversion>* storage)
{
    storage->Begin();
    try
    {
        AS_RECORD_STRING_CONTROL localeInfo[4];
        (void)Msvc6LocaleConstruct(localeInfo, PtrToU32("C"));
        storage->Initialize(localeInfo);
        (void)Msvc6LocaleDestruct(localeInfo);
    }
    catch (...)
    {
        storage->AbortConstruction();
        throw;
    }
}

static void Msvc6ConstructDefaultFacet(RecordDBWideCtypeFacet::Owner* storage)
{
    storage->Begin();
    try
    {
        AS_RECORD_STRING_CONTROL localeInfo[4];
        (void)Msvc6LocaleConstruct(localeInfo, PtrToU32("C"));
        storage->Initialize(localeInfo);
        (void)Msvc6LocaleDestruct(localeInfo);
    }
    catch (...)
    {
        storage->AbortConstruction();
        throw;
    }
}

template<class Table>
static void Msvc6ConstructDefaultFacet(Msvc6TimePutLocaleFacet<Table>* storage)
{
    storage->Begin();
    try
    {
        AS_RECORD_STRING_CONTROL localeInfo[4];
        (void)Msvc6LocaleConstruct(localeInfo, PtrToU32("C"));
        storage->Initialize(localeInfo);
        (void)Msvc6LocaleDestruct(localeInfo);
    }
    catch (...)
    {
        storage->AbortConstruction();
        throw;
    }
}

static void Msvc6ConstructDefaultFacet(RecordDBCharCtypeFacet::Owner* storage)
{
    (void)RecordDBCtypeCharCtor(storage, 0u, 0u, 0u);
}

static void Msvc6ConstructDefaultFacet(RecordDBCharNumPutFacet::Owner* storage)
{
    (void)RecordDBNumPutCtor(storage, 0u);
}

static void Msvc6ConstructDefaultFacet(RecordDBCharNumpunctFacet::Owner* storage)
{
    (void)RecordDBNumpunctCtor(storage, 0u);
}

static void Msvc6ConstructDefaultFacet(RecordDBCharCodecvtFacet::Owner* storage)
{
    (void)RecordDBCodecvtCtor(storage, 0u);
}

template<class Facet> static typename Facet::Owner* Msvc6CreateLocaleFacet(void* localeInfo, bool useDefault)
{
    typedef typename Facet::Owner Storage;
    Storage* const allocation = static_cast<Storage*>(CrtNew(sizeof(Storage)));
    if (!useDefault && !Facet::GuardFresh)
    {
        if (allocation)
        {
            allocation->Begin();
            allocation->Initialize(localeInfo);
            Msvc6FinishLocaleFacet(allocation);
        }
        return allocation;
    }
    if (allocation)
    {
        try
        {
            if (useDefault)
                Msvc6ConstructDefaultFacet(allocation);
            else
            {
                allocation->Begin();
                try
                {
                    allocation->Initialize(localeInfo);
                    Msvc6FinishLocaleFacet(allocation);
                }
                catch (...)
                {
                    allocation->AbortConstruction();
                    throw;
                }
            }
        }
        catch (...)
        {
            CrtFreePointer(allocation);
            throw;
        }
    }
    return allocation;
}

template<class Facet> static void __cdecl Msvc6SavedLocaleFacetCleanup()
{
    CrtStaticLockGuard lock;
    typename Facet::Data* const value = Facet::State::_Facsav;
    void* const unreferenced = RecordDBRefCountReleaseAndReturnIfZero(value);
    if (unreferenced) Msvc6::DeleteFacet(unreferenced);
    Facet::State::_Facsav = 0;
}

template<class Facet> static typename Facet::Owner* Msvc6SaveLocaleFacet(typename Facet::Owner* value)
{
    CrtStaticLockGuard lock;
    Facet::State::_Facsav = value;
    reinterpret_cast<MsvcLocaleFacetRef*>(value)->Retain();
    (void)RegisterCrtAtExit(&Msvc6SavedLocaleFacetCleanup<Facet>);
    return value;
}

template<> RecordDBCharCtypeFacet::Owner* Msvc6SaveLocaleFacet<RecordDBCharCtypeFacet>(RecordDBCharCtypeFacet::Owner* value)
{
    return reinterpret_cast<RecordDBCharCtypeFacet::Owner*>(
        RecordDBSaveCtypeFacet(reinterpret_cast<MsvcLocaleFacetRef*>(value)));
}

template<> RecordDBCharNumPutFacet::Owner* Msvc6SaveLocaleFacet<RecordDBCharNumPutFacet>(RecordDBCharNumPutFacet::Owner* value)
{
    return reinterpret_cast<RecordDBCharNumPutFacet::Owner*>(
        RecordDBSaveNumPutFacet(reinterpret_cast<MsvcLocaleFacetRef*>(value)));
}

template<> RecordDBCharNumpunctFacet::Owner* Msvc6SaveLocaleFacet<RecordDBCharNumpunctFacet>(RecordDBCharNumpunctFacet::Owner* value)
{
    return reinterpret_cast<RecordDBCharNumpunctFacet::Owner*>(
        RecordDBSaveNumpunctFacet(reinterpret_cast<MsvcLocaleFacetRef*>(value)));
}

template<> RecordDBCharCodecvtFacet::Owner* Msvc6SaveLocaleFacet<RecordDBCharCodecvtFacet>(RecordDBCharCodecvtFacet::Owner* value)
{
    return reinterpret_cast<RecordDBCharCodecvtFacet::Owner*>(
        RecordDBSaveCodecvtFacet(reinterpret_cast<MsvcLocaleFacetRef*>(value)));
}

template<class Facet> static const typename Facet::Data* Msvc6UseLocaleFacet(const RecordDBLocaleControl* locale)
{
    CrtStaticLockGuard lock;
    const unsigned int id = RecordDBGetOrAssignGlobalSequenceId(&Facet::State::id);
    const typename Facet::Data* value = reinterpret_cast<const typename Facet::Data*>(U32ToPtr(
        Msvc6LocaleGetFacet(const_cast<RecordDBLocaleControl*>(locale), id, 1u)));
    if (value == 0)
    {
        if (static_cast<unsigned char>(Msvc6LocaleCanSupplyFacet(
            const_cast<RecordDBLocaleControl*>(locale))) == 0u)
        {
            RecordDBBadCastControl exceptionObject;
            exceptionObject.Construct("missing locale facet");
            Msvc6::ThrowException(&exceptionObject, Msvc6::BadCastThrowInfo());
        }
        value = static_cast<const typename Facet::Data*>(Facet::State::_Psave);
        if (value == 0)
        {
            typename Facet::Owner* const created = Msvc6CreateLocaleFacet<Facet>(0, true);
            value = Msvc6SaveLocaleFacet<Facet>(created);
            Facet::State::_Psave = value;
        }
    }
    return value;
}

template<class Facet> static void Msvc6AddLocaleFacet(void* localeInfo, unsigned int mask,
    RecordDBLocaleImplControl* target, const RecordDBLocaleControl* sourceLocale)
{
    if ((mask&Facet::Mask) != 0u)
    {
        const typename Facet::Data* value;
        unsigned int id;
        if (sourceLocale)
        {
            id = RecordDBGetOrAssignGlobalSequenceId(&Facet::State::id);
            value = Msvc6UseLocaleFacet<Facet>(sourceLocale);
        }
        else
        {
            value = Msvc6CreateLocaleFacet<Facet>(localeInfo, false);
            id = RecordDBGetOrAssignGlobalSequenceId(&Facet::State::id);
        }
        target->AddFacet(const_cast<typename Facet::Data*>(value), id);
    }
}

static void Msvc6MakeCharLocaleFacets(void* localeInfo, unsigned int mask,
    RecordDBLocaleImplControl* target, const RecordDBLocaleControl* sourceLocale)
{
    CrtStaticLockGuard lock;
    Msvc6AddLocaleFacet<RecordDBCharCollateFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharMessagesFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharMoneyGetFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharMoneyPutFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharMoneypunctFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharMoneypunctIntlFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharTimeGetFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharTimePutFacet>(localeInfo, mask, target, sourceLocale);
}

static void Msvc6MakeWideLocaleFacets(void* localeInfo, unsigned int mask,
    RecordDBLocaleImplControl* target, const RecordDBLocaleControl* sourceLocale)
{
    CrtStaticLockGuard lock;
    Msvc6AddLocaleFacet<RecordDBWideCtypeFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideNumGetFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideNumPutFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideNumpunctFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideCollateFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideMessagesFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideMoneyGetFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideMoneyPutFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideMoneypunctFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideMoneypunctIntlFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideTimeGetFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideTimePutFacet>(localeInfo, mask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBWideCodecvtFacet>(localeInfo, mask, target, sourceLocale);
}

}

namespace Msvc6
{
template<> const FacetDataType<CharCtype>::Type*
LocaleFacetUse<CharCtype>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharCtypeFacet>(&locale);
}
template<> const FacetDataType<CharNumGet>::Type*
LocaleFacetUse<CharNumGet>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharNumGetFacet>(&locale);
}
template<> const FacetDataType<CharNumPut>::Type*
LocaleFacetUse<CharNumPut>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharNumPutFacet>(&locale);
}
template<> const FacetDataType<CharNumpunct>::Type*
LocaleFacetUse<CharNumpunct>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharNumpunctFacet>(&locale);
}
template<> const FacetDataType<CharCodecvt>::Type*
LocaleFacetUse<CharCodecvt>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharCodecvtFacet>(&locale);
}
template<> const FacetDataType<CharCollate>::Type*
LocaleFacetUse<CharCollate>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharCollateFacet>(&locale);
}
template<> const FacetDataType<CharMessages>::Type*
LocaleFacetUse<CharMessages>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharMessagesFacet>(&locale);
}
template<> const FacetDataType<CharMoneyGet>::Type*
LocaleFacetUse<CharMoneyGet>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharMoneyGetFacet>(&locale);
}
template<> const FacetDataType<CharMoneyPut>::Type*
LocaleFacetUse<CharMoneyPut>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharMoneyPutFacet>(&locale);
}
template<> const FacetDataType<CharMoneypunct>::Type*
LocaleFacetUse<CharMoneypunct>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharMoneypunctFacet>(&locale);
}
template<> const FacetDataType<CharMoneypunctIntl>::Type*
LocaleFacetUse<CharMoneypunctIntl>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharMoneypunctIntlFacet>(&locale);
}
template<> const FacetDataType<CharTimeGet>::Type*
LocaleFacetUse<CharTimeGet>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharTimeGetFacet>(&locale);
}
template<> const FacetDataType<CharTimePut>::Type*
LocaleFacetUse<CharTimePut>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBCharTimePutFacet>(&locale);
}
template<> const FacetDataType<WideCtype>::Type*
LocaleFacetUse<WideCtype>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideCtypeFacet>(&locale);
}
template<> const FacetDataType<WideNumGet>::Type*
LocaleFacetUse<WideNumGet>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideNumGetFacet>(&locale);
}
template<> const FacetDataType<WideNumPut>::Type*
LocaleFacetUse<WideNumPut>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideNumPutFacet>(&locale);
}
template<> const FacetDataType<WideNumpunct>::Type*
LocaleFacetUse<WideNumpunct>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideNumpunctFacet>(&locale);
}
template<> const FacetDataType<WideCollate>::Type*
LocaleFacetUse<WideCollate>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideCollateFacet>(&locale);
}
template<> const FacetDataType<WideMessages>::Type*
LocaleFacetUse<WideMessages>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideMessagesFacet>(&locale);
}
template<> const FacetDataType<WideMoneyGet>::Type*
LocaleFacetUse<WideMoneyGet>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideMoneyGetFacet>(&locale);
}
template<> const FacetDataType<WideMoneyPut>::Type*
LocaleFacetUse<WideMoneyPut>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideMoneyPutFacet>(&locale);
}
template<> const FacetDataType<WideMoneypunct>::Type*
LocaleFacetUse<WideMoneypunct>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideMoneypunctFacet>(&locale);
}
template<> const FacetDataType<WideMoneypunctIntl>::Type*
LocaleFacetUse<WideMoneypunctIntl>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideMoneypunctIntlFacet>(&locale);
}
template<> const FacetDataType<WideTimeGet>::Type*
LocaleFacetUse<WideTimeGet>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideTimeGetFacet>(&locale);
}
template<> const FacetDataType<WideTimePut>::Type*
LocaleFacetUse<WideTimePut>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideTimePutFacet>(&locale);
}
template<> const FacetDataType<WideCodecvt>::Type*
LocaleFacetUse<WideCodecvt>::Get(const RecordDBLocaleControl& locale)
{
    return Msvc6UseLocaleFacet<RecordDBWideCodecvtFacet>(&locale);
}
}

RecordDBLocaleImplControl* Msvc6MakeLocale(void* localeInfo, unsigned int categoryMask,
    RecordDBLocaleImplControl* target, const RecordDBLocaleControl* sourceLocale)
{
    CrtStaticLockGuard lock;
    Msvc6AddLocaleFacet<RecordDBCharCtypeFacet>(localeInfo, categoryMask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharNumGetFacet>(localeInfo, categoryMask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharNumPutFacet>(localeInfo, categoryMask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharNumpunctFacet>(localeInfo, categoryMask, target, sourceLocale);
    Msvc6AddLocaleFacet<RecordDBCharCodecvtFacet>(localeInfo, categoryMask, target, sourceLocale);
    Msvc6MakeCharLocaleFacets(localeInfo, categoryMask, target, sourceLocale);
    Msvc6MakeWideLocaleFacets(localeInfo, categoryMask, target, sourceLocale);
    target->category |= categoryMask;
    RecordDBLineString name(static_cast<const AS_RECORD_STRING_CONTROL*>(localeInfo)[3]);
    (void)RecordDBStringAssignSubstring(&target->name, &name, 0u, AS_RECORD_STRING_CONTROL::npos);
    return target;
}

int RecordDBInitializeLocaleMakers()
{
#if defined(_WIN32)
    Msvc6::BindFacetDestructor(Msvc6::CharNumGet::GetTable(),
        &RecordDBCharNumGetFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::CharCollate::GetTable(),
        &RecordDBCharCollateFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::CharMessages::GetTable(),
        &RecordDBCharMessagesFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::CharMoneyGet::GetTable(),
        &RecordDBCharMoneyGetFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::CharMoneyPut::GetTable(),
        &RecordDBCharMoneyPutFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::CharMoneypunct::GetTable(),
        &RecordDBCharMoneypunctFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::CharMoneypunctIntl::GetTable(),
        &RecordDBCharMoneypunctIntlFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::CharTimeGet::GetTable(),
        &RecordDBCharTimeGetFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::CharTimePut::GetTable(),
        &RecordDBCharTimePutFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideCtype::GetTable(),
        &RecordDBWideCtypeFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideNumGet::GetTable(),
        &RecordDBWideNumGetFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideNumPut::GetTable(),
        &RecordDBWideNumPutFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideNumpunct::GetTable(),
        &RecordDBWideNumpunctFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideCollate::GetTable(),
        &RecordDBWideCollateFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideMessages::GetTable(),
        &RecordDBWideMessagesFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideMoneyGet::GetTable(),
        &RecordDBWideMoneyGetFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideMoneyPut::GetTable(),
        &RecordDBWideMoneyPutFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideMoneypunct::GetTable(),
        &RecordDBWideMoneypunctFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideMoneypunctIntl::GetTable(),
        &RecordDBWideMoneypunctIntlFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideTimeGet::GetTable(),
        &RecordDBWideTimeGetFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideTimePut::GetTable(),
        &RecordDBWideTimePutFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideCodecvt::GetTable(),
        &RecordDBWideCodecvtFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::CharMoneyBase::GetTable(),
        &RecordDBCharMoneypunctFacet::Owner::ScalarDeletingDestructor);
    Msvc6::BindFacetDestructor(Msvc6::WideMoneyBase::GetTable(),
        &RecordDBWideMoneypunctFacet::Owner::ScalarDeletingDestructor);
#endif
    return 1;
}

namespace Msvc6
{
std::FILE* __cdecl Fiopen(const char* path, int openMode)
{
    enum OpenMode { Input = 1, Output = 2, AtEnd = 4, Append = 8, Truncate = 16, Binary = 32 };
    struct FileMode { int flags; const char* text; };
    static const FileMode modes[] =
    {
        {Input, "r"},
        {Output, "w"},
        {Output | Truncate, "w"},
        {Output | Append, "a"},
        {Input | Binary, "rb"},
        {Output | Binary, "wb"},
        {Output | Truncate | Binary, "wb"},
        {Output | Append | Binary, "ab"},
        {Input | Output, "r+"},
        {Input | Output | Truncate, "w+"},
        {Input | Output | Append, "a+"},
        {Input | Output | Binary, "r+b"},
        {Input | Output | Truncate | Binary, "w+b"},
        {Input | Output | Append | Binary, "a+b"},
        {0, 0}
    };
    const bool seekToEnd = (openMode & AtEnd) != 0;
    const int normalized = openMode & ~AtEnd;
    const FileMode* mode = modes;
    while (mode->flags != 0 && mode->flags != normalized) ++mode;
    if (mode->flags == 0) return 0;
    std::FILE* file = std::fopen(path, mode->text);
    if (file && seekToEnd && std::fseek(file, 0, SEEK_END) != 0)
    {
        (void)std::fclose(file);
        return 0;
    }
    return file;
}
}


namespace {

enum CrtCharacterMask
{
    CtypeUpper = 1, CtypeLower = 2, CtypeDigit = 4, CtypeSpace = 8,
    CtypePunctuation = 16, CtypeControl = 32, CtypeBlank = 64,
    CtypeHexDigit = 128, CtypeAlphabetic = 256, CtypeLeadByte = 32768
};

// Entry zero is the EOF entry; the active pointer addresses character zero.
unsigned short crtClassicCharacterTypes[257] = {
    0, CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl,
    CtypeControl, CtypeControl, CtypeControl | CtypeSpace, CtypeControl | CtypeSpace, CtypeControl | CtypeSpace, CtypeControl | CtypeSpace, CtypeControl | CtypeSpace, CtypeControl,
    CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl,
    CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl, CtypeControl,
    CtypeControl, CtypeBlank | CtypeSpace, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation,
    CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation,
    CtypePunctuation, CtypeDigit | CtypeHexDigit, CtypeDigit | CtypeHexDigit, CtypeDigit | CtypeHexDigit, CtypeDigit | CtypeHexDigit, CtypeDigit | CtypeHexDigit, CtypeDigit | CtypeHexDigit, CtypeDigit | CtypeHexDigit,
    CtypeDigit | CtypeHexDigit, CtypeDigit | CtypeHexDigit, CtypeDigit | CtypeHexDigit, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation,
    CtypePunctuation, CtypePunctuation, CtypeUpper | CtypeHexDigit, CtypeUpper | CtypeHexDigit, CtypeUpper | CtypeHexDigit, CtypeUpper | CtypeHexDigit, CtypeUpper | CtypeHexDigit, CtypeUpper | CtypeHexDigit,
    CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper,
    CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper,
    CtypeUpper, CtypeUpper, CtypeUpper, CtypeUpper, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation,
    CtypePunctuation, CtypePunctuation, CtypeLower | CtypeHexDigit, CtypeLower | CtypeHexDigit, CtypeLower | CtypeHexDigit, CtypeLower | CtypeHexDigit, CtypeLower | CtypeHexDigit, CtypeLower | CtypeHexDigit,
    CtypeLower, CtypeLower, CtypeLower, CtypeLower, CtypeLower, CtypeLower, CtypeLower, CtypeLower,
    CtypeLower, CtypeLower, CtypeLower, CtypeLower, CtypeLower, CtypeLower, CtypeLower, CtypeLower,
    CtypeLower, CtypeLower, CtypeLower, CtypeLower, CtypePunctuation, CtypePunctuation, CtypePunctuation, CtypePunctuation,
    CtypeControl, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0
};
unsigned short* crtCharacterTypes = crtClassicCharacterTypes + 1;
char crtDecimalPoint = '.';
int crtMaximumMultibyteLength = 1;
unsigned int crtCtypeLocale = 0;
unsigned int crtCollateLocale = 0;
unsigned int crtCtypeCodePage = 0;
unsigned int crtCollateCodePage = 0;
volatile LONG crtLocaleReaders = 0;
unsigned int crtLocaleWriters = 0;

char crtLocaleDecimalPoint[] = ".";
char crtLocaleEmptyText[] = "";
CrtLocaleConvention crtClassicLocaleConvention = {
    crtLocaleDecimalPoint, crtLocaleEmptyText, crtLocaleEmptyText,
    crtLocaleEmptyText, crtLocaleEmptyText, crtLocaleEmptyText,
    crtLocaleEmptyText, crtLocaleEmptyText, crtLocaleEmptyText,
    crtLocaleEmptyText, 127, 127, 127, 127, 127, 127, 127, 127
};
CrtLocaleConvention* crtCurrentLocaleConvention = &crtClassicLocaleConvention;
const char crtMayName[] = "May";
CrtTimeNames crtClassicTimeNames = {
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
    {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"},
    {"Jan", "Feb", "Mar", "Apr", crtMayName, "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"},
    {"January", "February", "March", "April", crtMayName, "June", "July", "August", "September", "October", "November", "December"},
    "AM", "PM", "M/d/yy", "dddd, MMMM dd, yyyy", "H:mm:ss"
};
CrtTimeNames* crtCurrentTimeNames = &crtClassicTimeNames;

CRITICAL_SECTION crtInitialLockOne = {};
CRITICAL_SECTION crtInitialLockNine = {};
CRITICAL_SECTION crtExitLockStorage = {};
CRITICAL_SECTION crtLockCreationStorage = {};
CRITICAL_SECTION* crtExitLock = &crtExitLockStorage;
CRITICAL_SECTION* crtLockCreationLock = &crtLockCreationStorage;
CRITICAL_SECTION* crtLocaleLock = 0;
CRITICAL_SECTION* crtTypeInfoLock = 0;

CRITICAL_SECTION stlStaticLock = {};
volatile LONG stlStaticLockState = 0;
CrtExitCallback* crtExitCallbacksBegin = 0;
CrtExitCallback* crtExitCallbacksEnd = 0;
unsigned int crtExitProcedureEntered = 0;
unsigned char crtExitReturnToCaller = 0;

}

static void __cdecl CrtAmsgExit(unsigned int code);

static void __cdecl CrtInitLocks()
{
    InitializeCriticalSection(crtLockCreationLock);
    InitializeCriticalSection(crtExitLock);
    InitializeCriticalSection(&crtInitialLockNine);
    InitializeCriticalSection(&crtInitialLockOne);
}

static void __cdecl CrtUnlock(CRITICAL_SECTION* const& section)
{
    LeaveCriticalSection(section);
}

static void __cdecl CrtLock(CRITICAL_SECTION*& slot)
{
    if (slot == 0)
    {
        CRITICAL_SECTION* const section = static_cast<CRITICAL_SECTION*>(
            CrtAllocateWithNewHandler(sizeof(CRITICAL_SECTION), crtNewMode));
        if (section == 0) CrtAmsgExit(17u);
        CrtLock(crtLockCreationLock);
        if (slot == 0)
        {
            InitializeCriticalSection(section);
            slot = section;
        }
        else
            CrtFreePointer(section);
        CrtUnlock(crtLockCreationLock);
    }
    EnterCriticalSection(slot);
}

void Msvc6::LockTypeInfo()
{
    CrtLock(crtTypeInfoLock);
}

void Msvc6::UnlockTypeInfo()
{
    CrtUnlock(crtTypeInfoLock);
}

static unsigned int __cdecl CrtLCMapStringA(
    unsigned int locale, unsigned int flags, unsigned int input, int count,
    unsigned int output, int outputCount, unsigned int codePage, unsigned int strict)
{
    if (crtCharacterMappingApi == 0u)
    {
        if (LCMapStringW(0u, LCMAP_LOWERCASE, L"", 1, 0, 0) != 0)
            crtCharacterMappingApi = 1u;
        else if (LCMapStringA(0u, LCMAP_LOWERCASE, "", 1, 0, 0) != 0)
            crtCharacterMappingApi = 2u;
        else
            return 0u;
    }
    if (count > 0)
    {
        const char* const begin = reinterpret_cast<const char*>(U32ToPtr(input));
        const char* end = begin;
        unsigned int remaining = static_cast<unsigned int>(count);
        while (remaining != 0u && *end != 0)
        {
            ++end;
            --remaining;
        }
        if (*end == 0) count = static_cast<int>(end-begin);
    }
    const unsigned int api = crtCharacterMappingApi;
    if (api == 2u)
        return static_cast<unsigned int>(LCMapStringA(locale, flags,
            reinterpret_cast<const char*>(U32ToPtr(input)), count,
            reinterpret_cast<char*>(U32ToPtr(output)), outputCount));
    if (api != 1u) return 0u;
    if (codePage == 0u) codePage = crtCtypeCodePage;
    const int wideCount = MultiByteToWideChar(codePage, strict != 0u?(MB_PRECOMPOSED|MB_ERR_INVALID_CHARS):MB_PRECOMPOSED,
        reinterpret_cast<const char*>(U32ToPtr(input)), count, 0, 0);
    if (wideCount == 0) return 0u;
    wchar_t* wide;
    __try
    {
        const unsigned int bytes = static_cast<unsigned int>(wideCount)*2u;
        wide = static_cast<wchar_t*>(_alloca((bytes+3u)&~3u));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        wide = 0;
    }
    if (wide == 0) return 0u;
    if (MultiByteToWideChar(codePage, MB_PRECOMPOSED, reinterpret_cast<const char*>(U32ToPtr(input)),
            count, wide, wideCount) == 0)
        return 0u;
    const int mappedCount = LCMapStringW(locale, flags, wide, wideCount, 0, 0);
    if (mappedCount == 0) return 0u;
    if ((flags&LCMAP_SORTKEY) != 0u)
    {
        if (outputCount == 0) return static_cast<unsigned int>(mappedCount);
        if (mappedCount > outputCount) return 0u;
        if (LCMapStringW(locale, flags, wide, wideCount,
                reinterpret_cast<wchar_t*>(U32ToPtr(output)), outputCount) == 0)
            return 0u;
        return static_cast<unsigned int>(mappedCount);
    }
    wchar_t* mapped;
    __try
    {
        const unsigned int bytes = static_cast<unsigned int>(mappedCount)*2u;
        mapped = static_cast<wchar_t*>(_alloca((bytes+3u)&~3u));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        mapped = 0;
    }
    if (mapped == 0) return 0u;
    if (LCMapStringW(locale, flags, wide, wideCount, mapped, mappedCount) == 0)
        return 0u;
    return static_cast<unsigned int>(WideCharToMultiByte(codePage, WC_COMPOSITECHECK|WC_SEPCHARS,
        mapped, mappedCount, outputCount != 0?reinterpret_cast<char*>(U32ToPtr(output)):0,
        outputCount != 0?outputCount:0, 0, 0));
}

unsigned int __cdecl CrtLowercase(unsigned int textAddress)
{
    char* const text = reinterpret_cast<char*>(U32ToPtr(textAddress));
    if (crtCtypeLocale == 0u)
    {
        for (char* cursor = text; *cursor != 0; ++cursor)
            if (*cursor >= 'A' && *cursor <= 'Z') *cursor = static_cast<char>(*cursor+('a'-'A'));
        return textAddress;
    }
    volatile LONG* const readers = &crtLocaleReaders;
    InterlockedIncrement(readers);
    const bool locked = crtLocaleWriters != 0u;
    if (locked)
    {
        InterlockedDecrement(readers);
        CrtLock(crtLocaleLock);
    }
    const unsigned int locale = crtCtypeLocale;
    if (locale == 0u)
    {
        if (locked) CrtUnlock(crtLocaleLock);
        else InterlockedDecrement(readers);
        for (char* cursor = text; *cursor != 0; ++cursor)
            if (*cursor >= 'A' && *cursor <= 'Z') *cursor = static_cast<char>(*cursor+('a'-'A'));
        return textAddress;
    }
    char* converted = 0;
    const unsigned int count = CrtLCMapStringA(locale, LCMAP_LOWERCASE, textAddress, -1, 0u, 0, 0u, 1u);
    if (count != 0u)
    {
        converted = static_cast<char*>(CrtAllocateWithNewHandler(count, crtNewMode));
        if (converted != 0 && CrtLCMapStringA(crtCtypeLocale,
                LCMAP_LOWERCASE, textAddress, -1, PtrToU32(converted), BitsToSigned32(count), 0u, 1u) != 0u)
            strcpy(text, converted);
    }
    if (locked) CrtUnlock(crtLocaleLock);
    else InterlockedDecrement(readers);
    CrtFreePointer(converted);
    return textAddress;
}

unsigned int __cdecl CrtUppercase(unsigned int textAddress)
{
    char* const text = reinterpret_cast<char*>(U32ToPtr(textAddress));
    if (crtCtypeLocale == 0u)
    {
        for (char* cursor = text; *cursor != 0; ++cursor)
            if (*cursor >= 'a' && *cursor <= 'z') *cursor = static_cast<char>(*cursor-('a'-'A'));
        return textAddress;
    }
    volatile LONG* const readers = &crtLocaleReaders;
    InterlockedIncrement(readers);
    const bool locked = crtLocaleWriters != 0u;
    if (locked)
    {
        InterlockedDecrement(readers);
        CrtLock(crtLocaleLock);
    }
    const unsigned int locale = crtCtypeLocale;
    if (locale == 0u)
    {
        if (locked) CrtUnlock(crtLocaleLock);
        else InterlockedDecrement(readers);
        for (char* cursor = text; *cursor != 0; ++cursor)
            if (*cursor >= 'a' && *cursor <= 'z') *cursor = static_cast<char>(*cursor-('a'-'A'));
        return textAddress;
    }
    char* converted = 0;
    const unsigned int count = CrtLCMapStringA(locale, LCMAP_UPPERCASE, textAddress, -1, 0u, 0, 0u, 1u);
    if (count != 0u)
    {
        converted = static_cast<char*>(CrtAllocateWithNewHandler(count, crtNewMode));
        if (converted != 0 && CrtLCMapStringA(crtCtypeLocale,
                LCMAP_UPPERCASE, textAddress, -1, PtrToU32(converted), BitsToSigned32(count), 0u, 1u) != 0u)
            strcpy(text, converted);
    }
    if (locked) CrtUnlock(crtLocaleLock);
    else InterlockedDecrement(readers);
    CrtFreePointer(converted);
    return textAddress;
}

static int __cdecl CrtMessageBoxA(const char* text, const char* caption, unsigned int type)
{
    if (crtMessageBox == 0)
    {
        HMODULE const userInterface = LoadLibraryA("user32.dll");
        if (userInterface == 0) return 0;
        FARPROC const messageBox = GetProcAddress(userInterface, "MessageBoxA");
        crtMessageBox = reinterpret_cast<CrtMessageBoxFunction>(messageBox);
        if (messageBox == 0) return 0;
        crtGetActiveWindow = reinterpret_cast<CrtGetActiveWindowFunction>(
            GetProcAddress(userInterface, "GetActiveWindow"));
        crtGetLastActivePopup = reinterpret_cast<CrtGetLastActivePopupFunction>(
            GetProcAddress(userInterface, "GetLastActivePopup"));
    }
    HWND parent = 0;
    const CrtGetActiveWindowFunction getActiveWindow = crtGetActiveWindow;
    if (getActiveWindow != 0)
    {
        parent = getActiveWindow();
        if (parent != 0)
        {
            const CrtGetLastActivePopupFunction getLastActivePopup = crtGetLastActivePopup;
            if (getLastActivePopup != 0) parent = getLastActivePopup(parent);
        }
    }
    return crtMessageBox(parent, text, caption, type);
}

static void __cdecl CrtWriteRuntimeMessage(unsigned int code)
{
    struct RuntimeMessage
    {
        unsigned int code;
        const char* text;
    };
    static const RuntimeMessage messages[19] = {
        {2u, "R6002\r\n- floating point not loaded\r\n"},
        {8u, "R6008\r\n- not enough space for arguments\r\n"},
        {9u, "R6009\r\n- not enough space for environment\r\n"},
        {10u, "\r\nabnormal program termination\r\n"},
        {16u, "R6016\r\n- not enough space for thread data\r\n"},
        {17u, "R6017\r\n- unexpected multithread lock error\r\n"},
        {18u, "R6018\r\n- unexpected heap error\r\n"},
        {19u, "R6019\r\n- unable to open console device\r\n"},
        {24u, "R6024\r\n- not enough space for _onexit/atexit table\r\n"},
        {25u, "R6025\r\n- pure virtual function call\r\n"},
        {26u, "R6026\r\n- not enough space for stdio initialization\r\n"},
        {27u, "R6027\r\n- not enough space for lowio initialization\r\n"},
        {28u, "R6028\r\n- unable to initialize heap\r\n"},
        {120u, "DOMAIN error\r\n"},
        {121u, "SING error\r\n"},
        {122u, "TLOSS error\r\n"},
        {252u, "\r\n"},
        {255u, "runtime error "},
        // Message lookup probes one entry past its 18 messages into the next constant.
        {0u, reinterpret_cast<const char*>(0x80000000u)}
    };
    unsigned int index = 0u;
    while (index < 18u && messages[index].code != code) ++index;
    if (messages[index].code != code) return;

    const unsigned int mode = crtErrorMode;
    if (mode == 1u || (mode == 0u && crtApplicationType == 1u))
    {
        const DWORD length = static_cast<DWORD>(strlen(
            messages[index].text));
        const char* const text = messages[index].text;
        HANDLE const errorOutput = GetStdHandle(STD_ERROR_HANDLE);
        DWORD written = code;
        (void)WriteFile(errorOutput, text, length, &written, 0);
        return;
    }
    if (code == 252u) return;

    char programName[260];
    if (GetModuleFileNameA(0, programName, 260u) == 0u)
        strcpy(programName, "<program name unknown>");
    char* displayedName = programName;
    if (strlen(programName)+1u > 60u)
    {
        displayedName += strlen(programName)-59u;
        memcpy(displayedName, "...", 3u);
    }
    char message[160];
    strcpy(message, "Runtime Error!\n\nProgram: ");
    strcat(message, displayedName);
    strcat(message, "\n\n");
    strcat(message, messages[index].text);
    (void)CrtMessageBoxA(message, "Microsoft Visual C++ Runtime Library",
        MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND);
}

static void __cdecl CrtWriteRuntimeBanner()
{
    const unsigned int mode = crtErrorMode;
    if (mode == 1u || (mode == 0u && crtApplicationType == 1u))
    {
        CrtWriteRuntimeMessage(252u);
        const CrtBannerFunction banner = crtRuntimeBanner;
        if (banner != 0) banner();
        CrtWriteRuntimeMessage(255u);
    }
}

static void __cdecl CrtAmsgExit(unsigned int code)
{
    if (crtErrorMode == 1u) CrtWriteRuntimeBanner();
    CrtWriteRuntimeMessage(code);
    crtExit(255);
}

static unsigned int __cdecl CrtGetStringTypeA(
    unsigned int infoType, unsigned int input, int count, unsigned int output,
    unsigned int codePage, unsigned int locale, unsigned int strict)
{
    unsigned int api = crtCharacterTypeApi;
    if (api == 0u)
    {
        unsigned short result;
        if (GetStringTypeW(1u, L"", 1, &result) != 0)
            api = 1u;
        else if (GetStringTypeA(0u, 1u, "", 1, &result) != 0)
            api = 2u;
        else
            return 0u;
        crtCharacterTypeApi = api;
    }
    if (api == 2u)
    {
        if (locale == 0u) locale = crtCtypeLocale;
        return static_cast<unsigned int>(GetStringTypeA(locale, infoType,
            reinterpret_cast<const char*>(U32ToPtr(input)), count,
            reinterpret_cast<unsigned short*>(U32ToPtr(output))));
    }
    if (api != 1u) return 0u;
    if (codePage == 0u) codePage = crtCtypeCodePage;
    const int size = MultiByteToWideChar(codePage, strict != 0u?9u:1u,
        reinterpret_cast<const char*>(U32ToPtr(input)), count, 0, 0);
    if (size == 0) return 0u;
    wchar_t* wide;
    __try
    {
        const unsigned int bytes = static_cast<unsigned int>(size)*2u;
        wide = static_cast<wchar_t*>(_alloca((bytes+3u)&~3u));
        memset(wide, 0, bytes);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        wide = 0;
    }
    if (wide == 0) return 0u;
    const int converted = MultiByteToWideChar(codePage, 1u,
        reinterpret_cast<const char*>(U32ToPtr(input)), count, wide, size);
    if (converted == 0) return 0u;
    return static_cast<unsigned int>(GetStringTypeW(infoType, wide, converted,
        reinterpret_cast<unsigned short*>(U32ToPtr(output))));
}

static unsigned int __cdecl CrtIsCtype(unsigned int ch, unsigned int mask)
{
    if (ch+1u <= 256u)
        return crtCharacterTypes[BitsToSigned32(ch)]&mask;
    char bytes[4];
    const unsigned int lead = (ch>>8u)&0xFFu;
    int count;
    if ((crtCharacterTypes[lead] & CtypeLeadByte) != 0u)
    {
        bytes[0] = static_cast<char>(lead);
        bytes[1] = static_cast<char>(ch);
        bytes[2] = 0;
        count = 2;
    }
    else
    {
        bytes[0] = static_cast<char>(ch);
        bytes[1] = 0;
        count = 1;
    }
    unsigned short result;
    if (CrtGetStringTypeA(1u, PtrToU32(bytes), count, PtrToU32(&result), 0u, 0u, 1u) == 0u)
        return 0u;
    return result&mask;
}

static unsigned int Msvc6AsciiCase(unsigned int value, bool upper)
{
    const int character = BitsToSigned32(value);
    if (upper)
        return character >= 'a' && character <= 'z'?value-('a'-'A'):value;
    return character >= 'A' && character <= 'Z'?value+('a'-'A'):value;
}

static unsigned int Msvc6CaseWithLocale(
    unsigned int value, const RecordDBCtypeDescriptor* descriptor, bool upper)
{
    const unsigned int locale = descriptor?descriptor->localeLow:crtCtypeLocale;
    const unsigned int codePage = descriptor?descriptor->localeHigh:crtCtypeCodePage;
    if (locale == 0u) return Msvc6AsciiCase(value, upper);

    if (BitsToSigned32(value)<256)
    {
        const unsigned int mask = upper?2u:1u;
        unsigned int classification;
        if (crtMaximumMultibyteLength>1)
            classification = CrtIsCtype(value, mask);
        else
            classification = crtCharacterTypes[BitsToSigned32(value)] & mask;
        if (classification == 0u) return value;
    }

    char input[3];
    const unsigned int lead = (value>>8u)&0xFFu;
    int count;
    if ((crtCharacterTypes[lead] & CtypeLeadByte) != 0u)
    {
        input[0] = static_cast<char>(lead);
        input[1] = static_cast<char>(value);
        input[2] = 0;
        count = 2;
    }
    else
    {
        input[0] = static_cast<char>(value);
        input[1] = 0;
        count = 1;
    }
    unsigned char output[3];
    const unsigned int converted = CrtLCMapStringA(locale,
        upper?LCMAP_UPPERCASE:LCMAP_LOWERCASE, PtrToU32(input), count,
        PtrToU32(output), 3, codePage, 1u);
    if (converted == 0u) return value;
    if (converted == 1u) return output[0];
    return (static_cast<unsigned int>(output[1])<<8u)|output[0];
}

static unsigned int Msvc6Case(
    unsigned int value, const RecordDBCtypeDescriptor* descriptor, bool upper)
{
    const unsigned int locale = descriptor?descriptor->localeLow:crtCtypeLocale;
    if (locale == 0u) return Msvc6AsciiCase(value, upper);

    volatile LONG* const readers = &crtLocaleReaders;
    InterlockedIncrement(readers);
    const bool locked = crtLocaleWriters != 0u;
    if (locked)
    {
        InterlockedDecrement(readers);
        CrtLock(crtLocaleLock);
    }
    const unsigned int result = Msvc6CaseWithLocale(value, descriptor, upper);
    if (locked) CrtUnlock(crtLocaleLock);
    else InterlockedDecrement(readers);
    return result;
}

unsigned int __cdecl Msvc6ToLower(unsigned int value, unsigned int descriptor)
{
    return Msvc6Case(value,
        reinterpret_cast<const RecordDBCtypeDescriptor*>(U32ToPtr(descriptor)), false);
}

unsigned int __cdecl Msvc6ToUpper(unsigned int value, unsigned int descriptor)
{
    return Msvc6Case(value,
        reinterpret_cast<const RecordDBCtypeDescriptor*>(U32ToPtr(descriptor)), true);
}

unsigned int __cdecl Msvc6CtypeDescriptor(unsigned int out32)
{
    volatile LONG* const readers = &crtLocaleReaders;
    InterlockedIncrement(readers);
    const bool locked = crtLocaleWriters != 0u;
    if (locked)
    {
        InterlockedDecrement(readers);
        CrtLock(crtLocaleLock);
    }
    RecordDBCtypeDescriptor descriptor;
    descriptor.localeLow = crtCollateLocale;
    descriptor.localeHigh = crtCtypeCodePage;
    const unsigned int copy = PtrToU32(CrtAllocate(512u));
    if (copy != 0u)
    {
        descriptor.table = reinterpret_cast<unsigned short*>(U32ToPtr(copy));
        memcpy(descriptor.table, crtCharacterTypes, 512u);
        descriptor.ownsTable = 1u;
    }
    else
    {
        descriptor.table = reinterpret_cast<unsigned short*>(crtCharacterTypes);
        descriptor.ownsTable = 0u;
    }
    if (locked) CrtUnlock(crtLocaleLock);
    else InterlockedDecrement(readers);
    *reinterpret_cast<RecordDBCtypeDescriptor*>(U32ToPtr(out32)) = descriptor;
    return out32;
}

unsigned long long __cdecl Msvc6LocaleStamp()
{
    volatile LONG* const readers = &crtLocaleReaders;
    InterlockedIncrement(readers);
    const bool locked = crtLocaleWriters != 0u;
    if (locked)
    {
        InterlockedDecrement(readers);
        CrtLock(crtLocaleLock);
    }
    const unsigned int locale = crtCtypeLocale;
    const unsigned int codePage = crtCtypeCodePage;
    if (locked) CrtUnlock(crtLocaleLock);
    else InterlockedDecrement(readers);
    return (static_cast<unsigned long long>(codePage)<<32u)|locale;
}

unsigned long long __cdecl Msvc6CollateStamp()
{
    volatile LONG* const readers = &crtLocaleReaders;
    InterlockedIncrement(readers);
    const bool locked = crtLocaleWriters != 0u;
    if (locked)
    {
        InterlockedDecrement(readers);
        CrtLock(crtLocaleLock);
    }
    const unsigned int locale = crtCollateLocale;
    const unsigned int codePage = crtCollateCodePage;
    if (locked) CrtUnlock(crtLocaleLock);
    else InterlockedDecrement(readers);
    return (static_cast<unsigned long long>(codePage)<<32u)|locale;
}

static int Msvc6MbrToWideWithLocale(unsigned short* destination, const char* source,
    unsigned int count, unsigned int* state, const RecordDBCtypeDescriptor* descriptor)
{
    if (source == 0 || count == 0u) return 0;
    const unsigned char byte = static_cast<unsigned char>(*source);
    if (byte == 0u)
    {
        if (destination) *destination = 0u;
        return 0;
    }
    const unsigned int locale = descriptor?descriptor->localeLow:crtCtypeLocale;
    const unsigned int codePage = descriptor?descriptor->localeHigh:crtCtypeCodePage;
    if (locale == 0u)
    {
        if (destination) *destination = byte;
        return 1;
    }
    if (*state != 0u)
    {
        reinterpret_cast<unsigned char*>(state)[1] = byte;
        if (crtMaximumMultibyteLength>1 &&
            MultiByteToWideChar(codePage, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
                reinterpret_cast<const char*>(state), 2,
                reinterpret_cast<wchar_t*>(destination), destination?1:0) != 0)
        {
            *state = 0u;
            return crtMaximumMultibyteLength;
        }
        *state = 0u;
        errno = EILSEQ;
        return -1;
    }
    if ((crtCharacterTypes[byte] & CtypeLeadByte) != 0u)
    {
        const unsigned int maximum = static_cast<unsigned int>(crtMaximumMultibyteLength);
        if (count<maximum)
        {
            *reinterpret_cast<unsigned char*>(state) = byte;
            return -2;
        }
        if (BitsToSigned32(maximum)>1 &&
            MultiByteToWideChar(codePage, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
                source, BitsToSigned32(maximum), reinterpret_cast<wchar_t*>(destination),
                destination?1:0) != 0)
            return crtMaximumMultibyteLength;
        if (source[1] != 0)
            return crtMaximumMultibyteLength;
        *state = 0u;
        errno = EILSEQ;
        return -1;
    }
    if (MultiByteToWideChar(codePage, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
            source, 1, reinterpret_cast<wchar_t*>(destination), destination?1:0) != 0)
        return 1;
    errno = EILSEQ;
    return -1;
}

int __cdecl Msvc6MbrToWide(unsigned short* destination, const char* source,
    unsigned int count, unsigned int* state, const RecordDBCtypeDescriptor* descriptor)
{
    volatile LONG* const readers = &crtLocaleReaders;
    InterlockedIncrement(readers);
    const bool locked = crtLocaleWriters != 0u;
    if (locked)
    {
        InterlockedDecrement(readers);
        CrtLock(crtLocaleLock);
    }
    const int result = Msvc6MbrToWideWithLocale(destination, source, count, state, descriptor);
    if (locked) CrtUnlock(crtLocaleLock);
    else InterlockedDecrement(readers);
    return result;
}

unsigned int __cdecl Msvc6ByteToWide(unsigned int value)
{
    if (value == 0xFFFFFFFFu) return value;
    const char input = static_cast<char>(value);
    unsigned short output;
    unsigned int state = 0u;
    const int result = Msvc6MbrToWide(&output, &input, 1u, &state, 0);
    return result<0?0xFFFFu:output;
}

static int CrtWideToMultibyteWithLocale(char* destination, unsigned short value,
    const RecordDBCtypeDescriptor* descriptor)
{
    const unsigned int locale = descriptor ? descriptor->localeLow : crtCtypeLocale;
    const unsigned int codePage = descriptor ? descriptor->localeHigh : crtCtypeCodePage;
    if (locale == 0)
    {
        if (value <= 255u)
        {
            *destination = static_cast<char>(value);
            return 1;
        }
    }
    else
    {
        BOOL usedDefaultCharacter = FALSE;
        const int count = WideCharToMultiByte(codePage, WC_COMPOSITECHECK | WC_SEPCHARS,
            reinterpret_cast<const wchar_t*>(&value), 1, destination,
            crtMaximumMultibyteLength, 0, &usedDefaultCharacter);
        if (count != 0 && !usedDefaultCharacter) return count;
    }
    errno = EILSEQ;
    return -1;
}

int Msvc6::WideToMultibyte(char* destination, unsigned short value,
    unsigned int* state, const RecordDBCtypeDescriptor* descriptor)
{
    (void)state;
    InterlockedIncrement(&crtLocaleReaders);
    const bool locked = crtLocaleWriters != 0u;
    if (locked)
    {
        InterlockedDecrement(&crtLocaleReaders);
        CrtLock(crtLocaleLock);
    }
    const int count = CrtWideToMultibyteWithLocale(destination, value, descriptor);
    if (locked) CrtUnlock(crtLocaleLock);
    else InterlockedDecrement(&crtLocaleReaders);
    return count;
}

int Msvc6::WideToByte(unsigned short value)
{
    if (value == 65535u) return -1;
    char bytes[2];
    if (WideToMultibyte(bytes, value, 0, 0) != 1) return -1;
    return static_cast<signed char>(bytes[0]);
}




namespace {

unsigned int crtTimeAlternateFormat = 0;
unsigned int crtTimeSuppressLeadingZeroes = 0;

static void CrtStoreTimeText(const char* text, char*& output, unsigned int& remaining)
{
    while (remaining != 0 && *text != 0)
    {
        *output++ = *text++;
        --remaining;
    }
}

static void CrtStoreUnpaddedTimeNumber(int value, char*& output, unsigned int& remaining)
{
    char* cursor = output;
    while (remaining > 1u)
    {
        *cursor++ = static_cast<char>('0' + value % 10);
        --remaining;
        value /= 10;
        if (value <= 0) break;
    }
    char* first = output;
    output = cursor;
    char* last = cursor - 1;
    // The native helper also performs the first exchange for an empty span.
    do
    {
        const char value = *first;
        *first++ = *last;
        *last-- = value;
    } while (first < last);
}

static void CrtStoreTimeNumber(int value, unsigned int digits,
    char*& output, unsigned int& remaining)
{
    if (crtTimeSuppressLeadingZeroes != 0)
    {
        CrtStoreUnpaddedTimeNumber(value, output, remaining);
        return;
    }
    if (digits >= remaining)
    {
        remaining = 0;
        return;
    }
    for (unsigned int position = digits; position != 0;)
    {
        output[--position] = static_cast<char>('0' + value % 10);
        value /= 10;
    }
    output += digits;
    remaining -= digits;
}

static void CrtExpandTimeField(char field, const tm* value,
    char*& output, unsigned int& remaining, const CrtTimeNames* names);
static void CrtFormatWindowsTime(const char* format, const tm* value,
    char*& output, unsigned int& remaining, const CrtTimeNames* names);

static const char* CrtTimeZoneName(const tm* value)
{
#if defined(_MSC_VER)
    _tzset();
    return _tzname[value->tm_isdst != 0 ? 1 : 0];
#else
    tzset();
    return tzname[value->tm_isdst != 0 ? 1 : 0];
#endif
}

static void CrtExpandTimeField(char field, const tm* value,
    char*& output, unsigned int& remaining, const CrtTimeNames* names)
{
    int number;
    unsigned int digits;
    switch (field)
    {
    case '%':
        *output++ = '%';
        --remaining;
        return;
    case 'a': CrtStoreTimeText(names->abbreviatedDays[value->tm_wday], output, remaining); return;
    case 'A': CrtStoreTimeText(names->days[value->tm_wday], output, remaining); return;
    case 'b': CrtStoreTimeText(names->abbreviatedMonths[value->tm_mon], output, remaining); return;
    case 'B': CrtStoreTimeText(names->months[value->tm_mon], output, remaining); return;
    case 'p': CrtStoreTimeText(value->tm_hour > 11 ? names->pm : names->am, output, remaining); return;
    case 'z':
    case 'Z': CrtStoreTimeText(CrtTimeZoneName(value), output, remaining); return;
    case 'c':
    {
        const bool alternate = crtTimeAlternateFormat != 0;
        if (alternate) crtTimeAlternateFormat = 0;
        CrtFormatWindowsTime(alternate ? names->longDate : names->shortDate,
            value, output, remaining, names);
        if (remaining != 0)
        {
            *output++ = ' ';
            --remaining;
            CrtFormatWindowsTime(names->time, value, output, remaining, names);
        }
        return;
    }
    case 'x':
    {
        const bool alternate = crtTimeAlternateFormat != 0;
        if (alternate) crtTimeAlternateFormat = 0;
        CrtFormatWindowsTime(alternate ? names->longDate : names->shortDate,
            value, output, remaining, names);
        return;
    }
    case 'X':
        crtTimeAlternateFormat = 0;
        CrtFormatWindowsTime(names->time, value, output, remaining, names);
        return;
    case 'd': number = value->tm_mday; digits = 2; break;
    case 'H': number = value->tm_hour; digits = 2; break;
    case 'I': number = value->tm_hour % 12; if (number == 0) number = 12; digits = 2; break;
    case 'j': number = value->tm_yday + 1; digits = 3; break;
    case 'm': number = value->tm_mon + 1; digits = 2; break;
    case 'M': number = value->tm_min; digits = 2; break;
    case 'S': number = value->tm_sec; digits = 2; break;
    case 'w': number = value->tm_wday; digits = 1; break;
    case 'y': number = value->tm_year % 100; digits = 2; break;
    case 'Y': number = BitsToSigned32(static_cast<unsigned int>(value->tm_year) + 1900u); digits = 4; break;
    case 'U':
    case 'W':
    {
        const int weekday = field == 'U' ? value->tm_wday :
            (value->tm_wday == 0 ? 6 : value->tm_wday - 1);
        number = value->tm_yday < weekday ? 0 :
            value->tm_yday / 7 + (value->tm_yday % 7 >= weekday ? 1 : 0);
        digits = 2;
        break;
    }
    default: return;
    }
    crtTimeSuppressLeadingZeroes = crtTimeAlternateFormat;
    CrtStoreTimeNumber(number, digits, output, remaining);
}

static bool CrtTimeFormatEquals(const char* format, const char* expected)
{
    for (;;)
    {
        const unsigned int left = static_cast<unsigned char>(*format++);
        const unsigned int right = static_cast<unsigned char>(*expected++);
        if (Msvc6ToLower(left, 0) != Msvc6ToLower(right, 0)) return false;
        if (left == 0) return true;
    }
}

static void CrtFormatWindowsTime(const char* format, const tm* value,
    char*& output, unsigned int& remaining, const CrtTimeNames* names)
{
    while (*format != 0 && remaining != 0)
    {
        crtTimeSuppressLeadingZeroes = 0;
        const char token = *format;
        const char* const followingCharacter = format + 1;
        const char* followingGroup = followingCharacter;
        int repetitions = 1;
        while (*followingGroup == token) { ++followingGroup; ++repetitions; }
        char field = 0;
        switch (token)
        {
        case 'M':
            if (repetitions == 1) { crtTimeSuppressLeadingZeroes = 1; field = 'm'; }
            else if (repetitions == 2) field = 'm';
            else if (repetitions == 3) field = 'b';
            else if (repetitions == 4) field = 'B';
            break;
        case 'd':
            if (repetitions == 1) { crtTimeSuppressLeadingZeroes = 1; field = 'd'; }
            else if (repetitions == 2) field = 'd';
            else if (repetitions == 3) field = 'a';
            else if (repetitions == 4) field = 'A';
            break;
        case 'H':
        case 'h':
        case 'm':
        case 's':
            if (repetitions <= 2)
            {
                if (repetitions == 1) crtTimeSuppressLeadingZeroes = 1;
                field = token == 'h' ? 'I' : token == 'm' ? 'M' : token == 's' ? 'S' : 'H';
            }
            break;
        case 'y':
            if (repetitions == 2) field = 'y';
            else if (repetitions == 4) field = 'Y';
            break;
        case 'A':
        case 'a':
            if (CrtTimeFormatEquals(format, "am/pm")) followingGroup = format + 5;
            else if (CrtTimeFormatEquals(format, "a/p")) followingGroup = format + 3;
            field = 'p';
            break;
        case 't':
        {
            const char* text = value->tm_hour > 11 ? names->pm : names->am;
            while (repetitions > 0 && remaining != 0)
            {
                if ((crtCharacterTypes[static_cast<unsigned char>(*text)] & CtypeLeadByte) != 0 && remaining > 1u)
                {
                    *output++ = *text++;
                    --remaining;
                }
                *output++ = *text++;
                --remaining;
                --repetitions;
            }
            format = followingGroup;
            continue;
        }
        case '\'':
            format += repetitions;
            if ((repetitions & 1) != 0)
            {
                while (*format != 0 && remaining != 0)
                {
                    if (*format == '\'') { ++format; break; }
                    if ((crtCharacterTypes[static_cast<unsigned char>(*format)] & CtypeLeadByte) != 0 && remaining > 1u)
                    {
                        *output++ = *format++;
                        --remaining;
                    }
                    *output++ = *format++;
                    --remaining;
                }
            }
            continue;
        }
        if (field != 0)
        {
            CrtExpandTimeField(field, value, output, remaining, names);
            format = followingGroup;
        }
        else
        {
            if ((crtCharacterTypes[static_cast<unsigned char>(token)] & CtypeLeadByte) != 0)
            {
                *output++ = token;
                --remaining;
                format = followingCharacter;
            }
            *output++ = *format++;
            --remaining;
        }
    }
}

}

unsigned int Msvc6::FormatTime(char* destination, unsigned int capacity,
    const char* format, const tm* value, const CrtTimeNames* names)
{
    unsigned int remaining = capacity;
    InterlockedIncrement(&crtLocaleReaders);
    const bool locked = crtLocaleWriters != 0;
    if (locked)
    {
        InterlockedDecrement(&crtLocaleReaders);
        CrtLock(crtLocaleLock);
    }
    if (names == 0) names = crtCurrentTimeNames;
    char* output = destination;
    while (remaining != 0 && *format != 0)
    {
        if (*format != '%')
        {
            if ((crtCharacterTypes[static_cast<unsigned char>(*format)] & CtypeLeadByte) != 0 && remaining > 1u)
            {
                *output++ = *format++;
                --remaining;
            }
            *output++ = *format++;
            --remaining;
        }
        else
        {
            crtTimeAlternateFormat = 0;
            if (*++format == '#') { crtTimeAlternateFormat = 1; ++format; }
            CrtExpandTimeField(*format++, value, output, remaining, names);
        }
    }
    if (locked) CrtUnlock(crtLocaleLock);
    else InterlockedDecrement(&crtLocaleReaders);
    if (remaining == 0) return 0;
    *output = 0;
    return capacity - remaining;
}

static double CrtParseDouble(const char* source, char** end);

#if defined(_MSC_VER) && defined(_M_IX86)
#pragma float_control(precise, on, push)
#pragma fenv_access(on)
#endif
double Msvc6::ParseFloating(const char* source, char** end, int decimalExponent)
{
    const double parsed = CrtParseDouble(source, end);
    if (decimalExponent == 0 || parsed == 0.0 || !std::isfinite(parsed))
        return parsed;

    // Keep the original sequence of decimal multiplications and its active
    // precision. A separate binary exponent defers double range rounding
    // until the result, including on compilers whose long double is double.
    int exponent;
    double fraction = std::frexp(parsed, &exponent);
    long long binaryExponent = exponent;
    // Read the native binary64 factors as stored values in every round mode.
    static const volatile double ten = 10.0;
    static const volatile double tenth = 0.1;
    const double factor = decimalExponent > 0 ? ten : tenth;
    for (int remaining = decimalExponent; remaining != 0;
        remaining += remaining > 0 ? -1 : 1)
    {
        fraction = std::frexp(fraction * factor, &exponent);
        binaryExponent += exponent;
    }

    // Keep library scaling inside the normal range. The final C++ multiply
    // and double store perform the native range rounding in this x87 unit.
    const int savedError = errno;
    const int minimumExponent = std::numeric_limits<double>::min_exponent;
    const int maximumExponent = std::numeric_limits<double>::max_exponent - 1;
    double rangeFactor = 1.0;
    if (binaryExponent < minimumExponent)
    {
        const long long remaining = binaryExponent - minimumExponent;
        const int minimumShift = -std::numeric_limits<double>::digits - 1;
        const int shift = remaining < minimumShift
            ? minimumShift : static_cast<int>(remaining);
        rangeFactor = std::ldexp(1.0, shift);
        binaryExponent = minimumExponent;
    }
    else if (binaryExponent > maximumExponent)
    {
        rangeFactor = binaryExponent == maximumExponent + 1 ? 2.0 : 4.0;
        binaryExponent = maximumExponent;
    }
    const double normal = std::ldexp(fraction, static_cast<int>(binaryExponent));
    // The native decimal scaling keeps the parser's error status unchanged.
    errno = savedError;
    volatile double result = normal * rangeFactor;
    return result;
}
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma fenv_access(off)
#pragma float_control(pop)
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
#pragma float_control(precise, on, push)
#pragma fenv_access(on)
#endif

void Msvc6::PrepareMoneyAmount(double& amount, bool& negative, unsigned int& extraZeros)
{
    // The native comparisons include unordered values in both branches.
    negative = !(amount >= 0.0);
    if (negative) amount = -amount;
    extraZeros = 0;
    while (!(amount < 1e35) && extraZeros < 5000u)
    {
        // Keep each double store and the active floating-point precision.
        volatile double reduced = amount * 1e-10;
        amount = reduced;
        extraZeros += 10u;
    }
}

#if defined(_MSC_VER) && defined(_M_IX86)
#pragma fenv_access(off)
#pragma float_control(pop)
#endif


static unsigned int Msvc6JoinTimeNames(const char* const* abbreviated,
    const char* const* full, unsigned int count)
{
    unsigned int size = 0u;
    for (unsigned int index = 0u;index<count;++index)
    {
        const unsigned int fullLength = static_cast<unsigned int>(std::strlen(full[index]));
        const unsigned int shortLength = static_cast<unsigned int>(std::strlen(abbreviated[index]));
        size+=shortLength+fullLength+2u;
    }
    char* const result = static_cast<char*>(CrtAllocate(size+1u));
    if (result)
    {
        char* cursor = result;
        for (unsigned int index = 0u;index<count;++index)
        {
            *cursor++ = ':';
            cursor+=std::strlen(std::strcpy(cursor, abbreviated[index]));
            *cursor++ = ':';
            cursor+=std::strlen(std::strcpy(cursor, full[index]));
        }
        *cursor = 0;
    }
    return PtrToU32(result);
}

unsigned int __cdecl Msvc6GetDays()
{
    const CrtTimeNames* const names = crtCurrentTimeNames;
    return Msvc6JoinTimeNames(names->abbreviatedDays, names->days, 7u);
}

unsigned int __cdecl Msvc6GetMonths()
{
    const CrtTimeNames* const names = crtCurrentTimeNames;
    return Msvc6JoinTimeNames(names->abbreviatedMonths, names->months, 12u);
}

unsigned int __cdecl Msvc6GetTimeNames()
{
    const CrtTimeNames* const source = crtCurrentTimeNames;
    unsigned int size = 0u;
    for (unsigned int index = 0u;index<7u;++index)
    {
        const unsigned int fullLength = static_cast<unsigned int>(std::strlen(source->days[index]));
        const unsigned int shortLength = static_cast<unsigned int>(std::strlen(source->abbreviatedDays[index]));
        size+=shortLength+fullLength+2u;
    }
    for (unsigned int index = 0u;index<12u;++index)
    {
        const unsigned int fullLength = static_cast<unsigned int>(std::strlen(source->months[index]));
        const unsigned int shortLength = static_cast<unsigned int>(std::strlen(source->abbreviatedMonths[index]));
        size+=shortLength+fullLength+2u;
    }
    size+=static_cast<unsigned int>(std::strlen(source->am))+1u;
    size+=static_cast<unsigned int>(std::strlen(source->pm))+1u;
    size+=static_cast<unsigned int>(std::strlen(source->shortDate))+1u;
    size+=static_cast<unsigned int>(std::strlen(source->longDate))+1u;
    size+=static_cast<unsigned int>(std::strlen(source->time))+1u;
    CrtTimeNames* const result = reinterpret_cast<CrtTimeNames*>(
        CrtAllocate(size+sizeof(CrtTimeNames)));
    if (result == 0) return 0u;
    char* cursor = reinterpret_cast<char*>(result+1);
    memcpy(result, crtCurrentTimeNames, sizeof(CrtTimeNames));
    for (unsigned int index = 0u;index<7u;++index)
    {
        result->abbreviatedDays[index] = cursor;
        cursor+=std::strlen(std::strcpy(cursor, source->abbreviatedDays[index]))+1u;
        result->days[index] = cursor;
        cursor+=std::strlen(std::strcpy(cursor, source->days[index]))+1u;
    }
    for (unsigned int index = 0u;index<12u;++index)
    {
        result->abbreviatedMonths[index] = cursor;
        cursor+=std::strlen(std::strcpy(cursor, source->abbreviatedMonths[index]))+1u;
        result->months[index] = cursor;
        cursor+=std::strlen(std::strcpy(cursor, source->months[index]))+1u;
    }
    result->am = cursor;
    cursor+=std::strlen(std::strcpy(cursor, source->am))+1u;
    result->pm = cursor;
    cursor+=std::strlen(std::strcpy(cursor, source->pm))+1u;
    result->shortDate = cursor;
    cursor+=std::strlen(std::strcpy(cursor, source->shortDate))+1u;
    result->longDate = cursor;
    cursor+=std::strlen(std::strcpy(cursor, source->longDate))+1u;
    result->time = cursor;
    return PtrToU32(result);
}


unsigned int __cdecl CrtIsAlpha(unsigned int ch)
{
    if (crtMaximumMultibyteLength>1) return CrtIsCtype(ch, CtypeAlphabetic | CtypeLower | CtypeUpper);
    return crtCharacterTypes[BitsToSigned32(ch)] & (CtypeAlphabetic | CtypeLower | CtypeUpper);
}

unsigned int __cdecl CrtIsDigit(unsigned int ch)
{
    if (crtMaximumMultibyteLength>1) return CrtIsCtype(ch, CtypeDigit);
    return crtCharacterTypes[BitsToSigned32(ch)] & CtypeDigit;
}

unsigned int __cdecl CrtIsSpace(unsigned int ch)
{
    if (crtMaximumMultibyteLength>1) return CrtIsCtype(ch, CtypeSpace);
    return crtCharacterTypes[BitsToSigned32(ch)] & CtypeSpace;
}

unsigned int __cdecl CrtIsAlnum(unsigned int ch)
{
    if (crtMaximumMultibyteLength>1) return CrtIsCtype(ch, CtypeAlphabetic | CtypeLower | CtypeUpper | CtypeDigit);
    return crtCharacterTypes[BitsToSigned32(ch)] & (CtypeAlphabetic | CtypeLower | CtypeUpper | CtypeDigit);
}

unsigned int  Msvc6BasicIosDestruct(void* self)
{
    RecordDBIosControl& storage = *static_cast<RecordDBIosControl*>(self);
    storage.vftable = Msvc6::IosBaseTable();
    bool cleanup = storage.registration == 0u;
    if (storage.registration>0u)
    {
        const unsigned char count = static_cast<unsigned char>(
            Msvc6::StreamRegistrations[storage.registration] - 1u);
        Msvc6::StreamRegistrations[storage.registration] = count;
        cleanup = static_cast<signed char>(
            Msvc6::StreamRegistrations[storage.registration]) <= 0;
    }
    if (cleanup)
    {
        RecordDBIosCallbackNode* callback = reinterpret_cast<RecordDBIosCallbackNode*>(U32ToPtr(storage.reserved1C));
        while (callback)
        {
            (void)reinterpret_cast<IosEventCallbackFunction>(AddressToUIntPtr(callback->callback))(0u, PtrToU32(self), static_cast<unsigned int>(callback->index));
            callback = callback->next;
        }
        RecordDBIosStorageNode* node = reinterpret_cast<RecordDBIosStorageNode*>(U32ToPtr(storage.reserved18));
        while (node)
        {
            RecordDBIosStorageNode* next = node->next;
            CrtFreePointer(node);
            node = next;
        }
        storage.reserved18 = 0u;
        callback = reinterpret_cast<RecordDBIosCallbackNode*>(U32ToPtr(storage.reserved1C));
        while (callback)
        {
            RecordDBIosCallbackNode* next = callback->next;
            CrtFreePointer(callback);
            callback = next;
        }
        storage.reserved1C = 0u;
    }
    Msvc6LocaleFacetDecref(&storage.locale);
    return PtrToU32(self);
}

unsigned int __cdecl Msvc6GetPos(unsigned int file, unsigned int out32)
{
    std::FILE* const native = reinterpret_cast<std::FILE*>(U32ToPtr(file));
    const __int64 position = _ftelli64(native);
    WriteGlobalU32(out32, static_cast<unsigned int>(position));
    WriteGlobalU32(out32+4u, static_cast<unsigned int>(static_cast<unsigned long long>(position)>>32u));
    return position == static_cast<__int64>(-1)?0xFFFFFFFFu:0u;
}

unsigned int __cdecl Msvc6SetPos(unsigned int file, unsigned int pos32)
{
    std::FILE* const native = reinterpret_cast<std::FILE*>(U32ToPtr(file));
    const unsigned long long lo = ReadGlobalU32(pos32);
    const unsigned long long hi = ReadGlobalU32(pos32+4u);
    return static_cast<unsigned int>(_fseeki64(native, static_cast<__int64>((hi<<32u)|lo), SEEK_SET));
}

unsigned int __cdecl Msvc6SetVbuf(unsigned int file, unsigned int buffer, unsigned int mode, unsigned int size)
{
    std::FILE* const native = reinterpret_cast<std::FILE*>(U32ToPtr(file));
    if (mode != 4u && (size<2u || size>0x7FFFFFFFu || (mode != 0u && mode != 0x40u)))
        return 0xFFFFFFFFu;
    const int buffering = mode == 4u?_IONBF:(mode == 0x40u?_IOLBF:_IOFBF);
    const unsigned int bufferSize = mode == 4u?2u:(size&0xFFFFFFFEu);
    return static_cast<unsigned int>(std::setvbuf(native, reinterpret_cast<char*>(U32ToPtr(buffer)), buffering, bufferSize));
}

const CrtLocaleConvention* Msvc6::GetLocaleConvention()
{
    return crtCurrentLocaleConvention;
}

int Msvc6::MaximumMultibyteLength()
{
    return crtMaximumMultibyteLength;
}


static int RegisterCrtExitDrain()
{
    static const int result = std::atexit(&RunCrtAtExitCallbacks);
    return result;
}

int __cdecl RegisterCrtAtExitCallback(CrtExitCallback callback)
{
    CrtLock(crtExitLock);
    const unsigned int capacity = static_cast<unsigned int>(_msize(crtExitCallbacksBegin));
    CrtExitCallback* const begin = crtExitCallbacksBegin;
    CrtExitCallback* end = crtExitCallbacksEnd;
    const unsigned int usedBytes = PtrToU32(end) - PtrToU32(begin);
    if (capacity < usedBytes + sizeof(CrtExitCallback))
    {
        const unsigned int grownCapacity = static_cast<unsigned int>(_msize(begin)) + 16u;
        CrtExitCallback* const grown = static_cast<CrtExitCallback*>(
            std::realloc(crtExitCallbacksBegin, grownCapacity));
        if (grown == 0)
        {
            CrtUnlock(crtExitLock);
            return -1;
        }
        const unsigned int used = PtrToU32(crtExitCallbacksEnd) - PtrToU32(crtExitCallbacksBegin);
        crtExitCallbacksBegin = grown;
        end = grown + used / sizeof(CrtExitCallback);
        crtExitCallbacksEnd = end;
    }
    *end = callback;
    ++crtExitCallbacksEnd;
    CrtUnlock(crtExitLock);
    return callback ? 0 : -1;
}


static void RunCrtAtExitCallbacks()
{
    CrtLock(crtExitLock);
    crtExitProcedureEntered = 1u;
    crtExitReturnToCaller = 0u;
    CrtExitCallback* const begin = crtExitCallbacksBegin;
    if (begin != 0)
    {
        CrtExitCallback* current = crtExitCallbacksEnd;
        while (PtrToU32(current) > PtrToU32(crtExitCallbacksBegin))
        {
            --current;
            const CrtExitCallback callback = *current;
            if (callback) callback();
        }
    }
    CrtUnlock(crtExitLock);
}

static volatile LONG* CrtStaticLockState()
{
    return &stlStaticLockState;
}
static CRITICAL_SECTION* CrtStaticLockCs()
{
    return &stlStaticLock;
}


void __cdecl CrtStaticLockCleanup()
{
    volatile LONG* const state = CrtStaticLockState();
    const LONG previous = InterlockedExchange(state, 3);
    if (previous == 2)DeleteCriticalSection(CrtStaticLockCs());
}

unsigned int __cdecl CrtStaticLockConstruct(unsigned int self)
{
    volatile LONG* const state = CrtStaticLockState();
    CRITICAL_SECTION* const cs = CrtStaticLockCs();
    const LONG current = *state;
    if (current == 2)
    {
        EnterCriticalSection(cs);
        return self;
    }
    if (current == 0)
    {
        const LONG previous = InterlockedExchange(state, 1);
        if (previous == 0)
        {
            InitializeCriticalSection(cs);
            (void)RegisterCrtAtExitCallback(&CrtStaticLockCleanup);
            *state = 2;
        }
        else if (previous == 2)
        {

            *state = 2;
        }
    }
    while (*state == 1)Sleep(1);
    if (*state == 2)EnterCriticalSection(cs);
    return self;
}


unsigned int __cdecl CrtStaticLockDestruct(unsigned int self)
{
    (void)self;
    if (*CrtStaticLockState() == 2)LeaveCriticalSection(CrtStaticLockCs());
    return 0u;
}
#if defined(_MSC_VER)
void Msvc6::ThrowException(void* object, const NativeThrowInfo* information)
{
    _CxxThrowException(object,
        reinterpret_cast<_ThrowInfo*>(const_cast<NativeThrowInfo*>(information)));
}
#endif


static volatile LONG crtStaticInitializationState=0;


static void SetStaticInitializationGuard(unsigned char& guard)
{
    if ((guard&1u) == 0u) guard = static_cast<unsigned char>(guard|1u);
}

static unsigned int __cdecl SharedNoopStaticInitializer()
{


    SetStaticInitializationGuard(CrtStaticInitializationGuards[0]);
    return static_cast<unsigned int>(RegisterCrtAtExitCallback(&MapStaticCleanupNoOp));
}

static unsigned int __cdecl InitializeInputMappingDefaults()
{
    for (unsigned int action = 0; action < zInputMap::ACTION_COUNT; ++action)
    {
        for (unsigned int slot = 0; slot < zInputMap::MAX_MAPPING_COUNT; ++slot)
        {
            InpAction& mapping = zInputMap::m_aiMappings[action][slot];
            mapping.inputMode = IM_NONE;
            mapping.inputValue = 0;
        }
    }
    return 0;
}

static unsigned int __cdecl InitializeInputClickPosition()
{
    INPUT::lClickMouseCoor = VECTOR2(0.0f, 0.0f);
    return 0;
}

static unsigned int __cdecl InitializeSharedCleanupGuardPair()
{
    SetStaticInitializationGuard(CrtStaticInitializationGuards[0]);
    const int a = RegisterCrtAtExitCallback(&MapStaticCleanupNoOp);
    SetStaticInitializationGuard(CrtStaticInitializationGuards[3]);
    const int b = RegisterCrtAtExitCallback(&MapStaticCleanupNoOp);
    return static_cast<unsigned int>(a?a:b);
}

static unsigned int __cdecl InitializeSharedCleanupGuardGroup()
{
    int rc = 0;
    for (unsigned int i = 0u;i<sizeof(CrtStaticInitializationGuards)/sizeof(CrtStaticInitializationGuards[0]);++i)
    {
        SetStaticInitializationGuard(CrtStaticInitializationGuards[i]);
        const int x = RegisterCrtAtExitCallback(&MapStaticCleanupNoOp);
        if (!rc)rc = x;
    }
    return static_cast<unsigned int>(rc);
}

static void __cdecl _cropzeros(char* text)
{
    const char decimal = crtDecimalPoint;
    char* fractionBegin = text;
    while (*fractionBegin && *fractionBegin != decimal)++fractionBegin;
    if (!*fractionBegin)return;
    ++fractionBegin;
    char* exponentBegin = fractionBegin;
    while (*exponentBegin && *exponentBegin != 'e' && *exponentBegin != 'E')++exponentBegin;
    char* trimEnd = exponentBegin;
    while (trimEnd[-1] == '0')--trimEnd;
    if (trimEnd[-1] == decimal)--trimEnd;
    while ((*trimEnd++ = *exponentBegin++) != 0)
    {
    }
}

static int __cdecl CrtToLowerWithLocale(int ch)
{
    if (crtCtypeLocale == 0u)
    {
        if (ch >= 'A' && ch <= 'Z')return ch + ('a' - 'A');
        return ch;
    }
    if (ch < 256)
    {
        unsigned int upper;
        if (crtMaximumMultibyteLength > 1)
            upper = CrtIsCtype(static_cast<unsigned int>(ch), 1u);
        else
        {
            upper = crtCharacterTypes[ch] & CtypeUpper;
        }
        if (upper == 0u)return ch;
    }
    char input[3];
    const unsigned int lead = (static_cast<unsigned int>(ch) >> 8u) & 0xFFu;
    int count;
    if ((crtCharacterTypes[lead] & CtypeLeadByte) != 0u)
    {
        input[0] = static_cast<char>(lead);
        input[1] = static_cast<char>(ch);
        input[2] = 0;
        count = 2;
    }
    else
    {
        input[0] = static_cast<char>(ch);
        input[1] = 0;
        count = 1;
    }
    unsigned char output[3];
    const unsigned int size = CrtLCMapStringA(crtCtypeLocale,
        LCMAP_LOWERCASE, PtrToU32(input), count, PtrToU32(output), 3, 0u, 1u);
    if (size == 0u)return ch;
    if (size == 1u)return output[0];
    return (static_cast<int>(output[1]) << 8) | output[0];
}

static int __cdecl CrtToLower(int ch)
{
    if (crtCtypeLocale == 0u)
    {
        if (ch >= 'A' && ch <= 'Z')return ch + ('a' - 'A');
        return ch;
    }
    volatile LONG* const localeGate =
        &crtLocaleReaders;
    InterlockedIncrement(localeGate);
    const bool separateLocaleLock = crtLocaleWriters != 0u;
    if (separateLocaleLock)
    {
        InterlockedDecrement(localeGate);
        CrtLock(crtLocaleLock);
    }
    const int result = CrtToLowerWithLocale(ch);
    if (separateLocaleLock)
        CrtUnlock(crtLocaleLock);
    else
        InterlockedDecrement(localeGate);
    return result;
}

static void __cdecl _forcdecpt(char* text)
{
    char* insertion = text;
    if (CrtToLower(static_cast<int>(static_cast<signed char>(*insertion))) != 'e')
    {
        do
        {
            ++insertion;
        }
        while (CrtIsDigit(static_cast<unsigned int>(
            static_cast<int>(static_cast<signed char>(*insertion)))) != 0u);
    }
    const char decimal = crtDecimalPoint;
    char displaced = *insertion;
    *insertion++ = decimal;
    char copied;
    do
    {
        const char next = *insertion;
        copied = displaced;
        *insertion++ = copied;
        displaced = next;
    }
    while (copied != 0);
}

static int __cdecl _positive(const double* value)
{
    volatile double number = *value;
    return number >= 0.0 ? 1 : 0;
}

union CrtLongDouble12
{
    unsigned int limbs[3];
    unsigned short words[6];
};

// Native 96-bit software floating-point format, not host long double.
static const CrtLongDouble12 _pow10pos[29] = {
    {{0x00000000u, 0x00000000u, 0x4002A000u}},
    {{0x00000000u, 0x00000000u, 0x4005C800u}},
    {{0x00000000u, 0x00000000u, 0x4008FA00u}},
    {{0x00000000u, 0x00000000u, 0x400C9C40u}},
    {{0x00000000u, 0x00000000u, 0x400FC350u}},
    {{0x00000000u, 0x00000000u, 0x4012F424u}},
    {{0x00000000u, 0x80000000u, 0x40169896u}},
    {{0x00000000u, 0x20000000u, 0x4019BEBCu}},
    {{0x00000000u, 0xC9BF0400u, 0x40348E1Bu}},
    {{0xA1000000u, 0x1BCECCEDu, 0x404ED3C2u}},
    {{0xB59EF020u, 0xADA82B70u, 0x40699DC5u}},
    {{0x25FD5DD0u, 0x4F8E1AE5u, 0x4083EB19u}},
    {{0x95D79671u, 0x8D050E43u, 0x409EAF29u}},
    {{0x44A0BFF9u, 0x8F1281EDu, 0x40B98281u}},
    {{0xA6D53CBFu, 0x1F49FFCFu, 0x40D3C278u}},
    {{0x8CE0C66Fu, 0x47C980E9u, 0x41A893BAu}},
    {{0x556B85BCu, 0xF78D3927u, 0x427CE070u}},
    {{0xDE8EDDBCu, 0xEBFB9DF9u, 0x4351AA7Eu}},
    {{0xE376E6A1u, 0x2F29F2CCu, 0x44268184u}},
    {{0xAA171028u, 0xE310AEF8u, 0x44FAC4C5u}},
    {{0xF3D4A7EBu, 0x4AE1EBF7u, 0x45CF957Au}},
    {{0x91C7CC65u, 0xA0AEA60Eu, 0x46A3E319u}},
    {{0x0C17650Du, 0x75868175u, 0x4D48C976u}},
    {{0xA7E44258u, 0x353B3993u, 0x53EDB2B8u}},
    {{0x5DE5A74Du, 0x3B5DC53Du, 0x5A929E8Bu}},
    {{0xF0A65DFFu, 0x54C020A1u, 0x61378CA5u}},
    {{0x5A8BFDD1u, 0x5D25D88Bu, 0x67DBF989u}},
    {{0xF3F895AAu, 0xC8A2BF27u, 0x6E80DD5Du}},
    {{0x979BC94Cu, 0x52028A20u, 0x7525C460u}},
};

static const CrtLongDouble12 _pow10neg[29] = {
    {{0xCCCDCCCDu, 0xCCCCCCCCu, 0x3FFBCCCCu}},
    {{0xD70A3D71u, 0x0A3D70A3u, 0x3FF8A3D7u}},
    {{0xDF3B645Au, 0x6E978D4Fu, 0x3FF58312u}},
    {{0x652CD3C3u, 0x1758E219u, 0x3FF1D1B7u}},
    {{0x84230FD0u, 0xAC471B47u, 0x3FEEA7C5u}},
    {{0x69B6A640u, 0xBD05AF6Cu, 0x3FEB8637u}},
    {{0x42BC3D33u, 0x94D5E57Au, 0x3FE7D6BFu}},
    {{0xCEFDFDC2u, 0x77118461u, 0x3FE4ABCCu}},
    {{0xE15B4C2Fu, 0x94BEC44Du, 0x3FC9E695u}},
    {{0x3B53C492u, 0x14CD4475u, 0x3FAF9ABEu}},
    {{0x94BA67DEu, 0x1EAD4539u, 0x3F94CFB1u}},
    {{0xE2C62324u, 0x313BBABCu, 0x3F7A8B61u}},
    {{0xC1595561u, 0x7C53B17Eu, 0x3F5FBB12u}},
    {{0x8D2FEED7u, 0x8592BE06u, 0x3F44FB15u}},
    {{0xE9A53F24u, 0xEA27A539u, 0x3F2AA87Fu}},
    {{0xE4A1AC7Du, 0x467C64BCu, 0x3E55DDD0u}},
    {{0xCC067B63u, 0x83775423u, 0x3D8191FFu}},
    {{0x193AFA91u, 0x4325637Au, 0x3CACC031u}},
    {{0x38D18921u, 0xB8974782u, 0x3BD7FD00u}},
    {{0x085888DCu, 0xE3E8B11Bu, 0x3B03A686u}},
    {{0x424584C6u, 0x7599B607u, 0x3A2EDB37u}},
    {{0xD21C7133u, 0xEE32DB23u, 0x395A9049u}},
    {{0xC0BE87A6u, 0x82A5DA57u, 0x32B5A2A6u}},
    {{0x11B268E2u, 0x449F52A7u, 0x2C10B759u}},
    {{0x2DE44925u, 0x534F3436u, 0x256BCEAEu}},
    {{0xA404598Fu, 0x7DC2DEC0u, 0x1EC6E8FBu}},
    {{0x5A88E79Eu, 0xBF3C9157u, 0x18228350u}},
    {{0x62654B4Eu, 0xAF8F83FDu, 0x117D9406u}},
    {{0x9FDE2DE4u, 0x04C8D2CEu, 0x0AD8A6DDu}},
};


#pragma pack(push, 2)
struct CrtLongDouble10
{
    unsigned int low;
    unsigned int high;
    unsigned short exponent;
};

struct CrtDecimalOutput
{
    short decpt;
    char sign;
    char length;
    char digits[22];
};
#pragma pack(pop)

struct CrtFloatOutput
{
    int sign;
    int decpt;
    int flag;
    char* mantissa;
};


static int __addl(unsigned int first, unsigned int second, unsigned int* result)
{
    const unsigned int sum = first + second;
    const int carry = sum < first || sum < second;
    *result = sum;
    return carry;
}

static void __add_12(CrtLongDouble12* value, const CrtLongDouble12* addend)
{
    if (__addl(value->limbs[0], addend->limbs[0], &value->limbs[0]))
        if (__addl(value->limbs[1], 1u, &value->limbs[1]))
            ++value->limbs[2];
    if (__addl(value->limbs[1], addend->limbs[1], &value->limbs[1]))
        ++value->limbs[2];
    __addl(value->limbs[2], addend->limbs[2], &value->limbs[2]);
}

static void __shl_12(CrtLongDouble12* value)
{
    const unsigned int low = value->limbs[0];
    const unsigned int middle = value->limbs[1];
    value->limbs[0] = low << 1;
    value->limbs[1] = (middle << 1) | (low >> 31);
    value->limbs[2] = (value->limbs[2] << 1) | (middle >> 31);
}

static void __shr_12(CrtLongDouble12* value)
{
    const unsigned int high = value->limbs[2];
    const unsigned int middle = value->limbs[1];
    value->limbs[1] = (middle >> 1) | (high << 31);
    const unsigned int low = value->limbs[0];
    value->limbs[2] = high >> 1;
    value->limbs[0] = (low >> 1) | (middle << 31);
}

static void __mtold12(const char* digits, unsigned int count, CrtLongDouble12* output)
{
    unsigned short exponent = 16462;
    output->limbs[0] = 0;
    output->limbs[1] = 0;
    output->limbs[2] = 0;
    while (count != 0)
    {
        CrtLongDouble12 addend = *output;
        __shl_12(output);
        __shl_12(output);
        __add_12(output, &addend);
        __shl_12(output);
        addend.limbs[0] = static_cast<unsigned int>(static_cast<int>(static_cast<signed char>(*digits)));
        addend.limbs[1] = 0;
        addend.limbs[2] = 0;
        __add_12(output, &addend);
        ++digits;
        --count;
    }
    while (output->limbs[2] == 0)
    {
        const unsigned int middle = output->limbs[1];
        output->limbs[2] = middle >> 16;
        const unsigned int low = output->limbs[0];
        output->limbs[1] = (middle << 16) | (low >> 16);
        output->limbs[0] = low << 16;
        exponent = static_cast<unsigned short>(exponent - 16);
    }
    while ((output->limbs[2] & 0x8000u) == 0)
    {
        __shl_12(output);
        --exponent;
    }
    output->words[5] = exponent;
}

static void __ld12mul(CrtLongDouble12* value, const CrtLongDouble12* multiplier)
{
    const unsigned short rightExponent = multiplier->words[5] & 0x7FFFu;
    const unsigned short leftExponent = value->words[5] & 0x7FFFu;
    const unsigned short sign = (value->words[5] ^ multiplier->words[5]) & 0x8000u;
    unsigned short exponent = static_cast<unsigned short>(leftExponent + rightExponent);
    if (leftExponent == 32767 || rightExponent == 32767 || exponent > 49149)
    {
        value->limbs[1] = 0;
        value->limbs[0] = 0;
        value->limbs[2] = 0x7FFF8000u + (sign != 0 ? 0x80000000u : 0u);
        return;
    }
    if (exponent <= 16319)
    {
        value->limbs[2] = 0;
        value->limbs[1] = 0;
        value->limbs[0] = 0;
        return;
    }
    if (leftExponent == 0)
    {
        ++exponent;
        if ((value->limbs[2] & 0x7FFFFFFFu) == 0 && value->limbs[1] == 0 && value->limbs[0] == 0)
        {
            value->words[5] = 0;
            return;
        }
    }
    if (rightExponent == 0)
    {
        ++exponent;
        if ((multiplier->limbs[2] & 0x7FFFFFFFu) == 0 && multiplier->limbs[1] == 0 && multiplier->limbs[0] == 0)
        {
            value->limbs[2] = 0;
            value->limbs[1] = 0;
            value->limbs[0] = 0;
            return;
        }
    }
    CrtLongDouble12 product = {};
    for (int column = 0; column < 5; ++column)
    {
        for (int digit = 0; digit < 5 - column; ++digit)
        {
            const unsigned int part = static_cast<unsigned int>(value->words[column + digit]) * multiplier->words[4 - digit];
            unsigned int accumulated = static_cast<unsigned int>(product.words[column]) |
                (static_cast<unsigned int>(product.words[column + 1]) << 16);
            const int carry = __addl(accumulated, part, &accumulated);
            product.words[column] = static_cast<unsigned short>(accumulated);
            product.words[column + 1] = static_cast<unsigned short>(accumulated >> 16);
            if (carry)
                ++product.words[column + 2];
        }
    }
    exponent = static_cast<unsigned short>(exponent - 16382);
    while (static_cast<short>(exponent) > 0 && (product.words[5] & 0x8000u) == 0)
    {
        __shl_12(&product);
        --exponent;
    }
    if (static_cast<short>(exponent) <= 0)
    {
        --exponent;
        if (static_cast<short>(exponent) < 0)
        {
            const int shift = -static_cast<short>(exponent);
            exponent = static_cast<unsigned short>(exponent + shift);
            unsigned int lostBits = 0;
            for (int bit = 0; bit < shift; ++bit)
            {
                if ((product.limbs[0] & 1u) != 0)
                    ++lostBits;
                __shr_12(&product);
            }
            if (lostBits != 0)
                product.words[0] |= 1u;
        }
    }
    if (product.words[0] > 0x8000u || (product.limbs[0] & 0x1FFFFu) == 0x18000u)
    {
        unsigned int low = static_cast<unsigned int>(product.words[1]) |
            (static_cast<unsigned int>(product.words[2]) << 16);
        if (low == 0xFFFFFFFFu)
        {
            product.words[1] = 0;
            product.words[2] = 0;
            unsigned int high = static_cast<unsigned int>(product.words[3]) |
                (static_cast<unsigned int>(product.words[4]) << 16);
            if (high == 0xFFFFFFFFu)
            {
                product.words[3] = 0;
                product.words[4] = 0;
                if (product.words[5] == 0xFFFFu)
                {
                    ++exponent;
                    product.words[5] = 0x8000u;
                }
                else
                    ++product.words[5];
            }
            else
            {
                ++high;
                product.words[3] = static_cast<unsigned short>(high);
                product.words[4] = static_cast<unsigned short>(high >> 16);
            }
        }
        else
        {
            ++low;
            product.words[1] = static_cast<unsigned short>(low);
            product.words[2] = static_cast<unsigned short>(low >> 16);
        }
    }
    if (exponent >= 32767)
    {
        value->limbs[1] = 0;
        value->limbs[0] = 0;
        value->limbs[2] = 0x7FFF8000u + (sign != 0 ? 0x80000000u : 0u);
        return;
    }
    for (int digit = 0; digit < 5; ++digit)
        value->words[digit] = product.words[digit + 1];
    value->words[5] = exponent | sign;
}

static void __multtenpow12(CrtLongDouble12* value, int exponent, int useExtended)
{
    if (exponent == 0)
        return;
    const CrtLongDouble12* table = _pow10pos;
    unsigned int tableOffset = 0;
    if (exponent < 0)
    {
        table = _pow10neg;
        exponent = BitsToSigned32(0u - static_cast<unsigned int>(exponent));
    }
    if (useExtended == 0)
        value->words[0] = 0;
    while (exponent != 0)
    {
        const int digit = exponent & 7;
        exponent >>= 3;
        if (digit != 0)
        {
            const CrtLongDouble12* factor = table + tableOffset + digit - 1;
            CrtLongDouble12 unrounded;
            if (factor->words[0] >= 0x8000u)
            {
                unrounded = *factor;
                unsigned int low = static_cast<unsigned int>(unrounded.words[1]) |
                    (static_cast<unsigned int>(unrounded.words[2]) << 16);
                --low;
                unrounded.words[1] = static_cast<unsigned short>(low);
                unrounded.words[2] = static_cast<unsigned short>(low >> 16);
                factor = &unrounded;
            }
            __ld12mul(value, factor);
        }
        tableOffset += 7;
    }
}

static void __dtold(CrtLongDouble10* output, const double* input)
{
    unsigned int bits[2];
    std::memcpy(bits, input, sizeof(bits));
    const unsigned short sign = static_cast<unsigned short>((bits[1] >> 16) & 0x8000u);
    unsigned short exponent = static_cast<unsigned short>((bits[1] >> 20) & 0x7FFu);
    unsigned int high = bits[1] & 0xFFFFFu;
    const unsigned int low = bits[0];
    unsigned int hidden = 0x80000000u;
    if (exponent != 0)
        exponent = exponent == 2047 ? 32767 : static_cast<unsigned short>(exponent + 15360);
    else
    {
        if (high == 0 && low == 0)
        {
            output->high = 0;
            output->low = 0;
            output->exponent = 0;
            return;
        }
        exponent = 15361;
        hidden = 0;
    }
    high = (high << 11) | (low >> 21) | hidden;
    output->high = high;
    output->low = low << 11;
    while ((high & 0x80000000u) == 0)
    {
        const unsigned int lower = output->low;
        high = (high << 1) | (lower >> 31);
        output->low = lower << 1;
        output->high = high;
        --exponent;
    }
    output->exponent = exponent | sign;
}

static int _I10_OUTPUT(CrtLongDouble10 input, int precision, int flags, CrtDecimalOutput* output)
{
    const unsigned short sign = input.exponent & 0x8000u;
    const unsigned short exponent = input.exponent & 0x7FFFu;
    output->sign = sign != 0 ? '-' : ' ';
    if (exponent == 0 && input.high == 0 && input.low == 0)
    {
        output->decpt = 0;
        output->sign = ' ';
        output->length = 1;
        output->digits[0] = '0';
        output->digits[1] = 0;
        return 1;
    }
    if (exponent == 32767)
    {
        output->decpt = 1;
        const char* text;
        if ((input.high != 0x80000000u || input.low != 0) && (input.high & 0x40000000u) == 0)
        {
            text = "1#SNAN";
            output->length = 6;
        }
        else if (sign != 0 && input.high == 0xC0000000u && input.low == 0)
        {
            text = "1#IND";
            output->length = 5;
        }
        else if (input.high == 0x80000000u && input.low == 0)
        {
            text = "1#INF";
            output->length = 5;
        }
        else
        {
            text = "1#QNAN";
            output->length = 6;
        }
        std::strcpy(output->digits, text);
        return 0;
    }
    CrtLongDouble12 fraction;
    fraction.words[0] = 0;
    fraction.words[1] = static_cast<unsigned short>(input.low);
    fraction.words[2] = static_cast<unsigned short>(input.low >> 16);
    fraction.words[3] = static_cast<unsigned short>(input.high);
    fraction.words[4] = static_cast<unsigned short>(input.high >> 16);
    fraction.words[5] = exponent;
    int decimalExponent = BitsToSigned32(static_cast<unsigned int>(exponent) * 19728u +
        ((static_cast<unsigned int>(exponent) >> 8) + (input.high >> 24) * 2u) * 77u - 323162868u) >> 16;
    __multtenpow12(&fraction, -static_cast<short>(decimalExponent), 1);
    if (fraction.words[5] >= 16383)
    {
        ++decimalExponent;
        const CrtLongDouble12 tenth = {{0xCCCCCCCCu, 0xCCCCCCCCu, 0x3FFBCCCCu}};
        __ld12mul(&fraction, &tenth);
    }
    output->decpt = static_cast<short>(decimalExponent);
    if ((flags & 1) != 0)
    {
        precision = BitsToSigned32(static_cast<unsigned int>(precision) + static_cast<unsigned int>(static_cast<short>(decimalExponent)));
        if (precision <= 0)
        {
            output->decpt = 0;
            output->sign = ' ';
            output->length = 1;
            output->digits[0] = '0';
            output->digits[1] = 0;
            return 1;
        }
    }
    if (precision > 21)
        precision = 21;
    int shift = static_cast<int>(fraction.words[5]) - 16382;
    fraction.words[5] = 0;
    for (int bit = 0; bit < 8; ++bit)
        __shl_12(&fraction);
    if (shift < 0)
        for (int bit = (-shift) & 255; bit > 0; --bit)
            __shr_12(&fraction);
    char* next = output->digits;
    for (int count = BitsToSigned32(static_cast<unsigned int>(precision) + 1u); count > 0; --count)
    {
        const CrtLongDouble12 addend = fraction;
        __shl_12(&fraction);
        __shl_12(&fraction);
        __add_12(&fraction, &addend);
        __shl_12(&fraction);
        const unsigned char digit = static_cast<unsigned char>(fraction.limbs[2] >> 24);
        fraction.limbs[2] &= 0x00FFFFFFu;
        *next++ = static_cast<char>(digit + '0');
    }
    const char roundingDigit = next[-1];
    char* last = next - 2;
    if (static_cast<signed char>(roundingDigit) >= '5')
    {
        while (last >= output->digits && *last == '9')
            *last-- = '0';
        if (last < output->digits)
        {
            ++last;
            ++output->decpt;
        }
        ++*last;
    }
    else
    {
        while (last >= output->digits && *last == '0')
            --last;
        if (last < output->digits)
        {
            output->decpt = 0;
            output->sign = ' ';
            output->length = 1;
            output->digits[0] = '0';
            output->digits[1] = 0;
            return 1;
        }
    }
    output->length = static_cast<char>(last - output->digits + 1);
    output->digits[static_cast<signed char>(output->length)] = 0;
    return 1;
}

static CrtFloatOutput* _fltout(double value, CrtFloatOutput* output, char* mantissa)
{
    CrtLongDouble10 extended;
    __dtold(&extended, &value);
    CrtDecimalOutput decimal;
    output->flag = _I10_OUTPUT(extended, 17, 0, &decimal);
    output->sign = static_cast<signed char>(decimal.sign);
    output->decpt = decimal.decpt;
    std::strcpy(mantissa, decimal.digits);
    output->mantissa = mantissa;
    return output;
}

static void _fptostr(char* output, int digits, CrtFloatOutput* value)
{
    const char* source = value->mantissa;
    char* next = output + 1;
    *output = '0';
    int rounding = digits;
    if (digits > 0)
    {
        rounding = 0;
        do
        {
            const char digit = *source;
            *next++ = digit != 0 ? *source++ : '0';
        }
        while (--digits != 0);
    }
    *next = 0;
    if (rounding >= 0 && static_cast<signed char>(*source) >= '5')
    {
        while (*--next == '9')
            *next = '0';
        ++*next;
    }
    if (*output == '1')
        ++value->decpt;
    else
        std::memmove(output, output + 1, std::strlen(output + 1) + 1);
}

static void shiftFloatText(char* text, int count)
{
    if (count != 0)
        std::memmove(text + count, text, std::strlen(text) + 1);
}

static char* formatExponentialDigits(char* output, int precision, int uppercase, const CrtFloatOutput* value, char general)
{
    if (general != 0)
        shiftFloatText(output + (value->sign == '-'), precision > 0);
    char* next = output;
    if (value->sign == '-')
        *next++ = '-';
    if (precision > 0)
    {
        next[0] = next[1];
        ++next;
        *next = crtDecimalPoint;
    }
    char* exponent = std::strcpy(next + precision + (general == 0), "e+000");
    if (uppercase != 0)
        exponent[0] = 'E';
    ++exponent;
    if (*value->mantissa != '0')
    {
        int magnitude = BitsToSigned32(static_cast<unsigned int>(value->decpt) - 1u);
        if (magnitude < 0)
        {
            magnitude = BitsToSigned32(0u - static_cast<unsigned int>(magnitude));
            *exponent = '-';
        }
        ++exponent;
        if (magnitude >= 100)
        {
            *exponent = static_cast<char>(*exponent + magnitude / 100);
            magnitude %= 100;
        }
        ++exponent;
        if (magnitude >= 10)
        {
            *exponent = static_cast<char>(*exponent + magnitude / 10);
            magnitude %= 10;
        }
        exponent[1] = static_cast<char>(exponent[1] + magnitude);
    }
    return output;
}

static char* formatFixedDigits(char* output, int precision, const CrtFloatOutput* value, char general)
{
    if (general != 0 && BitsToSigned32(static_cast<unsigned int>(value->decpt) - 1u) == precision)
    {
        char* last = output + (value->sign == '-') + precision;
        last[0] = '0';
        last[1] = 0;
    }
    char* next = output;
    if (value->sign == '-')
        *next++ = '-';
    if (value->decpt <= 0)
    {
        shiftFloatText(next, 1);
        *next++ = '0';
    }
    else
        next += value->decpt;
    if (precision > 0)
    {
        shiftFloatText(next, 1);
        *next++ = crtDecimalPoint;
        const int decpt = value->decpt;
        if (decpt < 0)
        {
            const int zeros = BitsToSigned32(0u - static_cast<unsigned int>(decpt));
            if (general != 0 || precision >= zeros)
                precision = zeros;
            shiftFloatText(next, precision);
            std::memset(next, '0', static_cast<unsigned int>(precision));
        }
    }
    return output;
}

static char* _cftoe(const double* value, char* output, int precision, int uppercase)
{
    CrtFloatOutput decimal;
    char mantissa[24];
    volatile double number = *value;
    _fltout(number, &decimal, mantissa);
    _fptostr(output + (decimal.sign == '-') + (precision > 0),
        BitsToSigned32(static_cast<unsigned int>(precision) + 1u), &decimal);
    return formatExponentialDigits(output, precision, uppercase, &decimal, 0);
}

static char* _cftof(const double* value, char* output, int precision)
{
    CrtFloatOutput decimal;
    char mantissa[24];
    volatile double number = *value;
    _fltout(number, &decimal, mantissa);
    _fptostr(output + (decimal.sign == '-'),
        BitsToSigned32(static_cast<unsigned int>(decimal.decpt) + static_cast<unsigned int>(precision)), &decimal);
    return formatFixedDigits(output, precision, &decimal, 0);
}

static char* _cftog(const double* value, char* output, int precision, int uppercase)
{
    CrtFloatOutput decimal;
    char mantissa[24];
    volatile double number = *value;
    _fltout(number, &decimal, mantissa);
    const int oldExponent = BitsToSigned32(static_cast<unsigned int>(decimal.decpt) - 1u);
    char* digits = output + (decimal.sign == '-');
    _fptostr(digits, precision, &decimal);
    const int exponent = BitsToSigned32(static_cast<unsigned int>(decimal.decpt) - 1u);
    const bool rounded = oldExponent < exponent;
    if (exponent >= -4 && exponent < precision)
    {
        if (rounded)
        {
            while (*digits++ != 0)
            {
            }
            digits[-2] = 0;
        }
        return formatFixedDigits(output, precision, &decimal, 1);
    }
    return formatExponentialDigits(output, precision, uppercase, &decimal, 1);
}

static char* __cdecl _cfltcvt(const double* value, char* output, int formatChar, int precision, int uppercase)
{
    if (formatChar == 'e' || formatChar == 'E')
        return _cftoe(value, output, precision, uppercase);
    if (formatChar == 'f')
        return _cftof(value, output, precision);
    return _cftog(value, output, precision, uppercase);
}

static int __cdecl _ZeroTail(const unsigned int* mantissa, int bit)
{
    int word = bit / 32;
    const unsigned int mask = ~(~0u << ((31 - bit % 32) & 31));
    if ((mantissa[word] & mask) != 0u)return 0;
    while (++word < 3)
    {
        if (mantissa[word] != 0u)return 0;
    }
    return 1;
}

static int __cdecl _IncMan(unsigned int* mantissa, int bit)
{
    int word = bit / 32;
    unsigned int addend = 1u << ((31 - bit % 32) & 31);
    unsigned int previous = mantissa[word];
    mantissa[word] = previous + addend;
    int carry = mantissa[word] < previous || mantissa[word] < addend;
    while (--word >= 0 && carry != 0)
    {
        previous = mantissa[word];
        mantissa[word] = previous + 1u;
        carry = mantissa[word] < previous || mantissa[word] < 1u;
    }
    return carry;
}

static int __cdecl _RoundMan(unsigned int* mantissa, int precision)
{
    const int word = precision / 32;
    const int shift = (31 - precision % 32) & 31;
    int carry = 0;
    if ((mantissa[word] & (1u << shift)) != 0u &&
        _ZeroTail(mantissa, precision + 1) == 0)
    {
        carry = _IncMan(mantissa, precision - 1);
    }
    mantissa[word] &= ~0u << shift;
    for (int index = word + 1; index < 3; ++index)mantissa[index] = 0u;
    return carry;
}

static void __cdecl _CopyMan(unsigned int* destination, const unsigned int* source)
{
    for (int index = 0; index < 3; ++index)destination[index] = source[index];
}

static void __cdecl _FillZeroMan(unsigned int* mantissa)
{
    mantissa[0] = 0u;
    mantissa[1] = 0u;
    mantissa[2] = 0u;
}

static int __cdecl _IsZeroMan(const unsigned int* mantissa)
{
    for (int index = 0; index < 3; ++index)
    {
        if (mantissa[index] != 0u)return 0;
    }
    return 1;
}

static void __cdecl _ShrMan(unsigned int* mantissa, int shift)
{
    const int wordShift = shift / 32;
    const int bitShift = shift % 32;
    const unsigned int mask = ~(~0u << (bitShift & 31));
    unsigned int carry = 0u;
    for (int index = 0; index < 3; ++index)
    {
        const unsigned int lowBits = mantissa[index] & mask;
        mantissa[index] = (mantissa[index] >> (bitShift & 31)) | carry;
        carry = lowBits << ((32 - bitShift) & 31);
    }
    for (int index = 2; index >= 0; --index)
    {
        mantissa[index] = index >= wordShift ? mantissa[index - wordShift] : 0u;
    }
}

struct CrtFloatingFormat
{
    int maximumExponent;
    int minimumExponent;
    int significandBits;
    int exponentBits;
    int storageBits;
    int exponentBias;
};

static const CrtFloatingFormat crtDoubleFormat = {1024, -1023, 53, 11, 64, 1023};
static const CrtFloatingFormat crtFloatFormat = {128, -127, 24, 8, 32, 127};

static int __cdecl _ld12cvt(const CrtLongDouble12* value, void* output, const CrtFloatingFormat& format)
{
    const unsigned int sign = value->words[5] & 0x8000u;
    int exponent = static_cast<int>(value->words[5] & 0x7FFFu) - 16383;
    unsigned int mantissa[3] = {
        static_cast<unsigned int>(value->words[3]) |
            (static_cast<unsigned int>(value->words[4]) << 16),
        static_cast<unsigned int>(value->words[1]) |
            (static_cast<unsigned int>(value->words[2]) << 16),
        static_cast<unsigned int>(value->words[0]) << 16
    };
    unsigned int encodedExponent = 0u;
    int status = 0;
    if (exponent == -16383)
    {
        if (_IsZeroMan(mantissa) == 0)
        {
            _FillZeroMan(mantissa);
            status = 2;
        }
    }
    else
    {
        unsigned int originalMantissa[3];
        _CopyMan(originalMantissa, mantissa);
        if (_RoundMan(mantissa, format.significandBits) != 0)++exponent;
        const int minimumExponent = format.minimumExponent;
        if (exponent < minimumExponent - format.significandBits)
        {
            _FillZeroMan(mantissa);
            status = 2;
        }
        else if (exponent <= minimumExponent)
        {
            _CopyMan(mantissa, originalMantissa);
            _ShrMan(mantissa, minimumExponent - exponent);
            _RoundMan(mantissa, format.significandBits);
            _ShrMan(mantissa, format.exponentBits + 1);
            status = 2;
        }
        else if (exponent >= format.maximumExponent)
        {
            _FillZeroMan(mantissa);
            mantissa[0] |= 0x80000000u;
            _ShrMan(mantissa, format.exponentBits);
            encodedExponent = static_cast<unsigned int>(format.exponentBias) +
                static_cast<unsigned int>(format.maximumExponent);
            status = 1;
        }
        else
        {
            mantissa[0] &= 0x7FFFFFFFu;
            encodedExponent = static_cast<unsigned int>(format.exponentBias) +
                static_cast<unsigned int>(exponent);
            _ShrMan(mantissa, format.exponentBits);
        }
    }
    const unsigned int high = mantissa[0] |
        (encodedExponent << ((31 - format.exponentBits) & 31)) |
        (sign != 0u ? 0x80000000u : 0u);
    if (format.storageBits == 64)
    {
        memcpy(static_cast<unsigned char*>(output) + 4, &high, 4);
        memcpy(output, &mantissa[1], 4);
    }
    else if (format.storageBits == 32)
    {
        memcpy(output, &high, 4);
    }
    return status;
}

static int __cdecl _ld12tod(const CrtLongDouble12* value, void* output)
{
    return _ld12cvt(value, output,
        crtDoubleFormat);
}

static int __cdecl _ld12tof(const CrtLongDouble12* value, void* output)
{
    return _ld12cvt(value, output,
        crtFloatFormat);
}

static int __cdecl __strgtold12(CrtLongDouble12* output, const char** end,
    const char* text, int useExtended, int scale, int decimalPosition,
    int allowImplicitExponent)
{
    enum ParseState
    {
        INITIAL, LEADING_ZERO, AFTER_SIGN, INTEGER_DIGITS, FRACTION_DIGITS,
        AFTER_DECIMAL, EXPONENT_START, AFTER_EXPONENT_SIGN, EXPONENT_ZERO,
        EXPONENT_DIGITS, FINISHED, IMPLICIT_EXPONENT
    };
    const char* cursor = text;
    const char* rollback = text;
    char digits[28];
    unsigned int digitCount = 0u;
    unsigned int decimalShift = 0u;
    unsigned int exponentMagnitude = 0u;
    unsigned int sign = 0u;
    bool sawDigit = false;
    bool sawDecimal = false;
    bool sawExponent = false;
    bool negativeExponent = false;
    ParseState state = INITIAL;
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r')
        ++cursor;

    for (;;)
    {
        signed char ch = static_cast<signed char>(*cursor++);
        switch (state)
        {
        case INITIAL:
            if (ch >= '1' && ch <= '9')
            {
                --cursor;
                state = INTEGER_DIGITS;
            }
            else if (ch == static_cast<signed char>(crtDecimalPoint))
                state = AFTER_DECIMAL;
            else if (ch == '+' || ch == '-')
            {
                sign = ch == '-' ? 0x8000u : 0u;
                state = AFTER_SIGN;
            }
            else if (ch == '0')
                state = LEADING_ZERO;
            else
            {
                --cursor;
                goto parsed;
            }
            break;

        case LEADING_ZERO:
            sawDigit = true;
            if (ch >= '1' && ch <= '9')
            {
                --cursor;
                state = INTEGER_DIGITS;
            }
            else if (ch == static_cast<signed char>(crtDecimalPoint))
                state = FRACTION_DIGITS;
            else if (ch == '+' || ch == '-')
            {
                --cursor;
                state = IMPLICIT_EXPONENT;
            }
            else if (ch == '0')
                state = LEADING_ZERO;
            else if (ch == 'D' || ch == 'E' || ch == 'd' || ch == 'e')
                state = EXPONENT_START;
            else
            {
                --cursor;
                goto parsed;
            }
            break;

        case AFTER_SIGN:
            if (ch >= '1' && ch <= '9')
            {
                --cursor;
                state = INTEGER_DIGITS;
            }
            else if (ch == static_cast<signed char>(crtDecimalPoint))
                state = AFTER_DECIMAL;
            else if (ch == '0')
                state = LEADING_ZERO;
            else
            {
                cursor = rollback;
                goto parsed;
            }
            break;

        case INTEGER_DIGITS:
            sawDigit = true;
            while (CrtIsDigit(static_cast<unsigned char>(ch)) != 0u)
            {
                if (digitCount < 25u)
                    digits[digitCount++] = static_cast<char>(ch - '0');
                else
                    ++decimalShift;
                ch = static_cast<signed char>(*cursor++);
            }
            if (ch == static_cast<signed char>(crtDecimalPoint))
            {
                state = FRACTION_DIGITS;
                break;
            }
            goto exponentOrEnd;

        case FRACTION_DIGITS:
            sawDigit = true;
            sawDecimal = true;
            if (digitCount == 0u)
            {
                while (ch == '0')
                {
                    --decimalShift;
                    ch = static_cast<signed char>(*cursor++);
                }
            }
            while (CrtIsDigit(static_cast<unsigned char>(ch)) != 0u)
            {
                if (digitCount < 25u)
                {
                    digits[digitCount++] = static_cast<char>(ch - '0');
                    --decimalShift;
                }
                ch = static_cast<signed char>(*cursor++);
            }
        exponentOrEnd:
            if (ch == '+' || ch == '-')
            {
                --cursor;
                state = IMPLICIT_EXPONENT;
            }
            else if (ch == 'D' || ch == 'E' || ch == 'd' || ch == 'e')
                state = EXPONENT_START;
            else
            {
                --cursor;
                goto parsed;
            }
            break;

        case AFTER_DECIMAL:
            sawDecimal = true;
            if (CrtIsDigit(static_cast<unsigned char>(ch)) != 0u)
            {
                --cursor;
                state = FRACTION_DIGITS;
            }
            else
            {
                cursor = rollback;
                goto parsed;
            }
            break;

        case EXPONENT_START:
            rollback = cursor - 2;
            if (ch >= '1' && ch <= '9')
            {
                --cursor;
                state = EXPONENT_DIGITS;
            }
            else if (ch == '+' || ch == '-')
            {
                negativeExponent = ch == '-';
                state = AFTER_EXPONENT_SIGN;
            }
            else if (ch == '0')
                state = EXPONENT_ZERO;
            else
            {
                cursor = rollback;
                goto parsed;
            }
            break;

        case AFTER_EXPONENT_SIGN:
            if (ch >= '1' && ch <= '9')
            {
                --cursor;
                state = EXPONENT_DIGITS;
            }
            else if (ch == '0')
                state = EXPONENT_ZERO;
            else
            {
                cursor = rollback;
                goto parsed;
            }
            break;

        case EXPONENT_ZERO:
            sawExponent = true;
            while (ch == '0')ch = static_cast<signed char>(*cursor++);
            if (ch >= '1' && ch <= '9')
            {
                --cursor;
                state = EXPONENT_DIGITS;
            }
            else
            {
                --cursor;
                goto parsed;
            }
            break;

        case EXPONENT_DIGITS:
            sawExponent = true;
            exponentMagnitude = 0u;
            while (CrtIsDigit(static_cast<unsigned char>(ch)) != 0u)
            {
                exponentMagnitude = exponentMagnitude * 10u +
                    static_cast<unsigned int>(static_cast<int>(ch) - '0');
                if (BitsToSigned32(exponentMagnitude) > 5200)
                {
                    exponentMagnitude = 5201u;
                    break;
                }
                ch = static_cast<signed char>(*cursor++);
            }
            while (CrtIsDigit(static_cast<unsigned char>(ch)) != 0u)
                ch = static_cast<signed char>(*cursor++);
            --cursor;
            goto parsed;

        case IMPLICIT_EXPONENT:
            if (allowImplicitExponent == 0)
            {
                --cursor;
                goto parsed;
            }
            rollback = cursor - 1;
            if (ch == '+' || ch == '-')
            {
                negativeExponent = ch == '-';
                state = AFTER_EXPONENT_SIGN;
            }
            else
            {
                cursor = rollback;
                goto parsed;
            }
            break;

        case FINISHED:
            goto parsed;
        }
    }

parsed:
    *end = cursor;
    CrtLongDouble12 value;
    int status = 0;
    if (!sawDigit)
    {
        value.limbs[0] = value.limbs[1] = value.limbs[2] = 0u;
        status = 4;
    }
    else
    {
        if (digitCount > 24u)
        {
            if (static_cast<signed char>(digits[23]) >= 5)++digits[23];
            digitCount = 24u;
            ++decimalShift;
        }
        if (digitCount == 0u)
            value.limbs[0] = value.limbs[1] = value.limbs[2] = 0u;
        else
        {
            char* tail = digits + digitCount;
            while (*--tail == 0)
            {
                --digitCount;
                ++decimalShift;
            }
            __mtold12(digits, digitCount, &value);
            unsigned int exponent = (negativeExponent ? 0u - exponentMagnitude :
                exponentMagnitude) + decimalShift;
            if (!sawExponent)exponent += static_cast<unsigned int>(scale);
            if (!sawDecimal)exponent -= static_cast<unsigned int>(decimalPosition);
            if (BitsToSigned32(exponent) > 5200)
            {
                value.limbs[0] = value.limbs[1] = value.limbs[2] = 0u;
                value.words[4] = 0x8000u;
                value.words[5] = 0x7FFFu;
                status = 2;
            }
            else if (BitsToSigned32(exponent) < -5200)
            {
                value.limbs[0] = value.limbs[1] = value.limbs[2] = 0u;
                status = 1;
            }
            else
                __multtenpow12(&value, BitsToSigned32(exponent), useExtended);
        }
    }
    value.words[5] |= static_cast<unsigned short>(sign);
    memcpy(reinterpret_cast<unsigned char*>(output) + 6,
        reinterpret_cast<const unsigned char*>(&value) + 6, 4);
    memcpy(reinterpret_cast<unsigned char*>(output) + 2,
        reinterpret_cast<const unsigned char*>(&value) + 2, 4);
    output->words[5] = value.words[5];
    output->words[0] = value.words[0];
    return status;
}

static int __cdecl _atodbl(void* output, const char* text)
{
    CrtLongDouble12 value;
    __strgtold12(&value, &text, text, 0, 0, 0, 0);
    return _ld12tod(&value, output);
}

static int __cdecl _atoflt(void* output, const char* text)
{
    CrtLongDouble12 value;
    __strgtold12(&value, &text, text, 0, 0, 0, 0);
    return _ld12tof(&value, output);
}

static void __cdecl _fassign(unsigned int mode, void* output, const char* text)
{
    if (mode != 0u)
    {
        unsigned int value[2];
        _atodbl(value, text);
        memcpy(output, &value[0], 4);
        memcpy(static_cast<unsigned char*>(output) + 4, &value[1], 4);
    }
    else
    {
        unsigned int value;
        _atoflt(&value, text);
        memcpy(output, &value, 4);
    }
}

enum CrtFloatInputFlag
{
    FloatInputConversionOverflow = 1,
    FloatInputInvalid = 64,
    FloatInputOverflow = 128,
    FloatInputUnderflow = 256,
    FloatInputNoDigits = 512
};

struct CrtFloatInput
{
    unsigned int flags;
    unsigned int consumed;
    unsigned int reserved[2];
    double value;
};

static CrtFloatInput* __cdecl _fltin(CrtFloatInput* output, const char* text,
    unsigned int length, int scale, int decimalPosition)
{
    (void)length;
    (void)scale;
    (void)decimalPosition;
    CrtLongDouble12 extended;
    const char* end;
    const int parsed = __strgtold12(&extended, &end, text, 0, 0, 0, 0);
    unsigned int flags = 0;
    double value;
    if ((parsed & 4) != 0)
    {
        flags = FloatInputNoDigits;
        value = 0.0;
    }
    else
    {
        const int converted = _ld12tod(&extended, &value);
        if ((parsed & 2) != 0 || converted == 1) flags = FloatInputOverflow;
        if ((parsed & 1) != 0 || converted == 2) flags |= FloatInputUnderflow;
    }
    output->flags = flags;
    output->value = value;
    output->consumed = static_cast<unsigned int>(end - text);
    return output;
}

static double CrtParseDouble(const char* source, char** end)
{
    const char* first = source;
    for (;;)
    {
        const unsigned char character = static_cast<unsigned char>(*first);
        const unsigned int space = crtMaximumMultibyteLength > 1
            ? CrtIsCtype(character, CtypeSpace)
            : crtCharacterTypes[character] & CtypeSpace;
        if (space == 0) break;
        ++first;
    }
    CrtFloatInput parsed;
    _fltin(&parsed, first, static_cast<unsigned int>(std::strlen(first)), 0, 0);
    if (end != 0) *end = const_cast<char*>(first) + parsed.consumed;
    if ((parsed.flags & (FloatInputInvalid | FloatInputNoDigits)) != 0)
    {
        if (end != 0) *end = const_cast<char*>(source);
        return 0.0;
    }
    if ((parsed.flags & (FloatInputConversionOverflow | FloatInputOverflow)) != 0)
    {
        const double huge = std::numeric_limits<double>::infinity();
        const double value = *first == '-' ? -huge : huge;
        errno = ERANGE;
        return value;
    }
    if ((parsed.flags & FloatInputUnderflow) != 0)
    {
        errno = ERANGE;
        return 0.0;
    }
    return parsed.value;
}


static void __cdecl _fptrap()
{
    CrtAmsgExit(2u);
}

struct CrtFloatFunctions
{
    typedef char* (__cdecl *Format)(const double*, char*, int, int, int);
    typedef void (__cdecl *TrimZeroes)(char*);
    typedef void (__cdecl *Assign)(unsigned int, void*, const char*);
    typedef void (__cdecl *ForceDecimal)(char*);
    typedef int (__cdecl *Positive)(const double*);
    Format format;
    TrimZeroes trimZeroes;
    Assign assign;
    ForceDecimal forceDecimal;
    Positive positive;
    Format alternateFormat;
};

static CrtFloatFunctions crtFloatFunctions = {
    reinterpret_cast<CrtFloatFunctions::Format>(&_fptrap),
    reinterpret_cast<CrtFloatFunctions::TrimZeroes>(&_fptrap),
    reinterpret_cast<CrtFloatFunctions::Assign>(&_fptrap),
    reinterpret_cast<CrtFloatFunctions::ForceDecimal>(&_fptrap),
    reinterpret_cast<CrtFloatFunctions::Positive>(&_fptrap),
    reinterpret_cast<CrtFloatFunctions::Format>(&_fptrap)
};
static unsigned int crtFloatingPointDivisionAdjustment = 0;
static unsigned int crtStartupFlag = 0;

static void __cdecl InitializeCrtFloatFormatting()
{
    crtFloatFunctions.trimZeroes = &_cropzeros;
    crtFloatFunctions.format = &_cfltcvt;
    crtFloatFunctions.assign = &_fassign;
    crtFloatFunctions.forceDecimal = &_forcdecpt;
    crtFloatFunctions.positive = &_positive;
    crtFloatFunctions.alternateFormat = &_cfltcvt;


    crtFloatingPointDivisionAdjustment =
        ::IsProcessorFeaturePresent(PF_FLOATING_POINT_PRECISION_ERRATA)?1u:0u;

#if defined(_MSC_VER) && defined(_M_IX86)


    unsigned int x87Control=0u;
    (void)__control87_2(_PC_53, _MCW_PC, &x87Control, 0);
    if (__isa_available >= __ISA_AVAILABLE_SSE2) {
        const unsigned int mxcsr = _mm_getcsr();
        (void)_clearfp();
        _mm_setcsr(mxcsr);
    } else {
        (void)_clearfp();
    }
#endif


}

static void (__cdecl* crtFloatingPointInitializer)() = &InitializeCrtFloatFormatting;

static unsigned int __cdecl InitializeCrtInitialStorage()
{
    CrtExitCallback* initialStorage = static_cast<CrtExitCallback*>(CrtAllocate(128u));
    crtExitCallbacksBegin = initialStorage;
    if (initialStorage == 0)
    {
        CrtAmsgExit(24u);
        initialStorage = crtExitCallbacksBegin;
    }
    *initialStorage = 0;
    CrtExitCallback* const begin = crtExitCallbacksBegin;
    crtExitCallbacksEnd = begin;
    return PtrToU32(begin);
}

static void __cdecl InitializeCrtStreamTable()
{
    if (crtStreamCount == 0) crtStreamCount = 512;
    else if (crtStreamCount < 20) crtStreamCount = 20;

    crtStreamTable = static_cast<CrtFileRecord**>(
        std::calloc(static_cast<unsigned int>(crtStreamCount), sizeof(CrtFileRecord*)));
    if (!crtStreamTable)
    {
        crtStreamCount = 20;
        crtStreamTable = static_cast<CrtFileRecord**>(
            std::calloc(20u, sizeof(CrtFileRecord*)));
        if (!crtStreamTable) CrtAmsgExit(26u);
    }

    for (int stream = 0; stream < 20; ++stream)
        crtStreamTable[stream] = &crtInitialStreams[stream];

    for (int stream = 0; stream < 3; ++stream)
    {
        std::intptr_t handle = _get_osfhandle(stream);
        // The modern CRT uses -2 for a missing standard stream; VC6 used -1.
        if (handle == -2) handle = -1;
        if (handle == -1 || handle == 0) crtInitialStreams[stream].file = -1;
    }
}

static unsigned int __cdecl InitializeCrtStartupFlag()
{


    if (crtStartupFlag == 0u) crtStartupFlag = 1u;
    return 1u;
}




static LPTOP_LEVEL_EXCEPTION_FILTER crtPreviousExceptionFilter = 0;

static LONG WINAPI CrtUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers)
{
    const EXCEPTION_RECORD& record = *exceptionPointers->ExceptionRecord;
    if (record.ExceptionCode == 0xE06D7363u && record.NumberParameters == 3u &&
        record.ExceptionInformation[0] == 0x19930520u)
    {
        std::terminate();
    }
    const LPTOP_LEVEL_EXCEPTION_FILTER previous = crtPreviousExceptionFilter;
    if (previous && !IsBadCodePtr(reinterpret_cast<FARPROC>(previous)))
    {
        const LPTOP_LEVEL_EXCEPTION_FILTER callback = crtPreviousExceptionFilter;
        return callback(exceptionPointers);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static unsigned int __cdecl InstallCrtExceptionFilter()
{
    const LPTOP_LEVEL_EXCEPTION_FILTER previous = SetUnhandledExceptionFilter(&CrtUnhandledExceptionFilter);
    crtPreviousExceptionFilter = previous;
    return PtrToU32(previous);
}

static HMODULE LoadD3DX9Module(const char* dll)
{
    HMODULE moduleHandle = GetModuleHandleA(dll);
    if (moduleHandle)return moduleHandle;
    moduleHandle = LoadLibraryA(dll);
    if (moduleHandle)return moduleHandle;

    char path[MAX_PATH+64];
    DWORD pathLength = GetCurrentDirectoryA(MAX_PATH, path);
    if (pathLength && pathLength<MAX_PATH)
    {
        if (path[pathLength-1] != '\\' && path[pathLength-1] != '/')path[pathLength++] = '\\';
        lstrcpynA(path+pathLength, dll, (int)(sizeof(path)-pathLength));
        moduleHandle = LoadLibraryA(path);
        if (moduleHandle)return moduleHandle;
    }
    pathLength = GetModuleFileNameA(0, path, MAX_PATH);
    if (pathLength && pathLength<MAX_PATH)
    {
        while (pathLength && path[pathLength-1] != '\\' && path[pathLength-1] != '/')--pathLength;
        lstrcpynA(path+pathLength, dll, (int)(sizeof(path)-pathLength));
        moduleHandle = LoadLibraryA(path);
        if (moduleHandle)return moduleHandle;
    }
    return 0;
}

static FARPROC ResolveD3DX9Proc(const char* name)
{
    HMODULE module = LoadD3DX9Module("d3dx9_31.dll");
    return module?GetProcAddress(module, name):0;
}

typedef HRESULT (__stdcall *D3DXCreateEffectFromFileAProc)(
    IDirect3DDevice9*, const char*, const D3DXMACRO*, ID3DXInclude*, DWORD,
    ID3DXEffectPool*, ID3DXEffect**, ID3DXBuffer**);

HRESULT __stdcall D3DXCreateEffectFromFileA(
    IDirect3DDevice9* device, const char* path, const D3DXMACRO* defines,
    ID3DXInclude* include, DWORD flags, ID3DXEffectPool* pool,
    ID3DXEffect** effectOut, ID3DXBuffer** errorsOut)
{
    FARPROC raw = ResolveD3DX9Proc("D3DXCreateEffectFromFileA");
    if (!raw)
    {
        return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }

    const long hr = reinterpret_cast<D3DXCreateEffectFromFileAProc>(raw)(
        device, path, defines, include, flags, pool, effectOut, errorsOut);
    return hr;
}

typedef long (__stdcall *D3DXSaveSurfaceToFileAProc)(
    const char*, D3DXIMAGE_FILEFORMAT, IDirect3DSurface9*, const PALETTEENTRY*, const RECT*);

long __stdcall D3DXSaveSurfaceToFileA(
    const char* destFile, D3DXIMAGE_FILEFORMAT destFormat, IDirect3DSurface9* sourceSurface,
    const PALETTEENTRY* sourcePalette, const RECT* sourceRect)
{
    FARPROC raw = ResolveD3DX9Proc("D3DXSaveSurfaceToFileA");
    if (!raw) return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    return reinterpret_cast<D3DXSaveSurfaceToFileAProc>(raw)(
        destFile, destFormat, sourceSurface, sourcePalette, sourceRect);
}

typedef long (__stdcall *D3DXGetImageInfoFromFileAProc)(const char*, D3DXIMAGE_INFO*);

long __stdcall D3DXGetImageInfoFromFileA(const char* sourceFile, D3DXIMAGE_INFO* sourceInfo)
{
    FARPROC raw = ResolveD3DX9Proc("D3DXGetImageInfoFromFileA");
    if (!raw) return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    return reinterpret_cast<D3DXGetImageInfoFromFileAProc>(raw)(sourceFile, sourceInfo);
}

typedef long (__stdcall *D3DXLoadSurfaceFromSurfaceProc)(
    IDirect3DSurface9*, const PALETTEENTRY*, const RECT*, IDirect3DSurface9*,
    const PALETTEENTRY*, const RECT*, unsigned long, unsigned long);

typedef long (__stdcall *D3DXLoadSurfaceFromMemoryProc)(
    IDirect3DSurface9*, const PALETTEENTRY*, const RECT*, const void*,
    D3DFORMAT, unsigned int, const PALETTEENTRY*, const RECT*, unsigned long, unsigned long);

long __stdcall D3DXLoadSurfaceFromMemory(
    IDirect3DSurface9* destSurface, const PALETTEENTRY* destPalette, const RECT* destRect,
    const void* sourceMemory, D3DFORMAT sourceFormat, unsigned int sourcePitch,
    const PALETTEENTRY* sourcePalette, const RECT* sourceRect,
    unsigned long filter, unsigned long colorKey)
{
    FARPROC raw = ResolveD3DX9Proc("D3DXLoadSurfaceFromMemory");
    if (!raw) return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    return reinterpret_cast<D3DXLoadSurfaceFromMemoryProc>(raw)(
        destSurface, destPalette, destRect, sourceMemory, sourceFormat, sourcePitch,
        sourcePalette, sourceRect, filter, colorKey);
}

long __stdcall D3DXLoadSurfaceFromSurface(
    IDirect3DSurface9* destSurface, const PALETTEENTRY* destPalette, const RECT* destRect,
    IDirect3DSurface9* sourceSurface, const PALETTEENTRY* sourcePalette, const RECT* sourceRect,
    unsigned long filter, unsigned long colorKey)
{
    FARPROC raw = ResolveD3DX9Proc("D3DXLoadSurfaceFromSurface");
    if (!raw) return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    return reinterpret_cast<D3DXLoadSurfaceFromSurfaceProc>(raw)(
        destSurface, destPalette, destRect, sourceSurface, sourcePalette, sourceRect, filter, colorKey);
}

typedef long (__stdcall *D3DXCreateTextureFromFileExAProc)(
    IDirect3DDevice9*, const char*, unsigned int, unsigned int, unsigned int, unsigned long,
    D3DFORMAT, D3DPOOL, unsigned long, unsigned long, unsigned long,
    D3DXIMAGE_INFO*, PALETTEENTRY*, IDirect3DTexture9**);

long __stdcall D3DXCreateTextureFromFileExA(
    IDirect3DDevice9* device, const char* sourceFile, unsigned int width, unsigned int height,
    unsigned int mipLevels, unsigned long usage, D3DFORMAT format, D3DPOOL pool,
    unsigned long filter, unsigned long mipFilter, unsigned long colorKey,
    D3DXIMAGE_INFO* sourceInfo, PALETTEENTRY* palette, IDirect3DTexture9** textureOut)
{
    FARPROC raw = ResolveD3DX9Proc("D3DXCreateTextureFromFileExA");
    if (!raw) return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    return reinterpret_cast<D3DXCreateTextureFromFileExAProc>(raw)(
        device, sourceFile, width, height, mipLevels, usage, format, pool, filter, mipFilter,
        colorKey, sourceInfo, palette, textureOut);
}

struct FindTimeBoundary
{
    int day;
    int milliseconds;
};

struct FindTimeZone
{
    long timezone;
    int daylight;
    long dstbias;
    bool windowsRules;
    TIME_ZONE_INFORMATION rules;

    FindTimeZone() : timezone(28800), daylight(1), dstbias(-3600), windowsRules(false), rules
{
}
    {
        const char* zone=std::getenv("TZ");
        if(zone)
        {
            if(*zone==0)return;
            const char* value=zone+3;
            const bool negative=*value=='-';
            if(negative)++value;
            timezone=BitsToSigned32(CrtAtoiText(value)*3600u);
            while(*value=='+' || (*value>='0' && *value<='9'))++value;
            if(*value==':')
            {
                ++value;
                timezone=BitsToSigned32(static_cast<unsigned int>(timezone)+CrtAtoiText(value)*60u);
                while(*value>='0' && *value<='9')++value;
                if(*value==':')
                {
                    ++value;
                    timezone=BitsToSigned32(static_cast<unsigned int>(timezone)+CrtAtoiText(value));
                    while(*value>='0' && *value<='9')++value;
                }
            }
            if(negative)timezone=BitsToSigned32(0u-static_cast<unsigned int>(timezone));
            daylight=static_cast<signed char>(*value);
            return;
        }
        if(GetTimeZoneInformation(&rules)==TIME_ZONE_ID_INVALID)return;
        windowsRules=true;
        timezone=rules.Bias*60;
        if(rules.StandardDate.wMonth!=0)timezone+=rules.StandardBias*60;
        if(rules.DaylightDate.wMonth!=0 && rules.DaylightBias!=0)
        {
            daylight=1;
            dstbias=(rules.DaylightBias-rules.StandardBias)*60;
        }
        else
        {
            daylight=0;
            dstbias=0;
        }
    }
};

static int FindTimeMonthStart(int year, unsigned int month)
{
    static const int starts[13] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    return starts[month]+((year%4 == 0 && month>2u)?1:0);
}

static FindTimeBoundary FindTimeTransition(int year, const SYSTEMTIME& rule,
                                         bool daylightStart, long dstbias)
{
    int day = FindTimeMonthStart(year, rule.wMonth);
    if (rule.wYear == 0)
    {
        const int sinceEpoch = (year-1970)*365+(year-1969)/4+day;
        const int firstWeekday = (sinceEpoch+4)%7;
        day+=(rule.wDay-1)*7+static_cast<int>(rule.wDayOfWeek)-firstWeekday;
        if (firstWeekday>rule.wDayOfWeek)day+=7;
        if (rule.wDay == 5)
        {
            const int last = rule.wMonth == 12?365+(year%4 == 0):FindTimeMonthStart(year, rule.wMonth+1);
            if (day >= last)day-=7;
        }
    }
    else
    {
        day+=rule.wDay-1;
    }
    int milliseconds = ((rule.wHour*60+rule.wMinute)*60+rule.wSecond)*1000+rule.wMilliseconds;
    if (!daylightStart)
    {
        milliseconds+=dstbias*1000;
        if (milliseconds<0)
        {
            milliseconds+=86400000;
            --day;
        }
        else if (milliseconds >= 86400000)
        {
            milliseconds-=86400000;
            ++day;
        }
    }
    return
    {
        day, milliseconds
    };
}

static bool FindTimeInDaylight(int year, int day, int milliseconds, const FindTimeZone& zone)
{
    if (zone.daylight == 0)return false;
    SYSTEMTIME start;
    SYSTEMTIME finish;
    if (zone.windowsRules)
    {
        start = zone.rules.DaylightDate;
        finish = zone.rules.StandardDate;
    }
    else
    {
        start = {0, 4, 0, 1, 2, 0, 0, 0};
        finish = {0, 10, 0, 5, 2, 0, 0, 0};
    }
    const FindTimeBoundary begin = FindTimeTransition(year, start, true, zone.dstbias);
    const FindTimeBoundary end = FindTimeTransition(year, finish, false, zone.dstbias);
    if (begin.day<end.day)
    {
        if (day<begin.day || day>end.day)return false;
        if (day>begin.day && day<end.day)return true;
    }
    else
    {
        if (day<end.day || day>begin.day)return true;
        if (day>end.day && day<begin.day)return false;
    }
    if (day == begin.day)return milliseconds >= begin.milliseconds;
    return milliseconds<end.milliseconds;
}

static __time32_t FindDataTime(const FILETIME& time)
{
    if (time.dwLowDateTime == 0u && time.dwHighDateTime == 0u)return -1;
    FILETIME localTime;
    SYSTEMTIME calendar;
    if (!FileTimeToLocalFileTime(&time, &localTime))return -1;
    if (!FileTimeToSystemTime(&localTime, &calendar))return -1;
    if (calendar.wYear<1970u || calendar.wYear>2038u)return -1;
    static const FindTimeZone zone;
    const int year = calendar.wYear;
    const int day = FindTimeMonthStart(year, calendar.wMonth)+calendar.wDay-1;
    const unsigned int days = static_cast<unsigned int>((year-1970)*365+(year-1969)/4+day);
    unsigned int bits = (((days*24u+calendar.wHour)*60u+calendar.wMinute)*60u+calendar.wSecond);
    bits+=static_cast<unsigned int>(zone.timezone);
    const int milliseconds = ((calendar.wHour*60+calendar.wMinute)*60+calendar.wSecond)*1000;
    if (FindTimeInDaylight(year, day, milliseconds, zone))bits+=static_cast<unsigned int>(zone.dstbias);
    return BitsToSigned32(bits);
}

intptr_t __cdecl CrtFindFirst(const char* path, _finddata32_t* data)
{
    WIN32_FIND_DATAA found;
    const HANDLE handle = FindFirstFileA(path, &found);
    if (handle == INVALID_HANDLE_VALUE)
    {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND || error == ERROR_NO_MORE_FILES)
            errno = ENOENT;
        else if (error == ERROR_NOT_ENOUGH_MEMORY)
            errno = ENOMEM;
        else
            errno = EINVAL;
        return -1;
    }
    data->attrib = found.dwFileAttributes == FILE_ATTRIBUTE_NORMAL?0u:found.dwFileAttributes;
    data->time_create = FindDataTime(found.ftCreationTime);
    data->time_access = FindDataTime(found.ftLastAccessTime);
    data->time_write = FindDataTime(found.ftLastWriteTime);
    data->size = found.nFileSizeLow;
    std::strcpy(data->name, found.cFileName);
    return reinterpret_cast<intptr_t>(handle);
}

int __cdecl CrtFindClose(intptr_t handle)
{
    if (FindClose(reinterpret_cast<HANDLE>(handle))) return 0;
    errno = EINVAL;
    return -1;
}


namespace {




static unsigned int __cdecl RegisterSharedStartupCleanup()
{
    unsigned char guard = CrtStaticInitializationGuards[0];
    if ((guard & 1u) == 0u)
        CrtStaticInitializationGuards[0] = static_cast<unsigned char>(guard | 1u);
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}


static int RunCrtStaticInitializers()
{
    LONG state = InterlockedCompareExchange(&crtStaticInitializationState, 1, 0);
    if (state == 2)return 1;
    if (state == 1)
    {
        while (crtStaticInitializationState == 1)Sleep(0);
        return crtStaticInitializationState == 2;
    }


    (void)RegisterCrtExitDrain();

    void (__cdecl* const initializeFloatingPoint)() = crtFloatingPointInitializer;
    if (initializeFloatingPoint != 0) initializeFloatingPoint();


    InitializeCrtInitialStorage();
    InitializeCrtStreamTable();
    InitializeCrtStartupFlag();
    InstallCrtExceptionFilter();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    ENGINE::InitializePathDots();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    InitializeGraphBlackAtStartup();
    InitializeGraphCyanAtStartup();
    InitializeGraphMagentaAtStartup();
    InitializeGraphBrownAtStartup();
    InitializeGraphWhiteAtStartup();
    InitializeGraphGrayAtStartup();
    InitializeGraphBlueAtStartup();
    InitializeGraphLightBlueAtStartup();
    InitializeGraphGreenAtStartup();
    InitializeGraphLightGreenAtStartup();
    InitializeGraphRedAtStartup();
    InitializeGraphLightRedAtStartup();
    InitializeGraphYellowAtStartup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    InitializeMapLoadingDirectionAtStartup();
    InitializeMapLoadingTextAtStartup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    SharedNoopStaticInitializer();
    SharedNoopStaticInitializer();
    SharedNoopStaticInitializer();
    RefreshQuarterTickAtStartup();
    MylibStaticCleanupGuard();
    NetHostListStaticInit();
    RegisterSharedStartupCleanup();
    SharedNoopStaticInitializer();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    R_MAP::InitializeStatic();
    SharedNoopStaticInitializer();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    SharedNoopStaticInitializer();
    SharedNoopStaticInitializer();
    RegisterSharedStartupCleanup();
    SharedNoopStaticInitializer();
    SharedNoopStaticInitializer();
    BaseTexture_StaticCleanupGuard01();
    BaseTexture_StaticCleanupGuard02();
    BaseTexture_StaticCleanupGuard03();
    BaseTexture_StaticCleanupGuard04();
    BaseTexture_StaticCleanupGuard05();
    SharedNoopStaticInitializer();
    InitializeVidRootAtStartup();
    InitializeVidFirstRangeAtStartup();
    InitializeVidSecondRangeAtStartup();
    zIniParser_RegisterSharedCleanupOnce();
    zItemInfo_RegisterSharedCleanupOnce();
    RegisterSharedStartupCleanup();
    SharedNoopStaticInitializer();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    SharedNoopStaticInitializer();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    SharedNoopStaticInitializer();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    SharedNoopStaticInitializer();
    zItemTaker::RegisterNetResponseSlots();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    SharedNoopStaticInitializer();
    RegisterSharedStartupCleanup();
    zScriptExec::RegisterSurviveReference();
    zScriptExec::RegisterCampaignReference();
    SharedNoopStaticInitializer();
    RegisterSharedStartupCleanup();
    zSurvive::RegisterMonsterCountCache();
    zSurvive::RegisterMonsterTimeCache();
    RegisterSharedStartupCleanup();
    RecordDBTouchFourStaticInitializers();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    InitializeInputClickPosition();
    SharedNoopStaticInitializer();
    InitializeInputMappingDefaults();
    SharedNoopStaticInitializer();
    InitializeSharedCleanupGuardPair();
    InitializeSharedCleanupGuardGroup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    RegisterSharedStartupCleanup();
    MainSwGlobalStringStaticInit();
    DialogSrvRegisterStaticCleanupSet();
    RegisterSharedStartupCleanup();
    ManStaticInitGlobalColorPalette();
    SharedNoopStaticInitializer();
    PLAYER_ARCADE::InitializeMessage();
    SharedNoopStaticInitializer();
    StaticAtexitGuard();

    InterlockedExchange(&crtStaticInitializationState, 2);
    return 1;
}

}

static int PublishCrtModulePath()
{
    char* const modulePath = crtModulePath;
    const DWORD modulePathLength = GetModuleFileNameA(0, modulePath, 260u);
    if (modulePathLength == 0u)
    {
        modulePath[0] = 0;
        return 0;
    }
    modulePath[259] = 0;
    crtProgramPath = modulePath;
    return 1;
}

int AS2RuntimeInitializeStandalone()
{
#if !defined(_WIN32)
    return 1;
#else

    CrtInitLocks();
    if(!RecordDBInitializeVirtualOwners())return 0;


    if(!PublishCrtModulePath())return 0;
    if(!RunCrtStaticInitializers()){

        return 0;
    }
    return 1;
#endif
}

#include <stdarg.h>
#include <float.h>
#include <string.h>
#include "myerror.h"
#include "unit.h"


static unsigned int __cdecl MainSwGlobalStringStaticInit();
static void __cdecl MainSwGlobalStringInitialize()
{
    ::new (synchroErrorLogFileNameStorage.Address()) STRING();
}
static unsigned int __cdecl MainSwGlobalStringRegisterCleanup()
{
    return RegisterCrtAtExit(&DialogServiceGlobalStringCleanup);
}
static unsigned int __cdecl MainSwGlobalStringStaticInit()
{
    MainSwGlobalStringInitialize();
    return MainSwGlobalStringRegisterCleanup();
}

void __cdecl DialogServiceGlobalStringCleanup()
{
    synchroErrorLogFileNameStorage.Get().~STRING();
}

void __cdecl DialogGlobalStringCleanup()
{
    dialogPasswordStorage.Get().~STRING();
}


