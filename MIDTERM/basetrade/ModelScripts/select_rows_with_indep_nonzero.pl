#!/usr/bin/perl
# This script takes :
# R_REG_DATA_INPUT_FILENAME  MIN_STDEV_MULTIPLIER  RW_REG_DATA_OUTPUT_FILENAME

use strict;
use warnings;
use Math::Complex ; # sqrt
use List::Util qw/max min/;    # for max

my $USER           = $ENV{'USER'};
my $HOME_DIR       = $ENV{'HOME'};
my $REPO           = "basetrade";
my $GENPERLLIB_DIR = $HOME_DIR . "/" . $REPO . "_install/GenPerlLib";
require "$GENPERLLIB_DIR/print_stacktrace.pl"
  ;                            # PrintStacktrace , PrintStacktraceAndDie

# start
my $USAGE="$0 R_REG_DATA_INPUT_FILENAME RW_REG_DATA_OUTPUT_FILENAME [tolerance]";

if ( $#ARGV < 1 ) { print $USAGE. "\n"; exit(0); }
my $r_reg_data_input_filename_   = $ARGV[0];
my $rw_reg_data_output_filename_ = $ARGV[1];
my $tolerance_ = 0.1; #fraction of non zero independents allowed
if ( $#ARGV > 1 )
{
	$tolerance_ = $ARGV[2];
}

open( OUTFILE, "> $rw_reg_data_output_filename_" )
  or PrintStacktraceAndDie(
	"Could not open file $rw_reg_data_output_filename_ for writing\n");

open( INFILE, "< $r_reg_data_input_filename_" )
  or PrintStacktraceAndDie(
	" Could not open $r_reg_data_input_filename_ for reading!\n");

my $okay_to_print_ = 1;
while ( my $inline_ = <INFILE> ) {
	my @words_ = split( ' ', $inline_ );
	my $t_zero_count_indep_ = 0;
	for (my $i = 1; $i<=$#words_ ;$i++)
	{
		if ( $words_[$i] == 0 )
		{
			$t_zero_count_indep_++;
		}
	}
	my $t_max_zeros_allowed_ = $tolerance_ * $#words_;
	if ( $t_zero_count_indep_ <= $t_max_zeros_allowed_ )
	{
		$okay_to_print_ = 1;
	}
	else
	{
		$okay_to_print_ = 0;
	}

	if ($okay_to_print_) {
		printf OUTFILE "%s", $inline_;
	}
	
}

close(INFILE);
close(OUTFILE);

