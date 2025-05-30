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
use Mojolicious::Lite;
use Mojo::JSON qw(decode_json encode_json);
use sigtrap qw(handler signal_handler normal-signals error-signals);

sub GetRunningQueries;
sub GetUnitTradeSizesForStrats;
sub GetSimilarVolatileDays;
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
my $BASETRADE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $WF_DB_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward/wf_db_utils";
my $WF_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
  $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $GLOBAL_RESULTS_DIR = $HOME_DIR."/ec2_globalresults";
my $GLOBAL_STAGEDRESULTS_DIR = $HOME_DIR."/ec2_staged_globalresults";
my $hostname_s_ = `hostname -s`; chomp ( $hostname_s_ );
if ( ! ( $USER eq "dvctrader" && ( ( $hostname_s_ eq "sdv-ny4-srv11" ) || ( $hostname_s_ eq "sdv-crt-srv11" ) ) ) )
{
  $GLOBAL_RESULTS_DIR = "/NAS1/ec2_globalresults"; # on non ny4srv11 and crtsrv11 ... look at NAS
  $GLOBAL_STAGEDRESULTS_DIR = "/NAS1/ec2_staged_globalresults"; # on non ny4srv11 and crtsrv11 ... look at NAS
}

my @unique_id_ = `date +\%s\%N`; chomp ( @unique_id_ );
my $OML_DIR = "/spare/local/".$USER."/OML";
if ( ! -d $OML_DIR ) { `mkdir -p $OML_DIR`; }

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/global_results_methods.pl"; # GetPnlFromGlobalResultsLine , GetMinPnlFromGlobalResultsLine
require "$GENPERLLIB_DIR/strat_utils.pl"; # CheckIfRegimeParam
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/config_utils.pl"; # IsValidConfig

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD [DAYS_TO_LOOK_BEHIND=60] [RATIO_OF_NUM_OF_MAX_LOSS_HITS=0.25] [NUM_TOP_LOSSES=5] [SIMILAR_DAYS_FRAC=0.4] [STRAT_NAME=ALL_INSTALLED] [ENDDATE=TODAY] [SKIP_DATES_FILE=INVALIDFILE] [results_dir=~/ec2_globalresults]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $today_yyyymmdd_ = `date +%Y%m%d`; chomp ( $today_yyyymmdd_ );

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $days_to_look_behind_ = 60;
my $ratio_ = 0.25;
my $num_top_losses_ = 5;
my $similar_frac_ = 0.4;
my $strat_name_ = "";
my $end_date_ = $today_yyyymmdd_;
my $skip_dates_file_ = "INVALIDFILE";

