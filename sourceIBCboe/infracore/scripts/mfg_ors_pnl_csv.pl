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
require "$GENPERLLIB_DIR/get_cme_commission_discount_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_cme_eu_volumes_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_number_of_working_days_BMF_between_two_dates.pl" ;
require "$GENPERLLIB_DIR/get_rts_ticksize.pl"; #RITickSize
require "$GENPERLLIB_DIR/get_commission_for_DI.pl"; #GetCommissionForDI


my $USAGE="$0 ors_trades_filename_ date\n";
if ( $#ARGV < 1 ) { print $USAGE; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];
my $date_ = $ARGV[1];

my $EUR_TO_DOL = 1.30; # Should have a way of looking this up from the currency info file.
my $CD_TO_DOL =  0.97; # canadian dollar
my $BR_TO_DOL = 0.51; # Brazil real
my $GBP_TO_DOL = 1.57;
my $HKD_TO_DOL = 0.1289 ;
my $JPY_TO_DOL = 0.0104 ;
my $RUB_TO_USD_ = 0.0309056 ;


my $today_date_ = `date +"%Y%m%d"` ;

#================================= fetching conversion rates from currency info file if available ====================#

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

    if( index ( $cwords_[0], "USDRUB" ) == 0 )
    {
      $RUB_TO_USD_ = sprintf( "%.4f", 1 / $cwords_[1] );
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
my %short_name_to_symbol_map_ =();
my %symbol_to_expiry_year_ = ("0" => "2010", "1" => "2011", "2" => "2012", "3" => "2013", "4" => "2014", "12"=>"2012", "5" => "2015", "6" => "2016", "7" => "2017", "8" => "2018", "9" => "2019");
my %symbol_to_expiry_month_ = ("F" => "01", "G" => "02", "H" => "03", "J" => "04", "K" => "05", "M" => "06", "N" => "07", "Q" => "08", "U" => "09", "V" => "10", "X" => "11", "Z" => "12");

my %liffe_symbol_to_expiry_year_ = ("12" => "2012", "13" => "2013", "14" => "2014", "15" => "2015", "16" => "2016", "17"=>"2017", "18" => "2018", "19" => "2019");
my %ose_symbol_to_expiry_year_ = ("12" => "2012", "13" => "2013", "14" => "2014", "15" => "2015", "16" => "2016", "17"=>"2017", "18" => "2018", "19" => "2019");

my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();

my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();

$exchange_to_volume_map_{"EUREX"} = 0;
$exchange_to_volume_map_{"CME"} = 0;
$exchange_to_volume_map_{"TMX"} = 0;
$exchange_to_volume_map_{"BMF"} = 0;
$exchange_to_volume_map_{"LIFFE"} = 0;
$exchange_to_volume_map_{"HKEX"} = 0;
$exchange_to_volume_map_{"OSE"} = 0;
$exchange_to_volume_map_{"CFE"} = 0;

$exchange_to_unrealpnl_map_{"EUREX"} = 0;
$exchange_to_unrealpnl_map_{"CME"} = 0;
$exchange_to_unrealpnl_map_{"TMX"} = 0;
$exchange_to_unrealpnl_map_{"BMF"} = 0;
$exchange_to_unrealpnl_map_{"LIFFE"} = 0;
$exchange_to_unrealpnl_map_{"HKEX"} = 0;
$exchange_to_unrealpnl_map_{"OSE"} = 0;
$exchange_to_unrealpnl_map_{"CFE"} = 0;

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

    if (!exists ($symbol_to_symbol_name_map_{$symbol_})) {
      SetSecDef ($symbol_);
    }

# Map symbol to a complete name with expiries.
# E.g. BAXH3 => BAX201303
    $symbol_ = $symbol_to_symbol_name_map_{$symbol_};

    if ( ! ( exists $symbol_to_unrealpnl_map_{$symbol_} ) )
    {
		InitSymbol( $symbol_ );
    }

    if ($buysell_ == 0)
    { # buy
		Buy( $symbol_, $tprice_, $tsize_ );
    }
    else
    {
    	Sell( $symbol_, $tprice_, $tsize_);
    }

	# update volume, last price and pnl ( deduct commision )
	UpdateVolumePricePnl( $symbol_, $tprice_, $tsize_) ;

  }
}

