#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use List::Util qw/max min/; # for max

my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $RECON_DIR=$ENV{"HOME"}."/".$REPO."/recon";

require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/calc_next_business_day.pl"; #
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; #
require "$RECON_DIR/get_symbol_from_future_code.pl"; # Get exchange symbol from future code
require "$RECON_DIR/init_future_code_map.pl"; # Init future code
require "$RECON_DIR/get_price_multiplier.pl"; # broker price multiplier
require "$GENPERLLIB_DIR/holiday_manager.pl";

my $USAGE="$0 YYYYMMDD";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }

my $expected_date_ = $ARGV[0];
my $abn_file_ = "/apps/data/AbnamroTrades/AbnamroFiles/".$expected_date_."_abnamro_trx.csv";

my $product_offset_ = 14; # XT, YT, AP, IR
my $product_expiry_offset_ = 15; # XT, YT
my $buysell_offset_= 35 ;
my $transaction_type_offset_ = 37;
my $quantity_offset_ = 39; # if buy, 38 if sell
my $business_date= 47;
my $transaction_date=48;
my $trade_price_offset_ = 50; # Price
my $curr_offset_ = 52;

my %symbol_to_commissions = ();
my $ClearingComission_offset_ = 91;
my $ExchangeComission_offset_ = 97;

open ABN_HANDLE1_, "< $abn_file_ " or die "could not open ne_file1_ $abn_file_\n";
my @abn_trades1_file_lines_ = <ABN_HANDLE1_>  ;
close ABN_HANDLE1_;

for (my $i = 0; $i <= $#abn_trades1_file_lines_; $i++) {
  my @fields_ = split (',', $abn_trades1_file_lines_[$i]);

  my $product_= $fields_[$product_offset_]; $product_ =~ s/^\s+|\s+$//g;
  my $product_expiry_= substr($fields_[$product_expiry_offset_],0,-2); $product_expiry_ =~ s/^\s+|\s+$//g;
  my $buy_or_sell_= $fields_[$buysell_offset_]; $buy_or_sell_ =~ s/^\s+|\s+$//g;
  my $transaction_type_= $fields_[$transaction_type_offset_]; $transaction_type_ =~ s/^\s+|\s+$//g;
  my $trade_price_ = $fields_[$trade_price_offset_]; $trade_price_ =~ s/^\s+|\s+$//g;
  my $side = 1; #sell by default
  if($transaction_type_ ne "TRADE") {
        next;
  }
  if($buy_or_sell_ eq 'BUY') {
        $side=0;
        $quantity_offset_ = 38;
  }
  else {
        $quantity_offset_ = 39;
  }
  my $quantity_ = $fields_[$quantity_offset_];  $quantity_ =~ s/^\s+|\s+$//g;
  
  my $exchange_symbol = $product_.$product_expiry_;
  my $clearing_comm_amount_ = $fields_[$ClearingComission_offset_]; $clearing_comm_amount_ =~ s/^\s+|\s+$//g;
  my $exchange_comm_amount_ = $fields_[$ExchangeComission_offset_]; $exchange_comm_amount_ =~ s/^\s+|\s+$//g;
  
  #REBATE: we are getting rebates on monthly basis which is 50% of exchang_commission
  #broker would provide the actual number in abn files
  #ors contains commission numbers after rebate. Adding handling here.
  $exchange_comm_amount_ = $exchange_comm_amount_/2;
  
  my $total_comm_ = ($clearing_comm_amount_ + $exchange_comm_amount_);
  if(! exists $symbol_to_commissions{$exchange_symbol}){
    $symbol_to_commissions{$exchange_symbol} = 0;
  }
  #print "$exchange_symbol  ".$clearing_comm_amount_/$quantity_."  ".$exchange_comm_amount_/$quantity_."\n";
  $symbol_to_commissions{$exchange_symbol} += $total_comm_;
}

foreach my $key (keys %symbol_to_commissions) {
    print "$key|$symbol_to_commissions{$key}\n";
}

#for abn recon, fetch trades corresponding to given broker file from two consecutive day's ors trade files.

