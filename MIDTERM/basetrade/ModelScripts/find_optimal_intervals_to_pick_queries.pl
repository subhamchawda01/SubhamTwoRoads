#!/usr/bin/perl

# \file ModelScripts/find_optimal_intervals_to_pick_queries.pl
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
use Math::Complex; # sqrt
use FileHandle;

sub LoadConfigFile;
sub SanityCheckConfigParams;

sub GetGlobalResults;

sub ScoreStrats;

sub SortResultsByAlgo;

sub PickStrats;

sub AddPickResultsToLocalDatabase;
sub WriteResultsToLocalDatabase;

sub SummarizeLocalResultsAndChoose;

sub RemoveIntermediateFiles;

sub IsTooInsample;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";

my $FOITPQ_WORK_DIR = $SPARE_HOME."FOITPQ/";

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELLING_STRATS_DIR=$HOME_DIR."/modelling/strats";
my $MODELLING_PRUNED_STRATS_DIR=$HOME_DIR."/modelling/pruned_strategies";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
  $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

$LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

my @intermediate_files_ = ( );

require "$GENPERLLIB_DIR/sqrt_sign.pl"; # SqrtSign
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

require "$GENPERLLIB_DIR/global_results_methods.pl"; # GetGlobalResultsForShortcodeDate
require "$GENPERLLIB_DIR/get_strat_type.pl"; #GetStratType

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD CONFIGFILE START_DATE END_DATE";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];

my $config_file_ = $ARGV [ 2 ];

my $start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 3 ] );
my $end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 4 ] );

my $num_strats_to_install_ = 0;
my $min_volume_per_strat_ = 0;
my @volume_cut_off_ratio_list_ = ( );

my @strats_to_keep_ = ( );
my @strats_to_exclude_ = ( );

my $intervals_to_compare_ = 0;
my @all_intervals_ = ( );
my %intervals_to_pick_from_ = ( );

my $diversities_to_compare_ = 0;
my %diversity_type_to_weight_ = ( );

my @sort_algo_list_ = ( );

my $summarization_algo_ = "";

my $unique_strategy_id_ = 1;
my %permutation_name_to_strat_id_ = ( );

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $FOITPQ_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
  if ( -d $work_dir_ )
  {
    print STDERR "Surprising but this dir exists\n";
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
    $work_dir_ = $FOITPQ_WORK_DIR.$unique_gsm_id_; 
  }
  else
  {
    last;
  }
}

my $local_results_base_dir_ = $work_dir_."/local_results_base_dir";
my $temp_results_base_dir_ = $work_dir_."/temp_results_base_dir";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;

