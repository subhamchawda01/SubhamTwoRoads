#!/bin/bash

echo "Getting Link... "
notebook_link=`ssh dvcinfra@10.23.5.66 ssh dvctrader@44.202.186.243 "grep localhost /home/dvctrader/Jupyter_NoteBooks/jupyter_notebook_daemon_logs.txt | grep -v NotebookApp" | awk '{print $1}'`
PORT=`echo $notebook_link | awk -F'/' '{print $3}' | awk -F: '{print $2}'`
echo -e "PORT: $PORT\nRunning ssh"

ssh_proc_id=`ps aux| grep "ssh -f -X -L" | grep "localhost:$PORT" | grep -v grep | awk '{print $2}' | tr '\n' ' '`
echo "ssh id: $ssh_proc_id"

if [ -z "$ssh_proc_id" ];
then
  echo "NULL DONE";
else
  echo "NOT NULL : Kill all localhost ssh";
  kill -9 $ssh_proc_id
fi

#exit

#notebook_link=`ssh dvctrader@44.202.186.243 "grep localhost /tmp/logj | grep -v NotebookApp" | awk '{print $1}'`
#PORT=`echo $notebook_link | awk -F'/' '{print $3}' | awk -F: '{print $2}'`

if [ -z "$notebook_link" ];
then
  echo "jupyter log file is empty. Check log file on worker: /home/dvctrader/Jupyter_NoteBooks/jupyter_notebook_daemon_logs.txt"
else
#  ssh  -X -L 8080:localhost:8080 dvcinfra@10.23.5.66 ssh -X -L 8080:localhost:8080 dvctrader@44.202.186.243


  ssh -f -X -L ${PORT}:localhost:${PORT} -N dvcinfra@10.23.5.66

  if [ $# -eq 1 ];
  then
    echo "FILE $1"
    file_=$1
    link_2="http://localhost:${PORT}/notebooks/${file_}.ipynb"
  #  firefox  -new-tab http://localhost:${PORT}/notebooks/${file_}.ipynb #test.ipynb
    echo "LINK_1: $notebook_link"
    echo "LINK_2: $link_2"
    
  else 
    echo "LINK: $notebook_link"
  fi

fi
