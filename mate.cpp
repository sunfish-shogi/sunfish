/* mate.cpp
 * R.Kubo 2011-2012
 * 詰み探索ルーチン
 */

 #include "match.h"

bool Think::Mate1Ply( Tree* tree, MOVE* pmove ){
/************************************************
1手詰みを調べる。
************************************************/
	int i;

	// 王手の列挙
	tree->InitMoves();
	tree->GenerateCheck();

	for( i = 0 ; i < tree->GetMovesNumber() ; i++ ){
		// make move
		tree->MakeMove( tree->GetMove( i ), false );

		if( tree->IsMate() ){
			// 詰み
			tree->UnmakeMove();
			if( pmove ){ *pmove = tree->GetMove( i ); }
			return true;
		}

		// unmake move
		tree->UnmakeMove();
	}

	// 不詰み
	return false;
}

bool Think::Mate3Ply( Tree* tree ){
/************************************************
3手詰みを調べる。
************************************************/
	int i;

	// 王手の列挙
	tree->InitMoves();
	tree->GenerateCheck();

	for( i = 0 ; i < tree->GetMovesNumber() ; i++ ){
		// make move
		tree->MakeMove( tree->GetMove( i ), false );

		if( tree->IsMate() || MateAnd( tree, false ) ){
			// 詰み
			tree->UnmakeMove();
			return true;
		}

		// unmake move
		tree->UnmakeMove();
	}

	// 不詰み
	return false;
}

bool Think::MateAnd( Tree* tree, bool captured ){
/************************************************
Andノード
************************************************/
	MOVE* pmove;
	bool mate;

	// 王手回避手の列挙
	// 2回目以降は紐の無い合駒はなし
	tree->InitEvasionGenerator( !captured );
	while( ( pmove = tree->GetNextEvasion() ) != NULL ){
		// make move
		tree->MakeMove( *pmove, false );

		// 1手詰みを調べる。
		mate = Mate1Ply( tree, NULL );

#if 1
		if( !mate ){
			// 不詰み?
			// 無駄合いなら取る手で延長
			MOVE move;
			// タダ捨ての合駒か?
			if( !tree->IsKingMove() && !tree->IsCapture() ){
				// 合駒を取る手を生成(絶対true?)
				if( tree->CapInterposition( move ) ){
					// ごくまれに非合法手もあり得る。
					// (ピン、移動合いによる逆王手)
					if( tree->IsLegalMove( move ) ){
						// make move
						tree->MakeMove( move, false );
	
						// 駒を取って再帰
						mate = MateAnd( tree, true );

						// unmake move
						tree->UnmakeMove();
					}
				}
				else{
#ifndef NDEBUG
					tree->pshogi->PrintBan();
					assert( false );
#endif
				}
			}
		}
#endif

		// unmake move
		tree->UnmakeMove();

		// 不詰み
		if( !mate ){ return false; }
	}

	// 詰み
	return true;
}

