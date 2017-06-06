#pragma once

#include <Windows.h>

#define PROFILE_CHECK
#define PROFILE_NUMMAX 30

struct PROFILE_DATA
{
	long flag;

	char szName[64];

	LARGE_INTEGER startTime;
	__int64 iTotalTime;
	__int64 iMin[2];
	__int64 iMax[2];

	__int64 iCall;
};

class CProfile
{
public:


	CProfile();
};

void ProfileBegin(char *_fName);
void ProfileEnd(char *_fName);
void ProfileOutText(char *szFileName);
void ProfileReset();