/* sunfish.cpp
 * R.Kubo 2008-2012
 * 将棋の対局プログラム
 */

#include <cstdlib>
#include <cmath>
#include <fstream>

#include "match.h"
#include "sunfish.h"

#ifdef NLEARN
	#define _SHOGI_DEVELOP_			0
#else
	#define _SHOGI_DEVELOP_			1
#endif

enum MODE{
	MODE_CONSOLE = 0,
	MODE_SELF,
	MODE_AGREE,
	MODE_NEXT, // 次の一手
};

bool getParamInt( int argc, char* argv[], int& idx,
	int& value, const int min, const int max, bool require, bool& get );
bool getParamDbl( int argc, char* argv[], int& idx,
	double& value, const double min, const double max, bool require, bool& get );
bool getParamStr( int argc, char* argv[], int& idx,
	char* &value, bool require, bool& get );
void ShowHelp( const char* argv0 );
void RandNormalTest( double _ave, double _dev );
bool GenMovesTest( const char* directory );
bool GenCheckTest( const char* directory );
void Mate3Test( const char* fname );
void EvaluateTest( const char* fname );
void DiffEvalTest( const char* fname );
void Uint96Test();
void MakeCode();
bool ExperimentKing( const char* directory );
bool ExperimentPosEval( const char* directory );
bool ExperimentPiecePos( const char* directory );
bool InputTest( const char* directory );
bool CopyTest( const char* fname );