if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir_ ) ) { `mkdir -p $local_results_base_dir_`; }
if ( ! ( -d $temp_results_base_dir_ ) ) { `mkdir -p $temp_results_base_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush ( 1 );

LoadConfigFile ( $config_file_ );
SanityCheckConfigParams ( );

my @all_strats_ = ( );
my %interval_to_real_results_ = ( );
my %interval_to_global_results_ = ( );
my %strat_name_to_global_result_score_ = ( );

my %strat_name_to_diversity_adjusted_score_ = ( );
my %strat_name_to_is_diversity_adjusted_ = ( );

my @algo_sorted_strat_names_ = ( );
my @algo_sorted_strat_results_ = ( );
my @algo_sorted_strat_scores_ = ( );

my @picked_strats_ = ( );

my %date_to_permutation_to_results_ = ( );
my @unique_results_filevec_ = ( );

for ( my $current_date_ = $end_yyyymmdd_ ; $current_date_ >= $start_yyyymmdd_ ; )
{ # Foreach day ,
  for ( my $interval_to_compare_ = 0 ; $interval_to_compare_ < $intervals_to_compare_ ; $interval_to_compare_ ++ )
  { # For each interval_weight_set ,
    foreach my $sort_algo_ ( @sort_algo_list_ )
    {
      for ( my $volume_cut_off_index_ = 0 ; $volume_cut_off_index_ <= $#volume_cut_off_ratio_list_ ; $volume_cut_off_index_ ++ )
      {
        @all_strats_ = ( );

        %interval_to_global_results_ = ( );
        GetGlobalResults ( $current_date_ , $interval_to_compare_ , $volume_cut_off_index_ );

        for ( my $diversity_to_compare_ = 0 ; $diversity_to_compare_ < $diversities_to_compare_ ; $diversity_to_compare_ ++ )
        { # For each diversity_weight set ,

          %strat_name_to_global_result_score_ = ( );

          %strat_name_to_diversity_adjusted_score_ = ( );
          %strat_name_to_is_diversity_adjusted_ = ( );

          @algo_sorted_strat_names_ = ( );
          @algo_sorted_strat_results_ = ( );
          ScoreStrats ( $sort_algo_ , $interval_to_compare_ );

          @picked_strats_ = ( );
          PickStrats ( $diversity_to_compare_ );

          AddPickResultsToLocalDatabase ( $current_date_ , $interval_to_compare_ , $diversity_to_compare_ , $sort_algo_ );
        }
      }
    }
  }

  $current_date_ = CalcPrevWorkingDateMult ( $current_date_ , 1 );
}

WriteResultsToLocalDatabase ( );

SummarizeLocalResultsAndChoose ( );

RemoveIntermediateFiles ( );

$main_log_file_handle_->close;

exit ( 0 );

sub LoadConfigFile
{
  my ( $t_config_file_ ) = @_;

  print $main_log_file_handle_ " # LoadConfigFile ( $t_config_file_ )\n";

  open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );

  print $main_log_file_handle_ " > CONFIG_FILE=".$t_config_file_."\n";

  my $current_param_ = "";
  foreach my $config_file_lines_ ( @config_file_lines_ )
  {
    if ( index ( $config_file_lines_ , "#" ) >= 0 )
    {
      next;
    }

    my @t_words_ = split ( ' ' , $config_file_lines_ );

    if ( $#t_words_ < 0 )
    {
      $current_param_ = "";
      next;
    }

    if ( ! $current_param_ )
    {
      $current_param_ = $t_words_ [ 0 ];
      next;
    }
    else
    {
      given ( $current_param_ )
      {
        when ( "SHORTCODE" )
        {
          my $t_shortcode_ = $t_words_ [ 0 ];
          if ( $t_shortcode_ ne $shortcode_ )
          {
            PrintStacktraceAndDie ( "$t_shortcode_ in config file != $shortcode_" );
          }
          print $main_log_file_handle_ " \t > SHORTCODE=".$t_shortcode_."\n";
        }

        when ( "NUM_STRATS_TO_INSTALL" )
        {
          $num_strats_to_install_ = $t_words_ [ 0 ];
          print $main_log_file_handle_ " \t > NUM_STRATS_TO_INSTALL=".$num_strats_to_install_."\n";
        }

        when ( "MIN_VOLUME_PER_STRAT" )
        {
          $min_volume_per_strat_ = $t_words_ [ 0 ];
          print $main_log_file_handle_ " \t > MIN_VOLUME_PER_STRAT=".$min_volume_per_strat_."\n";
        }

        when ( "INTERVALS_TO_PICK_FROM" )
        {
          print $main_log_file_handle_ " \t > INTERVALS_TO_PICK_FROM=";
          for ( my $i = 0 ; ( $i + 1 ) <= $#t_words_ ; $i += 2 )
          {
            $intervals_to_pick_from_ { $intervals_to_compare_ } { $t_words_ [ $i ] } = $t_words_ [ $i + 1 ];
            print $main_log_file_handle_ "[".$t_words_ [ $i ]."]=".$t_words_ [ $i + 1 ]." ";

            if ( ! FindItemFromVec ( $t_words_ [ $i ] , @all_intervals_ ) )
            {
              push ( @all_intervals_ , $t_words_ [ $i ] );
            }
          }

          print $main_log_file_handle_ "\n";

          $intervals_to_compare_ ++;
        }

        when ( "DIVERSITY_SCORES" )
        {
          print $main_log_file_handle_ " \t > DIVERSITY_SCORES=";
          for ( my $i = 0 ; ( $i + 1 ) <= $#t_words_ ; $i += 2 )
          {
            $diversity_type_to_weight_ { $diversities_to_compare_ } { $t_words_ [ $i ] } = $t_words_ [ $i + 1 ];
            print $main_log_file_handle_ "[".$t_words_ [ $i ]."]=".$t_words_ [ $i + 1 ]." ";
          }

          print $main_log_file_handle_ "\n";

          $diversities_to_compare_ ++;
        }

        when ( "VOLUME_CUT_OFF_RATIO" )
        {
          my $t_volume_cut_off_ratio_ = $t_words_[ 0 ];

          push ( @volume_cut_off_ratio_list_ , $t_volume_cut_off_ratio_ );

          print $main_log_file_handle_ " \t > VOLUME_CUT_OFF_RATIO=".$t_volume_cut_off_ratio_."\n";
        }

        when ( "SORT_ALGO" )
        {
          my $sort_algo_ = $t_words_ [ 0 ];

          push ( @sort_algo_list_ , $sort_algo_ );

          print $main_log_file_handle_ " \t > SORT_ALGO=".$sort_algo_."\n";
        }

        when ( "SUMMARIZATION_ALGO" )
        {
          $summarization_algo_ = $t_words_ [ 0 ];

          print $main_log_file_handle_ " \t > SUMMARIZATION_ALGO=".$summarization_algo_."\n";
        }
      }
    }
  }

  print $main_log_file_handle_ "\n\n";

  return;
}

sub SanityCheckConfigParams
{
  print $main_log_file_handle_ " # SanityCheckConfigParams\n";

  if ( $num_strats_to_install_ <= 0 )
  {
    PrintStacktraceAndDie ( "NUM_STRATS_TO_INSTALL=".$num_strats_to_install_ );
  }

  if ( ! $summarization_algo_ )
  {
    PrintStacktraceAndDie ( "Invalid SUMMARIZATION_ALGO '".$summarization_algo_."'" );
  }

# Normalize interval weights
  foreach my $interval_index_ ( sort { $a <=> $b }
                               keys %intervals_to_pick_from_ )
  {
    my $sum_interval_weights_ = 0;

    foreach my $interval_duration_ ( sort { $a <=> $b }
                                    keys % { $intervals_to_pick_from_ { $interval_index_ } } )
    {
      $sum_interval_weights_ += $intervals_to_pick_from_ { $interval_index_ } { $interval_duration_ };
    }

    foreach my $interval_duration_ ( sort { $a <=> $b }
                                    keys % { $intervals_to_pick_from_ { $interval_index_ } } )
    {
      $intervals_to_pick_from_ { $interval_index_ } { $interval_duration_ } /= $sum_interval_weights_;
    }
  }

# Normalize diversity scores
  foreach my $diversity_index_ ( sort { $a <=> $b }
                                keys %diversity_type_to_weight_ )
  {
    my $sum_diversity_weights_ = 0;

    foreach my $diversity_type_ ( keys % { $diversity_type_to_weight_ { $diversity_index_ } } )
    {
      $sum_diversity_weights_ += $diversity_type_to_weight_ { $diversity_index_ } { $diversity_type_ };
    }

    foreach my $diversity_type_ ( keys % { $diversity_type_to_weight_ { $diversity_index_ } } )
    {
      $diversity_type_to_weight_ { $diversity_index_ } { $diversity_type_ } /= $sum_diversity_weights_;
    }
  }

  return;
}

sub GetGlobalResults
{
  my ( $current_date_ , $interval_index_ , $volume_cutoff_index_ ) = @_;

  print $main_log_file_handle_ " # GetGlobalResults ( $current_date_ , $interval_index_ )\n";

  my $SUMMARIZE_LOCAL_RESULTS_BY_TIMEPERIOD = $LIVE_BIN_DIR."/summarize_local_results_by_timeperiod";

  foreach my $interval_ ( sort { $a <=> $b }
                         keys % { $intervals_to_pick_from_ { $interval_index_ } } )
  {
    my $current_start_date_ = CalcPrevWorkingDateMult ($current_date_, $interval_);
    my $exec_cmd_ = $SUMMARIZE_LOCAL_RESULTS_BY_TIMEPERIOD." $timeperiod_ $current_start_date_ $current_date_ ".$HOME_DIR."/ec2_globalresults/".$shortcode_ ;
    print $main_log_file_handle_ "$exec_cmd_\n";

    my @ss_noc_results_ = `$exec_cmd_`;

    my $volume_cut_off_ = 0;
    foreach my $ss_noc_result_line_ ( @ss_noc_results_ )
    {
      my @ss_noc_result_words_ = split ( ' ' , $ss_noc_result_line_ );
      if( $ss_noc_result_words_[ 1 ] > $volume_cut_off_ )
      {
        $volume_cut_off_ = $ss_noc_result_words_[ 1 ];
      }
    }

    $volume_cut_off_ *=  $volume_cut_off_ratio_list_ [ $volume_cutoff_index_ ];

    foreach my $ss_noc_result_line_ ( @ss_noc_results_ )
    {
# -84 644 -0.08 w_hv_ilist_CGB_0_US_Mkt_Mkt_J0_8_na_t3_20111226_20120220_EST_820_EST_1455_500_2_1_fst.5_FSLR_0.01_0_0_0.75.tt_EST_820_EST_1455.pfi_7

      my @ss_noc_result_words_ = split ( ' ' , $ss_noc_result_line_ );

      if ( $ss_noc_result_words_ [ 1 ] < $volume_cut_off_ )
      {
        next;
      }

      if ( ! $volume_cut_off_ && ( $ss_noc_result_words_ [ 1 ] < $min_volume_per_strat_ ) )
      {
        next;
      }

      if ( $ss_noc_result_words_ [ 6 ] <= 0 )
      {
        print $main_log_file_handle_ "ERROR ttc<=0 $exec_cmd_\n";
        $ss_noc_result_words_ [ 6 ] = 100000;
      }

      my $t_ss_noc_result_ = sprintf ( "%d %d %0.2f %d %0.2f %d" , $ss_noc_result_words_ [ 0 ] , $ss_noc_result_words_ [ 1 ] , $ss_noc_result_words_ [ 2 ] , $ss_noc_result_words_ [ 4 ] , $ss_noc_result_words_ [ 5 ] , $ss_noc_result_words_ [ 6 ] );
      my $t_strat_name_ = $ss_noc_result_words_ [ 3 ];

      if ( IsTooInsample ( $t_strat_name_ , $current_date_ , $interval_index_ ) )
      { # Use the weights to see if this strat is "too" insample for the periods.
        print $main_log_file_handle_ " Strat $t_strat_name_ IsTooInsample. Skipped.\n";
        next;
      }

      $interval_to_global_results_ { $interval_ } { $t_strat_name_ } = $t_ss_noc_result_;

      if ( ! FindItemFromVec ( $t_strat_name_ , @all_strats_ ) )
      {
        push ( @all_strats_ , $t_strat_name_ );
      }
    }
    print $main_log_file_handle_ " fetched all results for $exec_cmd_\n";
  }
}

sub ScoreStrats
{
  my ( $sort_algo_ , $interval_index_ ) = @_;

  print $main_log_file_handle_ " # ScoreStrats ( $sort_algo_ , $interval_index_ )\n\n";

# Remove strats that did not have results in all intervals.
  my %strat_name_to_result_count_ = ( );

  foreach my $interval_ ( sort { $a <=> $b }
                         keys % { $intervals_to_pick_from_ { $interval_index_ } } )
  {
    foreach my $strat_name_ ( @all_strats_ )
    {
      if ( ! exists ( $strat_name_to_result_count_ { $strat_name_ } ) )
      {
        $strat_name_to_result_count_ { $strat_name_ } = 0;
      }

      if ( exists ( $interval_to_global_results_ { $interval_ } { $strat_name_ } ) )
      {
        $strat_name_to_result_count_ { $strat_name_ } ++;
      }
    }
  }

  @all_strats_ = ( );
  foreach my $strat_name_ ( keys %strat_name_to_result_count_ )
  {
    if ( $strat_name_to_result_count_ { $strat_name_ } == keys % { $intervals_to_pick_from_ { $interval_index_ } } )
    {
      if ( ! FindItemFromVec ( $strat_name_ , @all_strats_ ) )
      {
        push ( @all_strats_ , $strat_name_ );
      }
    }
  }

  foreach my $interval_ ( sort { $a <=> $b }
                         keys % { $intervals_to_pick_from_ { $interval_index_ } } )
  {
    foreach my $strat_name_ ( @all_strats_ )
    {
      if ( exists ( $interval_to_global_results_ { $interval_ } { $strat_name_ } ) )
      {
        my $t_global_result_ = $interval_to_global_results_ { $interval_ } { $strat_name_ };

        my @global_result_words_ = split ( ' ' , $t_global_result_ );

        my $t_weighted_pnl_ = $global_result_words_ [ 0 ] * $intervals_to_pick_from_ { $interval_index_ } { $interval_ };
        my $t_weighted_vol_ = $global_result_words_ [ 1 ] * $intervals_to_pick_from_ { $interval_index_ } { $interval_ };
        my $t_weighted_dd_ = $global_result_words_ [ 2 ] * $intervals_to_pick_from_ { $interval_index_ } { $interval_ };
        my $t_weighted_pnl_stdev_ = $global_result_words_ [ 3 ] * $intervals_to_pick_from_ { $interval_index_ } { $interval_ };
        my $t_weighted_pnl_sharpe_ = $global_result_words_ [ 4 ] * $intervals_to_pick_from_ { $interval_index_ } { $interval_ };
        my $t_weighted_ttc_ = $global_result_words_ [ 5 ] * $intervals_to_pick_from_ { $interval_index_ } { $interval_ };

        my $t_existing_result_line_ = "0 0 0 0 0 0";
        if ( exists ( $strat_name_to_global_result_score_ { $strat_name_ } ) )
        {
          $t_existing_result_line_ = $strat_name_to_global_result_score_ { $strat_name_ };
        }

# Combine this result line with the existing result line for this strat.
        my @t_existing_result_words_ = split ( ' ' , $t_existing_result_line_ );
        $t_weighted_pnl_ += $t_existing_result_words_ [ 0 ];
        $t_weighted_vol_ += $t_existing_result_words_ [ 1 ];
        $t_weighted_dd_ += $t_existing_result_words_ [ 2 ];
        $t_weighted_pnl_stdev_ += $t_existing_result_words_ [ 3 ];
        $t_weighted_pnl_sharpe_ += $t_existing_result_words_ [ 4 ];
        $t_weighted_ttc_ += $t_existing_result_words_ [ 5 ];

        $strat_name_to_global_result_score_ { $strat_name_ } = sprintf ( "%d %d %0.2f %d %0.2f %d" , $t_weighted_pnl_ , $t_weighted_vol_ , $t_weighted_dd_ , $t_weighted_pnl_stdev_ , $t_weighted_pnl_sharpe_ , $t_weighted_ttc_ );
      }
    }
  }

  foreach my $strat_name_ ( @all_strats_ )
  {
    if ( exists ( $strat_name_to_global_result_score_ { $strat_name_ } ) )
    {
      push ( @algo_sorted_strat_names_ , $strat_name_ );
      push ( @algo_sorted_strat_results_ , $strat_name_to_global_result_score_ { $strat_name_ } );
    }
  }

  SortResultsByAlgo ( $sort_algo_ );

  return;
}

sub SortResultsByAlgo
{
  my ( $sort_algo_ ) = @_;
  print $main_log_file_handle_ " # SortResultsByAlgo ( $sort_algo_ )\n";

  my %strat_name_to_algo_sorted_score_ = ( );

  for ( my $i = 0 ; $i <= $#algo_sorted_strat_names_ ; $i ++ )
  {
    my $t_strat_name_ = $algo_sorted_strat_names_ [ $i ];
    my $t_result_line_ = $algo_sorted_strat_results_ [ $i ];

    my @t_result_words_ = split ( ' ' , $t_result_line_ );

    my $t_pnl_ = $t_result_words_ [ 0 ];
    my $t_vol_ = $t_result_words_ [ 1 ];
    my $t_dd_ = $t_result_words_ [ 2 ];
    my $t_pnl_stdev_ = $t_result_words_ [ 3 ];
    my $t_pnl_sharpe_ = $t_result_words_ [ 4 ];
    my $t_ttc_ = $t_result_words_ [ 5 ];

    given ( $sort_algo_ )
    {
      when ( "PNL" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_;
      }
      when ( "PNL_SQRT" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_ );
      }
      when ( "PNL_VOL" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_ * $t_vol_;
      }
      when ( "PNL_VOL_SQRT" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_ ) * sqrt ( $t_vol_ );
      }
      when ( "PNL_DD" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_ * abs ( $t_dd_ );
      }
      when ( "PNL_SQRT_DD" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_ * sqrt ( abs ( $t_dd_ ) );
      }
      when ( "PNL_VOL_SQRT_BY_DD" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_ * sqrt ( $t_vol_ ) * abs ( $t_dd_ ) ;
      }
      when ( "PNL_VOL_SQRT_BY_DD_SQRT" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ; # sign(pnl)*sqrt(abs(pnl)) * sqrt(abs(volume)) * sqrt(max(0,pnl_dd_ratio))
      }
      when ( "PNL_VOL_SQRT_BY_TTC_BY_DD_SQRT" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = ( SqrtSign ( $t_pnl_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / $t_ttc_;
      }
      when ( "PNL_VOL_SQRT_BY_TTC_SQRT_BY_DD_SQRT" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = ( SqrtSign ( $t_pnl_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / sqrt ( $t_ttc_ );
      }
      when ( "PNL_SHARPE" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_sharpe_;
      }
      when ( "PNL_SHARPE_SQRT" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_sharpe_ );
      }
      when ( "PNL_SHARPE_VOL" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_sharpe_ * $t_vol_;
      }
      when ( "PNL_SHARPE_VOL_SQRT" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ );
      }
      when ( "PNL_SHARPE_DD" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_sharpe_ * abs ( $t_dd_ );
      }
      when ( "PNL_SHARPE_SQRT_DD" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_sharpe_ * sqrt ( abs ( $t_dd_ ) );
      }
      when ( "PNL_SHARPE_VOL_SQRT_BY_DD" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_sharpe_ * sqrt ( $t_vol_ ) * abs ( $t_dd_ ) ;
      }
      when ( "PNL_SHARPE_VOL_SQRT_BY_DD_SQRT" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ; # sign(pnl_sharpe)*sqrt(abs(pnl_sharpe)) * sqrt(abs(volume)) * sqrt(max(0,pnl_sharpe_dd_ratio))
      }
      when ( "PNL_SHARPE_VOL_SQRT_BY_TTC_BY_DD_SQRT" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = ( SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / $t_ttc_;
      }
      when ( "PNL_SHARPE_VOL_SQRT_BY_TTC_SQRT_BY_DD_SQRT" )
      {
        $strat_name_to_algo_sorted_score_ { $t_strat_name_ } = ( SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / sqrt ( $t_ttc_ );
      }

      default
      {
        PrintStacktraceAndDie ( "SORT_ALGO=".$sort_algo_." NOT AVAILABLE" );
      }
    }
  }

  @algo_sorted_strat_names_ = ( );
  @algo_sorted_strat_results_ = ( );
  @algo_sorted_strat_scores_ = ( );

  foreach my $strat_name_ ( sort { $strat_name_to_algo_sorted_score_ { $b } <=> $strat_name_to_algo_sorted_score_ { $a } }
                           keys %strat_name_to_algo_sorted_score_ )
  {
    if ( FindItemFromVec ( $strat_name_ , @strats_to_exclude_ ) )
    { # We were explicitly instructed not to pick this strat.
      next;
    }

    if ( $#algo_sorted_strat_results_ > 5 * $num_strats_to_install_ )
    { # Consider 5 times the no. of strats to be installed ( a lot of them might have similar global results )
      last;
    }

    push ( @algo_sorted_strat_names_ , $strat_name_ );
    push ( @algo_sorted_strat_results_ , $strat_name_to_global_result_score_ { $strat_name_ } );
    push ( @algo_sorted_strat_scores_ , $strat_name_to_algo_sorted_score_ { $strat_name_ } );

    my $t_int_algo_sorted_score_ = sprintf ( "%d" , $strat_name_to_algo_sorted_score_ { $strat_name_ } );
    print $main_log_file_handle_ "\t ".$t_int_algo_sorted_score_." ".$strat_name_." ".$strat_name_to_global_result_score_ { $strat_name_ }."\n";

    foreach my $interval_ ( sort { $a <=> $b }
                           keys %interval_to_real_results_ )
    {
      if ( exists ( $interval_to_real_results_ { $interval_ } { $strat_name_ } ) )
      {
        print $main_log_file_handle_ "\t REAL-RESULT $interval_ ".$interval_to_real_results_ { $interval_ } { $strat_name_ }."\n";
      }
    }

    foreach my $interval_ ( sort { $a <=> $b }
                           keys %interval_to_global_results_ )
    {
      if ( exists ( $interval_to_global_results_ { $interval_ } { $strat_name_ } ) )
      {
        print $main_log_file_handle_ "\t GLOB-RESULT $interval_ ".$interval_to_global_results_ { $interval_ } { $strat_name_ }."\n";
      }
    }
  }

  return;
}

sub PickStrats
{
  my ( $diversity_index_ ) = @_ ;

  print $main_log_file_handle_ " # PickStrats ( $diversity_index_ )\n";

# First pick strats from STRATS_TO_KEEP list.
  foreach my $strat_name_ ( @strats_to_keep_ )
  {
    push ( @picked_strats_ , $strat_name_ );
  }

  my @algo_sorted_strat_similarities_ = ( );
  for (my $i = 0; $i <= $#algo_sorted_strat_names_ ; $i++)
  {
    push( @algo_sorted_strat_similarities_ , 0 );
  }

  foreach my $t_picked_strat_name_ ( @picked_strats_ )
  { # For each strat in the pool of candidate strats , compute similarity
# score against the currrent picked_strats pool.

    for ( my $i=0 ; $i <= $#algo_sorted_strat_names_ ; $i++ )
    {
      if ( $t_picked_strat_name_ eq $algo_sorted_strat_names_ [ $i ] )
      { # Already picked.
        $algo_sorted_strat_scores_ [ $i ] = -100000; # Invalidate score , so this strat doesn't get picked yet again.
            next;
      }

      my $similarity_ = GetSimilarity ( $algo_sorted_strat_names_[ $i ] , $t_picked_strat_name_ , $diversity_index_ );

      if( $similarity_ > $algo_sorted_strat_similarities_[ $i ])
      {
        $algo_sorted_strat_similarities_[ $i ] = $similarity_ ;
      }
    }
  }

  for ( my $i = 0 ; $i <= $#algo_sorted_strat_names_ && ( $#picked_strats_ + 1 ) < $num_strats_to_install_ ; $i ++ )
  {
    my $best_strat_index_ = 0;
    my $best_strat_score_ = -100000;
    for ( my $t_index_ = 0; $t_index_ <= $#algo_sorted_strat_names_ ; $t_index_++ )
    {
      if($algo_sorted_strat_scores_[ $t_index_ ] <= -100000)
      { # Already picked , skip.
        next;
      }

      my $t_strat_score_ = $algo_sorted_strat_scores_ [ $t_index_ ] * ( 1 - $algo_sorted_strat_similarities_[ $t_index_ ] );
      if( $t_strat_score_ > $best_strat_score_ )
      {
        $best_strat_score_ = $t_strat_score_ ;
        $best_strat_index_ = $t_index_ ;
      }
    }

    print $main_log_file_handle_ "pushing strat with index $best_strat_index_: score = ".$algo_sorted_strat_scores_[ $best_strat_index_ ]."\n";
    push ( @picked_strats_ , $algo_sorted_strat_names_ [ $best_strat_index_ ] );
    $algo_sorted_strat_scores_ [ $best_strat_index_ ] = -100000; # Invalidate score , so this strat doesn't get picked yet again.

        if ( $#picked_strats_ >= ( $num_strats_to_install_ - 1 ) )
        {
          last;
        }

# Update similarity scores for the remaining strats in the pool
# comparing against this pick ( best_strat_index_ )
    for ( my $t_index_ = 0; $t_index_ <= $#algo_sorted_strat_names_ ; $t_index_++ )
    {
      if ( $t_index_ == $best_strat_index_ )
      { # Don't compare a strat against itself.
        next;
      }

      my $similarity_ = GetSimilarity ( $algo_sorted_strat_names_[ $t_index_ ] , $algo_sorted_strat_names_[ $best_strat_index_ ] , $diversity_index_ );
      if( $similarity_ > $algo_sorted_strat_similarities_[ $t_index_ ])
      {
        $algo_sorted_strat_similarities_[ $t_index_ ] = $similarity_ ;
      }
    }
  }

  print $main_log_file_handle_ "\n\n\t PICKED :\n";
  for ( my $t_picked_ = 0 ; $t_picked_ <= $#picked_strats_ ; $t_picked_ ++ )
  {
    print $main_log_file_handle_ "\t\t ".$picked_strats_ [ $t_picked_ ]."\n";
  }

  return;
}

sub GetSimilarity
{
  my ($new_strat_name_ , $picked_strat_name_ , $diversity_index_ ) = @_;
  my $similarity_ = 0;
  foreach my $diversity_type_ ( keys % { $diversity_type_to_weight_{$diversity_index_} } )
  {
    given( $diversity_type_ )
    {
      when("REG_ALGO")
      {
        my $new_reg_algo_ = "";
        my $picked_reg_algo_ = "";
        my @new_words_ = split ('_', $new_strat_name_);
        foreach my $token (@new_words_)
        {
          if($token eq "FSLR" || $token eq "FSHLR" || $token eq "FSVLR" || $token eq "FSHDVLR" || $token eq "FSRR")
          {
            $new_reg_algo_ = $token;
          }
        }
        my @picked_words_ = split ('_', $picked_strat_name_);
        foreach my $token (@picked_words_)
        {
          if($token eq "FSLR" || $token eq "FSHLR" || $token eq "FSVLR" || $token eq "FSHDVLR" || $token eq "FSRR")
          {
            $picked_reg_algo_ = $token;
          }
        }
        if( $new_reg_algo_ eq $picked_reg_algo_ )
        {
          $similarity_ += $diversity_type_to_weight_{$diversity_index_}{$diversity_type_};
        }

      }
      when("PARAM_FILE_INDEX")
      {
        my $new_param_index_ = 0;
        my $picked_param_index_ = 0;
        my @new_words_ = split ( "_" , $new_strat_name_ );
        $new_param_index_ = $new_words_[ $#new_words_ ] ;
        my @picked_words_ = split ( "_" , $picked_strat_name_ );
        $picked_param_index_ = $picked_words_[ $#picked_words_ ] ;
        if( $new_param_index_ eq $picked_param_index_ )
        {
          $similarity_ += $diversity_type_to_weight_{$diversity_index_}{$diversity_type_};
        }
      }
      when("PRED_DURATION")
      {
        my $new_pred_duration_ = "";
        my $picked_pred_duration_ = "";

        my @new_words_ = split ( "_" , $new_strat_name_ );
#$new_pred_duration_ = $new_words_[ 8 ]."_".$new_words_[ 9 ] ;
        for ( my $i = 1; $i <= $#new_words_ ; $i++)
        {
          if($new_words_[ $i ] eq "na" || $new_words_[ $i ] eq "ac" || $new_words_[ $i ] eq "vj")
          {
            $new_pred_duration_ = $new_words_[ $i - 1 ];
            last;
          }
        }

        my @picked_words_ = split ( "_" , $picked_strat_name_ );
#$picked_pred_duration_ = $picked_words_[ 8 ]."_".$picked_words_[ 9 ] ;
        for ( my $i = 1; $i <= $#picked_words_ ; $i++)
        {
          if($picked_words_[ $i ] eq "na" || $picked_words_[ $i ] eq "ac" || $picked_words_[ $i ] eq "vj")
          {
            $picked_pred_duration_ = $picked_words_[ $i - 1 ];
            last;
          }
        }
        if($new_pred_duration_ eq "" || $picked_pred_duration_ eq "")
        {
          print "problem with pred_duration in $new_strat_name_ & $picked_strat_name_\n";
        }

        if( $new_pred_duration_ eq $picked_pred_duration_ && $new_pred_duration_ ne "" )
        {
          $similarity_ += $diversity_type_to_weight_{$diversity_index_}{$diversity_type_};
        }
      }
      when("FILTER_ALGO")
      {
        my $new_filter_algo_ = "";
        my $picked_filter_algo_ = "";
        my @new_words_ = split ('_', $new_strat_name_);
        foreach my $token (@new_words_)
        {
          if ( $token eq "f0" || $token eq "fst.5" || $token eq "fst1" || $token eq "fst2" || 
              $token eq "fsl2" || $token eq "fsl3" || $token eq "fsg.5" || $token eq "fsg1" || $token eq "fsg2" || 
              $token eq "fsr1" || $token eq "fsr.5" || $token eq "fv")
          {
            $new_filter_algo_ = $token;
          }
        }
        my @picked_words_ = split ('_', $picked_strat_name_);
        foreach my $token (@picked_words_)
        {
          if ( $token eq "f0" || $token eq "fst.5" || $token eq "fst1" || $token eq "fst2" || 
              $token eq "fsl2" || $token eq "fsl3" || $token eq "fsg.5" || $token eq "fsg1" || $token eq "fsg2" || 
              $token eq "fsr1" || $token eq "fsr.5" || $token eq "fv")
          {

            $picked_filter_algo_ = $token;
          }
        }
        if( $new_filter_algo_ eq $picked_filter_algo_ )
        {
          $similarity_ += $diversity_type_to_weight_{$diversity_index_}{$diversity_type_};
        }
      }
      when("STRATEGY_NAME")
      {
        my $new_strat_type_ = "";
        my $picked_strat_type_ = "";
        my @top_directories_ = ( );
        push ( @top_directories_ ,  $MODELLING_STRATS_DIR."/".$shortcode_ );
        push ( @top_directories_ , $MODELLING_PRUNED_STRATS_DIR."/".$shortcode_ );

        foreach my $top_directory_ ( @top_directories_ )
        {
          if( -d $top_directory_ )
          {
            if(opendir my $dh, $top_directory_)
            {
              my @t_list_=();
              while ( my $t_item_ = readdir $dh)
              {
                push(@t_list_, $t_item_);
              }
              closedir $dh;
              for my $dir_item_ (@t_list_)
              {
                next if( $dir_item_ eq "." || $dir_item_ eq "..");
                my $dir_name_ = $top_directory_."/".$dir_item_;

                if( -d $dir_name_)
                {
                  if( opendir my $dir, $dir_name_)
                  {
                    while ( my $t_strat_name_ = readdir $dir)
                    {
                      if( $t_strat_name_ eq $new_strat_name_ )
                      {
                        $new_strat_type_ = GetStratType("$dir_name_/$new_strat_name_");
                      }
                      if( $t_strat_name_ eq $picked_strat_name_ )
                      {
                        $picked_strat_type_ = GetStratType("$dir_name_/$picked_strat_name_");
                      }
                    }
                  }
                }
              }
            }

          }

          if ( $new_strat_type_ && $picked_strat_type_ )
          {
            last;
          }
        }

        if( $new_strat_type_ eq "" )
        {
          print "No data found for strat: $new_strat_name_\n";
        }
        if( $picked_strat_type_ eq "" )
        {
          print "No data found for strat: $picked_strat_name_\n";
        }
        if( $new_strat_type_ eq $picked_strat_type_ && $new_strat_type_ ne "" )
        {
          $similarity_ += $diversity_type_to_weight_{$diversity_index_}{ $diversity_type_ };
        }
      }
    }
  }
  return $similarity_;
}


sub RemoveIntermediateFiles
{
  print $main_log_file_handle_ " # RemoveIntermediateFiles\n";

  foreach my $intermediate_file_ ( @intermediate_files_ )
  {
    my $exec_cmd_ = "rm -f $intermediate_file_";
    print $main_log_file_handle_ "$exec_cmd_\n";
    `$exec_cmd_`;
  }

  return;
}

sub AddPickResultsToLocalDatabase
{
  my ( $current_date_ , $interval_index_ , $diversity_index_ , $sort_algo_ ) = @_;

  print $main_log_file_handle_ " # AddPickResultsToLocalDatabase ( $current_date_ , $interval_index_ , $diversity_index_ , $sort_algo_ )\n";

# Find the performance of strats picked in @picked_strats
  my %strat_name_to_final_global_result_ = ( );
  my @picked_strats_result_ = ( );
  my @t_global_results_ = ();
  GetGlobalResultsForShortcodeDate ( $shortcode_ , $current_date_ , \@t_global_results_);

  if($#t_global_results_<0)
  {return;        }

  foreach my $t_global_result_vec_ref_ ( @t_global_results_ )
  {
    my $t_strat_name_ = GetStratNameFromGlobalResultVecRef ( $t_global_result_vec_ref_ );

    if ( FindItemFromVec ( $t_strat_name_ , @picked_strats_ ) )
    { # This strat was picked , add to list to consider results
      my $t_global_result_line_ = join ( " ", @$t_global_result_vec_ref_ );
      $strat_name_to_final_global_result_ { $t_strat_name_ } = $t_global_result_line_;
      push ( @picked_strats_result_ , $t_global_result_line_ );
    }
  }

# Check that we have results for all picked strats.
  foreach my $t_picked_strat_ ( @picked_strats_ )
  {
    if ( ! exists ( $strat_name_to_final_global_result_ { $t_picked_strat_ } ) )
    {
      PrintStacktraceAndDie ( "No result found for $t_picked_strat_ for $current_date_" );
    }
  }

# Combine results.
  my $t_combined_result_ = CombineGlobalResultsLineFromVec ( @picked_strats_result_ );

  my @t_combined_result_words_ = split ( ' ' , $t_combined_result_ );
  $t_combined_result_words_ [ 0 ] = $sort_algo_."_".$interval_index_."_".$diversity_index_;

  if ( ! exists ( $permutation_name_to_strat_id_ { $t_combined_result_words_ [ 0 ] } ) )
  {
    $permutation_name_to_strat_id_ { $t_combined_result_words_ [ 0 ] } = $unique_strategy_id_ ++;
  }

  my @spliced_result_words_ = @t_combined_result_words_;
  splice ( @spliced_result_words_ , 0 , 2 );
  $t_combined_result_ = join ( ' ' , @spliced_result_words_ );

  $date_to_permutation_to_results_ { $current_date_ } { $t_combined_result_words_ [ 0 ] } = $t_combined_result_;

  print $main_log_file_handle_ "date_to_permutation_to_results_ $current_date_ ".$t_combined_result_words_ [ 0 ]." = $t_combined_result_\n";

  return;
}

sub WriteResultsToLocalDatabase
{
  print $main_log_file_handle_ " # WriteResultsToLocalDatabase\n";

  foreach my $date_ ( sort { $a <=> $b }
                     keys %date_to_permutation_to_results_ )
  {
    my $permutation_list_filename_ = $temp_results_base_dir_."/".$date_."_permutation_list.txt";
    my $results_list_filename_ = $temp_results_base_dir_."/".$date_."_results_list.txt";

    open ( PERM_LIST_FILE , ">" , $permutation_list_filename_ ) or PrintStacktraceAndDie ( "Could not create file $permutation_list_filename_" );
    open ( RESULTS_FILE , ">" , $results_list_filename_ ) or PrintStacktraceAndDie ( "Could not create file $results_list_filename_" );

    push ( @intermediate_files_ , $permutation_list_filename_ );
    push ( @intermediate_files_ , $results_list_filename_ );

    foreach my $permutation_ ( keys % { $date_to_permutation_to_results_ { $date_ } } )
    {
# Create a dummy strategy file corresponding to this permutation.
      my $t_permutation_file_name_ = $temp_results_base_dir_."/".$permutation_;

      if ( ! ExistsWithSize ( $t_permutation_file_name_ ) )
      {
        open ( DUMMY_PERM_FILE , ">" , $t_permutation_file_name_ ) or PrintStacktraceAndDie ( "Could not create file $t_permutation_file_name_" );

        push ( @intermediate_files_ , $t_permutation_file_name_ );

        print DUMMY_PERM_FILE "$permutation_ ".$permutation_name_to_strat_id_ { $permutation_ }."\n";

        close ( DUMMY_PERM_FILE );
      }	    

      print PERM_LIST_FILE $t_permutation_file_name_."\n";
      print RESULTS_FILE $date_to_permutation_to_results_ { $date_ } { $permutation_ }." ".$permutation_name_to_strat_id_ { $permutation_ }."\n";
    }

    close ( PERM_LIST_FILE );
    close ( RESULTS_FILE );

    my $ADD_RESULTS_SCRIPT = "$MODELSCRIPTS_DIR/add_results_to_local_database.pl";
    my $exec_cmd_ = "$ADD_RESULTS_SCRIPT $permutation_list_filename_ $results_list_filename_ $date_ $local_results_base_dir_";

    print $main_log_file_handle_ "# $exec_cmd_\n";
    my $t_local_results_database_file_ = `$exec_cmd_`;

    print $main_log_file_handle_ "t_local_results_database_file_ = $t_local_results_database_file_\n";

    chomp ( $t_local_results_database_file_ );
    if ( ! FindItemFromVec ( $t_local_results_database_file_ , @unique_results_filevec_ ) )
    {
      push ( @unique_results_filevec_ , $t_local_results_database_file_ );
    }
  }

  return;
}

sub SummarizeLocalResultsAndChoose
{
  print $main_log_file_handle_ " # SummarizeLocalResultsAndChoose\n";

  my $SUMMARIZE_LOCAL_RESULTS_AND_CHOOSE_BY_ALGO = $LIVE_BIN_DIR."/summarize_local_results_dir_and_choose_by_algo";

  my $exec_cmd_ = $SUMMARIZE_LOCAL_RESULTS_AND_CHOOSE_BY_ALGO." $summarization_algo_ 1 1 -10 0 10000 10000000 $local_results_base_dir_";
  print $main_log_file_handle_ $exec_cmd_."\n";

  my @exec_output_ = `$exec_cmd_`;

  print @exec_output_;

  PrintChosenPickConfig ( @exec_output_ );

  return;
}

sub PrintChosenPickConfig
{
  my ( @exec_output_ ) = @_;

  print $main_log_file_handle_ " # PrintChosenPickConfig\n";

  chomp ( @exec_output_ );

  my $permutation_name_ = "";

  foreach my $output_line_ ( @exec_output_ )
  {
    if ( index ( $output_line_ , "STRATEGYFILEBASE" ) >= 0 )
    {
      my @o_words_ = split ( ' ' , $output_line_ );

      $permutation_name_ = $o_words_ [ 1 ];
      last;
    }
  }

# $t_combined_result_words_ [ 0 ] = $sort_algo_."_".$interval_index_."_".$diversity_index_;
  my @permutation_words_ = split ( '_' , $permutation_name_ );
  my $t_diversity_index_ = $permutation_words_ [ $#permutation_words_ ];
  my $t_interval_index_ = $permutation_words_ [ $#permutation_words_ - 1 ];
  splice ( @permutation_words_ , $#permutation_words_ - 1 , 2 );
  my $t_sort_algo_ = join ( "_" , @permutation_words_ );

  print $main_log_file_handle_ "$t_sort_algo_ $t_interval_index_ $t_diversity_index_\n";

  print "\nSORT_ALGO\n";
  print $t_sort_algo_."\n";

  print "\nINTERVALS_TO_PICK_FROM\n";
  foreach my $interval_duration_ ( sort { $a <=> $b }
                                  keys % { $intervals_to_pick_from_ { $t_interval_index_ } } )
  {
    print $interval_duration_." ".$intervals_to_pick_from_ { $t_interval_index_ } { $interval_duration_ }."\n";
  }

  print "\n\nDIVERSITY_SCORES\n";
  foreach my $diversity_type_ ( keys % { $diversity_type_to_weight_ { $t_diversity_index_ } } )
  {
    print $diversity_type_." ".$diversity_type_to_weight_ { $t_diversity_index_ } { $diversity_type_ }."\n";
  }

  return;
}

sub IsTooInsample
{
  my ( $t_strat_name_ , $t_current_date_ , $t_interval_index_ ) = @_;
# print $main_log_file_handle_ "# IsTooInsample ( $t_strat_name_ , $t_current_date_ , $t_interval_index_ )\n";

  my $last_insample_date_ = GetInsampleDate ( $t_strat_name_ );

  my $total_weight_insample_ = 0;

  if ( $last_insample_date_ > 20110101 )
  {
    foreach my $interval_ ( sort { $a <=> $b }
                           keys % { $intervals_to_pick_from_ { $t_interval_index_ } } )
    {
      my $outsample_barrier_ = CalcPrevWorkingDateMult ( $t_current_date_ , ( ( $interval_ * 2 ) / 3 ) );

      if ( $last_insample_date_ > $outsample_barrier_ )
      {
        $total_weight_insample_ += $intervals_to_pick_from_ { $t_interval_index_ } { $interval_ };
      }
    }
  }

  return ( $total_weight_insample_ >= 0.5 );
}
