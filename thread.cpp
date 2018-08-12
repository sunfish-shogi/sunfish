/* thread.cpp
 * R.Kubo 2011-2012
 * スレッドレベル並列化
 */

#ifdef WIN32
#include <windows.h>
#else
#include <sched.h>
#endif

#include "match.h"

void Think::DebugPrintWorkers(){
/************************************************
ワーカーに関するデバプリ
************************************************/
	int i;

	cout << "***** DebugPrintWorkers() *****\n";
	cout << "idle :" << idle_num << '\n';

	cout << "worker\ttree\tworker\tjob\n";
	for( i = 0 ; i < worker_num ; i++ ){
		cout << i;
		cout << '\t' << workers[i].tree;
		cout << '\t' << workers[i].worker;
		cout << '\t' << workers[i].job;
		cout << '\n';
	}

	cout << "tree\tself\tparent\n";
	for( i = 0 ; i < tree_num ; i++ ){
		cout << i;
		cout << '\t' << trees[i].self;
		cout << '\t' << trees[i].parent;
		cout << '\n';
	}
	cout << "*******************************\n";
}

void Think::PresetTrees( ShogiEval* pshogi ){
/************************************************
ShogiEval::Copy()が遅いので予めCopy()を実行。
2回目以降はCopy()の代わりにQuickCopy()を呼ぶ。
************************************************/
	int i;

	for( i = 0 ; i < tree_num ; i++ ){
		trees[i].Copy( *pshogi );
	}
}

void Think::CreateWorkers( ShogiEval* pshogi ){
/************************************************
ワーカーの作成
************************************************/
	int i;

#if !defined(NDEBUG)
	// Test
	Timer tm;
	tm.SetTime();
#endif

	parallel = true;

	worker_num = _worker_num;

#if !defined(NDEBUG)
	// Test
	Debug::Print( "%03f %s(%d) CreateWorkers\n", tm.GetTime(), __FILE__, __LINE__ );
#endif

	// treeの初期化
	for( i = 0 ; i < tree_num ; i++ ){
		trees[i].parent = TREE_NULL;        // 親の番号
		trees[i].self = i;                  // 自分の番号
		trees[i].Copy( *pshogi );           // 局面の初期化
		trees[i].cnts.init();
	}
	// trees[0]はルートで使用
	trees[0].parent = TREE_ROOT;
	trees[0].worker = 0;
	trees[0].cnts.init();

#if !defined(NDEBUG)
	// Test
	Debug::Print( "%03f %s(%d) CreateWorkers\n", tm.GetTime(), __FILE__, __LINE__ );
#endif

	// workerの初期化
	for( i = 1 ; i < worker_num ; i++ ){
		workers[i].pthink = this;
		workers[i].tree = TREE_NULL;
		workers[i].worker = i;
		workers[i].job = false;
#ifdef WIN32
		if( INVALID_HANDLE_VALUE == ( workers[i].hThread
			= (HANDLE)_beginthreadex( NULL, 0, WorkerThread, (LPVOID)&workers[i], 0, NULL ) ) )
#else
		if( 0 != pthread_create( &workers[i].tid, NULL, WorkerThread, (void*)&workers[i] ) )
#endif
		{
			// スレッドの作成に失敗
			cerr << "ERROR!!..thread creation failed.\n";
			worker_num = i;
			break;
		}
	}
	workers[0].pthink = this;
	workers[0].tree = 0;
	workers[0].worker = 0;
	workers[0].job = true;

#if !defined(NDEBUG)
	// Test
	Debug::Print( "%03f %s(%d) CreateWorkers\n", tm.GetTime(), __FILE__, __LINE__ );
#endif

	idle_num  = worker_num - 1; // idle workerの数
	idle_tree = tree_num   - 1; // 空きtreeの数
	info.unused_tree = idle_tree;
}

