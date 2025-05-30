#!/bin/bash
this_machine=$1; runuser -l dvcinfra -c "ssh 10.23.74.51 'scp -p 10.23.82.55:/spare/local/files/CMEMDP/* $this_machine:/spare/local/files/CMEMDP/'" >/dev/null 2>/dev/null &
