#!/usr/bin/perl

# \fileaModelScripts/generate_strategies.pl
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
use Scalar::Util qw(looks_like_number);


sub PopulateFileList ;
sub LoadInstructionFile ; # used to read the instructions
sub MakeNewStrategy ;
sub MakeAllStrategies ;
sub MakeModelFileList ;
sub MakeStrategyFiles ; # takes model and paramfile and makes strategyfile
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database
sub SummarizeLocalResultsAndChoose ; # from the files created in the local_results_base_dir choose the best ones to send to pool

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/get_spare_local_dir_for_aws.pl"; # GetSpareLocalDir
my $SPARE_LOCAL="/spare/local/";
my $hostname_ = `hostname`;
if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
{
  $SPARE_LOCAL = GetSpareLocalDir();
}
my $SPARE_HOME=$SPARE_LOCAL.$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 

my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";
my $VOL_LOG_DIR = "/spare/local/tradeinfo/volatilitylogs/";

my $GENSTRATWORKDIR=$SPARE_HOME."GSW/";

my $strategy_progid_ = 1001;

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "ankit" || $USER eq "kputta" || $USER eq "rkumar" || $USER eq "diwakar" )
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STIR_STRATS_DIR=$MODELING_BASE_DIR."/stir_strats"; 
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $MODELING_MODELS_DIR=$MODELING_BASE_DIR."/models"; # this directory is used to store the chosen model files
my $MODELING_PARAMS_DIR=$MODELING_BASE_DIR."/params"; # this directory is used to store the chosen param files

require "$GENPERLLIB_DIR/is_model_corr_consistent.pl"; # IsModelCorrConsistent
require "$GENPERLLIB_DIR/get_bad_days_for_shortcode.pl"; # GetBadDaysForShortcode
require "$GENPERLLIB_DIR/get_very_bad_days_for_shortcode.pl"; # GetVeryBadDaysForShortcode
require "$GENPERLLIB_DIR/get_high_volume_days_for_shortcode.pl"; # GetHighVolumeDaysForShortcode
require "$GENPERLLIB_DIR/get_low_volume_days_for_shortcode.pl"; # GetLowVolumeDaysForShortcode
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
require "$GENPERLLIB_DIR/get_unique_sim_id_from_stir_cat_file.pl"; # GetUniqueSimIdFromCatFile
# require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
# require "$GENPERLLIB_DIR/exists_and_same.pl"; #ExistsAndSame
# require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
# require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; #FindItemFromVec
require "$GENPERLLIB_DIR/find_item_from_vec_ref.pl"; #FindItemFromVecRef
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams cartesian_product
require "$GENPERLLIB_DIR/install_strategy_modelling.pl"; # InstallStrategyModelling
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; # MakeStratVecFromDir
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_substr.pl"; # MakeStratVecFromDirSubstr
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
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
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_and_tt.pl"; # MakeStratVecFromDirAndTT
require "$GENPERLLIB_DIR/get_param_list_and_l1_norm.pl"; # GetParamListAndL1Norm
require "$GENPERLLIB_DIR/create_mid_base_model.pl"; # CreateMidBaseModel
require "$GENPERLLIB_DIR/get_num_regimes.pl"; # GetNumRegimesFromRegimeInd

my $MAX_STRAT_FILES_IN_ONE_SIM = 50; # please work on optimizing this value

my $work_dir_ = "";
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;

# start 
my $stime_ =`date`;
my $mail_body_ = " process_start_time : ".$stime_."\n" ;
my $USAGE="$0 instructionfilename [ work_dir ]";

$mail_body_.= "SPARE_LOCAL_DIR : ".$SPARE_LOCAL."\n" ;

