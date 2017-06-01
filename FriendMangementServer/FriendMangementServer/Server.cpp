#include "Server.h"
#include <map>
#include <algorithm>
using namespace std;

char gData[10000];
// �̹� ������� ģ����� <From, ģ�� ����ü>
multimap<UINT64, st_Friend *> g_FriendMap; 
// ģ����û <From, ģ�� ��û ����ü>
multimap<UINT64, st_FriendRequest *> g_RequestFriendMap;
// From ģ����û �ѻ��(Key), To ģ����û �������(Data)
multimap<UINT64, UINT64> g_RequestFrom;
// To ģ����û ������� (Key), From ģ����û �ѻ��(Data)
multimap<UINT64, UINT64> g_RequestTo;

SOCKET g_ListenSocket;

UINT64 g_ClientNo = 0;  // �� ����� Ŭ���̾�Ʈ ��
UINT64 g_AccountNo = 0;

UINT g_RecvCount = 0;
UINT g_SendCount = 0;

map<UINT64, st_CLIENT *> g_ClientMap;
map<UINT64, st_Account *> g_AccountMap;

void Draw()
{
	wprintf(L"Connected : %d \n", g_ClientNo);

	if (g_ClientNo > 0)
	{
		wprintf(L"ProcessCount: Send[%d Cnt/Sec], Recv[%d Cnt/Sec], Total[%d Cnt/Sec]  \n", g_RecvCount, g_SendCount, g_RecvCount + g_SendCount);
		g_RecvCount = 0;
		g_SendCount = 0;
	}
}
void NetworkClear()
{
	for_each(g_ClientMap.begin(), g_ClientMap.end(), [](pair<UINT64, st_CLIENT *> pairs)
	{
		DisconnectClient(pairs.first);
	});
}
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received; // recv �Լ��� ���ϰ�
	char *ptr = buf; // buf index pointer
	int left = len; // left�� �� �����ʿ��� �������� �ϳ��� �̵��Ѵ�.

	while (left > 0)
	{
		received = recv(s, ptr, left, flags); // ptr���� left��ŭ �޴´�
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;

		else if (received == 0)		// ���� ����
			break;

		left -= received; // received(���� ��ŭ) ���� �����͸� ���δ�.
		ptr += received; // received(���� ��ŭ) ��ġ�� �ű��.
	}

	return (len - left);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
