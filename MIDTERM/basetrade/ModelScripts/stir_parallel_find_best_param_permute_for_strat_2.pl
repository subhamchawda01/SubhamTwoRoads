#!/usr/bin/perl

# \file ModelScripts/parallel_find_best_params_permute_for_strat.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
#
# SHORTCODE
# TIMEPERIOD
# BASEPX
# PARAMFILE_WITH_PERMUTATIONS
# TRADING_START_YYYYMMDD 
# TRADING_END_YYYYMMDD

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use Fcntl qw (:flock);
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

package ResultLine;
use Class::Struct;

# declare the struct
struct ( 'ResultLine', { pnl_ => '$', volume_ => '$', ttc_ => '$' } );

package main;

sub MakeStrategyFiles ; # takes input_stratfile and makes strategyfiles corresponding to different scale constants
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database
sub SummarizeLocalResultsAndChoose ; # from the files created in the local_results_base_dir choose the best ones to send to pool

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $FBPA_WORK_DIR=$SPARE_HOME."FBPA/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "ankit" || $USER eq "anshul")
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to s

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/list_file_to_vec.pl"; #ListFileToVec
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams
require "$GENPERLLIB_DIR/permute_params_constrained.pl"; # PermuteParamsConstrained
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_start_end_hhmm.pl"; # GetStratStartEndHHMM
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_match_strat_base_excluding_sets.pl"; # MakeStratVecFromDirInTpMatchStratBaseExcludingSets
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort

require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllOutputFilesPopulated
#require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MAX_STRAT_FILES_IN_ONE_SIM = 40; # please work on optimizing this value
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

# start
my $USAGE="$0 shortcode timeperiod basepx strategyname param_file_with_permutations start_date end_date strategy_file_name [min_volume] [max_ttc] [sort_algo] [constrained_permute]";

sub GetPriceTypeFromCode 
{
    my $px_code_ = shift;
    my @StandardPxTypes = ( "Midprice", "MktSizeWPrice", "MktSinusoidal", "OrderWPrice", "OfflineMixMMS" , "TradeWPrice" , "ALL" );
    if ( scalar grep $px_code_ eq $_, @StandardPxTypes ) {
	return $px_code_ ;
    }
    my %px_code_map_ = ( "Mkt" => "MktSizeWPrice",
			 "Mid" => "Midprice",
			 "MidPrice" => "Midprice",
			 "Sin" => "MktSinusoidal",
			 "Owp" => "OrderWPrice",
			 "Twp" => "TradeWPrice",
			 "OMix" => "OfflineMixMMS" );
    
    if ( ! $px_code_map_{ $px_code_ } ) {
	print $px_code_map_ { $px_code_ };
	PrintStacktraceAndDie ( "Wrong PxtyPe: $px_code_.\n");
    }
    return $px_code_map_ { $px_code_ } ;
}

sub GetExecLogicFromCode
{
    my $exec_logic_code_ = shift;
    my @StandardExecLogics = ( "DirectionalAggressiveTrading",
			       "DirectionalInterventionAggressiveTrading",
			       "DirectionalInterventionLogisticTrading",
			       "DirectionalLogisticTrading",
			       "DirectionalPairAggressiveTrading",
			       "PriceBasedAggressiveTrading",
			       "PriceBasedInterventionAggressiveTrading",
			       "PriceBasedSecurityAggressiveTrading",
			       "PriceBasedTrading",
			       "PriceBasedVolTrading",
			       "PriceBasedScalper",
			       "PricePairBasedAggressiveTrading", 
                               "ReturnsBasedAggressiveTrading",
                               "EquityTrading2") ;
    foreach my $st_exec_logic_( @StandardExecLogics ) {
	my $t_ = $st_exec_logic_; 
	if ( $t_ =~ /$exec_logic_code_/ ) { return $st_exec_logic_; }
	$t_ =~ s/[a-z]//g ;
	# print "$t_ $st_exec_logic_\n" ;
	if ( $t_ =~ /$exec_logic_code_/ ) { return $st_exec_logic_; }
    }
    PrintStacktraceAndDie ( "Wrong ExecLogic Code: $exec_logic_code_\n" );
}

