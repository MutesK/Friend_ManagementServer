#pragma once

#include "Server.h"
#include "Rapidjson/document.h"
#include "Rapidjson/writer.h"
#include "Rapidjson/stringbuffer.h"
using namespace rapidjson;


bool UTF8toUTF16(const char *szText, WCHAR *szBuff, int iBuffLen);
bool UTF16toUTF8(const char *szText, WCHAR *szBuff, int iBuffLen);
bool LoadData(void);
bool SaveData(void);