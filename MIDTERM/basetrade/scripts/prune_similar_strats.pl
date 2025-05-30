#!/usr/bin/perl

# Script to Prune lesser-performing similar Strats from Pool

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
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $GLOBALRESULTSDBDIR = "DB";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate 
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday 
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday 
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate 
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate 
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult 
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize 
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec 
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie 
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList 
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/make_strat_vec_from_list_matchbase.pl"; #MakeStratVecFromListMatchBase
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/strat_utils.pl"; # IsStagedStrat
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl"; # FetchCorrelationForPair
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl"; # GetPnlSamplesCorrelation
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl";

# Hrishav says
# I have removed the advanced support to consider the execlogic-similarity,trading_days_similarity for the similarity b/w strats.
# I am just considering the strats' pnlsamples' correlations for that.

my $USAGE = "$0 SHORTCODE TIMEPERIOD [DATE=TODAY-1] [NUM_DAYS=150] [THRESHOLD=0.7] [SortAlgo=kCNAPnlSharpeAverage] [Remove(0/1)] [S/N (staged/normal)]";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }

my $shortcode_ = shift;
my $timeperiod_ = shift;

my $yyyymmdd_ = "TODAY-1";
$yyyymmdd_ = shift if $#ARGV >= 0;
$yyyymmdd_ = GetIsoDateFromStrMin1($yyyymmdd_);

my $num_days_ = 150;
$num_days_ = shift if $#ARGV >= 0;

my $sim_threshold_ = 0.75;
$sim_threshold_ = shift if $#ARGV >= 0;

my $sort_algo_ = "kCNAPnlSharpeAverage";
$sort_algo_ = shift if $#ARGV >= 0;

my $to_remove_ = 0;
$to_remove_ = shift if $#ARGV >= 0;

my $staged_ = 'N';
$staged_ = 'S' if $#ARGV >= 0 && shift eq 'S';

my $min_sample_size_for_correlation_ = 60;

my $verbose_ = 1;
my $use_offline_ = 1;

my %strat_to_exec_logic_ = ( );
my %strat_to_param_file_ = ( );
my %strat_to_start_date_ = ( );
my %strat_to_end_date_ = ( );

# Fetch the list of dates for which we see the pnlsamples correlation (defualt: 150 days)
my @dates_vec_ = GetDatesFromNumDays ($shortcode_, $yyyymmdd_, $num_days_);
print "Dates: ".join(' ' , @dates_vec_ )."\n";

my %sample_pnls_strats_vec_;

# Fetch the configs for this pool
my @strats_base_ = `$HOME_DIR/basetrade/walkforward/get_pool_configs.py -m POOL -shc $shortcode_ -tp $timeperiod_ -type $staged_`;
chomp ( @strats_base_ );
if ( $#strats_base_ < 0 ) {
  print "No strats found to do analysis\n";
  exit(0);
}

my ($start_hhmm_, $end_hhmm_) = split("-", $timeperiod_);
#everything is initialized now

# Fetch pnlsamples for the pool-configs for the dates_vec_
FetchPnlSamplesStrats ( $shortcode_, \@strats_base_, \@dates_vec_, \%sample_pnls_strats_vec_, $start_hhmm_, $end_hhmm_ );

# Compute/Fetch the similarity_scores_ b/w all pairs of strats. 
my %similarity_scores_ = ();
FindSimilarStrats ( );

my $stratlist_file_ = GetCSTempFileName ( $HOME_DIR."/cstemp" ); 
open CSTF, "> $stratlist_file_" or PrintStacktraceAndDie ( "Could not open $stratlist_file_ for writing\n" );
print CSTF $_."\n" foreach @strats_base_;
close CSTF;

# sort the strats by their performance on the dates_vec_ 
# sort_algo is an optional cmdline argument (default: kCNAPnlSharpeAverage)
my %strats_scores_ = ( ) ;
my @strats_sorted_ = ( ) ;
GetScores ( );

# if any pair of strats has correlation > sim_threshold_ (default: 0.75), 
# remove the worse-performing strat (determined by sort-algo)
# Algorithm: we go down the list of strats (reverse-sorted by sort-algo)
# For a strat, If its correlation with any of the already selected strats > sim_threshold_,
# we skip that strat, Else we include it in the selected list
# strats_to_prune: the strats which aren't in selected_strats after the entire run
my @strats_to_prune_ = ( );
my @selected_strats_ = ( );
GetPrunedStrats ( );

# Prune the strats_to_prune, if to_remove_ is set
PruneBadStrats ( );

`rm -f $stratlist_file_`;

