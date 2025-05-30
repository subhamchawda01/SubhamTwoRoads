#!/usr/bin/perl
use strict;
use warnings;

my $USAGE="$0 filename1 column\n";
if ( $#ARGV < 1 ) { print $USAGE; exit ( 0 ); }
my $filename1_ = $ARGV[0];
my $targetcol = $ARGV[1];

open (GP, "|gnuplot -persist ") or die "no gnuplot";
# force buffer to flush after each write
use FileHandle;
GP->autoflush(1);

#print GP "set xdata time; \n set timefmt \"\%s\"; \n set grid \n";

print GP "plot ";
print GP "\'$filename1_\' using $targetcol w lines";
print GP " \n";
close GP;
