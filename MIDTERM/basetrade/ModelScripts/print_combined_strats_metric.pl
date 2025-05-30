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
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub GetRunningQueries;
sub GetUnitTradeSizesForStrats;
sub GetGlobalResultsForRunningStrats;
sub ChooseMaxLoss;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $BASETRADE_BIN_DIR = $LIVE_BIN_DIR;
my $WF_DB_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward/wf_db_utils";
my $WF_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward";

# start
my $USAGE="$0 [SHORTCODE/ALL] STRAT_LIST [START_DATE / specific_dates_file] [END_DATE / -1] [skip_dates_file or INVALIDFILE] [DETAILED=0] [START_TIME] [END_TIME]";
if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/strat_utils.pl"; # CheckIfRegimeParam
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats, PnlSamplesGetStats, PnlSamplesGetStatsLong
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; 
require "$GENPERLLIB_DIR/config_utils.pl"; # IsValidConfig

my $shortcode_ = $ARGV[0];
my $strat_listname_ = $ARGV[1];
my $start_date_ = $ARGV[2];
my $end_date_ = $ARGV[3];

my $skip_dates_file_ = "INVALIDFILE";
if ( $#ARGV > 3 ) { $skip_dates_file_ = $ARGV [4]; }

my $detailed_ = 0;
if ( $#ARGV > 4 ) { $detailed_ = $ARGV [5]; }

my $start_hhmm_ = "0000";
if ( $#ARGV > 5 ) { $start_hhmm_ = $ARGV[6]; }
my $end_hhmm_ = "4800";
if ( $#ARGV > 6 ) { $end_hhmm_ = $ARGV[7]; }

my @dates_vec_ = GetDatesFromStartDate ( $shortcode_, $start_date_, $end_date_, $skip_dates_file_ );
@dates_vec_ = sort @dates_vec_;

my @strat_vec_ = ( );
my %strat_to_size_ = ( );
my %strat_to_shortcode_ = ( );
GetStrats ( );

my %strat_to_uts_ = ( );
GetUnitTradeSizesForStrats ( );

my %combined_pnl_series_ = ( );
GetCombinedPnlSamples ( );

print "\nCombined:\n";
if ( $detailed_ == 1 ) {
  PrintPnlStatsLong ( \%combined_pnl_series_ );
}
PrintPnlStats ( \%combined_pnl_series_ );

exit ( 0 );

sub GetStrats
{
  open STTLIST, "< $strat_listname_" or PrintStacktraceAndDie ( "Could not open $strat_listname_ for reading\n" );
  my @stratlines_ = <STTLIST>; chomp ( @stratlines_ );

  foreach my $query_uts_line_ ( @stratlines_ ) {
    my @qwords_ = split(" ", $query_uts_line_);
    if ( $#qwords_ < 0 ) { next; }
    push ( @strat_vec_, $qwords_[0] );

    if ( $#qwords_ > 0 ) {
      $strat_to_size_{ $qwords_[0] } = $qwords_[1];
    }
  }

  if ( $shortcode_ eq "ALL" ) {
    foreach my $strat_ ( @strat_vec_ ) {
      my ($spath_, $svalid_) = IsPoolStrat ( $strat_ );
      if ( ! defined $spath_ || $spath_ eq "" || $svalid_ == 0 ) {
        ($spath_, $svalid_) = IsStagedStrat ( $strat_ );
      }

      if ( ! defined $spath_ || $spath_ eq "" || $svalid_ == 0 ) {
        PrintStacktraceAndDie ( "Could not locate strategy $strat_\n" );
      }

      my $cmd = $WF_SCRIPTS."/print_strat_from_config.py -c $strat_ -d $end_date_ \| awk '{print \$2}' 2>/dev/null";
      my $tshc_ = `$cmd`; chomp ( $tshc_ );

      if ( $tshc_ eq "" ) {
        PrintStacktraceAndDie ( "Could not fetch shortcode for strategy $strat_\n" );
      }

      $strat_to_shortcode_{ $strat_ } = $tshc_;
    }
  }
}

sub GetUnitTradeSizesForStrats
{ 
  foreach my $strat_ ( @strat_vec_ )
  {
    if ( defined $strat_to_size_{ $strat_ } ) {
      my $is_structured_ = IsStructuredQuery ( $strat_ );
      my $tshc_ = ( $shortcode_ ne "ALL" ) ? $shortcode_ : $strat_to_shortcode_{ $strat_ };

      my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$tshc_." ".$strat_." ".$end_date_." ".$is_structured_;
      if(IsValidConfig($strat_))
      {
      	my $paramname = "INVALID";
      	my $cmd = $WF_SCRIPTS."/print_strat_from_config.py -c $strat_ -d $end_date_";
      	my $strategy_line = `$cmd`; chomp($cmd);
      	if ( $strategy_line ) {
      		my @strategy_line_words = split(' ', $strategy_line);
      		if ($#strategy_line_words >= 4){
      			$paramname = $strategy_line_words[4];
      		}
      	}
				#print "PARAM: $paramname $date_\n";
		$exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$tshc_." ".$paramname." ".$end_date_." 2 ";
      }
      $strat_to_uts_{ $strat_ } = `$exec_cmd_`; chomp ( $strat_to_uts_{ $strat_ } );
    }
  }
}

sub GetCombinedPnlSamples
{
  my %sample_pnls_strats_vec_ = ( );

  if ( $shortcode_ ne "ALL" ) {
  FetchPnlSamplesStrats ( $shortcode_, \@strat_vec_, \@dates_vec_, \%sample_pnls_strats_vec_ );
  }
  else {
    foreach my $strat_ ( @strat_vec_ ) {
      FetchPnlSamplesStrats ( $strat_to_shortcode_{ $strat_ }, [ $strat_ ], \@dates_vec_, \%sample_pnls_strats_vec_ );
    }
  }

  %combined_pnl_series_ = ( );
  foreach my $strat_ ( @strat_vec_ ) {
    my %t_pnl_series_ = ( );
    foreach my $t_sample_ ( keys %{ $sample_pnls_strats_vec_ { $strat_ } } ) {
      if ( ! defined $strat_to_size_{ $strat_ } ) {
        $t_pnl_series_{ $t_sample_ } = $sample_pnls_strats_vec_ { $strat_ }{ $t_sample_ };
      } else {
        $t_pnl_series_{ $t_sample_ } = $sample_pnls_strats_vec_ { $strat_ }{ $t_sample_ } * $strat_to_size_{ $strat_ } / $strat_to_uts_{ $strat_ };
      }
    }

    print "\nStrat: $strat_\n";
    if ( $detailed_ == 1 ) {
      PrintPnlStatsLong ( \%t_pnl_series_ );
    }
    PrintPnlStats ( \%t_pnl_series_ );

    if ( !%combined_pnl_series_ ) {
      %combined_pnl_series_ = %t_pnl_series_;
    } else {
      CombinePnlSamples ( \%combined_pnl_series_, \%t_pnl_series_, \%combined_pnl_series_ );
    }
  }
}

sub GetOverDaysDD
{
  my $pnlsamples_series_ref_ = shift;

  my %dates2pnl_ = ( );
  foreach my $t_sample_ ( keys %$pnlsamples_series_ref_ ) {
    my ( $t_date_, $t_slot_ ) = split ( "_", $t_sample_ );
    if ( ! defined $dates2pnl_{ $t_date_ } ) {
      $dates2pnl_{ $t_date_ } = 0;
    }
    $dates2pnl_{ $t_date_ } += $$pnlsamples_series_ref_{ $t_sample_ };
  }

  my @pnl_series_ = map { $dates2pnl_{ $_ } } sort keys %dates2pnl_;

  my $cumulative_pnl_ = 0;
  my $max_pnl_ = 0;
  my $drawdown_ = 0;

  foreach my $tpnl_ ( @pnl_series_ ) {
    $cumulative_pnl_ += $tpnl_;

    $max_pnl_ = max( $max_pnl_, $cumulative_pnl_ );
    $drawdown_ = max( $drawdown_, ($max_pnl_ - $cumulative_pnl_) );
  }
  return $drawdown_;
}

sub PrintPnlStats
{
  my $pnl_series_ref_ = shift;

  my %t_pnl_stats_map_ = ( );
  PnlSamplesGetStats ( $pnl_series_ref_, \%t_pnl_stats_map_, \@dates_vec_ );
  my $overdays_dd_ = GetOverDaysDD ( $pnl_series_ref_ ); 

  printf "STATISTICS %.2f sharpe: %.2f min: %d dd: %d gainpain: %.2f, over_days_dd: %.2f\n", $t_pnl_stats_map_{"PNL"}, $t_pnl_stats_map_{"DAILY_SHARPE"}, $t_pnl_stats_map_{"MINPNL"}, $t_pnl_stats_map_{"DD_HIGHAVG"}, $t_pnl_stats_map_{"GAIN_PAIN"}, $overdays_dd_; 
}

sub PrintPnlStatsLong
{
  my $pnl_series_ref_ = shift;

  my %t_pnl_stats_map_ = ( );
  PnlSamplesGetStatsLong ( $pnl_series_ref_, \%t_pnl_stats_map_, \@dates_vec_ );

  foreach my $idx_ ( 0..$#{ $t_pnl_stats_map_{"DATES"} } ) {
    printf "%d %d min: %d max: %d dd: %d\n", $t_pnl_stats_map_{"DATES"}[$idx_], $t_pnl_stats_map_{"PNL"}[$idx_], $t_pnl_stats_map_{"MINPNL"}[$idx_], $t_pnl_stats_map_{"MAXPNL"}[$idx_], $t_pnl_stats_map_{"DD"}[$idx_]; 
  }
}

sub IsStructuredQuery
{
  my $nm = $_[0];
  
  my $STRAT_FILE_PATH = $HOME_DIR."/modelling/stir_strats/".$strat_to_shortcode_{$nm}."/".$nm;
  if (-e $STRAT_FILE_PATH) { return $STRAT_FILE_PATH; }
  else {
    $STRAT_FILE_PATH = `ls $HOME_DIR"/modelling/stir_strats/*/*/"$nm 2>/dev/null`; chomp ( $STRAT_FILE_PATH ) ;
    if ( -e $STRAT_FILE_PATH ) { return $STRAT_FILE_PATH; }
    else { return ""; }
  }
}


