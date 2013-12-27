/* param.cpp
 * R.Kubo 2008-2010
 * 評価パラメータ
 */

#include <fstream>
#include "shogi.h"

void Param::Init0(){
/************************************************
全て0で初期化
************************************************/
	int i;

	// 駒割り
	for( i = 0 ; i < RY ; i++ )
		piece[i] = 0.0;

	// その他の要素
	for( i = 0 ; i < PARAM_NUM ; i++ )
		PARAM[i] = 0.0;

	Cumulate();
}

int Param::ReadData( const char* fname ){
/************************************************
パラメータの読み込み
************************************************/
	ifstream fin;

	fin.open( fname, ios::in | ios::binary );
	if( fin.fail() ){
		cerr << "Open Error!..[" << fname << ']' << '\n';
		return 0;
	}

	cerr << "Reading the parameters..";
	cerr.flush();

	fin.read( (char*)piece, sizeof(piece) );
	fin.read( (char*)PARAM, sizeof(float) * PARAM_NUM  );

	if( fin.fail() )
	{
		cerr << "Fail!" << '\n';
		fin.close();
		return 0;
	}

	fin.close();

	Cumulate();

	cerr << "OK!" << '\n';

	return 1;
}

int Param::WriteData( const char* fname ){
/************************************************
評価パラメータの書き込み
************************************************/
	ofstream fout;

	fout.open( fname, ios::out | ios::binary );
	if( fout.fail() ){
		cerr << "Open Error!..[" << fname << ']' << '\n';
		return 0;
	}

	cerr << "Writing the parameters..";

	fout.write( (char*)piece, sizeof(piece) );
	fout.write( (char*)PARAM, sizeof(float) * PARAM_NUM  );

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

Param& Param::operator+=( Param& param ){
	int i;

	// Piece
	for( i = 0 ; i < RY ; i++ )
		piece[i] += param.piece[i];

	// 駒割り以外
	for( i = 0 ; i < PARAM_NUM ; i++ )
		PARAM[i] += param.PARAM[i];

	return (*this);
}

Param& Param::operator-=( Param& param ){
	int i;

	// Piece
	for( i = 0 ; i < RY ; i++ )
		piece[i] -= param.piece[i];

	// 駒割り以外
	for( i = 0 ; i < PARAM_NUM ; i++ )
		PARAM[i] -= param.PARAM[i];

	return (*this);
}

Param& Param::operator*=( double d ){
	int i;

	// Piece
	for( i = 0 ; i < RY ; i++ )
		piece[i] *= d;

	// 駒割り以外
	for( i = 0 ; i < PARAM_NUM ; i++ )
		PARAM[i] *= d;

	return (*this);
}

Param& Param::operator=( Param& param ){
	int i;

	// Piece
	for( i = 0 ; i < RY ; i++ )
		piece[i] = param.piece[i];

	// 駒割り以外
	for( i = 0 ; i < PARAM_NUM ; i++ )
		PARAM[i] = param.PARAM[i];

	return (*this);
}

Param& Param::operator=( Evaluate& eval ){
	int i;

	// Piece
	for( i = 0 ; i < RY ; i++ )
		piece[i] = eval.piece[i];

	// 駒割り以外
	for( i = 0 ; i < PARAM_NUM ; i++ )
		PARAM[i] = eval.PARAM[i];

	return (*this);
}
