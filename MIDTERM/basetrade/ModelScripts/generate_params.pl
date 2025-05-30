#!/usr/bin/perl

# \file ModelScripts/generate_params.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
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
use Math::Complex; # sqrt
use FileHandle;
use POSIX;

sub LoadConfigFile;

sub ModifyThresholds;

sub RunFindBestParamPermuteForStrat;
sub SummarizeLocalResultsAndChooseParam;

sub FindAndReplaceSeedParam;

sub InstallParams;

sub CleanupFBPP;

sub SendReportMail;

sub StrategyNameToCode;

my $MAX_ITERATIONS_TO_TRY = 20;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/sqrt_sign.pl"; # SqrtSign
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr

require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams

require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; # GetBasepxStratFirstModel

# start
my $USAGE="$0 CONFIGFILE";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $mail_body_ = "";

my $config_file_ = $ARGV [ 0 ];

# default value initializations.
my $shortcode_ = "_";
my $name_ = "s";
my $strat_file_name_ = "_";
my $strategy_name_ = "_";
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( "TODAY-10" );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( "TODAY-1" );
my $trading_start_hhmm_ = "UTC_0010";
my $trading_end_hhmm_ = "UTC_2350";
my $trading_start_end_hhmm_ = $trading_start_hhmm_."-".$trading_end_hhmm_;
my $param_file_name_ = "_";
my $min_pnl_per_contract_ = -100.0;
my $min_volume_ = 100;
my $max_volume_ = 1000000;
my $min_ttc_ = 5;
my $max_ttc_ = 10000;
my $min_abs_pos_ = 0.0;
my $max_abs_pos_ = 500.0;
my $min_aggressive_ = 0.0;
my $max_aggressive_ = 80.0;
my $install_location_ = $HOME_DIR;
my $historical_sort_algo_ = "kCNAPnlAverage";
my $num_files_to_choose_ = 1;
my $email_address_ = "sghosh\@dvcnj.com";

my @paramvalues_to_permute_ = ( );
my $threshold_mod_factor_ = 1.4;

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $GP_WORK_DIR=$SPARE_HOME."GPW/";

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $GP_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	print STDERR "Surprising but this dir exists\n";
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $GP_WORK_DIR.$unique_gsm_id_; 
    }
    else
    {
	last;
    }
}

my $param_file_dir_ = $work_dir_."/params";

if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $param_file_dir_ ) ) { `mkdir -p $param_file_dir_`; }

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

LoadConfigFile ( $config_file_ );

my $done_ = 0;

my $last_avg_volume_ = -1;
my $last_ttc_ = -1;
my $last_pnl_per_contract_ = -1;
my $last_abs_pos_ = -1;
my $last_aggressive_ = -1;

# create a local copy to modify thresholds on.
my $permute_param_file_name_ = $param_file_dir_."/".basename ( $param_file_name_ );
`cp $param_file_name_ $permute_param_file_name_`;

my $this_run_fbpp_work_dir_ = "";

for ( my $iter_ = 0 ; $iter_ < $MAX_ITERATIONS_TO_TRY && ! $done_ ; $iter_ ++ )
{
    ModifyThresholds ( );

    RunFindBestParamPermuteForStrat ( );

    $done_ = SummarizeLocalResultsAndChooseParam ( );

    if ( $done_ )
    { # Found something that satisfied cut offs
	InstallParams ( );
    }

    CleanupFBPP ( );
}

if ( ! $done_ )
{
    print $main_log_file_handle_ "\n ERROR : Exhausted MAX_ITERATIONS_TO_TRY = $MAX_ITERATIONS_TO_TRY , failed\n";
    $mail_body_ = $mail_body_."\n ERROR : Exhausted MAX_ITERATIONS_TO_TRY = $MAX_ITERATIONS_TO_TRY , failed\n";
}

$main_log_file_handle_->close;

SendReportMail ( );

exit ( 0 );

