#!/usr/bin/perl
use strict;
use warnings;
use Fcntl qw(:flock SEEK_END);

#lock failed return 1 #TODO ADD ERROR HANDLING
sub lock {
    my ($fh) = @_;
    flock($fh, LOCK_EX) or return 1 ;
    # and, in case someone appended while we were waiting...
    seek($fh, 0, SEEK_END) or return 1 ;
    return 0 ;
}

sub unlock {
    my ($fh) = @_;
    flock($fh, LOCK_UN) or die "Cannot unlock mailbox - $!\n";
}

my $lock_file="/mnt/sdf/JOBS/job_q.lock";
my @ip = ();
my @job = ();

my $num_free_cores=`grep -c ^ /mnt/sdf/JOBS/free_instances.txt`;
my $num_jobs=`grep -c ^ /mnt/sdf/JOBS/job_q.txt`;
chomp($num_free_cores);
chomp($num_jobs);
my $lines_to_read=$num_free_cores;
if ( $num_free_cores > $num_jobs )
{
    $lines_to_read = $num_jobs;
} 
if ( $lines_to_read > 0 )  {
print "num jobs in this batch ".$lines_to_read."\n";

my $cmd = "head -n $lines_to_read /mnt/sdf/JOBS/free_instances.txt";
print "Executing: $cmd";
@ip=`$cmd`;
$cmd="head -n $lines_to_read /mnt/sdf/JOBS/job_q.txt";
print "Executing: $cmd";
@job=`$cmd`;

my $delete_top_lines_cmd="sed -i '1,".$lines_to_read."d' /mnt/sdf/JOBS/job_q.txt /mnt/sdf/JOBS/free_instances.txt";
print "deleting top $lines_to_read lines \n Exec: $delete_top_lines_cmd \n";
`$delete_top_lines_cmd`;

foreach my $i ( 0..$#ip) {
    my $this_ip=$ip[$i];
    chomp($this_ip);
    my $this_job=$job[$i];
    chomp($this_job);
    my $cmd="perl /home/dvctrader/controller_scripts/run_cmd.pl $this_ip $this_job ";
    #print "running: $cmd\n";
    `$cmd `;
}
}
