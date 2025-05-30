#!/usr/bin/perl

# \file ModelScripts/analyse_eco_events.pl
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

sub FindValidDates;
sub GenerateStrategyFiles;
sub SanityCheckStratFile;
sub PermuteEcoEventsFiles;
sub RunSimulations;
sub GetResultsForProgIdList;
sub AddResultsToLocalDatabase;
sub SummarizeResultsAndChoose;

sub InstallEcoEventFile;
sub UninstallEcoEvents;
sub CleanIntermediateFiles;

sub AreOtherSimStrategiesRunning;
sub SleepTillOtherSimStrategiesRunning;

sub RemoveLockFileAndExit;

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

my $SIM_STRATEGY_EXEC = $LIVE_BIN_DIR."/sim_strategy";

my $ORIGINAL_ECO_EVENTS_FILE_LOCATION = $HOME_DIR."/infracore_install/SysInfo/BloombergEcoReports";
my $ORIGINAL_ECO_EVENTS_FILE = $ORIGINAL_ECO_EVENTS_FILE_LOCATION."/merged_eco_2013_processed.txt";

my $MAX_SIM_STRATEGY_IN_PARALLEL = 1;

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $MAX_SIM_STRATEGY_IN_PARALLEL = 14;
}

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

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD START_DATE END_DATE STRAT_FILE_LIST ECO_EVENT_NAME [ TO_PLOT_ECO_PERIODS = 1 ]";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 2 ] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 3 ] );
my $strat_file_list_ = $ARGV [ 4 ];
my $eco_event_name_ = $ARGV [ 5 ];

my $mkt_model_ = GetMarketModelForShortcode ( $shortcode_ );

my $to_plot_eco_periods_ = 1;
if ( $#ARGV > 5 )
{
    $to_plot_eco_periods_ = $ARGV [ 6 ];
}

if ( ! ( -d $AEE_WORK_DIR ) ) { `mkdir -p $AEE_WORK_DIR`; }

my $lock_file_ = $AEE_WORK_DIR."a_e_e.lock";

if ( ! -e $lock_file_ )
{
    `touch $lock_file_`;
}
else
{
    print "$lock_file_ present. Another instance might be running , kill it and remove $lock_file_\n";
    exit ( 0 );
}

my $delete_intermediate_files_ = 0;

my @intermediate_files_ = ( );

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $AEE_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	print STDERR "Surprising but this dir exists\n";
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $AEE_WORK_DIR.$unique_gsm_id_; 
    }
    else
    {
	last;
    }
}

my $local_results_base_dir_ = $work_dir_."/local_results_base_dir";
my $temp_results_base_dir_ = $work_dir_."/temp_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $local_eco_events_dir_ = $work_dir_."/eco_events_dir";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;

if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }

