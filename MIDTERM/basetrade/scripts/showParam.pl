#!/usr/bin/perl

use warnings;
use strict;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/strat_utils.pl"; # GetParam

if ( $#ARGV < 0 )
{
    printf "usage: cmd file\n";
    exit ( 0 );
}

my $SFILE=$ARGV[0];
my $PFILE_="";

$PFILE_ = GetParam( $SFILE );
system ( "cat $PFILE_" );
