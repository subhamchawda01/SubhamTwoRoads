#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";

my $REPO = "basetrade";

my $CHOICEDIR = $HOME_DIR."/choices";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
my $MODELING_STRATS_DIR = $MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $MODELING_STAGED_STRATS_DIR = $MODELING_BASE_DIR."/staged_strats"; # this directory is used to store the chosen strategy files
my $TEMP_DIR="/spare/local/temp";
my $GLOBALRESULTSDBDIR = "DB";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie 
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList 
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; # GetUniqueList 
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/global_results_methods.pl"; # GetStratsWithGlobalResultsForShortcodeDate
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # FetchPnlSamplesStrats GetPnlSamplesCorrelation
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl";
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl";

sub GetCurrentPoolStrats
{
  my ($shc_, $strat_) = @_;

  my $stratpath_ = `$SCRIPTS_DIR/print_strat_from_base.sh $strat_ 2>/dev/null`; chomp ( $stratpath_ );
  return if $stratpath_ eq "";

  # main pool dir for the strategy
  my $pooldir_ = dirname ( $stratpath_ );
  $pooldir_ =~ s/staged_//g; 

  my $timeperiod_ = basename ( $pooldir_ );
  return if ( ! defined $timeperiod_ || $timeperiod_ eq "" );

  my @strat_paths_ = MakeStratVecFromDir( $pooldir_ );
  my @strats_vec_ = map { basename($_) } @strat_paths_;

  return @strats_vec_;
}

sub GetPerformanceDetails
{
  my ($shc_, $strat_, $stratlist_, $numdays_, $metrics_ref_, $skip_days_file_, $percentile_) = @_;
  $skip_days_file_ = "INVALIDFILE" if ! defined $skip_days_file_;
  $percentile_ = 0.3 if ! defined $percentile_;
  
  my $end_date_ = GetIsoDateFromStrMin1 ( "TODAY-1" );

  my %strat_resultlines_vec_ = ( );
  my $strat_summary_ = GetSummarizeResultsForStrat ( $shc_, $strat_, $end_date_, $numdays_, \%strat_resultlines_vec_, $skip_days_file_ );

  my %pool_resultlines_vec_ = ( );
  my %pool_summary_lines_ = ( );
  GetSummarizeResultsForStratDir ( $shc_, $stratlist_, $end_date_, $numdays_, \%pool_resultlines_vec_, \%pool_summary_lines_, $skip_days_file_ );

  my @strat_summary_words_ = split(/\s+/, $strat_summary_);
  if ( $#strat_summary_words_ < 14 ) {
    return "NoResultsForStrat";
  }

  $$metrics_ref_{ "STRAT_SHARPE" }  = $strat_summary_words_[4];
  $$metrics_ref_{ "STRAT_PNLSQRTDD" } = $strat_summary_words_[1] / sqrt($strat_summary_words_[14]);

  my @pool_sharpe_vec_ = ( );
  my @pool_pnl_sqrt_dd_ = ( );

  foreach my $tstrat_ ( keys %pool_summary_lines_ ) {
    next if ( ! defined $pool_resultlines_vec_{ $tstrat_ } );
    next if ( scalar keys %{ $pool_resultlines_vec_{ $tstrat_ } } < 0.6 * $numdays_ );

    @strat_summary_words_ = split(/\s+/, $pool_summary_lines_{ $tstrat_ });
    next if ( $#strat_summary_words_ < 14 );
    push ( @pool_sharpe_vec_, $strat_summary_words_[4] );
    push ( @pool_pnl_sqrt_dd_, $strat_summary_words_[1] / sqrt($strat_summary_words_[14]) );
  }

  if ( $#pool_sharpe_vec_ < 4 ) {
   return "NotEnoughStratsInPool";
  }

  @pool_sharpe_vec_ = sort { $a <=> $b } @pool_sharpe_vec_;
  @pool_pnl_sqrt_dd_ = sort { $a <=> $b } @pool_pnl_sqrt_dd_;
  my $cutoff_idx_ = 0.3 * ($#pool_sharpe_vec_ + 1);

  $$metrics_ref_{ "POOL_SHARPE" } = $pool_sharpe_vec_[ $cutoff_idx_ ];
  $$metrics_ref_{ "POOL_PNLSQRTDD" } = $pool_pnl_sqrt_dd_[ $cutoff_idx_ ];

  return "";
}

sub GetSimilarStrat
{
  if ( @_ < 2 ) { return; }

  my ($strat_, $correlated_strat_ref_, $corr_threshold_ ) = @_;
  $corr_threshold_ = 0.85 if (! defined $corr_threshold_ || $corr_threshold_ < 0 || $corr_threshold_ > 1); 
  FetchCorrelatedStrats ( $strat_, $correlated_strat_ref_ );

  foreach my $tstrat_ ( keys %$correlated_strat_ref_ ) {
    delete $$correlated_strat_ref_{ $tstrat_ } if $$correlated_strat_ref_{ $tstrat_ } < $corr_threshold_;
  }
}

sub GetVolumeCheck
{
  if ( @_ < 2 ) { return; }
  my ($shc_, $strat_) = @_;

  my $end_date_ = GetIsoDateFromStrMin1 ( "TODAY-1" );
  my $numdays_ = 40;

  my %strat_resultlines_vec_ = ( );
  my $strat_summary_ = GetSummarizeResultsForStrat ( $shc_, $strat_, $end_date_, $numdays_, \%strat_resultlines_vec_ );
  my $avg_volume_ = (split(/\s+/, $strat_summary_))[3];

  my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day $shc_ $strat_ $end_date_";
  my $unit_trade_size_ = `$exec_cmd_`;

  my $start_hhmm_ = `cat /home/dvctrader/modelling/\*strats/$shc_/\*/$strat_ | head -1 | cut -d' ' -f6`; chomp ( $start_hhmm_ );
  my $end_hhmm_ = `cat /home/dvctrader/modelling/\*strats/$shc_/\*/$strat_ | head -1 | cut -d' ' -f7`; chomp ( $end_hhmm_ );

  my @dates_vec_ = GetDatesFromNumDays ( $shc_, $end_date_, $numdays_ );

  my @l1sz_vec_ = ( );
  my @volume_vec_ = ( );

  foreach my $tdate_ ( @dates_vec_ ) {
    my ($l1sz_, $is_valid1_) = GetFeatureAverage ( $shc_, $end_date_, "L1SZ", undef, $start_hhmm_, $end_hhmm_ );
    push ( @l1sz_vec_, $l1sz_ ) if $is_valid1_;

    my ($vol_, $is_valid2_) = GetFeatureSum ( $shc_, $end_date_, "VOL", undef, $start_hhmm_, $end_hhmm_ );
    push ( @volume_vec_, $vol_ ) if $is_valid2_;
  }

  my $l1sz_ = GetAverage ( \@l1sz_vec_ );
  my $prod_volume_ = 3 * GetAverage ( \@volume_vec_ );

  my $rescaled_uts_ = 0.2 * $l1sz_;
  my $rescaled_volume_ = $avg_volume_ * $rescaled_uts_ / $unit_trade_size_;

  return ( $rescaled_volume_, $prod_volume_ );
} 

1;
