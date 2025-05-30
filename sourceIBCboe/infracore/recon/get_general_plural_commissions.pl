#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use List::Util qw/max min/; # for max

my $USAGE="$0 YYYYMMDD";
#print $#ARGV;
#print "@ARGV $ARGV[0]\n";
if ( @ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $expected_date_ = $ARGV[0];
my $plural_trades_filename_ = "/apps/data/PluralTrades/PluralFiles/InvoiceDetailed_$expected_date_.csv";

my $REPO="infracore_install";
my $GENPERLLIB_DIR=$ENV{"HOME"}."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; # 

# cat  8279396_trades_20131009_111533.csv | head -n1 | awk -F';' '{ for ( i = 1 ; i <= NF ; i ++ ) print i" "$i }' 
#1 ASSET,
#2 MARKET,
#3 MATURITY,
#4 ISIN_CODE,
#5 SIDE,
#6 QUANTITY,
#7 SETTLEMENT_DATE,
#8 CLIENT_NAME,
#9 CLIENTACCOUNT,
#10 TRADE_ID,
#11 SETTLEMENT_PRICE,
#12 FX_RATE,
#13 PRODUCT_CURRENCY,
#14 DESIGNATION,
#15 EXCHANGE_FEES,
#16 EXCHANGE_REGISTER_FEES,
#17 BROKERFEES,
#18 PEL

my $asset_offset_ = 0;
my $maturity_offset_ = 2;
my $buysell_offset_ = 4 ;
my $quantity_offset_ = 5; 
my $date_offset_ = 6; 
my $trade_price_offset_ = 10;
my $curr_offset_ = 12;

#commissions offset
my $exchange_fee_offset = 14;
my $exchange_register_fee_offset =15;
my $broker_fee_offset = 16;

my %symbol_to_commissions = ();

# RUBUSD
my $BRL_TO_USD_ = 0.26062122718;

#================================= fetching conversion rates from currency info file if available ====================#

my $curr_date_ = $expected_date_ ;
my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;

my $counter_ = 1 ;
while ( ! -e $curr_filename_ && $counter_ < 5 )
{
    $curr_date_ = CalcPrevBusinessDay ( $curr_date_ ) ;
    $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;
    $counter_ += 1 ;
}

my @curr_file_lines_ ;
if ( -e $curr_filename_ )
{
    open CURR_FILE_HANDLE, "< $curr_filename_" ;
    my @curr_file_lines_ = <CURR_FILE_HANDLE>;
    close CURR_FILE_HANDLE;
}

for ( my $itr = 0 ; $itr <= $#curr_file_lines_; $itr ++ )
{
    my @cwords_ = split ( ' ', $curr_file_lines_[$itr] );
    if ( $#cwords_ >= 1 )
    {
	    if( index ( $cwords_[0], "USDBRL" ) == 0 )
	    {
	        $BRL_TO_USD_ = sprintf( "%.4f", 1 / $cwords_[1] );
	    }
    }
}

open PLURAL_TRADES_FILE_HANDLE, "< $plural_trades_filename_" or die "could not open plural_trades_filename_ $plural_trades_filename_\n";

my @plural_trades_file_lines_ = <PLURAL_TRADES_FILE_HANDLE>;

close PLURAL_TRADES_FILE_HANDLE;

for ( my $i = 1; $i <= $#plural_trades_file_lines_; $i++ ) 
{
    my @fields_ = split (';', $plural_trades_file_lines_[$i] ); 
    my $asset = $fields_[$asset_offset_]; $asset =~ s/^\s+|\s+$//g;
    my $maturity = $fields_[$maturity_offset_]; $maturity =~ s/^\s+|\s+$//g;
    my $exchange_symbol = $asset.$maturity;

    my $exchange_fee = $fields_[$exchange_fee_offset]; $exchange_fee =~ s/^\s+|\s+$//g;
    my $exchange_register_fee = $fields_[$exchange_register_fee_offset]; $exchange_register_fee =~ s/^\s+|\s+$//g;
    my $broker_fee = $fields_[$broker_fee_offset]; $broker_fee =~ s/^\s+|\s+$//g;
    my $total_comm_ = ($exchange_fee + $exchange_register_fee + $broker_fee);
   
    if(! exists $symbol_to_commissions{$exchange_symbol}){
    $symbol_to_commissions{$exchange_symbol} = 0;
  }
    $symbol_to_commissions{$exchange_symbol} += $total_comm_;
   # print "$exchange_symbol|";
}

foreach my $key (keys %symbol_to_commissions) {
    print "$key|$symbol_to_commissions{$key}\n";
}

