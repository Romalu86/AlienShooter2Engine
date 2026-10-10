#pragma once
#include "mylib.h"
#include "basetype.h"
#pragma push_macro("INPUT")
#undef INPUT
#define INPUT WIN32_INPUT
#include <objbase.h>
#pragma pop_macro("INPUT")

typedef DWORD DPNHANDLE;
typedef DWORD DPNID;
typedef HRESULT (WINAPI *PFNDPNMESSAGEHANDLER)(void*,DWORD,void*);
struct IDirectPlay8Address;struct IDirectPlay8LobbiedApplication;
struct DPN_SECURITY_DESC;struct DPN_SECURITY_CREDENTIALS;
struct DPN_APPLICATION_DESC
{
    DWORD dwSize;
    DWORD dwFlags;

    GUID guidInstance,guidApplication;
    DWORD dwMaxPlayers;
    DWORD dwCurrentPlayers;

    WCHAR* pwszSessionName;
    WCHAR* pwszPassword;
    PVOID pvReservedData;
    DWORD dwReservedDataSize;
    PVOID pvApplicationReservedData;
    DWORD dwApplicationReservedDataSize;
};

struct DPN_PLAYER_INFO
{
    DWORD dwSize;
    DWORD dwInfoFlags;

    WCHAR* pwszName;
    PVOID pvData;
    DWORD dwDataSize;
    DWORD dwPlayerFlags;

};

struct DPN_BUFFER_DESC
{
    DWORD dwBufferSize;
    BYTE* pBufferData;
};

struct DPN_CONNECTION_INFO
{
    DWORD dwSize;
    DWORD dwRoundTripLatencyMS;
    DWORD dwThroughputBPS;
    DWORD dwPeakThroughputBPS;
    DWORD dwBytesSentGuaranteed;
    DWORD dwPacketsSentGuaranteed;
    DWORD dwBytesSentNonGuaranteed;
    DWORD dwPacketsSentNonGuaranteed;
    DWORD dwBytesRetried;
    DWORD dwPacketsRetried;
    DWORD dwBytesDropped;
    DWORD dwPacketsDropped;
    DWORD dwMessagesTransmittedHighPriority;
    DWORD dwMessagesTimedOutHighPriority;
    DWORD dwMessagesTransmittedNormalPriority;
    DWORD dwMessagesTimedOutNormalPriority;
    DWORD dwMessagesTransmittedLowPriority;
    DWORD dwMessagesTimedOutLowPriority;
    DWORD dwBytesReceivedGuaranteed;
    DWORD dwPacketsReceivedGuaranteed;
    DWORD dwBytesReceivedNonGuaranteed;
    DWORD dwPacketsReceivedNonGuaranteed;
    DWORD dwMessagesReceived;

};

struct DPL_PROGRAM_DESC;
typedef DPL_PROGRAM_DESC* PDPL_PROGRAM_DESC;
struct DPN_GROUP_INFO;
struct DPN_SERVICE_PROVIDER_INFO;
struct DPN_CAPS;
struct DPN_SP_CAPS;
typedef DPN_APPLICATION_DESC* PDPN_APPLICATION_DESC;

struct DPL_CONNECTION_SETTINGS
{
    DWORD dwSize;
    DWORD dwFlags;

    DPN_APPLICATION_DESC dpnAppDesc;
    IDirectPlay8Address* pdp8HostAddress;
    IDirectPlay8Address** ppdp8DeviceAddresses;
    DWORD cNumDeviceAddresses;
};

struct DPL_MESSAGE_CONNECT
{
    DWORD dwSize;
    DPNHANDLE hConnectId;
    DPL_CONNECTION_SETTINGS* pdplConnectionSettings;
    PVOID pvLobbyClientContext;
};

struct DPNMSG_ENUM_HOSTS_QUERY
{
    DWORD dwSize;
    IDirectPlay8Address* pAddressSender;
    IDirectPlay8Address* pAddressDevice;
    PVOID pvReceivedData;
    DWORD dwReceivedDataSize;
    DWORD dwMaxResponseDataSize;

