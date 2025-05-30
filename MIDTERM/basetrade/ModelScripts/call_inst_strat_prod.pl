#!/usr/bin/perl

# \file ModelScripts/call_inst_strat_prod.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/install_strategy_production.pl"; #InstallStrategyProduction

if ( $#ARGV < 2 ) 
{ print "USAGE : src_strategy_file destmac deststratid\n"; exit ( 0 ); }

my $dest_strat_file_ = InstallStrategyProduction ( $ARGV[0], $ARGV[1], $ARGV[2], "dvctrader" );
print "$dest_strat_file_\n";
