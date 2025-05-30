#!/usr/bin/perl

# \file ModelScripts/get_pnl_stats.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
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
use POSIX;

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin/";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";


require "$GENPERLLIB_DIR/array_ops.pl";
require "$GENPERLLIB_DIR/get_duration_ttc_normalization.pl"; # GetDurationTTCNormalization
require "$GENPERLLIB_DIR/get_shortcode_from_symbol.pl"; # GetShortcodeFromSymbol
require "$GENPERLLIB_DIR/get_yyyymmdd_from_unixtime.pl"; # GetYYYYMMDDFromUnixTime
require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec

# require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $GET_SHORTCODE_FOR_SYMBOL = SearchExec("get_shortcode_for_symbol");
my $GET_PERIODIC_VOLUME_ON_DAY = SearchExec("get_periodic_volume_on_day");

sub GetVolInTime ;
sub TestCacheAndLoad ;

# start 
my $USAGE="$0 tradesfile.txt [brief]";

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
my $last_pos_ = 0;
my $last_abs_pos_ = 0;
my $last_closed_trade_pnl_ = 0;
my $num_trades_ = 0;
my $final_volume_ = 0;
my $final_pnl_ = 0;

my $min_pnl_ = 0;
my $max_pnl_ = 0;
my $seen_first_trade_ = 0;
my $max_drawdown_ = 0;

my $total_positive_closed_trades_ = 0;
my $total_closed_trades_ = 0;

my @closed_trade_pnls_ = ();
my @times_to_close_trades_ = ();
my @normalized_time_to_close_trades_ = ();

my $yyyymmdd_ = 0;
my $last_index_of_trade_start_time_ = 0;
my @time_of_volume_measurements_ = ();
my @per_second_volume_traded_ = ();
my $duration_for_ttc_normalization_ = 0 ;
my @normalization_weights_ = ();

