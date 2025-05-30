#!/usr/bin/perl

# \file ModelScripts/get_pnl_stats.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
# This script takes a tradesfilename, and prints :
# Average abs position when a trade is on ( measure of risk taken in average ), 
#   where averaging is over time
# Average time to close a trade
# <Median, Average, Stdev, Sharpe, Posfrac> of closed trade pnls

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use List::Util qw/max min/; # for max


my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/array_ops.pl";
# require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 tradesfile.txt brief=[1|2|3]";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $tradesfilename_ = $ARGV[0];
my $brief_ = 0;
if ( $#ARGV >= 1 ) { $brief_ = $ARGV[1]; }

open TRADESFILEHANDLE, "< $tradesfilename_ " or PrintStacktraceAndDie ( "Could not open $tradesfilename_\n" );

my $tradeon_ = 0; # trade not open right now
my $time_position_ = 0; # total time that we have a non zero position
my $this_trade_open_sfm_ = 0;
my $last_pos_sfm_ = 0;
my $sum_abs_pos_time_ = 0;
my $last_abs_pos_ = 0;
my $last_closed_trade_pnl_ = 0;
my $num_trades_ = 0;
my $final_volume_ = 0;
my $final_pnl_ = 0;

my $min_pnl_ = 0;
my $max_pnl_ = 0;
my $seen_first_trade_ = 0;
my $max_drawdown_ = 0;

my @closed_trade_pnls_ = ();
my @times_to_close_trades_ = ();
while ( my $thisline_ = <TRADESFILEHANDLE> )
{
    my @this_words_ = split ( ' ', $thisline_ );
    if ( $#this_words_ >= 15 )
    {
	$num_trades_ ++;
	my $sfm_ = $this_words_[0];
#	my $pos_state_ = $this_words_[1];
#	my $symbol_ = $this_words_[2];
#	my $buysell_ = $this_words_[3];
	my $tsize_ = $this_words_[4];
#	my $tprice_ = $this_words_[5];
	my $tpos_ = $this_words_[6];
#	my $unrealtradepnl_ = $this_words_[7];
	my $totalpnl_ = $this_words_[8];
	
	$final_volume_ += $tsize_;
	$final_pnl_ = $totalpnl_ ;

	if ( $seen_first_trade_ == 0 )
	{
	    $seen_first_trade_ = 1;
	    $min_pnl_ = $totalpnl_ ;
	    $max_pnl_ = $totalpnl_ ;
	    $max_drawdown_ = 0;
	}
	else
	{
	    $min_pnl_ = min ( $totalpnl_, $min_pnl_ );
	    $max_pnl_ = max ( $totalpnl_, $max_pnl_ );
	    $max_drawdown_ = max ( $max_drawdown_, ( $max_pnl_ - $totalpnl_ ) ) ;
	}

	if ( $tradeon_ == 0 )
	{ # this trade opens a trade
	    $this_trade_open_sfm_ = $sfm_;
	    $tradeon_ = 1;
	    # $sum_abs_pos_time_ and $time_position_ don't count time spent being flat
	    $last_abs_pos_ = abs ( $tpos_ );
	    $last_pos_sfm_ = $sfm_;
	}
	else
	{
	    if ( $tpos_ == 0 )
	    { # this trade closes the trade
		$tradeon_ = 0;
		$time_position_ += ( $sfm_ - $last_pos_sfm_ );
		$sum_abs_pos_time_ += ( $sfm_ - $last_pos_sfm_ ) * $last_abs_pos_;
		$last_abs_pos_ = 0;

		push ( @closed_trade_pnls_, ( $totalpnl_ - $last_closed_trade_pnl_ ) );
		$last_closed_trade_pnl_ = $totalpnl_ ;

		$last_pos_sfm_ = $sfm_;

		push ( @times_to_close_trades_, ( $sfm_ - $this_trade_open_sfm_ ) );
#		printf "this trade life: %d\n", ( $sfm_ - $this_trade_open_sfm_ ) ;
	    }
	    else
	    {
		$time_position_ += ( $sfm_ - $last_pos_sfm_ );
		$sum_abs_pos_time_ += ( $sfm_ - $last_pos_sfm_ ) * $last_abs_pos_;
		$last_abs_pos_ = abs ( $tpos_ );
		$last_pos_sfm_ = $sfm_;
	    }
	}
    }
}
close TRADESFILEHANDLE;

my $average_abs_position_ = 1; 
my $median_time_to_close_trades_ = 1000;
my $average_time_to_close_trades_ = 1000;
my $max_time_to_close_trades_ = 1000;
my $median_closed_trade_pnls_ = 0;
my $average_closed_trade_pnls_ = 0;
my $stdev_closed_trade_pnls_ = 1;
my $sharpe_closed_trade_pnls_ = 0;
my $fracpos_closed_trade_pnls_ = 0;

if ( $time_position_ > 0 ) { $average_abs_position_ = ( $sum_abs_pos_time_ / $time_position_ ); }
if ( $#times_to_close_trades_ >= 0 )
{
    $median_time_to_close_trades_ = GetMedianAndSort ( \@times_to_close_trades_ );
    $average_time_to_close_trades_ = GetAverage ( \@times_to_close_trades_ );
    $max_time_to_close_trades_ = max ( @times_to_close_trades_ );
}
if ( $#closed_trade_pnls_ >= 0 )
{
    $median_closed_trade_pnls_ = GetMedianAndSort ( \@closed_trade_pnls_ );
    $average_closed_trade_pnls_ = GetAverage ( \@closed_trade_pnls_ );
    $stdev_closed_trade_pnls_ = GetStdev ( \@closed_trade_pnls_ );
    if ( $stdev_closed_trade_pnls_ > 0 )
    {
	$sharpe_closed_trade_pnls_ = ( $average_closed_trade_pnls_ / $stdev_closed_trade_pnls_ );
    }
    $fracpos_closed_trade_pnls_ = GetPosFracThresh ( \@closed_trade_pnls_ );
}

if ( $brief_ == 1)
{
    printf "%.1f %d %d %d %d %d %.2f %.2f %d %d %d %d %d\n", $average_abs_position_, $median_time_to_close_trades_, $average_time_to_close_trades_, $median_closed_trade_pnls_, $average_closed_trade_pnls_, $stdev_closed_trade_pnls_, $sharpe_closed_trade_pnls_, $fracpos_closed_trade_pnls_, $min_pnl_, $max_pnl_, $max_drawdown_ , $max_time_to_close_trades_, $last_abs_pos_;
}
elsif ( $brief_ == 2 ) 
{
    printf "%d %d %d %.1f %d %d %d\n", $final_pnl_, $final_volume_, $num_trades_, $average_abs_position_, $median_time_to_close_trades_, $max_drawdown_, $last_abs_pos_ ;
}
elsif ( $brief_ == 3 ) 
{
    printf "%.1f %d %d %d %d %d %.2f %.2f %d %d %d %d %d %d %d %d\n", $average_abs_position_, $median_time_to_close_trades_, $average_time_to_close_trades_, $median_closed_trade_pnls_, $average_closed_trade_pnls_, $stdev_closed_trade_pnls_, $sharpe_closed_trade_pnls_, $fracpos_closed_trade_pnls_, $min_pnl_, $max_pnl_, $max_drawdown_ , $max_time_to_close_trades_, $final_pnl_, $final_volume_, $num_trades_, $last_abs_pos_;
}

else
{
    printf "Average Abs position: %.1f\n", $average_abs_position_;
    printf "Median trade-close-time ( secs ): %d\n", $median_time_to_close_trades_;
    printf "Average trade-close-time ( secs ): %d\n", $average_time_to_close_trades_;
    printf "Max trade-close-time ( secs ): %d\n", $max_time_to_close_trades_;
    printf "TradePNL stats: Median: %d Avg: %d Stdev: %d Sharpe: %.2f Fracpos: %.2f\n", $median_closed_trade_pnls_, $average_closed_trade_pnls_, $stdev_closed_trade_pnls_, $sharpe_closed_trade_pnls_, $fracpos_closed_trade_pnls_ ;
    printf "PNL-min-max-draw: %d %d %d\n", $min_pnl_, $max_pnl_, $max_drawdown_ ;
    printf "NumTradeLines : %d\n", $num_trades_ ;
    printf "FinalVolume: %d\n", $final_volume_ ;
    printf "FinalPNL: %d\n", $final_pnl_ ;
    my $pnl_per_contract_ = 0;
    if ( $final_volume_ > 0 )
    {
	$pnl_per_contract_ = $final_pnl_ / $final_volume_ ;
    }
    printf "PNL_per_contract : %.2f\n", $pnl_per_contract_ ;
    printf "Last absolute position : %d\n", $last_abs_pos_;
}
