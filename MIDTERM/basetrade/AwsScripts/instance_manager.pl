#!/usr/bin/perl

# this scripts aim to automatically start and stop instances
# We devide all worker instances into 2 categories
#   i) temporary (This is described by the presence of its ip in a text file)
#   ii) permanent
# Depending on size of job_queue.txt and all_instances.txt we do
# if #job_queue.txt > 0 and #free_cores = 0, we create more cores by one of the following:
#    if a permanent instance is stopped, we start it
#    if all permanent instances are running, we launch a new instance subject to a MAX_ALLOWED contraint
# if #job_queue.txt = 0 and #free_cores >0 , we terminate all free temporary instances and stop all free permanent instances

use strict;
use warnings;

my $work_dir="/mnt/sdf/JOBS/";
my $SCRIPTS_HOME="/home/dvctrader/controller_scripts/";
my $instance_lock="/home/dvctrader/locks/instance_update.lock";

my $locked = 1;

my $MAX_RUNNING=1;

sub acquire_instance_lock{
    $locked = `mkdir $instance_lock 2>/dev/null ; echo \$?`;
    $locked = int ( $locked );
    return ;
#DO not sleep and retry
    if ($locked != 0) {
        sleep 10;
        $locked = `mkdir $instance_lock 2>/dev/null ; echo \$?`;
    }
    $locked = int ( $locked );
    if ($locked != 0) {
        sleep 10;
        $locked = `mkdir $instance_lock 2>/dev/null ; echo \$?`;
    }
    $locked = int ( $locked );
    if ($locked != 0) {
        sleep 10;
        $locked = `mkdir $instance_lock 2>/dev/null ; echo \$?`;
    }
    $locked = int ( $locked );
    #tried 4 times
}

sub free_instance_lock{
    if ( $locked == 0 ){
        `rmdir $instance_lock `; 
    }
}

sub free_computing_resource {
  my @running_resources=`grep running $work_dir"all_instances.txt"  | grep -v "10.0.0." | awk '{print \$1,\$4}'`;
  for my $instance(@running_resources) {
    (my$id, my$ip) = split /\s+/,  $instance ;
    my $cmd = "ls ".$work_dir."job_desc/COMMAND_".$ip."* 2>/dev/null | wc -l ";
    my $num_running=`$cmd`;
    $cmd = "ls ".$work_dir."job_desc/DONE_".$ip."* 2>/dev/null | wc -l ";
    my $num_done=`$cmd`;
    chomp ( $num_running );
    chomp ( $num_done );
    if ( $num_running <=  $num_done ) {
        print "stopping instance,ip $id, $ip\n";
      #  `$SCRIPTS_HOME/instance_init.sh  $id STOP `;
    }
  }
}

sub add_computing_resource {
  my $num_reqd = $_[0];
  $num_reqd = 1; # add no more than one instance at a time. 
  #adding many at a time may be risky if there is a bug in the code. we could launch many useless instances
  # also, adding one instance will ensure that in the next chance, a task is assigned to some core rather than all tasks waitin
  # for a huge number of machines to be launches simultaneously

  my $all_instance_count = `grep -v "10.0.0" $work_dir"all_instances.txt" | wc -l  | awk '{print \$1}'`;
    chomp ( $all_instance_count );
    if ( $all_instance_count >= $MAX_RUNNING ) {
      print "already running max number of instances($all_instance_count >= $MAX_RUNNING)\n";
      return;
    }

  my @stopped_resources=`grep stop $work_dir"all_instances.txt" | awk '{print \$1,\$4}'`;
  if ( $num_reqd > 0 && $#stopped_resources >= 0 ) {
    foreach my$instance(@stopped_resources){
      (my$id, my$ip) = split /\s+/,  $instance ;
      
      print "$SCRIPTS_HOME/instance_init.sh  $id START \n";
      `$SCRIPTS_HOME/instance_init.sh  $id START `;
      my$num_cores=`ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $ip nproc`;
      $num_cores=int ($num_cores);
      $num_reqd -= $num_cores;
      return; #unconditionally return
    }
  }
  if ( $num_reqd > 0 ) {
    #launch a new one
    my $all_instance_count = `grep -v "10.0.0" $work_dir"all_instances.txt" | wc -l  | awk '{print \$1}'`;
    chomp ( $all_instance_count );
    if ( $all_instance_count >= $MAX_RUNNING ) {
      print "already running max number of instances($all_instance_count >= $MAX_RUNNING)\n";
      return;
    }
    #for (my$i=0;$i<$MAX_RUNNING ; $i=$i+1)  #at most one in one loop
    {
      print "launching a new instance...\n";
      my $ip=`$SCRIPTS_HOME/instance_launch.sh | grep "ping successful" | awk '{print \$1}' `;
      chomp ( $ip ) ;

      my$num_cores=`ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $ip nproc`;
      $num_cores=int ($num_cores);
      $num_reqd -= $num_cores;
      return; #unconditionally return
    }
    return ;
  }
}

