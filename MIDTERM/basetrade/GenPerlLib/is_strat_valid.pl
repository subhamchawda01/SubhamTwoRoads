#!/usr/bin/perl

use strict;
use warnings;

sub IsStratValid; #Checks if the given strat file is valid or not

sub IsStratValid
{
  my $strat_file_ = shift;
  my $is_valid_ = `python ~/basetrade/scripts/is_strat_valid.py $strat_file_`;
  my $exitcode = $? >> 8;
  #print "$is_valid_ $exitcode\n";
  return ($is_valid_, $exitcode);
}

1
