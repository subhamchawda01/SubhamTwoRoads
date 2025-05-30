#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $MODELSCRIPTS_DIR=$HOME_DIR."/infracore_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

require "$GENPERLLIB_DIR/get_num_from_numsecname.pl";

if ( $#ARGV >= 0 )
{
    my $tn = GetNumFromNumSecname ( $ARGV[0] );
    printf ( "%s\n", $tn );
}