int main( int argc, char* argv[] ){
/************************************************
Main Routine
************************************************/
	MATCH_CONF config = DEF_CONF; // 設定
	MODE mode = MODE_CONSOLE;     // 対局モード
	char* directory = NULL;
	int thread_num = 1;           // スレッド数
	int hash_mbytes = -1;
	int i;

	// 乱数の初期化
	init_gen_rand( (unsigned int)time( NULL ) );

	cerr << PROGRAM_NAME << ' ';
	cerr << PROGRAM_VER << ' ';
	cerr << COPYRIGHT << '\n';
	cerr << "Help : " << argv[0] << " --help" << '\n' << '\n';

	// コマンド引数の処理
	for( i = 1 ; i < argc ; i++ ){
// 開発用
#if _SHOGI_DEVELOP_
		// コードの生成
		if( 0 == strcmp( argv[i], "-x" ) ){
			MakeCode();
			return 0;
		}
		// 正規乱数生成テスト
		else if( 0 == strcmp( argv[i], "-rn" ) ){
			if( i + 2 >= argc ){
				cerr << "Usage: sunfish -rn [ave] [dev]" << '\n';
				return 1;
			}
			RandNormalTest( strtod( argv[i+1], NULL ), strtod( argv[i+2], NULL ) );
			return 0;
		}
		// 合法手生成ルーチンのテスト
		else if( 0 == strcmp( argv[i], "-gm" ) ){
			if( i == argc -1 ){
				cerr << "Usage: sunfish -gm [directory]" << '\n';
				return 1;
			}
			GenMovesTest( argv[i+1] );
			return 0;
		}
		// 王手生成ルーチンのテスト
		else if( 0 == strcmp( argv[i], "-gc" ) ){
			if( i == argc -1 ){
				cerr << "Usage: sunfish -gc [directory]" << '\n';
				return 1;
			}
			GenCheckTest( argv[i+1] );
			return 0;
		}
		// 3手詰みルーチンのテスト
		else if( 0 == strcmp( argv[i], "-m3" ) ){
			if( i == argc -1 ){
				cerr << "Usage: sunfish -m3 [CSA file]" << '\n';
				return 1;
			}
			Mate3Test( argv[i+1] );
			return 0;
		}
		// 評価関数のテスト
		else if( 0 == strcmp( argv[i], "-ev" ) ){
			if( i == argc -1 ){
				cerr << "Usage: sunfish -ev [CSA file]" << '\n';
				return 1;
			}
			EvaluateTest( argv[i+1] );
			return 0;
		}
		// 差分計算のテスト
		else if( 0 == strcmp( argv[i], "-df" ) ){
			if( i == argc -1 ){
				cerr << "Usage: sunfish -df [CSA file]" << '\n';
				return 1;
			}
			DiffEvalTest( argv[i+1] );
			return 0;
		}
		// 96bit演算テスト
		else if( 0 == strcmp( argv[i], "-bt" ) ){
			Uint96Test();
			return 0;
		}
		// 玉の移動による評価値の変動
		else if( 0 == strcmp( argv[i], "--exp-king" ) ){
			if( i == argc -1 ){
				cerr << "Usage: sunfish --exp-king [directory]" << '\n';
				return 1;
			}
			ExperimentKing( argv[i+1] );
			return 0;
		}
		// 駒割り以外の評価値の変動
		else if( 0 == strcmp( argv[i], "--exp-pos-eval" ) ){
			if( i == argc -1 ){
				cerr << "Usage: sunfish --exp-pos-eval [directory]" << '\n';
				return 1;
			}
			ExperimentPosEval( argv[i+1] );
			return 0;
		}
		// 駒の存在比率
		else if( 0 == strcmp( argv[i], "--exp-piece-pos" ) ){
			if( i == argc -1 ){
				cerr << "Usage: sunfish --exp-piece-pos [directory]" << '\n';
				return 1;
			}
			ExperimentPiecePos( argv[i+1] );
			return 0;
		}
		// 棋譜の読み込みテスト
		else if( 0 == strcmp( argv[i], "--input-test" ) ){
			if( i == argc -1 ){
				cerr << "Usage: sunfish --inpt-test [directory]" << '\n';
				return 1;
			}
			InputTest( argv[i+1] );
			return 0;
		}
		// EvalShogi::Copy()のテスト
		else if( 0 == strcmp( argv[i], "--copy-test" ) ){
			if( i == argc -1 ){
				cerr << "Usage: sunfish --copy-test [filename]" << '\n';
				return 1;
			}
			CopyTest( argv[i+1] );
			return 0;
		}

		else
#endif // _SHOGI_DEVELOP_
		// 先手番はCOM
		if     ( 0 == strcmp( argv[i], "-s" ) ){
			config.com_s = 1;
		}
		// 後手番はCOM
		else if( 0 == strcmp( argv[i], "-g" ) ){
			config.com_g = 1;
		}
		// 探索の深さ
		else if( 0 == strcmp( argv[i], "-d" ) ||
		         0 == strcmp( argv[i], "--depth" ) ){
			bool get = true;
			if( getParamInt( (int)argc, argv, i, config.depth, 1, INT_MAX, true, get ) ){
			}
			else{
				cerr << "Usage: sunfish --depth [N(>=1)]\n";
				return 1;
			}
		}
		// 制限時間
		else if( 0 == strcmp( argv[i], "-t" ) ||
		         0 == strcmp( argv[i], "--time" ) ){
			bool get = true;
			if( getParamInt( (int)argc, argv, i, config.limit, 0, INT_MAX, true, get ) ){
			}
			else{
				cerr << "Usage: sunfish --time [N(>=0)]\n";
				return 1;
			}
		}
		// 制限ノード数
		else if( 0 == strcmp( argv[i], "-o" ) ||
		         0 == strcmp( argv[i], "--node" ) ){
			bool get = true;
			if( getParamInt( (int)argc, argv, i, config.node, 0, INT_MAX, true, get ) ){
			}
			else{
				cerr << "Usage: sunfish --node [N(>=0)]\n";
				return 1;
			}
		}
		// 投了のしきい値
		else if( 0 == strcmp( argv[i], "-i" ) ||
		         0 == strcmp( argv[i], "--resign" ) ){
			bool get = true;
			if( getParamInt( (int)argc, argv, i, config.resign, RESIGN_VALUE_MIN, INT_MAX, true, get ) ){
			}
			else{
				cerr << "Usage: sunfish --resign [V(>=" << RESIGN_VALUE_MIN << ")]\n";
				return 1;
			}
		}
		// スレッド数
		else if( 0 == strcmp( argv[i], "-p" ) ||
		         0 == strcmp( argv[i], "--parallel" ) ){
			bool get = true;
			if( getParamInt( (int)argc, argv, i, thread_num, 1, INT_MAX, true, get ) ){
			}
			else{
				cerr << "Usage: sunfish --node [N(>=1)]\n";
				return 1;
			}
		}
		// 置換テーブルのサイズ指定
		else if( 0 == strcmp( argv[i], "-m" ) ||
		         0 == strcmp( argv[i], "--hash-mbytes" ) ){
			bool get = true;
			if( getParamInt( (int)argc, argv, i, hash_mbytes, 0, INT_MAX, true, get ) ){
			}
			else{
				cerr << "Usage: sunfish --hash-mbytes [N(>=0)]\n";
				return 1;
			}
		}
		// 起動時棋譜読み込み
		else if( 0 == strcmp( argv[i], "-f" ) ||
		         0 == strcmp( argv[i], "--file" ) ){
			bool get = true;
			if( getParamStr( (int)argc, argv, i, config.fname, true, get ) ){
			}
			else{
				cerr << "Usage: sunfish --file [file name]\n";
				return 1;
			}
		}
		// 自己対局
		else if( 0 == strcmp( argv[i], "-r" ) ||
		         0 == strcmp( argv[i], "--repeat" ) ){
			bool get = true;
			if( getParamInt( (int)argc, argv, i, config.repeat, 1, INT_MAX, true, get ) ){
				mode = MODE_SELF;
			}
			else{
				cerr << "Usage: sunfish --repeat [N(>=1)]\n";
				return 1;
			}
		}
		// CSA形式で表示
		else if( 0 == strcmp( argv[i], "-c" ) ||
		         0 == strcmp( argv[i], "--csa" ) ){
			config.csa = 1;
		}
		// 自動quit
		else if( 0 == strcmp( argv[i], "-q" ) ||
		         0 == strcmp( argv[i], "--auto-quit" ) ){
			config.aquit = 1;
		}

#if !defined(NLEARN)
		// 機械学習
		else if( 0 == strcmp( argv[i], "-l" ) ||
		         0 == strcmp( argv[i], "--learn" ) ){
			Learn* plearn;

			plearn = new Learn();
			if( !plearn->LearnMain() ){
				cerr << "Error!" << '\n';
				delete plearn;
				return 1;
			}
			delete plearn;
			return 0;
		}
		// pvmove生成
		else if( 0 == strcmp( argv[i], "-lp" ) ){
			Learn* plearn;

			plearn = new Learn();
			if( !plearn->LearnMain( Learn::MODE_PVMOVE ) ){
				cerr << "Error!" << '\n';
				delete plearn;
				return 1;
			}
			delete plearn;
			return 0;
		}
		// 勾配計算
		else if( 0 == strcmp( argv[i], "-lg" ) ){
			Learn* plearn;

			plearn = new Learn();
			if( !plearn->LearnMain( Learn::MODE_GRADIENT ) ){
				cerr << "Error!" << '\n';
				delete plearn;
				return 1;
			}
			delete plearn;
			return 0;
		}
		// パラメータ更新
		else if( 0 == strcmp( argv[i], "-la" ) ){
			Learn* plearn;

			plearn = new Learn();
			if( !plearn->LearnMain( Learn::MODE_ADJUSTMENT ) ){
				cerr << "Error!" << '\n';
				delete plearn;
				return 1;
			}
			delete plearn;
			return 0;
		}
		// 機械学習(averaged perceptron)
		else if( 0 == strcmp( argv[i], "-l2" ) ||
		         0 == strcmp( argv[i], "--learn2" ) ){
			LearnAP* plearnAP;

			plearnAP = new LearnAP();
			if( !plearnAP->LearnAPMain() ){
				cerr << "Error!" << '\n';
				delete plearnAP;
				return 1;
			}
			delete plearnAP;
			return 0;
		}
#endif // !defined(NLEARN)
		// 定跡登録
		else if( 0 == strcmp( argv[i], "-b" ) ||
		         0 == strcmp( argv[i], "--book" ) ){
			Book* pbook;
			int ply = INT_MAX;;
			bool get = true;
			char* dir = NULL;
			if( getParamStr( (int)argc, argv, i, dir,             true,  get ) &&
				getParamInt( (int)argc, argv, i, ply, 1, INT_MAX, false, get )
			){
				pbook = new Book();
				if( !pbook->Make( dir, ply ) ){
					cerr << "Error!" << '\n';
					delete pbook;
					return 1;
				}
				delete pbook;
				return 0;
			}
			else{
				cerr << "Usage: sunfish --book [directory] [ply(>=1)]\n";
				return 1;
			}
		}
		// 日付を考慮した定跡
		else if( 0 == strcmp( argv[i], "-bd" ) ||
		         0 == strcmp( argv[i], "--book-date" ) ){
			Book* pbook;
			char* dir = NULL;
			double r = 0.8;
			int ply = INT_MAX;;
			bool get = true;
			if( getParamStr( (int)argc, argv, i, dir,             true,  get ) &&
				getParamDbl( (int)argc, argv, i,   r, 0.0, 1.0,   false, get ) &&
				getParamInt( (int)argc, argv, i, ply, 1, INT_MAX, false, get )
			){
				pbook = new Book();
				if( !pbook->Date( dir, r, ply ) ){
					cerr << "Error!" << '\n';
					delete pbook;
					return 1;
				}
				delete pbook;
				return 0;
			}
			else{
				cerr << "Usage: sunfish --book-date [directory] [reduced rate] [ply(>=1)]\n";
				return 1;
			}
		}
		// 一致率
		else if( 0 == strcmp( argv[i], "-a" ) ||
		         0 == strcmp( argv[i], "--agree" ) ){
			bool get = true;
			if( getParamStr( (int)argc, argv, i, directory, true, get ) ){
				mode = MODE_AGREE;
			}
			else{
				cerr << "Usage: sunfish --agree [directory]\n";
				return 1;
			}
		}
		// 次の一手
		else if( 0 == strcmp( argv[i], "-e" ) ||
		         0 == strcmp( argv[i], "--next" ) ){
			mode = MODE_NEXT;
		}
		// CSAプロトコルによる通信対局
		else if( 0 == strcmp( argv[i], "-n" ) ||
		         0 == strcmp( argv[i], "--network" ) ){
			MatchCSA* pmatchCSA;

			pmatchCSA = new MatchCSA();

			pmatchCSA->MatchNetwork();

			delete pmatchCSA;

			return 0;
		}
		// バージョンを表示
		else if( 0 == strcmp( argv[i], "-v" ) ||
		         0 == strcmp( argv[i], "--version" ) ){
			cout << "Version " << PROGRAM_VER << '\n';
			cout << "Table : " << HashSize::BytesPerSize( 1 ) << "[Bytes/Size]\n";
			return 0;
		}
		// ヘルプを表示
		else if( 0 == strcmp( argv[i], "-h" ) ||
		         0 == strcmp( argv[i], "--help" ) ){
			ShowHelp( argv[0] );
			return 0;
		}
		// 不明なオプション
		else{
			cerr << "Unknown Option!..[" << argv[i] << "]" << '\n';
			return 0;
		}
	}

	{
		// 通常の対局
		Match* pmatch;

		if( hash_mbytes != -1 ){
			Hash::SetSize( HashSize::MBytesToPow2( hash_mbytes, thread_num ) );
		}

		pmatch = new Match( thread_num );

		switch( mode ){
		case MODE_AGREE:
			pmatch->AgreementTest( directory, &config );
			break;
		case MODE_SELF:
			pmatch->MatchSelf( &config );
			break;
		case MODE_NEXT:
			pmatch->NextMove( &config );
			break;
		default:
			pmatch->MatchConsole( &config );
			break;
		}

		delete pmatch;
	}

	return 0;
}

