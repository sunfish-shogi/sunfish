/* consultation.cpp
 * 合議アルゴリズム
 */

#include <sstream>
#include "match.h"

#if 0
/************************************************
コンストラクタ
************************************************/
ConsultationParallel::ConsultationParallel( int _cons_num, int thread_num ){
	int i;

	cons_num = _cons_num;
	thinks = new Think[cons_num];
	for( i = 0 ; i < cons_num ; i++ ){
		thinks[i].SetNumberOfThreads( thread_num );
	}
	delegate = 0;
}

/************************************************
デストラクタ
************************************************/
ConsultationParallel::~ConsultationParallel(){
	delete [] thinks;
}
#endif

/************************************************
コンストラクタ
************************************************/
ConsultationSerial::ConsultationSerial( int _cons_num, int _thread_num ) : think( _thread_num ){
	hash_table_a = new HashTable   [_cons_num];
	eval_a       = new EvaluateNull[_cons_num];
	eval_loaded  = false;
	cons_num     = _cons_num;
	deviation    = 0.0;
}

/************************************************
デストラクタ
************************************************/
ConsultationSerial::~ConsultationSerial(){
	delete [] hash_table_a;
	delete [] eval_a;
}

/************************************************
時間制限付き探索
************************************************/
int ConsultationSerial::SearchLimit( ShogiEval* pshogi, Book* pbook,
	MOVE* pmove, int mdepth, THINK_INFO* _info, unsigned opt )
{
	MOVE mtemp;
	THINK_INFO itemp;
	int ret;
	int delegate = -1;
	int value = VMIN-1;

	if( !eval_loaded ){
		for( int i = 0 ; i < cons_num ; i++ ){
			ostringstream oss;
			oss << FNAME_EVDATA << i;
			if( !eval_a[i].ReadData( oss.str().c_str() ) ){
				eval_a[i].ReadData( FNAME_EVDATA );
			}
		}
		eval_loaded = true;
	}

	best_pv.Init();

	for( int i = 0 ; i < cons_num ; i++ ){
		HashTable* hash_table_org = think.ReplaceHashTable( &hash_table_a[i] );
		ShogiEval shogi( *pshogi, &eval_a[i] );
		shogi.SetDeviation( deviation );

		cout << "Consultation(Serial) " << i << '\n';

		ret = think.SearchLimit( &shogi, pbook, &mtemp, mdepth, &itemp, opt );

		// 楽観的合議
		if( ret != 0 ){
			if( itemp.value > value ){
				delegate = i;
				(*pmove) = mtemp;
				(*_info) = itemp;
				think.GetPVMove( best_pv );
				value = itemp.value;
			}

			Shogi::PrintMove( mtemp );
			cout << ' ' << (unsigned)itemp.value << '\n';
			cout << "Delegate : " << delegate << '\n';
		}

		think.ReplaceHashTable( hash_table_org );
	}

	if( delegate == -1 ){ return 0; }
	else                { return 1; }
}
