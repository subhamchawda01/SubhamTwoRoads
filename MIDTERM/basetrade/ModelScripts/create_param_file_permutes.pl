#!/usr/bin/perl

# \file ModelScripts/create_param_file_permutes.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch";

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_number_from_vec.pl"; # FindNumberFromVec

sub PermuteParamsInt;
sub PermuteParamsDouble;

# start 
my $USAGE="$0 INPUT_PARAM_FILE_NAME OUTPUT_PARAM_FILE_NAME PERMUTE_UP_FACTOR PERMUTE_DOWN_FACTOR";

if ( $#ARGV < 3 )
{
    print $USAGE."\n";
    exit ( 0 );
}

my $input_file_name_ = $ARGV [ 0 ];
my $output_file_name_ = $ARGV [ 1 ];
my $permute_up_factor_ = $ARGV [ 2 ];
my $permute_down_factor_ = $ARGV [ 3 ];

open ( INPUT_FILE , "<" , $input_file_name_ ) or PrintStacktraceAndDie ( "Could not open $input_file_name_" );
my @input_file_lines_ = <INPUT_FILE>; chomp ( @input_file_lines_ );
close ( INPUT_FILE );

open ( OUTPUT_FILE , ">" , $output_file_name_ ) or PrintStacktraceAndDie ( "Could not open $output_file_name_" );

foreach my $input_line_ ( @input_file_lines_ )
{
    my @input_line_words_ = split ( ' ' , $input_line_ );
    my @permuted_input_line_words_ = @input_line_words_;

    if ( index ( $input_line_words_ [ 0 ] , "#" ) >= 0 )
    {
	next;
    }

    given ( $input_line_words_ [ 1 ] )
    {
	when ( "WORST_CASE_UNIT_RATIO" )
	{
	    @permuted_input_line_words_ = PermuteParamsInt ( @input_line_words_ );
	}
	when ( "MAX_UNIT_RATIO" )
	{
	    @permuted_input_line_words_ = PermuteParamsInt ( @input_line_words_ );
	}
	when ( "HIGHPOS_LIMITS" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "HIGHPOS_THRESH_FACTOR" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "HIGHPOS_THRESH_DECREASE" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "HIGHPOS_SIZE_FACTOR" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "ZEROPOS_LIMITS" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "ZEROPOS_KEEP" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "PLACE_KEEP_DIFF" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "INCREASE_ZEROPOS_DIFF" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "ZEROPOS_DECREASE_DIFF" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "INCREASE_PLACE" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "INCREASE_KEEP" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "ZEROPOS_PLACE" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "DECREASE_PLACE" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
	when ( "DECREASE_KEEP" )
	{
	    @permuted_input_line_words_ = PermuteParamsDouble ( @input_line_words_ );
	}
    }

    print OUTPUT_FILE join ( ' ' , @permuted_input_line_words_ )."\n";
}

close ( OUTPUT_FILE );

exit ( 0 );

sub PermuteParamsInt
{
    my ( @input_line_words_ ) = @_;

    my @permuted_input_line_words_ = @input_line_words_;

    my @numeric_values_ = ( );
    for ( my $i = 2 ; $i <= $#permuted_input_line_words_ ; $i ++ )
    {
	push ( @numeric_values_ , $permuted_input_line_words_ [ $i ] );
    }

    if ( $permute_up_factor_ > 0.0 )
    {
	my $up_permutation_ = sprintf ( "%d" , ( $input_line_words_ [ 2 ] * ( 1 + $permute_up_factor_ ) ) );

	if ( ! FindNumberFromVec ( $up_permutation_ , @numeric_values_ ) )
	{
	    push ( @permuted_input_line_words_ , $up_permutation_ );
	}
    }
    if ( $permute_down_factor_ > 0.0 )
    {
	my $down_permutation_ = sprintf ( "%d" , ( $input_line_words_ [ 2 ] * ( 1 - $permute_down_factor_ ) ) );

	if ( ! FindNumberFromVec ( $down_permutation_ , @numeric_values_ ) )
	{
	    push ( @permuted_input_line_words_ , $down_permutation_ );
	}
    }

    return @permuted_input_line_words_;
}

sub PermuteParamsDouble
{
    my ( @input_line_words_ ) = @_;

    my @permuted_input_line_words_ = @input_line_words_;

    my @numeric_values_ = ( );
    for ( my $i = 2 ; $i <= $#permuted_input_line_words_ ; $i ++ )
    {
	push ( @numeric_values_ , $permuted_input_line_words_ [ $i ] );
    }

    if ( $permute_up_factor_ > 0.0 )
    {
	my $up_permutation_ = sprintf ( "%0.3f" , ( $input_line_words_ [ 2 ] * ( 1 + $permute_up_factor_ ) ) );

	if ( ! FindNumberFromVec ( $up_permutation_ , @numeric_values_ ) )
	{
	    push ( @permuted_input_line_words_ , $up_permutation_ );
	}
    }
    if ( $permute_down_factor_ > 0.0 )
    {
	my $down_permutation_ = sprintf ( "%0.3f" , ( $input_line_words_ [ 2 ] * ( 1 - $permute_down_factor_ ) ) );

	if ( ! FindNumberFromVec ( $down_permutation_ , @numeric_values_ ) )
	{
	    push ( @permuted_input_line_words_ , $down_permutation_ );
	}
    }

    return @permuted_input_line_words_;
}
