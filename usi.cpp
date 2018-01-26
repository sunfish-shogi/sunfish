/* usi2csa.cpp
 * USIプロトコル
 */

#include <windows.h>
#include <process.h>
#include <mbstring.h>
#include <fstream>
#include "match.h"
#include "sunfish.h"
#include "wconf.h"

namespace{
	const int a2p[] = {
	//  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S
		0, 6, 0, 0, 0, 0, 5, 0, 0, 0, 8, 2, 0, 3, 0, 1, 0, 7, 4,
	//  T  U  V  W  X  Y  Z
		0, 0, 0, 0, 0, 0, 0
	};
	const char p2a[] = { 'P', 'L', 'N', 'S', 'G', 'B', 'R' };

	ShogiEval* pshogi = NULL;
	Think* pthink = NULL;
	Book*  pbook = NULL;
	ResignController* presign = NULL;

	int chudan = 0;
	int chudanDfPn = 0;
	int chudanPonder = 0;
	bool stop = false;

	HANDLE hThink = NULL;
	HANDLE hPonder = NULL;
	HANDLE hMate = NULL;

	bool b_ponder = true;
	int thread_num = 1;
	int resign_value = RESIGN_VALUE_DEF;
	int hash_mbytes = 40;

	int btime = 0;
	int wtime = 0;
	int byoyomi = 0;
	int mate_limit = 0;
	bool infinite = false;

	WConf wconf;
} // namespace

bool isPonderThread = false;

#define THINK_CLOSE			\
do{ if( hThink != NULL ){ \
 chudan = chudanDfPn = 1; \
 WaitForSingleObject( hThink, INFINITE ); \
 CloseHandle( hThink ); hThink = NULL; } \
}while( 0 )

#define PONDER_CLOSE			\
do{ if( hPonder != NULL ){ \
 chudanPonder = 1; \
 WaitForSingleObject( hPonder, INFINITE ); \
 CloseHandle( hPonder ); hPonder = NULL; } \
}while( 0 )

#define MATE_CLOSE				\
do{ if( hMate != NULL ){ \
 chudanDfPn = 1; \
 WaitForSingleObject( hMate, INFINITE ); \
 CloseHandle( hMate ); hMate = NULL; } \
}while( 0 )

#if 0 // mutex
static HANDLE hMutex;
#define MUTEX_CREATE		do{ hMutex = CreateMutex( NULL, true, NULL ); }while( 0 )
#define MUTEX_CLOSE			do{ CloseHandle( hMutex );}while( 0 )
#define MUTEX_LOCK			do{ WaitForSingleObject( hMutex, INFINITE ); }while( 0 )
#define MUTEX_UNLOCK		do{ ReleaseMutex( hMutex ); }while( 0 )
#else // mutex
static long mutex;
#define MUTEX_CREATE		do{ mutex = 0; }while( 0 )
#define MUTEX_CLOSE			do{ }while( 0 )
#define MUTEX_LOCK			do{ \
long l; \
while( 1 ){ \
l = InterlockedExchange( &mutex, 1 ); \
if( l == 0 ){ break; } \
while( mutex != 0 ) \
	; \
} }while( 0 )
#define MUTEX_UNLOCK		do{ mutex = 0; }while( 0 )
#endif // mutex

void usi_init();
bool usi();
void usi_end();

int main( int argc, char* argv[] ){
	init_gen_rand( (unsigned int)time( NULL ) );
	usi_init();
	usi();
	usi_end();

	return 0;
}

void dividePath( char *path, char *name ){
	int k, l, m, n;

	m = 0;
	n = 0;
	while( path[m] != '\0' ){
		if( path[m] != '\"' ){
			path[n] = path[m];
			n++;
		}
		m++;
	}
	path[n] = '\0';

	l = strlen( path );
	n = -1;
	k = 0;
	for( m = 0 ; m < l ; m++ ){
		if( !k ){
			if( path[m] == '\\' || path[m] == '/' )
				n = m;
			k = _ismbblead( path[m] );
		}
		else
			k = 0;
	}
	if( name != NULL ){
		for( m = n+ 1 ; m <= l ; m++ )
			name[m-n-1] = path[m];
	}
	path[n+1] = '\0';
}

// カレントディレクトリをexeの位置に設定
bool setCurrentDirectory(){
	char new_curr[1024];
	GetModuleFileName( NULL, new_curr, sizeof(new_curr) );
	dividePath( new_curr, NULL );
	SetCurrentDirectory( new_curr );
	return true;
}

void chomp( char* p ){
	int length = strlen( p );
	if( length > 0 ){ p[length-1] = '\0'; }
}

