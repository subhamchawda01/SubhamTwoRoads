#!/usr/bin/perl

# \file ModelScripts/get_position_from_stir_tradefile.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes a tradesfilename, and prints :
# Average abs position when a trade is on ( measure of risk taken in average ), 
#   where averaging is over time
# Average time to close a trade
# <Median, Average, Stdev, Sharpe, Posfrac> of closed trade pnls

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use List::Util qw/max min/; # for max
use POSIX;

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'};
my $REPO="basetrade";


my $USAGE="$0 tradesfile.txt ";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $tradesfilename_ = $ARGV[0];

open TRADEFILEHANDLE , "< $tradesfilename_ " or PrintStackTraceAndDie ( " COuld not open $tradesfilename_ \n");

my @trade_lines_ = <TRADEFILEHANDLE>;
close ( TRADEFILEHANDLE);

my %symbol_to_position_map_ = ();
foreach my $line_ ( @trade_lines_ ) 
{
	if ( index ( $line_ , "PNLSPLIT") >= 0 )  { next ;}
	
	my @line_words_ = split ( ' ', $line_ );
	if ( $#line_words_ >= 9 )
	{
		$symbol_to_position_map_ { $line_words_[2]} = $line_words_[6];
	}
}

print "            #Product  #Position\n";
foreach my $key_ ( keys %symbol_to_position_map_ )
{
	printf "%20s\t %10d\n", $key_ ,$symbol_to_position_map_ {$key_};
}
