#!/usr/bin/perl
use strict;
use warnings;

my $USAGE="$0 tradesfilename ";
if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $tradesfilename_ = $ARGV[0];
my $targetcol = 7;

my $tradesfilebase_ = `basename $tradesfilename_`; chomp ($tradesfilebase_);

open (GP, "|gnuplot -persist ") or die "no gnuplot";
# force buffer to flush after each write
use FileHandle;
GP->autoflush(1);
# print GP "set terminal X11 size 1080,840 \n set autoscale xy \n show autoscale \n ";
print GP "set xdata time; \n set timefmt \"\%s\"; \n set grid \n; plot \'$tradesfilename_\' using 1:$targetcol with lines title \"$tradesfilebase_\" \n; ";
close GP
