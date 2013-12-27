/* match.cpp
 * R.Kubo 2008-2010
 * 将棋の対局を行なうMatchクラスのメンバ
 */

#include <cctype>
#include <fstream>

#include "match.h"

#define PRINT_MOVE			if(pconfig->csa){pshogi->PrintMoveCSA(true);}else{pshogi->PrintMove(true);}
#if POSIX
#define PRINT_BAN			if(pconfig->csa){pshogi->PrintBanCSA(true);}else{pshogi->PrintBan(true);}
#else
#define PRINT_BAN			if(pconfig->csa){pshogi->PrintBanCSA();}else{pshogi->PrintBan();}
#endif

#define FNAME_AGREE			"agreement"

int Match::MatchConsole( MATCH_CONF* pconfig ){
/************************************************
コンソールでの対局
************************************************/
	ShogiEval* pshogi;
	Book* pbook;
	ResignController resign( pconfig->resign );
	MOVE move;
	int cmd = 0;

	THINK_INFO think_info;

	int ret;
	string str = "";
	char buf[256];
	int n0, n1;
	int sen;
	int i, x;

	pshogi = new ShogiEval();
	pbook = new Book();

	// 起動時棋譜読み込み
	if( pconfig->fname != NULL ){
		if( pshogi->InputFileCSA( pconfig->fname ) ){
			if( !pconfig->aquit ){ cmd = 1; } // コマンドモード
		}
		else{
			cerr << "Open Error!..[" << pconfig->fname << ']' << '\n';
		}
	}

#if USE_QUICKCOPY
	PresetTrees( pshogi );
#endif

	// 対局開始
	for( ; ; ){
		PRINT_BAN;

		cout << "* STATIC VALUE = " << pshogi->GetValue() << '\n';

#if INANIWA && 0
		// 稲庭対策テスト
		if( IsInaniwa( pshogi, SENTE ) ){
			cout << "SENTE is Inaniwa!!\n";
		}

		if( IsInaniwa( pshogi, GOTE ) ){
			cout << "GOTE  is Inaniwa!!\n";
		}
#endif

		cout.flush();

#ifndef NDEBUG
		// Debug用棋譜出力
		pshogi->OutputFileCSA( "debug.csa" );
#endif

		sen = pshogi->IsRepetition();
		if( sen == 1 ){
			cout << "Repetition!!" << '\n';
			if( pconfig->aquit ){ break;   } // 終了
			else                { cmd = 1; } // コマンドモード
		}
		else if( sen >= 2 ){
			cout << "Checking Repetition!!!" << '\n';
			if( pconfig->aquit ){ break;   } // 終了
			else                { cmd = 1; } // コマンドモード
		}

		if( !cmd &&
			( ( pshogi->GetSengo() == SENTE && pconfig->com_s ) ||
			( pshogi->GetSengo() == GOTE && pconfig->com_g ) ) )
		{
			// COMの手番
			cout << "Computer is thinking..." << '\n';
			cout.flush();

			if( pconfig->node != 0 ){
				SetNodeLimit( pconfig->node );
			}
			SetLimit( pconfig->limit );
			ret = SearchLimit( pshogi, pbook, &move, pconfig->depth,
				&think_info, SEARCH_DISP | SEARCH_EASY );
			UnsetLimit();
			UnsetNodeLimit();

			cout << "Complete!" << '\n';

			if( ret == 0 || resign.bad( think_info ) ){
				// 投了
				cout << "End!" << '\n';
				if( pconfig->aquit ){ break;   } // 終了
				else                { cmd = 1; } // コマンドモード
				goto lab_input;
			}
			else if( !think_info.useBook ){
				// 情報表示
				PrintThinkInfo( &think_info );
			}
		}
		else{
lab_input:
			// USERの手番
			if( cmd )
				cout << "COMMAND >";
			else
				cout << "MOVE >";

			cin.getline( buf, sizeof(buf) );

			// 中断
			if( cin.eof() || 0 == strcmp( buf, "quit" ) ){
				cout << "Quit." << '\n';
				break;
			}

			// 0文字なら前のコマンドを繰り返す。
			if( buf[0] == '\0' ){ }
			else{ str = buf; }

			// 投了
			if( str == "0000" ){
				cout << "End!" << '\n';
				if( pconfig->aquit ){ break;   } // 終了
				else                { cmd = 1; } // コマンドモード
				goto lab_input;
			}
			// コマンドモード
			else if( str == "cmd" ){
				cmd = 1;
				goto lab_input;
			}
			// 対局モード
			else if( str == "play" ){
				cmd = 0;
				continue;
			}
			// 戻る
			else if( str == "prev" || str == "p" ){
				pshogi->GoBack();
				if( !cmd && ( pconfig->com_s || pconfig->com_g ) )
					pshogi->GoBack();
				continue;
			}
			// 進む
			else if( str == "next" || str == "n" ){
				if( pshogi->GoNext() ){
					cout << "* MOVE = ";
					PRINT_MOVE;
					if( !cmd && ( pconfig->com_s || pconfig->com_g ) ){
						if( pshogi->GoNext() )
							cout << "* MOVE = ";
							PRINT_MOVE;
					}
					continue;
				}
				else{
					cout << "There is not next move!" << '\n';
					goto lab_input;
				}
			}
			// 10手戻る
			else if( str == "p10" ){
				for( i = 0 ; i < 10 && pshogi->GoBack() ; i++ )
					;
				continue;
			}
			// 10手進む
			else if( str == "n10" ){
				for( i = 0 ; i < 10 && pshogi->GoNext() ; i++ )
					;
				continue;
			}
			// 初手へ戻る
			else if( str == "top" ){
				while( pshogi->GoBack() )
					;
				continue;
			}
			// 最終手へ進む
			else if( str == "end" ){
				while( pshogi->GoNext() )
					;
				continue;
			}
			// 設定の変更
			else if( str.find( '=' ) != string::npos ){
				// 対局者
				if     ( str == "sente=com"  ){ pconfig->com_s = 1; }
				else if( str == "sente=user" ){ pconfig->com_s = 0; }
				else if( str == "gote=com"   ){ pconfig->com_g = 1; }
				else if( str == "gote=user"  ){ pconfig->com_g = 0; }
				// 探索深さ
				else if( str.substr( 0, 6 ) == "depth=" ){
					x = strtol( str.substr( 6 ).c_str(), NULL, 10 );
					if( x > 0 )
						pconfig->depth = x;
					else
						cerr << "Bad Value!" << '\n';
				}
				// 思考時間
				else if( str.substr( 0, 5 ) == "time=" ){
					x = strtol( str.substr( 5 ).c_str(), NULL, 10 );
					if( x >= 0 )
						pconfig->limit = x;
					else
						cerr << "Bad Value!" << '\n';
				}
				// 置換テーブルのサイズ
				else if( str.substr( 0, 5 ) == "hash=" ){
					x = strtol( str.substr( 5 ).c_str(), NULL, 10 );
					x = HashSize::MBytesToPow2( x, GetNumberOfThreads() );
					if( x >= 1 && x <= 22 ){ Hash::SetSize( x ); }
					else{ cerr << "Bad Value!" << '\n'; }
				}
				// ファイルからの読み込み
				else if( str.substr( 0, 6 ) == "input=" ){
					if( pshogi->InputFileCSA( str.substr( 6 ).c_str() ) ){
						if( !pconfig->aquit ){ cmd = 1;} // コマンドモード
						continue;
					}
					else
						cerr << "Open Error!..[" << str.substr( 6 ) << ']' << '\n';
				}
				// ファイルへの出力
				else if( str.substr( 0, 7 ) == "output=" ){
					if( 0 == pshogi->OutputFileCSA( str.substr( 7 ).c_str() ) )
						cerr << "Open Error!..[" << str.substr( 7 ) << ']' << '\n';
				}
				// 手数を指定して移動
				else if( str.substr( 0, 4 ) == "num=" ){
					x = strtol( str.substr( 4 ).c_str(), NULL, 10 );
					while( pshogi->GetNumber() < x && pshogi->GoNext() )
						;
					while( pshogi->GetNumber() > x && pshogi->GoBack() )
						;
					continue;
				}
				else{
					cerr << "Unknown Command!" << '\n';
					goto lab_input;
				}

				if( cmd ){
					goto lab_input;
				}
				else{
					continue;
				}
			}
			else if( str == "config" ){
				// 設定の表示
				cout << "sente=" << ( pconfig->com_s ? "com" : "user" ) << '\n';
				cout << "gote=" << ( pconfig->com_g ? "com" : "user" ) << '\n';
				cout << "depth=" << pconfig->depth << '\n';
				cout << "time=" << pconfig->limit << '\n';
				cout << "hash=" << HashSize::SizeToMBytes(Hash::Size(),GetNumberOfThreads()) << '\n';
				goto lab_input;
			}
			else if( str == "num" ){
				// 初手からの手数を表示
				cout << pshogi->GetNumber() << '/' << pshogi->GetTotal() << '\n';
				goto lab_input;
			}
			else if( str == "print" ){
				// 棋譜の表示
				pshogi->PrintKifuCSA();
				goto lab_input;
			}
			else if( str == "moves" ){
				// 着手可能手を列挙
				PrintMoves( pshogi );
				goto lab_input;
			}
			else if( str == "captures" ){
				// 駒を取る手を列挙
				PrintCaptures( pshogi );
				goto lab_input;
			}
			else if( str == "nocaptures" ){
				// 駒を取らないを列挙
				PrintNoCaptures( pshogi );
				goto lab_input;
			}
			else if( str == "tactical" ){
				// 駒を取る手を列挙
				PrintTacticalMoves( pshogi );
				goto lab_input;
			}
			else if( str == "check" ){
				// 王手を列挙
				PrintCheck( pshogi );
				goto lab_input;
			}
			else if( str == "book" ){
				// 定跡手を列挙
				PrintBook( pshogi, pbook );
				goto lab_input;
			}
			else if( str == "dist" ){
				// Bookのハッシュ表における分布
				pbook->PrintDistribution();
				goto lab_input;
			}
			else if( str == "mate" ){
				// 詰みを調べる。(Df-Pn) 1手のみ取得
				ret = DfPnSearchLimit( pshogi, &move, NULL, &think_info, 0 );
				PrintThinkInfoMate( &think_info );
				if( ret ){
					cout << "* MOVE = ";
					Shogi::PrintMove( move );
					cout << '\n';
				}
				else{
					cout << "No mate." << '\n';
				}
				goto lab_input;
			}
			else if( str == "checkmate" ){
				// 詰みを調べる。(Df-Pn) 詰み手順を取得
				Moves moves;

				ret = DfPnSearchLimit( pshogi, NULL, &moves, &think_info, 0 );
				PrintThinkInfoMate( &think_info );
				if( ret ){
					cout << "* MOVE = ";
					for( i = 0 ; i < moves.GetNumber() ; i++ ){
						Shogi::PrintMove( moves[i] );
					}
					cout << '\n';
				}
				else{
					cout << "No mate." << '\n';
					for( i = 0 ; i < moves.GetNumber() ; i++ ){
						Shogi::PrintMove( moves[i] );
					}
					cout << '\n';
				}
				goto lab_input;
			}
			else if( str == "mate1" ){
				// 1手詰みを調べる。
				if( Mate1Ply( pshogi, NULL ) ){
					cout << "Mate in 1 ply!!\n";
				}
				else{
					cout << "No mate in 1 ply.\n";
				}
				goto lab_input;
			}
			else if( str == "mate3" ){
				// 3手詰みを調べる。
				if( Mate3Ply( pshogi ) ){
					cout << "Mate in 3 ply!!\n";
				}
				else{
					cout << "No mate in 3 ply.\n";
				}
				goto lab_input;
			}
			else if( str == "help" ){
				// コマンド一覧
				if( cmd )
					cout << "play ";
				else
					cout << "cmd ";
				cout << "prev p next n p10 n10 top end num" << '\n';
				cout << "config help print quit" << '\n';
				cout << "moves captures nocaptures" << '\n';
				cout << "tactical check book mate" << '\n';
				cout << "sente=** gote=** depth=** time=**" << '\n';
				cout << "hash=** input=** output=** num=**" << '\n';

				goto lab_input;
			}
			// ここから隠しコマンド
			else if( str == "tinit" ){
				// トランスポジションテーブルの初期化
				InitHashTable();
				goto lab_input;
			}
			// 指し手入力
			else if( !InputMove( &move, str.c_str(), pshogi->GetSengo() ) ){
				// 入力エラー
				cerr << "Unknown Command!..\"" << str << '\"' << '\n';
				goto lab_input;
			}

			n0 = pshogi->IsLegalMove( move );
			move.nari = 1;
			n1 = pshogi->IsLegalMove( move );
			if( n0 ){
				if( n1 ){
					// 成-不成の選択
					cout << "NARI[Y/n]:";
					cin.getline( buf, sizeof(buf) );
					if( buf[0] != 'Y' && buf[0] != 'y' )
						move.nari = 0;
				}
				else
					move.nari = 0;
			}
			else if( !n1 ){
				// 着手不可
				cerr << "Move Error!" << '\n';
				goto lab_input;
			}
		}

		if( pshogi->MoveD( move ) ){
			cout << "* MOVE = ";
			PRINT_MOVE;
		}
		else{
			// スタックが足りない。
			cerr << "Stack Error!" << '\n';
			if( pconfig->aquit ){ break;   } // 終了
			else                { cmd = 1; } // コマンドモード
		}
	}

	delete pshogi;
	delete pbook;

	return 0;
}

