/**
 * SUSIE32 '00AM' Plug-in Sample by shimitei (modified by gocha)
 * <http://www.asahi-net.or.jp/~kh4s-smz/spi/make_spi.html>
 */
#include "stdafx.h"

#include <windows.h>
#include <string.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <comip.h>
#include <comdef.h>

#include "spi00am.h"
#include "axvtag.h"
#include "infcache.h"
#include <exception>

 // ヘッダチェック等に必要なサイズ.2KB以内で
#define HEADBUF_SIZE  9
HMODULE g_hModule;
 /*-------------------------------------------------------------------------*/
 // このPluginの情報
 /*-------------------------------------------------------------------------*/
static const char *pluginfo[] = {
	/* Plug-in APIバージョン */
	"00AM",                           
	/* Plug-in名、バージョン及び copyright */
	"Virtual Tag Plugin",             
	/* 代表的な拡張子 ("*.JPG" "*.RGB;*.Q0" など) */
	"*.vtag",                         
	/* ファイル形式名 */
	"Virtual Tagging Marker File",    
};

//グローバル変数
InfoCache infocache; //アーカイブ情報キャッシュクラス

//---------------------------------------------------------------------------
/* エントリポイント */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		g_hModule = hModule;
		DisableThreadLibraryCalls(hModule);
		break;
	case DLL_PROCESS_DETACH:
		infocache.Clear();
		break;
	}
	return SpiEntryPoint(hModule, ul_reason_for_call, lpReserved);
}

/***************************************************************************
 * SPI関数
 ***************************************************************************/
 //---------------------------------------------------------------------------
