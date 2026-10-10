
#include "myerror.h"
#include "engine_globals.h"
#include "mylib.h"
#include "net.h"
#include "basetype.h"
#include "map.h"
#include <new>
#include <exception>
#include "zm/Utils/zStaticStorage.h"
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace {
enum NetPacketType : unsigned int
{
    NET_PACKET_BINARY = 0u,
    NET_PACKET_GAME_START = 134u,
    NET_PACKET_GAME_END = 135u,
    NET_PACKET_CHAT = 136u,
    NET_PACKET_STRING = 137u
};

enum NetDirectPlayMessageId : unsigned int
{
    NET_MSG_INDICATE_CONNECT = 0xFFFF000Eu,
    NET_MSG_ENUM_HOSTS_QUERY = 0xFFFF000Au,
    NET_MSG_ENUM_HOSTS_RESPONSE = 0xFFFF000Bu,
    NET_MSG_DESTROY_PLAYER = 0xFFFF0009u,
    NET_MSG_CREATE_PLAYER = 0xFFFF0007u,
    NET_MSG_RECEIVE = 0xFFFF0011u,
    NET_MSG_HOST_MIGRATE = 0xFFFF000Du,
    NET_MSG_PEER_INFO = 0xFFFF0010u,
    NET_MSG_TERMINATE_SESSION = 0xFFFF0016u
};
}

static void CopyDirectPlayQueueBytes(void* destination,const void* source,unsigned int size);


NET* Net = 0;
unsigned int networkTransmissionId = 0;
IDirectPlay8Peer* g_pDP = 0;
IDirectPlay8LobbiedApplication* g_pLobbyApp = 0;
IDirectPlay8Address* g_pDeviceAddress = 0;
IDirectPlay8Address* g_pHostAddress = 0;
DPNHANDLE g_hLobbyHandle = 0;
DPNID g_dpnidLocalPlayer = 0;
DPNID g_dpnidHostPlayer = 0;
DPNHANDLE hAsyncCancelEnumHosts = 0;
LONG g_lNumberOfActivePlayers = 0;
int g_NumberOfHost = 0;
CRITICAL_SECTION g_csHostList;

GUID g_guidApp = {0x7247AD9Du, 0x2607u, 0x40B0u,
    {0xBAu, 0x31u, 0x63u, 0x1Fu, 0xF2u, 0x4Au, 0x2Eu, 0x6Du}};

namespace {
const GUID CLSID_DirectPlay8Address = {0x934A9523u, 0xA3CAu, 0x4BC5u,
    {0xADu, 0xA0u, 0xD6u, 0xD9u, 0x5Du, 0x97u, 0x94u, 0x21u}};
const GUID IID_IDirectPlay8Address = {0x83783300u, 0x4063u, 0x4C8Au,
    {0x9Du, 0xB3u, 0x82u, 0x83u, 0x0Au, 0x7Fu, 0xEBu, 0x31u}};
const GUID CLSID_DP8SP_TCPIP = {0xEBFE7BA0u, 0x628Du, 0x11D2u,
    {0xAEu, 0x0Fu, 0x00u, 0x60u, 0x97u, 0xB0u, 0x14u, 0x11u}};
const GUID CLSID_DirectPlay8Peer = {0x286F484Du, 0x375Eu, 0x4458u,
    {0xA2u, 0x72u, 0xB1u, 0x38u, 0xE2u, 0xF8u, 0x0Au, 0x6Au}};
const GUID IID_IDirectPlay8Peer = {0x5102DACFu, 0x241Bu, 0x11D3u,
    {0xAEu, 0xA7u, 0x00u, 0x60u, 0x97u, 0xB0u, 0x14u, 0x11u}};
const GUID CLSID_DirectPlay8LobbiedApplication = {0x667955ADu, 0x6B3Bu, 0x43CAu,
    {0xB9u, 0x49u, 0xBCu, 0x69u, 0xB5u, 0xBAu, 0xFFu, 0x7Fu}};
const GUID IID_IDirectPlay8LobbiedApplication = {0x819074A3u, 0x016Cu, 0x11D3u,
    {0xAEu, 0x14u, 0x00u, 0x60u, 0x97u, 0xB0u, 0x14u, 0x11u}};

struct HOST_RESPONSE
{
    unsigned int checkListCrc;
    char playerNames[4092];
};
HOST_RESPONSE hostResponse;
}

struct HOST_NODE
{
    HOST_NODE();
    ~HOST_NODE() noexcept(false);

    DPN_APPLICATION_DESC applicationDesc;
    IDirectPlay8Address* address;
    STRING sessionName;
    unsigned int latencyMs;
    STRING playerNames;
    unsigned int checkListCrc;
};

StaticObjectStorage<HOST_NODE[16]> g_HostList;

HOST_NODE::HOST_NODE()
{
    address = 0;
}

HOST_NODE::~HOST_NODE() noexcept(false)
{
    if (address)
    {
        (void)address->Release();
        address = 0;
    }
}

void __cdecl NetHostListConstruct()
{
    HOST_NODE* const nodes = g_HostList.Get();
    unsigned int constructed = 0;
    try
    {
        for (; constructed < 16; ++constructed)
            ::new (static_cast<void*>(nodes + constructed)) HOST_NODE;
    }
    catch (...)
    {
        try
        {
            while (constructed != 0)
                nodes[--constructed].~HOST_NODE();
        }
        catch (...)
        {
            std::terminate();
        }
        throw;
    }
}

void __cdecl NetHostListCleanup()
{
    HOST_NODE* const nodes = g_HostList.Get();
    unsigned int remaining = 16;
    try
    {
        while (remaining != 0)
            nodes[--remaining].~HOST_NODE();
    }
    catch (...)
    {
        try
        {
            while (remaining != 0)
                nodes[--remaining].~HOST_NODE();
        }
        catch (...)
        {
            std::terminate();
        }
        throw;
    }
}

unsigned int __cdecl NetHostListRegisterCleanup()
{
    return RegisterCrtAtExit(&NetHostListCleanup);
}

unsigned int __cdecl NetHostListStaticInit()
{
    NetHostListConstruct();
    return NetHostListRegisterCleanup();
}

static void DirectPlayLogHr(const char* messageText, int category, long errorCode);

NET::NET(HINSTANCE__* hInstance, STRING title)
{
    (void)hInstance;
    (void)title;

    unsigned int initialFlags = m_flags;
    initialFlags &= 0xFFFFFFFEu;
    initialFlags &= 0xFFFFFF00u;
    m_receiveWrite = 0u;
    m_receiveRead = 0u;
    m_noPlayers = 0u;
    m_localPlayer = 0u;
    m_flags = initialFlags;
    static_cast<void>(::Net = this);
    m_chatEnd = 0u;

    for (unsigned int playerIndex = 0;playerIndex<8u;++playerIndex)
    {
        m_players[playerIndex].m_dpnid = 0u;
        m_players[playerIndex].m_chatColor = 0u;
        m_players[playerIndex].m_0C = 0u;
        m_players[playerIndex].m_pendingDestroy = 0u;
        m_players[playerIndex].m_roundTripLatencyMs = 0u;
    }

    (void)InitializeCriticalSection(&g_csHostList);
    const int directPlayResult = static_cast<int>(InitDirectPlay());
    if (directPlayResult >= 0)
    {
        if ((m_flags&0x04u) != 0u)
        {
            const int lobbyResult = static_cast<int>(LobbyLaunch());
            if (lobbyResult<0)
            {
                STRING message;
                STRING category;
                message = "Failed to LobbyLaunch for NET()";
                category = "NET";
                (void)::Error->Error(
                    category, E_ERROR, message, static_cast<unsigned int>(lobbyResult));
                m_flags &= 0xFFFFFFFBu;
            }
            else m_flags |= 1u;
        }
        else m_flags |= 1u;
    }
}

NET::NET_PLAYER::NET_PLAYER()
{
}

NET::NET_PLAYER::~NET_PLAYER()
{
}

NET::~NET() noexcept(false)
{
    ClearHostList();
    if ((m_flags&8u) != 0u)(void)GameDisconnect();

    IDirectPlay8Peer* peer = g_pDP;
    if (peer)(void)peer->Close(0u);
    IDirectPlay8LobbiedApplication* lobby = g_pLobbyApp;
    if (lobby)(void)lobby->Close(0u);

    IDirectPlay8Address* address = g_pDeviceAddress;
    if (address)
    {
        (void)address->Release();
        (void)(g_pDeviceAddress = 0);
    }
    address = g_pHostAddress;
    if (address)
    {
        (void)address->Release();
        (void)(g_pHostAddress = 0);
    }
    peer = g_pDP;
    if (peer)
    {
        (void)peer->Release();
        (void)(g_pDP = 0);
    }
    lobby = g_pLobbyApp;
    if (lobby)
    {
        (void)lobby->Release();
        (void)(g_pLobbyApp = 0);
    }

    (void)DeleteCriticalSection(&g_csHostList);
}

unsigned int __cdecl Session_StaticCleanupRegister02()
{
    return RegisterCrtAtExit(&MapStaticCleanupNoOp);
}

void NET::GameEnd()
{
    if ((m_flags&1u) != 0u)(void)SendGuaranteed(0, 0u, NET_PACKET_GAME_END, -1);
}

void NET::NetworkOff()
{
    if ((m_flags&1u) != 0u)
    {
        if ((m_flags&8u) != 0u)(void)GameDisconnect();
        m_flags &= ~0x20u;
    }
}

