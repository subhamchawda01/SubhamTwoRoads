#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $indicator_reading_ = 0;
given ( $indicator_reading_ )
{
    when ( -1 )
    {
	print "PRE\n";
	$indicator_reading_ = 0;
    }
    when ( 0 )
    {
	print "AT\n";
	$indicator_reading_ = 1;
    }
}

exit ();

