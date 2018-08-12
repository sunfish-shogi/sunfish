/* feature.cpp
 * R.Kubo 2010-2012
 * 局面の特徴抽出
 */

/************************************************
特徴抽出
evaluate.cpp と learn.cpp でインクルードして使う。
************************************************/
	// 局面
	const KOMA* __restrict__ ban = pshogi->GetBan();

	// 玉の位置
	int sou = pshogi->GetSOU();
	int gou = pshogi->GetGOU();

	int sou0 = _addr[sou];
	int gou0 = 80 - _addr[gou];

	// bitboard
	const uint96* __restrict__ bb = pshogi->GetBB();
	uint96 bb_all = pshogi->GetBB_All();
	uint96 bb_nking = bb_all & ~( bb[SOU] | bb[GOU] );

#ifndef DISABLE_FEATURE_2
	const int* __restrict__ dai = pshogi->GetDai();

	// 利き情報
	const unsigned int* __restrict__ effectS = pshogi->GetEffectS();
	const unsigned int* __restrict__ effectG = pshogi->GetEffectG();

	// 歩のある筋
	unsigned sfu = pshogi->GetSFU();
	unsigned gfu = pshogi->GetGFU();
#endif

#ifndef DISABLE_FEATURE_2
	// 玉-持ち駒
	FUNC_S( u_king_hand_sfu[sou0][dai[SFU]] );
	FUNC_S( u_king_hand_sky[sou0][dai[SKY]] );
	FUNC_S( u_king_hand_ske[sou0][dai[SKE]] );
	FUNC_S( u_king_hand_sgi[sou0][dai[SGI]] );
	FUNC_S( u_king_hand_ski[sou0][dai[SKI]] );
	FUNC_S( u_king_hand_ska[sou0][dai[SKA]] );
	FUNC_S( u_king_hand_shi[sou0][dai[SHI]] );

	FUNC_S( c_king_hand_gfu[sou0][dai[GFU]] );
	FUNC_S( c_king_hand_gky[sou0][dai[GKY]] );
	FUNC_S( c_king_hand_gke[sou0][dai[GKE]] );
	FUNC_S( c_king_hand_ggi[sou0][dai[GGI]] );
	FUNC_S( c_king_hand_gki[sou0][dai[GKI]] );
	FUNC_S( c_king_hand_gka[sou0][dai[GKA]] );
	FUNC_S( c_king_hand_ghi[sou0][dai[GHI]] );

	FUNC_G( u_king_hand_sfu[gou0][dai[GFU]] );
	FUNC_G( u_king_hand_sky[gou0][dai[GKY]] );
	FUNC_G( u_king_hand_ske[gou0][dai[GKE]] );
	FUNC_G( u_king_hand_sgi[gou0][dai[GGI]] );
	FUNC_G( u_king_hand_ski[gou0][dai[GKI]] );
	FUNC_G( u_king_hand_ska[gou0][dai[GKA]] );
	FUNC_G( u_king_hand_shi[gou0][dai[GHI]] );

	FUNC_G( c_king_hand_gfu[gou0][dai[SFU]] );
	FUNC_G( c_king_hand_gky[gou0][dai[SKY]] );
	FUNC_G( c_king_hand_gke[gou0][dai[SKE]] );
	FUNC_G( c_king_hand_ggi[gou0][dai[SGI]] );
	FUNC_G( c_king_hand_gki[gou0][dai[SKI]] );
	FUNC_G( c_king_hand_gka[gou0][dai[SKA]] );
	FUNC_G( c_king_hand_ghi[gou0][dai[SHI]] );
#endif //#ifdef DISABLE_FEATURE_2

#ifndef DISABLE_FEATURE_1

#ifndef LIST_S // リストをここで生成
#define LIST_GEN
	int list_s[8];     // 先手玉周りの駒リスト
	int list_snum = 0; // 先手玉周りの駒数
	int list_g[8];     // 後手玉周りの駒リスト
	int list_gnum = 0; // 後手玉周りの駒数