/************************************************
起動引数処理 整数値取得
************************************************/
bool getParamInt( int argc, char* argv[], int& idx,
	int& value, const int min, const int max, bool require, bool& get
){
	if( get ){
		if( argc > idx + 1 ){
			if( argv[idx+1][0] != '-' ){
				int a = strtol( argv[idx+1], NULL, 10 );
				if( a >= min && a <= max ){
					value = a;
					idx++;
					return true;
				}
			}
		}
		get = false;
		if( require ){ return false; }
		else         { return true; }
	}
	return true;
}

/************************************************
起動引数処理 実数値取得
************************************************/
bool getParamDbl( int argc, char* argv[], int& idx,
	double& value, const double min, const double max, bool require, bool& get
){
	if( get ){
		if( argc > idx + 1 ){
			if( argv[idx+1][0] != '-' ){
				double a = strtod( argv[idx+1], NULL );
				if( a >= min && a <= max ){
					value = a;
					idx++;
					return true;
				}
			}
		}
		get = false;
		if( require ){ return false; }
		else         { return true; }
	}
	return true;
}

/************************************************
起動引数処理 文字列取得
************************************************/
bool getParamStr( int argc, char* argv[], int& idx,
	char* &value, bool require, bool& get
){
	if( get ){
		if( argc > idx + 1 ){
			if( argv[idx+1][0] != '-' ){
				value = argv[idx+1];
				idx++;
				return true;
			}
		}
		get = false;
		if( require ){ return false; }
		else         { return true; }
	}
	return true;
}

void ShowHelp( const char* argv0 ){
/************************************************
ヘルプを表示
************************************************/
	cout << "Usage : " << argv0 << " [Option], .." << '\n' << '\n';

	cout << "Option" << '\n';
	cout << "-s        Sunfish makes the first move." << '\n';

	cout << "-g        Sunfish makes the second move." << '\n';

	cout << "-d [N]    --depth [N]" << '\n';
	cout << "          The depth.(default:N=" << DEF_DEPTH << ')' << '\n';

	cout << "-t [N]    --time [N]" << '\n';
	cout << "          The time limit.(default:non)" << '\n';

	cout << "-o [N]    --node [N]" << '\n';
	cout << "          The nodes limit.(default:non)" << '\n';

	cout << "-p [N]    --parallel [N]" << '\n';
	cout << "          The number of threads.(default:1)" << '\n';

	cout << "-i [N]    --resign [N]" << '\n';
	cout << "          The threshold of resignation.(default:" << RESIGN_VALUE_DEF <<")" << '\n';

	cout << "-m [N]    --hash-mbytes [N]" << '\n';
	cout << "          The hash table size.[MBytes]" << '\n';

	cout << "-f [F]    --file [F]" << '\n';
	cout << "          Open the CSA file." << '\n';
	cout << "          [F]..The CSA file name to open." << '\n';

	cout << "-r [N]    --repeat [N]" << '\n';
	cout << "          Repeat the match of shogi at two Sunfish." << '\n';
	cout << "          [N]..The number for repetition." << '\n';

	cout << "-c        --csa" << '\n';
	cout << "          Use CSA style for display." << '\n';

	cout << "-q        --auto-quit" << '\n';
	cout << "          Automatically quit when a game is ended." << '\n';
	cout << "          (It means not to use command mode.)" << '\n';

#ifndef NLEARN
	cout << "-l        --learn" << '\n';
	cout << "          Learning." << '\n';

	cout << "-l2       --learn2" << '\n';
	cout << "          Learning(averaged perceptron)." << '\n';
#endif

	cout << "-b [D]    --book [D] [N]" << '\n';
	cout << "          Make a book." << '\n';
	cout << "          [D]..The directory that there are CSA files." << '\n';
	cout << "          [N]..The maximum ply." << '\n';

	cout << "-a [D]    --agree [D]" << '\n';
	cout << "          Agreement test." << '\n';
	cout << "          [D]..The directory that there are CSA files." << '\n';

	cout << "-n        --network" << '\n';
	cout << "          Play on the network." << '\n';

	cout << "-v        --version" << '\n';
	cout << "          Show program version." << '\n';

	cout << "-h        --help" << '\n';
	cout << "          Show this help." << '\n';
}

// 開発用コード
#if _SHOGI_DEVELOP_

void RandNormalTest( double _ave, double _dev ){
/************************************************
正規乱数テスト
************************************************/
	int i;
	RandNormal rand;
	double sum  = 0.0;
	double sum2 = 0.0;
	double ave;
	double var;
	double dev;
	const int num = 1000000;
	for( i = 0 ; i < num ; i++ ){
		double r = rand.Generate( _ave, _dev );
		sum  += r;
		sum2 += r * r;
	}
	ave = sum / num;
	var =  sum2 / num - ave * ave;
	dev = sqrt( var ) * num / ( num - 1 );
	cout << "Averate   : " << ave << '\n';
	cout << "Variance  : " << var << '\n';
	cout << "Deviation : " << dev << '\n';
}

typedef void GENFUNC( Shogi&, Moves& );