char* getFirstToken( char* p ){
	while( *p == ' ' ){ p++; }
	if( *p == '\0' ){ return NULL; }
	char* p2 = p + 1;
	while( *p2 != '\0' ){
		if( *p2 == ' ' ){
			*p2 = '\0';
			break;
		}
		p2++;
	}
	return p;
}

char* getNextToken( char* p ){
	while( *p != '\0' ){ p++; }
	return getFirstToken( p + 1 );
}

bool setMove( MOVE& move, KOMA sengo, const char* p ){
	if( strlen( p ) < 4 ){ return false; }

	// 移動元
	if( p[0] >= '1' && p[0] <= '9' ){
		move.from = BanAddr( p[0]-'0', p[1]-'a'+1);
	}
	else{
		move.from = DAI + sengo + a2p[p[0]-'A'];
	}

	// 移動先
	move.to = BanAddr( p[2]-'0', p[3]-'a'+1 );
	move.nari = p[4] == '+' ? 1 : 0;

	return true;
}

void setMoveString( char* p, size_t size, const MOVE& move ){
	assert( size >= 6 );
	memset( p, 0, 6 );

	if( move.from & DAI ){
		p[0] = p2a[(move.from&(~DAI)&(~SENGO))-FU];
		p[1] = '*';
	}
	else{
		p[0] = '1' + Addr2X( move.from ) - 1;
		p[1] = 'a' + Addr2Y( move.from ) - 1;
	}
	p[2] = '1' + Addr2X( move.to ) - 1;
	p[3] = 'a' + Addr2Y( move.to ) - 1;
	if( move.nari != 0 ){ p[4] = '+'; }
}

void usi_init(){
	MUTEX_CREATE;

	// read settings
	wconf.Read();
	wconf.getValueInt( "NumberOfThreads", &thread_num );
	wconf.getValueInt( "ResignValue", &resign_value );
	wconf.getValueInt( "HashSize", &hash_mbytes );
}

void usi_end(){
	MUTEX_CLOSE;
	THINK_CLOSE;
	PONDER_CLOSE;
	MATE_CLOSE;

	// write settings
	wconf.setValueInt( "NumberOfThreads", thread_num );
	wconf.setValueInt( "ResignValue", resign_value );
	wconf.setValueInt( "HashSize", hash_mbytes );
	wconf.Write();
}