void NET::SetChatColor(int playerIndex, unsigned long chatColor)
{
    if (playerIndex >= 0 && playerIndex<8)m_players[playerIndex].m_chatColor = static_cast<unsigned int>(chatColor);
}


void NET::GameStart(STRING mapName)
{
    if ((m_flags&1u) == 0u)return;
    m_receiveWrite = 0u;
    m_receiveRead = 0u;
    const char* const mapNameChars = mapName.m_value;
    unsigned int mapNameLength = 0u;
    while (mapNameChars[mapNameLength] != 0)++mapNameLength;
    (void)SendGuaranteed(mapNameChars, mapNameLength+1u, NET_PACKET_GAME_START, -1);
}

unsigned long NET::Error(TYPE_ERROR errorType, const STRING& message, unsigned long errorDetail) const
{
    STRING category("NET");
    ::Error->Error(category, errorType, message, errorDetail);
    return errorDetail;
}


void NET::NetworkOn()
{
    if ((m_flags&1u) == 0u)return;
    if ((m_flags&8u) != 0u)(void)GameDisconnect();
    CRC32 checkListCrc;
    checkListCrc.value = 0u;
    STRING checkListFile("maps\\check_list.lst");
    (void)checkListCrc.CheckListFromFile(checkListFile);
    m_checkListCrc = checkListCrc.value;
    m_flags &= ~2u;
    m_flags |= 0x20u;
    m_flags &= ~0x80u;
    m_flags &= ~0x1Cu;
    m_receiveWrite = 0u;
    m_receiveRead = 0u;
    m_noPlayers = 0u;
    m_localPlayer = 0u;
    m_chatEnd = 0u;
    for (unsigned int playerIndex = 0;playerIndex<8u;++playerIndex)
    {
        m_players[playerIndex].m_dpnid = 0u;
        m_players[playerIndex].m_chatColor = 0u;
        m_players[playerIndex].m_0C = 0u;
        m_players[playerIndex].m_pendingDestroy = 0u;
    }
}

void NET::SendChat(STRING chatMessage)
{
    if ((m_flags&1u) == 0u)return;
    chatMessage.RemoveEndChars(" \n\r\t");
    const char* const chatChars = chatMessage.m_value;
    if (CompareCStringsUnchecked(chatChars, StringGlobals::EmptyText()) != 0)
    {
        const unsigned int messageBytes = CStringLength(chatChars)+1u;
        (void)Send(chatChars, messageBytes, NET_PACKET_CHAT, -1);
    }
}
const STRING WideCharToString(const unsigned short* wideText)
{
    if (wideText == 0)
        return STRING();

    char convertedText[4096];
    WideCharToMultiByte(0, 0, wideText, -1, convertedText, 4096, 0, 0);
    return STRING(convertedText);
}

long NET::GameJoinByName(STRING gameName)
{
    if ((m_flags&1u) == 0u)
    {
        return E_FAIL;
    }

    const char* const requestedName = gameName.m_value;
    const int hostCount = static_cast<int>(g_NumberOfHost);
    for (int hostIndex = 0;hostIndex<hostCount;++hostIndex)
    {
        const char* const sessionName = g_HostList.Get()[hostIndex].sessionName.value;
        if (CompareCStrings(sessionName, requestedName) == 0)
        {
            const long joinResult = GameJoin(hostIndex);
            return joinResult;
        }
    }

    STRING message = "Can't join with game '"+gameName;
    STRING category("NET");
    (void)::Error->Error(category, E_ERROR, message, 0u);
    return E_FAIL;
}


long NET::GameDisconnect()
{
    if ((m_flags&1u) == 0u)return E_FAIL;
    CancelHostSearch();
    ClearHostList();
    m_noPlayers = 0u;
    m_localPlayer = 0u;
    m_flags &= ~0x1Au;
    IDirectPlay8Peer* peer = g_pDP;
    if (peer)
    {
        (void)peer->Close(0u);
        peer = g_pDP;
    }
    const HRESULT initializeResult = peer->Initialize(0, &DirectPlayMessageHandler, 0u);
    if ((int)initializeResult >= 0)return 0u;
    STRING message("DirectPlay in GameDisconnect");
    STRING category("NET");
    (void)::Error->Error(category, E_INIT, message, initializeResult);
    return initializeResult;
}

void NET::SendString(int messageType, const STRING& messageText)
{
    if ((m_flags&1u) == 0u)return;
    if (messageType >= 0 && messageType <= 19)
    {
        char typePrefix[2];
        typePrefix[0] = static_cast<char>(messageType+'0');
        typePrefix[1] = 0;
        STRING packetText = typePrefix+messageText;
        const unsigned int packetBytes = CStringLength(packetText.m_value)+1u;
        (void)Send(packetText.m_value, packetBytes, NET_PACKET_STRING, -1);
        return;
    }
    STRING packetText("type in SendString");
    STRING category("NET");
    (void)::Error->Error(category, E_INVALID, packetText, static_cast<unsigned int>(messageType));
}

void NET::SetPlayerName(STRING playerName)
{
    if ((m_flags&1u) == 0u)return;
    unsigned short wideName[128] = {0};
    playerName.ToWideChar(wideName, 128);
    DPN_PLAYER_INFO playerInfo = {};
    playerInfo.dwSize = 24u;
    playerInfo.dwInfoFlags = 1u;
    playerInfo.pwszName = wideName;
    IDirectPlay8Peer* const peer = g_pDP;
    const HRESULT setNameResult = peer->SetPeerInfo(&playerInfo, 0, 0, 0x80000000u);
    if (setNameResult<0)
    {
        STRING message("SetPlayerName");
        STRING category("NET");
        (void)::Error->Error(category, E_SET, message, static_cast<unsigned int>(setNameResult));
    }
    m_players[m_localPlayer].m_name = playerName;
}

STRING NET::ReceiveString(int typeDigit, int playerFilter)
{
    if ((m_flags&1u) == 0u)
        return STRING();
    while ((static_cast<const volatile unsigned int&>(m_flags)&0x80u) != 0u)
    {
    }

    unsigned int matchedIndex = 64u;
    const unsigned int readIndex = m_receiveRead;
    const unsigned int writeIndex = m_receiveWrite;
    unsigned int queueIndex = readIndex;
    while (queueIndex != writeIndex)
    {
        const unsigned char packetType = m_receiveType[queueIndex];
        if (packetType == 0u) break;
        if ((playerFilter == -1 || playerFilter == static_cast<int>(m_receiveSender[queueIndex])) && packetType == NET_PACKET_STRING)
        {
            const unsigned char typeCharacter = m_receiveData[queueIndex][0];
            if (static_cast<int>(typeCharacter)-'0' == typeDigit)
            {
                matchedIndex = queueIndex;
                break;
            }
        }
        queueIndex = (queueIndex+1u)&63u;
    }
    if (matchedIndex == 64u)
        return STRING();

    (void)EnterCriticalSection(&g_csHostList);
    const char* messageText = reinterpret_cast<const char*>(&m_receiveData[matchedIndex][1]);
    STRING receivedText(messageText);

    queueIndex = matchedIndex;
    while (queueIndex != m_receiveRead)
    {
        const unsigned int previousIndex = (queueIndex+63u)&63u;
        const unsigned int messageBytes = m_receiveSize[previousIndex];
        CopyDirectPlayQueueBytes(m_receiveData[queueIndex], m_receiveData[previousIndex], messageBytes);
        m_receiveSize[queueIndex] = m_receiveSize[previousIndex];
        m_receiveSender[queueIndex] = m_receiveSender[previousIndex];
        m_receiveType[queueIndex] = m_receiveType[previousIndex];
        queueIndex = previousIndex;
    }
    m_receiveRead = (m_receiveRead+1u)&63u;
    (void)LeaveCriticalSection(&g_csHostList);
    return STRING(receivedText.value);
}


void NET::DestroyPlayer(int playerIndex)
{
    if ((m_flags&1u) == 0u)return;
    if (playerIndex<0 || playerIndex >= 8)
    {
        STRING subject("n_army for DestroyPlayer");
        STRING object("NET");
        ::Error->Error(object, E_INVALID, subject, static_cast<unsigned long>(playerIndex));
        return;
    }
    IDirectPlay8Peer* const peer = g_pDP;
    const DPNID playerId = m_players[playerIndex].m_dpnid;
    const HRESULT destroyResult = peer->DestroyPeer(playerId, 0, 0u, 0u);
    if (destroyResult<0)
    {
        STRING subject("DestroyPlayer");
        STRING object("NET");
        ::Error->Error(object, E_ERROR, subject, static_cast<unsigned long>(destroyResult));
    }
}

