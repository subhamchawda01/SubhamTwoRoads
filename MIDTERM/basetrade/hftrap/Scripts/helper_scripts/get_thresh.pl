#!/usr/bin/perl
use strict;
use warnings;

if($#ARGV < 2 )
{
  die "Usage: <script> <num_pairs_file> <stat_file> <thresh_frac>\n";
}
#file with format <date> <num_pairs_active_on_date>
my $num_file = $ARGV[0];

#TATAMOTORS_MARUTI 20090101 -1.694497 0.384615
#stat file is one with pair, date, adf stat and stability value
my $stat_file = $ARGV[1];

#frac denotes what %tile threshold to extract for stability value.
#higher value is more restrictive ( fewer pairs will pass the test )
my $frac = $ARGV[2];

my %thresh_map = ();

open FILE, "<$num_file";
my @dates = ();
while(my $line = <FILE>)
{
  chomp $line;
  my @tokens = split(' ', $line);
  $thresh_map{$tokens[0]} = int($frac*$tokens[1]);
  push @dates, $tokens[0];
}
close FILE;

for( my $ctr = 0; $ctr <= $#dates; $ctr = $ctr + 1 )
{
  my $cmd = "grep $dates[$ctr] $stat_file | sort --key=4 -g | head -n $thresh_map{$dates[$ctr]} | tail -n1";
  system($cmd);
}

