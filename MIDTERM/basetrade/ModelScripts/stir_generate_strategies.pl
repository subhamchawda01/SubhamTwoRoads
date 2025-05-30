#!/usr/bin/perl

# \file ModelScripts/stir_generate_strategies.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

#format of the input :
#line 1:components of the structure separated by space
#line i : shc_ strategy_

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
my $GSW_WORK_DIR=$SPARE_HOME."GSW/";

my $REPO="basetrade";
my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STIR_STRATS_DIR=$MODELING_BASE_DIR."/stir_strats";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "ankit" || $USER eq "anshul")
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

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
require "$GENPERLLIB_DIR/get_unique_sim_id_from_stir_cat_file.pl"; # GetUniqueSimIdFromStirCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/list_file_to_vec.pl"; #ListFileToVec
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
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
my $USAGE="$0 shortcode pool_of_strats start_date end_date StartegyName start_time end_time commonparamfile [min_volume] [min_sharpe_] [min_pnl_per_vol_] [sort_algo] [number_of_strats_to_install]";

if ( $#ARGV < 7 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $orig_pool_filename_ = $ARGV[1];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[3] );
my $strategy_name_ = $ARGV[4] ;
my $trading_start_hhmm = $ARGV[5];
my $trading_end_hhmm_ = $ARGV[6];
my $common_param_filename_= $ARGV[7];

my $min_volume_ = -1;
my $min_sharpe_ = -1;
my $min_pnl_per_vol_ = -1;
my $sort_algo_ = "kCNAPnlAverage";
my $number_of_strats_to_install_ = 1;

if ( $#ARGV >= 12 )
{
    $min_volume_ = $ARGV [ 8 ];
    $min_sharpe_ = $ARGV [ 9 ];
    $min_pnl_per_vol_ = $ARGV [ 10 ];
    $sort_algo_ = $ARGV [ 11 ];
    $number_of_strats_to_install_ = $ARGV [ 12 ];
}

my $pool_file_list_basename_ = basename ( $orig_pool_filename_ );
$GSW_WORK_DIR = $GSW_WORK_DIR.$shortcode_."/".$pool_file_list_basename_."/";

my $delete_intermediate_files_ = 1;

my $num_files_to_choose_ = 1;
my $min_pnl_per_contract_to_allow_ = -1.10 ;
my $min_volume_to_allow_ = 100; 
my $max_ttc_to_allow_ = 120;

my @model_filevec_ = ();
my @strategy_filevec_ = ();

my @intermediate_files_ = ();

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $GSW_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	print STDERR "Surprising but this dir exists\n";
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $GSW_WORK_DIR.$unique_gsm_id_; 
    }
    else
    {
	last;
    }
}

my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $temp_strat_filename_ = $local_strats_dir_."/temp_strat_file";
my $full_strategy_filename_ = "";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

my %structure_strats_ = ();
my %structure_models_ = ();
my %structure_params_ = ();
my %idx_to_shc_map_ = ();
my $unique_shc_ = 0;
my @offset_array = ();
my @permutation_index_array = ();
# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir ) ) { `mkdir -p $local_results_base_dir`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
if ( ! ( -d $MODELING_STRATS_DIR ) ) {`mkdir -p $MODELING_STRATS_DIR`;}
if ( ! ( -d $MODELING_STIR_STRATS_DIR ) ) {`mkdir -p $MODELING_STIR_STRATS_DIR`;}

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

# Get param files and model files
ProcessPoolFile ( );
MakeStrategyFiles ( );

# Run simulations on the created strategy files
RunSimulationOnCandidates ( );

# among the candidates choose the best
SummarizeLocalResultsAndChoose();
# end script
$main_log_file_handle_->close;

exit ( 0 );

