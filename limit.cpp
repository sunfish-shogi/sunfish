/* limit.cpp
 * R.Kubo 2010
 * 時間制限
 */

#include "match.h"

int Think::lim_iflag;

void Think::sigint( int ){
/************************************************
Ctrl + C 割り込み
************************************************/
	lim_iflag = 1;
}

#ifdef WIN32
VOID CALLBACK Think::sigtime( UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2 ){
#else
void Think::sigtime( int ){
#endif
/************************************************
制限時間経過
************************************************/
	lim_iflag = 1; // 打ち切りフラグ
}

int Think::SearchLimit( ShogiEval* pshogi, Book* pbook,
	MOVE* pmove, int mdepth, THINK_INFO* info, unsigned opt ){
/************************************************
思考ルーチン(制限時間を指定)
投了時は0を返し、pmoveは変更されない。
************************************************/
	int ret = 0;
	int mlimit_msec = 0;
	int mused_msec = 0;
	THINK_INFO minfo;
#ifdef WIN32
	MMRESULT tid; // タイマー
#endif
	unsigned limit_max_msec = limit_max * 1000;
	unsigned msec = 0;

	if( info != NULL )
		memset( info, 0, sizeof(THINK_INFO) );

	if( pbook != NULL ){
		// 定跡を調べる。
		if( ( ret = pbook->GetMove( pshogi, *pmove ) ) ){
			if( info != NULL )
				info->useBook = true;
			return ret;
		}
	}

	// 詰み探索
	// limit_maxが1の時に詰み探索をやると時間がなくなる。
	if( limit_max_msec != 1 ){
		mlimit_msec = limit_max_msec / 20;
		if( mlimit_msec <= 0 ){
			mlimit_msec = 1;
		}

		ret = DfPnSearchLimit( pshogi, pmove, NULL, &minfo, mlimit_msec );
		mused_msec = minfo.msec;

		if( info != NULL ){
			*info = minfo;
		}

		if( ret ){
			// 詰み
			return ret;
		}
	}

	// Ctrl + C 割り込み設定
	signal( SIGINT, sigint );

	// 通常探索
	lim_iflag = 0;

	// タイマ割り込み設定
	if( limit_max_msec > 0 ){
		msec = limit_max_msec - mused_msec;

#ifdef WIN32
		tid = timeSetEvent( msec, 10, sigtime, 0, TIME_ONESHOT );
#else
		signal( SIGALRM, sigtime );
		if( msec < 1000 ){ ualarm( msec * 1000, 0 ); }
		else             { alarm( msec / 1000 ); }
#endif
	}

	ret = Search( pshogi, pmove, mdepth, info, &lim_iflag, opt );

	// タイマ割り込み解除
	if( limit_max_msec > 0 ){
#ifdef WIN32
		timeKillEvent( tid );
#else
		if( msec < 1000 ){ ualarm( 0, 0 ); }
		else             { alarm( 0 ); }
		signal( SIGALRM, SIG_IGN );
#endif
	}

	// Ctrl + C 割り込み解除
	signal( SIGINT, SIG_DFL );

	if( info != NULL ){
		info->msec += mused_msec;
	}

	return ret;
}

int Think::DfPnSearchLimit( ShogiEval* pshogi, MOVE* pmove,
	Moves* pmoves, THINK_INFO* info, unsigned limit_msec ){
/************************************************
制限時間内で詰みを探す。
************************************************/
	int ret = 0;
#ifdef WIN32
	MMRESULT tid; // タイマー
#endif

	// Ctrl + C 割り込み設定
	signal( SIGINT, sigint );

	lim_iflag = 0;

	if( limit_msec > 0 ){
#ifdef WIN32
		tid = timeSetEvent( limit_msec, 10, sigtime, 0, TIME_ONESHOT );
#else
		signal( SIGALRM, sigtime );
		if( limit_msec < 1000 ){ ualarm( limit_msec * 1000, 0 ); }
		else                   { alarm( limit_msec / 1000 ); }
#endif
	}

	ret = DfPnSearch( pshogi, pmove, pmoves, info, &lim_iflag );

	if( limit_msec > 0 ){
#ifdef WIN32
		timeKillEvent( tid );
#else
		ualarm( 0, 0 );
		if( limit_msec < 1000 ){ ualarm( 0, 0 ); }
		else                   { alarm( 0 ); }
		signal( SIGALRM, SIG_IGN );
#endif
	}

	// Ctrl + C 割り込み解除
	signal( SIGINT, SIG_DFL );

	return ret;
}
