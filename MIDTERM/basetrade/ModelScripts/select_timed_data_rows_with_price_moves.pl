#!/usr/bin/perl

# \file ModelScripts/select_timed_data_rows_with_price_moves.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# TIMED_DATA_INPUT_FILENAME TIMED_DATA_OUTPUT_FILENAME DURATION_MSECS

use strict;
use warnings;
use List::Util qw/max min/; # max , min
use Math::Complex; # sqrt
use Carp; # Carp

use constant DEBUG => 0;
use constant SUMMARY => 1;
use constant COMPUTE_AVG_PRICE_MOVES_PER_SEC => 0;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_min_price_increment_for_shortcode.pl"; # GetMinPriceIncrementForShortcode
require "$GENPERLLIB_DIR/get_avg_price_moves_per_sec_for_shortcode.pl"; # GetAvgPriceMovesPerSecForShortcode
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub ComputeAveragePriceMoveStats;
sub PrintHighPriceMovePeriodRows;

# start 
my $USAGE="$0 SHORTCODE TIMED_DATA_INPUT_FILENAME TIMED_DATA_OUTPUT_FILENAME [ DURATION_SECS = 120 ] [ THRESH_FACTOR = 2 ]";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];

my $MIN_PRICE_INCREMENT = GetMinPriceIncrementForShortcode ( $shortcode_ );

if ( DEBUG ) { print "GetMinPriceIncrementForShortcode for $shortcode_ = $MIN_PRICE_INCREMENT\n"; }

my $timed_data_input_filename_ = $ARGV [ 1 ];
my $timed_data_output_filename_ = $ARGV [ 2 ];
my $duration_secs_ = 120;
my $thresh_multiplier_ = 3;

