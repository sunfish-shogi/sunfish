/* think.cpp
 * R.Kubo 2008-2012
 * 将棋の思考ルーチン
 */

#include "match.h"
#include <cmath>

#ifdef SIKOU_DLL
#	include <sstream>
	void AddProgress( const char* str );
#elif defined(USI)
	extern bool isPonderThread;
	void setMoveString( char* p, size_t size, const MOVE& move );
#endif

PVMove::PVMove(){
/************************************************
コンストラクタ
************************************************/
	bnum = new int[MAX_DEPTH];
	bmove = new MOVE[MAX_DEPTH][MAX_DEPTH];
}

PVMove::~PVMove(){
/************************************************
デストラクタ
************************************************/
	delete [] bnum;
	delete [] bmove;
}

void PVMove::Copy( int depth, PVMove* source, int depth2, const MOVE& move ){
/************************************************
最前手順を親ノードへコピー
************************************************/
	bmove[depth][0] = move;
	if( depth2 < MAX_DEPTH - 1 ){
		bnum[depth] = source->bnum[depth2] + 1;
		memcpy( &bmove[depth][1], &source->bmove[depth2][0],
			sizeof(MOVE) * source->bnum[depth2] );
	}
	else{
		bnum[depth] = 1;
	}
}

void PVMove::PrintPVMove( int mdepth, int depth, int value ){
/************************************************
最前手順を出力
************************************************/
	int i;

#ifdef SIKOU_DLL // sikou.dll(CSA将棋)
	ostringstream str;

	str << mdepth << ": ";
	for( i = 0 ; i < bnum[depth] ; i++ )
		str << Shogi::GetStrMoveSJIS( bmove[depth][i] );
	str << ':' << value;
#if !defined(NDEBUG)
	Debug::Print( "%s(%d) %s\n", __FILE__, __LINE__, str.str().c_str() );
#endif
	AddProgress( str.str().c_str() );

#elif defined(USI) // usi.exe(USIプロトコル)
	char mstr[6];
	cout << "info";
	if( !isPonderThread ){ cout << " score cp " << value; }
	cout << " depth " << mdepth;
	cout << " pv";
	for( i = 0 ; i < bnum[depth] ; i++ ){
		setMoveString( mstr, sizeof(mstr), bmove[depth][i] );
		cout << ' ' << mstr;
	}
	cout << '\n';
	cout.flush();

#else // CUI
	cout << mdepth << ": ";
	for( i = 0 ; i < bnum[depth] ; i++ )
		Shogi::PrintMove( bmove[depth][i] );
	cout << ':' << value << '\n';
	cout.flush();
#endif
}

Think::Think( int _thread_num ){
/************************************************
コンストラクタ
************************************************/
	if( _thread_num < 1 ){
		thread_num = 1;
	}
	else if( _thread_num > MAX_THREADS ){
		thread_num = MAX_THREADS;
	}
	else{
		thread_num = _thread_num;
	}
	tree_num = TREE_NUM( thread_num );
	_worker_num = worker_num = WORKER_NUM( thread_num );

	trees = new Tree[tree_num];
	workers = new WORKER[worker_num];
	hash_table = new HashTable();
	dfpn_table = new DfPnTable();
	history = new History();

	// 並列化
	parallel = false;

#if FUT_EXT
	// futility pruning のマージン
	for( int mvcnt = 0 ; mvcnt < FUT_MOVE_MAX ; mvcnt++ ){
		fut_mgn[0][mvcnt] = 0;
		for( int d = 1 ; d < DINC * MAX_DEPTH ; d++ ){
#if 1
			fut_mgn[d][mvcnt] = (int)( 64.0 * log( (double)d * d / 128.0 ) / log(2.0) + 50.0 + 4.0 * mvcnt );
#else
			fut_mgn[d][mvcnt] = (int)( 64.0 * log( (double)d * d / 128.0 ) / log(2.0) + 50.0 + 2.0 * mvcnt );
#endif
		}
	}
#if MOVE_COUNT_PRUNING
	// move count pruningのしきい値
	for( int d = 0 ; d < DINC * MAX_DEPTH ; d++ ){
		move_count[d] = int( 16.0 + 0.02 * (d+DINC) * (d+DINC) );
	}
#endif
#endif

	// mutex
#ifdef WIN32
	// InterlockedExchangeを使用するように変更
	//hMutex_split = CreateMutex( NULL, TRUE, NULL );
	mutex_split = 0;
#else
	pthread_mutex_init( &mutex_split, NULL );
#endif

	// 時間制限
	limit_max = 0;
	limit_min = 0;

	// ノード数制限
	limit_node = ULLONG_MAX;

#ifndef NLEARN
	// 学習中か
	is_learning = false;
#endif
}

Think::~Think(){
/************************************************
デストラクタ
************************************************/
	delete [] trees;
	delete [] workers;
	delete hash_table;
	delete dfpn_table;
	delete history;

#ifdef WIN32
	// InterlockedExchangeを使用するように変更
	//CloseHandle( hMutex_split );
#else
	// Linuxでは必要なし
	//pthread_mutex_destroy( &mutex_split );
#endif
}

void Think::SetNumberOfThreads( int _thread_num ){
/************************************************
スレッド数変更
************************************************/
	// いったん解放
	delete [] trees;
	delete [] workers;

	// 再確保
	if( _thread_num < 1 ){
		thread_num = 1;
	}
	else if( _thread_num > MAX_THREADS ){
		thread_num = MAX_THREADS;
	}
	else{
		thread_num = _thread_num;
	}
	tree_num = TREE_NUM( thread_num );
	_worker_num = worker_num = WORKER_NUM( thread_num );

	trees = new Tree[tree_num];
	workers = new WORKER[worker_num];
}

