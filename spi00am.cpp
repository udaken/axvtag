/**
 * SUSIE32 '00AM' Plug-in Sample by shimitei (modified by gocha)
 * <http://www.asahi-net.or.jp/~kh4s-smz/spi/make_spi.html>
 */
#include "stdafx.h"

#include <windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <comip.h>
#include <comdef.h>

#include <exception>
#include <clocale>

#include "spi00am.h"
#include "axvtag.h"
#include "infcache.h"
#include "utility.h"

 // ヘッダチェック等に必要なサイズ.2KB以内で
#define HEADBUF_SIZE  9
HMODULE g_hModule;
/*-------------------------------------------------------------------------*/
// このPluginの情報
/*-------------------------------------------------------------------------*/
static const char* const pluginfo[] = {
	/* Plug-in APIバージョン */
	"00AM",
	/* Plug-in名、バージョン及び copyright */
	"Virtual Tag Plugin(build at " __DATE__ " " __TIME__  ", with MSC " _STRINGIZE(_MSC_FULL_VER) ")",
	/* 代表的な拡張子 ("*.JPG" "*.RGB;*.Q0" など) */
	"*.vtag",
	/* ファイル形式名 */
	"Virtual Tagging Marker File",
};

//グローバル変数
InfoCache infocache; //アーカイブ情報キャッシュクラス
static char g_fallbackChar = '_';
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
inline bool fileInfoW2fileInfo(const fileInfoW* from, fileInfo* to)
{
	memcpy(to->method, from->method, sizeof(from->method));
	to->position = from->position;
	to->compsize = from->compsize;
	to->filesize = from->filesize;
	to->timestamp = from->timestamp;
	strcpy_s(to->path, w2a(from->path, g_fallbackChar).c_str());

	LPCWSTR ext = ::PathFindExtension(from->filename);
	size_t extLength = ext ? lstrlen(ext) : 0;
	WideCharToMultiByte(CP_ACP,
		WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS,
		from->filename,
		lstrlen(from->filename) - extLength,
		to->filename,
		_countof(to->filename) - (extLength + 1),
		&g_fallbackChar,
		nullptr);

	if (ext)
		strcat_s(to->filename, ::PathFindExtensionA(w2a(from->filename, g_fallbackChar).c_str()));
	to->crc = from->crc;
	return true;
}
/***************************************************************************
 * SPI関数
 ***************************************************************************/
 //---------------------------------------------------------------------------
extern "C" int __stdcall GetPluginInfo(int infono, LPSTR buf, int buflen) noexcept
{
	if (infono < 0 || infono >= (sizeof(pluginfo) / sizeof(char*)))
		return 0;
	strncpy_s(buf, buflen, pluginfo[infono], buflen);

	return (int)strlen(buf);
}
//---------------------------------------------------------------------------
extern "C" int __stdcall GetPluginInfoW(int infono, LPWSTR buf, int buflen) noexcept
{
	CHAR a[256] = {};
	GetPluginInfo(infono, a, _countof(a));
	wcscpy_s(buf, buflen, a2w(a).c_str());
	return lstrlen(buf);
}
//---------------------------------------------------------------------------
extern "C" int __stdcall IsSupportedW(LPCWSTR filename, void* dw) noexcept
{
	setlocale(LC_CTYPE, "");
	try
	{
		BYTE* data;
		BYTE buff[HEADBUF_SIZE];

		if ((reinterpret_cast<ULONG_PTR>(dw) & ~static_cast<ULONG_PTR>(0xFFFF)) == 0) {
			/* dwはファイルハンドル */
			DWORD ReadBytes;
			if (!ReadFile((HANDLE)dw, buff, HEADBUF_SIZE, &ReadBytes, NULL)) {
				return 0;
			}
			data = buff;
		}
		else {
			/* dwはバッファへのポインタ */
			data = (BYTE*)dw;
		}

		/* フォーマット確認 */
		if (IsSupportedEx(filename, data))
			return 1;

		return 0;
	}
	catch (const std::exception&)
	{
		return 0;
	}
}

