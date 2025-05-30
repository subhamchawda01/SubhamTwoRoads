#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $MODELSCRIPTS_DIR=$HOME_DIR."/infracore_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl";

if ( $#ARGV >= 0 )
{
    my $usi = GetUniqueSimIdFromCatFile ( $ARGV[0] );
    printf ( "%s\n", $usi );
}
