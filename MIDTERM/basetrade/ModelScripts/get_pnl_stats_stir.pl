#!/usr/bin/perl

# \file ModelScripts/get_pnl_stats_stir.pl
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
use feature "switch";          # for given, when
use File::Basename;            # for basename and dirname
use List::Util qw/max min/;    # for max
use POSIX;

my $HOME_DIR = $ENV{'HOME'};
my $USER     = $ENV{'USER'};
my $REPO     = "basetrade";

my $GENPERLLIB_DIR   = $HOME_DIR . "/" . $REPO . "_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR . "/" . $REPO . "_install/ModelScripts";
my $BIN_DIR          = $HOME_DIR . "/" . $REPO . "_install/bin/";
my $LIVE_BIN_DIR     = $HOME_DIR . "/LiveExec/bin";

require "$GENPERLLIB_DIR/array_ops.pl";
require "$GENPERLLIB_DIR/get_shortcode_from_symbol.pl";     # GetShortcodeFromSymbol
require "$GENPERLLIB_DIR/get_yyyymmdd_from_unixtime.pl";    # GetYYYYMMDDFromUnixTime

# require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetVolInTime;
sub TestCacheAndLoad;

# start
my $USAGE = "$0 tradesfile.txt [brief]";

if ( $#ARGV < 0 ) { print $USAGE. "\n"; exit(0); }

my $tradesfilename_ = $ARGV[0];
my $brief_          = 0;
if ( $#ARGV >= 1 ) { $brief_ = $ARGV[1]; }

open TRADESFILEHANDLE, "< $tradesfilename_ " or PrintStacktraceAndDie("Could not open $tradesfilename_\n");

my $tradeon_               = 0;    # trade not open right now
my $time_position_         = 0;    # total time that we have a non zero position
my $this_trade_open_sfm_   = 0;
my $last_pos_sfm_          = 0;
my $sum_abs_pos_time_      = 0;
my $last_closed_trade_pnl_ = 0;
my $num_trades_            = 0;
my $final_volume_          = 0;
my $final_pnl_             = 0;
my $last_pos_              = 0;
my $last_abs_pos_          = 0;
my $min_pnl_               = 0;
my $max_pnl_               = 0;
my $seen_first_trade_      = 0;
my $max_drawdown_          = 0;

my @closed_trade_pnls_               = ();
my @times_to_close_trades_           = ();
my @normalized_time_to_close_trades_ = ();

my $yyyymmdd_                       = 0;
my $last_index_of_trade_start_time_ = 0;
my @time_of_volume_measurements_    = ();
my @per_second_volume_traded_       = ();
my $duration_for_ttc_normalization_ = 0;
my @normalization_weights_          = ();

while ( my $thisline_ = <TRADESFILEHANDLE> ) {
  my @this_words_ = split( ' ', $thisline_ );
  if ( $#this_words_ >= 17 && !( $this_words_[0] eq "PNLSAMPLES" ) && !( $this_words_[0] eq "PNLSPLIT" ) ) {

    my $sfm_    = $this_words_[0];
    my $symbol_ = $this_words_[2];

    $num_trades_++;

    # set these variables ... probably used later for normalized_time_to_close_trades_
    my $tsize_    = $this_words_[4];
    my $tpos_     = $this_words_[6]; #taking 6th colume, will not make sense if number of prods are > 1
    my $totalpnl_ = $this_words_[17];

    $final_volume_ += $tsize_;
    $final_pnl_ = $totalpnl_;

    if ( $seen_first_trade_ == 0 ) {
      $seen_first_trade_ = 1;
      $min_pnl_          = $totalpnl_;
      $max_pnl_          = $totalpnl_;
      $max_drawdown_     = 0;
    }
    else {
      $min_pnl_ = min( $totalpnl_, $min_pnl_ );
      $max_pnl_ = max( $totalpnl_, $max_pnl_ );
      $max_drawdown_ = max( $max_drawdown_, ( $max_pnl_ - $totalpnl_ ) );
    }

    if ( $tradeon_ == 0 ) {
      $this_trade_open_sfm_ = $sfm_;
      $tradeon_             = 1;
      $last_abs_pos_        = abs($tpos_);
      $last_pos_sfm_        = $sfm_;
    }
    else {
      if ( $tpos_ == 0 ) {
        $tradeon_ = 0;
        $time_position_ += ( $sfm_ - $last_pos_sfm_ );
        $sum_abs_pos_time_ += ( $sfm_ - $last_pos_sfm_ ) * $last_abs_pos_;
        $last_abs_pos_ = 0;
        $last_pos_sfm_ = $sfm_;
      }
      else {
        $time_position_ += ( $sfm_ - $last_pos_sfm_ );
        $sum_abs_pos_time_ += ( $sfm_ - $last_pos_sfm_ ) * $last_abs_pos_;
        $last_abs_pos_ = abs($tpos_);
        $last_pos_sfm_ = $sfm_;
        push( @times_to_close_trades_, ( $sfm_ - $this_trade_open_sfm_ ) );

      }
    }
    $last_pos_ = $tpos_;
  }
}

close TRADESFILEHANDLE;

my $average_abs_position_                    = 1;
my $median_time_to_close_trades_             = 1000;
my $average_time_to_close_trades_            = 1000;
my $max_time_to_close_trades_                = 1000;
my $normalized_average_time_to_close_trades_ = 1000;
my $median_closed_trade_pnls_                = 0;
my $average_closed_trade_pnls_               = 0;
my $stdev_closed_trade_pnls_                 = 1;
my $sharpe_closed_trade_pnls_                = 0;
my $fracpos_closed_trade_pnls_               = 0;
my $total_positive_closed_trades_ =0;
my $total_closed_trades_=0;

if ( $#times_to_close_trades_ >= 0 ) {
  $median_time_to_close_trades_  = GetMedianAndSort( \@times_to_close_trades_ );
  $average_time_to_close_trades_ = GetAverage( \@times_to_close_trades_ );
  $max_time_to_close_trades_     = max(@times_to_close_trades_);
}

printf "%.1f %d %d %d %d %d %.2f %.2f %d %d %d %d %d %d %d %d\n", $average_abs_position_, $median_time_to_close_trades_,
  $average_time_to_close_trades_, $median_closed_trade_pnls_, $average_closed_trade_pnls_, $stdev_closed_trade_pnls_,
  $sharpe_closed_trade_pnls_, $fracpos_closed_trade_pnls_, $min_pnl_, $max_pnl_, $max_drawdown_,
  $max_time_to_close_trades_, $normalized_average_time_to_close_trades_,$last_abs_pos_,$total_positive_closed_trades_,$total_closed_trades_=0;

exit(0);
