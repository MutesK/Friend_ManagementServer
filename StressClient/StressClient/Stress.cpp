#include "Stress.h"
#include <map>
#include <algorithm>
using namespace std;

map<UINT64, st_CLIENT *> g_ClientMap;

WORD g_UserCount = 0;
WORD g_ConnectTry = 1500;
WORD g_ConnectFail = 0;
WORD g_ConnectSuccess = 0;

UINT g_ErrorCount = 0;

void Draw(DWORD nowTick)
{

	UINT RecvCount = 0;
	UINT SendCount = 0;

	DWORD Tick;
	DWORD minTick = 9999;
	DWORD maxTick = 0;
	int Count = 0;

	auto begin = g_ClientMap.begin();
	auto end = g_ClientMap.end();

	for_each(begin, end, [&RecvCount, &SendCount, &Tick, &nowTick, &minTick, &maxTick, &Count](pair<UINT64, st_CLIENT *> pair)
	{
		st_CLIENT *pClient = pair.second;

		if (pClient->Socket != INVALID_SOCKET)
		{
			// 레이턴시 계산, Recv 계산
			DWORD tTick = max(nowTick - pClient->dwRequestTick, 0);

			RecvCount += pClient->iResponsePacket;
			SendCount += pClient->iRequestPacket;

			if (tTick != 0)
			{

				if (tTick > maxTick)
					maxTick = tTick;

				if (tTick < minTick)
					minTick = tTick;

				Tick += tTick;

				Count++;
			}
			pClient->iResponsePacket = 0;
			pClient->iRequestPacket = 0;
		}
	});

	Tick = Tick / Count;


	wprintf(L"/////////////////////////////////////////\n");
	wprintf(L"ConnectTry : %d \n", g_ConnectTry);
	wprintf(L"ConnectFail : %d \n", g_ConnectFail);
	wprintf(L"ConnectSuccess : %d \n", g_ConnectSuccess);
	wprintf(L"Laterncy(RTT)  Avg : %d ms, Max : %d ms, Min : %d ms\n", Tick, maxTick, minTick);
	wprintf(L"Packet : RecvCount : %d, SendCount : %d, ErrorCount : %d \n\n\n", RecvCount, SendCount, g_ErrorCount);


}
bool NetworkInit()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKADDR_IN Addr;
	ZeroMemory(&Addr, sizeof(Addr));
	Addr.sin_family = AF_INET;
	InetPton(AF_INET, L"127.0.0.1", &Addr.sin_addr);
	Addr.sin_port = htons(dfNETWORK_PORT);

	for (auto i = 0; i < g_ConnectTry; i++)
	{
		st_CLIENT *pClient = new st_CLIENT;
		
		pClient->dwRequestTick = 0;
		pClient->iRequestPacket = 0;
		pClient->iResponsePacket = 0;
		pClient->iPacketCount = 0;
		pClient->pStressEcho = nullptr;

		pClient->Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		//-----------------------------------------------------
		// 접속 !
		//-----------------------------------------------------
		if (SOCKET_ERROR == connect(pClient->Socket, (SOCKADDR *)&Addr, sizeof(SOCKADDR_IN)))
			g_ConnectFail++;
		else
		{
			g_ConnectSuccess++;
			g_UserCount++;
			pClient->UserNo = g_UserCount;
			wprintf(L"ConnectOK:%d \n", g_UserCount);
			g_ClientMap.emplace(g_UserCount, pClient);
		}
	}
	

	return true;
}

void NetworkClear()
{
	auto begin = g_ClientMap.begin();
	auto end = g_ClientMap.end();

	for_each(begin, end, [](pair<UINT64, st_CLIENT *> pairs)
	{
		SOCKET Socket = pairs.second->Socket;

		closesocket(Socket);
	});

	WSACleanup();
}

st_CLIENT* FindClient(const UINT64 &UserNo)
{
	auto Finditer = g_ClientMap.find(UserNo);

	if (Finditer != g_ClientMap.end())
		return Finditer->second;

	return nullptr;
}

void DisconnectClient(const UINT64 &UserNo)
{
	st_CLIENT *pClient = FindClient(UserNo);

	if (pClient == nullptr)
		return;

	WCHAR szIP[16];

	closesocket(pClient->Socket);
	g_ClientMap.erase(UserNo);
	g_UserCount--;

	delete pClient;
}