bool GenerateTest( const char* directory, GENFUNC* Generate1, GENFUNC* Generate2 ){
/************************************************
GenerateXX の動作確認
************************************************/
	FileList flist;
	string fname;
	Shogi shogi;
	Moves moves, moves2;
	int i;
	bool bSame;
	int cnt = 0;
	int num;
	unsigned dcnt = 0U; // デバッグ用カウンタ
	Timer tm;

	cout << "Number of CSA files:" << ( num = flist.Enumerate( directory, "csa" ) ) << '\n';

	tm.SetTime();

	while( flist.Pop( fname ) ){
		Progress::Print( cnt * 100 / num );
		cnt++;

		if( 0 == shogi.InputFileCSA( fname.c_str() ) ){
			continue;
		}

		do{
			dcnt++;

			// 指し手生成
			Generate1( shogi, moves );
			Generate2( shogi, moves2 );

			// 正規化
			for( i = 0 ; i < moves.GetNumber() ; i++ ){
				moves [i].value = moves [i].Export();
			}
			for( i = 0 ; i < moves2.GetNumber() ; i++ ){
				moves2[i].value = moves2[i].Export();
			}
			moves.Sort();
			moves2.Sort();

			bSame = false;
			if( moves.GetNumber() == moves2.GetNumber() ){
				// 比較
				bSame = true;
				for( i = 0 ; i < moves.GetNumber() ; i++ ){
					if( moves[i].value != moves2[i].value ){
						bSame = false;
					}
				}
			}

			// 不一致
			if( !bSame ){
				cout << '\n';
				cout << "counter :" << dcnt << '\n';
				cout << fname << '\n';
				cout << shogi.GetNumber() << '\n';
				shogi.PrintBan();
				if( shogi.GetSengo() == SENTE ){
					cout << "SENTE" << '\n';
				}
				else{
					cout << "GOTE" << '\n';
				}

				cout << "1 :";
				for( i = 0 ; i < moves.GetNumber() ; i++ ){
					if( moves[i].value != moves2[i].value ){
						cout << '*';
					}
					shogi.PrintMove( moves[i] );
				}
				cout << '\n';

				cout << "2 :";
				for( i = 0 ; i < moves2.GetNumber() ; i++ ){
					if( moves[i].value != moves2[i].value ){
						cout << '*';
					}
					shogi.PrintMove( moves2[i] );
				}
				cout << '\n';

				return false;
			}
		}while( shogi.GoBack() );
	}

	Progress::Print( 100 );
	cerr << '\n';

	cout << "Time : " << tm.GetTime() << "sec\n";

	return true;
}

void GenerateMoves( Shogi& shogi, Moves& moves ){
/************************************************
合法手列挙
************************************************/
	moves.Init();
	shogi.GenerateMoves( moves );
}

void GenerateMoves2( Shogi& shogi, Moves& moves ){
/************************************************
IsLegalMoveを使って全合法手生成
************************************************/
	MOVE move;
	bool promote;

	moves.Init();
	for( int a = 0 ; a < 81 ; a++ ){
		move.to   = ConstData::_iaddr[a];
		// 盤上
		for( int b = 0 ; b < 81 ; b++ ){
			move.from = ConstData::_iaddr[b];
			move.nari = 1;
			promote = false;
			if( shogi.IsLegalMove( move ) ){
				moves.Add( move );
				promote = true;
			}
			move.nari = 0;
			KOMA koma = shogi.GetBan(move.from);
			if( ( !promote || Shogi::Narazu( koma, move.to ) )
				&& shogi.IsLegalMove( move ) )
			{
				moves.Add( move );
			}
		}
		// 駒台
		for( KOMA k = FU ; k <= RY ; k++ ){
			move.from = k | shogi.GetSengo() | DAI;
			move.nari = 0;
			if( shogi.IsLegalMove( move ) ){
				moves.Add( move );
			}
		}
	}
}

bool GenMovesTest( const char* directory ){
/************************************************
GenerateMovesの動作確認
************************************************/
	return GenerateTest( directory, GenerateMoves, GenerateMoves2 );
}

void GenerateCheck( Shogi& shogi, Moves& moves ){
/************************************************
王手列挙
************************************************/
	moves.Init();
	shogi.GenerateCheck( moves );
}

void GenerateCheck2( Shogi& shogi, Moves& moves ){
/************************************************
合法手全てから王手のみを抽出
************************************************/
	moves.Init();
	shogi.GenerateMoves( moves );
	for( int i = 0 ; i < moves.GetNumber() ; i++ ){
		if( !shogi.IsCheckMove( moves[i] ) ){
			moves.Remove( i );
			i--;
		}
	}
}

bool GenCheckTest( const char* directory ){
/************************************************
GenerateCheckの動作確認
************************************************/
	return GenerateTest( directory, GenerateCheck, GenerateCheck2 );
}

void Mate3Test( const char* fname ){
/************************************************
3手詰めテスト
************************************************/
	Think think;
	ShogiEval shogi;
	Timer tm;

	if( !shogi.InputFileCSA( fname ) ){
		cerr << "Open Error!.." << fname << "]\n";
	}

	while( shogi.GoBack() )
		;

	// 時間計測開始
	tm.SetTime();

	do{
		for( int i = 0 ; i < 10 ; i++ ){
			think.Mate3Ply( &shogi );
		}
	}while( shogi.GoNext() );

	// 時間計測終了
	cout << "Time : " << tm.GetTime() << "sec\n";
}

void EvaluateTest( const char* fname ){
/************************************************
評価関数テスト
************************************************/
	Evaluate eval;
	ShogiEval shogi( &eval );
	Timer tm;

	if( !shogi.InputFileCSA( fname ) ){
		cerr << "Open Error!.." << fname << "]\n";
	}

	while( shogi.GoBack() )
		;

	// 時間計測開始
	tm.SetTime();

	do{
		for( int i = 0 ; i < 10000 ; i++ ){
			eval._GetValue1_dis1_ns( &shogi );
		}
	}while( shogi.GoNext() );

	// 時間計測終了
	cout << "Time : " << tm.GetTime() << "sec\n";
}

void DiffEvalTest( const char* fname ){
/************************************************
差分計算テスト
************************************************/
	Evaluate eval;
	ShogiEval shogi( &eval );

	if( !shogi.InputFileCSA( fname ) ){
		cerr << "Open Error!.." << fname << "]\n";
	}

	while( shogi.GoBack() )
		;

	do{
		shogi.PrintBan();
		cout.flush();
		int a = eval.GetValue( &shogi );
		int b = shogi.GetValue();
		cout << a << ',' << b << endl;
		if( a != b ){ abort(); }
	}while( shogi.GoNext() );
}

