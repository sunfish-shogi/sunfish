/* wconf.cpp
 * R.Kubo 2011-2012
 * wconfê›íËÉtÉ@ÉCÉãÇÃä«óù
 */

#include "wconf.h"

bool WConf::Read(){
	ifstream fin( FNAME_WCONF );
	char str[1024];
	if( !fin.is_open() ){ return false; }
	while( 1 ){
		fin.getline( str, sizeof(str) );
		if( fin.eof() || fin.fail() ){ break; }
		char* p = strchr( str, '=' );
		if( p != NULL ){
			*p = '\0';
			conf.push_back( Data( str, p+1 ) );
		}
	}
	fin.close();
	return true;
}

bool WConf::Write(){
	ofstream fout( FNAME_WCONF );
	if( !fout.is_open() ){ return false; }
	for( list<Data>::iterator i = conf.begin() ; i != conf.end() ; i++ ){
		fout << (*i).getKey() << '=' << (*i).getValue() << '\n';
	}
	fout.close();
	return true;
}

bool WConf::getValue( const char* key, char* value ){
	for( list<Data>::iterator i = conf.begin() ; i != conf.end() ; i++ ){
		if( strcmp( (*i).getKey(), key ) == 0 ){
			strcpy( value, (*i).getValue() );
			return true;
		}
	}
	return false;
}

bool WConf::getValueInt( const char* key, int* value ){
	for( list<Data>::iterator i = conf.begin() ; i != conf.end() ; i++ ){
		if( strcmp( (*i).getKey(), key ) == 0 ){
			*value = strtol( (*i).getValue(), NULL, 10 );
			return true;
		}
	}
	return false;
}

void WConf::setValue( const char* key, const char* value ){
	for( list<Data>::iterator i = conf.begin() ; i != conf.end() ; i++ ){
		if( strcmp( (*i).getKey(), key ) == 0 ){
			(*i).setValue( value );
			return;
		}
	}
	conf.push_back( Data( key, value ) );
}
