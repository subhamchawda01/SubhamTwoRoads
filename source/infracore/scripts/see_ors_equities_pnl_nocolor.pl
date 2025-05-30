#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;
use List::Util qw/max min/; # for max

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $INSTALL_BIN="$HOME_DIR/$REPO"."_install/bin/";
my $LIVE_BIN="$HOME_DIR/LiveExec/bin/";

require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/get_hft_commission_discount_for_shortcode.pl"; # Get hft discount according to volume tiers for shortcode
require "$GENPERLLIB_DIR/get_cme_commission_discount_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_cme_eu_volumes_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_number_of_working_days_BMF_between_two_dates.pl" ;
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; #
require "$GENPERLLIB_DIR/get_rts_ticksize.pl"; #RITickSize
require "$GENPERLLIB_DIR/get_shortcode_from_symbol.pl"; #GetShortcodeFromSymbol

my $USAGE="$0 ors_trades_filename_ [ date_ ] \n";
if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];
my $input_date_ = -1;
my $today_date_ = `date +"%Y%m%d"` ; chomp ( $today_date_ ) ;
my $LOAD_ = 1;

if ( $#ARGV > 0 ){
  $input_date_ = int( $ARGV[1] );
  $today_date_ = $input_date_;
}

if ( $#ARGV > 1 )
{
 $LOAD_ = int( $ARGV[2]) ;
}

my $EUR_TO_DOL = 1.30; # Should have a way of looking this up from the currency info file.
my $CD_TO_DOL =  0.97; # canadian dollar
my $BR_TO_DOL = 0.51;
my $GBP_TO_DOL = 1.57;
my $HKD_TO_DOL = 0.1289 ;
my $JPY_TO_DOL = 0.0104 ;
my $RUB_TO_DOL = 0.031 ;
my $AUD_TO_DOL = 0.819 ;

my %symbol_to_expiry_year_ = ("0" => "2010", "1" => "2011", "2" => "2012", "3" => "2013", "4" => "2014", "5" => "2015", "6" => "2016", "7" => "2017", "8" => "2018", "9" => "2019");
my %symbol_to_expiry_month_ = ("F" => "01", "G" => "02", "H" => "03", "J" => "04", "K" => "05", "M" => "06", "N" => "07", "Q" => "08", "U" => "09", "V" => "10", "X" => "11", "Z" => "12");
my %spread_to_pos_symbol_ =();
my %spread_to_neg_symbol_ =();

my %micex_bcs_fee_tiers_ = ();
$micex_bcs_fee_tiers_{"5000"} = 0.006;
$micex_bcs_fee_tiers_{"10000"} = 0.005;
$micex_bcs_fee_tiers_{"15000"} = 0.004;
$micex_bcs_fee_tiers_{"10000000000"} = 0.003;
my $micex_exchange_fee_ = 0.008 ;
my $bmfeq_exchange_fee_ = 0.00025 ;
my $bmfeq_bp_fee_ = 0.00003;
#================================= fetching conversion rates from currency info file if available ====================#

#my $yesterday_file_ = "/tmp/YESTERDAY_DATE" ;
#open YESTERDAY_FILE_HANDLE, "< $yesterday_file_" ;
#
#my @yesterday_lines_ = <YESTERDAY_FILE_HANDLE> ;
#close YESTERDAY_FILE_HANDLE ;
#
#my @ywords_ = split(' ', $yesterday_lines_[0] );
#my $yesterday_date_ = $ywords_[0];

my $yesterday_date_ = `/home/dvcinfra/LiveExec/bin/calc_prev_week_day $today_date_`;

my $prev_date_ = $yesterday_date_ ; 

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
      
    if( index ( $currency_, "USDAUD" ) == 0 ){
      $AUD_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
    }
    
    }
  }
}

#======================================================================================================================#


my $targetcol = 9;

my $ors_trades_file_base_ = basename ($ors_trades_filename_); chomp ($ors_trades_file_base_);

open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "add_results_to_local_database.pl could not open ors_trades_filename_ $ors_trades_filename_\n";

