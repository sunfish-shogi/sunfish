/* tree.cpp
 * R.Kubo 2011-2012
 * Treeクラス
 */

#include "match.h"
#include <fstream>

TreeBase::TreeBase( const ShogiEval* pshogi_org ){
/************************************************
コンストラクタ
************************************************/
	shek_table = new ShekTable();
	if( pshogi_org == NULL ){
		pshogi = NULL;
	}
	else{
		pshogi = new ShogiEval( *pshogi_org );
		shek_table->Init();
		shek_table->Set( pshogi );
	}
	moves = new MovesStack[MAX_DEPTH];
	pmoves = &(moves[0]);
}

TreeBase::~TreeBase(){
/************************************************
デストラクタ
************************************************/
	if( pshogi ){
		delete pshogi;
	}
	delete [] moves;
	delete shek_table;
}

Tree::Tree( const ShogiEval* pshogi_org ) : TreeBase( pshogi_org ){
/************************************************
コンストラクタ
************************************************/
#if KILLER
	kmoves = new KillerMove[MAX_DEPTH+1];
#endif
	pvmove = new PVMove();
	copy_node = 0;
#ifdef WIN32
	mutex_split = 0;
#else
	pthread_mutex_init( &mutex_split, NULL );
#endif
#if SINGULAR_EXTENSION
	exdepth = -1;
#endif
}

Tree::~Tree(){
/************************************************
デストラクタ
************************************************/
#if KILLER
	delete [] kmoves;
#endif
	delete pvmove;
#ifdef WIN32
#else
	// POSIXの仕様によれば呼ばなくていいらしい。
	//pthread_mutex_destroy( &mutex_split );
#endif
}

bool Tree::DebugPrintBan(){
/************************************************
盤面を出力
************************************************/
	ofstream fout;
	time_t t;
	fout.open( TREE_DEBUG, ios::out | ios::app );
	if( !fout ){ return false; }
	t = time( NULL );
	fout << ctime( &t );
	fout << pshogi->GetStrBan();
	fout.close();
	return true;
}

bool Tree::DebugPrintMoves(){
/************************************************
pmovesの内容を出力
************************************************/
	ofstream fout;
	fout.open( TREE_DEBUG, ios::out | ios::app );
	if( !fout ){ return false; }
	for( int i = 0 ; i < pmoves->GetNumber() ; i++ ){
		fout << Shogi::GetStrMove( (*pmoves)[i] );
		fout << '(' << (*pmoves)[i].value << "), ";
	}
	fout << '\n';
	fout.close();
	return true;
}

bool Tree::DebugPrintDepth( int depth ){
/************************************************
深さ
************************************************/
	ofstream fout;
	fout.open( TREE_DEBUG, ios::out | ios::app );
	if( !fout ){ return false; }
	fout << "depth :" << depth << '\n';
	fout << '\n';
	fout.close();
	return true;
}

int Tree::SortSeeQuies(){
/************************************************
静止探索における並べ替え
SEEでマイナスの手は除去
************************************************/
	int i;
	int value = 0;

	for( i = pmoves->GetCurrentNumber() ; i < pmoves->GetNumber() ; i++ ){
		// 駒割りの変動
		value = pshogi->CaptureValueN( (*pmoves)[i] );

		// 成る場合
		if( (*pmoves)[i].nari ){
			value += pshogi->GetV_PromoteN( (*pmoves)[i].koma );
		}

		// 指し手を進める。
		pshogi->MoveD( (*pmoves)[i] );

		// SEE
		(*pmoves)[i].value = value - pshogi->StaticExchangeN();

		// 指し手を戻す。
		pshogi->GoBack();
	}

	// 並べ替え(降順)
	pmoves->SortAfter();

	// 損する交換は除去
	// 但し、王手回避は例外
	if( !pshogi->IsCheck() ){
		for( i = 0 ; i < pmoves->GetNumber() ; i++ ){
			if( (*pmoves)[i].value < 0 ){
				pmoves->RemoveAfter( i );
				return i;
			}
		}
	}
	return pmoves->GetNumber();
}

