#!/usr/bin/perl

# \file ModelScripts/find_optimal_max_loss.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use POSIX;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $BASETRADE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

my $GLOBAL_RESULTS_DIR = $HOME_DIR."/ec2_globalresults";
my $GLOBAL_STAGEDRESULTS_DIR = $HOME_DIR."/ec2_staged_globalresults";
my $hostname_s_ = `hostname -s`; chomp ( $hostname_s_ );
if ( ! ( $USER eq "dvctrader" && ( ( $hostname_s_ eq "sdv-ny4-srv13" ) || ( $hostname_s_ eq "sdv-ny4-srv11" ) || ( $hostname_s_ eq "sdv-crt-srv11" ) ) ) )
{
    $GLOBAL_RESULTS_DIR = "/NAS1/ec2_globalresults"; # on non ny4srv11 and crtsrv11 ... look at NAS
    $GLOBAL_STAGEDRESULTS_DIR = "/NAS1/ec2_staged_globalresults"; # on non ny4srv11 and crtsrv11 ... look at NAS
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/global_results_methods.pl"; # GetPnlFromGlobalResultsLine , GetMinPnlFromGlobalResultsLine
require "$GENPERLLIB_DIR/strat_utils.pl"; # CheckIfRegimeParam
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

my $USAGE="$0 SHORTCODE STRAT_NAME ENDDATE=TODAY BACKTEST_DAYS";

if ( $#ARGV < 3 ) {
  print $USAGE."\n";
  exit(1);
}

my $shortcode_ = $ARGV [ 0 ];
my $strat_name_ = $ARGV [ 1 ];
my $end_date_ = $ARGV [ 2 ];
my $backtest_ndays_ = $ARGV [ 3 ];
my $use_notional_ = 0;

my $start_date_ = CalcPrevWorkingDateMult ( $end_date_ , $backtest_ndays_ );

my @unique_id_ = `date +\%s\%N`; chomp ( @unique_id_ );
my $OML_strats_file_ = "/spare/local/".$USER."/OML_strats_file_".$shortcode_.$unique_id_ [ 0 ];

my %strat_to_min_pnl_ = ( );
my %strat_to_final_pnl_ = ( );
my %strat_to_lossno_ = ( );
my %strat_to_results_ = ( );
my %strat_to_unit_trade_size_ = ( );
my %date_to_normal_lossno_ = ( );
my %date_to_stdev_lossno_ = ( );

GetGlobalResultsForRunningStrats ( );

GetUnitTradeSizesForStrats ( );

GetStratsPnl ( );

RunOptimalMaxLossScript ( );

RunStdevOptimalMaxLossScript ( );

PrintLosses ( );

sub GetGlobalResultsForRunningStrats
{
  my $SUMMARIZE_STRATEGY_RESULTS_LONG = $LIVE_BIN_DIR."/summarize_strategy_results";
  
  my $file_handle_ = FileHandle->new;
  $file_handle_->open ( "> $OML_strats_file_ " ) or PrintStacktraceAndDie ( "Could not open $OML_strats_file_ for writing\n" );
  print $file_handle_ "$strat_name_\n";
  $file_handle_->close;
  
  my $exec_cmd_ = "$SUMMARIZE_STRATEGY_RESULTS_LONG $shortcode_ $OML_strats_file_ $GLOBAL_RESULTS_DIR $start_date_ $end_date_ IF kCNAPnlAdjAverage 0 INVALIDFILE 0 2>/dev/null";
  print $exec_cmd_."\n";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );
  my $strat_name_ = "";
  foreach my $exec_output_line_ ( @exec_output_ )
  {
    if ( $exec_output_line_ eq "" || $exec_output_line_ eq "\n" || index ( $exec_output_line_ , "STATISTICS" ) >= 0 )
    {
      next;
    }
    if ( index ( $exec_output_line_ , "STRATEGYFILEBASE" ) >= 0 )
    {
      chomp ( $exec_output_line_ );
      my @exec_output_line_words_ = split ( ' ' , $exec_output_line_ );
      $strat_name_ = $exec_output_line_words_ [ 1 ];
      @{ $strat_to_results_ { $strat_name_ } } = ( );
    }
    else
    {
      push ( @{ $strat_to_results_ { $strat_name_ } } , $exec_output_line_ );
    }
  }
  `rm -rf $OML_strats_file_`;
#  print join("\n", @{ $strat_to_results_ { $strat_name_ } } )."\n";
}

sub GetStratsPnl
{
  if ( exists $strat_to_results_ { $strat_name_ } ) {
    foreach my $result_line_ ( @ { $strat_to_results_ { $strat_name_ } } )
    {
      my @result_line_words_ = split ( ' ' , $result_line_ );
      my $date_ = $result_line_words_ [ 0 ];
      my $t_final_pnl_ = $result_line_words_ [ 1 ];
      my $t_min_pnl_ = $result_line_words_ [ 9 ];
      $t_min_pnl_ /= $strat_to_unit_trade_size_ { $strat_name_ }{ $date_ };
      $t_final_pnl_ /= $strat_to_unit_trade_size_ { $strat_name_ }{ $date_ };

      $strat_to_min_pnl_{ $strat_name_ }{ $date_ } = $t_min_pnl_;
      $strat_to_final_pnl_{ $strat_name_ }{ $date_ } = $t_final_pnl_;
    }
  }
}