void NetworkProcess()
{
	st_CLIENT *pClient = nullptr;

	DWORD UserTable_No[FD_SETSIZE];
	SOCKET UserTable_SOCKET[FD_SETSIZE];
	int iSocketCount = 0;


	FD_SET RSet;
	FD_SET WSet;

	FD_ZERO(&RSet);
	FD_ZERO(&WSet);
	memset(UserTable_No, -1, sizeof(__int64) * FD_SETSIZE);
	memset(UserTable_SOCKET, INVALID_SOCKET, sizeof(SOCKET) *FD_SETSIZE);

	auto begin = g_ClientMap.begin();
	auto end = g_ClientMap.end();

	// 리슨소켓 및 접속중인 모든 클라이언트에 대해 SOCKET 체크
	for (auto iter = begin; iter != end;)
	{
		pClient = iter->second;
		iter++;

		UserTable_No[iSocketCount] = pClient->UserNo;
		UserTable_SOCKET[iSocketCount] = pClient->Socket;

		FD_SET(pClient->Socket, &RSet);

		if (pClient->SendQ.GetDataSize() > 0)
			FD_SET(pClient->Socket, &WSet);

		iSocketCount++;

		// select 최대치 도달, 만들어진 테이블 정보로 select 호출뒤 정리
		if (FD_SETSIZE <= iSocketCount)
		{
			SelectSocket(UserTable_No, UserTable_SOCKET, &RSet, &WSet);

			FD_ZERO(&RSet);
			FD_ZERO(&WSet);

			memset(UserTable_No, -1, sizeof(__int64) * FD_SETSIZE);
			memset(UserTable_SOCKET, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
			iSocketCount = 0;
		}
	}

	// 전체 클라이언트 for문 종료 후, socketCount 수치가 있다면
	// 추가적으로 마지막 select 호출을 한다.
	if (iSocketCount > 0)
	{
		SelectSocket(UserTable_No, UserTable_SOCKET, &RSet, &WSet);
	}
}

void SelectSocket(DWORD* pTableNo, SOCKET* pTableSocket, FD_SET *pReadSet, FD_SET *pWriteSet)
{
	timeval Time;
	int iResult;

	Time.tv_sec = 0;
	Time.tv_usec = 1;

	iResult = select(0, pReadSet, pWriteSet, NULL, &Time);

	// 리턴값이 0보다 이상은 데이터 도착
	if (0 < iResult)
	{
		// TableSocket을 돌면서 어떤 소켓에 반응이 온것인지 확인
		for (auto iCnt = 0; iCnt < FD_SETSIZE; iCnt++)
		{
			if (pTableSocket[iCnt] == INVALID_SOCKET)
				continue;

			// WRITE 체크
			if (FD_ISSET(pTableSocket[iCnt], pWriteSet))
				SendProc(pTableNo[iCnt]);

			if (FD_ISSET(pTableSocket[iCnt], pReadSet))
				RecvProc(pTableNo[iCnt]);
		}
	}
	else if (iResult == SOCKET_ERROR)
	{
		wprintf(L"Select Socket Error ! \n");
		return;
	}
}

void SendProc(const UINT64& UserNo)
{
	st_CLIENT *pClient;
	pClient = FindClient(UserNo);

	if (pClient == nullptr)
		return;

	int iSendSize = pClient->SendQ.GetDataSize();
	iSendSize = min(dfRECV_BUFF, iSendSize);

	if (0 >= iSendSize)
		return;

	int iResult = send(pClient->Socket, pClient->SendQ.GetReadBufferPtr(), iSendSize, 0);


	if (iResult == SOCKET_ERROR)
	{
		DWORD dwError = WSAGetLastError();
		if (dwError == WSAEWOULDBLOCK)
			return;

		// 오류로 인한 끊김
		wprintf(L"Socket Error \n");
		g_ConnectSuccess--;
		g_ConnectFail++;

		DisconnectClient(pClient->UserNo);

		return;
	}
	else if (iResult > iSendSize)
	{
		wprintf(L"uncomplete Sended \n");
		return;
	}
	pClient->SendQ.MoveReadPos(iResult);

}
void RecvProc(const UINT64& UserNo)
{
	char pData[1200];

	st_CLIENT *pClient;
	int iResult;

	pClient = FindClient(UserNo);

	if (pClient == nullptr)
		return;


	iResult = recv(pClient->Socket, pClient->RecvQ.GetWriteBufferPtr(), dfRECV_BUFF, 0);
	pClient->RecvQ.MoveWritePos(iResult);


	if (SOCKET_ERROR == iResult)
	{
		// 오류로 인한 끊김
		wprintf(L"Socket Error \n");
		g_ConnectSuccess--;
		g_ConnectFail++;
		DisconnectClient(pClient->UserNo);
		return;
	}

	if (0 <= iResult)
	{
		//  패킷이 완료되었는지 확인
		// 패킷 처리중 문제 발생하면 종료
		// 패킷은 하나 이상의 버퍼에 있을수 있으므로 한번에 전부 처리해야 됨.
		while (1)
		{
			iResult = CompleteRecvPackcet(pClient);

			if (iResult == 1)
				break;

			if (iResult == -1)
			{
				g_ErrorCount++;
				wprintf(L"Packet Proc Error \n");
				return;
			}

		}
	}
}
int CompleteRecvPackcet(st_CLIENT *pClient)
{
	int iRecvSize = pClient->RecvQ.GetDataSize();

	// 헤더보다 적을때
	if (sizeof(st_PACKET_HEADER) > iRecvSize)
		return 1;


	st_PACKET_HEADER Header;
	pClient->RecvQ.PeekData((char *)&Header, sizeof(st_PACKET_HEADER));

	if (Header.byCode != (BYTE)dfPACKET_CODE)
		return -1;

	if (Header.wPayloadSize + sizeof(st_PACKET_HEADER) > (WORD)iRecvSize)
		return 1;

	pClient->RecvQ.MoveReadPos(sizeof(st_PACKET_HEADER));

	CSerializeBuffer Buffer;

	if (Header.wPayloadSize != Buffer.PutData(pClient->RecvQ.GetReadBufferPtr(), Header.wPayloadSize))
		return -1;

	if (pClient->RecvQ.MoveReadPos(Header.wPayloadSize) != Header.wPayloadSize)
	{
		wprintf(L"Move Read Pos Error \n");
		return -1;
	}

	if (!PacketProc(pClient, Header.wMsgType, &Buffer))
		return -1;

	return 0;
}
bool PacketProc(st_CLIENT *pClient, const WORD& MsgType, CSerializeBuffer *Buffer)
{
	switch (MsgType)
	{
	case df_RES_STRESS_ECHO:
		return ResStressEcho(pClient, Buffer);
		break;
	default:
		wprintf(L"Packet Error !!!!!!!!!!!!!!!!!!!!\n");
		wprintf(L"Packet Error !!!!!!!!!!!!!!!!!!!!\n");
		wprintf(L"Packet Error !!!!!!!!!!!!!!!!!!!!\n");
		return false;
	}
	return true;
}

// 서버 -> 클라이언트
bool ResStressEcho(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	if (pClient->pStressEcho == nullptr)
		return true;

	char *pData = pClient->pStressEcho;
	char *pTData = new char[1000];

	WORD iSize;
	*Buffer >> iSize;

	Buffer->GetData(pTData, iSize);

	if (memcmp(pTData, pData, iSize) != 0)
	{
		wprintf(L"Echo Message Error !! \n");
		g_ErrorCount++;
	}
	delete[] pClient->pStressEcho;
	delete[] pTData;

	pClient->pStressEcho = nullptr;

	pClient->iResponsePacket++;
	pClient->iPacketCount++;

	ReqStressEcho(pClient);

	return true;
}

// 클라이언트 -> 서버
bool ReqStressEcho(st_CLIENT *pClient)
{
	if (pClient->pStressEcho != nullptr)
		return true;

	char *pData = new char[1000];
	WORD iSize = (rand() % 500) + 500;
	CRingBuffer Buffer;
	
	for (int i = 0; i < iSize; i++)
		pData[i] = (i % 94) + 33;

	pClient->pStressEcho = pData;


	Buffer.Put(reinterpret_cast<char *>(&iSize), sizeof(WORD));
	Buffer.Put(pData, iSize);

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_REQ_STRESS_ECHO;
	Header.wPayloadSize = Buffer.GetUseSize();

	// 헤더 입력
	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	// 데이터 입력
	pClient->SendQ.PutData(Buffer.GetReadBufferPtr(), Header.wPayloadSize);

	pClient->iRequestPacket++;
	pClient->dwRequestTick = GetTickCount64();
	return true;
}

void SendMessages()
{
	auto begin = g_ClientMap.begin();
	auto end = g_ClientMap.end();

	for_each(begin, end, [](pair<UINT64, st_CLIENT *> pair)
	{
		st_CLIENT *pClient = pair.second;

		if (pClient->Socket != INVALID_SOCKET)
			ReqStressEcho(pClient);
	});
}