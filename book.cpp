/* book.cpp
 * R.Kubo 2008-2012
 * 将棋の定跡
 */

#include <cstdlib>
#include <fstream>
#include <map>
#include "shogi.h"

#define FNAME_BOOK			"book"

Book::Book(){
/************************************************
コンストラクタ
************************************************/
	blist = new list<BLIST>[BOOK_HASH_SIZE];

	// 定石の読み込み
	Read( FNAME_BOOK );
}

Book::Book( const char* _fname ){
/************************************************
コンストラクタ
************************************************/
	blist = new list<BLIST>[BOOK_HASH_SIZE];

	// 定石の読み込み
	if( _fname != NULL )
		Read( _fname );
}

Book::~Book(){
/************************************************
デストラクタ
************************************************/
	delete [] blist;
}

int Book::Read( const char* _fname ){
/************************************************
定跡の読み込み
************************************************/
	ifstream fin;
	uint64 hash;
	int num;
	int mv;
	int cnt;
	int val;
	int i;

	DelAll();

	if( _fname != NULL )
		fname = _fname;

	// ファイルのオープン
	fin.open( fname.c_str(), ios::in | ios::binary );
	if( fin.fail() ){
		cerr << "Open Error!..[" << fname.c_str() << ']' << '\n';
		return 0;
	}

	cerr << "Reading the book..";
	cerr.flush();

	for( ; ; ){
		// 定跡局面と指し手の数を読み込み
		fin.read( (char*)&hash, sizeof(hash) );
		fin.read( (char*)&num , sizeof(num) );
		if( fin.eof() )
			break;
		else if( fin.fail() ){
			cerr << "Fail! (Book)[1]" << '\n';
			fin.close();
			return 1;
		}

		// 指し手の読み込み
		for( i = 0 ; i < num ; i++ ){
			fin.read( (char*)&mv , sizeof(mv) );
			fin.read( (char*)&cnt, sizeof(cnt) );
			fin.read( (char*)&val, sizeof(val) );
			if( fin.fail() ){
				cerr << "Fail! (Book)[2]" << '\n';
				fin.close();
				return 1;
			}
			Add( hash, mv, ADD_NEW, cnt, val );
		}
	}

	fin.close();

	cerr << "OK!" << '\n';

	return 1;
}

int Book::Write( const char* _fname ){
/************************************************
定跡の書き込み
************************************************/
	ofstream fout;
	list<BLIST>::iterator ib;
	list<MLIST>::iterator im;
	list<MLIST>* pmlist;
	unsigned i;

	if( _fname != NULL )
		fname = _fname;

	// ファイルのオープン
	fout.open( fname.c_str(), ios::out | ios::binary );
	if( fout.fail() ){
		cerr << "Open Error!..[" << fname.c_str() << ']' << '\n';
		return 0;
	}

	cerr << "Writing the book..";

	for( i = 0 ; i < BOOK_HASH_SIZE ; i++ ){
		for( ib = blist[i].begin() ; ib != blist[i].end() ; ++ib ){
			// 局面と指し手の数の書き込み
			fout.write( (char*)&(*ib).hash, sizeof((*ib).hash) );
			fout.write( (char*)&(*ib).num , sizeof((*ib).num) );
			if( fout.fail() ){
				cerr << "Fail! (Book)" << '\n';
				fout.close();
				return 1;
			}

			pmlist = &(*ib).mlist;
			for( im = pmlist->begin() ; im != pmlist->end() ; ++im ){
				fout.write( (char*)&(*im).mv  , sizeof((*im).mv) );
				fout.write( (char*)&(*im).cnt , sizeof((*im).cnt) );
				fout.write( (char*)&(*im).val , sizeof((*im).val) );
				if( fout.fail() ){
					cerr << "Fail! (Book)" << '\n';
					fout.close();
					return 1;
				}
			}
		}
	}

	fout.close();

	cerr << "OK!" << '\n';

	return 1;
}

int Book::_Make( const char* dir, int max, ADD_METHOD method ){
/************************************************
定跡の作成
************************************************/
	Shogi* pshogi;
	MOVE move;
	uint64 hash;
	int num = 0;
	FileList flist;
	string fname;
	KIFU_INFO kinfo;

	// ファイルの列挙
	if( 0 == flist.Enumerate( dir, "csa" ) ){
		cerr << "Error!" << '\n';
		return 0;
	}

	pshogi = new Shogi( HIRATE );

	cerr << "Make a book.." << '\n';

	while( flist.Pop( fname ) ){
		if( 0 == pshogi->InputFileCSA( fname.c_str(), &kinfo ) ){
			continue;
		}

		num++;

		// 棋譜の最後かどうか
#if 1
		bool is_last = true;
#endif

		// 定跡登録
		while( pshogi->GetMove( move ) && pshogi->GoBack() ){
			// 勝った側の指し手だけを採用する。 2011/03/13
#if 1
			if( is_last ){
				// 初回は1手だけ戻す。
				// 最後に指した方が勝ったはず。
				is_last = false;
			}
			else{
				// それ以外は2手ずつ戻す。
				if( !( pshogi->GetMove( move ) && pshogi->GoBack() ) ){
					break;
				}
			}
#endif

			// 指定した手数より手前なら登録
			if( pshogi->GetNumber() < max ){
				hash = pshogi->GetHash();
				switch( method ){
				case OVER_WRITE:
					Add( hash, move, OVER_WRITE, 1 );
					break;
				case ADD_NEW:
					Add( hash, move, ADD_NEW, 1, kinfo.start.export_date() );
					break;
				default:
					Add( hash, move, OVER_WRITE, 1 );
					break;
				}
			}
		}

		// 進捗表示
		Progress::Print( num * 100 / flist.Size() );
	}

	// 進捗表示終了
	Progress::Clear();

	cerr << "Number of files : " << num << '\n';

	delete pshogi;

	return 1;
}

