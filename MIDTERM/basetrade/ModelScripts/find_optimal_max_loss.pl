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

sub GetRunningQueries;
sub GetUnitTradeSizesForStrats;
sub GetGlobalResultsForRunningStrats;
sub ChooseMaxLoss;
sub GetStratNameFromResultLine;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts"; 
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $WF_DB_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward/wf_db_utils";
my $WF_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward";

my $hostname_s_ = `hostname -s`; chomp ( $hostname_s_ );
my $GLOBAL_RESULTS_DIR = "DB";
my $GLOBAL_STAGEDRESULTS_DIR = "DB";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/global_results_methods.pl"; # GetPnlFromGlobalResultsLine , GetMinPnlFromGlobalResultsLine
require "$GENPERLLIB_DIR/strat_utils.pl"; # CheckIfRegimeParam
require "$GENPERLLIB_DIR/option_strat_utils.pl"; # IsOptionStrat
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/config_utils.pl"; # IsValidConfig

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD [DAYS_TO_LOOK_BEHIND=60] [RATIO_OF_NUM_OF_MAX_LOSS_HITS=0.25] [NUM_TOP_LOSSES=5] [STRAT_NAME=ALL_INSTALLED] [ENDDATE=TODAY] [SKIP_DATES_FILE=INVALIDFILE] [STDEV_FACT_=0.01] [results_dir=~/ec2_globalresults]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $today_yyyymmdd_ = `date +%Y%m%d`; chomp ( $today_yyyymmdd_ );

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $days_to_look_behind_ = 60;
my $ratio_ = 0.25;
my $num_top_losses_ = 5;
my $strat_name_ = "";
my $end_date_ = $today_yyyymmdd_;
my $skip_dates_file_ = "INVALIDFILE";
my $stdev_fact_ = 0.01;

