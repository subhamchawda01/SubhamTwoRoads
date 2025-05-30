#!/usr/bin/perl

# \file ModelScripts/generate_logit_strategies.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes an instructionfilename :
#
# indicatorlistfile ( this does not have weights ... so this is an indicatorlistfile and not a model_file ... the only difference between them is that the former has dummy weights = 1.0 )
# startdate 
# enddate
# predalgo
# predduration
# regress exec choice
# ( predduration and ) paramfiles 
#
# Broadly speaking, from the indicatorlistfile it generates normal timed_data
# then based on the arguments, it makes reg_data 
# and then different model_files by running the specified regressions
#
# For each of these days:
# it generates normal data 
# foreach duration
# foreach predalgo
# processes timed_data by 'timed_data_to_reg_data' on reg_data 
# foreach regress_exec_with_params makes a different model
# adds the model to the map duration_to_model_filevec_
# and based on the set of paramfiles for that duration makes the corresponding strategyfile
# finds results on the strategyfiles :

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub LoadInstructionFile ; # used to read the instructions
sub SanityCheckInstructionFile ;
sub RunRegressMakeModelFiles ;
sub AddModelFileToList ; # privately called in RunRegressMakeModelFiles
sub MakeStrategyFiles ; # takes model and paramfile and makes strategyfile
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database
sub SummarizeLocalResultsAndChoose ; # from the files created in the local_results_base_dir choose the best ones to send to pool
sub ComputeStatsSingleRegFile ; # not used now but to comput info of a single reg file
sub RunOutsampleSims ; # computes results of outsample days on the selected strategies

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $GENTIMEDDATA_DAILY_DIR="/spare/local/DailyTimedDataDir/"; # this directory is used to store data so that the calling script can also expect it there
my $GENREGDATA_DAILY_DIR="/spare/local/DailyRegDataDir/"; # this directory is used to store data so that the calling script can also expect it there
my $GENREGDATA_DIR="/spare/local/RegDataDir/"; # this directory is used to store data so that the calling script can also expect it there
my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $GENSTRATWORKDIR=$SPARE_HOME."GSW/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "rkumar" ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "ankit" )
{
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $MODELING_MODELS_DIR=$MODELING_BASE_DIR."/models"; # this directory is used to store the chosen model files
my $MODELING_PARAMS_DIR=$MODELING_BASE_DIR."/params"; # this directory is used to store the chosen param files

require "$GENPERLLIB_DIR/is_model_corr_consistent.pl"; # IsModelCorrConsistent
require "$GENPERLLIB_DIR/get_bad_days_for_shortcode.pl"; # GetBadDaysForShortcode
require "$GENPERLLIB_DIR/get_very_bad_days_for_shortcode.pl"; # GetVeryBadDaysForShortcode
require "$GENPERLLIB_DIR/get_high_volume_days_for_shortcode.pl"; # GetHighVolumeDaysForShortcode
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/name_strategy_from_model_and_param_index.pl"; # for NameStrategyFromModelAndParamIndex
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # for GetPredCountersForThisPredAlgo
require "$GENPERLLIB_DIR/get_indicator_lines_from_ilist.pl"; # GetIndicatorLinesFromIList
require "$GENPERLLIB_DIR/get_high_sharpe_indep_text.pl"; # GetHighSharpeIndepText
require "$GENPERLLIB_DIR/create_sub_model_files.pl"; # CreateSubModelFiles
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
# require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
# require "$GENPERLLIB_DIR/exists_and_same.pl"; #ExistsAndSame
# require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
# require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; #FindItemFromVec
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams
require "$GENPERLLIB_DIR/install_strategy_modelling.pl"; # InstallStrategyModelling
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; # MakeStratVecFromDir
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_substr.pl"; # MakeStratVecFromDirSubstr
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/get_min_pnl_per_contract_for_shortcode.pl"; # GetMinPnlPerContractForShortcode
require "$GENPERLLIB_DIR/get_min_volume_for_shortcode.pl"; # GetMinVolumeForShortcode
require "$GENPERLLIB_DIR/get_min_num_files_to_choose_for_shortcode.pl"; # GetMinNumFilesToChooseForShortcode
require "$GENPERLLIB_DIR/get_business_days_between.pl"; # GetBusinessDaysBetween
require "$GENPERLLIB_DIR/is_server_out_of_disk_space.pl"; # IsServerOutOfDiskSpace
require "$GENPERLLIB_DIR/get_sort_algo.pl"; # GetSortAlgo
require "$GENPERLLIB_DIR/get_index_of_high_sharpe_indep.pl"; #GetIndexOfHighSharpeIndep
require "$GENPERLLIB_DIR/is_strat_dir_in_timeperiod.pl"; # IsStratDirInTimePeriod
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName

my $reg_data_daily_dir = $GENREGDATA_DAILY_DIR;
my $timed_data_daily_dir = $GENTIMEDDATA_DAILY_DIR; # in case the calling script wants data that is not in the global timed data directory just yet
my $reg_data_dir = $GENREGDATA_DIR;
my $MAX_STRAT_FILES_IN_ONE_SIM = 50; # please work on optimizing this value

my $SAVE_CORR_FILE = 0;
my $SAVE_VAR_CORR_FILE = 0;
my $SAVE_CORRMATRIX_FILE = 0;
my $SAVE_HCP_FILE = 0;
my $SAVE_STATS_FILE = 0;
my $SAVE_TRADELOG_FILE = 0;
my $work_dir_ = "";
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;

my $RUN_SINGLE_REGRESS_EXEC = 1;
my $RUN_SINGLE_PREDALGO = 1;
my $RUN_SINGLE_DEP_BASED_FILTER = 1;
my $RUN_SINGLE_INDICATOR_LIST_FILE = 1; # choose a file randomly if 1, else, iterate over all files 


# start 
my $USAGE="$0 instructionfilename [ work_dir ]";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $instructionfilename_ = $ARGV[0];
if ( $#ARGV >= 1 ) 
{ 
    $work_dir_ = $ARGV[1]; 
    $work_dir_ = $work_dir_."/".$unique_gsm_id_; 
}
else 
{
    my $instbase_ = basename ( $instructionfilename_ ); 
    chomp ( $instbase_ );
    $work_dir_ = $GENSTRATWORKDIR.$instbase_."/".$unique_gsm_id_; 

    for ( my $i = 0 ; $i < 30 ; $i ++ )
    {
	if ( -d $work_dir_ )
	{
#	print STDERR "Surprising but this dir exists\n";
	    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	    $work_dir_ = $GENSTRATWORKDIR.$instbase_."/".$unique_gsm_id_;
	}
	else
	{
	    last;
	}
    }
}

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $shortcode_ = "";
my $name_ = "s";
my $indicator_list_filename_ = "";
my @indicator_list_file_vec_ = ( );
my @ilist_filename_vec_ = (); ##store ilist filenames
my @ilist_filename_base_vec_ = (); ##store ilist filenames
my $datagen_start_yyyymmdd_ = "";
my $datagen_end_yyyymmdd_ = "";
my $datagen_day_inclusion_prob_ = 1.00;
my $datagen_day_inclusion_count_ = 0;
my $datagen_start_hhmm_ = "";
my $datagen_start_hhmm_set_already_ = 0; #signal first time
my $datagen_end_hhmm_ = "";
my $datagen_end_hhmm_set_already_ = 0; #signal first time
my $timeout_set_already_ = 0; # signal first time
my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 15;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0; # non economic times periods
my $large_price_move_periods_ = 0; # Large directional price move periods
my $large_price_move_thresh_factor_ = 2; # The thresh factor to use. Default = 2
my $large_price_move_time_period_ = 120; # Time in secs to find large price moves. Default = 120 secs.
my @large_price_move_time_period_vec_ = ( );
my @large_price_move_thresh_factor_vec_ = ( );
my $bad_periods_only_ = 0;
my $to_print_on_economic_times_set_already_ = -1;
my @datagen_day_vec_ = (); # in case we want to give a set of days to regress on 
my @trading_exclude_days_ = ( ); # Do not run sims on a selected set of days.
my @datagen_exclude_days_ = ( ); # Do not run datagen on a selected set of days.
my @target_stdev_model_vec_ = ();
my $strategyname_ = "";
my $trading_start_yyyymmdd_ = "";
my $trading_end_yyyymmdd_ = "";
my $outsample_trading_start_yyyymmdd_ = "";
my $outsample_trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( "TODAY-1" );
my $trading_start_hhmm_ = "";
my $trading_start_hhmm_set_already_ = -1;
my $trading_end_hhmm_ = "";
my $trading_end_hhmm_set_already_ = -1;
my $num_files_to_choose_ = "";
my $min_num_files_to_choose_ = 0; # now we have some files ... don't need to select any every time
# filtering strategies
my $min_pnl_per_contract_to_allow_ = -0.10 ;
my $min_volume_to_allow_ = 50 ;
my $max_ttc_to_allow_ = 60 ;
my $historical_sort_algo_ = "kCNASqDDAdjPnlSqrtVolume" ;
my $delete_intermediate_files_ = 0;
my $mail_address_ = "";
my $author_ = "";
my $use_median_cutoff_ = 1;
my $use_old_genstrat_ = 0;
my $datagen_hhmm_index_ = 0;
my $trading_hhmm_index_ = 0;

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $use_old_genstrat_ = 1;
    $use_median_cutoff_ = 0;
}

my $min_external_datagen_day_vec_size_ = 3;

my @dep_based_filters_ = ();
# my $apply_all_dep_filters_ = 0;
my $apply_trade_vol_filter = 0;
my @predduration_ = ();
my @predalgo_ = ();
my @regress_exec_ = ();
my %duration_to_param_filevec_ ;
my %duration_to_model_filevec_ ;
my @strategy_filevec_ = ();
my @intermediate_files_ = ();

# temporary
my $indicator_list_filename_base_ = "";

my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_params_dir_ = $work_dir_."/params_dir";
my $local_ilist_dir_ = $work_dir_."/ilist_dir";
my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $avoid_high_sharpe_indep_check_index_filename_ = $work_dir_."/avoid_high_sharpe_check_index_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }
if ( ! ( -d $local_ilist_dir_ ) ) { `mkdir -p $local_ilist_dir_`; }
if ( ! ( -d $MODELING_STRATS_DIR ) ) { `mkdir -p $MODELING_STRATS_DIR`; }
if ( ! ( -d $MODELING_MODELS_DIR ) ) { `mkdir -p $MODELING_MODELS_DIR`; }
if ( ! ( -d $MODELING_PARAMS_DIR ) ) { `mkdir -p $MODELING_PARAMS_DIR`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    if ( IsServerOutOfDiskSpace ( ) )
    {
	my $hostname_ = `hostname -s`; chomp ( $hostname_ );
	
	PrintStacktraceAndDie ( "$hostname_ out of disk space. Exiting." );
    }
}

# load instruction file
LoadInstructionFile ( );

# Check if the arguments are complete
SanityCheckInstructionFile ( );

my $min_price_increment_ = `$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $yyyymmdd_` ; chomp ( $min_price_increment_ );

# generate reg data ( and timed data, and remove mean if needed ) and then find weights
RunRegressMakeModelFiles ( );

# with %duration_to_model_filevec_ and %duration_to_param_filevec_ make the strategyfiles
MakeStrategyFiles ( );

# find results of the strategies and put them in local_results_base_dir
RunSimulationOnCandidates ( );

# among the candidates choose the best
SummarizeLocalResultsAndChoose ( );

# end script
$main_log_file_handle_->close;

exit ( 0 );


sub LoadInstructionFile 
{
    print $main_log_file_handle_ "LoadInstructionFile $instructionfilename_\n";

    open INSTRUCTIONFILEHANDLE, "< $instructionfilename_ " or PrintStacktraceAndDie ( "$0 Could not open $instructionfilename_\n" );
    
    my $current_instruction_="";
    my $current_instruction_set_ = 0;
    my $t_ = 0;
    my $short_strategy_name_ = '';
    while ( my $thisline_ = <INSTRUCTIONFILEHANDLE> ) 
    {
	chomp ( $thisline_ ); # remove newline
	print $main_log_file_handle_ "$thisline_\n"; #logging for later

	my @instruction_line_words_ = split ( ' ', $thisline_ );
	# TODO{} remove empty space at the beginning

	if ( $#instruction_line_words_ < 0 ) 
	{ # empty line hence set $current_instruction_set_ 0
	    $current_instruction_ = "";
	    $current_instruction_set_ = 0;
	    next;
	} 
	else 
	{ # $#instruction_line_words_ >= 0

	    if ( substr ( $instruction_line_words_[0], 0, 1) ne '#' )
	    {

		if ( $current_instruction_set_ == 0 ) 
		{ # no instruction set currently being processed
		    $current_instruction_ = $instruction_line_words_[0];
		    $current_instruction_set_ = 1;
		    # print $main_log_file_handle_ "Setting instruction to $current_instruction_\n";
		} 
		else 
		{ # $current_instruction_ is valid
		    # print $main_log_file_handle_ "DEBUG current_instruction is $current_instruction_\n";
		    given ( $current_instruction_ ) 
		    {
			when ("SHORTCODE") 
			{
			    $shortcode_ = $instruction_line_words_[0];
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("NAME") 
			{
			    $name_ = $instruction_line_words_[0];
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("INDICATORLISTFILENAME") 
			{
			    if ( ExistsWithSize ( $instruction_line_words_ [ 0 ] ) )
			    { # Eliminate non-existant ilists.
				push ( @indicator_list_file_vec_ , $instruction_line_words_ [ 0 ] );
			    }
			    ;
			    print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("RUN_SINGLE_INDICATOR_LIST_FILE")
			{
			    $RUN_SINGLE_INDICATOR_LIST_FILE = $instruction_line_words_[0];
			}
			when ("DATAGEN_START_END_YYYYMMDD") 
			{
			    if ( ( $#instruction_line_words_ >= 1 ) && 
				 ( ( ! ( $datagen_start_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) ) )
			    {
				$datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[0] );
				$datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[1] );
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("DATAGEN_START_YYYYMMDD") 
			{
			    if ( ( ! ( $datagen_start_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) )
			    {
				if ( $instruction_line_words_[0] =~ /FROM_END/ )
				{ # e.g. FROM_END 30
				    $datagen_start_yyyymmdd_ = $instruction_line_words_[1] ;
				}
				else
				{
				    $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
				}
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("DATAGEN_END_YYYYMMDD") 
			{
			    if ( ( ! ( $datagen_end_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) )
			    {
				$datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("DATAGEN_DAYS")
			{
			    for ( my $i = 0 ; $i <= $#instruction_line_words_ ; $i ++ )
			    {
				if ( ( substr ( $instruction_line_words_[$i], 0, 1) ne '#' ) &&
				     ( ValidDate ( $instruction_line_words_[$i] ) ) && 
				     ( ! NoDataDateForShortcode ( $instruction_line_words_[$i] , $shortcode_ ) ) &&
				     ( ! IsProductHoliday ( $instruction_line_words_ [ $i ] , $shortcode_ ) ) )
				{
				    push ( @datagen_day_vec_, $instruction_line_words_[$i] );
				    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
				}
			    }
			}
			when ("DATAGEN_BAD_DAYS")
			{
			    print $main_log_file_handle_ "On reading DATAGEN_BAD_DAYS datagen_day_vec reset to empty\n";
			    @datagen_day_vec_ = ();
			    GetBadDaysForShortcode ( $shortcode_, \@datagen_day_vec_ );
			    if ( $#datagen_day_vec_ >= 3 )
			    {
				print $main_log_file_handle_ "Only model on bad days:",join ( ' ', @datagen_day_vec_ ),"\n";
			    }
			    else
			    {
				@datagen_day_vec_ = ();
			    }
			}
			when ("DATAGEN_VERY_BAD_DAYS")
			{
			    print $main_log_file_handle_ "On reading DATAGEN_VERY_BAD_DAYS datagen_day_vec reset to empty\n";
			    @datagen_day_vec_ = ();
			    GetVeryBadDaysForShortcode ( $shortcode_, \@datagen_day_vec_ );
			    if ( $#datagen_day_vec_ >= 3 )
			    {
				print $main_log_file_handle_ "Only model on very bad days:",join ( ' ', @datagen_day_vec_ ),"\n";
			    }
			    else
			    {
				@datagen_day_vec_ = ();
			    }
			}
			when ("DATAGEN_HIGH_VOLUME_DAYS")
			{
			    print $main_log_file_handle_ "On reading DATAGEN_HIGH_VOLUME_DAYS datagen_day_vec reset to empty\n";
			    @datagen_day_vec_ = ();
			    GetHighVolumeDaysForShortcode ( $shortcode_, \@datagen_day_vec_ );
			    if ( $#datagen_day_vec_ >= 3 )
			    {
				print $main_log_file_handle_ "Only model on high volume days:",join ( ' ', @datagen_day_vec_ ),"\n";

				if ( $name_ ne "hv" )
				{ 
				    $name_ = "hv".$name_; # change name to indicate hv
				}
			    }
			    else
			    {
				@datagen_day_vec_ = ();
			    }
			}
			when ("DATAGEN_DAY_INCLUSION_PROB")
			{
			    $datagen_day_inclusion_prob_ = max ( 0.00, min ( 1.00, $instruction_line_words_[0] ) ) ;
			}
			when ("DATAGEN_DAY_INCLUSION_COUNT")
			{
			    $datagen_day_inclusion_count_ = max ( 0.00, min ( 120, $instruction_line_words_[0] ) ) ;
			}
			when ("DATAGEN_EXCLUDE_DAYS")
			{
			    for ( my $i = 0 ; $i <= $#instruction_line_words_ ; $i ++ )
			    {
				if ( ( substr ( $instruction_line_words_[$i], 0, 1) ne '#' ) &&
				     ( ValidDate ( $instruction_line_words_[$i] ) ) && 
				     ( ! ( NoDataDateForShortcode ( $instruction_line_words_[$i] , $shortcode_ ) || ( SkipWeirdDate ( $instruction_line_words_[$i] ) ) ) ) )
				{
				    if ( ! FindItemFromVec ( $instruction_line_words_ [ $i ] , @datagen_exclude_days_ ) )
				    {
					push ( @datagen_exclude_days_ , $instruction_line_words_ [ $i ] );
				    }
				    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
				}
			    }
			}
			when ("DATAGEN_START_END_HHMM") 
			{
			    if ( ( $datagen_start_hhmm_set_already_ != 1 ) ||
				 ( rand(1) > 0.50 ) )
			    { # with 50 % probability take this one
				$datagen_start_hhmm_ = $instruction_line_words_[0];
				$datagen_start_hhmm_set_already_ = 1;

				$datagen_end_hhmm_ = $instruction_line_words_[1];
				$datagen_end_hhmm_set_already_ = 1;
				$datagen_hhmm_index_ = $t_;
			    }
			    $t_ ++;
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("DATAGEN_START_HHMM") 
			{
			    if ( ( $datagen_start_hhmm_set_already_ != 1 ) ||
				 ( rand(1) > 0.50 ) )
			    { # with 50 % probability take this one
				$datagen_start_hhmm_ = $instruction_line_words_[0];
				$datagen_start_hhmm_set_already_ = 1;
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("DATAGEN_END_HHMM") 
			{
			    if ( ( $datagen_end_hhmm_set_already_ != 1 ) ||
				 ( rand(1) > 0.50 ) )
			    { # with 50 % probability take this one
				$datagen_end_hhmm_ = $instruction_line_words_[0];
				$datagen_end_hhmm_set_already_ = 1;
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("DATAGEN_TIMEOUT")
			{
			    if ( $#instruction_line_words_ >= 2 )
			    { 
				if ( ( $timeout_set_already_ != 1 ) ||
				     ( rand(1) > 0.50 ) )
				{ # with 50 % probability take this one
				    $datagen_msecs_timeout_ = $instruction_line_words_[0];
				    $datagen_l1events_timeout_ = $instruction_line_words_[1];
				    $datagen_num_trades_timeout_ = $instruction_line_words_[2];
				    $timeout_set_already_ = 1;
				}
			    }
			    
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			
			when ("TO_PRINT_ON_ECO")
			{
			    if ( ( $to_print_on_economic_times_set_already_ != 1 ) ||
				 ( rand(1) > 0.50 ) )
			    { # with 50 % probability take this one
				$to_print_on_economic_times_ = $instruction_line_words_[0];
				$to_print_on_economic_times_set_already_ = 1;
				print $main_log_file_handle_ "TO_PRINT_ON_ECO_SET_TO $to_print_on_economic_times_\n"; 
			    }
			    
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}

			when ( "LARGE_PRICE_MOVE_PERIODS" )
			{
			    if ( $#instruction_line_words_ < 1 )
			    { # Old config , use default values.
				if ( $large_price_move_periods_ <= 0 && 
				     rand ( 1 ) > 0.5 )
				{
				    $large_price_move_periods_ = 1;
				    $large_price_move_time_period_ = 120;
				    $large_price_move_thresh_factor_ = 3;

				    print $main_log_file_handle_ "LARGE_PRICE_MOVE_PERIODS = $large_price_move_periods_ LARGE_PRICE_MOVE_TIME_PERIOD = $large_price_move_time_period_ LARGE_PRICE_MOVE_THRESH_FACTOR = $large_price_move_thresh_factor_\n";

				    $name_ = "lpm.".$large_price_move_time_period_.".".$large_price_move_thresh_factor_.".".$name_; # change name to indicate lpm
				}
			    }
			    else
			    {
				push ( @large_price_move_time_period_vec_ , $instruction_line_words_ [ 0 ] );
				push ( @large_price_move_thresh_factor_vec_ , $instruction_line_words_ [ 1 ] );
			    }
			}

			when ( "BAD_PERIODS" )
			{
			    if ( $bad_periods_only_ <= 0 &&
				 ( rand ( 1 ) > 0.5 ) ) # Take this with a 50 % prob.
			    {
				$bad_periods_only_ = 1;
				$name_ = "badp".$name_; # Change name to indicate bad period strat.
			    }
			}

			when ("PREDDURATION") 
			{ # expects one duration per line
			    push ( @predduration_, $instruction_line_words_[0] );
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("PREDALGO") 
			{
			    push ( @predalgo_, $instruction_line_words_[0] );
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("DEP_BASED_FILTERS") 
			{
			    # if($instruction_line_words_[0] eq "fv"){
			    # 	$apply_trade_vol_filter = 1;
			    # }
			    # elsif ($instruction_line_words_[0] eq "all"){
			    #     $apply_all_dep_filters_ = 1;
			    # }
			    # else{
			    push ( @dep_based_filters_, $instruction_line_words_[0] );	
			    # }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("REGRESS_EXEC") 
			{
			    push ( @regress_exec_, [ @instruction_line_words_ ] );
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("TARGET_STDEV_MODEL")
			{
			    for ( my $i = 0 ; $i <= $#instruction_line_words_ ; $i ++ )
			    {
				if ( substr ( $instruction_line_words_[$i], 0, 1) ne '#' )
				{
				    push ( @target_stdev_model_vec_, $instruction_line_words_[$i] );
				    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
				}
			    }
			}			
			when ("DELETE_INTERMEDIATE_FILES") 
			{
			    $delete_intermediate_files_ = 1; # Disabled , since a lot of sim_strats , datagens were segfaulting with bmf_eq
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("USE_PPC_CUTOFF")
			{
			    $use_median_cutoff_ = 0; # will use the old pnl_per_contract cutoffs to select a strategy for installation.
			}
			when ("USE_OLD_GENSTRAT")
			{
			    $use_old_genstrat_ = 1; $use_median_cutoff_=0; # using old genstrat no median cutoff, no algo change..
			}
    			when ("STRATEGYNAME") 
			{
			  {
				$strategyname_ = $instruction_line_words_[0];
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("TRADING_START_END_YYYYMMDD") 
			{
			    if ( ( $#instruction_line_words_ >= 1 ) && 
				 ( ( ! ( $trading_start_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) ) )
			    {
				$trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[0] );
				$trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[1] );
				$outsample_trading_start_yyyymmdd_ = CalcNextWorkingDateMult ( $trading_end_yyyymmdd_, 1 );			    
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("TRADING_START_YYYYMMDD") 
			{
			    if ( ( ! $trading_start_yyyymmdd_ ) ||
				 ( rand(1) > 0.50 ) )
			    {
				$trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("TRADING_END_YYYYMMDD") 
			{
			    if ( ( ! $trading_end_yyyymmdd_ ) ||
				 ( rand(1) > 0.50 ) )
			    {
				$trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
				$outsample_trading_start_yyyymmdd_ = CalcNextWorkingDateMult ( $trading_end_yyyymmdd_, 1 );
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("TRADING_EXCLUDE_DAYS")
			{
			    for ( my $i = 0 ; $i <= $#instruction_line_words_ ; $i ++ )
			    {
				if ( ( substr ( $instruction_line_words_[$i], 0, 1) ne '#' ) &&
				     ( ValidDate ( $instruction_line_words_[$i] ) && ( ! ( NoDataDateForShortcode ( $instruction_line_words_[$i] , $shortcode_ ) || ( SkipWeirdDate ( $instruction_line_words_[$i] ) ) ) ) ) )
				{
				    if ( ! FindItemFromVec ( $instruction_line_words_ [ $i ] , @trading_exclude_days_ ) )
				    {
					push ( @trading_exclude_days_ , $instruction_line_words_ [ $i ] );
				    }
				    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
				}
			    }
			}
			when ( "TRADING_START_END_HHMM" )
			{
			  {
				if ( ( $trading_start_hhmm_set_already_ != 1 ) ||
				     ( rand(1) > 0.50 ) )
				{ # with 50 % probability take this one
				    $trading_start_hhmm_ = $instruction_line_words_[0];
				    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
				    $trading_start_hhmm_set_already_ = 1;
				    
				    $trading_end_hhmm_ = $instruction_line_words_[1];
				    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
				    $trading_end_hhmm_set_already_ = 1;			    
				}
			    }
			}
			when ("TRADING_START_HHMM") 
			{
			    if ( ( $trading_start_hhmm_set_already_ != 1 ) ||
				 ( rand(1) > 0.50 ) )
			    { # with 50 % probability take this one
				$trading_start_hhmm_ = $instruction_line_words_[0];
				# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
				$trading_start_hhmm_set_already_ = 1;
			    }
			}
			when ("TRADING_END_HHMM") 
			{
			    if ( ( $trading_end_hhmm_set_already_ != 1 ) ||
				 ( rand(1) > 0.50 ) )
			    { # with 50 % probability take this one
				$trading_end_hhmm_ = $instruction_line_words_[0];
				# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
				$trading_end_hhmm_set_already_ = 1;
			    }
			}
			when ("PARAMFILENAME") 
			{
			    if ( ( $#instruction_line_words_ >= 1 ) && # has to have two words ... preddur paramfile
				 ( ( substr ( $instruction_line_words_[0], 0, 1) ne '#' ) &&
				   ( substr ( $instruction_line_words_[1], 0, 1) ne '#' ) ) ) # no comments in these words
			    {
				my $this_pred_duration_ = $instruction_line_words_[0];
				my $this_orig_param_filename_ = $instruction_line_words_[1];
				if ( ExistsWithSize ( $this_orig_param_filename_ ) )
				{ # file exists and isn't empty sized
				    print $main_log_file_handle_ "PermuteParams ( $this_orig_param_filename_, $local_params_dir_ )\n";
				    my @this_param_filename_vec_ = PermuteParams ( $this_orig_param_filename_, $local_params_dir_ );
				    for ( my $tpfv_index_ = 0 ; $tpfv_index_ <= $#this_param_filename_vec_ ; $tpfv_index_ ++ )
				    {
					my $this_param_filename_ = $this_param_filename_vec_[$tpfv_index_];

					if ( exists $duration_to_param_filevec_{$this_pred_duration_} ) { # seen duration for a different param_file already
					    my $scalar_ref_ = $duration_to_param_filevec_{$this_pred_duration_} ;
					    push ( @$scalar_ref_, $this_param_filename_ );
					} else { # seeing this duration for the first time
					    my @param_file_args_ = ();
					    push ( @param_file_args_, $this_param_filename_ );
					    $duration_to_param_filevec_{$this_pred_duration_} = [ @param_file_args_ ] ;
					    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
					}
				    }
				}
				else
				{   # PARAMDIR dirname_
				    # PARAMLIST file_with_params
				    if ( ( $#instruction_line_words_ >= 2 ) &&
					 ( substr ( $instruction_line_words_[2], 0, 1) ne '#' ) ) # no comments in these words
				    {
					given ( $instruction_line_words_[1] )
					{
					    when ( "PARAMDIR" )
					    {
						print $main_log_file_handle_ "PARAMDIR $instruction_line_words_[2]\n";
						my @this_param_filename_vec_ = MakeStratVecFromDirSubstr ( $instruction_line_words_[2], "param" );
						print $main_log_file_handle_ join ( "\n", @this_param_filename_vec_ )."\n";
						for ( my $tpfv_index_ = 0 ; $tpfv_index_ <= $#this_param_filename_vec_ ; $tpfv_index_ ++ )
						{
						    my $this_param_filename_ = $this_param_filename_vec_[$tpfv_index_];

						    if ( exists $duration_to_param_filevec_{$this_pred_duration_} ) { # seen duration for a different param_file already
							my $scalar_ref_ = $duration_to_param_filevec_{$this_pred_duration_} ;
							push ( @$scalar_ref_, $this_param_filename_ );
						    } else { # seeing this duration for the first time
							my @param_file_args_ = ();
							push ( @param_file_args_, $this_param_filename_ );
							$duration_to_param_filevec_{$this_pred_duration_} = [ @param_file_args_ ] ;
						    }
						}
					    }
					    when ( "PARAMLIST" )
					    {
						print $main_log_file_handle_ "PARAMLIST $instruction_line_words_[2]\n";
						my @this_param_filename_vec_ = MakeFilenameVecFromList ( $instruction_line_words_[2] );
						print $main_log_file_handle_ join ( "\n", @this_param_filename_vec_ )."\n";
						for ( my $tpfv_index_ = 0 ; $tpfv_index_ <= $#this_param_filename_vec_ ; $tpfv_index_ ++ )
						{
						    my $this_param_filename_ = $this_param_filename_vec_[$tpfv_index_];

						    if ( exists $duration_to_param_filevec_{$this_pred_duration_} ) { # seen duration for a different param_file already
							my $scalar_ref_ = $duration_to_param_filevec_{$this_pred_duration_} ;
							push ( @$scalar_ref_, $this_param_filename_ );
						    } else { # seeing this duration for the first time
							my @param_file_args_ = ();
							push ( @param_file_args_, $this_param_filename_ );
							$duration_to_param_filevec_{$this_pred_duration_} = [ @param_file_args_ ] ;
						    }
						}
					    }
					    default
					    {
						print $main_log_file_handle_ "PARAMLINE $thisline_ ignored\n";
					    }
					}
				    }
				}
			    }
			}
			when ("MIN_NUM_FILES_TO_CHOOSE") 
			{
			    $min_num_files_to_choose_ = $instruction_line_words_[0];

			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("MIN_PNL_PER_CONTRACT") 
			{
			    $min_pnl_per_contract_to_allow_ = $instruction_line_words_[0];
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";

			    if ( $USER ne "sghosh" && $USER ne "ravi" )
			    {
				if ( ! $author_ || 
				     ( $author_ && $author_ ne "sghosh" ) )
				{
				    $min_pnl_per_contract_to_allow_ = min ( $min_pnl_per_contract_to_allow_ , GetMinPnlPerContractForShortcode ( $shortcode_ ) );
				}
			    }
			}
			when ("MIN_VOLUME") 
			{
			    if ( ( $min_volume_to_allow_ == 50 ) ||
				 ( rand(1) > 0.50 ) )
			    {
				$min_volume_to_allow_ = $instruction_line_words_[0];
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("MAX_TTC") 
			{
			    $max_ttc_to_allow_ = $instruction_line_words_[0];
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("NUM_FILES_TO_CHOOSE") 
			{
			    $num_files_to_choose_ = $instruction_line_words_[0];
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}

			when ("HISTORICAL_SORT_ALGO")
			{
			    if ( ( rand(1) > 0.50 ) &&
				 ( substr ( $instruction_line_words_[0], 0, 1) ne '#' ) ) # no comments in this word

			    { # with 50 % probability take this one
				$historical_sort_algo_ = $instruction_line_words_[0];
				print $main_log_file_handle_ "HISTORICAL_SORT_ALGO set to $historical_sort_algo_\n"; 
			    }
			    
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}

			when ("MAIL_ADDRESS") 
			{
			    $mail_address_ = $instruction_line_words_[0];
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}

			when ( "AUTHOR" )
			{
			    $author_ = $instruction_line_words_ [ 0 ];
			}

			default
			{
			}
		    }
		}
	    }
	}
    }
    
    close ( INSTRUCTIONFILEHANDLE );

    my $sort_algo_string_ = "";
    if ( ! $use_old_genstrat_  && -e $HOME_DIR."/modelling/pick_strats_config/US/${shortcode_}.US.txt" ) 
    {
	my $t_cmd_="sed -n '/SORT_ALGO/,/^\$/{/#\\|^\$\\|SORT_ALGO/ !p;}' ~/modelling/pick_strats_config/US/${shortcode_}.US.txt";
	$sort_algo_string_ = `$t_cmd_`;
	chomp($sort_algo_string_);
	if ( substr ( $sort_algo_string_, 0, 1) ne '#' )
	{
	    my $pick_strats_sort_algo_ = GetSortAlgo($sort_algo_string_);
	    if ( $pick_strats_sort_algo_ ne "kCNAMAX" )
	    { 
		$historical_sort_algo_ = $pick_strats_sort_algo_;
	    }
	}
	print $main_log_file_handle_ $historical_sort_algo_."\n";
    }
    
    if ( $#indicator_list_file_vec_ < 0 )
    { # None of the ilistfiles exist !
	PrintStacktraceAndDie ( "Bad Config $instructionfilename_ . No indicatorilistfiles available." );
    }

    { # Pick a random ilist file.
	my $t_ilist_index_ = 0;
	my $t_ilist_ = "";
	
	if ( $RUN_SINGLE_INDICATOR_LIST_FILE )
	{
	    $t_ilist_index_ = int ( rand ( $#indicator_list_file_vec_ + 1 ) );
	    my $t_random_indicator_list_file_ = $indicator_list_file_vec_ [ $t_ilist_index_ ];
	    @indicator_list_file_vec_ = ( );
	    push ( @indicator_list_file_vec_ , $t_random_indicator_list_file_ );
	    #print @indicator_list_file_vec_;
	}
	
	
	for ( my $t_ilist_index_ = 0; $t_ilist_index_ <= $#indicator_list_file_vec_; $t_ilist_index_ ++ )
	{# for every indicator list file
	    
	    $t_ilist_ = $indicator_list_file_vec_ [ $t_ilist_index_ ]; chomp ( $t_ilist_ );

	    if ( length ( $t_ilist_ ) < 5 || ! ExistsWithSize ( $t_ilist_ ) )
	    {
		print $main_log_file_handle_ "Weird indicatorlistfilename = $t_ilist_ \n";
		PrintStacktraceAndDie ( "Weird indicatorlistfilename = $t_ilist_ \n" );
	    }

	    $indicator_list_filename_ = $t_ilist_; #TODO : check if being reused later
	    my $indicator_list_filename_new_dir_ = FileNameInNewDir ( $indicator_list_filename_, $local_ilist_dir_ );
	    my $exec_cmd = "cp $indicator_list_filename_ $indicator_list_filename_new_dir_";
	    print $main_log_file_handle_ "$exec_cmd\n";
	    `$exec_cmd`;
	    $indicator_list_filename_ = $indicator_list_filename_new_dir_;#TODO : check if being reused later
	    $indicator_list_filename_base_ = basename ( $indicator_list_filename_ ); chomp ($indicator_list_filename_base_);#TODO : check if being reused later
	    push ( @ilist_filename_vec_ , $indicator_list_filename_ );
	    push ( @ilist_filename_base_vec_ , $indicator_list_filename_base_ );
	}
        #print @ilist_filename_vec_;
        #print "here\n";
	#print @ilist_filename_base_vec_;
	
    }

    if ( $datagen_start_yyyymmdd_ < 20100101 )
    { # say "FROM_END 30"'s 30
	my $from_end_num_days_ = max ( 2, $datagen_start_yyyymmdd_ ) ;
	$datagen_start_yyyymmdd_ = CalcPrevWorkingDateMult ( $datagen_end_yyyymmdd_, min ( 75, $from_end_num_days_ ) ) ;
    }

    if ( $#datagen_day_vec_ >= $min_external_datagen_day_vec_size_ )
    { # at least a minimum quorum of days
	@datagen_day_vec_ = sort ( @datagen_day_vec_ );
	if ( $datagen_start_yyyymmdd_ ) 
	{ # specified
	    # remove days in vec that are before this 
	    while ( $#datagen_day_vec_ >= $min_external_datagen_day_vec_size_ )
	    { # at least 5 days 
		if ( $datagen_day_vec_[0] < $datagen_start_yyyymmdd_ )
		{
		    printf $main_log_file_handle_ "Removing datagen day %s\n", $datagen_day_vec_[0];
		    splice ( @datagen_day_vec_, 0, 1 );
		}
		else
		{
		    last;
		}
	    }
	}
	else
	{ # take start date from this 
	    $datagen_start_yyyymmdd_ = $datagen_day_vec_[0];
	    printf $main_log_file_handle_ "Setting new datagen start date %s\n", $datagen_day_vec_[0];
	}

	if ( $datagen_end_yyyymmdd_ )
	{ # specified
	    # remove days in vec that are after the given enddate
	    while ( $#datagen_day_vec_ >= $min_external_datagen_day_vec_size_ )
	    { # at least 5 days 
		if ( $datagen_day_vec_[$#datagen_day_vec_] > $datagen_end_yyyymmdd_ )
		{
		    printf $main_log_file_handle_ "Removing datagen day %s\n", $datagen_day_vec_[$#datagen_day_vec_];
		    splice ( @datagen_day_vec_, $#datagen_day_vec_, 1 );
		}
		else
		{
		    last;
		}
	    }
	}
	else
	{ # take end date from this
	    $datagen_end_yyyymmdd_ = $datagen_day_vec_[$#datagen_day_vec_];
	    printf $main_log_file_handle_ "Setting new datagen end date %s\n", $datagen_day_vec_[$#datagen_day_vec_];
	}
    }
    else
    {
	
	print $main_log_file_handle_ "Low count of datagen_day_vec ( $#datagen_day_vec_ ), reset to empty, searching from $datagen_start_yyyymmdd_ to $datagen_end_yyyymmdd_\n";

	# fill up datagen_day_vec_
	@datagen_day_vec_ = ();
	my $datagen_date_ = $datagen_end_yyyymmdd_;
	my $max_days_at_a_time_ = 2000;
	for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
	{
	    #print $main_log_file_handle_ "Considering datagen day $datagen_date_ \n";

	    if ( ( ! ValidDate ( $datagen_date_ ) ) ||
		 ( $datagen_date_ < $datagen_start_yyyymmdd_ ) )
	    {
		last;
	    }

	    if ( SkipWeirdDate ( $datagen_date_ ) ||
		 NoDataDateForShortcode ( $datagen_date_ , $shortcode_ ) ||
		 ( IsDateHoliday ( $datagen_date_ ) || ( ( $shortcode_ ) && ( IsProductHoliday ( $datagen_date_, $shortcode_ ) ) ) ) )
	    {
		print $main_log_file_handle_ "Skipping datagen day $datagen_date_ \n";
		$datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
		next;
	    }
	    
	    {   
		print $main_log_file_handle_ "Taking datagen day $datagen_date_\n";
		push ( @datagen_day_vec_, $datagen_date_ ) ;
	    }

	    $datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
	}

	# cut down to size
	if ( $USER ne "sghosh" &&
	     $author_ ne "sghosh" )
	{
	    if ( $datagen_day_inclusion_count_ > 0 )
	    {
		if ( $datagen_day_inclusion_count_ < ( $#datagen_day_vec_ + 1 ) )
		{
		    $datagen_day_inclusion_prob_ = $datagen_day_inclusion_count_ / ( $#datagen_day_vec_ + 1.00 );
		}
		else
		{
		    $datagen_day_inclusion_prob_ = 1.00;
		}
	    }
	}

	if ( $datagen_day_inclusion_prob_ < 0.99 )
	{
	    my @temp_datagen_day_vec_ = @datagen_day_vec_;
	    @datagen_day_vec_ = ();
	    foreach my $this_datagen_date_ ( @temp_datagen_day_vec_ )
	    {
		# if day fails random day inclusion ( if enabled )
		my $this_rv = rand(1);
		if ( $this_rv > $datagen_day_inclusion_prob_ )
		{
		    print $main_log_file_handle_ "Due to random check $this_rv > $datagen_day_inclusion_prob_ fail ... Skipping $this_datagen_date_ for $indicator_list_filename_base_  \n";#not sure if this requires change--to a vector
		}
		elsif ( ! ( SkipWeirdDate ( $this_datagen_date_ ) ||
			    NoDataDateForShortcode ( $this_datagen_date_ , $shortcode_ ) ||
			    ( IsDateHoliday ( $this_datagen_date_ ) || ( ( $shortcode_ ) && ( IsProductHoliday ( $this_datagen_date_, $shortcode_ ) ) ) ) ) )
		{
		    if ( ! FindItemFromVec ( $this_datagen_date_ , @datagen_exclude_days_ ) )
		    {
			print $main_log_file_handle_ "Pushed $this_datagen_date_ to datagen_day_vec_\n";
			push ( @datagen_day_vec_, $this_datagen_date_ );
		    }
		    else
		    {
			print $main_log_file_handle_ "Due to DATAGEN_EXCLUDE_DAYS $this_datagen_date_ skipped.\n";
		    }
		}
	    }
	}
    }

    if ( $bad_periods_only_ == 1 )
    {
	@large_price_move_time_period_vec_ = ();
	@large_price_move_thresh_factor_vec_ = ();
	$large_price_move_periods_ = 0;
    }
    else
    {
	if ( ( $#large_price_move_time_period_vec_ >= 0 ) && 
	     ( rand(1) > 0.50 ) ) # Set with probability 0.50
	{
	    $large_price_move_periods_ = 1;

	    my $t_random_index_ = int ( rand ( $#large_price_move_time_period_vec_ + 1 ) );
	    $large_price_move_time_period_ = $large_price_move_time_period_vec_ [ $t_random_index_ ];
	    $large_price_move_thresh_factor_ = $large_price_move_thresh_factor_vec_ [ $t_random_index_ ];

	    print $main_log_file_handle_ "LARGE_PRICE_MOVE_PERIODS = $large_price_move_periods_ LARGE_PRICE_MOVE_TIME_PERIOD = $large_price_move_time_period_ LARGE_PRICE_MOVE_THRESH_FACTOR = $large_price_move_thresh_factor_\n";

	    $name_ = "lpm.".$large_price_move_time_period_.".".$large_price_move_thresh_factor_.".".$name_; # change name to indicate lpm
	}
    }
}

sub SanityCheckInstructionFile 
{
    print $main_log_file_handle_ "SanityCheckInstructionFile\n";

    if ( ! ( $shortcode_ ) )
    {
	print $main_log_file_handle_ "SHORTCODE missing\n";
	exit ( 0 );
    }

    if ( $#ilist_filename_base_vec_ <0 )
    {
	print $main_log_file_handle_ "INDICATORLISTFILENAME vector empty\n";
	exit ( 0 );
    }

    if ( ! ( $datagen_start_yyyymmdd_ ) )
    {
	print $main_log_file_handle_ "DATAGEN_START_YYYYMMDD missing\n";
	exit ( 0 );
    }

    if ( ! ( ValidDate ( $datagen_start_yyyymmdd_ ) ) )
    {
	print $main_log_file_handle_ "DATAGEN_START_YYYYMMDD not Valid\n";
	exit ( 0 );
    }

    if ( ! ( $datagen_end_yyyymmdd_ ) )
    {
	print $main_log_file_handle_ "DATAGEN_END_YYYYMMDD missing\n";
	exit ( 0 );
    }

    if ( ! ( ValidDate ( $datagen_end_yyyymmdd_ ) ) )
    {
	print $main_log_file_handle_ "DATAGEN_END_YYYYMMDD not Valid\n";
	exit ( 0 );
    }
    
    if ( ! ( $datagen_start_hhmm_ ) )
    {
	print $main_log_file_handle_ "DATAGEN_START_HHMM missing\n";
	exit ( 0 );
    }

    if ( ! ( $datagen_end_hhmm_ ) )
    {
	print $main_log_file_handle_ "DATAGEN_END_HHMM missing\n";
	exit ( 0 );
    }

    my @valid_filter_list_ = ("f0", "fst.5", "fst1", "fst2", "fsl2", "fsl3", "fsg.5", "fsg1", "fsg2", "fsr1_5", "fsr.5_3" , "fv" );
    foreach my $this_filter_ ( @dep_based_filters_ )
    {
	my $is_valid = grep /^$this_filter_$/, @valid_filter_list_ ;
	if($is_valid == 0)
	{
	    print $main_log_file_handle_ "ERROR : filtration $this_filter_ not implemented\n";
	    print "ERROR : filtration $this_filter_ not implemented\n";
	    # exit(0);
	}
    }

    if ( !( $strategyname_ ) )
    {
	print $main_log_file_handle_ "STRATEGYNAME missing\n";
	exit ( 0 );
    }

    if ( ! ( $trading_start_yyyymmdd_ ) )
    {
	print $main_log_file_handle_ "TRADING_START_YYYYMMDD missing\n";
	exit ( 0 );
    }

    if ( ! ( ValidDate ( $trading_start_yyyymmdd_ ) ) )
    {
	print $main_log_file_handle_ "TRADING_START_YYYYMMDD not Valid\n";
	exit ( 0 );
    }

    if ( ! ( $trading_end_yyyymmdd_ ) )
    {
	print $main_log_file_handle_ "TRADING_END_YYYYMMDD missing\n";
	exit ( 0 );
    }

    if ( ! ( ValidDate ( $trading_end_yyyymmdd_ ) ) )
    {
	print $main_log_file_handle_ "TRADING_END_YYYYMMDD not Valid\n";
	exit ( 0 );
    }
    
    if ( ! ( $trading_start_hhmm_ ) )
    {
	print $main_log_file_handle_ "TRADING_START_HHMM missing\n";
	exit ( 0 );
    }

    if ( ! ( $trading_end_hhmm_ ) )
    {
	print $main_log_file_handle_ "TRADING_END_HHMM missing\n";
	exit ( 0 );
    }

    if ( ! ( $num_files_to_choose_ ) )
    {
	print $main_log_file_handle_ "NUM_FILES_TO_CHOOSE missing\n";
	exit ( 0 );
    }

    if ( int ( $num_files_to_choose_ ) <= 0 )
    {
	$num_files_to_choose_ = 1;
    }

    # foreach my $this_pred_duration_ ( keys %duration_to_param_filevec_ )
    # { # foreach duration for which at least one param file was made

    # 	my $scalar_ref_param_filevec_ = $duration_to_param_filevec_{$this_pred_duration_};
    # 	for ( my $param_filevec_index_ = 0 ; $param_filevec_index_ <= $#$scalar_ref_param_filevec_ ; $param_filevec_index_ ++ )
    # 	{ # foreach param_file generated for this duration
    # 	    my $this_param_filename_ = $$scalar_ref_param_filevec_[$param_filevec_index_];
    # 	    print STDERR "$this_pred_duration_ $this_param_filename_\n" ;
    # 	}
    # }
}

sub RunRegressMakeModelFiles 
{
    print $main_log_file_handle_ "RunRegressMakeModelFiles\n";

    if ( $RUN_SINGLE_PREDALGO )
    {
	# Randomly pick a single predalgo instead
	# of running them all.
	my $t_random_predalgo_index_ = int ( rand ( $#predalgo_ + 1 ) );
	my $t_random_predalgo_ = $predalgo_ [ $t_random_predalgo_index_ ];

	@predalgo_ = ( );
	push ( @predalgo_ , $t_random_predalgo_ );
    }
    
    my $t_iter = -1;
    foreach $indicator_list_filename_base_ ( @ilist_filename_base_vec_ ) ##
    { ## 
	$t_iter = $t_iter+1;
	$indicator_list_filename_ = $ilist_filename_vec_[$t_iter];
	print $main_log_file_handle_ "ILIST_FILE = $indicator_list_filename_\n";

	foreach my $this_pred_duration_ ( @predduration_ )
	{ # for each pred_duration

	    my $this_work_dir_ = $work_dir_."/".$this_pred_duration_;
	    if ( ! ( -d $this_work_dir_ ) ) { `mkdir -p $this_work_dir_`; }
	    
	    foreach my $this_predalgo_ ( @predalgo_ )
	    { # for each predalgo 

		# full reg_data file
		my $main_file_extension_ = $indicator_list_filename_base_."_".$this_pred_duration_."_".$this_predalgo_."_".$datagen_start_yyyymmdd_."_".$datagen_end_yyyymmdd_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_ ; 
		my $this_reg_data_filename_ = $reg_data_dir."/reg_data_".$main_file_extension_;
		if ( ! ( -d $reg_data_dir ) )
		{ 
		    print $main_file_extension_, "mkdir -p $reg_data_dir\n";
		    `mkdir -p $reg_data_dir`;
		}

		if ( -e $this_reg_data_filename_ )
		{ # if file exists already
		    # could be a bad sign
		    # `rm -f $this_reg_data_filename_`;
		    print $main_log_file_handle_ "ERROR: file $this_reg_data_filename_ already present\n";
		    print "ERROR: regfile $this_reg_data_filename_ already present\n";
		}
		else
		{ # generate reg data of this type
		    print $main_log_file_handle_ "generate reg data of this type\n";
		    print $main_log_file_handle_ "NUMDAYS $#datagen_day_vec_\n";
		    foreach my $tradingdate_ ( @datagen_day_vec_ )
		    {
			print $main_log_file_handle_ "for datagen date $tradingdate_\n";
                        # for this trading date generate the reg_data_file
			my $this_day_file_extension_ = $indicator_list_filename_base_."_".$this_pred_duration_."_".$this_predalgo_."_".$tradingdate_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_ ; 
			my $this_day_reg_data_filename_ = $reg_data_daily_dir."/reg_data_".$this_day_file_extension_;
			my $this_day_corr_filename_ = $reg_data_daily_dir."/corr_".$this_day_file_extension_;
			my $this_day_var_corr_filename_ = $reg_data_daily_dir."/var_corr_".$this_day_file_extension_;
			
			if ( $SAVE_CORR_FILE == 0 )
			{ push ( @intermediate_files_, $this_day_corr_filename_ ); }
			if ( $SAVE_VAR_CORR_FILE == 0 )
			{ push ( @intermediate_files_, $this_day_var_corr_filename_ ); }
			
			if ( ! ( -d $reg_data_daily_dir ) )
			{
			    print $main_log_file_handle_ "mkdir -p $reg_data_daily_dir\n"; 
			    `mkdir -p $reg_data_daily_dir`;
			}

			if ( ! ( -e $this_day_reg_data_filename_ ) )
			{   # reg_data_file missing 
			    # try to fetch the timed_data_file 
			    print $main_log_file_handle_ "reg_data_file $this_day_reg_data_filename_ missing \n try to fetch the timed_data_file \n";

			    if ( ! ( -d $timed_data_daily_dir ) )
			    {
				`mkdir -p $timed_data_daily_dir`;
			    }

			    my $this_day_timed_data_filename_ = $timed_data_daily_dir."/timed_data_".$indicator_list_filename_base_."_".$tradingdate_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_ ;
			    my $this_td_file_generated = 0; 
			    if ( ! ( -e $this_day_timed_data_filename_ ) )
			    { # generate this_day_timed_data_filename_
			        #check in indicator file if all sources are present for the given date
			        #if not, do not run datagen for this date
			        if ( CheckAbsentData($tradingdate_, $indicator_list_filename_) == 1)
			        {
			        	print $main_log_file_handle_ "skipping datagen for $indicator_list_filename_ for date $tradingdate_\n";
			        	next;
			        }
				my $exec_cmd="$LIVE_BIN_DIR/datagen $indicator_list_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
				print $main_log_file_handle_ "$exec_cmd\n";
				my @datagen_output_lines_ = `$exec_cmd`;
				if ( $USER eq "sghosh"  || $USER eq "ravi" )
				{
				    print $main_log_file_handle_ @datagen_output_lines_."\n";
				}

				my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$yyyymmdd_.".".$unique_gsm_id_;
				`rm -f $datagen_logfile_`;

				if ( ExistsWithSize ( $this_day_timed_data_filename_ ) )
				{
				    $this_td_file_generated = 1;
				    print $main_log_file_handle_ "Adding $this_day_timed_data_filename_ to be deleted later\n" ;
				    push ( @intermediate_files_, $this_day_timed_data_filename_ ); # if this is a new file we created then add it to set of files to delete at the end
				}
				else
				{
				    print $main_log_file_handle_ "Could not create non-zero-sized $this_day_timed_data_filename_\n" ;
				    print $main_log_file_handle_ "rm -f $this_day_timed_data_filename_\n" ;
				    `rm -f $this_day_timed_data_filename_`;
				}
			    }

			    if ( $large_price_move_periods_ == 1 && $this_td_file_generated == 1)
			    {
				# Filter for only periods with large directional price moves.
				my $this_day_large_price_timed_data_filename_ = $this_day_timed_data_filename_."_large_price";
				my $exec_cmd_ = "$MODELSCRIPTS_DIR/select_timed_data_rows_with_price_moves.pl $shortcode_ $this_day_timed_data_filename_ $this_day_large_price_timed_data_filename_ $large_price_move_time_period_ $large_price_move_thresh_factor_";

				print $main_log_file_handle_ "$exec_cmd_\n";
				`$exec_cmd_`;

				if ( ExistsWithSize ( $this_day_large_price_timed_data_filename_ ) )
				{
				    print $main_log_file_handle_ "Adding $this_day_large_price_timed_data_filename_ to be deleted later\n" ;
				    push ( @intermediate_files_, $this_day_large_price_timed_data_filename_ ); # if this is a new file we created then add it to set of files to delete at the end
				}
				else
				{
				    print $main_log_file_handle_ "Could not create non-zero-sized $this_day_large_price_timed_data_filename_\n" ;
				    print $main_log_file_handle_ "rm -f $this_day_large_price_timed_data_filename_\n" ;
				    `rm -f $this_day_large_price_timed_data_filename_`;
				}

				$this_day_timed_data_filename_ = $this_day_large_price_timed_data_filename_;
			    }

			    if ( $bad_periods_only_ == 1 && $this_td_file_generated == 1)
			    {
				# Only use periods where installed models performed badly.
				my $this_day_bad_period_timed_data_filename_ = $this_day_timed_data_filename_."_bad_periods";

				my $exec_cmd_ = "$MODELSCRIPTS_DIR/select_timed_data_rows_for_bad_periods.pl $this_day_timed_data_filename_ $this_day_bad_period_timed_data_filename_ $shortcode_ $tradingdate_";

				print $main_log_file_handle_ "$exec_cmd_\n";
				`$exec_cmd_`;

				if ( ExistsWithSize ( $this_day_bad_period_timed_data_filename_ ) )
				{
				    print $main_log_file_handle_ "Adding $this_day_bad_period_timed_data_filename_ to be deleted later\n" ;
				    push ( @intermediate_files_, $this_day_bad_period_timed_data_filename_ ); # if this is a new file we created then add it to set of files to delete at the end
				}
				else
				{
				    print $main_log_file_handle_ "Could not create non-zero-sized $this_day_bad_period_timed_data_filename_\n" ;
				    print $main_log_file_handle_ "rm -f $this_day_bad_period_timed_data_filename_\n" ;
				    `rm -f $this_day_bad_period_timed_data_filename_`;
				}

				$this_day_timed_data_filename_ = $this_day_bad_period_timed_data_filename_;
			    }

			    $apply_trade_vol_filter = 0;
			    for ( my $findex_ = 0; $findex_ <= $#dep_based_filters_; $findex_ ++ )
			    { # for every filtration of the main file
				my $this_filter_ = $dep_based_filters_[$findex_];
				if ( $this_filter_ eq "fv" )
				{
				    $apply_trade_vol_filter = 1;
				}
			    }

			    if ( -e $this_day_timed_data_filename_ )
			    { # if now timed_data_file exists and has non zero size
				my $this_day_wmean_reg_data_filename_ = $reg_data_daily_dir."/wmean_reg_data_".$this_day_file_extension_;
				# generate reg data file which is not mean zero
				{
				    my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( $shortcode_ , $this_pred_duration_, $this_predalgo_, $this_day_timed_data_filename_ );
                                    my $rnd_num_ =  int( rand(10000000) );
				    my $trade_per_sec_file = $DATAGEN_LOGDIR.$rnd_num_."_".$shortcode_."_trd_per_sec";
				    if($apply_trade_vol_filter == 1)
				    {
        				#do the trade volume file generation part
	               			my $exec_cmd="$LIVE_BIN_DIR/daily_trade_aggregator $shortcode_ $tradingdate_ $trade_per_sec_file";
					print $main_log_file_handle_ "$exec_cmd\n";
			             	my @daily_trade_agg_output_lines_ = `$exec_cmd`;
					print $main_log_file_handle_ @daily_trade_agg_output_lines_."\n";
        			    }
				    my $exec_cmd="$LIVE_BIN_DIR/timed_data_to_reg_data $indicator_list_filename_ $this_day_timed_data_filename_ $this_pred_counters_ $this_predalgo_ $this_day_wmean_reg_data_filename_";
				    if($apply_trade_vol_filter == 1){
				    	$exec_cmd = $exec_cmd." $trade_per_sec_file";
				    }
				    print $main_log_file_handle_ "$exec_cmd\n";
				    `$exec_cmd`;
				    if($apply_trade_vol_filter == 1){
				    	`rm $trade_per_sec_file`;
				    }
				    
				}

				if ( -e $this_day_wmean_reg_data_filename_ )
				{
				    # NOT REMOVING MEAN EVERYDAY ... SINCE ON SOME DAYS MEAN WAS SO MUCH THAT REMOVING MEAN WAS CAUSING WEIRD STUFF

				    # my $this_day_stats_reg_data_filename_ = $reg_data_daily_dir."/stats_reg_data_".$this_day_file_extension_;
				    # {
				    # 	my $exec_cmd="$LIVE_BIN_DIR/remove_mean_reg_data  $this_day_wmean_reg_data_filename_ $this_day_reg_data_filename_ $this_day_stats_reg_data_filename_";
				    # 	print $main_log_file_handle_ "$exec_cmd\n";
				    # 	`$exec_cmd`;
				    # }
				    # print $main_log_file_handle_ "rm -f $this_day_wmean_reg_data_filename_\n" ;
				    # `rm -f $this_day_wmean_reg_data_filename_`;
				    # if ( $SAVE_STATS_FILE == 0 )
				    # { push ( @intermediate_files_, $this_day_stats_reg_data_filename_ ); }
				    `mv $this_day_wmean_reg_data_filename_ $this_day_reg_data_filename_`; 
				}
				# remove mean, and generate statistics like mean, stdev and sharpe of the reg_data columns in a file 
				# TODO .. uses of this stats file:
				# see whether this data should be excluded if the stats of the dependant are abnormal
				# see which columns should be excluded, a column may be exculded if it's peak sharpe is very high
				# or more generally look at the distribution of sharpe

				if ( $SAVE_VAR_CORR_FILE != 0 )
				{
				    if ( -e $this_day_reg_data_filename_ )
				    { # compute correlations
					my $exec_cmd="$LIVE_BIN_DIR/get_dep_corr $this_day_reg_data_filename_ > $this_day_corr_filename_";
					print $main_log_file_handle_ "$exec_cmd\n";
					`$exec_cmd`;

					$exec_cmd="$MODELSCRIPTS_DIR/sort_by_corr.pl $indicator_list_filename_ $this_day_corr_filename_ > $this_day_var_corr_filename_";
					print $main_log_file_handle_ "$exec_cmd\n";				    
					`$exec_cmd`;
				    }
				}
			    }
			}
			
			if ( -e $this_day_reg_data_filename_ )
			{ # if now the reg data file exists
			    
#			    my $this_day_stats_reg_data_filename_ = $reg_data_daily_dir."/stats_reg_data_".$this_day_file_extension_;
#			  my @stats_for_day_ = LoadRegDataStats ( $this_day_stats_reg_data_filename_ );
#			  if ( $stats_for_day_[0]->sharpe() < 0.20 )
#			  {
			    # append this day's file to the global reg data file
			    my $exec_cmd="cat $this_day_reg_data_filename_ >> $this_reg_data_filename_";
			    print $main_log_file_handle_ "$exec_cmd\n";
			    `$exec_cmd`;
#			  }

			    push ( @intermediate_files_, $this_day_reg_data_filename_ );
			    if ( $delete_intermediate_files_ )
			    {
				if ( -e $this_day_reg_data_filename_ )
				{
				    print $main_log_file_handle_ "rm -f $this_day_reg_data_filename_\n" ;
				    `rm -f $this_day_reg_data_filename_`;
				}
			    }
			}
		    }
		}

		if ( ! ( -e $this_reg_data_filename_ ) )
		{
		    print $main_log_file_handle_ "ERROR Even after all this the reg_data file does not exist!\n";
		    print "ERROR Even after all this the reg_data file does not exist!\n";
		}
		else
		{ # now $this_reg_data_filename_ exists
		    ComputeStatsSingleRegFile ( $main_file_extension_ );

		    if ( $RUN_SINGLE_DEP_BASED_FILTER )
			# if ( $apply_all_dep_filters_ == 0 )
		    {
			# Randomly pick a single dep-based-filter instead
			# of running them all.
			my $t_random_dep_based_filter_index_ = int ( rand ( $#dep_based_filters_ + 1 ) );
			my $t_random_dep_based_filter_ = $dep_based_filters_ [ $t_random_dep_based_filter_index_ ];

			@dep_based_filters_ = ( );
			push ( @dep_based_filters_ , $t_random_dep_based_filter_ );
		    }

		    for ( my $findex_ = 0; $findex_ <= $#dep_based_filters_; $findex_ ++ )
		    { # for every filtration of the main file
			my $this_filter_ = $dep_based_filters_[$findex_];
			my $this_filtered_main_file_extension_ = $main_file_extension_."_".$this_filter_;
			my $this_filtered_reg_data_filename_ = $reg_data_dir."/reg_data_".$this_filtered_main_file_extension_;
			if ( ! ExistsWithSize ( $this_filtered_reg_data_filename_ ) )
			{
			    push ( @intermediate_files_, $this_filtered_reg_data_filename_ );
			    my $exec_cmd="$MODELSCRIPTS_DIR/apply_dep_filter.pl $shortcode_ $this_reg_data_filename_ $this_filter_  $this_filtered_reg_data_filename_  $trading_start_yyyymmdd_ " ;
                            print $main_log_file_handle_ "$exec_cmd\n";
                            `$exec_cmd`;
			}

			if ( ! ExistsWithSize ( $this_filtered_reg_data_filename_ ) )
			{
			    print "ERROR $this_filtered_reg_data_filename_ stil missing or 0sized\n";
			}
			else
			{
			    if ( $RUN_SINGLE_REGRESS_EXEC )
			    {
				# Randomly pick a single regress-exec instead
				# of running them all.
				my $t_random_regress_exec_index_ = int ( rand ( $#regress_exec_ + 1 ) );
				my $t_random_regress_exec_ = $regress_exec_ [ $t_random_regress_exec_index_ ];
				
				@regress_exec_ = ( );
				push ( @regress_exec_ , $t_random_regress_exec_ );
			    }
			    
			    for ( my $i = 0 ; $i <= $#regress_exec_; $i ++ ) 
			    {
				my $scalar_ref_ = $regress_exec_[$i];
				if ( $#$scalar_ref_ < 0 ) 
				{ next; }

				my $regtype_ = $$scalar_ref_[0];
				given ( $regtype_ ) 
				{
				    when ( "FSLR" )
				    {
					if ( $#$scalar_ref_ >= 4 ) 
					{
					    my $min_correlation_ = $$scalar_ref_[1];
					    my $first_indep_weight_ = $$scalar_ref_[2];
					    my $must_include_first_k_independants_ = $$scalar_ref_[3];
					    my $max_indep_correlation_ = $$scalar_ref_[4];

					    my $max_model_size_ = 20;
					    if ( $#$scalar_ref_ >= 5 ) 
					    { $max_model_size_ = $$scalar_ref_[5]; }				    

					    my $regress_dotted_string_ = join ( '_', ( $regtype_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_ ) );
					    # print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";
					    
					    my $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
					    my $this_unscaled_model_filename_ = $this_work_dir_."/unscaled_model_".$f_r_extension_;
					    my $this_regression_output_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_;
					    
					    {
						my @high_sharpe_indep_index_ = GetIndexOfHighSharpeIndep ( $indicator_list_filename_, $avoid_high_sharpe_indep_check_index_filename_ );
						print $main_log_file_handle_ "high_sharpe_indep_index_: @high_sharpe_indep_index_";
						
						my $exec_cmd="$LIVE_BIN_DIR/callFSLR $this_filtered_reg_data_filename_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $this_regression_output_filename_ $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_ $indicator_list_filename_ ";
						print $main_log_file_handle_ "$exec_cmd\n";
						my @regoutlines_ = `$exec_cmd 2>&1`; 
						print $main_log_file_handle_ "@regoutlines_\n";
						chomp ( @regoutlines_ );
						my $is_dep_high_sharpe_ = 0;
						my @high_sharpe_indep_indices_ = ();
						for ( my $regoutlines_idx_ = 0; $regoutlines_idx_ <= $#regoutlines_; $regoutlines_idx_ ++ )
						{
						    if ( $regoutlines_[ $regoutlines_idx_ ] =~ /Sharpe of indep/ )
						    {
							my $high_sharpe_line_ = $regoutlines_[ $regoutlines_idx_ ];
							my @high_sharpe_line_words_ = split ( ' ', $high_sharpe_line_ );
							if ( $#high_sharpe_line_words_ >= 4 )
							{ # Sharpe of indep ( 40 )
							    push ( @high_sharpe_indep_indices_, $high_sharpe_line_words_[4] );
							}
						    }
						    
						    if ( $regoutlines_[ $regoutlines_idx_ ] =~ /Sharpe of the dependant/ )
						    {
							$is_dep_high_sharpe_ = 1;
							print $main_log_file_handle_ "For $this_filtered_reg_data_filename_ Dependant has high sharpe\n";
#						print STDERR "For $this_filtered_reg_data_filename_ Dependant has high sharpe\n";
						    }
						}
						@high_sharpe_indep_indices_ = GetUniqueList ( @high_sharpe_indep_indices_ );
						my @high_sharpe_indep_text_ = GetHighSharpeIndepText ( $indicator_list_filename_, @high_sharpe_indep_indices_ );
						for ( my $hsit_idx_ = 0 ; $hsit_idx_ <= $#high_sharpe_indep_text_ ; $hsit_idx_ ++ )
						{
						    print $main_log_file_handle_ "For $this_filtered_reg_data_filename_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
#					    print STDERR "For $this_filtered_reg_data_filename_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
						}
					    }

					    if ( ExistsWithSize ( $this_regression_output_filename_ ) )
					    {
						my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_unscaled_model_filename_ $indicator_list_filename_ $this_regression_output_filename_ ";
						print $main_log_file_handle_ "$exec_cmd\n";
						`$exec_cmd`;
					    }
					    else
					    {
						print $main_log_file_handle_ "ERROR missing $this_regression_output_filename_\n";
					    }

#				    if ( ( ExistsWithSize ( $this_unscaled_model_filename_ ) ) &&
#					   ( IsModelCorrConsistent ( $this_unscaled_model_filename_ ) ) )
					    if ( ExistsWithSize ( $this_unscaled_model_filename_ ) ) # Allow inconsistent models.
					    {
						if ( ( $#target_stdev_model_vec_ < 0 ) ||
						     ( ( $#target_stdev_model_vec_ == 0 ) && ( $target_stdev_model_vec_[0] <= 0 ) ) )
						{
						    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;

						    print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
						    `mv $this_unscaled_model_filename_ $this_model_filename_`;

						    if ( ExistsWithSize ( $this_model_filename_ ) )
						    {
							AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
						    }
						    else
						    {
							print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
						    }
						}
						else
						{
						    my $exec_cmd="$MODELSCRIPTS_DIR/get_stdev_model.pl $this_unscaled_model_filename_ TODAY-4 TODAY-1 $datagen_start_hhmm_ $datagen_end_hhmm_ | head -n1 | awk '{print \$1}'";
						    print $main_log_file_handle_ "$exec_cmd\n";
						    my $current_stdev_model_ = `$exec_cmd`; chomp($current_stdev_model_);
						    print $main_log_file_handle_ "Current Stdev: $current_stdev_model_\n";
						    
						    if ( $current_stdev_model_ <= 0 )
						    {
							my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;

							print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
							`mv $this_unscaled_model_filename_ $this_model_filename_`;

							if ( ExistsWithSize ( $this_model_filename_ ) )
							{
							    AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
							}
							else
							{
							    print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
							}
						    }
						    else
						    {
							my $rs_i_ = 0;
							foreach my $target_stdev_model_ ( @target_stdev_model_vec_ ) 
							{
							    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_."_".$rs_i_;
							    
							    if ( $target_stdev_model_ > 0 )
							    {
								my $scale_const_ = $target_stdev_model_ / $current_stdev_model_ ;
								if ( $scale_const_ < 0 )
								{ # not sure how but some how
								    print $main_log_file_handle_ "cp $this_unscaled_model_filename_ $this_model_filename_\n";
								    `cp $this_unscaled_model_filename_ $this_model_filename_`;
								}
								else
								{
								    print $main_log_file_handle_ "For target_stdev_model $target_stdev_model_ and Current Stdev: $current_stdev_model_ scale_const = $scale_const_\n";
								    $exec_cmd="$MODELSCRIPTS_DIR/rescale_model.pl $this_unscaled_model_filename_ $this_model_filename_ $scale_const_";
								    print $main_log_file_handle_ "$exec_cmd\n";
								    `$exec_cmd`;
								}
							    }
							    else
							    { # this allows us to specify -1
								print $main_log_file_handle_ "cp $this_unscaled_model_filename_ $this_model_filename_\n";
								`cp $this_unscaled_model_filename_ $this_model_filename_`;
							    }
							    
							    if ( ExistsWithSize ( $this_model_filename_ ) )
							    {
								AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
								$rs_i_ ++;
							    }
							    else
							    {
								print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
							    }
							}
						    }
						}
					    }

					}
					else
					{
					    print $main_log_file_handle_ "FSLR line now needs 5 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
					}
				    }
				    when ( "FSHLR" )
				    {
					if ( $#$scalar_ref_ >= 4 ) 
					{
					    my $min_correlation_ = $$scalar_ref_[1];
					    my $first_indep_weight_ = $$scalar_ref_[2];
					    my $must_include_first_k_independants_ = $$scalar_ref_[3];
					    my $max_indep_correlation_ = $$scalar_ref_[4];

					    my $max_model_size_ = 20;
					    if ( $#$scalar_ref_ >= 5 ) 
					    { $max_model_size_ = $$scalar_ref_[5]; }				    
					    my $regress_dotted_string_ = join ( '_', ( $regtype_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_ ) );
					    # print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";

					    my $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
					    my $this_unscaled_model_filename_ = $this_work_dir_."/unscaled_model_".$f_r_extension_;
					    my $this_regression_output_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_;
					    
					    {
						my @high_sharpe_indep_index_ = GetIndexOfHighSharpeIndep ( $indicator_list_filename_, $avoid_high_sharpe_indep_check_index_filename_ );
						print $main_log_file_handle_ "high_sharpe_indep_index_: @high_sharpe_indep_index_";
						
						my $exec_cmd="$LIVE_BIN_DIR/callFSHLR $this_filtered_reg_data_filename_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $this_regression_output_filename_ $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_ $indicator_list_filename_";
						print $main_log_file_handle_ "$exec_cmd\n";
#					`$exec_cmd`;
						my @regoutlines_ = `$exec_cmd 2>&1`; 
						chomp ( @regoutlines_ );
						my $is_dep_high_sharpe_ = 0;
						my @high_sharpe_indep_indices_ = ();
						for ( my $regoutlines_idx_ = 0; $regoutlines_idx_ <= $#regoutlines_; $regoutlines_idx_ ++ )
						{
						    if ( $regoutlines_[ $regoutlines_idx_ ] =~ /Sharpe of indep/ )
						    {
							my $high_sharpe_line_ = $regoutlines_[ $regoutlines_idx_ ];
							my @high_sharpe_line_words_ = split ( ' ', $high_sharpe_line_ );
							if ( $#high_sharpe_line_words_ >= 4 )
							{ # Sharpe of indep ( 40 )
							    push ( @high_sharpe_indep_indices_, $high_sharpe_line_words_[4] );
							}
						    }
						    
						    if ( $regoutlines_[ $regoutlines_idx_ ] =~ /Sharpe of the dependant/ )
						    {
							$is_dep_high_sharpe_ = 1;
							print $main_log_file_handle_ "For $this_filtered_reg_data_filename_ Dependant has high sharpe\n";
#						print STDERR "For $this_filtered_reg_data_filename_ Dependant has high sharpe\n";
#						print $main_log_file_handle_ "Dependant has high sharpe\n";
						    }
						}
						@high_sharpe_indep_indices_ = GetUniqueList ( @high_sharpe_indep_indices_ );
						my @high_sharpe_indep_text_ = GetHighSharpeIndepText ( $indicator_list_filename_, @high_sharpe_indep_indices_ );
						for ( my $hsit_idx_ = 0 ; $hsit_idx_ <= $#high_sharpe_indep_text_ ; $hsit_idx_ ++ )
						{
						    print $main_log_file_handle_ "For $this_filtered_reg_data_filename_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
#					    print STDERR "For $this_filtered_reg_data_filename_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
						}
					    }

					    if ( ExistsWithSize ( $this_regression_output_filename_ ) )
					    {
						my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_unscaled_model_filename_ $indicator_list_filename_ $this_regression_output_filename_ ";
						print $main_log_file_handle_ "$exec_cmd\n";
						`$exec_cmd`;
					    }
					    else
					    {
						print $main_log_file_handle_ "ERROR: missing $this_regression_output_filename_\n";
					    }

					    if ( ExistsWithSize ( $this_unscaled_model_filename_ ) )
					    {
						if ( ( $#target_stdev_model_vec_ < 0 ) ||
						     ( ( $#target_stdev_model_vec_ == 0 ) && ( $target_stdev_model_vec_[0] <= 0 ) ) )
#					if ( $#target_stdev_model_vec_ < 0 )
						{
						    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;

						    print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
						    `mv $this_unscaled_model_filename_ $this_model_filename_`;
						    if ( ExistsWithSize ( $this_model_filename_ ) )
						    {
							AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
						    }
						    else
						    {
							print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
						    }
						}
						else
						{
						    my $exec_cmd="$MODELSCRIPTS_DIR/get_stdev_model.pl $this_unscaled_model_filename_ TODAY-4 TODAY-1 $datagen_start_hhmm_ $datagen_end_hhmm_ | head -n1 | awk '{print \$1}'";
						    print $main_log_file_handle_ "$exec_cmd\n";
						    my $current_stdev_model_ = `$exec_cmd`; chomp($current_stdev_model_);
						    print $main_log_file_handle_ "Current Stdev: $current_stdev_model_\n";
						    
						    if ( $current_stdev_model_ <= 0 )
						    {
							my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;

							print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
							`mv $this_unscaled_model_filename_ $this_model_filename_`;
							if ( ExistsWithSize ( $this_model_filename_ ) )
							{
							    AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
							}
							else
							{
							    print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
							}
						    }
						    else
						    {
							my $rs_i_ = 0;
							foreach my $target_stdev_model_ ( @target_stdev_model_vec_ ) 
							{
							    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_."_".$rs_i_;
							    
							    my $scale_const_ = $target_stdev_model_ / $current_stdev_model_ ;
							    if ( $scale_const_ < 0 )
							    { # not sure how but some how
								print $main_log_file_handle_ "cp $this_unscaled_model_filename_ $this_model_filename_\n";
								`cp $this_unscaled_model_filename_ $this_model_filename_`;
							    }
							    else
							    {
								print $main_log_file_handle_ "For target_stdev_model $target_stdev_model_ and Current Stdev: $current_stdev_model_ scale_const = $scale_const_\n";
								$exec_cmd="$MODELSCRIPTS_DIR/rescale_model.pl $this_unscaled_model_filename_ $this_model_filename_ $scale_const_";
								print $main_log_file_handle_ "$exec_cmd\n";
								`$exec_cmd`;
							    }
							    
							    if ( ExistsWithSize ( $this_model_filename_ ) )
							    {
								AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
								$rs_i_ ++;
							    }
							    else
							    {
								print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
							    }
							}
						    }
						}
					    }

					}
					else
					{
					    print $main_log_file_handle_ "FSHLR line now needs 5 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
					}
				    }
				    when ( "FSVLR" )
				    {
					if ( $#$scalar_ref_ >= 5 ) 
					{
					    my $stdev_thresh_factor_ = $$scalar_ref_[1];
					    my $min_correlation_ = $$scalar_ref_[2];
					    my $first_indep_weight_ = $$scalar_ref_[3];
					    my $must_include_first_k_independants_ = $$scalar_ref_[4];
					    my $max_indep_correlation_ = $$scalar_ref_[5];

					    my $max_model_size_ = 14;
					    if ( $#$scalar_ref_ >= 6 ) 
					    { $max_model_size_ = $$scalar_ref_[6]; }

					    my $regress_dotted_string_ = join ( '_', ( $regtype_, $stdev_thresh_factor_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_ ) );
					    # print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";

					    my $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
					    my $this_unscaled_model_filename_ = $this_work_dir_."/unscaled_model_".$f_r_extension_;
					    my $this_regression_output_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_;
					    
					    {
						my @high_sharpe_indep_index_ = GetIndexOfHighSharpeIndep ( $indicator_list_filename_, $avoid_high_sharpe_indep_check_index_filename_ );
						print $main_log_file_handle_ "high_sharpe_indep_index_: @high_sharpe_indep_index_";
						
						my $exec_cmd="$LIVE_BIN_DIR/callFSVLR $this_filtered_reg_data_filename_ $stdev_thresh_factor_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $this_regression_output_filename_ $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_ $indicator_list_filename_ ";
						print $main_log_file_handle_ "$exec_cmd\n";
						my @regoutlines_ = `$exec_cmd 2>&1`; 
						chomp ( @regoutlines_ );
						my $is_dep_high_sharpe_ = 0;
						my @high_sharpe_indep_indices_ = ();
						for ( my $regoutlines_idx_ = 0; $regoutlines_idx_ <= $#regoutlines_; $regoutlines_idx_ ++ )
						{
						    if ( $regoutlines_[ $regoutlines_idx_ ] =~ /Sharpe of indep/ )
						    {
							my $high_sharpe_line_ = $regoutlines_[ $regoutlines_idx_ ];
							my @high_sharpe_line_words_ = split ( ' ', $high_sharpe_line_ );
							if ( $#high_sharpe_line_words_ >= 4 )
							{ # Sharpe of indep ( 40 )
							    push ( @high_sharpe_indep_indices_, $high_sharpe_line_words_[4] );
							}
						    }
						    
						    if ( $regoutlines_[ $regoutlines_idx_ ] =~ /Sharpe of the dependant/ )
						    {
							$is_dep_high_sharpe_ = 1;
							print $main_log_file_handle_ "For $this_filtered_reg_data_filename_ Dependant has high sharpe\n";
#							print STDERR "For $this_filtered_reg_data_filename_ Dependant has high sharpe\n";
#							print $main_log_file_handle_ "Dependant has high sharpe\n";
						    }
						}
						@high_sharpe_indep_indices_ = GetUniqueList ( @high_sharpe_indep_indices_ );
						my @high_sharpe_indep_text_ = GetHighSharpeIndepText ( $indicator_list_filename_, @high_sharpe_indep_indices_ );
						for ( my $hsit_idx_ = 0 ; $hsit_idx_ <= $#high_sharpe_indep_text_ ; $hsit_idx_ ++ )
						{
						    print $main_log_file_handle_ "For $this_filtered_reg_data_filename_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
#						    print STDERR "For $this_filtered_reg_data_filename_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
						}
					    }

					    if ( ExistsWithSize ( $this_regression_output_filename_ ) )
					    {
						my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_unscaled_model_filename_ $indicator_list_filename_ $this_regression_output_filename_ ";
						print $main_log_file_handle_ "$exec_cmd\n";
						`$exec_cmd`;
					    }
					    else
					    {
						print $main_log_file_handle_ "ERROR missing $this_regression_output_filename_\n";
					    }

					    if ( ExistsWithSize ( $this_unscaled_model_filename_ ) )
					    {
						if ( ( $#target_stdev_model_vec_ < 0 ) ||
						     ( ( $#target_stdev_model_vec_ == 0 ) && ( $target_stdev_model_vec_[0] <= 0 ) ) )
#					if ( $#target_stdev_model_vec_ < 0 )
						{
						    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;
						    
						    print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
						    `mv $this_unscaled_model_filename_ $this_model_filename_`;
						    if ( ExistsWithSize ( $this_model_filename_ ) )
						    {
							AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
						    }
						    else
						    {
							print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
						    }
						}
						else
						{
						    my $exec_cmd="$MODELSCRIPTS_DIR/get_stdev_model.pl $this_unscaled_model_filename_ TODAY-4 TODAY-1 $datagen_start_hhmm_ $datagen_end_hhmm_ | head -n1 | awk '{print \$1}'";
						    print $main_log_file_handle_ "$exec_cmd\n";
						    my $current_stdev_model_ = `$exec_cmd`; chomp($current_stdev_model_);
						    print $main_log_file_handle_ "Current Stdev: $current_stdev_model_\n";
						    
						    if ( $current_stdev_model_ <= 0 )
						    {
							my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;

							print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
							`mv $this_unscaled_model_filename_ $this_model_filename_`;
							if ( ExistsWithSize ( $this_model_filename_ ) )
							{
							    AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
							}
							else
							{
							    print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
							}
						    }
						    else
						    {
							my $rs_i_ = 0;
							foreach my $target_stdev_model_ ( @target_stdev_model_vec_ ) 
							{
							    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_."_".$rs_i_;
							    
							    my $scale_const_ = $target_stdev_model_ / $current_stdev_model_ ;
							    if ( $scale_const_ < 0 )
							    { # not sure how but some how
								print $main_log_file_handle_ "cp $this_unscaled_model_filename_ $this_model_filename_\n";
								`cp $this_unscaled_model_filename_ $this_model_filename_`;
							    }
							    else
							    {
								print $main_log_file_handle_ "For target_stdev_model $target_stdev_model_ and Current Stdev: $current_stdev_model_ scale_const = $scale_const_\n";
								$exec_cmd="$MODELSCRIPTS_DIR/rescale_model.pl $this_unscaled_model_filename_ $this_model_filename_ $scale_const_";
								print $main_log_file_handle_ "$exec_cmd\n";
								`$exec_cmd`;
							    }
							    
							    if ( ExistsWithSize ( $this_model_filename_ ) )
							    {
								AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
								$rs_i_ ++;
							    }
							    else
							    {
								print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
							    }
							}
						    }
						}
					    }
					    
					}
					else
					{
					    print $main_log_file_handle_ "FSVLR line now needs 6 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
					}
				    }
				    when ( "FSHDVLR" )
				    {
					if ( $#$scalar_ref_ >= 4 ) 
					{
					    my $min_correlation_ = $$scalar_ref_[1];
					    my $first_indep_weight_ = $$scalar_ref_[2];
					    my $must_include_first_k_independants_ = $$scalar_ref_[3];
					    my $max_indep_correlation_ = $$scalar_ref_[4];
					    
					    my $max_model_size_ = 20;
					    if ( $#$scalar_ref_ >= 5 ) 
					    { $max_model_size_ = $$scalar_ref_[5]; }				    

					    my $regress_dotted_string_ = join ( '_', ( $regtype_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_ ) );
					    # print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";
					    
					    my $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
					    my $this_unscaled_model_filename_ = $this_work_dir_."/unscaled_model_".$f_r_extension_;
					    my $this_regression_output_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_;
					    
					    {
						my @high_sharpe_indep_index_ = GetIndexOfHighSharpeIndep ( $indicator_list_filename_, $avoid_high_sharpe_indep_check_index_filename_ );
						print $main_log_file_handle_ "high_sharpe_indep_index_: @high_sharpe_indep_index_";
						
						my $exec_cmd="$LIVE_BIN_DIR/callFSHDVLR $this_filtered_reg_data_filename_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $this_regression_output_filename_ $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_ $indicator_list_filename_ ";
						print $main_log_file_handle_ "$exec_cmd\n";
#					`$exec_cmd`;
						my @regoutlines_ = `$exec_cmd 2>&1`; 
						chomp ( @regoutlines_ );
						my $is_dep_high_sharpe_ = 0;
						my @high_sharpe_indep_indices_ = ();
						for ( my $regoutlines_idx_ = 0; $regoutlines_idx_ <= $#regoutlines_; $regoutlines_idx_ ++ )
						{
						    if ( $regoutlines_[ $regoutlines_idx_ ] =~ /Sharpe of indep/ )
						    {
							my $high_sharpe_line_ = $regoutlines_[ $regoutlines_idx_ ];
							my @high_sharpe_line_words_ = split ( ' ', $high_sharpe_line_ );
							if ( $#high_sharpe_line_words_ >= 4 )
							{ # Sharpe of indep ( 40 )
							    push ( @high_sharpe_indep_indices_, $high_sharpe_line_words_[4] );
							}
						    }
						    
						    if ( $regoutlines_[ $regoutlines_idx_ ] =~ /Sharpe of the dependant/ )
						    {
							$is_dep_high_sharpe_ = 1;
							print $main_log_file_handle_ "For $this_filtered_reg_data_filename_ Dependant has high sharpe\n";
#						print STDERR "For $this_filtered_reg_data_filename_ Dependant has high sharpe\n";
#						print $main_log_file_handle_ "Dependant has high sharpe\n";
						    }
						}
						@high_sharpe_indep_indices_ = GetUniqueList ( @high_sharpe_indep_indices_ );
						my @high_sharpe_indep_text_ = GetHighSharpeIndepText ( $indicator_list_filename_, @high_sharpe_indep_indices_ );
						for ( my $hsit_idx_ = 0 ; $hsit_idx_ <= $#high_sharpe_indep_text_ ; $hsit_idx_ ++ )
						{
						    print $main_log_file_handle_ "For $this_filtered_reg_data_filename_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
#					    print STDERR "For $this_filtered_reg_data_filename_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
						}
					    }
					    
					    if ( ExistsWithSize ( $this_regression_output_filename_ ) )
					    {
						my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_unscaled_model_filename_ $indicator_list_filename_ $this_regression_output_filename_ ";
						print $main_log_file_handle_ "$exec_cmd\n";
						`$exec_cmd`;
					    }
					    else
					    {
						print $main_log_file_handle_ "ERROR missing $this_regression_output_filename_\n";
					    }
					    
					    if ( ExistsWithSize ( $this_unscaled_model_filename_ ) )
					    {
						if ( ( $#target_stdev_model_vec_ < 0 ) ||
						     ( ( $#target_stdev_model_vec_ == 0 ) && ( $target_stdev_model_vec_[0] <= 0 ) ) )
#					if ( $#target_stdev_model_vec_ < 0 )
						{
						    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;
						    
						    print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
						    `mv $this_unscaled_model_filename_ $this_model_filename_`;
						    if ( ExistsWithSize ( $this_model_filename_ ) )
						    {
							AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
						    }
						    else
						    {
							print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
						    }
						}
						else
						{
						    my $exec_cmd="$MODELSCRIPTS_DIR/get_stdev_model.pl $this_unscaled_model_filename_ TODAY-4 TODAY-1 $datagen_start_hhmm_ $datagen_end_hhmm_ | head -n1 | awk '{print \$1}'";
						    print $main_log_file_handle_ "$exec_cmd\n";
						    my $current_stdev_model_ = `$exec_cmd`; chomp($current_stdev_model_);
						    print $main_log_file_handle_ "Current Stdev: $current_stdev_model_\n";
						    
						    if ( $current_stdev_model_ <= 0 )
						    {
							my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;
							
							print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
							`mv $this_unscaled_model_filename_ $this_model_filename_`;
							if ( ExistsWithSize ( $this_model_filename_ ) )
							{
							    AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
							}
							else
							{
							    print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
							}
						    }
						    else
						    {
							my $rs_i_ = 0;
							foreach my $target_stdev_model_ ( @target_stdev_model_vec_ ) 
							{
							    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_."_".$rs_i_;
							    
							    my $scale_const_ = $target_stdev_model_ / $current_stdev_model_ ;
							    if ( $scale_const_ < 0 )
							    { # not sure how but some how
								print $main_log_file_handle_ "cp $this_unscaled_model_filename_ $this_model_filename_\n";
								`cp $this_unscaled_model_filename_ $this_model_filename_`;
							    }
							    else
							    {
								print $main_log_file_handle_ "For target_stdev_model $target_stdev_model_ and Current Stdev: $current_stdev_model_ scale_const = $scale_const_\n";
								$exec_cmd="$MODELSCRIPTS_DIR/rescale_model.pl $this_unscaled_model_filename_ $this_model_filename_ $scale_const_";
								print $main_log_file_handle_ "$exec_cmd\n";
								`$exec_cmd`;
							    }
							    
							    if ( ExistsWithSize ( $this_model_filename_ ) )
							    {
								AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
								$rs_i_ ++;
							    }
							    else
							    {
								print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
							    }
							}
						    }
						}
					    }
					}
					else
					{
					    print $main_log_file_handle_ "FSHDVLR line now needs 5 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
					}				    
				    }
				    when ( "FSRR" )
				    {
					if ( $#$scalar_ref_ >= 5 ) 
					{
					    my $regularization_coeff_ = $$scalar_ref_[1];
					    my $min_correlation_ = $$scalar_ref_[2];  
					    my $first_indep_weight_ = $$scalar_ref_[3];  
					    my $must_include_first_k_independants_ = $$scalar_ref_[4]; 
					    my $max_indep_correlation_ = $$scalar_ref_[5];
					    
					    my $max_model_size_ = 12;
					    if ( $#$scalar_ref_ >= 6 ) 
					    { $max_model_size_ = $$scalar_ref_[6]; }				    
					    
					    my $regress_dotted_string_ = join ( '.', ( $regtype_, $regularization_coeff_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_ ) );
					    # print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";
					    
					    my $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
					    my $this_unscaled_model_filename_ = $this_work_dir_."/unscaled_model_".$f_r_extension_;
					    my $this_regression_output_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_;
					    
					    {
						my @high_sharpe_indep_index_ = GetIndexOfHighSharpeIndep ( $indicator_list_filename_, $avoid_high_sharpe_indep_check_index_filename_ );
						print $main_log_file_handle_ "high_sharpe_indep_index_: @high_sharpe_indep_index_";
						
						my $exec_cmd="$LIVE_BIN_DIR/callFSRR $this_filtered_reg_data_filename_ $regularization_coeff_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $this_regression_output_filename_ $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_ $indicator_list_filename_ ";
						print $main_log_file_handle_ "$exec_cmd\n";
						my @regoutlines_ = `$exec_cmd 2>&1`; 
						chomp ( @regoutlines_ );
						my $is_dep_high_sharpe_ = 0;
						my @high_sharpe_indep_indices_ = ();
						for ( my $regoutlines_idx_ = 0; $regoutlines_idx_ <= $#regoutlines_; $regoutlines_idx_ ++ )
						{
						    if ( $regoutlines_[ $regoutlines_idx_ ] =~ /Sharpe of indep/ )
						    {
							my $high_sharpe_line_ = $regoutlines_[ $regoutlines_idx_ ];
							my @high_sharpe_line_words_ = split ( ' ', $high_sharpe_line_ );
							if ( $#high_sharpe_line_words_ >= 4 )
							{ # Sharpe of indep ( 40 )
							    push ( @high_sharpe_indep_indices_, $high_sharpe_line_words_[4] );
							}
						    }
						    
						    if ( $regoutlines_[ $regoutlines_idx_ ] =~ /Sharpe of the dependant/ )
						    {
							$is_dep_high_sharpe_ = 1;
							print $main_log_file_handle_ "For $this_filtered_reg_data_filename_ Dependant has high sharpe\n";
#						print STDERR "For $this_filtered_reg_data_filename_ Dependant has high sharpe\n";
#						print $main_log_file_handle_ "Dependant has high sharpe\n";
						    }
						}
						
						@high_sharpe_indep_indices_ = GetUniqueList ( @high_sharpe_indep_indices_ );
						my @high_sharpe_indep_text_ = GetHighSharpeIndepText ( $indicator_list_filename_, @high_sharpe_indep_indices_ );
						for ( my $hsit_idx_ = 0 ; $hsit_idx_ <= $#high_sharpe_indep_text_ ; $hsit_idx_ ++ )
						{
						    print $main_log_file_handle_ "For $this_filtered_reg_data_filename_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
#					    print STDERR "For $this_filtered_reg_data_filename_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
						}
					    }
					    
					    if ( ExistsWithSize ( $this_regression_output_filename_ ) )
					    {
						my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_unscaled_model_filename_ $indicator_list_filename_ $this_regression_output_filename_ ";
						print $main_log_file_handle_ "$exec_cmd\n";
						`$exec_cmd`;
					    }
					    else
					    {
						print $main_log_file_handle_ "ERROR missing $this_regression_output_filename_\n";
					    }
					    
					    if ( ExistsWithSize ( $this_unscaled_model_filename_ ) )
					    {
						if ( ( $#target_stdev_model_vec_ < 0 ) ||
						     ( ( $#target_stdev_model_vec_ == 0 ) && ( $target_stdev_model_vec_[0] <= 0 ) ) )
#					if ( $#target_stdev_model_vec_ < 0 )
						{
						    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;
						    
						    print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
						    `mv $this_unscaled_model_filename_ $this_model_filename_`;
						    if ( ExistsWithSize ( $this_model_filename_ ) )
						    {
							AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
						    }
						    else
						    {
							print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
						    }
						}
						else
						{
						    my $exec_cmd="$MODELSCRIPTS_DIR/get_stdev_model.pl $this_unscaled_model_filename_ TODAY-4 TODAY-1 $datagen_start_hhmm_ $datagen_end_hhmm_ | head -n1 | awk '{print \$1}'";
						    print $main_log_file_handle_ "$exec_cmd\n";
						    my $current_stdev_model_ = `$exec_cmd`; chomp($current_stdev_model_);
						    print $main_log_file_handle_ "Current Stdev: $current_stdev_model_\n";
						    
						    if ( $current_stdev_model_ <= 0 )
						    {
							my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;
							
							`mv $this_unscaled_model_filename_ $this_model_filename_`;
							if ( ExistsWithSize ( $this_model_filename_ ) )
							{
							    AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
							}
							else
							{
							    print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
							}
						    }
						    else
						    {
							my $rs_i_ = 0;
							foreach my $target_stdev_model_ ( @target_stdev_model_vec_ ) 
							{
							    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_."_".$rs_i_;
							    
							    my $scale_const_ = $target_stdev_model_ / $current_stdev_model_ ;
							    if ( $scale_const_ < 0 )
							    { # not sure how but some how
								print $main_log_file_handle_ "cp $this_unscaled_model_filename_ $this_model_filename_\n";
								`cp $this_unscaled_model_filename_ $this_model_filename_`;
							    }
							    else
							    {
								print $main_log_file_handle_ "For target_stdev_model $target_stdev_model_ and Current Stdev: $current_stdev_model_ scale_const = $scale_const_\n";
								$exec_cmd="$MODELSCRIPTS_DIR/rescale_model.pl $this_unscaled_model_filename_ $this_model_filename_ $scale_const_";
								print $main_log_file_handle_ "$exec_cmd\n";
								`$exec_cmd`;
							    }
							    
							    if ( ExistsWithSize ( $this_model_filename_ ) )
							    {
								AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
								$rs_i_ ++;
							    }
							    else
							    {
								print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
							    }
							}
						    }
						}
					    }
					    
					}
					else
					{
					    print $main_log_file_handle_ "FSRR line now needs 6 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
					}
				    }
				    default
				    {
					print $main_log_file_handle_ "Not handling regtype_ $regtype_ right now \n";
					# exit ( 0 );
				    }
				}
			    }
			}
			
			print $main_log_file_handle_ "Removing this filtration: rm -f $this_filtered_reg_data_filename_\n";
			`rm -f $this_filtered_reg_data_filename_`;
		    }
		}
		
		print $main_log_file_handle_ "rm -f $this_reg_data_filename_\n";
		`rm -f $this_reg_data_filename_`;
		
	    }
	} #endof pred_duration loop
    } #endof ilist loop
    
    if ( $delete_intermediate_files_ )
    {
	print $main_log_file_handle_ "Delete_Intermediate_Files\n";
	for ( my $i = 0 ; $i <= $#intermediate_files_; $i ++ )
	{
	    if ( -e $intermediate_files_[$i] )
	    {
		print $main_log_file_handle_ "rm -f $intermediate_files_[$i]\n" ;
		`rm -f $intermediate_files_[$i]`;
	    }
	}
    }
}
sub CheckAbsentData
{
	my $trading_date = $_[0];
	my $indicator_file = $_[1];
	open (FH, $indicator_file) or die "Couldn't open location file: $indicator_file";
	my @all_lines= <FH>;
	close FH;
	foreach my $line_(@all_lines)
	{
		$line_ =~ s/^\s+//;
		my @words_ = split(/\s+/, $line_);
		if ( $words_[0] ne "INDICATOR" ){
			next;
		}
		if ( $words_[2] eq "ImpliedPriceMPS" ){
			next;
		}
		my $e1 = `$SCRIPTS_DIR/check_indicator_data.sh $words_[3] $trading_date`;
		chomp($e1);
		if ( $e1 == 0) { return 1; }
		if ( $words_[4] =~ /[A-Za-z]/ && !($words_[4] =~ /Midprice|MktSinusoidal|MktSizeWPrice|OrderWPrice/ ) 
		     && ! ($words_[4] =~ /^[-+]?[0-9]*\.?[0-9]+$/) ){
		        my $e2 = `$SCRIPTS_DIR/check_indicator_data.sh $words_[4] $trading_date`;
			chomp($e2 );
			if ( $e2 == 0 ) { return 1; }
		}
		
	}
	return 0; #Everything Ok. We have data for all indicators
}

sub AddModelFileToList
{
#    print $main_log_file_handle_ "AddModelFileToList\n";
    
    my ( $this_pred_duration_, $this_model_filename_ ) = @_;
    if ( ( -e $this_model_filename_ ) &&
	 ( ( -f $this_model_filename_ ) > 0 ) )
    { # if the full model_file was successfully created
	if ( exists $duration_to_model_filevec_{$this_pred_duration_} )
	{
	    my $scalar_ref_ = $duration_to_model_filevec_{$this_pred_duration_} ;
	    push ( @$scalar_ref_, $this_model_filename_ );
	}
	else
	{
	    my @just_args_ = ();
	    push ( @just_args_, $this_model_filename_ );
	    $duration_to_model_filevec_{$this_pred_duration_} = [ @just_args_ ] ;
	}
    }
}

sub MakeStrategyFiles 
{
    print $main_log_file_handle_ "MakeStrategyFiles\n";
    
    my $strategy_progid_ = 1001;
    foreach my $this_pred_duration_ ( keys %duration_to_model_filevec_ )
    { # foreach duration for which at least one model file was made
	
	my $scalar_ref_model_filevec_ = $duration_to_model_filevec_{$this_pred_duration_};
	for ( my $model_filevec_index_ = 0 ; $model_filevec_index_ <= $#$scalar_ref_model_filevec_ ; $model_filevec_index_ ++ )
	{ # foreach model_file generated for this duration
	    # my $this_model_filename_ = $$scalar_ref_model_filevec_[$model_filevec_index_];
	    foreach my $this_model_filename_(CreateSubModelFiles($$scalar_ref_model_filevec_[$model_filevec_index_]))
	    {
		if ( exists $duration_to_param_filevec_{$this_pred_duration_} )
		{ # if at least one paramfile was specified for that duration
		    # print "$this_model_filename_\n";
		    my $scalar_ref_param_filevec_ = $duration_to_param_filevec_{$this_pred_duration_};		
		    for ( my $param_filevec_index_ = 0 ; $param_filevec_index_ <= $#$scalar_ref_param_filevec_ ; $param_filevec_index_ ++ )
		    { # foreach param_file input for this duration
			my $this_param_filename_ = $$scalar_ref_param_filevec_[$param_filevec_index_];
			my $this_strategy_filename_ = NameStrategyFromModelAndParamIndex ( $name_, $this_model_filename_, $param_filevec_index_, $trading_start_hhmm_, $trading_end_hhmm_ ) ;
			{
			    my $exec_cmd="$MODELSCRIPTS_DIR/create_strategy_file.pl $this_strategy_filename_ $shortcode_ $strategyname_ $this_model_filename_ $this_param_filename_ $trading_start_hhmm_ $trading_end_hhmm_ $strategy_progid_";
			    print $main_log_file_handle_ "$exec_cmd\n";
			    `$exec_cmd`;
			    $strategy_progid_++;
			}
			
			if ( -e $this_strategy_filename_ )
			{
			    push ( @strategy_filevec_, $this_strategy_filename_ ); 
			}
		    }
		}
	    }
	}	    
    }
}

sub RunSimulationOnCandidates
{
    print $main_log_file_handle_ "RunSimulationOnCandidates\n";
    
    # make a file with the full paths of all the strategyfiles
    # why ?
    # my $strategy_list_file_ = $work_dir_."/strategy_list_file.txt";
    # {
    # 	open STRATLIST, "> $strategy_list_file_" or PrintStacktraceAndDie ( "Could not open strategy_list_file $strategy_list_file_ for writing!\n" );
    # 	for ( my $i = 0 ; $i <= $#strategy_filevec_; $i++ )
    # 	{
    # 	    print STRATLIST $strategy_filevec_[$i]."\n";
    # 	}
    # 	close STRATLIST;
    # }
    
    my @non_unique_results_filevec_=();
    my $tradingdate_ = $trading_end_yyyymmdd_;
    my $max_days_at_a_time_ = 2000;
    for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
    {
	if ( SkipWeirdDate ( $tradingdate_ ) ||
	     ( NoDataDateForShortcode ( $tradingdate_ , $shortcode_ ) ) || 
	     ( IsDateHoliday ( $tradingdate_ ) || 
	       ( ( $shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $shortcode_ ) ) ) ) )
	{
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next;
	}
	
	if ( FindItemFromVec ( $tradingdate_ , @trading_exclude_days_ ) )
	{
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next;
	}
	
	if ( ( ! ValidDate ( $tradingdate_ ) ) ||
	     ( $tradingdate_ < $trading_start_yyyymmdd_ ) )
	{
	    last;
	}
	else 
	{
	    # for this $tradingdate_ break the @strategy_filevec_ into blocks of size MAX_STRAT_FILES_IN_ONE_SIM
	    # for a block print the filenames in a $temp_strategy_list_file_
	    # run sim_strategy, store output in @sim_strategy_output_lines_
	    # print lines in @sim_strategy_output_lines_ that have the word "SIMRESULT" into $temp_results_list_file_.
            # run add_results_to_local_database.pl ... push the file written to @non_unique_results_filevec_
	    my $strategy_filevec_front_ = 0;
	    my $temp_strategy_list_file_index_ = 0;
	    while ( $strategy_filevec_front_ <= $#strategy_filevec_ )
	    { # till there are more files in the list to service
		
		my $strategy_filevec_back_ = min ( ( $strategy_filevec_front_ + $MAX_STRAT_FILES_IN_ONE_SIM - 1 ), $#strategy_filevec_ ) ;
		
		my $temp_strategy_list_file_ = $work_dir_."/temp_strategy_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
		my $temp_strategy_cat_file_ = $work_dir_."/temp_strategy_cat_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
		open TSLF, "> $temp_strategy_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_ for writing\n" );
#		open TSCF, "> $temp_strategy_cat_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_cat_file_ for writing\n" );
		if ( -e $temp_strategy_cat_file_ ) { `rm -f $temp_strategy_cat_file_`; }
		for ( my $t_strategy_filevec_index_ = $strategy_filevec_front_; $t_strategy_filevec_index_ <= $strategy_filevec_back_; $t_strategy_filevec_index_ ++ )
		{
		    print TSLF $strategy_filevec_[$t_strategy_filevec_index_]."\n";
		    `cat $strategy_filevec_[$t_strategy_filevec_index_] >> $temp_strategy_cat_file_`;
		}
#		close TSCF;
		close TSLF;
		
		my @sim_strategy_output_lines_=(); # stored to seive out the SIMRESULT lines
		my %unique_id_to_pnlstats_map_=(); # stored to write extended results to the database
		{
		    
		    my $market_model_index_ = 0 ;
		    if ( $USER eq "sghosh" || 
			 ( $author_ && $author_ eq "sghosh" ) )
		    {
			$market_model_index_ = GetMarketModelForShortcode ( $shortcode_ );
		    }
		    
		    # Assuming glibc goes to stderr - unconfirmed.
		    my $exec_cmd="$LIVE_BIN_DIR/sim_strategy SIM $temp_strategy_cat_file_ $unique_gsm_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1 2>&1"; # using hyper optimistic market_model_index, added nologs argument
		    print $main_log_file_handle_ "$exec_cmd\n";
		    @sim_strategy_output_lines_=`$exec_cmd`;
		    
		    foreach my $t_sim_strategy_line_ ( @sim_strategy_output_lines_ )
		    {
			if ( index ( $t_sim_strategy_line_ , "glibc" ) >= 0 )
			{
			    print @sim_strategy_output_lines_."\n"; # Print if glibc detected.
			    last;
			}
		    }
		    
		    # To detect glibcs.
		    print $main_log_file_handle_ @sim_strategy_output_lines_."\n";
		    
		    my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);
		    if ( ExistsWithSize ( $this_tradesfilename_ ) )
		    {
			$exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_";
			print $main_log_file_handle_ "$exec_cmd\n";
			my @pnlstats_output_lines_ = `$exec_cmd`;
			for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
			{
			    my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
			    if( $#rwords_ >= 1 )
			    {
				my $unique_sim_id_ = $rwords_[0];
				splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_sim_id_
				$unique_id_to_pnlstats_map_{$unique_sim_id_} = join ( ' ', @rwords_ );
			    }
			}
		    }
		    
		    # added deletion of tradesfiles
		    if ( $SAVE_TRADELOG_FILE == 0 ) 
		    { 
			`rm -f $this_tradesfilename_`; 
			my $this_tradeslogfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int($unique_gsm_id_);
			`rm -f $this_tradeslogfilename_`;
		    }
		}
		
		
		my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
		
		open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );
		for ( my $t_sim_strategy_output_lines_index_ = 0, my $psindex_ = 0; $t_sim_strategy_output_lines_index_ <= $#sim_strategy_output_lines_; $t_sim_strategy_output_lines_index_ ++ )
		{
		    if ( $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] =~ /SIMRESULT/ )
		    { # SIMRESULT pnl volume sup% bestlevel% agg%
			my @rwords_ = split ( ' ', $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] );
			if ( $#rwords_ >= 2 ) 
			{
			    splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc
			    my $remaining_simresult_line_ = join ( ' ', @rwords_ );
			    if ( ( $rwords_[1] > 0 ) || # volume > 0
				 ( ( $shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day
			    {
				my $unique_sim_id_ = GetUniqueSimIdFromCatFile ( $temp_strategy_cat_file_, $psindex_ );
				if ( ! exists $unique_id_to_pnlstats_map_{$unique_sim_id_} )
				{
				    $unique_id_to_pnlstats_map_{$unique_sim_id_} = "0 0 0 0 0 0 0 0 0 0 0 0 0";
#				PrintStacktraceAndDie ( "unique_id_to_pnlstats_map_ missing $unique_sim_id_ for listfile: $temp_results_list_file_ catfile: $temp_strategy_cat_file_ rline: $remaining_simresult_line_\n" );
				}
				printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_ ;
				printf TRLF "%s %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_;
			    }
			}
			else
			{
			    PrintStacktraceAndDie ( "ERROR: SIMRESULT line has less than 3 words\n" ); 
			}
			$psindex_ ++;
		    }
		}
		close TRLF;
		
		if ( ExistsWithSize ( $temp_results_list_file_ ) )
		{
		    my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database.pl $temp_strategy_list_file_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir"; # TODO init $local_results_base_dir
		    print $main_log_file_handle_ "$exec_cmd\n";
		    my $this_local_results_database_file_ = `$exec_cmd`;
		    push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
		}
		
		if ( $delete_intermediate_files_ )
		{
		    if ( -e $temp_strategy_cat_file_ ) { `rm -f $temp_strategy_cat_file_`; }
		    if ( -e $temp_strategy_list_file_ ) { `rm -f $temp_strategy_list_file_`; }
		    if ( -e $temp_results_list_file_ ) { `rm -f $temp_results_list_file_`; }
		}
		$strategy_filevec_front_ = $strategy_filevec_back_ + 1; # front is now set to the first item of the next block
		$temp_strategy_list_file_index_ ++;
	    }
	}
	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    }
    @unique_results_filevec_ = GetUniqueList ( @non_unique_results_filevec_ ); # the same result file is written to by all blocks of strategy files on the same date, that's why @non_unique_results_filevec_ might have duplicate entries
}

sub getIlistFileNameFromStrategy
{
    my ( $this_strat_filename_) = @_;
    for my $t_ilist_fl_(@ilist_filename_base_vec_)
    {
	for my $t_pred_dur_ (@predduration_)
	{
	    my $t_string_ = "${name_}_${t_ilist_fl_}_$t_pred_dur_";
	    if ( $this_strat_filename_ =~ /$t_string_/g )
	    {
		return $t_ilist_fl_;
	    }
	}
    }
    return "Could not Determine. Please Check.";
}

sub getTimePeriodFromTime 
{
    my $dir_name_ = "$trading_start_hhmm_-$trading_end_hhmm_";
    my $retval_ = "NULL";
    if      (IsStratDirInTimePeriod($dir_name_, "EU_MORN_DAY") ) { $retval_ = "EU_MORN_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_MORN_DAY") ) { $retval_ = "US_MORN_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "AS_MORN")     ) { $retval_ = "AS_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "AS_DAY")      ) { $retval_ = "AS_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_EARLY_MORN")){ $retval_ = "US_EARLY_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_MORN")     ) { $retval_ = "US_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_DAY")      ) { $retval_ = "US_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_MORN")     ) { $retval_ = "US_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_DAY")      ) { $retval_ = "US_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "EU_MORN_DAY_US_DAY")) { $retval_ = "EU_MORN_DAY_US_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "EU_US_MORN_DAY") ) {$retval_ = "EU_US_MORN_DAY";}
    $retval_;
}

sub SummarizeLocalResultsAndChoose
{
    my $pnl_per_contract_word_index_ = 9; # PNL_PER_CPONTRACT is 10th word in STATISTICS line in summarize_single_strategy_results
    print $main_log_file_handle_ "SummarizeLocalResultsAndChoose\n";
    
    if ( $#unique_results_filevec_ >= 0 )
    {
	# Override for some shortcodes.
	# one strategy per execution is installed for FGBM FGBS FGBL FESX BR_DOL BR_IND CGB UB
	if ( $USER ne "sghosh" && $USER ne "ravi" )
	{
	    if ( ! $author_ || 
		 ( $author_ && $author_ ne "sghosh" ) )
	    {
		$min_num_files_to_choose_ = max ( $min_num_files_to_choose_ , GetMinNumFilesToChooseForShortcode ( $shortcode_ ) );
	    }
	}
	
	# Some very low volume strats are meaningless.
	if ( $USER ne "sghosh" && $USER ne "ravi" )
	{
	    if ( ! $author_ || 
		 ( $author_ && $author_ ne "sghosh" ) )
	    {
		$min_volume_to_allow_ = max ( $min_volume_to_allow_ , GetMinVolumeForShortcode ( $shortcode_ ) );
	    }
	}
	print $main_log_file_handle_ " Using min_num_files_to_choose_=".$min_num_files_to_choose_." min_volume_to_allow_=".$min_volume_to_allow_."\n";
	
	# take the list of local_results_database files @unique_results_filevec_
	my $exec_cmd="$LIVE_BIN_DIR/summarize_local_results_dir_and_choose_by_algo $historical_sort_algo_ $min_num_files_to_choose_ $num_files_to_choose_ $min_pnl_per_contract_to_allow_ $min_volume_to_allow_ $max_ttc_to_allow_ $local_results_base_dir";
	my @best_strat_statistics=`$LIVE_BIN_DIR/summarize_local_results_dir_and_choose_by_algo $historical_sort_algo_ 1 $min_num_files_to_choose_ $min_pnl_per_contract_to_allow_ $min_volume_to_allow_ $max_ttc_to_allow_ $local_results_base_dir`;
	print $main_log_file_handle_ "$exec_cmd\n";
	my @slrac_output_lines_=`$exec_cmd`;
	print $main_log_file_handle_ @slrac_output_lines_;
	print $main_log_file_handle_ "\n";
	#print @slrac_output_lines_;
	my $mail_body_ = "";
#	$mail_body_ = join ( ' ', @slrac_output_lines_ )."\n"; # not printing the script output directly now ... only summarize_single_strategy_results output later
	$mail_body_ .= "\n
**SummarizeStrategyParams:** \n
Sort_Algo_: $historical_sort_algo_
Min_Num_Files_to_Choose: $min_num_files_to_choose_
Num_Files_To_Choose: $num_files_to_choose_
Min_Pnl_Per_Contract: $min_pnl_per_contract_to_allow_
Min_Volum: $min_volume_to_allow_
Max_TTC: $max_ttc_to_allow_\n===============================\n\n";
	
	my @strat_files_selected_ = ();
	my %strat_indices_ = ();
	my $num_strats_in_global_results_=0;

	my $datagen_start_end_str_ = $datagen_start_hhmm_."-".$datagen_end_hhmm_ ;
	my $trading_start_end_str_ = $trading_start_hhmm_."-".$trading_end_hhmm_ ;
	
	if ($use_median_cutoff_)
	{
	    # adding global result check
	    print $main_log_file_handle_ "Using Median Cutoff\n";
	    my $global_results_dir_path = "/NAS1/ec2_globalresults/$shortcode_/";
	    my $srv_name_=`hostname | cut -d'-' -f2`; chomp($srv_name_);
	    if($srv_name_ =~ "crt"){
		$global_results_dir_path = $HOME_DIR."/ec2_globalresults/$shortcode_/";
	    }
	    (my $localresults_dir_path = $unique_results_filevec_[0]) =~ s/201[1234]\/.*//;
	    my @global_results_files = @unique_results_filevec_;
	    foreach my $fl_(@global_results_files){
		my $global_fl_ = $fl_; $global_fl_ =~ s/$localresults_dir_path/$global_results_dir_path/g;
		print $main_log_file_handle_ "cat $global_fl_ >> $fl_\n";
		`cat $global_fl_ >> $fl_`;
	    }
	    
	    my $timeperiod_ = getTimePeriodFromTime();
	    print $main_log_file_handle_ "TimePeriod: $timeperiod_\n";
	    my $cstempfile_ = GetCSTempFileName ( $work_dir_."/cstemp" );
	    open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
	    my @exclude_tp_dirs_ = ();
	    my @all_strats_in_dir_ = MakeStratVecFromDirInTpExcludingSets ( $MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_, @exclude_tp_dirs_ );
	    print $main_log_file_handle_ "All global strats: ".join("\n", @all_strats_in_dir_)."\n";
	    for(my $i=0; $i < $#all_strats_in_dir_; $i++){
		my $t_strat_file_ = basename($all_strats_in_dir_[$i]);
		print CSTF "$t_strat_file_\n";
	    }
	    my $num_strats_in_global_results_ = $#all_strats_in_dir_ + 1;
	    for (@slrac_output_lines_){
		if ( $_ =~ /STRATEGYFILEBASE/ ){
		    my $strat_line = $_;
		    my $strat_ = (split(' ', $strat_line))[1];
		    print CSTF "$strat_\n";
		}
	    }
	    close CSTF;

	    if( "$num_strats_in_global_results_" eq ""){ 
		print STDERR "No previous strats in the pool";
		$num_strats_in_global_results_ = 0; 
	    }
	    
	    print $main_log_file_handle_ "Num of strats in global result: $num_strats_in_global_results_\n";
	    
	    my $cut_off_rank_ = min(int(0.5 * $num_strats_in_global_results_ + 5), 150);
	    my $t_ = max($num_strats_in_global_results_, 10);
	    #$exec_cmd="$LIVE_BIN_DIR/summarize_local_results_dir_and_choose_by_algo $historical_sort_algo_ $t_ $t_ $min_pnl_per_contract_to_allow_ $min_volume_to_allow_ $max_ttc_to_allow_ $local_results_base_dir | grep \"STRATEGYFILEBASE\\|STATISTICS\"";
	    $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results local_results_base_dir $cstempfile_ $work_dir_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ INVALIDFILE $historical_sort_algo_";
	    print $main_log_file_handle_ $exec_cmd."\n";
	    my @global_res_out_ = `$exec_cmd`; 
	    print $main_log_file_handle_ "@global_res_out_\n";chomp(@global_res_out_);

	    if($num_strats_in_global_results_ <=0 || $cut_off_rank_ <=0) {
		$mail_body_ .= "No previous strat in the pool to compare\n";
	    }
	    else{
		$mail_body_ .= "=============================  STRAT-POOL STATISTICS (Size: $num_strats_in_global_results_) =======================\n";
		$mail_body_ .= "     pnl  pnl_stdev  volume  pnl_shrp  pnl_cons  pnl_median  ttc  pnl_zs  avg_min_max_pnl  PPT  S B A  MAXDD\n";
		for (my $i=0; $i<$num_strats_in_global_results_ * 0.75 + 5; $i += int($cut_off_rank_/2.0)){
		    my @t_line_ = split(/ /, $global_res_out_[$i]);
		    $mail_body_ .= "[$i]: @t_line_[2 .. $#t_line_]\n";
		}
		$mail_body_ .= "==========================\n\n";
	    }
	    for (@slrac_output_lines_){
		if ( $_ =~ /STRATEGYFILEBASE/ ){
		    my $strat_line = $_;
		    my $strat_ = (split(' ', $strat_line))[1];
		    my @index_list = grep {$global_res_out_[$_] =~ $strat_} 0 .. $#global_res_out_;
		    if ($#index_list >= 0 && $index_list[0] <= $cut_off_rank_){
			push(@strat_files_selected_, $strat_);
		    }
		    if($#index_list >= 0 ){
			$strat_indices_{$strat_} = int($index_list[0]);
		    }
		}
	    }

	    print $main_log_file_handle_ "\n====================================\nAccepted Strats via global_result_analysis:\n@strat_files_selected_\n";
	    while (my ($key, $value) = each(%strat_indices_) ){
		print $main_log_file_handle_ "$key: $value\n";
	    }
	    print $main_log_file_handle_ "=======================================\n";
	}
	
	else {
	    # Commented to check wheather the global result check is working or not...
	    print $main_log_file_handle_ "Using PPC cutoff\n";
	    my @strat_files_selected_ = ();
	    my $last_strat_file_selected_ = "";
	    for ( my $t_slrac_output_lines_index_ = 0 ; $t_slrac_output_lines_index_ <= $#slrac_output_lines_ ; $t_slrac_output_lines_index_++ )
	    {
		my $this_line_ = $slrac_output_lines_[$t_slrac_output_lines_index_];
		if ( $this_line_ =~ /STRATEGYFILEBASE/ )
		{ # STRATEGYFILEBASE basename of strategyfile
		    my @strat_line_words_ = split ( ' ', $this_line_ );
		    if ( $#strat_line_words_ >= 1 )
		    { # STRATEGYFILEBASE w_strategy_ilist_ZB_EU_PBT_30_na_e3_20110629_20110729_CET_805_EST_800_fsg.5_FSRR.0.5.0.01.0.0.0.85.tt_CET_805_EST_800.pfi_0
			$last_strat_file_selected_ = $strat_line_words_[1];
		    }
		}
		if ( $this_line_ =~ /STATISTICS/ )
		{ # STATISTICS ....
		    if ( $last_strat_file_selected_ )
		    { # not ""
			my @stat_line_words_ = split ( ' ', $this_line_ );
			if ( $#stat_line_words_ >= $pnl_per_contract_word_index_ )
			{
			    my $t_insample_pnl_per_contract_ = $stat_line_words_[$pnl_per_contract_word_index_];
			    if ( $t_insample_pnl_per_contract_ > $min_pnl_per_contract_to_allow_ )
			    {
				push ( @strat_files_selected_, $last_strat_file_selected_ );
				$last_strat_file_selected_ = "";
			    }
			}
		    }
		}
	    }
	}

	if ( $#strat_files_selected_ >= 0 )
	{
	    #RunOutsampleSims ( @strat_files_selected_ );
	    
	    for ( my $i = 0 ; $i <= $#strat_files_selected_; $i ++ )
	    {
		$mail_body_ = $mail_body_."For strat: ".$strat_files_selected_[$i]."\n";
		print $main_log_file_handle_ "For strat: ".$strat_files_selected_[$i]."\n";


                # in sample results
		$exec_cmd = "$LIVE_BIN_DIR/summarize_single_strategy_results local_results_base_dir $strat_files_selected_[$i] $work_dir_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_";
		print $main_log_file_handle_ "$exec_cmd\n";
		my @t_insample_text_ = `$exec_cmd`;
		if ( $RUN_SINGLE_INDICATOR_LIST_FILE )
		{
		    $mail_body_ .= "Ilist File: ".$indicator_list_file_vec_[0]."\n";
		}
		my $strat_fl_ = FindItemFromVecWithBase ( $strat_files_selected_[$i], @strategy_filevec_  );
		#$exec_cmd = "$SCRIPTS_DIR/check_indicator_performance.py $strat_fl_ 20121001 20121103 2>/dev/null | cut -d':' -f 1,3";
		#print $main_log_file_handle_ "Checking Indicator Performance of installed strats:\n$exec_cmd\n";
		#my @ind_perf_ = `$exec_cmd`;
		#$mail_body_ .= "Ilist File: " . getIlistFileNameFromStrategy($strat_files_selected_[$i]) . "\n";
		#$mail_body_ .= "Indicator_performance: \n @ind_perf_\n";
		$mail_body_ = $mail_body_.join ( "", @t_insample_text_ ) . "STATISTICS PNL_avg PNL_stdev VOL_avg PNL_sharpe PNL_<avg-0.33*stdev> avg_min_adj_PNL median_TTC PPC NBL_order BL_order AGG_order avg_MAX_DD\n";
		
		if($use_median_cutoff_){
		    $mail_body_ .= "\nRank in the existing Pool(of Size: $num_strats_in_global_results_): $strat_indices_{$strat_files_selected_[$i]}\n";
		}
		print $main_log_file_handle_ @t_insample_text_."\n";
		
		my @t_outsample_text_ = ();
		
		my $outsample_sim_day_count_ = GetBusinessDaysBetween ( $outsample_trading_start_yyyymmdd_, $outsample_trading_end_yyyymmdd_ );
		print $main_log_file_handle_ "Outsample Days between $outsample_trading_start_yyyymmdd_ $outsample_trading_end_yyyymmdd_ = $outsample_sim_day_count_\n";
		if ( $outsample_sim_day_count_ > 5 )
		{
		    $mail_body_ = $mail_body_."Outsample\n";
		    
		    # outsample results
		    $exec_cmd="$LIVE_BIN_DIR/summarize_single_strategy_results local_results_base_dir $strat_files_selected_[$i] $work_dir_ $outsample_trading_start_yyyymmdd_ $outsample_trading_end_yyyymmdd_";
		    print $main_log_file_handle_ "$exec_cmd\n";
		    @t_outsample_text_ = `$exec_cmd`;
		    $mail_body_ = $mail_body_.join ( "", @t_outsample_text_ ) ;
		    print $main_log_file_handle_ @t_outsample_text_;
		    print $main_log_file_handle_ "\n";
		}
		
		if ( $#t_outsample_text_ >= 0 )
		{
		    chomp ( @t_outsample_text_ );
		    
		    my $t_outsample_pnl_per_contract_ = 0;
		    for ( my $t_o_idx_ = 0 ; $t_o_idx_ <= $#t_outsample_text_ ; $t_o_idx_ ++ )
		    {
			if ( $t_outsample_text_[$t_o_idx_] =~ /STATISTICS/ )
			{
			    my @stat_line_words_ = split ( ' ', $t_outsample_text_[$t_o_idx_] );
			    if ( $#stat_line_words_ >= $pnl_per_contract_word_index_ )
			    {
				$t_outsample_pnl_per_contract_ = $stat_line_words_[$pnl_per_contract_word_index_];
				last;
			    }
			}
		    }
		    if ( $t_outsample_pnl_per_contract_ > $min_pnl_per_contract_to_allow_ )
		    {
			# Install if positive in outsample
			my $t_temp_strategy_filename_ = FindItemFromVecWithBase ( $strat_files_selected_[$i], @strategy_filevec_  ) ;
			if ( $USER eq "sghosh" || $USER eq "ravi" )
			{
			    printf $main_log_file_handle_ "InstallStrategyModelling ( $t_temp_strategy_filename_, $shortcode_, $datagen_start_end_str_, $trading_start_end_str_ )\n";
			}
			InstallStrategyModelling ( $t_temp_strategy_filename_, $shortcode_, $datagen_start_end_str_, $trading_start_end_str_ );
		    }
		    else
		    {
			printf $main_log_file_handle_ "Failed to install %s since outsample pnl-avg = %d\n", $strat_files_selected_[$i], $t_outsample_pnl_per_contract_;
			$mail_body_ = $mail_body_."\nERROR: Failed to install ".$strat_files_selected_[$i]." since outsample pnl-avg = ".$t_outsample_pnl_per_contract_."\n";
		    }
		}
		else
		{
		    my $t_temp_strategy_filename_ = FindItemFromVecWithBase ( $strat_files_selected_[$i], @strategy_filevec_  ) ;
		    InstallStrategyModelling ( $t_temp_strategy_filename_, $shortcode_, $datagen_start_end_str_, $trading_start_end_str_ );		
		    
		    print $main_log_file_handle_ "Installing %s without outsample pnlcheck\n", $strat_files_selected_[$i];
		    $mail_body_ = $mail_body_."Installing ".$strat_files_selected_[$i]." without outsample pnlcheck\n";
		}
	    }
	}
	else {
	    if ($#slrac_output_lines_>=0){
		if($use_median_cutoff_){
		    $mail_body_ .= "*ERROR*: None of the strategies are better than the median of the existing strategies.\n\n";
		}
		else{
		    $mail_body_ .= "*ERROR*: None of the strategies could cross the minPNL cutoff.\n\n";
		}
	    }
	    else{
		$mail_body_ .= "\n*ERROR:* None of the strategies could pass the Summarize Strategy cutoffs.\n\n";
	    }
	    if ( $RUN_SINGLE_INDICATOR_LIST_FILE )
	    {
		$mail_body_ .= "Ilist File: ".$indicator_list_file_vec_[0]."\n";
	    }
	    my ($strat_fl_) = grep("STRATEGYFILEBASE", @best_strat_statistics ); my @st_fl_ = split(' ', $strat_fl_); 
	    $strat_fl_ = FindItemFromVecWithBase ( $st_fl_[1], @strategy_filevec_  );
	    $exec_cmd = "$SCRIPTS_DIR/check_indicator_performance.py $strat_fl_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ 2>/dev/null | sort -nr -t':' -k3 | cut -d':' -f 1,3";
	    print $main_log_file_handle_ "Checking Indicator Performance of best strats:\n$exec_cmd\n";
	    my @ind_perf_ = `$exec_cmd`;	    
	    $mail_body_ .= "Indicator_performance: \n @ind_perf_\n";

	    $mail_body_ .= "=========Best Strategy(s):=========\n".join("",@best_strat_statistics)."================END=========================\n";
	    $mail_body_ = $mail_body_."STATISTICS PNL_avg PNL_stdev VOL_avg PNL_sharpe PNL_<avg-0.33*stdev> avg_min_adj_PNL median_TTC PPC NBL_order BL_order AGG_order avg_MAX_DD\n";
	}
	if ( ( $mail_address_ ) &&
	     ( $mail_body_ ) )
	{
	    open(MAIL, "|/usr/sbin/sendmail -t");
	    
	    my $hostname_=`hostname`;
	    ## Mail Header
	    print MAIL "To: $mail_address_\n";
	    print MAIL "From: $mail_address_\n";
	    print MAIL "Subject: genstrat ( $instructionfilename_ ) $yyyymmdd_ $hhmmss_ $hostname_\n\n";
	    ## Mail Body
	    print MAIL $mail_body_ ;
	    
	    close(MAIL);
	    
	    print $main_log_file_handle_ "Mail Sent to $mail_address_\n$mail_body_\n";
	}
    }
}

sub ComputeStatsSingleRegFile
{ # compute correlations of main file
    print $main_log_file_handle_ "ComputeStatsSingleRegFile\n";
    
    my $main_file_extension_ = shift;
    
    my $this_reg_data_filename_ = $reg_data_dir."/reg_data_".$main_file_extension_;
    
    my $this_corr_filename_ = $reg_data_dir."/corr_".$main_file_extension_;
    my $this_corrmatrix_filename_ = $reg_data_dir."/corrmatrix_".$main_file_extension_;
    my $this_var_corr_filename_ = $reg_data_dir."/var_corr_".$main_file_extension_;
    my $this_high_corr_ind_filename_ = $reg_data_dir."/hcp_".$main_file_extension_;
    
    if ( $SAVE_CORR_FILE == 0 )
    { push ( @intermediate_files_, $this_corr_filename_ ); }
    if ( $SAVE_CORRMATRIX_FILE == 0 )
    { push ( @intermediate_files_, $this_corrmatrix_filename_ ); }
    if ( $SAVE_VAR_CORR_FILE == 0 )
    { push ( @intermediate_files_, $this_var_corr_filename_ ); }
    if ( $SAVE_HCP_FILE == 0 )
    { push ( @intermediate_files_, $this_high_corr_ind_filename_ ); }
    
    if ( $SAVE_VAR_CORR_FILE != 0 )
    { # the following code is only required if we are saving var_corr file
	if ( ! ( -e $this_corr_filename_ ) )
	{
	    my $exec_cmd="$LIVE_BIN_DIR/get_dep_corr $this_reg_data_filename_ > $this_corr_filename_";
	    print $main_log_file_handle_ "$exec_cmd\n";
	    `$exec_cmd`;
	}
	if ( ! ( -e $this_var_corr_filename_ ) )
	{
	    my $exec_cmd="$MODELSCRIPTS_DIR/sort_by_corr.pl $indicator_list_filename_ $this_corr_filename_ > $this_var_corr_filename_";
	    print $main_log_file_handle_ "$exec_cmd\n";
	    `$exec_cmd`;
	}
    }
    
    if ( $SAVE_HCP_FILE != 0 )
    { # the following code is only required if HCP file is being saved
	if ( ! ( -e $this_corrmatrix_filename_ ) )
	{
	    my $exec_cmd="$LIVE_BIN_DIR/get_correlation_matrix $this_reg_data_filename_ > $this_corrmatrix_filename_";
	    print $main_log_file_handle_ "$exec_cmd\n";
	    `$exec_cmd`;
	}
	if ( ! ( -e $this_high_corr_ind_filename_ ) )
	{
	    my $exec_cmd="$MODELSCRIPTS_DIR/print_highly_correlated_indicator_pairs.pl $indicator_list_filename_ $this_corrmatrix_filename_ > $this_high_corr_ind_filename_";
	    print $main_log_file_handle_ "$exec_cmd\n";
	    `$exec_cmd`;
	}
    }
}

sub RunOutsampleSims
{
    print $main_log_file_handle_ "RunOutsampleSims\n";
    
    my @strat_files_selected_ = @_;
    
    my $tradingdate_ = $outsample_trading_end_yyyymmdd_;
    my $max_days_at_a_time_ = 2000;
    for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
    {
        if ( ( SkipWeirdDate ( $tradingdate_ ) ) ||
             ( NoDataDateForShortcode ( $tradingdate_ , $shortcode_ ) ) ||
             ( IsDateHoliday ( $tradingdate_ ) || ( ( $shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $shortcode_ ) ) ) ) )
#            IsDateHoliday ( $tradingdate_ ) )
	{
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next;
	}
	
	if ( ( ! ValidDate ( $tradingdate_ ) ) ||
	     ( $tradingdate_ < $outsample_trading_start_yyyymmdd_ ) )
	{
	    last;
	}
	else 
	{
	    # for this $tradingdate_ break the @strategy_filevec_ into blocks of size MAX_STRAT_FILES_IN_ONE_SIM
	    # for a block print the filenames in a $temp_strategy_list_file_
	    # run sim_strategy, store output in @sim_strategy_output_lines_
	    # print lines in @sim_strategy_output_lines_ that have the word "SIMRESULT" into $temp_results_list_file_.
            # run add_results_to_local_database.pl ... push the file written to @non_unique_results_filevec_
	    my $strategy_filevec_front_ = 0;
	    my $temp_strategy_list_file_index_ = 0;
	    my $strategy_filevec_back_ = $#strat_files_selected_;
	    
	    my $temp_strategy_list_file_ = $work_dir_."/temp_selected_strategy_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
	    my $temp_strategy_cat_file_ = $work_dir_."/temp_selected_strategy_cat_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
	    open TSLF, "> $temp_strategy_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_ for writing\n" );
#		open TSCF, "> $temp_strategy_cat_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_cat_file_ for writing\n" );
	    if ( -e $temp_strategy_cat_file_ ) { `rm -f $temp_strategy_cat_file_`; }
	    for ( my $t_strategy_filevec_index_ = $strategy_filevec_front_; $t_strategy_filevec_index_ <= $strategy_filevec_back_; $t_strategy_filevec_index_ ++ )
	    {
		my $t_temp_strategy_filename_ = FindItemFromVecWithBase ( $strat_files_selected_[$t_strategy_filevec_index_], @strategy_filevec_  ) ;
		
		print TSLF $t_temp_strategy_filename_."\n";
		`cat $t_temp_strategy_filename_ >> $temp_strategy_cat_file_`;
	    }
#		close TSCF;
	    close TSLF;

	    my @sim_strategy_output_lines_=(); # stored to seive out the SIMRESULT lines
	    my %unique_id_to_pnlstats_map_=(); # stored to write extended results to the database
	    {
		
		my $market_model_index_ = 0 ;
		if ( $USER =~ /gchak/ )
		{
		    $market_model_index_ = 3;
		}
		my $exec_cmd="$LIVE_BIN_DIR/sim_strategy SIM $temp_strategy_cat_file_ $unique_gsm_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1"; # using hyper optimistic market_model_index, added nologs argument
		print $main_log_file_handle_ "$exec_cmd\n";
		@sim_strategy_output_lines_=`$exec_cmd`;
		
		my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);
		if ( ExistsWithSize ( $this_tradesfilename_ ) )
		{
		    $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_";
		    print $main_log_file_handle_ "$exec_cmd\n";
		    my @pnlstats_output_lines_ = `$exec_cmd`;
		    for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
		    {
			my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
			if( $#rwords_ >= 1 )
			{
			    my $unique_sim_id_ = $rwords_[0];
			    splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_sim_id_
			    $unique_id_to_pnlstats_map_{$unique_sim_id_} = join ( ' ', @rwords_ );
			}
		    }
		}
		
		# added deletion of tradesfiles
		if ( $SAVE_TRADELOG_FILE == 0 ) 
		{ `rm -f $this_tradesfilename_`; }
	    }
	    
	    
	    my $temp_results_list_file_ = $work_dir_."/temp_selected_results_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
	    open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );
	    
	    for ( my $t_sim_strategy_output_lines_index_ = 0, my $psindex_ = 0; $t_sim_strategy_output_lines_index_ <= $#sim_strategy_output_lines_; $t_sim_strategy_output_lines_index_ ++ )
	    {
		if ( $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] =~ /SIMRESULT/ )
		{ # SIMRESULT pnl volume sup% bestlevel% agg%
		    my @rwords_ = split ( ' ', $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] );
		    splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc
		    my $remaining_simresult_line_ = join ( ' ', @rwords_ );
		    if ( ( $rwords_[1] > 0 ) || # volume > 0
			 ( ( $shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day
		    {
			my $unique_sim_id_ = GetUniqueSimIdFromCatFile ( $temp_strategy_cat_file_, $psindex_ );
			if ( ! exists $unique_id_to_pnlstats_map_{$unique_sim_id_} )
			{
			    $unique_id_to_pnlstats_map_{$unique_sim_id_} = "0 0 0 0 0 0 0 0 0 0 0 0 0";
#     			PrintStacktraceAndDie ( "unique_id_to_pnlstats_map_ missing $unique_sim_id_ for listfile: $temp_results_list_file_ catfile: $temp_strategy_cat_file_ rline: $remaining_simresult_line_\n" );
			}
			printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_ ;
			printf TRLF "%s %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_;
		    }
		    $psindex_ ++;
		}
	    }
	    close TRLF;
	    
	    if ( ExistsWithSize ( $temp_results_list_file_ ) )
	    {
		my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database.pl $temp_strategy_list_file_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir"; # TODO init $local_results_base_dir
		print $main_log_file_handle_ "$exec_cmd\n";
		my $this_local_results_database_file_ = `$exec_cmd`;
	    }
	    
	    if ( $delete_intermediate_files_ )
	    {
		if ( -e $temp_strategy_cat_file_ ) { `rm -f $temp_strategy_cat_file_`; }
		if ( -e $temp_strategy_list_file_ ) { `rm -f $temp_strategy_list_file_`; }
		if ( -e $temp_results_list_file_ ) { `rm -f $temp_results_list_file_`; }
	    }
	}
	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    }
}
