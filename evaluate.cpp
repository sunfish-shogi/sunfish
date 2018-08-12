/* evaluate.cpp
 * R.Kubo 2008-2010
 * 局面評価
 */

#include <fstream>
#include <cstdlib>
#include "shogi.h"

#define __EVALUATE__

// 先後対称性のテスト
// 重みベクトルに乱数を入れて
// 先後同形で評価値0ならOK..みたいな?
#define SYM_TEST			0
#if SYM_TEST
#	include "SFMT/SFMT.h"
	// 0であるべき要素が非0だと
	// feature.h内のassertで引っかかる。
#	undef assert
#	define assert(a)			;
#endif

void Evaluate::Init(){
/************************************************
初期化
************************************************/
	int i;

	// 駒の価値だけ値を入れる。
	for( i = 0 ; i < RY ; i++ ){
		piece[i] = def_piece[i];
	}

	// それ以外は0を入れる。
#if !SYM_TEST
	memset( PARAM, 0, sizeof(short) * PARAM_NUM );
#else
	for( i = 0 ; i < PARAM_NUM ; i++ ){
		PARAM[i] = (short)gen_rand32();
	}
#endif

	// パラメータ変更時の処理
	Update();

	// 分散
	deviation = 0.0;

#if EV_TABLE_DEBUG
	edebug.open( "edebug" );
#endif
}

int Evaluate::_ReadData( const char* _fname ){
/************************************************
評価パラメータの読み込み
************************************************/
	ifstream fin;

	fin.open( _fname, ios::in | ios::binary );
	if( fin.fail() ){
		cerr << "Open Error!..[" << _fname << ']' << '\n';
		return 0;
	}

	cerr << "Reading the data for evaluation..";
	cerr.flush();

	fin.read( (char*)piece, sizeof(piece) );
	fin.read( (char*)PARAM, sizeof(short) * PARAM_NUM  );

	if( fin.fail() )
	{
		cerr << "Fail!" << '\n';
		fin.close();
		return 0;
	}

	fin.close();

	// パラメータ変更時の処理
	Update();

	cerr << "OK!" << '\n';

	return 1;
}

int Evaluate::_WriteData( const char* _fname ){
/************************************************
評価パラメータの書き込み
************************************************/
	ofstream fout;

	fout.open( _fname, ios::out | ios::binary );
	if( fout.fail() ){
		cerr << "Open Error!..[" << _fname << ']' << '\n';
		return 0;
	}

	cerr << "Writing the data for evaluation..";

	fout.write( (char*)piece, sizeof(piece) );
	fout.write( (char*)PARAM, sizeof(short) * PARAM_NUM  );

	if( fout.fail() )
	{
		cerr << "Fail!" << '\n';
		fout.close();
		return 0;
	}

	fout.close();

	cerr << "OK!" << '\n';

	return 1;
}

int Evaluate::GetValue0( ShogiEval* pshogi ) const{
/************************************************
駒の価値による評価
************************************************/
	// 局面
	const KOMA* ban = pshogi->GetBan();
	const int* dai = pshogi->GetDai();

	int v = 0;
	int i, j;

	// 盤上の駒の価値
	for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
		for( i = 1 ; i <= 9 ; i++ ){
			int addr = i + j;
			KOMA koma;
			if( ( koma = ban[addr] ) != EMPTY ){
				v += GetV_BAN( koma );
			}
		}
	}

	// 持ち駒
	for( i = SFU ; i <= SHI ; i++ ){
		v += GetV_BAN(i) * dai[i];
	}

	for( i = GFU ; i <= GHI ; i++ ){
		v += GetV_BAN(i) * dai[i];
	}

	return v;
}

int Evaluate::GetValue1_dis1_ns( ShogiEval* __restrict__ pshogi )
/************************************************
駒の価値以外による評価
テーブルに登録されていればそれを返す。 2010/09/11
************************************************/
{
	uint64 hash = pshogi->GetHash();
	EVAL_TABLE table = eval_table[hash&Hash::Mask()];

#define HASH_ERR_CHECK		1
#if HASH_ERR_CHECK
	if( table.entry && ( table.hash ^ (uint64)table.value ) == hash ){
#else
	if( table.entry && table.hash == hash ){
#endif
#if EV_TABLE_DEBUG
		edebug << __LINE__ << ' ' << (hash&Hash::Mask()) << ' ' << table.hash << ' ' << hash << ' ' << table.value << '\n';
#endif
		return table.value;
	}
	else{
		int value;
#if 0
		assert( deviation == 0.0 );
#endif
		value = _GetValue1_dis1_ns( pshogi ) + (int)rand_norm.Generate( 0.0, deviation );
#if HASH_ERR_CHECK
		table.hash = hash ^ (uint64)value;
#else
		table.hash = hash;
#endif
		table.entry = true;
		table.value = value;
		eval_table[hash&Hash::Mask()] = table;
#if EV_TABLE_DEBUG
		edebug << __LINE__ << ' ' << (hash&Hash::Mask()) << ' ' << table.hash << ' ' << hash << ' ' << table.value << '\n';
#endif
		return value;
	}
#undef HASH_ERR_CHECK
}

#define FUNC_S(x)			(v += (x))
#define FUNC_G(x)			(v -= (x))
#define FUNC_SO(x)			(v += (x))
#define FUNC_GO(x)			(v -= (x))
#define DEGREE_S(x,y,z)		(GetDegreeOfFreedomS(v,ban,diffS,diffG,addr,(x),(y),(z)))
#define DEGREE_G(x,y,z)		(GetDegreeOfFreedomG(v,ban,diffS,diffG,addr,(x),(y),(z)))
int Evaluate::_GetValue1_dis1_ns( ShogiEval* __restrict__ pshogi ) const{
/************************************************
駒の価値以外による評価
差分評価するものを除外
************************************************/
	int v = 0;
#define DISABLE_FEATURE_1
#include "feature.h"
#undef DISABLE_FEATURE_1
	return v;
}

int Evaluate::GetValue1_dis2_ns( ShogiEval* __restrict__ pshogi ) const{
/************************************************
駒の価値以外による評価
差分評価しないもののみ
************************************************/
	int v = 0;
#define DISABLE_FEATURE_2
#include "feature.h"
#undef DISABLE_FEATURE_2
	return v;
}
#undef FUNC_S
#undef FUNC_G
#undef FUNC_SO
#undef FUNC_GO
#undef DEGREE_S
#undef DEGREE_G

Evaluate& Evaluate::operator=( Param& param ){
	int i;

	// Piece
	for( i = 0 ; i < RY ; i++ )
		piece[i] = (short)( param.piece[i] + 0.5 );

	// 駒割り以外
	for( i = 0 ; i < PARAM_NUM ; i++ )
		PARAM[i] = (short)( param.PARAM[i] + 0.5 );

	return (*this);
}
