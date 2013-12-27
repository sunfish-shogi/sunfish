/* csa.cpp
 * R.Kubo 2010-2012
 * CSAプロトコルによる対局
 */

#include <fstream>
#include <sstream>
#include <typeinfo>

#if WIN32
#include <windows.h>
#include <shlwapi.h>
#else
#include <unistd.h>
#endif

#include "shogi.h"

#include "match.h"

MatchCSA::MatchCSA( int thread_num ){
/************************************************
コンストラクタ
************************************************/
	next = buf;
	buf[0] = '\0';
	rflag = 0U;

	cons_num = 1;
	deviation = 0.0;
	limit_node = 0;
	consultationType = CONS_SINGLE;
	pcons = new ConsultationSingle( thread_num );

	pshogi = new ShogiEval();
	pbook = new Book();

	host[0] = '\0';
	user[0] = '\0';
	pass[0] = '\0';
	depth = DEF_DEPTH;
	limit = 0;
	repeat = 1;
	//timeout = DEF_TIMEOUT;
	floodgate = 0;
	kifu_dir[0] = '\0';
	mstrS[0] = '\0';
	mstrG[0] = '\0';

#ifdef POSIX
	// keep-alive 2011 04/09
	keepalive = 0;
	keepidle = 7200;
	keepintvl = 75;
	keepcnt = 3;
#endif

	// Mutex
#ifdef WIN32
	hMutex = CreateMutex( NULL, TRUE, NULL );
#else
	pthread_mutex_init( &mutex, NULL );
#endif

	enemyTurnSearch = true;
}

MatchCSA::~MatchCSA(){
/************************************************
デストラクタ
************************************************/
	delete pshogi;
	delete pbook;

	// Mutexの破棄
#ifdef WIN32
	CloseHandle( hMutex );
#else
	// Linuxでは必要なし
	//pthread_mutex_destroy( &mutex );
#endif
}

int MatchCSA::StopFlag(){
/************************************************
連続対局の中断
************************************************/
	ifstream fin;
	char line[1024];
	int ret = 0;

	// ファイルを開く。
	fin.open( FNAME_NCONF );

	// ファイルがあるか。
	if( fin.fail() ){
		cerr << "Fail!..[" << FNAME_NCONF << ']' << '\n';
		return 0;
	}

	while( 1 ){
		fin.getline( line, sizeof(line) );
		if( fin.fail() || fin.eof() )
			break;

		if( String::WildCard( line, "stop=true" ) ){
			ret = 1;
		}
	}

	if( fin.fail() )
		fin.clear();

	// ファイルを閉じる。
	fin.close();

	return ret;
}

int MatchCSA::Timer(){
/************************************************
接続開始時刻を待つ。
************************************************/
	time_t t;
	vector<string>::iterator p;

	// 打ち切りフラグ
	if( StopFlag() ){ return 0; }

	// 指定がなければ直ちに
	if( timer.size() == 0 ){
		return 1;
	}

	while( 1 ){
		// 時刻をチェック
		t = time( NULL );
		for( p = timer.begin() ; p != timer.end() ; p++ ){
			MutexLock(); // ctimeはスレッドセーフじゃない!!
			if( String::WildCard( ctime( &t ), (*p).c_str() ) ){
				MutexUnlock();
				cout << "login schedule=\"" << (*p) << "\"\n";
				return 1;
			}
			MutexUnlock();
		}

		// 0.5秒スリープ
#ifdef WIN32
		Sleep( 500 );
#else
		usleep( 500 * 1000 );
#endif

		// 打ち切りフラグ
		if( StopFlag() ){ return 0; }
	}
}