int Tree::SortSee(){
/************************************************
SEEによる並べ替え
************************************************/
	int i;
	int value;

	for( i = pmoves->GetCurrentNumber() ; i < pmoves->GetNumber() ; i++ ){
		// 駒割りの変動
		value = pshogi->CaptureValueN( (*pmoves)[i] );

		// 成る場合
		if( (*pmoves)[i].nari ){
			value += pshogi->GetV_PromoteN( (*pmoves)[i].koma );
		}

		// 指し手を進める。
		pshogi->MoveD( (*pmoves)[i] );

		// SEE
		(*pmoves)[i].value = value - pshogi->StaticExchangeN();

		// 指し手を戻す。
		pshogi->GoBack();
	}

	// 並べ替え(降順)
	pmoves->SortAfter();

	return pmoves->GetNumber();
}

int Tree::SortSee( MOVE& hmove1, MOVE& hmove2
#if KILLER
	, MOVE& killer1, MOVE& killer2
#endif
){
/************************************************
SEEによる並べ替え
************************************************/
	int i;

	for( i = pmoves->GetCurrentNumber() ; i < pmoves->GetNumber() ; i++ ){
		// ハッシュと同じなら消す。
		if( (*pmoves)[i].IsSame( hmove1 ) ||
			(*pmoves)[i].IsSame( hmove2 ) )
		{
			pmoves->Remove( i );
			i--;
			continue;
		}

		// 駒割りの変動
		int capv = pshogi->CaptureValueN( (*pmoves)[i] );

		// 成る場合
		if( (*pmoves)[i].nari ){
			capv += pshogi->GetV_PromoteN( (*pmoves)[i].koma );
		}

		// 指し手を進める。
		pshogi->MoveD( (*pmoves)[i] );

		// SEE
		int value = capv - pshogi->StaticExchangeN();

		// 指し手を戻す。
		pshogi->GoBack();

#if KILLER
		// killer move
		if( (*pmoves)[i].IsSame( killer1 ) && value < killer1.value + capv ){
			value = killer1.value + capv;
			killer1.from = EMPTY;
			continue;
		}
		else if( (*pmoves)[i].IsSame( killer2 ) && value < killer2.value + capv ){
			value = killer2.value + capv;
			killer2.from = EMPTY;
			continue;
		}
#endif

		(*pmoves)[i].value = value;
	}

#if KILLER
	if( killer1.from != EMPTY ){
		pmoves->Add( killer1 );
	}

	if( killer2.from != EMPTY ){
		pmoves->Add( killer2 );
	}
#endif

	// 並べ替え(降順)
	pmoves->SortAfter();

	return pmoves->GetNumber();
}

void Tree::SortHistory( History* history,
	MOVE& hmove1, MOVE& hmove2
#if KILLER
	, MOVE& killer1, MOVE& killer2
#endif
){
/************************************************
historyによる並べ替え
************************************************/
	MOVE* pmove;
	int i;

	// History Huristic
	for( i = pmoves->GetCurrentNumber() ; i < pmoves->GetNumber() ; i++ ){
#if 1
		// ハッシュと同じなら消す。
		if( (*pmoves)[i].IsSame( hmove1 ) ||
			(*pmoves)[i].IsSame( hmove2 )
#if KILLER
			|| (*pmoves)[i].IsSame( killer1 )
			|| (*pmoves)[i].IsSame( killer2 )
#endif
		){
			pmoves->Remove( i );
			i--;
			continue;
		}
#endif

		pmove = &(*pmoves)[i];
		pmoves->SetValue( i, history->Get( pmove->from, pmove->to ) );
	}

	pmoves->SortAfter(); // 並べ替え(降順)
}

bool Tree::ConnectedThreat( MOVE& threat, MOVE& move ){
/************************************************
threatとの関連
************************************************/
	if( threat.from == EMPTY || move.from == EMPTY ){
		return false;
	}

#if 1
	// 移動元がthreatの移動先 => 取られるのを回避
	if( threat.to == move.from ){
		return true;
	}
#endif

#if 1
	// 移動先がthreatの移動先
	if( threat.to == move.to ){
		return true;
	}
#endif

#if 1
	// threatの移動先に利きを作る場合
	if( !pshogi->Obstacle( move.koma, threat.to, move.to ) ){
		return true;
	}
#endif

#if 1
	// threatの移動経路をふさぐ場合
	if( pshogi->IsStraight( threat.from, threat.to, move.to ) ){
		return true;
	}
#endif

	return false;
}

