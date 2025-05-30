#!/usr/bin/perl

# \file ModelScripts/union_bbg_fxs.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551

use strict;
use warnings;
use List::Util qw/max min/; # for max min
# use feature "switch"; # for given, when
# use File::Basename; # for basename and dirname
# use File::Copy; # for copy

sub GetEpochFromLine ;
sub AdjustBBGText;
sub AdjustFXSText;

#my $SPARE_DIR="/spare/local/basetrade/";
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 bbg_file fxs_file output_file";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $bbg_file_ = $ARGV[0];
my $fxs_file_ = $ARGV[1];
my $output_filename_ = $ARGV[2];

# these are the arrays that will hold items that have not been merged yet
my @bbg_current_epoch_lines_ = ();
my @fxs_current_epoch_lines_ = ();

my @bbg_yet_to_be_merged_lines_ = ();
my @fxs_yet_to_be_merged_lines_ = ();

# my @merged_lines_ = ();

my $earliest_unmerged_epoch_ = 0;

open BBG_FILE_HANDLE, "< $bbg_file_ " or PrintStacktraceAndDie ( "Could not open $bbg_file_\n" );

while ( my $thisline_ = <BBG_FILE_HANDLE> )
{
    chomp ( $thisline_ );
    push ( @bbg_yet_to_be_merged_lines_, $thisline_ );
}

close ( BBG_FILE_HANDLE );

