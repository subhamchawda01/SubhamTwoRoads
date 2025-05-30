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

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeStratVecFromList

my $USAGE = "ARGS: strat_list "; 
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

print "All Files: \n".join ( "\n", MakeFilenameVecFromList ( $ARGV[0] ) );