int Tree::EstimateValue( MOVE& move ){
/************************************************
着手後の静的評価の変化を推定
************************************************/
	KOMA koma, koma2;
	int value0 = 0;
	int value1 = 0;
	int vmax;
	const KOMA* ban;

	// 駒台
	if( move.from & DAI ){
		koma = move.from & ~DAI;

		value1 += pshogi->GetV_KingPieceS( move.to, koma );
		value1 -= pshogi->GetV_KingPieceG( move.to, koma );

		vmax = FUT_DIFF;
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

			vmax = FUT_DIFF;
		}
		// 玉
		else{
			vmax = FUT_DIFF_KING;
		}

		// 駒を取る場合
		if( ( koma2 = ban[move.to] ) ){
			value0 -= pshogi->GetV_BAN2( koma2 );
			value1 -= pshogi->GetV_KingPieceS( move.to, koma2 );
			value1 += pshogi->GetV_KingPieceG( move.to, koma2 );
		}
	}

	if( pshogi->GetSengo() == SENTE ){
		return  ( value0 + ( value1 / (int)EV_SCALE ) ) + vmax;
	}
	else{
		return -( value0 - ( value1 / (int)EV_SCALE ) ) + vmax;
	}
}

MOVE* Tree::GetNextMove( History* history
#if KILLER
	, int depth
#endif
){
/************************************************
次の合法手を取得
************************************************/
	MOVE* pmove;

	while( 1 ){
		if( ( pmove = pmoves->GetCurrentMove() ) != NULL ){
			// 現在のフェーズでまだ指し手がある場合
			return pmove;
		}

		// 新しいフェーズ
		switch( pmoves->GetPhase() ){
		// 駒を取る手
		case PHASE_CAPTURE:
			if( pshogi->IsCheck() ){
				// 王手の場合
				pmoves->SetPhase( PHASE_CAP_EVASION );
			}
			else{
				// 駒を取る手
				pmoves->SetPhase( PHASE_NOCAPTURE );
#if KILLER
				// キラームーブ
				GetKiller( depth, pmoves->GetKiller1(),
					pmoves->GetKiller2() );
#endif
				GenerateCaptures();
				SortSee( pmoves->GetHash1(), pmoves->GetHash2()
#if KILLER
					, pmoves->GetKiller1(), pmoves->GetKiller2()
#endif
					);
			}
			break;

		// 駒を取らない手
		case PHASE_NOCAPTURE:
			pmoves->SetPhase( PHASE_NULL );
			GenerateNoCaptures();
			SortHistory( history, pmoves->GetHash1(), pmoves->GetHash2()
#if KILLER
				, pmoves->GetKiller1(), pmoves->GetKiller2()
#endif
				 );
			break;

		// 王手回避
		case PHASE_CAP_EVASION:
			pmoves->SetPhase( PHASE_KING );
			GenerateCapEvasion();
			SortSee( pmoves->GetHash1(), pmoves->GetHash2()
#if KILLER
				, pmoves->GetKiller1(), pmoves->GetKiller2() 
#endif
				);
			break;
		case PHASE_KING:
			pmoves->SetPhase( PHASE_NOCAP_EVASION );
			GenerateMovesOu();
			SortSee( pmoves->GetHash1(), pmoves->GetHash2()
#if KILLER
				, pmoves->GetKiller1(), pmoves->GetKiller2() 
#endif
				);
			break;
		case PHASE_NOCAP_EVASION:
			pmoves->SetPhase( PHASE_NULL );
			GenerateNoCapEvasion();
			SortHistory( history, pmoves->GetHash1(), pmoves->GetHash2()
#if KILLER
				, pmoves->GetKiller1(), pmoves->GetKiller2()
#endif
				 );
			break;

		case PHASE_NULL:
		default:
			return NULL;
		}
	}
}

MOVE* Tree::GetNextEvasion(){
/************************************************
次の合法手を取得
seeによる並べ替えなどは行わない, 
詰み探索専用ルーチン
************************************************/
	MOVE* pmove;

	while( 1 ){
		if( ( pmove = pmoves->GetCurrentMove() ) != NULL ){
			// 現在のフェーズでまだ指し手がある場合
			return pmove;
		}

		// 新しいフェーズ
		switch( pmoves->GetPhase() ){
		case PHASE_CAP_EVASION:
			pmoves->SetPhase( PHASE_KING );
			GenerateCapEvasion();
			break;
		case PHASE_KING:
			pmoves->SetPhase( PHASE_NOCAP_EVASION );
			GenerateMovesOu();
			break;
		case PHASE_NOCAP_EVASION:
			pmoves->SetPhase( PHASE_NULL );
			GenerateNoCapEvasion( no_pro );
			break;
		case PHASE_NULL:
		default:
			return NULL;
		}
	}
}
