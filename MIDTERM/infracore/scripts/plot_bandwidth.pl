#!/usr/bin/perl
use strict;
use warnings;

my $USAGE="$0 DATE LOCATION SERVER_NO\n";
if ( $#ARGV < 2 ) { print $USAGE; exit ( 0 ); }

my $yyyymmdd_ = `date +%Y%m%d`; chomp ($yyyymmdd_);
if ($ARGV[0] ne "TODAY") {
    $yyyymmdd_ = $ARGV[0]; chomp ($yyyymmdd_);
}

my $location_ = $ARGV[1]; chomp ($location_);
my $server_no_ = $ARGV[2]; chomp ($server_no_);

my $yyyy_ = substr ($yyyymmdd_, 0, 4);
my $mm_ = substr ($yyyymmdd_, 4, 2);
my $dd_ = substr ($yyyymmdd_, 6, 2);

my $filename1_ = "/NAS1/data/LatencyReports/".$location_."/".$yyyy_."/".$mm_."/".$dd_."/"."srv".$server_no_;

if (! ( -e $filename1_)) {
    print "$filename1_ does not exist\n";
    exit (0);
}

open (GP, "|gnuplot -persist ") or die "no gnuplot";

use FileHandle;
GP->autoflush(1);

print GP "set xdata time; \n set timefmt \"\%s\"; \n set grid \n";

print GP "plot ";
print GP "\'$filename1_\' u 1:2 w lp t \"MAXIMUM $yyyymmdd_ $location_ srv $server_no_ \"";
print GP ", ";
print GP "\'$filename1_\' u 1:3 w lp t \"AVERAGE $yyyymmdd_ $location_ srv $server_no_ \"";
print GP ", ";
print GP "\'$filename1_\' u 1:4 w lp t \"WITHIN_LIMITS $yyyymmdd_ $location_ srv $server_no_ \"";
print GP " \n";
close GP;
