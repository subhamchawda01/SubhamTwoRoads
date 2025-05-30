#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

use Time::HiRes qw(gettimeofday);

my ( $tv_sec_, $tv_usec_ ) = [gettimeofday];

printf "%d%s\n", $tv_sec_, $tv_usec_;

my $newetime=localtime($^T);

printf "%s\n", $newetime;
