/* learn2.cpp
 * R.Kubo 2010
 * 機械学習(Averaged Perceptron)
 */

#ifndef NLEARN

#include "match.h"

// 学習時の探索深さ
#define L2_DEPTH			3
#define L2_QDEPTH			7

#define MOVE_SELECT			16 // 合法手からランダムに選択する手の数

LearnAP::LearnAP(){
/************************************************
コンストラクタ
************************************************/
	pparam = new Param();    // 最新のパラメータ(実数)
	pcumurate = new Param(); // パラメータの累計
}

LearnAP::~LearnAP(){
/************************************************
デストラクタ
************************************************/
	delete pparam;
	delete pcumurate;
}

bool LearnAP::LearnAPMain(){
/************************************************
学習のメインルーチン
************************************************/
	// 設定読み込み
	LoadConfig();

	// 設定書き出し
	ConfigLog();

	memset( &info, 0, sizeof(info) );

	return LearnAPMethod();
}

bool LearnAP::LearnAPMethod(){
/************************************************
学習処理
************************************************/
	Timer tm;
	int t;

	// 全体の時間計測
	tm.SetTime();

	// ファイルの列挙
	cout << "enumerating CSA files.." << '\n';
	cout.flush();

	flist.Clear();
	if( 0 == flist.Enumerate( directory, "csa" ) ){
		cout << "There is no CSA files!" << '\n';
		return false;
	}

	// 局面の列挙
	EnumeratePos();

	// シャッフル
	cout << "positions shuffling.." << '\n';
	cout.flush();

	pos_list.Shuffle();

	// AveragedPerceptron
	AveragedPerceptron();

	// 処理時間を計算
	t = (int)tm.GetTime();
	cout << "Complete! " << '\n';
	cout << "Total Time      : " << t << "sec" << '\n';

	return true;
}

void LearnAP::EnumeratePos(){
/************************************************
局面の列挙
ものすごく遅い->あとで考える。
************************************************/
	string fname;
	Shogi* pshogi;

	cout << "enumerating positions.." << '\n';
	cout.flush();

	pshogi = new Shogi();

	pos_list.Clear();
	flist.Begin();
	while( flist.Pop( fname ) ){
		if( pshogi->InputFileCSA( fname.c_str() ) ){
			while( pshogi->GoBack() ){
				pos_list.Enqueue( fname, pshogi->GetNumber() );
			}
		}
	}

	delete pshogi;
}