st_Account* AddAccount(WCHAR *szName, UINT64 AccountNo)
{
	st_Account *pAccount = new st_Account;

	g_AccountNo++;

	pAccount->AccountNo = AccountNo;
	lstrcpyW(pAccount->szID, szName);

	g_AccountMap.insert(map<UINT64, st_Account *>::value_type(AccountNo, pAccount));
	return pAccount;
}
st_Account* FindAccount(UINT64 &AccUserNo)
{
	auto Finditer = g_AccountMap.find(AccUserNo);

	if (Finditer != g_AccountMap.end())
		return Finditer->second;

	return nullptr;
}
st_CLIENT* FindClient(const UINT64 &UserNo)
{
	auto Finditer = g_ClientMap.find(UserNo);

	if (Finditer != g_ClientMap.end())
		return Finditer->second;

	return nullptr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

void AddFriend(UINT64 From, UINT64 To)
{
	st_Friend *pNewFriend = new st_Friend;
	pNewFriend->FromAccountNo = From;
	pNewFriend->ToAccountNo = To;

	g_FriendMap.emplace(From, pNewFriend);

}
int DeleteFriend(UINT64 From, UINT64 To)
{
	auto iter = g_FriendMap.begin();
	pair<multimap<UINT64, st_Friend*>::iterator,
		multimap<UINT64, st_Friend*>::iterator> range;

	range = g_FriendMap.equal_range(From);

	int deleteCount = 0;
	for (iter = range.first; iter != range.second;)
	{
		if ((iter->first == From && iter->second->ToAccountNo == To) ||
			(iter->first == To && iter->second->ToAccountNo == From))
		{
			deleteCount++;
			st_Friend* pTemp = iter->second;
			iter = g_FriendMap.erase(iter);
			delete pTemp;

			if (deleteCount == 2)
				break;
		}
		else
			iter++;
	}

	return deleteCount;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////


BYTE DeleteFriendRequest(UINT64& From, UINT64& To)
{
	auto iter = g_RequestFriendMap.begin();
	pair<multimap<UINT64, st_FriendRequest *>::iterator,
		multimap<UINT64, st_FriendRequest *>::iterator> Allrange;

	Allrange = g_RequestFriendMap.equal_range(From);

	for (iter = Allrange.first; iter != Allrange.second;)
	{
		if ((iter->first == From && iter->second->ToAccountNo == To))
		{
			iter = g_RequestFriendMap.erase(iter);
			break;
		}
		else
			iter++;
	}

	auto iterFrom = g_RequestFrom.begin();
	// g_RequestFrom���� ã�´�.
	pair<multimap<UINT64, UINT64>::iterator,
		multimap<UINT64, UINT64>::iterator> range;

	range = g_RequestFrom.equal_range(From);

	int deleteCount = 0;

	for (iterFrom = range.first; iterFrom != range.second;)
	{
		if ((iterFrom->first == From && iterFrom->second == To))
		{
			deleteCount++;
			iterFrom = g_RequestFrom.erase(iterFrom);
			break;
		}
		else
			iterFrom++;
	}

	auto iterTo = g_RequestTo.begin();
	pair<multimap<UINT64, UINT64>::iterator,
		multimap<UINT64, UINT64>::iterator> rangeTo;
	rangeTo = g_RequestTo.equal_range(To);

	for (iterTo = rangeTo.first; iterTo != rangeTo.second;)
	{
		if ((iterTo->first == To && iterTo->second == From))
		{
			deleteCount++;
			iterTo = g_RequestTo.erase(iterTo);
			break;
		}
		else
			iterTo++;
	}

	return deleteCount;
}
BYTE AddFriendRequest(UINT64 From, UINT64 To)
{
	// g_RequestFriendMap, g_RequestFrom, g_RequestTo �߰��ؾ� �ȴ�.
	int ret = FindFriendRequest(From, To);
	if (ret != df_RESULT_FRIEND_REQUEST_OK)
		return ret;

	// �߰��ϱ����� To -> From �������� ģ���� ���°��� Ȯ���Ѵ�.
	if (FindFriendRequest(To, From) == df_RESULT_FRIEND_REQUEST_AREADY)
	{
		// �� ��� To, From ������ ����� ģ���� �߰��Ѵ�.
		DeleteFriendRequest(To, From);
		AddFriend(From, To);
		AddFriend(To, From);
		return df_RESULT_FRIEND_REQUEST_OK;
	}
	else
	{
		st_FriendRequest *pRequest = new st_FriendRequest;
		pRequest->FromAccountNo = From;
		pRequest->ToAccountNo = To;

		g_RequestFriendMap.emplace(From, pRequest);

		g_RequestFrom.emplace(From, To);
		g_RequestTo.emplace(To, From);
	}

	return df_RESULT_FRIEND_REQUEST_OK;
}
BYTE FindFriendRequest(UINT64& From, UINT64& To)
{
	// �̹� ģ�����̶��?
	auto iter = g_FriendMap.begin();
	pair<multimap<UINT64, st_Friend*>::iterator,
		multimap<UINT64, st_Friend*>::iterator> range;

	range = g_FriendMap.equal_range(From);

	for (iter = range.first; iter != range.second; iter++)
	{
		if ((iter->first == From && iter->second->ToAccountNo == To) ||
			(iter->first == To && iter->second->ToAccountNo == From))
		{
			return df_RESULT_FRIEND_REQUEST_NOTFOUND;
		}
	}

	// �̹� ��û�� �� �ִٸ�? -> g_RequestFriendMap
	auto Reqiter = g_RequestFriendMap.begin();
	pair<multimap<UINT64, st_FriendRequest*>::iterator,
		multimap<UINT64, st_FriendRequest*>::iterator> Reqrange;

	Reqrange = g_RequestFriendMap.equal_range(From);

	for (Reqiter = Reqrange.first; Reqiter != Reqrange.second; Reqiter++)
	{
		if (Reqiter->first == From && Reqiter->second->ToAccountNo == To)
		{
			return df_RESULT_FRIEND_REQUEST_AREADY;
		}
	}

	return df_RESULT_FRIEND_REQUEST_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
bool NetworkInit()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	g_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN servAddr;
	ZeroMemory(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	InetPton(AF_INET, L"0.0.0.0", &servAddr.sin_addr);
	servAddr.sin_port = htons(dfNETWORK_PORT);


	if (bind(g_ListenSocket, (SOCKADDR *)&servAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		wprintf(L"bind Error() \n");
		return false;
	}

	bool optval = true;
	setsockopt(g_ListenSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&optval), sizeof(optval));


	// ��Ʈ ����
	if (listen(g_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		wprintf(L"Listen error \n");
		return false;
	}

	wprintf(L"Server Open.! \n");


	return true;
}
void DisconnectClient(const UINT64 &UserNo)
{
	st_CLIENT *pClient = FindClient(UserNo);

	if (pClient == nullptr)
		return;

	WCHAR szIP[16];
	InetNtop(AF_INET, &pClient->ConnectAdr.sin_addr, szIP, 16);
	wprintf(L"Disconnected : IP:[%s:%d]UserNo[%d] Socket[%d]  \n", szIP, ntohs(pClient->ConnectAdr.sin_port), UserNo, pClient->Socket);
	closesocket(pClient->Socket);
	g_ClientMap.erase(UserNo);
	g_ClientNo--;

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

	// ���������� 0���� ��.
	FD_SET(g_ListenSocket, &RSet);
	UserTable_No[iSocketCount] = 0;
	UserTable_SOCKET[iSocketCount] = g_ListenSocket;
	iSocketCount++;

	// �������� �� �������� ��� Ŭ���̾�Ʈ�� ���� SOCKET üũ
	for (auto iter = g_ClientMap.begin(); iter != g_ClientMap.end();)
	{
		pClient = iter->second;
		iter++;

		UserTable_No[iSocketCount] = pClient->UserNo;
		UserTable_SOCKET[iSocketCount] = pClient->Socket;

		FD_SET(pClient->Socket, &RSet);

		if (pClient->SendQ.GetDataSize() > 0)
			FD_SET(pClient->Socket, &WSet);

		iSocketCount++;

		// select �ִ�ġ ����, ������� ���̺� ������ select ȣ��� ����
		if (FD_SETSIZE <= iSocketCount)
		{
			SelectSocket(UserTable_No, UserTable_SOCKET, &RSet, &WSet);

			FD_ZERO(&RSet);
			FD_ZERO(&WSet);

			memset(UserTable_No, -1, sizeof(__int64) * FD_SETSIZE);
			memset(UserTable_SOCKET, INVALID_SOCKET, sizeof(SOCKET) * FD_SETSIZE);
			iSocketCount = 0;
		 // �� Sleep �ɸ� �ǳ�?
		}
	}
	// ��ü Ŭ���̾�Ʈ for�� ���� ��, socketCount ��ġ�� �ִٸ�
	// �߰������� ������ select ȣ���� �Ѵ�.
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
	Time.tv_usec = 0;

	iResult = select(0, pReadSet, pWriteSet, NULL, &Time);

	// ���ϰ��� 0���� �̻��� ������ ����
	if (0 < iResult)
	{
		// TableSocket�� ���鼭 � ���Ͽ� ������ �°����� Ȯ��
		for (auto iCnt = 0; iCnt < FD_SETSIZE; iCnt++)
		{
			if (pTableSocket[iCnt] == INVALID_SOCKET)
				continue;

			// WRITE üũ
			if (FD_ISSET(pTableSocket[iCnt], pWriteSet))
				SendProc(pTableNo[iCnt]);

			if (FD_ISSET(pTableSocket[iCnt], pReadSet))
			{
				if (pTableNo[iCnt] == 0)
					AcceptProc();
				else
					RecvProc(pTableNo[iCnt]);
			}
		}
	}
	else if (iResult == SOCKET_ERROR)
	{
		wprintf(L"Select Socket Error ! \n");
		return;
	}
}
void AcceptProc()
{
	st_CLIENT *pUser = nullptr;
	pUser = new st_CLIENT;
	int addrlen = sizeof(pUser->ConnectAdr);

	pUser->Socket = accept(g_ListenSocket, reinterpret_cast<SOCKADDR *>(&pUser->ConnectAdr), &addrlen);
	if (pUser->Socket == INVALID_SOCKET)
	{
		delete pUser;
		return;
	}
	g_ClientNo++;

	pUser->UserNo = g_ClientNo;

	g_ClientMap.insert(map<UINT64, st_CLIENT *>::value_type(g_ClientNo, pUser));

	WCHAR szClientIP[16] = { 0 };
	InetNtop(AF_INET, &pUser->ConnectAdr.sin_addr, szClientIP, 16);
	wprintf(L"Accpet - %s:%d Socket : %d \n", szClientIP, ntohs(pUser->ConnectAdr.sin_port), pUser->Socket);
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
	g_SendCount++;

	
	if (iResult == SOCKET_ERROR)
	{
		DWORD dwError = WSAGetLastError();
		if (dwError == WSAEWOULDBLOCK)
			return;

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

	g_RecvCount ++;

	if (SOCKET_ERROR == iResult)
	{
		DisconnectClient(pClient->UserNo);
		return;
	}

	if (0 <= iResult)
	{
		//  ��Ŷ�� �Ϸ�Ǿ����� Ȯ��
		// ��Ŷ ó���� ���� �߻��ϸ� ����
		// ��Ŷ�� �ϳ� �̻��� ���ۿ� ������ �����Ƿ� �ѹ��� ���� ó���ؾ� ��.
		while (1)
		{
			iResult = CompleteRecvPackcet(pClient);

			if (iResult == 1)
				break;

			if (iResult == -1)
			{
				wprintf(L"Packet Proc Error \n");
				return;
			}
			
		}
	}
}
int CompleteRecvPackcet(st_CLIENT *pClient)
{
	int iRecvSize = pClient->RecvQ.GetDataSize();

	// ������� ������
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
	case df_REQ_ACCOUNT_ADD:
		return ReqAddAccount(pClient, Buffer);
		break;
	case df_REQ_LOGIN:
		return ReqLogin(pClient, Buffer);
		break;
	case df_REQ_ACCOUNT_LIST:
		return ReqAccountList(pClient, Buffer);
		break;
	case df_REQ_FRIEND_LIST:
		return ReqFriendList(pClient, Buffer);
		break;
	case df_REQ_FRIEND_REQUEST_LIST:
		return ReqFriendRequestList(pClient, Buffer);
		break;
	case df_REQ_FRIEND_REPLY_LIST:
		return ReqFriendReplyList(pClient, Buffer);
		break;
	case df_REQ_FRIEND_REMOVE:
		return ReqFriendRemove(pClient, Buffer);
		break;
	case df_REQ_FRIEND_REQUEST:
		return ReqFriendRequest(pClient, Buffer);
		break;
	case df_REQ_FRIEND_CANCEL:
		return ReqFriendCancle(pClient, Buffer);
		break;
	case df_REQ_FRIEND_DENY:
		return ReqFriendDeny(pClient, Buffer);
		break;
	case df_REQ_FRIEND_AGREE:
		return ReqFriendAgree(pClient, Buffer);
		break;
	case df_REQ_STRESS_ECHO:
		return ReqStressEcho(pClient, Buffer);
		break;
	default:
		wprintf(L"Packet Error !!!!!!!!!!!!!!!!!!!!\n");
		wprintf(L"Packet Error !!!!!!!!!!!!!!!!!!!!\n");
		wprintf(L"Packet Error !!!!!!!!!!!!!!!!!!!!\n");
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ReqAddAccount(st_CLIENT *pClient, CSerializeBuffer *Buffer)  
{
	WCHAR szName[dfNICK_MAX_LEN];

	Buffer->GetData(reinterpret_cast<char *>(szName), dfNICK_MAX_LEN * 2);
	st_Account *pAccount = AddAccount(szName, g_AccountNo);

	return ResAddAccount(pClient, pAccount);
}		

bool ResAddAccount(st_CLIENT *pClient, st_Account *pAccount)	// Check
{
	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_ACCOUNT_ADD;
	Header.wPayloadSize =  sizeof(UINT64);

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(reinterpret_cast<char *>(&pAccount->AccountNo), sizeof(UINT64));

	return true;
}

bool ReqLogin(st_CLIENT *pClient, CSerializeBuffer *Buffer)	// Check
{
	UINT64 ReqloginAccountNo;
	*Buffer >> ReqloginAccountNo;

	st_Account *pAccount = FindAccount(ReqloginAccountNo);
	

	pClient->pAccount = pAccount;

	return ResLogin(pClient);
}

bool ResLogin(st_CLIENT *pClient)	// Check
{
	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_LOGIN;
	st_Account *pAccount = pClient->pAccount;
	CRingBuffer Buffer;

	if (pAccount == nullptr)
	{
		UINT64 T = 0;
		Buffer.Put(reinterpret_cast<char *>(&T), sizeof(UINT64));
	}
	else
	{
		Buffer.Put(reinterpret_cast<char *>(&pAccount->AccountNo), sizeof(UINT64));
		Buffer.Put(reinterpret_cast<char *>(pAccount->szID), dfNICK_MAX_LEN * 2);
	}

	Header.wPayloadSize = Buffer.GetUseSize();

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Header.wPayloadSize);

	return true;
}

bool ReqAccountList(st_CLIENT *pClient, CSerializeBuffer *Buffer)	// Check
{
	return ResAccountList(pClient);
}

bool ResAccountList(st_CLIENT *pClient)	// Check
{
	UINT Count = g_AccountMap.size();
	CRingBuffer Buffer(dfRECV_BUFF);
	Buffer.Put(reinterpret_cast<char *>(&Count), sizeof(UINT));

	for_each(g_AccountMap.begin(), g_AccountMap.end(), [&Buffer](pair<UINT64, st_Account *> pairs)
	{
		Buffer.Put(reinterpret_cast<char *>(&pairs.second->AccountNo),sizeof(UINT64));
		Buffer.Put(reinterpret_cast<char *>(pairs.second->szID), dfNICK_MAX_LEN * 2);
	});

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_ACCOUNT_LIST;
	Header.wPayloadSize = Buffer.GetUseSize();

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Header.wPayloadSize);

	return true;
}

bool ReqFriendList(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	// g_FriendMap ���� From(Key)�� �Ѱ��ִ°ɷ�
	return ResFriendList(pClient, pClient->pAccount);
}
bool ResFriendList(st_CLIENT *pClient, st_Account* Account)
{
	auto iter = g_FriendMap.begin();
	UINT64& AccountNo = Account->AccountNo;
	UINT UserCount = 0;
	CRingBuffer Buffer(dfRECV_BUFF);

	pair<multimap<UINT64, st_Friend*>::iterator, 
		multimap<UINT64, st_Friend*>::iterator> range;

	range = g_FriendMap.equal_range(AccountNo);

	for (iter = range.first; iter != range.second; ++iter)
	{
		if (iter->first == AccountNo)
		{
			UserCount++;

			Buffer.Put(reinterpret_cast<char *>(&iter->second->ToAccountNo), sizeof(UINT64));
			st_Account *pTemp = FindAccount(iter->second->ToAccountNo);

			Buffer.Put(reinterpret_cast<char *>(pTemp->szID), dfNICK_MAX_LEN * 2);
		}
	}

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_LIST;
	Header.wPayloadSize = Buffer.GetUseSize() + sizeof(UINT);

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ << static_cast<int>(UserCount);
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetUseSize());
	return true;
}

bool ReqFriendRequestList(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	return ResFriendRequestList(pClient);
}
bool ResFriendRequestList(st_CLIENT *pClient)
{
	auto iter = g_RequestFrom.begin();

	UINT FriendCount = 0;
	CRingBuffer Buffer(dfRECV_BUFF);
	pair<multimap<UINT64, UINT64>::iterator,
		multimap<UINT64, UINT64>::iterator> range;

	range = g_RequestFrom.equal_range(pClient->pAccount->AccountNo);

	for (iter = range.first; iter != range.second; ++iter)
	{
		if (iter->first == pClient->pAccount->AccountNo)
		{
			FriendCount++;

			Buffer.Put(reinterpret_cast<char *>(&iter->second), sizeof(UINT64));
			st_Account *pTemp = FindAccount(iter->second);

			Buffer.Put(reinterpret_cast<char *>(pTemp->szID), dfNICK_MAX_LEN * 2);
			break;
		}
	}

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_REQUEST_LIST;
	Header.wPayloadSize = Buffer.GetUseSize() + sizeof(UINT);

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ << static_cast<int>(FriendCount);
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetUseSize());

	return true;

}

bool ReqFriendReplyList(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	// pClient�� ������ȣ�� ������ Ȯ�� �ؾߵȴ�.
	return ResFriendReplyList(pClient);
}

bool ResFriendReplyList(st_CLIENT *pClient)
{
	auto iter = g_RequestTo.begin();
	UINT FriendCount = 0;
	CRingBuffer Buffer(dfRECV_BUFF);
	pair<multimap<UINT64, UINT64>::iterator,
		multimap<UINT64, UINT64>::iterator> range;


	UINT64 SenderAccountNo = pClient->pAccount->AccountNo;

	range = g_RequestTo.equal_range(SenderAccountNo);

	for (iter = range.first; iter != range.second; iter++)
	{
		if (iter->first == SenderAccountNo)
		{
			FriendCount++;

			Buffer.Put(reinterpret_cast<char *>(&iter->second), sizeof(UINT64));
			st_Account *pTemp = FindAccount(iter->second);

			Buffer.Put(reinterpret_cast<char *>(pTemp->szID), dfNICK_MAX_LEN * 2);
			break;
		}
	}

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_REPLY_LIST;
	Header.wPayloadSize = Buffer.GetUseSize() + sizeof(UINT);

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ << static_cast<int>(FriendCount);
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetUseSize());

	return true;
}


bool ReqFriendRemove(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	UINT64 FriendAccountNo; // �������ϴ� ���� ��ȣ
	*Buffer >> FriendAccountNo;

	return ResFriendRemove(pClient, FriendAccountNo);
}

bool ResFriendRemove(st_CLIENT *pClient, UINT64& FriendAccountNo)
{
	/*
	 A�� B�� ģ�����¶��
	 A -> B, B -> A�� �� ����. �� ���� �����ؾߵȴ�.
	 ������ �� 
	 Key�� A,  ������(A, B)  &  Key�� B, ������(B, A) ��Ȳ
	 
	 ���߿� �ϳ��� ������ df_RESULT_FRIEND_REMOVE_FAIL
	 �Ѵ� ���ٸ� df_RESULT_FRIEND_REMOVE_NOTFRIEND
	 �Ѵ� �ִٸ� df_RESULT_FRIEND_REMOVE_OK
	*/
	CRingBuffer Buffer(dfRECV_BUFF);
	UINT64 MyAccountNo = pClient->pAccount->AccountNo;
	
	int deleteCount = DeleteFriend(MyAccountNo, FriendAccountNo);
	BYTE errCode;
	Buffer.Put(reinterpret_cast<char *>(&FriendAccountNo), sizeof(UINT64));

	switch (deleteCount)
	{
	case 2:
		errCode = (df_RESULT_FRIEND_REMOVE_OK);
		break;
	case 0:
		errCode = (df_RESULT_FRIEND_REMOVE_NOTFRIEND);
		break;
	}
	Buffer.Put(reinterpret_cast<char *>(&errCode), sizeof(BYTE));


	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_REMOVE;
	Header.wPayloadSize = Buffer.GetUseSize();

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetUseSize());

	return true;
}


