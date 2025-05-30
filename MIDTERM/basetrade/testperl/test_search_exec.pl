#!/usr/bin/perl

use strict;
use warnings;
use List::Util qw/max min/; # for max
use FileHandle;

#
package main;
#
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
#
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
# my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec

# Exec dependencies
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my @ADDITIONAL_EXEC_PATHS=();
# Execs to be used here
my $SIM_STRATEGY_EXEC = SearchExec ( "sim_strategy", @ADDITIONAL_EXEC_PATHS ) ;

if ( ! $SIM_STRATEGY_EXEC ) {
  print "Did not find sim_strategy anywhere in search paths\n";
  exit(0);
} else {
  print "found $SIM_STRATEGY_EXEC\n";
}
# End
