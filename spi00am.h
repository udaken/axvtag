/**
 * SUSIE32 '00AM' Plug-in Sample by shimitei (modified by gocha)
 * <http://www.asahi-net.or.jp/~kh4s-smz/spi/make_spi.html>
 */

#ifndef spi00am_h
#define spi00am_h

#include <windows.h>
#include <stdint.h>

 //---------------------------------------------------------------------------//
 //
 // 時間型
 //
 //---------------------------------------------------------------------------//

 // Susie 独自の time_t 型
#if INTPTR_MAX == INT64_MAX
typedef __time64_t susie_time_t;
#elif INTPTR_MAX == INT32_MAX
typedef __time32_t susie_time_t;
#else
#error Susie.hpp : Unsupported Environment
#endif

/*-------------------------------------------------------------------------*/
// ファイル情報構造体
/*-------------------------------------------------------------------------*/
#pragma pack(push)
#pragma pack(1) //構造体のメンバ境界を1バイトにする
typedef struct fileInfo
{
	uint8_t      method[8];              // 圧縮法の種類
	size_t       position;               // ファイル上での位置
	size_t       compsize;               // 圧縮されたサイズ
	size_t       filesize;               // 元のファイルサイズ
	susie_time_t timestamp;              // ファイルの更新日時
	char         path[200];     // 相対パス
	char         filename[200]; // ファイルネーム
	uint32_t     crc;                    // CRC32
#if INTPTR_MAX == INT64_MAX
	uint8_t      dummy[4];               // アラインメント
#endif
} fileInfo;

typedef struct fileInfoW
{
	uint8_t      method[8];              // 圧縮法の種類
	size_t       position;               // ファイル上での位置
	size_t       compsize;               // 圧縮されたサイズ
	size_t       filesize;               // 元のファイルサイズ
	susie_time_t timestamp;              // ファイルの更新日時
	wchar_t      path[200];     // 相対パス
	wchar_t      filename[200]; // ファイルネーム
	uint32_t     crc;                    // CRC32
#if INTPTR_MAX == INT64_MAX
	uint8_t      dummy[4];               // アラインメント
#endif
} fileInfoW;

#pragma pack(pop)

/*-------------------------------------------------------------------------*/
// エラーコード
/*-------------------------------------------------------------------------*/
#define SPI_NO_FUNCTION         -1  /* その機能はインプリメントされていない */
#define SPI_ALL_RIGHT           0   /* 正常終了 */
#define SPI_ABORT               1   /* コールバック関数が非0を返したので展開を中止した */
#define SPI_NOT_SUPPORT         2   /* 未知のフォーマット */
#define SPI_OUT_OF_ORDER        3   /* データが壊れている */
#define SPI_NO_MEMORY           4   /* メモリーが確保出来ない */
#define SPI_MEMORY_ERROR        5   /* メモリーエラー */
#define SPI_FILE_READ_ERROR     6   /* ファイルリードエラー */
#define SPI_WINDOW_ERROR        7   /* 窓が開けない (非公開のエラーコード) */
#define SPI_OTHER_ERROR         8   /* 内部エラー */
#define SPI_FILE_WRITE_ERROR    9   /* 書き込みエラー (非公開のエラーコード) */
#define SPI_END_OF_FILE         10  /* ファイル終端 (非公開のエラーコード) */

/*-------------------------------------------------------------------------*/
// '00AM'関数のプロトタイプ宣言
/*-------------------------------------------------------------------------*/
//int PASCAL ProgressCallback(int nNum, int nDenom, long lData);
typedef int (CALLBACK *SPI_PROGRESS)(int, int, LONG_PTR);
extern "C"
{
	int __stdcall GetPluginInfo(int infono, LPSTR buf, int buflen) noexcept;
	int __stdcall GetPluginInfoW(int infono, LPWSTR buf, int buflen) noexcept;
	int __stdcall IsSupported(LPSTR filename, void* dw) noexcept;
	int __stdcall IsSupportedW(LPCWSTR filename, void *dw) noexcept;
	int __stdcall GetArchiveInfo(LPSTR buf, size_t len, unsigned int flag, HLOCAL *lphInf) noexcept;
	int __stdcall GetArchiveInfoW(LPCWSTR buf, size_t len, unsigned int flag, HLOCAL *lphInf) noexcept;
	int __stdcall GetFileInfo(LPSTR buf, size_t len, LPSTR filename, unsigned int flag, fileInfo *lpInfo) noexcept;
	int __stdcall GetFileInfoW(LPCWSTR buf, size_t len, LPCWSTR filename, unsigned int flag, fileInfoW *lpInfo) noexcept;
	int __stdcall GetFile(LPSTR src, size_t len, LPSTR dest, unsigned int flag, SPI_PROGRESS prgressCallback, LONG_PTR lData) noexcept;
	int __stdcall GetFileW(LPCWSTR src, size_t len, LPCWSTR dest, unsigned int flag, SPI_PROGRESS prgressCallback, LONG_PTR lData) noexcept;

	int __stdcall ConfigurationDlg(HWND parent, int fnc) noexcept;
}

#endif

extern HMODULE g_hModule;
