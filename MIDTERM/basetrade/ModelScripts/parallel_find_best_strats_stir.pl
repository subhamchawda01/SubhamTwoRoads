#!/usr/bin/perl

# \file ModelScripts/parallel_find_best_strats_stir.pl
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
my $STIR_WORK_DIR=$SPARE_HOME."STIR/";

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
my $USAGE="$0 structure START_TIME END_TIME strategyname model_file_list param_file_list start_date end_date num_files_to_choose [sort_algo]";

sub GetExecLogicFromCode
{
    my $exec_logic_code_ = shift;
    my @StandardExecLogics = ( "StructuredPriceBasedAggressiveTrading" ) ;
    foreach my $st_exec_logic_( @StandardExecLogics ) {
	my $t_ = $st_exec_logic_; 
	if ( $t_ =~ /$exec_logic_code_/ ) { return $st_exec_logic_; }
	$t_ =~ s/[a-z]//g ;
	# print "$t_ $st_exec_logic_\n" ;
	if ( $t_ =~ /$exec_logic_code_/ ) { return $st_exec_logic_; }
    }
    PrintStacktraceAndDie ( "Wrong ExecLogic Code: $exec_logic_code_\n" );
}

if ( $#ARGV < 8 ) { print $USAGE."\n"; exit ( 0 ); }
my $structure_ = $ARGV[0];
my $start_time_ = $ARGV[1];
my $end_time_ = $ARGV[2];
my $model_list_filename_ = $ARGV[4];
my $param_list_filename_ = $ARGV[5];

my $strategyname_ = $ARGV[3] ;
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[6] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[7] );

my $min_volume_ = -1;
my $max_ttc_ = -1;
# not using min volume and max ttc right now

my $sort_algo_ = "kCNAPnlAverage";
if ( $#ARGV >= 9 )
{
    $sort_algo_ = $ARGV [ 9 ];
}

$STIR_WORK_DIR = $STIR_WORK_DIR.$structure_."/".$strategyname_."/";

my $delete_intermediate_files_ = 1;

my $num_files_to_choose = $ARGV[8];
my $min_pnl_per_contract_to_allow_ = -1.10 ;
my $min_volume_to_allow_ = 100; 
my $max_ttc_to_allow_ = 120;

my %strat2param_ ;
my @model_filevec_ = ();
my @param_filevec_ = ();
my @strategy_filevec_ = ();
my @shortcodes_ = ( );
my %shc_to_paramlist_ = ( );
my %shc_to_modellist_ = ( );
my @common_params_ = ( );
my %param_to_resultvec_;
my %strat_to_resultvec_;
my %param_to_cmedpnl_;
my %param_to_cmedvolume_;

my @intermediate_files_ = ();

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $STIR_WORK_DIR.$unique_gsm_id_; 

for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	print STDERR "Surprising but this dir exists\n";
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $STIR_WORK_DIR.$unique_gsm_id_; 
    }
    else
    {
	last;
    }
}

my $running_strat_filename_ = $work_dir_."/running_strat_file";
my $tmp_strat_file_ = $work_dir_."/tmp_strat_file";
my $stir_info_file_ = "/spare/local/tradeinfo/StructureInfo/structured_trading.txt";

#print "$structure_ $timeperiod_ $basepx_pxtype_ $strategyname_ $strategy_file_name_ $work_dir_ \n";

