#!/usr/bin/perl
use strict;
use warnings;

my $usage_ = "run_latency_tests.pl TIMEOUT_SEC BANDWIDTH_LIMIT";

if ($#ARGV < 1) {
    print $usage_."\n";
    exit (0);
}

my $time_sec_ = $ARGV[0];
my $bandwidth_limit_ = $ARGV[1];

my $server_no_ = `hostname`; chomp ($server_no_);
if (index ($server_no_, ".") > 0) { # sdv-fr2-srv14.dvcap.local -> sdv-fr2-srv14
    my @location_words_ = split ('\.', $server_no_);

    $server_no_ = $location_words_[0]; chomp ($server_no_);
}

{ # sdv-fr2-srv14 -> srv14
    my @location_words_ = split ('-', $server_no_);

    $server_no_ = $location_words_[2]; chomp ($server_no_);
}

my $bandwidth_exec_name_ = "/home/sghosh/LiveExec/LatencyExec/bandwidth_monitor";
my $local_archive_location_ = "/home/sghosh/logs/latencylogs";
my $local_file_location_ = $local_archive_location_."/".$server_no_;

my $mkdir_output_ = `mkdir -p $local_archive_location_`;
print "mkdir output : ".$mkdir_output_."\n";

my $exec_cmd_ = "taskset -c 20,21,22,23 $bandwidth_exec_name_ $time_sec_ $bandwidth_limit_ SYSTEM";

$exec_cmd_ = $exec_cmd_." > $local_file_location_ &";

system ($exec_cmd_);
