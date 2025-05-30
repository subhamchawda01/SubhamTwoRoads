#!/usr/bin/perl

# \file ModelScripts/plot_rolling_correlation_instruments.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 351, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
#
# SHORTCODE1
# SHORTCODE2
# PRICE1
# PRICE2
# WINDOW_SIZE
# START_DATE
# END_DATE
# START_TIME
# END_TIME
# PRODUCT_TYPES (can take 4 values 00, 01, 10, 11)
# Plots rolling correlation between two instruments.

use strict;
use warnings;
use Class::Struct;

# declare the struct
struct ( 'Pair', { x_ => '$', y_ => '$' } );

my $USER = $ENV{'USER'};
#print $USER."\n";

my $HOME_DIR = $ENV{'HOME'};
#print $HOME_DIR."\n"; 

my $SPARE_HOME = "/spare/local/".$USER."/";
#print $SPARE_HOME."\n";

my $WORK_DIR = $SPARE_HOME."PRCI/";
#print $WORK_DIR."\n";

my $REPO = "basetrade";
#print $REPO."\n";

my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin/";
#print $BIN_DIR."\n";

my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
#print $GENPERLLIB_DIR."\n";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; #CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday

my $USAGE="$0 SHORTCODE1 SHORTCODE2 PRICE1 PRICE2 WINDOW_SIZE(IN SEC) START_DATE END_DATE START_TIME END_TIME PRODUCT_TYPES";
#print $#ARGV."\n";

if ( $#ARGV+1 < 10 ) { 
	print "USAGE: ".$USAGE."\n"; 
	exit ( 0 ); 
}

my $shortcode1 = $ARGV[0];
my $shortcode2 = $ARGV[1];
my $price1 = $ARGV[2];
my $price2 = $ARGV[3];
my $window_size = $ARGV[4];
my $start_date = $ARGV[5];
my $end_date = $ARGV[6];
my $start_time = $ARGV[7];
my $end_time = $ARGV[8];
my $product_types = $ARGV[9];

my $window_size_msecs = $window_size * 1000;

my $is_portfolio_1 = 0;
my $is_portfolio_2 = 0;
if ( $product_types == "01" ) {
	$is_portfolio_2 = 1;
}
elsif ( $product_types == "10" ) {
	$is_portfolio_1 = 1;
}
elsif ( $product_types == "11" ) {
	$is_portfolio_1 = 1;
	$is_portfolio_2 = 1;
}

my $unique_gsm_id_ = `date +%N`;
print $unique_gsm_id_;

chomp ( $unique_gsm_id_ ); # Removes newline character at the end of the string

my $work_dir_ = $WORK_DIR.$unique_gsm_id_;
#print $work_dir_."\n";

for ( my $i = 0 ; $i < 30 ; $i++ ) {
    if ( -d $work_dir_ ) {
		print STDERR "Surprising but this dir exists\n";
		$unique_gsm_id_ = `date +%N`; 
		chomp ( $unique_gsm_id_ );
		$work_dir_ = $WORK_DIR.$unique_gsm_id_; 
    }
    else {
		last;
    }
}

`mkdir -p $work_dir_`;

my $ilist = $work_dir_."/ilist";
#print $ilist."\n";

open FILE, ">", $ilist or PrintStacktraceAndDie ( "Could not write to file : $ilist\n" );
print FILE "MODELINIT DEPBASE"." "."NONAME"."\n";
print FILE "MODELMATH LINEAR CHANGE\n";
print FILE "INDICATORSTART\n";
if ( $is_portfolio_1 == 0 ) {
	print FILE "INDICATOR 1.00 SimpleTrend"." ".$shortcode1." ".$window_size." ".$price1."\n";
}
else {	
	print FILE "INDICATOR 1.00 SimpleTrendPort"." ".$shortcode1." ".$window_size." ".$price1."\n";
}
if ( $is_portfolio_2 == 0 ) {
	print FILE "INDICATOR 1.00 SimpleTrend"." ".$shortcode2." ".$window_size." ".$price2."\n";
}
else {
	print FILE "INDICATOR 1.00 SimpleTrendPort"." ".$shortcode2." ".$window_size." ".$price2."\n";
}
print FILE "INDICATOREND\n"; 
close FILE;

my $correlation = $work_dir_."/correlation";
#print $correlation."\n";

open FILE1, ">", $correlation or PrintStacktraceAndDie ( "Could not write to file : $correlation\n" );

my $current_date = $start_date;
my $days_count = 1;

my $sum_corr = 0;
my $data_days = 0;