my @modelfilelist_ = ( );
my @paramfilelist_ = ( );
my @modellist_ = ( );
my @paramlist_ = ( );

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
my @trading_days_ = ( ); # Only run sims on a selected set of days.
my @trading_exclude_days_ = ( ); # Do not run sims on a selected set of days.
my @datagen_exclude_days_ = ( ); # Do not run datagen on a selected set of days.
my @shortcode_vec_ = ( );
my %shortcode_to_modellist_ = ( );
my %shortcode_to_paramlist_ = ( );
my $strategyname_ = "";
my $generate_models_on_recent_days_ = 0 ;
my $trading_start_yyyymmdd_ = "";
my $trading_end_yyyymmdd_ = "";
my $first_shortcode_ = "DI1F17";
my $trading_start_hhmm_ = "";
my $trading_start_hhmm_set_already_ = -1;
my $trading_end_hhmm_ = "";
my $trading_end_hhmm_set_already_ = -1;
my $num_files_to_choose_ = "";
my $min_num_files_to_choose_ = 0; # now we have some files ... don't need to select any every time
# filtering strategies
my $historical_sort_algo_ = "kCNASqDDAdjPnlSqrtVolume" ;
my $delete_intermediate_files_ = 0;
my $mail_address_ = "";
my $author_ = "";
my $use_median_cutoff_ = 1;
my $install_ = 1;

my @strategy_filevec_ = ();
my @intermediate_files_ = ();

my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_params_dir_ = $work_dir_."/params_dir";
my $local_stir_strats_dir_ = $work_dir_."/stir_strats";
my $local_strats_dir_ = $work_dir_."/strats";
my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
if ( ! ( -d $local_stir_strats_dir_ ) ) { `mkdir -p $local_stir_strats_dir_`; }
if ( ! ( -d $MODELING_STRATS_DIR ) ) { `mkdir -p $MODELING_STRATS_DIR`; }
if ( ! ( -d $MODELING_MODELS_DIR ) ) { `mkdir -p $MODELING_MODELS_DIR`; }
if ( ! ( -d $MODELING_PARAMS_DIR ) ) { `mkdir -p $MODELING_PARAMS_DIR`; }
if ( ! ( -d $MODELING_STIR_STRATS_DIR ) ) { `mkdir -p $MODELING_STIR_STRATS_DIR`; }

print "$main_log_file_\n";
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);


# load instruction file
LoadInstructionFile ( );

