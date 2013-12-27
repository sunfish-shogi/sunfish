#!/usr/bin/perl -w
#cluster版のevinfoからPV更新時だけを取り出す。

$cnt = 0;
$block = 19;
$span = 50;

while( <> ){
	$line = $_;

	if( ( $cnt / $block ) % $span == 0 ){
		$line =~ s/0 - ([.0-9]*)/$1 - /;
		print $line;
	}

	$cnt++;
}
