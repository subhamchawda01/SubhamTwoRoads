#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use List::Util qw/max min/; # for max

my $USAGE="$0 bcs_trades_filename_ YYYYMMDD [1(product)/0(exchange)]";
if ( $#ARGV < 2 ) { print "$USAGE\n"; exit ( 0 ); }
my $bcs_trades_filename_ = $ARGV[0];
my $expected_date_ = $ARGV[1];
my $product_wise_ = $ARGV[2];

# my $rts_trades_filename_ =  #  8279396_trades_20131009_111533.csv 
# my $micex_trades_filename_ = # 10396_trades_20131009_111530.csv 

my $REPO="infracore_install";
my $GENPERLLIB_DIR=$ENV{"HOME"}."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; # 

my %bcs_generic_shc_ = ();
my %bcs_generic_shc_to_n2d_ = ();
my %bcs_generic_shc_to_exchange_ = ();
my %bcs_generic_shc_exchange_fee_ = ();
my %bcs_commisions_ = ();

my %shortcode_to_pnl_ = ();
my %shortcode_to_last_pos_ = ();
my %shortcode_to_last_price_ = ();
my %shortcode_to_n2d_ = ();
my %shortcode_to_volume_ = ();
my %shortcode_to_exchange_ = ();
my %shortcode_to_fees_ = ();
my %shortcode_to_curr_ = ();
my %shortcode_to_expected_fees_ = ();

my %exchange_to_pnl_ = ();
my %exchange_to_volume_ = ();

# RTS PRODUCTS Si RI
# assuming that products codes are XXYY XX -> generic name YY expiry_month expiry_year
$bcs_generic_shc_{"RI"} = "RI"; # RTS Index Future
$bcs_generic_shc_to_n2d_{"RI"} = 0.65; 
$bcs_generic_shc_to_exchange_{"RI"} = "RTS";
$bcs_generic_shc_exchange_fee_{"RI"} = 1; # ( 1 + 2 )/2
$bcs_commisions_{"RI"}=0.5;

$bcs_generic_shc_{"Si"} = "Si"; # USDRUB Future
$bcs_generic_shc_to_n2d_{"Si"} = 1;
$bcs_generic_shc_to_exchange_{"Si"} = "RTS";
$bcs_generic_shc_exchange_fee_{"Si"} = 0.25; # ( 0.25 + 0.5 )/2
$bcs_commisions_{"Si"}=0.5;

$bcs_generic_shc_{"GD"} = "GD"; # GLD
$bcs_generic_shc_to_n2d_{"GD"} = 32.401;
$bcs_generic_shc_to_exchange_{"GD"} = "RTS";
$bcs_generic_shc_exchange_fee_{"GD"} = 0.25; # ( 0.25 + 0.5 )/2
$bcs_commisions_{"GD"}=0.5;

$bcs_generic_shc_{"ED"} = "ED";
$bcs_generic_shc_to_n2d_{"ED"} = 32401;
$bcs_generic_shc_to_exchange_{"ED"} = "RTS";
$bcs_generic_shc_exchange_fee_{"ED"} = 0.25; # ( 0.25 + 0.5 )/2
$bcs_commisions_{"ED"}=0.5;

$bcs_generic_shc_{"SR"} = "SR"; # GLD
$bcs_generic_shc_to_n2d_{"SR"} = 1;
$bcs_generic_shc_to_exchange_{"SR"} = "RTS";
$bcs_generic_shc_exchange_fee_{"SR"} = 0.25; # ( 0.25 + 0.5 )/2
$bcs_commisions_{"SR"}=0.5;

# MICEX PRODUCTS USD000UTSTOM RUR 
$bcs_generic_shc_{"USD000000TOD"} = "USD000000TOD";
$bcs_generic_shc_to_n2d_{"USD000000TOD"} = 1000;
$bcs_generic_shc_to_exchange_{"USD000000TOD"} = "MICEX";
$bcs_generic_shc_exchange_fee_{"USD000000TOD"} = 0; 
$bcs_commisions_{"USD000000TOD"}=0;

$bcs_generic_shc_{"USD000TODTOM"} = "USD000TODTOM";
$bcs_generic_shc_to_n2d_{"USD000TODTOM"} = 100000;
$bcs_generic_shc_to_exchange_{"USD000TODTOM"} = "MICEX";
$bcs_generic_shc_exchange_fee_{"USD000TODTOM"} = 0; 
$bcs_commisions_{"USD000TODTOM"}=0;

$bcs_generic_shc_{"RUR"} = "RUR";
$bcs_generic_shc_to_n2d_{"RUR"} = 1;
$bcs_generic_shc_to_exchange_{"RUR"} = "MICEX";
$bcs_generic_shc_exchange_fee_{"RUR"} = 0; 
$bcs_commisions_{"RUR"}=0;

$bcs_generic_shc_{"USD000UTSTOM"} = "USD000UTSTOM";
$bcs_generic_shc_to_n2d_{"USD000UTSTOM"} = 1000;
$bcs_generic_shc_to_exchange_{"USD000UTSTOM"} = "MICEX";
$bcs_generic_shc_exchange_fee_{"USD000UTSTOM"} = 0; 
$bcs_commisions_{"USD000UTSTOM"}=0;

my %micex_bcs_fee_tiers_ = ();
$micex_bcs_fee_tiers_{"5000"} = 0.006;
$micex_bcs_fee_tiers_{"10000"} = 0.005;
$micex_bcs_fee_tiers_{"15000"} = 0.004;
$micex_bcs_fee_tiers_{"10000000000"} = 0.003;
my $micex_exchange_fee_ = 0.008 ;


# cat  8279396_trades_20131009_111533.csv | head -n1 | awk -F';' '{ for ( i = 1 ; i <= NF ; i ++ ) print i" "$i }' 
#1 TradeNum
#2 TradeDate              # 1
#3 TradeTime
#4 TSSection_Name
#5 BuySell                # 1
#6 FunctionType
#7 AgreeNum
#8 Subacc_SubaccCode      # 1
#9 PutAccount_AccountCode
#10 PayAccount_AccountCode
#11 Security_SecCode      # 1
#12 Asset_ShortName       # 1
#13 CurrPayAsset_ShortName# 1
#14 CPFirm_FirmShortName
#15 Price                 # 1
#16 Qty                   # 1
#17 IsAccrued
#18 AccruedInt
#19 Volume1               # 1
#20 ClearingComission     # 1
#21 ExchangeComission     # 1
#22 TechCenterComission   # 1
#23 PayPlannedDate
#24 PutPlannedDate
#25 IsRepo2
#26 RepoRate
#27 BackDate
#28 RepoDate#2
#29 BackPrice
#30 Volume2
#31 Accruedint2
#32 VarMargin             # 1
#33 Comment               

# awk -F';' '{ print $2" "$5" "$8" "$11" "$12" "$13" "$15" "$16" "$19" "$20" "$21" "$22" "$30" "$32 }' 8279396_trades_20131009_111533.csv | grep -v RIZ3 | head -n3
# TradeDate BuySell Subacc_SubaccCode Security_SecCode Asset_ShortName CurrPayAsset_ShortName Price Qty Volume1 ClearingComission ExchangeComission TechCenterComission Volume2 VarMargin
# 08.10.2013 2 8279396 SiZ3 SIZ3 RUR 32584.00 1.00 32584.00 0 0.50 0 0 18.00
# 08.10.2013 1 8279396 SiZ3 SIZ3 RUR 32583.00 1.00 32583.00 0 0 0 0 -17.00

# TradeDate BuySell Subacc_SubaccCode Security_SecCode Asset_ShortName CurrPayAsset_ShortName Price Qty Volume1 ClearingComission ExchangeComission TechCenterComission Volume2 VarMargin
# 08.10.2013 1 10396 RUR RUR USD 0.03097 4.00 0.12 0 0 0 0.12 0
# 08.10.2013 2 10396 RUR RUR USD 0.03096 4.00 0.12 0 0 0 0.12 0
# 08.10.2013 2 10396 USD000UTSTOM USD RUR 32.1875 1000.00 32187.50 0.43 0.42 0.15 0 0
# 08.10.2013 2 10396 USD000UTSTOM USD RUR 32.192 1000.00 32192.00 0.43 0.42 0.15 0 0
# 08.10.2013 1 10396 USD000UTSTOM USD RUR 32.193 1000.00 32193.00 0.43 0.42 0.15 0 0
# 08.10.2013 1 10396 USD000UTSTOM USD RUR 32.1935 1000.00 32193.50 0.43 0.42 0.15 0 0

my $date_offset_ = 0; # TradeDate MM.DD.YYYY
my $buysell_offset_= 1 ;
my $account_code_offset_ = 2; # Subacc_SubaccCode 8279396 10396
my $curr_offset_ = 5; # Subacc_SubaccCode 8279396 10396
my $bcs_future_code_offset_ = 3; # Security_SecCode
my $trade_price_offset_ = 6; # Price
my $quantity_offset_ = 7; # Qty

# Need to figure out how many charges are placed on us.
my $ClearingComission_ = 9;
my $ExchangeComission_ = 10;
my $TechCenterComission_ = 11;
#my $VarMargin_ = 13;

# RUBUSD
my $RUB_TO_USD_ = 0.0309056 ;

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
	if( index ( $cwords_[0], "USDRUB" ) == 0 )
	{
	    $RUB_TO_USD_ = sprintf( "%.4f", 1 / $cwords_[1] );
	}
    }
}
#======================================================================================================================#
# awk -F';' '{ print $2" "$5" "$8" "$11" "$12" "$13" "$15" "$16" "$19" "$20" "$21" "$22" "$30" "$32 }' 8279396_trades_20131009_111533.csv | grep -v RIZ3 | head -n3