void Uint96Test(){
/************************************************
96bit演算テスト
************************************************/
#if 1
	for( int i = 0 ; i < 10000 ; i++ ){
		uint96 r( (unsigned)gen_rand64(), gen_rand64() );
		uint96 r2( (unsigned)gen_rand64(), gen_rand64() );
		uint96 r3( (unsigned)gen_rand64(), gen_rand64() );

		r &= r2 & r3;

		{
			int bit = 0;
			int bit0 = 0;
			uint96 x = U64(0);
			while( ( bit = r.getNextBit( bit ) ) != 0 ){
				assert( bit <= 96 );
				assert( bit >= bit0 );
				bit0 = bit;
				if( bit <= 64 ){
					x |= uint96( U64(1) << (bit-1) );
				}
				else{
					x |= uint96( 1U << (bit-65), U64(0) );
				}
			}
	
			cerr.width( 8 );
			cerr << hex << r.high;
			cerr.width( 16 );
#if !defined(VC6)
			cerr << hex << r.low << "..";
#else
			cerr << hex << (unsigned)( r.low >> 32 );
			cerr << hex << (unsigned)( r.low ) << "..";
#endif
			cerr.flush();
			if( r != x ){
				cerr << "error\n";
				cerr.width( 8 );
				cerr << hex << x.high;
				cerr.width( 16 );
#if !defined(VC6)
				cerr << hex << x.low << '\n';
#else
				cerr << hex << (unsigned)( x.low >> 32 );
				cerr << hex << (unsigned)( x.low ) << "..";
#endif
				return;
			}
			cerr << "ok!";
		}

		{
			int first = r.getFirstBit();
			int last  = r.getLastBit();
			uint96 x = U64(0), y = U64(0), z = U64(0);
			assert( ( first == 0 ) == ( last == 0 ) );
			if( first != 0 ){
				x = ( uint96(U64(1)) << (first-1) ) | ( uint96(U64(1)) << (last-1) );
				y = ( uint96(U64(1)) << (first-1) );
				z = ( uint96(U64(1)) << (last-1) );
			}
			else{
				x = U64(0);
			}
			cerr << ' ';
			cerr.width( 8 );
			cerr << hex << x.high;
			cerr.width( 16 );
#if !defined(VC6)
			cerr << hex << x.low;
#else
			cerr << hex << (unsigned)( x.low >> 32 );
			cerr << hex << (unsigned)( x.low ) << "..";
#endif
			cerr << " | ";
			cerr.width( 8 );
			cerr << hex << y.high;
			cerr.width( 16 );
#if !defined(VC6)
			cerr << hex << y.low;
#else
			cerr << hex << (unsigned)( y.low >> 32 );
			cerr << hex << (unsigned)( y.low ) << "..";
#endif
			cerr << " | ";
			cerr.width( 8 );
			cerr << hex << z.high;
			cerr.width( 16 );
#if !defined(VC6)
			cerr << hex << z.low;
#else
			cerr << hex << (unsigned)( z.low >> 32 );
			cerr << hex << (unsigned)( z.low ) << "..";
#endif
		}

		cerr << '\n';
	}
#else
	for( int i = -5 ; i < 100 ; i++ ){
		uint96 x = uint96( U64(1) ) << i;
		cerr << dec << i << "\t";
		cerr.width( 8 );
		cerr << hex << x.high;
		cerr.width( 16 );
		cerr << hex << x.low;
		cerr << '\n';
	}
#endif
}

#define DIFF2ATTACK			0
#define DBITS				0
#define NBITS				0
#define WDISS				0
#define AA2NUM				1

void MakeCode(){
/************************************************
ソースコード自動生成
************************************************/
#if DIFF2ATTACK
	{
		static char s_koma[][3] = {
			"FU", "KY", "KE", "GI", "KI", "KA", "HI", "OU",
			"TO", "NY", "NK", "NG", "  ", "UM", "RY",
		};

		int (*__diff2cap)[16*17+1];
		int* _diff2cap[GRY-SFU+1];

		__diff2cap = new int [GRY-SFU+1][16*17+1];

		// 目標位置に移動可能かどうか
		{
			KOMA koma;
			int i, j;
			int diff;
			int dir;
			int mov;

			for( koma = SFU ; koma <= GRY ; koma++ ){
				for( i = -0x80 ; i <= 0x80 ; i += 0x10 ){
					for( j = -8 ; j <= 8 ; j++ ){
						if( i != 0x80 && j == 8 ){
							break;
						}
						diff = i + j;
						dir = -ConstData::_diff2dir[diff];
						if( dir != 0 && koma != SOU && koma != GOU ){
							mov = ConstData::_mov[koma][ConstData::_idr[dir]];
							if( diff / (-dir) >= 2 && mov == 1 ){
								mov = 0;
							}
						}
						else{
							mov = 0;
						}

						if( mov != 0 ){
							// 王手可能位置
							__diff2cap[koma-SFU][diff+0x88] = dir;
						}
						else{
							// 非王手
							__diff2cap[koma-SFU][diff+0x88] = 0;
						}
					}
				}

				_diff2cap[koma-SFU] = &__diff2cap[koma-SFU][0x88];
			}
		}

		// 次の手で目標位置に移動可能になるかどうか
		{
			KOMA koma;
			int i, j;
			int m, n;
			int diff;
			int diff2;
			int dir;
			unsigned flag;
			char str[64];

			cout << "const unsigned ConstData::__diff2attack[][17][17] = {" << '\n';
			for( koma = SFU ; koma <= GRY ; koma++ ){
				cout << "\t{ // " << s_koma[(koma&0xF)-FU] << '\n';
				for( i = -0x80 ; i <= 0x80 ; i += 0x10 ){
					cout << "\t\t{ ";
					for( j = -8 ; j <= 8 ; j++ ){
						diff = i + j;
						flag = 0U;

						// 既に王手になるはずの位置を除外
						if( _diff2cap[koma-SFU][diff] == 0 || ConstData::_diff2dis[diff] >= 2 ){
							// attackできる場所を探す。
							for( m = -0x80 ; m <= 0x80 ; m += 0x10 ){
								for( n = -8 ; n <= 8 ; n++ ){
									diff2 = m + n;

									// diff2からはattackになるか
									if( ( dir = _diff2cap[koma-SFU][diff2] ) != 0 ||
										( dir = _diff2cap[(koma|NARI)-SFU][diff2] ) != 0 )
									{
										// diff => diff2 の移動は可能か
										if( abs( diff - diff2 ) <= 0x88 && _diff2cap[koma-SFU][diff-diff2] != 0 ){
											// 意味のない方向を除外
											if( dir != ConstData::_diff2dir[diff-diff2] ){
												flag |= ConstData::_dir2bit[ConstData::_diff2dir[diff2-diff]];
											}
										}
									}
								}
							}
						}

						assert( flag < (1<<12) );
						if( flag != 0U ){
							sprintf( str, " 0x%03x,", flag );
						}
						else{
							strcpy( str, "     0," );
						}
						cout << str;
					}
					cout << " }" << '\n';
				}
				cout << "\t}," << '\n';
			}
			cout << "};" << '\n';

			cout << "const unsigned (*ConstData::_diff2attack[GRY+1])[17][17] = {" << '\n';
			for( koma = 0 ; koma < SFU ){
				cout << "NULL, ";
			}
			cout << '\n';
			for( koma = SFU ; koma <= GRY ; koma++ ){
				cout << "\t(const unsigned (*)[17][17])&__diff2attack[" << (koma-SFU) <<"][8][8]," << '\n';
			}
			cout << "};" << '\n';
		}

		delete [] __diff2cap;
	}
#endif // DIFF2ATTACK

#if DBITS
	{
		Shogi shogi;
		char str[64];
		char name[][16] = {
			"_dbits_lu",
			"_dbits_up",
			"_dbits_ru",
			"_dbits_lt",
			"_dbits_rt",
			"_dbits_ld",
			"_dbits_dn",
			"_dbits_rd",
		};
		for( int d = 0 ; d < _DNUM ; d++ ){
			cout << "const uint96 ConstData::" << name[d] << "[81] = {\n";
			for( int a = 0 ; a < 81 ; a++ ){
				int dir = ConstData::_dir[d];
				int addr = ConstData::_iaddr[a];
				uint96 dbits = 0U;

				for( addr += dir ; shogi.GetBan()[addr] != WALL ; addr += dir ){
					dbits.setBit( ConstData::_addr[addr] );
				}
				sprintf( str, "uint96(0x%05x,0x%08x%08xULL), ", dbits.high,
					(unsigned)(dbits.low>>32), (unsigned)(dbits.low&0xFFFFFFFF) );
				if( a % 3 == 0 ){ cout << '\t'; }
				cout << str;
				if( a % 3 == 2 ){ cout << '\n'; }
			}
			cout << "};\n\n";
		}
	}
#endif // DBITS

#if NBITS
	{
		Shogi shogi;
		char str[64];
		uint96 nbits8[81];
		uint96 nbits24[81];
		for( int a = 0 ; a < 81 ; a++ ){
			nbits8[a] = uint96( U64(0) );
			nbits24[a] = uint96( U64(0) );
		}
		for( int a = 0 ; a < 81 ; a++ ){
			int addr = ConstData::_iaddr[a];
			for( int d = 0 ; d < _DNUM ; d++ ){
				int addr2 = addr + ConstData::_dir[d];
				if( shogi.GetBan()[addr2] != WALL ){
					nbits8[a] |= uint96(U64(1)) << ConstData::_addr[addr2];
					nbits24[a] |= uint96(U64(1)) << ConstData::_addr[addr2];
					for( int d2 = 0 ; d2 < _DNUM ; d2++ ){
						int addr3 = addr2 + ConstData::_dir[d2];
						if( addr3 != addr && shogi.GetBan()[addr3] != WALL ){
							nbits24[a] |= uint96(U64(1)) << ConstData::_addr[addr3];
						}
					}
				}
			}
		}

		cout << "const uint96 ConstData::_nbits8[81] = {\n";
		for( int a = 0 ; a < 81 ; a++ ){
			sprintf( str, "uint96(0x%05x,0x%08x%08xULL), ", nbits8[a].high,
				(unsigned)(nbits8[a].low>>32), (unsigned)(nbits8[a].low&0xFFFFFFFF) );
			if( a % 3 == 0 ){ cout << '\t'; }
			cout << str;
			if( a % 3 == 2 ){ cout << '\n'; }
		}
		cout << "};\n\n";

		cout << "const uint96 ConstData::_nbits24[81] = {\n";
		for( int a = 0 ; a < 81 ; a++ ){
			sprintf( str, "uint96(0x%05x,0x%08x%08xULL), ", nbits24[a].high,
				(unsigned)(nbits24[a].low>>32), (unsigned)(nbits24[a].low&0xFFFFFFFF) );
			if( a % 3 == 0 ){ cout << '\t'; }
			cout << str;
			if( a % 3 == 2 ){ cout << '\n'; }
		}
		cout << "};\n\n";
	}
#endif // DBITS

#if WDISS
	{
		char name[][16] = {
			"lu", "up", "ru",
			"lt",       "rt",
			"ld", "dn", "rd",
		};
		int wdiss[81][_DNUM];
		for( int j = 0 ; j < 9 ; j++ ){
			for( int i = 0 ; i < 9 ; i++ ){
				int a = i + 9 * j;
				wdiss[a][0] = min( i, j );
				wdiss[a][1] = j;
				wdiss[a][2] = min( 8-i, j );
				wdiss[a][3] = i;
				wdiss[a][4] = 8-i;
				wdiss[a][5] = min( i, 8-j );
				wdiss[a][6] = 8-j;
				wdiss[a][7] = min( 8-i, 8-j );
			}
		}
		for( int d = 0 ; d < _DNUM ; d++ ){
			cout << "const int ConstData::wdiss_" << name[d] << "[81] = {\n";
			for( int j = 0 ; j < 9 ; j++ ){
				cout << "\t";
				for( int i = 0 ; i < 9 ; i++ ){
					int a = i + 9 * j;
					cout << ' ' << wdiss[a][d] << ',';
				}
				cout << '\n';
			}
			cout << "};\n\n";
		}
	}
#endif // WDISS

#if AA2NUM
	cout << "const int ConstData::_aa2num[81][81] = {\n";
	for( int i = 0 ; i < 81 ; i++ ){
		cout << "{";
		for( int j = 0 ; j < 81 ; j++ ){
			if( j != 0 && j % 27 == 0 ){
				cout << "\n ";
			}
			int n = ConstData::_diff2num[j/9-i/9][j%9-i%9];
			cout.width( 3 );
			cout << n << ',';
		}
		cout << "},\n";
	}
	cout << "};\n";
#endif
}

