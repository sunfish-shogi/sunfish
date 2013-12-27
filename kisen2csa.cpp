/* kisen2csa.cpp
 * R.Kubo 2011-2012
 * 棋泉 => CSA 変換
 */

#include "shogi.h"
#include <fstream>

#define FILTER_RATE			1900

bool kisen2csa( const char* source, const char* dist );

// メインルーチン
int main( int argc, char* argv[] ){
	// 乱数の初期化
	init_gen_rand( (unsigned int)time( NULL ) );

	if( argc < 3 ){
		cerr << "Usage : kisen2csa [source] [dist]\n";
	}

	if( !kisen2csa( argv[1], argv[2] ) ){
		cerr << "failed!\n";
		return 1;
	}
	cerr << "succeeded.\n";

	return 0;
}

// 変換ルーチン
bool kisen2csa( const char* source, const char* dist ){
	FileList flist;
	string fname;
	string fname2;
	Shogi shogi;
	int num = 0;
	int illegal = 0;

	// .kif を列挙
	if( 0 == flist.Enumerate( source, "kif" ) ){
		return false;
	}

	while( flist.Pop( fname ) ){
		ifstream kif;
		ifstream ipx;
		unsigned char buf[512];
		bool ipx_opened;

		// .kif
		kif.open( fname.c_str(), ios::in | ios::binary );
		if( !kif ){ continue; }
		cout << fname << '\n';

		// .ipx
		fname2 = fname.substr( 0, fname.length()-3 ) + "ipx";
		ipx.open( fname2.c_str(), ios::in | ios::binary );
		ipx_opened = ipx;

		// read
		while( true ){
			ofstream fout;
			char name1[15] = "";
			char name2[15] = "";
			unsigned rate1 = 0;
			unsigned rate2 = 0;
			char ofname[64];

			// 対局者情報
			if( ipx_opened ){
				ipx.read( (char*)buf, 256 );
				if( !ipx.eof() ){
					memcpy( name1, (char*)(buf+0x00), sizeof(name1) );
					name1[14] = '\0';
					memcpy( name2, (char*)(buf+0x0e), sizeof(name2) );
					name2[14] = '\0';
					rate1 = *(unsigned short*)(buf+0xd4);
					rate2 = *(unsigned short*)(buf+0xd6);
				}
			}

			// 棋譜
			kif.read( (char*)buf, 512 );
			if( kif.eof() ){ break; }

			// フィルタ
			if( rate1 < FILTER_RATE || rate2 < FILTER_RATE ){
				continue;
			}

			while( shogi.GoBack() )
				;

			for( int i = 0 ; i < 512 ; i += 2 ){
				MOVE move;
				int to   = buf[i  ];
				int from = buf[i+1];

				if( to == 0 || from == 0 ){ break; }

				// 移動先
				if( to > 100 ){
					to -= 100;
					move.nari = 1;
				}
				else{
					move.nari = 0;
				}
				assert( to >= 1 );
				assert( to <= 81 );
				move.to = ConstData::_iaddr0[to];
				assert( move.to >= 0x11 );
				assert( move.to <= 0x99 );

				// 移動元
				if( from > 100 ){
					KOMA koma;
					KOMA sengo = shogi.GetSengo();
					for( koma = HI ; koma >= FU ; koma-- ){
						from -= shogi.GetDai()[koma|sengo];
						if( from <= 100 ){ break; }
					}
					move.from = DAI | sengo | koma;
				}
				else{
					assert( from >= 1 );
					assert( from <= 81 );
					move.from = ConstData::_iaddr0[from];
					assert( move.from >= 0x11 );
					assert( move.from <= 0x99 );
				}

				if( !shogi.Move( move ) ){
#if 0
					cout << "illegal move!!\n";
					shogi.PrintBan();
					cout << dec;
					cout << "sengo:" << shogi.GetSengo() << '\n';
					cout << "from :" << from << '\n';
					cout << "to   :" << to   << '\n';
					cout << hex;
					cout << "from :0x" << move.from << '\n';
					cout << "to   :0x" << move.to   << '\n';
					cout << "nari :0x" << move.nari << '\n';
					cout.flush();
					abort();
#endif
					illegal++;
					break;
				}
			}

			sprintf( ofname, "%s/kifu%08d.csa", dist, num );
			fout.open( ofname );
			if( !fout ){ return false; }
			fout << "N+" << name1 << '(' << rate1 << ")\n";
			fout << "N-" << name2 << '(' << rate2 << ")\n";
			fout << shogi.GetStrKifuCSA();
			fout.close();

			num++;
		}

		// close
		if( kif ){ kif.close(); }
		if( ipx ){ ipx.close(); }
	}

	if( num == 0 ){ return false; }

	cout << "number  :" << num << '\n';
	cout << "illegal :" << illegal << '\n';

	return true;
}