int Match::MatchSelf( MATCH_CONF* pconfig ){
/************************************************
自己対局
************************************************/
	ShogiEval* pshogi1;
	ShogiEval* pshogi2;
	ShogiEval* pshogi;
	Book* pbook;
	int win1 = 0, win2 = 0;
	MOVE move;
	int turn;
	int ret;
	int num;
	int i;

	pshogi1 = new ShogiEval( "evdata" );
	pshogi2 = new ShogiEval( "evdata2" );
	pbook = new Book();

	num = pconfig->repeat;

	// 連続対局
	for( i = 0 ; i < num ; i++ ){
		pshogi1->SetBan( HIRATE );
		pshogi2->SetBan( HIRATE );

#if USE_QUICKCOPY
		PresetTrees( pshogi );
#endif

		turn = ( i + 1 ) % 2; // 先後

		// 対局
		for( ; ; ){
			pshogi = ( turn ? pshogi1 : pshogi2 ); // 手番

			if( pconfig->node != 0 ){
				SetNodeLimit( pconfig->node );
			}
			SetLimit( pconfig->limit );
			ret = SearchLimit( pshogi, pbook, &move,
				pconfig->depth, NULL, SEARCH_NULL );
			UnsetLimit();
			UnsetNodeLimit();

			if( !ret ){
				// 終局
				if( turn ){
					cout << "--- Win" << '\n';
					win2++;
				}
				else{
					cout << "Win ---" << '\n';
					win1++;
				}

				break;
			}

			if( !pshogi1->MoveD( move ) ||
				!pshogi2->MoveD( move ) )
			{
				// バッファが足りない。
				cerr << "Buffer Error!" << '\n';
				break;
			}

			if( pshogi->IsRepetition() ){
				// 千日手
				cout << "Repetition!";
				break;
			}

			turn = !turn;
		}
	}

	cout << "COM1:" << win1 << '\n';
	cout << "COM2:" << win2 << '\n';
	cout << "Draw:" << ( num - win1 - win2 ) << '\n';

	delete pshogi1;
	delete pshogi2;

	return num;
}

