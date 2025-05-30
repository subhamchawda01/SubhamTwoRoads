#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my @orig_array_ = (1,2,3,4,5);
my @zero_array_ = (0);
my @elig_array_ = (0) x ( 1 + $#orig_array_ );
print $#elig_array_;

exit ();

