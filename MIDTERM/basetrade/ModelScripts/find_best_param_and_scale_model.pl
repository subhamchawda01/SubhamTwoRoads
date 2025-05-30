#!/usr/bin/perl

# \file ModelScripts/find_best_param_and_scale_model.pl
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
# strategyfilename
# startdate 
# enddate
# outputdir
# [ model_scale_factor = 0.20 ]
# [ type_of_days = ALL / HV / BD / ECO ]
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub PerturbParamModelMakeStrategyFiles ; # takes model and paramfile and makes strategyfile
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database
sub SummarizeLocalResultsAndChoose ; # from the files created in the local_results_base_dir choose the best ones to send to pool

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $FBPSM_WORK_DIR=$SPARE_HOME."FBPSM/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDateForShortcode
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/exists_and_same.pl"; # ExistsAndSame
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; # FileNameInNewDir
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; # FindItemFromVecWithBase
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/get_strat_info.pl"; # GetStratInfo
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; # GetModelAndParamFileNames
require "$GENPERLLIB_DIR/perturb_params_from.pl"; # PerturbParamsFrom
require "$GENPERLLIB_DIR/perturb_model_from.pl"; # PerturbModelFrom
require "$GENPERLLIB_DIR/create_params_from_first_options.pl"; # CreateParamsFromFirstOptions

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MAX_STRAT_FILES_IN_ONE_SIM = 120; # please work on optimizing this value

my $SAVE_TRADELOG_FILE = 0;