DPN_PLAYER_INFO* GetPeerInfo(DPNID peerId)
{


    IDirectPlay8Peer* peer = g_pDP;
    if (!peer)return 0;
    DWORD bufferBytes = 0u;
    HRESULT queryResult = peer->GetPeerInfo(peerId, 0, &bufferBytes, 0u);
    if (((int)queryResult<0 && queryResult != 0x80158100u) || bufferBytes == 0u)
    {
        STRING subject("Failed GetPeerInfo:DirectPlayMessageHandler");
        STRING category("NET");
        ::Error->Error(category, E_GET, subject, queryResult);
        return 0;
    }
    DPN_PLAYER_INFO* const playerInfo = static_cast<DPN_PLAYER_INFO*>(CrtNew(bufferBytes));
    unsigned char* const bufferData = reinterpret_cast<unsigned char*>(playerInfo);
    for (unsigned int byteIndex = 0u;byteIndex<bufferBytes;++byteIndex)bufferData[byteIndex] = 0u;
    playerInfo->dwSize = sizeof(DPN_PLAYER_INFO);
    peer = g_pDP;
    queryResult = peer->GetPeerInfo(peerId, playerInfo, &bufferBytes, 0u);
    if (static_cast<int>(queryResult) >= 0)
        return playerInfo;
    {
        STRING subject("Failed GetPeerInfo2:DirectPlayMessageHandler");
        STRING category("NET");
        ::Error->Error(category, E_GET, subject, queryResult);
    }
    CrtFreePointer(playerInfo);
    return 0;
}

long NET::CreateDeviceAddress()
{
    IDirectPlay8Address* oldAddress = g_pDeviceAddress;
    if (oldAddress)
    {
        (void)oldAddress->Release();
        (void)(g_pDeviceAddress = 0);
    }

    if ((m_flags&0x01u) == 0u)
        return E_FAIL;

    unsigned int addressResult = CoCreateInstance(CLSID_DirectPlay8Address, 0, 1u,
        IID_IDirectPlay8Address, reinterpret_cast<void**>(&g_pDeviceAddress));
    if (static_cast<int>(addressResult)<0)
    {
        DirectPlayLogHr("IDirectPlay8Address", 3, static_cast<long>(addressResult));
        return static_cast<long>(addressResult);
    }

    IDirectPlay8Address* address = g_pDeviceAddress;
    addressResult = address->SetSP(&CLSID_DP8SP_TCPIP);
    if (static_cast<int>(addressResult)<0)
    {
        DirectPlayLogHr("the Service Provider", 8, static_cast<long>(addressResult));
        return static_cast<long>(addressResult);
    }

    if ((m_flags&0x02u) == 0u)
        return static_cast<long>(addressResult);

    address = g_pDeviceAddress;
    unsigned int localPort = 2588u;
    addressResult = address->AddComponent(L"port", &localPort, 4u, 2u);
    if (static_cast<int>(addressResult)<0)
        DirectPlayLogHr("the local port", 8, static_cast<long>(addressResult));
    return static_cast<long>(addressResult);
}


long NET::GameCreate(STRING gameName)
{

    if ((m_flags&1u) == 0u)
    {
        return E_FAIL;
    }


    m_flags = (m_flags&~0x48u)|0x02u;
    for (unsigned int initializationIndex = 0;initializationIndex<8u;++initializationIndex)
    {
        m_players[initializationIndex].m_dpnid = 0u;
        m_players[initializationIndex].m_chatColor = 0u;
        m_players[initializationIndex].m_0C = 0u;
        m_players[initializationIndex].m_pendingDestroy = 0u;
    }

    CancelHostSearch();

    unsigned short wideGameName[128];
    for (unsigned int initializationIndex = 0;initializationIndex<128u;++initializationIndex)wideGameName[initializationIndex] = 0u;
    gameName.ToWideChar(wideGameName, 128);

    unsigned int hostResult = CreateDeviceAddress();
    if (static_cast<int>(hostResult)<0)
    {
        DirectPlayLogHr("device address for GameCreate", 3, static_cast<long>(hostResult));
        return hostResult;
    }

    DPN_APPLICATION_DESC applicationDesc = {};
    applicationDesc.dwSize = 72u;
    applicationDesc.dwFlags = 64u;
    applicationDesc.guidApplication = g_guidApp;
    
    applicationDesc.dwMaxPlayers = 8u;
    applicationDesc.pwszSessionName = wideGameName;

    IDirectPlay8Peer* const peer = g_pDP;
    hostResult = peer->Host(&applicationDesc, &g_pDeviceAddress, 1u, 0, 0, 0, 0u);
    if (static_cast<int>(hostResult)<0)
    {
        DirectPlayLogHr("Failed Host in GameCreate()", 10, static_cast<long>(hostResult));
        return hostResult;
    }

    m_flags |= 0x08u;
    return 0u;
}

STRING NET::GetGameList(int hostIndex) const
{
    if ((m_flags&1u) == 0u || hostIndex<0 || hostIndex >= static_cast<int>(g_NumberOfHost))
        return STRING();

    STRING gameListText;
    (void)EnterCriticalSection(&g_csHostList);

    const HOST_NODE& hostNode = g_HostList.Get()[hostIndex];
    
    DWORD urlChars = 0u;
        HRESULT urlResult = hostNode.address->GetURLW(0, &urlChars);
        if (urlResult == 0x80158100u)
    {
            unsigned short* wideUrl = static_cast<unsigned short*>(::operator new(urlChars*2u));
            char* temporaryBuffer = static_cast<char*>(::operator new(urlChars));
            if (wideUrl && temporaryBuffer)
        {
                urlResult = hostNode.address->GetURLW(wideUrl, &urlChars);
                if (static_cast<int>(urlResult) >= 0)
            {
                    STRING hostUrl = WideCharToString(wideUrl);
                    STRING hostUrlTail = hostUrl.After("hostname=");
                    STRING hostDisplay = hostUrlTail.Before(";");
                    hostDisplay += "/";
                    hostDisplay += hostNode.sessionName;
                    gameListText += hostDisplay;
                    gameListText += Printf("  %ims %i/6",
                        static_cast<int>(hostNode.latencyMs),
                        static_cast<int>(hostNode.applicationDesc.dwCurrentPlayers));
                    if (hostNode.checkListCrc != m_checkListCrc)
                        gameListText += " err";
                }
            }
            if (temporaryBuffer) ::operator delete(temporaryBuffer);
            if (wideUrl) ::operator delete(wideUrl);
        }

    (void)LeaveCriticalSection(&g_csHostList);
    return gameListText;
}

STRING NET::GetChat(int historyOffset) const
{
    if ((m_flags&1u) == 0u)
        return STRING();

    STRING chatHistory;
    unsigned int chatIndex = (m_chatEnd-static_cast<unsigned int>(historyOffset))&0xFFu;
    while (chatIndex != m_chatEnd)
    {
        if (CompareCStringsUnchecked(m_chatText[chatIndex].value, StringGlobals::EmptyText()) != 0)
        {
            const NET_PLAYER& playerRecord = m_players[m_chatPlayer[chatIndex]];
            const unsigned int playerId = playerRecord.m_dpnid;
            const unsigned int chatColor = playerId == 0u ? 0xFF606060u : playerRecord.m_chatColor;

            if (chatColor != 0u)
            {
                STRING colorValue = Int2Str(static_cast<int>(chatColor));
                STRING colorPrefix = "<Color="+colorValue;
                const STRING colorTag = colorPrefix+">";
                chatHistory += colorTag;
            }

            {
                const STRING namePrefix = m_players[m_chatPlayer[chatIndex]].m_name.SubString(0u, 3u);
                const STRING nameTag = namePrefix+"> ";
                chatHistory += nameTag;
            }

            if (m_players[m_chatPlayer[chatIndex]].m_chatColor != 0u)
                chatHistory += "<Color=0>";

            const STRING chatLine = m_chatText[chatIndex]+"\r\n";
            chatHistory += chatLine;
        }
        chatIndex = (chatIndex+1u)&0xFFu;
    }
    return STRING(chatHistory.value);
}

long NET::CreateHostAddress(STRING hostAddress)
{
    IDirectPlay8Address* oldAddress = g_pHostAddress;
    if (oldAddress)
    {
        (void)oldAddress->Release();
        (void)(g_pHostAddress = 0);
    }

    if ((m_flags&0x01u) == 0u)
    {
        return E_FAIL;
    }

    unsigned int addressResult = CoCreateInstance(CLSID_DirectPlay8Address, 0, 1u,
        IID_IDirectPlay8Address, reinterpret_cast<void**>(&g_pHostAddress));
    if (static_cast<int>(addressResult)<0)
    {
        DirectPlayLogHr("IDirectPlay8Address for CreateHostAddress", 3, static_cast<long>(addressResult));
        return static_cast<long>(addressResult);
    }

    IDirectPlay8Address* address = g_pHostAddress;
    addressResult = address->SetSP(&CLSID_DP8SP_TCPIP);
    if (static_cast<int>(addressResult)<0)
    {
        DirectPlayLogHr("Service Provider for CreateHostAddress", 8, static_cast<long>(addressResult));
        return static_cast<long>(addressResult);
    }

    unsigned short wideHostName[256];
    for (unsigned int charIndex = 0;charIndex<256u;++charIndex)wideHostName[charIndex] = 0u;
    hostAddress.ToWideChar(wideHostName, 256);

    address = g_pHostAddress;
    unsigned int hostNameLength = 0u;
    while (wideHostName[hostNameLength] != 0u)++hostNameLength;
    if (hostNameLength != 0u)
    {
        const unsigned int hostNameBytes = hostNameLength*2u+2u;
        addressResult = address->AddComponent(L"hostname", wideHostName, hostNameBytes, 1u);
        if (static_cast<int>(addressResult)<0)
        {
            DirectPlayLogHr("the host_name for CreateHostAddress", 8, static_cast<long>(addressResult));
            return static_cast<long>(addressResult);
        }
    }

    address = g_pHostAddress;
    unsigned int localPort = 2588u;
    addressResult = address->AddComponent(L"port", &localPort, 4u, 2u);
    if (static_cast<int>(addressResult)<0)
    {
        DirectPlayLogHr("the local port for CreateHostAddress", 8, static_cast<long>(addressResult));
        return static_cast<long>(addressResult);
    }

    return 0;
}