int MatchCSA::LoadConfig(){
/************************************************
設定の読み込み
************************************************/
	ifstream fin;
	char line[1024];
	int thread_num = pcons->GetNumberOfThreads();
	int l = 0;
	int hash_mbytes = -1;

	// ファイルを開く。
	fin.open( FNAME_NCONF );

	// ファイルがあるか。
	if( fin.fail() ){
		cerr << "Fail!..[" << FNAME_NCONF << ']' << '\n';
		return 0;
	}

	while( 1 ){
		fin.getline( line, sizeof(line) );
		if( fin.fail() || fin.eof() ){ break; }

		l++;

		if( line[0] == '#' || line[0] == '\0' ){
			;
		}
		else if( String::WildCard( line, "stop=*" ) ){
			;
		}
		else if( String::WildCard( line, "host=*" ) ){
			strcpy( host, String::GetSecondToken( line, '=' ) );
		}
		else if( String::WildCard( line, "user=*" ) ){
			strcpy( user, String::GetSecondToken( line, '=' ) );
		}
		else if( String::WildCard( line, "pass=*" ) ){
			strcpy( pass, String::GetSecondToken( line, '=' ) );
		}
		else if( String::WildCard( line, "depth=*" ) ){
			depth = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "limit=*" ) ){
			limit = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "node=*" ) ){
			limit_node = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "resign=*" ) ){
			if( !resign.set_limit( strtol( String::GetSecondToken( line, '=' ), NULL, 10 ) ) ){
				cerr << "resign value is too low!\n";
			}
		}
		else if( String::WildCard( line, "hash=*" ) ){
			hash_mbytes = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "enemy=*" ) ){
			enemyTurnSearch = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "constype=*" ) ){
			switch( String::GetSecondToken( line, '=' )[0] ){
			case 'g':
				consultationType = CONS_SINGLE;
				if( typeid( *pcons ) != typeid( ConsultationSingle ) ){
					delete pcons;
					pcons = new ConsultationSingle( thread_num );
					cout << "Consultation : Single Mode\n";
				}
				break;
			case 's':
				consultationType = CONS_SERIAL;
				if( typeid( *pcons ) != typeid( ConsultationSerial ) ){
					delete pcons;
					pcons = new ConsultationSerial( cons_num, thread_num );
					cout << "Consultation : Serial Mode\n";
				}
				break;
#if 0
			case 'p':
				consultationType = CONS_PARALLEL;
				if( typeid( *pcons ) != typeid( ConsultationParallel ) ){
					delete pcons;
					pcons = new ConsultationParallel( cons_num, thread_num );
					cout << "Consultation : Parallel Mode\n";
				}
				break;
#endif
			}
		}
		else if( String::WildCard( line, "consnum=*" ) ){
			cons_num = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
			pcons->SetNumberOfSearchers( cons_num );
		}
		else if( String::WildCard( line, "threads=*" ) ){
			thread_num = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
			pcons->SetNumberOfThreads( thread_num );
		}
		else if( String::WildCard( line, "deviation=*" ) ){
			deviation = strtod( String::GetSecondToken( line, '=' ), NULL );
		}
		else if( String::WildCard( line, "repeat=*" ) ){
			repeat = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		//else if( String::WildCard( line, "timeout=*" ) ){
		//	timeout = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		//}
		else if( String::WildCard( line, "timer=*" ) ){
			timer.push_back( string( String::GetSecondToken( line, '=' ) ) );
		}
		else if( String::WildCard( line, "floodgate=*" ) ){
			floodgate = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "kifu=*" ) ){
			int len;
			strcpy( kifu_dir, String::GetSecondToken( line, '=' ) );
			len = strlen( kifu_dir );
			if( len > 0 && ( kifu_dir[len-1] == '/'
#ifdef WIN32
				|| kifu_dir[len-1] == '\\'
#endif
				) )
			{
				kifu_dir[len-1] = '\0';
			}
		}
#ifdef POSIX
		else if( String::WildCard( line, "keepalive=*" ) ){
			keepalive = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "keepidle=*" ) ){
			keepidle = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "keepintvl=*" ) ){
			keepintvl = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "keepcnt=*" ) ){
			keepcnt = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
#endif
		else{
			cerr << "Error in " << FNAME_NCONF << " at line " << l << ":" << line << '\n';
		}
	}

	if( fin.fail() )
		fin.clear();

	// ファイルを閉じる。
	fin.close();

	// ハッシュ表のサイズ変更
	if( hash_mbytes != -1 ){
		Hash::SetSize( HashSize::MBytesToPow2( hash_mbytes, thread_num, consultationType, cons_num ) );
	}

	return 1;
}

void MatchCSA::SetResult( GAME_RESULT res ){
/************************************************
終局結果
************************************************/
	info.result = res;
	if( rflag & MATCH_CSA_JISHOGI  ){ info.result |= RES_DRAW; }
	if( rflag & MATCH_CSA_WIN      ){ info.result |= RES_WIN;  }
	if( rflag & MATCH_CSA_LOSE     ){ info.result |= RES_LOSE; }
	if( rflag & MATCH_CSA_SENNICHI ){ info.result |= RES_REP;  }
	if( rflag & MATCH_CSA_OUTE_SEN ){ info.result |= RES_REP;  }
	if( rflag & MATCH_CSA_TIME_UP  ){ info.result |= RES_TIME; }
	if( rflag & MATCH_CSA_ILLEGAL  ){ info.result |= RES_ILL;  }
	if( rflag & MATCH_CSA_ERROR    ){ info.result |= RES_ERR;  }
}

