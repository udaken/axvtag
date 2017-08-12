#include "stdafx.h"

#include <windows.h>
#include <windowsx.h>
#include <comdef.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <cassert>

#include "spi00am.h"
#include "axvtag.h"
#include "commctrl.h"

#include "resource.h"


#define TrackBar_GetPos(hwndCtl)     ((int)(DWORD)SNDMSG((hwndCtl), TBM_GETPOS, (WPARAM)0, (LPARAM)0))
#define TrackBar_SetPos(hwndCtl, repaint,newPos)     ((int)(DWORD)SNDMSG((hwndCtl), TBM_SETPOS, (WPARAM)(BOOL)(repaint), (LPARAM)(newPos)))
#define TrackBar_SetRange(hwndCtl,repaint,minValue,maxValue)     ((int)(DWORD)SNDMSG((hwndCtl), TBM_SETRANGE, (WPARAM)(BOOL)(repaint),  MAKELPARAM((minValue), (maxValue))))

inline std::string w2a(LPCWSTR str, CHAR DefaultChar = '?')
{
	DWORD const dwFlags = WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS;
	int capacity = ::WideCharToMultiByte(CP_THREAD_ACP, dwFlags, str, -1, nullptr, 0, &DefaultChar, nullptr);
	if (capacity == 0)
		throw std::exception();

	std::string ret(static_cast<size_t>(capacity + 1), '\0');
	::WideCharToMultiByte(CP_THREAD_ACP, dwFlags, str, -1, ret.data(), capacity, &DefaultChar, nullptr);

	return std::move(ret);
}

inline std::wstring a2w(LPCSTR str)
{
	DWORD const dwFlags = MB_ERR_INVALID_CHARS;
	int capacity = ::MultiByteToWideChar(CP_THREAD_ACP, dwFlags, str, -1, nullptr, 0);
	if (capacity == 0)
		throw std::exception();

	std::wstring ret(static_cast<size_t>(capacity + 1), L'\0');
	::MultiByteToWideChar(CP_THREAD_ACP, dwFlags, str, -1, ret.data(), capacity);

	return std::move(ret);
}

struct Configuration
{
	std::wstring ExcludePattern = L"";
	UINT LowerBound = 2;
	bool enableParnthesis = true;
	bool enableFullwidthParnthesis = true;
	bool enableSquareBracket = true;
	bool enableFullwidthSquareBracket = true;
	bool enableBlackLenticularBracket = true;
	bool enableWhiteLenticularBracket = true;
	bool enableCornerBracket = true;
	bool enableWhiteCornerBracket = true;
	bool enableDoubleAngleBracket = true;

	inline void Save() const
	{
		auto const iniFile = GetGlobalIniFilePath();

		Configuration const defaultVal;

		if (defaultVal.ExcludePattern != ExcludePattern)
			::WritePrivateProfileString(keyName, L"ExcludePattern", ExcludePattern.c_str(), iniFile.c_str());
		for (auto a : GetBoolMembers())
		{
			if (defaultVal.*a.second != this->*a.second)
				::WritePrivateProfileString(keyName, a.first, this->*a.second ? L"1" : L"0", iniFile.c_str());
		}

		if (defaultVal.LowerBound != LowerBound)
			::WritePrivateProfileString(keyName, L"LowerBound", std::to_wstring(LowerBound).c_str(), iniFile.c_str());

	}

	static std::wstring GetGlobalIniFilePath()
	{
		WCHAR iniFile[MAX_PATH];
		::GetModuleFileName(g_hModule, iniFile, _countof(iniFile));
		::PathRenameExtension(iniFile, L".ini");
		return iniFile;
	}

	inline void Load(LPCWSTR iniFile)
	{
		{
			WCHAR ret[128];
			::GetPrivateProfileString(keyName, L"ExcludePattern", ExcludePattern.c_str(), ret, _countof(ret), iniFile);
			ExcludePattern = ret;
		}

		for (auto a : GetBoolMembers())
		{
			int ret = ::GetPrivateProfileInt(keyName, a.first, this->*a.second ? 1 : 0, iniFile);
			this->*a.second = ret;
		}

		{
			int ret = static_cast<int>(::GetPrivateProfileInt(keyName, L"LowerBound", LowerBound, iniFile));
			if (ret >= 0)
				LowerBound = ret;
		}
	}