while ( my $thisline_ = <TRADESFILEHANDLE> )
{
    my @this_words_ = split ( ' ', $thisline_ );
    if ( $#this_words_ >= 15  && !( $this_words_[0] eq "PNLSAMPLES" ))
    {

	$num_trades_ ++;
	my $sfm_ = $this_words_[0];

#	my $pos_state_ = $this_words_[1];
	my $symbol_ =  $this_words_[2] ; 

	# set these variables ... probably used later for normalized_time_to_close_trades_
	if ( $yyyymmdd_ == 0 ) { 
	  # it should be 0 only the first time.
	  $yyyymmdd_ = GetYYYYMMDDFromUnixTime ( $sfm_ ) ; 
	  if ( ( $yyyymmdd_ < 20110101 ) || ( $yyyymmdd_ > 20201231 ) ) {
	    $yyyymmdd_ = 20131031; # dummy ;
	  } else {
            my @sym_words_ = split ( '\.', $symbol_ ) ; 
            my $t_shc_ = "";
            if ( $#sym_words_ > 0 ) { $t_shc_ = $sym_words_[0]  ;}
	    my $shortcode_ = "";
            if  ( $t_shc_ ) 
            {
		if ( index ( $t_shc_, "NSE_" ) != -1 )
		{
		    $shortcode_ = $t_shc_;
		}
		else
		{
		    $shortcode_ = `$GET_SHORTCODE_FOR_SYMBOL $t_shc_ $yyyymmdd_`; chomp ( $shortcode_) ; # GetShortcodeFromSymbol ( $symbol_, $yyyymmdd_);
		}
            }
	    if ( $shortcode_ )
	      {
		if ( $duration_for_ttc_normalization_ == 0 ) 
		  { $duration_for_ttc_normalization_ = GetDurationTTCNormalization ( $shortcode_ ) ; }
		if ( $duration_for_ttc_normalization_ > 0 )
		  { # in case we don't ave data for a symbol
		    # reset in case it was set before ... should not be needed
		    @per_second_volume_traded_ = ();
		    @time_of_volume_measurements_ = ();
		    
		    my @vols_in_day_ = TestCacheAndLoad ( $shortcode_, $yyyymmdd_ ) ; 
		    foreach my $execout_line_ (@vols_in_day_)
		      {
			my @execout_words_ = split(' ', $execout_line_);
			if ( $#execout_words_ >= 1 ) {
			  push(@time_of_volume_measurements_, $execout_words_[0]);
			  push(@per_second_volume_traded_, $execout_words_[1]);
			}
		      }
		  }
	      }
	  }
	}

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
            $last_pos_ = $tpos_;
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
                $last_pos_ = 0;

		# calculating total closed trades ( going from position 0 to 0 ) 
		$total_closed_trades_ ++ ;

		# total positive closed trades ( ratio $total_positive_closed_trades_/$total_closed_trades_ gives us
		# an idea how many chances were grabbed versus the number of chances given  )
		if ( ( $totalpnl_ - $last_closed_trade_pnl_ )  > 0 )
		{
			$total_positive_closed_trades_ ++;
		}

		push ( @closed_trade_pnls_, ( $totalpnl_ - $last_closed_trade_pnl_ ) );
		$last_closed_trade_pnl_ = $totalpnl_ ;


		$last_pos_sfm_ = $sfm_;

		push ( @times_to_close_trades_, ( $sfm_ - $this_trade_open_sfm_ ) );

		if ( $#per_second_volume_traded_ >= 0 )
		  {
		    my $int_vol_ = GetVolInTime ( $this_trade_open_sfm_, $sfm_ );
#		    printf "DEBUG vol at time %.2f %d\n", $int_vol_, ( $sfm_ - $this_trade_open_sfm_ );
		    push ( @normalized_time_to_close_trades_, ($sfm_ - $this_trade_open_sfm_) * $int_vol_ );
		    push ( @normalization_weights_, $int_vol_ ) ;
		  }
		else
		  {
		    push ( @normalized_time_to_close_trades_, ($sfm_ - $this_trade_open_sfm_) );
		    push ( @normalization_weights_, 1 ) ;
		  }
	    } 
            else
            {
              if($last_pos_*$tpos_ < 0)
              {
		$time_position_ += ( $sfm_ - $last_pos_sfm_ );
		$sum_abs_pos_time_ += ( $sfm_ - $last_pos_sfm_ ) * $last_abs_pos_;
                $last_pos_ = $tpos_;
		$last_abs_pos_ = abs ( $tpos_ );

		# calculating total closed trades ( going from position 0 to 0 ) 
		$total_closed_trades_ ++ ;

		# total positive closed trades ( ratio $total_positive_closed_trades_/$total_closed_trades_ gives us
		# an idea how many chances were grabbed versus the number of chances given  )
		if ( ( $totalpnl_ - $last_closed_trade_pnl_ )  > 0 )
		{
			$total_positive_closed_trades_ ++;
		}

		push ( @closed_trade_pnls_, ( $totalpnl_ - $last_closed_trade_pnl_ ) );
		$last_closed_trade_pnl_ = $totalpnl_ ;


		$last_pos_sfm_ = $sfm_;

		push ( @times_to_close_trades_, ( $sfm_ - $this_trade_open_sfm_ ) );

		if ( $#per_second_volume_traded_ >= 0 )
		  {
		    my $int_vol_ = GetVolInTime ( $this_trade_open_sfm_, $sfm_ );
#		    printf "DEBUG vol at time %.2f %d\n", $int_vol_, ( $sfm_ - $this_trade_open_sfm_ );
		    push ( @normalized_time_to_close_trades_, ($sfm_ - $this_trade_open_sfm_) * $int_vol_ );
		    push ( @normalization_weights_, $int_vol_ ) ;
		  }
		else
		  {
		    push ( @normalized_time_to_close_trades_, ($sfm_ - $this_trade_open_sfm_) );
		    push ( @normalization_weights_, 1 ) ;
		  }              
              }
	      else
	      {
		  $time_position_ += ( $sfm_ - $last_pos_sfm_ );
		  $sum_abs_pos_time_ += ( $sfm_ - $last_pos_sfm_ ) * $last_abs_pos_;
                  $last_pos_ = $tpos_;
		  $last_abs_pos_ = abs ( $tpos_ );
		  $last_pos_sfm_ = $sfm_;
	      }
            }
	}
  }
}
close TRADESFILEHANDLE;

my $average_abs_position_ = 1; 
my $median_time_to_close_trades_ = 1000;
my $average_time_to_close_trades_ = 1000;
my $max_time_to_close_trades_ = 1000;
my $normalized_average_time_to_close_trades_ = 1000;
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

if ( $#normalized_time_to_close_trades_ >= 0 )
{
    my $sum_of_weights_ = GetSum ( \@normalization_weights_ );
    if ( $sum_of_weights_ > 0 ) {
	$normalized_average_time_to_close_trades_ = GetSum ( \@normalized_time_to_close_trades_ ) / $sum_of_weights_;
    } else {
      $normalized_average_time_to_close_trades_ = $average_time_to_close_trades_;
    }
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
    printf "%.1f %d %d %d %d %d %.2f %.2f %d %d %d %d %d %f %d %d\n", $average_abs_position_,$median_time_to_close_trades_, $average_time_to_close_trades_, $median_closed_trade_pnls_, $average_closed_trade_pnls_, $stdev_closed_trade_pnls_, $sharpe_closed_trade_pnls_, $fracpos_closed_trade_pnls_, $min_pnl_, $max_pnl_, $max_drawdown_ , $max_time_to_close_trades_, $normalized_average_time_to_close_trades_,$last_abs_pos_,$total_positive_closed_trades_,$total_closed_trades_;
}
elsif ( $brief_ == 2 ) 
{
    printf "%d %d %d %.1f %d %d\n", $final_pnl_, $final_volume_, $num_trades_, $average_abs_position_, $median_time_to_close_trades_, $max_drawdown_ ;
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
    printf "Normalized averagetrade-close-time ( secs ): %d\n", $normalized_average_time_to_close_trades_;
    printf "Last Abs position: %f\n", $last_abs_pos_;
    printf "Total Positive Closed Trades: %d\n", $total_positive_closed_trades_;
    printf "Total Closed Trades: %d\n", $total_closed_trades_;
}

exit ( 0 );


# subs #

sub GetVolInTime
{
  my ($start_time_, $end_time_) = @_;

  if ( ( $#time_of_volume_measurements_ < 0 ) ||
       ( $end_time_ <= $start_time_ ) ) { 
    # in case we don't have any data

    return 1;
  }

  my $diff_time_ = $end_time_ - $start_time_;
  if ($diff_time_ < $duration_for_ttc_normalization_)
  { # the duration of this trade is less than input argument for period
    my $time_to_increase_width_by_ = ($duration_for_ttc_normalization_ - $diff_time_)/2;
    $start_time_ = max ( $time_of_volume_measurements_[0], ( $start_time_ - $time_to_increase_width_by_ ) );
    $end_time_ = min ( $time_of_volume_measurements_[$#time_of_volume_measurements_], ( $end_time_ + $time_to_increase_width_by_ ) );
  }

  # not checking for index here ... asusming that last_index_of_trade_start_time_ is a valid index in the array
  if ( $time_of_volume_measurements_[$last_index_of_trade_start_time_] > $start_time_ ) {
    # we have to search backward ... not sure when this will happen
    while ( ( $time_of_volume_measurements_[$last_index_of_trade_start_time_] > $start_time_ ) && ( $last_index_of_trade_start_time_ > 0 ) ) {
      $last_index_of_trade_start_time_ = $last_index_of_trade_start_time_ - 1;
    }
  } elsif ( ( $time_of_volume_measurements_[$last_index_of_trade_start_time_] < $start_time_ ) && ( $last_index_of_trade_start_time_ < $#time_of_volume_measurements_ ) ) {
    while ( ( $time_of_volume_measurements_[$last_index_of_trade_start_time_] < $start_time_ ) && ( $last_index_of_trade_start_time_ < $#time_of_volume_measurements_ ) ) {
      $last_index_of_trade_start_time_ ++ ;
    }
    # we have to search backward ... not sure when this will happen
    while ( ( $time_of_volume_measurements_[$last_index_of_trade_start_time_] > $start_time_ ) && ( $last_index_of_trade_start_time_ > 0 ) ) {
      $last_index_of_trade_start_time_ = $last_index_of_trade_start_time_ - 1;
    }
  }

  my $num_ = 0;
  my $total_vol_ = 0;
  my $st_time_ = $time_of_volume_measurements_[$last_index_of_trade_start_time_];

  while ( ( $time_of_volume_measurements_[$last_index_of_trade_start_time_] <= $end_time_ ) && ( $last_index_of_trade_start_time_ < $#time_of_volume_measurements_ ) )
    {
      $num_ = $num_ + 1;
      $total_vol_ = $total_vol_ + $per_second_volume_traded_[$last_index_of_trade_start_time_] ;
      $last_index_of_trade_start_time_ = $last_index_of_trade_start_time_ + 1 ;
    }

  my $en_time_ = $time_of_volume_measurements_[$last_index_of_trade_start_time_];

  if ( ( $num_ == 0 ) && ( ( $last_index_of_trade_start_time_ + 1 ) <= $#time_of_volume_measurements_ ) )
  {
   return $per_second_volume_traded_[$last_index_of_trade_start_time_ + 1];
  }
  elsif ( $last_index_of_trade_start_time_ == $#time_of_volume_measurements_ )
  {
    return 0; #this will happen only if we dont have volume data for all trade time
    return $per_second_volume_traded_[$#per_second_volume_traded_]; #$end_time_ - $start_time_;
  }

  return $total_vol_/($en_time_ - $st_time_);
}

sub TestCacheAndLoad
{
    my $t_shortcode_ = shift;
    my $t_yyyymmdd_ = shift;
    my @ret_vols_in_day_ = ();
    my $cache_results_file_ = "/spare/local/dvctrader/GPV/gpv_cache_".$t_shortcode_."_".$t_yyyymmdd_.".txt";
    if ( -e $cache_results_file_ )
    {
	open GPVCACHEFILEHANDLE, "< $cache_results_file_ " ;
	@ret_vols_in_day_ = <GPVCACHEFILEHANDLE>;
	close GPVCACHEFILEHANDLE ;
    } 
    elsif ( $USER eq "sghosh" )
    { # do nothing if sghosh
    } 
    else
    {
	my $exec_command_ = $GET_PERIODIC_VOLUME_ON_DAY." ".$t_shortcode_." ".$t_yyyymmdd_." 1"; 
	@ret_vols_in_day_ = `$exec_command_`; 
	if ( $USER eq "dvctrader" ) {
	    # create cache file only if dvctrader
	    `mkdir -p /spare/local/dvctrader/GPV/`;
	    `touch $cache_results_file_`;
	    if ( -e $cache_results_file_ ) {
		# if we were successfully able to create the file in the step above
		open GPVCACHEFILEHANDLE, "> $cache_results_file_ " ;
		print GPVCACHEFILEHANDLE @ret_vols_in_day_;
		close GPVCACHEFILEHANDLE ;
	    }
	}
    }
    if ( $#ret_vols_in_day_ >= 0 ) {
	chomp ( @ret_vols_in_day_ );
    }
    @ret_vols_in_day_;
}