int __stdcall GetPluginInfo(int infono, LPSTR buf, int buflen) noexcept
{
	if (infono < 0 || infono >= (sizeof(pluginfo) / sizeof(char *)))
		return 0;

	strncpy_s(buf, buflen, pluginfo[infono], buflen);

	return (int)strlen(buf);
}
//---------------------------------------------------------------------------
int __stdcall IsSupported(LPSTR filename, void* dw) noexcept
{
	BYTE *data;
	BYTE buff[HEADBUF_SIZE];

	if ((reinterpret_cast<ULONG_PTR>(dw) & 0xFFFF0000) == 0) {
		/* dwはファイルハンドル */
		DWORD ReadBytes;
		if (!ReadFile((HANDLE)dw, buff, HEADBUF_SIZE, &ReadBytes, NULL)) {
			return 0;
		}
		data = buff;
	}
	else {
		/* dwはバッファへのポインタ */
		data = (BYTE *)dw;
	}

	/* フォーマット確認 */
	if (IsSupportedEx(filename, data)) return 1;

	return 0;
}
//---------------------------------------------------------------------------
//アーカイブ情報をキャッシュする
int GetArchiveInfoCache(const char *filename, size_t len, HLOCAL *phinfo, fileInfo *pinfo)
{
	int ret = infocache.Dupli(filename, phinfo, pinfo);
	if (ret != SPI_NO_FUNCTION) return ret;

	//キャッシュに無い
	HLOCAL hinfo;
	HANDLE hf;
	BYTE headbuf[HEADBUF_SIZE];
	DWORD ReadBytes;

	hf = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hf == INVALID_HANDLE_VALUE) return SPI_FILE_READ_ERROR;
	
	LARGE_INTEGER distanceToMove;
	distanceToMove.QuadPart = len;
	if (!SetFilePointerEx(hf, distanceToMove, NULL, FILE_BEGIN)) {
		CloseHandle(hf);
		return SPI_FILE_READ_ERROR;
	}
	if (!ReadFile(hf, headbuf, HEADBUF_SIZE, &ReadBytes, NULL)) {
		CloseHandle(hf);
		return SPI_FILE_READ_ERROR;
	}
	CloseHandle(hf);
	if (ReadBytes != HEADBUF_SIZE) return SPI_NOT_SUPPORT;
	if (!IsSupportedEx(filename, headbuf)) return SPI_NOT_SUPPORT;

	ret = GetArchiveInfoEx(filename, len, &hinfo);
	if (ret != SPI_ALL_RIGHT) return ret;

	//キャッシュ
	infocache.Add(filename, &hinfo);

	if (phinfo != NULL) {
		UINT size = (UINT)LocalSize(hinfo);
		/* 出力用のメモリの割り当て */
		*phinfo = LocalAlloc(LMEM_FIXED, size);
		if (*phinfo == NULL) {
			return SPI_NO_MEMORY;
		}

		memcpy(*phinfo, (void*)hinfo, size);
	}
	else {
		fileInfo *ptmp = (fileInfo *)hinfo;
		if (pinfo->filename[0] != '\0') {
			for (;;) {
				if (ptmp->method[0] == '\0') return SPI_NO_FUNCTION;
				if (_stricmp(ptmp->filename, pinfo->filename) == 0) break;
				ptmp++;
			}
		}
		else {
			for (;;) {
				if (ptmp->method[0] == '\0') return SPI_NO_FUNCTION;
				if (ptmp->position == pinfo->position) break;
				ptmp++;
			}
		}
		*pinfo = *ptmp;
	}
	return SPI_ALL_RIGHT;
}
//---------------------------------------------------------------------------
int __stdcall GetArchiveInfo(LPSTR buf, size_t len, unsigned int flag, HLOCAL *lphInf) noexcept
{
	//メモリ入力には対応しない
	if ((flag & 7) != 0) return SPI_NO_FUNCTION;

	*lphInf = NULL;
	return GetArchiveInfoCache(buf, len, lphInf, NULL);
}
//---------------------------------------------------------------------------
int __stdcall GetFileInfo
(LPSTR buf, size_t len, LPSTR filename, unsigned int flag, struct fileInfo *lpInfo) noexcept
{
	//  bool caseSensitive = !(flag & 0x80);

	  //メモリ入力には対応しない
	if ((flag & 7) != 0) return SPI_NO_FUNCTION;

	strcpy_s(lpInfo->filename, filename);

	return GetArchiveInfoCache(buf, len, NULL, lpInfo);
}
//---------------------------------------------------------------------------
int __stdcall GetFile(LPSTR src, size_t len,
	LPSTR dest, unsigned int flag,
	SPI_PROGRESS lpPrgressCallback, LONG_PTR lData) noexcept
{
	//メモリ入力には対応しない
	if ((flag & 7) != 0) return SPI_NO_FUNCTION;
	try
	{
		fileInfo info = {};
		info.position = len;
		int ret = GetArchiveInfoCache(src, 0, NULL, &info);
		if (ret != SPI_ALL_RIGHT)
			return ret;

		IStreamPtr stream = nullptr;
		if ((flag & 0x700) == 0)
		{
			CHAR path[MAX_PATH];
			wsprintfA(path, "%s\\%s", dest, info.filename);
			HRESULT hr = SHCreateStreamOnFileA(path, STGM_CREATE | STGM_WRITE, &stream);
			if (FAILED(hr)) 
				return SPI_FILE_WRITE_ERROR;
		}
		else
		{
			HRESULT hr = CreateStreamOnHGlobal(nullptr, FALSE, &stream);
			if (FAILED(hr))
				return SPI_FILE_WRITE_ERROR;
		}

		ret = GetFileEx(src, stream.GetInterfacePtr(), &info, lpPrgressCallback, lData);

		if ((flag & 0x700) != 0)
		{
			HRESULT hr = GetHGlobalFromStream(stream.GetInterfacePtr(), (HLOCAL*)dest);
			if (FAILED(hr))
				return SPI_FILE_WRITE_ERROR;
		}

		return ret;
	}
	catch (const std::exception&)
	{
		return SPI_OTHER_ERROR;
	}
}
//---------------------------------------------------------------------------