sub ProcessPoolFile
{
	open my $main_pool_file_handle_, "<", $orig_pool_filename_ or PrintStacktraceAndDie ( "Could not open $orig_pool_filename_ for reading\n" );
 	my $line_number_ = 0;
	while(my $line = <$main_pool_file_handle_>)
	{
		chomp($line);
		if($line_number_ == 0)
		{
			my @shc_ = split(/ /,$line);
			my $idx_ = 0;
			foreach(@shc_)
			{
				$structure_strats_{$_} = [];
				$structure_models_{$_} = [];
				$structure_params_{$_} = [];
				$idx_to_shc_map_{$idx_} = $_;
				$idx_++;
			}
			$unique_shc_ = $idx_;
		}
		else
		{
			my @split_line_ = split(/ /,$line);
			if($split_line_[0] eq "MODEL")
			{
				push(@{$structure_models_{$split_line_[1]}},$split_line_[2]);
			}
			elsif($split_line_[0] eq "PARAM")
			{
				push(@{$structure_params_{$split_line_[1]}},$split_line_[2]);
			}
		}
		++$line_number_;
	}
	for(my $idx_ = 0;$idx_ < $unique_shc_;$idx_++)
	{
		my @models_ = @{$structure_models_{$idx_to_shc_map_{$idx_}}};
		my @params_ = @{$structure_params_{$idx_to_shc_map_{$idx_}}};
		foreach(@models_)
		{
			my $modelname_ = $_;
			foreach(@params_)
			{
				my $paramname_ = $_;
				my $temp_strat_ = $modelname_." ".$paramname_;
				push(@{$structure_strats_{$idx_to_shc_map_{$idx_}}},$temp_strat_);
			}
		}
	}			
	push(@offset_array,1);
	for(my $idx_ = 1;$idx_ < $unique_shc_ ;$idx_++)
	{
		push(@offset_array,scalar @{$structure_strats_{$idx_to_shc_map_{$idx_-1}}} * $offset_array[$idx_-1]);
	}
	for(my $idx_ = 0;$idx_ < $offset_array[$unique_shc_ - 1] * scalar @{$structure_strats_{$idx_to_shc_map_{$unique_shc_-1}}} ; $idx_++)
	{
		my @index_array=();
		my $tmp_idx_ = $idx_;
		for(my $shc_idx_ = $unique_shc_ - 1;$shc_idx_ >=0 ;$shc_idx_--)
		{
			use integer;
			push(@index_array,$tmp_idx_/$offset_array[$shc_idx_]);
			$tmp_idx_ -= ($tmp_idx_ /$offset_array[$shc_idx_]) * $offset_array[$shc_idx_];
		}
		@index_array = reverse @index_array;
		for(my $idx_in_ = 0;$idx_in_ < $unique_shc_ ; $idx_in_++)
		{
			$permutation_index_array[$idx_][$idx_in_] = $index_array[$idx_in_];
		}
	}				
}

