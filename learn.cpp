/* learn.cpp
 * R.Kubo 2008-2012
 * 機械学習
 */

#ifndef NLEARN

#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include "match.h"

// 学習時の探索深さ
#define L_DEPTH					1

// Experimentation
// 親子関の評価値の差を小さくする方向に抑制する。
#define EXP_PARENT_CHILD		0
#define PC_TYPE					1
#define NPENALTY				0
#define PC_LAMBDA0				0.0
#define PC_LAMBDA				0.0

bool LEARN_INFO::Output( const char* fname ){
/************************************************
学習情報の書き出し
************************************************/
	ofstream fout;

	fout.open( fname );
	if( !fout.is_open() ){
		cerr << "Open Error!..[" << fname << ']' << '\n';
		return false;
	}

	fout << "swindow="   << swindow << '\n';
	fout << "num="       << num << '\n';
	fout << "loss_base=" << loss_base << '\n';
	fout << "loss="      << loss << '\n';
	fout << "loss0="     << loss0 << '\n';
	fout << "time="      << time << '\n';
	fout << "total="     << total << '\n';

	fout.close();

	return true;
}

bool LEARN_INFO::Input( const char* fname ){
/************************************************
学習情報の読み込み
************************************************/
	ifstream fin;
	char line[1024];

	fin.open( fname );
	if( !fin.is_open() ){
		cerr << "Open Error!..[" << fname << ']' << '\n';
		return false;
	}

	while( 1 ){
		fin.getline( line, sizeof(line) );
		if( fin.fail() || fin.eof() ){
			break;
		}

		if( String::WildCard( line, "swindow=*" ) ){
			swindow = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "num=*" ) ){
			num = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "loss_base=*" ) ){
			loss_base = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "loss=*" ) ){
			loss = strtod( String::GetSecondToken( line, '=' ), NULL );
		}
		else if( String::WildCard( line, "loss0=*" ) ){
			loss0 = strtod( String::GetSecondToken( line, '=' ), NULL );
		}
		else if( String::WildCard( line, "time=*" ) ){
			time = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "total=*" ) ){
			total = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
	}

	fin.close();

	return true;
}

Learn::Learn(){
/************************************************
コンストラクタ
************************************************/
	eval = new Evaluate();

	// 設定の初期化
	mode = MODE_NORMAL;
	directory[0] = '\0';
	gradients[0] = '\0';
	steps = DEF_STEPS;
	adjust = DEF_ADJUST;
	tnum = DEF_TNUM;
	swmin = swmax = swindow = DEF_SWINDOW;
	start = 0;
	resume = 0;
	detail = false;
}

Learn::~Learn(){
/************************************************
デストラクタ
************************************************/
	delete eval;
}

int Learn::LoadConfig(){
/************************************************
設定の読み込み
************************************************/
	ifstream fin;
	char line[1024];
	int hash_mbytes = -1;

	// ファイルを開く。
	fin.open( FNAME_LCONF );

	// ファイルがあるか。
	if( fin.fail() ){
		cerr << "Fail!..[" << FNAME_LCONF << ']' << '\n';
		return 0;
	}

	while( 1 ){
		fin.getline( line, sizeof(line) );
		if( fin.fail() || fin.eof() )
			break;

		if( String::WildCard( line, "mode=*" ) ){
			switch( String::GetSecondToken( line, '=' )[0] ){
			case 'p':
				// PVMove Mode
				mode = MODE_PVMOVE;
				break;
			case 'g':
				// Gradient Mode
				mode = MODE_GRADIENT;
				break;
			case 'a':
				// Adjustment Mode
				mode = MODE_ADJUSTMENT;
				break;
			default:
				// Normal Mode
				mode = MODE_NORMAL;
				break;
			}
		}
		if( String::WildCard( line, "directory=*" ) ){
			strcpy( directory, String::GetSecondToken( line, '=' ) );
		}
		if( String::WildCard( line, "gradients=*" ) ){
			strcpy( gradients, String::GetSecondToken( line, '=' ) );
		}
		else if( String::WildCard( line, "steps=*" ) ){
			steps = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "adjust=*" ) ){
			adjust = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "threads=*" ) ){
			tnum = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "hash=*" ) ){
			hash_mbytes = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "swmin=*" ) ){
			swmin = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "swmax=*" ) ){
			swmax = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "start=*" ) ){
			start = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "resume=*" ) ){
			resume = strtol( String::GetSecondToken( line, '=' ), NULL, 10 );
		}
		else if( String::WildCard( line, "detail=on" ) ){
			detail = true;
		}
	}

	if( fin.fail() )
		fin.clear();

	// ファイルを閉じる。
	fin.close();

	// ハッシュ表のサイズ変更
	if( hash_mbytes != -1 ){
		Hash::SetSize( HashSize::MBytesToPow2( hash_mbytes, 1, CONS_PARALLEL, tnum ) );
	}

	return 1;
}

int Learn::LearnMain( int _mode ){
/************************************************
棋譜からの学習
************************************************/
	// 設定読み込み
	LoadConfig();

	if( _mode != MODE_DEFAULT ){
		mode = _mode;
	}

	// 設定書き出し
	ConfigLog();

	// 学習情報初期化
	info.Init();

	switch( mode ){
	case MODE_NORMAL:
		// 全行程
		return LearnNormal();

	case MODE_PVMOVE:
		// pvmove生成
		return LearnPVMove();

	case MODE_GRADIENT:
		// 勾配計算
		return LearnGradient();

	case MODE_ADJUSTMENT:
		// パラメータを更新
		return LearnAdjust();
	}

	return 0;
}

