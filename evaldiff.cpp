/* evaldiff.cpp
 * R.Kubo 2012
 * 局面評価の差分計算
 */

#include "shogi.h"

#define __EVALUATE__

#define LIST_S				(el->list_s)
#define LIST_G				(el->list_g)
#define LIST_SNUM			(el->list_snum)
#define LIST_GNUM			(el->list_gnum)
#define FUNC_S(x)			(el->pos_sou += (x))
#define FUNC_G(x)			(el->pos_gou -= (x))
#define FUNC_SO(x)			(el->pos_ou += (x))
#define FUNC_GO(x)			(el->pos_ou -= (x))
#define DEGREE_S(x,y,z)		(GetDegreeOfFreedomS(v,ban,diffS,diffG,addr,(x),(y),(z)))
#define DEGREE_G(x,y,z)		(GetDegreeOfFreedomG(v,ban,diffS,diffG,addr,(x),(y),(z)))
void Evaluate::GetValue1_dis2_ns( ELEMENTS* __restrict__ el, ShogiEval* __restrict__ pshogi ) const{
/************************************************
駒の価値以外による評価
差分評価しないもののみ
************************************************/
	el->pos_sou = 0;
	el->pos_gou = 0;
	el->pos_ou = 0;
#define DISABLE_FEATURE_2
#include "feature.h"
#undef DISABLE_FEATURE_2
}

void Evaluate::GetValueDiffSOU( ELEMENTS* __restrict__ el, ShogiEval* pshogi ){
/************************************************
先手玉の移動による評価値の差分
************************************************/
	el->pos_sou = 0;
	el->pos_ou = 0;
#define DISABLE_FEATURE_2
#define DISABLE_GOU
#include "feature.h"
#undef DISABLE_GOU
#undef DISABLE_FEATURE_2
}

void Evaluate::GetValueDiffGOU( ELEMENTS* __restrict__ el, ShogiEval* pshogi ){
/************************************************
先手玉の移動による評価値の差分
************************************************/
	el->pos_gou = 0;
	el->pos_ou = 0;
#define DISABLE_FEATURE_2
#define DISABLE_SOU
#include "feature.h"
#undef DISABLE_SOU
#undef DISABLE_FEATURE_2
}
#undef LIST_S
#undef LIST_G
#undef LIST_SNUM
#undef LIST_GNUM
#undef FUNC_S
#undef FUNC_G
#undef FUNC_SO
#undef FUNC_GO
#undef DEGREE_S
#undef DEGREE_G

template<KOMA type>
void Evaluate::GenerateListOU( ELEMENTS* el, ShogiEval* pshogi ){
/************************************************
玉の周囲の駒リスト生成
************************************************/
	// 局面
	const KOMA* __restrict__ ban = pshogi->GetBan();

	// 玉の位置
	int sou = pshogi->GetSOU();
	int gou = pshogi->GetGOU();

#define LIST_K(a,b)			\
if( type != GOU && ( ( koma = ban[sou+(b)] ) & SENGO ) ){ \
	el->list_s[el->list_snum++] = (a) * 18 + sente_num[koma]; \
} \
if( type != SOU && ( ( koma = ban[gou-(b)] ) & SENGO ) ){ \
	el->list_g[el->list_gnum++] = (a) * 18 + gote_num[koma]; \
}
	KOMA koma;
	if( type != GOU ){ el->list_snum = 0; }
	if( type != SOU ){ el->list_gnum = 0; }
	LIST_K( 0, -0x10-0x01 );LIST_K( 1, -0x10      );LIST_K( 2, -0x10+0x01 );
	LIST_K( 3,      -0x01 );                        LIST_K( 4,      +0x01 );
	LIST_K( 5, +0x10-0x01 );LIST_K( 6, +0x10      );LIST_K( 7, +0x10+0x01 );
#undef LIST_K
}
template void Evaluate::GenerateListOU<OU>( ELEMENTS* el, ShogiEval* pshogi );
template void Evaluate::GenerateListOU<SOU>( ELEMENTS* el, ShogiEval* pshogi );
template void Evaluate::GenerateListOU<GOU>( ELEMENTS* el, ShogiEval* pshogi );

