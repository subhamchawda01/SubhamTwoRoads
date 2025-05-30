#!/usr/bin/perl

# \file ModelScripts/generate_models.pl
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
# predduration
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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $GENTIMEDDATA_DAILY_DIR="/spare/local/DailyTimedDataDir/"; # this directory is used to store data so that the calling script can also expect it there
my $GENREGDATA_DAILY_DIR="/spare/local/DailyRegDataDir/"; # this directory is used to store data so that the calling script can also expect it there
my $GENREGDATA_DIR="/spare/local/RegDataDir/"; # this directory is used to store data so that the calling script can also expect it there

my $GENMODELWORKDIR=$SPARE_HOME."GMW/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/holiday.pl"; # Holiday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # for GetPredCountersForThisPredAlgo
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_avg_event_count_per_sec_for_shortcode.pl"; # GetAvgEventCountPerSecForShortcode
require "$GENPERLLIB_DIR/get_avg_trade_count_per_sec_for_shortcode.pl"; # GetAvgTradeCountPerSecForShortcode

my $reg_data_daily_dir = $GENREGDATA_DAILY_DIR;
my $timed_data_daily_dir = $GENTIMEDDATA_DAILY_DIR; # in case the calling script wants data that is not in the global timed data directory just yet
my $reg_data_dir = $GENREGDATA_DIR;

