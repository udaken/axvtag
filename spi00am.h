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
 // ���Ԍ^
 //
 //---------------------------------------------------------------------------//

 // Susie �Ǝ��� time_t �^
#if INTPTR_MAX == INT64_MAX
typedef __time64_t susie_time_t;
#elif INTPTR_MAX == INT32_MAX
typedef __time32_t susie_time_t;
#else
#error Susie.hpp : Unsupported Environment
#endif

/*-------------------------------------------------------------------------*/
// �t�@�C�����\����
/*-------------------------------------------------------------------------*/
#pragma pack(push)
#pragma pack(1) //�\���̂̃����o���E��1�o�C�g�ɂ���
typedef struct fileInfo
{
	uint8_t      method[8];              // ���k�@�̎��
	size_t       position;               // �t�@�C����ł̈ʒu
	size_t       compsize;               // ���k���ꂽ�T�C�Y
	size_t       filesize;               // ���̃t�@�C���T�C�Y
	susie_time_t timestamp;              // �t�@�C���̍X�V����
	char         path[200];     // ���΃p�X
	char         filename[200]; // �t�@�C���l�[��
	uint32_t     crc;                    // CRC32
#if INTPTR_MAX == INT64_MAX
	uint8_t      dummy[4];               // �A���C�������g
#endif
} fileInfo;

typedef struct fileInfoW
{
	uint8_t      method[8];              // ���k�@�̎��
	size_t       position;               // �t�@�C����ł̈ʒu
	size_t       compsize;               // ���k���ꂽ�T�C�Y
	size_t       filesize;               // ���̃t�@�C���T�C�Y
	susie_time_t timestamp;              // �t�@�C���̍X�V����
	wchar_t      path[200];     // ���΃p�X
	wchar_t      filename[200]; // �t�@�C���l�[��
	uint32_t     crc;                    // CRC32
#if INTPTR_MAX == INT64_MAX
	uint8_t      dummy[4];               // �A���C�������g
#endif
} fileInfoW;

#pragma pack(pop)

/*-------------------------------------------------------------------------*/
// �G���[�R�[�h
/*-------------------------------------------------------------------------*/
#define SPI_NO_FUNCTION         -1  /* ���̋@�\�̓C���v�������g����Ă��Ȃ� */
#define SPI_ALL_RIGHT           0   /* ����I�� */
#define SPI_ABORT               1   /* �R�[���o�b�N�֐�����0��Ԃ����̂œW�J�𒆎~���� */
#define SPI_NOT_SUPPORT         2   /* ���m�̃t�H�[�}�b�g */
#define SPI_OUT_OF_ORDER        3   /* �f�[�^�����Ă��� */
#define SPI_NO_MEMORY           4   /* �������[���m�ۏo���Ȃ� */
#define SPI_MEMORY_ERROR        5   /* �������[�G���[ */
#define SPI_FILE_READ_ERROR     6   /* �t�@�C�����[�h�G���[ */
#define SPI_WINDOW_ERROR        7   /* �����J���Ȃ� (����J�̃G���[�R�[�h) */
#define SPI_OTHER_ERROR         8   /* �����G���[ */
#define SPI_FILE_WRITE_ERROR    9   /* �������݃G���[ (����J�̃G���[�R�[�h) */
#define SPI_END_OF_FILE         10  /* �t�@�C���I�[ (����J�̃G���[�R�[�h) */

/*-------------------------------------------------------------------------*/
// '00AM'�֐��̃v���g�^�C�v�錾
/*-------------------------------------------------------------------------*/
//int PASCAL ProgressCallback(int nNum, int nDenom, long lData);
typedef int (CALLBACK *SPI_PROGRESS)(int, int, LONG_PTR);
extern "C"
{
	int WINAPI GetPluginInfo(int infono, LPSTR buf, int buflen) noexcept;
	int WINAPI IsSupported(LPSTR filename, void* dw) noexcept;
	int WINAPI GetArchiveInfo(LPSTR buf, size_t len, unsigned int flag, HLOCAL *lphInf) noexcept;
	int WINAPI GetFileInfo(LPSTR buf, size_t len, LPSTR filename, unsigned int flag, fileInfo *lpInfo) noexcept;
	int WINAPI GetFile(LPSTR src, size_t len, LPSTR dest, unsigned int flag, SPI_PROGRESS prgressCallback, LONG_PTR lData) noexcept;

	int WINAPI ConfigurationDlg(HWND parent, int fnc) noexcept;
}

#endif

extern HMODULE g_hModule;
