#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $USAGE="$0 YYYYMMDD";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }

my $expected_date_ = $ARGV[0];

my $REPO="infracore";
my $GENPERLLIB_DIR=$ENV{"HOME"}."/".$REPO."/GenPerlLib";
my $RECON_DIR=$ENV{"HOME"}."/".$REPO."/recon";
my $HOME_DIR=$ENV{'HOME'};
my $BASETRADE_BIN="$HOME_DIR/basetrade_install/bin";
my $LIVE_BIN="$HOME_DIR/LiveExec/bin/";
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$RECON_DIR/get_symbol_from_future_code.pl"; # Get exchange symbol from future code
require "$RECON_DIR/init_future_code_map.pl"; # Init future code
require "$RECON_DIR/get_price_multiplier.pl"; # broker price multiplier

my %ne_exch_future_code_to_shortcode_ = GetFuturecodeToShortcodeMap();
my %ne_exch_future_code_to_exchange_ = GetFuturecodeToExchangeMap();
my %broker_symbol_to_exchange_symbol_ = ();
my %fee_discrepancy_shortcodes_ = ();
my %symbol_to_last_traded_time = ();
my %symbol_to_last_traded_price = ();
my %symbol_to_commissions = ();
my %not_found_symbol=();

my $size = keys %ne_exch_future_code_to_shortcode_;

# These offsets were obtained using the 'FIMST4F1.xls & FIMST4F1.csv' template files
# These will be used to determine the field being processed after splitting each trade line

my $record_id_offset_ = 0; # PRECID
my $date_offset_ = 11; # PTDATE
my $ne_future_code_offset_ = 23; # PFC
my $exch_code_offset_ = 22; # PEXCH
my $contract_yr_month_offset_ = 6; # PCTYM  # YYYYMM
my $currency_offset = 28; #PCURSY

# Need to figure out how many charges are placed on us.
my $comm_amount_offset_ = 36;
my $fee1_amount_offset_ = 37;
my $fee2_amount_offset_ = 38;
my $fee3_amount_offset_ = 39;
my $fee4_amount_offset_ = 40;
my $session_offset_=106;
my $quantity_offset_ =17;

# EURTODOL
my $CD_TO_DOL_ = 0.97;
my $EUR_TO_DOL_ = 1.30;
my $GBP_TO_DOL_ = 1.57;
my $HKD_TO_DOL_ = 0.1289 ;
my $JPY_TO_DOL_ = 0.0104 ;

#================================= fetching conversion rates from currency info file if available ====================#


my $curr_date_ = $expected_date_ ;
my $yesterday_file_ = "/tmp/YESTERDAY_DATE" ;
open YESTERDAY_FILE_HANDLE, "< $yesterday_file_" ;

my @yesterday_lines_ = <YESTERDAY_FILE_HANDLE> ;
close YESTERDAY_FILE_HANDLE ;

my @ywords_ = split(' ', $yesterday_lines_[0] );
my $yesterday_date_ = $ywords_[0];

my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$yesterday_date_.'.txt' ;

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
      $EUR_TO_DOL_ = sprintf( "%.4f", $cwords_[1] );
    }

    if( index ( $currency_, "USDCAD" ) == 0 ){
      $CD_TO_DOL_ = sprintf( "%.4f", 1 / $cwords_[1] );
    }

    if( index ( $currency_, "GBPUSD" ) == 0 ){
      $GBP_TO_DOL_ = sprintf( "%.4f", $cwords_[1] );
    }

    if( index ( $currency_, "USDHKD" ) == 0 ){
      $HKD_TO_DOL_ = sprintf( "%.4f", 1/$cwords_[1] );
    }

    if( index ( $currency_, "USDJPY" ) == 0 ){
      $JPY_TO_DOL_ = sprintf( "%.4f", 1 / $cwords_[1] );
    }

  }
}

#======================================================================================================================#

my $ne_trades_filename_ = "/apps/data/MFGlobalTrades/MFGFiles/GMIST4_".$expected_date_.".csv" ;
open NE_TRADES_FILE_HANDLE, "< $ne_trades_filename_" or die "could not open ne_trades_filename_ $ne_trades_filename_\n";

my @ne_trades_file_lines_ = <NE_TRADES_FILE_HANDLE>;

close NE_TRADES_FILE_HANDLE;

