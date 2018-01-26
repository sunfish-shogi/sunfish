/* offset.h
 * 各パラメータのoffsetを管理
 */

#include "shogi.h"

struct PARAM_OFFSET{
	string name;
	unsigned offset;
	bool flag;
	PARAM_OFFSET( string _name, unsigned _offset ){
		name = _name;
		offset = _offset;
		flag = true;
	}
};

bool readEval( Evaluate* eval, const char* fname, vector<PARAM_OFFSET>& offset );
bool writeEval( Evaluate* eval, const char* fname, vector<PARAM_OFFSET>& offset );
void getEvalOffset( vector<PARAM_OFFSET>& offset );