my $SAVE_CORR_FILE = 0;
my $SAVE_VAR_CORR_FILE = 0;
my $SAVE_CORRMATRIX_FILE = 0;
my $SAVE_HCP_FILE = 0;
my $SAVE_STATS_FILE = 0;
my $work_dir_ = "";
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
#my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );

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
    $work_dir_ = $GENMODELWORKDIR.$instbase_."/".$unique_gsm_id_; 

    for ( my $i = 0 ; $i < 30 ; $i ++ )
    {
	if ( -d $work_dir_ )
	{
#	print STDERR "Surprising but this dir exists\n";
	    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	    $work_dir_ = $GENMODELWORKDIR.$instbase_."/".$unique_gsm_id_;
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
my $name_ = "strategy";
my $indicator_list_filename_ = "";
my $datagen_start_yyyymmdd_ = "";
my $datagen_end_yyyymmdd_ = "";
my $datagen_start_hhmm_ = "";
my $datagen_end_hhmm_ = "";
my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 15;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0;
my $strategyname_ = "";
my $trading_start_yyyymmdd_ = "";
my $trading_end_yyyymmdd_ = "";
my $trading_start_hhmm_ = "";
my $trading_end_hhmm_ = "";
my $num_files_to_choose_ = "";
my $delete_intermediate_files_ = 0;

my @dep_based_filters_ = ();
my @predduration_ = ();
my @predalgo_ = ();
my @regress_exec_ = ();
my %duration_to_param_filevec_ ;
my %duration_to_model_filevec_ ;
my @strategy_filevec_ = ();
my @intermediate_files_ = ();

# temporary
my $indicator_list_filename_base_ = "";

my $local_ilist_dir_ = $work_dir_."/ilist_dir";
my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_ilist_dir_ ) ) { `mkdir -p $local_ilist_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

# load instruction file
LoadInstructionFile ( );

# Check if the arguments are complete
SanityCheckInstructionFile ( );

# generate reg data ( and timed data, and remove mean if needed ) and then find weights
RunRegressMakeModelFiles ( );

# end script
$main_log_file_handle_->close;

exit ( 0 );


sub LoadInstructionFile 
{

    open INSTRUCTIONFILEHANDLE, "< $instructionfilename_ " or PrintStacktraceAndDie ( "Could not open $instructionfilename_\n" );

    my $current_instruction_="";
    my $current_instruction_set_ = 0;
    my @timeout_string_choices_ = ();
    while ( my $thisline_ = <INSTRUCTIONFILEHANDLE> ) 
    {
	chomp ( $thisline_ ); # remove newline

	my @instruction_line_words_ = split ( ' ', $thisline_ );
	#	print $main_log_file_handle_ "Handling $thisline_ $#instruction_line_words_\n";
	if ( $#instruction_line_words_ < 0 ) 
	{ # empty line hence set $current_instruction_set_ 0
	    $current_instruction_ = "";
	    $current_instruction_set_ = 0;
	    next;
	} 
	else 
	{ # $#instruction_line_words_ >= 0

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
			$indicator_list_filename_ = $instruction_line_words_[0];
			my $indicator_list_filename_new_dir_ = FileNameInNewDir ( $indicator_list_filename_, $local_ilist_dir_ );
			my $exec_cmd = "cp $indicator_list_filename_ $indicator_list_filename_new_dir_";
			print $main_log_file_handle_ "$exec_cmd\n";
			`$exec_cmd`;
			$indicator_list_filename_ = $indicator_list_filename_new_dir_;
			$indicator_list_filename_base_ = basename ( $indicator_list_filename_ ); chomp ($indicator_list_filename_base_);
			# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
		    }
		    when ("DATAGEN_START_YYYYMMDD") 
		    {
			$datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
			# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
		    }
		    when ("DATAGEN_END_YYYYMMDD") 
		    {
			$datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
			# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
		    }
		    when ("DATAGEN_START_HHMM") 
		    {
			$datagen_start_hhmm_ = $instruction_line_words_[0];
			# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
		    }
		    when ("DATAGEN_END_HHMM") 
		    {
			$datagen_end_hhmm_ = $instruction_line_words_[0];
			# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
		    }
		    when ("DATAGEN_TIMEOUT")
		    {
			if ( $#instruction_line_words_ >= 1 )
			{ 
			    push ( @timeout_string_choices_, $thisline_ );
			}
			# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
		    }
		    when ("PREDDURATION") 
		    {
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
			push ( @dep_based_filters_, $instruction_line_words_[0] );
			# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
		    }
		    when ("REGRESS_EXEC") 
		    {
			push ( @regress_exec_, [ @instruction_line_words_ ] );
			# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
		    }
		    when ("DELETE_INTERMEDIATE_FILES") 
		    {
			$delete_intermediate_files_ = 1;
			# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
		    }
		    default
		    { # ignore others, this allows the same instruction file of a strategy file to be used
		    }
		}
	    }
	}
    }

    if ( $#timeout_string_choices_ >= 0 )
    {
	my $t_random_timeout_string_ = $timeout_string_choices_ [ 0 ];
	if ( $#timeout_string_choices_ >= 1 )
	{ # at least 2 words
	    my $t_random_timeout_index_ = int ( rand ( $#timeout_string_choices_ + 1 ) );
	    $t_random_timeout_string_ = $timeout_string_choices_ [ $t_random_timeout_index_ ];
	}
	    
	my $timeout_set_already_ = 0; # signal need to set ahead
	my @timeout_string_words_ = split ( ' ', $t_random_timeout_string_ );
	if ( $timeout_string_words_[0] eq "TIM1" )
	{
	    ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 100, 0, 0 );
	    $timeout_set_already_ = 1;
	}
	if ( $timeout_string_words_[0] eq "EVT1" )
	{
	    ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 4000, 10, 0 );
	    if ( $shortcode_ )
	    {
		$datagen_l1events_timeout_ = GetAvgEventCountPerSecForShortcode ( $shortcode_, $datagen_start_hhmm_, $datagen_end_hhmm_, $datagen_end_yyyymmdd_ ) / 2.0;
		if ( $datagen_l1events_timeout_ < 1 )
		{ # very slow products
		    $datagen_msecs_timeout_ = max ( $datagen_msecs_timeout_, (1000/$datagen_l1events_timeout_) );
		    $datagen_msecs_timeout_ = min ( 60000, $datagen_msecs_timeout_ );
		}
	    }
	    $timeout_set_already_ = 1;
	}
	if ( $timeout_string_words_[0] eq "TRD1" )
	{
	    ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 8000, 20, 1 );
	    if ( $shortcode_ )
	    {
		$datagen_l1events_timeout_ = GetAvgEventCountPerSecForShortcode ( $shortcode_, $datagen_start_hhmm_, $datagen_end_hhmm_, $datagen_end_yyyymmdd_ ) * 2.0 ;
		$datagen_num_trades_timeout_ = max ( 1, int ( GetAvgTradeCountPerSecForShortcode ( $shortcode_ ) ) ) ;
		
		if ( $datagen_l1events_timeout_ < 1 )
		{ # very slow products
		    $datagen_msecs_timeout_ = max ( $datagen_msecs_timeout_, (1000/$datagen_l1events_timeout_) );
		    $datagen_msecs_timeout_ = min ( 60000, $datagen_msecs_timeout_ );
		}
	    }
	    $timeout_set_already_ = 1;
	}
	if ( $#timeout_string_words_ >= 2 )
	{
	    if ( $timeout_set_already_ == 0 )
	    {
		$datagen_msecs_timeout_ = $timeout_string_words_[0];
		$datagen_l1events_timeout_ = $timeout_string_words_[1];
		$datagen_num_trades_timeout_ = $timeout_string_words_[2];
	    }
	}
    }

    close ( INSTRUCTIONFILEHANDLE );
}

sub SanityCheckInstructionFile 
{
    if ( ! ( $shortcode_ ) )
    {
	print $main_log_file_handle_ "SHORTCODE missing\n";
	exit ( 0 );
    }

    if ( ! ( $indicator_list_filename_base_ ) )
    {
	print $main_log_file_handle_ "INDICATORLISTFILENAME missing\n";
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

    for ( my $findex_ = 0; $findex_ <= $#dep_based_filters_; $findex_ ++ )
    {
	my $this_filter_ = $dep_based_filters_[$findex_];
    }
}

sub RunRegressMakeModelFiles 
{

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
		my $tradingdate_ = $datagen_start_yyyymmdd_;
		my $max_days_at_a_time_ = 2000;
		for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
		{
		    if ( SkipWeirdDate ( $tradingdate_ ) ||
			 NoDataDateForShortcode ( $tradingdate_ , $shortcode_ ) ||
			 IsDateHoliday ( $tradingdate_ ) )
		    {
			$tradingdate_ = CalcNextDate ( $tradingdate_ );
			next;
		    }

		    if ( ( ! ValidDate ( $tradingdate_ ) ) ||
			 ( $tradingdate_ > $datagen_end_yyyymmdd_ ) )
		    {
			last;
		    }
		    else 
		    {   # for this trading date generate the reg_data_file
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
			    `mkdir -p $reg_data_daily_dir`;
			}

			if ( ! ( -e $this_day_reg_data_filename_ ) )
			{   # reg_data_file missing 
			    # try to fetch the timed_data_file 
			    if ( ! ( -d $timed_data_daily_dir ) )
			    {
				`mkdir -p $timed_data_daily_dir`;
			    }

			    my $this_day_timed_data_filename_ = $timed_data_daily_dir."/timed_data_".$indicator_list_filename_base_."_".$tradingdate_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_ ; 
			    if ( ! ( -e $this_day_timed_data_filename_ ) )
			    { # generate it
				my $exec_cmd="$LIVE_BIN_DIR/datagen $indicator_list_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
				print $main_log_file_handle_ "$exec_cmd\n";
				`$exec_cmd`;

				if ( ExistsWithSize ( $this_day_timed_data_filename_ ) )
				{
				    push ( @intermediate_files_, $this_day_timed_data_filename_ ); # if this is a new file we created then add it to set of files to delete at the end
				}
				else
				{				    
				    print $main_log_file_handle_ "Could not create non-zero-sized $this_day_timed_data_filename_\n" ;
				    print $main_log_file_handle_ "rm -f $this_day_timed_data_filename_\n" ;
				    `rm -f $this_day_timed_data_filename_`;
				}
			    }
			  
			    if ( -e $this_day_timed_data_filename_ )
			    { # if now timed_data_file exists and has non zero size
				my $this_day_wmean_reg_data_filename_ = $reg_data_daily_dir."/wmean_reg_data_".$this_day_file_extension_;
				# generate reg data file which is not mean zero
				{
				    my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( $shortcode_ , $this_pred_duration_, $this_predalgo_, $this_day_timed_data_filename_ );
				    my $exec_cmd="$LIVE_BIN_DIR/timed_data_to_reg_data $indicator_list_filename_ $this_day_timed_data_filename_ $this_pred_counters_ $this_predalgo_ $this_day_wmean_reg_data_filename_";
				    print $main_log_file_handle_ "$exec_cmd\n";
				    `$exec_cmd`;
				}

				if ( -e $this_day_wmean_reg_data_filename_ )
				{
				    my $this_day_stats_reg_data_filename_ = $reg_data_daily_dir."/stats_reg_data_".$this_day_file_extension_;
				    {
					my $exec_cmd="$LIVE_BIN_DIR/remove_mean_reg_data  $this_day_wmean_reg_data_filename_ $this_day_reg_data_filename_ $this_day_stats_reg_data_filename_";
					print $main_log_file_handle_ "$exec_cmd\n";
					`$exec_cmd`;
				    }
				    print $main_log_file_handle_ "rm -f $this_day_wmean_reg_data_filename_\n" ;
				    `rm -f $this_day_wmean_reg_data_filename_`;
				    if ( $SAVE_STATS_FILE == 0 )
				    { push ( @intermediate_files_, $this_day_stats_reg_data_filename_ ); }
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
			    
#			  my $this_day_stats_reg_data_filename_ = $reg_data_daily_dir."/stats_reg_data_".$this_day_file_extension_;
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
		    $tradingdate_ = CalcNextDate ( $tradingdate_ );
		}
	    }

	    if ( ! ( -e $this_reg_data_filename_ ) )
	    {
		print $main_log_file_handle_ "ERROR Even after all this the reg_data file does not exist!\n";
		print "ERROR Even after all this the reg_data file does not exist!\n";
	    }
	    else
	    { # now $this_reg_data_filename_ exists

		{ # compute correlations of main file
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


		# for every filtration of the main file
		for ( my $findex_ = 0; $findex_ <= $#dep_based_filters_; $findex_ ++ )
		{
		    my $this_filter_ = $dep_based_filters_[$findex_];
		    my $this_filtered_main_file_extension_ = $main_file_extension_."_".$this_filter_;
		    my $this_filtered_reg_data_filename_ = $reg_data_dir."/reg_data_".$this_filtered_main_file_extension_;
		    if ( ! ExistsWithSize ( $this_filtered_reg_data_filename_ ) )
                    {
                      push ( @intermediate_files_, $this_filtered_reg_data_filename_ );
                      my $exec_cmd="$MODELSCRIPTS_DIR/apply_dep_filter.pl  $shortcode_ $this_reg_data_filename_ $this_filter_ $this_filtered_reg_data_filename_ $datagen_end_yyyymmdd_ " ;
                      print $main_log_file_handle_ "$exec_cmd\n";
                      `$exec_cmd`;
                    }

		    # for each regress_exec
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

				    my $regress_dotted_string_ = join ( '_', ( $regtype_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_ ) );
				    # print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";

				    my $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
				    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;
				    my $this_regression_output_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_;
				    
				    {
					my $exec_cmd="$LIVE_BIN_DIR/callFSLR $this_filtered_reg_data_filename_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $this_regression_output_filename_";
					print $main_log_file_handle_ "$exec_cmd\n";
					`$exec_cmd`;
				    }
				    {
					my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_model_filename_ $indicator_list_filename_ $this_regression_output_filename_ ";
					print $main_log_file_handle_ "$exec_cmd\n";
					`$exec_cmd`;
				    }

				    AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
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

				    my $regress_dotted_string_ = join ( '_', ( $regtype_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_ ) );
				    # print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";

				    my $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
				    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;
				    my $this_regression_output_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_;
				    
				    {
					my $exec_cmd="$LIVE_BIN_DIR/callFSHLR $this_filtered_reg_data_filename_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $this_regression_output_filename_";
					print $main_log_file_handle_ "$exec_cmd\n";
					`$exec_cmd`;
				    }
				    {
					my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_model_filename_ $indicator_list_filename_ $this_regression_output_filename_ ";
					print $main_log_file_handle_ "$exec_cmd\n";
					`$exec_cmd`;
				    }

				    AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
				}
				else
				{
				    print $main_log_file_handle_ "FSHLR line now needs 5 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
				}
			    }
			    when ( "FSVLR" )
			    {
				if ( $#$scalar_ref_ >= 4 ) 
				{
				    my $min_correlation_ = $$scalar_ref_[1];
				    my $first_indep_weight_ = $$scalar_ref_[2];
				    my $must_include_first_k_independants_ = $$scalar_ref_[3];
				    my $max_indep_correlation_ = $$scalar_ref_[4];

				    my $regress_dotted_string_ = join ( '_', ( $regtype_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_ ) );
				    # print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";

				    my $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
				    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;
				    my $this_regression_output_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_;
				    
				    {
					my $exec_cmd="$LIVE_BIN_DIR/callFSVLR $this_filtered_reg_data_filename_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $this_regression_output_filename_";
					print $main_log_file_handle_ "$exec_cmd\n";
					`$exec_cmd`;
				    }
				    {
					my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_model_filename_ $indicator_list_filename_ $this_regression_output_filename_ ";
					print $main_log_file_handle_ "$exec_cmd\n";
					`$exec_cmd`;
				    }

				    AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
				}
				else
				{
				    print $main_log_file_handle_ "FSVLR line now needs 5 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
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

				    my $regress_dotted_string_ = join ( '.', ( $regtype_, $regularization_coeff_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_ ) );
				    # print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";

				    my $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
				    my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;
				    my $this_regression_output_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_;
				    
				    {
					my $exec_cmd="$LIVE_BIN_DIR/callFSRR $this_filtered_reg_data_filename_ $regularization_coeff_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $this_regression_output_filename_";
					print $main_log_file_handle_ "$exec_cmd\n";
					`$exec_cmd`;
				    }
				    {
					my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_model_filename_ $indicator_list_filename_ $this_regression_output_filename_";
					print $main_log_file_handle_ "$exec_cmd\n";
					`$exec_cmd`;
				    }

				    AddModelFileToList ( $this_pred_duration_, $this_model_filename_ );
				}
				else
				{
				    print $main_log_file_handle_ "FSRR line now needs 6 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
				}
			    }
			    default
			    {
				print $main_log_file_handle_ "Not handling regtype_ $regtype_ right now \n";
				exit ( 0 );
			    }
			}
		    }
		}
	    }
	}
    }

    if ( $delete_intermediate_files_ )
    {
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

sub AddModelFileToList
{
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

