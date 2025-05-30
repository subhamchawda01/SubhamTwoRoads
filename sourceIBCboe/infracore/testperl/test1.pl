#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $MODELSCRIPTS_DIR=$HOME_DIR."/infracore_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";
require "$GENPERLLIB_DIR/get_unique_list.pl";

my @list = qw/a b c d d a e b a b d e f/;
my @uniq = GetUniqueList ( @list );

print "@list\n@uniq\n";