int MatchCSA::WriteResult( int itr ){
/************************************************
結果の出力
************************************************/
	ofstream fout;
	ostringstream str;
	int succeed = 0;
	time_t t;
	unsigned i;

	// 棋譜の保存
	if( info.game_name[0] != '\0' ){
		// Game IDをファイル名にして保存してみる。
		str << kifu_dir << '/' << info.game_name << ".csa";
		for( i = 2 ; ; i++ ){
			// ファイルの存在チェック
#if WIN32
			if( FALSE == PathFileExists( str.str().c_str() ) ){ break; }
#else
			if( 0 != access( str.str().c_str(), F_OK ) ){ break; }
#endif

			// ファイルが存在したらファイル名決め直し
			str.str( "" );
			str << kifu_dir << '/' << info.game_name << '-' << i << ".csa";
		}
		// 保存
		succeed = OutputFileCSA( str.str().c_str() );
	}

	if( succeed == 0 ){
		// 保存に失敗したら日時をファイル名にして保存する。
		str.str( "" );
		time( &t );
		MutexLock(); // ctimeはスレッドセーフじゃない!!
		str << kifu_dir << '/' << ctime( &t ) << ".csa";
		MutexUnlock();

		// 保存
		succeed = OutputFileCSA( str.str().c_str() );
	}

	// 対局情報の記録
	fout.open( FNAME_CSALOG, ios::out | ios::app );
	if( fout.fail() ){
		cerr << "Open Error!..[" << FNAME_CSALOG << ']' << '\n';
		return 0;
	}

	if( itr == 0 ){
		fout << "Game name,Black,White,Draw,Win,Lose,Repetition,Time up,Illegal,Error," << '\n';
	}

	fout << info.game_name << ',';
	fout << info.sente << ',';
	fout << info.gote << ',';
	fout << ( ( info.result & RES_DRAW ) ? "1," : "0," );
	fout << ( ( info.result & RES_WIN  ) ? "1," : "0," );
	fout << ( ( info.result & RES_LOSE ) ? "1," : "0," );
	fout << ( ( info.result & RES_REP  ) ? "1," : "0," );
	fout << ( ( info.result & RES_TIME ) ? "1," : "0," );
	fout << ( ( info.result & RES_ILL  ) ? "1," : "0," );
	fout << ( ( info.result & RES_ERR  ) ? "1," : "0," );
	fout << '\n';

	fout.close();

	return 1;
}

