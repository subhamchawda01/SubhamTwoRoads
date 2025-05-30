#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl";

printf "%d\n", ExistsWithSize ( $ARGV[0] );