sub assign_tasks{
   my $this_job = $_[0];
   my $this_ip = ` head -n1 /mnt/sdf/JOBS/free_instances.txt `;
   ` sed -i ' 1d ' /mnt/sdf/JOBS/free_instances.txt `;
   chomp($this_ip);
   chomp($this_job);
   my $cmd="";
   if ( $this_job =~ /^tag:/ ) {
      my ($first, $rest)= split /\s+/ , $this_job, 2 ;
      $first=~s/tag://;
      $cmd="perl /home/dvctrader/controller_scripts/run_cmd.pl $this_ip -t $first $rest ";
   }
   else {
      $cmd="perl /home/dvctrader/controller_scripts/run_cmd.pl $this_ip $this_job ";
   }
   #print "running: $cmd\n";
   `$cmd `;
}

sub find_next_task{
  
  my %hash = ();

  my @tag_config=`cat /mnt/sdf/JOBS/q.cfg | grep -v "^#"`;
  foreach my$i(0..$#tag_config){
    chomp ( $tag_config[$i] ) ;
 
    my @words = split /\s+/, $tag_config[$i];
    my $num_running = ` ls /mnt/sdf/JOBS/job_desc/RUNNING*-$words[0] 2>/dev/null| wc -l `; 
    my $num_done = ` ls /mnt/sdf/JOBS/job_desc/DONE*-$words[0] 2>/dev/null| wc -l `; 
    chomp ( $num_running) ; 
    chomp ( $num_done ) ;
    if ( ( $num_running-$num_done ) <= $words[1] ) {
      $hash{$tag_config[$i]} = $words[2] + ($num_running - $num_done)/$words[1];
    }
  }
  
  foreach my$i( sort { $hash{$a} <=> $hash{$b} } keys %hash ) {
    my @words = split /\s+/, $i;
    my $jobq_file = "/mnt/sdf/JOBS/queues/$words[0]";
    my $jobq_file_backup = $jobq_file."bk";
    if ( -s $jobq_file ) {
#to check if the task is already running
      my $job_line = `head -n1 $jobq_file`;
      my $job_last_run=((` cat /mnt/sdf/JOBS/running_q.txt | grep "$job_line" | wc -l`)>0)?` cat /mnt/sdf/JOBS/running_q.txt | grep "$job_line" | tail -n1 | awk '{print \$2}' ` : "0";
      my $job_last_done=((` cat /mnt/sdf/JOBS/done_q.txt | grep "$job_line" | wc -l`)>0)?` cat /mnt/sdf/JOBS/done_q.txt | grep "$job_line" | tail -n1 | awk '{print \$3}' ` : "0";
      my $job_to_run = (($job_last_run-$job_last_done)>0)?"false" : "true";
      #true if task is not running already
      if($job_to_run eq "true"){
        my $task_line=`head -n1 $jobq_file | sed 's/^/tag:$words[0] /'`;
        `awk 'NR==1{store=\$0;next}1;END{print store}' $jobq_file > $jobq_file_backup ; mv $jobq_file_backup $jobq_file`;
        return $task_line;
      }
    }
  }
  return "";
}


#main task
sub main {
    acquire_instance_lock();
    ` $SCRIPTS_HOME/update_free_cores.sh `;

    if ( $locked != 0 ) {
#        print "lock was not aquired, exiting..\n";
        return 0; #lock was not acquired
    }
    my $free_core_count = ` grep -vc "10.0.0." $work_dir"free_instances.txt" | awk '{print \$1}' `;
    chomp ( $free_core_count );
    my $job_assigned=0;
    foreach my$i(1..$free_core_count){
      my $task_line = find_next_task();
      if ( $task_line ne "" && $free_core_count > 0 ) {
        print "assign_tasks called. freecores:$free_core_count task:$task_line\n";
        assign_tasks($task_line);
        $job_assigned ++ ;
      }
    } 

    if ( 0 == $free_core_count ){
        print "add_computing_resource called. freecores:$free_core_count \n";
        add_computing_resource ( 1 );
        ` $SCRIPTS_HOME/update_instances.sh `;
        ` $SCRIPTS_HOME/update_free_cores.sh `;
    }
    elsif ( $job_assigned < $free_core_count ){
#        print "free_computing_resource called. $job_assigned jobs for $free_core_count cores. Must free some resources\n";
        free_computing_resource();
        ` $SCRIPTS_HOME/update_instances.sh `;
        ` $SCRIPTS_HOME/update_free_cores.sh `;
    }

    if ( $locked == 0 ) {
        free_instance_lock();
    }
}

main(); #called main
