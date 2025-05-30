#!/usr/bin/perl

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore_install";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $INSTALL_BIN="$HOME_DIR/$REPO/bin/";
my $LIVE_BIN="$HOME_DIR/LiveExec/bin/";

require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; #

my $EUR_TO_DOL = 1.30; # Should have a way of lookin
my $CD_TO_DOL =  0.97; # canadian dollar
my $BR_TO_DOL = 0.51;
my $GBP_TO_DOL = 1.57;
my $HKD_TO_DOL = 0.1289 ;
my $JPY_TO_DOL = 0.0104 ;
my $RUB_TO_DOL = 0.031 ;
my $AUD_TO_DOL = 0.819 ;

my $USAGE="$0 notional [discount/fees(1/0)]";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $notional_ = $ARGV[0];
my $print_discount_ = 1 ;
if ( $#ARGV >= 1 ) { $print_discount_ = $ARGV[1] ; } 

my $yesterday_date_ = `date +%Y%m%d`; chomp ( $yesterday_date_ ) ;
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
  }
}  
my $broker_bps_ = 0.00003;
my $exchange_bps_ = 0.00025 ;
if ( $notional_ <= 20 * 1000 * 1000 ) 
{
  $exchange_bps_ = 0.00025 ;
}
elsif ( $notional_ <= 50 * 1000 * 1000 )
{
  $exchange_bps_ = 0.00023 ;
}
elsif ( $notional_ <= 250 * 1000 * 10000 ) 
{
  $exchange_bps_ = 0.00020 ;
}
elsif ( $notional_ <= 500 * 1000 * 1000 ) 
{
  $exchange_bps_ = 0.00018 ;
}
else
{
  $exchange_bps_ = 0.00016 ;
}
my $discount_ = 0.00025 - $exchange_bps_ ;

if ( $print_discount_ == 1 )
{
  printf ( "%.4f", $notional_ * ( $discount_) * $BR_TO_DOL ) ;
}
else
{
  printf (  "%.4f", $notional_ * ( $exchange_bps_ + $broker_bps_ ) * $BR_TO_DOL ) ;
}