if ( $#ARGV > 1 ) { $days_to_look_behind_ = $ARGV [ 2 ]; }
if ( $#ARGV > 2 ) { $ratio_ = $ARGV [ 3 ]; }
if ( $#ARGV > 3 ) { $num_top_losses_ = max ( 1 , $ARGV [ 4 ] ); }
if ( $#ARGV > 4 ) { $similar_frac_ = max( 0.1, $ARGV [ 5 ] ); }
$num_top_losses_ = int ( max(1, min ( $num_top_losses_, $ratio_*$days_to_look_behind_ ) ) ); 

if ( $#ARGV > 5 ) { $strat_name_ = $ARGV [ 6 ]; }

if ( $strat_name_ && !IsPoolStrat($strat_name_) && IsStagedStrat($strat_name_))
{
  $GLOBAL_RESULTS_DIR = $GLOBAL_STAGEDRESULTS_DIR;
}

if ( $#ARGV > 6 ) { $end_date_ = $ARGV [ 7 ]; }
if ( $#ARGV > 7 ) { $skip_dates_file_ = $ARGV [ 8 ]; }
if ( $#ARGV > 8 ) { $GLOBAL_RESULTS_DIR = $ARGV [ 9 ]; }

my $OML_strats_file_ = $OML_DIR."/strats_file_".$shortcode_."_".$unique_id_[0];

my $trading_location_ = GetTradingLocationForShortcode ( $shortcode_ , $today_yyyymmdd_ );

# Get a list of running queries.
my $SEE_STATS_OF_RUNNING_QUERIES = $SCRIPTS_DIR."/see_stats_of_running_queries.sh";

my @running_queries_ = ( );
GetRunningQueries ( );

my @similar_days_ = ( );
GetSimilarVolatileDays ( $shortcode_, $end_date_, $days_to_look_behind_, $similar_frac_, \@similar_days_ );
my $num_similar_days_ = @similar_days_;

my $dates_file_ = $OML_DIR."/dates_file_".$shortcode_."_".$unique_id_[0];
open DATESFHANDLE, "> $dates_file_" or PrintStacktraceAndDie ( "Could not open $dates_file_ for writing" );
print DATESFHANDLE join("\n", @similar_days_)."\n";
close DATESFHANDLE;

my $start_date_ = min @similar_days_;

my %strat_to_results_ = ( );
GetGlobalResultsForRunningStrats ( );

my %strat_to_unit_trade_size_ = ( );
GetUnitTradeSizesForStrats ( );

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

sub GetUnitTradeSizesForStrats
{ 
  foreach my $running_query_ ( @running_queries_ )
  {
    foreach my $result_line_ ( @{ $strat_to_results_ { $running_query_ } } )
    {
      my @result_line_words_ = split ( ' ' , $result_line_ );
      my $date_ = $result_line_words_ [ 0 ];
      my $unit_trade_size_ = 1;
      if ( IsStructuredQuery ( $running_query_) )
      {
        my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$shortcode_." ".$running_query_." ".$date_." 1 ";
        $unit_trade_size_ = `$exec_cmd_`;
      }
      elsif(IsValidConfig($running_query_))
      {
      	my $paramname = "INVALID";
      	my $cmd = $WF_SCRIPTS."/print_strat_from_config.py -c $running_query_ -d $date_";
      	my $strategy_line = `$cmd`; chomp($cmd);
      	if ( $strategy_line ) {
      		my @strategy_line_words = split(' ', $strategy_line);
      		if ($#strategy_line_words >= 4){
      			$paramname = $strategy_line_words[4];
      		}
      	}
				#print "PARAM: $paramname $date_\n";
		my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$shortcode_." ".$paramname." ".$date_." 2 ".$use_notional_;
		$unit_trade_size_ = `$exec_cmd_`;
      }
      else
      {
        my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$shortcode_." ".$running_query_." ".$date_;
        $unit_trade_size_ = `$exec_cmd_`;
      }
      chomp ( $unit_trade_size_ );
      $strat_to_unit_trade_size_ { $running_query_ }{ $date_ } = $unit_trade_size_;
    }
  }
  return;
}

sub IsStructuredQuery
{
  my $STRAT_FILE_PATH = $HOME_DIR."/modelling/stir_strats/".$shortcode_."/".$_ [ 0 ];
  if (-e $STRAT_FILE_PATH)
  {
    return $STRAT_FILE_PATH;
  }
  else
  {
    my $nm=$_[0];
    $STRAT_FILE_PATH = `ls $HOME_DIR/modelling/stir_strats/*/*/$nm 2>/dev/null`; chomp ( $STRAT_FILE_PATH ) ;
    if ( -e $STRAT_FILE_PATH )
    {
      return $STRAT_FILE_PATH;
    } 
    else
    {
      return "";
    }
  }
}

sub GetGlobalResultsForRunningStrats
{
  my $SUMMARIZE_STRATEGY_RESULTS_LONG = $BASETRADE_BIN_DIR."/summarize_strategy_results";
  my $file_handle_ = FileHandle->new;
  $file_handle_->open ( "> $OML_strats_file_ " ) or PrintStacktraceAndDie ( "Could not open $OML_strats_file_ for writing\n" );
  for ( my $i_ = 0; $i_ <= $#running_queries_; $i_++ )
  {
    print $file_handle_ "$running_queries_[ $i_ ]\n";
  }
  $file_handle_->close;
  my @exec_output_ = `$SUMMARIZE_STRATEGY_RESULTS_LONG $shortcode_ $OML_strats_file_ $GLOBAL_RESULTS_DIR $start_date_ $end_date_ $skip_dates_file_ kCNAPnlAdjAverage 0 $dates_file_ 0 2>/dev/null `;
  my $strat_name_ = "";
  foreach my $exec_output_line_ ( @exec_output_ )
  {
    if ( $exec_output_line_ eq "\n" || index ( $exec_output_line_ , "STATISTICS" ) >= 0 )
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

    foreach my $result_line_ ( @ { $strat_to_results_ { $strat_name_ } } )
    {
      my @result_line_words_ = split ( ' ' , $result_line_ );
      my $date = $result_line_words_ [ 0 ];
      my $t_min_pnl_ = $result_line_words_ [ 9 ];
      my $t_final_pnl_ = $result_line_words_ [ 1 ];
      $t_min_pnl_ /= $strat_to_unit_trade_size_ { $strat_name_ }{ $date };
      $t_final_pnl_ /= $strat_to_unit_trade_size_ { $strat_name_ }{ $date };
      push ( @min_pnl_list_ , $t_min_pnl_ );
      push ( @final_pnl_list_ , $t_final_pnl_ );
    }

    my @max_losses_to_try_ = @min_pnl_list_;
    @max_losses_to_try_ = sort { $a <=> $b } ( @max_losses_to_try_ );
# Compute results for each value of max loss.
    foreach my $max_loss_ ( @max_losses_to_try_ )
    {

      $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } = 0;
      $strat_to_max_loss_to_num_times_max_loss_hit_ { $strat_name_ } { $max_loss_ } = 1;

      for ( my $i = 0 ; $i <= $#min_pnl_list_ ; $i ++ )
      {
        my $t_min_pnl_ = $min_pnl_list_ [ $i ];
        my $t_final_pnl_ = $final_pnl_list_ [ $i ];

        if ( $t_min_pnl_ < $max_loss_ )
        { 
# With this value of max loss , this query would hit max loss and stop
          $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } += $max_loss_;
          $strat_to_max_loss_to_num_times_max_loss_hit_ { $strat_name_ } { $max_loss_ } ++;
        }
        else
        { 
# Query would continue to trade till final pnl
          $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } += $t_final_pnl_;
        }
      }
      if ( @min_pnl_list_ ) { $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } /= ($#min_pnl_list_ + 1); }

      if ( ! FindItemFromVec ( $max_loss_ , @all_max_loss_values_ ) )
      {
        push ( @all_max_loss_values_ , $max_loss_ );
      }
    }

    printf ( "%s =>\n" , $strat_name_ );
    printf ( "%10s %10s %s\n" , "MAX_LOSS" , "AVG_PNL" , "NUM_MAX_LOSS_HITS" );
    my $already_printed_ = 0;
    foreach my $max_loss_ ( sort { $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $b } <=> $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $a } }
        keys ( % { $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } } ) )
    {
      next if ( max ( $ratio_ * $num_similar_days_ , 1 ) < $strat_to_max_loss_to_num_times_max_loss_hit_ {  $strat_name_ } { $max_loss_ } ) ;	    

      printf ( "%10.2f %10.2lf %3d\n" , abs ( $max_loss_ ) , $strat_to_max_loss_to_avg_pnl_ { $strat_name_ } { $max_loss_ } , $strat_to_max_loss_to_num_times_max_loss_hit_ {  $strat_name_ } { $max_loss_ } );
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
      next if ( max( $ratio_ * $num_similar_days_ * ( $#running_queries_ + 1 ), 1) < $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } ) ;

      printf ( "%10.2f %10.2lf %3d\n" , abs ( $max_loss_ ) , $max_loss_to_sum_pnl_ { $max_loss_ } , $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } ); #/ ( $#running_queries_ + 1 ) );
      $already_printed_++;
      last if ( $already_printed_ >= ( $num_top_losses_ * ( $#running_queries_ + 1) ) ) ;
    }
  }
  return;
}

sub GetVolatileSimilarityMap
{
  my $shortcode_ = shift;
  my $yyyymmdd_ = shift;
  my $lookback_days_ = shift;
  my $factor_ = shift;
  my $similarity_ref_ = shift;

  my $prev_yyyymmdd_ = CalcPrevWorkingDateMult ( $yyyymmdd_, 1 );

  my $config_file_ = $OML_DIR."/stdev_feature_".$shortcode_;
  open CHANDLE, "> $config_file_" or PrintStacktraceAndDie ("Could not open $config_file_ for writing\n");
  print CHANDLE "$shortcode_ STDEV\n";
  close CHANDLE;

  my $feature_file_ = $OML_DIR."/features_stdev_".$shortcode_;
  my $generate_features_exec_ = $HOME_DIR."/".$REPO."/WKoDii/get_day_features.pl";
  my $generate_features_cmd_ = "$generate_features_exec_ $shortcode_ $prev_yyyymmdd_ $lookback_days_ $config_file_ > $feature_file_";
  print $generate_features_cmd_."\n";
  `$generate_features_cmd_ 2>/dev/null`;

  my $similarity_exec_ = "python ".$HOME_DIR."/".$REPO."/WKoDii/obtain_weights_on_days.py";
  my $similarity_exec_cmd_ = "$similarity_exec_ $yyyymmdd_ -1 0 $feature_file_ ARIMA_DEF Euclidean";
  print $similarity_exec_cmd_."\n";

  my $tradingdates_similarity_json_string_ = `$similarity_exec_cmd_ 2>/dev/null`; chomp( $tradingdates_similarity_json_string_ );
#  print $tradingdates_similarity_json_string_."\n";
  my $tradingdates_similarity_ref_ = decode_json $tradingdates_similarity_json_string_ ||
    PrintStacktraceAndDie ( "Error in finding_similar_days\n Json string to decode: ".$tradingdates_similarity_json_string_."\n$!" ) ;
  if ( ! exists $$tradingdates_similarity_ref_{ $prev_yyyymmdd_ } ) {
    print "Warning: DayFeatures could not be generated for Yesterday: $prev_yyyymmdd_\n Probably the SampleData for $prev_yyyymmdd_ does NOT exist\n Continuing without that\n";
  }
  %$similarity_ref_ = %$tradingdates_similarity_ref_;
}

sub GetSimilarVolatileDays
{
  my $shortcode_ = shift;
  my $yyyymmdd_ = shift;
  my $lookback_days_ = shift;
  my $factor_ = shift;
  my $similar_days_ref_ = shift;

  my %tradingdates_similarity_map_ = ( );
  GetVolatileSimilarityMap ( $shortcode_, $yyyymmdd_, $lookback_days_, $factor_, \%tradingdates_similarity_map_ );

  my @valid_tradingdates_ = grep { $tradingdates_similarity_map_{ $_ } > 0 } keys %tradingdates_similarity_map_;
  my @valid_tradingdates_sorted_ = sort { $tradingdates_similarity_map_{ $a } <=> $tradingdates_similarity_map_{ $b } } keys %tradingdates_similarity_map_;

  my $cutoff_idx_ = max( 0, min( (1 - $factor_) * @valid_tradingdates_sorted_, $#valid_tradingdates_sorted_ ) );

  @$similar_days_ref_ = sort @valid_tradingdates_sorted_[ $cutoff_idx_..$#valid_tradingdates_sorted_ ];
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
