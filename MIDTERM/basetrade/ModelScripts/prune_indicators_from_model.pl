#!/usr/bin/perl

# \file ModelScripts/prune_indicators_from_model.pl
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

sub CreateStrategyFileList;

sub RunSimulationsOnStrategyFiles;

sub AddResultsToLocalDatabase;

sub GetListOfDatesFromResultFile;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";

my $PIM_WORK_DIR = $SPARE_HOME."PIM/";

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams

require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/find_number_from_vec.pl"; # FindNumberFromVec
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList

# start
my $USAGE="$0 STRAT_FILE_LIST START_DATE END_DATE";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }

my $strat_file_list_ = $ARGV [ 0 ];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 1 ] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 2 ] );

if ( ! ( -d $PIM_WORK_DIR ) ) { `mkdir -p $PIM_WORK_DIR`; }

my $lock_file_ = $PIM_WORK_DIR."p_i_f_m.lock";

if ( ! -e $lock_file_ )
{
    `touch $lock_file_`;
}
else
{
    print "$lock_file_ present. Another instance might be running , kill it and remove $lock_file_\n";
    exit ( 0 );
}

my $delete_intermediate_files_ = 1;

my @intermediate_files_ = ( );

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $PIM_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	print STDERR "Surprising but this dir exists\n";
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $PIM_WORK_DIR.$unique_gsm_id_; 
    }
    else
    {
	last;
    }
}

my $local_results_base_dir_ = $work_dir_."/local_results_base_dir";
my $temp_results_base_dir_ = $work_dir_."/temp_results_base_dir";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;

