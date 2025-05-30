#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use List::Util qw/max min/; # for max

my $USAGE="$0 plural_trades_filename_ YYYYMMDD";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $plural_trades_filename_ = $ARGV[0];
my $expected_date_ = $ARGV[1];

my $HOME_DIR=$ENV{'HOME'};
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
my %symbol_to_last_traded_price_ = ();

# Need to figure out how many charges are placed on us.
my $ClearingComission_ = 9;
my $ExchangeComission_ = 10;
my $TechCenterComission_ = 11;
#my $VarMargin_ = 13;

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
    
    my $date_ = $fields_[$date_offset_];

    my $asset = $fields_[$asset_offset_]; $asset =~ s/^\s+|\s+$//g;
    my $maturity = $fields_[$maturity_offset_]; $maturity =~ s/^\s+|\s+$//g;
    my $exchange_symbol = $asset.$maturity; 
    my $quantity = $fields_[$quantity_offset_]; $quantity =~ s/^\s+|\s+$//g;
    
    # Add the trade price into the pnl
    my $buysell_ = $fields_[$buysell_offset_]; $buysell_ =~ s/^\s+|\s+$//g;
    my $side = 0;
    if ($buysell_ eq "S"){
    	$side = 1;
    }
    my $currency = $fields_[$curr_offset_]; $currency =~ s/^\s+|\s+$//g;
    my $trade_price = $fields_[$trade_price_offset_]; $trade_price =~ s/^\s+|\s+$//g;
    
    if(!(exists $symbol_to_last_traded_price_{$exchange_symbol})) {
	    $symbol_to_last_traded_price_{$exchange_symbol}=$trade_price;
	} 
    
    print "$exchange_symbol$side$quantity$trade_price0\n";
}

###To ensure we use correct lpx incase of open positions###
my $plural_pnl=$HOME_DIR."/recon/plural_ors_pnl";
foreach my $symbol (sort keys %symbol_to_last_traded_price_)
{
  if(-e $plural_pnl) {
  	my $last_price=`grep $symbol $plural_pnl | awk -F'|' '{print \$7}' | awk -F':' '{print \$2}' | tr -d ' '`; chomp ($last_price);
    print "$symbol00$last_price0\n";
  }
}