int Match::AgreementTest( const char* dir, MATCH_CONF* pconfig ){
/************************************************
棋譜との一致度を調べる。
************************************************/
	ShogiEval* pshogi;
	Timer tm;
	double t;
	MOVE move0;            // 棋譜の指し手
	MOVE move;             // COMの指し手
	int agree = 0;         // 一致数
	int mnum = 0;          // 比較した指し手の総数
	int nocnt = 0;         // エラー数
	THINK_INFO think_info;
	int total;
	int ret;
	int num = 0;
	FileList flist;
	string fname;
	ofstream fout;

	// ファイルの列挙
	if( 0 == flist.Enumerate( dir, "csa" ) ){
		cout << "Error!" << '\n';
		return 0;
	}

	pshogi = new ShogiEval( HIRATE );

	tm.SetTime();

	while( flist.Pop( fname ) ){
		// ファイルを開く。
		if( 0 == pshogi->InputFileCSA( fname.c_str() ) )
			continue;

#if USE_QUICKCOPY
		PresetTrees( pshogi );
#endif

		num++;

		cout << fname << '\n';
		cout << "Number : " << num << '\n';

		// 一致度の調査
		total = pshogi->GetTotal();
		pshogi->GetMove( move0 );
		while( pshogi->GoBack() ){
			if( pconfig->node != 0 ){
				SetNodeLimit( pconfig->node );
			}
			SetLimit( pconfig->limit );
			ret = SearchLimit( pshogi, NULL, &move, pconfig->depth,
				&think_info, SEARCH_NULL );
			UnsetLimit();
			UnsetNodeLimit();

			// 詰みなどは除外
			if( ret && (int)think_info.value > VMIN && (int)think_info.value < VMAX ){
				if( move.IsSame( move0 ) ){
					// 指し手が一致
					agree++;
				}
				mnum++;
			}
			else
				nocnt++;

			pshogi->GetMove( move0 );
			Progress::Print( 100 - pshogi->GetNumber() * 100 / total );
		}

		Progress::Clear();
	}

	t = tm.GetTime();

	cout << "Number of files : " << num << '\n';
	cout << "Number of moves : " << mnum << '\n';
	cout << "Agree           : " << agree << '\n';
	cout << "No agree        : " << mnum - agree << '\n';
	if( mnum != 0 )
		cout << "Agreement rate  : " << (double)agree / mnum << '\n';
	cout << "No count        : " << nocnt << '\n';
	cout << "Time            : " << (int)( t * 1.0e+3 ) << "msec" << '\n';

	// ファイルへの書き出し
	fout.open( FNAME_AGREE, ios::out | ios::app );
	if( fout.fail() ){
		cerr << "Open Error!..[" << FNAME_AGREE << ']' << '\n';
		return 0;
	}

	cout << "Writing to log file..";

	fout << "Number of files : " << num << '\n';
	fout << "Number of moves : " << mnum << '\n';
	fout << "Agree           : " << agree << '\n';
	fout << "No agree        : " << mnum - agree << '\n';
	if( mnum != 0 )
		fout << "Agreement rate  : " << (double)agree / mnum << '\n';
	fout << "No count        : " << nocnt << '\n';
	fout << '\n';

	cout << "OK" << '\n';

	fout.close();

	return mnum;
}