if ( $#bbg_yet_to_be_merged_lines_ < 0 )
{
    PrintStacktraceAndDie ( "No BBG lines read\n" );
}

open FXS_FILE_HANDLE, "< $fxs_file_ " or PrintStacktraceAndDie ( "Could not open $fxs_file_\n" );

while ( my $thisline_ = <FXS_FILE_HANDLE> )
{
    chomp ( $thisline_ );
    push ( @fxs_yet_to_be_merged_lines_, $thisline_ );
}

close ( FXS_FILE_HANDLE );

open OUTPUT_FILE_HANDLE, "> $output_filename_ " or PrintStacktraceAndDie ( "Could not open $output_filename_ for writing\n" );

if ( $#fxs_yet_to_be_merged_lines_ < 0 )
{
    PrintStacktraceAndDie ( "No FXS lines read\n" );
}

$earliest_unmerged_epoch_ = min ( GetEpochFromLine ( $bbg_yet_to_be_merged_lines_[0] ), GetEpochFromLine ( $fxs_yet_to_be_merged_lines_[0] ) );

while ( 1 )
{
    while ( $#bbg_yet_to_be_merged_lines_ >= 0 )
    {
	# print "D\n";
	if ( $bbg_yet_to_be_merged_lines_[0] =~ / USD / )
	{
	    # print "E\n";
	    if ( GetEpochFromLine ( $bbg_yet_to_be_merged_lines_[0] ) <= $earliest_unmerged_epoch_ )
	    {
		# print "B\n";
		push ( @bbg_current_epoch_lines_, AdjustBBGText ( $bbg_yet_to_be_merged_lines_[0] ) ) ;
		shift ( @bbg_yet_to_be_merged_lines_ );
	    }
	    else
	    {
		# print "U\n";
		last;
	    }
	}
	else
	{
	    # print "G\n";
	    printf OUTPUT_FILE_HANDLE "%s\n", $bbg_yet_to_be_merged_lines_[0];
	    shift ( @bbg_yet_to_be_merged_lines_ );	    
	}
    }

    while ( $#fxs_yet_to_be_merged_lines_ >= 0 )
    {
	# print "d\n";
	if ( $fxs_yet_to_be_merged_lines_[0] =~ / USD / )
	{
	    # print "e\n";
	    if ( GetEpochFromLine ( $fxs_yet_to_be_merged_lines_[0] ) <= $earliest_unmerged_epoch_ )
	    {
		my $adjusted_line_ = AdjustFXSText ( $fxs_yet_to_be_merged_lines_[0] );
		# print "b $adjusted_line_\n";
		push ( @fxs_current_epoch_lines_, $adjusted_line_ ) ;
		shift ( @fxs_yet_to_be_merged_lines_ );
		# print "resolve $#fxs_current_epoch_lines_ $#bbg_current_epoch_lines_\n";
	    }
	    else
	    {
		# print "u\n";
		last;
	    }
	}
	else
	{
	    # print "g\n";
	    printf OUTPUT_FILE_HANDLE "%s\n", $fxs_yet_to_be_merged_lines_[0];
	    shift ( @fxs_yet_to_be_merged_lines_ );	    
	}
    }

    # print "resolve $#fxs_current_epoch_lines_ $#bbg_current_epoch_lines_\n";

    if ( $#bbg_current_epoch_lines_ < 0 )
    {
	while ( $#fxs_current_epoch_lines_ >= 0 )
	{
	    printf OUTPUT_FILE_HANDLE "%s\n", $fxs_current_epoch_lines_[0] ;
	    shift ( @fxs_current_epoch_lines_ );
	}
    }

    # print "resolve $#fxs_current_epoch_lines_ $#bbg_current_epoch_lines_\n";

    if ( $#fxs_current_epoch_lines_ < 0 )
    {
	while ( $#bbg_current_epoch_lines_ >= 0 )
	{
	    # printf "%s\n", $bbg_current_epoch_lines_[0] ;
	    printf OUTPUT_FILE_HANDLE "%s\n", $bbg_current_epoch_lines_[0] ;
	    shift ( @bbg_current_epoch_lines_ );
	}
    }

    # MergeLines
    if ( ( $#bbg_current_epoch_lines_ >= 0 ) &&
	 ( $#fxs_current_epoch_lines_ >= 0 ) )
    {
	# my @sorted_bbg_current_epoch_lines_ = sort ( @bbg_current_epoch_lines_ );
	my @sorted_fxs_current_epoch_lines_ = sort ( @fxs_current_epoch_lines_ );
	
	# for ( my $i = 0 ; $i <= $#sorted_bbg_current_epoch_lines_ ; $i ++ )
	# {
	#     printf "BBG : %s\n", $sorted_bbg_current_epoch_lines_[$i];
	# }
	# for ( my $i = 0 ; $i <= $#sorted_fxs_current_epoch_lines_ ; $i ++ )
	# {
	#     printf "FXS : %s\n", $sorted_fxs_current_epoch_lines_[$i];
	# }

	while ( $#sorted_fxs_current_epoch_lines_ >= 0 )
	{
	    printf OUTPUT_FILE_HANDLE "%s\n", $sorted_fxs_current_epoch_lines_[0];
	    shift ( @sorted_fxs_current_epoch_lines_ );	    
	}

	while ( $#bbg_current_epoch_lines_ >= 0 )
	{
	    shift ( @bbg_current_epoch_lines_ );	    
	}
	while ( $#fxs_current_epoch_lines_ >= 0 )
	{
	    shift ( @fxs_current_epoch_lines_ );	    
	}
    }

    if ( ( $#bbg_current_epoch_lines_ < 0 ) &&
	 ( $#fxs_current_epoch_lines_ < 0 ) )
    {

	if ( $#fxs_yet_to_be_merged_lines_ < 0 )
	{
	    while ( $#bbg_yet_to_be_merged_lines_ >= 0 )
	    {
		printf OUTPUT_FILE_HANDLE "%s\n", $bbg_yet_to_be_merged_lines_[0];
		shift ( @bbg_yet_to_be_merged_lines_ );	    
	    }
	}

	if ( $#bbg_yet_to_be_merged_lines_ < 0 )
	{
	    while ( $#fxs_yet_to_be_merged_lines_ >= 0 )
	    {
		printf OUTPUT_FILE_HANDLE "%s\n", $fxs_yet_to_be_merged_lines_[0];
		shift ( @fxs_yet_to_be_merged_lines_ );	    
	    }
	}

	if ( ( $#fxs_yet_to_be_merged_lines_ < 0 ) && 
	     ( $#bbg_yet_to_be_merged_lines_ < 0 ) )
	{
	    last;
	}
	else
	{
	    $earliest_unmerged_epoch_ = min ( GetEpochFromLine ( $bbg_yet_to_be_merged_lines_[0] ), GetEpochFromLine ( $fxs_yet_to_be_merged_lines_[0] ) );
	}
    }
}

close ( OUTPUT_FILE_HANDLE );

exit ( 0 );

sub AdjustBBGText
{
    my $this_bbg_line_ = shift;
    $this_bbg_line_ =~ s/Continuing_Claims/Continuing_Jobless_Claims/g;
    $this_bbg_line_ =~ s/Continuing_Claims/Continuing_Jobless_Claims/g;
    $this_bbg_line_ =~ s/Construction_Spending_MoM/Construction_Spending_(MoM)/g;
    $this_bbg_line_ =~ s/Avg_Hourly_Earning_YOY_All_Emp/Average_Hourly_Earnings_(YoY)/g;
    $this_bbg_line_ =~ s/Avg_Hourly_Earning_MOM_All_Emp/Average_Hourly_Earnings_(MoM)/g;
    $this_bbg_line_ =~ s/Avg_Weekly_Hours_All_Employees/Avg_Weekly_Hours/g;
    $this_bbg_line_;
}

sub AdjustFXSText
{
    my $this_fxs_line_ = shift;
    $this_fxs_line_ =~ s/FOMC_Minutes/Minutes_of_FOMC_Meeting/g;
    $this_fxs_line_;
}

sub GetEpochFromLine
{
    my $thisline_ = shift;
    my @this_words_ = split ( ' ', $thisline_ );
    $this_words_[0];
}