my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;

close ORS_TRADES_FILE_HANDLE;

my %symbol_to_commish_map_ = ();
my %symbol_to_n2d_map_ = ();
my %symbol_to_noof_working_days_till_expiry_ = ();


my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();

my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();

my %symbol_to_mkt_vol_ =();

my $wdo_secname = `$LIVE_BIN/get_exchange_symbol BR_WDO_0 $today_date_`;
my $dol_secname = `$LIVE_BIN/get_exchange_symbol BR_DOL_0 $today_date_`;
my $win_secname = `$LIVE_BIN/get_exchange_symbol BR_WIN_0 $today_date_`;
my $ind_secname = `$LIVE_BIN/get_exchange_symbol BR_IND_0 $today_date_`;

#=================================================================================================================
if ( $LOAD_ == 1 ){
my $overnight_pos_file = "/spare/local/files/EODPositions/overnight_pnls_$prev_date_.txt";
if (-e $overnight_pos_file) {
  open OVN_FILE_HANDLE, "< $overnight_pos_file" or die "add_results_to_local_database.pl could not open ors_trades_filename_ $overnight_pos_file\n";

  my @ovn_file_lines_ = <OVN_FILE_HANDLE>;
  close OVN_FILE_HANDLE;
  for ( my $i = 0 ; $i <= $#ovn_file_lines_; $i ++ )
  {
    my @words_ = split ( ',', $ovn_file_lines_[$i] );
    if ( $#words_ >= 2 )
    {

      my $symbol_ = $words_[0];
      SetSecDef ( $symbol_ ) ;
#if ( ($symbol_to_exchange_map_{$symbol_} eq $EXCH_) or ($EXCH_ eq '') )
#     {
#       ;
#     }
#     else
#     {
#       next;
#     }
      $symbol_to_pos_map_{$symbol_} = $words_[1];
      $symbol_to_price_map_{$symbol_} = $words_[2];
      if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {
          $symbol_to_unrealpnl_map_{$symbol_} = $symbol_to_pos_map_{$symbol_} * ( 100000 / ( $symbol_to_price_map_{$symbol_} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
        }
        else
        {
      		$symbol_to_unrealpnl_map_{$symbol_} = -$symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_};
        }
      $symbol_to_volume_map_{$symbol_} = 0;
    }
  }
}
}
#=================================================================================================================

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

    if ( ! ( exists $symbol_to_unrealpnl_map_{$symbol_} ) )
    {
      $symbol_to_unrealpnl_map_{$symbol_} = 0;
      $symbol_to_pos_map_{$symbol_} = 0;
      $symbol_to_price_map_{$symbol_} = 0;
      $symbol_to_volume_map_{$symbol_} = 0;
      SetSecDef ( $symbol_ ) ;
    }
    if( index ( $symbol_to_exchange_map_{$symbol_}, "MICEX" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = ( max ( 1 , $micex_exchange_fee_ * $tprice_ * $tsize_ ) + $micex_bcs_fee_tiers_{5000} * $tprice_ * $tsize_ ) * $RUB_TO_DOL ;
      $symbol_to_commish_map_{$symbol_} /= $tsize_ ;
    }
    if( index ( $symbol_to_exchange_map_{$symbol_}, "BMFEQ" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = ( $bmfeq_bp_fee_ + $bmfeq_exchange_fee_) * $tprice_ * $tsize_   * $BR_TO_DOL;
      $symbol_to_commish_map_{$symbol_} /= $tsize_ ;
    }

    if ( $buysell_ == 0 )
    { # buy

      if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {

        $symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;


      }else {

        $symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};

      }

      $symbol_to_pos_map_{$symbol_} += $tsize_;

    }
    else
    {

      if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {

        $symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;

      }else {

        $symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};

      }

      $symbol_to_pos_map_{$symbol_} -= $tsize_;

    }

    $symbol_to_volume_map_{$symbol_} += $tsize_;
    $symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
    $symbol_to_price_map_{$symbol_} = $tprice_;

    if ($symbol_ eq $wdo_secname && (exists $symbol_to_unrealpnl_map_{$dol_secname})) {
        $symbol_to_price_map_{$dol_secname} = $tprice_;
    }
    elsif ($symbol_ eq $dol_secname && (exists $symbol_to_unrealpnl_map_{$wdo_secname})) {
        $symbol_to_price_map_{$wdo_secname} = $tprice_;
    }
    elsif ($symbol_ eq $win_secname && (exists $symbol_to_unrealpnl_map_{$ind_secname})) {
        $symbol_to_price_map_{$ind_secname} = $tprice_;
    }
    elsif ($symbol_ eq $ind_secname && (exists $symbol_to_unrealpnl_map_{$win_secname})) {
        $symbol_to_price_map_{$win_secname} = $tprice_;
    }
  }
}

my $date_ = `date`;
printf ("\n$date_\n");

my $totalpnl_ = 0.0;
foreach my $symbol_ ( sort keys %symbol_to_price_map_ )
{
  if ( ! ($symbol_to_exchange_map_{$symbol_} eq "BMFEQ" )){ next ; } 
  my $unreal_pnl_ = 0 ;

  if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {

    $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} - $symbol_to_pos_map_{$symbol_} * ( ( 100000 / ( $symbol_to_price_map_{$symbol_} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ) - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};

  }else{

    $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};

  }

# BMF has different tiers of discount for HFT commission
  if( index ( $symbol_to_exchange_map_{$symbol_}, "BMF" ) == 0 ){

    my $shortcode_ = "";

    if ( index ( $symbol_, "DOL" ) == 0 ) { $shortcode_ = "DOL" ; }
    if ( index ( $symbol_, "WDO" ) == 0 ) { $shortcode_ = "WDO" ; }
    if ( index ( $symbol_, "WIN" ) == 0 ) { $shortcode_ = "WIN" ; }
    if ( index ( $symbol_, "IND" ) == 0 ) { $shortcode_ = "IND" ; }

    $unreal_pnl_ += ( $symbol_to_volume_map_{$symbol_} * GetHFTDiscount( $shortcode_, $symbol_to_volume_map_{$symbol_} ) * $BR_TO_DOL ) ;

  }elsif ( index ( $symbol_to_exchange_map_{$symbol_}, "CME" ) == 0 ){

    my $shortcode_ ="" ;

    if ( index ( $symbol_, "ZN" ) == 0 ) { $shortcode_ = "ZN" ; }
    if ( index ( $symbol_, "ZF" ) == 0 ) { $shortcode_ = "ZF" ; }
    if ( index ( $symbol_, "ZB" ) == 0 ) { $shortcode_ = "ZB" ; }
    if ( index ( $symbol_, "UB" ) == 0 ) { $shortcode_ = "UB" ; }

    my $trade_date_ = `date +"%Y%m%d"` ;

    my $trade_hours_string_ = "US" ;

    $unreal_pnl_ += ( ( $symbol_to_volume_map_{$symbol_} - GetCMEEUVolumes( $shortcode_, $trade_date_ ) ) * GetCMEDiscount( $shortcode_, $trade_hours_string_ ) ) ;

  }
  
  GetMktVol($symbol_);

  printf "| %16s ", $symbol_;
  printf "| PNL : ";
  printf "%10.3f ", $unreal_pnl_;
  my $last_closing_price = sprintf("%.4f", $symbol_to_price_map_{$symbol_});
  printf "| POS : %4d | VOL : %4d | v/V: %.1f | LPX : %s |\n", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_},$symbol_to_volume_map_{$symbol_}/$symbol_to_mkt_vol_{$symbol_}, $last_closing_price;
  $totalpnl_ += $unreal_pnl_;

  $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
  $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};
}

