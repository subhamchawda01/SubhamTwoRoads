#!/usr/bin/perl

# \file scripts/move_staged_strats.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# USAGE:
# move_staged_strats.pl shortcode timeperiod [instruction-file/-1] [ lookback-days1 percentile_threshold1 lookback-days2 percentile_threshold2 .. -1 [pool-similarity-threshold=0.85] [self-similarity-threshold=0.75] [sort-algo] should-move[0/1, default:0] ]
#
# Followings are the checks(in that order) imployed on the staged strategies before moving to the main pool
# 1) To pass individual performance cutoff (min volume, max TTC, sharpe, min PNL etc.) on the longest interval provided.
# 2) To pass pool cutoffs for each interval
# 3) To pass diversity check
#

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

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $WF_SCRIPTS_DIR = $HOME_DIR."/".$REPO."/walkforward";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
my $TEMP_DIR="/spare/local/temp";
my $GLOBALRESULTSDBDIR = "DB";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie 
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList 
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/global_results_methods.pl"; # GetStratsWithGlobalResultsForShortcodeDate
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl";
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl";
require "$GENPERLLIB_DIR/pool_utils.pl";
require "$GENPERLLIB_DIR/date_utils.pl"; # GetTrainingDatesFromDB
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # GetTstampFromCname

sub LoadConfigFile;
sub LoadArgsCmdLine;
sub LoadDatesAndStagedStrats;
sub LoadStagedStratsResults;
sub FilterStratsCutoff;
sub LoadStratsPoolCutOffs;
sub ComputeOptimalMaxLoss;
sub LoadStratsPoolPerformanceThresholds;
sub FilterStratsIntervals;
sub FilterStratsDiversity;
sub SimilarStratPresent;
sub FindSimilarStrats;
sub PruneBadStrats;
sub RemoveIntermediateFiles;
sub SendReportMail;

# start 
my $USAGE="$0 shortcode timeperiod [instruction-file/-1] [ lookback-days1 percentile_threshold1 lookback-days2 percentile_threshold2 .. -1 [pool-similarity-threshold=0.85] [self-similarity-threshold=0.75] [sort-algo] should-move[0/1, default:0] ]";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = shift;
my $timeperiod_string_ = shift;
my $instruction_file_ = shift;

my @timeperiods_vec_ = ( );
if ( $timeperiod_string_ eq "ALL" ) {
  @timeperiods_vec_ = GetPoolsForShortcode ( $shortcode_ );
}
else {
  push ( @timeperiods_vec_, $timeperiod_string_ );
}

my @lookback_days_vec_ = ( );
my %percentile_thresh_vec_ = ( );

my $strat_last_training_date_ = GetIsoDateFromStrMin1 ( "TODAY-10" );
my $pool_similarity_thresh_ = 0.85;
my $self_similarity_thresh_ = 0.75;
my $sort_algo_ = "kCNAPnlSharpeAverage";
my $skip_dates_file_ = "INVALIDFILE";
my @skip_dates_vec_ = ( );
my @intermediate_files_ = ( );
my $verbose_ = 0;
my $to_move_ = 0;
my $email_address_;

my ( $volume_cutoff_, $ttc_cutoff_, $sharpe_cutoff_, $pnl_maxloss_cutoff_ );
my ( $msg_count_cutoff_, $minpnl_cutoff_ );

my $OML_lookback_days_ = 80;
my $OML_hit_ratio_ = 0.1;
my $OML_number_top_loss_ = 10;

if ( $instruction_file_ ne "-1" ) {
  LoadIFile ( $instruction_file_ );
} else {
  LoadArgsCmdLine ( @ARGV );
}

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $last_working_day_ = CalcPrevWorkingDateMult( $yyyymmdd_, 1);

my $min_days_frac_ = 0.8;

my $mail_str_ = "";

my %intervals2startdates_ = ( );
my @staged_strats_to_consider_ = ( );
my %staged_strats_2_intervals_2_results_ = ( );
my %intervals2cutoff_ = ( );
my %strats_2_intervals_2_results_ = ( );
my @strats_passed_cutoff_ = ( );
my @strats_passed_performance_ = ( );
my @strats_passed_diversity_ = ( );
my %strats_replace_map_ = ( );
my @strats_recently_installed_ = ( );
my $timeperiod_;
my $max_strats_to_move_ = 2;
my $pool_list_ = GetCSTempFileName ( "/spare/local/temp/" );
my $staged_list_ = GetCSTempFileName ( "/spare/local/temp/" );