void Think::PrintSearchInfo( const char* str ){
/************************************************
探索中の情報表示
************************************************/
#ifdef SIKOU_DLL // sikou.dll(CSA将棋)
	AddProgress( str );

#elif defined(USI) // usi.exe(USIプロトコル)
	cout << "info string " << str << '\n';

#else // CUI
	cout << str << '\n';
#endif
}

int Think::Quiescence( Tree* tree, int depth, int qdepth, int alpha, int beta )
/************************************************
静止探索
************************************************/
{
	MOVE* pmove;
	int stand_pat;
	int value;
	int val;

	tree->InitNode( depth );

	// スタックのサイズをオーバー
	if( depth >= MAX_DEPTH ){
		return tree->GetValueN();
	}

#if 0 // tacticalな手のみを調べるのにいらない気がする。
	// 千日手判定
	switch( tree->IsRepetition() ){
	case IS_REP: return 0;
	case PC_WIN: return VMAX;
	case PC_LOS: return VMIN;
	}
#endif

	tree->cnts.node++;

	// ノード数打ち切り
	if( info.node >= limit_node ){
		tree->abort = true;
		return 0;
	}

	// 静的評価
	stand_pat = tree->GetValueN();

	if( stand_pat >= beta ){
		return stand_pat;
	}

#if 1 // 1ply
	if( !tree->IsCheck() ){
		// 1手詰みのチェック
		if( Mate1Ply( tree, NULL ) ){
			return VMAX;
		}
	}
#endif

	// Tacticalな手を生成
	// 但し, 深さ7からはCaptureに限定
	tree->InitMoves();
	if( qdepth < 7 ){
		tree->GenerateTacticalMoves();
	}
	else{
		tree->GenerateCaptures();
	}
	value = stand_pat;
	if( tree->GetMovesNumber() > 0 ){
		// 並べ替え
		tree->SortSeeQuies();

		while( ( pmove = tree->GetNextMove( history
#if KILLER
			, depth
#endif
			) ) != NULL
		){
			int alpha_new = max( value, alpha );

#if 1
			// futility pruning
			if( !tree->IsCheck() && !tree->IsCheckMove( *pmove ) ){
				int estimate = stand_pat + tree->EstimateValue( *pmove );
				if( estimate <= alpha_new ){
					tree->cnts.futility_cut++;
					continue;
				}
			}
#endif

			// Make move
			tree->MakeMove( *pmove );

			val = -Quiescence( tree, depth + 1, qdepth + 1, -beta, -alpha_new );

			// Unmake move
			tree->UnmakeMove();

			// 打ち切りフラグ
			if( INTERRUPT ){ return 0; }

			if( val > value ){
				value = val;
				tree->PVCopy( depth, *pmove );
				if( value >= beta ){
					// beta cut
					return value;
				}
			}
		}
	}

#if 1 // 3ply
	if( qdepth < 7 && !tree->IsCheck() ){
		// 3手詰みのチェック
		if( Mate3Ply( tree ) ){
			return VMAX;
		}
	}
#endif

	return value;
}

#define RAZOR_DEPTH				( 3 * DINC )

