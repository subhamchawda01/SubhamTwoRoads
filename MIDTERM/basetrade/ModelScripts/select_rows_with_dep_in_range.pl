#!/usr/bin/perl

# \file ModelScripts/select_rows_with_dep_exceeding_stdev.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# R_REG_DATA_INPUT_FILENAME MIN_STDEV_MULTIPLIER MAX_STDEV_MULTIPLIER RW_REG_DATA_OUTPUT_FILENAME

use strict;
use warnings;
use Math::Complex; # sqrt
use List::Util qw/max min/; # for max

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0  R_REG_DATA_INPUT_FILENAME  MIN_STDEV_MULTIPLIER  MAX_STDEV_MULTIPLIER  RW_REG_DATA_OUTPUT_FILENAME";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $r_reg_data_input_filename_ = $ARGV[0];
my $min_stdev_multiplier_ = $ARGV[1];
my $max_stdev_multiplier_ = $ARGV[2];
my $rw_reg_data_output_filename_ = $ARGV[3];
my $num_preds_ = 1;
if ( $#ARGV >= 4 ) { $num_preds_ = $ARGV[4]; }

my $lastcolindex_ = -1; # primarily used to check 

my @l0_dep_sum_ = ();
my @l1_dep_sum_ = ();
my @l2_dep_sum_ = ();

for ( my $i=0; $i<$num_preds_; $i++ )
{
    push (@l0_dep_sum_, 0 );
    push (@l1_dep_sum_, 0 );
    push (@l2_dep_sum_, 0 );
}

open ( OUTFILE, "> $rw_reg_data_output_filename_" ) or PrintStacktraceAndDie ( "Could not open file $rw_reg_data_output_filename_ for writing\n" );

open ( INFILE, "< $r_reg_data_input_filename_" ) or PrintStacktraceAndDie ( " Could not open $r_reg_data_input_filename_ for reading!\n" );

my $okay_to_print_ = 1;
while ( my $inline_ = <INFILE> ) 
{
    my @words_ = split ( ' ', $inline_ );
    if ( $lastcolindex_ == -1 ) 
    { #first iteration
	$lastcolindex_ = $#words_;
    }

    if ( $#words_ >= $lastcolindex_ ) 
    { 
	$okay_to_print_ = 1;
	for ( my $i = 0; $i<$num_preds_; $i++ )
	{
	    $l0_dep_sum_[$i] ++;
	    $l1_dep_sum_[$i] += $words_[$i];
	    $l2_dep_sum_[$i] += $words_[$i] * $words_[$i];

	    if ( $l0_dep_sum_[$i] > 100 )
	    { # online computation of mean and stdev
		my $mean_dep_ = $l1_dep_sum_[$i] / $l0_dep_sum_[$i] ;
		my $var_dep_ = max ( 0.0000000000001, ( ( $l2_dep_sum_[$i] - ( $l1_dep_sum_[$i] * $l1_dep_sum_[$i] / $l0_dep_sum_[$i] ) ) / ( $l0_dep_sum_[$i] - 1 ) ) ) ;
#	    if ( $var_dep_ >= 0.0000000000001 )
		{
		    my $stdev_dep_ = sqrt ( $var_dep_ ) ;
		    my $abs_dev_px_ = abs ( $words_[$i] - $mean_dep_ );
		    if ( ( $abs_dev_px_ < ( $min_stdev_multiplier_ * $stdev_dep_ ) )
			 || ( $abs_dev_px_ > ( $max_stdev_multiplier_ * $stdev_dep_ ) ) )
		    { # if the abs deviation falls out of acceptable band
			# mark this line as unacceptable
			# another options would have been to clip
			$okay_to_print_ = 0;
		    }
		    else
		    {
			$okay_to_print_ = 1;
		    }
		}
	    }

	    if ( $okay_to_print_ == 1 )
	    {
		printf OUTFILE "%s", $inline_;
	    }
	}
    }
}
close ( INFILE );
close ( OUTFILE );