int MatchCSA::MatchNetwork(){
/************************************************
通信連続対局
************************************************/
#ifdef WIN32
	HANDLE hTh_r;       // レシーブスレッド
	HANDLE hTh_e;       // 相手番思考スレッド
#else
	pthread_t tid_r;    // レシーブスレッド
	pthread_t tid_e;    // 相手番思考スレッド
#endif
	// スレッドを沸かしたかどうか
	bool bTh_r = false; // レシーブスレッド
	bool bTh_e = false; // 相手番思考スレッド

	bool started;

	int i;

	LoadConfig();

	for( i = 0 ; i < repeat ; i++ ){
		started = false;

		// 定刻まで待機
		cout << "Waiting.." << '\n';
		if( !Timer() ){
			// 打ち切り
			cout << "Stop!" << '\n';
			return 1;
		}

		// フラグを下げる。
		rflag = 0U;

		// 接続
		if( !Connect() ){
			return 0;
		}

		cout << "Connected." << '\n';

		// 受信スレッドを沸かす。
#ifdef WIN32
		if( INVALID_HANDLE_VALUE != ( hTh_r = (HANDLE)_beginthreadex( NULL, 0, ReceiveThread, (LPVOID)this, 0, NULL ) ) ){
#else
		if( 0 == pthread_create( &tid_r, NULL, ReceiveThread, (void*)this ) ){
#endif
			bTh_r = true;
		}

		// ログイン
		if( !Login() ){
			Disconnect();
			return 0;
		}

		cout << "Login OK." << '\n';

		// 対局条件取得
		if( !GetRecFlag( MATCH_CSA_SUMMARY ) ){
			goto lab_logout;
		}

#if USE_QUICKCOPY
		pcons->PresetTrees( pshogi );
#endif

		// 対局開始
		if( !SendAgree() ){
			goto lab_logout;
		}

		started = true;

		MutexLock();
		cout << "Start." << '\n';
		MutexUnlock();

		// 開始時刻
		start.setNow();
		end = start;

		while( 1 ){
			// 局面の表示
#if POSIX
			pshogi->PrintBan( true );
#else
			pshogi->PrintBan();
#endif

#ifndef NDEBUG
			// Debug用棋譜出力
			OutputFileCSA( "debug.csa" );
#endif

			if( pshogi->GetSengo() == info.sengo ){
				// 自分の手番
				THINK_INFO think_info;
				MOVE move;
				int ret;
				int l;

				memset( &think_info, 0, sizeof(THINK_INFO) );

				// 思考時間
				l = limit;

				// 時間制限あり
				if( atime.IsValidSetting() ){
					// 秒読みあり
					if( atime.GetReadingOffSeconds() > 0 ){
						// 秒読みも含めた残り時間を超えようとしている場合
						if( atime.GetUsableTime() <= (double)limit ){
							l = (int)( atime.GetUsableTime() - 1.0 );
						}
					}
					// 切れ負け
					else{
						// 持ち時間の20分の1を上限
						if( atime.GetRemainingTime() < limit * 20.0 ){
							l = (int)( atime.GetRemainingTime() / 20.0 ) + 1;
						}
					}
				}
				else{
					cout << "unlimited.\n";
				}

				// 乱数合議用正規乱数の偏差
				pcons->SetDeviation( deviation );

				// 探索
				if( limit_node != U64(0) ){
					pcons->SetNodeLimit( limit_node );
				}
				pcons->SetLimit( l );
				ret = pcons->SearchLimit( pshogi, pbook, &move, depth,
					&think_info, Think::SEARCH_DISP | Think::SEARCH_EASY );
				pcons->UnsetLimit();
				if( limit_node != U64(0) ){ // UnsetNodeLimitは無条件で読んでも良い。
					pcons->UnsetNodeLimit();
				}

#ifndef NDEBUG
				Debug::Print( "split=%u, split_failed=%u, unused_tree=%u\n",
					(unsigned)think_info.split, (unsigned)think_info.split_failed,
					(unsigned)think_info.unused_tree );
#endif

				// 時間切れなど
				if( rflag & MATCH_CSA_MASK_END ){
					SetResult();
					goto lab_logout;
				}

				// 投了
				if( ret == 0 || resign.bad( think_info ) ){
					SendToryo();
					SetResult( RES_LOSE );
					goto lab_logout;
				}

				if( pshogi->MoveD( move ) ){
					// 指し手の送信
					if( !SendMove( &think_info ) ){
						SetResult( RES_ERR );
						goto lab_logout;
					}
				}
				else{
					// 投了
					SendToryo();
					SetResult( RES_LOSE );
					goto lab_logout;
				}
			}
			else{
				// 相手の手番
				unsigned rec;

				// 相手番思考を開始
				// 思考スレッドを沸かす。
#if 1
				eflag = 0;
				if( enemyTurnSearch &&
#ifdef WIN32
					INVALID_HANDLE_VALUE != ( hTh_e = (HANDLE)_beginthreadex( NULL, 0, EnemyTurnSearchThread, (LPVOID)this, 0, NULL ) )
#else
					0 == pthread_create( &tid_e, NULL, EnemyTurnSearchThread, (void*)this )
#endif
				){
					bTh_e = true;
				}
#endif

				// 相手の手を待つ。
				rec = GetRecFlag( MATCH_CSA_MASK_MOVE | MATCH_CSA_MASK_END );

				// 相手番思考を終了
				// スレッドの終了を待つ。
#if 1
				eflag = 1;
				if( bTh_e ){
#ifdef WIN32
					WaitForSingleObject( hTh_e, INFINITE );
					CloseHandle( hTh_e );
#else
					pthread_join( tid_e, NULL );
#endif
					bTh_e = false;
				}
#endif

				if( rec & MATCH_CSA_MASK_MOVE ){
					// 相手の指し手の読み込み
					if( pshogi->InputLineCSA( MoveStrEnemy() ) ){
						InputTime( MoveStrEnemy() ); // 消費時間
					}
					else{
						// 反則手?
						if( GetRecFlag( MATCH_CSA_MASK_END ) ){
							// 反則などで終了
							goto lab_logout;
						}
						else{
							// 原因不明のトラブル(?)
							goto lab_logout;
						}
					}
				}
				else{
					// 終局 or 中断
					goto lab_logout;
				}
			}

			// 指し手の表示
			pshogi->PrintMove( true );
		}

lab_logout:
		// 相手番思考を終了
		eflag = 1;
		if( bTh_e ){
#ifdef WIN32
			WaitForSingleObject( hTh_e, INFINITE );
			CloseHandle( hTh_e );
#else
			pthread_join( tid_e, NULL );
#endif
			bTh_e = false;
		}

		// ログアウト
		Logout();

		cout << "Logout." << '\n';

		// 切断
		Disconnect();

		cout << "Disconnected." << '\n';

		// スレッドの終了を待つ。
		if( bTh_r ){
#ifdef WIN32
			WaitForSingleObject( hTh_r, INFINITE );
			CloseHandle( hTh_r );
#else
			pthread_join( tid_r, NULL );
#endif
			bTh_r = false;
		}

		// 終了時刻
		end.setNow();

		// 結果の記録
		if( started ){
			WriteResult( i );
		}
	}

	return 1;
}

#ifdef WIN32
unsigned __stdcall MatchCSA::EnemyTurnSearchThread( LPVOID p ){
#else
void* MatchCSA::EnemyTurnSearchThread( void* p ){
#endif
/************************************************
相手番で探索をするスレッド
************************************************/
	MatchCSA* pmatchCSA = (MatchCSA*)p;
	THINK_INFO info;

	pmatchCSA->pcons->EnemyTurnSearch( pmatchCSA->pshogi, NULL,
		MAX_DEPTH, &info, &pmatchCSA->eflag, Think::SEARCH_DISP );

#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
}

int MatchCSA::ReceiveMessage(){
/************************************************
メッセージを受け取る。
************************************************/
	char* p;

	// バッファにデータがなくなったら受信
	if( next[0] == '\0' ){
		memset( buf, 0, sizeof(buf) );
		if( 0 >= recv( sock, buf, sizeof(buf) - 1, 0 ) )
			return 0;
		next = buf;
	}

	// 一行ずつ取り出す。
	p = next;
	while( 1 ){
		if( p[0] == '\0' ){
			strcpy( line , next );
			next = p;
			break;
		}
		else if( p[0] == '\n' ){
			p[0] = '\0';
			p++;
			strcpy( line , next );
			next = p;
			break;
		}
		p++;
	}

	return 1;
}

int MatchCSA::SendMessage( const char* str, const char* disp ){
/************************************************
メッセージを送る。
************************************************/
	int ret;

	if( -1 != ( ret = send( sock, str, strlen( str ), 0 ) ) ){
		// 送信に成功した場合
		MutexLock();
#if POSIX
		cout << "\x1b[34m";
#endif
		cout << '<' << ( disp == NULL ? str : disp );
#if POSIX
		cout << "\x1b[39m";
		cout.flush();
#endif
		MutexUnlock();
	}

	return ret;
}

int MatchCSA::Connect(){
/************************************************
接続
************************************************/
	struct hostent* he;
	struct sockaddr_in sin;
#ifdef WIN32
	WORD    wVersionRequested = MAKEWORD(2,2);
	WSADATA WSAData;

	WSAStartup( wVersionRequested, &WSAData );
#endif

	if( NULL == ( he = gethostbyname( host ) ) ){
		cerr << "No Hosts (hostname=" << host << ")" << '\n';
		return 0;
	}

	cout << "Got the host." << '\n';

	if( -1 == ( sock = socket( AF_INET, SOCK_STREAM, 0 ) ) ){
		cerr << "Cannot get a socket! (" << errno << ')' << '\n';
		return 0;
	}

	cout << "Opened the socket" << '\n';

#ifdef POSIX
	// keep-alive 2011 04/09
	cout << "setting SO_KEEPALIVE=" << keepalive << '\n';
	if( 0 != setsockopt( sock, SOL_SOCKET , SO_KEEPALIVE , (void*)&keepalive , sizeof(keepalive) ) ){
		cerr << "error(" << errno << ")\n";
	}

	cout << "setting SO_KEEPIDLE=" << keepidle << '\n';
	if( 0 != setsockopt( sock, IPPROTO_TCP, TCP_KEEPIDLE , (void*)&keepidle , sizeof(keepidle) ) ){
		cerr << "error(" << errno << ")\n";
	}

	cout << "setting SO_KEEPINTVL=" << keepintvl << '\n';
	if( 0 != setsockopt( sock, IPPROTO_TCP, TCP_KEEPINTVL, (void*)&keepintvl, sizeof(keepintvl) ) ){
		cerr << "error(" << errno << ")\n";
	}

	cout << "setting SO_KEEPCNT=" << keepcnt << '\n';
	if( 0 != setsockopt( sock, IPPROTO_TCP, TCP_KEEPCNT  , (void*)&keepcnt  , sizeof(keepcnt) ) ){
		cerr << "error(" << errno << ")\n";
	}
#endif

	memcpy( &sin.sin_addr, he->h_addr, sizeof(struct in_addr) );
	sin.sin_family = AF_INET;
	sin.sin_port = htons( CSA_PORT );

	if( -1 == connect( sock, (struct sockaddr*)(&sin), sizeof(sin) ) ){
		cerr << "Couldn't connect! ("
#ifdef WIN32
			<< GetLastError()
#else
			<< errno
#endif
			<< ')' << '\n';
		Disconnect();
		return 0;
	}

	return 1;
}

int MatchCSA::Disconnect(){
/************************************************
切断
************************************************/
#ifdef WIN32
	closesocket( sock );

	WSACleanup();
#else
	close( sock );
#endif

	return 1;
}

int MatchCSA::Login(){
/************************************************
ログイン
************************************************/
	char str[256];
	char disp[256];

	// ユーザ名とパスワード送信
	sprintf( str , "LOGIN %s %s\n", user, pass );
	sprintf( disp, "LOGIN %s *****\n", user );
	if( -1 == SendMessage( str, disp ) ){
		cerr << "Couldn't send a message." << '\n';
		return 0;
	}

	// 応答を待つ。
	if( GetRecFlag( MATCH_CSA_LOGIN_OK | MATCH_CSA_LOGIN_INC ) & MATCH_CSA_LOGIN_OK ){
		return 1;
	}

	return 0;
}

int MatchCSA::Logout(){
/************************************************
ログアウト
************************************************/
	if( -1 == SendMessage( "LOGOUT\n" ) ){
		cerr << "Couldn't send a message." << '\n';
		return 0;
	}

	// 応答を待つ。
	if( GetRecFlag( MATCH_CSA_LOGOUT ) ){
		return 1;
	}

	return 0;
}

int MatchCSA::SendAgree(){
/************************************************
対局条件承諾
************************************************/
	if( -1 == SendMessage( "AGREE\n" ) ){
		cerr << "Couldn't send a message." << '\n';
		return 0;
	}

	// 応答を待つ。
	if( GetRecFlag( MATCH_CSA_START | MATCH_CSA_REJECT ) & MATCH_CSA_START ){
		return 1;
	}

	return 0;
}

int MatchCSA::SendReject(){
/************************************************
対局条件拒否
************************************************/
	if( -1 == SendMessage( "REJECT\n" ) ){
		cerr << "Couldn't send a message." << '\n';
		return 0;
	}

	// 応答を待つ。
	if( GetRecFlag( MATCH_CSA_REJECT ) ){
		return 1;
	}

	return 0;
}

int MatchCSA::InputTime( const char* mstr ){
	const char* p = strstr( mstr, ",T" );
	int ret = 0;

	if( p != NULL ){
		atime.Reduce( strtol( p + 2, NULL, 10 ) );
		ret = 1;
	}
	else{
		atime.Reduce( 0 );
		ret = 0;
	}

	// 消費時間の表示(コンピュータ将棋選手権必須機能)
	MutexLock();
	cout << "------------------------------" << '\n';
	cout << "turn\tused\tremaining\n";
	cout << "SENTE\t";
#if POSIX
	if( atime.GetTurn() == AvailableTime::G ){
		cout << "\x1b[31m";
	}
#endif
	cout << atime.GetCumUsedTime(AvailableTime::S) << '\t';
	cout << atime.GetRemainingTime(AvailableTime::S) << '\n';
#if POSIX
	cout << "\x1b[39m";
#endif
	cout << "GOTE \t";
#if POSIX
	if( atime.GetTurn() == AvailableTime::S ){
		cout << "\x1b[31m";
	}
#endif
	cout << atime.GetCumUsedTime(AvailableTime::G) << '\t';
	cout << atime.GetRemainingTime(AvailableTime::G) << '\n';
#if POSIX
	cout << "\x1b[39m";
	cout.flush();
#endif
	cout << "------------------------------" << '\n';
	MutexUnlock();

	return ret;
}

int MatchCSA::SendMove( THINK_INFO* pthink_info ){
/************************************************
指し手の送信
************************************************/
	ostringstream ss;
	int value;

	// 送信する文字列を生成
	ss << pshogi->GetStrMoveCSA();
	if( floodgate != 0 && pthink_info != NULL && pthink_info->bPVMove ){
		// floodgateモード 評価値を送信
		value = (int)pthink_info->value * 100 / pshogi->GetV_BAN( SFU );
		if( info.sengo == GOTE ){
			value = -value;
		}
		ss << ",\'* "<< value;

		// 読みを送信 2010/09/10
		int i;
		KOMA sengo;
		Moves moves( MAX_DEPTH );

		pcons->GetPVMove( moves );
		sengo = pshogi->GetSengo();
		for( i = 1 ; i < moves.GetNumber() ; i++ ){
			ss << ' ' << Shogi::GetStrMoveCSA( moves[i], sengo );
			sengo ^= SENGO;
		}
	}
	// 改行
	ss << '\n';

	if( -1 == SendMessage( ss.str().c_str() ) ){
		cerr << "Couldn't send a message." << '\n';
		return 0;
	}

	// 応答を待つ。
	if( info.sengo == SENTE ){
		if( GetRecFlag( MATCH_CSA_SENTE ) ){
			InputTime( MoveStrSelf() );
			return 1;
		}
	}
	else{
		if( GetRecFlag( MATCH_CSA_GOTE ) ){
			InputTime( MoveStrSelf() );
			return 1;
		}
	}

	return 0;
}

int MatchCSA::SendToryo(){
/************************************************
投了
************************************************/
	if( -1 == SendMessage( "%TORYO\n" ) ){
		cerr << "Couldn't send a message." << '\n';
		return 0;
	}

	// 応答を待つ。
	if( GetRecFlag( MATCH_CSA_MASK_END ) ){
		return 1;
	}

	return 0;
}

#define INTERVAL			20

unsigned int MatchCSA::GetRecFlag( unsigned int mask ){
/************************************************
メッセージの受信を待つ。
************************************************/
	//int i;
	unsigned int temp;
	//int sec;

	//sec = timeout;
	for( /*i = 0*/ ; ; /*i++*/ ){
		// フラグのチェック
		if( ( temp = rflag & mask ) ){
			SetResult();
			rflag &= (~mask); // フラグを落とす。
			return temp;
		}

		// 強制ログアウト, 切断
		if( rflag & MATCH_CSA_ERROR ||
			rflag & MATCH_CSA_LOGOUT )
		{
			return 0;
		}

		/*
		if( i * INTERVAL >= 1000 ){
			// 1秒経過
			i = 0;

			if( sec != 0 && --sec == 0 ){
				// タイムアウト
				MutexLock();
				cout << "Time out!" << '\n';
				MutexUnlock();

				return 0;
			}
		}
		*/

#ifdef WIN32
		Sleep( INTERVAL );
#else
		usleep( INTERVAL * 1000 );
#endif
	}

	return 0U;
}

#ifdef WIN32
unsigned __stdcall MatchCSA::ReceiveThread( LPVOID p ){
#else
void* MatchCSA::ReceiveThread( void* p ){
#endif
/************************************************
受信スレッド
************************************************/
	MatchCSA* pmatchCSA = (MatchCSA*)p;
	bool position = false;
	double sec_a = 0.0, sec_r = 0.0;

	// メッセージループ
	while( pmatchCSA->ReceiveMessage() ){
		pmatchCSA->MutexLock();
#if POSIX
		cout << "\x1b[35m";
#endif
		cout << '>' << pmatchCSA->line << '\n';
#if POSIX
		cout << "\x1b[39m";
		cout.flush();
#endif
		pmatchCSA->MutexUnlock();

		if( position ){
			if( String::WildCard( pmatchCSA->line, "END Position" ) ){
				position = false;
				pmatchCSA->atime.Reset( sec_a, sec_r );
			}
			else{
				if( !pmatchCSA->pshogi->InputLineCSA( pmatchCSA->line ) ){
					cout << "Input Error!!" << '\n';
				}
			}
		}
		else if( pmatchCSA->line[0] == '%' ){
			// 特別な指し手
		}
		else if( pmatchCSA->line[0] == '+' ){
			// 先手の指し手
			strcpy( pmatchCSA->mstrS, pmatchCSA->line );
			pmatchCSA->rflag |= MATCH_CSA_SENTE;
		}
		else if( pmatchCSA->line[0] == '-' ){
			// 後手の指し手
			strcpy( pmatchCSA->mstrG, pmatchCSA->line );
			pmatchCSA->rflag |= MATCH_CSA_GOTE;
		}

		// 対局条件
		else if( String::WildCard( pmatchCSA->line, "BEGIN Game_Summary" ) ){
			// 対局条件取得開始
			memset( &pmatchCSA->info, 0, sizeof(GAME_INFO) );
			sec_a = sec_r = 0.0;
		}
		else if( String::WildCard( pmatchCSA->line, "Protocol_Version:*" ) ){
			// プロトコルバージョン
		}
		else if( String::WildCard( pmatchCSA->line, "Protocol_Mode:*" ) ){
			// プロトコルモード(Server)
		}
		else if( String::WildCard( pmatchCSA->line, "Format:*" ) ){
			// フォーマット(Shogi 1.0)
		}
		else if( String::WildCard( pmatchCSA->line, "Declaration:*" ) ){
			// 宣言法(Jishogi1.1)
		}
		else if( String::WildCard( pmatchCSA->line, "Game_ID:*" ) ){
			// ゲーム識別子
			strcpy( pmatchCSA->info.game_name, String::GetSecondToken( pmatchCSA->line, ':' ) );
		}
		else if( String::WildCard( pmatchCSA->line, "Name+:*" ) ){
			// 先手対局者名
			strcpy( pmatchCSA->info.sente, String::GetSecondToken( pmatchCSA->line, ':' ) );
		}
		else if( String::WildCard( pmatchCSA->line, "Name-:*" ) ){
			// 後手対局者名
			strcpy( pmatchCSA->info.gote, String::GetSecondToken( pmatchCSA->line, ':' ) );
		}
		else if( String::WildCard( pmatchCSA->line, "Your_Turn:*" ) ){
			// 自分の手番(+ or -)
			if( *String::GetSecondToken( pmatchCSA->line, ':' ) == '+' ){
				pmatchCSA->info.sengo = SENTE;
			}
			else{
				pmatchCSA->info.sengo = GOTE;
			}
		}
		else if( String::WildCard( pmatchCSA->line, "Rematch_On_Draw:*" ) ){
			// 引き分け時の自動再試合
		}
		else if( String::WildCard( pmatchCSA->line, "To_Move:*" ) ){
			// 開始直後の手番(+ or -)
			if( *String::GetSecondToken( pmatchCSA->line, ':' ) == '+' ){
				pmatchCSA->info.to_move = SENTE;
			}
			else{
				pmatchCSA->info.to_move = GOTE;
			}
		}

		// 時間情報
		else if( String::WildCard( pmatchCSA->line, "BEGIN Time" ) ){
		}
		else if( String::WildCard( pmatchCSA->line, "Time_Unit:*" ) ){
			// 時間計測の単位
		}
		else if( String::WildCard( pmatchCSA->line, "Least_Time_Per_Move:*" ) ){
			// 1手の最小消費時間
		}
		else if( String::WildCard( pmatchCSA->line, "Total_Time:*" ) ){
			// 持ち時間(sec or min or msec )
			sec_a = strtol( String::GetSecondToken( pmatchCSA->line, ':' ), NULL, 10 );
			if( String::WildCard( pmatchCSA->line, "*msec*" ) ){
				sec_a *= 1.0e-3; // msec
			}
			else if( String::WildCard( pmatchCSA->line, "*min*" ) ){
				sec_a *= 60.0; // min
			}
		}
		else if( String::WildCard( pmatchCSA->line, "Byoyomi:*" ) ){
			// 秒読み
			sec_r = strtol( String::GetSecondToken( pmatchCSA->line, ':' ), NULL, 10 );
		}
		else if( String::WildCard( pmatchCSA->line, "END Time" ) ){
		}

		// 開始局面
		else if( String::WildCard( pmatchCSA->line, "BEGIN Position" ) ){
			pmatchCSA->pshogi->SetBan( HIRATE );
			position = true;
		}

		else if( String::WildCard( pmatchCSA->line, "END Game_Summary" ) ){
			// 対局条件取得終了
			pmatchCSA->rflag |= MATCH_CSA_SUMMARY;
		}

		// ログイン・ログアウト
		else if( String::WildCard( pmatchCSA->line, "LOGIN:* OK" ) ){
			// ログイン成功
			pmatchCSA->rflag |= MATCH_CSA_LOGIN_OK;
		}
		else if( String::WildCard( pmatchCSA->line, "LOGIN:incorrect" ) ){
			// ログイン失敗
			pmatchCSA->rflag |= MATCH_CSA_LOGIN_INC;
		}
		else if( String::WildCard( pmatchCSA->line, "LOGOUT:completed" ) ){
			// ログアウト
			pmatchCSA->rflag |= MATCH_CSA_LOGOUT;

			// ログアウトしたらスレッド終了
#ifdef WIN32
			return 0;
#else
			return NULL;
#endif
		}

		// 対局開始・終了
		else if( String::WildCard( pmatchCSA->line, "START:*" ) ){
			// 対局開始
			pmatchCSA->rflag |= MATCH_CSA_START;
		}
		else if( String::WildCard( pmatchCSA->line, "REJECT:* by *" ) ){
			// 対局拒否
			pmatchCSA->rflag |= MATCH_CSA_REJECT;
		}
		else if( String::WildCard( pmatchCSA->line, "#WIN(LOSE)" ) ){
			// 都万さんの作った疑似サーバが#WIN(LOSE)を送ってくる。
			// (疑似サーバのバグ?)
			pmatchCSA->rflag |= MATCH_CSA_WIN | MATCH_CSA_LOSE;
			pmatchCSA->pcons->SetIFlag( 1 );
		}
		else if( String::WildCard( pmatchCSA->line, "#WIN" ) ){
			// 勝ち
			pmatchCSA->rflag |= MATCH_CSA_WIN;
			pmatchCSA->pcons->SetIFlag( 1 );
		}
		else if( String::WildCard( pmatchCSA->line, "#LOSE" ) ){
			// 負け
			pmatchCSA->rflag |= MATCH_CSA_LOSE;
			pmatchCSA->pcons->SetIFlag( 1 );
		}
		else if( String::WildCard( pmatchCSA->line, "#DRAW" ) ){
			// 引き分け
			pmatchCSA->rflag |= MATCH_CSA_DRAW;
			pmatchCSA->pcons->SetIFlag( 1 );
		}
		else if( String::WildCard( pmatchCSA->line, "#CHUDAN" ) ){
			// 中断
			pmatchCSA->rflag |= MATCH_CSA_CHUDAN;
			pmatchCSA->pcons->SetIFlag( 1 );
		}
		else if( String::WildCard( pmatchCSA->line, "#SENNICHITE" ) ){
			// 千日手
			pmatchCSA->rflag |= MATCH_CSA_SENNICHI;
		}
		else if( String::WildCard( pmatchCSA->line, "#OUTE_SENNICHITE" ) ){
			// 連続王手の千日手
			pmatchCSA->rflag |= MATCH_CSA_OUTE_SEN;
		}
		else if( String::WildCard( pmatchCSA->line, "#ILLEGAL_MOVE" ) ){
			// 反則手
			pmatchCSA->rflag |= MATCH_CSA_ILLEGAL;
		}
		else if( String::WildCard( pmatchCSA->line, "#TIME_UP" ) ){
			// 時間切れ
			pmatchCSA->rflag |= MATCH_CSA_TIME_UP;
		}
		else if( String::WildCard( pmatchCSA->line, "#RESIGN" ) ){
			// 投了
			pmatchCSA->rflag |= MATCH_CSA_RESIGN;
		}
		else if( String::WildCard( pmatchCSA->line, "#JISHOGI" ) ){
			// 持将棋
			pmatchCSA->rflag |= MATCH_CSA_JISHOGI;
		}
	}

	pmatchCSA->rflag |= MATCH_CSA_ERROR;

#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
}
