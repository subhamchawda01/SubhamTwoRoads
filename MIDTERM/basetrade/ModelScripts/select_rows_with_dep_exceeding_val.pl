#!/usr/bin/perl

# \file ModelScripts/select_rows_with_dep_exceeding_val.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# R_REG_DATA_INPUT_FILENAME  MIN_VALUE_TO_FILTER  RW_REG_DATA_OUTPUT_FILENAME

use strict;
use warnings;
use List::Util qw/max min/; # for max

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0  R_REG_DATA_INPUT_FILENAME  MIN_VALUE_TO_FILTER  RW_REG_DATA_OUTPUT_FILENAME [NUM_COLS = 1]";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $r_reg_data_input_filename_ = $ARGV[0];
my $min_value_to_filter_ = $ARGV[1];
my $rw_reg_data_output_filename_ = $ARGV[2];

my $num_preds_ = 1;
if ( $#ARGV >= 3 ) { $num_preds_ = $ARGV[3]; }

my $lastcolindex_ = -1; # primarily used to check 

open ( OUTFILE, "> $rw_reg_data_output_filename_" ) or PrintStacktraceAndDie ( "Could not open file $rw_reg_data_output_filename_ for writing\n" );

open ( INFILE, "< $r_reg_data_input_filename_" ) or PrintStacktraceAndDie ( " Could not open $r_reg_data_input_filename_ for reading!\n" );

while ( my $inline_ = <INFILE> ) 
{
    my @words_ = split ( ' ', $inline_ );
    if ( $lastcolindex_ == -1 ) 
    { #first iteration
	$lastcolindex_ = $#words_;
    }

    if ( $#words_ >= $lastcolindex_ ) 
    { 
        my $val_ = 1;
        for ( my $i=0; $i < $num_preds_; $i++ )
	{
	    if ( abs ( $words_[$i] ) <= $min_value_to_filter_ )
	    {
    		$val_ = 0;
	    }
	}
	if ( $val_ == 1 )
	{
	    printf OUTFILE "%s", $inline_;
	}
    }
}
close ( INFILE );
close ( OUTFILE );


