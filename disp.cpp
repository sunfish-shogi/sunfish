/* disp.cpp
 * 一部のパラメータを表示する。
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

	for( int i = 1 ; i + 1 < argc ; i += 2 ){
		unsigned offset0 = UINT_MAX; // 基本オフセット
		unsigned offset1 = strtol( argv[i+1], NULL, 10 ); // オフセット
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
		// 表示
		cout << *((short*)((char*)eval+offset0)+offset1) << '\n';
	}

	return 0;
}
