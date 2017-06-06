#include "Server.h"
#include "json.h"

int main()
{
	setlocale(LC_ALL, "");
	timeBeginPeriod(1);
	DWORD timeTickCount;
	DWORD dwTick;
	if (!NetworkInit())
	{
		wprintf(L"네트워크 초기화 에러 \n");
		return 0;
	} 

	if (!LoadData())
	{
		wprintf(L"Read Unicode FilePointer Error \n");
	}
	timeTickCount = GetTickCount64();
	dwTick = timeTickCount;

	while (1)
	{

		NetworkProcess();
		
		if (GetAsyncKeyState(0x51) && GetAsyncKeyState(0x39))  // Q + 숫자 9
		{
			SaveData();
			timeEndPeriod(1);
			ProfileOutText("FreqTest.txt");
			NetworkClear();
			return 0;
		}
		
		dwTick = GetTickCount64();
		if (dwTick - timeTickCount  > 1000)
		{
			Draw();
			dwTick = 0;
			timeTickCount = GetTickCount64();
		}

		Sleep(0);
	}
}