my $base_stratfilename_ = $work_dir_."/stir_strats/w_stir_strat_".$shortcode_;
my $base_im_stratfilename_ = $work_dir_."/strats/w_strat_".$shortcode_;


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

    open INSTRUCTIONFILEHANDLE, "+< $instructionfilename_ " or PrintStacktraceAndDie ( "$0 Could not open $instructionfilename_\n" );
    
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
		} 
		else 
		{ # $current_instruction_ is valid
		    given ( $current_instruction_ ) 
		    {
			when ("SHORTCODE") 
			{
			    $shortcode_ = $instruction_line_words_[0];
			}
			when ("NAME") 
			{
			    $name_ = $instruction_line_words_[0];
			}
			when ("INSTALL") 
			{
			    $mail_body_.= "-------INSTALL FLAG IS OFF---------\n" ;
			    $install_ = $instruction_line_words_[0];
			}
			
			when ("DELETE_INTERMEDIATE_FILES") 
			{
			    $delete_intermediate_files_ = 1; # Disabled , since a lot of sim_strats , datagens were segfaulting with bmf_eq
			}
    			when ("STRATEGYNAME") 
			{
			    $strategyname_ = $instruction_line_words_[0];
			}
			when ("TRADING_START_END_YYYYMMDD") 
			{
			    if ( ( $#instruction_line_words_ >= 1 ) && 
				 ( ( ! ( $trading_start_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) ) )
			    {
				$trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[0] );
				$trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[1] );
			    }
			}
			when ("TRADING_START_YYYYMMDD") 
			{
			    if ( ( ! $trading_start_yyyymmdd_ ) ||
				 ( rand(1) > 0.50 ) )
			    {
				$trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
			    }
			}
			when ("TRADING_END_YYYYMMDD") 
			{
			    if ( ( ! $trading_end_yyyymmdd_ ) ||
				 ( rand(1) > 0.50 ) )
			    {
				$trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
			    }
			}
			when ("TRADING_DAYS")
			{
			    for ( my $i = 0 ; $i <= $#instruction_line_words_ ; $i ++ )
			    {
				if ( ( $instruction_line_words_[$i] ) && 
				     ( substr ( $instruction_line_words_[$i], 0, 1) ne '#' ) &&
				     ( ValidDate ( $instruction_line_words_[$i] ) && ( ! ( NoDataDateForShortcode ( $instruction_line_words_[$i] , $shortcode_ ) || ( SkipWeirdDate ( $instruction_line_words_[$i] ) ) ) ) ) )
				{
				    if ( ! FindItemFromVec ( $instruction_line_words_ [ $i ] , @trading_days_ ) )
				    {
					push ( @trading_days_ , $instruction_line_words_ [ $i ] );
				    }
				}
			    }
			}
			when ("TRADING_EXCLUDE_DAYS")
			{
			    for ( my $i = 0 ; $i <= $#instruction_line_words_ ; $i ++ )
			    {
				if ( ( $instruction_line_words_[$i] ) && 
				     ( substr ( $instruction_line_words_[$i], 0, 1) ne '#' ) &&
				     ( ValidDate ( $instruction_line_words_[$i] ) && ( ! ( NoDataDateForShortcode ( $instruction_line_words_[$i] , $shortcode_ ) || ( SkipWeirdDate ( $instruction_line_words_[$i] ) ) ) ) ) )
				{
				    if ( ! FindItemFromVec ( $instruction_line_words_ [ $i ] , @trading_exclude_days_ ) )
				    {
					push ( @trading_exclude_days_ , $instruction_line_words_ [ $i ] );
				    }
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
				    $trading_start_hhmm_set_already_ = 1;
				    
				    $trading_end_hhmm_ = $instruction_line_words_[1];
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
				$trading_start_hhmm_set_already_ = 1;
			    }
			}
			when ("TRADING_END_HHMM") 
			{
			    if ( ( $trading_end_hhmm_set_already_ != 1 ) ||
				 ( rand(1) > 0.50 ) )
			    { 
				$trading_end_hhmm_ = $instruction_line_words_[0];
				$trading_end_hhmm_set_already_ = 1;
			    }
			}
                        when ("MODELFILELIST")
                        {
                            my @t_modelfilelist_ = PopulateFileList ( $instruction_line_words_[1] );
			    $shortcode_to_modellist_{$instruction_line_words_[0]} = [ @t_modelfilelist_ ];                           
                        }                     
                        when ("PARAMFILELIST")
                        {
                            my @t_paramfilelist_ = PopulateFileList  ( $instruction_line_words_[1] );
                            $shortcode_to_paramlist_{$instruction_line_words_[0]} = [ @t_paramfilelist_ ];
                            push(@shortcode_vec_, $instruction_line_words_[0]);
                        }
			when ("MIN_NUM_FILES_TO_CHOOSE") 
			{
			    $min_num_files_to_choose_ = $instruction_line_words_[0];
			}
			when ("NUM_FILES_TO_CHOOSE") 
			{
			    $num_files_to_choose_ = $instruction_line_words_[0];
			}
			when ("HISTORICAL_SORT_ALGO")
			{
			    if ( ( substr ( $instruction_line_words_[0], 0, 1) ne '#' ) ) # no comments in this word
			    { 
				$historical_sort_algo_ = $instruction_line_words_[0];
				print $main_log_file_handle_ "HISTORICAL_SORT_ALGO set to $historical_sort_algo_\n"; 
			    }
			}
			when ("MAIL_ADDRESS") 
			{
			    $mail_address_ = $instruction_line_words_[0];
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
}

sub PopulateFileList 
{
    my ( $filelist_file_ ) = @_;
    my @t_filenamelist_ = ( );
    open FILELISTHANDLE, "< $filelist_file_ " or PrintStacktraceAndDie ( "$0 Could not open $filelist_file_\n" );
    while ( my $thisline_ = <FILELISTHANDLE> )
    {
        chomp ( $thisline_ ); # remove newline

        my @filelist_line_words_ = split ( ' ', $thisline_ );
        push ( @t_filenamelist_, $filelist_line_words_[0] );
    }    
    return @t_filenamelist_;
}

sub MakeAllStrategies
{
    my ( $index_, @t_params_ ) = @_;
    if ( $index_ > $#shortcode_vec_ )
    {
        MakeNewStrategy ( @t_params_ );
        $strategy_progid_ ++;
	return;
    }else {                    
        my $t_shc_ = $shortcode_vec_[$index_];
        foreach my $t_param_ ( @ { $shortcode_to_paramlist_{$t_shc_} } ) 
        {
             $t_params_[$index_] = $t_param_;
             MakeAllStrategies ( $index_+1, @t_params_ );
        }
    }    
}

sub MakeModelFileList 
{
    foreach my $t_shc_ ( keys %shortcode_to_modellist_ )
    {
        my @t_modellist_ = $shortcode_to_modellist_{$t_shc_};
        my $r_indx_ = int(rand($#t_modellist_+1 ));    	
        push ( @modelfilelist_, $shortcode_to_modellist_{$t_shc_}[$r_indx_] );
    }
}

sub MakeNewStrategy
{
    my ( @t_paramlist_ ) = @_;
    my $im_strategy_file_ = $base_im_stratfilename_."_".$strategy_progid_;
    my $strategy_file_ = $base_stratfilename_."_".$strategy_progid_;
    my $exec_cmd_ = "echo STRUCTURED_STRATEGYLINE $im_strategy_file_ $strategy_progid_ > $strategy_file_";
    `$exec_cmd_`;    
    open IMSTRATFILE, "> $im_strategy_file_" or PrintStacktraceAndDie ( "Could not open strategy_list_file $im_strategy_file_ for writing!\n" );
    print IMSTRATFILE "STRUCTURED_TRADING $shortcode_ $strategyname_ $t_paramlist_[0] $trading_start_hhmm_ $trading_end_hhmm_ $strategy_progid_";
    for ( my $i=1; $i <= $#t_paramlist_; $i++ )
    {
        print IMSTRATFILE "\nSTRATEGYLINE $shortcode_vec_[$i] $modelfilelist_[$i-1] $t_paramlist_[$i]"; 
    }     
    push ( @strategy_filevec_, $strategy_file_ );
}

sub MakeStrategyFiles 
{
    print $main_log_file_handle_ "MakeStrategyFiles\n";
    MakeModelFileList ( );
    my @tt_params_ = ("") x ($#shortcode_vec_ + 1 );
    MakeAllStrategies ( 0, @tt_params_  );    
}

sub RunSimulationOnCandidates
{
    print $main_log_file_handle_ "RunSimulationOnCandidates\n";
    
    print $main_log_file_handle_ " number of unique strat file ".scalar ( @strategy_filevec_ )."\n" ;
    
    my @non_unique_results_filevec_=();
    my $tradingdate_ = $trading_end_yyyymmdd_;
    my $max_days_at_a_time_ = 2000;
    for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
    {
	if ( ! defined $tradingdate_ || $tradingdate_ eq '' ) { 
	    print "Hey the tradingdate_ is missing. File: ",__FILE__, " Liine: ", __LINE__, "\n t_day_index=$t_day_index_, trading_start_end_date=[$trading_start_yyyymmdd_] [$trading_end_yyyymmdd_]\n";
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next; 
	}
	if ( ( $tradingdate_ < 20101231 ) || ( $tradingdate_ > 20200220 ) )
	{
	    last;
	}
	if ( SkipWeirdDate ( $tradingdate_ ) ||
	     ( NoDataDateForShortcode ( $tradingdate_ , $first_shortcode_ ) ) || 
	     ( IsDateHoliday ( $tradingdate_ ) || 
	       ( ( $first_shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $first_shortcode_ ) ) ) ) )
	{
	    print $main_log_file_handle_ " ignoring trading date ".$tradingdate_." no-data/holiday" ;
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next;
	}
	
	if ( FindItemFromVec ( $tradingdate_ , @trading_exclude_days_ ) )
	{
	    print $main_log_file_handle_ " ignoring trading date ".$tradingdate_." part of exclude trading days vector" ;
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next;
	}

	if ( $#trading_days_ >= 0 &&
	     ! FindItemFromVec ( $tradingdate_ , @trading_days_ ) )
	{
	    print $main_log_file_handle_ " ignoring trading date ".$tradingdate_." not part of trading days vector" ;
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next;
	}

	
	if ( ( ! ValidDate ( $tradingdate_ ) ) ||
	     ( $tradingdate_ < $trading_start_yyyymmdd_ ) )
	{
	    print $main_log_file_handle_ " end of sumulation ".$tradingdate_." less than trading_strat_date ".$trading_start_yyyymmdd_ ;
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
		#if ( -e $temp_strategy_cat_file_ ) { `rm -f $temp_strategy_cat_file_`; }
		for ( my $t_strategy_filevec_index_ = $strategy_filevec_front_; $t_strategy_filevec_index_ <= $strategy_filevec_back_; $t_strategy_filevec_index_ ++ )
		{
		    print $main_log_file_handle_ $strategy_filevec_[$t_strategy_filevec_index_]."\n";
		    print TSLF $strategy_filevec_[$t_strategy_filevec_index_]."\n";                    
		    `cat $strategy_filevec_[$t_strategy_filevec_index_] >> $temp_strategy_cat_file_`;
		}
		close TSLF;
		
		my @sim_strategy_output_lines_=(); # stored to seive out the SIMRESULT lines
		my %unique_id_to_pnlstats_map_=(); # stored to write extended results to the database
		{

		    # using the correct market model for all users		    
		    my $market_model_index_ = GetMarketModelForShortcode ( $first_shortcode_ );

		    my $sim_strat_cerr_file_ = $temp_strategy_cat_file_."_cerr";

		    # Assuming glibc goes to stderr - unconfirmed.
		    my $exec_cmd="$LIVE_BIN_DIR/sim_strategy SIM $temp_strategy_cat_file_ $unique_gsm_id_ $tradingdate_ $market_model_index_ 0 0.0 0 ADD_DBG_CODE -1 2>/dev/null"; # using hyper optimistic market_model_index, added nologs argument, not using fake delays as ec2_globalresults are also not using them
                    print $main_log_file_handle_ $exec_cmd."\n"; 
		    @sim_strategy_output_lines_=`$exec_cmd`;			
		    print $main_log_file_handle_ "@sim_strategy_output_lines_";
		    
		    my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);
		    if ( ExistsWithSize ( $this_tradesfilename_ ) )
		    {
			$exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_stir_2.pl $this_tradesfilename_";
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
		    { 
			#`rm -f $this_tradesfilename_`; 
			my $this_tradeslogfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int($unique_gsm_id_);
			#`rm -f $this_tradeslogfilename_`;
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
                            if ( ( $rwords_[1] > 0 ) ) 
                            {
                                my $unique_sim_id_ = GetUniqueSimIdFromStirCatFile ( $temp_strategy_cat_file_, $psindex_ );                                
                                if ( ! exists $unique_id_to_pnlstats_map_{$unique_sim_id_} )
                                {
                                    $unique_id_to_pnlstats_map_{$unique_sim_id_} = "0 0 0 0 0 0 0 0 0 0 0 0 0";
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
		    #if ( -e $temp_strategy_cat_file_ ) { `rm -f $temp_strategy_cat_file_`; }
		    #if ( -e $temp_strategy_list_file_ ) { `rm -f $temp_strategy_list_file_`; }
		    #if ( -e $temp_results_list_file_ ) { `rm -f $temp_results_list_file_`; }
		}
		$strategy_filevec_front_ = $strategy_filevec_back_ + 1; # front is now set to the first item of the next block
		$temp_strategy_list_file_index_ ++;
	    }
	}
	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    }
    @unique_results_filevec_ = GetUniqueList ( @non_unique_results_filevec_ ); # the same result file is written to by all blocks of strategy files on the same date, that's why @non_unique_results_filevec_ might have duplicate entries
}

sub SummarizeLocalResultsAndChoose
{
}
