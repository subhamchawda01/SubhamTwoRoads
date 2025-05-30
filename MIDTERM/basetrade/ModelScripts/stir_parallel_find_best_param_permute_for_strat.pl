#!/usr/bin/perl

# \file ModelScripts/stir_parallel_find_best_params_permute_for_strat.pl
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

require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec
require "$GENPERLLIB_DIR/search_script.pl"; # SearchScript
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromNumDays, GetDatesFromStartDate #IsValidStartEndDate
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
require "$GENPERLLIB_DIR/get_unique_sim_id_from_stir_cat_file.pl"; # GetUniqueSimIdFromStirCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/list_file_to_vec.pl"; #ListFileToVec
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams
require "$GENPERLLIB_DIR/permute_params_constrained.pl"; # PermuteParams
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_start_end_hhmm.pl"; # GetStratStartEndHHMM
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_match_strat_base_excluding_sets.pl"; # MakeStratVecFromDirInTpMatchStratBaseExcludingSets
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllOutputFilesPopulated
require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec
#require "$MODELSCRIPTS_DIR/parllel_params_permute.pl"; #

#require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MAX_STRAT_FILES_IN_ONE_SIM = 40; # please work on optimizing this value
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

# start
my $USAGE="$0 shortcode param_file_with_permutations index[starting with 1] start_date end_date strategy_file_name [min_volume] [max_ttc] [sort_algo] [use_constrained_permute]";

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
                               "ReturnsBasedAggressiveTrading" ) ;
    foreach my $st_exec_logic_( @StandardExecLogics ) {
	my $t_ = $st_exec_logic_; 
	if ( $t_ =~ /$exec_logic_code_/ ) { return $st_exec_logic_; }
	$t_ =~ s/[a-z]//g ;
	# print "$t_ $st_exec_logic_\n" ;
	if ( $t_ =~ /$exec_logic_code_/ ) { return $st_exec_logic_; }
    }
    PrintStacktraceAndDie ( "Wrong ExecLogic Code: $exec_logic_code_\n" );
}

if ( $#ARGV < 5 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $orig_param_filename_ = $ARGV[1];
my $param_file_index_in_strat_ = $ARGV[2];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[3] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[4] );
my $strategy_file_name_ = $ARGV[5] ;
my $use_constrained_permute_ = "";

