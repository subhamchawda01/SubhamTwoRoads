#!/usr/bin/perl
use strict;
use warnings;

my $usage_ = "run_latency_tests.pl THIS_LOCATION OTHER_LOCATION_1 OTHER_LOCATION_2 ...";

if ($#ARGV < 2) {
    print $usage_."\n";
    exit (0);
}

my $this_location_ = $ARGV[0]; chomp ($this_location_);

my @other_location_list_ = ();
for (my $arg_ = 1; $arg_ <= $#ARGV; $arg_++) {
    my $t_other_location_ = $ARGV[$arg_]; chomp ($t_other_location_);

    push (@other_location_list_, $t_other_location_);
}

# Ports are assigned as follows:
# Prefix :        999
# Receiver code : 1/2/3/4/5
# Sender code :   1/2/3/4/5
# Postfix :       1
my $port_prefix_ = "999";
my %location_to_port_ = ("NY4"=>"1", "CHI"=>"2", "TOR"=>"3", "FR2"=>"4", "BRZ"=>"5");
my %location_to_interface_ = ("NY4"=>"bond0", "CHI"=>"eth5", "TOR"=>"eth4", "FR2"=>"eth5", "BRZ"=>"eth5");

my %location_to_exec_prefix_ = ("NY4"=>"", "CHI"=>"", "TOR"=>"onload", "FR2"=>"", "BRZ"=>"");

my $ip_ = "225.2.9.9";

# ==========================================================================
# Start all receivers first.
# ==========================================================================
my $receiver_exec_name_ = "/home/sghosh/LiveExec/LatencyExec/receiver_day";
my $local_archive_location_ = "/home/sghosh/logs/latencylogs";

for (my $location_ = 0; $location_ <= $#other_location_list_; $location_++) {
    if ($this_location_ eq $other_location_list_[$location_]) {
	print "Sender and receiver both at ".$this_location_." ?\n";
	exit (0);
    }

    my $interface_ = $location_to_interface_{$this_location_};

    my $port_ = $port_prefix_.$location_to_port_{$this_location_}.$location_to_port_{$other_location_list_[$location_]}."1";

    my $local_file_location_ = $local_archive_location_."/".$other_location_list_[$location_];

    my $mkdir_output_ = `mkdir -p $local_archive_location_`;
    print "mkdir output : ".$mkdir_output_."\n";

    my $exec_cmd_ = "$location_to_exec_prefix_{$this_location_} $receiver_exec_name_ $interface_ $ip_ $port_ > $local_file_location_ &";

    system ($exec_cmd_);
}

# Sleep a minute to make sure all receivers on all other servers have been set up.
sleep (60);

# ==========================================================================
# Start all senders.
# ==========================================================================
my $sender_exec_name_ = "/home/sghosh/LiveExec/LatencyExec/sender";
my $sleep_interval_usec_ = "100000"; # 100 msecs => 10 packets / sec.

for (my $location_ = 0; $location_ <= $#other_location_list_; $location_++) {
    my $port_ = $port_prefix_.$location_to_port_{$other_location_list_[$location_]}.$location_to_port_{$this_location_}."1";

    my $exec_cmd_ = "$location_to_exec_prefix_{$this_location_} $sender_exec_name_ $ip_ $port_ 2 $sleep_interval_usec_ &";

    system ($exec_cmd_);
}
