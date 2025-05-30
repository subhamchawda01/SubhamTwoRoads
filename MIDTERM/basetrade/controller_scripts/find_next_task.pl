#!/usr/bin/perl

my $global_jobq_file="/mnt/sdf/JOBS/job_q.txt";

sub top{
  
  my %hash = ();

  @tag_config=`cat /mnt/sdf/JOBS/q.cfg | grep -v "^#"`;
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
    my $jobq_file_backup = $jobq_file."bk";
    if ( -s $jobq_file ) {
      `head -n1 $jobq_file | sed 's/^/tag:$words[0] /' >> $global_jobq_file `;
      `awk 'NR==1{store=\$0;next}1;END{print store}' $jobq_file > $jobq_file_backup ; mv $jobq_file_backup $jobq_file`;
      return;
    }
  }
}

top (  );
