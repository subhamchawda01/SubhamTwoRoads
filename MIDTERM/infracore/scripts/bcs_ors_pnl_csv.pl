#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/get_hft_commission_discount_for_shortcode.pl"; # Get hft discount according to volume tiers for shortcode 
require "$GENPERLLIB_DIR/calc_next_business_day.pl"; # 
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; # 


my $USAGE="$0 ors_trades_filename_ date";
if ( $#ARGV < 1 ) { print $USAGE; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];
my $date_ = $ARGV[1];

my $RUB_TO_USD = 0.0309056 ;
my $DEBUG = 0 ;


my %micex_bcs_fee_tiers_ = ();
$micex_bcs_fee_tiers_{"5000"} = 0.006;
$micex_bcs_fee_tiers_{"10000"} = 0.005;
$micex_bcs_fee_tiers_{"15000"} = 0.004;
$micex_bcs_fee_tiers_{"10000000000"} = 0.003;
my $micex_exchange_fee_ = 0.008 ;

#================================= fetching conversion rates from currency info file if available ====================#

my $curr_date_ =  $date_ ; # 
my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;
my $counter_ = 1 ;
while ( ! -e $curr_filename_ && $counter_ < 5 )
{
    $curr_date_ = CalcPrevBusinessDay ( $curr_date_ ) ;
    $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;
    $counter_ += 1 ;
}

my @curr_file_lines_ = ( );
if ( -e $curr_filename_ )
{
    open CURR_FILE_HANDLE, "< $curr_filename_" ;
    @curr_file_lines_ = <CURR_FILE_HANDLE>;
    close CURR_FILE_HANDLE;
}

for ( my $itr = 0 ; $itr <= $#curr_file_lines_; $itr ++ )
{
    my @cwords_ = split ( ' ', $curr_file_lines_[$itr] );
    if ( $#cwords_ >= 1 )
    {
        if( index ( $cwords_[0], "USDRUB" ) == 0 )
	{
            $RUB_TO_USD = sprintf( "%.4f", 1 / $cwords_[1] );
        }

    }
}

#======================================================================================================================#


my $ors_trades_file_base_ = basename ($ors_trades_filename_); chomp ($ors_trades_file_base_);

open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "Could not open ors_trades_filename_ $ors_trades_filename_\n";
my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;
close ORS_TRADES_FILE_HANDLE;

my %symbol_to_commish_map_ = ();
my %symbol_to_noof_working_days_till_expiry_ = ();
my %symbol_to_n2d_map_ = ();
my %symbol_to_symbol_name_map_ = ();

my %symbol_to_expiry_month_ = ("H" => "03", "M" => "06", "U" => "09", "Z" => "12", "G" => "02");
my %symbol_to_expiry_year_ = ("2" => "2012", "3" => "2013", "4" => "2014", "5" => "2015", "6" => "2016", "7"=>"2017", "8" => "2018", "9" => "2019");

my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();

my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();

$exchange_to_volume_map_{"MOS"} = 0;
$exchange_to_unrealpnl_map_{"MOS"} = 0;

for ( my $i = 0 ; $i <= $#ors_trades_file_lines_; $i ++ )
{
    my @words_ = split ( '', $ors_trades_file_lines_[$i] );
    if ( $#words_ >= 4 )
    {
	my $symbol_ = $words_[0];
	my $buysell_ = $words_[1];
	my $tsize_ = $words_[2];
	my $tprice_ = $words_[3];
	my $saos_ = $words_[4];

	if ( !exists ($symbol_to_symbol_name_map_{$symbol_}) ) 
	{
	    SetSecDef ($symbol_);
	}

	# Map symbol to a complete name with expiries.
	# E.g. BAXH3 => BAX201303
	$symbol_ = $symbol_to_symbol_name_map_{$symbol_};
	
	if ( ! ( exists $symbol_to_unrealpnl_map_{$symbol_} ) )
	{
	    $symbol_to_unrealpnl_map_{$symbol_} = 0;
	    $symbol_to_pos_map_{$symbol_} = 0;
	    $symbol_to_price_map_{$symbol_} = 0;
	    $symbol_to_volume_map_{$symbol_} = 0;
	}

	if ($buysell_ == 0)
	{ # buy
            $symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
            $symbol_to_pos_map_{$symbol_} += $tsize_;
	}
	else
	{
            $symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
            $symbol_to_pos_map_{$symbol_} -= $tsize_;
	}
	if ( $symbol_to_exchange_map_{$symbol} eq "MICEX" )
	{
	    $symbol_to_unrealpnl_map_{$symbol_} -= ( max ( 1 , $micex_exchange_fee_ * $tprice_ * $tsize_ ) + $micex_bcs_fee_tiers_{5000} * $tprice_ * $tsize_  ) * $RUB_TO_USD ;
	}
	elsif ( $symbol_to_exchange_map_{$symbol} eq "RTS" )
	{
	    $symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
	}

	$symbol_to_volume_map_{$symbol_} += $tsize_;
	$symbol_to_price_map_{$symbol_} = $tprice_;
    }
}

foreach my $symbol_ ( sort keys %symbol_to_price_map_ ) {
    
    my $unreal_pnl_ = 0 ;
    $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};
    $symbol_to_unrealpnl_map_{$symbol_} = $unreal_pnl_;
    $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
    $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};

}

my $yyyy = substr ($date_, 0, 4);
my $mm = substr ($date_, 4, 2);
my $dd = substr ($date_, 6, 2);

my $bcs_exchange_pnl_filename_ = "/apps/data/MFGlobalTrades/ExchangePnl/".$yyyy."/".$mm."/".$dd."/pnl.csv";
my $bcs_product_pnl_filename_ = "/apps/data/MFGlobalTrades/ProductPnl/".$yyyy."/".$mm."/".$dd."/pnl.csv";

open BCS_EXCHANGE_PNL_FILE_HANDLE, "< $bcs_exchange_pnl_filename_" or die "Could not open '$bcs_exchange_pnl_filename_'. Exiting.\n";
my @bcs_exchange_pnl_file_lines_ = <BCS_EXCHANGE_PNL_FILE_HANDLE>;
close BCS_EXCHANGE_PNL_FILE_HANDLE;

open BCS_PRODUCT_PNL_FILE_HANDLE, "< $bcs_product_pnl_filename_" or die "Could not open '$bcs_product_pnl_filename_'. Exiting.\n";
my @bcs_product_pnl_file_lines_ = <BCS_PRODUCT_PNL_FILE_HANDLE>;
close BCS_PRODUCT_PNL_FILE_HANDLE;

for (my $i = 0; $i <= $#bcs_product_pnl_file_lines_; $i++) {

    my @words_ = split ( ',', $bcs_product_pnl_file_lines_[$i] );
    my $symbol_ = $words_[1];

    $symbol_ =~ s/^\s+//; # Remove leading/trailing white spaces.
    $symbol_ =~ s/\s+$//;

    if ( index ( $symbol_, "Si" ) == 0 || index ( $symbol_, "RI" ) == 0 || 
	 index ( $symbol_, "USD000000TOD" ) == 0 || index ( $symbol_, "USD000UTSTOM" ) == 0 ||
	 index ( $symbol_, "USD000TODTOM" ) == 0 || index ( $symbol_, "RUR" ) == 0 ) 
    {
	my $pnl_ = $words_[2];
	my $traded_vol_ = $words_[3];
	
	if ( $DEBUG == 1 )
	{
	    print ("$date_\n\tORS :\t$symbol_ | PNL : $symbol_to_unrealpnl_map_{$symbol_} | VOL : $symbol_to_volume_map_{$symbol_}\n\tBCS:\t$symbol_ | PNL : $pnl_ | VOL : $traded_vol_\n\n");
	}
	if (!exists $symbol_to_unrealpnl_map_{$symbol_}) 
	{
	    print "symbol_to_unrealpnl_map_ does not have |$symbol_|\n";
	}
	if (abs ($symbol_to_unrealpnl_map_{$symbol_} - $pnl_) >= 5.00 ||
	    $symbol_to_volume_map_{$symbol_} != $traded_vol_) 
	{
	    print ("$date_\n\tORS :\t$symbol_ | PNL : $symbol_to_unrealpnl_map_{$symbol_} | VOL : $symbol_to_volume_map_{$symbol_}\n\tBCS:\t$symbol_ | PNL : $pnl_ | VOL : $traded_vol_\n\n");
	}
    }
}

for (my $i = 0; $i <= $#bcs_exchange_pnl_file_lines_; $i++) {
    my @words_ = split ( ',', $bcs_exchange_pnl_file_lines_[$i] );

    my $symbol_ = $words_[1];
    if ( index ( $symbol_, "RTS" ) == 0 || index ( $symbol_, "MICEX" ) == 0 ) 
    {
	my $pnl_ = $words_[2];
	my $traded_vol_ = $words_[3];
	
	if ($exchange_to_volume_map_{$symbol_} != $traded_vol_ ||
	    abs ($exchange_to_unrealpnl_map_{$symbol_} - $pnl_) >= 100.00) {
	    print ("$date_\n\tORS :\t$symbol_ | PNL : $exchange_to_unrealpnl_map_{$symbol_} | VOL : $exchange_to_volume_map_{$symbol_}\n\tBCS:\t$symbol_ | PNL : $pnl_ | VOL : $traded_vol_\n\n");
	}
    }
}

exit (0);

sub SetSecDef 
{
    my $symbol_ = shift;
    if ( index ( $symbol_, "Si" ) == 0 )
    {
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "Si" )  * $RUB_TO_USD;
	$symbol_to_n2d_map_{$symbol_} = 1 * $RUB_TO_USD;
	$symbol_to_exchange_map_{$symbol_} = "RTS";	
    }
    elsif  ( index ( $symbol_, "RI" ) == 0 )
    {
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "RI" )  * $RUB_TO_USD;
	$symbol_to_n2d_map_{$symbol_} = 0.65 * $RUB_TO_USD;
	$symbol_to_exchange_map_{$symbol_} = "RTS";
    }
    elsif  ( index ( $symbol_, "USD000000TOD" ) == 0 )
    {
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "USD000000TOD" )  * $RUB_TO_USD;
	$symbol_to_n2d_map_{$symbol_} = 1000 * $RUB_TO_USD;
	$symbol_to_exchange_map_{$symbol_} = "MICEX";
    }
    elsif ( index ( $symbol_, "USD000UTSTOM" ) == 0 )
    {
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "USD000UTSTOM" )  * $RUB_TO_USD;
	$symbol_to_n2d_map_{$symbol_} = 1000 * $RUB_TO_USD;
	$symbol_to_exchange_map_{$symbol_} = "MICEX";
    }
    elsif ( index ( $symbol_, "RUR" ) == 0 )
    {   
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "RUR" )  * $RUB_TO_USD;
	$symbol_to_n2d_map_{$symbol_} = 1 ;
	$symbol_to_exchange_map_{$symbol_} = "MICEX";
    }
    elsif ( index ( $symbol_, "USD000TODTOM" ) == 0 )
    {
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "USD000TODTOM" )  * $RUB_TO_USD;
	$symbol_to_n2d_map_{$symbol_} = 100000 * $RUB_TO_USD;
	$symbol_to_exchange_map_{$symbol_} = "MICEX";
    }
}
