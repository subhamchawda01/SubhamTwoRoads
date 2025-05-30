#!/usr/bin/perl
use strict;
use warnings;
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use Math::Complex; # sqrt
use FileHandle;
use POSIX;
use sigtrap qw(handler signal_handler normal-signals error-signals);

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV{'HOME'};
my $REPO = "basetrade";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LOCAL_TEMP_DIR = "/spare/local/".$USER."/temp_dir";
my $GLOBAL_RESULTS_DIR = "DB"; #changed the primary file location of the globalresults from FS to DB

if ( ! -e $LOCAL_TEMP_DIR ) { `mkdir $LOCAL_TEMP_DIR`; }

require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_query_type_for_id_date.pl"; # GetQueryTypeForIdDate
require "$GENPERLLIB_DIR/get_unix_time_from_utc.pl"; #GetUnixtimeFromUTC
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # RunParallelProcesses
require "$GENPERLLIB_DIR/array_ops.pl";

my $mail_address_ = "nseall@tworoads.co.in";

my $usage_ = "$0 SHORTCODE STRAT_DIR START_DATE END_DATE LOOKBACK USE_DAYS [RESULTS_DIR = DB ] [SortAlgo = kCNAPnlSharpeAverage] [LOCAL_RESULTS = 0]";

if ( $#ARGV < 5 ) {
  print $usage_."\n";
  exit ( 0 );
}

my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( ) ;

my $shortcode_ = $ARGV [ 0 ]; chomp ( $shortcode_ );
my $strat_dir_ = $ARGV [ 1 ]; chomp ( $strat_dir_ );
my $start_date_ = $ARGV [ 2 ]; chomp ( $start_date_ );
my $end_date_ = $ARGV [ 3 ]; chomp ( $end_date_ );
my $lookback_ = $ARGV [ 4 ]; chomp ( $lookback_ );
my $use_days_ = $ARGV [ 5 ]; chomp ( $use_days_ );
my $results_dir_ = "DB";
my $sort_algo_ = "kCNAPnlSharpeAverage";
my $use_local_result_ = 0;

if( $#ARGV > 5 )
{
  $results_dir_ = $ARGV [ 6 ]; chomp ( $results_dir_ );
}

if( $#ARGV > 6 )
{
  $sort_algo_ = $ARGV [ 7 ]; chomp ( $sort_algo_ );
}

if( $#ARGV > 7 )
{
  $use_local_result_ = int($ARGV [ 8 ] > 0) + 0;
}

my $summarize_script_ = "$MODELSCRIPTS_DIR/summarize_strats_for_options.pl";

if($use_local_result_ eq 1)
{
  $summarize_script_ = "$MODELSCRIPTS_DIR/summarize_local_strats_for_options.pl";  
}

my $exec_cmd_ = "$BIN_DIR/calc_prev_week_day $start_date_";
my $temp_end_date_ = `$exec_cmd_`; chomp($temp_end_date_); 

my $strat_start_date_ = $start_date_;

while($temp_end_date_ < $end_date_)
{
  $exec_cmd_ = "$BIN_DIR/calc_prev_week_day $strat_start_date_ $lookback_";
  my $temp_start_date_ = `$exec_cmd_` ; chomp($temp_start_date_);
  my $summarize_command_ = "$summarize_script_ $shortcode_ $strat_dir_ $results_dir_ $temp_start_date_ $temp_end_date_ IF $sort_algo_ | head -n1";
  my $summarize_output_  = `$summarize_command_`;chomp ($summarize_output_);
  $exec_cmd_ =  "$BIN_DIR/calc_next_week_day $temp_end_date_ $use_days_";
  my $strat_end_date_ = `$exec_cmd_`;chomp($strat_end_date_);
  
  if ($summarize_output_ ne "")
  {
    print $summarize_output_."\n";
    my @tokens_ = split(' ',$summarize_output_);
    my $strat_name_ = $tokens_[1];
    print $strat_start_date_." ".$strat_end_date_." ".$strat_name_." ".$temp_start_date_." ".$temp_end_date_."\n";
  }

  $temp_end_date_ = $strat_end_date_;
  $exec_cmd_ = "$BIN_DIR/calc_next_week_day $temp_end_date_";
  $strat_start_date_ = `$exec_cmd_`;chomp($strat_start_date_);
}

