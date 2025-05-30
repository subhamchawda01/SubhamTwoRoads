#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";
require "$GENPERLLIB_DIR/exists_with_size.pl";
printf "%d\n", ExistsWithSize ( $ARGV[0] );