double cal_deviation( double ave, double var, int num ){
/************************************************
標準偏差
************************************************/
	double x;
	x = ( var - ave * ave ) * num / ( num - 1 ); // 普遍分散
	return sqrt( x ); // 標準偏差
}

int EstimateValue( ShogiEval* pshogi, MOVE& move ){
/************************************************
着手後の静的評価の変化を推定
************************************************/
	KOMA koma, koma2;
	int value0 = 0;
	int value1 = 0;
	const KOMA* ban;

	// 駒台
	if( move.from & DAI ){
		koma = move.from & ~DAI;

		value1 += pshogi->GetV_KingPieceS( move.to, koma );
		value1 -= pshogi->GetV_KingPieceG( move.to, koma );
	}
	// 盤上
	else{
		ban = pshogi->GetBan();
		koma = ban[move.from];

		// 玉以外
		if( koma != SOU && koma != GOU ){
			// 移動元
			value1 -= pshogi->GetV_KingPieceS( move.from, koma );
			value1 += pshogi->GetV_KingPieceG( move.from, koma );

			// 駒を成る場合
			if( move.nari ){
				value0 -= pshogi->GetV_BAN( koma );
				koma |= NARI;
				value0 += pshogi->GetV_BAN( koma );
			}

			// 移動先
			value1 += pshogi->GetV_KingPieceS( move.to, koma );
			value1 -= pshogi->GetV_KingPieceG( move.to, koma );
		}

		// 駒を取る場合
		if( ( koma2 = ban[move.to] ) ){
			value0 -= pshogi->GetV_BAN2( koma2 );
			value1 -= pshogi->GetV_KingPieceS( move.to, koma2 );
			value1 += pshogi->GetV_KingPieceG( move.to, koma2 );
		}
	}

	if( pshogi->GetSengo() == SENTE ){
		return  ( value0 + ( value1 / (int)EV_SCALE ) );
	}
	else{
		return -( value0 - ( value1 / (int)EV_SCALE ) );
	}
}

bool ExperimentKing( const char* directory ){
/************************************************
[玉/玉以外の駒]の移動による評価値の変動
************************************************/
	ShogiEval shogi;
	FileList flist;
	string fname;
	int num;
	int cnt = 0;

	double ave_king  = 0.0;
	double ave_nking = 0.0;
	double dev_king  = 0.0;
	double dev_nking = 0.0;
	int num_king     = 0;
	int num_nking    = 0;
	int value_p, value;
	double x;
	MOVE move;

	cerr << "Number of CSA files:" << ( num = flist.Enumerate( directory, "csa" ) ) << '\n';

	while( flist.Pop( fname ) ){
		Progress::Print( cnt * 100 / num );
		cnt++;

		if( 0 == shogi.InputFileCSA( fname.c_str() ) ){
			continue;
		}

		while( shogi.GoBack() )
			;

		shogi.GetNextMove( move );
		value_p = shogi.GetValue() + EstimateValue( &shogi, move );
		while( shogi.GoNext() ){
			value = shogi.GetValue();
			x = (double)abs( value - value_p );

			if( shogi.IsKingMove() ){
				ave_king += x;
				dev_king += x * x;
				num_king++;
			}
			else{
				ave_nking += x;
				dev_nking += x * x;
				num_nking++;
			}

			value_p = value;
		}
	}

	Progress::Print( 100 );
	cerr << '\n';

	ave_king /= num_king;
	dev_king /= num_king;
	dev_king = cal_deviation( ave_king, dev_king, num_king );

	ave_nking /= num_nking;
	dev_nking /= num_nking;
	dev_nking = cal_deviation( ave_nking, dev_nking, num_nking );

	cout << "     \t" << "number"  << '\t' << "average" << '\t' << "deviation" << '\n';
	cout << "king \t" << num_king  << '\t' << ave_king  << '\t' << dev_king    << '\n';
	cout << "other\t" << num_nking << '\t' << ave_nking << '\t' << dev_nking   << '\n';

	return true;
}

