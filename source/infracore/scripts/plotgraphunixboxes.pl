#!/usr/bin/perl
use strict;
use warnings;

my $USAGE="$0 filename [firstcol secondcol]";
if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $datafilename_ = $ARGV[0];
my $firstcol=1;
my $secondcol=2;
if ( $#ARGV >= 2 ) { $firstcol = $ARGV[1]; $secondcol = $ARGV[2]; }

open (GP, "|gnuplot -persist ") or die "no gnuplot";
# force buffer to flush after each write
use FileHandle;
GP->autoflush(1);
print GP "set xdata time; \n set timefmt \"\%s\"; \n plot \'$datafilename_\' using $firstcol:$secondcol with boxes\n; ";
close GP
