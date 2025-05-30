#!/usr/bin/perl

# \file ModelScripts/scale_reg_data_udm.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite 217, Level 2, Prestige Omega,
#	 No 104, EPIP Zone, Whitefield,
#	 Bangalore - 560066, India
#	 +91 80 4060 0717
#
# This script takes :
# R_REG_DATA_INPUT_FILENAME  RW_REG_DATA_OUTPUT_FILENAME

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0  R_REG_DATA_INPUT_FILENAME  RW_REG_DATA_OUTPUT_FILENAME";
if ( $#ARGV < 1 ) 
{ 
    print $USAGE."\n"; exit ( 0 ); 
}

my $r_reg_data_input_filename_ = $ARGV[0];
my $rw_reg_data_output_filename_ = $ARGV[1];

open ( OUTFILE, "> $rw_reg_data_output_filename_" ) or PrintStacktraceAndDie ( "Could not open file $rw_reg_data_output_filename_ for writing\n" );
open ( INFILE, "< $r_reg_data_input_filename_" ) or PrintStacktraceAndDie ( " Could not open $r_reg_data_input_filename_ for reading!\n" );

while ( my $inline_ = <INFILE> ) 
{
    my @words_ = split ( ' ', $inline_ );

    foreach my $x ( @words_ )
    {
	$x = $x * abs ( $words_[0] );
    }

    my $outline_ = join ( ' ', @words_ );
   printf OUTFILE "%s\n", $outline_;
}

close ( INFILE );
close ( OUTFILE );