int Think::NegaMaxR( Tree* __restrict__ tree, int depth, int rdepth, int alpha, int beta, unsigned state )
/************************************************
NegaMax探索
************************************************/
{
	// stand pat
	int stand_pat = VMAX;
#define SVALUE			((stand_pat)==(VMAX)?((stand_pat)=(tree->GetValueN())):(stand_pat))

	tree->InitNode( depth );

	// スタックのサイズをオーバー
	if( depth >= MAX_DEPTH ){
		return tree->GetValueN();
	}

	// 深さ1でやると、相手番の王手千日手がわからない。
	if( depth >= 2
#ifndef NLEARN
	// 学習時はPVの末端の静的評価値を返さなければいけない
	&& !is_learning
#endif
	){
		// SHEK "強度の水平線効果"対策
		switch( tree->ShekCheck() ){
		case SHEK_FIRST:  // 初めての局面
			break;
		case SHEK_EQUAL:  // 等しい局面
			// 千日手判定
			switch( tree->IsRepetition() ){
			case IS_REP: return 0;
			case PC_WIN: return VMAX;
			case PC_LOS: return VMIN;
			}
			return 0;
		case SHEK_PROFIT: // 得な局面
			return VMAX;
		case SHEK_LOSS:   // 損な局面
			return VMIN;
		}
	}

	if( !tree->IsCheck() && rdepth < DINC ){
		// 静止探索
		return Quiescence( tree, depth, 1, alpha, beta );
	}

	tree->cnts.node++;

	// ノード数打ち切り
	if( info.node >= limit_node ){
		tree->abort = true;
		return 0;
	}

	// ハッシュを調べる。
	bool hash_ok = false;
	uint64 hash = tree->GetHash();
	int hash_value = VMIN;
	VALUE_TYPE hash_type = VALUE_NULL;

#ifndef NLEARN
	// 学習時はPVの末端の静的評価値を返さなければいけない
	if( !is_learning )
#endif
	{
		switch( hash_table->Check( hash, rdepth, hash_type, hash_value ) ){
		case VALUE_LOWER:
			// lower (fail-high)
			if( hash_value >= beta /*&& beta == alpha + 1*/ ){
				tree->cnts.hash_cut++;
				return hash_value;
			}
			hash_ok = true;
			break;
		case VALUE_UPPER:
			// upper (fail-low)
			if( hash_value <= alpha /*&& beta == alpha + 1*/ ){
				tree->cnts.hash_cut++;
				return hash_value;
			}
			else if( hash_value < beta ){
				state &= ~STATE_NULL;
			}
			hash_ok = true;
			break;
		case VALUE_EXACT:
			// exact
			tree->cnts.hash_cut++;
			return hash_value;
		case VALUE_SHALL:
			// shallow
			hash_ok = true;
			break;
		case VALUE_NULL:
			break;
		}
	}
#ifndef NLEARN
	else{
		// 学習時
		if( hash_table->Check( hash, rdepth, hash_type, hash_value ) != VALUE_NULL ){
			hash_ok = true;
		}
	}
#endif

	// PV-nodeか
	bool is_pv = ( alpha == root_alpha && beta == root_beta   )
	     || ( alpha == -root_beta && beta == -root_alpha );

#if 1 // 1ply
	if( !tree->IsCheck() ){
#if 1 // 3ply
		// 3手詰みのチェック
		if( is_pv ){
			if( Mate3Ply( tree ) ){
				return VMAX;
			}
		}
		else
#endif
		// 1手詰みのチェック
		if( Mate1Ply( tree, NULL ) ){
			return VMAX;
		}
	}
#endif

#if RAZORING
	// razoring
	if( alpha + 1 == beta
		&& rdepth < RAZOR_DEPTH
		&& !tree->IsCheck()
		&& SVALUE + GetRazorMgn( rdepth ) < beta
		&& !hash_ok
	){
		int rbeta = beta - GetRazorMgn( rdepth );
		int vtemp = Quiescence( tree, depth, 1, rbeta-1, rbeta );
		if( vtemp < rbeta ){
			tree->cnts.razoring++;
			return vtemp
#if 1
				+ GetRazorMgn( depth )
#endif
				;
		}
	}
#endif

#if STATIC_NULL_MOVE
	// static null move pruniing
	if( alpha + 1 == beta
		&& state & STATE_NULL
		&& rdepth < RAZOR_DEPTH
		&& !tree->IsCheck()
		&& SVALUE - GetFutMgn( rdepth, 0 ) >= beta
	){
		tree->cnts.s_nullmove_cut++;
		return SVALUE - GetFutMgn( rdepth, 0 );
	}
#endif

	// キラームーブ初期化
#if KILLER
	tree->InitKiller( depth + 1 );
#endif

#if MOVE_COUNT_PRUNING
	// move count pruning
	MOVE threat;
	threat.from = EMPTY;
#endif

	// Null Move Pruning
	// 条件変更 2010/10/22
	{
		int ndepth = rdepth - ( rdepth <= 4 * DINC ? 2 * DINC : 
			( rdepth <= 6 ? 4 * DINC : 6 * DINC ) );
		if( state & STATE_NULL    // state of this node
#if 0
			&& alpha + 1 == beta  // null window search
#endif
			&& depth < ndepth     // depth
			&& beta < SVALUE      // stand pat
		){
			// Make move
			if( tree->MakeNullMove() ){
				unsigned state_new = STATE_ROOT & (~STATE_NULL);
				int nval = -NegaMaxR( tree, depth + 1, ndepth, -beta, -beta+1, state_new );

				// 打ち切りフラグ 2010/10/20
				if( INTERRUPT ){
					tree->UnmakeNullMove();
					return 0;
				}

				if( nval >= beta ){
					tree->cnts.nullmove_cut++;
					tree->UnmakeNullMove();
					return nval;
				}
				else{
#if MOVE_COUNT_PRUNING
					tree->GetBestMove( threat, depth + 1 );
#endif

#if 1
					// 詰みチェック
					if( nval > VMIN ){
						nval = -NegaMaxR( tree, depth + 1, ndepth, VMAX-1, VMAX, state_new );
					}
#endif
				}

				// パスすると詰む = 詰めろ
				if( nval <= VMIN ){
					state |= STATE_MATE;
				}
				tree->UnmakeNullMove();
			}
		}
	}

	// recurcive iterative-deepening
#if RIDEEP
	if( !hash_ok && rdepth >= 3 * DINC ){
		if( is_pv ){
			NegaMaxR( tree, depth, rdepth - DINC * 2, alpha, beta, state );
			if( INTERRUPT ){ return 0; }
		}
#if 1
		else if( !tree->IsCheck() && SVALUE + 80 >= beta ){
			NegaMaxR( tree, depth, rdepth / 2, alpha, beta, state );
			if( INTERRUPT ){ return 0; }
		}
#endif
	}
#endif

	// 前回の最善手
	MOVE hmove1, hmove2;
	if( !hash_table->GetMoves( hash, tree->pshogi, hmove1, hmove2 ) ){
		hmove1.from = hmove2.from = EMPTY;
	}

#if SINGULAR_EXTENSION
	bool do_check_singular = (
		   hash_type == VALUE_LOWER
		&& !tree->Singular()
		&& rdepth >= 6 * DINC
		&& hash_ok
		&& hmove1.from != EMPTY
		);
#endif

	// 子ノードを展開して探索
	MOVE bmove;
	MOVE* pmove;
	int value = VMIN;
	int mvcnt = 0;

	bmove.from = EMPTY;
	tree->InitMovesGenerator( hmove1, hmove2 );
	while( ( pmove = tree->GetNextMove( history
#if KILLER
			, depth
#endif
		) ) != NULL ){
#define move					(*pmove)

#if SINGULAR_EXTENSION
		if( tree->Exclude( move, depth ) ){ continue; }
#endif

		int stand_pat_new = VMAX;
#define SVALUE_NEW			((stand_pat_new)==(VMAX)?((stand_pat_new)=(tree->GetValueN())):(stand_pat_new))
		int estimate_diff = tree->EstimateValue( move );
		int alpha_new = max( value, alpha ); // 新しいalpha値
		int rdepth_new = rdepth - DINC;      // 新しい残り深さ
		unsigned state_new = STATE_ROOT;
		int vtemp;
		bool isHash1 = move.IsSame( hmove1 );
		bool isHash2 = move.IsSame( hmove2 );
		bool isHash = isHash1 || isHash2
#if KILLER
			|| tree->IsKiller( move )
#endif
			;

		// 最善手に選ばれた(深さを重みとした)割合を
		// 算出するために出現した回数を数える。
		history->Appear( move.from, move.to, rdepth );

#if DEPTH_EXT
		{
			// extensions
			int ext;

#if LIMIT_EXT
			//if( depth < root_depth * 2 ){
			if( depth < root_depth * 1 ){
				ext = DINC;
			}
			//else if( depth < root_depth * 3 ){
			else if( depth < root_depth * 2 ){
				ext = DINC * 3 / 4;
			}
			//else if( depth < root_depth * 4 ){
			else if( depth < root_depth * 3 ){
				ext = DINC * 2 / 4;
			}
			else{
				ext = DINC * 1 / 4;
			}
#else
			ext = DINC;
#endif

			// check
			if( tree->IsCheckMove( move ) ){
				rdepth_new += ext;
			}
#if 1
			// mate threat
			else if( state & STATE_MATE ){
				rdepth_new += ext / 4;
			}
#endif
			// one reply
			else if( tree->IsCheck() ){
				if( tree->GetMovesNumber() == 1 ){
					rdepth_new += ext / 2;
				}
#if 0
				else if( tree->GetMovesNumber() == 2 ){
					rdepth_new += ext / 8;
				}
#endif
			}
			else{
				// recapture
				if( state & STATE_RECAP &&
					tree->IsRecapture( move )
				){
					//rdepth_new += ext / 2;
					rdepth_new += ext * 3 / 4;
					state_new &= ~STATE_RECAP;
				}
			}
		}
#endif

#if EXT05
		// 0.5手延長
		if( move.IsSame( hmove1 ) ){
			rdepth_new += DINC / 2;
		}
#endif

		// late move reduction
		int reduction = 0;
		if( !( state & STATE_MATE ) && !tree->IsTacticalMove( move )
			&& !tree->IsCheck() && !isHash
		){
#if 1
			int hist = history->Get( move.from, move.to );

			if     ( hist * 0x20 < HIST_FRACTION ){
				reduction = DINC * 2;
			}
			else if( hist * 0x04 < HIST_FRACTION ){
				reduction = DINC * 1;
			}
			else if( hist * 0x02 < HIST_FRACTION ){
				reduction = DINC * 1 / 2;
			}
#else
			reduction = DINC * 1;
#endif
		}

		rdepth_new -= reduction;

#if SINGULAR_EXTENSION
		// singular extension
		if( do_check_singular
			&& rdepth_new <= rdepth - DINC
			&& isHash1
		){
			unsigned state_sin = STATE_ROOT & (~STATE_NULL);
			int beta_sin = hash_value - rdepth / 2;
			tree->SetExclude( move, depth+1 );
			vtemp = NegaMaxR( tree, depth+1, rdepth / 2, beta_sin-1, beta_sin, state_sin );
			tree->UnsetExclude();
			if( vtemp < beta_sin ){
				rdepth_new += DINC;
				info.singular_ext++;
			}
		}
#endif

		// futility pruning
#if 1
		if( /*!is_pv &&*/ !( state & STATE_MATE ) && !isHash && !tree->IsCheck()
			&& !tree->IsCheckMove( move ) && !tree->IsTacticalMove( move )
		){
#if MOVE_COUNT_PRUNING
			// move count based pruning
			if( mvcnt > GetMoveCount( rdepth_new ) /* && value > VMIN */
				&& ( threat.from == EMP || !tree->ConnectedThreat( threat, move ) )
			){
				tree->cnts.move_count_cut++;
				mvcnt++;
				value = alpha_new;
				continue;
			}
#endif

			// value based pruning
			int estimate = SVALUE + estimate_diff;
#if FUT_EXT
			if( estimate + GetFutMgn( rdepth_new, mvcnt )
				+ GetGain( move.koma, move.to ) <= alpha_new )
#else
			if( ( rdepth_new < DINC * 2 && estimate + FUT_MGN  <= alpha_new ) ||
				( rdepth_new < DINC * 3 && estimate + FUT_MGN2 <= alpha_new ) )
#endif
			{
				tree->cnts.futility_cut++;
				mvcnt++;
				value = alpha_new;
				continue;
			}
		}
#endif

#if FUT_EXT
		// MakeMoveした後だと取得できないのでここで書いておく。
		SVALUE;
#endif

		// SHEK
		tree->ShekSet();

		// make move
		tree->MakeMove( move );

#if 1
		// extended futility pruning
		if( /*!is_pv &&*/ !( state & STATE_MATE ) && !isHash
			&& !tree->IsReply() && !tree->IsCheck() && !tree->IsTactical()
		){
#if FUT_EXT
			if( SVALUE_NEW - GetFutMgn( rdepth_new, mvcnt ) >= -alpha_new )
#else
			if( ( rdepth_new < DINC     && SVALUE_NEW            >= -alpha_new ) ||
				( rdepth_new < DINC * 2 && SVALUE_NEW - FUT_MGN  >= -alpha_new ) ||
				( rdepth_new < DINC * 3 && SVALUE_NEW - FUT_MGN2 >= -alpha_new ) )
#endif
			{
				tree->UnmakeMove();
				tree->ShekUnset();

				tree->cnts.e_futility_cut++;
				mvcnt++;
				value = alpha_new;
				continue;
			}
		}
#endif

#if FUT_EXT
		// ゲイン調整
		UpdateGain( move.koma, move.to, stand_pat + estimate_diff, SVALUE_NEW );
#endif

		if( mvcnt == 0 && reduction == 0 ){
			vtemp = -NegaMaxR( tree, depth + 1, rdepth_new, -beta, -alpha_new, state_new );
		}
		else{
			// nega-scout
			vtemp = -NegaMaxR( tree, depth + 1, rdepth_new, -(alpha_new+1), -alpha_new, state_new );

			// 打ち切りフラグ 2010/10/31
#if 1 // => thread.cpp
			if( reduction == 0 ){
				// 短縮がない場合はvtemp以上であることが保証される。
				if( vtemp > alpha_new && beta > vtemp && !(INTERRUPT) )
				{
					// null window search失敗
					state_new &= ~STATE_NULL; // null windowのときにnull moveをやってある。
					vtemp = -NegaMaxR( tree, depth + 1, rdepth_new, -beta, -vtemp, state_new );
				}
			}
			else{
				// 短縮があるならnull windowでもやり直し。
				if( vtemp > alpha_new /*&& beta > alpha_new + 1*/ && !(INTERRUPT) )
				{
					// null window search失敗
					rdepth_new += reduction;
					state_new &= ~STATE_NULL; // null windowのときにnull moveをやってある。
					vtemp = -NegaMaxR( tree, depth + 1, rdepth_new, -beta, -alpha_new, state_new );
				}
			}
#else
			if( vtemp > alpha_new && beta > alpha_new + 1 && !(INTERRUPT) )
			{
				// null window search失敗
				rdepth_new += reduction;
				state_new &= ~STATE_NULL; // null windowのときにnull moveをやってある。
				vtemp = -NegaMaxR( tree, depth + 1, rdepth_new, -beta, -alpha_new, state_new );
			}
#endif
		}

		// unmake move
		tree->UnmakeMove();

		// SHEK
		tree->ShekUnset();

		// 打ち切りフラグ
		if( INTERRUPT ){ return 0; }

		if( vtemp > value ){
			value = vtemp;
			bmove = move;
			tree->PVCopy( depth, move );
			if( value >= beta ){
				// beta cut
				if( !tree->IsCapture( move ) ){
					// 最善手に選ばれた(深さを重みとした)回数を数える。
					// 但し、beta cutなので真の最善手とは限らない。
					history->Good( move.from, move.to, rdepth );
#if KILLER
					tree->AddKiller( depth, move, value - stand_pat );
				}
				else if( !isHash && move.value < 0 ){
					tree->AddKiller( depth, move, value - stand_pat );
#endif
				}
				goto lab_add_hash;
			}
		}
		mvcnt++;

		// 粒度が大きければsplit
		if( parallel && /*idle_num > 0 &&*/
			( !tree->IsCheck() || tree->GetMovesNumber() >= 4 ) &&
			( rdepth >= DINC * 4 || ( root_depth <= 10 && rdepth >= DINC * 3 ) )
		){
			// split
			if( Split( tree, depth, rdepth, value, max( value, alpha ), beta, state_new, mvcnt
#if MOVE_COUNT_PRUNING
				, threat
#endif
				) ){
				tree->cnts.split++;

				// 打ち切りフラグ
				if( INTERRUPT ){ return 0; }

				if( tree->value > value ){
					value = tree->value;
					bmove = tree->bmove;
				}
				break;
			}
		}
#undef move
	}

	if( mvcnt == 0 ){
		// 指し手がない場合
		return VMIN;
	}

	// 最善手に選ばれた(深さを重みとした)回数を数える。
	if( bmove.from != EMPTY && value > alpha && value < beta ){
		history->Good( bmove.from, bmove.to, rdepth );
	}

	// ハッシュテーブルに追加。
lab_add_hash:
	switch( hash_table->Entry( hash, rdepth,
		alpha, beta, max( value, alpha ), bmove ) )
	{
	case HASH_COLLISION:
		tree->cnts.hash_collision++;
	case HASH_ENTRY:
		tree->cnts.hash_entry++;
		break;
	case HASH_NOENTRY:
		break;
	case HASH_OVERWRITE:
		break;
	}

	return value;
}

