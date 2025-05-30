#!/bin/bash

#
worker_id=`ssh dvctrader@44.202.186.243 "ps aux | grep jupyter | grep -v grep" | awk '{print $2}' | tr '\n' ' '`
echo "Worker proc_id: $worker_id"

if [ -z "$worker_id" ]; 
then 
  echo "NULL DONE"; 
else 
  echo "NOT NULL : Kill all jupyter notebooks"; 
  ssh dvctrader@44.202.186.243 "kill -9 $worker_id" 
fi

ssh_proc_id=`ps aux| egrep "ssh -f -X|ssh -X" | grep -v grep | awk '{print $2}' | tr '\n' ' '`
echo "ssh id: $ssh_proc_id"

if [ -z "$ssh_proc_id" ]; 
then 
  echo "NULL DONE"; 
else 
  echo "NOT NULL : Kill all worker ssh"; 
  kill -9 $ssh_proc_id
fi
#
#exit

log_file="/home/dvctrader/Jupyter_NoteBooks/jupyter_notebook_daemon_logs.txt"
#ssh dvctrader@44.202.186.243 "ulimit -m 1; echo {1..1000000000}"
#exit
ssh dvctrader@44.202.186.243 "ulimit -m 16777216; jupyter notebook --notebook-dir=/home/dvctrader/Jupyter_NoteBooks/ --no-browser --port=8081 >${log_file} 2>&1 " &
ssh_proc_id=$!
echo "ssh proc_ID: $ssh_proc_id"

sleep 3
echo "WC:"
ssh dvctrader@44.202.186.243 "wc ${log_file}"

echo "DATA: "
notebook_link=`ssh dvctrader@44.202.186.243 "grep localhost ${log_file} | grep -v NotebookApp" | awk '{print $1}'`

scp dvctrader@44.202.186.243:${log_file} /home/dvcinfra/
scp dvctrader@44.202.186.243:${log_file} /home/dvcinfra/
#
cat /home/dvcinfra/jupyter_notebook_daemon_logs.txt
#notebook_link=`grep localhost /tmp/logj | grep -v NotebookApp | awk '{print $1}'`

echo "Link: $notebook_link"

PORT=`echo $notebook_link | awk -F'/' '{print $3}' | awk -F: '{print $2}'`

echo "PORT: $PORT"
#ssh -L 8081:localhost:8081 dvctrader@44.202.186.243 #&
ssh -f -X -L ${PORT}:localhost:${PORT} -N dvctrader@44.202.186.243 #"sleep 3000"

##worker_pid=$!
#echo "SSH DONE $worker_pid"
sleep 15
#echo "RUNNING FIREFOX"
#firefox  -new-tab $notebook_link & #http://localhost:8081/?token=a4b213849ca6c088445ea793ec73fcad4a1d6d8d0bc415aa
##sleep 3
##
##if [ $# -eq 1 ];
##then
##  echo "FILE $1"
##  file_=$1
##  firefox  -new-tab http://localhost:${PORT}/notebooks/Jupyter_NoteBooks/${file_} #test.ipynb
##fi
###wait
#echo "FIREFOX RUNNING"