foreach my $tp_ ( @timeperiods_vec_ )
{
  $timeperiod_ = $tp_;
  print "\nLooking for Timeperiod: ".$timeperiod_."\n";

  MakeStratLists ( );

  %intervals2startdates_ = ( );
  @staged_strats_to_consider_ = ( );
  LoadDatesAndStagedStrats ( );
#print "Staged strats to consider: ".join("\n", @staged_strats_to_consider_)."\n";

  %staged_strats_2_intervals_2_results_ = ( );
  LoadStagedStratsResults ( );

  @strats_passed_cutoff_ = ( );
  FilterStratsCutoff ( );

  %intervals2cutoff_ = ( );
  %strats_2_intervals_2_results_ = ( );
  LoadStratsPoolPerformanceThresholds ( );
  print "intervals cutoff: ".join(" ", map { $_.":".$intervals2cutoff_{$_} } keys %intervals2cutoff_)."\n";

  @strats_passed_performance_ = ( );
  FilterStratsIntervals ( );
#print join("\n", @strats_passed_performance_)."\n";

  @strats_passed_diversity_ = ( );
  %strats_replace_map_ = ( );
  FilterStratsDiversity ( );

  if ( $#strats_passed_diversity_ >= $max_strats_to_move_ ) {
    @strats_passed_diversity_ = @strats_passed_diversity_[0 .. ($max_strats_to_move_ - 1)] ;
  }

#print join("\n", @strats_passed_diversity_)."\n";
  @strats_recently_installed_ = ( );
  ReadPickstratConfigs( \@strats_recently_installed_ );
#print join("\n", @strats_recently_installed_)."\n";

  my $logstr_ = "";
  foreach my $t_stt_ ( @strats_passed_diversity_ ) {
    $logstr_ = $logstr_.$t_stt_;
    if ( defined $strats_replace_map_{ $t_stt_ } && ! FindItemFromVec( $strats_replace_map_{ $t_stt_ }, @strats_recently_installed_ ) ) {
      $logstr_ = $logstr_." --replace--> ".$strats_replace_map_{ $t_stt_ }."\n";
    }
    $logstr_ = $logstr_."\n";
  }

  if ( $to_move_ ) {
    if ( $#strats_passed_diversity_ >= 0 ) {
      print "TimePeriod: $timeperiod_\nStrats Moved :\n".$logstr_."\n\n";
      $mail_str_ .= "TimePeriod: $timeperiod_\nStrats Moved :\n".$logstr_."\n\n";
    } 
    else {
      print "TimePeriod: $timeperiod_\nNo Strats Moved\n\n";
    }
  }
  else {
    if ( $#strats_passed_diversity_ >= 0 ) {
      print "TimePeriod: $timeperiod_\nStrats to be Moved :\n".$logstr_."\n\n";
      $mail_str_ .= "TimePeriod: $timeperiod_\nStrats to be Moved :\n".$logstr_."\n\n";
    } 
    else {
      print "TimePeriod: $timeperiod_\nNo Strats to be Moved\n\n";
    }
  }

  if ( $to_move_ && $#strats_passed_diversity_ >= 0 ) {
    my $log_file_ = $MODELING_BASE_DIR."/move_staged_logs/".$shortcode_."_".$timeperiod_."_logs.txt";
    my $log_dir_ = dirname ( $log_file_ );

    if ( ! -f $log_dir_ ) {
      `mkdir -p $log_dir_`;
    }

    open LOGFHANDLE, ">> $log_file_" or PrintStacktraceAndDie ( "Could not open $log_file_ for writing" );

    foreach my $cname ( @strats_passed_diversity_ ) {
      my $exec_cmd_ = "$WF_SCRIPTS_DIR/process_config.py -c $cname -m MOVE_TO_POOL";
      `$exec_cmd_`;
    }

    my @strats_to_prune_from_pool_ = grep { ! FindItemFromVec( $_, @strats_recently_installed_ ) } values %strats_replace_map_;
    print "# of Pool Strats being replaced: ".@strats_to_prune_from_pool_."\n";
    PruneBadStrats( \@strats_to_prune_from_pool_ );

    print LOGFHANDLE "\n\nStrats Moved: rundate: $yyyymmdd_ :\n\n";
    print LOGFHANDLE $logstr_."\n";
    close LOGFHANDLE;
  }
}

SendReportMail ( $mail_str_ );

RemoveIntermediateFiles ( );

exit ( 0 );