void Think::DeleteWorkers( /*ShogiEval* pshogi, */THINK_INFO& info ){
/************************************************
ワーカースレッドの終了
************************************************/
	int i;

	parallel = false;

	for( i = 0 ; i < tree_num ; i++ ){
#if 0
		// SHEKの探索中フラグを戻す。
		// Tree::Copy()内でShek::Clear()を呼ぶので
		// ここで元に戻さなくて良い。
		trees[i].shek_table->Unset( pshogi );
#endif

		// 探索情報
		info.add( trees[i].cnts );
	}

	// スレッドの終了を待ってハンドルを解放
	for( i = 1 ; i < worker_num ; i++ ){
#ifdef WIN32
		WaitForSingleObject( workers[i].hThread, INFINITE );
		CloseHandle( workers[i].hThread );
#else
		pthread_join( workers[i].tid, NULL );
#endif
	}
}

bool Think::Split( Tree* tree, int depth, int rdepth,
	int value, int alpha, int beta, unsigned state, int mvcnt
#if MOVE_COUNT_PRUNING
	, MOVE& threat
#endif
	){
/************************************************
Split
************************************************/
	int w = 0;            // worker
	int t = 1;            // trees[0]は親スレッドが使用済み
	int my_t = TREE_NULL; // 自分のtree
	int num = 0;          // 確保したワーカー数

	tree->value = value;      // 評価値
	tree->bmove.from = EMPTY; // 最善手
	tree->mvcnt = mvcnt;      // 兄弟節点のカウンタ
#if MOVE_COUNT_PRUNING
	tree->threat = threat;    // 脅威(詰めろ)
#endif

	MutexLockSplit();

	// アイドルワーカーの数
	// 空きtreeの数(自分のを入れて2個必要)
	if( idle_num == 0 || idle_tree <= 1 ){
		MutexUnlockSplit();
		return false;
	}

	// 自分用のtreeを確保
	for( ; t < tree_num ; t++ ){
		// 使用中か
		if( trees[t].parent == TREE_NULL ){
			// tree作成
#if 0
			DebugPrintWorkers();
#endif
			trees[t].SetNode( tree, tree, tree->worker,
				depth, rdepth, alpha, beta, state );
			idle_tree--;
			assert( idle_tree >= 0 );

			workers[tree->worker].tree = t;

			my_t = t;
			t++;
			break;
		}
	}

	if( my_t == TREE_NULL ){
		// もうtreeがあいていない。
		tree->cnts.split_failed++;
		MutexUnlockSplit();
		return false;
	}

	// 暇をしているワーカーを捕まえる。
	for( ; w < worker_num ; w++ ){
		// 仕事中か
		if( workers[w].job == false ){
			// あいているtreeを探す。
			for( ; t < tree_num ; t++ ){
				// 使用中か
				if( trees[t].parent == TREE_NULL ){
					assert( t != my_t );

					// tree作成
					trees[t].SetNode( &trees[my_t], tree, w, depth,
						rdepth, alpha, beta, state );

					// 空きtreeの数
					idle_tree--;
					assert( idle_tree >= 0 );
					info.unused_tree = min( (int)info.unused_tree, idle_tree );

					// idle workerの数
					idle_num--;
					assert( idle_num >= 0 );

					// ジョブの割り当て
					workers[w].tree = t;
					workers[w].job = true;

					num++; // 割り当てた数
					t++;
					break;
				}
			}

			if( t >= tree_num ){
				// もうtreeがない
				assert( idle_tree == 0 );
				break;
			}
		}
	}

	if( num == 0 ){
#if 0
		// Debug
		DebugPrintWorkers();
		assert( false );
#endif

		// 失敗したら自分用のtreeを解放
		trees[my_t].parent = TREE_NULL;
		idle_tree++;
		workers[tree->worker].tree = tree->self;

		tree->cnts.split_failed++;
		MutexUnlockSplit();
		return false;
	}

#if 0
		// Debug
		DebugPrintWorkers();
#endif

	tree->sibling = num;

	MutexUnlockSplit();

	// 自分も処理にはいる。
	DoJob( &trees[my_t] );

	// treeを解放
	MutexLockSplit();
	trees[my_t].parent = TREE_NULL;
	idle_tree++;
	workers[tree->worker].tree = tree->self;
	MutexUnlockSplit();

#if 0
	// 兄弟が終わってくれるのを待つ。
	while( 1 ){
		bool completed = true;
		MutexLockSplit();
		for( t = 1 ; t < tree_num ; t++ ){
			// 同じ親か
			if( trees[t].parent == tree->self ){
				completed = false;
				break;
			}
		}
		MutexUnlockSplit();

		// 全部終了?
		if( completed ){
			break;
		}

		YieldTimeSlice();
	}
#else
	WaitJob( &workers[tree->worker], tree );
#endif

	return true;
}