void LearnAP::AveragedPerceptron(){
/************************************************
Averaged Perceptron
************************************************/
	string fname;           // 棋譜のファイル名
	int npos;               // 棋譜中での局面の位置
	ShogiEval* pshogi;      // 棋譜と盤面
	Think* pthink;          // 思考部
	MOVE move0;             // 棋譜の指し手
	Moves moves;            // 比較する指し手
	Moves pv0( MAX_DEPTH ); // PV(棋譜の指し手)
	Moves pv( MAX_DEPTH );  // PV
	int val0;               // 評価値(棋譜の指し手)
	int val;                // 評価値
	bool outputed = false;  // 保存済み
	double sign;            // 先後に応じた符号
	int move_cnt;           // 扱った指し手の数
	Param* ptemp;           // パラメータの途中計算用
	int ev_margin;          // 評価値のマージン
	int mnum;               // 指し手の数
	int i, j;

	cout << "Averaged perceptron method.." << '\n';
	cout.flush();

	// 思考部
	pthink = new Think();      // メモリ確保と初期化
	pthink->IsLearning(); // 学習用

	// 途中計算用メモリ
	ptemp = new Param();

	// 初期値
	(*pparam) = (*eval);

	// evalはどんどん更新されるけど、
	// 棋譜を読み込むたびに評価値は計算しなおすから大丈夫(多分)..
	pshogi = new ShogiEval( eval );

	cum_cnt = 0;
	while( pos_list.Dequeue( fname, npos ) ){
		// 棋譜の読み込み
		if( !pshogi->InputFileCSA( fname.c_str() ) ){
			continue;
		}

		// 局面の移動
		while( pshogi->GetNumber() > npos && pshogi->GoBack() )
			;

		// Debug 局面表示
		cout << '\n';
		pshogi->PrintBan();
		cout.flush();

		// 先後
		if( pshogi->GetSengo() == SENTE ){
			sign = 1.0;
		}
		else{
			sign = -1.0;
		}

		// マージン
		ev_margin = EvalMargin( pshogi->GetNumber() );

		pshogi->GetNextMove( move0 );   // 棋譜の指し手を取得
		moves.Init();                   // moves初期化
		pshogi->GenerateMoves( moves ); // 指し手の列挙
		moves.Remove( move0 );          // 棋譜の指し手を除外
		moves.Shuffle();                // 指し手をシャッフル

		// Debug 指し手表示
		pshogi->PrintMove( move0 );
		cout.flush();

		pthink->InitHashTable();        // ハッシュテーブルの初期化

		// 棋譜の指し手について探索
		pshogi->MoveD( move0 ); // 1手進める。
#if 0
		val0 = -pthink->NegaMax( pshogi, L2_DEPTH,
			VMIN, VMAX, NULL, NULL );
#else // 反復深化探索
		THINK_INFO info;
		pthink->Search( pshogi, NULL, L2_DEPTH,
			&info, NULL, Think::SEARCH_NULL );
		val0 = -info.value;
#endif
		pshogi->GoBack(); // 1手戻す。

		if( val0 == VMIN || val0 == VMAX ){
			continue; // 詰みなら除外
		}

		val0 -= ev_margin; // マージン

		pthink->GetPVMove( pv0 ); // PVを取得

		mnum = min( moves.GetNumber(), MOVE_SELECT ); // 16手にしぼる。

		// 各指し手について..
		move_cnt = 0;
		ptemp->Init0();
		for( i = 0 ; i < mnum ; i++ ){
			// Debug 指し手表示
			pshogi->PrintMove( moves[i] );
			cout.flush();

			pshogi->MoveD( moves[i] ); // 1手進める。
#if 0
			val = -pthink->NegaMax( pshogi, L2_DEPTH,
				-(val0+256), -val0, NULL, NULL );
#else // 反復深化探索
			val = -pthink->IDeepening( pshogi, L2_DEPTH,
				-(val0+256), -val0, NULL, NULL, NULL );
#endif

			// 棋譜の指し手より評価が高いものに限定
			if( val <= val0 ||
				val == VMAX || val == VMIN )
			{
				pshogi->GoBack(); // 1手戻す。
				continue;
			}

			pthink->GetPVMove( pv ); // PVを取得
			for( j = 0 ; j < pv.GetNumber() ; j++ ){
				pshogi->MoveD( pv[j] ); // PVをたどる。
			}

			IncParam( pshogi, ptemp, -( 1.0 * sign ) ); // 勾配

			// 局面を戻す。
			while( pshogi->GetNumber() > npos && pshogi->GoBack() )
				;

			move_cnt++;
		}

		if( move_cnt == 0 ){
			continue; // 比較するべき指し手がなかった場合
		}

		(*ptemp) *= 1.0 / (double)move_cnt; // 手の数で割る。

		// 棋譜の指し手について..
		pshogi->MoveD( move0 ); // 1手進める。
		for( j = 0 ; j < pv0.GetNumber() ; j++ ){
			pshogi->MoveD( pv0[j] ); // PVをたどる。
		}

		IncParam( pshogi, ptemp, 1.0 * sign ); // 勾配

		// 最後なので局面は戻さなくていい

		ptemp->Symmetry();           // 対称化
		(*pparam) += (*ptemp);       // パラメータ更新
		(*ptemp) *= (double)cum_cnt;
		(*pcumurate) += (*ptemp);    // 累計

		// Debug
		assert( ptemp->king_piece[_aa2num[0][0]][sente_num[SKI]] == 0.0 );
		assert( pparam->king_piece[_aa2num[0][0]][sente_num[SKI]] == 0.0 );
		assert( pcumurate->king_piece[_aa2num[0][0]][sente_num[SKI]] == 0.0 );

		if( cum_cnt % 100 == 0 ){
			OutputAverage(); // 平均を保存
			outputed = true; // 保存済み
		}
		else{
			outputed = false; // 未保存
		}

		(*eval) = (*pparam); // 整数に直す。
		eval->Update();      // 更新時の処理

		cum_cnt++;
	}

	// 平均を保存
	if( !outputed ){ // 未保存なら
		OutputAverage();
	}

	// 解放
	delete ptemp;
	delete pthink;
	delete pshogi;
}

void LearnAP::OutputAverage(){
/************************************************
平均を保存
************************************************/
	if( cum_cnt != 0 ){
		(*pcumurate) *= 1.0 / (double)cum_cnt;
		(*pparam) -= (*pcumurate);
		(*eval) = (*pparam); // 整数に直す。

		// Debug
		assert( eval->king_piece[_aa2num[0][0]][sente_num[SKI]] == 0 );

		eval->WriteData( FNAME_EVDATA );
	}
}

int LearnAP::EvalMargin( int num ){
/************************************************
手数からマージンを計算
本当は進行度とかを使いたい。
************************************************/
	return (int)( num * 2.0 );
}

#endif
