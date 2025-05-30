#!/bin/bash

TEMP_REPO_FILE=$HOME/.repo
TEMP_AWS_FILE=$HOME/.aws
TEMP_DIFF_FILE=$HOME/.diff 
TEMP_ADDITION_FILE=$HOME/.additions
TEMP_DELETION_FILE=$HOME/.deletions 

AWS_QUEUE_DIR=/mnt/sdf/JOBS/queues 

for queue in `ls $HOME/modelling/aws_gsq_store/*` 
do 

   queuename=`echo $queue | awk -F"/" '{print $NF}'`
   
   if [ `awk -vqn=$queuename '{if($1==qn){print 1;}}' /mnt/sdf/JOBS/q.cfg | wc -l` -le 0 ] ; then continue; fi ;  # ignoring queues not in q.cfg or commented in q.cfg
   if [ `awk -vqn=$queuename '{if(($1==qn) && ($4=="L")){print 1;}}' /mnt/sdf/JOBS/q.cfg | head -n1` ] ; then continue; fi ;  # ignoring linear queues

   if [ -f $AWS_QUEUE_DIR/$queuename ] && [ -s $AWS_QUEUE_DIR/$queuename ] 
   then 

     sort -u $queue > $TEMP_REPO_FILE ; 
     sort -u $AWS_QUEUE_DIR/$queuename > $TEMP_AWS_FILE ;
     diff -u $TEMP_REPO_FILE $TEMP_AWS_FILE > $TEMP_DIFF_FILE ;

     grep -e "+" $TEMP_DIFF_FILE | grep -v "@" | grep -v "repo" | grep -v "aws" > $TEMP_DELETION_FILE ;
     grep -e "-" $TEMP_DIFF_FILE | grep -v "@" | grep -v "repo" | grep -v "aws" > $TEMP_ADDITION_FILE ;

     if [ -s $TEMP_ADDITION_FILE ] 
     then

       cat $TEMP_ADDITION_FILE | grep -e "-/" | awk -F"-/" '{print "/"$2}' >> $AWS_QUEUE_DIR/$queuename ;
       echo "ADDING ENTRIES" ; echo ; cat $TEMP_ADDITION_FILE ; 

     fi 

     if [ -s $TEMP_DELETION_FILE ] 
     then

       echo "FILE LOCALLY EDITED, UPDATING" ; echo ; cat $TEMP_DELETION_FILE 

       /bin/mail -s "QueueCleanUpAlert - Local Edit Detected For Queue : $AWS_QUEUE_DIR/$queuename" -r "nseall@tworoads.co.in" "nseall@tworoads.co.in" < $TEMP_DELETION_FILE ;
       cp -f $queue $AWS_QUEUE_DIR/   #just overwrite, don't care about ordering/scheduling 

     fi 

   else 

     cp -f $queue $AWS_QUEUE_DIR/   #File Doesn't exist, create it forcefully 

   fi 

done 

rm -rf $TEMP_REPO_FILE $TEMP_AWS_FILE $TEMP_DIFF_FILE $TEMP_ADDITION_FILE $TEMP_DELETION_FILE ; 
