/* ins.cpp
 * 評価パラメータを一部カットする。
 */

#include "offset.h"

int main( int argc, char* argv[] ){
	int i;
	unsigned j;
	Evaluate* eval;
	vector<PARAM_OFFSET> offset;

	eval = new Evaluate( false );

	getEvalOffset( offset );

	// インサートするもの
	for( i = 1 ; i < argc ; i++ ){
		bool exist = false;
		for( j = 0 ; j < offset.size() - 1 ; j++ ){
			if( offset[j].name == argv[i] ){
				offset[j].flag = false;
				exist = true;
				break;
			}
		}
		if( !exist ){
			cout << "Unknown..[" << argv[i] << "]\n";
		}
	}

	// インサートするものを除外して読み込む。
	readEval( eval, FNAME_EVDATA, offset );

	// 全部フラグを立てる。
	for( j = 0 ; j < offset.size() - 1 ; j++ ){
		offset[j].flag = true;
	}

	// インサートして書き出す。
	writeEval( eval, FNAME_EVDATA, offset );

	delete eval;

	return 0;
}
