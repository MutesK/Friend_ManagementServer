#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Winmm.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Protocol.h"
#include "RingBuffer.h"
#include "SerializeBuffer.h"

#define dfRECV_BUFF 1000

struct st_CLIENT  // 계정
{
	DWORD UserNo;

	SOCKET Socket;

	CSerializeBuffer SendQ;
	CSerializeBuffer RecvQ;

	UINT iRequestPacket; // 요청 패킷
	UINT iResponsePacket;	// 반응 패킷

	DWORD dwRequestTick;
	UINT iPacketCount;

	char *pStressEcho;
};

void Draw(DWORD nowTick);
bool NetworkInit();
void NetworkClear();
st_CLIENT* FindClient(const UINT64 &UserNo);
void DisconnectClient(const UINT64 &UserNo);
void NetworkProcess();
void SelectSocket(DWORD* pTableNo, SOCKET* pTableSocket, FD_SET *pReadSet, FD_SET *pWriteSet);

void SendProc(const UINT64& UserNo);
void RecvProc(const UINT64& UserNo);

int CompleteRecvPackcet(st_CLIENT *pClient);
bool PacketProc(st_CLIENT *pClient, const WORD& MsgType, CSerializeBuffer *Buffer);

bool ResStressEcho(st_CLIENT *pClient, CSerializeBuffer *Buffer);
bool ReqStressEcho(st_CLIENT *pClient);

void SendMessages();