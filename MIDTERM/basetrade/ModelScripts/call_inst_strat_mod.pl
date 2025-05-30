#!/usr/bin/perl

# \file ModelScripts/call_inst_strat_mod.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/install_strategy_modelling.pl"; #InstallStrategyModelling

if ( $#ARGV < 3 ) 
{ 
    print "USAGE : src_strategy_file shortcode datagen_start_end_str trading_start_end_str\n";
    exit ( 0 ); 
}

InstallStrategyModelling ( $ARGV[0], $ARGV[1], $ARGV[2], $ARGV[3] );
