#!/usr/bin/perl

use strict;
use warnings;
use List::Util qw /min max/;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";

require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl"; # ValidDate

if ( $#ARGV < 0 )
{
  print "USAGE: $0 shc\n";
  exit(0);
} 
print IsValidShc($ARGV[0])."\n";