#if 1
int Think::IDeepening( Tree* tree, int depth, int rdepth,
	int alpha, int beta, MOVE* pbmove )
/************************************************
反復深化探索
************************************************/
{
	MOVE* pmove;
	MOVE bmove;         // 最善手
	int value;
	int vtemp;
	int rd = 0;         // 深さ
	int snum = 0;       // ソートの対象となる指し手の数
	int i;

	tree->InitNode( depth );

	if( pbmove != NULL ){
		pbmove->from = EMPTY;
	}

	tree->InitMoves();
	tree->GenerateMoves();
	tree->GetMove( 0 ).value = VMIN;

	if( tree->GetMovesNumber() == 0 ){
		// 指し手がない。
		if( pbmove != NULL ){ pbmove->from = EMPTY; }
		return VMIN;
	}

	bmove.from = EMPTY;

	// 反復深化探索
	rd = rdepth % DINC + DINC;
	for( ; rd <= rdepth ; rd += DINC ){
		if( rd > DINC && tree->GetMovesNumber() >= 2 ){
			// 並べ替え(降順)
			tree->StableSortMoves( 0, snum );
		}

		value = VMIN - 1;

		// キラームーブ初期化
#if KILLER
		tree->InitKiller( depth + 1 );
#endif

		for( i = 0 ; i < tree->GetMovesNumber() ; i++ ){
			pmove = &tree->GetMove( i );

			// SHEK
			tree->ShekSet();

			// Make move
			tree->MakeMove( *pmove );

			vtemp = -NegaMaxR( tree, depth + 1, rd - DINC,
				-beta, -max(value,alpha), STATE_ROOT );

			// Unmake move
			tree->UnmakeMove();

			// SHEK
			tree->ShekUnset();

			// 強制打ち切り
			if( INTERRUPT ){
				if( pbmove != NULL ){ *pbmove = bmove; }
				return 0;
			}

			tree->SetMoveValue( i, vtemp );
			if( vtemp > value ){
				value = vtemp;
				tree->PVCopy( depth, *pmove );
				bmove = *pmove;
				if( value >= VMAX ){
					if( pbmove != NULL ){ *pbmove = bmove; }
					return VMAX; // 勝ち
				}
				else if( rd == rdepth && value >= beta ){
					if( pbmove != NULL ){ *pbmove = bmove; }
					return value; // Beta Cut
				}
			}
		}

		if( value <= VMIN ){
			if( pbmove != NULL ){ pbmove->from = EMPTY; }
			return VMIN; // 負け
		}

		if( i < tree->GetMovesNumber() ){
			snum = i + 1;
		}
		else{
			snum = tree->GetMovesNumber();
		}
	}

	if( pbmove != NULL ){ *pbmove = bmove; }

	return value;
}
#endif