sub GetUnitTradeSizesForStrats
{
  if ( exists $strat_to_results_ { $strat_name_ } ) {
    foreach my $result_line_ ( @{ $strat_to_results_ { $strat_name_ } } )
    {
      my @result_line_words_ = split ( ' ' , $result_line_ );
      my $date_ = $result_line_words_ [ 0 ];
      my $unit_trade_size_ = 1;

      my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$shortcode_." ".$strat_name_." ".$date_." 0 ".$use_notional_;
      $unit_trade_size_ = `$exec_cmd_`;

      chomp ( $unit_trade_size_ );
      if ( $use_notional_ ) {
        my @contract_specs_ = `$LIVE_BIN_DIR/get_contract_specs $shortcode_ $date_ ALL`; chomp ( @contract_specs_ );
        my ( $lotsize_, $close_px_, $contract_multiplier_ );
        foreach my $spec_line_ ( @contract_specs_ ) {
          my @swords_ = split(/[: ]+/, $spec_line_);
          given ( $swords_[0] ) {
            when ( "LOTSIZE" ) { $lotsize_ = $swords_[1]; }
            when ( "LAST_CLOSE_PRICE" ) { $close_px_ = $swords_[1]; }
            when ( "CONTRACT_MULTIPLIER" ) { $contract_multiplier_ = $swords_[1]; }
          }
        }
        my $denom_ = $lotsize_ * $close_px_ * $contract_multiplier_;

        $unit_trade_size_ = ceil( $unit_trade_size_ / $denom_ ) * $denom_;
      }
      $strat_to_unit_trade_size_ { $strat_name_ }{ $date_ } = $unit_trade_size_;
    }
  }
}

sub RunStdevOptimalMaxLossScript
{
  print "Using Stdev script:\n";
  my $maxloss_script_ = $MODELSCRIPTS_DIR."/find_optimal_stdev_max_loss.pl";

  %strat_to_lossno_ = ( );
  if ( exists $strat_to_final_pnl_ { $strat_name_ } ) {
    foreach my $date_ ( keys %{ $strat_to_final_pnl_{ $strat_name_ } } ) {
      my $exec_cmd_ = "$maxloss_script_ $shortcode_ EU 200 0.05 0.2 $strat_name_ $date_";
#      print $exec_cmd_."\n";
      my @optimal_lines_ = `$exec_cmd_ 2>/dev/null`; chomp ( @optimal_lines_ );

      foreach my $line_ ( @optimal_lines_ ) {
        if ( $line_ =~ /Max Loss for / ) {
          my @lwords_ = split( ' ', $line_ );
          $strat_to_lossno_{ $strat_name_ }{ $date_ } = $lwords_[ $#lwords_ ];
          $date_to_stdev_lossno_{ $date_ } = $lwords_[ $#lwords_ ];
        }
      }
    }
  }
  ComputeTotalPnl ( );
}

sub RunOptimalMaxLossScript
{
  print "Using Old script:\n";
  my $maxloss_script_ = $MODELSCRIPTS_DIR."/find_optimal_max_loss.pl";

  %strat_to_lossno_ = ( );
  if ( exists $strat_to_final_pnl_ { $strat_name_ } ) {
    foreach my $date_ ( keys %{ $strat_to_final_pnl_{ $strat_name_ } } ) {
      my $exec_cmd_ = "$maxloss_script_ $shortcode_ EU 100 0.05 10 $strat_name_ $date_";
#      print $exec_cmd_."\n";
      my $optimal_line_ = `$exec_cmd_ | head -3 | tail -1 2>/dev/null`; chomp ( $optimal_line_ );

      my @lwords_ = split( ' ', $optimal_line_ );
      if ( $#lwords_ > 1 ) {
        $strat_to_lossno_{ $strat_name_ }{ $date_ } = $lwords_[ 0 ];
        $date_to_normal_lossno_{ $date_ } = $lwords_[ 0 ];
      }
    }
  }
  ComputeTotalPnl ( );
}

sub PrintLosses
{
  print "\nDATE FINAL_PNL MIN_PNL OLD_LOSS STDEV_LOSS\n";
  foreach my $date_ ( keys %{ $strat_to_final_pnl_{ $strat_name_ } } ) {
    if ( exists $date_to_normal_lossno_{ $date_ } && $date_to_stdev_lossno_{ $date_ } ) {
      print "$date_ ".$strat_to_final_pnl_{ $strat_name_ }{ $date_ }." ".$strat_to_min_pnl_ { $strat_name_ }{ $date_ }." ".$date_to_normal_lossno_{ $date_ }." ".$date_to_stdev_lossno_{ $date_ }."\n";
    }
  }
}

sub ComputeTotalPnl
{
  my $sum_pnl_ = 0;
  my $sum_pnl_per_1kloss_ = 0;
  my $num_hits_ = 0;

  if ( exists $strat_to_lossno_{ $strat_name_ } ) {
    foreach my $date_ ( keys %{ $strat_to_lossno_{ $strat_name_ } } ) {
      if ( exists $strat_to_final_pnl_ { $strat_name_ }{ $date_ } ) {
        my $t_pnl_ = $strat_to_final_pnl_{ $strat_name_ }{ $date_ };

        if ( $strat_to_min_pnl_ { $strat_name_ }{ $date_ } < -1 * $strat_to_lossno_{ $strat_name_ }{ $date_ } ) {
          $num_hits_++;
          $t_pnl_ = -1 * $strat_to_lossno_{ $strat_name_ }{ $date_ };
        }
        $sum_pnl_ += $t_pnl_;
        $sum_pnl_per_1kloss_ += ( $t_pnl_ / $strat_to_lossno_{ $strat_name_ }{ $date_ } ) * 1000;
      }
    }
  }

  print "Sum_pnl: $sum_pnl_, Sum_pnl_per_1kloss: $sum_pnl_per_1kloss_, Num_hits_: $num_hits_\n";
}

