#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/results_db_access_manager.pl"; #SetStratType SetConfigType


if ( $#ARGV < 0 )
{
  print "USAGE: $0 stratname\n";
  exit(0);
}

my $stratname_ = basename($ARGV[0]);
my $strat = 1;
if ($#ARGV > 0 ){
  $strat = $ARGV[1]
}
if ( $strat == 1 && SetStratType($stratname_,'S') )
{
  print "STAGED STRAT $stratname_ in DB\n";
}
elsif ($strat == 0 && SetConfigType($straname_,'S'))
{
  print "STAGED CONFIG $stratname_ in DB\n";
}

