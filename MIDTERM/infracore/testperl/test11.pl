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

require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir

if ( $#ARGV < 1 )
{
    print "USAGE: filename new_dir_name\n";
    exit(0);   
}

printf ( STDOUT "new filename = %s", FileNameInNewDir ( $ARGV[0], $ARGV[1] ) );
