#include "stdafx.h"

#include <windows.h>
#include <windowsx.h>
#include <comdef.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <commctrl.h>

#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <cassert>

#include "spi00am.h"
#include "axvtag.h"
#include "utility.h"

#include "resource.h"

#define TrackBar_GetPos(hwndCtl)     ((int)(DWORD)SNDMSG((hwndCtl), TBM_GETPOS, (WPARAM)0, (LPARAM)0))
#define TrackBar_SetPos(hwndCtl, repaint,newPos)     ((int)(DWORD)SNDMSG((hwndCtl), TBM_SETPOS, (WPARAM)(BOOL)(repaint), (LPARAM)(newPos)))
#define TrackBar_SetRange(hwndCtl,repaint,minValue,maxValue)     ((int)(DWORD)SNDMSG((hwndCtl), TBM_SETRANGE, (WPARAM)(BOOL)(repaint),  MAKELPARAM((minValue), (maxValue))))

struct Configuration
{
	std::wstring ExcludePattern = L"";
	UINT LowerBound = 2;

	bool EnableSpace = false;
	bool EnableParnthesis = true;
	bool EnableFullwidthParnthesis = true;
	bool EnableSquareBracket = true;
	bool EnableFullwidthSquareBracket = true;
	bool EnableBlackLenticularBracket = true;
	bool EnableWhiteLenticularBracket = true;
	bool EnableCornerBracket = true;
	bool EnableWhiteCornerBracket = true;
	bool EnableDoubleAngleBracket = true;

	inline void Save() const
	{
		auto const iniFile = GetGlobalIniFilePath();

		::WritePrivateProfileString(keyName, L"ExcludePattern", ExcludePattern.c_str(), iniFile.c_str());
		for (auto a : GetBoolMembers())
		{
			::WritePrivateProfileString(keyName, a.first, this->*a.second ? L"1" : L"0", iniFile.c_str());
		}

		::WritePrivateProfileString(keyName, L"LowerBound", std::to_wstring(LowerBound).c_str(), iniFile.c_str());
	}

	static std::wstring GetGlobalIniFilePath()
	{
		WCHAR iniFile[MAX_PATH];
		::GetModuleFileName(g_hModule, iniFile, _countof(iniFile));
		::PathRenameExtension(iniFile, L".ini");
		return iniFile;
	}

	inline void LoadDefault()
	{
		Load(GetGlobalIniFilePath().c_str());
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

		if (EnableSpace)
			map.insert(std::make_pair(L" ", L" "));
		if (EnableParnthesis)
			map.insert(std::make_pair(L"(", L")"));
		if (EnableFullwidthParnthesis)
			map.insert(std::make_pair(L"（", L"）"));
		if (EnableSquareBracket)
			map.insert(std::make_pair(L"[", L"]"));
		if (EnableFullwidthSquareBracket)
			map.insert(std::make_pair(L"［", L"］"));
		if (EnableBlackLenticularBracket)
			map.insert(std::make_pair(L"【", L"】"));
		if (EnableWhiteLenticularBracket)
			map.insert(std::make_pair(L"〖", L"〗"));
		if (EnableCornerBracket)
			map.insert(std::make_pair(L"「", L"」"));
		if (EnableWhiteCornerBracket)
			map.insert(std::make_pair(L"『", L"』"));
		if (EnableDoubleAngleBracket)
			map.insert(std::make_pair(L"《", L"》"));

		return std::move(map);
	}

	inline static std::map<LPCWSTR, bool(Configuration::*)> GetBoolMembers()
	{
		const static std::pair<LPCWSTR, bool Configuration::*> boolMembers[] =
		{
			std::make_pair(L"EnableSpace", &Configuration::EnableSpace),
			std::make_pair(L"EnableParnthesis", &Configuration::EnableParnthesis),
			std::make_pair(L"EnableFullwidthParnthesis", &Configuration::EnableFullwidthParnthesis),
			std::make_pair(L"EnableSquareBracket", &Configuration::EnableSquareBracket),
			std::make_pair(L"EnableFullwidthSquareBracket", &Configuration::EnableFullwidthSquareBracket),
			std::make_pair(L"EnableBlackLenticularBracket", &Configuration::EnableBlackLenticularBracket),
			std::make_pair(L"EnableWhiteLenticularBracket", &Configuration::EnableWhiteLenticularBracket),
			std::make_pair(L"EnableCornerBracket", &Configuration::EnableCornerBracket),
			std::make_pair(L"EnableWhiteCornerBracket", &Configuration::EnableWhiteCornerBracket),
			std::make_pair(L"EnableDoubleAngleBracket", &Configuration::EnableDoubleAngleBracket),
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
BOOL IsSupportedEx(const wchar_t *filename, const BYTE *data)
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
			auto closePos = wcsstr(openPos + lstrlenW(tagSep.first), tagSep.second);
			if (closePos && openPos < firstPos) // 先頭のタグを見つける
			{
				firstPos = openPos;
				endPos = closePos + wcslen(tagSep.second);
			}
		}
	}

