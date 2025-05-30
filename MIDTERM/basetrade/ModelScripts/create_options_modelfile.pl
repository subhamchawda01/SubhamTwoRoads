#!/usr/bin/perl

# \file ModelScripts/generate_strategies_from_model.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# output_strategyfilename shortcode StrategyName modelfile paramfile tradingstarttime tradingendtime progid

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 output_modelfilefilename input_modelfilename new_paramfile ALL=-1/INDEX";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $output_modelfilename = $ARGV[0];
my $input_modelfilename = $ARGV[1];
my $new_paramfile = $ARGV[2];
my $index_ = $ARGV[3];

open MODELFILENAME, "< $input_modelfilename " or PrintStacktraceAndDie ( "Could not open $input_modelfilename\n" );
open OUTMFILE, "> $output_modelfilename" or PrintStacktraceAndDie ( "Could not open the output_modelfilename $output_modelfilename\n" );
my $found_ = 0;
my $idx_ = -1;
while ( my $thismline_ = <MODELFILENAME> ) {
    {
	chomp($thismline_);
	if ( ( $thismline_ eq "INDVIDUAL_INDICATORS" ||
	     $thismline_ eq "GLOBAL_INDICATORS" ||
	     $thismline_ eq "INDICATORSEND" ||
	     $thismline_ eq "" ) && $found_ == 1) 
	{
	    $found_ = 0;
	}

	if ( $thismline_ eq "PARAMSHCPAIR" ) {
	    $found_ = 1;
	    $idx_++;
	    print OUTMFILE $thismline_."\n";
	} elsif ( $found_ == 1 ) {
	    if ( $idx_ == $index_ || $index_ == -1 ) {
		my @m_words_ = split ( ' ', $thismline_ );
		$m_words_[1] = $new_paramfile;
		$thismline_ = join ( ' ', @m_words_ );
	    }
	    print OUTMFILE $thismline_."\n";
	} else {
	    print OUTMFILE $thismline_."\n";
	}
    }
}


close OUTMFILE;
