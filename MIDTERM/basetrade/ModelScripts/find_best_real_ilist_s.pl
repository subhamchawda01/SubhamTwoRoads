#!/usr/bin/perl

# \file ModelScripts/find_best_real_ilist_s.pl
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
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";

require "$GENPERLLIB_DIR/get_real_query_id_prefix_for_shortcode_time_period.pl"; # GetRealQueryIdPrefixForShortcodeTimePeriod

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/get_real_min_pnl_stats_for_shortcode.pl"; # GetRealMinPnlStatsForShortcode

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetGoodQueries;
sub GetIndicatorsForDateQueryId;

sub ScoreIndicators;

sub WriteIlistFilesToDirectory;

sub PriceTypeToString;

my $output_dir_ = $ENV { 'PWD' };

if ( $#ARGV < 0 ) 
{ print "USAGE = SHORTCODE [ ILIST_OUTPUT_DIR = $output_dir_ ] [ NUM_PAST_DAYS = 10 ] [ TIMEPERIOD = US ]\n"; exit ( 0 ); }

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );

my $shortcode_ = $ARGV [ 0 ];
my $num_past_days_ = 10;
my $timeperiod_ = "US";

if ( $#ARGV > 0 ) { $output_dir_ = $ARGV [ 1 ]; }
if ( $#ARGV > 1 ) { $num_past_days_ = $ARGV [ 2 ]; }
if ( $#ARGV > 2 ) { $timeperiod_ = $ARGV [ 3 ]; }

my $query_id_ = GetRealQueryIdPrefixForShortcodeTimePeriod ( $shortcode_ , $timeperiod_ );

if ( $query_id_ eq "" )
{
    print "query id prefix not found for $shortcode_ \n";
    exit ( 0 ) ;
}


my %date_to_good_queries_ = ( );
GetGoodQueries ( $query_id_ , $num_past_days_ );

my %indicator_to_score_ = ( );
my %indicator_to_position_ = ( );
my %indicator_to_corr_ = ( );

foreach my $date_ ( keys ( %date_to_good_queries_ ) )
{
#    print "$date_\n";

    foreach my $qid_ ( keys ( % { $date_to_good_queries_ { $date_ } } ) )
    {
	my $t_pnl_per_pos_ = $date_to_good_queries_ { $date_ } { $qid_ };

#	print "QID = $qid_ $t_pnl_per_contract_\n";

	%indicator_to_position_ = GetIndicatorsForDateQueryId ( $date_ , $qid_ );

	ScoreIndicators ( $t_pnl_per_pos_ , 5 ); # Ignore indicators after the 5th
    }
}

foreach my $indicator_name_ ( sort { $indicator_to_score_ { $b } <=> $indicator_to_score_ { $a } }
			      keys ( %indicator_to_score_ ) )
{
    my $indicator_score_ = $indicator_to_score_ { $indicator_name_ };
}

WriteIlistFilesToDirectory ( $output_dir_ , 30 ); # Write at most 30 indicators to the ilist.

exit ( 0 );

sub GetGoodQueries
{
    my ( $query_id_ , $num_past_days_ ) = @_;

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

	my $t_yyyy_ = substr ( $current_yyyymmdd_ , 0 , 4 );
	my $t_mm_ = substr ( $current_yyyymmdd_ , 4 , 2 );
	my $t_dd_ = substr ( $current_yyyymmdd_ , 6 , 2 );

	my $t_trades_files_ = "/NAS1/logs/QueryTrades/".$t_yyyy_."/".$t_mm_."/".$t_dd_."/trades.".$current_yyyymmdd_.".".$query_id_."??";

	my @trades_file_list_ = `ls $t_trades_files_ 2>/dev/null`;

	for ( my $i = 0 ; $i <= $#trades_file_list_ ; $i ++ )
	{
	    my $t_trades_file_ = $trades_file_list_ [ $i ]; chomp ( $t_trades_file_ );

#	    print $t_trades_file_."\n";

	    my $t_pnl_per_contract_ = `$MODELSCRIPTS_DIR/get_pnl_stats.pl $t_trades_file_ | grep PNL_per_contract | awk '{ print \$3; }'`; chomp ( $t_pnl_per_contract_ );
	    my $t_volume_ = `$MODELSCRIPTS_DIR/get_pnl_stats.pl $t_trades_file_ | grep FinalVolume | awk '{ print \$2; }'`; chomp ( $t_volume_ );
	    my $t_pnl_ = `$MODELSCRIPTS_DIR/get_pnl_stats.pl $t_trades_file_ | grep FinalPNL | awk '{ print \$2; }'`; chomp ( $t_pnl_ );
	    my $t_avg_abs_pos_ = `$MODELSCRIPTS_DIR/get_pnl_stats.pl $t_trades_file_ | grep \"Average Abs position\" | awk '{ print \$4; }'`; chomp ( $t_avg_abs_pos_ );
	    my $t_pnl_per_pos_ = $t_pnl_ / ( $t_avg_abs_pos_ + 0.000001 );

	    my @trades_file_words_ = split ( '\.' , $t_trades_file_ );
	    my $t_qid_ = $trades_file_words_ [ $#trades_file_words_ ]; chomp ( $t_qid_ );

	    my ( $min_real_pnl_per_contract_ , $min_real_volume_ , $min_real_pnl_per_pos_ ) = GetRealMinPnlStatsForShortcode ( $shortcode_ );

	    if ( $t_pnl_per_pos_ > $min_real_pnl_per_pos_ )
	    {
		print "$current_yyyymmdd_ $t_qid_ $t_pnl_ $t_pnl_ $t_volume_ $t_pnl_per_contract_ $t_pnl_per_pos_\n";

#		$date_to_good_queries_ { $current_yyyymmdd_ } { $t_qid_ } = $t_pnl_per_pos_;
		$date_to_good_queries_ { $current_yyyymmdd_ } { $t_qid_ } = 1;
	    }
	}

	$current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
    }

    return;
}

sub GetIndicatorsForDateQueryId
{
    my ( $date_ , $qid_ ) = @_;

    my %indicator_to_position_ = ( );

    my $t_yyyymmdd_ = $date_;
    my $t_yyyy_ = substr ( $t_yyyymmdd_ , 0 , 4 );
    my $t_mm_ = substr ( $t_yyyymmdd_ , 4 , 2 );
    my $t_dd_ = substr ( $t_yyyymmdd_ , 6 , 2 );

    my $t_log_file_ = "/NAS1/logs/QueryLogs/".$t_yyyy_."/".$t_mm_."/".$t_dd_."/log.".$t_yyyymmdd_.".".$qid_;

    my @indicator_list_ = `/usr/bin/zgrep INDICATOR $t_log_file_`;
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

	$ilist_file_name_ = $ilist_file_name_."ilist_".$shortcode_."_".$timeperiod_."_".$t_price_dep_string_."_".$t_price_indep_string_."_s";

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

	default { $price_string_ = ""; }
    }

    return $price_string_;
}