void Think::DfPn( Tree* tree, int check, int* ppn, int* pdn ){
/************************************************
df-pn
************************************************/
	uint64 hash;
	int pn, dn;
	int pni, dni;
	int sum_pn, _pn, min_dn, min_dn2, nmin;
	int i;
	MOVE* pmove;

	info.node++;

	hash = tree->GetHash();

	// ハッシュテーブルを参照
	dfpn_table->GetValue( hash, check, pn, dn );

	// 閾値を超えていたら終了
	if( pn >= *ppn || dn >= *pdn ){
		*ppn = pn;
		*pdn = dn;
		return;
	}

	tree->InitMoves();

	// 手生成
	if( check ){
		// 攻め方
		tree->GenerateCheck();
	}
	else{
		// 玉方
		tree->GenerateMoves();
	}

	if( tree->GetMovesNumber() == 0 ){
		// 合法手無しなら手番側負け
		*ppn = INT_MAX;
		*pdn = 0;
		dfpn_table->Entry( hash, check, *ppn, *pdn );
		return;
	}

	// スタックが足りない。
	if( tree->pmoves >= &tree->moves[MAX_DEPTH-1] ){
		return;
	}

	switch( tree->ShekCheck() ){
	case SHEK_EQUAL:  // 等しい局面
		break;
	case SHEK_PROFIT: // 得な局面
	case SHEK_LOSS:   // 損な局面
	case SHEK_FIRST:  // 初めての局面
		dfpn_table->Entry( hash, check, *ppn, *pdn );
		break;
	}

	while( 1 ){
		for( i = 0 ; i < tree->GetMovesNumber() ; i++ ){
			pmove = &tree->GetMove( i );
			tree->MakeMove( *pmove, false );
			/*
			// SHEKをdf-pnのループ検出に利用
			switch( tree->ShekCheck() ){
			case SHEK_EQUAL:  // 等しい局面
				// 不詰
				if( check ){
					pmove->pn = 0;
					pmove->dn = INT_MAX;
				}
				else{
					pmove->pn = INT_MAX;
					pmove->dn = 0;
				}
			case SHEK_PROFIT: // 得な局面
			case SHEK_LOSS:   // 損な局面
			case SHEK_FIRST:  // 初めての局面
			*/
				dfpn_table->GetValue( tree->GetHash(), !check, pmove->pn, pmove->dn );
			/*
				break;
			}
			*/
			tree->UnmakeMove();
		}

		sum_pn = 0;
		_pn = 0;
		min_dn = INT_MAX;
		min_dn2 = INT_MAX;
		nmin = 0;
		for( i = 0 ; i < tree->GetMovesNumber() ; i++ ){
			pmove = &tree->GetMove( i );
			pn = pmove->pn;
			dn = tree->GetMove( i ).dn;

			// 反証数の合計
			if( INT_MAX - sum_pn >= pn ){
				sum_pn += pn;
			}
			else{
				sum_pn = INT_MAX;
			}

			// 証明数の最小値
			if( dn < min_dn ){
				min_dn2 = min_dn;
				min_dn = dn;
				nmin = i;
				_pn = pn;
			}
			else if( dn < min_dn2 ){
				min_dn2 = dn;
			}

			// 証明数が無限大
			if( pn == INT_MAX )
				break;
		}

		// 閾値を超えたら終了
		if( min_dn >= *ppn || sum_pn >= *pdn ){
			*ppn = min_dn;
			*pdn = sum_pn;
			dfpn_table->Entry( hash, check, *ppn, *pdn );
			return;
		}

		// 子ノードの閾値
		pni = *pdn - ( sum_pn - _pn );
		if( pni < 0 ) pni = 0;
		dni = ( *ppn <= min_dn2 ? *ppn : min_dn2 + 1 );

		// 子ノードを探索
		pmove = &tree->GetMove( nmin );
		tree->ShekSet();
		tree->MakeMove( *pmove, false );
		DfPn( tree, !check, &pni, &dni );
		tree->UnmakeMove();
		tree->ShekUnset();

		// 打ち切りフラグ
		if( iflag != NULL && (*iflag) != 0 ){
			*ppn = INT_MAX;
			*pdn = 0;
			// ハッシュ表に登録はしない。
			return;
		}
	}
}

int Think::PrincipalVariationDfPnOr( Tree* tree, int depth ){
/************************************************
df-pnの結果からPVを取得(or node)
************************************************/
	int value;
	int rep;
	int pn, dn;
	int max_pn = -1;
	int best = 0;
	MOVE move;

	//cout << "'or  " << depth << endl;
	//tree->pshogi->PrintBan();
	//cout.flush();

	tree->InitNode( depth );

	// 千日手
	if( depth != 0 && ( IS_REP == ( rep = tree->IsRepetition() )
		|| PC_WIN == rep || PC_LOS == rep ) )
	{
		return INT_MAX;
	}

	// 1手詰み
	if( Mate1Ply( tree, &move ) ){
		tree->InitNode( depth+1 );
		tree->PVCopy( tree, depth, move );
		return depth + 1;
	}

	dfpn_table->GetValue( tree->GetHash(), 0, pn, dn );
	if( !( pn == 0 || dn >= INT_MAX - 1 ) ){
		pn = INT_MAX - 1;
		dn = INT_MAX - 1;
		dfpn_table->Init();
		DfPn( tree, 1, &pn, &dn );
	}

	tree->InitMoves();
	tree->GenerateCheck(); // 指し手生成

	if( tree->GetMovesNumber() == 0 ){ return INT_MAX; } // 不詰み

	for( int i = 0 ; i < tree->GetMovesNumber() ; i++ ){
		tree->MakeMove( tree->GetMove( i ), false );
		dfpn_table->GetValue( tree->GetHash(), 0, pn, dn );
		if( pn - dn > max_pn ){
			max_pn = pn - dn;
			best = i;
		}
		tree->UnmakeMove();
	}

	tree->MakeMove( tree->GetMove( best ), false );
	value = PrincipalVariationDfPnAnd( tree, depth + 1 );
	tree->UnmakeMove();
	tree->PVCopy( tree, depth, tree->GetMove( best ) );

	return value;
}

