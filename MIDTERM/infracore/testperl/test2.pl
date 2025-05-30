#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $MODELSCRIPTS_DIR=$HOME_DIR."/infracore_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

use Time::HiRes qw(gettimeofday);

my ( $tv_sec_, $tv_usec_ ) = [gettimeofday];

printf "%d%s\n", $tv_sec_, $tv_usec_;

my $newetime=localtime($^T);

printf "%s\n", $newetime;