foreach my $symbol_ ( sort keys %symbol_to_price_map_ ) {

  my $unreal_pnl_ = 0 ;

  if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {

    $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} - $symbol_to_pos_map_{$symbol_} * ( ( 100000 / ( $symbol_to_price_map_{$symbol_} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ) ;

  }else{

    $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} ;

  }

# BMF has different tiers of discount for HFT commission
  if( index ( $symbol_to_exchange_map_{$symbol_}, "BMF" ) == 0 ){

    my $shortcode_ = "";

    if ( index ( $symbol_, "DOL" ) == 0 ) {
      $shortcode_ = "DOL" ; 
      my $wdo_volume_ = $symbol_to_volume_map_{$short_name_to_symbol_map_{"WDO"}};
      $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( $shortcode_ , $symbol_to_volume_map_{$symbol_},$wdo_volume_) * $BR_TO_DOL;
    }
    if ( index ( $symbol_, "WDO" ) == 0 ) {
      $shortcode_ = "WDO" ; 
      my $dol_volume_ = $symbol_to_volume_map_{$short_name_to_symbol_map_{"DOL"}};
      $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( $shortcode_ , $symbol_to_volume_map_{$symbol_},$dol_volume_) * $BR_TO_DOL;
    }
    if ( index ( $symbol_, "WIN" ) == 0 ) {
      $shortcode_ = "WIN" ; 
      my $ind_volume_ = $symbol_to_volume_map_{$short_name_to_symbol_map_{"IND"}};
     $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( $shortcode_ , $symbol_to_volume_map_{$symbol_},$ind_volume_) * $BR_TO_DOL;
    }
    if ( index ( $symbol_, "IND" ) == 0 ) {
      $shortcode_ = "IND" ; 
      my $win_volume_ = $symbol_to_volume_map_{$short_name_to_symbol_map_{"WIN"}};
     $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( $shortcode_ , $symbol_to_volume_map_{$symbol_}, $win_volume_) * $BR_TO_DOL;
    }
    if (index ($symbol_, "DI") == 0 ) {
   $symbol_to_commish_map_{$symbol_} = GetCommissionForDI($shortcode_); 
    }

    $unreal_pnl_ += ( $symbol_to_volume_map_{$symbol_} * GetHFTDiscount( $shortcode_, $symbol_to_volume_map_{$symbol_} ) * $BR_TO_DOL ) ;
    $unreal_pnl_ -= ( $symbol_to_volume_map_{$symbol_}*$symbol_to_commish_map_{$symbol_});

  }elsif( index ( $symbol_to_exchange_map_{$symbol_}, "CME" ) == 0 ){

    my $shortcode_ ="" ;

    if ( index ( $symbol_, "ZN" ) == 0 ) { $shortcode_ = "ZN" ; }
    if ( index ( $symbol_, "ZF" ) == 0 ) { $shortcode_ = "ZF" ; }
    if ( index ( $symbol_, "ZB" ) == 0 ) { $shortcode_ = "ZB" ; }
    if ( index ( $symbol_, "UB" ) == 0 ) { $shortcode_ = "UB" ; }

    my $trade_date_ = $date_ ;

    my $trade_hours_string_ = "US" ;

    $unreal_pnl_ += ( ( $symbol_to_volume_map_{$symbol_} - GetCMEEUVolumes( $shortcode_, $trade_date_ ) ) * GetCMEDiscount( $shortcode_, $trade_hours_string_ ) ) ;

  }

  $symbol_to_unrealpnl_map_{$symbol_} = $unreal_pnl_;

  $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;

  $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};

}

my $yyyy = substr ($date_, 0, 4);
my $mm = substr ($date_, 4, 2);
my $dd = substr ($date_, 6, 2);

my $mfg_exchange_pnl_filename_ = "/apps/data/MFGlobalTrades/ExchangePnl/".$yyyy."/".$mm."/".$dd."/pnl.csv";
my $mfg_product_pnl_filename_ = "/apps/data/MFGlobalTrades/ProductPnl/".$yyyy."/".$mm."/".$dd."/pnl.csv";

open MFG_EXCHANGE_PNL_FILE_HANDLE, "< $mfg_exchange_pnl_filename_" or die "Could not open '$mfg_exchange_pnl_filename_'. Exiting.\n";
my @mfg_exchange_pnl_file_lines_ = <MFG_EXCHANGE_PNL_FILE_HANDLE>;
close MFG_EXCHANGE_PNL_FILE_HANDLE;

