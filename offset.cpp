/* offset.cpp
 * 各パラメータのoffsetを管理
 */

#include <stddef.h>
#include <fstream>
#include "offset.h"

bool readEval( Evaluate* eval, const char* fname, vector<PARAM_OFFSET>& offset ){
	ifstream fin;

	fin.open( fname, ios::in | ios::binary );
	if( fin.fail() ){
		cerr << "Open Error!..[" << fname << ']' << '\n';
		return 0;
	}

	cerr << "Reading the data for evaluation..";
	cerr.flush();

	fin.read( (char*)eval->piece, sizeof(eval->piece) );

	for( unsigned i = 0 ; i < offset.size() - 1 ; i++ ){
		if( offset[i].flag ){
			fin.read( (char*)eval+offset[i].offset, offset[i+1].offset - offset[i].offset );

			if( fin.fail() )
			{
				cerr << "Fail!" << '\n';
				fin.close();
				return false;
			}
		}
		else{
			memset( (char*)eval+offset[i].offset, 0, offset[i+1].offset - offset[i].offset );
		}
	}

	fin.close();

	// パラメータ変更時の処理
	eval->Update();

	cerr << "OK!" << '\n';

	return true;
}

bool writeEval( Evaluate* eval, const char* fname, vector<PARAM_OFFSET>& offset ){
	ofstream fout;

	fout.open( fname, ios::out | ios::binary );
	if( fout.fail() ){
		cerr << "Open Error!..[" << fname << ']' << '\n';
		return 0;
	}

	cerr << "Writing the data for evaluation..";
	cerr.flush();

	fout.write( (char*)eval->piece, sizeof(eval->piece) );

	for( unsigned i = 0 ; i < offset.size() - 1 ; i++ ){
		if( offset[i].flag ){
			fout.write( (char*)eval+offset[i].offset, offset[i+1].offset - offset[i].offset );

			if( fout.fail() )
			{
				cerr << "Fail!" << '\n';
				fout.close();
				return false;
			}
		}
	}

	fout.close();

	cerr << "OK!" << '\n';

	return true;
}

void getEvalOffset( vector<PARAM_OFFSET>& offset ){
#define offset( p )			offset.push_back( PARAM_OFFSET( #p, offsetof(Evaluate,p) ) )
	offset( hand_fu );
	offset( hand_ky );
	offset( hand_ke );
	offset( hand_gi );
	offset( hand_ki );
	offset( hand_ka );
	offset( hand_hi );

	offset( king_hand_sfu );
	offset( king_hand_sky );
	offset( king_hand_ske );
	offset( king_hand_sgi );
	offset( king_hand_ski );
	offset( king_hand_ska );
	offset( king_hand_shi );

	offset( king_hand_gfu );
	offset( king_hand_gky );
	offset( king_hand_gke );
	offset( king_hand_ggi );
	offset( king_hand_gki );
	offset( king_hand_gka );
	offset( king_hand_ghi );

	offset( king_piece );
	offset( kingX_piece );
	offset( kingY_piece );

	offset( king_adjoin_r  );
	offset( king_adjoin_ld );
	offset( king_adjoin_d  );
	offset( king_adjoin_rd );

	offset( free_ky   );
	offset( free_ka_l );
	offset( free_ka_r );
	offset( free_ka_L );
	offset( free_ka_R );
	offset( free_hi_f );
	offset( free_hi_l );
	offset( free_hi_r );
	offset( free_hi_b );

	offset( free_ka_a );
	offset( free_hi_a );

	offset( effect_ky   );
	offset( effect_ka_l );
	offset( effect_ka_r );
	offset( effect_ka_L );
	offset( effect_ka_R );
	offset( effect_hi_f );
	offset( effect_hi_l );
	offset( effect_hi_r );
	offset( effect_hi_b );

#if 1
	offset( front2_sfu );
	offset( front2_gfu );
#endif

#if 1
	offset( fu_file );
	offset( sfu_file );
	offset( gfu_file );
#endif

	offset( king8_piece );

#if 1
	offset( king_check_fu );
#endif

	offset( ke );
	offset( kingS_ke_l );
	offset( kingS_ke_r );
	offset( kingG_ke_l );
	offset( kingG_ke_r );

	offset( king_effnum9 );
	offset( kingX_effnum9 );
	offset( kingY_effnum9 );

	offset( king_effwin );

	offset( king_addr );

	// ここまで
	offset( c_hand_fu );
}
