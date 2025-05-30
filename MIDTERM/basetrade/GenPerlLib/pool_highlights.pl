#!/usr/bin/perl
#
# It provides functions for Pool Performance, Similarity-Metrics and other highlights
# Note: For fetching pool-configs we use: walkforward/get_pool_configs.py
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use Math::Complex; # sqrt
use FileHandle;
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
my $MODELING_STRATS_DIR = $MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie 
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList 
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/make_strat_vec_from_list_matchbase.pl"; #MakeStratVecFromListMatchBase
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; 
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # FetchPnlSamples
require "$GENPERLLIB_DIR/global_results_methods.pl"; 
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFilteredSamples
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl"; 

# Returns Pool Performance for different regimes
# Regimes: High/Low periods (SAMPLE-wise) of VOL, STDEV, L1SZ, SSTREND
# Pool Performance: Sharpe of 70%ile strat for that regime
# It classifies 15min-SAMPLES into diff regimes
# Note: It is supposed to be used in Pool Highlights Page
#  
sub RegimePoolPerformanceSamples {
  my $USAGE = "$0 SHORTCODE STRAT_DIR/STRAT_LIST MAPREFERENCE";

  my $shortcode_ = shift;
  my $timeperiod_ = shift;
  my $pool_regime_performance_ref_ = shift;

  # Fetch the last 200 days
  my $end_date_ = GetIsoDateFromStrMin1("TODAY-1");
  my $num_days_ = 200;
  my @dates_vec_ = GetDatesFromNumDays( $shortcode_, $end_date_, $num_days_ );

  # define regime factors
  my @factor_vec_ = ("VOL", "STDEV", "L1SZ", "SSTREND");
  my $frac_ = 0.5;
  my @high_low_ = ("HIGH", "LOW");
 
  # Fetch the Pool wfconfigs
  my $fetch_configs_cmd_ = "$HOME_DIR/basetrade/walkforward/get_pool_configs.py -m POOL -shc $shortcode_ -tp $timeperiod_ -type N";
  my @strats_base_ = `$fetch_configs_cmd_ 2>/dev/null`; chomp ( @strats_base_ );
  return if $#strats_base_ < 0;

  my ($start_hhmm_, $end_hhmm_) = split("-", $timeperiod_);
  
  # Get the PnlSamples for all the wfconfigs
  my %sample_pnls_strats_vec_ = ();
  FetchPnlSamplesStrats ( $shortcode_, \@strats_base_, \@dates_vec_, \%sample_pnls_strats_vec_, $start_hhmm_, $end_hhmm_ );
  
  foreach my $factor_ (@factor_vec_) {
    foreach my $regime_ (@high_low_) {
      my @filtered_samples_ = ();
      my %filtered_samples_exists_ = ();
     
      # Get the 15min samples for current regime
      GetFilteredSamples ( $shortcode_, \@dates_vec_, $frac_, $regime_, $factor_, [], \@filtered_samples_, $start_hhmm_, $end_hhmm_ );
      %filtered_samples_exists_ = map { $_ => 1 } @filtered_samples_;
      
      my %strat_avg_pnl_;
      my %strat_sharpe_;
      for my $t_strat_ ( keys %sample_pnls_strats_vec_ ) {
        my @filtered_slots_ = grep { exists $filtered_samples_exists_{ ConvertKeyToFormat1( $_ ) } } keys %{ $sample_pnls_strats_vec_{ $t_strat_ } };
        my @pnl_vals_ = @{ $sample_pnls_strats_vec_{ $t_strat_ } }{ @filtered_slots_ };
        $strat_avg_pnl_ { $t_strat_ } = GetAverage ( \@pnl_vals_ ); 
        $strat_sharpe_ { $t_strat_ } = ( GetAverage ( \@pnl_vals_ ) / GetStdev ( \@pnl_vals_ ) ) * sqrt( $#filtered_samples_ / $#dates_vec_ ) ;
      }
      # record the 60%ile strat's sharpe as metric for each regime-performance
      my @strats_sorted_ = sort { $strat_sharpe_{$a} <=> $strat_sharpe_{$b} } keys %strat_sharpe_;
      $$pool_regime_performance_ref_{ $factor_ }{ $regime_ } = $strat_sharpe_{ $strats_sorted_[ ceil( $#strats_sorted_ * 0.7 )] };;
    }
  }
}



# Returns Pool Performance for different regimes
# Regimes: High/Low periods (DAY-wise) of VOL, STDEV, L1SZ, SSTREND
# Pool Performance: Sharpe of 70%ile strat for that regime
# It classifies DAYS into diff regimes
# Note: It is supposed to be used in Pool Highlights Page
sub RegimePoolPerformanceDays {
  my $USAGE = "$0 SHORTCODE STRAT_DIR/STRAT_LIST MAPREFERENCE";

  my $shortcode_ = shift;
  my $timeperiod_ = shift;
  my $pool_regime_performance_ref_ = shift;

  # Fetch the last 200 days
  my $end_date_ = GetIsoDateFromStrMin1("TODAY-1");
  my $num_days_ = 200;
  my @dates_vec_ = GetDatesFromNumDays( $shortcode_, $end_date_, $num_days_ ); 

  # Fetch the Pool wfconfigs 
  my $fetch_configs_cmd_ = "$HOME_DIR/basetrade/walkforward/get_pool_configs.py -m POOL -shc $shortcode_ -tp $timeperiod_ -type N";
  my @strats_base_ = `$fetch_configs_cmd_ 2>/dev/null`; chomp ( @strats_base_ );
  return if $#strats_base_ < 0;

  my $cstempfile_ = GetCSTempFileName ( "/spare/local/temp/" );
  open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
  print CSTF $_."\n" foreach @strats_base_;
  close CSTF;

  # Fetch the Pnls for all the wfconfigs
  my %strats_to_results_map_ = ();
  my %strat_to_summary_ = ();
  GetSummarizeResultsForStratDir ( $shortcode_, $cstempfile_, $end_date_, $num_days_, \%strats_to_results_map_, \%strat_to_summary_, "INVALIDFILE" );
  my %strats_to_pnls_map_ = ();

  foreach my $t_strat_ ( keys %strats_to_results_map_ ) {
    %{ $strats_to_pnls_map_{ $t_strat_ } } = map { $_ => (split(/\s+/, $strats_to_results_map_{$t_strat_}{$_} ))[1] } keys %{ $strats_to_results_map_{ $t_strat_ } };
  }

  my ($start_hhmm_, $end_hhmm_) = split("-", $timeperiod_);

  # define regime factors
  my @factor_vec_ = ("VOL", "STDEV", "L1SZ", "SSTREND");
  my $frac_ = 0.5;
  my @high_low_ = ("HIGH", "LOW");

  foreach my $factor_ (@factor_vec_) {
    foreach my $regime_ (@high_low_) {
      my @filtered_samples_ = ();
      my %filtered_samples_exists_ = ();

      # Get the days for the current regime
      GetFilteredDays ( $shortcode_, \@dates_vec_, $frac_, $regime_, $factor_, [], \@filtered_samples_, $start_hhmm_, $end_hhmm_ );
      %filtered_samples_exists_ = map { $_ => 1 } @filtered_samples_;
      
      my %strat_avg_pnl_;
      my %strat_sharpe_;
      for my $t_strat_ ( keys %strats_to_pnls_map_ ) {
        my @filtered_slots_ = grep { exists $filtered_samples_exists_{ $_ } } keys %{ $strats_to_pnls_map_{ $t_strat_ } };
        my @pnl_vals_ = @{ $strats_to_pnls_map_{ $t_strat_ } }{ @filtered_slots_ };
        $strat_sharpe_ { $t_strat_ } = GetAverage ( \@pnl_vals_ ) / GetStdev ( \@pnl_vals_ ) ;
      }
      # record the 60%ile strat's sharpe as metric for each regime-performance
      my @strats_sorted_ = sort { $strat_sharpe_{$a} <=> $strat_sharpe_{$b} } keys %strat_sharpe_;
      $$pool_regime_performance_ref_{ $factor_ }{ $regime_ } = $strat_sharpe_{ $strats_sorted_[ ceil( $#strats_sorted_ * 0.7 )] };;
    }
  }
}

# Returns the Pool Correlations (series of nC2 correlation values) 
# Note: It is supposed to be used in Pool Highlights Page
# In the page: we display different percentiles of cerrelation series (for e.g.: 20%, 50% 70%, 80%)
#
sub PoolCorrelationSeries {
  my $USAGE = "$0 SHORTCODE STRAT_DIR/STRAT_LIST MAPREFERENCE";

  my $shortcode_ = shift;
  my $timeperiod_ = shift;
  my $pool_correlation_series_arr_ref_ = shift();
  my %pool_correlation_series_map_ = ();

  my $fetch_configs_cmd_ = "$HOME_DIR/walkforward/get_pool_configs.py -m POOL -shc $shortcode_ -tp $timeperiod_ -type N";
  my @strats_base_ = `$fetch_configs_cmd_ 2>/dev/null`; chomp ( @strats_base_ );

  FetchAllCorrelationForStrats(\@strats_base_, \%pool_correlation_series_map_);
  @$pool_correlation_series_arr_ref_ = values %pool_correlation_series_map_;
}
1
