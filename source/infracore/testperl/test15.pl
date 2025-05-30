#!/usr/bin/perl
use strict;
use warnings;
use feature "switch"; # for given, when
# use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase

if ( $#ARGV < 1 )
{
    print "USAGE: text vec\n";
    exit(0);   
}

my $substr_ = $ARGV[0];
my @rest_vec_ = ();
for ( my $i = 1 ; $i <= $#ARGV; $i ++ )
{
    push ( @rest_vec_, $ARGV[$i] );
}

printf ( STDOUT "result = %s", FindItemFromVecWithBase ( $substr_, @rest_vec_ ) );