int Match::NextMove( MATCH_CONF* pconfig ){
/************************************************
次の一手
************************************************/
	ShogiEval* pshogi;
	MOVE move;
	int ret;

	pshogi = new ShogiEval();

	// 起動時棋譜読み込み
	if( pconfig->fname != NULL ){
		if( !pshogi->InputFileCSA( pconfig->fname ) ){
			cerr << "Open Error!..[" << pconfig->fname << ']' << '\n';
		}
	}

	if( pconfig->node != 0 ){
		SetNodeLimit( pconfig->node );
	}
	SetLimit( pconfig->limit );
	ret = SearchLimit( pshogi, NULL, &move, pconfig->depth, NULL, SEARCH_NULL );
	UnsetLimit();
	UnsetNodeLimit();

	if( ret ){
		if(pconfig->csa){ pshogi->PrintMoveCSA( move, true ); }
		else            { pshogi->PrintMove( move, true ); }
		return move.Export();
	}
	else{
		return 0;
	}
}

int Match::InputMove( MOVE* move, const char str[], KOMA sengo ){
/************************************************
ユーザの指し手入力
************************************************/
	char s_koma[][3] = { "FU", "KY", "KE", "GI", "KI", "KA", "HI" };
	char s_koma2[][3] = { "fu", "ky", "ke", "gi", "ki", "ka", "hi" };
	int len;
	int x, y;
	int i;

	len = strlen( str );
	if( len < 4 )
		return 0;

	if( !isdigit( str[2] ) || !isdigit( str[3] ) )
		return 0;

	move->from = 0;
	if( isdigit( str[0] ) && isdigit( str[1] ) ){
		x = str[0] - '0';
		y = str[1] - '0';
		move->from = BanAddr( x, y );
	}
	else{
		for( i = 0 ; i < 7 ; i++ ){
			if( ( str[0] == s_koma[i][0] || str[0] == s_koma2[i][0] ) &&
				( str[1] == s_koma[i][1] || str[1] == s_koma2[i][1] ) )
			{
				move->from = i + 1 + sengo + DAI;
				break;
			}
		}
		if( move->from == 0 )
			return 0;
	}

	x = str[2] - '0';
	y = str[3] - '0';
	move->to = BanAddr( x, y );

	move->nari = 0;

	return 1;
}