	if (firstPos == end)
		return nullptr;

	std::wstring tag(firstPos, endPos);
	tags.insert(std::make_pair(tag, index));
	return endPos;
}

bool enumFiles(const Configuration &config, LPCWSTR filename, std::vector<WIN32_FIND_DATA> &files)
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

inline susie_time_t FileTimeToUnixTime(const FILETIME& ft)
{
	LARGE_INTEGER li;
	li.HighPart = ft.dwHighDateTime;
	li.LowPart = ft.dwLowDateTime;
	return static_cast<susie_time_t>((li.QuadPart - 116444736000000000) / 10000000);
}

int GetArchiveInfoEx(LPCWSTR filename, size_t len, HLOCAL *lphInf)
{
	Configuration config;
	config.LoadDefault();
	config.Load(filename);

	std::vector<WIN32_FIND_DATA> files;
	if (!enumFiles(config, filename, files))
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

	std::vector<fileInfoW> ret;
	for (auto pair : tags)
	{
		if (tags.count(pair.first) < config.LowerBound)
		{
			continue;
		}

		fileInfoW fi = { {'V','T', 'A', 'G'} };
		fi.position = pair.second;
		const auto findData = files.at(pair.second);
		ULARGE_INTEGER li = { findData.nFileSizeLow , findData.nFileSizeHigh };
		fi.compsize = fi.filesize = static_cast<size_t>(li.QuadPart);
		fi.timestamp = FileTimeToUnixTime(findData.ftLastWriteTime);
		fi.crc = 0;
		wcsncpy_s(fi.path, pair.first.c_str(), _countof(fi.path) - 1);
		wcscat_s(fi.path, L"\\");
		wcsncpy_s(fi.filename, findData.cFileName, _countof(fi.filename) - 1);
		ret.push_back(fi);
	}

	HLOCAL hLocal = ::LocalAlloc(LPTR, sizeof(fileInfoW) * (ret.size() + 1));
	if (hLocal == nullptr)
		return SPI_MEMORY_ERROR;

	memcpy(hLocal, ret.data(), sizeof(fileInfoW) * ret.size());

	*lphInf = hLocal;

	return SPI_ALL_RIGHT;
	UNREFERENCED_PARAMETER(len);
}

int GetFileEx(LPCWSTR filename, IStream *dest, const fileInfoW *pinfo, SPI_PROGRESS lpPrgressCallback, LONG_PTR lData)
{
	Configuration config;
	config.LoadDefault();
	config.Load(filename);

	std::vector<WIN32_FIND_DATA> files;
	if (!enumFiles(config, filename, files))
		return SPI_FILE_READ_ERROR;

	auto found = std::find_if(files.begin(), files.end(),
		[&pinfo](const WIN32_FIND_DATA& elem) { return _wcsicmp(pinfo->filename, elem.cFileName) == 0; });
	if (found == files.end())
		return SPI_FILE_READ_ERROR;

	auto& findData = *found;

	WCHAR longpath[MAX_PATH];
	::GetLongPathName(filename, longpath, _countof(longpath));
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


static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static Configuration* config = nullptr;

	const static std::pair<INT, bool Configuration::*> boolMembers[] =
	{
		std::make_pair(IDC_CHECK_SPACE, &Configuration::EnableSpace),
		std::make_pair(IDC_CHECK_PARNTHESIS, &Configuration::EnableParnthesis),
		std::make_pair(IDC_CHECK_FULLWIDTH_PARNTHESIS, &Configuration::EnableFullwidthParnthesis),
		std::make_pair(IDC_CHECK_SUARE_BRACKET, &Configuration::EnableSquareBracket),
		std::make_pair(IDC_CHECK_FULLWIDTH_SUARE_BRACKET, &Configuration::EnableFullwidthSquareBracket),
		std::make_pair(IDC_CHECK_BLACK_LENTICULAR_BRACKET, &Configuration::EnableBlackLenticularBracket),
		std::make_pair(IDC_CHECK_WHITE_LENTICULAR_BRACKET, &Configuration::EnableWhiteLenticularBracket),
		std::make_pair(IDC_CHECK_CORNER_BRACKET, &Configuration::EnableCornerBracket),
		std::make_pair(IDC_CHECK_WHITE_CORNER_BRACKET, &Configuration::EnableWhiteCornerBracket),
		std::make_pair(IDC_CHECK_DOUBLE_ANGLE_BRACKET, &Configuration::EnableDoubleAngleBracket),
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
		config.LoadDefault();

		INT_PTR ret = ::DialogBoxParam(g_hModule, MAKEINTRESOURCE(IDD_DIALOG1), parent, &DialogProc, reinterpret_cast<LPARAM>(&config));
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