int Learn::LearnNormal(){
/************************************************
学習の全行程を行う。
************************************************/
	Timer tm;
	int t;
	int i, j;

	// 全体の時間計測
	tm.SetTime();

	// ファイルの列挙
	flist.Clear();
	if( 0 == flist.Enumerate( directory, "csa" ) ){
		cout << "There is no CSA files!" << '\n';
		return 0;
	}

	info.adjust = adjust;

	for( i = start ; i < steps ; i++ ){
		cout << "Iteration : " << i + 1 << '\n';
		iteration = i;

		cout << "Make PV-moves." << '\n';
		cout.flush();

		// Experimentation
		// 段階的な探索窓
		if( steps >= 2 ){
			swindow = swmin + ( swmax - swmin ) * i / ( steps - 1 );
		}
		else{
			swindow = ( swmin + swmax ) / 2;
		}
		info.swindow = swindow;

		// Experimentation
		// 段階的な訓練例の増加
#if defined(A)
#	error
#endif
#define A					1.0 // 初回の割合(A*yからyまで線形に増加)
		if( steps > 1 ){
			double y = (double)flist.Size();
			info.record = (int)( ( 1.0 - A ) * y * i / ( steps - 1 ) + A * y );
		}
		else{
			info.record = flist.Size();
		}
#undef A

		if( resume != 0 ){
			// Learn2から再開
			resume = 0;
			cout << "Resume mode!!\n";
			cout << "Skip the Phase 1.\n";
			goto lab_resume;
		}

		if( 0 == Learn1() ){
			cerr << "Learning Error! (1)" << '\n';
			return 0;
		}

lab_resume:
		for( j = 0 ; j < info.adjust ; j++ ){
			cout << "Update parameters." << '\n';
			cout.flush();

			if( 0 == Learn2() ){
				cerr << "Learning Error! (2)" << '\n';
				return 0;
			}

			// 初回
			if( j == 0 ){
				info.loss0 = info.loss;
			}

			cout << "Loss   : " << info.loss << '\n';
		}

		info.total = (int)tm.GetTime();

		// 経過を記録
		LearnLog();

		// Experimentation
		// 更新回数の変化(N/D倍)
#if defined(N) || defined(D)
#	error
#endif
#define N					1 // 分子
#define D					1 // 分母
		info.adjust = ( info.adjust * N + D - 1 ) / D;
#undef N
#undef D
	};

	t = (int)tm.GetTime();
	cout << "Complete! " << '\n';
	cout << "Total Time      : " << t << "sec" << '\n';

	return 1;
}

int Learn::LearnPVMove(){
/************************************************
pvmove生成
************************************************/
	int ret;

	iteration = start;

	// ファイルの列挙
	flist.Clear();
	if( 0 == flist.Enumerate( directory, "csa" ) ){
		cout << "There is no CSA files!" << '\n';
		return 0;
	}

	ret = Learn1();

	// 情報の書き出し
	info.Output( FNAME_LINFO );

	return ret;
}

int Learn::LearnGradient(){
/************************************************
勾配計算
************************************************/
	int ret;

	iteration = start;

	// 情報の読み込み
	info.Input( FNAME_LINFO );

	ret = Learn2();

	info.Output( FNAME_LINFO );

	return ret;
}

int Learn::LearnAdjust(){
/************************************************
ファイルから勾配を読み込んでパラメータを更新
************************************************/
	string fname;
	LEARN_INFO litemp;
	Param* pparam;
	Param* ptemp;
	int ret = 1;

	iteration = start;

	// ここだけflistを.grの列挙に使っている。
	// どうするか、あとで考える...。
	flist.Clear();
	if( 0 == flist.Enumerate( gradients, "gr" ) ||
		0 == flist.Pop( fname ) )
	{
		cout << "There is no GR files!" << '\n';
		return 0;
	}

	pparam = new Param();
	ptemp = new Param();

	pparam->ReadData( fname.c_str() );

	if( flist.Size() > 1 ){
		while( flist.Pop( fname ) ){
			ptemp->ReadData( fname.c_str() );

			(*pparam) += (*ptemp);
		}
	}

	delete ptemp;

	// パラメータの更新
	AdjustParam( pparam );

	// パラメータの保存
	if( 0 == eval->WriteData() )
		ret = 0;

	delete pparam;

	// LEARN_INFOを集計
	flist.Clear();
	if( 0 == flist.Enumerate( gradients, "info" ) ){
		cout << "There is no INFO files!" << '\n';
		return 0;
	}

	info.Init();

	while( flist.Pop( fname ) ){
		litemp.Input( fname.c_str() );
		info.num  += litemp.num;
		info.loss += litemp.loss * litemp.num;
	}
	if( info.num != 0 ){
		info.loss /= info.num;
	}

	cout << "Loss   : " << info.loss << '\n';

	LearnLog();

	return ret;
}

int Learn::ConfigLog(){
/************************************************
設定の書き出し
************************************************/
	ofstream fout;
	time_t t;

	fout.open( FNAME_EVINFO, ios::out | ios::app );
	if( fout.fail() ){
		cerr << "Open Error!..[" << FNAME_EVINFO << ']' << '\n';
		return 0;
	}

	fout << "Configuration" << '\n';
	fout << "Directory     : " << directory << '\n';
	fout << "Steps         : " << steps << '\n';
	fout << "Adjustment    : " << adjust << '\n';
	fout << "Threads       : " << tnum << '\n';
	fout << "Window(min)   : " << swmin << '\n';
	fout << "Window(max)   : " << swmax << '\n';
	fout << "Resume mode   : " << resume << '\n';
	t = time( NULL );
	fout << "Time          : " << ctime( &t ) << '\n';
	fout << '\n';

	fout.close();

	return 1;
}