	std::map<const wchar_t *, const wchar_t *> GetSupportedTags() const
	{
		std::map<const wchar_t *, const wchar_t *> map;

		if (enableParnthesis)
			map.insert(std::make_pair(L"(", L")"));
		if (enableFullwidthParnthesis)
			map.insert(std::make_pair(L"（", L"）"));
		if (enableSquareBracket)
			map.insert(std::make_pair(L"[", L"]"));
		if (enableFullwidthSquareBracket)
			map.insert(std::make_pair(L"［", L"］"));
		if (enableBlackLenticularBracket)
			map.insert(std::make_pair(L"【", L"】"));
		if (enableWhiteLenticularBracket)
			map.insert(std::make_pair(L"『", L"』"));
		if (enableCornerBracket)
			map.insert(std::make_pair(L"「", L"」"));
		if (enableWhiteCornerBracket)
			map.insert(std::make_pair(L"『", L"』"));
		if (enableDoubleAngleBracket)
			map.insert(std::make_pair(L"《", L"》"));

		return std::move(map);
	}

	inline static std::map<LPCWSTR, bool(Configuration::*)> GetBoolMembers()
	{
		const static std::pair<LPCWSTR, bool Configuration::*> boolMembers[] =
		{
			std::make_pair(L"enableParnthesis", &Configuration::enableParnthesis),
			std::make_pair(L"enableFullwidthParnthesis", &Configuration::enableFullwidthParnthesis),
			std::make_pair(L"enableSquareBracket", &Configuration::enableSquareBracket),
			std::make_pair(L"enableFullwidthSquareBracket", &Configuration::enableFullwidthSquareBracket),
			std::make_pair(L"enableBlackLenticularBracket", &Configuration::enableBlackLenticularBracket),
			std::make_pair(L"enableWhiteLenticularBracket", &Configuration::enableWhiteLenticularBracket),
			std::make_pair(L"enableCornerBracket", &Configuration::enableCornerBracket),
			std::make_pair(L"enableWhiteCornerBracket", &Configuration::enableWhiteCornerBracket),
			std::make_pair(L"enableDoubleAngleBracket", &Configuration::enableDoubleAngleBracket),
		};

		return std::move(std::map<LPCWSTR, bool(Configuration::*)>(boolMembers, boolMembers + _countof(boolMembers)));
	}

	static constexpr LPCWSTR const keyName = L"AXVTAG";
};
/**
 * エントリポイント
 */
BOOL APIENTRY SpiEntryPoint(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
	UNREFERENCED_PARAMETER(hModule);
	UNREFERENCED_PARAMETER(lpReserved);
}

/**
 * ファイル先頭2KB以内から対応フォーマットか判断
 * (判断に使用するサイズはヘッダファイルで定義)
 * ファイル名も判断材料として渡されているみたい
 */
BOOL IsSupportedEx(const char *filename, const BYTE *data)
{
	const static BYTE sig[] = { '[', 'A','X','V','T','A','G', ']' };

	// 先頭バイト列のみの簡易チェックを行う
	if (memcmp(data, sig, sizeof(sig)) == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
	UNREFERENCED_PARAMETER(filename);
}

LPCWSTR findTag(const Configuration &config, size_t index, LPCWSTR filename, std::multimap<const std::wstring, size_t>& tags)
{
	auto const end = filename + lstrlenW(filename) + 1;
	auto firstPos = end;
	decltype(firstPos) endPos = nullptr;
	for (auto tagSep : config.GetSupportedTags())
	{
		auto openPos = wcsstr(filename, tagSep.first);
		if (openPos)
		{
			openPos += lstrlenW(tagSep.first);
			auto closePos = wcsstr(openPos, tagSep.second);
			if (closePos && openPos < firstPos) // 先頭のタグを見つける
			{
				firstPos = openPos;
				endPos = closePos;
			}
		}
	}

	if (firstPos == end)
		return nullptr;

	std::wstring tag(firstPos, endPos);
	tags.insert(std::make_pair(tag, index));
	return endPos;
}

bool enumFiles(Configuration &config, LPCWSTR filename, std::vector<WIN32_FIND_DATA> &files)
{
	WCHAR dir[MAX_PATH];
	wcscpy_s(dir, filename);
	PathRemoveFileSpec(dir);

	::PathAddBackslash(dir);
	::PathAppend(dir, L"*.*");

	WIN32_FIND_DATA findData;
	std::shared_ptr<void> hFind(::FindFirstFile(dir, &findData), ::FindClose);

	if (hFind.get() == INVALID_HANDLE_VALUE)
		return false;

	do
	{
		if (lstrcmp(findData.cFileName, L".") == 0 || lstrcmp(findData.cFileName, L"..") == 0)
		{
			continue;
		}
		else if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}
		else if (lstrcmpi(L".vtag", ::PathFindExtension(findData.cFileName)) == 0)
		{
			continue;
		}
		else if (config.ExcludePattern.length() > 0 && ::PathMatchSpec(findData.cFileName, config.ExcludePattern.c_str()))
		{
			continue;
		}

		files.emplace_back(findData);
	} while (::FindNextFile(hFind.get(), &findData));
	return true;
}

