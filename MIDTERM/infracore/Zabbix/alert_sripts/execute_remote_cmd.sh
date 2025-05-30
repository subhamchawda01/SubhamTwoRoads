#!/bin/bash

current_host=$2 ; 
cmd_with_args=$3 ; 

echo "Executing Remote Cmd : ssh -n -f $2 '$cmd_with_args' &" >> /root/remote_exec.log
ssh -n -f $2 "$cmd_with_args" &