static void DirectPlayReportInitFailure(const char* messageText, unsigned int errorType, unsigned int errorResult)
{
    STRING message;
    STRING category;
    message = messageText;
    category = "NET";
    (void)::Error->Error(category, static_cast<TYPE_ERROR>(errorType), message, errorResult);
}

long NET::InitDirectPlay()
{


    unsigned int initializeResult = CoCreateInstance(CLSID_DirectPlay8Peer, 0, 1u,
        IID_IDirectPlay8Peer, reinterpret_cast<void**>(&g_pDP));
    if (static_cast<int>(initializeResult)<0)
    {
        DirectPlayReportInitFailure("DirectPlay8Peer", 3u, initializeResult);
        return initializeResult;
    }


    initializeResult = CoCreateInstance(CLSID_DirectPlay8LobbiedApplication, 0, 1u,
        IID_IDirectPlay8LobbiedApplication, reinterpret_cast<void**>(&g_pLobbyApp));
    if (static_cast<int>(initializeResult)<0)
    {
        DirectPlayReportInitFailure("DirectPlay8LobbiedApplication", 3u, initializeResult);
        return initializeResult;
    }

    IDirectPlay8Peer* peer = g_pDP;
    initializeResult = peer->Initialize(0, &DirectPlayMessageHandler, 0u);
    if (static_cast<int>(initializeResult)<0)
    {
        DirectPlayReportInitFailure("DirectPlay", 12u, initializeResult);
        return initializeResult;
    }

    IDirectPlay8LobbiedApplication* lobby = g_pLobbyApp;
    initializeResult = lobby->Initialize(0, &LobbyAppMessageHandler, &g_hLobbyHandle, 0u);
    if (static_cast<int>(initializeResult)<0)
    {
        DirectPlayReportInitFailure("Lobby", 12u, initializeResult);
        return initializeResult;
    }

    if (!IsServiceProviderValid(&CLSID_DP8SP_TCPIP))
    {
        STRING text;
        text = "CLSID_NETWORKSIMULATOR_DP8SP_TCPIP You must have TCP/IP installed on your";
        (void)Error(E_INVALID, text, 0u);
        return E_FAIL;
    }

    unsigned int lobbyFlags = m_flags&~0x04u;
    if (g_hLobbyHandle != 0u)lobbyFlags |= 0x04u;
    m_flags = lobbyFlags;
    return 0u;
}

long NET::LobbyLaunch()
{
    IDirectPlay8LobbiedApplication* const lobby = g_pLobbyApp;
    DWORD settingsBytes = 0u;
    HRESULT settingsResult = lobby->GetConnectionSettings(g_hLobbyHandle, 0, &settingsBytes, 0u);
    if (settingsResult != 0x80158100u)
    {
        DirectPlayReportInitFailure("first GetConnectionSettings", 9u, settingsResult);
        return settingsResult;
    }

    DPL_CONNECTION_SETTINGS* const settings = static_cast<DPL_CONNECTION_SETTINGS*>(CrtNew(settingsBytes));
    if (!settings)
    {
        DirectPlayReportInitFailure("Allocating Buffer", 2u, 0u);
        return E_FAIL;
    }
    IDirectPlay8LobbiedApplication* const currentLobby = g_pLobbyApp;
    settingsResult = currentLobby->GetConnectionSettings(g_hLobbyHandle, settings, &settingsBytes, 0u);
    unsigned int sessionResult = settingsResult;
    if (static_cast<int>(settingsResult)<0)
    {
        DirectPlayReportInitFailure("second GetConnectionSettings", 9u, settingsResult);
    }
    else
    {
        IDirectPlay8Peer* const peer = g_pDP;
        if ((settings->dwFlags&1u) != 0u)
        {
            sessionResult = peer->Host(&settings->dpnAppDesc, settings->ppdp8DeviceAddresses, settings->cNumDeviceAddresses, 0, 0, 0, 0u);
            if (static_cast<int>(sessionResult)<0)
            {
                DirectPlayReportInitFailure("Failed Host in LobbyLaunch()", 10u, sessionResult);
            }
            else
            {
                m_flags |= 0x02u;
            }
        }
        else
        {
            sessionResult = peer->Connect(&settings->dpnAppDesc, settings->pdp8HostAddress, settings->ppdp8DeviceAddresses[0], 0, 0, 0, 0u, 0, 0, 0, 0x80000000u);
            if (static_cast<int>(sessionResult)<0)
                DirectPlayReportInitFailure("Failed Lobby App Connect", 10u, sessionResult);
        }
        if (static_cast<int>(sessionResult) >= 0)
        {
            STRING playerName = WideCharToString(settings->dpnAppDesc.pwszSessionName);
            m_playerName = playerName;
            m_flags |= 0x08u;
        }
    }

    if (settings->pdp8HostAddress)
    {
        (void)settings->pdp8HostAddress->Release();
        settings->pdp8HostAddress = 0;
    }
    for (unsigned int addressIndex = 0u;addressIndex<settings->cNumDeviceAddresses;++addressIndex)
    {
        if (settings->ppdp8DeviceAddresses[addressIndex])
        {
            (void)settings->ppdp8DeviceAddresses[addressIndex]->Release();
            settings->ppdp8DeviceAddresses[addressIndex] = 0;
        }
    }
    CrtFreePointer(settings);
    return sessionResult;
}

long NET::GameJoin(int gameIndex)
{
    const unsigned int hostIndex = static_cast<unsigned int>(gameIndex);
    const HRESULT failureResult = E_FAIL;
    if ((m_flags&1u) == 0u)return failureResult;

    m_flags &= 0xFFFFFFB5u;
    for (unsigned int playerIndex = 0u;playerIndex<8u;++playerIndex)
    {
        m_players[playerIndex].m_dpnid = 0u;
        m_players[playerIndex].m_chatColor = 0u;
        m_players[playerIndex].m_0C = 0u;
        m_players[playerIndex].m_pendingDestroy = 0u;
    }

    const unsigned int enumeratedCount = static_cast<unsigned int>(g_NumberOfHost);
    if (hostIndex >= enumeratedCount)
    {
        STRING subject("number_in_gamelist");
        STRING object("NET");
        ::Error->Error(object, E_INVALID, subject, hostIndex);
        return failureResult;
    }

    const HOST_NODE& hostNode = g_HostList.Get()[hostIndex];
    if (hostNode.checkListCrc != m_checkListCrc)return failureResult;

    unsigned int joinResult = CreateDeviceAddress();
    if ((int)joinResult<0)
    {
        STRING subject("device address for GameJoin");
        STRING object("NET");
        ::Error->Error(object, E_CREATE, subject, joinResult);
        return joinResult;
    }

    DPN_APPLICATION_DESC applicationDesc = {};
    applicationDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
    applicationDesc.guidApplication = g_guidApp;

    IDirectPlay8Address* duplicatedHostAddress = 0;
    (void)EnterCriticalSection(&g_csHostList);

    joinResult = hostNode.address->Duplicate(&duplicatedHostAddress);

    if ((int)joinResult >= 0)
    {
        IDirectPlay8Peer* const peer = g_pDP;
        joinResult = peer->Connect(&applicationDesc, duplicatedHostAddress,
            g_pDeviceAddress, 0, 0, 0, 0u, 0, 0, 0, 0x80000000u);
        if ((int)joinResult >= 0)
        {
            m_flags |= 0x08u;
            m_playerName = hostNode.sessionName;
            CancelHostSearch();
        }
        else
        {
            STRING subject("Failed Connecting to Host");
            STRING object("NET");
            ::Error->Error(object, E_ERROR, subject, joinResult);
        }
    }
    else
    {
        STRING subject("Failed Duplicating Host Address");
        STRING object("NET");
        ::Error->Error(object, E_ERROR, subject, joinResult);
    }

    (void)LeaveCriticalSection(&g_csHostList);
    if (duplicatedHostAddress != 0u)
    {
        (void)duplicatedHostAddress->Release();
    }
    return joinResult;
}

static void DirectPlayQueueDiagnostic(const char* messageText, int errorCode);