open MFG_PRODUCT_PNL_FILE_HANDLE, "< $mfg_product_pnl_filename_" or die "Could not open '$mfg_product_pnl_filename_'. Exiting.\n";
my @mfg_product_pnl_file_lines_ = <MFG_PRODUCT_PNL_FILE_HANDLE>;
close MFG_PRODUCT_PNL_FILE_HANDLE;

for (my $i = 0; $i <= $#mfg_product_pnl_file_lines_; $i++) {
  my @words_ = split ( ',', $mfg_product_pnl_file_lines_[$i] );

  my $symbol_ = $words_[1]; 

  if ( index ( $symbol_, "NK" ) == 0 || index ( $symbol_, "JGBL" ) == 0 || index ( $symbol_, "TOPIX" ) == 0|| 
      index ( $symbol_, "Si" ) == 0 || index ( $symbol_, "RI" ) == 0 ||
      index ( $symbol_, "USD000000TOD" ) == 0 || index ( $symbol_, "USD000UTSTOM" ) == 0 ||
      index ( $symbol_, "USD000TODTOM" ) == 0 || index ( $symbol_, "RUR" ) == 0 ) { next ; } 

  $symbol_ =~ s/^\s+//; # Remove leading/trailing white spaces.
    $symbol_ =~ s/\s+$//;
  my $pnl_ = $words_[2];
  my $traded_vol_ = $words_[3];

  if (!exists $symbol_to_unrealpnl_map_{$symbol_}) {
    print "symbol_to_unrealpnl_map_ does not have |$symbol_|\n";
  }
  if (abs ($symbol_to_unrealpnl_map_{$symbol_} - $pnl_) >= 20.00 ||
      $symbol_to_volume_map_{$symbol_} != $traded_vol_) {
    print ("$date_\n\tORS :\t$symbol_ | PNL : $symbol_to_unrealpnl_map_{$symbol_} | VOL : $symbol_to_volume_map_{$symbol_}\n\tMFG:\t$symbol_ | PNL : $pnl_ | VOL : $traded_vol_\n\n");
  }
}

for (my $i = 0; $i <= $#mfg_exchange_pnl_file_lines_; $i++) {
  my @words_ = split ( ',', $mfg_exchange_pnl_file_lines_[$i] );

  my $symbol_ = $words_[1];

  if ( index ( $symbol_, "OSE" ) == 0 || index ( $symbol_, "RTS" ) == 0 || index ( $symbol_, "MICEX" ) == 0 ) { next ; } 

  my $pnl_ = $words_[2];
  my $traded_vol_ = $words_[3];

  if ($exchange_to_volume_map_{$symbol_} != $traded_vol_ ||
      abs ($exchange_to_unrealpnl_map_{$symbol_} - $pnl_) >= 100.00) {
    print ("$date_\n\tORS :\t$symbol_ | PNL : $exchange_to_unrealpnl_map_{$symbol_} | VOL : $exchange_to_volume_map_{$symbol_}\n\tMFG:\t$symbol_ | PNL : $pnl_ | VOL : $traded_vol_\n\n");
  }
}


exit (0);

sub Buy
{
	my $symbol_ = shift;
	my $tprice_ = shift;
	my $tsize_ = shift;
	
	if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {
       $symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
    }else {
        $symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
    }
	
   $symbol_to_pos_map_{$symbol_} += $tsize_;
	
}

sub Sell
{
	my $symbol_ = shift;
	my $tprice_ = shift;
	my $tsize_ = shift;
	
  	if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {
       	$symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
  	}else {
       	$symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
   	}
    
    $symbol_to_pos_map_{$symbol_} -= $tsize_;
}

sub InitSymbol
{
	my $symbol_ = shift;
	
	$symbol_to_unrealpnl_map_{$symbol_} = 0;
    $symbol_to_pos_map_{$symbol_} = 0;
    $symbol_to_price_map_{$symbol_} = 0;
    $symbol_to_volume_map_{$symbol_} = 0;
}
sub UpdateVolumePricePnl
{
	my $symbol_ = shift;
	my $tprice_ = shift;
	my $tsize_ = shift;
	
	$symbol_to_volume_map_{$symbol_} += $tsize_;
    if( index ( $symbol_to_exchange_map_{$symbol_}, "BMF" ) != 0 ){
    	$symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
    }
    $symbol_to_price_map_{$symbol_} = $tprice_;
}

