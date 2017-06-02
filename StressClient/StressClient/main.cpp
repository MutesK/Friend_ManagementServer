#include "Stress.h"
#include <locale.h>
int main()
{
	setlocale(LC_ALL, "");
	timeBeginPeriod(1);
	
	DWORD timeTickCount;
	DWORD dwTick;

	NetworkInit();

	timeTickCount = GetTickCount64();
	dwTick = timeTickCount;
	
	SendMessages();
	while (1)
	{
		// RecvQ, SendQÀÇ ÀÀ´äÀÌ ¿©±â¼­ ÀÌ·ïÁü.
		NetworkProcess();

		
		dwTick = GetTickCount64();
		if (dwTick - timeTickCount  > 1000)
		{
			Draw(dwTick);

			dwTick = 0;
			timeTickCount = GetTickCount64();
		}
		SendMessages();
	}
}