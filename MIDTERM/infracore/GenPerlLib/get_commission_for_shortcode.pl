#
#   file GenPerlLib/get_commission_for_shortcode.pl
#
#   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#   Address:
#   Suite No 162, Evoma, #14, Bhattarhalli,
#   Old Madras Road, Near Garden City College,
#   KR Puram, Bangalore 560049, India
#   +91 80 4190 3551

use strict;
use warnings;
use feature "switch";

my $EUR_TO_DOL = 1.30; # Should have a way of looking this up from the currency info file.
my $CD_TO_DOL =  0.97; # canadian dollar
my $BR_TO_DOL = 0.51;
my $GBP_TO_DOL = 1.57;
my $HKD_TO_DOL = 0.1289 ;
my $JPY_TO_DOL = 0.0104 ;
my $RUB_TO_DOL = 0.031 ;
my $AUD_TO_DOL = 0.819 ;
my $today_date_ = `date +"%Y%m%d"` ; chomp ( $today_date_ ) ;

sub GetConversionrates() 
{ 
	#================================= fetching conversion rates from currency info file if available ====================#
	
	my $yesterday_file_ = "/tmp/YESTERDAY_DATE" ;
	open YESTERDAY_FILE_HANDLE, "< $yesterday_file_" ;
	
	my @yesterday_lines_ = <YESTERDAY_FILE_HANDLE> ;
	close YESTERDAY_FILE_HANDLE ;
	
	my @ywords_ = split(' ', $yesterday_lines_[0] );
	my $yesterday_date_ = $ywords_[0];
	
	my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$yesterday_date_.'.txt' ;
	my $attempt_ = 1 ;
	while ( ! -e $curr_filename_ && $attempt_ < 10 ) 
	{
	  $yesterday_date_ = CalcPrevBusinessDay ( $yesterday_date_ );
	  $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$yesterday_date_.'.txt' ;
	  $attempt_ ++ ;
	}
	
	open CURR_FILE_HANDLE, "< $curr_filename_" ;
	
	my @curr_file_lines_ = <CURR_FILE_HANDLE>;
	close CURR_FILE_HANDLE;
	
	for ( my $itr = 0 ; $itr <= $#curr_file_lines_; $itr ++ )
	{
	
	  my @cwords_ = split ( ' ', $curr_file_lines_[$itr] );
	  if ( $#cwords_ >= 1 )
	  {
	    my $currency_ = $cwords_[0] ;
	    if ( index ( $currency_, "EURUSD" ) == 0 ){
	      $EUR_TO_DOL = sprintf( "%.4f", $cwords_[1] );
	    }
	
	    if( index ( $currency_, "USDCAD" ) == 0 ){
	      $CD_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
	    }
	
	    if( index ( $currency_, "USDBRL" ) == 0 ){
	      $BR_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
	    }
	
	    if( index ( $currency_, "GBPUSD" ) == 0 ){
	      $GBP_TO_DOL = sprintf( "%.4f", $cwords_[1] );
	    }
	
	    if( index ( $currency_, "USDHKD" ) == 0 ){
	      $HKD_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
	    }
	
	    if( index ( $currency_, "USDJPY" ) == 0 ){
	      $JPY_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
	    }
	
	    if( index ( $currency_, "USDRUB" ) == 0 ){
	      $RUB_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
	    }
	    
	    if( index ( $currency_, "USDAUD" ) == 0 ){
	      $AUD_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
	    }
	
	  }
	}
	#======================================================================================================================#
}
sub GetCommissionForShortcode
{

  GetConversionrates();
  
  my $shortcode_ = shift;
  my $volume_ = shift;
  $volume_ = defined($volume_)?$volume_:"";
  my $volume_1_ = shift;
  my $commission_ = 0.0;


  given ( $shortcode_ )
  {
    when ( "ES" )
    {
      $commission_ = 0.13;
    }
    when ( "ZF" )
    {
      $commission_ = 0.14;
    }
    when ( "ZN" )
    {
      $commission_ = 0.14;
    }
    when ( "ZB" )
    {
      $commission_ = 0.14;
    }
    when ( "ZT" )
    {
      $commission_ = 0.13;
    }
    when ( "GE" )
    {
      $commission_ = 0.46;
    }
    when ( "NKD" )
    {
      $commission_ = 1.04 ; 
    }
    when ( "NIY" )
    {
      $commission_ = 1.05 ;
    }
    when ( "UB" )
    {
      $commission_ = 0.14;
    }
    when ( "CGB" )
    {
      $commission_ = 0.15;
    }
    when ( "CGF" )
    {
      $commission_ = 0.15;
    }
    when ( "CGZ" )
    {
      $commission_ = 0.15;
    }
    when ( "BAX" )
    {
      $commission_ = 0.15;
    }
    when ( "SXF" )
    {
      $commission_ = 0.15;
    }
    when ( "FGBS" )
    {
      $commission_ = 0.22;
    }
    when ( "FOAT" )
    {
      $commission_ = 0.22;
    }
    when ( "FGBM" )
    {
      $commission_ = 0.22;
    }
    when ( "FBTP" )
    {
      $commission_ = 0.22;
    }
    when ( "FBTS" )
    {
      $commission_ = 0.22;
    }
    when ( "FGBL" )
    {
      $commission_ = 0.22;
    }
    when ( "FGBX" )
    {
      $commission_ = 0.22;
    }
    when ( "FESX" )
    {
      $commission_ = 0.32;
    }
    when ( "FVS" )
    {
      $commission_ = 0.22;
    }
    when ( "FEU3" )
    {
      $commission_ = 0.32;
    }
    when ( "FDAX" )
    {
      $commission_ = 0.52;  #0.5 exch fees + 0.2 newedge
    }
    when ( "JFFCE" )
    {
      $commission_ = 0.30;
    }
    when ( "YFEBM" )
    {
      $commission_ = 1.03 ;
    }
    when ( "XFC" )
    {
      $commission_ = 0.57 ;
    }
    when ( "XFRC" )
    {
      $commission_ = 0.85;
    }
    when ( "KFFTI" )
    {
      $commission_ = 0.47;
    }
    when ( "LFZ" )
    {
      $commission_ = 0.31;
    }
    when ( "LFL" )
    {
      $commission_ = 0.31;
    }
    when ( "LFR" )
    {
      $commission_ = 0.25;
    }
    when ( "LFI" )
    {
      $commission_ = 0.37;
    }
    when ( "DOL" )
    {
      if ($volume_ eq "")
      {
        $commission_ = 0.416;
      }
      else
      {
	      my $volume_calc_ = $volume_ + $volume_1_/5;
	      
	      if ($volume_calc_ < 3500)
	      {
	        $commission_ = ( 0.2 + 0.24 )*( 1/$BR_TO_DOL ) + 0.116 + 0.05;
	      }
	      elsif( $volume_calc_ < 7000)
	      {
	        $commission_ = ( 0.12 + 0.10 )*( 1/$BR_TO_DOL ) + 0.116 + 0.05;
	      }
	      elsif ($volume_calc_ < 14000)
	      {
	        $commission_ = ( 0.09 + 0.08 )*( 1/$BR_TO_DOL ) + 0.116 + 0.05;
	      }
	      elsif ($volume_calc_ < 28000)
	      {
	        $commission_ = ( 0.08 + 0.05 )*( 1/$BR_TO_DOL ) + 0.116 + 0.05;
	      }
	      else
	      {
	        $commission_ = ( 0.07 + 0.04 )*( 1/$BR_TO_DOL ) + 0.116 + 0.05;
	      }
      }
    }
    when ( "WDO" )
    {
      if ($volume_ eq "")
      {
        $commission_ = 0.045;
      }
      else
      {
      	
	      my $volume_calc_ = $volume_/5 + $volume_1_;
	      if ($volume_calc_ eq "")
	      {
	        $commission_ = 0.045;
	      }
	      elsif ($volume_calc_ < 3500)
	      {
	        $commission_ = 0.18*( 0.2 + 0.24 )*( 1/$BR_TO_DOL ) + 0.01;
	      }
	      elsif( $volume_calc_ < 7000)
	      {
	        $commission_ = 0.18*( 0.12 + 0.10 )*( 1/$BR_TO_DOL ) + 0.01;
	      }
	      elsif ($volume_calc_ < 14000)
	      {
	        $commission_ =  0.18*( 0.09 + 0.08 )*( 1/$BR_TO_DOL ) + 0.01;
	      }
	      elsif ($volume_calc_ < 28000)
	      {
	        $commission_ = 0.18*( 0.08 + 0.05 )*( 1/$BR_TO_DOL ) + 0.01;
	      }
	      else
	      {
	        $commission_ = 0.18*( 0.07 + 0.04 )*( 1/$BR_TO_DOL ) + 0.01;
	      }
      }
    }
    when ( "IND" )
    {
      if ($volume_ eq "")
      {
        $commission_ = 0.336;
      }
      else
      {
      my $volume_calc_ = $volume_ + $volume_1_/5;
      if ($volume_calc_ < 1800)
      {
        $commission_ = 0.24 + 0.2 + 0.05 + 0.116;
      }
      elsif( $volume_calc_ < 3600)
      {
        $commission_ = 0.12 + 0.1 + 0.05 + 0.116;
      }
      elsif ($volume_calc_ < 5400)
      {
        $commission_ = 0.09 + 0.08 + 0.05 + 0.116;
      }
      elsif ($volume_calc_ < 9000)
      {
        $commission_ = 0.08 + 0.05 + 0.05 + 0.116;
      }
      else
      {
        $commission_ = 0.07 + 0.04 + 0.04 + 0.116;
      }
      }

    }
    when ( "WIN" )
    {
      if ($volume_ eq "")
      {
        $commission_ = 0.03;
      }
      else
      {
      my $volume_calc_ = $volume_/5 + $volume_1_;
      if ($volume_calc_ eq "")
      {
        $commission_ = 0.03;
      }
      elsif ($volume_calc_ < 1800)
      {
        $commission_ = int(0.2*20)/100 + int(0.2*24)/100 + 0.01;
      }
      elsif( $volume_calc_ < 3600)
      {
        $commission_ = int(0.2*12)/100 + int(0.2*10)/100 + 0.01;
      }
      elsif ($volume_calc_ < 5400)
      {
        $commission_ =  int(0.2*9)/100 + int(0.2*8)/100 + 0.01;
      }
      elsif ($volume_calc_ < 9000)
      {
        $commission_ = int(0.2*8)/100 + int(0.2*5)/100 + 0.01;
      }
      else
      {
        $commission_ = int(0.2*7)/100 + int(0.2*4)/100 + 0.01;
      }
      }
    }
    when ( "DI" )
    {
      $commission_ = 0.396; #used to be 0.353
    }
    when ( "HHI" )
    {
      $commission_ = 5.85 ;
    }
    when ( "HSI" )
    {
      $commission_ = 12.35 ;
    }
    when ( "MHI" )
    {
      $commission_ = 4.62 ; # this used to be 4.37
    }
    when ( "MCH" )
    {
      $commission_ = 2.62;
    }
    when ( "NK" )
    {
      $commission_ = 86; # yen supposed to be 86
    }
    when ( "NKM" )
    {
      $commission_ = 11; # yen supposed to be 11
    }
    when ( "JP400" )
    {
      $commission_ = 15; # yen supposed to be 11
    }

    when ( "Si" )
    {
      $commission_ = 0.75; # rub
    }
    when ( "RI" )
    {
      $commission_ = 1.5; # rub
    }
    when ( "GD" )
    {
      $commission_ = 1; # rub
    }
    when ( "ED" )
    {
      $commission_ = 1; # rub
    }
    when ( "Eu" )
    {
      $commission_ = 1; # rub
    }
    when ( "SR" )
    {
      $commission_ = 0.75; # rub
    }
    when ( "OX" )
    {
      $commission_ = 0.75; # rub
    }
    when ( "OV" )
    {
      $commission_ = 0.75; # rub
    }
    when ( "O2" )
    {
      $commission_ = 0.75; # rub
    }
    when ( "O4" )
    {
      $commission_ = 0.75; # rub
    }
    when ( "O6" )
    {
      $commission_ = 0.75; # rub
    }
    when ( "USD000UTSTOM" )
    {
      $commission_ = 0.43; # rub
    }
    when ( "USD000000TOD" )
    {
      $commission_ = 0.43; # rub
    }
    when ( "JGBL" )
    {
      $commission_ = 194; 
    }
    when ( "TOPIX" )
    {
      $commission_ = 125; 
    }
    when ( "VX" )
    {
      $commission_ = 1.32 ;
    }
    when ( "VX2" )
    {
      $commission_ = 0.66 ;
    }
    when ( "FVS" )
    {
      $commission_ = 0.22 ; 
    }
    when ( "XT" )
    {
      $commission_ = 0.74 ; # to be changed with actual ASX provided values
    }
    when ( "YT" )
    {
      $commission_ = 0.74 ; 
    }
    when ( "IR" )
    {
      $commission_ = 0.74 ; 
    }
    when ( "AP" )
    {
      $commission_ = 0.82 ; 
    }
    when ( "MX" )
    {
      $commission_ = 2.0 ;
    }
    when ( "BR" )
    {
      $commission_ = 1.0 ;
    }
    when ( "LK" )
    {
      $commission_ = 1.0 ;
    }
    when ( "VB" )
    {
      $commission_ = 0.75 ;
    }
    when ( "GZ" )
    {
      $commission_ = 0.75 ;
    }
    when ( "GM" )
    {
      $commission_ = 1 ;
    }
    when ( "RN" )
    {
      $commission_ = 1 ;
    }
    when ( "USD000000TOD" )
    {
        $commission_ = 0.00833398 ;
    }
    when ( "USD000TODTOM" )
    {
        $commission_ = 0.00348864;
    }
    when ( "USD000UTSTOM" )
    {
        $commission_ = 0.00833398 ;
    }
    default
    {
      $commission_ = 0.0;
    }
  }

  $commission_;
}

1