int Learn::LearnLog(){
/************************************************
ログの書き出し
************************************************/
	ofstream fout;

	fout.open( FNAME_EVINFO, ios::out | ios::app );
	if( fout.fail() ){
		cerr << "Open Error!..[" << FNAME_EVINFO << ']' << '\n';
		return 0;
	}

	cout << "Writing [" << FNAME_EVINFO << "]..";

	fout << "Step          : " << ( iteration + 1 ) << '\n';
	fout << "Search Window : " << info.swindow << '\n';
	fout << "Adjusts       : " << info.adjust << '\n';
	fout << "Records       : " << info.record << '\n';
	fout << "Loss          : " << info.loss0 << " - " << info.loss << '\n';
	fout << "Time(sec)     : " << info.time << '\n';
	fout << "Total(sec)    : " << info.total << '\n';
	fout << '\n';

	cout << "OK" << '\n';

	fout.close();

	return 1;
}

int Learn::Learn1(){
/************************************************
PVを求めるフェーズ
************************************************/
	Timer tm;
	int t;
	THREAD_INFO th[32];
#ifdef WIN32
	HANDLE hTh[32];
	HANDLE hMutex;      // Mutex
#else
	pthread_t tid[32];
	pthread_mutex_t mutex;
#endif
	int i;

	info.num = 0;
	info.loss_base = 0;

	tm.SetTime();

	fnum = 0;

	// ファイルを先頭から見ていく。
	flist.Begin();

	// スレッドの最大数調整
	if( tnum > (int)( sizeof(th) / sizeof(th[0]) ) ){
		tnum = (int)( sizeof(th) / sizeof(th[0]) );
	}

	// Mutex
#ifdef WIN32
	hMutex = CreateMutex( NULL, TRUE, NULL );
#else
	pthread_mutex_init( &mutex, NULL );
#endif

	// スレッドを沸かす。
	for( i = 1 ; i < tnum ; i++ ){
		th[i].num = i;
		th[i].plearn = this;
		th[i].pparam = NULL;
		th[i].pshogi = new ShogiEval( eval );

#ifdef WIN32
		th[i].hMutex = hMutex;
		hTh[i] = (HANDLE)_beginthreadex( NULL, 0, Learn1Thread, (LPVOID)&th[i], 0, NULL );
#else
		th[i].pmutex = &mutex;
		pthread_create( &tid[i], NULL, Learn1Thread, (void*)&th[i] );
#endif
	}

	th[0].num = 0;
	th[0].plearn = this;
	th[0].pparam = NULL;
	th[0].pshogi = new ShogiEval( eval );

#ifdef WIN32
	th[0].hMutex = hMutex;
	Learn1Thread( (LPVOID)&th[0] );
#else
	th[0].pmutex = &mutex;
	Learn1Thread( (void*)&th[0] );
#endif

	// スレッドの終了待ち
	for( i = 1 ; i < tnum ; i++ ){
#ifdef WIN32
		WaitForSingleObject( hTh[i], INFINITE );
		CloseHandle( hTh[i] );
#else
		pthread_join( tid[i], NULL );
#endif

		delete th[i].pshogi;
	}

	delete th[0].pshogi;

	// Mutexの破棄
#ifdef WIN32
	CloseHandle( hMutex );
#else
	// Linuxでは必要なし
	//pthread_mutex_destroy( &mutex );
#endif

	info.time = t = (int)tm.GetTime();

	cout << "Number of files : " << fnum << '\n';
	cout << "Time            : " << t << "sec" << '\n';
	cout.flush();

	return 1;
}

#ifdef WIN32
#	define MUTEX_LOCK		WaitForSingleObject( th.hMutex, INFINITE );
#	define MUTEX_UNLOCK		ReleaseMutex( th.hMutex );
#else
#	define MUTEX_LOCK		pthread_mutex_lock( th.pmutex );
#	define MUTEX_UNLOCK		pthread_mutex_unlock( th.pmutex );
#endif