long __stdcall DirectPlayMessageHandler(void* userContext, unsigned long messageId, void* messageBody)
{
    (void)userContext;
    NET* network = ::Net;
    switch (messageId)
    {
    case NET_MSG_INDICATE_CONNECT:
    {

        if ((network->m_flags&0x10u) != 0u)
        {
            ::Error->Log("DPN_MSGID_INDICATE_CONNECT - DPNERR_NOTALLOWED");
            return static_cast<long>(0x80158520u);
        }

        const unsigned int currentTime = ::RealCurrentTime;
        int playerIndex = 3;
        for (;playerIndex<8;++playerIndex)
        {
            const NET::NET_PLAYER& player = network->m_players[playerIndex];
            if (player.m_dpnid == 0u && currentTime-player.m_0C>5000u)
                break;
        }
        const long connectResult = playerIndex<8?0:static_cast<long>(0x80158520u);
        ::Error->Log("DPN_MSGID_INDICATE_CONNECT - player reserved %i", playerIndex);
        return connectResult;
    }
    case NET_MSG_ENUM_HOSTS_QUERY:
    {

        (void)EnterCriticalSection(&g_csHostList);
        ::Error->Log("DPN_MSGID_ENUM_HOSTS_QUERY %u", ::CurrentTime);

        network = ::Net;
        if ((network->m_flags&0x10u) != 0u)
        {
            (void)LeaveCriticalSection(&g_csHostList);
            return static_cast<long>(0x80158520u);
        }

        hostResponse.checkListCrc = network->m_checkListCrc;
        char* const responseText = hostResponse.playerNames;
        unsigned int textLength = 0u;
        responseText[0] = '\0';
        for (unsigned int playerIndex = 0;playerIndex<8u;++playerIndex)
        {
            const NET::NET_PLAYER& player = network->m_players[playerIndex];
            if (player.m_dpnid == 0u)continue;
            const char* playerName = player.m_name.value;
            while (*playerName)responseText[textLength++] = *playerName++;
            responseText[textLength++] = '\n';
            responseText[textLength++] = '\r';
            responseText[textLength] = '\0';
        }
        unsigned int responseTextBytes = textLength+1u;
        if (responseTextBytes>4090u)responseTextBytes = 4090u;
        static_cast<DPNMSG_ENUM_HOSTS_QUERY*>(messageBody)->dwResponseDataSize = responseTextBytes+4u;
        static_cast<DPNMSG_ENUM_HOSTS_QUERY*>(messageBody)->pvResponseData = &hostResponse;
        (void)LeaveCriticalSection(&g_csHostList);
        return 0;
    }
    case NET_MSG_ENUM_HOSTS_RESPONSE:
    {

        int hostCount = static_cast<int>(g_NumberOfHost);
        if (hostCount >= 16)
        {
            STRING message;
            STRING category;
            message = "Too many host session";
            category = "NET";
            (void)::Error->Error(category, E_ERROR, message, static_cast<unsigned int>(hostCount));
            return 0;
        }

        const DPN_APPLICATION_DESC* const applicationDesc = static_cast<DPNMSG_ENUM_HOSTS_RESPONSE*>(messageBody)->pApplicationDescription;
        (void)EnterCriticalSection(&g_csHostList);
        ::Error->Log("DPN_MSGID_ENUM_HOSTS_RESPONSE %u", ::CurrentTime);

        hostCount = static_cast<int>(g_NumberOfHost);
        const unsigned char* const instanceGuidBytes =
            reinterpret_cast<const unsigned char*>(&applicationDesc->guidInstance);
        for (int itemIndex = 0;itemIndex<hostCount;++itemIndex)
        {
            const unsigned char* const existingGuidBytes = reinterpret_cast<const unsigned char*>(&g_HostList.Get()[itemIndex].applicationDesc.guidInstance);
            bool sameInstance = true;
            for (unsigned int guidByteIndex = 0;guidByteIndex<16u;++guidByteIndex)if (instanceGuidBytes[guidByteIndex] != existingGuidBytes[guidByteIndex])
            {
                sameInstance = false;
                break;
            }
            if (sameInstance)
            {
                (void)LeaveCriticalSection(&g_csHostList);
                return 0;
            }
        }

        HOST_NODE& addressNode = g_HostList.Get()[static_cast<unsigned int>(hostCount)];
        IDirectPlay8Address* const responseAddress = static_cast<DPNMSG_ENUM_HOSTS_RESPONSE*>(messageBody)->pAddressSender;
        const HRESULT duplicateHr = responseAddress->Duplicate(&addressNode.address);
        if (duplicateHr<0)
        {
            (void)LeaveCriticalSection(&g_csHostList);
            return 0;
        }

        const unsigned char* const applicationDescBytes = reinterpret_cast<const unsigned char*>(applicationDesc);
        HOST_NODE& hostNode = g_HostList.Get()[g_NumberOfHost];
        unsigned char* const copiedDescBytes = reinterpret_cast<unsigned char*>(&hostNode.applicationDesc);
        for (unsigned int itemIndex = 0;itemIndex<sizeof(DPN_APPLICATION_DESC);++itemIndex)copiedDescBytes[itemIndex] = 0u;
        for (unsigned int itemIndex = 0;itemIndex<sizeof(DPN_APPLICATION_DESC);++itemIndex)copiedDescBytes[itemIndex] = applicationDescBytes[itemIndex];
        hostNode.applicationDesc.pwszSessionName = 0;
        hostNode.applicationDesc.pwszPassword = 0;
        hostNode.applicationDesc.pvReservedData = 0;
        hostNode.applicationDesc.dwReservedDataSize = 0u;
        hostNode.applicationDesc.pvApplicationReservedData = 0;
        hostNode.applicationDesc.dwApplicationReservedDataSize = 0u;

        STRING sessionName = WideCharToString(applicationDesc->pwszSessionName);
        g_HostList.Get()[g_NumberOfHost].sessionName = sessionName;

        unsigned int hostIndex = g_NumberOfHost;
        g_HostList.Get()[hostIndex].latencyMs = static_cast<DPNMSG_ENUM_HOSTS_RESPONSE*>(messageBody)->dwRoundTripLatencyMS;
        if (static_cast<DPNMSG_ENUM_HOSTS_RESPONSE*>(messageBody)->dwResponseDataSize != 0u)
        {
            void* responseData = static_cast<DPNMSG_ENUM_HOSTS_RESPONSE*>(messageBody)->pvResponseData;
            g_HostList.Get()[hostIndex].playerNames = static_cast<const char*>(responseData)+4u;
            hostIndex = g_NumberOfHost;
            g_HostList.Get()[hostIndex].checkListCrc = ReadU32(responseData, 0u);
        }
        (void)(g_NumberOfHost = hostIndex+1u);
        (void)LeaveCriticalSection(&g_csHostList);

        return 0;
    }
    case NET_MSG_DESTROY_PLAYER:
    {

        const unsigned int playerId = static_cast<DPNMSG_DESTROY_PLAYER*>(messageBody)->dpnidPlayer;
        unsigned int playerIndex = 0u;
        for (;playerIndex<8u;++playerIndex)
            if (network->m_players[playerIndex].m_dpnid == playerId)break;

        if (playerIndex<8u)
        {
            NET::NET_PLAYER& playerRecord = network->m_players[playerIndex];
            if (g_dpnidLocalPlayer == playerRecord.m_dpnid)
            {
                playerRecord.m_dpnid = 0u;
                network = ::Net;
            }
            if ((network->m_flags&0x10u) != 0u)
            {
                network->m_players[playerIndex].m_pendingDestroy = 1u;
            }
            else
            {
                network->m_players[playerIndex].m_dpnid = 0u;
                network = ::Net;
                if ((network->m_flags&0x40u) == 0u)
                {
                    for (unsigned int shiftIndex = playerIndex;shiftIndex<7u;++shiftIndex)
                    {
                        network->m_players[shiftIndex].m_dpnid = network->m_players[shiftIndex+1u].m_dpnid;
                        network = ::Net;
                        network->m_players[shiftIndex].m_name = network->m_players[shiftIndex+1u].m_name;
                        network = ::Net;
                        network->m_players[shiftIndex].m_chatColor = network->m_players[shiftIndex+1u].m_chatColor;
                        network = ::Net;

                        unsigned int chatIndex = (network->m_chatEnd+1u)&0xFFu;
                        while (chatIndex != network->m_chatEnd)
                        {
                            const unsigned int chatPlayerIndex = network->m_chatPlayer[chatIndex];
                            if (chatPlayerIndex == shiftIndex)
                            {
                                network->m_chatPlayer[chatIndex] = 7u;
                                network = ::Net;
                            }
                            else if (chatPlayerIndex == shiftIndex+1u)
                            {
                                network->m_chatPlayer[chatIndex] = shiftIndex;
                                network = ::Net;
                            }
                            chatIndex = (chatIndex+1u)&0xFFu;
                        }
                        if (network->m_localPlayer == shiftIndex+1u)
                        {
                            network->m_localPlayer = shiftIndex;
                            network = ::Net;
                        }
                    }
                    network->m_players[7].m_dpnid = 0u;
                }
            }
        }

        ::Error->Log("DPN_MSGID_DESTROY_PLAYER %u %u %u", static_cast<DPNMSG_DESTROY_PLAYER*>(messageBody)->dpnidPlayer, playerIndex, ::CurrentTime);

        switch (static_cast<DPNMSG_DESTROY_PLAYER*>(messageBody)->dwReason)
        {
        case 2u:
            DirectPlayQueueDiagnostic("Connection lost:DirectPlayMessageHandler", 0);
            break;
        case 3u:
            DirectPlayQueueDiagnostic("Session terminated:DirectPlayMessageHandler", 0);
            break;
        case 4u:
            DirectPlayQueueDiagnostic("Host destroyed local player:DirectPlayMessageHandler", 0);
            break;
        default:
            break;
        }

        if (static_cast<DPNMSG_DESTROY_PLAYER*>(messageBody)->dpnidPlayer == g_dpnidLocalPlayer)g_dpnidLocalPlayer = 0u;
        if (static_cast<DPNMSG_DESTROY_PLAYER*>(messageBody)->dpnidPlayer == g_dpnidHostPlayer)
        {
            g_dpnidHostPlayer = 0u;
            network = ::Net;
            network->GameEnd();
        }
        if (g_lNumberOfActivePlayers != 0u)
            (void)InterlockedDecrement(&g_lNumberOfActivePlayers);

        return 0;
    }
    case NET_MSG_CREATE_PLAYER:
    {

        const unsigned int playerId = static_cast<DPNMSG_CREATE_PLAYER*>(messageBody)->dpnidPlayer;
        DPN_PLAYER_INFO* const playerInfo = GetPeerInfo(playerId);
        if (!playerInfo)return 0;
        network = ::Net;

        unsigned int playerIndex = 8u;

        if ((playerInfo->dwPlayerFlags&0x04u) != 0u)
        {

            g_dpnidHostPlayer = static_cast<DPNMSG_CREATE_PLAYER*>(messageBody)->dpnidPlayer;
            playerIndex = 0u;
        }
        else
        {

            for (unsigned int vacantIndex = 3u;vacantIndex<8u;++vacantIndex)
            {
                if (network->m_players[vacantIndex].m_dpnid != 0u || vacantIndex >= 7u)continue;
                for (unsigned int shiftIndex = vacantIndex;shiftIndex<7u;++shiftIndex)
                {
                    network->m_players[shiftIndex].m_dpnid = network->m_players[shiftIndex+1u].m_dpnid;
                    network = ::Net;
                    network->m_players[shiftIndex].m_name = network->m_players[shiftIndex+1u].m_name;
                    network = ::Net;
                    network->m_players[shiftIndex].m_chatColor = network->m_players[shiftIndex+1u].m_chatColor;
                    network = ::Net;
                    {
        unsigned int chatIndex = (network->m_chatEnd+1u)&0xFFu;
        while (chatIndex != network->m_chatEnd)
        {
            const unsigned int chatPlayerIndex = network->m_chatPlayer[chatIndex];
            if (chatPlayerIndex == shiftIndex)
            {
                network->m_chatPlayer[chatIndex] = 7u;
                network = ::Net;
            }
            else if (chatPlayerIndex == shiftIndex+1u)
            {
                network->m_chatPlayer[chatIndex] = shiftIndex;
                network = ::Net;
            }
            chatIndex = (chatIndex+1u)&0xFFu;
        }

    }
                    if (network->m_localPlayer == shiftIndex+1u)
                    {
                        network->m_localPlayer = shiftIndex;
                        network = ::Net;
                    }
                }
                network->m_players[7].m_dpnid = 0u;
                network = ::Net;
            }


            playerIndex = 3u;
            while (playerIndex<8u)
            {
                const unsigned int existingPlayerId = network->m_players[playerIndex].m_dpnid;
                if (existingPlayerId == 0u || static_cast<DPNMSG_CREATE_PLAYER*>(messageBody)->dpnidPlayer<existingPlayerId)break;
                ++playerIndex;
            }
            if (playerIndex <= 6u && network->m_players[playerIndex].m_dpnid != 0u)
            {
                for (int shiftIndex = 6;shiftIndex >= static_cast<int>(playerIndex);--shiftIndex)
                {
                    const unsigned int shiftedPlayerIndex = static_cast<unsigned int>(shiftIndex);
                    network->m_players[shiftedPlayerIndex+1u].m_dpnid = network->m_players[shiftedPlayerIndex].m_dpnid;
                    network = ::Net;
                    network->m_players[shiftedPlayerIndex+1u].m_name = network->m_players[shiftedPlayerIndex].m_name;
                    network = ::Net;
                    network->m_players[shiftedPlayerIndex+1u].m_chatColor = network->m_players[shiftedPlayerIndex].m_chatColor;
                    network = ::Net;
                    {
        unsigned int chatIndex = (network->m_chatEnd+1u)&0xFFu;
        while (chatIndex != network->m_chatEnd)
        {
            if (network->m_chatPlayer[chatIndex] == shiftedPlayerIndex)
            {
                network->m_chatPlayer[chatIndex] = shiftedPlayerIndex+1u;
                network = ::Net;
            }
            chatIndex = (chatIndex+1u)&0xFFu;
        }

    }
                    if (network->m_localPlayer == shiftedPlayerIndex)
                    {
                        network->m_localPlayer = shiftedPlayerIndex+1u;
                        network = ::Net;
                    }
                }
            }

            if (playerIndex >= 8u)
                DirectPlayQueueDiagnostic(
                    "n_player = %i",
                    static_cast<int>(playerIndex));
        }

        if (playerIndex<8u)
        {
            if ((playerInfo->dwPlayerFlags&0x02u) != 0u)
            {
                g_dpnidLocalPlayer = static_cast<DPNMSG_CREATE_PLAYER*>(messageBody)->dpnidPlayer;
                network->m_localPlayer = playerIndex;
                network = ::Net;
            }
            network->m_players[playerIndex].m_dpnid = static_cast<DPNMSG_CREATE_PLAYER*>(messageBody)->dpnidPlayer;
            {
                STRING playerName = WideCharToString(playerInfo->pwszName);
                network = ::Net;
                network->m_players[playerIndex].m_name = playerName;
            }
            network = ::Net;
            network->m_players[playerIndex].m_chatColor = 0u;
            (void)InterlockedIncrement(&g_lNumberOfActivePlayers);
        }

        STRING playerDescription;
        playerDescription = (playerInfo->dwPlayerFlags&0x04u) != 0u ? " host " : StringGlobals::EmptyText();
        if ((playerInfo->dwPlayerFlags&0x02u) != 0u)
            playerDescription += " local ";

        STRING logFormat = "DPN_MSGID_CREATE_PLAYER %u %u"+playerDescription;
        ::Error->Log(logFormat, static_cast<DPNMSG_CREATE_PLAYER*>(messageBody)->dpnidPlayer, playerIndex);
        CrtFreePointer(playerInfo);

        return 0;
    }
    case NET_MSG_RECEIVE:
    {

        (void)EnterCriticalSection(&g_csHostList);
        network = ::Net;

        const unsigned int senderId = static_cast<DPNMSG_RECEIVE*>(messageBody)->dpnidSender;
        unsigned char* packetData = reinterpret_cast<unsigned char*>(static_cast<DPNMSG_RECEIVE*>(messageBody)->pReceiveData);
        const unsigned int packetBytes = static_cast<DPNMSG_RECEIVE*>(messageBody)->dwReceiveDataSize;
        const unsigned int packetType = packetData[0];
        if (packetType == NET_PACKET_BINARY || packetType == NET_PACKET_STRING)
        {
            network->AddReceivedData(packetData+1u, packetBytes-1u, senderId, packetType);
        }
        else if (packetType == NET_PACKET_GAME_START)
        {
            ::Error->Log("DPN_MSGID_RECEIVE game start %s %u", reinterpret_cast<const char*>(packetData+1u), ::CurrentTime);
            Map->PauseOff();
            packetData = reinterpret_cast<unsigned char*>(static_cast<DPNMSG_RECEIVE*>(messageBody)->pReceiveData);
            {
                STRING networkMapName(reinterpret_cast<const char*>(packetData+1u));
                Map->SetNetworkMapName(networkMapName);
            }
            network = ::Net;
            network->m_receiveWrite = 0u;
            network->m_receiveRead = 0u;
            network = ::Net;
            network->m_flags |= 0x10u;
            network = ::Net;
            network->m_flags |= 0x40u;
        }
        else if (packetType == NET_PACKET_GAME_END)
        {
            ::Error->Log("DPN_MSGID_RECEIVE game end %u", ::CurrentTime);
            network = ::Net;
            network->m_flags &= ~0x10u;
        }
        else if (packetType == NET_PACKET_CHAT)
        {
            unsigned int playerIndex = 0u;
            for (;playerIndex<8u;++playerIndex)
                if (network->m_players[playerIndex].m_dpnid == senderId)break;
            if (playerIndex<8u)
            {
                network->m_chatPlayer[network->m_chatEnd] = playerIndex;
                network = ::Net;
            }
            packetData = static_cast<const volatile DPNMSG_RECEIVE*>(messageBody)->pReceiveData;
            network->m_chatText[network->m_chatEnd] = reinterpret_cast<const char*>(packetData+1u);
            network = ::Net;
            network->m_chatEnd = (network->m_chatEnd+1u)&0xFFu;
        }
        (void)LeaveCriticalSection(&g_csHostList);

        return 0;
    }
    case NET_MSG_HOST_MIGRATE:
        (void)EnterCriticalSection(&g_csHostList);
        ::Error->Log("DPN_MSGID_HOST_MIGRATE %u", ::CurrentTime);
        g_dpnidHostPlayer = static_cast<DPNMSG_HOST_MIGRATE*>(messageBody)->dpnidNewHost;
        (void)LeaveCriticalSection(&g_csHostList);
        return 0;
    case NET_MSG_PEER_INFO:
    {

        const unsigned int peerId = static_cast<DPNMSG_PEER_INFO*>(messageBody)->dpnidPeer;
        DPN_PLAYER_INFO* playerInfo = GetPeerInfo(peerId);
        if (!playerInfo)return 0;
        network = ::Net;

        unsigned int playerIndex = 0u;
        for (;playerIndex<8u;++playerIndex)
            if (network->m_players[playerIndex].m_dpnid == static_cast<DPNMSG_PEER_INFO*>(messageBody)->dpnidPeer)break;

        if (playerIndex<8u)
        {
            STRING playerName = WideCharToString(playerInfo->pwszName);
            network = ::Net;
            network->m_players[playerIndex].m_name = playerName;
        }
        else
        {
            DirectPlayQueueDiagnostic(
                "DPN_MSGID_PEER_INFO:DirectPlayMessageHandler() invalid id player", static_cast<int>(static_cast<DPNMSG_PEER_INFO*>(messageBody)->dpnidPeer));
        }
        CrtFreePointer(playerInfo);

        return 0;
    }
    case NET_MSG_TERMINATE_SESSION:
        ClearHostList();
        (void)EnterCriticalSection(&g_csHostList);
        ::Error->Log("DPN_MSGID_TERMINATE_SESSION %u", ::CurrentTime);
        g_lNumberOfActivePlayers = 0u;
        g_dpnidLocalPlayer = 0u;
        g_dpnidHostPlayer = 0u;
        (void)LeaveCriticalSection(&g_csHostList);
        return 0;
    case 0xFFFF0008u:
    case 0xFFFF000Cu:
    case 0xFFFF000Fu:
    case 0xFFFF0012u:
    case 0xFFFF0013u:
    case 0xFFFF0014u:
    case 0xFFFF0015u:
    default:
        return 0;
    }
}