    PVOID pvResponseData;
    DWORD dwResponseDataSize;
    PVOID pvResponseContext;
};

struct DPNMSG_ENUM_HOSTS_RESPONSE
{
    DWORD dwSize;
    IDirectPlay8Address* pAddressSender;
    IDirectPlay8Address* pAddressDevice;
    const DPN_APPLICATION_DESC* pApplicationDescription;
    PVOID pvResponseData;
    DWORD dwResponseDataSize;
    PVOID pvUserContext;
    DWORD dwRoundTripLatencyMS;
};

struct DPNMSG_CREATE_PLAYER
{
    DWORD dwSize;
    DPNID dpnidPlayer;
    PVOID pvPlayerContext;
};


struct DPNMSG_DESTROY_PLAYER
{
    DWORD dwSize;
    DPNID dpnidPlayer;
    PVOID pvPlayerContext;
    DWORD dwReason;
};


struct DPNMSG_PEER_INFO
{
    DWORD dwSize;
    DPNID dpnidPeer;
    PVOID pvPlayerContext;
};


struct DPNMSG_HOST_MIGRATE
{
    DWORD dwSize;
    DPNID dpnidNewHost;
    PVOID pvPlayerContext;
};

struct DPNMSG_RECEIVE
{
    DWORD dwSize;
    DPNID dpnidSender;
    PVOID pvPlayerContext;
    PBYTE pReceiveData;
    DWORD dwReceiveDataSize;
    DPNHANDLE hBufferHandle;
};


struct IDirectPlay8Address : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE BuildFromURLW(WCHAR* pwszSourceURL) = 0;
    virtual HRESULT STDMETHODCALLTYPE BuildFromURLA(CHAR* pszSourceURL) = 0;
    virtual HRESULT STDMETHODCALLTYPE Duplicate(IDirectPlay8Address** ppdpaNewAddress) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEqual(IDirectPlay8Address* pdpaAddress) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsEqual( IDirectPlay8Address* pdpaAddress) = 0;
    virtual HRESULT STDMETHODCALLTYPE Clear() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetURLW(WCHAR* pwszURL, PDWORD pdwNumChars) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetURLA(CHAR* pszURL, PDWORD pdwNumChars) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSP(GUID* pguidSP) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetUserData(LPVOID pvUserData, PDWORD pdwBufferSize) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetSP(const GUID* pguidSP) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetUserData(const void* pvUserData, DWORD dwDataSize) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetNumComponents(PDWORD pdwNumComponents) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentByName(const WCHAR* pwszName, LPVOID pvBuffer, PDWORD pdwBufferSize, PDWORD pdwDataType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetComponentByIndex(DWORD dwComponentID, WCHAR* pwszName, PDWORD pdwNameLen, void* pvBuffer, PDWORD pdwBufferSize, PDWORD pdwDataType) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddComponent(const WCHAR* pwszName, const void* lpvData, DWORD dwDataSize, DWORD dwDataType) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDevice(GUID* pDevGuid) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetDevice(const GUID* devGuid) = 0;
    virtual HRESULT STDMETHODCALLTYPE BuildFromDirectPlay4Address(LPVOID pvAddress, DWORD dwDataSize) = 0;
};

