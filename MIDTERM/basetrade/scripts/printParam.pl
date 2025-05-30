#!/usr/bin/perl

use warnings;
use strict;


my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/strat_utils.pl"; #GetParam

if ( $#ARGV < 0 )
{
    printf "usage: cmd file [dt=TODAY]\n";
    exit ( 0 );
}

my $SFILE=$ARGV[0];
my $dt="TODAY";

if($#ARGV >= 1) {$dt=$ARGV[1];}
my $PFILE_=GetParam($SFILE, $dt);

print "$PFILE_\n";
