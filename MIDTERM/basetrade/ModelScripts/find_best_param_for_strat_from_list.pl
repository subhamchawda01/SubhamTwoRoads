#!/usr/bin/perl

# \file ModelScripts/find_best_param_for_strat_from_list.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
#
# SHORTCODE
# TIMEPERIOD
# STRATEGYBASENAME
# PARAMLIST
# TRADING_START_YYYYMMDD 
# TRADING_END_YYYYMMDD

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

package ResultLine;
use Class::Struct;

# declare the struct
struct ( 'ResultLine', { pnl_ => '$', volume_ => '$', ttc_ => '$' } );

package main;

sub MakeStrategyFiles ; # takes input_stratfile and makes strategyfiles corresponding to differnet scale constants
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database
sub SummarizeLocalResultsAndChoose ; # from the files created in the local_results_base_dir choose the best ones to send to pool

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $FBPA_WORK_DIR=$SPARE_HOME."FBPSL/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

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
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/list_file_to_vec.pl"; #ListFileToVec
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_start_end_hhmm.pl"; # GetStratStartEndHHMM
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_match_strat_excluding_sets.pl"; # MakeStratVecFromDirInTpMatchStratExcludingSets
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_match_strat_excluding_sets.pl"; # MakeStratVecFromDirInTpMatchStratExcludingSets
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MAX_STRAT_FILES_IN_ONE_SIM = 120; # please work on optimizing this value

# start
my $USAGE="$0 shortcode timeperiod strategybasename strategyname param_file_list start_date end_date";

if ( $#ARGV < 6 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $timeperiod_ = $ARGV[1];
my $strategy_basename_ = $ARGV[2];
my $strategyname_ = $ARGV[3];
my $param_file_list_ = $ARGV[4];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[5] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[6] );

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

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir ) ) { `mkdir -p $local_results_base_dir`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }

print "$0 $shortcode_ $timeperiod_ $strategy_basename_ $strategyname_ $param_file_list_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ \n\t $local_results_base_dir\n";

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

print $main_log_file_handle_ "$0 $shortcode_ $timeperiod_ $strategy_basename_ $strategyname_ $param_file_list_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ \n\t $local_results_base_dir\n";

# Get param files and model files
ProcessListArgs ( );

# From the given arguments create the strategy files for simulations
MakeStrategyFiles ( );

# Run simulations on the created strategy files
RunSimulationOnCandidates ( );

# among the candidates choose the best
SummarizeLocalResultsAndChoose ( );

# end script
$main_log_file_handle_->close;

exit ( 0 );

sub ProcessListArgs
{
    #models
    my @exclude_tp_dirs_ = ();
    my @all_strats_in_dir_ = MakeStratVecFromDirInTpMatchStratExcludingSets ( $MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_, $strategyname_, @exclude_tp_dirs_ );

    my @filtered_strats_in_dir_ = ( );
    foreach my $t_strat_in_dir_ ( @all_strats_in_dir_ )
    {
	if ( index ( $t_strat_in_dir_ , $strategy_basename_ ) >= 0 )
	{
	    push ( @filtered_strats_in_dir_ , $t_strat_in_dir_ );
	    last;
	}
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

#params
    @param_filevec_ = MakeFilenameVecFromList ( $param_file_list_ );

}

# For each param file & for each model file, we create a strategy file
sub MakeStrategyFiles 
{

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
#		    print $main_log_file_handle_ "$exec_cmd\n";
		    `$exec_cmd`;
		    $strategy_progid_++; # uniqueness of progid ensures that we can run them in sim together
		    
		    if ( ExistsWithSize (  $this_strategy_filename_ ) )
		    {
			push ( @strategy_filevec_, $this_strategy_filename_ ); 
			$strat2param_ { $this_strategy_filebase_ } = $this_param_filename_ ;
		    }
		}
	    }
	}
    }
}