#if !defined(_MSC_VER)
#  if !defined(__stdcall)
#    define __stdcall
#  endif
#endif

void NET::CancelHostSearch()
{
    const unsigned long asyncHandle = hAsyncCancelEnumHosts;
    if (asyncHandle == 0)
        return;

    IDirectPlay8Peer* const peer = g_pDP;
    if (peer == 0)
        return;

    (void)peer->CancelAsyncOperation(asyncHandle, 2u);

    hAsyncCancelEnumHosts = 0u;
}

#if !defined(_MSC_VER)
#  if !defined(__cdecl)
#    define __cdecl
#  endif
#  if !defined(__stdcall)
#    define __stdcall
#  endif
#endif


long __stdcall LobbyAppMessageHandler(
    void*                ,
    unsigned long messageId,
    void* messageBody)
{

    if (messageId != 0x00008002u)
        return 0;

    MYERROR* log =
        ::Error;
    log->Log("DPL_MSGID_CONNECT");

    IDirectPlay8Peer* const peer = g_pDP;
    const DPNHANDLE connectHandle = static_cast<DPL_MESSAGE_CONNECT*>(messageBody)->hConnectId;
    (void)peer->RegisterLobby(connectHandle,
        g_pLobbyApp, 1u);
    return 0;
}

#if !defined(_MSC_VER) && !defined(__stdcall)
#define __stdcall
#endif


