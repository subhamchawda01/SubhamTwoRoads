#!/usr/bin/perl

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/sync_to_all_machines.pl";

if ( $#ARGV < 0 ) { exit ( 0 ); }
SyncToAllMachines ( $ARGV[0] );
