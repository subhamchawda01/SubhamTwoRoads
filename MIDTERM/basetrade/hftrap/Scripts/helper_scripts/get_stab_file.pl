#!/usr/bin/perl
use strict;
use warnings;

if ($#ARGV < 1 )
{
  die "Usage: <script> <thresh_file> <stat_file>\n";
}

#thresh_file has lines of form - on date 20080701, we select a pair
#only if in addition to adf stat check, it has historically passed 
#the adf test on atleast 15.7895% of cases - columns 1 and 3 are 
#irrelevant
#RCOM_VOLTAS 20080701 -3.138468 0.157895
my $thresh_file = $ARGV[0];

#stat_file has lines of the form 
#TATAMOTORS_MARUTI 20090401 -0.572941 0.3125
#<pair, date, adf stat, stability_thresh>
my $stat_file = $ARGV[1];

#script checks to see that the pair matches the stability criteria and 
#if so outputs the pair

my %thresh_map = ();

open FILE, "<$thresh_file";
my @dates = ();
while(my $line = <FILE>)
{
  chomp $line;
  my @tokens = split(' ', $line);
  $thresh_map{$tokens[1]} = $tokens[3];
}
close FILE;

open FILE, "<$stat_file";
while( my $line = <FILE>)
{
  chomp $line;
  my @tokens = split(' ', $line);
  if( $tokens[3] >= $thresh_map{$tokens[1]} )
  {
    printf "%s\n",$line;
  } 
}
close FILE;