//---------------------------------------------------------------------------
extern "C" int __stdcall IsSupported(LPSTR filename, void* dw) noexcept
{
	return IsSupportedW(a2w(filename).c_str(), dw);
}
//---------------------------------------------------------------------------
//アーカイブ情報をキャッシュする
int GetArchiveInfoCache(const wchar_t* filename, size_t len, HLOCAL* phinfo, fileInfoW* pinfo)
{
	int ret = infocache.Dupli(filename, phinfo, pinfo);
	if (ret != SPI_NO_FUNCTION) return ret;

	//キャッシュに無い
	HLOCAL hinfo;
	HANDLE hf;
	BYTE headbuf[HEADBUF_SIZE];
	DWORD ReadBytes;

	hf = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
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
		auto size = LocalSize(hinfo);
		/* 出力用のメモリの割り当て */
		*phinfo = LocalAlloc(LMEM_FIXED, size);
		if (*phinfo == NULL) {
			return SPI_NO_MEMORY;
		}

		memcpy(*phinfo, (void*)hinfo, size);
	}
	else {
		fileInfoW* ptmp = (fileInfoW*)hinfo;
		if (pinfo->filename[0] != '\0') {
			for (;;) {
				if (ptmp->method[0] == '\0') return SPI_NO_FUNCTION;
				if (_wcsicmp(ptmp->filename, pinfo->filename) == 0) break;
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
extern "C" int __stdcall GetArchiveInfoW(LPCWSTR buf, size_t len, unsigned int flag, HLOCAL* lphInf) noexcept
{
	//メモリ入力には対応しない
	if ((flag & 7) != 0) return SPI_NO_FUNCTION;

	try
	{
		*lphInf = NULL;
		return GetArchiveInfoCache(buf, len, lphInf, NULL);
	}
	catch (const std::exception&)
	{
		return SPI_OTHER_ERROR;
	}
}
int __stdcall GetArchiveInfo(LPSTR buf, size_t len, unsigned int flag, HLOCAL* lphInf) noexcept
{
	HLOCAL tmpHlocal = nullptr;
	auto ret = GetArchiveInfoW(
		(flag & 7) == 0 ? a2w(buf).c_str() : reinterpret_cast<LPWSTR>(buf),
		len, flag, &tmpHlocal);
	if (ret != SPI_ALL_RIGHT)
		return ret;

	HLOCAL newInfo = LocalAlloc(LPTR, (LocalSize(tmpHlocal) / sizeof(struct fileInfoW)) * sizeof(fileInfo));
	auto a = reinterpret_cast<struct fileInfoW*>(LocalLock(tmpHlocal));
	auto b = reinterpret_cast<struct fileInfo*>(newInfo);
	if (!a)
	{
		return SPI_MEMORY_ERROR;
	}
	for (int i = 0;; i++)
	{
		auto from = a + i;
		auto to = b + i;
		if (from->method[0] == '\0')
			break;
		fileInfoW2fileInfo(from, to);
	}
	LocalFree(tmpHlocal);
	*lphInf = newInfo;

	return ret;
}

//---------------------------------------------------------------------------
extern "C" int __stdcall GetFileInfoW
(LPCWSTR buf, size_t len, LPCWSTR filename, unsigned int flag, struct fileInfoW* lpInfo) noexcept
{
	//  bool caseSensitive = !(flag & 0x80);

	//メモリ入力には対応しない
	if ((flag & 7) != 0) return SPI_NO_FUNCTION;

	try
	{
		wcscpy_s(lpInfo->filename, filename);

		return GetArchiveInfoCache(buf, len, NULL, lpInfo);
	}
	catch (const std::exception&)
	{
		return SPI_OTHER_ERROR;
	}
}
//---------------------------------------------------------------------------
extern "C" int __stdcall GetFileInfo
(LPSTR buf, size_t len, LPSTR filename, unsigned int flag, struct fileInfo* lpInfo) noexcept
{
	fileInfoW fi;
	auto ret = GetFileInfoW((flag & 7) == 0 ? a2w(buf).c_str() : reinterpret_cast<LPWSTR>(buf), len, a2w(filename).c_str(), flag, &fi);
	if (ret != SPI_ALL_RIGHT)
		return ret;
	fileInfoW2fileInfo(&fi, lpInfo);

	return SPI_ALL_RIGHT;
}
//---------------------------------------------------------------------------
int __stdcall GetFileW(LPCWSTR src, size_t len,
	LPCWSTR dest, unsigned int flag,
	SPI_PROGRESS lpPrgressCallback, LONG_PTR lData) noexcept
{
	//メモリ入力には対応しない
	if ((flag & 7) != 0) return SPI_NO_FUNCTION;

	try
	{
		fileInfoW info = {};
		info.position = len;
		int ret = GetArchiveInfoCache(src, 0, NULL, &info);
		if (ret != SPI_ALL_RIGHT)
			return ret;

		IStreamPtr stream = nullptr;
		if ((flag & 0x700) == 0)
		{
			WCHAR path[MAX_PATH];
			wsprintfW(path, L"%s\\%s", dest, info.filename);
			HRESULT hr = SHCreateStreamOnFile(path, STGM_CREATE | STGM_WRITE, &stream);
			if (FAILED(hr))
				return SPI_FILE_WRITE_ERROR;
		}
		else
		{
			HRESULT hr = CreateStreamOnHGlobal(::GlobalAlloc(GPTR, info.filesize), FALSE, &stream);
			if (FAILED(hr))
				return SPI_FILE_WRITE_ERROR;
		}

		ret = GetFileEx(src, stream, &info, lpPrgressCallback, lData);

		if ((flag & 0x700) != 0)
		{
			auto p = (HLOCAL*)dest;
			HRESULT hr = GetHGlobalFromStream(stream, p);
			if (FAILED(hr))
				return SPI_FILE_WRITE_ERROR;
			GlobalUnlock(*p);
		}

		return ret;
	}
	catch (const std::exception&)
	{
		return SPI_OTHER_ERROR;
	}
}
//---------------------------------------------------------------------------
int __stdcall GetFile(LPSTR src, size_t len, LPSTR dest, unsigned int flag, SPI_PROGRESS prgressCallback, LONG_PTR lData) noexcept
{
	auto ret = GetFileW(
		(flag & 7) == 0 ? a2w(src).c_str() : reinterpret_cast<LPWSTR>(src),
		len,
		(flag & 0x700) == 0 ? a2w(dest).c_str() : reinterpret_cast<LPWSTR>(dest),
		flag,
		prgressCallback,
		lData);

	return ret;
}


//---------------------------------------------------------------------------