int Think::PrincipalVariationDfPnAnd( Tree* tree, int depth ){
/************************************************
df-pnの結果からPVを取得(and node)
************************************************/
	int value = 0;
	int rep;

	//cout << "'and " << depth << endl;
	//tree->pshogi->PrintBan();
	//cout.flush();

	tree->InitNode( depth );

	tree->InitMoves();
	tree->GenerateMoves(); // 指し手生成

	if( tree->GetMovesNumber() == 0 ){ return depth; } // 詰み

	// 千日手
	if( IS_REP == ( rep = tree->IsRepetition() )
		|| PC_WIN == rep || PC_LOS == rep )
	{
		return INT_MAX;
	}

	for( int i = 0 ; i < tree->GetMovesNumber() ; i++ ){
		int v;

		tree->MakeMove( tree->GetMove( i ), false );
		v = PrincipalVariationDfPnOr( tree, depth + 1 );
		tree->UnmakeMove();

		if( v > value ){
			value = v;
			tree->PVCopy( tree, depth, tree->GetMove( i ) );
			if( value == INT_MAX ){
				break;
			}
		}
	}
	return value;
}

int Think::DfPnSearch( ShogiEval* pshogi, MOVE* pmove,
	Moves* pmoves, THINK_INFO* _info, const int* _iflag ){
/************************************************
df-pnで詰みを探す。
************************************************/
	Timer tm;
	int pn = INT_MAX - 1;
	int dn = INT_MAX - 1;
	int max_pn;
	int ret = 0;
	int i, j;
	double d;

	iflag = _iflag;

	// 処理時間計測開始
	tm.SetTime();

	// 探索情報の初期化
	memset( &info, 0, sizeof(THINK_INFO) );

	// trees[0]に局面を設定
	trees[0].Copy( *pshogi );

	if( pmoves == NULL ){
		dfpn_table->Init();

		DfPn( &(trees[0]), 1, &pn, &dn );

		if( pn == 0 || dn >= INT_MAX - 1 ){
			max_pn = -1;
			trees[0].InitMoves();
			trees[0].GenerateCheck();
			for( i = 0 ; i < trees[0].GetMovesNumber() ; i++ ){
				// make move
				trees[0].MakeMove( trees[0].GetMove( i ), false );

				// 千日手チェック
				// DAGとloopの検出ができるまでとりあえず..
				int rep;
				bool bRep = false;

				if( IS_REP == ( rep = trees[0].IsRepetition() )
					|| PC_WIN == rep || PC_LOS == rep // IS_LOSはありえない(?)
				){
					bRep = true;
				}

				if( !bRep ){
					// 相手番での千日手チェック
					trees[0].InitMoves();
					trees[0].GenerateMoves();
					for( j = 0 ; j < trees[0].GetMovesNumber() ; j++ ){
						// make move
						trees[0].MakeMove( trees[0].GetMove( j ), false );

						if( IS_REP == ( rep = trees[0].IsRepetition() )
							|| PC_WIN == rep || PC_LOS == rep // IS_WINはありえない(?)
						){
							bRep = true;
						}

						// unmake move
						trees[0].UnmakeMove();
					}
				}

				// 証明数と反証数を取得
				dfpn_table->GetValue( trees[0].GetHash(), 0, pn, dn );

				// unmake move
				trees[0].UnmakeMove();

				if( pn - dn > max_pn ){
					max_pn = pn - dn;
					if( !bRep ){ ret = 1; }
					else       { ret = 0; }
					if( pmove ){ *pmove = trees[0].GetMove( i ); }
				}
			}
		}
	}
	else{
		// 詰み手順取得
		if( INT_MAX != PrincipalVariationDfPnOr( &trees[0], 0 ) ){
			ret = 1;
			trees[0].pvmove->GetMoves( *pmoves );
			if( pmove ){ *pmove = (*pmoves)[0]; }
		}
		else{
			trees[0].pvmove->GetMoves( *pmoves );
			ret = 0;
		}
	}

	// 評価値
	info.value = ( ret ? VMAX : 0 );

	// 処理時間
	d = tm.GetTime();
	info.msec = (int)( d * 1.0e+3 );

	// node / second
	info.nps = (int)( (int)info.node / d );

	// 探索情報
	if( _info != NULL ){
		*_info = info;
	}

	return ret;
}
