#include "Server.h"
#include "json.h"

int main()
{
	setlocale(LC_ALL, "");
	timeBeginPeriod(1);

	if (!NetworkInit())
	{
		wprintf(L"네트워크 초기화 에러 \n");
		return 0;
	} 

	if (!LoadData())
	{
		wprintf(L"Read Unicode FilePointer Error \n");
	}

	while (1)
	{
		NetworkProcess();

		if (GetAsyncKeyState(0x51))
		{
			SaveData();
			timeEndPeriod(1);
			return 0;
		}
	}
}
