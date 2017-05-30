#include "Server.h"
#include <map>
#include <algorithm>
using namespace std;

map<UINT64, st_CLIENT *> g_ClientMap;
map<UINT64, st_Account *> g_AccountMap;


// 이미 만들어진 친구목록 <From, 친구 구조체>
multimap<UINT64, st_Friend *> g_FriendMap; 
// 친구요청 <From, 친구 요청 구조체>
multimap<UINT64, st_FriendRequest *> g_RequestFriendMap;
// From 친구요청 한사람(Key), To 친구요청 받은사람(Data)
multimap<UINT64, UINT64> g_RequestFrom;
// To 친구요청 받은사람 (Key), From 친구요청 한사람(Data)
multimap<UINT64, UINT64> g_RequestTo;

SOCKET g_ListenSocket;

UINT64 g_ClientNo = 0;
UINT64 g_AccountNo = 0;

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


int DeleteFriendRequest(UINT64& From, UINT64& To)
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
	// g_RequestFrom에서 찾는다.
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
	// g_RequestFriendMap, g_RequestFrom, g_RequestTo 추가해야 된다.
	int ret = FindFriendRequest(From, To);
	if (ret != df_RESULT_FRIEND_REQUEST_OK)
		return ret;

	// 추가하기전에 To -> From 방향으로 친구가 들어온건지 확인한다.
	if (FindFriendRequest(To, From) == df_RESULT_FRIEND_REQUEST_AREADY)
	{
		// 이 경우 To, From 방향을 지우고 친구로 추가한다.
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
	// 이미 친구사이라면?
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

	// 이미 요청이 들어가 있다면? -> g_RequestFriendMap
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


	// 포트 오픈
	if (listen(g_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		wprintf(L"Listen error \n");
		return false;
	}

	u_long on = 1;
	ioctlsocket(g_ListenSocket, FIONBIO, &on);

	wprintf(L"Server Open.! \n");


	return true;
}
void DisconnectClient(const UINT64 &UserNo)
{
	st_CLIENT *pClient = FindClient(UserNo);
	g_ClientMap.erase(UserNo);
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

	// 리슨소켓은 0으로 함.
	FD_SET(g_ListenSocket, &RSet);
	UserTable_No[iSocketCount] = 0;
	UserTable_SOCKET[iSocketCount] = g_ListenSocket;
	iSocketCount++;

	// 리슨소켓 및 접속중인 모든 클라이언트에 대해 SOCKET 체크
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
	Time.tv_usec = 0;

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
	{
		pClient->SendQ.Clear();
		return;
	}
	int iResult = send(pClient->Socket, pClient->SendQ.GetReadBufferPtr(), iSendSize, 0);


	if (iResult == SOCKET_ERROR)
	{
		DWORD dwError = WSAGetLastError();
		if (dwError == WSAEWOULDBLOCK)
			return;

		closesocket(pClient->Socket);
		DisconnectClient(pClient->UserNo);
		
		return;
	}

	pClient->SendQ.MoveReadPos(iResult);

}
void RecvProc(const UINT64& UserNo)
{
	st_CLIENT *pClient;
	int iResult;

	pClient = FindClient(UserNo);

	if (pClient == nullptr)
		return;

	iResult = recv(pClient->Socket, pClient->RecvQ.GetReadBufferPtr(), dfRECV_BUFF, 0);
	pClient->RecvQ.MoveWritePos(iResult);

	if (SOCKET_ERROR == iResult)
	{
		pClient->RecvQ.Clear();
		closesocket(pClient->Socket);
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
				wprintf(L"Packet Error : UserNo %lld \n", pClient->UserNo);
				return;
			}
		}
	}

	pClient->RecvQ.Clear();
}
int CompleteRecvPackcet(st_CLIENT *pClient)
{
	int iRecvSize = pClient->RecvQ.GetDataSize();

	// 헤더보다 적을때
	if (sizeof(st_PACKET_HEADER) > iRecvSize)
		return 1;

	BYTE byCode;
	WORD MsgType;
	WORD PayloadSize;

	pClient->RecvQ >> byCode;
	pClient->RecvQ >> MsgType;
	pClient->RecvQ >> PayloadSize;

	if (byCode != dfPACKET_CODE)
		return -1;

	if (PayloadSize + sizeof(st_PACKET_HEADER) > (WORD)iRecvSize)
		return 1;

	CSerializeBuffer Buffer;

	if (PayloadSize != Buffer.PutData(pClient->RecvQ.GetReadBufferPtr(), PayloadSize))
		return -1;

	if (!PacketProc(pClient, MsgType, &Buffer))
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
		break;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check
bool ReqAddAccount(st_CLIENT *pClient, CSerializeBuffer *Buffer)  
{
	WCHAR szName[dfNICK_MAX_LEN];

	Buffer->GetData(reinterpret_cast<char *>(szName), dfNICK_MAX_LEN * 2);
	st_Account *pAccount = AddAccount(szName, g_AccountNo);

	return ResAddAccount(pClient, pAccount);
}		
// Check
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
// Check
bool ReqLogin(st_CLIENT *pClient, CSerializeBuffer *Buffer)	// Check
{
	UINT64 ReqloginAccountNo;
	*Buffer >> ReqloginAccountNo;

	st_Account *pAccount = FindAccount(ReqloginAccountNo);
	

	pClient->pAccount = pAccount;

	return ResLogin(pClient);
}
// Check
bool ResLogin(st_CLIENT *pClient)	// Check
{
	st_PACKET_HEADER Header;
	CSerializeBuffer Buffer;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_LOGIN;
	st_Account *pAccount = pClient->pAccount;

	if (pAccount == nullptr)
	{
		Buffer << static_cast<__int64>(0);
	}
	else
	{
		Buffer << static_cast<__int64>(pAccount->AccountNo);
		Buffer.PutData(reinterpret_cast<char *>(pAccount->szID), dfNICK_MAX_LEN * 2);
	}

	Header.wPayloadSize = Buffer.GetDataSize();

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Header.wPayloadSize);

	return true;
}
// Check
bool ReqAccountList(st_CLIENT *pClient, CSerializeBuffer *Buffer)	// Check
{
	return ResAccountList(pClient);
}
// Check
bool ResAccountList(st_CLIENT *pClient)	// Check
{
	UINT Count = g_AccountMap.size();
	CSerializeBuffer Buffer;
	Buffer << static_cast<int>(Count);

	for_each(g_AccountMap.begin(), g_AccountMap.end(), [&Buffer](pair<UINT64, st_Account *> pairs)
	{
		Buffer << static_cast<__int64>(pairs.second->AccountNo);
		Buffer.PutData(reinterpret_cast<char *>(pairs.second->szID), dfNICK_MAX_LEN * 2);
	});

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_ACCOUNT_LIST;
	Header.wPayloadSize = Buffer.GetDataSize();

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Header.wPayloadSize);

	return true;
}