sub MakeStrategyFiles 
{
#    print $main_log_file_handle_ "\nMakeStrategyFiles\n\n";

    my $strategy_progid_ = 1001;

    for (my $idx_ = 0; $idx_ < $offset_array[$unique_shc_ - 1] * scalar @{$structure_strats_{$idx_to_shc_map_{$unique_shc_-1}}} ; $idx_++) 
    { 
		
	    my $this_strategy_im_filebase_ = "strat_im_".$strategy_progid_ ;
	    my $this_strategy_im_filename_ = $local_strats_dir_."/".$this_strategy_im_filebase_;
	
	    my $this_strategy_filebase_ = "strat_".$strategy_progid_;
	    my $this_strategy_filename_ = $local_strats_dir_."/".$this_strategy_filebase_;
		    
	    open my $im_file_handle_, ">", $this_strategy_im_filename_ or PrintStacktraceAndDie ( "Could not open $this_strategy_im_filename_ for writing\n" );
	    print $im_file_handle_ "STRUCTURED_TRADING ".$shortcode_." ".$strategy_name_." ".$common_param_filename_." ".$trading_start_hhmm." ".$trading_end_hhmm_." ".$strategy_progid_."\n";
	    for(my $shc_idx_ = 0;$shc_idx_ < $unique_shc_ ; $shc_idx_ ++)
	    {
		print $im_file_handle_ "STRATEGYLINE ".$idx_to_shc_map_{$shc_idx_}." ".$structure_strats_{$idx_to_shc_map_{$shc_idx_}}[$permutation_index_array[$idx_][$shc_idx_]]."\n";;
	    }
	    close($im_file_handle_);
	    my $exec_cmd_ = "echo STRUCTURED_STRATEGYLINE $this_strategy_im_filename_ $strategy_progid_ > $this_strategy_filename_";
	    `$exec_cmd_`;
	    if ( ExistsWithSize (  $this_strategy_filename_ ) )
	    {
		push ( @strategy_filevec_, $this_strategy_filename_ ); 
	    }
	    ++$strategy_progid_;
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
            my $exec_cmd_ = $LIVE_BIN_DIR."/sim_strategy SIM ".$temp_strategy_cat_file_." ".$unique_sim_id_." ".$tradingdate_." ".$market_model_index_." ADD_DBG_CODE -1 > ".$temp_strategy_output_file_." 2>/dev/null";

	    print $main_log_file_handle_ "$exec_cmd_\n";

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

	while ( ! AllOutputFilesPopulatedLocal ( @output_files_to_poll_this_run_ ) )
	{ # there are still some sim-strats which haven't output SIMRESULT lines
	    my $result_ = AllOutputFilesPopulatedLocal ( @output_files_to_poll_this_run_ );	
	    sleep ( 1 );
	}
    }

    for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ )
    {
        my %unique_id_to_pnlstats_map_ = ( );
	
	my $unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ];
	my $tradingdate_ = $tradingdate_list_ [ $command_index_ ];
	
	my $temp_strategy_cat_file_ = $temp_strategy_cat_file_list_ [ $command_index_ ];
	my $exec_cmd_ = "cat ".$temp_strategy_cat_file_;
	my $strategy_cat_file_lines_ = `$exec_cmd_`; chomp($strategy_cat_file_lines_);
	my @strategy_cat_file_line_vec_ = split('\n', $strategy_cat_file_lines_);


	my $temp_strategy_output_file_ = $temp_strategy_output_file_list_ [ $command_index_ ];
	$exec_cmd_ = "cat ".$temp_strategy_output_file_;
	my $tradeinit_output_lines_ = `$exec_cmd_`; chomp($tradeinit_output_lines_);
	my @tradeinit_output_line_vec_ = split('\n', $tradeinit_output_lines_);

	my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
	my @pnlstats_output_line_vec_ = ();
	if ( ExistsWithSize ( $this_trades_filename_ ) )
	{
	    my $exec_cmd_ = $MODELSCRIPTS_DIR."/get_pnl_stats_stir_2.pl $this_trades_filename_";
	    print $main_log_file_handle_ $exec_cmd_."\n";
	    my $pnlstats_output_lines_ = `$exec_cmd_`; chomp ( $pnlstats_output_lines_ );
	    @pnlstats_output_line_vec_ = split('\n', $pnlstats_output_lines_);
	}

	{
	    my $this_tradeslogfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int ( $unique_sim_id_ );
	    `rm -f $this_tradeslogfilename_`;
	}

	#to check if pnlstats_output_line_vec_, tradeinit_output_line_vec_, strategy_cat_file_line_vec_ have same size
	if ( ($#tradeinit_output_line_vec_ != $#pnlstats_output_line_vec_) || ($#pnlstats_output_line_vec_ != $#strategy_cat_file_line_vec_) )
	{
		print $main_log_file_handle_ "somehow pnlstats_output_line_vec_, tradeinit_output_line_vec_, strategy_cat_file_line_vec_ dont't have same size, check files $temp_strategy_output_file_, $temp_strategy_cat_file_, $this_trades_filename_\n";
		next;
	}


	my $temp_results_list_file_ = `echo $temp_strategy_output_file_ | sed 's\/temp_strategy_output_file_\/temp_results_list_file_\/g'`; chomp($temp_results_list_file_);
	open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );

	for (my $t_strat_idx_ = 0 ; $t_strat_idx_ <= $#tradeinit_output_line_vec_ ; $t_strat_idx_++ )
	{
		my $strategy_cat_file_line_ = $strategy_cat_file_line_vec_[$t_strat_idx_]; chomp($strategy_cat_file_line_);
		my @strategy_cat_file_line_words_ = split(' ', $strategy_cat_file_line_);
		my $unique_strat_id_ = 	$strategy_cat_file_line_words_[2];

		my $pnlstats_output_line_ = $pnlstats_output_line_vec_[$t_strat_idx_]; chomp($pnlstats_output_line_);
		my $tradeinit_output_line_ = $tradeinit_output_line_vec_[$t_strat_idx_]; chomp($tradeinit_output_line_);
		
		{
		    if ( $tradeinit_output_line_ =~ /SIMRESULT/ )
		    { # SIMRESULT pnl volume S B A I                                                                                                                                                                                               
			my @rwords_ = split ( ' ', $tradeinit_output_line_ );
			splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc                                                                                     
			my $remaining_simresult_line_ = join ( ' ', @rwords_ );
			if ( ( $rwords_[1] > 0 ) || # volume > 0                                                                                                                                                  
			     ( ( $shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day                                                                      
			{
			    printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $pnlstats_output_line_, $unique_strat_id_ ;
			    printf TRLF "%s %s %s\n",$remaining_simresult_line_,$pnlstats_output_line_, $unique_strat_id_;
			}
		    }
		}
	}

	close TRLF;		

	if ( ExistsWithSize ( $temp_results_list_file_ ) )
	{
	    my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database.pl $temp_strategy_cat_file_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir";
	    print $main_log_file_handle_ "$exec_cmd\n";
	    my $this_local_results_database_file_ = `$exec_cmd`;
	    push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
	}
    }

    @unique_results_filevec_ = GetUniqueList ( @non_unique_results_filevec_ );
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

sub SummarizeLocalResultsAndChoose
{
	my $exec_cmd_ =  $LIVE_BIN_DIR."/summarize_local_results_dir_and_choose_by_algo ".$sort_algo_." 5000 5000 -1.85 10000 5000 -1 ".$local_results_base_dir." 2>/dev/null | grep \"STRAT\\|STAT\" ";
	print $main_log_file_handle_ $exec_cmd_."\n";
	my $rawresults = `$exec_cmd_`;
	chomp($rawresults);
	my @results=split(/\n/,$rawresults);
	if ( ! ( -d $MODELING_STIR_STRATS_DIR."/".$shortcode_) ) {`mkdir -p $MODELING_STIR_STRATS_DIR."/".$shortcode_`;}
	if ( ! ( -d $MODELING_STRATS_DIR."/".$shortcode_) ) {`mkdir -p $MODELING_STRATS_DIR."/".$shortcode_`;}
	for(my $idx_ = 1;$idx_ <= $number_of_strats_to_install_;$idx_++)
	{
		my @first_line_ = split(/ /,$results[2*($idx_-1)]);
		my $strat_filename_ = $first_line_[1];
		my @strat_name_vec_ =  split(/_/,$strat_filename_);
		my $strat_id_ = $strat_name_vec_[-1];
		my @second_line_ = split(/ /,$results[2*$idx_-1]);
		$exec_cmd_ = "date +%s";
		my $timestamp_ = `$exec_cmd_`;
		chomp($timestamp_);
		#is the criteria satisifed?
		if($second_line_[1]/$second_line_[3] > $min_pnl_per_vol_ && $second_line_[3] > $min_volume_ && $second_line_[4] > $min_sharpe_)
		{ 
			my $exec_cmd_ = "cp ".$local_strats_dir_."/".$strat_filename_." ".$MODELING_STRATS_DIR."/".$shortcode_."/w_strat_".$shortcode_."_".$trading_start_hhmm."_".$trading_end_hhmm_."_".$strat_id_."_".$timestamp_;
			`$exec_cmd_`;
			print $main_log_file_handle_ $exec_cmd_."\n";
			my $stir_strat_filename_ = $MODELING_STIR_STRATS_DIR."/".$shortcode_."/"."w_stir_strat_".$shortcode_."_".$trading_start_hhmm."_".$trading_end_hhmm_."_".$strat_id_."_".$timestamp_;
			$strat_filename_ = $MODELING_STRATS_DIR."/".$shortcode_."/w_strat_".$shortcode_."_".$trading_start_hhmm."_".$trading_end_hhmm_."_".$strat_id_."_".$timestamp_;

	    		$exec_cmd_ = "echo STRUCTURED_STRATEGYLINE $strat_filename_ $strat_id_ > $stir_strat_filename_";
			`$exec_cmd_`;
			print $main_log_file_handle_ $exec_cmd_."\n";
		}
		else
		{
			print $main_log_file_handle_ $strat_id_." failed to clear cutoff\n";
		}
	}
}