static void EnterDirectPlayRuntimeCriticalSection()
{
    EnterCriticalSection(&g_csHostList);
}
static void LeaveDirectPlayRuntimeCriticalSection()
{
    LeaveCriticalSection(&g_csHostList);
}


void ClearHostList()
{
    EnterDirectPlayRuntimeCriticalSection();
    int hostCount = g_NumberOfHost;
    for (int hostIndex = 0; hostIndex < hostCount; ++hostIndex)
    {
        HOST_NODE& hostNode = g_HostList.Get()[hostIndex];
        if (hostNode.address)
        {
            (void)hostNode.address->Release();
            hostNode.address = 0;
        }

        hostCount = g_NumberOfHost;
    }
    g_NumberOfHost = 0;
    LeaveDirectPlayRuntimeCriticalSection();
}


int DirectPlayReceiveQueueHasMatch(NET* network, int senderIndex)
{
    if ((network->m_flags & 1u) == 0)
        return 0;
    if ((static_cast<const volatile unsigned int&>(network->m_flags) & 0x80u) != 0)
    {
        while ((static_cast<const volatile unsigned int&>(network->m_flags) & 0x80u) != 0)
        {
        }
    }

    unsigned int readIndex = network->m_receiveRead;
    const unsigned int writeIndex = network->m_receiveWrite;
    while (readIndex != writeIndex)
    {
        if (senderIndex == -1 || senderIndex == static_cast<int>(network->m_receiveSender[readIndex]))
        {
            if (network->m_receiveType[readIndex] == 0)
                return 1;
        }
        ++readIndex;
        if (readIndex >= 64u)
            readIndex = 0;
    }
    return 0;
}


void DirectPlayReconcilePlayerRecords(NET* network)
{
    for (int playerIndex = 0; playerIndex != 8; ++playerIndex)
    {
        NET::NET_PLAYER& player = network->m_players[playerIndex];
        if (player.m_pendingDestroy != 0 && DirectPlayReceiveQueueHasMatch(network, playerIndex) == 0)
        {
            player.m_pendingDestroy = 0u;
            player.m_dpnid = 0u;
            MYERROR* log = ::Error;
            log->Log("DPN_Real_Destroy_Player[%i] %u", playerIndex, ::CurrentTime);
        }
    }
}


void NET::Tact()
{
    EnterDirectPlayRuntimeCriticalSection();
    m_noPlayers = g_lNumberOfActivePlayers;
    m_flags &= ~2u;
    if (g_dpnidHostPlayer == g_dpnidLocalPlayer)m_flags |= 2u;
    if (g_dpnidLocalPlayer == 0)
    {
        m_localPlayer = 0u;
        m_flags &= ~0x0Au;
        m_flags &= ~0x10u;
    }
    LeaveDirectPlayRuntimeCriticalSection();
    DirectPlayReconcilePlayerRecords(this);
}


static void DirectPlayLogIndexedHr(
    const char* messagePrefix, int itemIndex, int category, long errorCode)
{
    STRING indexString = Int2Str(itemIndex);
    STRING message = messagePrefix+indexString;
    STRING categoryText("NET");
    MYERROR* log = ::Error;
    log->Error(categoryText, static_cast<TYPE_ERROR>(category), message, (int)errorCode);
}

STRING NET::GetPlayerName(int playerIndex) const
{
    if ((m_flags&1u) == 0u || playerIndex<0 || playerIndex >= 8)
        return STRING();

    const NET_PLAYER& player = m_players[playerIndex];
    if ((m_flags&0x40u) != 0u)
    {
        const char* const directName = player.m_name.value;
        return directName[0] ? STRING(directName) : STRING();
    }

    const unsigned int playerId = player.m_dpnid;
    if (playerId == 0u)
        return STRING();

    IDirectPlay8Peer* const peer = g_pDP;
    DWORD bufferSize = 0u;
    HRESULT queryResult = peer->GetPeerInfo(playerId, 0, &bufferSize, 0u);
    if (queryResult<0 && (unsigned long)queryResult != 0x80158100u)
    {
        DirectPlayLogIndexedHr(
            "GetPeerInfo:GetPlayerName ", playerIndex, 9, queryResult);
        return STRING();
    }

    DPN_PLAYER_INFO* const playerInfo = static_cast<DPN_PLAYER_INFO*>(::operator new(bufferSize));
    unsigned char* const bufferData = reinterpret_cast<unsigned char*>(playerInfo);
    for (unsigned int byteIndex = 0;byteIndex != bufferSize;++byteIndex)bufferData[byteIndex] = 0;
    playerInfo->dwSize = sizeof(DPN_PLAYER_INFO);

    queryResult = g_pDP->GetPeerInfo(
        player.m_dpnid, playerInfo, &bufferSize, 0u);
    if (queryResult<0)
    {
        DirectPlayLogIndexedHr(
            "GetPeerInfo2:GetPlayerName ", playerIndex, 9, queryResult);
        ::operator delete(playerInfo);
        return STRING();
    }

    STRING playerName = WideCharToString(playerInfo->pwszName);
    ::operator delete(playerInfo);
    return playerName;
}

static void DirectPlayLogHr(const char* messageText, int category, long errorCode)
{
    STRING message;
    STRING categoryText;
    message = messageText;
    categoryText = "NET";
    MYERROR* log =
        ::Error;
    log->Error(categoryText, static_cast<TYPE_ERROR>(category), message, (int)errorCode);
}


void NET::StartHostSearch(STRING hostAddress)
{
    if ((m_flags & 1u) == 0)
    {
        return;
    }

    ClearHostList();
    m_flags &= ~2u;

    long searchResult = (long)CreateDeviceAddress();
    if (searchResult < 0)
    {
        DirectPlayLogHr(
            "device address for GetGameList", 3, searchResult);
        return;
    }

    STRING requestedHost;
    requestedHost = hostAddress;
    searchResult = CreateHostAddress(requestedHost);
    if (searchResult < 0)
    {
        DirectPlayLogHr(
            "host address for GetGameList", 3, searchResult);
        return;
    }

    DPN_APPLICATION_DESC applicationDesc = {};
    applicationDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
    applicationDesc.guidApplication = g_guidApp;

    CancelHostSearch();

    IDirectPlay8Peer* const peer = g_pDP;
    searchResult = peer->EnumHosts(&applicationDesc,
        g_pHostAddress,
        g_pDeviceAddress,
        0, 0u, 4u, 500u, 2000u, 0,
        &hAsyncCancelEnumHosts, 0u);
    if (searchResult < 0)
        DirectPlayLogHr(
            "Failed Enumerating the Hosts for GetGameList", 10, searchResult);

}


#if !defined(_MSC_VER) && !defined(__stdcall)
#define __stdcall
#endif

static void CopyDirectPlayPayload(void* destination, const void* source, unsigned int byteCount)
{
#if defined(_MSC_VER)
    memcpy(destination, source, byteCount);
#else
    unsigned char* destinationBytes = static_cast<unsigned char*>(destination);
    const unsigned char* sourceBytes = static_cast<const unsigned char*>(source);
    for (unsigned int byteIndex = 0; byteIndex < byteCount; ++byteIndex)
        destinationBytes[byteIndex] = sourceBytes[byteIndex];
#endif
}