# start 
my $USAGE="$0 strategyfilename trading_start_yyyymmdd trading_end_yyyymmdd outputdirectory [model_scale_factor=0.20] [type_of_days=ALL] [sort-algo=kCNASqDDAdjPnlSqrtVolume] ";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $given_strategy_filename_ = $ARGV[0];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[1] ) ;
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] ) ;
my $output_directory_ = $ARGV[3] ;
my $model_scale_factor_ = 0.20 ;
if ( $#ARGV >= 4 ) { $model_scale_factor_ = max ( 0.05, $ARGV[4] ) ; }
my $type_of_days_ = "ALL";
if ( $#ARGV >= 5 ) { $type_of_days_ = $ARGV[5] ; }
my $sort_algo_ = "kCNASqDDAdjPnlSqrtVolume";
if ( $#ARGV >= 6 ) { $sort_algo_ = $ARGV [ 6 ]; }

$given_strategy_filename_ = File::Spec->rel2abs($given_strategy_filename_);
my $given_strategy_dirname_ = dirname ( $given_strategy_filename_ ); chomp ( $given_strategy_dirname_ );

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $shortcode_ = "";
my $strategyname_ = "";
my $given_model_filename_ = "";
my $given_param_filename_ = "";
my $trading_start_hhmm_ = "";
my $trading_end_hhmm_ = "";
my $given_strategy_progid_ = 1001;

( $shortcode_, $strategyname_, $given_model_filename_, $given_param_filename_, $trading_start_hhmm_, $trading_end_hhmm_, $given_strategy_progid_ ) = GetStratInfo ( $given_strategy_filename_ ) ;

my $min_pnl_per_contract_to_allow_ = -0.10 ;
my $min_volume_to_allow_ = 100; 
my $max_ttc_to_allow_ = 120;


my $delete_intermediate_files_ = 1;

my @strategy_filevec_ = ();
my @intermediate_files_ = ();

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $FBPSM_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	print STDERR "Surprising but this dir exists\n";
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $FBPSM_WORK_DIR.$unique_gsm_id_; 
    }
    else
    {
	last;
    }
}
my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_params_dir_ = $work_dir_."/params_dir";
my $local_models_dir_ = $work_dir_."/models_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }
if ( ! ( -d $local_models_dir_ ) ) { `mkdir -p $local_models_dir_`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1); # just to output to log file promptly ... might help in debugging

#initial param
my $current_best_paramfilename_ = $local_params_dir_."/".basename($given_param_filename_);
CreateParamsFromFirstOptions ( $given_param_filename_, $current_best_paramfilename_ );
#initial model
my $current_best_modelfilename_ = $local_models_dir_."/".basename($given_model_filename_);
`cp $given_model_filename_ $current_best_modelfilename_`;

if ( ! ( ExistsWithSize ( $current_best_paramfilename_ ) && ExistsWithSize ( $current_best_modelfilename_ ) ) )
{
# there is no reason for this to happen
    printf "Error check param %d chec model %d\n", ExistsWithSize ( $current_best_paramfilename_ ), ExistsWithSize ( $current_best_modelfilename_ ) ;
    exit ( 0 );
}

my $last_stats_for_printing_ = "";
my $max_iterations_to_try_ = 10;
my $num_iterations_completed_ = 0 ;
my $improvement_made_in_this_iteration_ = 1 ;
while ( ( $improvement_made_in_this_iteration_ == 1 ) &&
	( $num_iterations_completed_ < $max_iterations_to_try_ ) )
  {
      # remove strategy_filevec_
      # remove unique_results_filevec_

    # From the given arguments create the strategy files for simulations
    PerturbParamModelMakeStrategyFiles ( );
    if ( $#strategy_filevec_ < 0 )
      {
	print $main_log_file_handle_ "Exiting due to empty strategy_filevec_\n";
      }
    else
      {
	# find results of the strategies and put them in local_results_base_dir
	RunSimulationOnCandidates ( );
	
	# among the candidates choose the best
	# set the file $current_best_modelfilename_ $current_best_paramfilename_ if there is any progress to be made, else keep them same
	$improvement_made_in_this_iteration_ = SummarizeLocalResultsAndChoose ( );
      }
    $num_iterations_completed_ ++;
  }

{
    my $output_strategy_filename_ = $output_directory_."/".basename($given_strategy_filename_);
    my $output_model_filename_ = $output_directory_."/".basename($given_model_filename_);
    my $output_param_filename_ = $output_directory_."/".basename($given_param_filename_);

    `cp $current_best_modelfilename_ $output_model_filename_`;
    `cp $current_best_paramfilename_ $output_param_filename_`;

    my $exec_cmd="$MODELSCRIPTS_DIR/create_strategy_file.pl $output_strategy_filename_ $shortcode_ $strategyname_ $output_model_filename_ $output_param_filename_ $trading_start_hhmm_ $trading_end_hhmm_ $given_strategy_progid_";
    print $main_log_file_handle_ "$exec_cmd\n";
    `$exec_cmd`;
    print "Created $output_strategy_filename_\n";
    if ( $last_stats_for_printing_ ) {
	print "$last_stats_for_printing_\n";
    }
}

# end script
$main_log_file_handle_->close;

exit ( 0 );

sub PerturbParamModelMakeStrategyFiles 
{
# For each paramfile that is generated as a perturbation
# For each modelfile that is generated as a perturbation
# generate a strategy file for that combination

    my $t_strategy_progid_ = 1001;
    @strategy_filevec_ = ();

    if ( ExistsWithSize ( $current_best_paramfilename_ ) && ExistsWithSize ( $current_best_modelfilename_ ) )
    { # paramfile, modelfile exist and aren't empty sized
	print $main_log_file_handle_ "PerturbParamsFrom ( $current_best_paramfilename_, $given_param_filename_, $local_params_dir_ )\n";
	my @this_param_filename_vec_ = PerturbParamsFrom ( $current_best_paramfilename_, $given_param_filename_, $local_params_dir_ );
	print $main_log_file_handle_ "PerturbModelFrom ( $current_best_modelfilename_, $model_scale_factor_, $local_models_dir_ )\n";
	my @this_model_filename_vec_ = PerturbModelFrom ( $current_best_modelfilename_, $model_scale_factor_, $local_models_dir_ );
	for ( my $tpfv_index_ = 0 ; $tpfv_index_ <= $#this_param_filename_vec_ ; $tpfv_index_ ++ )
	{
	    my $this_param_filename_ = $this_param_filename_vec_[$tpfv_index_];
	    if ( ExistsWithSize ( $this_param_filename_ ) )
	    {
		for ( my $tmfv_index_ = 0 ; $tmfv_index_ <= $#this_model_filename_vec_ ; $tmfv_index_ ++ )
		{
		    my $this_model_filename_ = $this_model_filename_vec_[$tmfv_index_];
		    if ( ExistsWithSize ( $this_model_filename_ ) )
		    {
			my $this_strategy_filename_ = FileNameInNewDir ( $given_strategy_filename_, $local_strats_dir_ );
			$this_strategy_filename_ = $this_strategy_filename_."_".$t_strategy_progid_ ;
			
			my $exec_cmd="$MODELSCRIPTS_DIR/create_strategy_file.pl $this_strategy_filename_ $shortcode_ $strategyname_ $this_model_filename_ $this_param_filename_ $trading_start_hhmm_ $trading_end_hhmm_ $t_strategy_progid_";
			print $main_log_file_handle_ "$exec_cmd\n";
			`$exec_cmd`;
			$t_strategy_progid_++; # uniqueness of progid ensures that we can run them in sim together

			if ( ExistsWithSize (  $this_strategy_filename_ ) )
			{
			    push ( @strategy_filevec_, $this_strategy_filename_ ); 
			}
		    }
		}
	    }
	}
    }
    else
    {
	print $main_log_file_handle_ "Not exists $current_best_paramfilename_ or $current_best_modelfilename_ !\n";
    }
}

# this function is a direct copy from find_best_param_files.pl
# here we are trying to create the result fiels for the strat files.
sub RunSimulationOnCandidates
{
# TODO : clean current files in local_results_base_dir

    # make a file with the full paths of all the strategyfiles

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
		if ( -e $temp_strategy_cat_file_ ) { `rm -f $temp_strategy_cat_file_`; }
		for ( my $t_strategy_filevec_index_ = $strategy_filevec_front_; $t_strategy_filevec_index_ <= $strategy_filevec_back_; $t_strategy_filevec_index_ ++ )
		{
		    my $this_strategy_filename_ = $strategy_filevec_[$t_strategy_filevec_index_];
		    print TSLF $this_strategy_filename_."\n";
		    `cat $this_strategy_filename_ >> $temp_strategy_cat_file_`;
		}
		close TSLF;
		
		my @sim_strategy_output_lines_=(); # stored to seive out the SIMRESULT lines
		my %unique_id_to_pnlstats_map_=(); # stored to write extended results to the database
		{
		    my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ );
		    my $exec_cmd="$LIVE_BIN_DIR/sim_strategy SIM $temp_strategy_cat_file_ $unique_gsm_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1"; 
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
    my $t_improvement_made_in_this_iteration_ = 0;
    if ( $#unique_results_filevec_ >= 0 )
    {
	# take the list of local_results_database files @resul
	my $min_num_files_to_choose_ = 1;

	my $historical_sort_algo_ = "kCNASqDDAdjPnlSqrtVolume" ;
	my $exec_cmd="$LIVE_BIN_DIR/summarize_local_results_dir_and_choose_by_algo $historical_sort_algo_ 1 $min_num_files_to_choose_ $min_pnl_per_contract_to_allow_ $min_volume_to_allow_ $max_ttc_to_allow_ 10000000 $local_results_base_dir";

	print $main_log_file_handle_ "$exec_cmd\n";
	my @slrac_output_lines_=`$exec_cmd`;
	print $main_log_file_handle_ @slrac_output_lines_;
	print @slrac_output_lines_;
	chomp ( @slrac_output_lines_ );

	# remove old results files since file names are repeated
	for ( my $i = 0 ; $i <= $#unique_results_filevec_ ; $i ++ ) {
	    `rm -f $unique_results_filevec_[$i]`;
	}
	@unique_results_filevec_ = ();

	my $trading_start_end_str_ = $trading_start_hhmm_."-".$trading_end_hhmm_ ;

	for ( my $t_slrac_output_lines_index_ = 0 ; $t_slrac_output_lines_index_ <= $#slrac_output_lines_ ; $t_slrac_output_lines_index_++ )
	{
	    my $this_line_ = $slrac_output_lines_[$t_slrac_output_lines_index_];
	    if ( $this_line_ =~ /STRATEGYFILEBASE/ )
	    { # STRATEGYFILEBASE basename of strategyfile

		# this ensures we onlylook at top file
		if ( $t_improvement_made_in_this_iteration_ == 1 ) {
		    last;
		}

		my @strat_line_words_ = split ( ' ', $this_line_ );
		if ( $#strat_line_words_ >= 1 )
		{ # STRATEGYFILEBASE w_strategy_ilist_ZB_EU_PBT_30_na_e3_20110629_20110729_CET_805_EST_800_fsg.5_FSRR.0.5.0.01.0.0.0.85.tt_CET_805_EST_800.pfi_0
		    my $t_best_strategy_filename_ = FindItemFromVecWithBase ( $strat_line_words_[1], @strategy_filevec_  ) ;
		    
		    if ( ExistsWithSize ( $t_best_strategy_filename_ ) )
		    {
			my ( $t_shortcode_, $t_strategyname_, $t_modelfilename_, $t_paramfilename_, $t_trading_start_hhmm_, $t_trading_end_hhmm_, $t_strategy_progid_ ) = GetStratInfo ( $t_best_strategy_filename_ ) ;
			if ( ExistsAndSame ( $t_paramfilename_, $current_best_paramfilename_ ) != 1 )
			{ # differen t file with same name ... so improvement made
			    $t_improvement_made_in_this_iteration_ = 1;
			    `cp $t_paramfilename_ $current_best_paramfilename_`;
			}
			if ( ExistsAndSame ( $t_modelfilename_, $current_best_modelfilename_ ) != 1 )
			{ # differen t file with same name ... so improvement made
			    $t_improvement_made_in_this_iteration_ = 1;
			    `cp $t_modelfilename_ $current_best_modelfilename_`;
			}
		    }
		}
	    }
	    if ( $this_line_ =~ /STATISTICS/ ) {
		$last_stats_for_printing_ = $this_line_;

		# exit if stats recorded
		if ( $t_improvement_made_in_this_iteration_ == 1 ) {
		    last;
		}
	    }
	}
    }
    return $t_improvement_made_in_this_iteration_ ;
}
