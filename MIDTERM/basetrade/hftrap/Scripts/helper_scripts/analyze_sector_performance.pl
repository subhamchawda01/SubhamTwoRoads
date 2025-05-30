#!/usr/bin/perl
use warnings;
use strict;

if( $#ARGV < 1 )
{
  die "Usage:<script> <sector_file> <result_file>\n";
}

#sector file consists of a filename per line. 
#filenames in turn have a single stock name per line
my $sector_file_ = $ARGV[0];

#result file consists of "pair time_period return" lines
#ACC_ULTRACEMCO 20150101_20151231 -5.02199
my $result_file_ = $ARGV[1];

my %sector_map_ = ();
my @sector_names_ = ();

#load details of sector from sector file
open FILE,"<$sector_file_" or die "Could not open $sector_file_\n";
while( my $line = <FILE> )
{
  chomp $line;
  my @tokens = split('/', $line);
  push @sector_names_, $tokens[$#tokens];
  
  open FILE_2,"<$line" or die "Could not open $line\n";
  while( my $line2 = <FILE_2> )
  {
    chomp($line2);
    $sector_map_{$line2} = $#sector_names_;
  }
  close FILE_2;
}
close FILE;

#load details of pairs and returns from result file
my %num_occurences_ = ();
my %sum_returns_ = ();
my %sum_returns_sqr_ = ();

open FILE, "<$result_file_" or die "Could not open $result_file_";
while( my $line = <FILE> )
{
  chomp $line;
  my @tokens = split(' ', $line);
  my $one_pair_ = $tokens[0];
  chomp $one_pair_;
  my @pair_constituents_ = split('_', $one_pair_);
  if( exists $sector_map_{$pair_constituents_[0]} && 
      exists $sector_map_{$pair_constituents_[1]} )
  {
    if( $sector_map_{$pair_constituents_[0]} != $sector_map_{$pair_constituents_[1]} )
    {
      printf "Error - pair %s of different sectors %s %s \n", $one_pair_, $sector_names_[$sector_map_{$pair_constituents_[0]}], $sector_names_[$sector_map_{$pair_constituents_[1]}];
    }
    else
    {
      if( exists $num_occurences_{$sector_map_{$pair_constituents_[0]}} )
      {
        $num_occurences_{$sector_map_{$pair_constituents_[0]}} =  $num_occurences_{$sector_map_{$pair_constituents_[0]}} + 1;
        $sum_returns_{$sector_map_{$pair_constituents_[0]}} = $sum_returns_{$sector_map_{$pair_constituents_[0]}} + $tokens[2];
        $sum_returns_sqr_{$sector_map_{$pair_constituents_[0]}} = $sum_returns_sqr_{$sector_map_{$pair_constituents_[0]}} + $tokens[2]*$tokens[2];
      }
      else
      {
        $num_occurences_{$sector_map_{$pair_constituents_[0]}} = 1;
        $sum_returns_{$sector_map_{$pair_constituents_[0]}} = $tokens[2];
        $sum_returns_sqr_{$sector_map_{$pair_constituents_[0]}} = $tokens[2]*$tokens[2];
      }
    }
  }
  else
  {
    printf "Error - pair %s not in map \n", $one_pair_;
  }
}
close FILE;

#print results
for( my $ctr = 0; $ctr <= $#sector_names_; $ctr = $ctr + 1 )
{
  if( exists $num_occurences_{$ctr} )
  {
    printf "Sector: %s\t Num_Occurences: %d\t Avg_Return: %f\t Sharpe %f\n", $sector_names_[$ctr], $num_occurences_{$ctr}, $sum_returns_{$ctr}/$num_occurences_{$ctr}, $sum_returns_{$ctr}/$num_occurences_{$ctr}/sqrt( $sum_returns_sqr_{$ctr}/$num_occurences_{$ctr} - $sum_returns_{$ctr}/$num_occurences_{$ctr}*$sum_returns_{$ctr}/$num_occurences_{$ctr} );
  }
  else
  {
    printf "Sector: %s\t Num_Occurences: %d\t Avg_Return: %f\t Sharpe %f\n", $sector_names_[$ctr], 0, 0.0, 0.0;
  }
}