inline susie_time_t FileTimeToUnixTime(const FILETIME& ft) {
	LARGE_INTEGER li;
	li.HighPart = ft.dwHighDateTime;
	li.LowPart = ft.dwLowDateTime;
	return static_cast<susie_time_t>((li.QuadPart - 116444736000000000) / 10000000);
}

int GetArchiveInfoEx(LPCSTR filename, size_t len, HLOCAL *lphInf)
{
	Configuration config;
	config.Load(a2w(filename).c_str());

	std::vector<WIN32_FIND_DATA> files;
	if (!enumFiles(config, a2w(filename).c_str(), files))
		return SPI_FILE_READ_ERROR;

	std::multimap<const std::wstring, size_t> tags;
	for (size_t i = 0; i < files.size(); i++)
	{
		LPCWSTR startPos = files.at(i).cFileName;
		do
		{
			startPos = findTag(config, i, startPos, tags);
		} while (startPos);
	}

	std::vector<fileInfo> ret;
	for (auto pair : tags)
	{
		if (tags.count(pair.first) < config.LowerBound)
		{
			continue;
		}

		fileInfo fi = { {'V','T', 'A', 'G'} };
		fi.position = pair.second;
		auto findData = files[pair.second];
		fi.compsize = fi.filesize = findData.nFileSizeLow;
		fi.timestamp = FileTimeToUnixTime(findData.ftLastWriteTime);
		fi.crc = 0;
		strncpy_s(fi.path, w2a(pair.first.c_str()).c_str(), _countof(fi.path) - 1);
		strcat_s(fi.path, "\\");
		strncpy_s(fi.filename, w2a(findData.cFileName, '_').c_str(), _countof(fi.filename) - 1);
		ret.push_back(fi);
	}

	HLOCAL hLocal = ::LocalAlloc(LPTR, sizeof(fileInfo) * (ret.size() + 1));
	if (hLocal == nullptr)
		return SPI_MEMORY_ERROR;

	memcpy(hLocal, ret.data(), sizeof(fileInfo) * ret.size());

	*lphInf = hLocal;

	return SPI_ALL_RIGHT;
	UNREFERENCED_PARAMETER(len);
}

int GetFileEx(LPCSTR filename, IStream *dest, const fileInfo *pinfo,
	SPI_PROGRESS lpPrgressCallback, LONG_PTR lData)
{
	Configuration config;
	config.Load(a2w(filename).c_str());

	std::vector<WIN32_FIND_DATA> files;
	if (!enumFiles(config, a2w(filename).c_str(), files))
		return SPI_FILE_READ_ERROR;

	auto& findData = files[pinfo->position];

	WCHAR longpath[MAX_PATH];
	::GetLongPathName(a2w(filename).c_str(), longpath, _countof(longpath));
	WCHAR fullpath[MAX_PATH];
	::GetFullPathName(longpath, _countof(fullpath), fullpath, nullptr);
	::PathRemoveFileSpec(fullpath);

	::PathAppend(fullpath, findData.cFileName);

	IStreamPtr stream;
	HRESULT hr = ::SHCreateStreamOnFile(fullpath, STGM_READ | STGM_SHARE_DENY_WRITE, &stream);
	if (FAILED(hr))
		return SPI_FILE_READ_ERROR;

	ULARGE_INTEGER cb;
	cb.LowPart = findData.nFileSizeLow;
	cb.HighPart = findData.nFileSizeHigh;
	hr = stream->CopyTo(dest, cb, nullptr, nullptr);
	if (FAILED(hr))
		return SPI_FILE_WRITE_ERROR;

	return SPI_ALL_RIGHT;

	UNREFERENCED_PARAMETER(lpPrgressCallback);
	UNREFERENCED_PARAMETER(lData);
}