#ifdef WIN32
unsigned __stdcall Learn::Learn1Thread( LPVOID p ){
#else
void* Learn::Learn1Thread( void* p ){
#endif
/************************************************
PVを求めるフェーズ(スレッド)
************************************************/
	string fname;
	string fname_nj;
	ostringstream str;
	THREAD_INFO& th = *((THREAD_INFO*)p);
	int flag;

	// 古いデータを削除
	str << FNAME_PVMOVE << th.num;
	remove( str.str().c_str() );

	while( 1 ){
		// 次のファイル
MUTEX_LOCK
		flag = th.plearn->flist.Pop( fname );
MUTEX_UNLOCK

		// ファイルがなければ終了
		if( flag == 0 )
			break;

		// ファイルを開く。
		if( 0 == th.pshogi->InputFileCSA( fname.c_str() ) ){
			continue;
		}

MUTEX_LOCK
		// Experimentation
		// 段階的な訓練例増加
		if( th.plearn->fnum >= th.plearn->info.record ){
MUTEX_UNLOCK
			break;
		}

		th.plearn->fnum++;
		cout << "Thread(" << th.num << "): "
			<< th.plearn->fnum << " : " << fname << '\n';
		cout.flush();
MUTEX_UNLOCK

#if SELECT_1ST_TIME_ONLY
		th.fname = fname;
#endif

		// 最善応手手順生成
		th.plearn->MakePVMoves( th );
	}

#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
}

void Learn::WriteMoves( ofstream& fout, const MOVE& move0, Moves& moves ){
/************************************************
指し手の書き込み
************************************************/
	unsigned int a;
	int i;

	a = moves.GetNumber() + 1;
	fout.write( (char*)&a, sizeof(a) );

	a = move0.Export();
	fout.write( (char*)&a, sizeof(a) );

	for( i = 0 ; i < moves.GetNumber() ; i++ ){
		a = moves[i].Export();
		fout.write( (char*)&a, sizeof(a) );
	}
}

int Learn::ReadMoves( ifstream& fin, Moves& moves ){
/************************************************
指し手の読み込み
************************************************/
	int num;
	unsigned int a;
	MOVE move;
	int i;

	moves.Init();

	fin.read( (char*)&num, sizeof(num) );

	if( num == 0 || fin.fail() || fin.eof() ){
		return 0; // 終端
	}

	if( num == -1 ){
		return -1; // 終端(レコード群)
	}

	for( i = 0 ; i < num ; i++ ){
		fin.read( (char*)&a, sizeof(a) );
		move.Import( a );

		moves.Add( move );
	}

	return num;
}

bool Learn::PVOk( THREAD_INFO& th, Moves& pv, int /*value*/, int /*alpha*/, int /*beta*/ ){
/************************************************
PVに問題がないか確認する。
************************************************/
#if 0
	KOMA sengo = th.pshogi->GetSengo();
#endif
	int know = th.pshogi->GetNumber();
	bool bOk = true;
	int illegal = 0;
	ostringstream str;
	int i;

	// 指し手を進めてみる。
	for( i = 0 ; i < pv.GetNumber() ; i++ ){
		// 合法手チェック
		if( !th.pshogi->Move( pv[i] ) ){
			// エラー
			bOk = false;
			illegal = i;
			break;
		}
	}

#if 0
	// 評価値のチェック
	// fail-lowやfail-highはやらない
	if( bOk && value > alpha && value < beta ){
		int vtemp = th.pshogi->GetValueN();
		if( sengo == th.pshogi->GetSengo() ){
			vtemp *= -1;
		}

		if( vtemp != value ){
			bOk = false;
			str << "Error! (" << __LINE__ << ") " << value << ' ' << vtemp << '\n';
			str << '\n';
		}
	}
#endif

	// 局面を戻しておく。
	while( know < th.pshogi->GetNumber() ){
		th.pshogi->GoBack();
	}

	// 非合法手の場合は局面と指し手を表示
	if( illegal != 0 )
	{
		str << "Error! (" << __LINE__ << ')' << '\n';
		str << th.pshogi->GetStrBan();
		for( i = 0 ; i < pv.GetNumber() ; i++ ){
			str << ' ';
			if( i == illegal ){
				str << '*';
			}
			str << Shogi::GetStrMove( pv[i], false );
		}
		str << '\n';
		str << '\n';
	}

	if( !bOk )
	{
		ofstream fout;
		time_t t = time( NULL );

MUTEX_LOCK
		fout.open( "pverr", ios::app | ios::out );
		fout << ctime( &t );
		fout << str.str();
		fout.close();
MUTEX_UNLOCK
	}

	return bOk;
}

int Learn::MakePVMoves( THREAD_INFO& th ){
/************************************************
最善応手手順生成
************************************************/
	ostringstream str;
	ofstream fout;
	Think* pthink;       // 思考部
	Moves moves;         // 着手可能手
	MOVE move0;          // 棋譜の指し手
	int val0, val;       // 評価値
	int depth;           // 探索深さ
	int fail_high;       // fail-highになった手の数
	int fail_low;        // fail-lowになった手の数
	bool is_last = true; // 棋譜の最後かどうか
	bool progress = ( tnum == 1 );
	int total = th.pshogi->GetTotal();

	// 局面
	const KOMA* ban;
	const int* dai;
	KOMA sengo;

	// 探索窓
	int alpha, beta;

	// PV
	Moves pv( MAX_DEPTH ); // PV(principal variation)
	bool pvOk;

	int i;
	bool is_max;

	pthink = new Think();
	pthink->IsLearning(); // 学習用

	// PVMOVEはスレッド毎に作成
	str << FNAME_PVMOVE << th.num;
	fout.open( str.str().c_str(), ios::out | ios::binary | ios::app );

#if SELECT_1ST_TIME_ONLY
	int snumber = -1;
	ifstream sin;
	ofstream sout;
	string sname = th.fname + ".select";
	sin.open( sname.c_str() );
	if( !sin ){
		sout.open( sname.c_str(), ios::out | ios::app );
	}
#endif

	while( th.pshogi->GetMove( move0 ) && th.pshogi->GoBack() ){
		bool is_first = true;

		// 勝った側の指し手だけを採用する。 2011/01/31
		if( is_last ){
			// 初回は1手だけ戻す。
			// 最後に指した方が勝ったはず。
			is_last = false;
		}
#if 0
		else{
			// それ以外は2手ずつ戻す。
			if( !( th.pshogi->GetMove( move0 ) && th.pshogi->GoBack() ) ){
				break;
			}
		}
#endif

#if SELECT_RAND
#if SELECT_1ST_TIME_ONLY
		if( sin ){
			if( snumber == -1 ){
				char str[16];
				sin.getline( str, sizeof(str) );
				if( !sin ){ break; }
				snumber = strtol( str, NULL, 10 );
			}
			if( snumber != th.pshogi->GetNumber() ){
				continue;
			}
			snumber = -1;
		}
		else{
#endif
			// ランダムに局面を選ぶ。
			// 確率1/8
			if( gen_rand32() % 8 != 0 ){
				continue;
			}
#if SELECT_1ST_TIME_ONLY
			sout << th.pshogi->GetNumber() << '\n';
		}
#endif
#endif

		// ハッシュテーブルの初期化
		pthink->InitHashTable();

		moves.Init();
		th.pshogi->GenerateMoves( moves );

		if( moves.GetNumber() < 2 ){
			continue;
		}

		// 棋譜の指し手は除く
		moves.Remove( move0 );

MUTEX_LOCK
		info.num++;
MUTEX_UNLOCK

		fail_high = 0;
		fail_low = 0;
#if 1
		const int depth1st = 1;
#else
		const int depth1st = L_DEPTH;
#endif
		for( depth = depth1st ; depth <= L_DEPTH ; depth++ ){
			// 最後かどうか
			is_max = ( depth == L_DEPTH );

			// もう指し手が無ければ終了
			if( moves.GetNumber() == 0 ){
				break;
			}

			// 棋譜の指し手
			th.pshogi->MoveD( move0 );
			val0 = -pthink->NegaMax( th.pshogi, depth,
				VMIN, VMAX, NULL, NULL );
			pthink->GetPVMove( pv );
			pvOk = PVOk( th, pv, val0, VMIN, VMAX );
			th.pshogi->GoBack();

			// 詰みだったら使わない。
			if( val0 >= VMAX || val0 <= VMIN ){
MUTEX_LOCK
				info.num--;
MUTEX_UNLOCK
				break;
			}

			// PVに問題あり
			if( !pvOk ){
MUTEX_LOCK
				info.num--;
MUTEX_UNLOCK
				break;
			}

			// search window

			// alpha値
			alpha = val0 - swindow;
			if( alpha < VMIN ) alpha = VMIN;

			// beta値
			if( is_max ){
				// 最後はval0+ウィンドウ幅
				beta  = val0 + swindow;
				if( beta  > VMAX ) beta  = VMAX;
			}
			else{
				// 最後以外はval0
				beta = val0;
			}

			// 初回でない場合
			if( !is_first ){
				// レコードの終端
				int a = 0;
				fout.write( (char*)&a, sizeof(a) );
			}

			// レコードあり
			is_first = false;

			// 局面の保存 (レコードの先頭)
			ban = th.pshogi->GetBan();
			dai = th.pshogi->GetDai();
			sengo = th.pshogi->GetSengo();
			for( i = 0x11 ; i <= 0x91 ; i += 0x10 )
				fout.write( (const char*)&ban[i], sizeof(ban[0]) * 9 );
			fout.write( (const char*)&dai[SFU], sizeof(dai[0]) * ( HI - FU + 1 ) );
			fout.write( (const char*)&dai[GFU], sizeof(dai[0]) * ( HI - FU + 1 ) );
			fout.write( (const char*)&sengo, sizeof(sengo) );

			// PVの保存
			WriteMoves( fout, move0, pv );
	
			// その他の手を調べる。
			for( i = 0 ; i < moves.GetNumber() ; i++ ){
				th.pshogi->MoveD( moves[i] );
				val = -pthink->NegaMax( th.pshogi, depth,
					-beta, -alpha, NULL, NULL );
				pthink->GetPVMove( pv );
				pvOk = PVOk( th, pv, val, alpha, beta );
				th.pshogi->GoBack();

				// fail-high
				if( val >= beta ){
					// 不一致度の計算のためにfail-highをカウント
					if( is_max ){
						fail_high++;
					}
				}
				// 探索窓の範囲内
				else if( val > alpha && pvOk )
				{
					// PVを保存してmovesから除外
					WriteMoves( fout, moves[i], pv ); // 保存
					moves.Remove( i );                // 削除
					i--;                              // 消した分を戻す。
				}
				// fail-low
				else{
					// movesから除外
					moves.Remove( i ); // 削除
					i--;               // 消した分を戻す。
					fail_low++;        // fail-lowをカウント
				}
			}
		}

		if( !is_first ){
			// レコード群の終端
			int a = -1;
			fout.write( (char*)&a, sizeof(a) );

			// fail-high, fail-low
			fout.write( (char*)&fail_high, sizeof(fail_high) );
			fout.write( (char*)&fail_low , sizeof(fail_low ) );
		}

MUTEX_LOCK
		info.loss_base += fail_high;
MUTEX_UNLOCK

		if( progress ){
			Progress::Print( 100 - th.pshogi->GetNumber() * 100 / total );
		}
	}

	if( progress ){
		Progress::Clear();
	}

	fout.close();

#if SELECT_1ST_TIME_ONLY
	if( sin.is_open()  ){ sin.close(); }
	if( sout.is_open() ){ sout.close(); }
#endif

	delete pthink;

	return 1;
}
 
int Learn::Learn2(){
/************************************************
パラメータの更新を行うフェーズ
************************************************/
	THREAD_INFO th[32];
#ifdef WIN32
	HANDLE hTh[32];
	HANDLE hMutex;      // Mutex
#else
	pthread_t tid[32];
	pthread_mutex_t mutex;
#endif
	int i;
	int ret = 1;

	info.loss = info.loss_base;

	// Mutex
#ifdef WIN32
	hMutex = CreateMutex( NULL, TRUE, NULL );
#else
	pthread_mutex_init( &mutex, NULL );
#endif

	// スレッドを沸かす。
	for( i = 1 ; i < tnum ; i++ ){
		th[i].num = i;
		th[i].plearn = this;
		th[i].pshogi = new ShogiEval( eval );
		th[i].pparam = new Param();
#ifdef WIN32
		th[i].hMutex = hMutex;
		hTh[i] = (HANDLE)_beginthreadex( NULL, 0, Learn2Thread, (LPVOID)&th[i], 0, NULL );
#else
		th[i].pmutex = &mutex;
		pthread_create( &tid[i], NULL, Learn2Thread, (void*)&th[i] );
#endif
	}

	th[0].num = 0;
	th[0].plearn = this;
	th[0].pshogi = new ShogiEval( eval );
	th[0].pparam = new Param();
#ifdef WIN32
	th[0].hMutex = hMutex;
	Learn2Thread( (LPVOID)&th[0] );
#else
	th[0].pmutex = &mutex;
	Learn2Thread( (void*)&th[0] );
#endif

	// スレッドの終了待ち
	for( i = 1 ; i < tnum ; i++ ){
#ifdef WIN32
		WaitForSingleObject( hTh[i], INFINITE );
		CloseHandle( hTh[i] );
#else
		pthread_join( tid[i], NULL );
#endif

		delete th[i].pshogi;
	}

	delete th[0].pshogi;

	// Mutexの破棄
#ifdef WIN32
	CloseHandle( hMutex );
#else
	// Linuxでは必要なし
	//pthread_mutex_destroy( &mutex );
#endif

	// 勾配を足しあわせる。
	th[0].pparam->CumulateR();
	for( i = 1 ; i < tnum ; i++ ){
		th[i].pparam->CumulateR();
		*(th[0].pparam) += *(th[i].pparam);
		delete th[i].pparam;
	}

	// 左右対称化
	th[0].pparam->Symmetry();

	if( mode == MODE_GRADIENT ){
		// 勾配ベクトルの保存
		if( 0 == th[0].pparam->WriteData( FNAME_GRADIENT ) ){
			ret = 0;
		}
	}
	else{
		// パラメータの更新
		AdjustParam( th[0].pparam );

		// パラメータの保存
		if( 0 == eval->WriteData( FNAME_EVDATA) )
			ret = 0;

		// backup
		if( 0 == eval->WriteData( GetBackupName() ) ){
		}
	}

	delete th[0].pparam;

	info.loss /= info.num;

	return ret;
}

#ifdef WIN32
unsigned __stdcall Learn::Learn2Thread( LPVOID p ){
#else
void* Learn::Learn2Thread( void* p ){
#endif
/************************************************
パラメータの更新を行うフェーズ(スレッド)
************************************************/
	THREAD_INFO& th = *((THREAD_INFO*)p);

	th.plearn->Gradient( th );

#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
}

int Learn::Gradient( THREAD_INFO& th ){
/************************************************
勾配ベクトル
************************************************/
	ifstream fin;
	ofstream fdetail;
	ShogiEval* pshogi = th.pshogi;
	Param* pparam = th.pparam;
	int i;

	// 最善応手手順
	Moves bmove( MAX_DEPTH );
	Moves bmove0( MAX_DEPTH );

	// 局面
	KOMA _ban0[16*13] = {
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,
		WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL,WALL
	};
	KOMA* ban0 = _ban0 + BAN_OFFSET;
	int dai0[64] = { 0 };
	KOMA sengo;

	int val0, val;
	double d, dsum;

#if EXP_PARENT_CHILD
	int valR;
	double d0R, dsum0R, dsumR;
#endif

	uint64 hash   = U64(0);
	bool is_same  = false;
	int fail_high = 0;
	int fail_low  = 0;

	// PVMOVEはスレッド毎に作成
	{
		ostringstream str;
		str << FNAME_PVMOVE << th.num;
		fin.open( str.str().c_str(), ios::in | ios::binary );
	}

	// 詳細
	if( detail ){
		ostringstream str;
		str << FNAME_DETAIL << th.num;
		fdetail.open( str.str().c_str() );
	}

	if( fin.fail() ){
		cerr << "Open Error!..[" << FNAME_PVMOVE << ']' << '\n';
		fin.close();
		return 0;
	}

	while( 1 ){
		// 局面の読み込み
		for( i = 0x11 ; i <= 0x91 ; i += 0x10 ){
			fin.read( (char*)&ban0[i], sizeof(ban0[0]) * 9 );
		}

		if( fin.fail() || fin.eof() ){ break; } // EOF

		fin.read( (char*)&dai0[SFU], sizeof(dai0[0]) * ( HI - FU + 1 ) );
		fin.read( (char*)&dai0[GFU], sizeof(dai0[0]) * ( HI - FU + 1 ) );
		fin.read( (char*)&sengo, sizeof(sengo) );
		pshogi->SetBan( _ban0, dai0, sengo );

		// 前のレコードと同じ局面か
		if( hash != pshogi->GetHash() ){
			hash      = pshogi->GetHash();
			is_same   = false;
			fail_high = 0;
			fail_low  = 0;
		}
		else{
			is_same   = true;
		}

		// 詳細
		if( detail && !is_same ){
			fdetail << pshogi->GetStrBanNum();
		}

		/*************** ルート局面 ***************/
#if EXP_PARENT_CHILD
		// Experimentation
		// 親子の評価値を近づける実験
		valR = pshogi->GetValueN();
		assert( sengo == pshogi->GetSengo() );
#endif

		/*************** 棋譜の指し手 ***************/

		// 最善応手手順読み込み
		ReadMoves( fin, bmove0 );

		// 指し手を進める。
		for( i = 0 ; i < bmove0.GetNumber() ; i++ ){
			pshogi->MoveD( bmove0.GetMove( i ) );
		}

		// 評価値
		val0 = pshogi->GetValueN();
		if( sengo != pshogi->GetSengo() ){
			val0 = -val0;
		}

#if EXP_PARENT_CHILD
		// Experimentation
		// 親子の評価値を近づける実験
		d = dLossPC( val0 - valR,bmove0.GetNumber() ) * PC_LAMBDA0; // dLossPC( 末端 - ルート )
		if( sengo == GOTE ){ d = -d; }
		d0R = d;
#endif

		// 局面を戻す。
		while( pshogi->GoBack() )
			;

		// 詳細
		if( detail && !is_same && pshogi->IsLegalMove( bmove0[0] ) ){
			fdetail << pshogi->GetStrMoveNum( bmove0[0] );
			fdetail << '\n';
		}

		/**************** その他の手 ****************/
		dsum = 0.0;
#if EXP_PARENT_CHILD
		dsum0R = 0.0;
		dsumR = 0.0;
#endif
		while( 1 ){
			// 最善応手手順読み込み
			int ret = ReadMoves( fin, bmove );

			// レコードの終端
			if( ret == 0 ){ break; }

			// レコード群の終端
			if( ret == -1 ){
				fin.read( (char*)&fail_high, sizeof(fail_high) );
				fin.read( (char*)&fail_low , sizeof(fail_low ) );
				if( detail ){
					fdetail << "fail-high," << fail_high << '\n';
					fdetail << "fail-low," << fail_low << '\n';
				}
				break;
			}

			// 指し手を進める。
			for( i = 0 ; i < bmove.GetNumber() ; i++ ){
				pshogi->MoveD( bmove.GetMove( i ) );
			}

			// 評価値
			val = pshogi->GetValueN();
			if( sengo != pshogi->GetSengo() ){
				val = -val;
			}

			// 損失
MUTEX_LOCK
			info.loss += Loss( val0 - val );
MUTEX_UNLOCK

			// 勾配
			d = dLoss( val0 - val );
			if( sengo == GOTE ){ d = -d; }
			dsum += d;
			IncParam( pshogi, pparam, -d );

#if EXP_PARENT_CHILD
			// Experimentation
			// 親子の評価値を近づける実験
			d = dLossPC( val - valR, bmove.GetNumber() ) * PC_LAMBDA; // dLossPC( 末端 - ルート )
			if( sengo == GOTE ){ d = -d; }
			dsum0R += d0R;
			dsumR += d;

			// 末端局面 勾配が正 => 小さく
			IncParam2( pshogi, pparam, -d );
#endif

			// 局面を戻す。
			while( pshogi->GoBack() )
				;

			// 詳細
			if( detail && pshogi->IsLegalMove( bmove[0] ) ){
				fdetail << pshogi->GetStrMoveNum( bmove[0] );
				fdetail << ',' << ( val - val0 ) << '\n';
			}
		}

		/*************** ルート局面 ***************/
#if EXP_PARENT_CHILD
		// Experimentation
		// 親子の評価値を近づける実験

		// ルート局面 勾配が正 => 大きく
		IncParam2( pshogi, pparam, dsumR + dsum0R );
#endif

		/*************** 棋譜の指し手 ***************/

		// 指し手を進める。
		for( i = 0 ; i < bmove0.GetNumber() ; i++ ){
			pshogi->MoveD( bmove0.GetMove( i ) );
		}

		IncParam( pshogi, pparam, dsum );

#if EXP_PARENT_CHILD
		// Experimentation
		// 親子の評価値を近づける実験

		// 末端局面 勾配が正 => 小さく
		IncParam2( pshogi, pparam, -dsum0R );
#endif

		// 局面を戻す。
		while( pshogi->GoBack() )
			;
	}

	fin.close();

	// 詳細
	if( detail ){ fdetail.close(); }

	return 1;
}

double Learn::Loss( double x ){
	const double swin = DEF_SWINDOW;
	const double dt = swin / 7.0;
	if( x > swin ) x = swin;
	else if( x < -swin ) x = -swin;
	return 1.0 / ( 1.0 + exp( x / dt ) );
}

double Learn::dLoss( double x ){
	const double swin = DEF_SWINDOW;
	const double dt = swin / 7.0;
	double d;
	if( x >= swin || x <= -swin )
		return 0.0;
	x = exp( x / dt );
	d = x + 1.0;
	return x / ( dt * d * d );
}

#if EXP_PARENT_CHILD
double Learn::dLossPC( double x, double depth ){
/************* type 0 *************/
#if   PC_TYPE == 0
#define PC_W					NON  // 重み
#define PC_A					NON  // 大きいほど傾きが急になる。
#define PC_B					NON  // マージン
	double d;
	d = 1.0 / ( 1.0 + exp( -PC_A * ( x - PC_B ) ) );
	return -PC_W * d * ( 1.0 - d ) * ( 1.0 - 2.0 * d );
#undef PC_W
#undef PC_A
#undef PC_B
/************* type 1 *************/
#elif PC_TYPE == 1
#define PC_A					NON
	return PC_A * x / depth / depth;
#undef PC_A
/************* type 2 *************/
#elif PC_TYPE == 2
#define PC_A					NON
	if     ( x > 0.0 ){ return  PC_A / depth; }
	else if( x < 0.0 ){ return -PC_A / depth; }
	else              { return  0.0; }
/************* type 3 *************/
#elif PC_TYPE == 3
#define PC_A					NON
	return PC_A * x / depth;
#undef PC_A
/************* type 4 *************/
#elif PC_TYPE == 4
#define PC_A					NON
	if     ( x > 0.0 ){ return  PC_A / sqrt(depth); }
	else if( x < 0.0 ){ return -PC_A / sqrt(depth); }
	else              { return  0.0; }
#else              // unkown type
#error
#endif
/**********************************/
}
#endif

int comp_int( const void* p1, const void* p2 );

void Learn::IncParam( ShogiEval* pshogi, Param* p, double delta ){
/************************************************
特徴抽出
************************************************/
	const KOMA* ban;
	const int* dai;
	int addr;
	int i, j;
	KOMA koma;

	// 駒割り以外
	IncParam2( pshogi, p, delta );

	// 局面
	ban = pshogi->GetBan();
	dai = pshogi->GetDai();

	// 盤上の駒
	for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
		for( i = 1 ; i <= 9 ; i++ ){
			addr = i + j;
			if( ( koma = ban[addr] ) != EMPTY ){
				if     ( koma & SENTE && koma != SOU ){
					p->piece[koma-SFU] += delta;
				}
				else if( koma & GOTE  && koma != GOU ){
					p->piece[koma-GFU] -= delta;
				}
			}
		}
	}

	// 持ち駒
	p->piece[SFU-SFU] += delta * ( dai[SFU] - dai[GFU] );
	p->piece[SKY-SFU] += delta * ( dai[SKY] - dai[GKY] );
	p->piece[SKE-SFU] += delta * ( dai[SKE] - dai[GKE] );
	p->piece[SGI-SFU] += delta * ( dai[SGI] - dai[GGI] );
	p->piece[SKI-SFU] += delta * ( dai[SKI] - dai[GKI] );
	p->piece[SKA-SFU] += delta * ( dai[SKA] - dai[GKA] );
	p->piece[SHI-SFU] += delta * ( dai[SHI] - dai[GHI] );
}

#define FUNC_S(x)			(p->x += delta)
#define FUNC_G(x)			(p->x -= delta)
#define FUNC_SO(x)			(p->x += delta)
#define FUNC_GO(x)			(p->x -= delta)
#define DEGREE_S(x,y,z)		(GetDegreeOfFreedomS(delta,ban,diffS,diffG,addr,(x),p->y,p->z))
#define DEGREE_G(x,y,z)		(GetDegreeOfFreedomG(delta,ban,diffS,diffG,addr,(x),p->y,p->z))
#define S					(Param::S)
#define G					(Param::G)
void Learn::IncParam2( ShogiEval* pshogi, Param* p, double delta ){
/************************************************
特徴抽出
2010/10/20 evaluate.cpp の _GetValue1 とソースを統合(feature.h)
2011/08/20 駒割りと分離
************************************************/
#define __LEARN__
#include "feature.h"
#undef __LEARN__
}
#undef FUNC_S
#undef FUNC_G
#undef FUNC_SO
#undef FUNC_GO
#undef DEGREE_S
#undef DEGREE_G

double Learn::Penalty( Param* p ){
/************************************************
ペナルティの計算
************************************************/
	int i;
	double penalty;

	penalty = 0.0;
	for( i = 0 ; i < PARAM_NUM ; i++ )
		penalty += fabs( p->PARAM[i] );

	return penalty / PARAM_NUM * 0.001;
}

void Learn::AdjustParam( Param* p ){
/************************************************
パラメータの更新
************************************************/
	int i, j;
	double* v[13];
	double* temp;
	int a;

#if !NPENALTY
	// ペナルティ
	double penalty = NON;
#else
	double penalty = 0.0;
#endif

#if 0
	// 駒の価値
	v[ 0] = &p->piece[FU-FU];
	v[ 1] = &p->piece[KY-FU];
	v[ 2] = &p->piece[KE-FU];
	v[ 3] = &p->piece[GI-FU];
	v[ 4] = &p->piece[KI-FU];
	v[ 5] = &p->piece[KA-FU];
	v[ 6] = &p->piece[HI-FU];
	v[ 7] = &p->piece[TO-FU];
	v[ 8] = &p->piece[NY-FU];
	v[ 9] = &p->piece[NK-FU];
	v[10] = &p->piece[NG-FU];
	v[11] = &p->piece[UM-FU];
	v[12] = &p->piece[RY-FU];

	// 昇順にソート
	for( i = 1 ; i < 13 ; i++ ){
		temp = v[i];
		for( j = i ; j > 0 ; j-- ){
			if( *temp < *v[j-1] )
				v[j] = v[j-1];
			else
				break;
		}
		v[j] = temp;
	}

	// シャッフル
	// 0 から 5
	for( i = 5 ; i > 0 ; i-- ){
		j = (int)( gen_rand32() % i );
		temp = v[i];
		v[i] = v[j];
		v[j] = temp;
	}

	// 6 から 12
	for( i = 12 ; i > 6 ; i-- ){
		j = (int)( gen_rand32() % ( i - 6 ) ) + 6;
		temp = v[i];
		v[i] = v[j];
		v[j] = temp;
	}

	*v[ 0] = *v[ 1]          = -2.0;
	*v[ 2] = *v[ 3] = *v[ 4] = -1.0;
	*v[ 5] = *v[ 6] = *v[ 7] =  0.0;
	*v[ 8] = *v[ 9] = *v[10] =  1.0;
	*v[11] = *v[12]          =  2.0;

	p->piece[OU-FU] = 0.0;
	p->piece[KI+NARI-FU] = 0.0;

	for( i = 0 ; i < RY ; i++ ){
		// パラメータ更新
		eval->Add1( i, (int)p->piece[i] );
	}
#endif

	// 駒割り以外
	for( i = 0 ; i < PARAM_NUM ; i++ ){
		// 更新値決定
		a = AdjustmentValue( eval->GetV_PARAM2( i ), p->PARAM[i], penalty );

		// パラメータ更新
		eval->Add2( i, a );
	}

	// 左右対称化
	eval->Symmetry();

	eval->Update();
}

int Learn::AdjustmentValue( int v, double d, double penalty ){
	int r;
	int x;

	r = gen_rand32();
	x  = ( r >> 5 ) & 0x01;
	x += ( r >> 6 ) & 0x01;

	if     ( v > 0 ) d -= penalty;
	else if( v < 0 ) d += penalty;

	if( d > 0.0 ){
		return  x;
	}

	else if( d < 0.0 ){
		return -x;
	}

	return 0;
}

#endif
