#!/bin/sh
echo 11.9.2001,2.2.2002 >load_save_time.txt
teapot -b load_save_time.tp<<EOF
goto &(0,0,1)
load-csv load_save_time.txt
from &(0,1,0)
to &(0,1,0)
save-csv load_save_result.txt
EOF
rm -f load_save_time.txt
cat load_save_result.txt
rm -f load_save_result.txt