static BOOL CALLBACK DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static Configuration* config = nullptr;

	const static std::pair<INT, bool Configuration::*> boolMembers[] =
	{
		std::make_pair(IDC_CHECK_PARNTHESIS, &Configuration::enableParnthesis),
		std::make_pair(IDC_CHECK_FULLWIDTH_PARNTHESIS, &Configuration::enableFullwidthParnthesis),
		std::make_pair(IDC_CHECK_SUARE_BRACKET, &Configuration::enableSquareBracket),
		std::make_pair(IDC_CHECK_FULLWIDTH_SUARE_BRACKET, &Configuration::enableFullwidthSquareBracket),
		std::make_pair(IDC_CHECK_BLACK_LENTICULAR_BRACKET, &Configuration::enableBlackLenticularBracket),
		std::make_pair(IDC_CHECK_WHITE_LENTICULAR_BRACKET, &Configuration::enableWhiteLenticularBracket),
		std::make_pair(IDC_CHECK_CORNER_BRACKET, &Configuration::enableCornerBracket),
		std::make_pair(IDC_CHECK_WHITE_CORNER_BRACKET, &Configuration::enableWhiteCornerBracket),
		std::make_pair(IDC_CHECK_DOUBLE_ANGLE_BRACKET, &Configuration::enableDoubleAngleBracket),
	};
	assert(Configuration::GetBoolMembers().size() == _countof(boolMembers));

	switch (msg)
	{
	case WM_CLOSE:
	{
		EndDialog(hWnd, IDCANCEL);
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch (wParam)
		{
		case IDOK:
		{
			for (auto a : boolMembers)
			{
				config->*a.second = Button_GetCheck(GetDlgItem(hWnd, a.first));
			}
			{
				WCHAR buf[128];
				GetDlgItemText(hWnd, IDC_EDIT_EXCLUDE_PATTERN, buf, _countof(buf));
				config->ExcludePattern = buf;
			}
			config->LowerBound = TrackBar_GetPos(GetDlgItem(hWnd, IDC_SLIDER1));
		}
		case IDCANCEL:
			EndDialog(hWnd, wParam);
			return TRUE;
		default:
			break;
		}
		break;
	}
	case WM_INITDIALOG:
	{
		config = reinterpret_cast<Configuration*>(lParam);

		for (auto a : boolMembers)
		{
			Button_SetCheck(GetDlgItem(hWnd, a.first), config->*a.second);
		}
		SetDlgItemText(hWnd, IDC_EDIT_EXCLUDE_PATTERN, config->ExcludePattern.c_str());

		TrackBar_SetRange(GetDlgItem(hWnd, IDC_SLIDER1), FALSE, 1, 10);
		TrackBar_SetPos(GetDlgItem(hWnd, IDC_SLIDER1), TRUE, config->LowerBound);

		return TRUE;
	}
	}
	return FALSE;
}

int WINAPI ConfigurationDlg(HWND parent, int fnc) noexcept
{
	if (fnc == 1)
	{
		Configuration config;
		config.Load(Configuration::GetGlobalIniFilePath().c_str());

		int ret = ::DialogBoxParam(g_hModule, MAKEINTRESOURCE(IDD_DIALOG1), parent, &DialogProc, reinterpret_cast<LPARAM>(&config));
		switch (ret)
		{
		case IDOK:
			config.Save();
			break;
		default:
			break;
		}
		return SPI_ALL_RIGHT;
	}
	else
	{
		return SPI_NO_FUNCTION;
	}

}