int Book::Date( const char* dir, double r, int max ){
/************************************************
日付を考慮した定跡生成
************************************************/
	// 読み込み
	if( 0 == _Make( dir, max, ADD_NEW ) ){ return 0; }

	// 解析
	for( unsigned i = 0 ; i < BOOK_HASH_SIZE ; i++ ){
		for( list<BLIST>::iterator ib = blist[i].begin() ; ib != blist[i].end() ; ++ib ){
			int num = 0;
			list<MLIST>* pmlist = &(*ib).mlist;
			list<MLIST>::iterator im;
			list<MLIST>::iterator im2;
			double scale = INT_MAX / (*ib).num;

			// 重み計算
			for( im = pmlist->begin() ; im != pmlist->end() ; ++im ){
				if( (*im).date != 0U ){
					int num2 = num;
					double w = 0.0;
					for( im2 = im ; im2 != pmlist->end() ; ++im2 ){
						if( (*im2).mv == (*im).mv ){
							w += pow( r, num2 ); // 重み
							if( im2 != im ){
								(*im2).date = 0U; // 重複
							}
						}
						num2++;
					}
					assert( w * scale <= (double)INT_MAX );
					assert( num != 0 || w * scale >= 1.0 );
					(*im).cnt = (int)( w * scale );
				}
				num++;
			}

			// 重複削除
			im = pmlist->begin();
			while( im != pmlist->end() ){
				if( (*im).date == 0U ){
					im = pmlist->erase( im );
				}
				else{
					im++;
				}
			}

			(*ib).num = pmlist->size();

			assert( pmlist->size() < 1000 );
		}
	}

	// 保存
	if( 0 == Write() ){ return 0; }

	return 1;
}

int Book::Add( uint64 hash, unsigned int mv, ADD_METHOD method, int cnt, int val ){
/************************************************
定跡の追加
************************************************/
	int i;
	int flag;
	list<BLIST>::iterator ib;
	BLIST btemp;
	list<MLIST>* pmlist;
	list<MLIST>::iterator im;
	MLIST mtemp;

	i = (int)( hash & BOOK_HASH_MASK );

	// 局面が既にあるか調べる。
	flag = 0;
	for( ib = blist[i].begin() ; ib != blist[i].end() ; ++ib ){
		if( (*ib).hash == hash ){
			flag = 1;
			break;
		}
	}

	if( flag == 0 ){
		btemp.hash = hash;
		btemp.num = 0;
		btemp.cnt = 0;
		blist[i].push_front( btemp );
		ib = blist[i].begin();
	}

	pmlist = &(*ib).mlist;

	if( method == OVER_WRITE ){
		// 既に指し手があるか調べる。
		for( im = pmlist->begin() ; im != pmlist->end() ; ++im ){
			if( (*im).mv == mv ){
				(*im).cnt += cnt;
				(*im).val  = val;
				(*ib).cnt += cnt;
				return 1;
			}
		}
	}

	mtemp.mv   = mv;
	mtemp.cnt  = cnt;
	mtemp.val  = val;
	if( method == ADD_NEW ){
		// 挿入位置を探す。(valについて降順の状態を保つ)
		for( im = pmlist->begin() ; im != pmlist->end() ; ++im ){
			if( (*im).val <= val ){
				pmlist->insert( im, mtemp );
				break;
			}
		}
		if( im == pmlist->end() ){
			pmlist->push_front( mtemp );
		}
	}
	else{
		pmlist->push_front( mtemp );
	}
	(*ib).cnt += cnt;
	(*ib).num++;

	return 1;
}

void Book::DelAll(){
/************************************************
定跡の全削除
************************************************/
	unsigned i;

	for( i = 0 ; i < BOOK_HASH_SIZE ; i++ ){
		// 局面を削除
		blist[i].clear();
	}
}

