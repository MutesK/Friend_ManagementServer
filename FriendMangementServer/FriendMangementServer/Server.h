#pragma once

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Winmm.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Protocol.h"
#include "SerializeBuffer.h"

#define dfRECV_BUFF 20000


struct st_Account
{
	UINT64 AccountNo;
	WCHAR szID[dfNICK_MAX_LEN];
};

struct st_Friend
{
	UINT64 FromAccountNo;
	UINT64 ToAccountNo;
};

struct st_FriendRequest
{
	UINT64 FromAccountNo;
	UINT64 ToAccountNo;
};

struct st_CLIENT  // 계정
{
	DWORD UserNo;

	SOCKET Socket;
	SOCKADDR_IN ConnectAdr;			//  클라이언트 접속정보

	CSerializeBuffer SendQ;
	CSerializeBuffer RecvQ;

	st_Account *pAccount;
};

bool NetworkInit();

st_CLIENT* FindClient(const UINT64 &UserNo);

void DisconnectClient(const UINT64 &UserNo);
void NetworkProcess();
void SelectSocket(DWORD* pTableNo, SOCKET* pTableSocket, FD_SET *pReadSet, FD_SET *pWriteSet);

void AcceptProc();
void SendProc(const UINT64& UserNo);

void RecvProc(const UINT64& UserNo);
int CompleteRecvPackcet(st_CLIENT *pClient);
bool PacketProc(st_CLIENT *pClient, const WORD& MsgType, CSerializeBuffer *Buffer);
///////////////////////////////////////////////////////////////////////////////
// 계정추가, 찾기
st_Account* AddAccount(WCHAR *szName, UINT64);
st_Account* FindAccount(UINT64 &AccUserNo);

// 친구 추가, 삭제
void AddFriend(UINT64 From, UINT64 To);
int DeleteFriend(UINT64 From, UINT64 To);

// 요청 함수
BYTE FindFriendRequest(UINT64& From, UINT64& To);
BYTE AddFriendRequest(UINT64 From, UINT64 To);
int DeleteFriendRequest(UINT64& From, UINT64& To);
///////////////////////////////////////////////////////////////////////////////
// Request 패킷 처리 함수
bool ReqAddAccount(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqLogin(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqAccountList(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqFriendList(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqFriendRequestList(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqFriendReplyList(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqFriendRemove(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqFriendRequest(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqFriendDeny(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqFriendCancle(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqFriendAgree(st_CLIENT *pClient, CSerializeBuffer *Buffer);
///////////////////////////////////////////////////////////////////////////////
// Response 패킷 처리 함수
bool ResAddAccount(st_CLIENT *pClient, st_Account *pAccount);
bool ResLogin(st_CLIENT *pClient);
bool ResAccountList(st_CLIENT *pClient);
bool ResFriendList(st_CLIENT *pClient, st_Account* AccountNo);
bool ResFriendRequestList(st_CLIENT *pClient);
bool ResFriendReplyList(st_CLIENT *pClient);
bool ResFriendRemove(st_CLIENT *pClient, UINT64& FriendAccountNo);
bool ResFriendRequest(st_CLIENT *pClient, UINT64& FriendAccountNo);
bool ResFriendDeny(st_CLIENT *pClient, UINT64& FriendAccountNo);
bool ResFriendCancle(st_CLIENT *pClient, UINT64& FriendAccountNo);
bool ResFriendAgree(st_CLIENT *pClient, UINT64& FriendAccountNo);
///////////////////////////////////////////////////////////////////////////////