$totalpnl_ = 0.0;
my $totalvol_ = 0;

if ( exists $exchange_to_unrealpnl_map_{"EUREX"} )
{
  printf "\n\n%17s |", "EUREX";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"EUREX"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"EUREX"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"EUREX"};
  $totalvol_ += $exchange_to_volume_map_{"EUREX"};
}

if ( exists $exchange_to_unrealpnl_map_{"LIFFE"} )
{
  printf "%17s |", "LIFFE";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"LIFFE"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"LIFFE"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"LIFFE"};
  $totalvol_ += $exchange_to_volume_map_{"LIFFE"};
}

if ( exists $exchange_to_unrealpnl_map_{"ICE"} )
{
  printf "%17s |", "ICE";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"ICE"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"ICE"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"ICE"};
  $totalvol_ += $exchange_to_volume_map_{"ICE"};
}
if ( exists $exchange_to_unrealpnl_map_{"TMX"} )
{
  printf "%17s |", "TMX";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"TMX"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"TMX"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"TMX"};
  $totalvol_ += $exchange_to_volume_map_{"TMX"};
}

if ( exists $exchange_to_unrealpnl_map_{"CME"} )
{
  printf "%17s |", "CME";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"CME"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"CME"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"CME"};
  $totalvol_ += $exchange_to_volume_map_{"CME"};
}

