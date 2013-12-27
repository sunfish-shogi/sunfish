/* change.cpp
 * 一部のパラメータを書き換える。
 */

#include <climits>
#include <iostream>
using namespace std;

#include "offset.h"

int main( int argc, char* argv[] ){
	Evaluate* eval;
	vector<PARAM_OFFSET> offset;

	// 読み込み
	eval = new Evaluate();

	// オフセット情報初期化
	getEvalOffset( offset );

	for( int i = 1 ; i + 2 < argc ; i += 3 ){
		unsigned offset0 = UINT_MAX; // 基本オフセット
		unsigned offset1 = strtol( argv[i+1], NULL, 10 ); // オフセット
		int      value = strtol( argv[i+2], NULL, 10 ); // 書き換え値
		// オフセット確認
		for( unsigned j = 0 ; j < offset.size() - 1 ; j++ ){
			if( offset[j].name == argv[i] ){
				offset0 = offset[j].offset;
				break;
			}
		}
		// 認識できない名前
		if( offset0 == UINT_MAX ){
			cout << "Unknown Parmeter.. " << argv[i] << '\n';
			return 1;
		}
		// 書き換え
		*((short*)((char*)eval+offset0)+offset1) = value;
	}

	// 書き出し
	eval->WriteData();

	return 0;
}