bool ReqFriendRequest(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	UINT64	FriendAccountNo;
	*Buffer >> FriendAccountNo;

	return ResFriendRequest(pClient, FriendAccountNo);
}

bool ResFriendRequest(st_CLIENT *pClient, UINT64& FriendAccountNo)
{
	UINT64 MyAccount = pClient->pAccount->AccountNo;
	CRingBuffer Buffer(dfRECV_BUFF);
	BYTE ret;

	if (MyAccount == FriendAccountNo)
	{
		ret = df_RESULT_FRIEND_REQUEST_NOTFOUND;
	}
	else 
		ret = AddFriendRequest(MyAccount, FriendAccountNo);


	Buffer.Put(reinterpret_cast<char *>(&FriendAccountNo), sizeof(UINT64));
	Buffer.Put(reinterpret_cast<char *>(&ret), sizeof(BYTE));

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_REQUEST;
	Header.wPayloadSize = Buffer.GetUseSize();


	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetUseSize());

	return true;
}


bool ReqFriendDeny(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	UINT64 FriendAccountNo; // �ź��� ���� ��ȣ
	*Buffer >> FriendAccountNo;

	return ResFriendDeny(pClient, FriendAccountNo);
}
bool ResFriendDeny(st_CLIENT *pClient, UINT64& FriendAccountNo)
{
	// ģ�� ��û�� �ź���
	// g_RequestFrom���� �� �ڽ��� ������ȣ, FriendAccountNo�� �����.
	// g_RequestTo���� FriendAccountNo, �� �ڽ��� ������ȣ�� �����.
	CRingBuffer Buffer(dfRECV_BUFF);

	UINT64 MyAccountNo = pClient->pAccount->AccountNo;
	BYTE delRet = DeleteFriendRequest(FriendAccountNo, MyAccountNo);


	Buffer.Put(reinterpret_cast<char *>(&FriendAccountNo), sizeof(UINT64));

	switch (delRet)
	{
	case 2:
		delRet = df_RESULT_FRIEND_DENY_OK;
		break;
	case 1:
		delRet = (df_RESULT_FRIEND_DENY_FAIL);
		break;
	case 0:
		delRet = (df_RESULT_FRIEND_DENY_NOTFRIEND);
		break;
	}

	Buffer.Put(reinterpret_cast<char *>(&delRet), sizeof(BYTE));

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_DENY;
	Header.wPayloadSize = Buffer.GetUseSize();

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetUseSize());

	return true;
}


