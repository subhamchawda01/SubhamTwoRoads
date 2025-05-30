#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $item1_ = "HELLO";
my @just_args_ = ();
push ( @just_args_, $item1_ );

my $scalar_ref_ = [ @just_args_ ] ;

my $item2_ = "HELLO";
my $found_ = 0;
for ( my $list_idx_ = 0 ; $list_idx_ <= $#$scalar_ref_ ; $list_idx_ ++ )
  {
    if ( $$scalar_ref_[$list_idx_] eq $item2_ )
      {
	$found_ = 1;
	last;
      }
  }
if ( $found_ == 0 )
  {
    push ( @$scalar_ref_, $item2_ );
  }

print $#$scalar_ref_."\n";
exit ();

