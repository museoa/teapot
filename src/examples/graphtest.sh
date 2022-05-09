#!/bin/sh

( echo This is the same data, once as a pie graph and once as a box graph.
echo .PS
./graph -w 2 <<eof
"a"	1	1
"b"	2.5
"c"	4
eof
./graph -n -b -w 2 <<eof
"a"	1
"b"	2.5
"c"	4
eof
echo with .w at last [].e
echo .PE ) | groff -Tps -p -mm >graphtest.ps
