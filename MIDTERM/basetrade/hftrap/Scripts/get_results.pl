#!/usr/bin/perl

use strict;
use warnings;

if($#ARGV < 3)
{
  print "Usage: <script> <candidate_file> <start_date> <end_date> <param>\n";
  exit(0);
}

my $candidate_file_ = shift;
my $start_date_ = shift;
my $end_date_ = shift;
my $paramfile_ = shift;

open CANDIDATE_FILE, "< $candidate_file_" or die;
while( my $candidate_pair = <CANDIDATE_FILE>)
{
  chomp($candidate_pair);
  my @tokens = split(' ', $candidate_pair);
  system("echo $candidate_pair >>/spare/local/rkumar/resfile_0701_1231");
  my $cmd_ = "~/basetrade_install/bin/spread_exec --paramfile ".$paramfile_." --start_date ".$start_date_." --end_date ".$end_date_." --logfile /spare/local/rkumar/file.test --instrument_1 ".$tokens[0]." --instrument_2 ".$tokens[1]." --notify_last_event --serialized_file 1234 --trade_file /spare/local/rkumar/tradesfile >>/spare/local/rkumar/resfile_0701_1231";
  system($cmd_);
}
