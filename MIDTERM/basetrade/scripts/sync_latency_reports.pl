#!/usr/bin/perl
use strict;
use warnings;

my $usage_ = "sync_latency_reports.pl DATE THIS_LOCATION OTHER_LOCATION_1 OTHER_LOCATION_2 ...";

if ($#ARGV < 2) {
    print $usage_."\n";
    exit (0);
}

my $yyyymmdd_ = `date +%Y%m%d`; chomp ($yyyymmdd_);
if ($ARGV[0] ne "TODAY") {
    $yyyymmdd_ = $ARGV[0]; chomp ($yyyymmdd_);
}

my $this_location_ = $ARGV[1]; chomp ($this_location_);

my @other_location_list_ = ();
for (my $arg_ = 2; $arg_ <= $#ARGV; $arg_++) {
    my $t_other_location_ = $ARGV[$arg_]; chomp ($t_other_location_);

    push (@other_location_list_, $t_other_location_);
}

my $yyyy_ = substr ($yyyymmdd_, 0, 4);
my $mm_ = substr ($yyyymmdd_, 4, 2);
my $dd_ = substr ($yyyymmdd_, 6, 2);

# Kill any running senders first followed by the receivers.
`pkill -9 sender`;
sleep (5);
`pkill -9 receiver_day`;

my $running_sender_receiver_ = `pgrep sender`.`pgrep receiver_day`;

print "pgrep sender/receiver output : ".$running_sender_receiver_."\n";

my $remote_archive_location_ = "/NAS1/data/LatencyReports/".$this_location_."/".$yyyy_."/".$mm_."/".$dd_;
my $local_archive_location_ = "/home/sghosh/logs/latencylogs";

for (my $location_ = 0; $location_ <= $#other_location_list_; $location_++) {
    my $t_local_file_location_ = $local_archive_location_."/".$other_location_list_[$location_];
    my $t_remote_file_location_ = $remote_archive_location_;

    my $mkdir_cmd_ = `ssh sghosh\@10.1.3.11 'mkdir -p $t_remote_file_location_'`;
    my $rsync_cmd_ = `rsync -avz --quiet $t_local_file_location_ sghosh\@10.1.3.11:$t_remote_file_location_`;

    print "mkdir output : ".$mkdir_cmd_."\n";
    print "rsync output : ".$rsync_cmd_."\n";
}
