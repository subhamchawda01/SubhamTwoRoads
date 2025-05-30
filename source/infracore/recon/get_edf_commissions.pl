#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $USAGE="$0 YYYYMMDD";
if ( $#ARGV != 0 ) { print "$USAGE\n"; exit ( 0 ); }

my $expected_date_ = $ARGV[0];
my $IST_FILE="PFDFST4_$expected_date_".".CSV";
my $edf_trades_filename_ = "/apps/data/EDFTrades/EDFFiles/$IST_FILE";

my $REPO="infracore";
my $GENPERLLIB_DIR=$ENV{"HOME"}."/".$REPO."/GenPerlLib";
my $RECON_DIR=$ENV{"HOME"}."/".$REPO."/recon";
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$RECON_DIR/get_symbol_from_future_code.pl"; # Get exchange symbol from future code
require "$RECON_DIR/init_future_code_map.pl"; # Init future code
require "$RECON_DIR/get_price_multiplier.pl"; # broker price multiplier

my %edf_exch_future_code_to_shortcode_ = GetFuturecodeToShortcodeMap();
my %edf_exch_future_code_to_exchange_ = GetFuturecodeToExchangeMap();
my %broker_symbol_to_exchange_symbol_ = ();

my %fee_discrepancy_shortcodes_ = ();
my %symbol_to_last_traded_time = ();
my %symbol_to_last_traded_price = ();
my %symbol_to_commissions = ();
my %not_found_symbol=();
my $size = keys %edf_exch_future_code_to_shortcode_;

my $record_id_offset_ = 0; # RECORD I D
my $date_offset_ = 7; # TRADE DATE
my $buysell_offset_ = 8; # BUY/sell # 1 = buy ; 2 = sell
my $quantity_offset_ = 9; # QUANTITY
my $edf_future_code_offset_ = 11; # FUTURES CODE
my $exch_code_offset_ = 10; # EXCHANGE
my $contract_yr_month_offset_ = 13; # CONTRACT YEAR MONTH  # YYYYMM
my $trade_price_offset_ = 18; # TRADE PRICE # Trade price may be different than price of order

# Need to figure out how many charges are placed on us.
my $comm_amount_offset_ = 34;
my $fee1_amount_offset_ = 36;
my $fee2_amount_offset_ = 38;
my $fee3_amount_offset_ = 40;
my $fee4_amount_offset_ = 42;
my $session_offset_ = 31;

open EDF_TRADES_FILE_HANDLE, "< $edf_trades_filename_" or die "could not open edf_trades_filename_ $edf_trades_filename_\n";
my @edf_trades_file_lines_ = <EDF_TRADES_FILE_HANDLE>;
close EDF_TRADES_FILE_HANDLE;

for (my $i = 0; $i <= $#edf_trades_file_lines_; $i++) {
  my @fields_ = split (',', $edf_trades_file_lines_[$i]);

  if ($#fields_ != 62) {
    next;
  }

  if (length ($fields_[$edf_future_code_offset_]) < 2) {
    next;
  }

  my $record_id_ = substr ($fields_[$record_id_offset_], 1, -1); $record_id_ =~ s/^\s+|\s+$//g;
  if ($record_id_ ne "T") {
    next;
  }

  my $exch_code_ = substr ( $fields_ [ $exch_code_offset_ ] , 1 , -1 ); $exch_code_ =~ s/^\s+|\s+$//g;
  my $future_code_ = substr ($fields_[$edf_future_code_offset_], 1, -1); $future_code_ =~ s/^\s+|\s+$//g;

  my $exch_future_code_ = $exch_code_.$future_code_;

  if (!exists ( $edf_exch_future_code_to_shortcode_{$exch_future_code_})) {
  	if(! exists $not_found_symbol{$exch_future_code_}){
  	  print STDERR "RECON : EXCHANGE_FUTURE_CODE_NOT_FOUND $exch_future_code_\n";
  	  $not_found_symbol{$exch_future_code_} = 1;
  	}
  	next;
  }

  if ( $edf_exch_future_code_to_exchange_{$exch_future_code_} eq "OSE"){
  	next;
  }
  if ( $edf_exch_future_code_to_exchange_{$exch_future_code_} eq "CFE"){
    next;
  }
  if ( $edf_exch_future_code_to_exchange_{$exch_future_code_} eq "HKEX"){
    next;
  }

  my $date_ = $fields_[$date_offset_]; $date_ =~ s/^\s+|\s+$//g;

  if ($date_ ne $expected_date_) {
    next;
  }

  my $contract_date_ = substr ($fields_[$contract_yr_month_offset_], 1, -1); $contract_date_ =~ s/^\s+|\s+$//g;
  my $broker_symbol = $edf_exch_future_code_to_shortcode_{$exch_future_code_}.$contract_date_;

  if(exists $not_found_symbol{$exch_future_code_}){
   next;
  }

  if(! exists $broker_symbol_to_exchange_symbol_{$broker_symbol}){
    my $exchange_symbol = GetSymbolFromFuturecode($broker_symbol, $contract_date_, $edf_exch_future_code_to_shortcode_{$exch_future_code_}, $expected_date_);
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
  
# for CME, base_commish.cpp considers all commission for a single session with fee3= 0.06
# broker files have commision amount=0.15 for session DV03 in the field fee2.
# Its easier to adjust fee2 amount to 0.06 even for DV03 session and recon it with ors trade data.

  my $cme_DV03_commish_=-0.15;
  my $session_id;
  if(! $fields_[$session_offset_] eq '') { $session_id= substr ($fields_[$session_offset_],1,-1)};
  if ( $edf_exch_future_code_to_exchange_{$exch_future_code_} eq "CME" && $session_id eq "DV03") {
    $fee2_amount_ = $fee2_amount_ - (-0.15*$quantity_traded_) + (-0.06*$quantity_traded_);
  }
  my $total_comm_ = ($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_ + $fee4_amount_);
  if(! exists $symbol_to_commissions{$exchange_symbol}){
    $symbol_to_commissions{$exchange_symbol} = 0;
  }
  $symbol_to_commissions{$exchange_symbol} += $total_comm_;
}
foreach my $key (keys %symbol_to_commissions) {
    print "$key|$symbol_to_commissions{$key}\n";
}