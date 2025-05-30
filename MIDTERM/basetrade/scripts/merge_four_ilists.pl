#!/usr/bin/perl

# \file scripts/merge_three_ilists.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes as input :
# key file_with_columns

use strict;
use warnings;

sub ProcLine ;

#my $SPARE_DIR="/spare/local/basetrade/";
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="$0 file1 file2 file3 file4 num1 num2 num3 num4";

if ( $#ARGV < 7 ) { print $USAGE."\n"; exit ( 0 ); }

my $file1_ = $ARGV[0];
my $file2_ = $ARGV[1];
my $file3_ = $ARGV[2];
my $file4_ = $ARGV[3];
my $num1 = $ARGV[4];
my $num2 = $ARGV[5];
my $num3 = $ARGV[6];
my $num4 = $ARGV[7];

my %iline_acorr_map_ ;
my $c1 = 0;
my $c2 = 0;
my $c3 = 0;
my $c4 = 0;

open FILE1H, "< $file1_" or PrintStacktraceAndDie ( "$0 could not open $file1_\n" );

while ( my $txt_line_ = <FILE1H> )
{
    chomp ( $txt_line_ );
    $c1 += ProcLine ( $txt_line_ );
    if ( $c1 >= $num1 )
    {
	last;
    }
}

close FILE1H;
open FILE2H, "< $file2_" or PrintStacktraceAndDie ( "$0 could not open $file2_\n" );

while ( my $txt_line_ = <FILE2H> )
{
    chomp ( $txt_line_ );
    $c2 += ProcLine ( $txt_line_ );
    if ( $c2 >= $num2 )
    {
	last;
    }
}

close FILE2H;
open FILE3H, "< $file3_" or PrintStacktraceAndDie ( "$0 could not open $file3_\n" );

while ( my $txt_line_ = <FILE3H> )
{
    chomp ( $txt_line_ );
    $c3 += ProcLine ( $txt_line_ );
    if ( $c3 >= $num3 )
    {
	last;
    }
}

close FILE3H;

open FILE4H, "< $file4_" or PrintStacktraceAndDie ( "$0 could not open $file4_\n" );

while ( my $txt_line_ = <FILE4H> )
{
    chomp ( $txt_line_ );
    $c4 += ProcLine ( $txt_line_ );
    if ( $c4 >= $num4 )
    {
	last;
    }
}

close FILE4H;

exit ( 0 );

sub ProcLine
{
    my $txt_line_ = shift;
    if ( $txt_line_ =~ /INDICATOR / )
    {
	my @txt_words_ = split ( '#', $txt_line_ );
	if ( $#txt_words_ >= 1 )
	{
	    my $iline_ = $txt_words_[0];
	    my $acorr_ = $txt_words_[1];
	    if ( ! ( exists $iline_acorr_map_{$iline_} ) )
	    { # not seen yet
		$iline_acorr_map_{$iline_} = $acorr_ ;
		printf "%s#%s\n", $iline_, $acorr_ ; 
		return 1;
	    }
	}
    }
    return 0;
}
