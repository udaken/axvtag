#pragma once

#include <windows.h>
#include <objbase.h> // IStream
#include "spi00am.h"

BOOL APIENTRY SpiEntryPoint(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);
BOOL IsSupportedEx(const char *filename,const BYTE *data);
int GetArchiveInfoEx(LPCSTR filename, size_t len, HLOCAL *lphInf);
int GetFileEx(LPCSTR filename, IStream *dest, const fileInfo *pinfo, SPI_PROGRESS lpPrgressCallback, LONG_PTR lData);