if ( $#ARGV > 1 ) { $days_to_look_behind_ = $ARGV [ 2 ]; }
if ( $#ARGV > 2 ) { $ratio_ = $ARGV [ 3 ]; }
if ( $#ARGV > 3 ) { $num_top_losses_ = max ( 1 , $ARGV [ 4 ] ); }
$num_top_losses_ = int ( max(1, min ( $num_top_losses_, $ratio_*$days_to_look_behind_ ) ) ); 

if ( $#ARGV > 4 ) { $strat_name_ = $ARGV [ 5 ]; }

if ( $strat_name_ && !IsPoolStrat($strat_name_) && IsStagedStrat($strat_name_))
{
  $GLOBAL_RESULTS_DIR = $GLOBAL_STAGEDRESULTS_DIR;
}

if (IsValidConfig($strat_name_)){
  $GLOBAL_RESULTS_DIR = "DB";
}

my $strat_type_ = `$WF_SCRIPTS/process_config.py -c $strat_name_ -m VIEW 2>/dev/null | grep -m1 '^STRAT_TYPE:' | awk '{print \$2}'`;
chomp $strat_type_;


if ( $#ARGV > 5 ) { $end_date_ = $ARGV [ 6 ]; }
if ( $#ARGV > 6 ) { $skip_dates_file_ = $ARGV [ 7 ]; }
if ( $#ARGV > 7 ) { $stdev_fact_ = $ARGV [ 8 ]; }
if ( $#ARGV > 8 ) { $GLOBAL_RESULTS_DIR = $ARGV [ 9 ]; }

my $start_date_ = CalcPrevWorkingDateMult ( $end_date_ , $days_to_look_behind_ );
$end_date_ = CalcPrevWorkingDateMult ( $end_date_ , 1 );

if ( ! ( $skip_dates_file_ eq "INVALIDFILE" ) )
{
  open FILE, " < $skip_dates_file_" or die "can't open '$skip_dates_file_': $!";
  my @skip_dates_vector_ = <FILE>;
  close FILE;
  @skip_dates_vector_ = sort {$a <=> $b}  @skip_dates_vector_;
  for ( my $i = $#skip_dates_vector_; $i >= 0; $i = $i - 1 )
  {
    my $date_ = $skip_dates_vector_ [ $i ];
    if ( ( $date_ >= $start_date_ ) && ( $date_ <= $end_date_ ) )
    {
      $start_date_ = CalcPrevWorkingDateMult ( $start_date_ , 1 );
    }
  }
}

my $use_notional_ = 0;

if (( $shortcode_ =~ /^(NSE_|BSE_)/ ) && !($strat_name_ && (IsOptionStrat($strat_name_)))) {
  $use_notional_ = 1;
}

my $trading_location_ = GetTradingLocationForShortcode ( $shortcode_ , $today_yyyymmdd_ );

my @unique_id_ = `date +\%s\%N`; chomp ( @unique_id_ );
my $OML_strats_file_ = "/spare/local/".$USER."/OML_strats_file_".$shortcode_.$unique_id_ [ 0 ];

# Get a list of running queries.
my $SEE_STATS_OF_RUNNING_QUERIES = $SCRIPTS_DIR."/see_stats_of_running_queries.sh";

my @running_queries_ = ( );
GetRunningQueries ( );

my %strat_to_results_ = ( );
GetGlobalResultsForRunningStrats ( );

AdjustUTSforNSE ( );

ChooseMaxLoss ( );

exit ( 0 );

sub GetRunningQueries
{
  if ( $strat_name_ )
  {
    push ( @running_queries_ , $strat_name_ );
    return;
  }
  my $SEE_STATS_OF_RUNNING_QUERIES = $SCRIPTS_DIR."/see_stats_of_running_queries.sh";
  @running_queries_ = `$SEE_STATS_OF_RUNNING_QUERIES $trading_location_ $shortcode_ $days_to_look_behind_ $timeperiod_`;
  for ( my $t_rq_ = 0 ; $t_rq_ <= $#running_queries_ ; $t_rq_++ )
  {
    $running_queries_ [ $t_rq_ ] = GetStratNameFromResultLine ( $running_queries_ [ $t_rq_ ] );
  }
  return;
}

sub AdjustUTSforNSE
{
  if ($use_notional_ == 1) {
    foreach my $running_query_ ( @running_queries_ ) {
      foreach my $res_idx_ ( 0 .. $#{$strat_to_results_{$running_query_}} ) {
        my @result_line_words_ = split ( ' ' , $strat_to_results_{$running_query_}[$res_idx_] );
        my $date_ = $result_line_words_[0];

        my $lastprice_ = `$LIVE_BIN_DIR/get_contract_specs $shortcode_ $date_ LAST_CLOSE_PRICE | awk '{print \$2}'`; chomp $lastprice_;
        next if ! defined $lastprice_ or $lastprice_ eq "";

        $result_line_words_[33] *= $lastprice_;
        $strat_to_results_{$running_query_}[$res_idx_] = join(' ', @result_line_words_);
      }
    }
  }
}

sub GetGlobalResultsForRunningStrats
{
  my $SUMMARIZE_STRATEGY_RESULTS_LONG = $LIVE_BIN_DIR."/summarize_strategy_results";

  open OMLFHANDLE, "> $OML_strats_file_" or PrintStacktraceAndDie ( "Could not open $OML_strats_file_ for writing\n" );

  foreach my $query_ (@running_queries_) 
  {
    if (IsOptionStrat ($query_) and $shortcode_ ne "STKStrats" and $shortcode_ ne "CURStrats") {
      print OMLFHANDLE $query_."_".$shortcode_."\n";
    }
    else {
      print OMLFHANDLE "$query_\n";
    }
  }
  close OMLFHANDLE;

  my @exec_output_ = `$SUMMARIZE_STRATEGY_RESULTS_LONG $shortcode_ $OML_strats_file_ $GLOBAL_RESULTS_DIR $start_date_ $end_date_ $skip_dates_file_ kCNAPnlAdjAverage 0 INVALIDFILE 0 2>/dev/null`;
  chomp @exec_output_;

  $strat_name_ = "";
  foreach my $exec_output_line_ ( @exec_output_ )
  {
    next if $exec_output_line_ eq "" or $exec_output_line_ =~ /^STATISTICS/;
    
    if ($exec_output_line_ =~ /^STRATEGYFILEBASE/) {
      my @exec_output_line_words_ = split /\s+/, $exec_output_line_;

      $strat_name_ = $exec_output_line_words_[1];
      my $tstrat_ = $strat_name_;
      $tstrat_ =~ s/"_".$shortcode_$//g;

      if (IsOptionStrat ($tstrat_) and $shortcode_ ne "STKStrats" and $shortcode_ ne "CURStrats") {
        $strat_name_ = $tstrat_;
      }
      @{ $strat_to_results_ { $strat_name_ } } = ( );
    }
    else {
      push ( @{$strat_to_results_{$strat_name_}} , $exec_output_line_ );
    }
  }
  `rm -rf $OML_strats_file_`;
  return;
}

sub ChooseMaxLoss
{
  my @all_max_loss_values_ = ( );
  my %strat_to_max_loss_to_avg_pnl_ = ( );
  my %strat_to_max_loss_to_num_times_max_loss_hit_ = ( );

  foreach my $strat_name_ ( @running_queries_ )
  {
    my @min_pnl_list_ = ( );
    my @final_pnl_list_ = ( );

    foreach my $result_line_ ( @{$strat_to_results_{$strat_name_}} )
    {
      my @result_line_words_ = split ( ' ' , $result_line_ );
      my $date = $result_line_words_ [ 0 ];
      my $t_min_pnl_ = $result_line_words_ [ 9 ];
      my $t_final_pnl_ = $result_line_words_ [ 1 ];
      my $uts_ = $result_line_words_ [ 33 ];

      # for MRT strats, UTS is currently not supported in resultline
      if ( $strat_type_ ne "MRT" ) {
        next if ! defined $uts_ or $uts_ <= 0;
        $t_min_pnl_ /= $uts_;
        $t_final_pnl_ /= $uts_;
      }

      if ( $t_final_pnl_ < $t_min_pnl_ ) { $t_final_pnl_ = $t_min_pnl_; }
      push ( @min_pnl_list_, $t_min_pnl_ );
      push ( @final_pnl_list_, $t_final_pnl_ );
    }

    my $stdev_ = GetStdev(\@final_pnl_list_);
    my @max_losses_to_try_ = @min_pnl_list_;
    @max_losses_to_try_ = sort { $a <=> $b } ( @max_losses_to_try_ );

# Compute results for each value of max loss.
    foreach my $max_loss_ ( @max_losses_to_try_ )
    {
      $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } = 0;
      $strat_to_max_loss_to_num_times_max_loss_hit_ { $strat_name_ } { $max_loss_ } = 1;

      foreach my $i ( 0 .. $#min_pnl_list_ )
      {
        my $t_min_pnl_ = $min_pnl_list_ [ $i ];
        my $t_final_pnl_ = $final_pnl_list_ [ $i ];

        if ( $t_min_pnl_ < $max_loss_ ) { 
# With this value of max loss , this query would hit max loss and stop
          $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } += $max_loss_;
          $strat_to_max_loss_to_num_times_max_loss_hit_ { $strat_name_ } { $max_loss_ } ++;
        }
        else {
# Query would continue to trade till final pnl
          $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } += $t_final_pnl_;
        }
      }


      if ( @min_pnl_list_ ) { 
        $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } /= ($#min_pnl_list_ + 1); 
      }

      if ( ! FindItemFromVec ( $max_loss_ , @all_max_loss_values_ ) )
      {
        push ( @all_max_loss_values_ , $max_loss_ );
      }
    }

    printf ( "%s =>\n" , $strat_name_ );
    printf ( "%10s %10s %s\n" , "MAX_LOSS" , "AVG_PNL" , "NUM_MAX_LOSS_HITS" );
    my $already_printed_ = 0;
    foreach my $max_loss_ ( sort { if( abs($strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $b } - $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $a }) > $stdev_fact_ * $stdev_) {
                           $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $b } <=> $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $a }
                           } else {
                           $strat_to_max_loss_to_num_times_max_loss_hit_ { $strat_name_ } { $a } <=> $strat_to_max_loss_to_num_times_max_loss_hit_ { $strat_name_ } { $b } } }
                           keys ( % { $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } } ) )
    {
      next if ( max ( $ratio_ * $days_to_look_behind_ , 1 ) < $strat_to_max_loss_to_num_times_max_loss_hit_ {  $strat_name_ } { $max_loss_ } ) ;	    

      if ( ! $use_notional_ ) {
        printf ( "%10.2f %10.2lf %3d\n" , abs ( $max_loss_ ) , $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } , $strat_to_max_loss_to_num_times_max_loss_hit_ {  $strat_name_ } { $max_loss_ } );
      } else {
        printf ( "%10.2e %10.2le %3d\n" , abs ( $max_loss_ ) , $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } , $strat_to_max_loss_to_num_times_max_loss_hit_ {  $strat_name_ } { $max_loss_ } );
      }
      $already_printed_++;
      last if ( $already_printed_ >= $num_top_losses_ ); 
    }	
  }

  if ( $#running_queries_ > 0 )
  {
# Now optimize max-loss across this strat set.
    my %max_loss_to_sum_pnl_ = ( );
    my %max_loss_to_num_times_max_loss_hit_ = ( );

    @all_max_loss_values_ = sort ( @all_max_loss_values_ );

    foreach my $max_loss_ ( @all_max_loss_values_ )
    {
      foreach my $strat_name_ ( keys ( %strat_to_max_loss_to_avg_pnl_ ) )
      {
        my $t_avg_pnl_ = 0;
        my $t_max_loss_diff_ = 999999999999999;
        my $t_num_times_max_loss_hit_ = 0;

        foreach my $s_max_loss_ ( keys ( % { $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } } ) )
        {
# Find the max-loss value "closest" to the one under consideration.
          if ( abs ( $max_loss_ - $s_max_loss_ ) < $t_max_loss_diff_ )
          {
            $t_avg_pnl_ = $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $s_max_loss_ };
            $t_num_times_max_loss_hit_ = $strat_to_max_loss_to_num_times_max_loss_hit_ { $strat_name_ } { $s_max_loss_ };
            $t_max_loss_diff_ = abs ( $max_loss_ - $s_max_loss_ );
          }
        }

        $max_loss_to_sum_pnl_ { $max_loss_ } += $t_avg_pnl_;
        $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } += $t_num_times_max_loss_hit_;
      }
    }

    printf ( "%10s %10s %s\n" , "MAX_LOSS" , "AVG_PNL" , "NUM_MAX_LOSS_HITS" );
    my $already_printed_ = 0;
    foreach my $max_loss_ ( sort { $max_loss_to_sum_pnl_ { $b } <=> $max_loss_to_sum_pnl_ { $a } }
                           keys ( %max_loss_to_sum_pnl_ ) )
    {
      next if ( max( $ratio_ * $days_to_look_behind_ * ( $#running_queries_ + 1 ), 1) < $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } ) ;

      if ( ! $use_notional_ ) {
        printf ( "%10.2f %10.2lf %3d\n" , abs ( $max_loss_ ) , $max_loss_to_sum_pnl_ { $max_loss_ } , $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } ); #/ ( $#running_queries_ + 1 ) );
      } else {
        printf ( "%10.2e %10.2le %3d\n" , abs ( $max_loss_ ) , $max_loss_to_sum_pnl_ { $max_loss_ } , $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } ); #/ ( $#running_queries_ + 1 ) );
      }
      $already_printed_++;
      last if ( $already_printed_ >= ( $num_top_losses_ * ( $#running_queries_ + 1) ) ) ;
    }
  }
  return;
}

sub GetStratNameFromResultLine
{
# 22029 16505 2.65 w_strategy_ilist_BR_DOL_0_US_Mkt_Mkt_J0.noeu_8_na_e3_20111206_20120123_EST_800_EST_1400_500_0_0_fsr.5_3_FSHLR_0.01_0_0_0.7.tt_EST_700_EST_1400.pfi_3
  my $result_line_ = shift;
  my @result_words_ = split ( ' ' , $result_line_ );
  if ( $#result_words_ >= 3 )
  {
    return $result_words_ [ $#result_words_ ];
  }
  else
  {
    return "";
  }
}