sub SetSecDef 
{
  my $symbol_ = shift;
  if ( index ( $symbol_, "ES" ) == 0 )
  {
    my $base_symbol_length_ = length ("ES");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "ES" ); 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 0.50;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
  }
  elsif ( index ( $symbol_, "NIY" ) == 0 )
  {
    my $base_symbol_length_ = length ("NIY");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "NIY" ); 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 500 * $JPY_TO_DOL ; 
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
  }
  elsif ( index ( $symbol_, "NKD" ) == 0 )
  {
    my $base_symbol_length_ = length ("NKD");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "NKD" ); 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 5 ;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
  }
  elsif ( index ( $symbol_, "UB" ) == 0 )
  {
    my $base_symbol_length_ = length ("UB");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "UB" ); 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
  }
  elsif ( index ( $symbol_, "ZB" ) == 0 )
  {
    my $base_symbol_length_ = length ("ZB");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "ZB" ); 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
  }
  elsif ( index ( $symbol_, "ZN" ) == 0 )
  {
    my $base_symbol_length_ = length ("ZN");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "ZN" ); 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
  }
  elsif ( index ( $symbol_, "ZF" ) == 0 )
  {
    my $base_symbol_length_ = length ("ZF");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "ZF" );
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
  }
  elsif ( index ( $symbol_, "ZT" ) == 0 )
  {
    my $base_symbol_length_ = length ("ZT");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "ZT" ); 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 2000;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
  }
  elsif ( index ( $symbol_, "FGBS" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FGBS" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "EUREX";

    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
  }
  elsif ( index ( $symbol_, "FGBM" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FGBM" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "EUREX";

    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
  }
  elsif ( index ( $symbol_, "FGBL" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FGBL" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "EUREX";

    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
  }
  elsif ( index ( $symbol_, "FGBX" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FGBX" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "EUREX";

    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
  }
  elsif ( index ( $symbol_, "FESX" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FESX" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 10 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "EUREX";

    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
  }
  elsif ( index ( $symbol_, "FOAT" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FOAT" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "EUREX";

    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
  }
  elsif ( index ( $symbol_, "FDAX" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FDAX" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 25 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "EUREX";

    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
  }
  elsif ( index ( $symbol_, "FBTP" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FBTP" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "EUREX";

    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
  }
  elsif ( index ( $symbol_, "FBTS" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FBTS" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "EUREX";

    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
  }

  elsif ( index ( $symbol_, "JFFCE" ) == 0 )
  {
    my $base_symbol_length_ = length ("JFFCE");
    my $expiry_year_offset_ = $base_symbol_length_ ;
    my $expiry_month_offset_ = $expiry_year_offset_ + 2;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "JFFCE" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

  }
  elsif ( index ( $symbol_, "KFFTI" ) == 0 )
  {
    my $base_symbol_length_ = length ("KFFTI");
    my $expiry_year_offset_ = $base_symbol_length_ ;
    my $expiry_month_offset_ = $expiry_year_offset_ + 2;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "KFFTI" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 200 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

  }
  elsif ( index ( $symbol_, "LFZ" ) == 0 )
  {
    my $base_symbol_length_ = length ("LFZ");
    my $expiry_year_offset_ = $base_symbol_length_ + 2;
    my $expiry_month_offset_ = $expiry_year_offset_ + 2;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "LFZ" ) * $GBP_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10 * $GBP_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

  }
  elsif ( index ( $symbol_, "L   FM" ) == 0 )
  {
#my $base_symbol_length_ = length ("LFL");
#   my $expiry_year_offset_ = $base_symbol_length_ + 2;
#   my $expiry_month_offset_ = $expiry_year_offset_ + 2;

#   my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
#    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    my $symbol_name_with_expiry_ = "LFL".$liffe_symbol_to_expiry_year_{substr($symbol_, 9, 2)}.$symbol_to_expiry_month_{substr($symbol_, 6, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;
    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "LFL" ) * $GBP_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1250 * $GBP_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

  }
  elsif ( index ( $symbol_, "Z   FM" ) == 0 )
  {
#my $base_symbol_length_ = length ("LFL");
#   my $expiry_year_offset_ = $base_symbol_length_ + 2;
#   my $expiry_month_offset_ = $expiry_year_offset_ + 2;

#   my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
#    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    my $symbol_name_with_expiry_ = "LFZ".$liffe_symbol_to_expiry_year_{substr($symbol_, 9, 2)}.$symbol_to_expiry_month_{substr($symbol_, 6, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;
    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "LFZ" ) * $GBP_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10 * $GBP_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

  }
  elsif ( index ( $symbol_, "R   FM" ) == 0 )
  {

#    my $base_symbol_length_ = length ("LFR");
#    my $expiry_year_offset_ = $base_symbol_length_ + 2;
#    my $expiry_month_offset_ = $expiry_year_offset_ + 2;
#
#    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
#    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;
#
    my $symbol_name_with_expiry_ = "LFR".$liffe_symbol_to_expiry_year_{substr($symbol_, 9, 2)}.$symbol_to_expiry_month_{substr($symbol_, 6, 1)};
        $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;
    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "LFR" ) * $GBP_TO_DOL ;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000 * $GBP_TO_DOL ;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

  }
  elsif ( index ( $symbol_, "YFEBM" ) == 0 )
  {

    my $base_symbol_length_ = length ("YFEBM");
    my $expiry_year_offset_ = $base_symbol_length_ ;
    my $expiry_month_offset_ = $expiry_year_offset_ + 2;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "YFEBM" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 50 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

  }
  elsif ( index ( $symbol_, "XFC" ) == 0 )
  {
    my $base_symbol_length_ = length ("XFC");
    my $expiry_year_offset_ = $base_symbol_length_ + 2;
    my $expiry_month_offset_ = $expiry_year_offset_ + 2;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "XFC" ) * $GBP_TO_DOL ;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10 * $GBP_TO_DOL ;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

  }
  elsif ( index ( $symbol_, "XFRC" ) == 0 )
  {
    my $base_symbol_length_ = length ("XFRC");
    my $expiry_year_offset_ = $base_symbol_length_ + 1;
    my $expiry_month_offset_ = $expiry_year_offset_ + 2;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "XFRC" ) * $GBP_TO_DOL ; 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10 ;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

  }
  elsif ( index ( $symbol_, "I   FM" ) == 0 )
  {
#    my $base_symbol_length_ = length ("LFI");
#    my $expiry_year_offset_ = $base_symbol_length_ + 2 ;
#    my $expiry_month_offset_ = $expiry_year_offset_ + 2;
#
#    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
#    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    my $symbol_name_with_expiry_ = "LFI".$liffe_symbol_to_expiry_year_{substr($symbol_, 9, 2)}.$symbol_to_expiry_month_{substr($symbol_, 6, 1)};
        $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;
    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "LFI" ) * $EUR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 2500 * $EUR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

  }
  elsif ( index ( $symbol_, "CGB" ) == 0 )
  {
    my $base_symbol_length_ = length ("CGB");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "CGB" ) * $CD_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000 * $CD_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "TMX";
  }
  elsif ( index ( $symbol_, "CGF" ) == 0 )
  {
    my $base_symbol_length_ = length ("CGF");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "CGF" ) * $CD_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000 * $CD_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "TMX";
  }
  elsif ( index ( $symbol_, "CGZ" ) == 0 )
  {
    my $base_symbol_length_ = length ("CGZ");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "CGZ" ) * $CD_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000 * $CD_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "TMX";
  }
  elsif ( index ( $symbol_, "BAX" ) == 0 )
  {
    my $base_symbol_length_ = length ("BAX");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "BAX" ) * $CD_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 2500 * $CD_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "TMX";
  }
  elsif ( index ( $symbol_, "SXF" ) == 0 )
  {
    my $base_symbol_length_ = length ("SXF");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "SXF" ) * $CD_TO_DOL;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 200 * $CD_TO_DOL;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "TMX";
  }
  elsif ( index ( $symbol_, "WIN" ) == 0 )
  {
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    $short_name_to_symbol_map_{"WIN"} =$symbol_;
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "WIN" ) * $BR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 0.2 * $BR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "BMF";
  }
  elsif ( index ( $symbol_, "IND" ) == 0 )
  {
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    $short_name_to_symbol_map_{"IND"} =$symbol_;
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "IND" ) * $BR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 1.0 * $BR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "BMF";
  }
  elsif ( index ( $symbol_, "DOL" ) == 0 )
  {
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    $short_name_to_symbol_map_{"DOL"} =$symbol_;
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "DOL" ) * $BR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 50.0 * $BR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "BMF";
  }
  elsif ( index ( $symbol_, "WDO" ) == 0 )
  {
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    $short_name_to_symbol_map_{"WDO"} =$symbol_;
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "WDO" )  * $BR_TO_DOL;
    $symbol_to_n2d_map_{$symbol_} = 10.0 * $BR_TO_DOL;
    $symbol_to_exchange_map_{$symbol_} = "BMF";
  }
  elsif ( index ( $symbol_, "DI" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "DI" ) * $BR_TO_DOL;

    if ( index ( $symbol_ , "DI1F13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140101 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F15" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150101 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F16" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160101 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F17" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170131 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F18" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20180131 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130731 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140731 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N15" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150731 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N16" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160731 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N17" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170731 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N18" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20180731 ) ;
    }
    elsif ( index ( $symbol_ , "DI1G13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130228 ) ;
    }
    elsif ( index ( $symbol_ , "DI1G14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140228 ) ;
    }
    elsif ( index ( $symbol_ , "DI1G15" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150228 ) ;
    }
    elsif ( index ( $symbol_ , "DI1J13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130430 ) ;
    }
    elsif ( index ( $symbol_ , "DI1J14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140431 ) ;
    }
    else
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170131 ) ;
    }

    $symbol_to_exchange_map_{$symbol_} = "BMF";
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_ ;
  }
  elsif ( index ( $symbol_, "HSI" ) == 0 )
  {

    my $base_symbol_length_ = length ("HSI");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "HSI" ) * $HKD_TO_DOL ;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 50.0 * $HKD_TO_DOL ; 
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "HKEX";

  }
  elsif ( index ( $symbol_, "HHI" ) == 0 )
  {

    my $base_symbol_length_ = length ("HHI");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "HHI" ) * $HKD_TO_DOL ; 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 50.0 * $HKD_TO_DOL ; 
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "HKEX";

  }
  elsif ( index ( $symbol_, "MHI" ) == 0 )
  {
    my $base_symbol_length_ = length ("MHI");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "MHI" ) * $HKD_TO_DOL ; 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10.0 * $HKD_TO_DOL ; 
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "HKEX";

  }
  elsif ( index ( $symbol_, "MCH" ) == 0 )
  {
    my $base_symbol_length_ = length ("MCH");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "MCH" ) * $HKD_TO_DOL ; 
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10.0 * $HKD_TO_DOL ; 
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "HKEX";
  }
  elsif ( index ( $symbol_, "NKM" ) == 0 )
  {

    my $base_symbol_length_ = length ("NKM");
    my $expiry_year_offset_ = $base_symbol_length_;
    my $expiry_month_offset_ = $expiry_year_offset_ + 2 ;

#	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;

    my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$ose_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2)}.substr ( $symbol_, $expiry_month_offset_, 2) ; 
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "NKM" ) * $JPY_TO_DOL ;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 100 * $JPY_TO_DOL ;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "OSE";

  }
  elsif ( index ( $symbol_, "GD" ) == 0 )
  {
    my $base_symbol_length_ = length ("GD");
    my $expiry_month_offset_ = $base_symbol_length_;
    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

#my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
    my $symbol_name_with_expiry_ = $symbol_;
    $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

    $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "GD" )* $RUB_TO_USD_;
#     $symbol_to_commish_map_{$symbol_name_with_expiry_} = 1.2* $RUB_TO_USD_;
    $symbol_to_n2d_map_{$symbol_name_with_expiry_} = GetRTSTickSize( "GD_0" ) * $RUB_TO_USD_;
    $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "RTS";
  }

  elsif ( index ( $symbol_, "VX" ) == 0 )
  {
    if( index ( $symbol_, "_") == -1)
    {
      my $symbol_name_with_expiry_ = $symbol_;
      $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
      $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "VX" ) ;
      $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000;
      $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CFE";

    }
  }
    else {

      if  ( index ( $symbol_, "NK" ) == 0 )
      {
        my $base_symbol_length_ = length ("NK");
        my $expiry_year_offset_ = $base_symbol_length_;
        my $expiry_month_offset_ = $expiry_year_offset_ + 2;

        my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$ose_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_,2)}.substr ($symbol_, $expiry_month_offset_, 2);
        $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

        $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "NK" ) * $JPY_TO_DOL ;
        $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000 * $JPY_TO_DOL ;
        $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "OSE";

      }

    }

  }