bool ExperimentPosEval( const char* directory ){
/************************************************
駒割り以外の評価値の変動
************************************************/
	ShogiEval shogi;
	FileList flist;
	string fname;
	int num;
	int cnt = 0;

	ofstream fout( "posEval.dat" );
	bool is_first = true;

	static const int depth = 10;
	double abs_ave[depth] = { 0.0 };
	double abs_dev[depth] = { 0.0 };
	double abs_var[depth] = { 0.0 };
	double sgn_ave[depth] = { 0.0 };
	double sgn_dev[depth] = { 0.0 };
	double sgn_var[depth] = { 0.0 };
	int num_pos= 0;

	int diff_max = INT_MIN;
	int diff_min = INT_MAX;

	static const int dist_mgn = 2500;
	static const int dist_max = 2500;
	static const int dist_min = -2500;
	int dist[5001] = { 0 };

	int i;

	cerr << "Number of CSA files:" << ( num = flist.Enumerate( directory, "csa" ) ) << '\n';

	while( flist.Pop( fname ) ){
		Progress::Print( cnt * 100 / num );
		cnt++;

		if( 0 == shogi.InputFileCSA( fname.c_str() ) ){
			continue;
		}

		while( shogi.GoBack() )
			;

		int value_prev[depth] = { 0 };
		int d = 1;
		value_prev[0] = -shogi.GetValue1N();
		while( shogi.GoNext() ){
			int value = shogi.GetValue1N();

			for( i = 0 ; i < d ; i++ ){
				int diff = - value + value_prev[i];

				abs_ave[i] += (double)abs(diff);
				abs_dev[i] += (double)diff*diff;

				sgn_ave[i] += (double)diff;
				sgn_dev[i] += (double)diff*diff;
			}

			for( i = d - 1 ; i >= 0 ; i-- ){
				value_prev[i+1] = -value_prev[i];
				value_prev[i] = -value;
			}
			d = min( d+1, depth );

			diff_max = max( diff_max, value );
			diff_min = min( diff_min, value );

			int diff = value + value_prev[0];

			if( is_first ){ is_first = false; }
			else          { fout << ", "; }
			fout << diff;

			if( diff > dist_max ){ diff = dist_max; }
			if( diff < dist_min ){ diff = dist_min; }
			dist[diff+dist_mgn]++;

			num_pos++;
		}
	}

	Progress::Print( 100 );
	cerr << '\n';

	cout.setf(ios::fixed, ios::floatfield); // %f
	cout.setf(ios::showpoint);
	cout.precision( 4 );
	for( i = 0 ; i < depth ; i++ ){
		abs_ave[i] /= num_pos;
		abs_dev[i] /= num_pos;
		abs_dev[i] = cal_deviation( abs_ave[i], abs_dev[i], num_pos );
		abs_var[i] = abs_dev[i] * abs_dev[i];

		cout << "depth : " << (i+1) << '\n';
		cout << "    \t" << "number" << '\t' << "average"  << '\t' << "deviation" << '\t' << "variance" << '\n';
		cout << "abs \t" << num_pos  << '\t' << abs_ave[i] << '\t' << abs_dev[i]  << '\t' << abs_var[i] << '\t' << (abs_var[i]*abs_var[i]) << '\n';

		sgn_ave[i] /= num_pos;
		sgn_dev[i] /= num_pos;
		sgn_dev[i] = cal_deviation( sgn_ave[i], sgn_dev[i], num_pos );
		sgn_var[i] = sgn_dev[i] * sgn_dev[i];

		cout << "depth : " << (i+1) << '\n';
		cout << "    \t" << "number" << '\t' << "average"  << '\t' << "deviation" << '\t' << "variance" << '\n';
		cout << "sgn \t" << num_pos  << '\t' << sgn_ave[i] << '\t' << sgn_dev[i]  << '\t' << sgn_var[i] << '\t' << (abs_var[i]*abs_var[i]) << '\n';
	}

	cout << "max : " << diff_max << '\n';
	cout << "min : " << diff_min << '\n';

	ofstream fout2( "posEval.csv" );
	for( i = dist_min ; i <= dist_max ; i++ ){
		fout2 << i << ',' << dist[i+dist_mgn] << '\n';
	}

	return true;
}

bool ExperimentPiecePos( const char* directory ){
/************************************************
各駒毎の各マスに対する存在比率
************************************************/
	ShogiEval shogi;
	FileList flist;
	string fname;
	int num;
	int cnt = 0;

	unsigned num_pos = 0;

	unsigned (*posS)[81];
	unsigned (*posG)[81];

	static char s_koma[][3] = {
		"FU", "KY", "KE", "GI", "KI", "KA", "HI", "OU",
		"TO", "NY", "NK", "NG", "  ", "UM", "RY",
	};

	posS = new unsigned[RY][81];
	posG = new unsigned[RY][81];
	memset( posS, 0, sizeof(unsigned[RY][81]) );
	memset( posG, 0, sizeof(unsigned[RY][81]) );

	cerr << "Number of CSA files:" << ( num = flist.Enumerate( directory, "csa" ) ) << '\n';
	if( num == 0 ){ return false; }

	while( flist.Pop( fname ) ){
		Progress::Print( cnt * 100 / num );
		cnt++;

		if( 0 == shogi.InputFileCSA( fname.c_str() ) ){
			continue;
		}

		while( shogi.GoBack() )
			;

		while( shogi.GoNext() ){
			const KOMA* ban = shogi.GetBan();
			for( int i = 0x10 ; i <= 0x90 ; i += 0x10 ){
				for( int j = 0x01 ; j <= 0x09 ; j += 0x01 ){
					int addr = i + j;
					KOMA koma = ban[addr];
					assert( ConstData::_addr[addr] >= 0 );
					assert( ConstData::_addr[addr] < 81 );
					if( koma != EMPTY ){
						if( koma & SENTE ){
							assert( koma-SFU < RY );
							assert( posS[koma-SFU][ConstData::_addr[addr]] <= num_pos );
							posS[koma-SFU][ConstData::_addr[addr]]++;
						}
						else if( koma & GOTE ){
							assert( koma-GFU < RY );
							assert( posG[koma-GFU][ConstData::_addr[addr]] <= num_pos );
							posG[koma-GFU][ConstData::_addr[addr]]++;
						}
					}
				}
			}
			num_pos++;
		}
	}

	//cout.setf(ios::scientific, ios::floatfield); // %e
	cout.setf(ios::fixed, ios::floatfield); // %f
	cout.setf(ios::showpoint);
	cout.precision( 4 );
	for( KOMA koma = FU ; koma <= RY ; koma++ ){
		int sum;
		cout << "============================= " << s_koma[koma-FU] << " =============================" << '\n';
		cout << "SENTE\n";
		sum = 0;
		for( int k = 0 ; k < 81 ; k++ ){
			sum += posS[koma-FU][k];
		}
		for( int j = 1 ; j <= 9 ; j++ ){
			for( int i = 9 ; i >= 1 ; i-- ){
				cout << ((double)posS[koma-FU][i+j*9-10]/sum) << ' ';
			}
			cout << '\n';
		}
		cout << "GOTE\n";
		sum = 0;
		for( int k = 0 ; k < 81 ; k++ ){
			sum += posG[koma-FU][k];
		}
		for( int j = 1 ; j <= 9 ; j++ ){
			for( int i = 9 ; i >= 1 ; i-- ){
				cout << ((double)posG[koma-FU][i+j*9-10]/sum) << ' ';
			}
			cout << '\n';
		}
	}

	Progress::Print( 100 );
	cerr << '\n';

	delete [] posS;
	delete [] posG;

	return true;
}