if ( exists $exchange_to_unrealpnl_map_{"BMF"} )
{
  printf "%17s |", "BMF";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"BMF"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"BMF"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"BMF"};
  $totalvol_ += $exchange_to_volume_map_{"BMF"};
}

if ( exists $exchange_to_unrealpnl_map_{"BMFEQ"} )
{
  printf "%17s |", "BMFEQ";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"BMFEQ"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"BMFEQ"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"BMFEQ"};
  $totalvol_ += 0 ; #ignoring BMFEQ volumes 
}

if ( exists $exchange_to_unrealpnl_map_{"HKEX"} )
{
  printf "%17s |", "HKEX";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"HKEX"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"HKEX"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"HKEX"};
  $totalvol_ += $exchange_to_volume_map_{"HKEX"};
}

if ( exists $exchange_to_unrealpnl_map_{"OSE"} )
{
  printf "%17s |", "OSE";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"OSE"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"OSE"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"OSE"};
  $totalvol_ += $exchange_to_volume_map_{"OSE"};
}
if ( exists $exchange_to_unrealpnl_map_{"RTS"} )
{

  printf "%17s |", "RTS";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"RTS"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"RTS"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"RTS"};
  $totalvol_ += $exchange_to_volume_map_{"RTS"};

}
if ( exists $exchange_to_unrealpnl_map_{"MICEX"} )
{

  printf "%17s |", "MICEX";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"MICEX"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"MICEX"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"MICEX"};
  $totalvol_ += $exchange_to_volume_map_{"MICEX"};

}

if ( exists $exchange_to_unrealpnl_map_{"CFE"} )
{
  printf "\n\n%17s |", "CFE";
  printf "| PNL : ";
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"CFE"};
  printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"CFE"};

  $totalpnl_ += $exchange_to_unrealpnl_map_{"CFE"};
  $totalvol_ += $exchange_to_volume_map_{"CFE"};
}


printf "\n%17s |", "TOTAL";
printf "| PNL : ";
printf "%10.3f ", $totalpnl_;
printf "| VOLUME : %4d |\n", $totalvol_;

exit ( 0 );

sub GetMktVol
{
	my $symbol_ = shift ;
	my $vol_ = `$HOME_DIR/infracore/scripts/get_curr_mkt_vol.sh "$symbol_"` ; chomp($vol_);

	my $is_valid_ = `echo -n $vol_ | wc -w` ; chomp($is_valid_);
	my $is_real_ = 0;
      
       if (  $vol_ =~ /^-?\d+\.?\d*\z/ )
       {
          $is_real_ = 1 ;       
       }

        $is_valid_ = $is_valid_ || $is_real_ ;

	if ( ! exists( $symbol_to_mkt_vol_{$symbol_} )  )
	{
		$symbol_to_mkt_vol_{$symbol_} = -1000000000 ;
	}
	
	if ( $is_valid_ == 1 && $is_real_ == 1  )
	{
		$symbol_to_mkt_vol_{$symbol_} = ( $vol_ + 1 ) / 100 ;
	}
	
#print "$symbol_, $vol_, $is_valid_, $is_real_, $symbol_to_mkt_vol_{$symbol_}\n";
	
}

