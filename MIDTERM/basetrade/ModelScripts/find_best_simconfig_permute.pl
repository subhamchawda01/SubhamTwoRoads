#!/usr/bin/perl

# \file ModelScripts/find_best_simconfig_permute.pl
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

sub GetParamsAndModels;
sub GetSimConfigFiles;
sub PruneRedundantSimConfigFiles;
sub InstallAccurateEcoEvents;
sub CountTotalSimStratsToBeRun;
sub GenerateStrategyFilesAndRunSimulations;
sub AddResultsToLocalDatabase;
sub UninstallAccurateEcoEvents;
sub CleanIntermediateFiles;

sub CreateStratFile;

sub GetStartTimeFromStratFileLine;

sub GetDateFromStratFile;
sub GetSimConfigFromStratFile;
sub GetSimConfigDescriptionFromFile;

sub GetSimrealResults;
sub CombineSimrealResults;

sub AreOtherSimStrategiesRunning;
sub SleepTillOtherSimStrategiesRunning;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";

my $TRADELOG_DIR = "/spare/local/logs/tradelogs/"; 
my $FBSCP_WORK_DIR = $SPARE_HOME."FBSCP/";

my $REPO = "basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $SIM_TRADES_LOCATION = "/spare/local/logs/tradelogs/";
my $SIM_LOG_LOCATION = "/spare/local/logs/tradelogs/";

my $ORIGINAL_ECO_EVENTS_FILE_LOCATION = $HOME_DIR."/infracore_install/SysInfo/BloombergEcoReports";
my $ORIGINAL_ECO_EVENTS_FILE = $ORIGINAL_ECO_EVENTS_FILE_LOCATION."/merged_eco_2012_processed.txt";
my $ACCURATE_ECO_EVENTS_FILE = $HOME_DIR."/".$REPO."/SysInfo/TradingInfo/SimConfig/accurate_merged_eco_2012_processed.txt";

my $MAX_SIM_STRATEGY_IN_PARALLEL = 14;

require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec
require "$GENPERLLIB_DIR/search_script.pl"; # SearchScript
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams

require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD START_DATE END_DATE SIMCONFIG_PERMUATION_FILE [ PLOT=1 ]";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 2 ] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 3 ] );
my $simconfig_permutation_file_ = $ARGV [ 4 ];

my $mkt_model_ = GetMarketModelForShortcode ( $shortcode_ );

my $to_plot_ = 1;
if ( $#ARGV > 4 )
{
    $to_plot_ = $ARGV [ 5 ];
}

if ( ! ( -d $FBSCP_WORK_DIR ) ) { `mkdir -p $FBSCP_WORK_DIR`; }

my $lock_file_ = $FBSCP_WORK_DIR."f_b_sc_p.lock";

if ( ! -e $lock_file_ )
{
    `touch $lock_file_`;
}
else
{
    print "$lock_file_ present. Another instance might be running , kill it and remove $lock_file_\n";
    exit ( 0 );
}

my $delete_intermediate_files_ = 1;

my @intermediate_files_ = ( );
my %date_to_sim_id_to_real_id_ = ( );
my %date_to_strat_file_list_ = ( );
my $total_num_sim_strategies_to_be_run_ = 0;
my $total_num_sim_strategies_already_run_ = 0;
my %strat_file_to_simreal_results_ = ( );
# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $FBSCP_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	print STDERR "Surprising but this dir exists\n";
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $FBSCP_WORK_DIR.$unique_gsm_id_; 
    }
    else
    {
	last;
    }
}

my $local_results_base_dir_ = $work_dir_."/local_results_base_dir";
my $temp_results_base_dir_ = $work_dir_."/temp_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $local_params_dir_ = $work_dir_."/params_dir";
my $local_models_dir_ = $work_dir_."/models_dir";
my $local_simconfigs_dir_ = $work_dir_."/simconfigs_dir";
my $local_tradesdiff_dir_ = $work_dir_."/tradesdiffs_dir";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;

