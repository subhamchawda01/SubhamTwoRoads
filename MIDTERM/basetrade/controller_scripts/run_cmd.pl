#!/usr/bin/perl
use strict;
use warnings;

my $UUID=`uuidgen`;
chomp ( $UUID );
my $CMD="";
my $CMD_ID="";
my $IP=$ARGV[0];
if( $#ARGV > 2 && $ARGV[1] eq "-t" ) {
  $UUID=$UUID."-".$ARGV[2];
  $CMD=join ( " ", @ARGV[3..$#ARGV]);
  $CMD_ID=join ( "~", @ARGV[3..$#ARGV]);
}
else {
  $CMD=join ( " ", @ARGV[1..$#ARGV]);
  $CMD_ID=join ( "~", @ARGV[1..$#ARGV]);
}

my $command_file="/mnt/sdf/JOBS/job_desc/COMMAND_".$IP."_".$UUID;
my $log_file="/mnt/sdf/JOBS/log/log_".$IP."_".$UUID;
my $done_file="/mnt/sdf/JOBS/job_desc/DONE_".$IP."_".$UUID;
my $running_file="/mnt/sdf/JOBS/job_desc/RUNNING_".$IP."_".$UUID;

my $fh;
open ( $fh, ">$command_file");
print $fh "touch $running_file\n";
print $fh "export DVC_JOB_ID=$CMD_ID\n";
print $fh "$CMD > $log_file 2>&1 \n";
print $fh "touch $done_file";
close ($fh);

my $cmd_remote = "ssh -f -n -o ConnectTimeout=5 -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $IP \"nice -n 5 sh -c 'nohup sh $command_file >/dev/null 2>&1 </dev/null & ' \"";
# my $cmd_remote = "ssh -f -n -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $IP \"sh -c 'nohup sh $command_file 2>&1 </dev/null & ' \" | mail -s \"ec2 JOB on $IP\" nseall@tworoads.co.in";
my $datestring = localtime();
print "Executing... $cmd_remote\n";
print "Executed at: $datestring\n";
`$cmd_remote`;