sub SetSecDef
{
  my $symbol_ = shift;
  if ( index ( $symbol_, "DI" ) == 0 )
  {
    $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "DI" ) * $BR_TO_DOL;
    if ( index ( $symbol_ , "DI1F13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F15" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F16" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F17" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F18" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20180102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F19" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20190102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F20" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20200102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F21" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20210102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F22" ) >= 0 )
    {
	$symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20220102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F23" ) >= 0 )
    {
	$symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20230102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F24" ) >= 0 )
    {
	$symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20240102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F25" ) >= 0 )
    {
	$symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20250102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N15" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N16" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N17" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N18" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20180701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1G13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130201 ) ;
    }
    elsif ( index ( $symbol_ , "DI1G14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140201 ) ;
    }
    elsif ( index ( $symbol_ , "DI1G15" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150201 ) ;
    }
    elsif ( index ( $symbol_ , "DI1J13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130401 ) ;
    }
    elsif ( index ( $symbol_ , "DI1J14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140401 ) ;
    }
    elsif ( index ( $symbol_ , "DI1J15" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150401 ) ;
    }
    elsif ( index ( $symbol_ , "DI1J16" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160401 ) ;
    }
    elsif ( index ( $symbol_ , "DI1J17" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170401 ) ;
    }
    else
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170131 ) ;
    }

    $symbol_to_exchange_map_{$symbol_} = "BMF";
  }
  elsif ( index ( $symbol_, "VX" ) == 0 )
  {
    if( index ( $symbol_, "_") != -1)
    {
      my $base_symbol_length_ = length ("VX");
      my $expiry_month_offset_ = $base_symbol_length_;
      my $expiry_year_offset_ = $expiry_month_offset_ + 1;

      my $symbol_1_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_,1)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
      $spread_to_neg_symbol_{$symbol_}=$symbol_1_name_with_expiry_;
      $base_symbol_length_ = length ("VX-----");
      $expiry_month_offset_ = $base_symbol_length_;
      $expiry_year_offset_ = $expiry_month_offset_ + 1;

      my $symbol_2_name_with_expiry_ = substr ($symbol_, 0, length("VX")).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
      $spread_to_pos_symbol_{$symbol_}=$symbol_2_name_with_expiry_;
      $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "VX" ) ;
    }
    else
    {
      $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "VX2" ) ;
    }
    $symbol_to_n2d_map_{$symbol_} = 1000 ;
    $symbol_to_exchange_map_{$symbol_} = "CFE";
  }
  else
  {
    my $shc_ = `$INSTALL_BIN/get_shortcode_for_symbol  "$symbol_" $today_date_ ` ; chomp ( $shc_ ) ;
    my $exchange_name_ = `$INSTALL_BIN/get_contract_specs $shc_ $today_date_ EXCHANGE | awk '{print \$2}'`; chomp ( $exchange_name_ ) ;
    if ( $exchange_name_ eq "HONGKONG" ) { $exchange_name_ = "HKEX" ; } 
    if ( index ( $exchange_name_, "MICEX" ) == 0 )  { $exchange_name_ = "MICEX" ; }
    $symbol_to_exchange_map_{$symbol_}=$exchange_name_;
    my $commish_ = `$INSTALL_BIN/get_contract_specs "$shc_" $today_date_ COMMISH | awk '{print \$2}'`; chomp ( $commish_ ) ;
    $symbol_to_commish_map_ {$symbol_} =  $commish_ ;#GetCommissionForShortcode ( $symbol_ ) ;
    my $n2d_ = `$INSTALL_BIN/get_contract_specs $shc_ $today_date_ N2D | awk '{print \$2}'`; chomp $n2d_ ;
    $symbol_to_n2d_map_{$symbol_} = $n2d_ ;
  }
}