my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir ) ) { `mkdir -p $local_results_base_dir`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

GetListofShortcodes ( );

my $first_shortcode_ = $shortcodes_[0];

print $first_shortcode_."\n";

GetParamFiles ( );
GetModelFiles ( );

print "Done making param files\n";

# From the given arguments create the strategy files for simulations
    MakeStrategyFiles ( );

print "Done making strat files\n";
print "num strats ".$#strategy_filevec_."\n";
# Run simulations on the created strategy files
    RunSimulationOnCandidates ( );

print "Done running sims\n";

# among the candidates choose the best
    SummarizeLocalResultsAndChoose ( );
# end script
$main_log_file_handle_->close;

exit ( 0 );

sub GetListofShortcodes 
{   
    open STIR_FILE , "< $stir_info_file_" or PrintStacktraceAndDie ( "Could not open the input $stir_info_file_\n" );
    my @stir_file_vec_ = <STIR_FILE>;

    foreach my $line_ ( @stir_file_vec_ )
    {
	chomp($line_);
	my @stir_line_words_ = split ( ' ', $line_ );
	
	if ( $#stir_line_words_ < 1 )
	{
	   next;
	}	
	
	if ( $stir_line_words_[0] eq $structure_ )
	{
	   for ( my $stir_line_index_ = 1; $stir_line_index_ <= $#stir_line_words_; $stir_line_index_ ++ )
	   {
		push ( @shortcodes_, $stir_line_words_[$stir_line_index_] );
	   }    
	   last;
	}
    }
    close STIR_FILE;
    open OUTRUNNINGSTRATFILE , "> $running_strat_filename_" or PrintStacktraceAndDie ( "Could not open the output_strategyfilename $running_strat_filename_\n" );
    print OUTRUNNINGSTRATFILE "STRUCTURED_TRADING ".$structure_." ".$strategyname_." dummy_common_param ".$start_time_." ".$end_time_." 00000";

    for ( my $i = 0; $i <= $#shortcodes_; $i++ )
    {
	print OUTRUNNINGSTRATFILE "\nSTRATEGYLINE ".$shortcodes_[$i]." dummy_model dummy_param";
    }
    close OUTRUNNINGSTRATFILE;     
}

sub GetModelFiles
{
  open MODELLISTNAME, "< $model_list_filename_" or PrintStacktraceAndDie ( "Could not open the input  $model_list_filename_ \n" );
  my @model_list_file_vec_ = <MODELLISTNAME>;

  if ( $#model_list_file_vec_ < 0 )
  {
     print "model list empty\n";
     exit(0);
  }

  foreach my $line_ ( @model_list_file_vec_ )
  {
     chomp($line_);
     my @model_list_line_words_ = split ( ' ', $line_ );

     if ( $#model_list_line_words_ < 1 )
     {
        next;
     }

     my $t_shortcode_ = $model_list_line_words_[0];
     my @t_modellist_ = MakeFilenameVecFromList ( $model_list_line_words_[1] );
     print "shortcode : ".$t_shortcode_." num models : ".$#t_modellist_."\n";
     $shc_to_modellist_{$t_shortcode_} = @t_modellist_ ;
  }
}

sub GetParamFiles
{
  print "GetParamFiles\n";
  open PARAMLISTNAME, "< $param_list_filename_" or PrintStacktraceAndDie ( "Could not open the input  $param_list_filename_ \n" );  
  my @param_list_file_vec_ = <PARAMLISTNAME>;

  if ( $#param_list_file_vec_ < 0 )
  {
     print "param list empty\n";
     exit(0);
  }  

  foreach my $line_ ( @param_list_file_vec_ )
  {
     chomp($line_);
     my @param_list_line_words_ = split ( ' ', $line_ );
     
     if ( $#param_list_line_words_ < 1 )
     {
	next;	
     }

     if ( $param_list_line_words_[0] eq "COMMON" )
     {
	@common_params_ = MakeFilenameVecFromList ( $param_list_line_words_[1] );	
	print "num common params ".$#common_params_."\n";
	next;
     }     

     my $t_shortcode_ = $param_list_line_words_[0];
     my @t_paramlist_ = MakeFilenameVecFromList ( $param_list_line_words_[1] );
     print "shortcode : ".$t_shortcode_." num params : ".$#t_paramlist_."\n";
     $shc_to_paramlist_{$t_shortcode_} = @t_paramlist_ ;      
  }
}

# For each param file & for each model file, we create a strategy file
sub MakeStrategyFiles 
{    
    print $main_log_file_handle_ "\nMakeStrategyFiles\n\n";
    print "MakeStrategyFiles\n";

    my $strategy_progid_ = 1001;
   
    my $model_list_basename_ = basename ( $model_list_filename_ );
    my $param_list_basename_ = basename ( $param_list_filename_ );       

    my $strat_base_name_ = "w_stir_".$model_list_basename_."_".$param_list_basename_."_".$start_time_."_".$end_time_;    	
    
       
    print "number common param ".$#common_params_."\n";

    foreach my $common_param_ ( @common_params_ )
    {		
	print "common param : ".$common_param_."\n";
	ReplaceCommonParam ( $running_strat_filename_, $common_param_ );	
	for ( my $shc_index_ = 0; $shc_index_ <= $#shortcodes_; $shc_index_++ )	
	{
	    my $t_shc_ = $shortcodes_[$shc_index_];
	    my @t_paramlist_ = $shc_to_paramlist_{$t_shc_};
	    my @t_modellist_ = $shc_to_modellist_{$t_shc_};
	   
	    print "shortcode ".$t_shc_." index ".$shc_index_."\n";
	    foreach my $t_param_ ( @t_paramlist_ )
	    {
		ReplaceParam ( $running_strat_filename_, $t_param_, $shc_index_ + 2 );
		foreach my $t_model_ ( @t_modellist_ )
		{
		    ReplaceModel ( $running_strat_filename_, $t_model_, $shc_index_ + 2 );
		    my $new_strat_filename_ = $local_strats_dir_."/".$strat_base_name_."_".$strategy_progid_;		    		    
		    my $exec_cmd_ = "cp $running_strat_filename_ $new_strat_filename_";
		    `$exec_cmd_`;
		    push ( @strategy_filevec_, $new_strat_filename_ );
		}		
	    }
	}
    }
}

sub ReplaceCommonParam
{
    my ( $t_strat_filename_, $t_common_param_ ) = @_;
    my $exec_cmd_ = "cp $t_strat_filename_ $tmp_strat_file_";
    `$exec_cmd_`;
    $exec_cmd_ = "cat $tmp_strat_file_ | awk '{if(NR==1){\$4=$t_common_param_} print \$0}' > $t_strat_filename_";
    `$exec_cmd_`;
}

sub ReplaceParam
{
    my ( $t_strat_filename_, $t_param_, $t_index_ ) = @_;
    my $exec_cmd_ = "cp $t_strat_filename_ $tmp_strat_file_";
    `$exec_cmd_`;
    $exec_cmd_ = "cat $tmp_strat_file_ | awk '{if(NR==$t_index_){\$4=$t_param_} print \$0}' > $t_strat_filename_";
    `$exec_cmd_`;
}


sub ReplaceModel
{
    my ( $t_strat_filename_, $t_model_, $t_index_ ) = @_;
    my $exec_cmd_ = "cp $t_strat_filename_ $tmp_strat_file_";
    `$exec_cmd_`;
    $exec_cmd_ = "cat $tmp_strat_file_ | awk '{if(NR==1){\$3=$t_model_} print \$0}' > $t_strat_filename_";
    `$exec_cmd_`;
}

sub RunSimulationOnCandidates
{
    print $main_log_file_handle_ "\n\n RunSimulationOnCandidates\n\n";

    my @non_unique_results_filevec_ = ( );

    my @unique_sim_id_list_ = ( );
    my @independent_parallel_commands_ = ( );
    my @tradingdate_list_ = ( );
    my @temp_strategy_cat_file_list_ = ( );
    my @temp_strategy_output_file_list_ = ( );

    my $tradingdate_ = $trading_end_yyyymmdd_;
    my $max_days_at_a_time_ = 2000;
    for ( my $i = 0 ; $i < $max_days_at_a_time_ ; $i ++ )
    {
        if ( SkipWeirdDate ( $tradingdate_ ) ||
             IsDateHoliday ( $tradingdate_ ) ||
             IsProductHoliday ( $tradingdate_ , $first_shortcode_ ) ||
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
        my $temp_index_ = 0;

        for ( my $strategy_filevec_index_ = 0 ; $strategy_filevec_index_ <= $#strategy_filevec_ ; )
        {
            my $temp_strategy_cat_file_ = $work_dir_."/temp_strategy_cat_file_".$tradingdate_."_$temp_index_".".txt";
            my $temp_strategy_output_file_ = $work_dir_."/temp_strategy_output_file_".$tradingdate_."_$temp_index_".".txt";

             $temp_index_ ++;

            my $this_strategy_filename_ = $strategy_filevec_ [ $strategy_filevec_index_ ];

            my $unique_sim_id_ = GetGlobalUniqueId ( ); # Get a concurrency safe id.

            my $market_model_index_ = GetMarketModelForShortcode ( $first_shortcode_ );

            my $exec_cmd_ = $LIVE_BIN_DIR."/sim_strategy SIM ".$this_strategy_filename_." ".$unique_sim_id_." ".$tradingdate_." ".$market_model_index_." ADD_DBG_CODE -1 2>/dev/null  > ".$temp_strategy_output_file_;
            print $main_log_file_handle_ "$exec_cmd_\n";

            push ( @unique_sim_id_list_ , $unique_sim_id_ );
            push ( @independent_parallel_commands_ , $exec_cmd_ );

            push ( @tradingdate_list_ ,$tradingdate_ );

            push ( @temp_strategy_cat_file_list_ , $this_strategy_filename_ );
            push ( @temp_strategy_output_file_list_ , $temp_strategy_output_file_ );
            $strategy_filevec_index_ ++;
        }

        $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_ , 1 );
    }

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

        while ( ! AllOutputFilesPopulatedLocal ( @output_files_to_poll_this_run_ ) )
        { # there are still some sim-strats which haven't output SIMRESULT lines
            my $result_ = AllOutputFilesPopulatedLocal ( @output_files_to_poll_this_run_ );
            sleep ( 1 );
        }
    }

    for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ )
    {
        my $unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ];
        my $tradingdate_ = $tradingdate_list_ [ $command_index_ ];
        my $temp_strategy_cat_file_ = $temp_strategy_cat_file_list_ [ $command_index_ ];
        my $sid_exec_cmd_ = "cat $temp_strategy_cat_file_ | head -n1 | awk '{print \$7}'";
        my $unique_strat_id_ =  `$sid_exec_cmd_`;
        chomp($unique_strat_id_);
        my $temp_strategy_output_file_ = $temp_strategy_output_file_list_ [ $command_index_ ];
        my $exec_cmd_ = "cat ".$temp_strategy_output_file_list_ [ $command_index_ ];
        my $tradeinit_output_line_ = `$exec_cmd_`;
        my $pnlstats_output_line_ = "0 0 0 0 0 0 0 0 0 0 0";

        my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
        if ( ExistsWithSize ( $this_trades_filename_ ) )
        {
            my $exec_cmd_ = $MODELSCRIPTS_DIR."/get_pnl_stats_stir.pl $this_trades_filename_";
            print $main_log_file_handle_ $exec_cmd_."\n";
            $pnlstats_output_line_ = `$exec_cmd_`;
            chomp ( $pnlstats_output_line_ );
        }

        {
            my $this_tradeslogfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int ( $unique_sim_id_ );
            `rm -f $this_tradeslogfilename_`;
        }

        my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_.".txt";

        open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );
        {
            if ( $tradeinit_output_line_ =~ /SIMRESULT/ )
            { # SIMRESULT pnl volume                                                                                                                                                                                               
                my @rwords_ = split ( ' ', $tradeinit_output_line_ );
                splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc                                                                                     
                my $remaining_simresult_line_ = join ( ' ', @rwords_ );
                if ( ( $rwords_[1] > 0 ) || # volume > 0                                                                                                                                                           
                     ( ( $first_shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day                                                                      
                {
                    chomp ( $unique_strat_id_ );
                    printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $pnlstats_output_line_, $unique_strat_id_ ;
                    printf TRLF "%s %s %s\n",$remaining_simresult_line_,$pnlstats_output_line_, $unique_strat_id_;
                }
            }
        }
        close TRLF;
       my $temp_strategy_list_file_ = $temp_strategy_cat_file_."_list_file";
        `echo $temp_strategy_cat_file_ > $temp_strategy_list_file_`;

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

    printf $main_log_file_handle_ "#PARAM    CMEDPNL   CMEDVOL\n";
    for my $this_param_file_ ( @params_sorted_by_cmedpnl_ ) 
    {
	printf $main_log_file_handle_ "%s %d %d\n", $this_param_file_, $param_to_cmedpnl_{$this_param_file_}, $param_to_cmedvolume_{$this_param_file_};
    }
}


sub AllOutputFilesPopulatedLocal 
{
        my ( @output_files_to_poll_this_run_ ) = @_;
        for ( my $output_file_index_ = 0; $output_file_index_ <= $#output_files_to_poll_this_run_ ; $output_file_index_ ++ )
        {
                my $exec_cmd_ = "grep SIMRESULT ".$output_files_to_poll_this_run_ [ $output_file_index_ ]." 2>/dev/null | wc -l 2>/dev/null";
                my @exec_cmd_output_ = `$exec_cmd_`;

                chomp ( @exec_cmd_output_ );

                if ( $#exec_cmd_output_ >= 0 )
                {
                        my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ 0 ] );
                        chomp ( @exec_cmd_output_words_ );
                        if ( $#exec_cmd_output_words_ >= 0 )
                        {
                                my $simresult_count_ = $exec_cmd_output_words_[ 0 ];
                                if ( $simresult_count_ <= 0 )
                                {
                                        return 0;
                                        last;
                                }
                        }
                        else
                        {
                                return 0;
                                last;
                        }
                }
                else
                {
                        return 0;
                        last;
                }
        }
        return 1;
}