int Book::GetMove( Shogi* pshogi, MOVE& move, TYPE type ){
/************************************************
定跡から指し手を探す。
************************************************/
	unsigned i;
	int flag;
	list<BLIST>::iterator ib;
	list<MLIST>::iterator im;
	int cnt;
	uint64 hash = pshogi->GetHash();
	MOVE mtemp;

	i = (unsigned)( hash & BOOK_HASH_MASK );

	// 局面が既にあるか調べる。
	flag = 0;
	for( ib = blist[i].begin() ; ib != blist[i].end() ; ++ib ){
		if( (*ib).hash == hash ){
			flag = 1;
			break;
		}
	}

	if( flag == 0 )
		return 0;

	// 指し手を決定
	switch( type ){
	// 評価値を用いる場合
	case TYPE_EVAL:
		{
			list<MLIST>* pmlist = &(*ib).mlist;
			int vmax = (*pmlist->begin()).val - 1;
			for( im = pmlist->begin() ; im != pmlist->end() ; ++im ){
				mtemp.Import( (*im).mv );
				if( (*im).val > vmax && pshogi->IsLegalMove( mtemp ) ){
					vmax = (*im).val;
					move = mtemp;
				}
			}
			return 1;
		}
		break;

	// 出現頻度を用いる場合
	case TYPE_FREQ:
	default:
		list<MLIST>* pmlist = &(*ib).mlist;
		cnt = (int)( gen_rand32() % (*ib).cnt );
		for( im = pmlist->begin() ; im != pmlist->end() ; ++im ){
			cnt -= (*im).cnt;
			if( cnt < 0 ){
				mtemp.Import( (*im).mv );
				if( pshogi->IsLegalMove( mtemp ) ){
					move = mtemp;
					return 1;
				}
			}
		}
		break;
	}

	return 0;
}

int Book::GetMoveAll( Shogi* pshogi, Moves& moves, TYPE type ){
/************************************************
定跡手を列挙する。
************************************************/
	int i;
	int flag;
	list<BLIST>::iterator ib;
	list<MLIST>::iterator im;
	list<MLIST>* pmlist;
	uint64 hash = pshogi->GetHash();
	MOVE mtemp;

	i = (int)( hash & BOOK_HASH_MASK );

	// 局面が既にあるか調べる。
	flag = 0;
	for( ib = blist[i].begin() ; ib != blist[i].end() ; ++ib ){
		if( (*ib).hash == hash ){
			flag = 1;
			break;
		}
	}

	if( flag == 0 )
		return 0;

	// 指し手を列挙
	pmlist = &(*ib).mlist;
	for( im = pmlist->begin() ; im != pmlist->end() ; ++im ){
		mtemp.Import( (*im).mv );
		if( pshogi->IsLegalMove( mtemp ) ){
			switch( type ){
			// 評価値を用いる場合
			case TYPE_EVAL:
				mtemp.value = (*im).val;
				break;
			// 出現頻度を用いる場合
			case TYPE_FREQ:
			default:
				mtemp.value = (*im).cnt * 100 / (*ib).cnt;
				break;
			}
			moves.Add( mtemp );
		}
	}
	moves.Sort();

	return moves.GetNumber();
}

int Book::EntryCheck( uint64 hash, const MOVE& move ){
/************************************************
定跡に指し手があるか調べる。
************************************************/
	int i;
	int flag;
	list<BLIST>::iterator ib;
	list<MLIST>::iterator im;
	list<MLIST>* pmlist;
	unsigned int mv;

	i = (int)( hash & BOOK_HASH_MASK );

	// 局面が既にあるか調べる。
	flag = 0;
	for( ib = blist[i].begin() ; ib != blist[i].end() ; ++ib ){
		if( (*ib).hash == hash ){
			flag = 1;
			break;
		}
	}

	if( flag == 0 )
		return 0;

	// 指し手を探す。
	pmlist = &(*ib).mlist;
	for( im = pmlist->begin() ; im != pmlist->end() ; ++im ){
		mv = move.Export();
		if( (*im).mv == mv )
			return 1;
	}

	return 0;
}

Book& Book::operator+=( Book& book ){
/************************************************
定跡データの併合
************************************************/
	list<BLIST>::iterator ib;
	list<MLIST>::iterator im;
	list<MLIST>* pmlist;
	unsigned h;
	MOVE mtemp;

	for( h = 0 ; h < BOOK_HASH_SIZE ; ++h ){
		for( ib = book.blist[h].begin() ; ib != book.blist[h].end() ; ++ib ){
			pmlist = &(*ib).mlist;
			for( im = pmlist->begin() ; im != pmlist->end() ; ++im ){
				mtemp.Import( (*im).mv );
				Add( (*ib).hash, mtemp, OVER_WRITE, (*im).cnt );
			}
		}
	}

	return *(this);
}

void Book::PrintDistribution(){
/************************************************
ハッシュ表での分布の調査
************************************************/
	map<int,int> dist;
	int max_num = 0;
	unsigned h;

	for( h = 0 ; h < BOOK_HASH_SIZE ; ++h ){
		int num = blist[h].size();
		if( dist.count(num) == 0 ){
			dist[num] = 0;
		}
		dist[num]++;
		max_num = max( max_num, num );
	}

	for( int i = 0 ; i < max_num ; i++ ){
		cout << i << '\t';
		if( dist.count(i) == 1 ){
			cout << dist[i] << '\n';
		}
		else{
			cout << "0\n";
		}
	}
}
