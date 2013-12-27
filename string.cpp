/* string.cpp
 * R.Kubo 2010
 * 文字列処理Stringクラス
 */

#include "match.h"

int String::WildCard( const char* str, const char* wcard ){
/************************************************
ワイルドカード
************************************************/
	while( 1 ){
		if( wcard[0] == '*' ){
			// 1文字ずつずらしながら比較。
			wcard++;
			while( 1 ){
				if( WildCard( str, wcard ) ){
					return 1;
				}

				if( str[0] == '\0' ){
					// 不一致
					return 0;
				}

				str++;
			}
		}

		else if( str[0] != wcard[0] ){
			// 不一致
			return 0;
		}

		else if( str[0] == '\0' ){
			// 一致
			return 1;
		}

		str++;
		wcard++;
	}
}