#define LIST_K(a,b)			\
if( ( koma = ban[sou+(b)] ) & SENGO ){ \
	list_s[list_snum++] = (a) * 18 + sente_num[koma]; \
} \
if( ( koma = ban[gou-(b)] ) & SENGO ){ \
	list_g[list_gnum++] = (a) * 18 + gote_num[koma]; \
}
	KOMA koma;
	LIST_K( 0, -0x10-0x01 );LIST_K( 1, -0x10      );LIST_K( 2, -0x10+0x01 );
	LIST_K( 3,      -0x01 );                        LIST_K( 4,      +0x01 );
	LIST_K( 5, +0x10-0x01 );LIST_K( 6, +0x10      );LIST_K( 7, +0x10+0x01 );
#undef LIST_K

#define LIST_S				(list_s)
#define LIST_G				(list_g)
#define LIST_SNUM			(list_snum)
#define LIST_GNUM			(list_gnum)
#endif

// 盤上
{
	uint96 bb_temp = bb_nking;
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;

		// 駒番号
		int sk, gk;
		{
			KOMA koma = ban[addr];
			assert( koma != EMPTY );
			assert( koma != WALL );
			sk = sente_num[koma];
			gk = gote_num[koma];
		}

		// 王-他の駒
#ifndef DISABLE_SOU
		FUNC_S( u_king_piece[sou0][   a][sk] );
#endif
#ifndef DISABLE_GOU
		FUNC_G( u_king_piece[gou0][80-a][gk] );
#endif

		// 相対位置
#ifndef DISABLE_SOU
		int diffS = _aa2num[sou0][a];
#endif
#ifndef DISABLE_GOU
		int diffG = _aa2num[gou0][80-a];
#endif

		{
			// 王-8近傍-その他
			int i;
#ifndef DISABLE_SOU
			for( i = 0 ; i < LIST_SNUM ; i++ ){
				FUNC_S( king8_piece[0][LIST_S[i]][diffS][sk] );
			}
#endif
#ifndef DISABLE_GOU
			for( i = 0 ; i < LIST_GNUM ; i++ ){
				FUNC_G( king8_piece[0][LIST_G[i]][diffG][gk] );
			}
#endif
		}

		{
			// 王-隣接2枚
			KOMA koma2;
			if( 0x07 & ( koma2 = ban[addr+0x00+0x01] ) ){
#ifndef DISABLE_SOU
				FUNC_S( king_adjoin_r [diffS][sk][sente_num[koma2]] );
#endif
#ifndef DISABLE_GOU
				FUNC_G( king_adjoin_r [diffG-1][gote_num[koma2]][gk] );
#endif
			}
			if( 0x07 & ( koma2 = ban[addr+0x10-0x01] ) ){
#ifndef DISABLE_SOU
				FUNC_S( king_adjoin_ld[diffS][sk][sente_num[koma2]] );
#endif
#ifndef DISABLE_GOU
				FUNC_G( king_adjoin_ld[diffG-16][gote_num[koma2]][gk] );
#endif
			}
			if( 0x07 & ( koma2 = ban[addr+0x10+0x00] ) ){
#ifndef DISABLE_SOU
				FUNC_S( king_adjoin_d [diffS][sk][sente_num[koma2]] );
#endif
#ifndef DISABLE_GOU
				FUNC_G( king_adjoin_d [diffG-17][gote_num[koma2]][gk] );
#endif
			}
			if( 0x07 & ( koma2 = ban[addr+0x10+0x01] ) ){
#ifndef DISABLE_SOU
				FUNC_S( king_adjoin_rd[diffS][sk][sente_num[koma2]] );
#endif
#ifndef DISABLE_GOU
				FUNC_G( king_adjoin_rd[diffG-18][gote_num[koma2]][gk] );
#endif
			}
		}
	}
}

	// Debug
#ifdef __LEARN__
	assert( p->king_piece[_aa2num[0][0]][sente_num[SKI]] == 0.0 );
#endif
#ifdef __EVALUATE__
	assert( king_piece[_aa2num[0][0]][sente_num[SKI]] == 0 );
#endif