bool ReqFriendCancle(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	UINT64	FriendAccountNo;
	*Buffer >> FriendAccountNo;

	return ResFriendCancle(pClient, FriendAccountNo);
}

bool ResFriendCancle(st_CLIENT *pClient, UINT64& FriendAccountNo)
{
	CRingBuffer Buffer(dfRECV_BUFF);
	UINT64& MyAccount = pClient->pAccount->AccountNo;

	BYTE delCnt = DeleteFriendRequest(MyAccount, FriendAccountNo);

	Buffer.Put(reinterpret_cast<char *>(&FriendAccountNo), sizeof(UINT64));

	switch (delCnt)
	{
	case 0:
		delCnt = (df_RESULT_FRIEND_CANCEL_NOTFRIEND);
		break;
	case 1:
		delCnt = (df_RESULT_FRIEND_CANCEL_FAIL);
		break;
	case 2:
		delCnt = (df_RESULT_FRIEND_CANCEL_OK);
		break;
	}

	Buffer.Put(reinterpret_cast<char *>(&delCnt), sizeof(BYTE));

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_CANCEL;
	Header.wPayloadSize = Buffer.GetUseSize();


	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetUseSize());

	return true;
}

bool ReqFriendAgree(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	UINT64	FriendAccountNo;
	*Buffer >> (FriendAccountNo);

	return ResFriendAgree(pClient, FriendAccountNo);
}

