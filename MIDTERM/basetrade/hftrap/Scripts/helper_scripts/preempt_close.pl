#!/usr/bin/perl
use strict;
use warnings;
use List::Util qw(first);

#Preempts end-time in cases where tstat falls
if( $#ARGV < 2 )
{
  die "Usage: <script> <selected_pairs_file> <stat_file> <preempt_thresh>\n";
}

my $selected_pairs_file_ = $ARGV[0];
my $all_stat_file_ = $ARGV[1]; #assumes file is sorted by date 
my $exit_thresh_ = $ARGV[2]; #thresh at which to exit trade

my @selected_pairs_ = ();
my @begin_dates_selected_pairs_ = ();
my @end_dates_selected_pairs_ = ();


open FILE, "<$selected_pairs_file_" or die "Could not open $selected_pairs_file_\n";
while( my $line = <FILE> )
{
  chomp $line;
  my @tokens = split(' ', $line);
  push @selected_pairs_, $tokens[0];
  push @begin_dates_selected_pairs_, $tokens[1];
  push @end_dates_selected_pairs_, ($tokens[1]+10000);
}
close FILE;

for( my $ctr = 0; $ctr <= $#selected_pairs_; $ctr = $ctr + 1 )
{
  my @one_pair_dates_ = ();
  my @one_pair_adfstats_ = ();
  open FILE, "<$all_stat_file_" or die "Could not open $all_stat_file_\n";
  while ( my $line = <FILE> )
  {
    chomp $line;
    my @tokens = split(' ', $line);
    if( $tokens[0] eq $selected_pairs_[$ctr] )
    {
      push @one_pair_dates_, $tokens[1];
      push @one_pair_adfstats_, $tokens[2];
    }
  }
  close FILE;
  my $trade_start_index_ = first{$one_pair_dates_[$_] >= $begin_dates_selected_pairs_[$ctr] } 0..$#one_pair_dates_;
  if( ! defined $trade_start_index_ )
  {
    die "Trade start date not found in stat file $begin_dates_selected_pairs_[$ctr],$selected_pairs_[$ctr]\n";
  }
  #weighted adf stat over last 3 observations - initialiization
  my $astat_minus2_ = $one_pair_adfstats_[$trade_start_index_];
  my $astat_minus1_ = $one_pair_adfstats_[$trade_start_index_];
  my $astat_minus0_ = $one_pair_adfstats_[$trade_start_index_];
  my $weighted_adfstat_ = $one_pair_adfstats_[$trade_start_index_];
  my $running_index_ = $trade_start_index_;
  while( $running_index_ < $#one_pair_adfstats_ )
  {
    $running_index_ = $running_index_ + 1;
    $astat_minus2_ = $astat_minus1_;
    $astat_minus1_ = $astat_minus0_;
    $astat_minus0_ = $one_pair_adfstats_[$running_index_];
    $weighted_adfstat_ = 0.5*$astat_minus0_ + 0.33*$astat_minus1_ + 0.17*$astat_minus2_;
    if( $weighted_adfstat_ >= $exit_thresh_ )
    {   
      last;
    }
  }
  if( $one_pair_dates_[$running_index_] < $end_dates_selected_pairs_[$ctr] )
  {
    $end_dates_selected_pairs_[$ctr] =  $one_pair_dates_[$running_index_];
  }
  printf "%s %d %d\n", $selected_pairs_[$ctr], $begin_dates_selected_pairs_[$ctr], $end_dates_selected_pairs_[$ctr];
}