sub RunSimulationOnCandidates
{
    # make a file with the full paths of all the strategyfiles
    my @non_unique_results_filevec_=();
    my $tradingdate_ = $trading_end_yyyymmdd_;
    my $max_days_at_a_time_ = 2000;
    for ( my $j = 0 ; $j < $max_days_at_a_time_ ; $j ++ ) 
    {
	if ( SkipWeirdDate ( $tradingdate_ ) ||
	     IsDateHoliday ( $tradingdate_ ) ||
	     IsProductHoliday ( $tradingdate_ , $shortcode_ ) ||
	     NoDataDate ( $tradingdate_ ) )
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
		    my $this_strategy_filename_ = $strategy_filevec_[$t_strategy_filevec_index_];
		    print TSLF $this_strategy_filename_."\n";
		    `cat $this_strategy_filename_ >> $temp_strategy_cat_file_`;
		}
#		close TSCF;
		close TSLF;
		
		my @sim_strategy_output_lines_=(); # stored to seive out the SIMRESULT lines
		my %unique_id_to_pnlstats_map_=(); # stored to write extended results to the database
		{
		    my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ );
		    my $exec_cmd="$LIVE_BIN_DIR/sim_strategy SIM $temp_strategy_cat_file_ $unique_gsm_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1"; # for dvctrader using optimistic market_model_index, added nologs argument
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
		    { `rm -f $this_tradesfilename_`; }
		}


		my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;

		open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );
		for ( my $t_sim_strategy_output_lines_index_ = 0, my $psindex_ = 0; $t_sim_strategy_output_lines_index_ <= $#sim_strategy_output_lines_; $t_sim_strategy_output_lines_index_ ++ )
		{
		    if ( $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] =~ /SIMRESULT/ )
		    { # SIMRESULT pnl volume
			my @rwords_ = split ( ' ', $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] );
			splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc
			my $remaining_simresult_line_ = join ( ' ', @rwords_ );
			if ( $rwords_[1] > 0 )
			{
			    my $unique_sim_id_ = GetUniqueSimIdFromCatFile ( $temp_strategy_cat_file_, $psindex_ );
			    if ( exists $unique_id_to_pnlstats_map_{$unique_sim_id_} )
			    {
				printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_ ;
				printf TRLF "%s %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_;
			    }
			    else
			    {
				PrintStacktraceAndDie ( "unique_id_to_pnlstats_map_ missing $unique_sim_id_\n" );
			    }
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

sub SummarizeLocalResultsAndChoose
{
    for ( my $i = 0 ; $i <= $#unique_results_filevec_; $i++ )
    {
	my $t_results_filename_ = $unique_results_filevec_[$i];
	open RESULTSFILEHANDLE, "< $t_results_filename_ " or PrintStacktraceAndDie ( "$0 Could not open $t_results_filename_\n" );
	while ( my $result_line_ = <RESULTSFILEHANDLE> )
	{
	    my @result_line_words_ = split ( ' ', $result_line_ );
	    if ( $#result_line_words_ >= 9 )
	    { # dependant on format in results
		# 0 is stratfilename
		# 2 is pnl
		# 3 is volume
		# 9 is avg_ttc
		my $strategy_filebase_ = $result_line_words_[0];
		my $pnl_ = $result_line_words_[2];
		my $volume_ = $result_line_words_[3];
		my $ttc_ = $result_line_words_[9];

		if ( exists $strat2param_{$strategy_filebase_} )
		{
		    my $this_param_file_ = $strat2param_{$strategy_filebase_} ;

		    my $new_result_line_ = new ResultLine;
		    $new_result_line_->pnl_ ( $pnl_ );
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

    foreach my $this_param_file_ ( keys %param_to_resultvec_ )
    {
	my @pnl_vec_ = ();
	
	my $scalar_ref_resultvec_ = $param_to_resultvec_{$this_param_file_};
	for ( my $resultvec_index_ = 0 ; $resultvec_index_ <= $#$scalar_ref_resultvec_ ; $resultvec_index_ ++ )
	{ # foreach result vec item
	    my $this_result_line_ = $$scalar_ref_resultvec_[$resultvec_index_];
	    push ( @pnl_vec_, $this_result_line_->pnl_() ) ;
	}

	my $cmed_pnl_ = GetConsMedianAndSort ( \@pnl_vec_ );

	$param_to_cmedpnl_{$this_param_file_} = $cmed_pnl_;
    }

    my @params_sorted_by_cmedpnl_ = sort { $param_to_cmedpnl_{$b} <=> $param_to_cmedpnl_{$a} } keys %param_to_cmedpnl_;

    printf "#PARAM    CMEDPNL\n";
    for my $this_param_file_ ( @params_sorted_by_cmedpnl_ ) 
    {
	printf "%s %d\n", $this_param_file_, $param_to_cmedpnl_{$this_param_file_};
    }
    
}