bool ResFriendAgree(st_CLIENT *pClient, UINT64& FriendAccountNo)
{
	CRingBuffer Buffer(dfRECV_BUFF);
	UINT64 MyAccount = pClient->pAccount->AccountNo;

	// FriendAccount -> MyAccount���� �°� ����� ģ���� �߰��Ѵ�.
	BYTE retCnt = DeleteFriendRequest(FriendAccountNo, MyAccount);

	Buffer.Put(reinterpret_cast<char *>(&FriendAccountNo), sizeof(UINT64));

	switch (retCnt)
	{
	case 2:
	case 1:
		retCnt = (df_RESULT_FRIEND_AGREE_OK);
		AddFriend(MyAccount, FriendAccountNo);
		AddFriend(FriendAccountNo, MyAccount);
		break;
	case 0:
		retCnt = (df_RESULT_FRIEND_AGREE_NOTFRIEND);
		break;
	}

	Buffer.Put(reinterpret_cast<char *>(&retCnt), sizeof(BYTE));

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_AGREE;
	Header.wPayloadSize = Buffer.GetUseSize();

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetUseSize());

	return true;
}


bool ReqStressEcho(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	WORD szSize;
	*Buffer >> szSize;

	memset(gData, '\0', szSize);
	WORD ret = Buffer->GetData(gData, szSize);

	if (szSize  != ret)
	{
		wprintf(L"ReqStressEcho: UnComplete Get Data \n");
		pClient->RecvQ.Clear();		
		return false;
	}


	return ResStressEcho(pClient, gData, szSize);
}

bool ResStressEcho(st_CLIENT *pClient, char *szStr, WORD strSize)
{
	st_PACKET_HEADER Header;
	CRingBuffer Buffer(dfRECV_BUFF);
	pClient->SendQ.Clear();

	Buffer.Put(reinterpret_cast<char *>(&strSize), sizeof(WORD));
	Buffer.Put(szStr, strSize);

	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_STRESS_ECHO;
	Header.wPayloadSize = Buffer.GetUseSize();

	
	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetReadBufferPtr(), Header.wPayloadSize);

	return true;
}