sub LoadIFile
{
  print "Loading Config File ...\n";

  my $t_instruction_file_ = shift;

  open ( CONFIG_FILE , "<" , $t_instruction_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_instruction_file_" );
  my @instruction_file_lines_ = <CONFIG_FILE>; chomp ( @instruction_file_lines_ );
  close ( CONFIG_FILE );

  my $current_param_ = "";
  foreach my $iline_ ( @instruction_file_lines_ )
  {
    if ( $iline_ =~ /^#/ ) {  # not ignoring lines with # not at the beginning
      next;
    }
    my @t_words_ = split ( ' ' , $iline_ );

    if ( $#t_words_ < 0 ) {
      $current_param_ = "";
      next;
    }

    if ( ! $current_param_ ) {
      $current_param_ = $t_words_ [ 0 ];
      next;
    }
    else {
      given ( $current_param_ ) {
        when ( "INTERVAL_PERCENTILES" ) {
          if ( scalar @t_words_ < 2 ) {
            next;
          }
          my ( $intv_, $percentile_ ) = @t_words_[0..1];
          push ( @lookback_days_vec_, $intv_ );
          $percentile_thresh_vec_{ $intv_ } = $percentile_;
        }
        when ( "POOL_SIMILARITY_THRESHOLD" ) {
          $pool_similarity_thresh_ = $t_words_[0];
        }
        when ( "SELF_SIMILARITY_THRESHOLD" ) {
          $self_similarity_thresh_ = $t_words_[0];
        }
# If volume or TTC < 1, then assume them to be percentile cutoff for the current pool. 
# vol_cutoff_ = 0.3, implies, the volume_cutoff is 30%tile in ascending sorted order.
# ttc_cutoff_ = 0.3, implies, the ttc_cutoff is 30%tile in desceding sorted order.
        when ( "VOLUME_CUTOFF" ) {
          $volume_cutoff_ = $t_words_[0];
        }
        when ( "TTC_CUTOFF" ) {
          $ttc_cutoff_ = $t_words_[0];
        }
        when ( "SHARPE_CUTOFF" ) {
          $sharpe_cutoff_ = $t_words_[0];
        }
        when ( "PNL_MAXLOSS_CUTOFF" ) {
          $pnl_maxloss_cutoff_ = $t_words_[0];
        }
        when ( "MIN_PNL_CUTOFF" ) {
          $minpnl_cutoff_ = $t_words_[0];
        }
        when ( "MSG_COUNT_CUTOFF" ) {
          $msg_count_cutoff_ = $t_words_[0];
        }
        when ( "TO_MOVE" ) {
          $to_move_ = $t_words_[0];
        }
        when ( "MAX_STRATS_TO_MOVE" ) {
          $max_strats_to_move_ = $t_words_[0];
        }
        when ( "SORT_ALGO" ) {
          $sort_algo_ = $t_words_[0];
        }
        when ( "LAST_TRAINING_DATE" ) {
          my $t_strat_last_training_date_ = GetIsoDateFromStrMin1 ( $t_words_[0] );
          if ( ValidDate ( $t_strat_last_training_date_ ) ) {
            $strat_last_training_date_ = $t_strat_last_training_date_;
          }
        }
        when ( "OPTIMAL_MAX_LOSS_SETTINGS" ) {
          $OML_lookback_days_ = max ( 30, $t_words_ [ 0 ] ) ;
          $OML_hit_ratio_ = min ( 0.2, $t_words_ [ 1 ] ) ; #greater than 20% is not sane
        }
        when ( "SKIP_DATES_FILE" ) {
          $skip_dates_file_ = $t_words_[0];
        }
        when ( "SKIP_DATES" ) {
          push ( @skip_dates_vec_, $t_words_[0] );
        }
        when ( "VERBOSE" ) {
          $verbose_ = $t_words_[0];
        }
        when ( "MIN_DAYS_FRAC" ) {
          $min_days_frac_ = $t_words_[0];
        }
        when ( "EMAIL_ADDRESS" ) {
          $email_address_ = $t_words_[0];
        }
      }
    }
  }

  if ( $skip_dates_file_ ne "INVALIDFILE" && $#skip_dates_vec_ >= 0 ) {
    my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
    $skip_dates_file_ = $TEMP_DIR."/skip_dates_".$unique_gsm_id_;
    push ( @intermediate_files_, $skip_dates_file_ );
  }
}

sub LoadArgsCmdLine
{
  my @arg_words_ = @_;

  my $t_lookback_days_ = shift @arg_words_;
  my $t_percentile_thresh_;
  while ( defined $t_lookback_days_ && $t_lookback_days_ ne "-1" ) {
    $t_percentile_thresh_ = shift @arg_words_;
    if ( ! defined $t_percentile_thresh_ ) { last; }
    push ( @lookback_days_vec_, $t_lookback_days_ );
    $percentile_thresh_vec_{ $t_lookback_days_ } = $t_percentile_thresh_;
    print "Lookback days: ".$t_lookback_days_." ".$t_percentile_thresh_."\n";

    $t_lookback_days_ = shift @arg_words_;
  }

  if ( @arg_words_ > 0 ) { $pool_similarity_thresh_ = shift @arg_words_; }

  if ( @arg_words_ > 0 ) { $self_similarity_thresh_ = shift @arg_words_; }

  if ( @arg_words_ > 0 ) { $sort_algo_ = shift @arg_words_; }

  if ( @arg_words_ > 0 ) { $to_move_ = shift @arg_words_; }
}

sub LoadDatesAndStagedStrats
{
  my %intervals2dates_ = ( );
  my @repeated_combined_dates_ = ( );

  foreach my $num_days_ ( @lookback_days_vec_ ) {
    my @t_dates_vec_ = GetDatesFromNumDays ($shortcode_, $last_working_day_, $num_days_);
    $intervals2dates_{ $num_days_ } = \@t_dates_vec_;
    $intervals2startdates_{ $num_days_ } = min (@t_dates_vec_); 
    
    push ( @repeated_combined_dates_, @t_dates_vec_ );
  }
  my @combined_dates_ = GetUniqueList (@repeated_combined_dates_);

  my @staged_strats_ = GetConfigsForPool ($shortcode_, $timeperiod_, 'S');

  my %date_strat_to_existance_ = ( );

  foreach my $this_date_ ( @combined_dates_ ) {
    my @t_strats_with_res_ = ();
    GetStratsWithGlobalResultsForShortcodeDate( $shortcode_, $this_date_, \@t_strats_with_res_, $GLOBALRESULTSDBDIR, "S");
    
    if ( $#t_strats_with_res_ >= 0 ) {
      foreach  my $t_strat_base_name_ ( @t_strats_with_res_ ) {
        $date_strat_to_existance_ { $this_date_.".".$t_strat_base_name_ } = 1;
      }
    }
  }

  my %strat_update_date_ = ( );
  foreach my $t_strat_ ( @staged_strats_ ) {
    my $strat_tstamp_ = GetTstampFromCname($t_strat_);
    my $date_from_tstamp_ = substr $strat_tstamp_, 0, 10;
    my @ymd_ =  split /-/, $date_from_tstamp_;
    my $date_yyyymmdd_ = $ymd_[0].$ymd_[1].$ymd_[2];
    $strat_update_date_{ $t_strat_ } = $date_yyyymmdd_;
  }
  @staged_strats_to_consider_ = ( );

  foreach my $t_strat_ ( @staged_strats_ ) {
    my $strat_passed_ = 1;
    if ( $strat_update_date_{ $t_strat_ } gt CalcPrevWorkingDateMult ( $last_working_day_, 20 ) ){
      print "Strat ".$t_strat_." was last updated within 20 days from today. skipping strat.. \n";
      $strat_passed_ = 0;
      next;
    }
    foreach my $num_days_ ( @lookback_days_vec_ ) {
      my $numdays_results_present_ = 0;
      foreach my $t_date_ ( @{ $intervals2dates_{ $num_days_ } } ) {
        if ( exists $date_strat_to_existance_ { $t_date_.".".$t_strat_ } ) {
          $numdays_results_present_++;
        }
      }
#print "Details ".$t_strat_." ".$num_days_." ".$numdays_results_present_."\n";
      if ( $numdays_results_present_ < $min_days_frac_ * @{ $intervals2dates_{ $num_days_ } } ) {
        print "Warning: strat ".$t_strat_." has results for just ".$numdays_results_present_." days in the lookback-period ".$num_days_.". skipping strat..\n";
        $strat_passed_ = 0;
        last;
      }
    }
    if ( $strat_passed_ ) {
      push ( @staged_strats_to_consider_, $t_strat_ );
    }
  }
}

sub LoadStagedStratsResults 
{
  foreach my $num_days_ ( @lookback_days_vec_ ) {
    my $t_start_date_ = $intervals2startdates_{ $num_days_ };

    my $exec_cmd_ = "$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $staged_list_ $GLOBALRESULTSDBDIR $t_start_date_ $last_working_day_ $skip_dates_file_ $sort_algo_ 0 IF 1 S 2";
    print $exec_cmd_."\n";
    my @ssr_output_ = `$exec_cmd_`;
    chomp ( @ssr_output_ );
    foreach my $ssr_line_ ( @ssr_output_ ) {
      my @ssr_words_ = split (' ', $ssr_line_ );
      chomp ( @ssr_words_ );
      if ( FindItemFromVec( $ssr_words_[1], @staged_strats_to_consider_ )
            && ( ! defined $staged_strats_2_intervals_2_results_{ $ssr_words_[1] }{ $num_days_ } ) ) {
        $staged_strats_2_intervals_2_results_{ $ssr_words_[1] }{ $num_days_ } = $ssr_words_[ $#ssr_words_ ];
      }
    }
  }
}

sub FilterStratsCutoff
{
# If volume or TTC < 1, then assume them to be percentile cutoff for the current pool. 
# vol_cutoff_ = 0.3, implies, the volume_cutoff is 30%tile in ascending sorted order.
# ttc_cutoff_ = 0.3, implies, the ttc_cutoff is 30%tile in desceding sorted order.
  LoadStratsPoolCutOffs ( );

  my $intv_ = max ( 50, @lookback_days_vec_ );
  my $start_date_ = CalcPrevWorkingDateMult ( $yyyymmdd_, $intv_ );

  my $exec_cmd_ = "$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $staged_list_ $GLOBALRESULTSDBDIR $start_date_ $last_working_day_ $skip_dates_file_ $sort_algo_ 0 IF 1 S 2";

  print $exec_cmd_."\n";
  my @ssr_output_ = `$exec_cmd_`;
  chomp ( @ssr_output_ );
  foreach my $ssr_line_ ( @ssr_output_ ) {
    my @ssr_words_ = split (' ', $ssr_line_ );
    chomp ( @ssr_words_ );

    if ( FindItemFromVec( $ssr_words_[1], @staged_strats_to_consider_ ) ) {
      my $t_volume_average_ = $ssr_words_[4];
      my $t_sharpe_ = $ssr_words_[5];
      my $t_normalized_ttc_ = $ssr_words_[9];
      my $t_msg_count_ = $ssr_words_[19];
      my $t_min_pnl_ = $ssr_words_[22];

      if ( defined $volume_cutoff_ && $t_volume_average_ < $volume_cutoff_ ) 
      { 
        if ( $verbose_ ) { print "Vol $t_volume_average_ < $volume_cutoff_ . Skipping $ssr_words_[1]\n"; }   
        next; 
      }

      if ( defined $ttc_cutoff_ && $t_normalized_ttc_ > $ttc_cutoff_ ) 
      { 
        if ( $verbose_ ) { print "TTC $t_normalized_ttc_ > $ttc_cutoff_ . Skipping $ssr_words_[1]\n"; }   
        next; 
      }

      if ( defined $sharpe_cutoff_ && $t_sharpe_ < $sharpe_cutoff_ ) 
      { 
        if ( $verbose_ ) { print "Sharpe $t_sharpe_ < $sharpe_cutoff_ . Skipping $ssr_words_[1]\n"; }   
        next; 
      }

      if ( defined $msg_count_cutoff_ && $t_msg_count_ > $msg_count_cutoff_ )
      { 
        if ( $verbose_ ) { print "MsgCount $t_msg_count_ > $msg_count_cutoff_ . Skipping $ssr_words_[1]\n"; }   
        next; 
      }

      if ( defined $minpnl_cutoff_ && $t_min_pnl_ < $minpnl_cutoff_ )
      { 
        if ( $verbose_ ) { print "MinPnl $t_min_pnl_ < $minpnl_cutoff_ . Skipping $ssr_words_[1]\n"; }   
        next;
      }

      if ( defined $pnl_maxloss_cutoff_ ) { 
        my $t_pnl_maxloss_ = ComputeOptimalMaxLoss ( $ssr_words_[1] );
        if ( $t_pnl_maxloss_ == -1 || $t_pnl_maxloss_ < $pnl_maxloss_cutoff_) 
        {
          if ( $verbose_ ) { print "MaxLossByPNL $t_pnl_maxloss_ < $pnl_maxloss_cutoff_ . Skipping $ssr_words_[1]\n"; }   
          next; 
        }
      }
      
      push ( @strats_passed_cutoff_, $ssr_words_[1] );
    }
  }

  print "# of Strats Passed Cuttoffs: ".@strats_passed_cutoff_."\n";
}

sub LoadStratsPoolCutOffs
{
  my ( $volume_percentile_, $ttc_percentile_);
  if ( defined $volume_cutoff_ && $volume_cutoff_ > 0 && $volume_cutoff_ < 1 ) {
    $volume_percentile_ = $volume_cutoff_;
  }
  if ( defined $ttc_cutoff_ && $ttc_cutoff_ > 0 && $ttc_cutoff_ < 1 ) {
    $ttc_percentile_ = $ttc_cutoff_;
  }

  if ( defined $volume_percentile_ || defined $ttc_percentile_ ) {
    my $intv_ = max ( 50, @lookback_days_vec_ );
    my $start_date_ = CalcPrevWorkingDateMult ( $yyyymmdd_, $intv_ );

    my @vol_vec_ = ( );
    my @ttc_vec_ = ( );

    my $exec_cmd_ = "$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $pool_list_ $GLOBALRESULTSDBDIR $start_date_ $last_working_day_ $skip_dates_file_ $sort_algo_ 0 IF 1 N 2";

    print $exec_cmd_."\n";
    my @ssr_output_ = `$exec_cmd_`;
    chomp ( @ssr_output_ );
    foreach my $ssr_line_ ( @ssr_output_ ) {
      my @ssr_words_ = split (' ', $ssr_line_ );
      chomp ( @ssr_words_ );

      push ( @vol_vec_, $ssr_words_[4] );
      push ( @ttc_vec_, $ssr_words_[9] );
    }
    my $t_size_ = @vol_vec_;

    if ( $t_size_ > 0 ) {
      if ( defined $volume_percentile_ ) {
        @vol_vec_ = sort { $a <=> $b } @vol_vec_;
        my $vol_cutoff_index_ = max( 0, $volume_percentile_ * $t_size_ - 1 );
        $volume_cutoff_ = $vol_vec_[ $vol_cutoff_index_ ];
        print "Volume cutoff is: ".$volume_cutoff_."\n";
      }

      if ( defined $ttc_percentile_ ) {
        @ttc_vec_ = reverse sort { $a <=> $b } @ttc_vec_;
        my $ttc_cutoff_index_ = max( 0, $ttc_cutoff_ * $t_size_ - 1 );
        $ttc_cutoff_ = $ttc_vec_[ $ttc_cutoff_index_ ];
        print "TTC cutoff is: ".$ttc_cutoff_."\n";
      }
    }
  }
}

sub ComputeOptimalMaxLoss
{
  my $t_strat_ = shift;

  my $FIND_OPTIMAL_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_max_loss.pl";
  my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shortcode_ $timeperiod_ $OML_lookback_days_ $OML_hit_ratio_ $OML_number_top_loss_ $t_strat_ $yyyymmdd_ $skip_dates_file_";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

  my ( $max_loss_, $avg_pnl_, $num_max_loss_hits_ );
  foreach my $max_loss_line_ ( @exec_output_ )
  {
    if ( index ( $max_loss_line_ , "=>" ) >= 0 || index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 ) {
      next;
    }

    my @max_loss_words_ = split ( ' ' , $max_loss_line_ );
    if ( $#max_loss_words_ >= 2 )
    {
      $max_loss_ = $max_loss_words_ [ 0 ];
      $avg_pnl_ = $max_loss_words_ [ 1 ];
      $num_max_loss_hits_ = $max_loss_words_ [ 2 ];

      last;
    }
  }

  if ( defined $avg_pnl_ && defined $max_loss_ ) {
    my $pnl_maxloss_ = $avg_pnl_ / $max_loss_;
    return $pnl_maxloss_;
  }
  else {
    return -1;
  }
}
      
sub LoadStratsPoolPerformanceThresholds
{
  foreach my $num_days_ ( @lookback_days_vec_ ) {
    my $t_start_date_ = $intervals2startdates_{ $num_days_ };

    my $exec_cmd_ = "$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $pool_list_ $GLOBALRESULTSDBDIR $t_start_date_ $last_working_day_ $skip_dates_file_ $sort_algo_ 0 IF 1 N 2";
    print $exec_cmd_."\n";
    my @ssr_output_ = `$exec_cmd_`;
    chomp ( @ssr_output_ );

    my %strats2metric_ = ( );
    foreach my $ssr_line_ ( @ssr_output_ ) {
      my @ssr_words_ = split (' ', $ssr_line_ );
      chomp ( @ssr_words_ );
      if ( ! defined $strats2metric_{ $ssr_words_[1] } ) {
        $strats2metric_{ $ssr_words_[1] } = $ssr_words_[ $#ssr_words_ ];
        $strats_2_intervals_2_results_{ $ssr_words_[1] }{ $num_days_ } = $ssr_words_[ $#ssr_words_ ];
      }
    }
    my $t_size_ = keys %strats2metric_;

    if ( $t_size_ > 5 ) {
      my $cutoff_index_ = max( 5, ($percentile_thresh_vec_{ $num_days_ } * $t_size_) - 1 ); 
      my @metrics_sorted_ = reverse sort { $a <=> $b } values %strats2metric_;
      $intervals2cutoff_{ $num_days_ } = $metrics_sorted_ [ $cutoff_index_ ];
    }
    else{
      print "Number of strategies in main pool is $t_size_.  For pool size smaller than 6, please add strategies manually.\n";
      last;
    }
  }
}

sub FilterStratsIntervals
{
  @strats_passed_performance_ = ( );
  foreach my $t_strat_ ( @strats_passed_cutoff_ ) {
#print "Considering: ".$t_strat_."\n";
    my $is_passed_ = 1;
    foreach my $num_days_ ( @lookback_days_vec_ ) {
      if ( ! defined $staged_strats_2_intervals_2_results_{ $t_strat_ }{ $num_days_ }
          || ( ! defined $intervals2cutoff_{ $num_days_ }
            || $staged_strats_2_intervals_2_results_{ $t_strat_ }{ $num_days_ } < $intervals2cutoff_{ $num_days_ } ) ) {
#print "$t_strat_ performance on ".$num_days_." is ".$staged_strats_2_intervals_2_results_{ $t_strat_ }{ $num_days_ }.", cutoff: ".$intervals2cutoff_{ $num_days_ }."\n";
        $is_passed_ = 0;
        last;
      }
    }
    if ( $is_passed_ ) {
#print "$t_strat_ passed the performance thresholds\n";
      push ( @strats_passed_performance_, $t_strat_ );
    }
  }
}

sub FilterStratsDiversity
{
  my @strats_vec_ = @strats_passed_performance_;
  push ( @strats_vec_, keys %strats_2_intervals_2_results_ );

  my $num_days_ = 150;
  my $start_date_ = CalcPrevWorkingDateMult($last_working_day_, $num_days_);
  my @dates_vec_ = GetTrainingDatesFromDB($start_date_, $last_working_day_, "", "INVALIDFILE");

  my %sample_pnls_strats_vec_;
  FetchPnlSamplesStrats ( $shortcode_, \@strats_vec_, \@dates_vec_, \%sample_pnls_strats_vec_ );

  my $sort_interval_ = max ( keys %intervals2cutoff_ );
  my @strats_sorted_ = sort { $staged_strats_2_intervals_2_results_{ $a }{ $sort_interval_ } <=> $staged_strats_2_intervals_2_results_{ $b }{ $sort_interval_ } } @strats_passed_performance_;
  foreach my $t_strat_ ( @strats_sorted_ ) {
    if ( SimilarStratPresent ( $t_strat_, \@strats_passed_diversity_, \%sample_pnls_strats_vec_) ) {
      next;
    }

    my %similar_strats_ = ( );
    FindSimilarStrats ( $t_strat_, \%similar_strats_, \%sample_pnls_strats_vec_);

    if ( !keys %similar_strats_ ) {
      push ( @strats_passed_diversity_, $t_strat_ );
      next;
    }

    my $strat_to_replace_ = "";
    foreach my $sim_strat_ ( sort { $similar_strats_{ $b } <=> $similar_strats_{ $a } } keys %similar_strats_ ) {
      my $is_staged_better_ = 1;
      foreach my $num_days_ ( @lookback_days_vec_ ) {
        if ( $strats_2_intervals_2_results_{ $sim_strat_ }{ $num_days_ } > $staged_strats_2_intervals_2_results_{ $t_strat_ }{ $num_days_ } ) {
          $is_staged_better_ = 0;
          last;
        }
      }
      if ( $is_staged_better_ ) {
        push ( @strats_passed_diversity_, $t_strat_ );
        $strats_replace_map_{ $t_strat_ } = $sim_strat_;
        last;
      }
    }
  }
}

sub SimilarStratPresent
{
  my $strat_ = shift;
  my $strats_vec_ref_ = shift;
  my $sample_pnls_strats_vec_ref_ = shift;

  foreach my $t_strat_ ( @$strats_vec_ref_ ) {
    my $corr_ = GetPnlSamplesCorrelation ( $strat_, $t_strat_, $sample_pnls_strats_vec_ref_ );
    if ( $corr_ > $self_similarity_thresh_ ) {
      return 1;
    }
  }
  return 0;
}

sub FindSimilarStrats
{
  my $strat_ = shift;
  my $similar_strats_ref_ = shift;
  my $sample_pnls_strats_vec_ref_ = shift;

#print "Similar strats to ".$strat_.":\n";
  foreach my $pool_strat_ ( keys %strats_2_intervals_2_results_ ) {
    my $corr_ = GetPnlSamplesCorrelation ( $strat_, $pool_strat_, $sample_pnls_strats_vec_ref_ );
    if ( $corr_ > $pool_similarity_thresh_ ) {
      $$similar_strats_ref_{ $pool_strat_ } = $corr_;
#print $pool_strat_.": ".$corr_."\n";
    }
  }
}

sub PruneBadStrats
{
  my $strats_to_prune_ref_ = shift;

  foreach my $strat_ ( @$strats_to_prune_ref_ ) {
    my $exec_cmd_ = "$WF_SCRIPTS_DIR/process_config.py -c $strat_ -m PRUNE";
    `$exec_cmd_`;
  }
}

sub ReadPickstratConfigs
{
  my $strat_names_ref_ = shift;

  my @query_ids_vec_ = ( );

  my @configs_ = `ls $MODELING_BASE_DIR/pick_strats_config/\*/$shortcode_\.\*\.txt 2>/dev/null`;
  chomp( @configs_ );

  foreach my $tconfig_ ( @configs_ ) {
    if ( $tconfig_ ne "" ) {
      my $query_id_start_ = `$SCRIPTS_DIR/get_config_field.pl $tconfig_ PROD_QUERY_START_ID | grep -v '^#' | head -1 | awk '{print \$1}'`;
      chomp( $query_id_start_ );

      if ( $query_id_start_ eq "" ) {
        next;
      }

      my $query_id_end_ = `$SCRIPTS_DIR/get_config_field.pl $tconfig_ PROD_QUERY_STOP_ID | grep -v '^#' | head -1 | awk '{print \$1}'`;
      chomp ( $query_id_end_ );

      if ( $query_id_end_ eq "" ) { 
        $query_id_end_ = $query_id_start_ + 8;
      }

      push ( @query_ids_vec_, $query_id_start_..$query_id_end_ );
    }
  }

  my $tdate_ = $yyyymmdd_;
  my $numdays_ = 10;

  for ( my $iday_ = 0; $iday_ < $numdays_; $iday_++ ) {
    $tdate_ = CalcPrevWorkingDateMult ( $tdate_, 1 );
    
    ( my $yyyy_ , my $mm_ , my $dd_ ) = BreakDateYYYYMMDD ( $tdate_ );
    my $query_log_dir_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_;

    foreach my $queryid_ ( @query_ids_vec_ ) {
      my $query_log_file_ = $query_log_dir_."/log.".$tdate_.".".$queryid_.".gz";
      if ( ! -e $query_log_file_ ) { next; }

      my @grep_lines_ = `zgrep STRATEGYLINE $query_log_file_`; chomp ( @grep_lines_ );

      foreach my $grep_line_  ( @grep_lines_ ) {
        my @grep_line_words_ = split ( ' ', $grep_line_ );
        push ( @$strat_names_ref_, $grep_line_words_[8] );
      }
    }
  }

  my %strat_names_hash_ = map { $_ => 1 } @$strat_names_ref_;
  @$strat_names_ref_ = keys %strat_names_hash_;
}

sub MakeStratLists
{
  my @configs_ = GetConfigsForPool ($shortcode_, $timeperiod_, 'N');
  open CSTF, "> $pool_list_" or PrintStacktraceAndDie ( "Could not open $pool_list_ for writing\n" );
  print CSTF $_."\n" foreach @configs_;
  close CSTF;

  @configs_ = GetConfigsForPool ($shortcode_, $timeperiod_, 'S');
  open CSTF, "> $staged_list_" or PrintStacktraceAndDie ( "Could not open $staged_list_ for writing\n" );
  print CSTF $_."\n" foreach @configs_;
  close CSTF;
}

sub RemoveIntermediateFiles
{
  foreach my $intermediate_file_ ( @intermediate_files_ )
  {
    my $exec_cmd_ = "rm -f $intermediate_file_";
    `$exec_cmd_`;
  }
  return;
}

sub SendReportMail
{
  my $mailstr_ = shift;

  if ( ! defined $email_address_ || ! $to_move_ )
  {
    return;
  }
  if ( $email_address_ && $mailstr_ ne "" )
  {
    open ( MAIL , "|/usr/sbin/sendmail -t" );
    print MAIL "To: $email_address_\n";
    print MAIL "From: $email_address_\n";
    print MAIL "Subject: Move Staged Strats $shortcode_ $timeperiod_ ( $instruction_file_ )\n\n";
    print MAIL $mailstr_; 
    close(MAIL);
  }
}

