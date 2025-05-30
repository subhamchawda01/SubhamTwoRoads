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

my $REPO = "infracore";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."/bin"; # set to debug right now

require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets

my $USAGE = "ARGS: strat_dir timeperiod exclude_sets "; 
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

print "All Files: \n".join ( "\n", MakeStratVecFromDirInTpExcludingSets ( $ARGV[0], $ARGV[1], @ARGV[2 .. $#ARGV] ) );
