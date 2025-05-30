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
my $USAGE="$0 output_strategyfilename shortcode strategyName modelfile paramfile tradingstarttime tradingendtime progid traded_ezone [PORT/opt]";

if ( $#ARGV < 7 ) { print $USAGE."\n"; exit ( 0 ); }
my $output_strategyfilename = $ARGV[0];
my $shortcode = $ARGV[1];
my $strategyname = $ARGV[2];
my $modelfile = $ARGV[3];
my $paramfile = $ARGV[4];
my $tradingstarttime = $ARGV[5];
my $tradingendtime = $ARGV[6];
my $progid = $ARGV[7];
my $traded_ezone_ = "";
my $strategy_line_type_ = "STRATEGYLINE";
if ( $#ARGV >= 9 )
{
   $traded_ezone_ = $ARGV[8];
   $strategy_line_type_ = "PORT_STRATEGYLINE"
}
if ( $#ARGV == 8 )
{
    if ($ARGV[8] eq "PORT")
    {
        $strategy_line_type_ = "PORT_STRATEGYLINE"
    }
    else
    {
       $traded_ezone_ = $ARGV[8];
    }
}
open OUTSTARTFILE, "> $output_strategyfilename" or PrintStacktraceAndDie ( "Could not open the output_strategyfilename $output_strategyfilename\n" );
print OUTSTARTFILE "$strategy_line_type_ $shortcode $strategyname $modelfile $paramfile $tradingstarttime $tradingendtime $progid $traded_ezone_\n";
close OUTSTARTFILE;