bool ReqFriendList(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	// g_FriendMap 에서 From(Key)값 넘겨주는걸로
	return ResFriendList(pClient, pClient->pAccount);
}
bool ResFriendList(st_CLIENT *pClient, st_Account* Account)
{
	auto iter = g_FriendMap.begin();
	UINT64& AccountNo = Account->AccountNo;
	UINT UserCount = 0;
	CSerializeBuffer Buffer;

	pair<multimap<UINT64, st_Friend*>::iterator, 
		multimap<UINT64, st_Friend*>::iterator> range;

	range = g_FriendMap.equal_range(AccountNo);

	for (iter = range.first; iter != range.second; ++iter)
	{
		if (iter->first == AccountNo)
		{
			UserCount++;

			Buffer << static_cast<__int64>(iter->second->ToAccountNo);
			st_Account *pTemp = FindAccount(iter->second->ToAccountNo);

			Buffer.PutData(reinterpret_cast<char *>(pTemp->szID), dfNICK_MAX_LEN * 2);
		}
	}

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_LIST;
	Header.wPayloadSize = Buffer.GetDataSize() + sizeof(UINT);

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ << static_cast<int>(UserCount);
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetDataSize());
	return true;
}
// 친구요청 보낸 목록  요청
bool ReqFriendRequestList(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	return ResFriendRequestList(pClient);
}
bool ResFriendRequestList(st_CLIENT *pClient)
{
	auto iter = g_RequestFrom.begin();

	UINT FriendCount = 0;
	CSerializeBuffer Buffer;
	pair<multimap<UINT64, UINT64>::iterator,
		multimap<UINT64, UINT64>::iterator> range;

	range = g_RequestFrom.equal_range(pClient->pAccount->AccountNo);

	for (iter = range.first; iter != range.second; ++iter)
	{
		if (iter->first == pClient->pAccount->AccountNo)
		{
			FriendCount++;

			Buffer << static_cast<__int64>(iter->second);
			st_Account *pTemp = FindAccount(iter->second);

			Buffer.PutData(reinterpret_cast<char *>(pTemp->szID), dfNICK_MAX_LEN * 2);
			break;
		}
	}

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_REQUEST_LIST;
	Header.wPayloadSize = Buffer.GetDataSize() + sizeof(UINT);

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ << static_cast<int>(FriendCount);
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetDataSize());

	return true;

}
// 친구요청 받은거 목록  요청 <일단 되는듯>
bool ReqFriendReplyList(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	// pClient의 계정번호와 같은지 확인 해야된다.
	return ResFriendReplyList(pClient);
}
//  <일단 되는듯>
bool ResFriendReplyList(st_CLIENT *pClient)
{
	auto iter = g_RequestTo.begin();
	UINT FriendCount = 0;
	CSerializeBuffer Buffer;
	pair<multimap<UINT64, UINT64>::iterator,
		multimap<UINT64, UINT64>::iterator> range;


	UINT64 SenderAccountNo = pClient->pAccount->AccountNo;

	range = g_RequestTo.equal_range(SenderAccountNo);

	for (iter = range.first; iter != range.second; iter++)
	{
		if (iter->first == SenderAccountNo)
		{
			FriendCount++;

			Buffer << static_cast<__int64>(iter->second);
			st_Account *pTemp = FindAccount(iter->second);

			Buffer.PutData(reinterpret_cast<char *>(pTemp->szID), dfNICK_MAX_LEN * 2);
			break;
		}
	}

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_REPLY_LIST;
	Header.wPayloadSize = Buffer.GetDataSize() + sizeof(UINT);

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ << static_cast<int>(FriendCount);
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetDataSize());

	return true;
}

