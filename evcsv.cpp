/* evscv.cpp
 * R.Kubo 2008-2010
 * 重みベクトルの情報をcsv形式で出力
 */

#include <cstdlib>
#include <cmath>
#include <fstream>

#include "offset.h"

#define FNAME_EVCSV			"evdata.csv"

typedef TempParam<short,short> param_t;

static char s_koma[][3] = {
	"FU", "KY", "KE", "GI", "KI", "KA", "HI", "OU",
	"TO", "NY", "NK", "NG", "  ", "UM", "RY",
};

void Statistics( ostream& fout, const short* p, int num );
void KingPiece( ostream& fout, const param_t& p, int sou, KOMA koma );
KOMA GetKomaCSA( char* str );
int SimpleInfo( Evaluate* eval, bool cname );
int DetailInfo( Evaluate* eval );

int main( int argc, char** argv ){
/************************************************
メインルーチン
************************************************/
	Evaluate* eval;
	int ret = 0;
	bool simple = false;
	bool cname = false;
	char fname[1024] = "evdata";

	// 乱数の初期化
	init_gen_rand( (unsigned int)time( NULL ) );

	for( int i = 1 ; i < argc ; i++ ){
		if     ( strcmp( argv[i], "-s" ) == 0 ){
			// 簡易出力
			simple = true;
		}
		else if( strcmp( argv[i], "-c" ) == 0 ){
			// 簡易出力での列名表示
			cname = true;
		}
		else{
			strcpy( fname, argv[i] );
		}
	}

	// evdataの読み込み
	eval = new Evaluate( fname );

	if( simple ){
		// 簡易
		ret = SimpleInfo( eval, cname );
	}
	else{
		// 詳細
		ret = DetailInfo( eval );
	}

	delete eval;

	return ret;
}

void Statistics( ostream& fout, const short* p, int num ){
/************************************************
統計情報
************************************************/
	int i;
	double aav = 0.0;
	double ave = 0.0;
	double var = 0.0;
	double dev;
	int zero = 0;
	int max = INT_MIN;
	int min = INT_MAX;
	int plus = 0;
	int minus = 0;

	for( i = 0 ; i < num ; i++ ){
		aav += abs( p[i] );
		ave += p[i];
		var += p[i] * p[i];

		if( p[i] > max ){ max = p[i]; }
		if( p[i] < min ){ min = p[i]; }

		if     ( p[i] > 0 ){ plus++; }
		else if( p[i] < 0 ){ minus++; }
		else               { zero++; }
	}

	aav /= num;
	ave /= num;
	var = var / num - ave * ave;
	dev = sqrt( var ) * num / ( num - 1 );

	fout << num << ',';
	fout << max << ',';
	fout << min << ',';
	fout << aav << ',';
	fout << ave << ',';
	fout << dev << ',';
	fout << ( (double)plus / num ) << ',';
	fout << ( (double)minus / num ) << ',';
	fout << ( (double)zero / num ) << ',';
	fout << '\n';
}

void KingPiece( ostream& fout, const param_t& p, int sou, KOMA koma ){
/************************************************
玉-駒
************************************************/
	int i, j, k;
	int addr, diff;
	int value;
	int souX = sou % 16 - 1;
	int souY = sou / 16 - 1;

	fout << "King" << Addr2X(sou) << Addr2Y(sou);
	fout << "-" << s_koma[koma-SFU] << '\n';
	k = ConstData::sente_num[koma];
	for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
		for( i = 0x09 ; i >= 0x01 ; i -= 0x01 ){
			addr = i + j;
			if( addr == sou ){
				fout << "x,";
			}
			else{
				diff = ConstData::_aa2num[ConstData::_addr[sou]][ConstData::_addr[addr]];
				value  = p.king_piece[diff][k];
				value += p.kingX_piece[souX][diff][k];
				value += p.kingY_piece[souY][diff][k];
				fout << value << ',';
			}
		}
		fout << '\n';
	}
}

KOMA GetKomaCSA( char* str ){
/************************************************
2文字から駒の種類を取得。(CSA形式)
************************************************/
	KOMA koma = EMPTY;
	int i;

	for( i = 0 ; i < 15 ; i++ ){
		if( str[0] == s_koma[i][0] && str[1] == s_koma[i][1] ){
			koma = FU + i;
			break;
		}
	}

	return koma;
}

int DetailInfo( Evaluate* eval ){
/************************************************
詳細出力
************************************************/
	ofstream fout;
	KOMA koma;
	const param_t& param = (*eval);

	assert( param.king_piece[ConstData::_aa2num[0][0]][ConstData::sente_num[SKI]] == 0 );

	// ファイルへの書き込み
	fout.open( FNAME_EVCSV );
	if( fout.fail() ){
		cerr << "Open Error!..[" << FNAME_EVCSV << ']' << '\n';
		return 1;
	}

	cerr << "Writing the data for evaluate to CSV file..";

	fout << "Size," << PARAM_NUM << '\n';

	// 駒の価値
	for( koma = SFU ; koma <= SRY ; koma++ ){
		fout << "BAN " << s_koma[koma-SFU];
		fout << ',';
		fout << eval->GetV_BAN( koma );
		fout << '\n';
	}

	fout << "Feature,Size,Max,Min,Average(ABS),Average,Deviance,Plus,Minus,Zero" << '\n';

	// All
	fout << "All,"; Statistics( fout, (short*)param.king_hand_sfu, PARAM_NUM );

	// 各パラメータ
	vector<PARAM_OFFSET> offset;

	getEvalOffset( offset );

	for( unsigned i = 0 ; i < offset.size() - 1 ; i++ ){
		fout << offset[i].name << ',';
		Statistics( fout, (short*)((char*)&param+offset[i].offset), (offset[i+1].offset - offset[i].offset ) / sizeof(short) );
	}

	// 8八玉-金
	KingPiece( fout, param, 0x88, SKI );

	// 9九玉-金
	KingPiece( fout, param, 0x99, SKI );

	fout.close();

	cerr << "OK!" << '\n';

	return 0;
}

int SimpleInfo( Evaluate* eval, bool cname ){
/************************************************
簡易出力
************************************************/
	KOMA koma;
	const param_t& param = (*eval);

	assert( param.king_piece[ConstData::_aa2num[0][0]][ConstData::sente_num[SKI]] == 0 );

	// 列名
	if( cname ){
		// 駒の価値
		cout << "FU,KY,KE,GI,KI,KA,HI,,TO,NY,NK,NG,,UM,RY,";
		// 統計
		cout << "Size,Max,Min,Average(ABS),Average,Deviance,Plus,Minus,Zero";
		cout << '\n';
	}

	// 駒の価値
	for( koma = SFU ; koma <= SRY ; koma++ ){
		cout << eval->GetV_BAN( koma );
		cout << ',';
	}

	// 統計
	Statistics( cout, (short*)param.king_hand_sfu, PARAM_NUM );

	return 0;
}