template <int sign>
void Evaluate::GetValueDiff( ELEMENTS* __restrict__ el, ShogiEval* pshogi, int addr ){
/************************************************
駒の移動による評価値の差分
************************************************/
	// 局面
	const KOMA* __restrict__ ban = pshogi->GetBan();

	// bitboard
	const uint96* __restrict__ bb = pshogi->GetBB();
	uint96 bb_all = pshogi->GetBB_All();

	// 玉の位置
	int sou = pshogi->GetSOU();
	int gou = pshogi->GetGOU();

	KOMA koma = ban[addr];
	assert( koma != EMPTY );
	assert( koma != WALL );
	int a = _addr[addr];
	int sk = sente_num[koma];
	int gk = gote_num[koma];

	int sou0 = _addr[sou];
	int gou0 = 80 - _addr[gou];

	// 相対位置
	int diffS = _aa2num[sou0][a];
	int diffG = _aa2num[gou0][80-a];

	// 引き算するときはlistの更新はあと
	if( sign == -1 ){ goto lab2; }
lab1:
	// 玉の周囲の駒の場合
	{
		int dir;

		// 先手玉との距離が1の場合
		dir = addr - sou;
		if( _diff2dis[dir] == 1 ){
			int idr = _idr[dir];

			// 玉の周りの駒リスト更新
			int list_v = idr * 18 + sk;
			// 追加
			if( sign == 1 ){
				el->list_s[el->list_snum++] = list_v;
				assert( el->list_gnum <= 8 );
			}
			// 削除
			else{
				for( int i = 0 ; i < el->list_snum ; i++ ){
					if( el->list_s[i] == list_v ){
						el->list_s[i] = el->list_s[--(el->list_snum)];
						break;
					}
				}
			}

			// 先手玉-8近傍-後手玉
			int diff = _aa2num[sou0][80-gou0];
			el->pos_ou += sign * king8_piece[0][list_v][diff][18/*sente_num[SOU]*/];

			// 王-8近傍-その他
			uint96 bb_temp = bb_all & ~( bb[SOU] | bb[GOU] );
			for( int a2 = bb_temp.pickoutFirstBit() ; a2 != 0 ; a2 = bb_temp.pickoutFirstBit() ){
				int addr2 = _iaddr0[a2];
				a2--;
				if( a == a2 ){ continue; }
				el->pos_sou += sign * king8_piece[idr][sk][_aa2num[sou0][a2]][sente_num[ban[addr2]]];
			}
		}

		// 後手玉との距離が1の場合
		dir = -(addr - gou);
		if( _diff2dis[dir] == 1 ){
			int idr = _idr[dir];

			// 玉の周りの駒リスト更新
			int list_v = idr * 18 + gk;
			// 追加
			if( sign == 1 ){
				el->list_g[el->list_gnum++] = list_v;
				assert( el->list_gnum <= 8 );
			}
			// 削除
			else{
				for( int i = 0 ; i < el->list_gnum ; i++ ){
					if( el->list_g[i] == list_v ){
						el->list_g[i] = el->list_g[--(el->list_gnum)];
						break;
					}
				}
			}

			// 後手玉-8近傍-先手玉
			int diff = _aa2num[sou0][80-gou0];
			el->pos_ou -= sign * king8_piece[0][list_v][diff][18/*sente_num[SOU]*/];

			// 王-8近傍-その他
			uint96 bb_temp = bb_all & ~( bb[SOU] | bb[GOU] );
			for( int a2 = bb_temp.pickoutFirstBit() ; a2 != 0 ; a2 = bb_temp.pickoutFirstBit() ){
				int addr2 = _iaddr0[a2];
				a2--;
				if( a == a2 ){ continue; }
				el->pos_gou -= sign * king8_piece[idr][gk][_aa2num[gou0][80-a2]][gote_num[ban[addr2]]];
			}
		}
	}
	if( sign == -1 ){ return; }

lab2:
	// 王-他の駒
	el->pos_sou += sign * u_king_piece[sou0][   a][sk];
	el->pos_gou -= sign * u_king_piece[gou0][80-a][gk];

	{
		// 王-8近傍-その他
		int i;
		for( i = 0 ; i < el->list_snum ; i++ ){
			el->pos_sou += sign * king8_piece[0][el->list_s[i]][diffS][sk];
		}
		for( i = 0 ; i < el->list_gnum ; i++ ){
			el->pos_gou -= sign * king8_piece[0][el->list_g[i]][diffG][gk];
		}
	}

	{
		// 王-隣接2枚
		KOMA koma2;
		if( 0x07 & ( koma2 = ban[addr-0x00-0x01] ) ){
			el->pos_sou += sign * king_adjoin_r [diffS-1][sente_num[koma2]][sk];
			el->pos_gou -= sign * king_adjoin_r [diffG][gk][gote_num[koma2]];
		}
		if( 0x07 & ( koma2 = ban[addr-0x10+0x01] ) ){
			el->pos_sou += sign * king_adjoin_ld[diffS-16][sente_num[koma2]][sk];
			el->pos_gou -= sign * king_adjoin_ld[diffG][gk][gote_num[koma2]];
		}
		if( 0x07 & ( koma2 = ban[addr-0x10-0x00] ) ){
			el->pos_sou += sign * king_adjoin_d [diffS-17][sente_num[koma2]][sk];
			el->pos_gou -= sign * king_adjoin_d [diffG][gk][gote_num[koma2]];
		}
		if( 0x07 & ( koma2 = ban[addr-0x10-0x01] ) ){
			el->pos_sou += sign * king_adjoin_rd[diffS-18][sente_num[koma2]][sk];
			el->pos_gou -= sign * king_adjoin_rd[diffG][gk][gote_num[koma2]];
		}

		if( 0x07 & ( koma2 = ban[addr+0x00+0x01] ) ){
			el->pos_sou += sign * king_adjoin_r [diffS][sk][sente_num[koma2]];
			el->pos_gou -= sign * king_adjoin_r [diffG-1][gote_num[koma2]][gk];
		}
		if( 0x07 & ( koma2 = ban[addr+0x10-0x01] ) ){
			el->pos_sou += sign * king_adjoin_ld[diffS][sk][sente_num[koma2]];
			el->pos_gou -= sign * king_adjoin_ld[diffG-16][gote_num[koma2]][gk];
		}
		if( 0x07 & ( koma2 = ban[addr+0x10+0x00] ) ){
			el->pos_sou += sign * king_adjoin_d [diffS][sk][sente_num[koma2]];
			el->pos_gou -= sign * king_adjoin_d [diffG-17][gote_num[koma2]][gk];
		}
		if( 0x07 & ( koma2 = ban[addr+0x10+0x01] ) ){
			el->pos_sou += sign * king_adjoin_rd[diffS][sk][sente_num[koma2]];
			el->pos_gou -= sign * king_adjoin_rd[diffG-18][gote_num[koma2]][gk];
		}
	}
	if( sign == -1 ){ goto lab1; }
}
template void Evaluate::GetValueDiff<1>( ELEMENTS* __restrict__ el, ShogiEval* pshogi, int addr );
template void Evaluate::GetValueDiff<-1>( ELEMENTS* __restrict__ el, ShogiEval* pshogi, int addr );
