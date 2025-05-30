#!/usr/bin/perl
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'}; 
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

require "$GENPERLLIB_DIR/exists_and_same.pl"; #ExistsAndSame

if ( $#ARGV < 1 )
{
    print "USAGE: f1 f2\n";
    exit(0);   
}

my $a_file_ = $ARGV[0];
my $b_file_ = $ARGV[1];

printf ( STDOUT "result = %s\n", ExistsAndSame ( $a_file_, $b_file_ ) );
