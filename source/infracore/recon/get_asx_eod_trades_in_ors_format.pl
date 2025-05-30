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

my $ClearingComission_ = 91;
my $ExchangeComission_ = 97;

open ABN_HANDLE1_, "< $abn_file_ " or die "could not open ne_file1_ $abn_file_\n";
my @abn_trades1_file_lines_ = <ABN_HANDLE1_>  ;
close ABN_HANDLE1_;

my %symbol_to_last_traded_price_ = ();
my $ASX_TRADES_=$HOME_DIR."/recon/asx_ors_trade_".$expected_date_;

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
  my $exchange_symbol = $product_.$product_expiry_;

  if(!(exists $symbol_to_last_traded_price_{$exchange_symbol})) {
    $symbol_to_last_traded_price_{$exchange_symbol}=$trade_price_;
  }

  if($buy_or_sell_ eq 'BUY') {
        $side=0;
        $quantity_offset_ = 38;
  }
  else {
        $quantity_offset_ = 39;
  }
  my $quantity_ = $fields_[$quantity_offset_];  $quantity_ =~ s/^\s+|\s+$//g;

  print "$exchange_symbol".$side."$quantity_$trade_price_0\n";
}
###To ensure we use correct lpx incase of open positions###
foreach my $symbol (sort keys %symbol_to_last_traded_price_)
{
  if(-e $ASX_TRADES_) {
    my $last_price=`grep $symbol $ASX_TRADES_ | tail -1 | tr '\001' ' ' | awk '{print \$4}'`; chomp ($last_price);
    print "$symbol00$last_price0\n";
  }
}