#define ASP_MGN				150
#define ASP_MGN2			300
#define ASP_MGN3			450

int Think::Search( ShogiEval* pshogi, MOVE* pmove, int mdepth,
	THINK_INFO* _info, const int* _iflag, unsigned opt ){
/************************************************
思考ルーチン
投了時は0を返し、pmoveは変更されない。
************************************************/
	Tree* tree = &trees[0];
	Timer tm;
	int ret = 1;
	int alpha, beta;
	int alpha_new;
	int fail_high, fail_low;
	int value_max;
	int value_min;
	int vtemp;
	int best = 0;
	int md = 0;
	int last_value = 0;
	int easy_value = 0;
	int easy_diff = 0;
	int i;
	double d;
#if INANIWA
	bool isInaniwa;
	int v_ina;
#endif

#if !defined(NDEBUG)
	Debug::Print( "%s(%d) Search called\n", __FILE__, __LINE__ );
#endif

	iflag = _iflag;

	// 処理時間計測開始
	tm.SetTime();

	// 探索情報の初期化
	memset( &info, 0, sizeof(THINK_INFO) );

	// スタックのサイズをオーバー
	if( mdepth > MAX_DEPTH ){
		mdepth = MAX_DEPTH;
	}

#if !defined(NDEBUG)
	Debug::Print( "%03f %s(%d) Search\n", tm.GetTime(), __FILE__, __LINE__ );
#endif

	// 並列探索
	// return の前に必ずDeleteWorkersを呼ぶこと。
	CreateWorkers( pshogi );

#if !defined(NDEBUG)
	Debug::Print( "%03f %s(%d) Search\n", tm.GetTime(), __FILE__, __LINE__ );
#endif

	tree->InitMoves();
	tree->GenerateMoves();

	// PVの初期化
	// 投了や確定手でPVがとれないのでここに移動 2010/11/03
	tree->InitNode( 0 );

	if( tree->GetMovesNumber() == 0 ){
		// 投了
		ret = 0;
		goto lab_end;
	}

	if( tree->GetMovesNumber() == 1 ){
		// 確定手
		if( pmove != NULL ){
			(*pmove) = tree->GetMove( 0 );
		}
		tree->SetMoveValue( best, 0 );

		goto lab_end;
	}

//	hash_table->Clear();       // ハッシュテーブルの初期化
	Init4Root();

	info.bPVMove = true;

#ifdef TDEBUG 
	tree->DebugPrintBan();
#endif

#if INANIWA
	// 相手が稲庭か?
	isInaniwa = IsInaniwa( tree->pshogi, pshogi->GetSengo() ^ SENGO );
#endif

#if !defined(NDEBUG)
	Debug::Print( "%03f %s(%d) Search\n", tm.GetTime(), __FILE__, __LINE__ );
#endif

#if 0
	// 静止探索
	tree->SortSee();
	alpha = VMIN;
	for( i = 0 ; i < tree->GetMovesNumber() ; i++ ){
		// 静止探索にSHEKは不要
		//tree->ShekSet();                      // SHEK
		tree->MakeMove( tree->GetMove( i ) ); // make move

		vtemp = -Quiescence( tree, 1, 1, VMIN, -alpha );

		tree->UnmakeMove();                   // unmake move
		//tree->ShekUnset();                    // SHEK

#if INANIWA
		if( isInaniwa ){ vtemp += GetInaValue( tree->GetMove( i ) ); }
#endif
		tree->SetMoveValue( i, vtemp );

		alpha = max( alpha, vtemp );
	}

	// 並べ替え(降順)
	tree->StableSortMoves();
#else
	tree->SortSee();
	tree->GetMove( 0 ).value = VMIN;
#endif

	easy_value = tree->GetMove( 0 ).value;
	easy_diff = tree->GetMove( 0 ).value - tree->GetMove( 1 ).value;

	// 反復深化探索
	for( md = 0 ; md < mdepth ; md++ ){
#if !defined(NDEBUG)
		Debug::Print( "%03f %s(%d) Search DEPTH=%d\n", tm.GetTime(), __FILE__, __LINE__, md );
#endif

		// 前回の最善手
		last_value = tree->GetMove( 0 ).value;

#ifdef TDEBUG 
		tree->DebugPrintMoves();
#endif

		// Aspiration Search
		if( md == 0 ){
			alpha = VMIN;
			beta = VMAX;
			fail_high = INT_MAX;
			fail_low = INT_MAX;
		}
		else{
			alpha = tree->pmoves->GetMove( 0 ).value - ASP_MGN;
			beta = tree->pmoves->GetMove( 0 ).value + ASP_MGN;
			fail_high = 0;
			fail_low = 0;
		}

lab_search:
		// 最善手
		best = 0;

		// root局面の探索窓
		root_alpha = alpha;
		root_beta  = beta;
		root_depth = md;

		// キラームーブ初期化
#if KILLER
		tree->InitKiller( 1 );
#endif

		// 評価値
		value_max  = alpha;
		value_min  = beta;

		for( i = 0 ; i < tree->GetMovesNumber() ; i++ ){
			alpha_new = max(value_max,alpha);
#if INANIWA
			if( isInaniwa ){
				v_ina= GetInaValue( tree->GetMove( i ) );
				/*
				if( v_ina > 100 ){
					tree->pshogi->PrintMove( tree->GetMove( i ) );
				}
				*/
				alpha_new -= v_ina;
				beta      -= v_ina;
			}
#endif

			// SHEK
			tree->ShekSet();

			// make move
			tree->MakeMove( tree->GetMove( i ) );
			if( i == 0 ){
				// PV-node
				vtemp = -NegaMaxR( &(trees[0]), 1, md * DINC,
					-beta, -alpha_new, STATE_ROOT );
			}
			else{
				// PV search
				vtemp = -NegaMaxR( &(trees[0]), 1, md * DINC,
					-(alpha_new+1), -alpha_new, STATE_ROOT );

				if( vtemp > alpha_new ) {
					vtemp = -NegaMaxR( &(trees[0]), 1, md * DINC,
						-beta, -alpha_new, STATE_ROOT & (~STATE_NULL) );
				}
			}

			// unmake move
			tree->UnmakeMove();

			// SHEK
			tree->ShekUnset();

#if INANIWA
			if( isInaniwa ){
				vtemp     += v_ina;
				beta      += v_ina;
			}
#endif

			// 強制打ち切り
			if( INTERRUPT ){
				// 読み筋の表示
#ifndef USI
				if( opt & SEARCH_DISP ){ tree->PrintPVMove( md+1, 0, value_max ); }
#endif
				goto lab_break;
			}

			value_min = min( value_min, vtemp );

#if 0
			// Debug
			// 最善手以外の読み筋
			if( opt & SEARCH_DISP ){
				cout << " " << i << " ";
				tree->pshogi->PrintMove( tree->GetMove( i ) );
				tree->PrintPVMove( md, 1, vtemp );
			}
#endif

			// 最大値を更新するか
			if( vtemp > value_max ){
				best = i;                              // best move
				value_max = vtemp;                     // max
				tree->PVCopy( 0, tree->GetMove( i ) ); // PV
				tree->SetMoveValue( i, vtemp );            // ソート用

				// 勝ち
				if( value_max >= VMAX ){
					// 読み筋の表示
					if( opt & SEARCH_DISP ){ tree->PrintPVMove( md+1, 0, value_max ); }
					goto lab_break;
				}
				else if( value_max >= beta ){ break; } // fail-high
			}
			else{
				// fail-low
				// ソート後に悪い手が上位に来ないようにvalue_minを代入
				if( value_min == VMIN && vtemp > VMIN ){
					tree->SetMoveValue( i, VMIN+1 );
				}
				else{
					tree->SetMoveValue( i, value_min );
				}
			}
		}

		// 読み筋の表示
		if( opt & SEARCH_DISP ){ tree->PrintPVMove( md+1, 0, value_max ); }

		// fail-low
		if( value_max <= alpha && alpha > VMIN ){
			if     ( fail_low == 0 ){
				alpha = alpha - ASP_MGN2;
				fail_low++;
			}
			//else if( fail_low == 1 ){
				//alpha = alpha - ASP_MGN3;
				//fail_low++;
			//}
			else{
				alpha = VMIN;
			}
			PrintSearchInfo( "fail-low" );
			goto lab_search;
		}
		// fail-high
		else if( value_max >= beta && beta < VMAX ){
			if     ( fail_high == 0 ){
				beta = beta + ASP_MGN2;
				fail_high++;
			}
			//else if( fail_high == 1 ){
				//beta = beta + ASP_MGN3;
				//fail_high++;
			//}
			else{
				beta = VMAX;
			}
			PrintSearchInfo( "fail-high" );
			goto lab_search;
		}

		// 負け
		if( value_max <= VMIN ){ goto lab_break; }

		// 並べ替え(降順)
		tree->StableSortMoves( 0, min( i + 1, tree->GetMovesNumber() ) );
		best = 0;

		if( opt & SEARCH_EASY && md >= 3 ){
			// 当然の1手
			// 条件がかなりあやしい...
			int value_diff = tree->GetMove( 0 ).value - tree->GetMove( 1 ).value;

			if( value_diff >= 400 ){
				if( easy_diff >= 600 ){
					if( value_max >= easy_value - 200
						&& value_max <= easy_value + 400
						&& abs(value_max) <= 500
					){
#if !defined(NDEBUG)
						cout << "easy(" << __LINE__ << ")\n";
						Debug::Print( "easy(%d)\n", __LINE__ );
#endif
						goto lab_break;
					}
				}
				else if( easy_diff >= 400 ){
					if( value_max >= easy_value - 100
						&& value_max <= easy_value + 200
						&& abs(value_max) <= 200
					){
#if !defined(NDEBUG)
						cout << "easy(" << __LINE__ << ")\n";
						Debug::Print( "easy(%d)\n", __LINE__ );
#endif
						goto lab_break;
					}
				}
			}

			// 十分に考えたか
			if( limit_min > 0 && (int)tm.GetTime() >= limit_min ){
				if( value_max >= last_value
					&& value_max <= last_value + 150
				){
#if !defined(NDEBUG)
					cout << "easy(" << __LINE__ << ")\n";
					Debug::Print( "easy(%d)\n", __LINE__ );
#endif
					goto lab_break;
				}
				else if( value_diff >= 300
					&& value_max >= last_value - 100
				){
#if !defined(NDEBUG)
					cout << "easy(" << __LINE__ << ")\n";
					Debug::Print( "easy(%d)\n", __LINE__ );
#endif
					goto lab_break;
				}
			}
		}
	}
lab_break:
#ifdef TDEBUG 
	tree->DebugPrintMoves();
	tree->DebugPrintDepth( md );
#endif

	// 打切り時にbestがVMINの手を指している場合
	// 未探索の手からVMINで無い可能性があるものを選択
	if( tree->GetMove( best ).value <= VMIN ){
		for( i = best + 1 ; i < tree->GetMovesNumber() ; i++ ){
			if( tree->GetMove( i ).value > VMIN ){
				best = i;
				break;
			}
		}
	}

	// 探索終了
	if( pmove != NULL ){
		(*pmove) = tree->GetMove( best );
	}

lab_end:
	// 後始末(CreateWorkers)
	DeleteWorkers( info );

	// 評価値
	info.value = tree->GetMove( best ).value;

	// 処理時間
	d = tm.GetTime();
	info.msec = (int)( d * 1.0e+3 );

	// node / second
	info.nps = (int)( (int)info.node / d );

	// depth
	info.depth = md;

	// 探索情報
	if( _info != NULL ){
		*_info = info;
	}

	return ret;
}