static void DirectPlayNoPlayersMessage()
{
    MessageBoxA(0, "No one is around to wave at! :(",
       "SimplePeer", 0u);
}

int IsServiceProviderValid(const GUID* serviceProviderGuid)
{
    IDirectPlay8Peer* const peer = g_pDP;
    DWORD bufferBytes = 0u, providerCount = 0u;
    HRESULT enumerateResult = peer->EnumServiceProviders(serviceProviderGuid, 0, 0, &bufferBytes, &providerCount, 0u);

    if (static_cast<int>(enumerateResult)<0 && enumerateResult != 0x80158100u)
        return 0;

    DPN_SERVICE_PROVIDER_INFO* const providers = static_cast<DPN_SERVICE_PROVIDER_INFO*>(CrtNew(bufferBytes));
    IDirectPlay8Peer* const currentPeer = g_pDP;
    enumerateResult = currentPeer->EnumServiceProviders(serviceProviderGuid, 0, providers, &bufferBytes, &providerCount, 0u);

    const int providerFound = (static_cast<int>(enumerateResult) >= 0 && providerCount != 0u)?1:0;
    if (providers)CrtFreePointer(providers);
    return providerFound;
}
static long DirectPlaySendPacket(const void* payload, unsigned int payloadSize,
                          unsigned char packetType, unsigned long destination)
{
    unsigned char packet[4096];
    packet[0] = packetType;
    if (payloadSize != 0)
        CopyDirectPlayPayload(packet + 1, payload, payloadSize);

    DPN_BUFFER_DESC packetDesc;
    packetDesc.dwBufferSize = payloadSize+1u;
    packetDesc.pBufferData = packet;
    DPNHANDLE asyncHandle;
    IDirectPlay8Peer* const peer = g_pDP;
    (void)peer->SendTo(destination, &packetDesc, 1u, 0u, 0, &asyncHandle, 8u);
    return 0;
}


long NET::Send(
    const void* payload, unsigned int payloadSize,
    unsigned char packetType, int playerIndex)
{
    if ((m_flags&1u) == 0u)return E_FAIL;
    if ((m_flags&8u) == 0u)return 0;
    if (m_noPlayers == 0u)
    {
        DirectPlayNoPlayersMessage();
        return 0;
    }
    unsigned long destination = 0;
    if (playerIndex != -1)destination = m_players[playerIndex].m_dpnid;
    return DirectPlaySendPacket(payload, payloadSize, packetType, destination);
}


long NET::SendGuaranteed(
    const void* payload, unsigned int payloadSize,
    unsigned char packetType, int                    )
{
    if ((m_flags&1u) == 0u)return E_FAIL;
    if ((m_flags&8u) == 0u)return 0;
    if (m_noPlayers == 0u)
    {
        DirectPlayNoPlayersMessage();
        return 0;
    }
    return DirectPlaySendPacket(payload, payloadSize, packetType, 0);
}

#if !defined(_MSC_VER) && !defined(__stdcall)
#define __stdcall
#endif


static void CopyDirectPlayQueueBytes(void* destination, const void* source, unsigned int byteCount)
{
    unsigned char* const destinationBytes=static_cast<unsigned char*>(destination);
    const unsigned char* const sourceBytes=static_cast<const unsigned char*>(source);
#if defined(_MSC_VER)
    __movsd(reinterpret_cast<unsigned long*>(destinationBytes),
            reinterpret_cast<const unsigned long*>(sourceBytes),byteCount>>2);
    const unsigned int wordCopyBytes=byteCount&~3u;
    __movsb(destinationBytes+wordCopyBytes,sourceBytes+wordCopyBytes,byteCount&3u);
#else
    for(unsigned int byteIndex=0;byteIndex<byteCount;++byteIndex)
        destinationBytes[byteIndex]=sourceBytes[byteIndex];
#endif
}
static void EnterDirectPlayQueueCriticalSection()
{
    EnterCriticalSection(&g_csHostList);
}
static void LeaveDirectPlayQueueCriticalSection()
{
    LeaveCriticalSection(&g_csHostList);
}
static void DirectPlayQueueDiagnostic(const char* messageText, int errorCode)
{
    STRING message;
    STRING categoryText;
    message = messageText;
    categoryText = "NET";

    MYERROR* log =
        ::Error;
    log->Error(categoryText, E_ERROR, message, errorCode);

}


void NET::AddReceivedData(
    void* payload, unsigned int payloadBytes,
    unsigned long senderPlayerId, unsigned char packetType)
{
    const unsigned int senderId = static_cast<unsigned int>(senderPlayerId);
    if ((m_flags&1u) == 0u || payloadBytes>1024u)return;
    while ((static_cast<const volatile unsigned int&>(m_flags)&0x80u) != 0u)
    {
    }
    m_flags |= 0x80u;

    CopyDirectPlayQueueBytes(m_receiveData[m_receiveWrite], payload, payloadBytes);
    m_receiveSize[m_receiveWrite] = payloadBytes;
    m_receiveType[m_receiveWrite] = packetType;
    unsigned int senderIndex = 0u;
    for (;senderIndex<8u;++senderIndex)if (m_players[senderIndex].m_dpnid == senderId)break;
    if (senderIndex >= 8u)
    {
        DirectPlayQueueDiagnostic("unknown sender", static_cast<int>(senderId));
        m_flags = (m_flags&0xFFFFFF00u)|((m_flags&0xFFu)&0x7Fu);
        return;
    }
    m_receiveSender[m_receiveWrite] = senderIndex;
    unsigned int writeIndex = (m_receiveWrite+1u)&63u;
    m_receiveWrite = writeIndex;
    if (writeIndex == m_receiveRead)
    {
        m_receiveRead = (m_receiveRead+1u)&63u;
        DirectPlayQueueDiagnostic("Received messages stack is full", static_cast<int>(writeIndex));
    }
    m_flags = (m_flags&0xFFFFFF00u)|((m_flags&0xFFu)&0x7Fu);
}


int NET::Receive(
    void* output, unsigned int expectedSize, int senderIndex)
{
    if ((m_flags&1u) == 0u)return 1;
    while ((static_cast<const volatile unsigned int&>(m_flags)&0x80u) != 0u)
    {
    }
    unsigned int matchedIndex = m_receiveRead;
    const unsigned int writeIndex = m_receiveWrite;
    if (matchedIndex == writeIndex)return 1;
    for (;;)
    {
        if ((senderIndex == -1 || senderIndex == static_cast<int>(m_receiveSender[matchedIndex])) && m_receiveType[matchedIndex] == 0u)break;
        matchedIndex = (matchedIndex+1u)&63u;
        if (matchedIndex == writeIndex)return 1;
    }
    if (expectedSize != m_receiveSize[matchedIndex])
    {
        DirectPlayQueueDiagnostic("Incorrect order message", 0);
        return 1;
    }
    EnterDirectPlayQueueCriticalSection();
    CopyDirectPlayQueueBytes(output, m_receiveData[matchedIndex], expectedSize);
    while (matchedIndex != m_receiveRead)
    {
        const unsigned int previousIndex = matchedIndex == 0u?63u:matchedIndex-1u;
        CopyDirectPlayQueueBytes(m_receiveData[matchedIndex], m_receiveData[previousIndex], m_receiveSize[previousIndex]);
        m_receiveSize[matchedIndex] = m_receiveSize[previousIndex];
        m_receiveSender[matchedIndex] = m_receiveSender[previousIndex];
        m_receiveType[matchedIndex] = m_receiveType[previousIndex];
        matchedIndex = previousIndex;
    }
    m_receiveRead = (m_receiveRead+1u)&63u;
    LeaveDirectPlayQueueCriticalSection();
    return 0;
}


int DirectPlayQueryPeerMetric(void* self, int playerIndex)
{
    NET* const network = static_cast<NET*>(self);
    if ((network->m_flags & 1u) == 0)
        return 0;
    if (playerIndex < 0 || playerIndex >= 8)
        return 999;

    NET::NET_PLAYER& player = network->m_players[playerIndex];
    const unsigned long playerId = player.m_dpnid;
    if (playerId == 0)
        return 0;
    if (playerIndex == static_cast<int>(network->m_localPlayer))
        return 0;

    DPN_CONNECTION_INFO connectionInfo = {};
    connectionInfo.dwSize = 92u;
    IDirectPlay8Peer* const peer = g_pDP;
    const HRESULT queryResult = peer->GetConnectionInfo(playerId, &connectionInfo, 0u);
    if (queryResult < 0)
        return 998;

    player.m_roundTripLatencyMs = connectionInfo.dwRoundTripLatencyMS;
    return static_cast<int>(connectionInfo.dwRoundTripLatencyMS);
}

STRING NET::GetPlayerNameForHost(int playerIndex) const
{
    const char* playerNames = StringGlobals::EmptyText();
    if ((m_flags&1u) != 0u &&
        playerIndex >= 0 && playerIndex<g_NumberOfHost)
    {
        playerNames = g_HostList.Get()[playerIndex].playerNames.value;
    }
    return playerNames[0] ? STRING(playerNames) : STRING();
}

int NET::GetNoGame() const
{
    return g_NumberOfHost;
}