{
	// 相対位置
	int i;
	int diff = _aa2num[sou0][80-gou0];

	assert( diff != _aa2num[0][0] );

	// 先手玉-8近傍-後手玉
	for( i = 0 ; i < LIST_SNUM ; i++ ){
		FUNC_SO( king8_piece[0][LIST_S[i]][diff][18/*sente_num[SOU]*/] );
	}

	// 後手玉-8近傍-先手玉
	for( i = 0 ; i < LIST_GNUM ; i++ ){
		FUNC_GO( king8_piece[0][LIST_G[i]][diff][18/*sente_num[SOU]*/] );
	}
}

#ifdef LIST_GEN
#	undef LIST_S
#	undef LIST_G
#	undef LIST_SNUM
#	undef LIST_GNUM
#endif

#endif //#ifdef DISABLE_FEATURE_1

#ifndef DISABLE_FEATURE_2
{
	// 先手の歩(2マス前に駒がある場合のみ)
	uint96 bb_temp = bb[SFU] & (bb_all<<18);
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == SFU );

		// 歩の2マス前の駒
		KOMA koma = ban[addr-0x20];
		// 相対位置
		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];

		FUNC_S( front2_sfu[diffS][sente_num[koma]] );
		FUNC_G( front2_gfu[diffG][gote_num[koma]] );
	}
}

{
	// 後手の歩(2マス前に駒がある場合のみ)
	uint96 bb_temp = bb[GFU] & (bb_all>>18);
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == GFU );

		// 歩の2マス前の駒
		KOMA koma = ban[addr+0x20];
		// 相対位置
		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];

		FUNC_S( front2_gfu[diffS][sente_num[koma]] );
		FUNC_G( front2_sfu[diffG][gote_num[koma]] );
	}
}

#if 0 // 左と右で分離
{
	// 先手の桂 + 左側の利き
	uint96 bb_temp = bb[SKE] & (bb_all<<19) & nf1;
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == SKE );

		KOMA koma = ban[addr-0x20-0x01];
		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		FUNC_S( u_kingS_ke_l[diffS][sente_num[koma]] );
		FUNC_G(   kingG_ke_l[diffG][ gote_num[koma]] );
	}
}

{
	// 先手の桂 + 右側の利き
	uint96 bb_temp = bb[SKE] & (bb_all<<17) & nf9;
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == SKE );

		KOMA koma = ban[addr-0x20+0x01];
		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		FUNC_S( u_kingS_ke_r[diffS][sente_num[koma]] );
		FUNC_G(   kingG_ke_r[diffG][ gote_num[koma]] );
	}
}

{
	// 後手の桂 + 左側の利き
	uint96 bb_temp = bb[GKE] & (bb_all>>19) & nf9;
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == GKE );

		KOMA koma = ban[addr+0x20+0x01];
		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		FUNC_S(   kingG_ke_l[diffS][sente_num[koma]] );
		FUNC_G( u_kingS_ke_l[diffG][ gote_num[koma]] );
	}
}

{
	// 後手の桂 + 右側の利き
	uint96 bb_temp = bb[GKE] & (bb_all>>17) & nf1;
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == GKE );

		KOMA koma = ban[addr+0x20-0x01];
		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		FUNC_S(   kingG_ke_r[diffS][sente_num[koma]] );
		FUNC_G( u_kingS_ke_r[diffG][ gote_num[koma]] );
	}
}
#else
{
	// 先手の桂
	uint96 bb_temp = bb[SKE];
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == SKE );

		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		KOMA koma;
		if( 0x07 & ( koma = ban[addr-0x20-0x01] ) ){
			FUNC_S( u_kingS_ke_l[diffS][sente_num[koma]] );
			FUNC_G(   kingG_ke_l[diffG][ gote_num[koma]] );
		}
		if( 0x07 & ( koma = ban[addr-0x20+0x01] ) ){
			FUNC_S( u_kingS_ke_r[diffS][sente_num[koma]] );
			FUNC_G(   kingG_ke_r[diffG][ gote_num[koma]] );
		}
	}
}

