#!/usr/bin/perl

# \file ModelScripts/analyse_eco_events_for_day.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

# ( i ) Call economic_events_of_the_day to get all events for given date.
# ( ii ) Group together economic events by time.
# ( iii ) For each shortcode , for each eco-event-group , 
#         call analyse_eco_events.pl , summarize results and present results.

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub GetEcoEventsForEventDate;
sub GenerateProdStrats;
sub RunSimsForShortcodes;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";

my $TRADELOG_DIR = "/spare/local/logs/tradelogs/"; 
my $AEE_WORK_DIR = $SPARE_HOME."AEE/";

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $SIM_TRADES_LOCATION = "/spare/local/logs/tradelogs/";
my $SIM_LOG_LOCATION = "/spare/local/logs/tradelogs/";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $ORIGINAL_ECO_EVENTS_FILE_LOCATION = $HOME_DIR."/infracore_install/SysInfo/BloombergEcoReports";
my $ORIGINAL_ECO_EVENTS_FILE = $ORIGINAL_ECO_EVENTS_FILE_LOCATION."/merged_eco_2013_processed.txt";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

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

require "$GENPERLLIB_DIR/global_results_methods.pl"; # CombineGlobalResultsLineFromVec

# start
my $USAGE="$0 SHORTCODE1 SHORTCODE2 SHORTCODE3 ... TIMEPERIOD EVENT_DATE START_DATE END_DATE";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my @shortcode_list_ = ( );
for ( my $i = 0 ; $i <= $#ARGV - 4 ; $i ++ )
{
    push ( @shortcode_list_ , $ARGV [ $i ] );
}

my $timeperiod_ = $ARGV [ $#ARGV - 3 ];
my $event_yyyymmdd_ = $ARGV [ $#ARGV - 2 ];
if ( $event_yyyymmdd_ eq "TODAY" )
{
    $event_yyyymmdd_ = `date +%Y%m%d`; chomp ( $event_yyyymmdd_ );
}

my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ $#ARGV - 1 ] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ $#ARGV ] );

my %time_to_eventgroups_ = ( );
GetEcoEventsForEventDate ( );

GenerateProdStrats ( );

my %shortcode_to_eventgroup_to_results_ = ( );
RunSimsForShortcodes ( );

exit ( 0 );

sub GetEcoEventsForEventDate
{
    my $ECONOMIC_EVENTS_OF_THE_DAY = $LIVE_BIN_DIR."/economic_events_of_the_day";

    my $exec_cmd_ = "$ECONOMIC_EVENTS_OF_THE_DAY $event_yyyymmdd_ NYC";

    my @event_lines_ = `$exec_cmd_`; chomp ( @event_lines_ );

    foreach my $event_line_ ( @event_lines_ )
    {
	if ( index ( $event_line_ , "EpochTime" ) >= 0 )
	{
	    next;
	}

	my @event_line_words_ = split ( ' ' , $event_line_ );

	my $t_msecs_ = $event_line_words_ [ 0 ];
	my $t_nyc_time_ = substr ( $event_line_words_ [ 2 ] , 0 , 2 ).substr ( $event_line_words_ [ 2 ] , 3 , 2 ).substr ( $event_line_words_ [ 2 ] , 6 , 2 );
	my $t_event_timezone_ = $event_line_words_ [ 4 ];
	my $t_event_severity_ = $event_line_words_ [ 5 ];
	my $t_event_name_ = $event_line_words_ [ 6 ];

	if ( index ( $timeperiod_ , "US" ) >= 0 &&
	     ( ( $t_nyc_time_ >= 80000 && $t_nyc_time_ <= 163000 ) ) ) # us
	{
	    if ( $t_event_timezone_ eq "USD" ||
		 $t_event_timezone_ eq "EUR" ||
		 $t_event_timezone_ eq "GER" ||
		 $t_event_timezone_ eq "GBP" )
	    {
		if ( $t_event_severity_ > 0 )
		{
		    my $concise_event_name_ = $t_event_timezone_." ".$t_event_name_;
		    push ( @ { $time_to_eventgroups_ { $t_msecs_ } } , $concise_event_name_ );
		}
	    }
	}
    }
    
    return;
}

sub GenerateProdStrats
{
    my $UPDATE_PROD_STRATS = $SCRIPTS_DIR."/update_prod_strats.sh";

    foreach my $shortcode_ ( @shortcode_list_ )
    {
	`$UPDATE_PROD_STRATS $shortcode_`;
    }

    return;
}

sub RunSimsForShortcodes
{
    my $ANALYSE_ECO_EVENTS = $MODELSCRIPTS_DIR."/analyse_eco_events.pl";

    foreach my $shortcode_ ( @shortcode_list_ )
    {
	my $prod_strats_filename_ = $HOME_DIR."/prod_strats/".$shortcode_;

	foreach my $time_ ( sort { $a <=> $b }
			    keys ( %time_to_eventgroups_ ) )
	{
	    my @event_group_ = @ { $time_to_eventgroups_ { $time_ } };

	    my $exec_cmd_ = "$ANALYSE_ECO_EVENTS $shortcode_ $timeperiod_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ $prod_strats_filename_";

	    `rm -rf /spare/local/$USER/AEE/`;

	    foreach my $group_event_ ( @event_group_ )
	    {
		my $t_exec_cmd_ = $exec_cmd_." \"".$group_event_."\"";
		print $t_exec_cmd_."\n";
		`$t_exec_cmd_`;
	    }

	    my $enabled_stats_exec_cmd_ = "grep -h STATISTICS /spare/local/$USER/AEE/*/main_log_file.txt | grep enabled";
	    my @enabled_stats_lines_ = `$enabled_stats_exec_cmd_`; chomp ( @enabled_stats_lines_ );

	    my $enabled_combo_result_line_ = CombineGlobalResultsLineFromVec ( @enabled_stats_lines_ );
	    
	    print "    STOPPING : $enabled_combo_result_line_\n";

	    my $disabled_stats_exec_cmd_ = "grep -h STATISTICS /spare/local/$USER/AEE/*/main_log_file.txt | grep disabled";
	    my @disabled_stats_lines_ = `$disabled_stats_exec_cmd_`; chomp ( @disabled_stats_lines_ );

	    my $disabled_combo_result_line_ = CombineGlobalResultsLineFromVec ( @disabled_stats_lines_ );

	    print "NOT-STOPPING : $disabled_combo_result_line_\n";
	}
    }

    return;
}
