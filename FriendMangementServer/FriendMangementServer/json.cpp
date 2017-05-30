#include "json.h"
#include <map>
#include <algorithm>
using namespace std;

extern map<UINT64, st_Account *> g_AccountMap;
extern multimap<UINT64, st_Friend *> g_FriendMap;
extern multimap<UINT64, st_FriendRequest *> g_RequestFriendMap;

bool UTF8toUTF16(const char *szText, WCHAR *szBuff, int iBuffLen)
{
	int iRe = MultiByteToWideChar(CP_UTF8, 0, szText, strlen(szText), szBuff, iBuffLen);
	if (iRe < iBuffLen)
		szBuff[iRe] = L'\0';
	return true;
}

bool UTF16toUTF8(char *szText, const WCHAR *szBuff, int iBuffLen)
{
	int iRe = WideCharToMultiByte(CP_UTF8, 0, szBuff, lstrlenW(szBuff), static_cast<LPSTR>(szText), iBuffLen, NULL, NULL);
	if (iRe < iBuffLen)
		szText[iRe] = '\0';
	return true;
}

bool LoadData(void)
{
	FILE *fp;
	_wfopen_s(&fp, L"DB_json.txt", L"rt");
	
	if (fp == nullptr)
		return false;

	fseek(fp, 0, SEEK_END);
	int iFileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *pJson = new char[iFileSize + 1];
	ZeroMemory(pJson, iFileSize + 1);
	fread(pJson, iFileSize, 1, fp);

	fclose(fp);
	Document Doc;
	Doc.Parse(pJson);

	UINT64 AccountNo;
	WCHAR szNickname[dfNICK_MAX_LEN];

	Value &AccountArray = Doc["Account"];
	for (SizeType i = 0; i < AccountArray.Size(); i++)
	{
		Value &AccountObject = AccountArray[i];
		AccountNo = AccountObject["AccountNo"].GetUint64();
		UTF8toUTF16(AccountObject["Nickname"].GetString(), szNickname, dfNICK_MAX_LEN);

		// 읽혀진 데이터 사용
		AddAccount(szNickname, AccountNo);
	}

	Value &FriendArray = Doc["Friend"];
	UINT FriendNo, FromAccountNo, ToAccountNo;
	for (SizeType i = 0; i < FriendArray.Size(); i++)
	{
		Value &FriendObject = FriendArray[i];
		FriendNo = FriendObject["FriendNo"].GetUint64();
		FromAccountNo = FriendObject["FromAccountNo"].GetUint64();
		ToAccountNo = FriendObject["ToAccountNo"].GetUint64();

		// 읽혀진 데이터 사용
		AddFriend(FromAccountNo, ToAccountNo);
	}

	Value &FriendRequestArray = Doc["FriendRequest"];
	for (SizeType i = 0; i < FriendRequestArray.Size(); i++)
	{
		Value &FriendObject = FriendRequestArray[i];
		FriendNo = FriendObject["RequestNo"].GetUint64();
		FromAccountNo = FriendObject["FromAccountNo"].GetUint64();
		ToAccountNo = FriendObject["ToAccountNo"].GetUint64();
		// 읽혀진 데이터 사용

		AddFriendRequest(FromAccountNo, ToAccountNo);
	}

	return true;
}

bool SaveData(void)
{
	StringBuffer StringJSON;
	Writer<StringBuffer, UTF16<>> writer(StringJSON);

	// Account 정보 저장
	writer.StartObject();
	writer.String(L"Account");
	writer.StartArray();
	for_each(g_AccountMap.begin(), g_AccountMap.end(), [&writer](pair<UINT64, st_Account *> pairs)
	{
		writer.StartObject();
		writer.String(L"AccountNo");
		writer.Uint64(pairs.first);
		writer.String(L"Nickname");
		writer.String(pairs.second->szID);
		writer.EndObject();
	});
	writer.EndArray();

	// 친구정보 저장
	writer.String(L"Friend");
	writer.StartArray();
	for_each(g_FriendMap.begin(), g_FriendMap.end(), [&writer](pair<UINT64, st_Friend *> pairs)
	{
		writer.StartObject();
		writer.String(L"FriendNo");
		writer.Uint64(pairs.first);
		writer.String(L"FromAccountNo");
		writer.Uint64(pairs.second->FromAccountNo);
		writer.String(L"ToAccountNo");
		writer.Uint64(pairs.second->ToAccountNo);
		writer.EndObject();
	});
	writer.EndArray();

	// 친구요청 정보 저장
	writer.String(L"FriendRequest");
	writer.StartArray();
	for_each(g_RequestFriendMap.begin(), g_RequestFriendMap.end(), [&writer](pair<UINT64, st_FriendRequest *> pairs)
	{
		writer.StartObject();
		writer.String(L"RequestNo");
		writer.Uint64(pairs.first);
		writer.String(L"FromAccountNo");
		writer.Uint64(pairs.second->FromAccountNo);
		writer.String(L"ToAccountNo");
		writer.Uint64(pairs.second->ToAccountNo);
		writer.EndObject();
	});
	writer.EndArray();
	writer.EndObject();

	const char *pJson = StringJSON.GetString();

	FILE *fp;
	_wfopen_s(&fp, L"DB_json.txt", L"wt");

	if (fp == nullptr)
		return false;

	fwrite(pJson, StringJSON.GetSize(), 1, fp);

	fclose(fp);

	return true;
}