sub GetDays {
  my $date_ = shift;
  ( my $yyyy, my $mm, my $dd ) = BreakDateYYYYMMDD ( $date_ );
# Approximate!
  return $yyyy * 365 + $mm * 30 + $dd;
}


sub GetTrainingScore {
  my $s1 = shift;
  my $e1 = shift;
  my $s2 = shift;
  my $e2 = shift;
  if ( $s1 > $s2 ) { my $tmp = $s1; $s1 = $s2; $s2 = $tmp; $tmp = $e1; $e1 = $e2; $e2 = $tmp; }
  if ( $s2 > $e1 ) { return 0; }
  return ( $e1 - $s2 + 1 ) / ( max ( $e1 , $e2 ) - min ( $s1 , $s2 ) + 1 );
}


# Add your similarty logic here.
sub GetSimilarityScore 
{
  my @strat_names_ = @_;

  my $pnl_series_score_ = -1;
  if ( $use_offline_ == 1 ) {
    $pnl_series_score_ = FetchCorrelationForPair ( $strat_names_[0], $strat_names_[1] );
    if ( ! defined $pnl_series_score_ || $pnl_series_score_ == -1 ) {
      $pnl_series_score_ = GetPnlSamplesCorrelation ( $strat_names_ [0], $strat_names_ [1], \%sample_pnls_strats_vec_, $min_sample_size_for_correlation_ );
    }
  } else {
    $pnl_series_score_ = GetPnlSamplesCorrelation ( $strat_names_ [0] , $strat_names_ [1] , \%sample_pnls_strats_vec_, $min_sample_size_for_correlation_ );
  }

  return $pnl_series_score_;
}


sub FindSimilarStrats
{
  for ( my $stt_i_ = 0; $stt_i_ <= $#strats_base_; $stt_i_++ ) {
      for ( my $stt_j_ = $stt_i_ + 1; $stt_j_ <= $#strats_base_; $stt_j_++ ) {
        my $score_ = GetSimilarityScore ( $strats_base_[$stt_i_], $strats_base_[$stt_j_] );
        $similarity_scores_ { $strats_base_[$stt_i_]." ".$strats_base_[$stt_j_] } = $score_;
        $similarity_scores_ { $strats_base_[$stt_j_]." ".$strats_base_[$stt_i_] } = $score_;
      }
  }

  my $size_ = keys %similarity_scores_; $size_ /= 2;  
  print "No. of Total Pairs: ".$size_."\n";
}


sub GetScores
{
  my $trading_end_yyyymmdd_ = $yyyymmdd_;
  my $trading_start_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_end_yyyymmdd_, $num_days_ , $shortcode_ );
  my $exec_cmd_ = "$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $stratlist_file_ $GLOBALRESULTSDBDIR $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ INVALIDFILE $sort_algo_";
  print $exec_cmd_."\n";
  my @ssr_output_ = `$exec_cmd_`;
  chomp ( @ssr_output_ );
  foreach my $ssr_line_ ( @ssr_output_ ) {
    my @ssr_words_ = split (' ', $ssr_line_ );
    if ( FindItemFromVec( $ssr_words_[1], @strats_base_ ) ) {
        push( @strats_sorted_, $ssr_words_[1] );
    }
  }

  print "No. of Sorted Strats: ".$#strats_sorted_."\n";
}


# For each strat, find if it is similar to any of selected strats 
sub GetPrunedStrats {
  foreach my $strat_ ( @strats_sorted_ ) {
    my $t_sel_strat_ = "";

    foreach my $sel_strat_ ( @selected_strats_ ) {
      if ( $similarity_scores_ { $strat_." ".$sel_strat_ } > $sim_threshold_ )
      {  #if current strat is similar to any of the selected strat then just prune it and move on            
        $t_sel_strat_ = $sel_strat_;
        last;                                            
      }                  
    }

    if ( $t_sel_strat_ ne "" ) { 
      push(@strats_to_prune_, $strat_);   
      print "PRUNE: $strat_ because similarity with $t_sel_strat_ = ".( $similarity_scores_ { $strat_." ".$t_sel_strat_ } )."\n";
    }        
    else { 
      push(@selected_strats_, $strat_); 
    }
  } 
  print "Strats to Prune:\n".join("\n", @strats_to_prune_)."\n";
}


sub PruneBadStrats
{
  if ( $to_remove_ ) {
    print "Pruning Strats :\n";
    foreach my $strat_ ( @strats_to_prune_ ) {
      my $exec_cmd_ = "$HOME_DIR/walkforward/process_config.py -c $strat_ -m PRUNE";
      `$exec_cmd_`;
    }
  }
}

