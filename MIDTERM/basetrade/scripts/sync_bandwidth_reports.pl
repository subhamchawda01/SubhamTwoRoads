#!/usr/bin/perl
use strict;
use warnings;

my $usage_ = "sync_bandwidth_reports.pl DATE THIS_LOCATION";

if ($#ARGV < 1) {
    print $usage_."\n";
    exit (0);
}

my $yyyymmdd_ = `date +%Y%m%d`; chomp ($yyyymmdd_);
if ($ARGV[0] ne "TODAY") {
    $yyyymmdd_ = $ARGV[0]; chomp ($yyyymmdd_);
}

my $this_location_ = $ARGV[1];

my $server_no_ = `hostname`; chomp ($server_no_);
if (index ($server_no_, ".") > 0) { # sdv-fr2-srv14.dvcap.local -> sdv-fr2-srv14
    my @location_words_ = split ('\.', $server_no_);

    $server_no_ = $location_words_[0]; chomp ($server_no_);
}

{ # sdv-fr2-srv14 -> srv14
    my @location_words_ = split ('-', $server_no_);

    $server_no_ = $location_words_[2]; chomp ($server_no_);
}

my $yyyy_ = substr ($yyyymmdd_, 0, 4);
my $mm_ = substr ($yyyymmdd_, 4, 2);
my $dd_ = substr ($yyyymmdd_, 6, 2);

# Kill any running monitors.
`pkill -9 bandwidth_mon`;

my $running_monitor_ = `pgrep bandwidth_mon`;

print "pgrep bandwidth_mon output : ".$running_monitor_."\n";

my $remote_archive_location_ = "/NAS1/data/LatencyReports/".$this_location_."/".$yyyy_."/".$mm_."/".$dd_;
my $local_archive_location_ = "/home/sghosh/logs/latencylogs";

my $t_local_file_location_ = $local_archive_location_."/".$server_no_;
my $t_remote_file_location_ = $remote_archive_location_;

my $mkdir_cmd_ = `ssh sghosh\@10.1.3.11 'mkdir -p $t_remote_file_location_'`;
my $rsync_cmd_ = `rsync -avz --quiet $t_local_file_location_ sghosh\@10.1.3.11:$t_remote_file_location_`;

print "mkdir output : ".$mkdir_cmd_."\n";
print "rsync output : ".$rsync_cmd_."\n";