{
	// 後手の桂
	uint96 bb_temp = bb[GKE];
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == GKE );

		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		KOMA koma;
		if( 0x07 & ( koma = ban[addr+0x20+0x01] ) ){
			FUNC_S(   kingG_ke_l[diffS][sente_num[koma]] );
			FUNC_G( u_kingS_ke_l[diffG][ gote_num[koma]] );
		}
		if( 0x07 & ( koma = ban[addr+0x20-0x01] ) ){
			FUNC_S(   kingG_ke_r[diffS][sente_num[koma]] );
			FUNC_G( u_kingS_ke_r[diffG][ gote_num[koma]] );
		}
	}
}
#endif

	// 跳び駒の利き上の駒、自由度
#define	EFF_OPE( _GETBIT, dname, dir, SG, S, G, free, effect )				{ \
	register int diss; \
	register int x; \
	if( ( x = bb_all._GETBIT(_dbits_##dname[a]) ) != 0 ){ \
		x = _iaddr0[x]; \
		diss = ( x - addr ) / (dir); \
		register KOMA k = ban[x]; \
		if( k & (SG) ){ diss--; } \
		FUNC_S( effect[S][diffS][sente_num[k]] ); \
		FUNC_G( effect[G][diffG][ gote_num[k]] ); \
	} \
	else{ \
		diss = wdiss_ ## dname [a]; \
	} \
	FUNC_S( free[S][diffS][diss] ); \
	FUNC_G( free[G][diffG][diss] ); \
	cnt += diss; \
}

{
	// 先手の香
	uint96 bb_temp = bb[SKY];
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == SKY );

		int cnt = 0;
		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		EFF_OPE( getLastBit, up, -16, SENTE, S, G, c_free_ky, effect_ky )
	}
}

{
	// 後手の香
	uint96 bb_temp = bb[GKY];
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == GKY );

		int cnt = 0;
		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		EFF_OPE( getFirstBit, dn, +16, GOTE, G, S, c_free_ky, effect_ky );
	}
}

{
	// 先手の角と馬
	uint96 bb_temp = bb[SKA] | bb[SUM];
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == SKA || ban[addr] == SUM );

		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		int cnt = 0;
		EFF_OPE( getLastBit , lu, -17, SENTE, S, G, c_free_ka_r, effect_ka_r )
		EFF_OPE( getLastBit , ru, -15, SENTE, S, G, c_free_ka_l, effect_ka_l )
		EFF_OPE( getFirstBit, ld, +15, SENTE, S, G, c_free_ka_R, effect_ka_R )
		EFF_OPE( getFirstBit, rd, +17, SENTE, S, G, c_free_ka_L, effect_ka_L )
		FUNC_S( c_free_ka_a[S][diffS][cnt] );
		FUNC_G( c_free_ka_a[G][diffG][cnt] );
	}
}

{
	// 後手の角と馬
	uint96 bb_temp = bb[GKA] | bb[GUM];
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == GKA || ban[addr] == GUM );

		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		int cnt = 0;
		EFF_OPE( getLastBit , lu, -17, GOTE, G, S, c_free_ka_L, effect_ka_L );
		EFF_OPE( getLastBit , ru, -15, GOTE, G, S, c_free_ka_R, effect_ka_R );
		EFF_OPE( getFirstBit, ld, +15, GOTE, G, S, c_free_ka_l, effect_ka_l );
		EFF_OPE( getFirstBit, rd, +17, GOTE, G, S, c_free_ka_r, effect_ka_r );
		FUNC_S( c_free_ka_a[G][diffS][cnt] );
		FUNC_G( c_free_ka_a[S][diffG][cnt] );
	}
}

{
	// 先手の飛と竜
	uint96 bb_temp = bb[SHI] | bb[SRY];
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == SHI || ban[addr] == SRY );

		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		int cnt = 0;
		EFF_OPE( getLastBit , up, -16, SENTE, S, G, c_free_hi_f, effect_hi_f )
		EFF_OPE( getLastBit , lt, - 1, SENTE, S, G, c_free_hi_r, effect_hi_r )
		EFF_OPE( getFirstBit, rt, + 1, SENTE, S, G, c_free_hi_l, effect_hi_l )
		EFF_OPE( getFirstBit, dn, +16, SENTE, S, G, c_free_hi_b, effect_hi_b )
		FUNC_S( c_free_hi_a[S][diffS][cnt] );
		FUNC_G( c_free_hi_a[G][diffG][cnt] );
	}
}

