#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

sub GetSymbolFromFuturecode
{
    my $broker_symbol = shift;
    my $contract_date_ = shift;
    my $exchange_ = shift;
    my $YYYYMMDD = shift;
        
    my %symbol_to_expiry_month_ = ("F" => "01", "G" => "02", "H" => "03", "J" => "04", "K" => "05", "M" => "06", "N" => "07", "Q" => "08", "U" => "09", "V" => "10", "X" => "11", "Z" => "12");
    my %expiry_month_to_symbol_ = ("01" => "F", "02" => "G", "03" => "H", "04" => "J", "05" => "K", "06" => "M", "07" => "N", "08" => "Q", "09" => "U", "10" => "V", "11" => "X", "12" => "Z");
    
    
    my $exchange_symbol = $broker_symbol;
    
    if(substr($broker_symbol,0,2) eq "LF"){
      my $contract_month=substr($contract_date_,4,2);
      my $contract_year=substr($contract_date_,2,2);
      $exchange_symbol = substr($broker_symbol, 2, 1);
      $exchange_symbol = $exchange_symbol."~~~FM";
      $exchange_symbol = $exchange_symbol.$expiry_month_to_symbol_{$contract_month};
      $exchange_symbol = $exchange_symbol."00";
      $exchange_symbol = $exchange_symbol.$contract_year;
      $exchange_symbol = $exchange_symbol."!";
    }
    
    my $first_three= substr($broker_symbol,0,3);
    if($first_three eq "BAX" || $first_three eq "CGB" || $first_three eq "SXF" || $first_three eq "NIY" || $first_three eq "NKD"
                || $first_three eq "HHI" || $first_three eq "HSI" || $first_three eq "MHI"){
      my $contract_month=substr($contract_date_,4,2);
      my $contract_year=substr($contract_date_,3,1);
      $exchange_symbol = substr($exchange_symbol,0,3);
      $exchange_symbol = $exchange_symbol.$expiry_month_to_symbol_{$contract_month};
      $exchange_symbol = $exchange_symbol.$contract_year;
    }
    
    my $first_two= substr($broker_symbol,0,2);
    if($first_two eq "ZB" || $first_two eq "ZN" || $first_two eq "ZF" || $first_two eq "UB" 
        || $first_two eq "ZT" || $first_two eq "ES" || $first_two eq "GE" || $first_two eq "6M" 
        || $first_two eq "6A" || $first_two eq "6C" || $first_two eq "6J" || $first_two eq "RB" 
        || $first_two eq "CL" || $first_two eq "TN" || $first_two eq "BZ" || $first_two eq "KE"
        || $first_two eq "HO"){
      my $contract_month=substr($contract_date_,4,2);
      my $contract_year=substr($contract_date_,3,1);
      $exchange_symbol = substr($exchange_symbol,0,2);
      $exchange_symbol = $exchange_symbol.$expiry_month_to_symbol_{$contract_month};
      $exchange_symbol = $exchange_symbol.$contract_year;
    }
    
    my $first_five= substr($broker_symbol,0,5);
    if($first_five eq "YFEBM" || $first_five eq "KFFTI" || $first_five eq "JFFCE"){
      my $contract_month=substr($contract_date_,4,2);
      my $contract_year=substr($contract_date_,2,2);
      $exchange_symbol = substr($exchange_symbol,0,5);
      $exchange_symbol = $exchange_symbol.$contract_year;
      $exchange_symbol = $exchange_symbol.$contract_month;
      $exchange_symbol = $exchange_symbol."00000F";
    }
    
    if ($exchange_ eq "OSE"){
      my $contract_month=substr($contract_date_,4,2);
      my $contract_year=substr($contract_date_,2,2);
      $exchange_symbol = substr($broker_symbol,0,-6).$contract_year.$contract_month;
    }
    
    if ($exchange_ eq "SGX"){
      my $contract_month=substr($contract_date_,4,2);
      my $contract_year=substr($contract_date_,2,2);
      $exchange_symbol = substr($broker_symbol,0,-6).$expiry_month_to_symbol_{$contract_month}.$contract_year;
    }
    
    my $shortcode_ = `/home/dvcinfra/infracore_install/bin/get_shortcode_for_symbol $exchange_symbol $YYYYMMDD`; $shortcode_ =~ s/^\s+|\s+$//g;
    if($shortcode_ eq "")
    {
      $exchange_symbol= "";
    }
      
    return $exchange_symbol;
}

1;