bool DateTimeTest( const char* ){
/************************************************
DateTimeクラスのテスト
************************************************/
	DateTime dt;
	unsigned d, t;
	char str[16];

	for( int i = 0 ; i <= 30 ; i+=1 ){
		dt.set( 1948, 5, 1, 0, 0, 0 );
		dt.year += i;
	
		dt.toString( str ); cout << str << ' ';
		d = dt.export_date(); t = dt.export_time();
		dt.import( d, t );
		dt.toString( str ); cout << str << '\n';
	}

	return true;
}

bool InputTest( const char* directory ){
/************************************************
棋譜の読み込みテスト
************************************************/
	FileList flist;
	string fname;
	Shogi shogi;
	KIFU_INFO kinfo;
	AvailableTime atime;

	cout << "Number of CSA files," << flist.Enumerate( directory, "csa" ) << '\n';
	cout << "\
file name,event,sente,gote,year s,month s,day s,year e,month e,day e,\
time limit,used time s,remaining time s,used time g,remaining time g,number of moves,\
\n";
	while( flist.Pop( fname ) ){
		if( 0 == shogi.InputFileCSA( fname.c_str(), &kinfo, &atime ) ){
			continue;
		}

		if( !kinfo.start.isLegal() ){
			continue;
		}

		DateTime dt;
		unsigned d, t;
		d = kinfo.start.export_date();
		t = kinfo.start.export_time();
		dt.import( d, t );

		if( kinfo.start.year  != dt.year  ||
			kinfo.start.month != dt.month ||
			kinfo.start.day   != dt.day   ||
			kinfo.start.hour  != dt.hour  ||
			kinfo.start.min   != dt.min   ||
			kinfo.start.sec   != dt.sec
		){
			char str[16];
			cout << kinfo.start.year << ' ';
			cout << kinfo.start.month << ' ';
			cout << kinfo.start.day << ' ';
			cout << kinfo.start.hour << ' ';
			cout << kinfo.start.min << ' ';
			cout << kinfo.start.sec << ' ';
			cout << '\n';
			kinfo.start.toString( str );
			cout << str << '\n';

			cout << dt.year << ' ';
			cout << dt.month << ' ';
			cout << dt.day << ' ';
			cout << dt.hour << ' ';
			cout << dt.min << ' ';
			cout << dt.sec << ' ';
			cout << '\n';
			dt.toString( str );
			cout << str << '\n';

			exit( 1 );
		}

		cout << fname << ',';
		cout << kinfo.event       << ',';
		cout << kinfo.sente       << ',';
		cout << kinfo.gote        << ',';
		cout << kinfo.start.year  << ',';
		cout << kinfo.start.month << ',';
		cout << kinfo.start.day   << ',';
		cout << kinfo.end.year    << ',';
		cout << kinfo.end.month   << ',';
		cout << kinfo.end.day     << ',';
		cout << atime.GetAvailableTime() << ',';
		cout << atime.GetCumUsedTime( AvailableTime::S ) << ',';
		cout << atime.GetRemainingTime( AvailableTime::S ) << ',';
		cout << atime.GetCumUsedTime( AvailableTime::G ) << ',';
		cout << atime.GetRemainingTime( AvailableTime::G ) << ',';
		cout << atime.GetNumber() << ',';
		cout << '\n';
	}

	return true;
}

bool CopyTest( const char* fname ){
	ShogiEval* pshogi;
	ShogiEval* pshogi2;
	bool ret = true;

	pshogi = new ShogiEval();
	pshogi->InputFileCSA( fname );

	// コピー
#if 0
	pshogi2 = new ShogiEval( *pshogi );
#else

#if 1 // 同じ開始局面
	pshogi2 = new ShogiEval();
#if 1 // 同じ指し手
	{
		int r = gen_rand32() % pshogi->GetTotal();
		MOVE move;
		for( int i = 0 ; i < r ; i++ ){
			pshogi->GetMove( i, move );
			pshogi2->MoveD( move );
		}
	}
#endif
#else // 異なる初期局面
	pshogi2 = new ShogiEval( M2_OCHI );
#endif

#if 1 // 任意の指し手
	{
		int r = gen_rand32() % 150;
		Moves moves;
		for( int i = 0 ; i < r ; i++ ){
			moves.Init();
			pshogi2->GenerateMoves( moves );
			if( moves.GetNumber() == 0 ){ break; }
			pshogi2->MoveD( moves[gen_rand32()%moves.GetNumber()] );
		}
	}
#endif

	pshogi2->QuickCopy( *pshogi );
#endif

	// 確認
	while( ret && pshogi->GoBack() ){
		pshogi2->GoBack();

		if( memcmp( (void*)pshogi->_GetBan(), (void*)pshogi2->_GetBan(), 16*13*sizeof(KOMA) ) != 0 ){
			cout << "error :" << __FILE__ << ' ' << __LINE__ << '\n';
			pshogi->PrintBan();
			pshogi2->PrintBan();
			ret = false;
		}
		else if( memcmp( (void*)pshogi->GetDai(), (void*)pshogi2->GetDai(), 64*sizeof(unsigned) ) != 0 ){
			cout << "error :" << __FILE__ << ' ' << __LINE__ << '\n';
			pshogi->PrintBan();
			pshogi2->PrintBan();
			ret = false;
		}
	}

	while( ret && pshogi->GoNext() ){
		pshogi2->GoNext();

		if( memcmp( (void*)pshogi->_GetBan(), (void*)pshogi2->_GetBan(), 16*13*sizeof(KOMA) ) != 0 ){
			cout << "error :" << __FILE__ << ' ' << __LINE__ << '\n';
			pshogi->PrintBan();
			pshogi2->PrintBan();
			ret = false;
		}
		else if( memcmp( (void*)pshogi->GetDai(), (void*)pshogi2->GetDai(), 64*sizeof(unsigned) ) != 0 ){
			cout << "error :" << __FILE__ << ' ' << __LINE__ << '\n';
			pshogi->PrintBan();
			pshogi2->PrintBan();
			ret = false;
		}
	}

	delete pshogi;
	delete pshogi2;

	return ret;
}

#endif // _SHOGI_DEVELOP_