// 친구관계 끊기
bool ReqFriendRemove(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	UINT64 FriendAccountNo; // 끊고자하는 계정 번호
	*Buffer >> FriendAccountNo;

	return ResFriendRemove(pClient, FriendAccountNo);
}
bool ResFriendRemove(st_CLIENT *pClient, UINT64& FriendAccountNo)
{
	/*
	 A와 B가 친구상태라면
	 A -> B, B -> A가 들어가 있음. 이 둘을 제거해야된다.
	 데이터 상 
	 Key가 A,  데이터(A, B)  &  Key가 B, 데이터(B, A) 상황
	 
	 둘중에 하나만 있을때 df_RESULT_FRIEND_REMOVE_FAIL
	 둘다 없다면 df_RESULT_FRIEND_REMOVE_NOTFRIEND
	 둘다 있다면 df_RESULT_FRIEND_REMOVE_OK
	*/
	CSerializeBuffer Buffer;

	UINT64 MyAccountNo = pClient->pAccount->AccountNo;
	
	int deleteCount = DeleteFriend(MyAccountNo, FriendAccountNo);

	Buffer << static_cast<__int64>(FriendAccountNo);

	switch (deleteCount)
	{
	case 2:
		Buffer << static_cast<BYTE>(df_RESULT_FRIEND_REMOVE_OK);
		break;
	case 0:
		Buffer << static_cast<BYTE>(df_RESULT_FRIEND_REMOVE_NOTFRIEND);
		break;
	}

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_REMOVE;
	Header.wPayloadSize = Buffer.GetDataSize();

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetDataSize());
	return true;
}

// 친구관계 요청  <일단 되는듯>
bool ReqFriendRequest(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	UINT64	FriendAccountNo;
	*Buffer >> FriendAccountNo;

	return ResFriendRequest(pClient, FriendAccountNo);
}
//  <일단 되는듯>
bool ResFriendRequest(st_CLIENT *pClient, UINT64& FriendAccountNo)
{
	UINT64 MyAccount = pClient->pAccount->AccountNo;
	CSerializeBuffer Buffer;
	int ret;

	if (MyAccount == FriendAccountNo)
	{
		ret = df_RESULT_FRIEND_REQUEST_NOTFOUND;
	}
	else 
		ret = AddFriendRequest(MyAccount, FriendAccountNo);


	Buffer << static_cast<__int64>(FriendAccountNo);
	Buffer << static_cast<BYTE>(ret);

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_REQUEST;
	Header.wPayloadSize = Buffer.GetDataSize();


	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetDataSize());

	return true;
}