#ifdef _WIN64
static VOID CALLBACK timelimit( UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2 ){
#else
static VOID CALLBACK timelimit( UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2 ){
#endif
/************************************************
制限時間経過
************************************************/
	chudan = 1; // 打ち切りフラグ
}

#ifdef _WIN64
static VOID CALLBACK timelimit2( UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2 ){
#else
static VOID CALLBACK timelimit2( UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2 ){
#endif
/************************************************
制限時間経過
************************************************/
	chudanDfPn = 1; // 打ち切りフラグ(詰み探索)
}

unsigned __stdcall ponder( LPVOID p ){
/************************************************
相手番で探索をするスレッド
************************************************/
	THINK_INFO info;

	if( pshogi != NULL && pthink != NULL ){
		isPonderThread = true;
		pthink->Search( pshogi, NULL, 32, &info, &chudanPonder, 1 );
		isPonderThread = false;
	}

	return 0;
}

unsigned __stdcall think( LPVOID p ){
/************************************************
思考スレッド
************************************************/
	Moves moves;
	MOVE move;
	THINK_INFO info;
	MMRESULT tid;
	int msec_mate = 0;
	int limit = byoyomi;
	int ret;

	memset( &info, 0, sizeof(THINK_INFO) );

	// 時間制限
	if( pshogi->GetSengo() == SENTE ){
		limit += min( btime, btime / 20 + 1 );
	}
	else{
		limit += min( wtime, wtime / 20 + 1 );
	}

	// 時間無制限
	if( infinite ){ limit = 0; }

	// 定跡を調べる。
	if( pbook != NULL ){
		ret = pbook->GetMove( pshogi, move );
	}

	// 詰み探索
	if( ret == 0 ){
		if( limit > 0 ){
			msec_mate = limit / 10;
			if( msec_mate > 1000 ){ msec_mate = 1000; } // 最大1秒
			tid = timeSetEvent( msec_mate, 10, timelimit2, 0, TIME_ONESHOT );
		}
		ret = pthink->DfPnSearch( pshogi, &move, NULL, &info, &chudanDfPn );
		msec_mate = (int)info.msec;
		if( limit > 0 ){ timeKillEvent( tid ); }
	}

	// 通常探索
	if( ret == 0 ){
		if( limit > 0 ){
			tid = timeSetEvent( limit - msec_mate, 10, timelimit, 0, TIME_ONESHOT );
			int err = GetLastError();
		}
		ret = pthink->Search( pshogi, &move, 32, &info, &chudan, 1 );
		if( limit > 0 ){ timeKillEvent( tid ); }
	}

	// infiniteの場合、stopを待つ。
	if( infinite ){
		while( !stop ){ Sleep( 0 ); }
	}

	// 結果を返す。
	if( ret != 0 && presign->good( info ) ){
		char mstr[6];

		if     ( info.value >= VMAX ){
			cout << "info score mate +\n";
		}
		else if( info.value <= VMIN ){
			cout << "info score mate -\n";
		}
		else{
			cout << "info score cp " << (int)info.value << '\n';
		}

		setMoveString( mstr, sizeof(mstr), move );

#ifndef NDEBUG
		MUTEX_LOCK;
		pshogi->PrintBan();
		cout << "// " << pshogi->GetStrMove( move, true );
		cout << "// " << mstr << '\n';
		cout.flush();
		MUTEX_UNLOCK;
#endif

		MUTEX_LOCK;
		cout << "bestmove " << mstr << "\n";
		cout.flush();
		MUTEX_UNLOCK;

		// ponder
		if( b_ponder ){
			pshogi->MoveD( move );
			chudanPonder = 0;
			hPonder = (HANDLE)_beginthreadex( NULL, 0, ponder, (LPVOID)NULL, 0, NULL );
		}
	}
	else{
		MUTEX_LOCK;
		cout << "bestmove resign\n";
		cout.flush();
		MUTEX_UNLOCK;
	}

	return 0;
}

unsigned __stdcall mate( LPVOID p ){
/************************************************
詰み探索スレッド
************************************************/
	//MOVE move;
	Moves moves;
	THINK_INFO info;
	MMRESULT tid;
	int ret;

	memset( &info, 0, sizeof(THINK_INFO) );

	// 詰み探索
	if( !infinite ){
		tid = timeSetEvent( mate_limit, 10, timelimit2, 0, TIME_ONESHOT );
	}
	ret = pthink->DfPnSearch( pshogi, /*&move*/NULL, &moves, &info, &chudanDfPn );
	if( !infinite ){
		timeKillEvent( tid );
	}

	// 結果を返す。
	if( ret != 0 ){
		char mstr[6];

		MUTEX_LOCK;
		cout << "checkmate";
		for( int i = 0 ; i < moves.GetNumber() ; i++ ){
			setMoveString( mstr, sizeof(mstr), moves[i] );
			cout << ' ' << mstr;
		}
		cout << "\n";
		cout.flush();
		MUTEX_UNLOCK;
	}
	else{
		// 本当は時間切れの場合にcheckmate timeoutを返す。
		MUTEX_LOCK;
		cout << "checkmate nomate\n";
		cout.flush();
		MUTEX_UNLOCK;
	}

	return 0;
}

void shogi_init(){
	if( pshogi == NULL ){
		pshogi = new ShogiEval();
	}
	if( pthink == NULL ){
		pthink = new Think( thread_num );
	}
	if( pbook == NULL ){
		pbook = new Book();
	}
	if( presign == NULL ){
		presign = new ResignController( resign_value );
	}
}

void shogi_end(){
	THINK_CLOSE;
	PONDER_CLOSE;
	MATE_CLOSE;
	if( pshogi ){
		delete pshogi;
		pshogi = NULL;
	}
	if( pthink ){
		delete pthink;
		pthink = NULL;
	}
	if( pbook ){
		delete pbook;
		pbook = NULL;
	}
	if( presign ){
		delete presign;
		presign = NULL;
	}
}

void getBan( KOMA* _ban, const char* p ){
	int i;
	int x, y;
	KOMA nari = 0;
	KOMA* ban = _ban + BAN_OFFSET;

	// 盤面
	for( i = 0 ; i < 16*13 ; i++ ){
		_ban[i] = ConstData::_empty[i];
	}
	x = 9; y = 1;
	for( i = 0 ; p[i] != '\0' ; i++ ){
		if( p[i] == '+' ){
			nari = NARI;
		}
		else if( p[i] == '/' ){
			x = 9;
			y++;
		}
		else if( p[i] >= '1' && p[i] <= '9' ){
			x -= p[i] - '0';
		}
		else if( p[i] >= 'A' && p[i] <= 'Z' ){
			ban[BanAddr(x,y)] = a2p[p[i]-'A'] | SENTE | nari;
			nari = 0;
			x--;
		}
		else if( p[i] >= 'a' && p[i] <= 'z' ){
			ban[BanAddr(x,y)] = a2p[p[i]-'a'] | GOTE | nari;
			nari = 0;
			x--;
		}
	}
}

KOMA getSengo( const char* p, KOMA def_sengo ){
	// 手番
	if     ( strcmp( p, "b" ) == 0 ){ return SENTE; }
	else if( strcmp( p, "w" ) == 0 ){ return GOTE; }
	return def_sengo;
}

void getDai( int* dai, const char* p ){
	// 持ち駒
	int i;
	int num = 0;

	memset( dai, 0, 64 * sizeof(dai[0]) );
	for( i = 0 ; p[i] != '\0' ; i++ ){
		// 数字 => 駒の枚数
		if     ( p[i] >= '0' && p[i] <= '9' ){
			num = num * 10 + ( p[i] - '0' );
		}
		// 大文字 => 先手の駒
		else if( p[i] >= 'A' && p[i] <= 'Z' ){
			dai[a2p[p[i]-'A']|SENTE] += ( num == 0 ? 1 : num );
			num = 0;
		}
		// 小文字 => 後手の駒
		else if( p[i] >= 'a' && p[i] <= 'z' ){
			dai[a2p[p[i]-'a']|GOTE ] += ( num == 0 ? 1 : num );
			num = 0;
		}
	}
}

bool getOptionValueInt( char* p, int* out ){
	if( strcmp( p, "value" ) != 0 ){ return false; }

	if( ( p = getNextToken( p ) ) == NULL ){ return false; }

	*out = strtol( p, NULL, 10 );

	return true;
}

bool getOptionValueBool( char* p, bool* out ){
	if( strcmp( p, "value" ) != 0 ){ return false; }

	if( ( p = getNextToken( p ) ) == NULL ){ return false; }

	if     ( strcmp( p, "true" ) == 0 ){
		*out = true;
		return true;
	}
	else if( strcmp( p, "false" ) == 0 ){
		*out = false;
		return true;
	}

	return false;
}

// USIプロトコル
bool usi(){
	bool ready = false;

	while( 1 ){
		char line[16*1024];
		char* p;

		fgets( line, sizeof(line) - 1, stdin );
		chomp( line );
		if( feof( stdin ) ){ return false; }

		// 初めのトークン
		p = getFirstToken( line );
		if( p == NULL ){ continue; }

		// quit
		if     ( strcmp( p, "quit" ) == 0 ){
			shogi_end();
			break; // 正常終了
		}
		// usi
		else if( strcmp( p, "usi" ) == 0 ){
			MUTEX_LOCK;
			cout << "id name " << PROGRAM_NAME << ' ' << PROGRAM_VER << "\n";
			cout << "option name NumberOfThreads type spin default " << thread_num << " min 1 max 32\n";
			cout << "option name ResignValue type spin default " << resign_value << " min " << RESIGN_VALUE_MIN << " max " << VMAX << "\n";
			cout << "usiok\n";
			cout.flush();
			MUTEX_UNLOCK;
		}
		// setoption
		else if( strcmp( p, "setoption" ) == 0 ){
			if( ( p = getNextToken( p ) ) == NULL ){ continue; }
			if( strcmp( p, "name" ) == 0 ){
				if( ( p = getNextToken( p ) ) == NULL ){ continue; }
				// ponder
				if     ( strcmp( p, "USI_Ponder" ) == 0 ){
					if( ( p = getNextToken( p ) ) == NULL ){ continue; }
					getOptionValueBool( p, &b_ponder );
				}
				// hash
				else if( strcmp( p, "USI_Hash" ) == 0 ){
					if( ( p = getNextToken( p ) ) == NULL ){ continue; }
					if( getOptionValueInt( p, &hash_mbytes ) ){
						Hash::SetSize( HashSize::MBytesToPow2( hash_mbytes, thread_num ) );
					}
				}
				// スレッド数
				else if( strcmp( p, "NumberOfThreads" ) == 0 ){
					if( ( p = getNextToken( p ) ) == NULL ){ continue; }
					if( getOptionValueInt( p, &thread_num ) ){
						if( pthink ){ pthink->SetNumberOfThreads( thread_num ); }
					}
				}
				// 投了のしきい値
				else if( strcmp( p, "ResignValue" ) == 0 ){
					if( ( p = getNextToken( p ) ) == NULL ){ continue; }
					if( getOptionValueInt( p, &resign_value ) ){
						if( pthink ){ presign->set_limit( resign_value ); }
					}
				}
			}
		}
		// usinewgame
		else if( strcmp( p, "usinewgame" ) == 0 ){ }
		// isready
		else if( strcmp( p, "isready" ) == 0 ){
			if( setCurrentDirectory() == false ){
				return false;
			}

			shogi_init();
			ready = true;

			MUTEX_LOCK;
			cout << "readyok\n";
			cout.flush();
			MUTEX_UNLOCK;
		}
		// position
		else if( strcmp( p, "position" ) == 0 ){
			// スレッドの終了待ち。
			THINK_CLOSE;
			PONDER_CLOSE;
			MATE_CLOSE;

			if( ( p = getNextToken( p ) ) == NULL ){ continue; }

			// 平手の初期局面
			if( strcmp( p, "startpos" ) == 0 ){
				// 指し手
				pshogi->SetBan( HIRATE );
			}
			else if( strcmp( p, "sfen" ) == 0 ){
				KOMA ban[16*13];
				int dai[64];
				KOMA sengo = SENTE;
				int i;

				// 盤面
				for( i = 0 ; i < sizeof(ban)/sizeof(ban[0]) ; i++ ){
					ban[i] = ConstData::_empty[i];
				}
				if( ( p = getNextToken( p ) ) == NULL ){ continue; }
				getBan( ban, p );

				// 手番
				if( ( p = getNextToken( p ) ) == NULL ){ continue; }
				sengo = getSengo( p, sengo );

				// 持ち駒
				if( ( p = getNextToken( p ) ) == NULL ){ continue; }
				getDai( dai, p );

				// 手数 将棋所は常に1
				if( ( p = getNextToken( p ) ) == NULL ){ continue; }

				// 局面のセット
				pshogi->SetBan( ban, dai, sengo );
			}
			else{
				return false;
			}

			// 棋譜
			if( ( p = getNextToken( p ) ) == NULL ){ continue; }
			if( strcmp( p, "moves" ) == 0 ){
				while( ( p = getNextToken( p ) ) != NULL ){
					MOVE move;
					if( setMove( move, pshogi->GetSengo(), p ) == false ){ break; }
					pshogi->MoveD( move );
				}
			}

			// Debug
#ifndef NDEBUG
			pshogi->PrintBan();
			cout.flush();
#endif
		}
		// go
		else if( strcmp( p, "go" ) == 0 ){
			bool is_mate_search = false;
			infinite = false;

			// スレッドの終了待ち。
			THINK_CLOSE;
			PONDER_CLOSE;
			MATE_CLOSE;

			while( ( p = getNextToken( p ) ) != NULL ){
				// 先手の持ち時間
				if     ( strcmp( p, "btime" ) == 0 ){
					if( ( p = getNextToken( p ) ) == NULL ){ break; }
					btime = strtol( p, NULL, 10 );
				}
				// 後手の持ち時間
				else if( strcmp( p, "wtime" ) == 0 ){
					if( ( p = getNextToken( p ) ) == NULL ){ break; }
					wtime = strtol( p, NULL, 10 );
				}
				// 秒読み
				else if( strcmp( p, "byoyomi" ) == 0 ){
					if( ( p = getNextToken( p ) ) == NULL ){ break; }
					byoyomi = strtol( p, NULL, 10 );
				}
				// 時間無制限
				else if( strcmp( p, "infinite" ) == 0 ){
					infinite = true;
				}
				// 詰将棋
				else if( strcmp( p, "mate" ) == 0 ){
					is_mate_search = true;
					if( ( p = getNextToken( p ) ) == NULL ){ break; }
					// 時間無制限
					if( strcmp( p, "infinite" ) == 0 ){
						infinite = true;
					}
					// 時間制限
					else{
						mate_limit = strtol( p, NULL, 10 );
					}
				}
			}

			// 思考開始
			if( ready ){
				stop = false;

				// 詰み探索
				if( is_mate_search ){
					chudanDfPn = 0;
					hMate = (HANDLE)_beginthreadex( NULL, 0, mate, (LPVOID)NULL, 0, NULL );
				}
				// 通常探索
				else{
					chudan = 0;
					chudanDfPn = 0;
					hThink = (HANDLE)_beginthreadex( NULL, 0, think, (LPVOID)NULL, 0, NULL );
				}
			}
		}
		// stop
		else if( strcmp( p, "stop" ) == 0 ){
			THINK_CLOSE;
			PONDER_CLOSE;
			MATE_CLOSE;
			stop = true;
		}
		// ponderhit
		else if( strcmp( p, "ponderhit" ) == 0 ){ }
		// gameover
		else if( strcmp( p, "gameover" ) == 0 ){
			THINK_CLOSE;
			PONDER_CLOSE;
			MATE_CLOSE;
		}
	}

	return true;
}
