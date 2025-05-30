#!/usr/bin/perl

use strict;
use warnings;

my $USAGE="$0 unixtime
";
if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $unixtime_ = $ARGV[0];

print scalar gmtime($unixtime_)."\n";