struct IDirectPlay8Peer : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Initialize(PVOID pvUserContext, PFNDPNMESSAGEHANDLER pfn, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumServiceProviders(const GUID* pguidServiceProvider, const GUID* pguidApplication, DPN_SERVICE_PROVIDER_INFO* pSPInfoBuffer, DWORD* pcbEnumData, DWORD* pcReturned, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE CancelAsyncOperation(DPNHANDLE hAsyncHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Connect(const DPN_APPLICATION_DESC* pdnAppDesc, IDirectPlay8Address* pHostAddr, IDirectPlay8Address* pDeviceInfo, const DPN_SECURITY_DESC* pdnSecurity, const DPN_SECURITY_CREDENTIALS* pdnCredentials, const void* pvUserConnectData, DWORD dwUserConnectDataSize, void* pvPlayerContext, void* pvAsyncContext, DPNHANDLE* phAsyncHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SendTo(DPNID dpnid, const DPN_BUFFER_DESC* prgBufferDesc, DWORD cBufferDesc, DWORD dwTimeOut, void* pvAsyncContext, DPNHANDLE* phAsyncHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSendQueueInfo(DPNID dpnid, DWORD* pdwNumMsgs, DWORD* pdwNumBytes, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Host(const DPN_APPLICATION_DESC* pdnAppDesc, IDirectPlay8Address **prgpDeviceInfo, DWORD cDeviceInfo, const DPN_SECURITY_DESC* pdnSecurity, const DPN_SECURITY_CREDENTIALS* pdnCredentials, void* pvPlayerContext, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetApplicationDesc(DPN_APPLICATION_DESC* pAppDescBuffer, DWORD* pcbDataSize, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetApplicationDesc(const DPN_APPLICATION_DESC* pad, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateGroup(const DPN_GROUP_INFO* pdpnGroupInfo, void* pvGroupContext, void* pvAsyncContext, DPNHANDLE* phAsyncHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE DestroyGroup(DPNID idGroup, PVOID pvAsyncContext, DPNHANDLE* phAsyncHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddPlayerToGroup(DPNID idGroup, DPNID idClient, PVOID pvAsyncContext, DPNHANDLE* phAsyncHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemovePlayerFromGroup(DPNID idGroup, DPNID idClient, PVOID pvAsyncContext, DPNHANDLE* phAsyncHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetGroupInfo(DPNID dpnid, DPN_GROUP_INFO* pdpnGroupInfo,PVOID pvAsyncContext, DPNHANDLE* phAsyncHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetGroupInfo(DPNID dpnid, DPN_GROUP_INFO* pdpnGroupInfo, DWORD* pdwSize, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumPlayersAndGroups(DPNID* prgdpnid, DWORD* pcdpnid, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumGroupMembers(DPNID dpnid, DPNID* prgdpnid, DWORD* pcdpnid, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPeerInfo(const DPN_PLAYER_INFO* pdpnPlayerInfo,PVOID pvAsyncContext, DPNHANDLE* phAsyncHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPeerInfo(DPNID dpnid, DPN_PLAYER_INFO* pdpnPlayerInfo, DWORD* pdwSize, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPeerAddress(DPNID dpnid, IDirectPlay8Address** pAddress, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLocalHostAddresses(IDirectPlay8Address** prgpAddress, DWORD* pcAddress, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Close(DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumHosts(PDPN_APPLICATION_DESC pApplicationDesc, IDirectPlay8Address* pAddrHost, IDirectPlay8Address* pDeviceInfo,PVOID pUserEnumData, DWORD dwUserEnumDataSize, DWORD dwEnumCount, DWORD dwRetryInterval, DWORD dwTimeOut,PVOID pvUserContext, DPNHANDLE* pAsyncHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE DestroyPeer(DPNID dpnidClient, const void* pvDestroyData, DWORD dwDestroyDataSize, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE ReturnBuffer(DPNHANDLE hBufferHandle, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPlayerContext(DPNID dpnid,PVOID* ppvPlayerContext, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetGroupContext(DPNID dpnid,PVOID* ppvGroupContext, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCaps(DPN_CAPS* pdpCaps, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCaps(const DPN_CAPS* pdpCaps, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetSPCaps(const GUID* pguidSP, const DPN_SP_CAPS* pdpspCaps, DWORD dwFlags ) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSPCaps(const GUID* pguidSP, DPN_SP_CAPS* pdpspCaps, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectionInfo(DPNID dpnid, DPN_CONNECTION_INFO* pdpConnectionInfo, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE RegisterLobby(DPNHANDLE dpnHandle, struct IDirectPlay8LobbiedApplication* pIDP8LobbiedApplication, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE TerminateSession(void* pvTerminateData, DWORD dwTerminateDataSize, DWORD dwFlags) = 0;
};

struct IDirectPlay8LobbiedApplication : IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Initialize(PVOID pvUserContext, PFNDPNMESSAGEHANDLER pfn, DPNHANDLE* pdpnhConnection, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE RegisterProgram(PDPL_PROGRAM_DESC pdplProgramDesc, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnRegisterProgram(GUID* pguidApplication, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Send(DPNHANDLE hConnection, BYTE* pBuffer, DWORD pBufferSize, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetAppAvailable(BOOL fAvailable, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateStatus(DPNHANDLE hConnection, DWORD dwStatus, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Close(DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectionSettings(DPNHANDLE hConnection, DPL_CONNECTION_SETTINGS* pdplSessionInfo, DWORD* pdwInfoSize, DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetConnectionSettings(DPNHANDLE hConnection, const DPL_CONNECTION_SETTINGS* pdplSessionInfo, DWORD dwFlags) = 0;
};


class STRING;
struct HINSTANCE__;

class NET
{
public:
    NET(HINSTANCE__* hInstance, STRING title);
    ~NET() noexcept(false);


    virtual long Send(const void* data,unsigned int size,unsigned char type,int player);
    virtual long SendGuaranteed(const void* data,unsigned int size,unsigned char type,int player);
    virtual int Receive(void* out,unsigned int size,int player);

    long InitDirectPlay();
    long LobbyLaunch();
    long CreateDeviceAddress();
    long CreateHostAddress(STRING hostAddress);
    void AddReceivedData(void* data,unsigned int size,unsigned long sender,unsigned char ready);
    void Tact();

    STRING GetPlayerName(int player) const;
    int GetNoGame() const;
    STRING GetGameList(int index) const;
    void StartHostSearch(STRING hostAddress);
    void CancelHostSearch();
    long GameCreate(STRING gameName);
    STRING GetPlayerNameForHost(int player) const;
    long GameJoinByName(STRING gameName);
    long GameJoin(int index);
    long GameDisconnect();
    void SetPlayerName(STRING playerName);
    void GameStart(STRING text);
    void GameEnd();
    void NetworkOn();
    void NetworkOff();
    void SetChatColor(int index,unsigned long color);
    void SendChat(STRING text);
    STRING GetChat(int index) const;
    void SendString(int type,const STRING& text);
    STRING ReceiveString(int type,int player);
    void DestroyPlayer(int player);
    unsigned long Error(TYPE_ERROR type, const STRING& text, unsigned long detail) const;

    unsigned int IsConnected() const;
    unsigned int IsHost() const;
    unsigned int IsLobby() const;
    unsigned int NoPlayers() const;
    unsigned long PlayerDPNID(unsigned int index) const;

    struct NET_PLAYER
    {
        NET_PLAYER();
        ~NET_PLAYER();

        unsigned int m_dpnid;
        STRING       m_name;
        unsigned int m_chatColor;
        unsigned int m_0C;
        unsigned int m_pendingDestroy;
        unsigned int m_roundTripLatencyMs;
    };

    unsigned int  m_flags;
    unsigned int  m_receiveSender[64];
    unsigned int  m_receiveSize[64];
    unsigned char m_receiveData[64][1024];
    unsigned char m_receiveType[64];
    unsigned int  m_receiveRead;
    unsigned int  m_receiveWrite;
    unsigned int  m_checkListCrc;
    unsigned int  m_noPlayers;
    unsigned int  m_localPlayer;
    NET_PLAYER    m_players[8];
    STRING        m_playerName;
    unsigned int  m_chatPlayer[256];
    STRING        m_chatText[256];
    unsigned int  m_chatEnd;
};


void ClearHostList();
int IsServiceProviderValid(const GUID* serviceProviderGuid);
DPN_PLAYER_INFO* GetPeerInfo(DPNID peerId);
const STRING WideCharToString(const unsigned short* wideText);
long __stdcall DirectPlayMessageHandler(void* userContext, unsigned long messageId, void* messageBody);
long __stdcall LobbyAppMessageHandler(void* userContext, unsigned long messageId, void* messageBody);
int DirectPlayReceiveQueueHasMatch(NET* network, int senderIndex);
void DirectPlayReconcilePlayerRecords(NET* network);
int DirectPlayQueryPeerMetric(void* self, int playerIndex);

unsigned int Session_StaticCleanupRegister02();

extern unsigned int networkTransmissionId;
