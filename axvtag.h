#pragma once

#include <windows.h>
#include <objbase.h> // IStream
#include "spi00am.h"

BOOL APIENTRY SpiEntryPoint(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);
BOOL IsSupportedEx(const wchar_t *filename,const BYTE *data);
int GetArchiveInfoEx(LPCWSTR filename, size_t len, HLOCAL *lphInf);
int GetFileEx(LPCWSTR filename, IStream *dest, const fileInfoW *pinfo, SPI_PROGRESS lpPrgressCallback, LONG_PTR lData);