sub LoadConfigFile
{
    my ( $t_config_file_ ) = @_;

    open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
    my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
    close ( CONFIG_FILE );

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    $mail_body_ = $mail_body_." > CONFIG_FILE=".$t_config_file_."\n";

    my $current_param_ = "";
    foreach my $config_file_lines_ ( @config_file_lines_ )
    {
	if ( index ( $config_file_lines_ , "#" ) == 0 ) # not ignoring lines with # not at the beginning
	{
	    next;
	}

	my @t_words_ = split ( ' ' , $config_file_lines_ );

	if ( $#t_words_ < 0 )
	{
	    $current_param_ = "";
	    next;
	}

	if ( ! $current_param_ )
	{
	    $current_param_ = $t_words_ [ 0 ];
	    next;
	}
	else
	{
	    given ( $current_param_ )
	    {
		when ( "SHORTCODE" )
		{
		    $shortcode_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > SHORTCODE=".$shortcode_."\n";
		    print $main_log_file_handle_ " \t > SHORTCODE=".$shortcode_."\n";
		}

		when ( "NAME" )
		{
		    $name_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > NAME=".$name_."\n";
		    print $main_log_file_handle_ " \t > NAME=".$name_."\n";
		}

		when ( "STRATFILENAME" )
		{
		    $strat_file_name_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > STRATFILENAME=".$strat_file_name_."\n";
		    print $main_log_file_handle_ " \t > STRATFILENAME=".$strat_file_name_."\n";
		}

		when ( "STRATEGYNAME" )
		{
		    $strategy_name_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > STRATEGYNAME=".$strategy_name_."\n";
		    print $main_log_file_handle_ " \t > STRATEGYNAME=".$strategy_name_."\n";
		}

		when ( "TRADING_START_END_YYYYMMDD" )
		{
		    $trading_start_yyyymmdd_ = $t_words_ [ 0 ];
		    $trading_end_yyyymmdd_ = $t_words_ [ 1 ];
		    my $trading_start_end_yyyymmdd_ = $trading_start_yyyymmdd_."-".$trading_end_yyyymmdd_;

		    $mail_body_ = $mail_body_." \t > TRADING_START_END_YYYYMMDD=".$trading_start_end_yyyymmdd_."\n";
		    print $main_log_file_handle_ " \t > TRADING_START_END_YYYYMMDD=".$trading_start_end_yyyymmdd_."\n";
		}

		when ( "TRADING_START_END_HHMM" )
		{
		    $trading_start_hhmm_ = $t_words_ [ 0 ];
		    $trading_end_hhmm_ = $t_words_ [ 1 ];
		    $trading_start_end_hhmm_ = $trading_start_hhmm_."-".$trading_end_hhmm_;

		    $mail_body_ = $mail_body_." \t > TRADING_START_END_HHMM=".$trading_start_end_hhmm_."\n";
		    print $main_log_file_handle_ " \t > TRADING_START_END_HHMM=".$trading_start_end_hhmm_."\n";
		}

		when ( "PARAMFILENAME" )
		{
		    $param_file_name_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > PARAMFILENAME=".$param_file_name_."\n";
		    print $main_log_file_handle_ " \t > PARAMFILENAME=".$param_file_name_."\n";
		}

		when ( "MIN_PNL_PER_CONTRACT" )
		{
		    $min_pnl_per_contract_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MIN_PNL_PER_CONTRACT=".$min_pnl_per_contract_."\n";
		    print $main_log_file_handle_ " \t > MIN_PNL_PER_CONTRACT=".$min_pnl_per_contract_."\n";
		}

		when ( "MIN_VOLUME" )
		{
		    $min_volume_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MIN_VOLUME=".$min_volume_."\n";
		    print $main_log_file_handle_ " \t > MIN_VOLUME=".$min_volume_."\n";
		}

		when ( "MAX_VOLUME" )
		{
		    $max_volume_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MAX_VOLUME=".$max_volume_."\n";
		    print $main_log_file_handle_ " \t > MAX_VOLUME=".$max_volume_."\n";
		}

		when ( "MIN_TTC" )
		{
		    $min_ttc_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MIN_TTC=".$min_ttc_."\n";
		    print $main_log_file_handle_ " \t > MIN_TTC=".$min_ttc_."\n";
		}

		when ( "MAX_TTC" )
		{
		    $max_ttc_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MAX_TTC=".$max_ttc_."\n";
		    print $main_log_file_handle_ " \t > MAX_TTC=".$max_ttc_."\n";
		}

		when ( "MIN_ABS_POS" )
		{
		    $min_abs_pos_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MIN_ABS_POS=".$min_abs_pos_."\n";
		    print $main_log_file_handle_ " \t > MIN_ABS_POS=".$min_abs_pos_."\n";
		}

		when ( "MAX_ABS_POS" )
		{
		    $max_abs_pos_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MAX_ABS_POS=".$max_abs_pos_."\n";
		    print $main_log_file_handle_ " \t > MAX_ABS_POS=".$max_abs_pos_."\n";
		}

		when ( "MIN_AGGRESSIVE" )
		{
		    $min_aggressive_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MIN_AGGRESSIVE=".$min_aggressive_."\n";
		    print $main_log_file_handle_ " \t > MIN_AGGRESSIVE=".$min_aggressive_."\n";
		}

		when ( "MAX_AGGRESSIVE" )
		{
		    $max_aggressive_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MAX_AGGRESSIVE=".$max_aggressive_."\n";
		    print $main_log_file_handle_ " \t > MAX_AGGRESSIVE=".$max_aggressive_."\n";
		}

		when ( "INSTALL_LOCATION" )
		{
		    $install_location_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > INSTALL_LOCATION=".$install_location_."\n";
		    print $main_log_file_handle_ " \t > INSTALL_LOCATION=".$install_location_."\n";

		    `mkdir -p $install_location_`;
		}

		when ( "HISTORICAL_SORT_ALGO" )
		{
		    $historical_sort_algo_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > HISTORICAL_SORT_ALGO=".$historical_sort_algo_."\n";
		    print $main_log_file_handle_ " \t > HISTORICAL_SORT_ALGO=".$historical_sort_algo_."\n";
		}

		when ( "PARAMVALUES_TO_PERMUTE" )
		{
		    my $t_paramvalues_to_permute_ = $t_words_ [ 0 ];

		    if ( ! FindItemFromVec ( $t_paramvalues_to_permute_ , @paramvalues_to_permute_ ) )
		    {
			push ( @paramvalues_to_permute_ , $t_paramvalues_to_permute_ );
		    }

		    $mail_body_ = $mail_body_." \t > PARAMVALUES_TO_PERMUTE=".$t_paramvalues_to_permute_."\n";
		    print $main_log_file_handle_ " \t > PARAMVALUES_TO_PERMUTE=".$t_paramvalues_to_permute_."\n";
		}

		when ( "THRESHOLD_MOD_FACTOR" )
		{
		    $threshold_mod_factor_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > THRESHOLD_MOD_FACTOR=".$threshold_mod_factor_."\n";
		    print $main_log_file_handle_ " \t > THRESHOLD_MOD_FACTOR=".$threshold_mod_factor_."\n";
		}

		when ( "NUM_FILES_TO_CHOOSE" )
		{
		    $num_files_to_choose_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > NUM_FILES_TO_CHOOSE=".$num_files_to_choose_."\n";
		    print $main_log_file_handle_ " \t > NUM_FILES_TO_CHOOSE=".$num_files_to_choose_."\n";
		}

		when ( "MAIL_ADDRESS" )
		{
		    $email_address_ = $t_words_ [ 0 ];
		    print $main_log_file_handle_ " \t > MAIL_ADDRESS=".$email_address_."\n";
		}
	    }
	}
    }
    
    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";

    return;
}

sub SendReportMail
{
    if ( ! $email_address_ )
    {
	print STDOUT $mail_body_;

	return;
    }
    else
    {
	if ( $email_address_ && $mail_body_ )
	{
	    open ( MAIL , "|/usr/sbin/sendmail -t" );
	    
	    print MAIL "To: $email_address_\n";
	    print MAIL "From: $email_address_\n";
	    print MAIL "Subject: generate_params ( $config_file_ ) $yyyymmdd_ $hhmmss_ \n\n";
	    
	    print MAIL $mail_body_ ;
	    
	    close(MAIL);
	}
    }
}

sub ModifyThresholds
{
    print $main_log_file_handle_ "\n# ModifyThresholds\n\n";

    if ( $last_avg_volume_ < 0 )
    { # first time
	print $main_log_file_handle_ "last_avg_volume_ < 0 , so generating base-results\n";

	my @param_file_contents_ = `cat $permute_param_file_name_`; chomp ( @param_file_contents_ );
	print $main_log_file_handle_ "Paramfile ( $permute_param_file_name_ ) contents :\n".join ( "\n" , @param_file_contents_ )."\n";

	return;
    }

    my $increase_volume_ = 0;

    if ( $last_avg_volume_ < $min_volume_ )
    {
	$increase_volume_ = 1;
    }
    elsif ( $last_avg_volume_ > $max_volume_ )
    {
	$increase_volume_ = -1;
    }

    my $decrease_ttc_ = 0;

    if ( $last_ttc_ > $max_ttc_ )
    {
	$decrease_ttc_ = 1;
    }
    elsif ( $last_ttc_ < $min_ttc_ )
    {
	$decrease_ttc_ = -1;
    }

    my $increase_abs_pos_ = 0;

    if ( $last_abs_pos_ < $min_abs_pos_ )
    {
	$increase_abs_pos_ = 1;
    }
    elsif ( $last_abs_pos_ > $max_abs_pos_ )
    {
	$increase_abs_pos_ = -1;
    }

    my $increase_aggressive_ = 0;

    if ( $last_aggressive_ < $min_aggressive_ )
    {
	$increase_aggressive_ = 1;
    }
    elsif ( $last_aggressive_ > $max_aggressive_ )
    {
	$increase_aggressive_ = -1;
    }

    open ( PARAM_FILE , "<" , $permute_param_file_name_ ) or PrintStacktraceAndDie ( "Could not open param file $permute_param_file_name_" );
    my @param_file_lines_ = <PARAM_FILE>; chomp ( @param_file_lines_ );
    close ( PARAM_FILE );

    open ( PARAM_FILE , ">" , $permute_param_file_name_ ) or PrintStacktraceAndDie ( "Could not open param file $permute_param_file_name_" );

    my %paramvalue_to_values_ = ( );

    foreach my $line_ ( @param_file_lines_ )
    {
	my @line_words_ = split ( ' ' , $line_ );
	if ( $#line_words_ >= 2 )
	{
	    my $paramvalue_ = $line_words_ [ 1 ];

	    # add the original value for this key
	    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , $line_words_ [ 2 ] );

	    if ( FindItemFromVec ( $paramvalue_ , @paramvalues_to_permute_ ) )
	    { # progressively permute this paramvalue

		given ( $paramvalue_ )
		{
		    when ( "WORST_CASE_UNIT_RATIO" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( $threshold_mod_factor_ ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , floor ( - $threshold_mod_factor_ ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , floor ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) );
			    }
			}
		    }
		    when ( "MAX_UNIT_RATIO" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) );
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , floor ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) );
			}
		    }
		    when ( "UNIT_TRADE_SIZE" )
		    {
		    }
		    when ( "HIGHPOS_LIMITS_UNIT_RATIO" )
		    {
		    }
		    when ( "HIGHPOS_THRESH_FACTOR" )
		    {
		    }
		    when ( "HIGHPOS_SIZE_FACTOR" )
		    {
		    }
		    when ( "INCREASE_PLACE" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( - $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) ) );
			    }
			}
		    }
		    when ( "INCREASE_KEEP" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( - $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) ) );
			    }
			}
		    }
		    when ( "ZEROPOS_LIMITS_UNIT_RATIO" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) );
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , floor ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) );
			}
		    }
		    when ( "ZEROPOS_PLACE" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( - $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) ) );
			    }
			}
		    }
		    when ( "ZEROPOS_KEEP" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( - $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) ) );
			    }
			}
		    }
		    when ( "DECREASE_PLACE" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( - $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) ) );
			    }
			}
		    }
		    when ( "DECREASE_KEEP" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( - $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) ) );
			    }
			}
		    }
		    when ( "MAX_LOSS" )
		    {
		    }
		    when ( "MAX_OPENTRADE_LOSS" )
		    {
			print $main_log_file_handle_ "Reached MAX_OPENTRADE_LOSS\n";

			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) );

			    print $main_log_file_handle_ "pushing $paramvalue_ = ".ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ )."\n";
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , floor ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) );

			    print $main_log_file_handle_ "pushing $paramvalue_ = ".ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ )."\n";
			}
		    }
		    when ( "COOLOFF_INTERVAL" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    { # do nothing cool off is already disabled
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , floor ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( 1000 * $threshold_mod_factor_ ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) );
			    }
			}
		    }
		    when ( "STDEV_FACT" )
		    {
		    }
		    when ( "STDEV_CAP" )
		    {
		    }
		    when ( "USE_SQRT_STDEV_MULT" )
		    {
		    }
		    when ( "LOW_STDEV_LVL" )
		    {
		    }
		    when ( "QUEUE_LONGEVITY_SUPPORT" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( 0.1 * $threshold_mod_factor_ ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    { # do nothing , long support already disabled.
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , floor ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) );
			    }
			}
		    }
		    when ( "ALLOWED_TO_AGGRESS" )
		    {
		    }
		    when ( "AGGRESSIVE" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( - $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" , ( $threshold_mod_factor_ ) ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , sprintf ( "%0.4f" ,  ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) ) );
			    }
			}
		    }
		    when ( "MAX_POSITION_TO_AGGRESS_UNIT_RATIO" )
		    {
		    }
		    when ( "MAX_POSITION_TO_CANCEL_ON_AGGRESS_UNIT_RATIO" )
		    {
		    }
		    when ( "MAX_INT_SPREAD_TO_PLACE" )
		    {
		    }
		    when ( "MAX_INT_SPREAD_TO_CROSS" )
		    {
		    }
		    when ( "AGG_COOLOFF_INTERVAL" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    { # do nothing cool off is already disabled
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , floor ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) );
			    }
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    if ( $line_words_ [ 2 ] == 0 )
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( 1000 * $threshold_mod_factor_ ) );
			    }
			    else
			    {
				push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) );
			    }
			}
		    }
		    when ( "MODERATE_TIME_LIMIT" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , floor ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) );
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) );
			}
		    }
		    when ( "HIGH_TIME_LIMIT" )
		    {
			if ( $increase_volume_ > 0 )
			{ # DECREASE zeropos-* , increase-pos-* , decrease-pos-* , INCREASE worst-case-* , max-unit-*
			    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , floor ( $line_words_ [ 2 ] / $threshold_mod_factor_ ) );
			}
			elsif ( $increase_volume_ < 0 )
			{ # INCREASE zeropos-* , increase-pos-* , decrease-pos-* , DECREASE worst-case-* , max-unit-*
			    push ( @ { $paramvalue_to_values_ { $paramvalue_ } } , ceil ( $line_words_ [ 2 ] * $threshold_mod_factor_ ) );
			}
		    }
		}
	    }

	    print PARAM_FILE "PARAMVALUE ".$paramvalue_." ";

	    my @added_values_ = ( );
	    foreach my $permute_value_ ( @ { $paramvalue_to_values_ { $paramvalue_ } } )
	    {
		chomp ( $permute_value_ );

		if ( ! FindItemFromVec ( $permute_value_ , @added_values_ ) )
		{
		    print PARAM_FILE $permute_value_." ";
		    push ( @added_values_ , $permute_value_ );
		}
	    }

	    print PARAM_FILE "\n";
	}
    }

    close ( PARAM_FILE );

    my @param_file_contents_ = `cat $permute_param_file_name_`; chomp ( @param_file_contents_ );
    print $main_log_file_handle_ "Paramfile ( $permute_param_file_name_ ) contents :\n".join ( "\n" , @param_file_contents_ )."\n";

    return;
}