if ( ! ( -d $local_results_base_dir_ ) ) { `mkdir -p $local_results_base_dir_`; }
if ( ! ( -d $temp_results_base_dir_ ) ) { `mkdir -p $temp_results_base_dir_`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
if ( ! ( -d $local_eco_events_dir_ ) ) { `mkdir -p $local_eco_events_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or RemoveLockFileAndExit ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush ( 1 );

print "Log file = ".$main_log_file_."\n";

my %date_to_is_valid_ = ( );
FindValidDates ( $shortcode_ , $trading_start_yyyymmdd_ , $trading_end_yyyymmdd_ );

my $local_stratlist_file_name_ = "";
GenerateStrategyFiles ( $strat_file_list_ );

my %date_to_contains_event_ = ( );
my @eco_event_files_list_ = ( );
PermuteEcoEventsFiles ( $eco_event_name_ , $trading_start_yyyymmdd_ , $trading_end_yyyymmdd_ );

my %date_to_eco_event_permute_to_prog_id_list_ = ( );
RunSimulations ( );

my @all_strat_files_list_ = ( );
AddResultsToLocalDatabase ( );

SummarizeResultsAndChoose ( );

UninstallEcoEvents ( );

# CleanIntermediateFiles ( );

$main_log_file_handle_->close;

if ( -e $lock_file_ )
{
    `rm -f $lock_file_`;
}

exit ( 0 );

sub FindValidDates
{
    my ( $t_shortcode_ , $t_trading_start_yyyymmdd_ , $t_trading_end_yyyymmdd_ ) = @_;

    print $main_log_file_handle_ "# FindValidDates ( $t_shortcode_ , $t_trading_start_yyyymmdd_ , $t_trading_end_yyyymmdd_ )\n";

    for ( my $t_date_ = $t_trading_end_yyyymmdd_ ; $t_date_ >= $t_trading_start_yyyymmdd_ ; )
    {
	if ( ValidDate ( $t_date_ ) &&
	     ! SkipWeirdDate ( $t_date_ ) &&
	     ! IsDateHoliday ( $t_date_ ) &&
	     ! NoDataDateForShortcode ( $t_date_ , $t_shortcode_ ) &&
	     ! IsProductHoliday ( $t_date_ , $t_shortcode_ ) )
	{
	    $date_to_is_valid_ { $t_date_ } = 1;
	}

	$t_date_ = CalcPrevWorkingDateMult ( $t_date_ , 1 );
    }

    return;
}

sub GenerateStrategyFiles
{
    my ( $t_strat_file_list_ ) = @_;

    print $main_log_file_handle_ "# GenerateStrategyFiles ( $t_strat_file_list_ )\n";

    # Just sanity check provided stratfilelist and copy to temp local dir
    SanityCheckStratFile ( $t_strat_file_list_ );

    $local_stratlist_file_name_ = $local_strats_dir_."/".basename ( $t_strat_file_list_ );
    my $exec_cmd_ = "cp $t_strat_file_list_ $local_stratlist_file_name_";

    print $main_log_file_handle_ "# GenerateStrategyFiles : $exec_cmd_\n";
    `$exec_cmd_`;

    push ( @intermediate_files_ , $local_stratlist_file_name_ );

    return;
}

sub SanityCheckStratFile
{
    my ( $t_strat_file_list_ ) = @_;

    print $main_log_file_handle_ "# SanityCheckStratFile $t_strat_file_list_\n";

    open ( STRAT_LIST_FILE , "<" , $t_strat_file_list_ ) or RemoveLockFileAndExit ( "Could not open $t_strat_file_list_" );
    my @strat_file_lines_ = <STRAT_LIST_FILE>; chomp ( @strat_file_lines_ );
    close ( STRAT_LIST_FILE );

    my $contains_duplicate_strat_ids_ = 0;
    my %existing_strat_id_ = ( );

    foreach my $t_line_ ( @strat_file_lines_ )
    {
	my @strat_line_words_ = split ( ' ' , $t_line_ );

	my $t_strat_id_ = $strat_line_words_ [ $#strat_line_words_ ];

	if ( exists ( $existing_strat_id_ { $t_strat_id_ } ) )
	{
	    $contains_duplicate_strat_ids_ = 1;
	    last;
	}

	$existing_strat_id_ { $t_strat_id_ } = 1;
    }

    if ( $contains_duplicate_strat_ids_ )
    {
	print $main_log_file_handle_ "# SanityCheckStratFile : contains_duplicate_strat_ids_=".$contains_duplicate_strat_ids_." . Fixing\n";

	open ( STRAT_LIST_FILE , ">" , $t_strat_file_list_ ) or RemoveLockFileAndExit ( "Could not create $t_strat_file_list_" );

	my $last_id_ = 99901;
	foreach my $t_line_ ( @strat_file_lines_ )
	{
	    my @strat_line_words_ = split ( ' ' , $t_line_ );
	    
	    for ( my $i = 0 ; $i < $#strat_line_words_ ; $i ++ )
	    {
		print STRAT_LIST_FILE $strat_line_words_ [ $i ]." ";
	    }

	    print STRAT_LIST_FILE $last_id_."\n";

	    $last_id_ ++;
	}
	
	close ( STRAT_LIST_FILE );
    }

    return;
}

sub PermuteEcoEventsFiles
{
    my ( $t_eco_event_name_ , $t_trading_start_yyyymmdd_ , $t_trading_end_yyyymmdd_ ) = @_;
    print $main_log_file_handle_ "# PermuteEcoEventsFiles ( $t_eco_event_name_ , $t_trading_start_yyyymmdd_ , $t_trading_end_yyyymmdd_ )\n";

    if ( ! ExistsWithSize ( $ORIGINAL_ECO_EVENTS_FILE ) )
    {
	print $main_log_file_handle_ "# PermuteEcoEventsFiles : $ORIGINAL_ECO_EVENTS_FILE doesnot exist\n";
	RemoveLockFileAndExit ( "# PermuteEcoEventsFiles : $ORIGINAL_ECO_EVENTS_FILE doesnot exist." );
    }

    my $exec_cmd_ = "/bin/grep \"$t_eco_event_name_\" $ORIGINAL_ECO_EVENTS_FILE";
    print $main_log_file_handle_ "# PermuteEcoEventsFiles : $exec_cmd_\n";

    my @all_event_occurences_ = `$exec_cmd_`; chomp ( @all_event_occurences_ );

    my @event_occurences_ = ( );
    my %mfm_to_is_event_time_ = ( );

    foreach my $event_line_ ( @all_event_occurences_ )
    {
	my @event_words_ = split ( ' ' , $event_line_ );

	my $t_mfm_ = $event_words_ [ 0 ];

	my @date_time_words_ = split ( '_' , $event_words_ [ $#event_words_ ] );
	my $t_date_ = $date_time_words_ [ 0 ];

	if ( exists ( $date_to_is_valid_ { $t_date_ } ) )
	{
	    push ( @event_occurences_ , $event_line_ );
	    $mfm_to_is_event_time_ { $t_mfm_ } = 1;
	    $date_to_contains_event_ { $t_date_ } = 1;
	}
    }

    if ( $#event_occurences_ < 0 )
    {
	print $main_log_file_handle_ "# PermuteEcoEventsFiles : #event_occurences_=".$#event_occurences_." . Exiting .\n";
	RemoveLockFileAndExit ( "# PermuteEcoEventsFiles : #event_occurences_=".$#event_occurences_." . Exiting ." );
    }

    print $main_log_file_handle_ "# PermuteEcoEventsFiles : event_occurences_= \n\t".join ( "\n\t" , @event_occurences_ )."\n";

    my @valid_events_ = ( );
    foreach my $t_date_ ( sort { $a <=> $b }
			  keys ( %date_to_contains_event_ ) )
    {
	my $exec_cmd_ = "/bin/grep \"$t_date_\" $ORIGINAL_ECO_EVENTS_FILE";
	my @t_date_events_ = `$exec_cmd_`; chomp ( @t_date_events_ );

	push ( @valid_events_ , @t_date_events_ );
    }

    # Create one file with events disabled , another with events enabled.
    my $t_eco_event_enabled_permute_file_name_ = $local_eco_events_dir_."/enabled_eco_2013_processed.txt";
    my $t_eco_event_disabled_permute_file_name_ = $local_eco_events_dir_."/disabled_eco_2013_processed.txt";

    open ( ENABLED_ECO_EVENT_FILE , ">" , $t_eco_event_enabled_permute_file_name_ ) or RemoveLockFileAndExit ( "Could not write $t_eco_event_enabled_permute_file_name_" );
    open ( DISABLED_ECO_EVENT_FILE , ">" , $t_eco_event_disabled_permute_file_name_ ) or RemoveLockFileAndExit ( "Could not write $t_eco_event_disabled_permute_file_name_" );

    foreach my $t_event_line_ ( @valid_events_ )
    {
	my @event_words_ = split ( ' ' , $t_event_line_ );

	my $t_mfm_ = $event_words_ [ 0 ];

	if ( exists ( $mfm_to_is_event_time_ { $t_mfm_ } ) )
	{
	    # Write different severity in different files.
	    $event_words_ [ 3 ] = 3;

	    if ( index ( $shortcode_ , "BR_" ) >= 0 )
	    {
		# BR stuff won't stop on USD events , elevate this event to a BRL event.
		$event_words_ [ 1 ] = "BRL";
	    }

	    if ( index ( $shortcode_ , "FOAT_0" ) >= 0 ||
		 index ( $shortcode_ , "FBTP_0" ) >= 0 ||
                 index ( $shortcode_ , "FBTS_0" ) >= 0 ||
                 index ( $shortcode_ , "FOAM_0" ) >= 0 )
	    {
		# oat/btp stuff won't stop on USD events , elevate this event to an EUR event.
		$event_words_ [ 1 ] = "EUR";
	    }

	    print ENABLED_ECO_EVENT_FILE join ( ' ' , @event_words_ )."\n";

	    $event_words_ [ 3 ] = 0;
	    print DISABLED_ECO_EVENT_FILE join ( ' ' , @event_words_ )."\n";
	}
	else
	{
	    print ENABLED_ECO_EVENT_FILE $t_event_line_."\n";
	    print DISABLED_ECO_EVENT_FILE $t_event_line_."\n";
	}
    }

    close ( ENABLED_ECO_EVENT_FILE );
    close ( DISABLED_ECO_EVENT_FILE );

    print $main_log_file_handle_ "# PermuteEcoEventsFiles : $t_eco_event_enabled_permute_file_name_ , $t_eco_event_disabled_permute_file_name_\n";

    push ( @eco_event_files_list_ , $t_eco_event_enabled_permute_file_name_ );
    push ( @eco_event_files_list_ , $t_eco_event_disabled_permute_file_name_ );

    push ( @intermediate_files_ , $t_eco_event_enabled_permute_file_name_ );
    push ( @intermediate_files_ , $t_eco_event_disabled_permute_file_name_ );

    my $t_local_original_eco_events_file_name_ = $local_eco_events_dir_."/".basename ( $ORIGINAL_ECO_EVENTS_FILE );

    $exec_cmd_ = "cp $ORIGINAL_ECO_EVENTS_FILE $t_local_original_eco_events_file_name_";
    print $main_log_file_handle_ "# PermuteEcoEventsFiles : $exec_cmd_\n";
    `$exec_cmd_`;

    push ( @intermediate_files_ , $t_local_original_eco_events_file_name_ );

    return;
}

sub InstallEcoEventFile
{
    my ( $eco_event_permute_file_ ) = @_;

    print $main_log_file_handle_ "# InstallEcoEventFile ( $eco_event_permute_file_ )\n";

    my $exec_cmd_ = "cp $eco_event_permute_file_ $ORIGINAL_ECO_EVENTS_FILE";
    print $main_log_file_handle_ "$exec_cmd_\n";
    `$exec_cmd_`;

    return;
}

sub UninstallEcoEvents
{
    print $main_log_file_handle_ "# UninstallEcoEvents\n";

    my $t_local_original_eco_events_file_name_ = $local_eco_events_dir_."/".basename ( $ORIGINAL_ECO_EVENTS_FILE );

    my $exec_cmd_ = "cp $t_local_original_eco_events_file_name_ $ORIGINAL_ECO_EVENTS_FILE";
    print $main_log_file_handle_ "# UninstallEcoEvents $exec_cmd_\n";
    `$exec_cmd_`;

    return;
}

sub RunSimulations
{
    print $main_log_file_handle_ "# RunSimulations PARALLEL_SIMSTRATEGIES=".$MAX_SIM_STRATEGY_IN_PARALLEL."\n";

    my $prog_id_ = 66601;
    for my $eco_event_permute_file_ ( @eco_event_files_list_ )
    {
	InstallEcoEventFile ( $eco_event_permute_file_ );

	my @progid_list_ = ( );
	my @sim_exec_cmds_list_ = ( );
	my @date_list_ = ( );

	foreach my $date_ ( keys ( %date_to_contains_event_ ) )
	{
	    my $t_sim_output_file_name_ = $temp_results_base_dir_."/SIMOUT.".$date_.".".$prog_id_;
	    my $exec_cmd_ = "$SIM_STRATEGY_EXEC SIM $local_stratlist_file_name_ $prog_id_ $date_ $mkt_model_ ADD_DBG_CODE -1 > $t_sim_output_file_name_ 2>/dev/null &";

	    # Needed to extract and arrange results later.
	    push ( @ { $date_to_eco_event_permute_to_prog_id_list_ { $date_ } { $eco_event_permute_file_ } } , $prog_id_ );

	    $prog_id_ ++;

	    push ( @progid_list_ , $prog_id_ );
	    push ( @sim_exec_cmds_list_ , $exec_cmd_ );
	    push ( @date_list_ , $date_ );

	    if ( $#progid_list_ == $MAX_SIM_STRATEGY_IN_PARALLEL )
	    {
		for ( my $i = 0 ; $i <= $#progid_list_ ; $i ++ )
		{
		    my $t_prog_id_ = $progid_list_ [ $i ];
		    my $t_exec_cmd_ = $sim_exec_cmds_list_ [ $i ];
		    my $t_date_ = $date_list_ [ $i ];

		    my $t_sim_log_file_ = $SIM_LOG_LOCATION."log.".$t_date_.".".$t_prog_id_;
		    my $t_sim_trades_file_ = $SIM_TRADES_LOCATION."trades.".$t_date_.".".$t_prog_id_;

		    if ( -e $t_sim_trades_file_ ) { `rm -f $t_sim_trades_file_`; }
		    if ( -e $t_sim_log_file_ ) { `rm -f $t_sim_log_file_`; }

		    print $main_log_file_handle_ "# RunSimulations : $t_exec_cmd_\n";

		    system ( $t_exec_cmd_ );
		}

		SleepTillOtherSimStrategiesRunning ( );

		@progid_list_ = ( );
		@sim_exec_cmds_list_ = ( );
		@date_list_ = ( );
	    }
	}

	if ( $#progid_list_ >= 0 )
	{
	    for ( my $i = 0 ; $i <= $#progid_list_ ; $i ++ )
	    {
		my $t_prog_id_ = $progid_list_ [ $i ];
		my $t_exec_cmd_ = $sim_exec_cmds_list_ [ $i ];
		my $t_date_ = $date_list_ [ $i ];

		my $t_sim_log_file_ = $SIM_LOG_LOCATION."log.".$t_date_.".".$t_prog_id_;
		my $t_sim_trades_file_ = $SIM_TRADES_LOCATION."trades.".$t_date_.".".$t_prog_id_;

		if ( -e $t_sim_trades_file_ ) { `rm -f $t_sim_trades_file_`; }
		if ( -e $t_sim_log_file_ ) { `rm -f $t_sim_log_file_`; }

		print $main_log_file_handle_ "# RunSimulations : $t_exec_cmd_\n";

		system ( $t_exec_cmd_ );
	    }

	    SleepTillOtherSimStrategiesRunning ( );

	    @progid_list_ = ( );
	    @sim_exec_cmds_list_ = ( );
	    @date_list_ = ( );
	}
    }

    return;
}

sub CleanIntermediateFiles
{
    print $main_log_file_handle_ "# CleanIntermediateFiles\n";
    if ( $delete_intermediate_files_ )
    {
	chomp ( @intermediate_files_ );

	foreach my $intermediate_file_ ( @intermediate_files_ )
	{
	    my $exec_cmd_ = "rm -f $intermediate_file_";
	    print $main_log_file_handle_ $exec_cmd_."\n";
	    `$exec_cmd_`;
	}
    }

    return;
}

sub GetResultsForProgIdList
{
    my ( $t_date_ , @t_prog_id_list_ ) = @_;

    print $main_log_file_handle_ "# GetResultsForProgIdList ( ".join ( " " , @t_prog_id_list_ )." , $t_date_ )\n";

    my @t_results_list_ = ( );

    my $PNL_STATS_SCRIPT = $MODELSCRIPTS_DIR."/get_pnl_stats_2.pl";

    foreach my $t_prog_id_ ( @t_prog_id_list_ )
    {
	my $t_trades_file_ = $SIM_TRADES_LOCATION."trades.".$t_date_.".".$t_prog_id_;
	my $t_log_file_ = $SIM_LOG_LOCATION."log.".$t_date_.".".$t_prog_id_;

	if ( -e $t_trades_file_ )
	{
	    my $exec_cmd_ = "$PNL_STATS_SCRIPT $t_trades_file_";
	    print $main_log_file_handle_ "# GetResultsForProgIdList $exec_cmd_\n";

	    my @t_results_ = `$exec_cmd_`; chomp ( @t_results_ );

	    my %unique_id_to_pnl_stats_results_ = ( );
	    for ( my $i = 0 ; $i <= $#t_results_ ; $i ++ )
	    {
		my $result_line_ = $t_results_ [ $i ];
		my @result_line_words_ = split ( ' ' , $result_line_ );

		my $unique_id_ = $result_line_words_ [ 0 ];
		splice ( @result_line_words_ , 0 , 1 );

		$unique_id_to_pnl_stats_results_ { $unique_id_ } = join ( ' ' , @result_line_words_ );
	    }

	    my $t_sim_output_file_name_ = $temp_results_base_dir_."/SIMOUT.".$t_date_.".".$t_prog_id_;
	    my @t_sim_out_results_ = `grep SIMRESULT $t_sim_output_file_name_`; chomp ( @t_sim_out_results_ );
	    for ( my $i = 0 ; $i <= $#t_sim_out_results_ ; $i ++ )
	    {
		my $sim_output_line_ = $t_sim_out_results_ [ $i ];
		my @sim_output_words_ = split ( ' ' , $sim_output_line_ );

		splice ( @sim_output_words_ , 0 , 1 );

		my $unique_id_ = GetUniqueSimIdFromCatFile ( $local_stratlist_file_name_ , $i );

		if ( ! exists ( $unique_id_to_pnl_stats_results_ { $unique_id_ } ) )
		{
		    $unique_id_to_pnl_stats_results_ { $unique_id_ } = "0 0 0 0 0 0 0 0 0 0 0 0 0";
		}

		my $combined_result_line_ = $unique_id_." ".join ( ' ' , @sim_output_words_ )." ".$unique_id_to_pnl_stats_results_ { $unique_id_ };
		push ( @t_results_list_ , $combined_result_line_ );
	    }

	    push ( @intermediate_files_ , $t_sim_output_file_name_ );
	    push ( @intermediate_files_ , $t_trades_file_ );
	    push ( @intermediate_files_ , $t_log_file_ );
	}
    }

    return @t_results_list_;
}

sub AddResultsToLocalDatabase
{
    print $main_log_file_handle_ "# AddResultsToLocalDatabase\n";    

    foreach my $date_ ( keys ( %date_to_eco_event_permute_to_prog_id_list_ ) )
    {
	my $eco_event_list_filename_ = $temp_results_base_dir_."/".$date_."_eco_event_list.txt";
	my $results_list_filename_ = $temp_results_base_dir_."/".$date_."_results_list.txt";

	open ( ECO_EVENT_LIST_FILE , ">" , $eco_event_list_filename_ ) or RemoveLockFileAndExit ( "Could not write $eco_event_list_filename_" );
	open ( RESULTS_LIST_FILE , ">" , $results_list_filename_ ) or RemoveLockFileAndExit ( "Could not write $results_list_filename_" );

	foreach my $eco_event_permute_file_ ( keys % { $date_to_eco_event_permute_to_prog_id_list_ { $date_ } } )
	{
	    my @prog_id_results_list_ = GetResultsForProgIdList ( $date_ , @ { $date_to_eco_event_permute_to_prog_id_list_ { $date_ } { $eco_event_permute_file_ } } );

	    foreach my $t_prog_id_result_line_ ( @prog_id_results_list_ )
	    {
		my @prog_id_result_line_words_ = split ( ' ' , $t_prog_id_result_line_ );
		my $t_strat_id_ = $prog_id_result_line_words_ [ 0 ];

		splice ( @prog_id_result_line_words_ , 0 , 1 );

		my $t_strat_file_name_ = basename ( $eco_event_permute_file_ )."_".$t_strat_id_;

		my $t_full_strat_file_name_ = $temp_results_base_dir_."/".$t_strat_file_name_;

		# Create a dummy strategy file corresponding to this permutation.
		if ( ! ExistsWithSize ( $t_full_strat_file_name_ ) )
		{
		    open ( FULL_STRAT_FILE , ">" , $t_full_strat_file_name_ ) or RemoveLockFileAndExit ( "Could not create file $t_full_strat_file_name_" );

		    push ( @intermediate_files_ , $t_full_strat_file_name_ );

		    if ( index ( $t_strat_file_name_ , "enabled" ) >= 0 )
		    {
			print FULL_STRAT_FILE "$t_strat_file_name_ ".$t_strat_id_."0\n";
		    }
		    else
		    {
			print FULL_STRAT_FILE "$t_strat_file_name_ ".$t_strat_id_."1\n";
		    }

		    close ( FULL_STRAT_FILE );
		}	    

		print ECO_EVENT_LIST_FILE $t_full_strat_file_name_."\n";
		if ( index ( $t_strat_file_name_ , "enabled" ) >= 0 )
		{
		    print RESULTS_LIST_FILE join ( ' ' , @prog_id_result_line_words_ )." ".$t_strat_id_."0\n";
		}
		else
		{
		    print RESULTS_LIST_FILE join ( ' ' , @prog_id_result_line_words_ )." ".$t_strat_id_."1\n";
		}

		if ( ! FindItemFromVec ( $t_strat_file_name_ , @all_strat_files_list_ ) )
		{
		    push ( @all_strat_files_list_ , $t_strat_file_name_ );
		}
	    }
	}

	close ( ECO_EVENT_LIST_FILE );
	close ( RESULTS_LIST_FILE );

	my $ADD_RESULTS_SCRIPT = "$MODELSCRIPTS_DIR/add_results_to_local_database.pl";
	my $exec_cmd_ = "$ADD_RESULTS_SCRIPT $eco_event_list_filename_ $results_list_filename_ $date_ $local_results_base_dir_";

	print $main_log_file_handle_ "# AddResultsToLocalDatabase $exec_cmd_\n";
	`$exec_cmd_`;
    }

    return;
}

sub SummarizeResultsAndChoose
{
    print $main_log_file_handle_ "# SummarizeResultsAndChoose\n";

    my $SUMMARIZE_EXEC = $LIVE_BIN_DIR."/summarize_single_strategy_results";

    my %strat_file_to_stats_ = ( );
    foreach my $strat_file_name_ ( @all_strat_files_list_ )
    {
	my $exec_cmd_ = "$SUMMARIZE_EXEC local_results_base_dir $strat_file_name_ $work_dir_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ | grep STATISTICS | awk '{ print \"STATISTICS \"\$2\" \"\$4\" \"\$10\" \"\$11\" \"\$12\" 0 \"\$7\" \"\$7\" 0 0 0 0 0 \"\$6\" 0 \"\$13; }'";
	# print $main_log_file_handle_ "# SummarizeResultsAndChoose : $exec_cmd_\n";

	my @t_stats_ = `$exec_cmd_`; chomp ( @t_stats_ );
	if ( $#t_stats_ < 0 || index ( $t_stats_ [ 0 ] , "STATISTICS" ) < 0 )
	{ # Summarize failed.
	    $exec_cmd_ = "grep -h $strat_file_name_ $local_results_base_dir_"."/*/*/*/results_database.txt | awk '{ \$1=\"\"; \$2=\"STATISTICS\"; print \$0; }'";
	    # print $main_log_file_handle_ "# SummarizeResultsAndChoose : $exec_cmd_\n";
	    @t_stats_ = `$exec_cmd_`; chomp ( @t_stats_ );
	}

	$strat_file_to_stats_ { $strat_file_name_ } = $t_stats_ [ 0 ];
    }

    my %strat_file_name_to_is_printed_once_ = ( );
    foreach my $strat_file_name_ ( @all_strat_files_list_ )
    {
	my $t_enabled_strat_file_name_ = $strat_file_name_;
	my $t_disabled_strat_file_name_ = $strat_file_name_;

	$t_enabled_strat_file_name_ =~ s/disabled/enabled/g;
	$t_disabled_strat_file_name_ =~ s/enabled/disabled/g;

	if ( ! exists ( $strat_file_name_to_is_printed_once_ { $t_enabled_strat_file_name_ } ) )
	{
	    if ( exists ( $strat_file_to_stats_ { $t_enabled_strat_file_name_ } ) &&
		 exists ( $strat_file_to_stats_ { $t_disabled_strat_file_name_ } ) )
	    {
		print $main_log_file_handle_ " $t_enabled_strat_file_name_ ".$strat_file_to_stats_ { $t_enabled_strat_file_name_ }."\n";
		print $main_log_file_handle_ " $t_disabled_strat_file_name_ ".$strat_file_to_stats_ { $t_disabled_strat_file_name_ }."\n";
	    }

	    $strat_file_name_to_is_printed_once_ { $t_enabled_strat_file_name_ } = 1;
	}
    }

    return;
}

sub AreOtherSimStrategiesRunning
{
    my $are_other_instances_running_ = 0;

    my $exec_cmd_ = "ps -efH | grep $USER | grep $SIM_STRATEGY_EXEC | grep $work_dir_ | grep -v grep";

    my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

    foreach my $exec_line_ ( @exec_output_ )
    {
	if ( index ( $exec_line_ , $SIM_STRATEGY_EXEC ) >= 0 )
	{
	    $are_other_instances_running_ = 1;
	    last;
	}
    }

    return $are_other_instances_running_;
}

sub SleepTillOtherSimStrategiesRunning
{
    my $wait_secs_ = 0;
    while ( AreOtherSimStrategiesRunning ( ) )
    {
	sleep ( 1 );
	$wait_secs_ += 4;

	if ( $wait_secs_ > 60 * 20 )
	{ # Running for more than 10 mins , some thing has to be wrong.
	    my @hostname_lines_ = `/bin/hostname`; chomp ( @hostname_lines_ );
	    my $hostname_ = $hostname_lines_ [ 0 ];

	    my $mail_cmd_ = "/bin/cat non_existant_file | /bin/mail -s \"sim_strategy stuck on $hostname_ for $wait_secs_ secs.\" \"sghosh\@circulumvite.com\"";
	    my @mail_output_ = `$mail_cmd_`;

	    $wait_secs_ = - ( 60 * 10 );
	}
    }

    sleep ( 1 ); # Just to be sure.

    return;
}

sub RemoveLockFileAndExit
{
    my ( $msg_ ) = @_;

    if ( -e $lock_file_ )
    {
	`rm -f $lock_file_`;
    }

    PrintStacktraceAndDie ( $msg_ );

    return;
}
