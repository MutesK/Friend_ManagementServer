#include <iostream>
#include "Profiler.h"

LARGE_INTEGER g_lFreqency;
double g_fMicroFreqeuncy;
PROFILE_DATA Data[PROFILE_NUMMAX];

CProfile Profile;


CProfile::CProfile()
{
	for (int i = 0; i < PROFILE_NUMMAX; i++)
	{
		Data[i].flag = false;
		Data[i].iTotalTime = 0;
		Data[i].iMin[0] = 0x7fffffffffffffff;
		Data[i].iMin[1] = 0x7fffffffffffffff;
		Data[i].iCall = 0;
	}

	QueryPerformanceFrequency(&g_lFreqency);
	g_fMicroFreqeuncy = 1000000;
}

void ProfileBegin(char *_fName)
{
	int index = 0;

	while (index < PROFILE_NUMMAX)
	{
		if (Data[index].flag == true)
		{
			if (strcmp(Data[index].szName, _fName) == 0)
			{
				Data[index].iCall++;
				QueryPerformanceCounter(&(Data[index].startTime));
				return;
			}
			index++;
			continue;
		}


		Data[index].flag = true;
		strcpy_s(Data[index].szName, 64, _fName);
		Data[index].iCall++;
		QueryPerformanceCounter(&(Data[index].startTime));
		return;
	}

	throw 1;
}
void ProfileEnd(char *_fName)
{
	int index = 0;

	while (index < PROFILE_NUMMAX)
	{
		if (Data[index].flag == true)
		{
			if (strcmp(_fName, Data[index].szName) == 0)
			{
				LARGE_INTEGER endTime;
				QueryPerformanceCounter(&endTime);

				__int64 time = (endTime.QuadPart - Data[index].startTime.QuadPart);
				// time * 1000 = ms
				// 0ÀÌ Á¦ÀÏ ÀÛÀ½ -> ¹«½Ã
				// 
				if (time <= Data[index].iMin[1])
				{
					if (time <= Data[index].iMin[0])
					{
						Data[index].iMin[1] = Data[index].iMin[0];
						Data[index].iMin[0] = time;
					}
					else
					{
						Data[index].iMin[1] = time;
					}
				}

				if (time >= Data[index].iMax[1])
				{
					if (time >= Data[index].iMax[0])
					{
						Data[index].iMax[1] = Data[index].iMax[0];
						Data[index].iMax[0] = time;
					}
					else
					{
						Data[index].iMax[1] = time;
					}
				}

				Data[index].iTotalTime += time;

				return;
			}
		}

		index++;
	}

	throw 1;
}

void ProfileOutText(char *szFileName)
{
	FILE *fp;
	fopen_s(&fp, szFileName, "wb");


	if (fp != nullptr)
	{
		fprintf(fp, "           Name  |     Average  |        Min   |        Max   |      Call |\r\n");
		fprintf(fp, "-------------------------------------------------------------------------- \r\n");

		for (int i = 0; i < PROFILE_NUMMAX; i++)
		{
			if (Data[i].flag)
				fprintf(fp, "%16s |%11.4fμs |%11.4fμs |%11.4fμs |%10d \r\n",
					Data[i].szName,
					((Data[i].iTotalTime / (double)Data[i].iCall) / g_lFreqency.QuadPart) * g_fMicroFreqeuncy,
					(double)Data[i].iMin[1] / (double)g_lFreqency.QuadPart * g_fMicroFreqeuncy,
					(double)Data[i].iMax[1] / (double)g_lFreqency.QuadPart * g_fMicroFreqeuncy,
					Data[i].iCall);
		}

		fclose(fp);
	}
}

void ProfileReset()
{
	for (int i = 0; i < PROFILE_NUMMAX; i++)
	{
		memset(&Data[i], 0, sizeof(PROFILE_DATA));

		Data[i].flag = false;
		Data[i].iTotalTime = 0;
		Data[i].iMin[0] = 0x7fffffffffffffff;
		Data[i].iMin[1] = 0x7fffffffffffffff;
		Data[i].iCall = 0;
	}

	QueryPerformanceFrequency(&g_lFreqency);
	g_fMicroFreqeuncy = 1000000;
}