{
	// 後手の飛と竜
	uint96 bb_temp = bb[GHI] | bb[GRY];
	for( int a = bb_temp.pickoutFirstBit() ; a != 0 ; a = bb_temp.pickoutFirstBit() ){
		int addr = _iaddr0[a];
		a--;
		assert( ban[addr] == GHI || ban[addr] == GRY );

		int diffS = _aa2num[sou0][a];
		int diffG = _aa2num[gou0][80-a];
		int cnt = 0;
		EFF_OPE( getLastBit , up, -16, GOTE, G, S, c_free_hi_b, effect_hi_b );
		EFF_OPE( getLastBit , lt, - 1, GOTE, G, S, c_free_hi_l, effect_hi_l );
		EFF_OPE( getFirstBit, rt, + 1, GOTE, G, S, c_free_hi_r, effect_hi_r );
		EFF_OPE( getFirstBit, dn, +16, GOTE, G, S, c_free_hi_f, effect_hi_f );
		FUNC_S( c_free_hi_a[G][diffS][cnt] );
		FUNC_G( c_free_hi_a[S][diffG][cnt] );
	}
}

	// 歩を打てる筋
	FUNC_S( c_sfu_file[sou0][sfu>>1] ); FUNC_G( c_gfu_file[gou0][_brev9[sfu>>1]] );
	FUNC_S( c_gfu_file[sou0][gfu>>1] ); FUNC_G( c_sfu_file[gou0][_brev9[gfu>>1]] );

{
	// 玉の周りの利きの勝ち負け
#define EFFWIN(a,b)		\
if( Shogi::CountEffect(effectS[sou+(a)]) > Shogi::CountEffect(effectG[sou+(a)]) ){ \
FUNC_S( king_effwin[(b)] ); \
}
	EFFWIN(-0x10-0x01,8);EFFWIN(-0x10     ,7);EFFWIN(-0x10+0x01,4);
	EFFWIN(     -0x01,5);                    ;EFFWIN(     +0x01,5);
	EFFWIN(+0x10-0x01,2);EFFWIN(+0x10     ,1);EFFWIN(+0x10+0x01,0);
#undef EFFWIN

#define EFFWIN(a,b)		\
if( Shogi::CountEffect(effectG[gou-(a)]) > Shogi::CountEffect(effectS[gou-(a)]) ){ \
FUNC_G( king_effwin[(b)] ); \
}
	EFFWIN(-0x10-0x01,8);EFFWIN(-0x10     ,7);EFFWIN(-0x10+0x01,4);
	EFFWIN(     -0x01,5);                    ;EFFWIN(     +0x01,5);
	EFFWIN(+0x10-0x01,2);EFFWIN(+0x10     ,1);EFFWIN(+0x10+0x01,0);
#undef EFFWIN
}

{
	// 玉の位置
	FUNC_S( king_addr[sou0] );
	FUNC_G( king_addr[gou0] );
}

{
	// 歩打ちで王手できるか
	if( !( gfu & ( 1 << Addr2X(sou) ) ) // 王様の筋に歩がない。
		&& ban[sou-0x10] == EMPTY       // 玉の前に駒がない。
		&& dai[GFU] != 0                // 歩を持っている。
		&& Addr2Y(sou) != 1             // 9段目ではない。
	){
		FUNC_S( king_check_fu[sou0] );
	}

	if( !( sfu & ( 1 << Addr2X(gou) ) ) // 王様の筋に歩がない。
		&& ban[gou+0x10] == EMPTY       // 玉の前に駒がない。
		&& dai[SFU] != 0                // 歩を持っている。
		&& Addr2Y(gou) != 9             // 9段目ではない。
	){
		FUNC_G( king_check_fu[gou0] );
	}
}

	// Debug
#ifdef __LEARN__
	assert( p->king_piece[_aa2num[0][0]][sente_num[SKI]] == 0.0 );
#endif
#ifdef __EVALUATE__
	assert( king_piece[_aa2num[0][0]][sente_num[SKI]] == 0 );
#endif
#endif //#ifdef DISABLE_FEATURE_2
