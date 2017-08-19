#pragma once
#include <string>
#include <windows.h>

inline std::string w2a(LPCWSTR str, CHAR const DefaultChar = '?')
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