if ( $#ARGV < 7 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $timeperiod_ = $ARGV[1];
my $basepx_pxtype_ = GetPriceTypeFromCode( $ARGV[2] );
my $strategyname_ = GetExecLogicFromCode( $ARGV[3] );
my $orig_param_filename_ = $ARGV[4];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[5] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[6] );
my $strategy_file_name_ = basename ( $ARGV[7] );
my $use_constrained_permute_ = "";
if ( $#ARGV >= 8 ) { $use_constrained_permute_ = $ARGV[8] ; }
my $min_volume_ = -1;
my $max_ttc_ = -1;
my $sort_algo_ = "kCNAPnlAverage";
if ( $#ARGV >= 10 )
{
    $min_volume_ = $ARGV [ 8 ];
    $max_ttc_ = $ARGV [ 9 ];
    $sort_algo_ = $ARGV [ 10 ];
}

my $param_file_list_basename_ = basename ( $orig_param_filename_ );
$FBPA_WORK_DIR = $FBPA_WORK_DIR.$shortcode_."/".$timeperiod_."/".$strategyname_."/".$param_file_list_basename_."/";

my $trading_start_hhmm_ = "";
my $trading_end_hhmm_ = "";

my $delete_intermediate_files_ = 1;

my $num_files_to_choose_ = 1;
my $min_pnl_per_contract_to_allow_ = -1.10 ;
my $min_volume_to_allow_ = 100; 
my $max_ttc_to_allow_ = 120;

my %strat2param_ ;
my @model_filevec_ = ();
my @param_filevec_ = ();
my @strategy_filevec_ = ();
my %param_to_resultvec_;
my %param_to_cmedpnl_;
my %param_to_cmedvolume_;

my @intermediate_files_ = ();

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $FBPA_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	print STDERR "Surprising but this dir exists\n";
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $FBPA_WORK_DIR.$unique_gsm_id_; 
    }
    else
    {
	last;
    }
}

print "$shortcode_ $timeperiod_ $basepx_pxtype_ $strategyname_ $strategy_file_name_ $work_dir_ \n";