if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir_ ) ) { `mkdir -p $local_results_base_dir_`; }
if ( ! ( -d $temp_results_base_dir_ ) ) { `mkdir -p $temp_results_base_dir_`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }
if ( ! ( -d $local_models_dir_ ) ) { `mkdir -p $local_models_dir_`; }
if ( ! ( -d $local_simconfigs_dir_ ) ) { `mkdir -p $local_simconfigs_dir_`; }
if ( ! ( -d $local_tradesdiff_dir_ ) ) { `mkdir -p $local_tradesdiff_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush ( 1 );

print "Log file = ".$main_log_file_."\n";

my @sim_config_file_list_ = ( );
GetSimConfigFiles ( );
if ( $#sim_config_file_list_ < 0 )
{
    print $main_log_file_handle_ "Exiting due to empty sim_config_file_list_\n";
}
else
{

  PruneRedundantSimConfigFiles ( );

  GetParamsAndModels ( );

  InstallAccurateEcoEvents ( );

  CountTotalSimStratsToBeRun ( );

  GenerateStrategyFilesAndRunSimulations ( );

  AddResultsToLocalDatabase ( );

  UninstallAccurateEcoEvents ( );

  CleanIntermediateFiles ( );
}

$main_log_file_handle_->close;

if ( -e $lock_file_ )
{
    `rm -f $lock_file_`;
}

exit ( 0 );

sub GetParamsAndModels
{
    print $main_log_file_handle_ "# GetParamsAndModels\n";

    # Run the accurate_sim_real script for the specified
    # days and the specified shortcode to get
    # the accurate strategy files.

    my $ACCURATE_SIM_REAL_SCRIPT = SearchScript ( "see_accurate_sim_real_for_prod_nograph.sh", () ) ;

    for ( my $trading_yyyymmdd_ = $trading_end_yyyymmdd_ ; $trading_yyyymmdd_ >= $trading_start_yyyymmdd_ ; )
    {
	if ( ! ValidDate ( $trading_yyyymmdd_ ) ||
	     SkipWeirdDate ( $trading_yyyymmdd_ ) ||
	     IsDateHoliday ( $trading_yyyymmdd_ ) ||
	     IsWeirdSimDayForShortcode ( $shortcode_ , $trading_yyyymmdd_ ) )
	{
	    $trading_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_yyyymmdd_ , 1 );
	    next;
	}

	my $t_local_strats_dir_ = $local_strats_dir_."/".$trading_yyyymmdd_;
	my $t_local_params_dir_ = $local_params_dir_."/".$trading_yyyymmdd_;
	my $t_local_models_dir_ = $local_models_dir_."/".$trading_yyyymmdd_;

	my $exec_cmd_ = "$ACCURATE_SIM_REAL_SCRIPT $timeperiod_ $shortcode_ $trading_yyyymmdd_ 0 $t_local_strats_dir_ $t_local_params_dir_ $t_local_models_dir_";
	print $main_log_file_handle_ "# ".$exec_cmd_."\n";

	my @accurate_output_ = `$exec_cmd_`; chomp ( @accurate_output_ );
	print $main_log_file_handle_ "# accurate_output_ = \n".join ( "\n" , @accurate_output_ )."\n";

	# Create the mapping from sim_id -> real_id
	my $t_real_id_ = -1;
	my $t_sim_id_ = -1;
	foreach my $accurate_output_line_ ( @accurate_output_ )
	{
	    if ( index ( $accurate_output_line_ , "REALID " ) >= 0 )
	    {
		my @accurate_output_words_ = split ( ' ' , $accurate_output_line_ );
		$t_real_id_ = $accurate_output_words_ [ 1 ];
	    }
	    if ( index ( $accurate_output_line_ , "SIMID " ) >= 0 )
	    {
		my @accurate_output_words_ = split ( ' ' , $accurate_output_line_ );
		$t_sim_id_ = $accurate_output_words_ [ 1 ];

		if ( $t_sim_id_ > -1 && $t_real_id_ > -1 )
		{
		    $date_to_sim_id_to_real_id_ { $trading_yyyymmdd_ } { $t_sim_id_ } = $t_real_id_;
		}

		$t_sim_id_ = -1 ; $t_real_id_ = -1;
	    }
	}

	my @t_strat_file_list_ = `ls $t_local_strats_dir_ 2>/dev/null`; chomp ( @t_strat_file_list_ );
	my @t_param_file_list_ = `ls $t_local_params_dir_ 2>/dev/null`; chomp ( @t_param_file_list_ );
	my @t_model_file_list_ = `ls $t_local_models_dir_ 2>/dev/null`; chomp ( @t_model_file_list_ );

	print $main_log_file_handle_ "# t_strat_file_list_ = \n".join ( "\n" , @t_strat_file_list_ )."\n";
	print $main_log_file_handle_ "# t_param_file_list_ = \n".join ( "\n" , @t_param_file_list_ )."\n";
	print $main_log_file_handle_ "# t_model_file_list_ = \n".join ( "\n" , @t_model_file_list_ )."\n";

	foreach my $t_strat_file_ ( @t_strat_file_list_ )
	{
	    push ( @ { $date_to_strat_file_list_ { $trading_yyyymmdd_ } } , $t_strat_file_ );
	}

	$trading_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_yyyymmdd_ , 1 );
    }
}

sub GetSimConfigFiles
{
    print $main_log_file_handle_ "# GetSimConfigFiles\n";
    @sim_config_file_list_ = PermuteParams ( $simconfig_permutation_file_ , $local_simconfigs_dir_ );
    chomp ( @sim_config_file_list_ );

    print $main_log_file_handle_ "# PermuteParams = ".$#sim_config_file_list_."\n";

    return;
}

sub PruneRedundantSimConfigFiles
{
    print $main_log_file_handle_ "# PruneRedundantSimConfigFiles\n";
    my @original_sim_config_file_list_ = @sim_config_file_list_;

    @sim_config_file_list_ = ( );
    my %sim_config_description_to_is_added_ = ( );

    foreach my $sim_config_file_name_ ( @original_sim_config_file_list_ )
    {
	my $sim_config_description_ = GetSimConfigDescriptionFromFile ( $sim_config_file_name_ );

	# Check if this sim_config has already been added to the list
	# of permutations to run on.
	if ( ! exists ( $sim_config_description_to_is_added_ { $sim_config_description_ } ) )
	{
	    push ( @sim_config_file_list_ , $sim_config_file_name_ );
	    $sim_config_description_to_is_added_ { $sim_config_description_ } = 1;
	}
    }

    print $main_log_file_handle_ "# PruneRedundantSimConfigFiles = ".$#sim_config_file_list_."\n";

    return;
}

sub GetSimConfigDescriptionFromFile
{
    my ( $sim_config_file_name_ ) = @_;

    my $SIM_CONFIG_TO_DESCRIPTION_EXEC = SearchExec ( "get_sim_config_description_from_file", () ) ;
    my $exec_cmd_ = "$SIM_CONFIG_TO_DESCRIPTION_EXEC $shortcode_ $sim_config_file_name_";
    my @sim_config_description_lines_ = `$exec_cmd_`; chomp ( @sim_config_description_lines_ );

    # print "sim_config_description_lines_ = ".join ( "\n" , @sim_config_description_lines_ )."\n";

    my $sim_config_description_ = "";
    foreach my $sim_config_line_ ( @sim_config_description_lines_ )
    {
	if ( index ( $sim_config_line_ , $shortcode_ ) >= 0 && 
	     index ( $sim_config_line_ , "[ " ) >= 0 &&
	     index ( $sim_config_line_ , "UASTC" ) >= 0 &&
	     index ( $sim_config_line_ , " | " ) >= 0 )
	{
	    $sim_config_description_ = $sim_config_line_;
	    last;
	}
    }

    # print "sim_config_description_ = ".$sim_config_description_."\n";

    return $sim_config_description_;
}

sub InstallAccurateEcoEvents
{
    print $main_log_file_handle_ "# InstallAccurateEcoEvents\n";

    my $exec_cmd_ = "cp $ORIGINAL_ECO_EVENTS_FILE $work_dir_";
    print $main_log_file_handle_ "# InstallAccurateEcoEvents : ".$exec_cmd_."\n";
    `$exec_cmd_`;

    if ( ExistsWithSize ( $ACCURATE_ECO_EVENTS_FILE ) )
    {
	$exec_cmd_ = "cp $ACCURATE_ECO_EVENTS_FILE $ORIGINAL_ECO_EVENTS_FILE";
	print $main_log_file_handle_ "# InstallAccurateEcoEvents : ".$exec_cmd_."\n";
	`$exec_cmd_`;
    }

    return;
}

sub UninstallAccurateEcoEvents
{
    print $main_log_file_handle_ "# UninstallAccurateEcoEvents\n";

    if ( ExistsWithSize ( $ACCURATE_ECO_EVENTS_FILE ) )
    {
	my $t_original_eco_file_ = $work_dir_."/merged_eco_2012_processed.txt";

	my $exec_cmd_ = "cp $t_original_eco_file_ $ORIGINAL_ECO_EVENTS_FILE";
	print $main_log_file_handle_ "# UninstallAccurateEcoEvents : ".$exec_cmd_."\n";
	`$exec_cmd_`;
    }

    return;
}

sub CountTotalSimStratsToBeRun
{
    print $main_log_file_handle_ "# CountTotalSimStratsToBeRun\n";

    $total_num_sim_strategies_to_be_run_ = 0;

    foreach my $date_ ( sort { $date_to_strat_file_list_ { $a } <=> $date_to_strat_file_list_ { $b } }
			keys %date_to_strat_file_list_ )
    {
	my @t_strat_file_list_ = @ { $date_to_strat_file_list_ { $date_ } };

	for ( my $t_strat_file_index_ = 0 ; $t_strat_file_index_ <= $#t_strat_file_list_ ; $t_strat_file_index_ ++ )
	{
	    for ( my $t_sim_config_file_index_ = 0 ; $t_sim_config_file_index_ <= $#sim_config_file_list_ ; $t_sim_config_file_index_ ++ )
	    {
		$total_num_sim_strategies_to_be_run_ ++;
	    }
	}
    }

    print $main_log_file_handle_ "# CountTotalSimStratsToBeRun=".$total_num_sim_strategies_to_be_run_."\n";

    return ;
}

sub GenerateStrategyFilesAndRunSimulations
{
    print $main_log_file_handle_ "# GenerateStrategyFiles PARALLEL_SIMSTRATEGIES = ".$MAX_SIM_STRATEGY_IN_PARALLEL."\n";

    my $SIM_STRATEGY_EXEC = SearchExec ( "sim_strategy", () ) ;

    my $unique_offset_ = `date +%N`;
    my $strategy_progid_ = 9901;

    foreach my $date_ ( sort { $date_to_strat_file_list_ { $a } <=> $date_to_strat_file_list_ { $b } }
			keys %date_to_strat_file_list_ )
    {
	my @t_strat_file_list_ = @ { $date_to_strat_file_list_ { $date_ } };
	my @permuted_strat_file_list_ = ( );

	for ( my $t_strat_file_index_ = 0 ; $t_strat_file_index_ <= $#t_strat_file_list_ ; $t_strat_file_index_ ++ )
	{
	    my $t_strat_file_name_ = $local_strats_dir_."/".$date_."/".$t_strat_file_list_ [ $t_strat_file_index_ ];

	    open ( IN_STRAT_FILE , "<" , $t_strat_file_name_ ) or PrintStacktraceAndDie ( "Could not open file $t_strat_file_name_" );
	    my @strat_file_lines_ = <IN_STRAT_FILE>; chomp ( @strat_file_lines_ );
	    close ( IN_STRAT_FILE );

	    my $strat_file_line_ = $strat_file_lines_ [ 0 ];

	    for ( my $t_sim_config_file_index_ = 0 ; $t_sim_config_file_index_ <= $#sim_config_file_list_ ; $t_sim_config_file_index_ ++ )
	    {
		my $t_sim_config_file_name_ = $sim_config_file_list_ [ $t_sim_config_file_index_ ];

		my $t_start_time_ = GetStartTimeFromStratFileLine ( $strat_file_line_ );

		my $t_strategy_file_name_ = $local_strats_dir_."/strat_dt.".$date_."_stime.".$t_start_time_."_sfi.".$t_strat_file_index_."_scfi.".$t_sim_config_file_index_."_progid.".$strategy_progid_;

		my $old_sim_id_ = CreateStratFile ( $strat_file_line_ , $t_strategy_file_name_ , $strategy_progid_ , $t_sim_config_file_name_ );

		if ( ! exists ( $date_to_sim_id_to_real_id_ { $date_ } { $old_sim_id_ } ) )
		{
		    PrintStacktraceAndDie ( "Fatal error : Should already have entry for $date_ and $old_sim_id_" );
		}

		$date_to_sim_id_to_real_id_ { $date_ } { $strategy_progid_ } = $date_to_sim_id_to_real_id_ { $date_ } { $old_sim_id_ };

		push ( @permuted_strat_file_list_ , $t_strategy_file_name_ );


		$strategy_progid_ ++;
	    }
	}

	# For each strat file for this day , run sim_strategy and get
	# trade lines.
	chomp ( @permuted_strat_file_list_ );

	my $PLOT_GRAPH_SCRIPT = SearchScript ( "plot_multifile_cols.pl", () ) ;

	my ( $t_yyyy_ , $t_mm_ , $t_dd_ ) = BreakDateYYYYMMDD ( $date_ );
	my %real_id_to_plot_cmd_ = ( );

	my @parallel_progids_ = ( );
	my @parallel_real_ids_ = ( );
	my @parallel_strat_file_permutations_ = ( );
	
	foreach my $strat_file_permutation_ ( @permuted_strat_file_list_ )
	{
	    my @permuted_strat_file_words_ = split ( '\.' , $strat_file_permutation_ );
	    my $prog_id_ = $permuted_strat_file_words_ [ $#permuted_strat_file_words_ ];
	    my $real_id_ = $date_to_sim_id_to_real_id_ { $date_ } { $prog_id_ };

	    push ( @parallel_progids_ , $prog_id_ );
	    push ( @parallel_real_ids_ , $real_id_ );
	    push ( @parallel_strat_file_permutations_ , $strat_file_permutation_ );

	    if ( $#parallel_progids_ == $MAX_SIM_STRATEGY_IN_PARALLEL )
	    {
		SleepTillOtherSimStrategiesRunning ( );

		for ( my $i = 0 ; $i <= $#parallel_progids_ ; $i ++ )
		{
		    my $t_real_id_ = $parallel_real_ids_ [ $i ];
		    my $t_progid_ = $parallel_progids_ [ $i ];
		    my $t_strat_file_permutation_ = $parallel_strat_file_permutations_ [ $i ];

		    my $t_sim_log_file_ = $SIM_LOG_LOCATION."log.".$date_.".".$t_progid_;
		    my $t_sim_trades_file_ = $SIM_TRADES_LOCATION."trades.".$date_.".".$t_progid_;

		    if ( -e $t_sim_trades_file_ ) { `rm -f $t_sim_trades_file_`; }
		    if ( -e $t_sim_log_file_ ) { `rm -f $t_sim_log_file_`; }

		    my $exec_cmd_ = "$SIM_STRATEGY_EXEC SIM $t_strat_file_permutation_ $t_progid_ $date_ $mkt_model_ ADD_DBG_CODE -1 >/dev/null 2>&1 &";

		    print $main_log_file_handle_ "# $exec_cmd_\n";
		    print $main_log_file_handle_ "  $strat_file_permutation_ REALID= $t_real_id_\n";

		    system ( $exec_cmd_ );
		    $total_num_sim_strategies_already_run_ ++;
		}

		print $main_log_file_handle_ "# GenerateStrategyFilesAndRunSimulations >>>> PROGRESS = ".$total_num_sim_strategies_already_run_." / ".$total_num_sim_strategies_to_be_run_."\n";

		@parallel_progids_ = ( );
		@parallel_real_ids_ = ( );
		@parallel_strat_file_permutations_ = ( );
	    }
	}

	if ( $#parallel_progids_ >= 0 )
	{ # Do the same as above , run the remaining strats.
	    SleepTillOtherSimStrategiesRunning ( );

	    for ( my $i = 0 ; $i <= $#parallel_progids_ ; $i ++ )
	    {
		my $t_real_id_ = $parallel_real_ids_ [ $i ];
		my $t_progid_ = $parallel_progids_ [ $i ];
		my $t_strat_file_permutation_ = $parallel_strat_file_permutations_ [ $i ];

		my $t_sim_log_file_ = $SIM_LOG_LOCATION."log.".$date_.".".$t_progid_;
		my $t_sim_trades_file_ = $SIM_TRADES_LOCATION."trades.".$date_.".".$t_progid_;

		if ( -e $t_sim_trades_file_ ) { `rm -f $t_sim_trades_file_`; }
		if ( -e $t_sim_log_file_ ) { `rm -f $t_sim_log_file_`; }

		my $exec_cmd_ = "$SIM_STRATEGY_EXEC SIM $t_strat_file_permutation_ $t_progid_ $date_ $mkt_model_ ADD_DBG_CODE -1 >/dev/null 2>&1 &";

		print $main_log_file_handle_ "# $exec_cmd_\n";

		system ( $exec_cmd_ );
		$total_num_sim_strategies_already_run_ ++;
	    }

	    @parallel_progids_ = ( );
	    @parallel_real_ids_ = ( );
	    @parallel_strat_file_permutations_ = ( );
	}   

	print $main_log_file_handle_ "# GenerateStrategyFilesAndRunSimulations >>>> PROGRESS = ".$total_num_sim_strategies_already_run_." / ".$total_num_sim_strategies_to_be_run_."\n";

	# Gather results.
	foreach my $strat_file_permutation_ ( @permuted_strat_file_list_ )
	{
	    my @permuted_strat_file_words_ = split ( '\.' , $strat_file_permutation_ );
	    my $progid_ = $permuted_strat_file_words_ [ $#permuted_strat_file_words_ ];
	    my $real_id_ = $date_to_sim_id_to_real_id_ { $date_ } { $progid_ };

	    my $REAL_TRADES_LOCATION = "/NAS1/logs/QueryTrades/".$t_yyyy_."/".$t_mm_."/".$t_dd_."/";

	    my $t_sim_log_file_ = $SIM_LOG_LOCATION."log.".$date_.".".$progid_;
	    my $t_sim_trades_file_ = $SIM_TRADES_LOCATION."trades.".$date_.".".$progid_;

	    if ( $to_plot_ )
	    {
		if ( ! exists ( $real_id_to_plot_cmd_ { $real_id_ } ) )
		{
		    my $t_real_trades_file_ = $REAL_TRADES_LOCATION."trades.".$date_.".".$real_id_;

		    $real_id_to_plot_cmd_ { $real_id_ } = "$PLOT_GRAPH_SCRIPT $t_real_trades_file_ 9 $real_id_ WL ";
		}
	    }

	    my $t_real_trades_file_ = $REAL_TRADES_LOCATION."trades.".$date_.".".$real_id_;

	    if ( ExistsWithSize ( $t_real_trades_file_ ) && ExistsWithSize ( $t_sim_trades_file_ ) )
	    {
		$strat_file_to_simreal_results_ { $strat_file_permutation_ } = GetSimrealResults ( $t_real_trades_file_ , $t_sim_trades_file_ );

		print $main_log_file_handle_ "# SIMREAL-RESULT $strat_file_permutation_ = ".$strat_file_to_simreal_results_ { $strat_file_permutation_ }."\n";
	    }

	    if ( -e $t_sim_trades_file_ ) { `rm -f $t_sim_trades_file_`; }
	    if ( -e $t_sim_log_file_ ) { `rm -f $t_sim_log_file_`; }

	    if ( $to_plot_ && 
		( ExistsWithSize ( $t_real_trades_file_ ) && ExistsWithSize ( $t_sim_trades_file_ ) ) )
	    {
		$real_id_to_plot_cmd_ { $real_id_ } = $real_id_to_plot_cmd_ { $real_id_ }." $t_sim_trades_file_ 9 $progid_ WL ";
	    }
	}

	if ( $to_plot_ )
	{
	    foreach my $real_id_ ( keys %real_id_to_plot_cmd_ )
	    {
		my $plot_cmd_ = $real_id_to_plot_cmd_ { $real_id_ };
		`$plot_cmd_`;
	    }
	}
    }

    return;
}

sub CreateStratFile
{
    my ( $strat_file_line_ , $strat_file_name_ , $strat_progid_ , $sim_config_file_name_ ) = @_;

#    print $main_log_file_handle_ "# CreateStratFile =";

    open ( OUT_STRAT_FILE , "> " , $strat_file_name_ ) or PrintStacktraceAndDie ( "Could not create file $strat_file_name_" );
    my $old_sim_id_ = 1717; # placeholder
    my @strat_file_words_ = split ( ' ' , $strat_file_line_ );
    if ( $#strat_file_words_ >= 0 )
    {

    for ( my $strat_file_word_index_ = 0 ; $strat_file_word_index_ < $#strat_file_words_ ; $strat_file_word_index_ ++ )
    {
	print OUT_STRAT_FILE $strat_file_words_ [ $strat_file_word_index_ ]." ";
#	print $main_log_file_handle_ $strat_file_words_ [ $strat_file_word_index_ ]." ";
    }

    $old_sim_id_ = $strat_file_words_ [ $#strat_file_words_ ];
    }

    print OUT_STRAT_FILE $strat_progid_." ";
#    print $main_log_file_handle_ $strat_progid_." ";
    print OUT_STRAT_FILE $sim_config_file_name_."\n";
#    print $main_log_file_handle_ $sim_config_file_name_."\n";

    close ( OUT_STRAT_FILE );
#    print $main_log_file_handle_ "\n";

    return $old_sim_id_;
}

sub GetStartTimeFromStratFileLine
{
    my ( $strat_line_ ) = @_;

    my @strat_line_words_ = split ( ' ' , $strat_line_ ); chomp ( @strat_line_words_ );

    if ( $#strat_line_words_ < 6 )
    {
	PrintStacktraceAndDie ( "Malformed strategy line= $strat_line_" );
    }

    return $strat_line_words_ [ 5 ];
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

sub GetSimrealResults
{
    my ( $real_trades_file_ , $sim_trades_file_ ) = @_;

    chomp ( $real_trades_file_ );
    chomp ( $sim_trades_file_ );

    print $main_log_file_handle_ "# GetSimrealResults ( $real_trades_file_ , $sim_trades_file_ )\n";

    my $TRADES_FILE_DIFF_SCRIPT = SearchScript ( "generate_trades_file_diffs.pl", () ) ;

    my @real_trades_file_words_ = split ( '/' , $real_trades_file_ );
    my @sim_trades_file_words_ = split ( '/' , $sim_trades_file_ );

    my $t_trades_diff_file_ = $local_tradesdiff_dir_."/".$real_trades_file_words_ [ $#real_trades_file_words_ ]."_".$sim_trades_file_words_ [ $#sim_trades_file_words_ ];

    my $exec_cmd_ = "$TRADES_FILE_DIFF_SCRIPT $real_trades_file_ $sim_trades_file_ $t_trades_diff_file_";
    print $main_log_file_handle_ "# $exec_cmd_\n";

    `$exec_cmd_`;

    open ( TRADES_DIFF_FILE , "<" , $t_trades_diff_file_ ) or PrintStacktraceAndDie ( "Could not open file $t_trades_diff_file_" );
    my @trades_diff_lines_ = <TRADES_DIFF_FILE>; chomp ( @trades_diff_lines_ );
    close ( TRADES_DIFF_FILE );

    $exec_cmd_ = "rm -f $t_trades_diff_file_";
    `$exec_cmd_`;

    my @abs_pnl_diff_ = ( );
    my @pnl_diff_ = ( );
    my @abs_vol_diff_ = ( );

    # TIMESTAMP SIGNED_PNL_DIFF SIGNED_POS_DIFF SIGNED_VOL_DIFF
    foreach my $trades_diff_line_ ( @trades_diff_lines_ )
    {
	my @trades_diff_words_ = split ( ' ' , $trades_diff_line_ );
	if ( $#trades_diff_words_ >= 3 )
	{
	my $t_time_ = $trades_diff_words_ [ 0 ];
	my $t_pnl_diff_ = $trades_diff_words_ [ 1 ];
	my $t_pos_diff_ = $trades_diff_words_ [ 2 ];
	my $t_vol_diff_ = $trades_diff_words_ [ 3 ];

	push ( @abs_pnl_diff_ , abs ( $t_pnl_diff_ ) );
	push ( @abs_vol_diff_ , abs ( $t_vol_diff_ ) );

	push ( @pnl_diff_ , $t_pnl_diff_ );
	}
    }

    my $pnl_diff_median_ = GetAverage ( \@pnl_diff_ ); # Given two equally incorrect sims , would prefer to have one that is pessimistic.
    my $signed_final_pnl_diff_ = $pnl_diff_ [ $#pnl_diff_ ];

    my $final_pnl_diff_ = abs ( $abs_pnl_diff_ [ $#abs_pnl_diff_ ] );

    my $abs_pnl_diff_mean_ = GetAverage ( \@abs_pnl_diff_ );
    my $abs_pnl_diff_median_ = GetMedianConst ( \@abs_pnl_diff_ );
    my $abs_pnl_diff_stdev_ = GetStdev ( \@abs_pnl_diff_ );

    my $abs_vol_diff_mean_ = GetAverage ( \@abs_vol_diff_ );
    my $abs_vol_diff_median_ = GetMedianConst ( \@abs_vol_diff_ );
    my $abs_vol_diff_stdev_ = GetStdev ( \@abs_vol_diff_ );

    my $avg_trades_ = ( $#abs_pnl_diff_ / 2.0 );

    # PNLDIFFMEAN PNLDIFFMEDIAN PNLDIFFSTDEV FINALPNLDIFF 
    # ABSVOLDIFFMEAN ABSVOLDIFFMEDIAN ABSVOLDIFFSTDEV 
    # AVGTRADES 
    # SIGNEDPNLDIFFMEDIAN SIGNEDFINALPNLDIFF
    my $simreal_results_ = sprintf ( "%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %+7.2f %+7.2f" , 
				     $abs_pnl_diff_mean_ , $abs_pnl_diff_median_ , $abs_pnl_diff_stdev_ , $final_pnl_diff_ , 
				     $abs_vol_diff_mean_ , $abs_vol_diff_median_ , $abs_vol_diff_stdev_ , 
				     $avg_trades_ , 
				     $pnl_diff_median_ , $signed_final_pnl_diff_ );

    return $simreal_results_;
}

sub AddResultsToLocalDatabase
{
    print $main_log_file_handle_ "# AddResultsToLocalDatabase\n";    

    my %date_to_sim_config_to_simreal_results_ = ( );

    foreach my $strat_file_ ( keys %strat_file_to_simreal_results_ )
    {
    	my $t_date_ = GetDateFromStratFile ( $strat_file_ );
    	my $t_sim_config_ = GetSimConfigFromStratFile ( $strat_file_ );

    	push ( @ { $date_to_sim_config_to_simreal_results_ { $t_date_ } { $t_sim_config_ } } , $strat_file_to_simreal_results_ { $strat_file_ } );
    }

    foreach my $date_ ( keys %date_to_sim_config_to_simreal_results_ )
    {
	my $sim_config_list_filename_ = $temp_results_base_dir_."/".$date_."_sim_config_list.txt";
	my $results_list_filename_ = $temp_results_base_dir_."/".$date_."_results_list.txt";

	open ( SIM_CONFIG_LIST_FILE , ">" , $sim_config_list_filename_ ) or PrintStacktraceAndDie ( "Could not create file : $sim_config_list_filename_" );
	open ( RESULTS_FILE , ">" , $results_list_filename_ ) or PrintStacktraceAndDie ( "Could not create file : $results_list_filename_" );

	foreach my $sim_config_ ( keys % { $date_to_sim_config_to_simreal_results_ { $date_ } } )
	{
	    my $combined_results_ = CombineSimrealResults ( @ { $date_to_sim_config_to_simreal_results_ { $date_ } { $sim_config_ } } );

	    print SIM_CONFIG_LIST_FILE $sim_config_."\n";
	    print RESULTS_FILE $combined_results_."\n";
	}

	close ( SIM_CONFIG_LIST_FILE );
	close ( RESULTS_FILE );

	my $add_results_to_local_database_script = SearchScript ( "add_results_to_local_database.pl", () );
	my $exec_cmd_ = "$add_results_to_local_database_script $sim_config_list_filename_ $results_list_filename_ $date_ $local_results_base_dir_";

	print $main_log_file_handle_ "# $exec_cmd_\n";
	`$exec_cmd_`;
    }

    return;
}

sub GetDateFromStratFile
{
    my ( $strat_file_name_ ) = @_;

    my $date_ = "";
    if ( index ( $strat_file_name_ , "strat_dt" ) >= 0 &&
	 index ( $strat_file_name_ , "stime" ) >= 0 )
    {
	$date_ = substr ( $strat_file_name_ , index ( $strat_file_name_ , "strat_dt" ) + 9 , 8 ); chomp ( $date_ );
    }
    else
    {
	PrintStacktraceAndDie ( "Invalid strat-file name = $strat_file_name_" );
    }

    return $date_;
}

sub GetSimConfigFromStratFile
{
    my ( $strat_file_name_ ) = @_;

    my $sim_config_file_name_ = "";

    open ( STRAT_FILE , "<" , $strat_file_name_ ) or PrintStacktraceAndDie ( "Could not open $strat_file_name_" );
    while ( my $strat_file_line_ = <STRAT_FILE> )
    {
	if ( index ( $strat_file_line_ , "STRATEGYLINE" ) >= 0 )
	{
	    my @strat_line_words_ = split ( ' ' , $strat_file_line_ ); chomp ( @strat_line_words_ );

	    if ( $#strat_line_words_ < 8 )
	    {
		PrintStacktraceAndDie ( "Malformed strategy file line = $strat_file_line_" );
	    }

	    $sim_config_file_name_ = $strat_line_words_ [ 8 ]; chomp ( $sim_config_file_name_ );
	    last;
	}
    }
    close ( STRAT_FILE );

    return $sim_config_file_name_;
}

sub CombineSimrealResults
{
    my ( @results_list_ ) = @_;

    my $combined_simreal_result_ = "";

    my $final_pnl_diff_ = 0;

    my $abs_pnl_diff_mean_ = 0;
    my $abs_pnl_diff_median_ = 0;
    my $abs_pnl_diff_stdev_ =  0;

    my $pnl_diff_median_ = 0;
    my $signed_final_pnl_diff_ = 0;

    my $abs_vol_diff_mean_ = 0;
    my $abs_vol_diff_median_ = 0;
    my $abs_vol_diff_stdev_ = 0;

    my $average_trades_ = 0;
    my $total_trades_ = 0;

    foreach my $result_line_ ( @results_list_ )
    {
	my @result_words_ = split ( ' ' , $result_line_ ); chomp ( @result_words_ );

	if ( $#result_words_ > 7 )
	{
	    $final_pnl_diff_ += ( $result_words_ [ 3 ] * $result_words_ [ 7 ] );

	    $abs_pnl_diff_mean_ += ( $result_words_ [ 0 ] * $result_words_ [ 7 ] );
	    $abs_pnl_diff_median_ += ( $result_words_ [ 1 ] * $result_words_ [ 7 ] );
	    $abs_pnl_diff_stdev_ += ( $result_words_ [ 2 ] * $result_words_ [ 7 ] );

	    $abs_vol_diff_mean_ += ( $result_words_ [ 4 ] * $result_words_ [ 7 ] );
	    $abs_vol_diff_median_ += ( $result_words_ [ 5 ] * $result_words_ [ 7 ] );
	    $abs_vol_diff_stdev_ += ( $result_words_ [ 6 ] * $result_words_ [ 7 ] );

	    $pnl_diff_median_ += ( $result_words_ [ 8 ] * $result_words_ [ 7 ] );
	    $signed_final_pnl_diff_ += ( $result_words_ [ 9 ] * $result_words_ [ 7 ] );

	    $average_trades_ += $result_words_ [ 7 ];
	    $total_trades_ += $result_words_ [ 7 ];
	}
    }

    # Normalize combined results.
    $final_pnl_diff_ /= $total_trades_;

    $abs_pnl_diff_mean_ /= $total_trades_;
    $abs_pnl_diff_median_ /= $total_trades_;
    $abs_pnl_diff_stdev_ /= $total_trades_;

    $abs_vol_diff_mean_ /= $total_trades_;
    $abs_vol_diff_median_ /= $total_trades_;
    $abs_vol_diff_stdev_ /= $total_trades_;

    $pnl_diff_median_ /= $total_trades_;
    $signed_final_pnl_diff_ /= $total_trades_;

    $average_trades_ /= $total_trades_;

    # PNLDIFFMEAN PNLDIFFMEDIAN PNLDIFFSTDEV FINALPNLDIFF 
    # ABSVOLDIFFMEAN ABSVOLDIFFMEDIAN ABSVOLDIFFSTDEV 
    # AVGTRADES 
    # SIGNEDPNLDIFFMEDIAN SIGNEDFINALPNLDIFF
    $combined_simreal_result_ = sprintf ( "%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %+7.2f %+7.2f" , 
					  $abs_pnl_diff_mean_ , $abs_pnl_diff_median_ , $abs_pnl_diff_stdev_ , $final_pnl_diff_ , 
					  $abs_vol_diff_mean_ , $abs_vol_diff_median_ , $abs_vol_diff_stdev_ , 
					  $average_trades_ , 
					  $pnl_diff_median_ , $signed_final_pnl_diff_ );

    return $combined_simreal_result_;
}

sub AreOtherSimStrategiesRunning
{
    my $are_other_instances_running_ = 0;

    my $SIM_STRATEGY_EXEC = SearchExec ( "sim_strategy", () ) ;
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