#if INANIWA
int Think::GetInaValue( MOVE move ){
/************************************************
稲庭用加点
************************************************/
	if( move.from & DAI ){ return 0; }

	switch( move.koma ){
#define INA_VAL(ina)			( ina[_addr[move.to]] - ina[_addr[move.from]] )
	case SFU: return INA_VAL( inaniwaFU );
	case SKY: return INA_VAL( inaniwaKY );
	case SKE: return INA_VAL( inaniwaKE );
	case SGI: return INA_VAL( inaniwaGI );
	case SKI: return INA_VAL( inaniwaKI );
	case SKA: return INA_VAL( inaniwaKA );
	case SHI: return INA_VAL( inaniwaHI );
	case SOU: return INA_VAL( inaniwaOU );
#undef INA_VAL
#define INA_VAL(ina)			( ina[80-_addr[move.to]] - ina[80-_addr[move.from]] )
	case GFU: return INA_VAL( inaniwaFU );
	case GKY: return INA_VAL( inaniwaKY );
	case GKE: return INA_VAL( inaniwaKE );
	case GGI: return INA_VAL( inaniwaGI );
	case GKI: return INA_VAL( inaniwaKI );
	case GKA: return INA_VAL( inaniwaKA );
	case GHI: return INA_VAL( inaniwaHI );
	case GOU: return INA_VAL( inaniwaOU );
#undef INA_VAL
	}
	return 0;
}

