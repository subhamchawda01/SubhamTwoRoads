#!/usr/bin/perl

# \file ModelScripts/generate_timed_indicator_data_V2.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
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

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use Scalar::Util qw(looks_like_number);

sub LoadInstructionFile ; # used to read the instructions
sub SanityCheckInstructionFile ;
sub RunRegressMakeModelFiles ;
sub AddModelFileToList ; # privately called in RunRegressMakeModelFiles
sub MakeStrategyFiles ; # takes model and paramfile and makes strategyfile
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database
sub GetTimePeriodFromTime ; #
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

if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "mayank" || $USER eq "rahul" || $USER eq "ankit" || $USER eq "kputta" )
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
require "$GENPERLLIB_DIR/get_high_stdev_days_for_shortcode.pl"; # GetHighStdevDaysForShortcode
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
require "$GENPERLLIB_DIR/find_item_from_vec_ref.pl"; #FindItemFromVecRef
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
require "$GENPERLLIB_DIR/check_ilist_data.pl"; # CheckIndicatorData
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/get_avg_event_count_per_sec_for_shortcode.pl"; # GetAvgEventCountPerSecForShortcode
require "$GENPERLLIB_DIR/get_avg_trade_count_per_sec_for_shortcode.pl"; # GetAvgTradeCountPerSecForShortcode

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

my $RUN_SINGLE_INDICATOR_LIST_FILE = 1; # choose a file if 1, else, iterate over all files
# given it is one file
# option 0: pick randomly from all
# option 1: pick in round robin fashion using LAST_PICKED_INDICATOR
# TODO
# option 2: more intelligent pick, the 'expected' best performing indicator file
my $INDICATOR_PICK_STRATEGY = -1;
my $LAST_PICKED_INDICATOR_INDEX = -1;

my $CREATE_SUBMODELS = 1; #set to 0 for EARTH

# start 
my $USAGE="$0 indicatorlistfilename datagen_start_yyyymmdd datagen_end_yyyymmdd starthhmm endhhmm msecs_timeout_ l1events_timeout_ num_trades_timeout_ horizon_ algo_ outfile_";

if ( $#ARGV < 10 ) { print $USAGE."\n"; exit ( 0 ); }
my $indicator_list_filename_ = $ARGV[0];
my $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[1] );
my $datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $datagen_start_hhmm_ = $ARGV[3];
my $datagen_end_hhmm_ = $ARGV[4];
my $datagen_msecs_timeout_ = $ARGV[5];
my $datagen_l1events_timeout_ = $ARGV[6];
my $datagen_num_trades_timeout_ = $ARGV[7];
my $to_print_on_economic_times_ = 1;
my $td2rd_horizon_ = max ( 1, $ARGV[8] );
my $td2rd_algo_ = $ARGV[9] ;
my $this_reg_data_filename_ = $ARGV[10];

{
    my $instbase_ = basename ( $indicator_list_filename_ ); 
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
my $datagen_start_yyyymmdd_ = "";
my $datagen_end_yyyymmdd_ = "";
my $datagen_day_inclusion_prob_ = 1.00;
my $datagen_day_inclusion_count_ = 0;
my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 15;
my $datagen_num_trades_timeout_ = 0;
my @datagen_day_vec_ = (); # in case we want to give a set of days to regress on 
my @datagen_exclude_days_ = ( ); # Do not run datagen on a selected set of days.
my $delete_intermediate_files_ = 0;
my $mail_address_ = "";
my $author_ = "";
my $datagen_hhmm_index_ = 0;
my $trading_hhmm_index_ = 0;

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $use_old_genstrat_ = 1;
    $use_median_cutoff_ = 0;
}

my $min_external_datagen_day_vec_size_ = 3;

my @dep_based_filter_ = ();
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

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $avoid_high_sharpe_indep_check_index_filename_ = $work_dir_."/avoid_high_sharpe_check_index_file.txt";
my $main_log_file_handle_ = FileHandle->new;

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }

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

$shortcode_ = GetShortcodeFromIList ( );

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

    {
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
	    if ( ( $datagen_date_ < 20101231 ) || ( $datagen_date_ > 20200220 ) )
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
		elsif ( ( ( $this_datagen_date_ > 20121231 ) && ( $this_datagen_date_ < 20200220 ) ) &&
			! ( SkipWeirdDate ( $this_datagen_date_ ) ||
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

}


sub RunRegressMakeModelFiles 
{
    print $main_log_file_handle_ "RunRegressMakeModelFiles\n";

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
			        if ( CheckIndicatorData($tradingdate_, $indicator_list_filename_) == 1)
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

                               
			    $apply_trade_vol_filter = 0;
			    for ( my $findex_ = 0; $findex_ <= $#dep_based_filter_; $findex_ ++ )
			    { # for every filtration of the main file
				my $this_filter_ = $dep_based_filter_[$findex_];
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
				    if ( $apply_trade_vol_filter == 1 )
				    {
        				#do the trade volume file generation part
	               			my $exec_cmd="$LIVE_BIN_DIR/daily_trade_aggregator $shortcode_ $tradingdate_ $trade_per_sec_file";
					print $main_log_file_handle_ "$exec_cmd\n";
			             	my @daily_trade_agg_output_lines_ = `$exec_cmd`;
					print $main_log_file_handle_ @daily_trade_agg_output_lines_."\n";
        			    }
				    my $exec_cmd="$LIVE_BIN_DIR/timed_data_to_reg_data $indicator_list_filename_ $this_day_timed_data_filename_ $this_pred_counters_ $this_predalgo_ $this_day_wmean_reg_data_filename_";
				    if ( $apply_trade_vol_filter == 1 )
				    {
				    	$exec_cmd = $exec_cmd." $trade_per_sec_file";
				    }
				    print $main_log_file_handle_ "$exec_cmd\n";
				    `$exec_cmd`;
				    if ( $apply_trade_vol_filter == 1 )
				    {
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

		# DO Something
		
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


sub GetTimePeriodFromTime 
{
    my $dir_name_ = "$trading_start_hhmm_-$trading_end_hhmm_";
    my $retval_ = $dir_name_;
    if    (IsStratDirInTimePeriod($dir_name_, "EU_MORN_DAY") ) { $retval_ = "EU_MORN_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_MORN_DAY") ) { $retval_ = "US_MORN_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "AS_MORN")     ) { $retval_ = "AS_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "AS_DAY")      ) { $retval_ = "AS_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_EARLY_MORN")){ $retval_ = "US_EARLY_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_MORN")     ) { $retval_ = "US_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_DAY")      ) { $retval_ = "US_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_MORN")     ) { $retval_ = "US_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_DAY")      ) { $retval_ = "US_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "EU_MORN_DAY_US_DAY")) { $retval_ = "EU_MORN_DAY_US_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "EU_US_MORN_DAY")) { $retval_ = "EU_US_MORN_DAY";}
    $retval_;
}

