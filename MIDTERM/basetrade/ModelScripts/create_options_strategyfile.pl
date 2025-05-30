#!/usr/bin/perl
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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
#STRATEGYLINE OptionsTrading /home/kputta/atm_ivmodel kMethodBlackScholes 2997 IST_920 IST_1520
my $USAGE="$0 output_strategyfilename trading_exec_ modelfile ivadaptor progid tradingstarttime tradingendtime";

if ( $#ARGV < 6 ) { print $USAGE."\n"; exit ( 0 ); }
my $output_strategyfilename = $ARGV[0];
my $tradingexec = $ARGV[1];
my $modelfile = $ARGV[2];
my $ivadaptor = $ARGV[3];
my $tradingstarttime = $ARGV[4];
my $tradingendtime = $ARGV[5];
my $progid = $ARGV[6];

my $market_model_ = 0;
my $use_accurate_ = 1;

if($#ARGV > 6) {
    $market_model_ = $ARGV[7];
}
if($#ARGV > 7) {
    $use_accurate_ = $ARGV[8];
}

open OUTSTARTFILE, "> $output_strategyfilename" or PrintStacktraceAndDie ( "Could not open the output_strategyfilename $output_strategyfilename\n" );
print OUTSTARTFILE "STRATEGYLINE $tradingexec $modelfile $ivadaptor $progid $tradingstarttime $tradingendtime $market_model_ $use_accurate_\n";
close OUTSTARTFILE;
