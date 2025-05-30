#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use List::Util 'shuffle';
use FileHandle;

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};
my $REPO="basetrade";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/array_ops.pl"; # GetSum GetIndexOfMaxValue  

my @list=();
push ( @list, 1 );
push ( @list, 2 );
push ( @list, 3 );
push ( @list, 4 );
push ( @list, 5 );
push ( @list, 6 );
push ( @list, 7 );
push ( @list, 8 );

@list = shuffle(@list);
printf ( "%s\n", join ( ' ', @list ) );
printf ( "%d\n", GetIndexOfMaxValue ( \@list ) );
