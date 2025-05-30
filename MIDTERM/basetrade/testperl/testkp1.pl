#!/usr/bin/perl
# \file ModelScripts/test23.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."/bin"; # set to debug right now

my $USAGE = "ARGS: file1 file2 "; 
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $file1_ = $ARGV[0];
my $file2_ = $ARGV[1];

open F1H, "< $file1_";
open F2H, "< $file2_";
my @file1_lines_ = <F1H>; 
my @file2_lines_ = <F2H>; #chomp ( @file2_lines_ );

for ( my $i = 0 ; $i <= min ( $#file1_lines_, $#file2_lines_ ) ; $i ++ )
{

    my @char_1_array_ = split ( "", $file1_lines_[$i] );
    my @char_2_array_ = split ( "", $file2_lines_[$i] );

    print "Line ".$i." ".scalar(@char_1_array_)." ".scalar (@char_2_array_)."\n";

    for ( my $j = 0 ; $j <= min ( $#char_1_array_, $#char_2_array_ ) ; $j ++ )
    {
	if ( $char_1_array_[$j] ne $char_2_array_[$j] )
	{
	    print $j." ".ord($char_1_array_[$j])."-".ord($char_2_array_[$j])."\n";
	}
    }
    if ( $#char_1_array_ > $#char_2_array_ )
    {
	for ( my $j = $#char_2_array_ + 1 ; $j <= $#char_1_array_; $j ++ )
	{
	    print $j." ".ord($char_1_array_[$j])."\n";
	}
    }
}
