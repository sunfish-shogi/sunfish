/* FileList.cpp
 * R.Kubo 2010
 * ファイルを列挙するFileListクラス
 */

#ifdef WIN32
#	include <windows.h>
#else
#	include <dirent.h>
#	include <strings.h>
#endif

#include "shogi.h"

int FileList::Enumerate( const char* directory, const char* extension ){
/************************************************
ファイルの列挙
************************************************/
	string dir;
	string fname;
	string ext;
	unsigned int len;
	int num = 0;

#ifdef WIN32
	HANDLE hFind;
	WIN32_FIND_DATAA fd;
#else
	DIR *pdir;
	struct dirent *pent;
#endif

	// ディレクトリ
	dir = directory;
	len = dir.length();

	// 拡張子
	ext = '.';
	ext += extension;

	// スラッシュを除去
	if( dir[len-1] == '/'
#ifdef WIN32
		|| dir[len-1] == '\\'
#endif
		)
	{
		dir = dir.substr( 0, len - 1 );
	}

#ifdef WIN32
	// 拡張子に当てはまるファイルを列挙
	fname = dir + "\\*";
	if( 1 < ext.length() )
		fname += extension;

	hFind = FindFirstFileA( fname.c_str(), &fd );

	if( hFind == INVALID_HANDLE_VALUE ){
		cerr << "There are no files!..["
			<< fname << ']' << '\n';
		return 0;
	}
	dir += '\\';
#else
	// ディレクトリのオープン
	if( ( pdir = opendir( dir.c_str() ) ) == NULL ){
		cerr << "Couldn't open the directory!..["
			<< dir << ']' << '\n';
		return 0;
	}
	dir += '/';
#endif

	for( ; ; ){
#ifdef WIN32
		fname = fd.cFileName;
#else
		if( NULL == ( pent = readdir( pdir ) ) ){
			closedir( pdir );
			break;
		}

		fname = pent->d_name;
#endif

		// ディレクトリ".", ".."の場合
		if( fname[fname.length()-1] == '.' )
			continue;

#ifndef WIN32
		// 拡張子のチェック
		if( 1 < ( len = ext.length() ) && ( fname.length() <= len ||
			0 != strcasecmp( fname.substr( fname.length() - len ).c_str(), ext.c_str() ) ) )
		{
			continue;
		}
#endif

		// リストに追加
		flist.push_back( dir + fname );

		num++;

#ifdef WIN32
		// 次のファイル
		if( !FindNextFileA( hFind, &fd ) ){
			// ハンドルのクローズ
			FindClose( hFind );
			break;
		}
#endif
	}

	it = flist.begin();

	return num;
}
