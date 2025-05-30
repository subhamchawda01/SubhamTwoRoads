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
my $REPO = "basetrade";
my $SPARE_HOME = "/spare/local/".$USER."/";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $ORIGINAL_ECO_EVENTS_FILE_LOCATION = $HOME_DIR."/infracore_install/SysInfo/BloombergEcoReports/";
my $ALLOWED_ECO_EVENTS_FILE = $HOME_DIR."/infracore_install/SysInfo/AllowedEconomicEvents/allowed_economic_events.txt";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDateForShortcode
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec


my $USAGE="$0 SHORTCODE STRAT_DIRECTORY/STRATLIST_FILE ECO_EVENT_NAME NUM_DAYS [STOP_SECS=600] [START_SECS=300]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV [ 0 ];
my $base_dir_ = $ARGV [ 1 ];
my $eco_event_name_ = $ARGV [ 2 ];
my $num_days_ = $ARGV [ 3 ];
my $stop_secs_ = -600;
my $start_secs_ = 300;
if ( $#ARGV > 3 ) {
  $stop_secs_ = -1*$ARGV [ 4 ] ;
  $start_secs_ = $ARGV [ 5 ];
}

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $SPARE_HOME.$unique_gsm_id_;
my $end_date_ = `date +%Y%m%d`; chomp ($end_date_);
my $start_date_ = CalcPrevWorkingDateMult ( $end_date_ , $num_days_ );
my $end_yyyy_ = int( $end_date_/10000 );
my $start_yyyy_ = int( $start_date_/10000 );
my @dates = ();
foreach my $yyyy_ ($start_yyyy_ .. $end_yyyy_ )
{
  my $exec_cmd_ = "grep -w ".$eco_event_name_." ".$ORIGINAL_ECO_EVENTS_FILE_LOCATION."merged_eco_".$yyyy_."_processed.txt | cut -d' ' -f 5 | awk -F'_' '{ if(\$1 < $end_date_ && \$1 > $start_date_) print \$1}' | sort -n ";
#  print $exec_cmd_;
  my @tmp_ = `$exec_cmd_`;chomp(@tmp_);
  push(@dates, @tmp_ );
}
#foreach my $t_(@dates){print $t_." ";}

`cp $ALLOWED_ECO_EVENTS_FILE $ALLOWED_ECO_EVENTS_FILE."_bkp"`;

print "Running without stopping at the event\n";
my $local_results_base_dir_ = $work_dir_."/local_results_base_dir_not_stopping";
print $local_results_base_dir_."\n";
my $exec_cmd_ = "sed -i \"\/".$shortcode_." ".$eco_event_name_."\/d\" ".$ALLOWED_ECO_EVENTS_FILE;
#print $exec_cmd_."\n";
`$exec_cmd_`;
foreach my $dt_ (@dates)
{
  $exec_cmd_ = "$MODELSCRIPTS_DIR/run_simulations_to_result_dir.pl $shortcode_ $base_dir_ $dt_ $dt_ ALL $local_results_base_dir_";
  `$exec_cmd_`;
}
print "Running with stopping $stop_secs_ before and starting $start_secs_ secs after the event\n";
$local_results_base_dir_ = $work_dir_."/local_results_base_dir_stopping_".abs($stop_secs_)."_starting_".$start_secs_;
print $local_results_base_dir_."\n";
$exec_cmd_ = "sed -i \"\/".$shortcode_." ".$eco_event_name_."\/d\" ".$ALLOWED_ECO_EVENTS_FILE;
#print $exec_cmd_."\n";
`$exec_cmd_`;
$exec_cmd_ = "echo $shortcode_ $eco_event_name_ $stop_secs_ $start_secs_ > $ALLOWED_ECO_EVENTS_FILE";
`$exec_cmd_`;
foreach my $dt_ (@dates)
{
  $exec_cmd_ = "$MODELSCRIPTS_DIR/run_simulations_to_result_dir.pl $shortcode_ $base_dir_ $dt_ $dt_ ALL $local_results_base_dir_";
  `$exec_cmd_`;
}
`mv $ALLOWED_ECO_EVENTS_FILE."_bkp" $ALLOWED_ECO_EVENTS_FILE`;
