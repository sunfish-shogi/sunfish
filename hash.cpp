/* hash.cpp
 * R.Kubo 2008-2012
 * 局面のハッシュ
 */

#include <ctime>
#include <cstdlib>
#include <fstream>
#include "match.h"

#define FNAME_HASH			"hash"

int Hash::InitHash(){
/************************************************
ハッシュの初期化
ファイルから読み込めた場合は0以外の値を返す。
************************************************/
	int i, j;

	if( 0 == ReadHash() ){
		// ファイルからの読み込みに失敗
		for( i = 0 ; i <= GRY - SFU ; i++ ){
			for( j = 0x11 ; j <= 0x99 ; j++ )
				hash_ban[i][j] = gen_rand64();
		}
		for( i = 0 ; i <= GHI - SFU ; i++ ){
			for( j = 0 ; j <= 18 ; j++ )
				hash_dai[i][j] = gen_rand64();
		}
		hash_sengo = gen_rand64();

		// 生成した乱数を記録
		WriteHash();

		return 0;
	}

	return 1;
}

int Hash::ReadHash(){
/************************************************
ハッシュの読み込み
************************************************/
	ifstream fin;

	// ファイルのオープン
	fin.open( FNAME_HASH, ios::in | ios::binary );
	if( fin.fail() ){
	    
		cerr << "Open Error!..[" << FNAME_HASH << ']' << '\n';
		return 0;
	}

	cerr << "Reading the hash..";
	cerr.flush();

	// 読み込み
	fin.read( (char*)hash_ban[0], sizeof(hash_ban) );
	fin.read( (char*)hash_dai[0], sizeof(hash_dai) );
	fin.read( (char*)&hash_sengo, sizeof(hash_sengo) );
	if( fin.fail() ){
		cerr << "Fail!" << '\n';
		fin.close();
		return 0;
	}

	fin.close();

	cerr << "OK!" << '\n';

	return 1;
}

int Hash::WriteHash(){
/************************************************
ハッシュの書き込み
************************************************/
	ofstream fout;

	// ファイルのオープン
	fout.open( FNAME_HASH, ios::out | ios::binary );
	if( fout.fail() ){
		cerr << "Open Error!..[" << FNAME_HASH << ']' << '\n';
		return 0;
	}

	cerr << "Writing the hash..";

	// 書き込み
	fout.write( (char*)hash_ban[0], sizeof(hash_ban) );
	fout.write( (char*)hash_dai[0], sizeof(hash_dai) );
	fout.write( (char*)&hash_sengo, sizeof(hash_sengo) );
	if( fout.fail() ){
		cerr << "Fail!" << '\n';
		fout.close();
		return 0;
	}

	fout.close();

	cerr << "OK!" << '\n';

	return 1;
}

HashTable::HashTable(){
/************************************************
コンストラクタ
************************************************/
	Hash::SetHandler( this );
	hash_table = new HASH_TABLE[Hash::Size()];
	Init();
}

HashTable::~HashTable(){
/************************************************
デストラクタ
************************************************/
	Hash::UnsetHandler( this );
	delete [] hash_table;
}

void HashTable::Resize(){
/************************************************
サイズ変更
************************************************/
	delete [] hash_table;
	hash_table = new HASH_TABLE[Hash::Size()];
	Init();
}

ENTRY_TYPE HashTable::Entry( uint64 hash, int rdepth,
	 int alpha, int beta, int value, const MOVE& move ){
/************************************************
ハッシュテーブルに追加
************************************************/
	HASH_TABLE table;
	ENTRY_TYPE ret = HASH_NOENTRY;

	if( !GetTable( hash, table ) || table.rdepth <= rdepth ){
		if( table.hash == hash ){
			ret = HASH_OVERWRITE;
		}
		else{
			if( table.cnt == cnt ){
				ret = HASH_COLLISION;
			}
			else{
				ret = HASH_ENTRY;
			}
			table.first = 0U;
		}
		table.cnt = cnt;
		table.hash = hash;
		if( value >= VMAX || value <= VMIN ){
			table.rdepth = INT_MAX;
			table.type = VALUE_EXACT;
		}
		else{
			table.rdepth = rdepth;
			if( value >= beta ){
				table.type = VALUE_LOWER;
			}
			else if( value <= alpha ){
				table.type = VALUE_UPPER;
			}
			else{
				table.type = VALUE_EXACT;
			}
		}
		table.value = value;
		unsigned m = move.Export();
		if( table.first != m ){
			table.second = table.first;
		}
		else{
			table.second = 0U;
		}
		table.first = m;

		SetTable( table );
	}

	return ret;
}

DfPnTable::DfPnTable(){
/************************************************
コンストラクタ
************************************************/
	Hash::SetHandler( this );
	dfpn_table = new DFPN_TABLE[Hash::Size()];
	Init();
}

DfPnTable::~DfPnTable(){
/************************************************
デストラクタ
************************************************/
	Hash::UnsetHandler( this );
	delete [] dfpn_table;
}

void DfPnTable::Resize(){
/************************************************
サイズ変更
************************************************/
	delete [] dfpn_table;
	dfpn_table = new DFPN_TABLE[Hash::Size()];
	Init();
}

ShekTable::ShekTable(){
/************************************************
コンストラクタ
************************************************/
	Hash::SetHandler( this );
	shek_table = new SHEK_TABLE[Hash::Size()];
	Init();
}

ShekTable::~ShekTable(){
/************************************************
デストラクタ
************************************************/
	Hash::UnsetHandler( this );
	delete [] shek_table;
}

void ShekTable::Resize(){
/************************************************
サイズ変更
************************************************/
	delete [] shek_table;
	shek_table = new SHEK_TABLE[Hash::Size()];
	Init();
}

void ShekTable::Set( Shogi* pshogi ){
/************************************************
1手前までの局面を登録して探索中フラグを立てる。
************************************************/
	int num = pshogi->GetNumber();

	// 局面を戻す。
	while( pshogi->GoBack() )
		;

	// 局面を進めながら局面を登録する。
	while( pshogi->GetNumber() < num ){
		Set( pshogi->GetHashB(), pshogi->GetDai(), pshogi->GetSengo() );
		pshogi->GoNext();
	}
}

void ShekTable::Unset( Shogi* pshogi ){
/************************************************
1手前までの局面について探索中フラグをおろす。
************************************************/
	int num = pshogi->GetNumber();

	// 局面を戻しながら登録を解除する。
	while( pshogi->GoBack() ){
		Unset( pshogi->GetHashB() );
	}

	// 局面を進める。
	while( pshogi->GetNumber() < num ){
		pshogi->GoNext();
	}
}