my $mail_body_="";
my @bcs_trades_file_lines_ = `awk -F';' '{ print \$2" "\$5" "\$8" "\$11" "\$12" "\$13" "\$15" "\$16" "\$19" "\$20" "\$21" "\$22" "\$30" "\$32 }' $bcs_trades_filename_` ;


for ( my $i = 0; $i <= $#bcs_trades_file_lines_; $i++ ) 
{
    my @fields_ = split (' ', $bcs_trades_file_lines_[$i] ); 
    
    my $is_header_ = $fields_[0];$is_header_ =~ s/^\s+|\s+$//g;
    if ( $is_header_ eq "TradeDate" )
    {
	next ;
    }

    my $future_code_ = substr ($fields_[$bcs_future_code_offset_], 0, 2); $future_code_ =~ s/^\s+|\s+$//g;
    if ( ! exists ( $bcs_generic_shc_{$future_code_} ) ) 
    {
	$future_code_ = $fields_[$bcs_future_code_offset_]; $future_code_ =~ s/^\s+|\s+$//g;
	if ( !exists ( $bcs_generic_shc_{$future_code_}))
	{
	    $mail_body_=$mail_body_." bcs_to_pnl_per_prod_csv script dont accom shortcode : $future_code_\n";
	    next ;
	}
    }

    my $date_ = $fields_[$date_offset_];
    my @mmddyyyy_ = split ( '\.' , $date_ ) ;
    if ( scalar ( @mmddyyyy_ == 3 ) )
    {
	$date_ = $mmddyyyy_[2].$mmddyyyy_[1].$mmddyyyy_[0];
    }

    if ( $date_ ne $expected_date_ ) 
    {
	$mail_body_=$mail_body_."date mismatched expected_date_::trade_date_ ".$expected_date_."::".$date_."\n";
	open ( MAIL , "|/usr/sbin/sendmail -t" );
	print MAIL "To: nseall@tworoads.co.in\n";
	print MAIL "Subject: comm fee discrepancy - $expected_date_\n\n";
	print MAIL $mail_body_;
	close(MAIL);
	exit ( 0 )  ;
    }

    my $specfic_shortcode_ = "NA" ;
    my $generic_shortcode_ = "NA" ;
    my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;
    my $fee1_amount_ = $fields_[$ClearingComission_]; $fee1_amount_ =~ s/^\s+|\s+$//g;
    my $fee2_amount_ = $fields_[$ExchangeComission_]; $fee2_amount_ =~ s/^\s+|\s+$//g;
    my $fee3_amount_ = $fields_[$TechCenterComission_]; $fee3_amount_ =~ s/^\s+|\s+$//g;
    if ( $fields_[$account_code_offset_] eq "8279396" ) # RTS
    {
	$generic_shortcode_ = $bcs_generic_shc_{ substr ( $fields_[$bcs_future_code_offset_] , 0 , 2 ) } ;
	$specfic_shortcode_ = $fields_[$bcs_future_code_offset_];
    }
    elsif ( $fields_[$account_code_offset_] eq "10396" ) # MICEX
    {
	$generic_shortcode_ = $bcs_generic_shc_{ $fields_[$bcs_future_code_offset_] } ;
	$specfic_shortcode_ = $fields_[$bcs_future_code_offset_];
	$quantity_ = $quantity_ / 1000 ;   # should this be compensated at n2d ? 
    }

    my $number_to_dollar_ = $bcs_generic_shc_to_n2d_{$generic_shortcode_};
    $shortcode_to_n2d_{$specfic_shortcode_} = $number_to_dollar_ ;
    $shortcode_to_curr_{$specfic_shortcode_} = $fields_[$curr_offset_];
    
    # Add the trade price into the pnl
    my $buysell_ = $fields_[$buysell_offset_]; $buysell_ =~ s/^\s+|\s+$//g;
    my $trade_price_ = $fields_[$trade_price_offset_]; $trade_price_ =~ s/^\s+|\s+$//g;
    $shortcode_to_last_price_{$specfic_shortcode_} = $trade_price_;

    # Factor the commissions into the pnl
    # += because the commission ammounts are already -ve
    if ( ! exists $shortcode_to_pnl_{$specfic_shortcode_}) 
    {
	$shortcode_to_pnl_{$specfic_shortcode_} = 0.0;
	$shortcode_to_fees_{$specfic_shortcode_} = 0.0;
	$shortcode_to_expected_fees_{$specfic_shortcode_} = 0.0;
    }

    $shortcode_to_pnl_{$specfic_shortcode_} -= $fee1_amount_ + $fee2_amount_ + $fee3_amount_;  # note bcs commission are not found in bcs_trades_file
    $shortcode_to_fees_{$specfic_shortcode_} += $fee1_amount_ + $fee2_amount_ + $fee3_amount_;  # note bcs commission are not found in bcs_trades_file

    if ( $fields_[$account_code_offset_] eq "8279396" ) # RTS
    {
	$shortcode_to_pnl_{$specfic_shortcode_} -= $bcs_commisions_{$generic_shortcode_}*$quantity_;
	$shortcode_to_fees_{$specfic_shortcode_} += $bcs_commisions_{$generic_shortcode_}*$quantity_;
	$shortcode_to_expected_fees_{$specfic_shortcode_} += $quantity_ * ( $bcs_commisions_{$generic_shortcode_} + $bcs_generic_shc_exchange_fee_{$generic_shortcode_} );
    }
    elsif ( $fields_[$account_code_offset_] eq "10396" ) # MICEX
    {
	$shortcode_to_pnl_{$specfic_shortcode_} -= $micex_bcs_fee_tiers_{5000}*$trade_price_*$quantity_;
	$shortcode_to_fees_{$specfic_shortcode_} += $micex_bcs_fee_tiers_{5000}*$trade_price_*$quantity_;
	$shortcode_to_expected_fees_{$specfic_shortcode_} += max ( 1 , $micex_exchange_fee_*$trade_price_*$quantity_ ) + $micex_bcs_fee_tiers_{5000} * $trade_price_ * $quantity_ ;
    }

    if ($buysell_ == 1) 
    { # Buy
	$shortcode_to_pnl_{$specfic_shortcode_} -= ($trade_price_ * $quantity_ * $number_to_dollar_);
        $shortcode_to_last_pos_{$specfic_shortcode_} += $quantity_ ;  #
    } 
    elsif ($buysell_ == 2) 
    { # Sell
	$shortcode_to_pnl_{$specfic_shortcode_} += ($trade_price_ * $quantity_ * $number_to_dollar_);
        $shortcode_to_last_pos_{$specfic_shortcode_} -= $quantity_ ;
    } 
    else 
    {
	print "Illegal buysell code : $buysell_ :: $bcs_trades_file_lines_[$i]\n";
	exit (1);
    }

    $shortcode_to_volume_{$specfic_shortcode_} += $quantity_;

    if (!exists ($shortcode_to_exchange_{$specfic_shortcode_})) 
    {
	$shortcode_to_exchange_{$specfic_shortcode_} = $bcs_generic_shc_to_exchange_{$generic_shortcode_};
    }
}

# match commission && fees ## 
foreach my $shortcode_ ( keys %shortcode_to_fees_ )
{
    if( abs($shortcode_to_fees_{$shortcode_} - $shortcode_to_expected_fees_{$shortcode_}) > abs ( 0.01 * $shortcode_to_expected_fees_{$shortcode_} ) )
    {
#	print $shortcode_to_volume_{ $shortcode_ } ," " , $shortcode_to_expected_fees_ { $shortcode_ },"\n";
	$mail_body_ = $mail_body_."Discrepancy for $shortcode_: in BCS file $shortcode_to_fees_{$shortcode_}, expected: $shortcode_to_expected_fees_{$shortcode_}\n";
    }
}
if( $mail_body_ )
{
    open ( MAIL , "|/usr/sbin/sendmail -t" );
    print MAIL "To: nseall@tworoads.co.in\n";
    print MAIL "Subject: comm fee discrepancy - $expected_date_\n\n";
    print MAIL $mail_body_;
    close(MAIL);
}

my $shortcode_ = "";
foreach $shortcode_ (sort keys %shortcode_to_pnl_) 
{
    my $currency_ = 1 ;
    if ( $shortcode_to_curr_{$shortcode_} eq "RUR" )
    {
	$currency_ = $RUB_TO_USD_;
    }
# Values are still in euros, convert to doller	
    $shortcode_to_pnl_{$shortcode_} *= $currency_;
    if( $shortcode_to_last_pos_{$shortcode_} > 0) 
    {  #open positions are closed with closing price , commision is not taken into account here
	$shortcode_to_pnl_{$shortcode_} += ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $currency_)  * $shortcode_to_last_pos_{$shortcode_} ) ;	
    }
    elsif ( $shortcode_to_last_pos_{$shortcode_} < 0 )
    {
	$shortcode_to_pnl_{$shortcode_} -= ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $currency_ ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ; 
    }
    $exchange_to_pnl_{$shortcode_to_exchange_{$shortcode_}} += $shortcode_to_pnl_{$shortcode_};
    $exchange_to_volume_{$shortcode_to_exchange_{$shortcode_}} += $shortcode_to_volume_{$shortcode_};
    if ( $product_wise_ == 1 )
    {
	print "$expected_date_,$shortcode_,$shortcode_to_pnl_{$shortcode_},$shortcode_to_volume_{$shortcode_}\n";
#	print "BCS_FEES : $shortcode_to_fees_{$shortcode_} EXPECTED_FEES : $shortcode_to_expected_fees_{$shortcode_}\n";
    }
}

if ( $product_wise_ == 0 )
{
    my $exchange_ = "";
    foreach $exchange_ (sort keys %exchange_to_pnl_) 
    {
	print "$expected_date_,$exchange_,$exchange_to_pnl_{$exchange_},$exchange_to_volume_{$exchange_}\n";
    }
}

