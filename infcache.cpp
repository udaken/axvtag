﻿/**
 * Cache system for Susie Plug-in by shimitei (modified by gocha)
 * <http://www.asahi-net.or.jp/~kh4s-smz/spi/make_spi.html>
 */
#include "stdafx.h"

#include <string.h>
#include "infcache.h"

InfoCache::InfoCache()
{
  nowno = 0;
  for (int i=0; i<INFOCACHE_MAX; i++) {
    arcinfo[i].hinfo = NULL;
  }
}

InfoCache::~InfoCache()
{
  for (int i=0; i<INFOCACHE_MAX; i++) {
    if (arcinfo[i].hinfo) LocalFree(arcinfo[i].hinfo);
  }
}

void InfoCache::Clear(void)
{
  cs.Enter();
  for (int i=0; i<INFOCACHE_MAX; i++) {
    if (arcinfo[i].hinfo) {
      LocalFree(arcinfo[i].hinfo);
      arcinfo[i].hinfo = NULL;
    }
  }
  nowno = 0;
  cs.Leave();
}

//キャッシュ追加
//filepath:ファイルパス
//ph:ハンドルへのポインタ
//INFOCACHE_MAX超えたら古いのは消す
void InfoCache::Add(const wchar_t *filepath, HLOCAL *ph)
{
  cs.Enter();
  if (arcinfo[nowno].hinfo) LocalFree(arcinfo[nowno].hinfo);
  arcinfo[nowno].hinfo = *ph;
  wcscpy_s(arcinfo[nowno].path, filepath);
  nowno = (nowno+1)%INFOCACHE_MAX;
  cs.Leave();
}

//キャッシュにあればハンドルを返す
bool InfoCache::GetCache(const wchar_t *filepath, HLOCAL *ph)
{
  bool ret = false;
  int no = nowno-1;
  if (no < 0) no = INFOCACHE_MAX -1;
  for (int i=0; i<INFOCACHE_MAX; i++) {
    if (arcinfo[no].hinfo == NULL) break;
    if (_wcsicmp(arcinfo[no].path, filepath) == 0) {
      *ph = arcinfo[no].hinfo;
      ret = true;
      break;
    }
    no--;
    if (no < 0) no = INFOCACHE_MAX -1;
  }
  return ret;
}

//キャッシュにあればコピー
//ph:アーカイブ情報を受け取るハンドルへのポインタ
//pinfo:アーカイブのファイル情報を受け取るポインタ
//      あらかじめ pinfo に filename か position をセットしておく。
//      キャッシュがあれば filename(position) の一致する情報を返す。
//キャッシュになければ、SPI_NO_FUNCTION が返る。
//キャッシュにあれば SPI_ALL_RIGHT が返る。
//アーカイブ情報はキャッシュにあるが、filename(position) が一致しない場合は
//SPI_NOT_SUPPORT が返る。エラーの場合はエラーコードが返る。
int InfoCache::Dupli(const wchar_t *filepath, HLOCAL *ph, fileInfoW *pinfo)
{
  cs.Enter();
  HLOCAL hinfo;
  int ret = GetCache(filepath, &hinfo);

if (ret) {
  ret = SPI_ALL_RIGHT;
  if (ph != NULL) {
    auto size = LocalSize(hinfo);
    /* 出力用のメモリの割り当て */
    *ph = LocalAlloc(LMEM_FIXED, size);
    if (*ph == NULL) {
      ret = SPI_NO_MEMORY;
    } else {
      memcpy(*ph, (void*)hinfo, size);
    }
  } else {
    fileInfoW *ptmp = (fileInfoW *)hinfo;
    if (pinfo->filename[0] != L'\0') {
      for (;;) {
        if (ptmp->method[0] == '\0') {
          ret = SPI_NOT_SUPPORT;
          break;
        }
        if (_wcsicmp(ptmp->filename, pinfo->filename) == 0) break;
        ptmp++;
      }
    } else {
      for (;;) {
        if (ptmp->method[0] == '\0') {
          ret = SPI_NOT_SUPPORT;
          break;
        }
        if (ptmp->position == pinfo->position) break;
        ptmp++;
      }
    }
    if (ret == SPI_ALL_RIGHT) *pinfo = *ptmp;
  }
} else {
  ret = SPI_NO_FUNCTION;
}

  cs.Leave();
  return ret;
}