int Match::PrintMoves( Shogi* pshogi ){
/************************************************
着手可能手の表示
************************************************/
	Moves moves;
	int i;

	pshogi->GenerateMoves( moves );
	for( i = 0 ; i < moves.GetNumber() ; i++ )
		pshogi->PrintMove( moves[i] );
	cout << '\n';
	cout << "There are " << moves.GetNumber() << " moves." << '\n';

	return moves.GetNumber();
}

int Match::PrintCaptures( Shogi* pshogi ){
/************************************************
駒を取る手の表示
************************************************/
	Moves moves;
	int i;

	pshogi->GenerateCaptures( moves );
	for( i = 0 ; i < moves.GetNumber() ; i++ )
		pshogi->PrintMove( moves[i] );
	cout << '\n';
	cout << "There are " << moves.GetNumber() << " moves." << '\n';

	return moves.GetNumber();
}

int Match::PrintNoCaptures( Shogi* pshogi ){
/************************************************
駒を取らない手の表示
************************************************/
	Moves moves;
	int i;

	pshogi->GenerateNoCaptures( moves );
	for( i = 0 ; i < moves.GetNumber() ; i++ )
		pshogi->PrintMove( moves[i] );
	cout << '\n';
	cout << "There are " << moves.GetNumber() << " moves." << '\n';

	return moves.GetNumber();
}

