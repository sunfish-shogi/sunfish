#!/usr/bin/perl -w

$temp = "dctemp";

while( <*.cpp *.c *.h *.pl *.txt *.csa *.cs *.rc> ){
    system "cp $_ $temp";
    system "tr -d '\r' < $temp > $_";
}
system "rm $temp"
