#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

sub GetPriceMultiplier
{
  my $exchange_symbol = shift;
  my $exchange = shift;
  my $first_two = substr($exchange_symbol,0,2);
  my $first_three = substr($exchange_symbol,0,3);
 
  if($first_two eq "NK" && $exchange eq "OSE"){
    return 1;
  }

  if($exchange eq "SGX"){
    if($first_two eq "NK") {
    	return 100;
    }
  }
  
  if($exchange eq "OSE" && substr($exchange_symbol,0,5) ne "JP400" ){
    return 100;
  }
  
  if($first_two eq "6A" || $first_two eq "6C" || $first_two eq "6J" || $first_two eq "ES" 
  	|| $first_two eq "GE" || $first_two eq "BZ" || $first_three eq "HSI" || $first_two eq "RB" 
  	|| $first_two eq "CL" || $first_two eq "KE"){
    return 100;
  }
  
  if($first_two eq "6M"){
    return 1000;
  }
  
  return 1;
}

1;