if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir_ ) ) { `mkdir -p $local_results_base_dir_`; }
if ( ! ( -d $temp_results_base_dir_ ) ) { `mkdir -p $temp_results_base_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush ( 1 );

print "Log file = ".$main_log_file_."\n";

my @strategy_file_list_ = ( );
CreateStrategyFileList ( );

my %strat_name_to_results_file_ = ( );
RunSimulationsOnStrategyFiles ( );

AddResultsToLocalDatabase ( );

CleanIntermediateFiles ( );

$main_log_file_handle_->close;

if ( -e $lock_file_ )
{
    `rm -f $lock_file_`;
}

exit ( 0 );

sub CreateStrategyFileList
{
    print $main_log_file_handle_ "# CreateStrategyFileList\n";

    open ( STRATEGY_LIST_FILE , "<" , $strat_file_list_ ) or PrintStacktraceAndDie ( "Could not open file $strat_file_list_" );

    my @t_strategy_file_list_ = <STRATEGY_LIST_FILE>; chomp ( @t_strategy_file_list_ );

    @strategy_file_list_ = GetUniqueList ( @t_strategy_file_list_ );

    close ( STRATEGY_LIST_FILE );

    return;
}

sub RunSimulationsOnStrategyFiles
{
    print $main_log_file_handle_ "# RunSimulationsOnStrategyFiles\n";

    my $IND_PERFORMANCE_SCRIPT = $SCRIPTS_DIR."/find_ind_performance.py";

    foreach my $t_strat_file_ ( @strategy_file_list_ )
    {
	print $main_log_file_handle_ "------> Running $t_strat_file_\n";

	my $exec_cmd_ = $IND_PERFORMANCE_SCRIPT." $t_strat_file_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_";

	print $main_log_file_handle_ $exec_cmd_."\n";

	my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

	foreach my $exec_output_line_ ( @exec_output_ )
	{
	    if ( index ( $exec_output_line_ , "Detailed results are in" ) >= 0 )
	    {
		my @exec_line_words_ = split ( ' ' , $exec_output_line_ );

		my $results_file_ = $exec_line_words_ [ $#exec_line_words_ ];
		my $strat_name_ = basename ( $results_file_ );

		$strat_name_to_results_file_ { $strat_name_ } = $results_file_;
	    }
	}

	print $main_log_file_handle_ join ( "\n" , @exec_output_ )."\n";
    }
}

sub AddResultsToLocalDatabase
{
    print $main_log_file_handle_ "# AddResultsToLocalDatabase\n";    

    foreach my $strat_name_ ( keys %strat_name_to_results_file_ )
    {
	my $t_results_file_ = $strat_name_to_results_file_ { $strat_name_ };

	my @dates_list_ = GetListOfDatesFromResultFile ( $t_results_file_ );

	my %date_to_model_to_results_ = ( );

	{
	    open ( RESULTS_FILE , "<" , $t_results_file_ ) or PrintStacktraceAndDie ( "Could not open file $t_results_file_" );
	    my @t_results_lines_ = <RESULTS_FILE>; chomp ( @t_results_lines_ );
	    close ( RESULTS_FILE );

	    my $num_indicators_ = -1;
	    for ( my $i = 1 ; $i <= $#t_results_lines_ ; $i ++ )
	    {
		my @t_result_words_ = split ( ' ' , $t_results_lines_ [ $i ] );

		if ( $#t_result_words_ > 0 )
		{
		    if ( index ( $t_results_lines_ [ $i ] , "#" ) >= 0 )
		    {
			$num_indicators_ ++;
			next;
		    }

		    if ( $#t_result_words_ >= 15 )
		    { # 20120808 2.4 27 125 -6166.00 6.00 -3.00 100.00 -0.03 0.53 -592 271 590 3220 30 69 0
			my $t_model_name_ = "I_".$num_indicators_;
			$date_to_model_to_results_ { $t_result_words_ [ 0 ] } { $t_model_name_ } = $t_results_lines_ [ $i ];
			print $main_log_file_handle_ " date_to_model_to_results_ ".$t_result_words_ [ 0 ]." $t_model_name_ ".$t_results_lines_ [ $i ]." \n";
		    }
		}
	    }
	}
    }

    # foreach my $strat_file_ ( keys %strat_file_to_simreal_results_ )
    # {
    # 	my $t_date_ = GetDateFromStratFile ( $strat_file_ );
    # 	my $t_mkt_model_ = GetMarketModelFromStratFileName ( $strat_file_ );

    # 	push ( @ { $date_to_mkt_model_to_simreal_results_ { $t_date_ } { $t_mkt_model_ } } , $strat_file_to_simreal_results_ { $strat_file_ } );
    # }

    # foreach my $date_ ( keys %date_to_mkt_model_to_simreal_results_ )
    # {
    # 	my $mkt_model_list_file_name_ = $temp_results_base_dir_."/".$date_."_mkt_model_list.txt";
    # 	my $results_list_file_name_ = $temp_results_base_dir_."/".$date_."_results_list.txt";

    # 	open ( MKT_MODEL_LIST_FILE , ">" , $mkt_model_list_file_name_ ) or PrintStacktraceAndDie ( "Could not create file : $mkt_model_list_file_name_" );
    # 	open ( RESULTS_LIST_FILE , ">" , $results_list_file_name_ ) or PrintStacktraceAndDie ( "Could not create file : $results_list_file_name_" );

    # 	foreach my $mkt_model_ ( keys % { $date_to_mkt_model_to_simreal_results_ { $date_ } } )
    # 	{
    # 	    my $combined_results_ = CombineSimrealResults ( @ { $date_to_mkt_model_to_simreal_results_ { $date_ } { $mkt_model_ } } );

    # 	    my $t_strat_file_name_ = $temp_results_base_dir_."/strat_mmi".$mkt_model_;

    # 	    if ( ! ExistsWithSize ( $t_strat_file_name_ ) )
    # 	    {
    # 		open ( STRAT_FILE , ">" , $t_strat_file_name_ ) or PrintStacktraceAndDie ( "Could not create file $t_strat_file_name_" );

    # 		print STRAT_FILE basename ( $t_strat_file_name_ )." ".$mkt_model_."\n";

    # 		close ( STRAT_FILE );
    # 	    }
	    
    # 	    print MKT_MODEL_LIST_FILE $t_strat_file_name_."\n";
    # 	    print RESULTS_LIST_FILE $combined_results_." ".$mkt_model_."\n";
    # 	}

    # 	close ( MKT_MODEL_LIST_FILE );
    # 	close ( RESULTS_LIST_FILE );

    # 	my $ADD_RESULTS_SCRIPT = "$MODELSCRIPTS_DIR/add_results_to_local_database.pl";
    # 	my $exec_cmd_ = "$ADD_RESULTS_SCRIPT $mkt_model_list_file_name_ $results_list_file_name_ $date_ $local_results_base_dir_";

    # 	print $main_log_file_handle_ "# AddResultsToLocalDatabase : $exec_cmd_\n";
    # 	`$exec_cmd_`;
    # }

    return;
}

sub CleanIntermediateFiles
{
    print $main_log_file_handle_ "# CleanIntermediateFiles\n";
    if ( $delete_intermediate_files_ )
    {
	chomp ( @intermediate_files_ );

	foreach my $intermediate_file_ ( @intermediate_files_ )
	{
	    my $exec_cmd_ = "rm -f $intermediate_file_";
	    print $main_log_file_handle_ $exec_cmd_."\n";
	    `$exec_cmd_`;
	}
    }

    return;
}

sub GetListOfDatesFromResultFile
{
    my ( $results_file_name_ ) = @_;

    open ( RESULTS_FILE , "<" , $results_file_name_ ) or PrintStacktraceAndDie ( "Could not open file $results_file_name_" );
    my @t_results_lines_ = <RESULTS_FILE>; chomp ( @t_results_lines_ );
    close ( RESULTS_FILE );

    my @t_dates_list_ = ( );

    for ( my $i = 1 ; $i <= $#t_results_lines_ ; $i ++ )
    {
	my @t_results_line_words_ = split ( ' ' , $t_results_lines_ [ $i ] );

	if ( $#t_results_line_words_ > 10 )
	{
	    if ( ! FindNumberFromVec ( $t_results_line_words_ [ 1 ] , @t_dates_list_ ) )
	    {
		push ( @t_dates_list_ , $t_results_line_words_ [ 1 ] );
	    }
	}
    }

    return @t_dates_list_;
}