my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $local_params_dir_ = $work_dir_."/params_dir";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir ) ) { `mkdir -p $local_results_base_dir`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

# Get param files and model files
ProcessListArgs ( );
if ( $#param_filevec_ < 0 )
{
    print "Exiting due to 0 param_filevec_\n";
    print $main_log_file_handle_ "Exiting due to 0 param_filevec_\n";
}
else
{
# From the given arguments create the strategy files for simulations
    MakeStrategyFiles ( );

# Run simulations on the created strategy files
    RunSimulationOnCandidates ( );

# among the candidates choose the best
    SummarizeLocalResultsAndChoose ( );
}
# end script
$main_log_file_handle_->close;

exit ( 0 );

sub ProcessListArgs
{
    print $main_log_file_handle_ "\nProcessListArgs\n\n";

    #models
    my @exclude_tp_dirs_ = ();
    my @all_strats_in_dir_ = MakeStratVecFromDirInTpMatchStratBaseExcludingSets ( $MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_, $strategyname_, $basepx_pxtype_, @exclude_tp_dirs_ );

    my @filtered_strats_in_dir_ = ( );
    foreach my $t_strat_in_dir_ ( @all_strats_in_dir_ )
    {
	if ( index ( $t_strat_in_dir_ , $strategy_file_name_ ) >= 0 )
	{
	    push ( @filtered_strats_in_dir_ , $t_strat_in_dir_ );
	    last;
	}
    }

    if ( $#filtered_strats_in_dir_ < 0 )
    {
	print $main_log_file_handle_ "Exiting due to filtered_strats_in_dir_ = 0 but all_strats_in_dir_ = ".($#all_strats_in_dir_ + 1 )."\n";
    }
    else
    {
	print $main_log_file_handle_ "Found $strategy_file_name_\n";
    }

    my @t_model_filevec_ = ();
    for my $t_strategy_filename_ ( @filtered_strats_in_dir_ )
    {
	if ( ! ( $trading_start_hhmm_ ) )
	{ # first time... so take down start and hhmm
	    ( $trading_start_hhmm_, $trading_end_hhmm_ ) = GetStratStartEndHHMM ( $t_strategy_filename_ ) ;
	}

	my ( $t_model_filename_, $t_param_filename_ ) = GetModelAndParamFileNames ( $t_strategy_filename_ ) ;
	push ( @t_model_filevec_, $t_model_filename_ );
    }
    @model_filevec_ = GetUniqueList ( @t_model_filevec_ ) ;

    if ( $#model_filevec_ < 0 ) { 
	print $main_log_file_handle_ "No model file in your modelling folder: $MODELING_STRATS_DIR/$shortcode_, $timeperiod_, $strategyname_, $basepx_pxtype_, @exclude_tp_dirs_\n" ; 
	print "No model file in your modelling folder: $MODELING_STRATS_DIR/$shortcode_, $timeperiod_, $strategyname_, $basepx_pxtype_, @exclude_tp_dirs_\n" ; 
    }


#params

    if ( $use_constrained_permute_ )
    {
      @param_filevec_ = PermuteParamsConstrained ( $orig_param_filename_, $local_params_dir_ );
    }
    else
    {
      @param_filevec_ = PermuteParams ( $orig_param_filename_, $local_params_dir_ );
    }
    print $main_log_file_handle_ "param_filevec_ = ".$#param_filevec_."\n";
}

# For each param file & for each model file, we create a strategy file
sub MakeStrategyFiles 
{
    print $main_log_file_handle_ "\nMakeStrategyFiles\n\n";

    my $strategy_progid_ = 1001;

    for (my $model_file_index_ = 0; $model_file_index_ <= $#model_filevec_; $model_file_index_++) 
    { # For each model

	my $this_model_filename_ = $model_filevec_[$model_file_index_];
	if ( ExistsWithSize ( $this_model_filename_ ) )
	{
	    for (my $param_file_index_ = 0; $param_file_index_ <= $#param_filevec_ ; $param_file_index_++) 
	    { # For each param
		
		my $this_param_filename_ = $param_filevec_[$param_file_index_];
		
		if ( ExistsWithSize ( $this_param_filename_ ) )
		{
		    my $this_strategy_filebase_ = "strat_".$param_file_index_."_".$model_file_index_."_".$strategy_progid_ ;
		    my $this_strategy_filename_ = $local_strats_dir_."/".$this_strategy_filebase_;
		    
		    my $exec_cmd="$MODELSCRIPTS_DIR/create_strategy_file.pl $this_strategy_filename_ $shortcode_ $strategyname_ $this_model_filename_ $this_param_filename_ $trading_start_hhmm_ $trading_end_hhmm_ $strategy_progid_";
		    print $main_log_file_handle_ "$exec_cmd\n";
		    `$exec_cmd`;
		    $strategy_progid_++; # uniqueness of progid ensures that we can run them in sim together
		    
		    if ( ExistsWithSize (  $this_strategy_filename_ ) )
		    {
			push ( @strategy_filevec_, $this_strategy_filename_ ); 
			$strat2param_ { $this_strategy_filebase_ } = $this_param_filename_ ;
		    }
		}
		else
		{
		    print $main_log_file_handle_ "Skipping param ".$this_param_filename_."\n";
		}
	    }
	}
	else
	{
	    print $main_log_file_handle_ "Skipping model ".$this_model_filename_."\n";
	}
    }
}

sub RunSimulationOnCandidates
{
    print $main_log_file_handle_ "\n\n RunSimulationOnCandidates\n\n";

    my @non_unique_results_filevec_ = ( );

    my @unique_sim_id_list_ = ( );
    my @independent_parallel_commands_ = ( );
    my @tradingdate_list_ = ( );
    my @temp_strategy_list_file_index_list_ = ( );
    my @temp_strategy_list_file_list_ = ( );
    my @temp_strategy_cat_file_list_ = ( );
    my @temp_strategy_output_file_list_ = ( );

    # generate a list of commands which are unique , 
    # independent from each other and can be safely run in parallel.

    my $tradingdate_ = $trading_end_yyyymmdd_;
    my $max_days_at_a_time_ = 2000;
    for ( my $i = 0 ; $i < $max_days_at_a_time_ ; $i ++ )
    {
	if ( SkipWeirdDate ( $tradingdate_ ) ||
	     IsDateHoliday ( $tradingdate_ ) ||
	     IsProductHoliday ( $tradingdate_ , $shortcode_ ) ||
	     NoDataDate ( $tradingdate_ ) )
	{
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_ , 1 );
	    next;
	}

	if ( ! ValidDate ( $tradingdate_ ) ||
	     $tradingdate_ < $trading_start_yyyymmdd_ )
	{
	    last;
	}

	my $temp_strategy_list_file_index_ = 0;
	for ( my $strategy_filevec_index_ = 0 ; $strategy_filevec_index_ <= $#strategy_filevec_ ; )
	{
	    my $temp_strategy_list_file_ = $work_dir_."/temp_strategy_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
	    my $temp_strategy_cat_file_ = $work_dir_."/temp_strategy_cat_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
	    my $temp_strategy_output_file_ = $work_dir_."/temp_strategy_output_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";

	    open ( TSLF , ">" , $temp_strategy_list_file_ ) or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_\n" );
	    for ( my $num_files_ = 0 ; $num_files_ < $MAX_STRAT_FILES_IN_ONE_SIM && $strategy_filevec_index_ <= $#strategy_filevec_ ; $num_files_ ++ )
	    {
		my $this_strategy_filename_ = $strategy_filevec_ [ $strategy_filevec_index_ ];
		$strategy_filevec_index_ ++;

		print TSLF $this_strategy_filename_."\n";
		`cat $this_strategy_filename_ >> $temp_strategy_cat_file_`;
	    }
	    close ( TSLF );

	    my $unique_sim_id_ = GetGlobalUniqueId ( ); # Get a concurrency safe id.

	    my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ );
	    my $exec_cmd_ = $LIVE_BIN_DIR."/sim_strategy SIM ".$temp_strategy_cat_file_." ".$unique_sim_id_." ".$tradingdate_." ".$market_model_index_." ADD_DBG_CODE -1 > ".$temp_strategy_output_file_." 2>&1";

	    push ( @unique_sim_id_list_ , $unique_sim_id_ );
	    push ( @independent_parallel_commands_ , $exec_cmd_ );

	    push ( @tradingdate_list_ ,$tradingdate_ );
	    push ( @temp_strategy_list_file_index_list_ , $temp_strategy_list_file_index_ ); $temp_strategy_list_file_index_ ++;

	    push ( @temp_strategy_list_file_list_ , $temp_strategy_list_file_ );
	    push ( @temp_strategy_cat_file_list_ , $temp_strategy_cat_file_ );
	    push ( @temp_strategy_output_file_list_ , $temp_strategy_output_file_ );
	}

	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_ , 1 );
    }

    # process the list of commands , processing MAX_CORES_TO_USE_IN_PARALLEL at once
    for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; )
    {
	my @output_files_to_poll_this_run_ = ( );

	my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
	for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ )
	{
	    push ( @output_files_to_poll_this_run_ , $temp_strategy_output_file_list_ [ $command_index_ ] );

	    { # empty the output result file.
		my $exec_cmd_ = "> ".$temp_strategy_output_file_list_ [ $command_index_ ];
		`$exec_cmd_`;
	    }

	    print $main_log_file_handle_ $independent_parallel_commands_ [ $command_index_ ]."\n";
	    my $exec_cmd_ = $independent_parallel_commands_ [ $command_index_ ]." &";
	    `$exec_cmd_`;

	    $command_index_ ++;
	    sleep ( 1 );
	}

	while ( ! AllOutputFilesPopulated ( @output_files_to_poll_this_run_ ) )
	{ # there are still some sim-strats which haven't output SIMRESULT lines
	    sleep ( 1 );
	}
    }

    for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ )
    {
	my %unique_id_to_pnlstats_map_ = ( );

	my $unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ];
	my $tradingdate_ = $tradingdate_list_ [ $command_index_ ];
	my $temp_strategy_list_file_index_ = $temp_strategy_list_file_index_list_ [ $command_index_ ];
	my $temp_strategy_list_file_ = $temp_strategy_list_file_list_ [ $command_index_ ];
	my $temp_strategy_cat_file_ = $temp_strategy_cat_file_list_ [ $command_index_ ];
	my $temp_strategy_output_file_ = $temp_strategy_output_file_list_ [ $command_index_ ];
	my $exec_cmd_ = "cat ".$temp_strategy_output_file_list_ [ $command_index_ ];
	my @tradeinit_output_lines_ = `$exec_cmd_`;

	my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
	if ( ExistsWithSize ( $this_trades_filename_ ) )
	{
	    my $exec_cmd_ = $MODELSCRIPTS_DIR."/get_pnl_stats_2.pl $this_trades_filename_";
	    print $main_log_file_handle_ $exec_cmd_."\n";
	    my @pnlstats_output_lines_ = `$exec_cmd_`;

	    for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
	    {
		my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
		if( $#rwords_ >= 1 )
		{
		    my $unique_strat_id_ = $rwords_[0];
		    splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_strat_id_
		    $unique_id_to_pnlstats_map_{$unique_strat_id_} = join ( ' ', @rwords_ );
		}
	    }
	}

	{
	    `rm -f $this_trades_filename_`;
	    my $this_tradeslogfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int ( $unique_sim_id_ );
	    `rm -f $this_tradeslogfilename_`;
	}

	my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";

	open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );
	for ( my $t_tradeinit_output_lines_index_ = 0, my $psindex_ = 0; $t_tradeinit_output_lines_index_ <= $#tradeinit_output_lines_; $t_tradeinit_output_lines_index_ ++ )
	{
	    if ( $tradeinit_output_lines_ [ $t_tradeinit_output_lines_index_ ] =~ /SIMRESULT/ )
	    { # SIMRESULT pnl volume                                                                                                                                                                                               
		my @rwords_ = split ( ' ', $tradeinit_output_lines_[$t_tradeinit_output_lines_index_] );
		splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc                                                                                     
		my $remaining_simresult_line_ = join ( ' ', @rwords_ );
		if ( ( $rwords_[1] > 0 ) || # volume > 0                                                                                                                                                                           
		     ( ( $shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day                                                                      
		{
		    my $unique_strat_id_ = GetUniqueSimIdFromCatFile ( $temp_strategy_cat_file_, $psindex_ );
		    if ( ! exists $unique_id_to_pnlstats_map_{$unique_strat_id_} )
		    {
			$unique_id_to_pnlstats_map_{$unique_strat_id_} = "0 0 0 0 0 0 0 0 0 0 0";
		    }
		    printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_{$unique_strat_id_}, $unique_strat_id_ ;
		    printf TRLF "%s %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_{$unique_strat_id_}, $unique_strat_id_;
		}
		$psindex_ ++;
	    }
	}
	close TRLF;

	if ( ExistsWithSize ( $temp_results_list_file_ ) )
	{
	    my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database.pl $temp_strategy_list_file_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir";
	    print $main_log_file_handle_ "$exec_cmd\n";
	    my $this_local_results_database_file_ = `$exec_cmd`;
	    push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
	}
    }

    @unique_results_filevec_ = GetUniqueList ( @non_unique_results_filevec_ );
}

sub SummarizeLocalResultsAndChoose
{
    for ( my $i = 0 ; $i <= $#unique_results_filevec_; $i++ )
    {
	my $t_results_filename_ = $unique_results_filevec_[$i];
	my $is_open_ = open RESULTSFILEHANDLE, "< $t_results_filename_ ";
	if ( ! $is_open_ )
	{
	    print STDERR "$0 SummarizeLocalResultsAndChoose: Could not open results_file $t_results_filename_\n" ;
	}
	else
	{
	    while ( my $result_line_ = <RESULTSFILEHANDLE> )
	    {
		my @result_line_words_ = split ( ' ', $result_line_ );
		if ( $#result_line_words_ >= 17 )
		{ # dependant on format in results
		    # 0 is stratfilename
		    # 2 is pnl
		    # 3 is volume
		    # 9 is avg_ttc
		    # 17 is drawdown
		    my $strategy_filebase_ = $result_line_words_[0];
		    my $pnl_ = $result_line_words_[2];
		    my $volume_ = $result_line_words_[3];
		    my $ttc_ = $result_line_words_[9];
		    my $drawdown_ = $result_line_words_[17];

		    if ( exists $strat2param_{$strategy_filebase_} )
		    {
			my $this_param_file_ = $strat2param_{$strategy_filebase_} ;

			my $new_result_line_ = new ResultLine;
			$new_result_line_->pnl_ ( $pnl_ - ( 0.33 * $drawdown_ ) ); # conservative pnl
			$new_result_line_->volume_ ( $volume_ );
			$new_result_line_->ttc_ ( $ttc_ );
			
			if ( exists $param_to_resultvec_{$this_param_file_} ) 
			{ 
			    my $scalar_ref_ = $param_to_resultvec_{$this_param_file_};
			    push ( @$scalar_ref_, $new_result_line_ );
			} 
			else 
			{ # seeing this param for the first time
			    my @result_vec_ = ();
			    push ( @result_vec_, $new_result_line_ );
			    $param_to_resultvec_{$this_param_file_} = [ @result_vec_ ] ;
			}
			
		    }
		}
	    }
	    close RESULTSFILEHANDLE ;
	}
    }

    foreach my $this_param_file_ ( keys %param_to_resultvec_ )
    {
	my @pnl_vec_ = ();
	
	my $scalar_ref_resultvec_ = $param_to_resultvec_{$this_param_file_};
	for ( my $resultvec_index_ = 0 ; $resultvec_index_ <= $#$scalar_ref_resultvec_ ; $resultvec_index_ ++ )
	{ # foreach result vec item
	    my $this_result_line_ = $$scalar_ref_resultvec_[$resultvec_index_];
	    push ( @pnl_vec_, $this_result_line_->pnl_() ) ;
	}

	my $cmed_pnl_ = GetAverage ( \@pnl_vec_ );

	$param_to_cmedpnl_{$this_param_file_} = $cmed_pnl_;
    }

    foreach my $this_param_file_ ( keys %param_to_resultvec_ )
    {
	my @volume_vec_ = ();
	
	my $scalar_ref_resultvec_ = $param_to_resultvec_{$this_param_file_};
	for ( my $resultvec_index_ = 0 ; $resultvec_index_ <= $#$scalar_ref_resultvec_ ; $resultvec_index_ ++ )
	{ # foreach result vec item
	    my $this_result_line_ = $$scalar_ref_resultvec_[$resultvec_index_];
	    push ( @volume_vec_, $this_result_line_->volume_() ) ;
	}

	my $cmed_volume_ = GetAverage ( \@volume_vec_ );

	$param_to_cmedvolume_{$this_param_file_} = $cmed_volume_;
    }

    my @params_sorted_by_cmedpnl_ = sort { $param_to_cmedpnl_{$b} <=> $param_to_cmedpnl_{$a} } keys %param_to_cmedpnl_;

#    printf "#PARAM    CMEDPNL   CMEDVOL\n";
    printf $main_log_file_handle_ "#PARAM    CMEDPNL   CMEDVOL\n";
    for my $this_param_file_ ( @params_sorted_by_cmedpnl_ ) 
    {
#	printf "%s %d %d\n", $this_param_file_, $param_to_cmedpnl_{$this_param_file_}, $param_to_cmedvolume_{$this_param_file_};
	printf $main_log_file_handle_ "%s %d %d\n", $this_param_file_, $param_to_cmedpnl_{$this_param_file_}, $param_to_cmedvolume_{$this_param_file_};
    }
}