if ( $#ARGV >= 3 ) { $duration_secs_ = $ARGV [ 3 ]; }
if ( $#ARGV >= 4 ) { $thresh_multiplier_ = $ARGV [ 4 ]; }

open ( OUTFILE , "> $timed_data_output_filename_" ) or PrintStacktraceAndDie ( "Could not open file $timed_data_output_filename_ for writing\n" );
open ( INFILE , "< $timed_data_input_filename_" ) or PrintStacktraceAndDie ( "Could not open file $timed_data_output_filename_ for reading\n" );

my @infile_lines_ = <INFILE>;
close ( INFILE );

# Use pre-computed R = ( Price_Moves_ / Total_Seconds_ ) across last 20 days.
my $avg_price_moves_per_sec_ = GetAvgPriceMovesPerSecForShortcode ( $shortcode_ );
if ( COMPUTE_AVG_PRICE_MOVES_PER_SEC )
{
    my ( $apmps_ , $max_ , $min_ , $pmc_ , $ts_ ) = ComputeAveragePriceMoveStats ( 0 , $#infile_lines_ , @infile_lines_ );
    close ( OUTFILE );
    exit ( 0 );
}

my $thresh_factor_ = $thresh_multiplier_ * sqrt ( $duration_secs_ * $avg_price_moves_per_sec_ );

if ( DEBUG || SUMMARY ) { print "$shortcode_ MIN_PRICE_INCREMENT = $MIN_PRICE_INCREMENT avg_price_moves_per_sec = $avg_price_moves_per_sec_ thresh_factor_ = $thresh_factor_\n"; }

# Find periods where ( ( Pmax - Pmin ) / T ) > 2 * sqrt ( ( T * R ) ) * ThreshFactor
PrintHighPriceMovePeriodRows ( $avg_price_moves_per_sec_ , $thresh_factor_ , @infile_lines_ );

close ( OUTFILE );

exit ( 0 );

sub PrintHighPriceMovePeriodRows ( )
{
    my ( $t_avg_price_moves_per_sec_ , $t_thresh_factor_ , @t_infile_lines_ ) = @_;

    my $period_count_ = 0;
    my $price_move_period_count_ = 0;

    for ( my $i = 0 ; $i <= $#t_infile_lines_ ; $i ++ )
    {
	my $start_index_ = $i;
	my $stop_index_ = $i;

	my $start_msecs_ = -1;
	for ( $stop_index_ = $start_index_ ; $stop_index_ < $#t_infile_lines_ ; $stop_index_ ++ )
	{
	    my $inline_ = $t_infile_lines_ [ $stop_index_ ];

	    my @timed_data_words_ = split ( ' ' , $inline_ );

	    if ( $#timed_data_words_ < 3 ) { PrintStacktraceAndDie ( "Malformed timed_data_line_ : $inline_\n" ); }

	    my $t_msecs_ = $timed_data_words_ [ 0 ];

	    if ( $start_msecs_ == -1 ) { $start_msecs_ = $t_msecs_; next; }

	    # Msecs turned over , break on last index.
	    if ( $t_msecs_ < $start_msecs_ ) { $stop_index_ --; last; }

	    if ( ( $t_msecs_ - $start_msecs_ ) >= ( $duration_secs_ * 1000 ) )
	    {
		last;
	    }
	}

	# For the range [ $start_index_ , $stop_index_ ] compute stats.
	my ( $t_a_p_m_p_s_ , $t_max_px_ , $t_min_px_ , $t_total_price_moves_ , $t_total_secs_ ) = ComputeAveragePriceMoveStats ( $start_index_ , $stop_index_ , @t_infile_lines_ );

	# Find if this time period qualifies as "high price move"
	my $t_max_swing_ = ( ( $t_max_px_ - $t_min_px_ ) / $MIN_PRICE_INCREMENT );

	if ( DEBUG ) { print "Comparing $t_max_swing_ against $t_thresh_factor_\n"; }

	if ( $t_max_swing_ > $t_thresh_factor_ )
	{
	    for ( my $j = $start_index_ ; $j <= $stop_index_ ; $j ++ )
	    {
		print OUTFILE $t_infile_lines_ [ $j ];
	    }

	    if ( DEBUG ) { print "PICKED\n"; }

	    $price_move_period_count_ ++;
	}

	$period_count_ ++;

	$i = $stop_index_;
    }

    if ( SUMMARY )
    {
	# Print Summary
	print "$price_move_period_count_ / $period_count_ $duration_secs_ sec periods labelled LARGE_MOVE\n";
    }

    return 0;
}

sub ComputeAveragePriceMoveStats ( )
{
    my ( $start_index_ , $end_index_ , @t_infile_lines_ ) = @_;

    my $t_avg_price_moves_per_sec_ = 0.0;

    my $price_move_count_ = 0;
    my $total_secs_ = 0.0;

    my $max_px_ = 0;
    my $min_px_ = 10000000;

    my $last_px_ = 0.0;
    my $last_msecs_ = 0;

    if ( DEBUG ) { print "Computing stats for $start_index_ - $end_index_\n"; }
    for ( my $i = $start_index_ ; $i <= $end_index_ ; $i ++ )
    {
	my $inline_ = $t_infile_lines_ [ $i ];

	my @timed_data_words_ = split ( ' ' , $inline_ );

	if ( $#timed_data_words_ < 3 ) { PrintStacktraceAndDie ( "Malformed timed_data_line_ : $inline_\n" ); }

	my $t_msecs_ = $timed_data_words_ [ 0 ];
	my $t_px_ = $timed_data_words_ [ 2 ];

	$max_px_ = max ( $t_px_ , $max_px_ );
	$min_px_ = min ( $t_px_ , $min_px_ );

	# Catted timed data file , msecs turned over.
	if ( $t_msecs_ < $last_msecs_ || $last_px_ == 0.0 ) 
	{ 
	    $last_msecs_ = $t_msecs_; 
	    $last_px_ = $t_px_; 

	    $max_px_ = 0;
	    $min_px_ = 10000000;

	    next; 
	}

	if ( int ( $t_px_ / $MIN_PRICE_INCREMENT ) != int ( $last_px_ / $MIN_PRICE_INCREMENT ) ) 
	{ 
	    $price_move_count_ += abs ( int ( ( $t_px_ - $last_px_ ) / $MIN_PRICE_INCREMENT ) );
	    $last_px_ = $t_px_;
	}

	$total_secs_ += ( ( $t_msecs_ - $last_msecs_ ) / 1000.0 );
	$last_msecs_ = $t_msecs_;
    }

    $t_avg_price_moves_per_sec_ = $price_move_count_ / ( $total_secs_ + 0.001 );
    my $t_avg_secs_per_price_move_ = $total_secs_ / ( $price_move_count_ + 0.001 );

    if ( DEBUG || COMPUTE_AVG_PRICE_MOVES_PER_SEC ) { print "Price_Moves_ : $price_move_count_ Total_Seconds_ : $total_secs_ Avg P/S : $t_avg_price_moves_per_sec_ Avg S/P : $t_avg_secs_per_price_move_ Max : $max_px_ Min $min_px_\n"; }

    return ( $t_avg_price_moves_per_sec_ , $max_px_ , $min_px_ , $price_move_count_ , $total_secs_ );
}