#ifdef WIN32
unsigned __stdcall Think::WorkerThread( LPVOID p ){
#else
void* Think::WorkerThread( void* p ){
#endif
/************************************************
ワーカースレッド
************************************************/
	WORKER* pw = (WORKER*)p;

	pw->pthink->WaitJob( pw, NULL );

#ifdef WIN32
	return 0;
#else
	return NULL;
#endif
}

void Think::WaitJob( WORKER* pw, Tree* suspend ){
/************************************************
待機
************************************************/
	if( suspend ){
		MutexLockSplit();
		assert( &trees[pw->tree] == suspend );
		assert( pw->job == true );
		assert( pw->tree != TREE_NULL );
		pw->tree = TREE_NULL; // 一時的に解放
		pw->job = false;      // 一時的に解放
		idle_num++;           // idle workerの数
		MutexUnlockSplit();
	}

	// ジョブを待つループ
	while( 1 ){
		// 兄弟が終わったら元に戻る。
		if( suspend ){
			MutexLockSplit();
			if( suspend->sibling == 0 && !pw->job ){
				pw->tree = suspend->self; // 元に戻す。
				pw->job = true;           // 元に戻す。
				idle_num--;               // idle workerの数
				MutexUnlockSplit();
				return;
			}
			MutexUnlockSplit();
		}

		// Search終了
		if( !parallel ){ return; }

		// ジョブをキャッチ
		if( pw->job ){
			Tree* tree = &trees[pw->tree];
			Tree* parent = &trees[tree->parent];

			// ジョブの開始
			DoJob( tree );

			// ジョブの終了
			// ツリーを解放してジョブのフラグを落とす。
			MutexLockSplit();
			tree->parent = TREE_NULL; // 親tree
			idle_tree++;
			parent->sibling--;        // 兄弟の数
			if( suspend && suspend->sibling == 0 ){
				pw->tree = suspend->self; // 元に戻す。
				MutexUnlockSplit();
				return;
			}
			pw->job = false; // job
			idle_num++;      // idle workerの数
			MutexUnlockSplit();
		}

		YieldTimeSlice();
	}
}

void Think::DoJob( Tree* tree ){
/************************************************
ジョブの実行
************************************************/
	Tree* parent = &trees[tree->parent];

	// stand pat
	int stand_pat = VMAX;
#define SVALUE			((stand_pat)==(VMAX)?((stand_pat)=(tree->GetValueN())):(stand_pat))

	MOVE hmove1, hmove2;
	parent->GetHash( hmove1, hmove2 );
	while( 1 ){
		int mvcnt;
		int vtemp;
		MOVE move;
		int stand_pat_new = VMAX;
#define SVALUE_NEW			((stand_pat_new)==(VMAX)?((stand_pat_new)=(tree->GetValueN())):(stand_pat_new))

		MutexLockSplit( parent->mutex_split );
		{
#if 1
			assert( parent->pshogi->GetHash() == tree->pshogi->GetHash() );
#elif !defined(NDEBUG)
			if( parent->pshogi->GetHash() != tree->pshogi->GetHash() ){
				parent->pshogi->PrintBan();
				do{
					cout << parent->pshogi->GetHash() << ' ';
					parent->pshogi->PrintMove();
				}while( parent->pshogi->GoBack() );
				cout << '\n';
				tree->pshogi->PrintBan();
				do{
					cout << tree->pshogi->GetHash() << ' ';
					tree->pshogi->PrintMove();
				}while( tree->pshogi->GoBack() );
				cout << '\n';
				assert( false );
			}
#endif

			// 次の指し手
			MOVE* pmove;
			pmove = parent->GetNextMove( history
#if KILLER
			, tree->depth
#endif
				);
			if( pmove == NULL ){
				// 指し手がなくなったので終了
				MutexUnlockSplit( parent->mutex_split );
				break;
			}
			move = *pmove; // 実体にコピーしないとポインタの参照先が使えなくなる。
			mvcnt = parent->mvcnt++; // 兄弟節点のカウント

			// 最善手に選ばれた(深さを重みとした)割合を
			// 算出するために出現した回数を数える。
			history->Appear( move.from, move.to, tree->rdepth );
		}
		MutexUnlockSplit( parent->mutex_split );

		int rdepth_new = tree->rdepth - DINC;      // 新しい残り深さ
		unsigned state_new = STATE_ROOT;

		bool isHash = move.IsSame( hmove1 ) || move.IsSame( hmove2 )
#if KILLER
			|| parent->IsKiller( move )
#endif
			;

#if DEPTH_EXT
		{
			// extensions
			int ext;

#if LIMIT_EXT
			//if( tree->depth < root_depth * 2 ){
			if( tree->depth < root_depth * 1 ){
				ext = DINC;
			}
			//else if( tree->depth < root_depth * 3 ){
			else if( tree->depth < root_depth * 2 ){
				ext = DINC * 3 / 4;
			}
			//else if( tree->depth < root_depth * 4 ){
			else if( tree->depth < root_depth * 3 ){
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
			else if( tree->state & STATE_MATE ){
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
			// recapture
			else{
				if( tree->state & STATE_RECAP &&
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
		if( !( tree->state & STATE_MATE ) && !tree->IsTacticalMove( move )
			&& !tree->IsCheck() && !isHash
		){
#if 1
			// History::Get()..排他しなくても実用上大丈夫(?)
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

		// futility pruning
#if 1
		if( /*!is_pv &&*/ !( tree->state & STATE_MATE ) && !isHash && !tree->IsCheck()
			&& !tree->IsCheckMove( move ) && !tree->IsTacticalMove( move )
		){
#if MOVE_COUNT_PRUNING
			// move count based pruning
			if( mvcnt > GetMoveCount( rdepth_new ) /* && parent->value > VMIN */
				&& ( parent->threat.from == EMP || !tree->ConnectedThreat( parent->threat, move ) )
			){
				tree->cnts.move_count_cut++;
				MutexLockSplit( parent->mutex_split );
				{
					parent->value = tree->alpha;
				}
				MutexUnlockSplit( parent->mutex_split );
				continue;
			}
#endif

			// value based pruning
			int estimate = SVALUE + tree->EstimateValue( move );
#if FUT_EXT
			if( estimate + GetFutMgn( rdepth_new, mvcnt )
				+ GetGain( move.koma, move.to ) <= tree->alpha )
#else
			if( ( rdepth_new < DINC * 2 && estimate + FUT_MGN <= tree->alpha )
				|| ( rdepth_new < DINC * 3 && estimate + FUT_MGN2 <= tree->alpha ) )
#endif
			{
				tree->cnts.futility_cut++;
				MutexLockSplit( parent->mutex_split );
				{
					parent->value = tree->alpha;
				}
				MutexUnlockSplit( parent->mutex_split );
				continue;
			}
		}
#endif

		// SHEK
		tree->ShekSet();

		// make move
		// (pmovesを進める必要が無いのでpshogi->MoveDだけでも良い。)
		tree->MakeMove( move );

#if 1
		// extended futility pruning
		if( /*!is_pv &&*/ !( tree->state & STATE_MATE ) && !isHash
			&& !tree->IsReply() && !tree->IsCheck() && !tree->IsTactical()
		){
#if FUT_EXT
			if( SVALUE_NEW - GetFutMgn( rdepth_new, mvcnt ) >= -tree->alpha )
#else
			if( ( rdepth_new < DINC     && SVALUE_NEW            >= -tree->alpha ) ||
				( rdepth_new < DINC * 2 && SVALUE_NEW - FUT_MGN  >= -tree->alpha ) ||
				( rdepth_new < DINC * 3 && SVALUE_NEW - FUT_MGN2 >= -tree->alpha ) )
#endif
			{
				tree->UnmakeMove();
				tree->ShekUnset();

				tree->cnts.futility_cut++;
				MutexLockSplit( parent->mutex_split );
				{
					parent->value = tree->alpha;
				}
				MutexUnlockSplit( parent->mutex_split );
				continue;
			}
		}
#endif

#if FUT_EXT
		// ゲイン調整
		UpdateGain( move.koma, move.to, SVALUE, SVALUE_NEW );
#endif
		// 子ノード以下を探索
		// nega-scout
		vtemp = -NegaMaxR( tree, tree->depth + 1, rdepth_new, -tree->beta, -tree->alpha, state_new );

#if 1 // => think.cpp
		{
			if( reduction == 0 ){
				// 短縮がない場合はvtemp以上であることが保証される。
				if( vtemp > tree->alpha && tree->beta > vtemp && !(INTERRUPT) )
				{
					// null window search失敗
					state_new &= ~STATE_NULL; // null windowのときにnull moveをやってある。
					vtemp = -NegaMaxR( tree, tree->depth + 1, rdepth_new, -tree->beta, -vtemp, state_new );
				}
			}
			else{
				// 短縮があるならnull windowsでもやり直し。
				if( vtemp > tree->alpha /*&& beta > tree->alpha + 1*/ && !(INTERRUPT) )
				{
					// null window search失敗
					rdepth_new += reduction;
					state_new &= ~STATE_NULL; // null windowのときにnull moveをやってある。
					vtemp = -NegaMaxR( tree, tree->depth + 1, rdepth_new, -tree->beta, -tree->alpha, state_new );
				}
			}
		}
#else
		if( vtemp > tree->alpha && tree->beta > tree->alpha + 1 && !(INTERRUPT) ){
			// null window探索失敗
			rdepth_new += reduction;
			state_new &= ~STATE_NULL; // null windowのときにnull moveをやってある。
			vtemp = -NegaMaxR( tree, tree->depth + 1, rdepth_new, -tree->beta, -tree->alpha, state_new );
		}
#endif

		// unmake move
		tree->UnmakeMove();

		// SHEK
		tree->ShekUnset();

		// 打ち切りフラグ
		if( INTERRUPT ){ break; }

		MutexLockSplit( parent->mutex_split );
		{
			// alpha値の更新
			if( vtemp > parent->value ){
				parent->value = vtemp;
				parent->bmove = move;
				parent->PVCopy( tree, tree->depth, move );
				if( vtemp >= tree->beta ){
					// beta cut
					if( !tree->IsCapture( move ) ){
						// 最善手に選ばれた(深さを重みとした)回数を数える。
						// 但し、beta cutなので真の最善手とは限らない。
						history->Good( move.from, move.to, tree->rdepth );
#if KILLER
						parent->AddKiller( tree->depth, move, vtemp - stand_pat );
					}
					else if( move.value < 0 ){
						parent->AddKiller( tree->depth, move, vtemp - stand_pat );
#endif
					}

					// 兄弟も探索を終了させる。
					SetAbort( tree->parent );

					MutexUnlockSplit( parent->mutex_split );
					break;
				}
			}

			// 新しいalpha値
			tree->alpha = max( tree->alpha, parent->value );
		}
		MutexUnlockSplit( parent->mutex_split );
	}
}
