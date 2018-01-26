/* debug.cpp
 * R.Kubo 2011-2012
 * デバッグ用
 */

#if WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <fstream>
#include <cstdio>
#include <cstdarg>

#include "shogi.h"

#ifndef NDEBUG

void Debug::Print( const char* str, ... ){
	ofstream file;
	char buf[1024];
	time_t t;
	struct tm* tmDate;
	char strDate[64];

	va_list argx;
	va_start( argx, str );

	for( int i = 0 ; i < 10 ; i++ ){
		// ファイルオープン
		file.open( "debug.txt", ios::out | ios::app );
		if( file.is_open() ){
			// 時刻
			t = time( NULL );
			tmDate = localtime( &t );
			strftime( strDate, sizeof(strDate), "%F %H:%M:%S ", tmDate );

			// 出力文字列
			vsprintf( buf, str, argx );

			// 出力
			file << strDate << buf;
			file.close();

			break;
		}
#ifdef WIN32
		Sleep( 10 );
#else
		usleep( 10 * 1000 );
#endif
	}

	va_end( argx );
}

#endif // NDEBUG