for (my $i = 0; $i <= $#ne_trades_file_lines_; $i++) {
  my @fields_ = split (',', $ne_trades_file_lines_[$i]);

  if ($#fields_ != 107) {
#   print "Malformatted line : $ne_trades_file_lines_[$i] No. of fields : $#fields_\n";
    next;
  }

  if (length ($fields_[$ne_future_code_offset_]) < 2) {
    next;
  }

  my $record_id_ = substr ($fields_[$record_id_offset_], 1, -1); $record_id_ =~ s/^\s+|\s+$//g;

  if ($record_id_ ne "T") {
    next;
  }

  my $exch_code_ = substr ( $fields_ [ $exch_code_offset_ ] , 1 , -1 ); $exch_code_ =~ s/^\s+|\s+$//g;
  my $future_code_ = substr ($fields_[$ne_future_code_offset_], 1, -1); $future_code_ =~ s/^\s+|\s+$//g;

  my $exch_future_code_ = $exch_code_.$future_code_;

  if (!exists ( $ne_exch_future_code_to_shortcode_{$exch_future_code_})) {
    if(! exists $not_found_symbol{$exch_future_code_}){
      print STDERR "RECON : EXCHANGE_FUTURE_CODE_NOT_FOUND $exch_future_code_\n";
      $not_found_symbol{$exch_future_code_} = 1;
    }
    next;
  }

  if ( $ne_exch_future_code_to_exchange_{$exch_future_code_} eq "OSE"){
    next;
  }
  if ( $ne_exch_future_code_to_exchange_{$exch_future_code_} eq "CFE"){
    next;
  }
  if ( $ne_exch_future_code_to_exchange_{$exch_future_code_} eq "HKEX"){
    next;
  }
  my $date_ = substr ($fields_[$date_offset_], 1, -1); $date_ =~ s/^\s+|\s+$//g;

  if ($date_ ne $expected_date_) {
    next;
  }

  my $contract_date_ = substr ($fields_[$contract_yr_month_offset_], 1, -1); $contract_date_ =~ s/^\s+|\s+$//g;
  my $broker_symbol = $ne_exch_future_code_to_shortcode_{$exch_future_code_}.$contract_date_;

  if(exists $not_found_symbol{$exch_future_code_}){
   next;
  }

  if(! exists $broker_symbol_to_exchange_symbol_{$broker_symbol}){
    my $exchange_symbol = GetSymbolFromFuturecode($broker_symbol, $contract_date_, $ne_exch_future_code_to_shortcode_{$exch_future_code_}, $expected_date_);
    if($exchange_symbol eq "")
    {
      print STDERR "RECON : TRANSLATION FAILED $broker_symbol\n";
      $not_found_symbol{$exch_future_code_} = 1;
      next;
    }
    $broker_symbol_to_exchange_symbol_{$broker_symbol} = $exchange_symbol;
  }

  my $exchange_symbol =  $broker_symbol_to_exchange_symbol_{$broker_symbol};
  my $quantity_traded_ =$fields_[$quantity_offset_]; $quantity_traded_ =~ s/^\s+|\s+$//g;
  my $comm_amount_ = $fields_[$comm_amount_offset_]; $comm_amount_ =~ s/^\s+|\s+$//g;
  my $fee1_amount_ = $fields_[$fee1_amount_offset_]; $fee1_amount_ =~ s/^\s+|\s+$//g;
  my $fee2_amount_ = $fields_[$fee2_amount_offset_]; $fee2_amount_ =~ s/^\s+|\s+$//g;
  my $fee3_amount_ = $fields_[$fee3_amount_offset_]; $fee3_amount_ =~ s/^\s+|\s+$//g;
  my $fee4_amount_ = $fields_[$fee4_amount_offset_]; $fee4_amount_ =~ s/^\s+|\s+$//g;

#for FVS products, dont consider fee2 in daily commission
  if ( index ( $exchange_symbol, "FVS" ) == 0 ){
    $fee2_amount_=0;
  }

#BAX products has different handling for commissions
  if ( index ( $exchange_symbol, "BAX" ) == 0 ){ #&& index ( $exchange_symbol, "_" ) != -1){
     my $shc_ = `$BASETRADE_BIN/get_shortcode_for_symbol  "$exchange_symbol" $expected_date_ ` ; chomp ( $shc_ );
     my @words_ = split ( '_',$shc_);
     #print "DEBUG: $fields_[18] $exchange_symbol $words_[0] $words_[1]\n";
     if($words_[1] >=4)
     {
       $fee4_amount_ = $fee4_amount_ + ($quantity_traded_*-0.05);
     }
     else
     {
       $fee4_amount_ = $fee4_amount_ + ($quantity_traded_*-0.11);
     }
  }

#SXF & CGB commissions
  if ( (index ( $exchange_symbol, "CGB" ) == 0) || (index ( $exchange_symbol, "SXF") == 0)) { #&& index ( $exchange_symbol, "_" ) != -1){
     $fee4_amount_ = $fee4_amount_ + ($quantity_traded_ * -0.11);
  }

# CGF & CGZ
  if ( (index ( $exchange_symbol, "CGF" ) == 0) || (index ( $exchange_symbol, "CGZ") == 0)) { #&& index ( $exchange_symbol, "_" ) != -1){
     $fee4_amount_ = $fee4_amount_ + ($quantity_traded_ * -0.08);
  }


  my $total_comm_ = ($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_ + $fee4_amount_);
  if(! exists $symbol_to_commissions{$exchange_symbol}){
    $symbol_to_commissions{$exchange_symbol} = 0;
  }

=begin comment
  my $currency = substr ($fields_[$currency_offset], 1, -1);$currency =~ s/^\s+|\s+$//g;
  if($currency eq "JPY"){
  	$total_comm_ *= $JPY_TO_DOL_;
  }
  if($currency eq "GBP"){
    $total_comm_ *= $GBP_TO_DOL_;
  }
  if($currency eq "CAD"){
    $total_comm_ *= $CD_TO_DOL_;
  }
  if($currency eq "EUR"){
    $total_comm_ *= $EUR_TO_DOL_;
  }
=end comment
=cut

  $symbol_to_commissions{$exchange_symbol} += $total_comm_;
}

foreach my $key (keys %symbol_to_commissions) {
    print "$key|$symbol_to_commissions{$key}\n";
}
