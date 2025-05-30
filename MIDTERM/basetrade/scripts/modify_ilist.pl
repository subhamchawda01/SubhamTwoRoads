#!/usr/bin/perl

use List::Util;
use strict;
use warnings;

my $USAGE="$0";


my $USER = $ENV{'USER'};
my $SPARE_HOME="/spare/local/".$USER."/";
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib/";

if ( $#ARGV < 1) { print $USAGE." filename num_lines_remove \n"; exit ( 0 ); }

my $ilist_file=$ARGV[0];
my $num_lines_remove=$ARGV[1];

my $ilist_file_modified=$ilist_file."_modified";


open(DATA,"< $ilist_file") or die "Can't open data";
my @lines = <DATA>;
my $str= $lines[3];
my @words = split / /, $str;

splice @lines, 3, $num_lines_remove;

close(DATA);

open(FILE, "> $ilist_file_modified") or die "Can't open outputfile";
print FILE @lines