int Match::PrintTacticalMoves( Shogi* pshogi ){
/************************************************
Tacticalな手を表示
************************************************/
	Moves moves;
	int i;

	pshogi->GenerateTacticalMoves( moves );
	for( i = 0 ; i < moves.GetNumber() ; i++ )
		pshogi->PrintMove( moves[i] );
	cout << '\n';
	cout << "There are " << moves.GetNumber() << " moves." << '\n';

	return moves.GetNumber();
}

int Match::PrintCheck( Shogi* pshogi ){
/************************************************
王手を表示
************************************************/
	Moves moves;
	int i;

	pshogi->GenerateCheck( moves );
	for( i = 0 ; i < moves.GetNumber() ; i++ )
		pshogi->PrintMove( moves[i] );
	cout << '\n';
	cout << "There are " << moves.GetNumber() << " moves." << '\n';

	return moves.GetNumber();
}

int Match::PrintBook( Shogi* pshogi, Book* pbook ){
/************************************************
定跡手の表示
************************************************/
	Moves moves;
	int i;

	pbook->GetMoveAll( pshogi, moves );
	for( i = 0 ; i < moves.GetNumber() ; i++ ){
		pshogi->PrintMove( moves[i] );
		cout << moves[i].value << "%, ";
	}
	cout << '\n';
	cout << "There are " << moves.GetNumber() << " moves." << '\n';

	return moves.GetNumber();
}

int Match::PrintBookEval( Shogi* pshogi, Book* pbook ){
/************************************************
定跡手の表示(評価値バージョン)
************************************************/
	Moves moves;
	int i;

	pbook->GetMoveAll( pshogi, moves, Book::TYPE_EVAL );
	for( i = 0 ; i < moves.GetNumber() ; i++ ){
		pshogi->PrintMove( moves[i] );
		cout << moves[i].value << ", ";
	}
	cout << '\n';
	cout << "There are " << moves.GetNumber() << " moves." << '\n';

	return moves.GetNumber();
}

/************************************************
情報表示
************************************************/
void Match::PrintThinkInfo( THINK_INFO* info ){
	cout << "* VALUE             = " << (int)info->value << '\n';
	cout << "* NODE              = " << (int)info->node << '\n';
	cout << "* TIME              = " << (int)info->msec << "msec" << '\n';
	cout << "* NPS               = " << (int)info->nps << "nps" << '\n';
	cout << "* HASH ENTRY        = " << (int)info->hash_entry << '\n';
	cout << "* HASH COLLISION    = " << (int)info->hash_collision << '\n';
	cout << "* HASH PRUNING      = " << (int)info->hash_cut << '\n';
#if RAZORING
	cout << "* RAZORING          = " << (int)info->razoring << '\n';
#endif
#if STATIC_NULL_MOVE
	cout << "* STATIC NULL MOVE  = " << (int)info->s_nullmove_cut << '\n';
#endif
	cout << "* NULL MOVE PRUNING = " << (int)info->nullmove_cut << '\n';
	cout << "* FUTILITY PRUNING  = " << (int)info->futility_cut << '\n';
	cout << "* EXT FUT PRUNING   = " << (int)info->e_futility_cut << '\n';
#if MOVE_COUNT_PRUNING
	cout << "* MOVE COUNT PRUNE. = " << (int)info->move_count_cut << '\n';
#endif
#if SINGULAR_EXTENSION
	cout << "* SINGULAR EXT.     = " << (int)info->singular_ext << '\n';
#endif
	cout << "* SPLIT             = " << (int)info->split << '\n';
	cout << "* SPLIT FAIL        = " << (int)info->split_failed << '\n';
	cout << "* UNUSED TREE       = " << (int)info->unused_tree << '\n';
}

/************************************************
情報表示(詰み探索用)
************************************************/
void Match::PrintThinkInfoMate( THINK_INFO* info ){
	cout << "* VALUE = " << (int)info->value << '\n';
	cout << "* NODE  = " << (int)info->node << '\n';
	cout << "* TIME  = " << (int)info->msec << "msec" << '\n';
	cout << "* NPS   = " << (int)info->nps << "nps" << '\n';
}
