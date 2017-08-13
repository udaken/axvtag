/**
 * Cache system for Susie Plug-in by shimitei (modified by gocha)
 * <http://www.asahi-net.or.jp/~kh4s-smz/spi/make_spi.html>
 */

#ifndef infcache_h
#define infcache_h

#include <windows.h>
#include "critsect.h"
#include "spi00am.h"

typedef struct ArcInfo
{
  HLOCAL hinfo;         // fileInfo[]
  WCHAR path[MAX_PATH];  // ファイルパス
} ArcInfo;

#define INFOCACHE_MAX 0x10
class InfoCache
{
public:
  InfoCache();
  ~InfoCache();
  void Clear(void); //キャッシュクリア
  void Add(const wchar_t *filepath, HLOCAL *ph); //キャッシュに追加。INFOCACHE_MAX を超えると古いのは消す。
  //キャッシュにあればアーカイブ情報をコピー。
  int Dupli(const wchar_t *filepath, HLOCAL *ph, fileInfoW *pinfo);
private:
  CriticalSection cs;
  ArcInfo arcinfo[INFOCACHE_MAX];
  int nowno;
  bool GetCache(const wchar_t *filepath, HLOCAL *ph);
};

#endif