// 친구요청 거부
bool ReqFriendDeny(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	UINT64 FriendAccountNo; // 거부할 계정 번호
	*Buffer >> FriendAccountNo;

	return ResFriendDeny(pClient, FriendAccountNo);
}
bool ResFriendDeny(st_CLIENT *pClient, UINT64& FriendAccountNo)
{
	// 친구 요청을 거부함
	// g_RequestFrom에서 내 자신의 계정번호, FriendAccountNo를 지운다.
	// g_RequestTo에서 FriendAccountNo, 내 자신의 계정번호를 지운다.
	CSerializeBuffer Buffer;

	UINT64 MyAccountNo = pClient->pAccount->AccountNo;
	int delRet = DeleteFriendRequest(FriendAccountNo, MyAccountNo);


	Buffer << static_cast<__int64>(FriendAccountNo);

	switch (delRet)
	{
	case 2:
		Buffer << static_cast<BYTE>(df_RESULT_FRIEND_DENY_OK);
		break;
	case 1:
		Buffer << static_cast<BYTE>(df_RESULT_FRIEND_DENY_FAIL);
		break;
	case 0:
		Buffer << static_cast<BYTE>(df_RESULT_FRIEND_DENY_NOTFRIEND);
		break;
	}

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_DENY;
	Header.wPayloadSize = Buffer.GetDataSize();

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetDataSize());

	return true;
}

// 친구요청 취소
bool ReqFriendCancle(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	UINT64	FriendAccountNo;
	*Buffer >> FriendAccountNo;

	return ResFriendCancle(pClient, FriendAccountNo);
}
bool ResFriendCancle(st_CLIENT *pClient, UINT64& FriendAccountNo)
{
	CSerializeBuffer Buffer;
	UINT64& MyAccount = pClient->pAccount->AccountNo;

	int delCnt = DeleteFriendRequest(MyAccount, FriendAccountNo);

	Buffer << static_cast<__int64>(FriendAccountNo);

	switch (delCnt)
	{
	case 0:
		Buffer << static_cast<BYTE>(df_RESULT_FRIEND_CANCEL_NOTFRIEND);
		break;
	case 1:
		Buffer << static_cast<BYTE>(df_RESULT_FRIEND_CANCEL_FAIL);
		break;
	case 2:
		Buffer << static_cast<BYTE>(df_RESULT_FRIEND_CANCEL_OK);
		break;
	}

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_CANCEL;
	Header.wPayloadSize = Buffer.GetDataSize();


	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetDataSize());

	return true;
}
// <일단됨>
bool ReqFriendAgree(st_CLIENT *pClient, CSerializeBuffer *Buffer)
{
	UINT64	FriendAccountNo;
	*Buffer >> (FriendAccountNo);

	return ResFriendAgree(pClient, FriendAccountNo);
}
// <일단됨>
bool ResFriendAgree(st_CLIENT *pClient, UINT64& FriendAccountNo)
{
	CSerializeBuffer Buffer;
	UINT64 MyAccount = pClient->pAccount->AccountNo;

	// FriendAccount -> MyAccount으로 온걸 지우고 친구로 추가한다.
	int retCnt = DeleteFriendRequest(FriendAccountNo, MyAccount);

	Buffer << static_cast<__int64>(FriendAccountNo);

	switch (retCnt)
	{
	case 2:
	case 1:
		Buffer << static_cast<BYTE>(df_RESULT_FRIEND_AGREE_OK);
		AddFriend(MyAccount, FriendAccountNo);
		AddFriend(FriendAccountNo, MyAccount);
		break;
	case 0:
		Buffer << static_cast<BYTE>(df_RESULT_FRIEND_AGREE_NOTFRIEND);
		break;
	}

	st_PACKET_HEADER Header;
	Header.byCode = dfPACKET_CODE;
	Header.wMsgType = df_RES_FRIEND_AGREE;
	Header.wPayloadSize = Buffer.GetDataSize();

	pClient->SendQ.PutData(reinterpret_cast<char *>(&Header), sizeof(st_PACKET_HEADER));
	pClient->SendQ.PutData(Buffer.GetBufferPtr(), Buffer.GetDataSize());

	return true;
}