while ()
{
	# TODO: Find the differences.
	if ( SkipWeirdDate( $current_date ) ||
		IsDateHoliday( $current_date ) ||
		IsProductHoliday( $current_date, $shortcode1 ) ||
		IsProductHoliday( $current_date, $shortcode2 ) ||
		NoDataDate( $current_date ) ||
		!ValidDate( $current_date ) ) {		
		print $current_date."\t"."no data\n";
		$current_date = CalcNextWorkingDateMult ( $current_date, 1 );
		next;
	}

	if ( $current_date > $end_date ) {
		last;
	}

	my $datagen_output = $work_dir_."/datagen_output";
	#print $datagen_output."\n";

	my $exec_datagen = $BIN_DIR."datagen"." ".$ilist." ".$current_date." ".$start_time." ".$end_time." "."5000"." ".$datagen_output." ".int ( $window_size_msecs / 5 )." "."0 0 0\n";
	#print $exec_datagen."\n";

	`$exec_datagen`;

	open FILE2, "<", $datagen_output or PrintStacktraceAndDie ( "Could not read from file : $datagen_output\n" );

	my @points_vec = ();

	while ( my $line = <FILE2> ) {
		my @words = split ( ' ', $line );
		my $point = Pair->new ( x_=> $words[2], y_ => $words[3] );
		push ( @points_vec, $point );
	}
	
	close FILE2;

	if ( $#points_vec + 1 > 0 ) {
		my $corr = get_correlation ( @points_vec );
		if ( $corr != 100 ) {
			print $current_date."\t".$corr."\n";
			$sum_corr = $sum_corr+$corr;
			$data_days = $data_days+1;
			print FILE1 $days_count."\t".$corr."\n";
		}
		else {
			print $current_date."\t"."no data\n";
		}
	}
	else {
		print $current_date."\t"."no data\n";
	}

	$days_count++;
	$current_date = CalcNextWorkingDateMult ( $current_date, 1 );
};

my $avg_corr = $sum_corr/$data_days;
print $avg_corr."\n";

close FILE1;

my $gnuplot = $work_dir_."/gnuplot";
open FILE, ">", $gnuplot or PrintStacktraceAndDie ( "Could not write to file : $gnuplot\n" );
print FILE "set title \"Rolling Correlation"." (".$shortcode1."_".$shortcode2."_".$window_size."_".$start_date."_".$end_date."_".$start_time."_".$end_time.")"."\"\n";
print FILE "set xlabel \"Time (in days)\"\n";
print FILE "set ylabel \"Correlation\"\n";
print FILE "plot \"$correlation\" with lines, \\"."\n";
print FILE "\"$correlation\" smooth bezier lw 2 lt rgb \"blue\"\n";
print FILE "set terminal png size 1600,960 enhanced font \"Helvetica,20\"\n";
print FILE "set output \"".$shortcode1."_".$shortcode2.".png\"\n";
print FILE "replot\n";
print FILE "set term x11\n";
print FILE "replot\n";
print FILE "pause -1 \"Hit any key to continue\"\n";
close FILE;

`gnuplot $gnuplot`;
`rm -rf $work_dir_`;

sub get_correlation
{
	my @points_vec = @_;
	my $num_points = $#points_vec + 1;

	# Remove 1% of outliers.

	my $num_remove_one_side = int ( $num_points / 400 );

	@points_vec = sort { return $a->x_ <=> $b->x_; } @points_vec;

	@points_vec = splice ( @points_vec, $num_remove_one_side, $#points_vec + 1 - 2 * $num_remove_one_side );

	@points_vec = sort { return $a->y_ <=> $b->y_; } @points_vec;
	
	@points_vec = splice ( @points_vec, $num_remove_one_side, $#points_vec + 1 - 2 * $num_remove_one_side );

	my $mean_x = 0;
	my $mean_y = 0;

	for ( my $i = 0; $i <= $#points_vec; $i++ ) {
		$mean_x = $mean_x + $points_vec[$i]->x_;
		$mean_y = $mean_y + $points_vec[$i]->y_;
	}

	$mean_x /= ( $#points_vec + 1 );
	$mean_y /= ( $#points_vec + 1 );

	my $std_x = 0;
	my $std_y = 0;

	my $num = 0;

	for ( my $i = 0; $i <= $#points_vec; $i++) {
		$std_x = $std_x + ( $points_vec[$i]->x_ - $mean_x ) ** 2;
		$std_y = $std_y + ( $points_vec[$i]->y_ - $mean_y ) ** 2;

		$num = $num + ( $points_vec[$i]->x_ - $mean_x ) * ( $points_vec[$i]->y_ - $mean_y );
	}

	$std_x /= ( $#points_vec + 1 );
	$std_x = sqrt ( $std_x );

	$std_y /= ( $#points_vec + 1 );
	$std_y = sqrt ( $std_y );

	$num /= ( $#points_vec + 1 );
	
	if ( $std_x == 0 || $std_y == 0 ) {
		return 100; # Invalid Value
	}
	return $num / ( $std_x * $std_y );
}
