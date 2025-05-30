#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl";

if ( $#ARGV >= 0 )
{
    my $usi = GetUniqueSimIdFromCatFile ( $ARGV[0] );
    printf ( "%s\n", $usi );
}
