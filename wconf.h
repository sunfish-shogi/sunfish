/* wconf.h
 * R.Kubo 2011-2012
 * wconf設定ファイルの管理
 */

#ifndef __WCONF__
#define __WCONF__

#include <fstream>
#include "shogi.h"

class Data{
	string key;
	string value;
public:
	Data( const char* c_key, const char* c_value ) : key(c_key), value(c_value){
	}
	virtual ~Data(){
	}
	void setKey( const char* c_key ){
		key = c_key;
	}
	void setValue( const char* c_value ){
		value = c_value;
	}
	const char* getKey(){
		return key.c_str();
	}
	const char* getValue(){
		return value.c_str();
	}
};

#define FNAME_WCONF			"wconf"

class WConf{
private:
	list<Data> conf;

public:
	WConf(){
	}
	virtual ~WConf(){
	}
	bool Read();
	bool Write();
	bool getValue( const char* key, char* value );
	bool getValueInt( const char* key, int* value );
	void setValue( const char* key, const char* value );
	void setValueInt( const char* key, int value ){
		char str[16];
		sprintf( str, "%d", value );
		setValue( key, str );
	}
};

#endif
