#!/usr/bin/perl

use strict;
use warnings;

#input is file with colums being 
#<time in secs> pnl_1 pnl_2 .. pnl_k

if( $#ARGV != 0 )
{
  die "Usage: script <file>\n";
}

my $start_time_ = -1;
my $end_time_ = 0;
my @pnl_factors_ = (); #normalization factors for individual pnl series to account for eq weighted port
#for computing max drawdown
my $max_pnl_ = 0;
my $max_dd_ = 0;

#for computing stev
my $sum_ret_ = 0;
my $sum_sqr_ret_ = 0;
my $count = 0;
my @hist_pnls_ = ();

my $ret_offset_ = 60; #return duration in minutes

#for ann return comp
my $curr_pnl_ = 0;
my $init_pnl_ = 0;

open FILE, "<$ARGV[0]" or die "Could not open $ARGV[0]\n";
while( my $line = <FILE> )
{
  chomp $line;
  my @tokens = split(' ',$line);
  #process first line separately
  if( $start_time_ == -1 )
  {
    $start_time_ = $tokens[0];
    for( my $ctr = 1; $ctr <= $#tokens; $ctr = $ctr + 1 )
    {
      push @pnl_factors_, 10000000.0/$tokens[$ctr];      
    }
    $init_pnl_ = ($#tokens * 10000000.0);
  }

  $end_time_ = $tokens[0];
  $curr_pnl_ = 0;
  for( my $ctr = 1; $ctr <= $#tokens; $ctr = $ctr + 1 )
  {
    $curr_pnl_ = $curr_pnl_ + $pnl_factors_[$ctr-1]*$tokens[$ctr];
  }
  
  #modify max and dd if needed
  if( $curr_pnl_ > $max_pnl_ )
  {
     $max_pnl_ = $curr_pnl_;
  }
  if (1.0 - $curr_pnl_/$max_pnl_ > $max_dd_ )
  {
    $max_dd_ = 1.0 - $curr_pnl_/$max_pnl_;  
  }

  #stdev comp variables
  push @hist_pnls_, $curr_pnl_;

  if( $#hist_pnls_ >= $ret_offset_ )
  {
    shift @hist_pnls_;
  }

  if( $#hist_pnls_ == $ret_offset_-1 )
  {
    $count = $count + 1;
    my $t_ret_ = $hist_pnls_[$#hist_pnls_]/$hist_pnls_[0] - 1.0;
    $sum_ret_ = $sum_ret_ + $t_ret_;
    $sum_sqr_ret_ = $sum_sqr_ret_ + $t_ret_*$t_ret_;  
  }
}
close FILE;

printf "Annualized Return: %f\n", 100.0*(($curr_pnl_/$init_pnl_)**(365*86400/($end_time_ - $start_time_)) - 1.0);
printf "Annualized Stdev: %f\n", sqrt(262*6.5*60/$ret_offset_)*sqrt($sum_sqr_ret_/$count - $sum_ret_/$count*$sum_ret_/$count)*100.0;
printf "Max Drawdown: %f\n", $max_dd_*100.0;