bool Think::IsInaniwa( ShogiEval* pshogi, KOMA sengo ){
/************************************************
稲庭判定
************************************************/
	const KOMA* ban = pshogi->GetBan();
	const int*  dai = pshogi->GetDai();
	int i;

	if( sengo == SENTE ){
		int sou = pshogi->GetSOU();

		// 2から8筋の歩がそのままか
		for( i = 0x72 ; i <= 0x78 ; i += 0x01 ){
			if( ban[i] != SFU ){ return false; }
		}

		// 端歩は5段目以下か
		for( i = 0x51 ; i <= 0x91 ; i += 0x10 ){
			if( ban[i] == SFU ){ break; }
		}
		if( i > 0x91 ){ return false; }

		for( i = 0x59 ; i <= 0x99 ; i += 0x10 ){
			if( ban[i] == SFU ){ break; }
		}
		if( i > 0x99 ){ return false; }

		// 7八銀型は除外
		if( ban[0x87] == SGI ){
			return false;
		}

		// 玉が右に行く場合は除外
		if( 0x82 <= sou && sou <= 0x85 ){
			return false;
		}

		// 初期配置とほぼ同じなら除外
		if( ban[0x82] == SHI && ban[0x88] == SKA &&
			ban[0x96] == SKI && ban[0x94] == SKI && sou == 0x95
		){
			return false;
		}

		// 持ち駒がないか
		if( dai[SFU] != 0 || dai[SKY] != 0 || dai[SKE] != 0 ||
			dai[SGI] != 0 || dai[SKI] != 0 || dai[SKA] != 0 || dai[SHI] != 0 )
		{
			return false;
		}
	}
	else{
		int gou = pshogi->GetGOU();

		// 2から8筋の歩がそのままか
		for( i = 0x32 ; i <= 0x38 ; i += 0x01 ){
			if( ban[i] != GFU ){ return false; }
		}

		// 端歩は5段目以下か
		for( i = 0x11 ; i <= 0x51 ; i += 0x10 ){
			if( ban[i] == GFU ){ break; }
		}
		if( i > 0x51 ){ return false; }

		for( i = 0x19 ; i <= 0x59 ; i += 0x10 ){
			if( ban[i] == GFU ){ break; }
		}
		if( i > 0x59 ){ return false; }

		// 3二銀型は除外
		if( ban[0x23] == GGI ){
			return false;
		}

		// 玉が右に行く場合は除外
		if( 0x25 <= gou && gou <= 0x28 ){
			return false;
		}

		// 初期配置とほぼ同じなら除外
		if( ban[0x28] == GHI && ban[0x22] == GKA &&
			ban[0x16] == GKI && ban[0x14] == GKI && gou == 0x15
		){
			return false;
		}

		// 持ち駒がないか
		if( dai[GFU] != 0 || dai[GKY] != 0 || dai[GKE] != 0 ||
			dai[GGI] != 0 || dai[GKI] != 0 || dai[GKA] != 0 || dai[GHI] != 0 )
		{
			return false;
		}
	}

	return true;
}
#endif