my $min_volume_ = -1;
my $max_ttc_ = -1;
my $sort_algo_ = "kCNAPnlAverage";
if ( $#ARGV >= 10 )
{
    $min_volume_ = $ARGV [ 6 ];
    $max_ttc_ = $ARGV [ 7 ];
    $sort_algo_ = $ARGV [ 8 ];
}
if ( $#ARGV >= 9 ) { $use_constrained_permute_ = $ARGV[9];}

my $param_file_list_basename_ = basename ( $orig_param_filename_ );
$FBPA_WORK_DIR = $FBPA_WORK_DIR.$shortcode_."/".$param_file_list_basename_."/";

my $trading_start_hhmm_ = "";
my $trading_end_hhmm_ = "";

my $delete_intermediate_files_ = 1;

my $num_files_to_choose_ = 1;
my $min_pnl_per_contract_to_allow_ = -1.10 ;
my $min_volume_to_allow_ = 100; 
my $max_ttc_to_allow_ = 120;

my %strat2param_;
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

my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $temp_strat_filename_ = $local_strats_dir_."/temp_strat_file";
my $local_params_dir_ = $work_dir_."/params_dir";
my $full_strategy_filename_ = "";

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

my @trading_days_vec_ = ( );

if ( ! IsValidStartEndDate($trading_start_yyyymmdd_, $trading_end_yyyymmdd_) ) {
  if ( -f $trading_start_yyyymmdd_ ) {
    open DATESHANDLE, "< $trading_start_yyyymmdd_" or PrintStacktraceAndDie ( "Could not open file $trading_start_yyyymmdd_ for reading" );
    @trading_days_vec_ = <DATESHANDLE>;
    chomp @trading_days_vec_;
    close DATESHANDLE;
  }
  else {
    print STDERR "Either Start/End Dates are invalid or start date is after end-date\n";
    exit(0);
  }
}
else {
  @trading_days_vec_ = GetDatesFromStartDate( $shortcode_, $trading_start_yyyymmdd_, $trading_end_yyyymmdd_, "INVALIDFILE", 2000 );
}

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
}


# end script
$main_log_file_handle_->close;

exit ( 0 );

sub ProcessListArgs
{
    print $main_log_file_handle_ "\nProcessListArgs\n\n";

    print $main_log_file_handle_ "\n$strategy_file_name_\n";
    #models
    my $full_stir_strategy_filename_ = "";
    if ( ExistsWithSize ( $strategy_file_name_ ) )
    {
	$full_stir_strategy_filename_ = $strategy_file_name_;
    }   
    else
    {
	print $main_log_file_handle_ "Strategy not found\n";	
    }

    print $main_log_file_handle_ "\n$full_stir_strategy_filename_\n";  
	
    
    my $exec_cmd_ = "cat $full_stir_strategy_filename_ | awk '{ print \$2}'";
    $full_strategy_filename_ = `$exec_cmd_`;
    chomp( $full_strategy_filename_ ); 
    if ( $use_constrained_permute_ )
    {
      @param_filevec_ = PermuteParamsConstrained ( $orig_param_filename_, $local_params_dir_ , 0.003,20);
    }
    else
    {
      @param_filevec_ = PermuteParams ( $orig_param_filename_, $local_params_dir_ );
    }
    #params
    print $main_log_file_handle_ "param_filevec_ = ".$#param_filevec_."\n";
}

# For each param file & for each model file, we create a strategy file
sub MakeStrategyFiles 
{
    print $main_log_file_handle_ "\nMakeStrategyFiles\n\n";

    my $strategy_progid_ = 1001;

    for (my $param_file_index_ = 0; $param_file_index_ <= $#param_filevec_ ; $param_file_index_++) 
    { # For each param
		
	my $this_param_filename_ = $param_filevec_[$param_file_index_];
		
	if ( ExistsWithSize ( $this_param_filename_ ) )
	{
	    my $this_strategy_im_filebase_ = "strat_im_".$param_file_index_."_".$strategy_progid_ ;
	    my $this_strategy_im_filename_ = $local_strats_dir_."/".$this_strategy_im_filebase_;
	
	    my $this_strategy_filebase_ = "strat_".$param_file_index_."_".$strategy_progid_;
	    my $this_strategy_filename_ = $local_strats_dir_."/".$this_strategy_filebase_;
		    
	    my $exec_cmd_ = "cat $full_strategy_filename_ | awk '{if( NR == ($param_file_index_in_strat_ + 1 ) ) { \$4= \"$this_param_filename_\" } print \$0 }' > $temp_strat_filename_";
	    print $main_log_file_handle_ "$exec_cmd_\n";
	    `$exec_cmd_`;
	    $exec_cmd_ = "cat $temp_strat_filename_ | awk '{if ( NR == 1 ) { \$7=$strategy_progid_ } print \$0 }' > $this_strategy_im_filename_";
	    print $main_log_file_handle_ "$exec_cmd_\n";
	    `$exec_cmd_`;
	
            my $random_id_ = "$strategy_progid_";	
            $exec_cmd_ = "echo STRUCTURED_STRATEGYLINE $this_strategy_im_filename_ $random_id_ > $this_strategy_filename_";
	    `$exec_cmd_`; 
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

sub RunSimulationOnCandidates
{
	print $main_log_file_handle_ "\n\n RunSimulationOnCandidates\n\n";


	my @independent_parallel_commands_ = ( );


	# generate a list of commands which are unique ,
	# independent from each other and can be safely run in parallel.

	my $tradingdate_ = $trading_end_yyyymmdd_;

	foreach my $tradingdate_ (@trading_days_vec_) {

		my $temp_strategy_list_file_index_ = 0;
		for (my $strategy_filevec_index_ = 0; $strategy_filevec_index_ <= $#strategy_filevec_; ) {

			my $temp_strategy_list_file_ = $work_dir_."/temp_strategy_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
			my $temp_strategy_cat_file_ = $work_dir_."/temp_strategy_cat_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
			my $temp_strategy_output_file_ = $work_dir_."/temp_strategy_output_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
			my $temp_strategy_pnl_stats_file_ = $work_dir_."/temp_strategy_pnl_stats_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";

			open ( TSLF, ">",
				$temp_strategy_list_file_ ) or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_\n" );

			for (my $num_files_ = 0; ( $num_files_ < $MAX_STRAT_FILES_IN_ONE_SIM ) && ( $strategy_filevec_index_ <= $#strategy_filevec_ ); $num_files_++) {
				my $this_strategy_filename_ = $strategy_filevec_[ $strategy_filevec_index_ ];
				$strategy_filevec_index_++;

				print TSLF $this_strategy_filename_."\n";
				`cat $this_strategy_filename_ >> $temp_strategy_cat_file_`;
			}
			close ( TSLF );

			# Get a concurrency safe id.
			my $unique_sim_id_ = GetGlobalUniqueId ( );
			my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
			my $this_log_filename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".$unique_sim_id_;

			my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ );
			my $exec_cmd_ = "";
			my $script_path = SearchScript("run_single_strat_file_stir.pl", ());
			$exec_cmd_ = "$script_path $shortcode_ $temp_strategy_cat_file_ $tradingdate_ $temp_strategy_list_file_ $work_dir_ $local_results_base_dir INVALIDDIR $market_model_index_ ";
			push (@independent_parallel_commands_, $exec_cmd_);


		}
	}

	for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; ) {


    my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = $MAX_CORES_TO_USE_IN_PARALLEL;


      $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );


    for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ ) {


      print $main_log_file_handle_ $independent_parallel_commands_[ $command_index_ ]."\n";
      my $exec_cmd_ = $independent_parallel_commands_[ $command_index_ ];
      my $temp_script_ = $work_dir_."/temp_script_".$command_index_;
      open SCRIPT, "> $temp_script_" or PrintStacktraceAndDie ( "Could not open $temp_script_ for writing\n" );
      print SCRIPT "$exec_cmd_\n";
      close ( SCRIPT );

      my $pid_ = fork();
      die "unable to fork $!" unless defined($pid_);

# child process
      if ( ! $pid_ ) {
        exec("sh $temp_script_");
      }

#back to parent process
      print $main_log_file_handle_ "PID of $temp_script_ is $pid_\n";
      $command_index_ ++;
      sleep ( 1 );
    }

    my $t_pid_ = 9999;
    while ( $t_pid_ > 0 ) {
# there are still some child processes running, wait returns -1 when no child process is left
      $t_pid_ = wait();
      print $main_log_file_handle_ "PID of completed process: $t_pid_\n";
    }
  }
}