sub RunFindBestParamPermuteForStrat
{
    print $main_log_file_handle_ "\n# RunFindBestParamPermuteForStrat\n\n";

    my $FBPP_SCRIPT = $MODELSCRIPTS_DIR."/find_best_param_permute_for_strat.pl";

    # /home/sghosh/basetrade/ModelScripts/find_best_param_permute_for_strat.pl shortcode timeperiod basepx strategyname param_file_with_permutations start_date end_date strategy_file_name

    my $exec_cmd_ = $FBPP_SCRIPT." ".$shortcode_." ".$trading_start_end_hhmm_." ALL ".$strategy_name_." ".$permute_param_file_name_." ".$trading_start_yyyymmdd_." ".$trading_end_yyyymmdd_." ".$strat_file_name_;

    print $main_log_file_handle_ $exec_cmd_."\n";

    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );
    print $main_log_file_handle_ join ( "\n" , @exec_cmd_output_ )."\n";

    my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ 0 ] ); chomp ( @exec_cmd_output_words_ );
    $this_run_fbpp_work_dir_ = $exec_cmd_output_words_ [ $#exec_cmd_output_words_ ];

    return;
}

sub SummarizeLocalResultsAndChooseParam
{
    print $main_log_file_handle_ "\n# SummarizeLocalResultsAndChooseParam\n\n";

    my $SUMMARIZE_EXEC = $LIVE_BIN_DIR."/summarize_local_results_and_choose_by_algo_extended_checks";
    my $local_results_base_dir_ = $this_run_fbpp_work_dir_."/local_results_base_dir";

    my $summarize_exec_cmd_ = $SUMMARIZE_EXEC." ".$historical_sort_algo_." 0 ".$num_files_to_choose_." ".$min_pnl_per_contract_." ".$min_volume_." ".$max_volume_." ".$min_ttc_." ".$max_ttc_." ".$min_abs_pos_." ".$max_abs_pos_." ".$min_aggressive_." ".$max_aggressive_." ".$local_results_base_dir_."/*/*/*/*";

    print $main_log_file_handle_ $summarize_exec_cmd_."\n";
    my @summarize_exec_cmd_output_ = `$summarize_exec_cmd_`; chomp ( @summarize_exec_cmd_output_ );

    print $main_log_file_handle_ join ( "\n" , @summarize_exec_cmd_output_ )."\n";

    my $are_all_cutoffs_filled_ = 0; # flag to specify that all cut-offs were passed

    if ( $#summarize_exec_cmd_output_ >= 0 )
    {
	$are_all_cutoffs_filled_ = 1;
    }
    else
    {
	print $main_log_file_handle_ "Nothing satisfied cut-offs\n";
	$summarize_exec_cmd_ = $SUMMARIZE_EXEC." ".$historical_sort_algo_." 1 ".$num_files_to_choose_." ".$min_pnl_per_contract_." ".$min_volume_." ".$max_volume_." ".$min_ttc_." ".$max_ttc_." ".$min_abs_pos_." ".$max_abs_pos_." ".$min_aggressive_." ".$max_aggressive_." ".$local_results_base_dir_."/*/*/*/*";

	print $main_log_file_handle_ $summarize_exec_cmd_."\n";
	@summarize_exec_cmd_output_ = `$summarize_exec_cmd_`; chomp ( @summarize_exec_cmd_output_ );

	print $main_log_file_handle_ join ( "\n" , @summarize_exec_cmd_output_ )."\n";

	$are_all_cutoffs_filled_ = 0;
    }

    if ( $last_avg_volume_ < 0 )
    { # base-results , include in the email.
	$mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";

	$mail_body_ = $mail_body_."\n Base-results :\n";
	$mail_body_ = $mail_body_.join ( "\n" , @summarize_exec_cmd_output_ )."\n";

	$mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    }
    elsif ( ! $are_all_cutoffs_filled_ )
    { # atleast the 2nd iteration , try to converge on the target statistics

	# check volume within range.
	
    }

    foreach my $line_ ( @summarize_exec_cmd_output_ )
    {
	chomp ( $line_ );

	my @line_words_ = split ( ' ' , $line_ );
	if ( $#line_words_ > 0 )
	{
	    if ( $line_words_ [ 0 ] eq "STRATEGYFILEBASE" )
	    {
		my $t_strategy_file_name_ = $line_words_ [ 1 ];
		
		$t_strategy_file_name_ = $this_run_fbpp_work_dir_."/*/".$t_strategy_file_name_;

		print $main_log_file_handle_ "cat $t_strategy_file_name_\n";

		my @strategy_file_contents_ = `cat $t_strategy_file_name_`; chomp ( @strategy_file_contents_ );

		print $main_log_file_handle_ join ( "\n" , @strategy_file_contents_ )."\n";

		my @strategy_file_words_ = split ( ' ' , $strategy_file_contents_ [ 0 ] ); chomp ( @strategy_file_words_ );

		my $t_param_file_ = $strategy_file_words_ [ 4 ];

		print $main_log_file_handle_ "Picked param file ".$t_param_file_." to replace seed\n";
		print $main_log_file_handle_ "cp $t_param_file_ $permute_param_file_name_\n";

		`cp $t_param_file_ $permute_param_file_name_`;
	    }
	    elsif ( $line_words_ [ 0 ] eq "STATISTICS" )
	    {
		$last_avg_volume_ = $line_words_ [ 3 ];
		$last_ttc_ = $line_words_ [ 7 ];
		$last_pnl_per_contract_ = $line_words_ [ 9 ];
		$last_abs_pos_ = $line_words_ [ 14 ];
		$last_aggressive_ = $line_words_ [ 12 ];

		print $main_log_file_handle_ " last_avg_volume_ = ".$last_avg_volume_." last_ttc_ = ".$last_ttc_." last_pnl_per_contract_ = ".$last_pnl_per_contract_." last_abs_pos_ = ".$last_abs_pos_." last_aggressive_ = ".$last_aggressive_."\n";
	    }
	}
    }


    return $are_all_cutoffs_filled_;
}

sub InstallParams
{
    print $main_log_file_handle_ "\n# InstallParams\n\n";

    my $installed_param_file_name_ = $install_location_."/param_".$shortcode_."_".StrategyNameToCode ( $strategy_name_ )."_".$name_;

    for ( my $pfi_ = 0 ; $pfi_ < 100 ; $pfi_ ++ )
    {
	my $t_installed_param_file_name_ = $installed_param_file_name_.$pfi_;

	if ( ExistsWithSize ( $t_installed_param_file_name_ ) )
	{
	    print $main_log_file_handle_ "$t_installed_param_file_name_ exists , skipping\n";
	}
	else
	{
	    print $main_log_file_handle_ "Installing $permute_param_file_name_ as $t_installed_param_file_name_\n";
	    print $main_log_file_handle_ "cp $permute_param_file_name_ $t_installed_param_file_name_\n";

	    `cp $permute_param_file_name_ $t_installed_param_file_name_`;
	    last;
	}
    }

    return;
}

sub CleanupFBPP
{
    print $main_log_file_handle_ "\n# CleanupFBPP\n\n";

    `rm -rf $this_run_fbpp_work_dir_`;
    $this_run_fbpp_work_dir_ = "";

    return;
}

sub StrategyNameToCode
{
    my ( $t_strategy_name_ ) = @_;

    my $code_ = "s";

    my %strategy_to_code_ = ( "DirectionalAggressiveTrading" => "dat" ,
			      "DirectionalInterventionAggressiveTrading" => "diat" ,
			      "DirectionalInterventionLogisticTrading" => "dilt" ,
			      "DirectionalLogisticTrading" => "dlt" ,
			      "DirectionalPairAggressiveTrading" => "dpat" ,
			      "PriceBasedAggressiveTrading" => "pbat" ,
			      "PriceBasedInterventionAggressiveTrading" => "pbiat" ,
			      "PriceBasedSecurityAggressiveTrading" => "pbsat" ,
			      "PriceBasedTrading" => "pbt" ,
			      "PriceBasedVolTrading" => "pbvt" ,
			      "PriceBasedScalper" => "pbs" ,
			      "PricePairBasedAggressiveTrading" => "ppbat" ,
			      "ReturnsBasedAggressiveTrading" => "rbat" ) ;

    if ( exists ( $strategy_to_code_ { $t_strategy_name_ } ) )
    {
	$code_ = $strategy_to_code_ { $t_strategy_name_ };
    }

    return $code_;
}
