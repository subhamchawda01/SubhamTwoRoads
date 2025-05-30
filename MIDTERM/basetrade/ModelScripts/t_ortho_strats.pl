#!/usr/bin/perl

# \file ModelScripts/t_model_components.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 351, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
# file(output of summarize_.._long ), min_pnl
# outputs pairs of strats sorted in increasing order of pnl series corr with both strats meeting min_pnl 
# criteria

use strict;
use warnings;

my $USER=$ENV{ 'USER' };
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
 
my $USAGE="$0 pnl_file min_pnl_thresh";

if( $#ARGV < 1 ) { print $USAGE."\n"; exit( 0 ); }

my $pnl_file_ = $ARGV[0];
my $min_pnl_thresh_ = $ARGV[1];


my @strat_names_ = ();
my @dates_vec_ = ();
my @record_count_ = ();
my @pnls_vec_ = ();
my $curr_record_count_ = 0;
my $first_record_ = 1;

open PFILE, "< $pnl_file_" or PrintStacktraceAndDie ( "Could not open $pnl_file_ for reading\n" );

while ( my $line_ = <PFILE> )
{
    chomp( $line_ );
    my @words_ = split( ' ', $line_ );
    my @one_date_vec_ = ();
    my @one_pnl_vec_ = ();
    if( $line_ =~ /STRATEGYFILEBASE/ )
    {
	push( @strat_names_, $words_[1] );
    }
    elsif( $line_ =~ /STATISTICS/ )
    {
	push( @record_count_, $curr_record_count_ );
	$curr_record_count_ = 0;
    }
    elsif( $#words_ > 1 ) 
    {
	if( $words_[0] < 20110101 || $words_[0] > 20991231 )
	{
	    printf "File has a malformed line %s\n", $line_;
	    die;
	}
	else
	{
	    push( @dates_vec_ , $words_[0] );
	    push( @pnls_vec_, $words_[1] );
	    $curr_record_count_ += 1;
	}
    }
}


for( my $counter_1 = 0; $counter_1 < $#strat_names_ - 1; $counter_1++ )
{
    for( my $counter_2 = $counter_1 + 1; $counter_2 < $#strat_names_; $counter_2++ )	
    {
	my $vec_offset_1 = 0;
	my $vec_offset_2 = 0; 
	my $base_vec_offset_1 = 0;
	my $base_vec_offset_2 = 0;
	my $sum_xy = 0;
	my $sum_xx = 0;
	my $sum_yy = 0;
	my $sum_x = 0;
	my $sum_y = 0;
	my $num_points_ = 0;

	for( my $t_counter_ = 0; $t_counter_ < $counter_1; $t_counter_++ )
	{
	    $vec_offset_1 += $record_count_[$t_counter_];
	}
	for( my $t_counter_ = 0; $t_counter_ < $counter_2; $t_counter_++ )
	{
	    $vec_offset_2 += $record_count_[$t_counter_];
	}
	$base_vec_offset_1 = $vec_offset_1;
	$base_vec_offset_2 = $vec_offset_2;
	
	#all this is to deal with situations where #days is not identical for two strats
	while( ( $vec_offset_1 < $base_vec_offset_1 + $record_count_[$counter_1] ) &&
	       ( $vec_offset_2 < $base_vec_offset_2 + $record_count_[$counter_2] ) )
	{
	    if( $dates_vec_[$vec_offset_1] == $dates_vec_[$vec_offset_2] )
	    {
		$sum_xy += $pnls_vec_[$vec_offset_1] * $pnls_vec_[$vec_offset_2];
		$sum_xx += $pnls_vec_[$vec_offset_1] * $pnls_vec_[$vec_offset_1];
		$sum_yy += $pnls_vec_[$vec_offset_2] * $pnls_vec_[$vec_offset_2];
		$sum_x += $pnls_vec_[$vec_offset_1];
		$sum_y += $pnls_vec_[$vec_offset_2];
		$num_points_ += 1;
		$vec_offset_1 += 1;
		$vec_offset_2 += 1;
	    }
	    elsif( $dates_vec_[ $vec_offset_1 ] < $dates_vec_[ $vec_offset_2 ] )
	    {
		#dates are in increasing order in summarize_results
		$vec_offset_1 += 1;
	    }
	    else
	    {
		$vec_offset_2 += 1;
	    }
	}	
	if ( $num_points_ >= 1 && $sum_x > $min_pnl_thresh_ * $num_points_ && $sum_y > $min_pnl_thresh_ * $num_points_ )
	{
	    printf "Strats %s \t %s \t %.2f \n", $strat_names_[$counter_1], $strat_names_[$counter_2], ($sum_xy/$num_points_ - $sum_x/$num_points_*$sum_y/$num_points_ )/( sqrt( $sum_xx/$num_points_ - $sum_x/$num_points_*$sum_x/$num_points_ )* sqrt( $sum_yy/$num_points_ - $sum_y/$num_points_*$sum_y/$num_points_ ) ); 
	}

    }   
}

