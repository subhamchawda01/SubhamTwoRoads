#!/usr/bin/perl

# \file ModelScripts/summarize_simconfigs.pl
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

sub GetResults;
sub SummarizeResults;
sub DisplaySummarizedResults;

sub CombineSimrealResults;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";
my $FBSCP_WORK_DIR = $SPARE_HOME."FBSCP/";

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

# start 
my $USAGE = "$0 SHORTCODE START_DATE END_DATE FBSCPID [SUMMARIZATION_CRITERA=SFINALPNL]";
my @SUMMARIZATION_CRITERIA = ( "PNLMEAN" , "PNLMEDIAN" , "PNLSTDEV" , "FINALPNL" ,
			       "VOLMEAN" , "VOLMEDIAN" , "VOLSTDEV" , "TRADES" , "SPNLMEDIAN" , "SFINALPNL" );

if ( $#ARGV < 3 ) { print $USAGE."\n\t".join ( "\n\t" , @SUMMARIZATION_CRITERIA )."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 1 ] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 2 ] );

my $fbscp_id_ = $ARGV [ 3 ];

my $summarization_criteria_ = "SFINALPNL";
if ( $#ARGV >= 4 ) { $summarization_criteria_ = $ARGV [ 4 ]; }

# Map summarization_criteria -> summarization_index
my $summarization_index_ = 9;
for ( my $i = 0 ; $i <= $#SUMMARIZATION_CRITERIA ; $i ++ )
{
    if ( $SUMMARIZATION_CRITERIA [ $i ] eq $summarization_criteria_ )
    {
	$summarization_index_ = $i;
	last;
    }
}

print "USING SUMMARIZATION_CRITERIA=$summarization_criteria_\n\n";

my $work_dir_ = $FBSCP_WORK_DIR.$fbscp_id_;

my $local_results_base_dir_ = $work_dir_."/local_results_base_dir";
my $temp_results_base_dir_ = $work_dir_."/temp_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $local_params_dir_ = $work_dir_."/params_dir";
my $local_models_dir_ = $work_dir_."/models_dir";
my $local_simconfigs_dir_ = $work_dir_."/simconfigs_dir";
my $local_tradesdiff_dir_ = $work_dir_."/tradesdiffs_dir";

my $main_log_file_ = $work_dir_."/main_log_file.txt";

if ( ! ( -d $work_dir_ ) ) { PrintStacktraceAndDie ( "$work_dir_ doesnot exist" ); }
if ( ! ( -d $local_results_base_dir_ ) ) { PrintStacktraceAndDie ( "$local_results_base_dir_ doesnot exist" ); }
if ( ! ( -d $temp_results_base_dir_ ) ) { PrintStacktraceAndDie ( "$temp_results_base_dir_ doesnot exist" ); }
if ( ! ( -d $local_strats_dir_ ) ) { PrintStacktraceAndDie ( "$local_strats_dir_ doesnot exist" ); }
if ( ! ( -d $local_params_dir_ ) ) { PrintStacktraceAndDie ( "$local_params_dir_ doesnot exist" ); }
if ( ! ( -d $local_models_dir_ ) ) { PrintStacktraceAndDie ( "$local_models_dir_ doesnot exist" ); }
if ( ! ( -d $local_simconfigs_dir_ ) ) { PrintStacktraceAndDie ( "$local_simconfigs_dir_ doesnot exist" ); }
if ( ! ( -d $local_tradesdiff_dir_ ) ) { PrintStacktraceAndDie ( "$local_tradesdiff_dir_ doesnot exist" ); }
if ( ! ( -e $main_log_file_ ) ) { PrintStacktraceAndDie ( "$main_log_file_ doesnot exist" ); }

my %sim_config_to_date_to_results_ = ( );
GetResults ( );

my %sim_config_to_summarized_results_ = ( );
SummarizeResults ( );

DisplaySummarizedResults ( );

exit ( 0 );

sub GetResults
{
    for ( my $trading_yyyymmdd_ = $trading_end_yyyymmdd_ ; $trading_yyyymmdd_ > $trading_start_yyyymmdd_ ; )
    {
	if ( ! ValidDate ( $trading_yyyymmdd_ ) ||
	     SkipWeirdDate ( $trading_yyyymmdd_ ) ||
	     IsDateHoliday ( $trading_yyyymmdd_ ) ||
	     IsWeirdSimDayForShortcode ( $shortcode_ , $trading_yyyymmdd_ ) )
	{
	    $trading_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_yyyymmdd_ , 1 );
	    next;
	}

	my ( $yyyy_ , $mm_ , $dd_ ) = BreakDateYYYYMMDD ( $trading_yyyymmdd_ );
	my $t_date_local_results_database_ = $local_results_base_dir_."/".$yyyy_."/".$mm_."/".$dd_."/results_database.txt";

	if ( ! -e $t_date_local_results_database_ ) 
	{ 
	    $trading_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_yyyymmdd_ , 1 );
	    next;
	}

	open ( DATE_LOCAL_RESULTS_FILE , "<" , $t_date_local_results_database_ ) or PrintStacktraceAndDie ( "Could not open file $t_date_local_results_database_" );

	while ( my $t_line_ = <DATE_LOCAL_RESULTS_FILE> )
	{
	    chomp ( $t_line_ );
	    my @results_words_ = split ( ' ' , $t_line_ );

	    if ( $#results_words_ < 11 ) { PrintStacktraceAndDie ( "Malformed results line in $t_date_local_results_database_" ); }

	    my $t_sim_config_ = $results_words_ [ 0 ];
	    my $t_date_ = $results_words_ [ 1 ];

	    @results_words_ = splice ( @results_words_ , 2 , $#results_words_ );
	    my $t_results_ = join ( ' ' , @results_words_ );

	    $sim_config_to_date_to_results_ { $t_sim_config_ } { $t_date_ } = $t_results_;
	    # print "# sim_config_to_date_to_results $t_sim_config_ $t_date_ = $t_results_\n";
	}

	close ( DATE_LOCAL_RESULTS_FILE ); 

	$trading_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_yyyymmdd_ , 1 );
    }

    return;
}

sub SummarizeResults
{
    foreach my $sim_config_ ( keys %sim_config_to_date_to_results_ )
    {
	my @t_all_results_ = ( );

	foreach my $date_ ( keys % { $sim_config_to_date_to_results_ { $sim_config_ } } )
	{
	    push ( @t_all_results_ , $sim_config_to_date_to_results_ { $sim_config_ } { $date_ } );
	}

	$sim_config_to_summarized_results_ { $sim_config_ } = CombineSimrealResults ( @t_all_results_ );	
    }

    return;
}

sub CombineSimrealResults
{
    my ( @results_list_ ) = @_;

    # print "# CombineSimrealResults = ".join ( "\n\t" , @results_list_ )."\n";

    my $combined_simreal_result_ = "";

    my $final_pnl_diff_ = 0;

    my $abs_pnl_diff_mean_ = 0;
    my $abs_pnl_diff_median_ = 0;
    my $abs_pnl_diff_stdev_ =  0;

    my $pnl_diff_median_ = 0;
    my $signed_final_pnl_diff_ = 0;

    my $abs_vol_diff_mean_ = 0;
    my $abs_vol_diff_median_ = 0;
    my $abs_vol_diff_stdev_ = 0;

    my $average_trades_ = 0;
    my $total_trades_ = 0;

    foreach my $result_line_ ( @results_list_ )
    {
	my @result_words_ = split ( ' ' , $result_line_ ); chomp ( @result_words_ );

	if ( $#result_words_ > 7 )
	{
	    $final_pnl_diff_ += ( $result_words_ [ 3 ] * $result_words_ [ 7 ] );

	    $abs_pnl_diff_mean_ += ( $result_words_ [ 0 ] * $result_words_ [ 7 ] );
	    $abs_pnl_diff_median_ += ( $result_words_ [ 1 ] * $result_words_ [ 7 ] );
	    $abs_pnl_diff_stdev_ += ( $result_words_ [ 2 ] * $result_words_ [ 7 ] );

	    $abs_vol_diff_mean_ += ( $result_words_ [ 4 ] * $result_words_ [ 7 ] );
	    $abs_vol_diff_median_ += ( $result_words_ [ 5 ] * $result_words_ [ 7 ] );
	    $abs_vol_diff_stdev_ += ( $result_words_ [ 6 ] * $result_words_ [ 7 ] );

	    $pnl_diff_median_ += ( $result_words_ [ 8 ] * $result_words_ [ 7 ] );
	    $signed_final_pnl_diff_ += ( $result_words_ [ 9 ] * $result_words_ [ 7 ] );

	    $average_trades_ += $result_words_ [ 7 ];
	    $total_trades_ += $result_words_ [ 7 ];
	}
    }

    # Normalize combined results.
    $final_pnl_diff_ /= $total_trades_;

    $abs_pnl_diff_mean_ /= $total_trades_;
    $abs_pnl_diff_median_ /= $total_trades_;
    $abs_pnl_diff_stdev_ /= $total_trades_;

    $abs_vol_diff_mean_ /= $total_trades_;
    $abs_vol_diff_median_ /= $total_trades_;
    $abs_vol_diff_stdev_ /= $total_trades_;

    $pnl_diff_median_ /= $total_trades_;
    $signed_final_pnl_diff_ /= $total_trades_;

    $average_trades_ /= $total_trades_;

    # PNLDIFFMEAN PNLDIFFMEDIAN PNLDIFFSTDEV FINALPNLDIFF 
    # ABSVOLDIFFMEAN ABSVOLDIFFMEDIAN ABSVOLDIFFSTDEV 
    # AVGTRADES 
    # SIGNEDPNLDIFFMEDIAN SIGNEDFINALPNLDIFF
    $combined_simreal_result_ = sprintf ( "%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %+7.2f %+7.2f" , 
					  $abs_pnl_diff_mean_ , $abs_pnl_diff_median_ , $abs_pnl_diff_stdev_ , $final_pnl_diff_ , 
					  $abs_vol_diff_mean_ , $abs_vol_diff_median_ , $abs_vol_diff_stdev_ , 
					  $average_trades_ , 
					  $pnl_diff_median_ , $signed_final_pnl_diff_ );

    return $combined_simreal_result_;
}

sub DisplaySummarizedResults
{
    my $t_head_ = sprintf ( "%60s %9s %9s %9s %9s %9s %9s %9s %9s %10s %10s\n" , 
			    "SIMCONFIG" , "PNLMEAN" , "PNLMEDIAN" , "PNLSTDEV" , "FINALPNL" ,
			    "VOLMEAN" , "VOLMEDIAN" , "VOLSTDEV" , "TRADES" , "SPNLMEDIAN" , "SFINALPNL" );

    print $t_head_;

    my %summarization_criteria_to_sim_config_ = ( );

    foreach my $sim_config_ ( keys %sim_config_to_summarized_results_ )
    {
	my @result_words_ = split ( ' ' , $sim_config_to_summarized_results_ { $sim_config_ } );

	my $t_summarization_field_ = $result_words_ [ $summarization_index_ ];

	$summarization_criteria_to_sim_config_ { $t_summarization_field_ } = $sim_config_;
    }

    foreach my $summarization_field_ ( sort { abs ( $a ) <=> abs ( $b ) }
				       keys %summarization_criteria_to_sim_config_ )
    {	
	my $t_sim_config_ = $summarization_criteria_to_sim_config_ { $summarization_field_ };

	my @result_words_ = split ( ' ' , $sim_config_to_summarized_results_ { $t_sim_config_ } );

	my $t_line_ = sprintf ( "%60s %9.2f %9.2f %9.2f %9.2f %9.2f %9.2f %9.2f %9.2f %+10.2f %+10.2f\n" , 
				$t_sim_config_ , $result_words_ [ 0 ] , $result_words_ [ 1 ] , $result_words_ [ 2 ] , $result_words_ [ 3 ] , $result_words_ [ 4 ],
				$result_words_ [ 5 ] , $result_words_ [ 6 ] , $result_words_ [ 7 ] , $result_words_ [ 8 ] , $result_words_ [ 9 ] );

	print $t_line_;
    }

    return;
}
