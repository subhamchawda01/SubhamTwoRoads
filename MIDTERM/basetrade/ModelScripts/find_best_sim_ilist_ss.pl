#!/usr/bin/perl

# \file ModelScripts/find_best_sim_ilist_ss.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use feature "switch";

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' }; 

my $REPO = "basetrade";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/global_results_methods.pl"; # Get*

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetGoodStrategies;
sub GetIndicatorsForStrategy;

sub ScoreIndicators;

sub WriteIlistFilesToDirectory;

sub PriceTypeToString;

my $output_dir_ = $ENV { 'PWD' };

if ( $#ARGV < 0 ) 
{ print "USAGE = SHORTCODE [ ILIST_OUTPUT_DIR = $output_dir_ ] [ NUM_PAST_DAYS = 10 ] [ TIMEPERIOD = US ] [ PNL_PER_AVG_ABS_POS=200 ]\n"; exit ( 0 ); }

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );

my $shortcode_ = $ARGV [ 0 ];
my $num_past_days_ = 10;
my $timeperiod_ = "US";
my $min_pnl_per_pos_ = 200;

if ( $#ARGV > 0 ) { $output_dir_ = $ARGV [ 1 ]; }
if ( $#ARGV > 1 ) { $num_past_days_ = $ARGV [ 2 ]; }
if ( $#ARGV > 2 ) { $timeperiod_ = $ARGV [ 3 ]; }
if ( $#ARGV > 3 ) { $min_pnl_per_pos_ = $ARGV [ 4 ]; }

my %date_to_good_strategies_ = ( );
GetGoodStrategies ( $shortcode_ , $num_past_days_ );

my %indicator_to_score_ = ( );
my %indicator_to_position_ = ( );
my %indicator_to_corr_ = ( );

foreach my $date_ ( keys ( %date_to_good_strategies_ ) )
{
#    print "$date_\n";

    foreach my $strat_name_ ( keys ( % { $date_to_good_strategies_ { $date_ } } ) )
    {
	my $t_pnl_per_pos_ = $date_to_good_strategies_ { $date_ } { $strat_name_ };

#	print "QID = $qid_ $t_pnl_per_contract_\n";

	%indicator_to_position_ = GetIndicatorsForStrategy ( $strat_name_ );

	ScoreIndicators ( $t_pnl_per_pos_ , 5 ); # Ignore indicators after the 5th
    }
}

foreach my $indicator_name_ ( sort { $indicator_to_score_ { $b } <=> $indicator_to_score_ { $a } }
			      keys ( %indicator_to_score_ ) )
{
    my $indicator_score_ = $indicator_to_score_ { $indicator_name_ };
}

WriteIlistFilesToDirectory ( $output_dir_ , 60 ); # Write at most 60 indicators to the ilist.

exit ( 0 );

sub GetGoodStrategies
{
    my ( $shortcode_ , $num_past_days_ ) = @_;

    my $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $yyyymmdd_ , 1 );

    for ( ; $num_past_days_ > 0 ; $num_past_days_ -- )
    {
	if ( ! ValidDate ( $current_yyyymmdd_ ) ) { PrintStacktraceAndDie ( "Invalid date $current_yyyymmdd_\n" ); }

	if ( SkipWeirdDate ( $current_yyyymmdd_ ) || IsDateHoliday ( $current_yyyymmdd_ ) )
	{
	    $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
	    $num_past_days_ ++;
	    next;
	}

	my @global_results_ = ();
  GetGlobalResultsForShortcodeDate ( $shortcode_ , $current_yyyymmdd_ , \@global_results_ );

	for ( my $i = 0 ; $i <= $#global_results_ ; $i ++ )
	{
	    my $global_result_vec_ref_ = $global_results_ [ $i ];

	    my $t_strat_name_ = GetStratNameFromGlobalResultVecRef ( $global_result_vec_ref_ );

	    my $t_volume_ = GetVolFromGlobalResultVecRef ( $global_result_vec_ref_ );
	    my $t_pnl_ = GetPnlFromGlobalResultVecRef ( $global_result_vec_ref_ );

	    my $t_avg_abs_pos_ = GetAvgAbsPosFromGlobalResultVecRef ( $global_result_vec_ref_ );
	    my $t_pnl_per_pos_ = $t_pnl_ / ( $t_avg_abs_pos_ + 0.000001 );

	    if ( $t_pnl_per_pos_ > $min_pnl_per_pos_ )
	    {
		$date_to_good_strategies_ { $current_yyyymmdd_ } { $t_strat_name_ } = 1;
		print "date_to_good_strategy ( $current_yyyymmdd_ ) = ( $t_strat_name_ , $t_pnl_per_pos_ )\n";
	    }
	}

	$current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
    }

    return;
}

sub GetIndicatorsForStrategy
{
    my ( $strat_name_ ) = @_;

    my %indicator_to_position_ = ( );

    my $PRINT_MODEL_FROM_BASE = $SCRIPTS_DIR."/print_model_from_base.sh";
    my $exec_cmd_ = $PRINT_MODEL_FROM_BASE." $strat_name_";

    my @model_file_full_path_ = `$exec_cmd_ 2>/dev/null`; chomp ( $exec_cmd_ );

    if ( $#model_file_full_path_ < 0 )
    {
#	PrintStacktrace ( "Could not find / open model file for $strat_name_" );
	return;
    }

    my $t_model_file_ = $model_file_full_path_ [ 0 ];
    my @indicator_list_ = `/bin/grep INDICATOR $t_model_file_`;
    my @all_indicators_ = ( );

    for ( my $i = $#indicator_list_ ; $i >= 0 ; $i -- )
    {
	if ( index ( $indicator_list_ [ $i ] , "INDICATOREND" ) != -1 ) { next; }
	if ( index ( $indicator_list_ [ $i ] , "INDICATORSTART" ) != -1 ) { last; }

	my @indicator_words_ = split ( ' ' , $indicator_list_ [ $i ] ); chomp ( @indicator_words_ );

	$indicator_words_ [ 1 ] = "1.00";

	my $t_indicator_name_ = "";
	for ( my $iw = 0 ; $iw <= $#indicator_words_ ; $iw ++ )
	{
	    if ( $indicator_words_ [ $iw ] eq "#" ) 
	    { 
		# Save the corrs.
		if ( $#indicator_words_ >= ( $iw + 1 ) )
		{
		    $indicator_to_corr_ { $t_indicator_name_ } = $indicator_words_ [ $iw + 1 ];
		}
		last;
	    }

	    if ( $iw )
	    { $t_indicator_name_ = $t_indicator_name_." ".$indicator_words_ [ $iw ]; }
	    else
	    { $t_indicator_name_ = $indicator_words_ [ $iw ]; }
	}

	push ( @all_indicators_ , $t_indicator_name_ );
    }

    for ( my $i = $#all_indicators_ ; $i >= 0 ; $i -- )
    {
	$indicator_to_position_ { $all_indicators_ [ $i ] } = ( $i + 1 ) / ( $#all_indicators_ + 1 );

	my $t_ind_ = $all_indicators_ [ $i ];
	my $t_pos_score_ = $indicator_to_position_ { $t_ind_ };
#	print "$t_ind_ => $t_pos_score_\n";
    }

    return %indicator_to_position_;
}

sub ScoreIndicators
{
    my ( $pnl_per_pos_ , $num_top_indicators_to_pick_ ) = @_;

#    print "ScoreIndicators $pnl_per_contract_\n";

    foreach my $indicator_name_ ( sort { $indicator_to_position_ { $b } <=> $indicator_to_position_ { $a } }
				  keys ( %indicator_to_position_ ) )
    {
#	print $indicator_name_." ".$indicator_to_position_ { $indicator_name_ }."\n";

	$indicator_to_score_ { $indicator_name_ } += 
	    ( $indicator_to_position_ { $indicator_name_ } * $pnl_per_pos_ );

	$num_top_indicators_to_pick_ --; # Pick top 'k' indicators.
	if ( $num_top_indicators_to_pick_ <= 0 ) { last; }
    }

    return;
}

sub WriteIlistFilesToDirectory
{
    my ( $output_dir_ , $max_indicators_to_write_ ) = @_;

    if ( scalar ( keys ( %indicator_to_score_ ) ) <= 0 ) { return; }

    my @price_types_list_ = ( );

    push ( @price_types_list_ , "MidPrice MidPrice" );
    push ( @price_types_list_ , "MktSizeWPrice MktSizeWPrice" );
    push ( @price_types_list_ , "MidPrice MktSizeWPrice" );
    push ( @price_types_list_ , "MktSinusoidal MktSinusoidal" );
    push ( @price_types_list_ , "OrderWPrice OrderWPrice" );
    push ( @price_types_list_ , "OfflineMixMMS OfflineMixMMS" );
    push ( @price_types_list_ , "TradeWPrice TradeWPrice" );

    for ( my $i = 0 ; $i <= $#price_types_list_ ; $i ++ )
    {
	my $t_price_type_ = $price_types_list_ [ $i ];
	my @t_price_type_words_ = split ( ' ' , $t_price_type_ );

	my $t_price_dep_type_ = $t_price_type_words_ [ 0 ];
	my $t_price_indep_type_ = $t_price_type_words_ [ 1 ];

	my $t_price_dep_string_ = PriceTypeToString ( $t_price_dep_type_ );
	my $t_price_indep_string_ = PriceTypeToString ( $t_price_indep_type_ );

	my $ilist_file_name_ = "";
	if ( $output_dir_ ) { $ilist_file_name_ = $output_dir_."/"; }

	$ilist_file_name_ = $ilist_file_name_."ilist_".$shortcode_."_".$timeperiod_."_".$t_price_dep_string_."_".$t_price_indep_string_."_ss";

	open ( ILIST_FILE , "> $ilist_file_name_ " ) or PrintStacktraceAndDie ( "Could not open file to write $ilist_file_name_\n" );

	print ILIST_FILE "MODELINIT DEPBASE ".$shortcode_." ".$t_price_dep_type_." ".$t_price_indep_type_."\n";
	print ILIST_FILE "MODELMATH LINEAR CHANGE\n";
	print ILIST_FILE "INDICATORSTART\n";

	my $t_max_indicators_to_write_ = $max_indicators_to_write_;
	foreach my $indicator_name_ ( sort { $indicator_to_score_ { $b } <=> $indicator_to_score_ { $a } }
				      keys ( %indicator_to_score_ ) )
	{
	    if ( exists ( $indicator_to_corr_ { $indicator_name_ } ) )
	    {
		my $t_corr_ = $indicator_to_corr_ { $indicator_name_ };
		print ILIST_FILE "$indicator_name_ # $t_corr_\n";
	    }
	    else
	    {
		print ILIST_FILE "$indicator_name_\n";
	    }

	    $t_max_indicators_to_write_ --;
	    if ( $t_max_indicators_to_write_ <= 0 ) { last; }
	}

	print ILIST_FILE "INDICATOREND\n";

	close ( ILIST_FILE );
    }

    return;
}

sub PriceTypeToString
{
    my ( $price_type_ ) = @_;

    my $price_string_ = "";

    given ( $price_type_ )
    {
	when ( "MidPrice" ) { $price_string_ = "Mid"; }

	when ( "MktSizeWPrice" ) { $price_string_ = "Mkt"; }

	when ( "MktSinusoidal" ) { $price_string_ = "Sin"; }

	when ( "OrderWPrice" ) { $price_string_ = "Owp"; }

	when ( "OfflineMixMMS" ) { $price_string_ = "OMix"; }

	when ( "TradeWPrice" ) { $price_string_ = "Twp"; }

	default { $price_string_ = ""; }
    }

    return $price_string_;
}
