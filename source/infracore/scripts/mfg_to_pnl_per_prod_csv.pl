#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $USAGE="$0 mfglobal_trades_filename_ YYYYMMDD";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $mfg_trades_filename_ = $ARGV[0];
my $expected_date_ = $ARGV[1];

my $REPO="infracore";
my $GENPERLLIB_DIR=$ENV{"HOME"}."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode

my %mfg_exch_future_code_to_shortcode_ = ();
my %mfg_exch_future_code_to_n2d_ = ();
my %mfg_exch_future_code_to_exchange_ = ();

my %broker_symbol_to_exchange_symbol_ = ();
my %shortcode_to_pnl_ = ();
my %shortcode_to_last_pos_ = ();
my %shortcode_to_last_price_ = ();
my %shortcode_to_n2d_ = ();
my %shortcode_to_volume_ = ();
my %shortcode_to_exchange_ = ();
my %shortcode_to_trade_time_ = ();
my %fee_discrepancy_shortcodes_ = ();
my %shortcode_to_comm_ = ();
my %shortcode_to_fee1_ = ();
my %shortcode_to_fee2_ = ();
my %shortcode_to_fee3_ = ();
my %shortcode_to_fee4_ = ();

# These instruments need to be configured with NewEdge instrument codes.
$mfg_exch_future_code_to_shortcode_{"22NI"} = "NK";
$mfg_exch_future_code_to_n2d_{"22NI"} = 0;
$mfg_exch_future_code_to_exchange_{"22NI"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"22MN"} = "NKM";
$mfg_exch_future_code_to_n2d_{"22MN"} = 0;
$mfg_exch_future_code_to_exchange_{"22MN"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"24RV"} = "JGBL";
$mfg_exch_future_code_to_n2d_{"24RV"} = 0;
$mfg_exch_future_code_to_exchange_{"24RV"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"24TP"} = "TOPIX";
$mfg_exch_future_code_to_n2d_{"24TP"} = 0;
$mfg_exch_future_code_to_exchange_{"24TP"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"22N4"} = "JP400";
$mfg_exch_future_code_to_n2d_{"22N4"} = 0;
$mfg_exch_future_code_to_exchange_{"22N4"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"27SD"} = "FGBS";
$mfg_exch_future_code_to_n2d_{"27SD"} = 1000;
$mfg_exch_future_code_to_exchange_{"27SD"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27BC"} = "FGBM";
$mfg_exch_future_code_to_n2d_{"27BC"} = 1000;
$mfg_exch_future_code_to_exchange_{"27BC"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27VS"} = "FVS";
$mfg_exch_future_code_to_n2d_{"27VS"} = 1000;
$mfg_exch_future_code_to_exchange_{"27VS"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27BM"} = "FGBL";
$mfg_exch_future_code_to_n2d_{"27BM"} = 1000;
$mfg_exch_future_code_to_exchange_{"27BM"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27SF"} = "FESX";
$mfg_exch_future_code_to_n2d_{"27SF"} = 10;
$mfg_exch_future_code_to_exchange_{"27SF"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27I*"} = "FOAT";
$mfg_exch_future_code_to_n2d_{"27I*"} = 1000;
$mfg_exch_future_code_to_exchange_{"27I*"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27FE"} = "FDAX";
$mfg_exch_future_code_to_n2d_{"27FE"} = 25;
$mfg_exch_future_code_to_exchange_{"27FE"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27F7"} = "FBTP";
$mfg_exch_future_code_to_n2d_{"27F7"} = 1000;
$mfg_exch_future_code_to_exchange_{"27F7"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27FA"} = "FBTS";
$mfg_exch_future_code_to_n2d_{"27FA"} = 1000;
$mfg_exch_future_code_to_exchange_{"27FA"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27BV"} = "FGBX";
$mfg_exch_future_code_to_n2d_{"27BV"} = 1000;
$mfg_exch_future_code_to_exchange_{"27BV"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"30CA"} = "JFFCE";
$mfg_exch_future_code_to_n2d_{"30CA"} = 10;
$mfg_exch_future_code_to_exchange_{"30CA"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"28EJ"} = "KFFTI";
$mfg_exch_future_code_to_n2d_{"28EJ"} = 200;
$mfg_exch_future_code_to_exchange_{"28EJ"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"05SF"} = "LFI";
$mfg_exch_future_code_to_n2d_{"05SF"} = 2500;
$mfg_exch_future_code_to_exchange_{"05SF"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"05RJ"} = "LFL";
$mfg_exch_future_code_to_n2d_{"05RJ"} = 1250;
$mfg_exch_future_code_to_exchange_{"05RJ"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"05RH"} = "LFR";
$mfg_exch_future_code_to_n2d_{"05RH"} = 1000;
$mfg_exch_future_code_to_exchange_{"05RH"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"05FT"} = "LFZ";
$mfg_exch_future_code_to_n2d_{"05FT"} = 10;
$mfg_exch_future_code_to_exchange_{"05FT"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"25B3"} = "YFEBM";
$mfg_exch_future_code_to_n2d_{"25B3"} = 50 ;
$mfg_exch_future_code_to_exchange_{"25B3"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"14LA"} = "XFC";
$mfg_exch_future_code_to_n2d_{"14LA"} = 10 ;
$mfg_exch_future_code_to_exchange_{"14LA"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"14RC"} = "XFRC";
$mfg_exch_future_code_to_n2d_{"14RC"} = 10 ;
$mfg_exch_future_code_to_exchange_{"14RC"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"0125"} = "ZF";
$mfg_exch_future_code_to_n2d_{"0125"} = 1000;
$mfg_exch_future_code_to_exchange_{"0125"} = "CME";

$mfg_exch_future_code_to_shortcode_{"0121"} = "ZN";
$mfg_exch_future_code_to_n2d_{"0121"} = 1000;
$mfg_exch_future_code_to_exchange_{"0121"} = "CME";

$mfg_exch_future_code_to_shortcode_{"0126"} = "ZT";
$mfg_exch_future_code_to_n2d_{"0126"} = 1000;
$mfg_exch_future_code_to_exchange_{"0126"} = "CME";

$mfg_exch_future_code_to_shortcode_{"16N1"} = "NIY";
$mfg_exch_future_code_to_n2d_{"16N1"} = 500;
$mfg_exch_future_code_to_exchange_{"16N1"} = "CME";  # one chicago 

$mfg_exch_future_code_to_shortcode_{"16NK"} = "NKD";
$mfg_exch_future_code_to_n2d_{"16NK"} = 50;
$mfg_exch_future_code_to_exchange_{"16NK"} = "CME"; # one chicago 

$mfg_exch_future_code_to_shortcode_{"01CBT 2YR T-NOTE"} = "ZT";
$mfg_exch_future_code_to_n2d_{"01CBT 2YR T-NOTE"} = 2000;
$mfg_exch_future_code_to_exchange_{"01CBT 2YR T-NOTE"} = "CME";

$mfg_exch_future_code_to_shortcode_{"0117"} = "ZB";
$mfg_exch_future_code_to_n2d_{"0117"} = 1000;
$mfg_exch_future_code_to_exchange_{"0117"} = "CME";

$mfg_exch_future_code_to_shortcode_{"01UL"} = "UB";
$mfg_exch_future_code_to_n2d_{"01UL"} = 1000;
$mfg_exch_future_code_to_exchange_{"01UL"} = "CME";

$mfg_exch_future_code_to_shortcode_{"10BA"} = "BAX";
$mfg_exch_future_code_to_n2d_{"10BA"} = 2500;
$mfg_exch_future_code_to_exchange_{"10BA"} = "TMX";

$mfg_exch_future_code_to_shortcode_{"10CG"} = "CGB";
$mfg_exch_future_code_to_n2d_{"10CG"} = 1000;
$mfg_exch_future_code_to_exchange_{"10CG"} = "TMX";

$mfg_exch_future_code_to_shortcode_{"10CF"} = "CGF";
$mfg_exch_future_code_to_n2d_{"10CF"} = 1000;
$mfg_exch_future_code_to_exchange_{"10CF"} = "TMX";

$mfg_exch_future_code_to_shortcode_{"10CZ"} = "CGZ";
$mfg_exch_future_code_to_n2d_{"10CZ"} = 2000;
$mfg_exch_future_code_to_exchange_{"10CZ"} = "TMX";

$mfg_exch_future_code_to_shortcode_{"10SX"} = "SXF";
$mfg_exch_future_code_to_n2d_{"10SX"} = 200;
$mfg_exch_future_code_to_exchange_{"10SX"} = "TMX";

$mfg_exch_future_code_to_shortcode_{"7BVX"} = "VX";
$mfg_exch_future_code_to_n2d_{"7BVX"} = 1000;
$mfg_exch_future_code_to_exchange_{"7BVX"} = "CFE";

$mfg_exch_future_code_to_shortcode_{"16ES"} = "ES";
$mfg_exch_future_code_to_n2d_{"16ES"} = 0;
$mfg_exch_future_code_to_exchange_{"16ES"} = "CME";

$mfg_exch_future_code_to_shortcode_{"16ED"} = "ED";
$mfg_exch_future_code_to_n2d_{"16ED"} = 0;
$mfg_exch_future_code_to_exchange_{"16ED"} = "CME";

$mfg_exch_future_code_to_shortcode_{"16MP"} = "6M";
$mfg_exch_future_code_to_n2d_{"16MP"} = 0;
$mfg_exch_future_code_to_exchange_{"16MP"} = "CME";



my %symbol_to_expiry_month_ = ("F" => "01", "G" => "02", "H" => "03", "J" => "04", "K" => "05", "M" => "06", "N" => "07", "Q" => "08", "U" => "09", "V" => "10", "X" => "11", "Z" => "12");
my %expiry_month_to_symbol_ = ("01" => "F", "02" => "G", "03" => "H", "04" => "J", "05" => "K", "06" => "M", "07" => "N", "08" => "Q", "09" => "U", "10" => "V", "11" => "X", "12" => "Z");

# These offsets were obtained using the 'FIMST4F1.xls & FIMST4F1.csv' template files
# These will be used to determine the field being processed after splitting each trade line

my $record_id_offset_ = 0; # PRECID # Needed to figure out which records to use towards pnl computation.
my $date_offset_ = 11; # PTDATE
my $buysell_offset_ = 12; # PBS # 1 = buy ; 2 = sell
my $quantity_offset_ = 17; # PQTY
my $mfg_future_code_offset_ = 24; # PFC
my $exch_code_offset_ = 23; # PEXCH
my $contract_yr_month_offset_ = 6; # PCTYM  # YYYYMM
my $trade_price_offset_ = 27; # PTPRIC # Trade price may be different than price of order
my $closing_price_offset_ = 28 ; 
my $time_offset_ = 50;
my $spread_id_offset_ = 13;

# Need to figure out how many charges are placed on us.
my $comm_amount_offset_ = 37;
my $fee1_amount_offset_ = 38;
my $fee2_amount_offset_ = 39;
my $fee3_amount_offset_ = 40;
my $fee4_amount_offset_ = 41;

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


# pnl computation from the MFG trades files are simpler, given the commission amounts
open MFG_TRADES_FILE_HANDLE, "< $mfg_trades_filename_" or die "could not open mfg_trades_filename_ $mfg_trades_filename_\n";

my @mfg_trades_file_lines_ = <MFG_TRADES_FILE_HANDLE>;

close MFG_TRADES_FILE_HANDLE;

for (my $i = 0; $i <= $#mfg_trades_file_lines_; $i++) {
  my @fields_ = split (',', $mfg_trades_file_lines_[$i]);

  if ($#fields_ != 120) {
#	print "Malformatted line : $mfg_trades_file_lines_[$i] No. of fields : $#fields_\n";
    next;
  }

  if (length ($fields_[$mfg_future_code_offset_]) < 2) {
    next;
  }

# NewEdge trade entries are listed twice in the file.
# We only use the one specifying trade information.
  my $record_id_ = substr ($fields_[$record_id_offset_], 1, -1); $record_id_ =~ s/^\s+|\s+$//g;
 # my $spread_id_ = substr ($fields_[$spread_id_offset_], 1, -1); $spread_id_ =~ s/^\s+|\s+$//g;

  if ($record_id_ ne "T") {
    next;
  }
  #if ($spread_id_ eq "S" and $expected_date_ < 20141220) {
   # next;
  #}


# Handling use of same future code for multiple products.
# E.g. FESX & EURIBOR use "SF"

  my $exch_code_ = substr ( $fields_ [ $exch_code_offset_ ] , 1 , -1 ); $exch_code_ =~ s/^\s+|\s+$//g;
  my $future_code_ = substr ($fields_[$mfg_future_code_offset_], 1, -1); $future_code_ =~ s/^\s+|\s+$//g;

  my $exch_future_code_ = $exch_code_.$future_code_;

  if (!exists ( $mfg_exch_future_code_to_shortcode_{$exch_future_code_})) {
	print "\n $exch_future_code_ \n";
	exit ( 1 );
  }

  my $date_ = substr ($fields_[$date_offset_], 1, -1); $date_ =~ s/^\s+|\s+$//g;

  if ($date_ ne $expected_date_) {
#	print "MFGTrades file doesnot match date provided\n";
#	exit (1);
next;
  }

  my $contract_date_ = substr ($fields_[$contract_yr_month_offset_], 1, -1); $contract_date_ =~ s/^\s+|\s+$//g;
  
  my $shortcode_ = $mfg_exch_future_code_to_shortcode_{$exch_future_code_}.$contract_date_;
  
  my $broker_symbol=$shortcode_;
  if(! exists $broker_symbol_to_exchange_symbol_{$broker_symbol}){
	  
	  if(substr($mfg_exch_future_code_to_shortcode_{$exch_future_code_},0,2) eq "LF"){
	  	my $contract_month=substr($contract_date_,4,2);
	  	my $contract_year=substr($contract_date_,2,2);
	  	$shortcode_ = substr($mfg_exch_future_code_to_shortcode_{$exch_future_code_}, 2, 1);
	  	$shortcode_ = $shortcode_."~~~FM";
	  	$shortcode_ = $shortcode_.$expiry_month_to_symbol_{$contract_month};
	  	$shortcode_ = $shortcode_."00";
	  	$shortcode_ = $shortcode_.$contract_year;
	  	$shortcode_ = $shortcode_."!";
	  }
	  
	  my $first_three= substr($mfg_exch_future_code_to_shortcode_{$exch_future_code_},0,3);
	  if($first_three eq "BAX" || $first_three eq "CGB" || $first_three eq "SXF" || $first_three eq "NIY" || $first_three eq "NKD"){
	  	my $contract_month=substr($contract_date_,4,2);
	  	my $contract_year=substr($contract_date_,3,1);
	  	$shortcode_ = substr($shortcode_,0,3);
	  	$shortcode_ = $shortcode_.$expiry_month_to_symbol_{$contract_month};
	  	$shortcode_ = $shortcode_.$contract_year;
	  }
	  
	  my $first_two= substr($mfg_exch_future_code_to_shortcode_{$exch_future_code_},0,2);
	  if($first_two eq "ZB" || $first_two eq "ZN" || $first_two eq "ZF" || $first_two eq "UB" 
	      || $first_two eq "ZT" || $first_two eq "ES" || $first_two eq "ED" || $first_two eq "6M"){
	  	my $contract_month=substr($contract_date_,4,2);
	  	my $contract_year=substr($contract_date_,3,1);
	  	$shortcode_ = substr($shortcode_,0,2);
	  	$shortcode_ = $shortcode_.$expiry_month_to_symbol_{$contract_month};
	  	$shortcode_ = $shortcode_.$contract_year;
	  }
	  
	  my $first_five= substr($mfg_exch_future_code_to_shortcode_{$exch_future_code_},0,5);
	  if($first_five eq "YFEBM" || $first_five eq "KFFTI" || $first_five eq "JFFCE"){
	  	my $contract_month=substr($contract_date_,4,2);
	  	my $contract_year=substr($contract_date_,2,2);
	  	$shortcode_ = substr($shortcode_,0,5);
	  	$shortcode_ = $shortcode_.$contract_year;
	  	$shortcode_ = $shortcode_.$contract_month;
	  	$shortcode_ = $shortcode_."00000F";
	  }
	  
	  if ($mfg_exch_future_code_to_exchange_{$exch_future_code_} eq "OSE"){
	  	my $contract_month=substr($contract_date_,4,2);
	  	my $contract_year=substr($contract_date_,2,2);
	  	$shortcode_ = $mfg_exch_future_code_to_shortcode_{$exch_future_code_}.$contract_year.$contract_month;
	  }
	  
	  my $shc = `/home/dvcinfra/infracore_install/bin/get_shortcode_for_symbol $shortcode_ $expected_date_`; $shc =~ s/^\s+|\s+$//g;
	  if($shc eq "")
	  {
	  	print "\n ##$shortcode_ \n";
	  	exit ( 1 );
	  }
	 
	 # print "$shortcode_ $shc\n";
	  $broker_symbol_to_exchange_symbol_{$broker_symbol} = $shortcode_;
  
  }
  $shortcode_ =  $broker_symbol_to_exchange_symbol_{$broker_symbol};
  my $number_to_dollar_ = $mfg_exch_future_code_to_n2d_{$exch_future_code_};
  $shortcode_to_n2d_{$shortcode_} = $number_to_dollar_ ;
  
  my $comm_amount_ = $fields_[$comm_amount_offset_]; $comm_amount_ =~ s/^\s+|\s+$//g;
  my $fee1_amount_ = $fields_[$fee1_amount_offset_]; $fee1_amount_ =~ s/^\s+|\s+$//g;
  my $fee2_amount_ = $fields_[$fee2_amount_offset_]; $fee2_amount_ =~ s/^\s+|\s+$//g;
  my $fee3_amount_ = $fields_[$fee3_amount_offset_]; $fee3_amount_ =~ s/^\s+|\s+$//g;
  my $fee4_amount_ = $fields_[$fee4_amount_offset_]; $fee4_amount_ =~ s/^\s+|\s+$//g;
  
  my $trade_time_ = $fields_[$time_offset_]; $trade_time_ =~ s/\"//g;
# Factor the commissions into the pnl
# += because the commission ammounts are already -ve
  if (! exists $shortcode_to_pnl_{$shortcode_}) {
    $shortcode_to_pnl_{$shortcode_} = 0.0;
    $shortcode_to_trade_time_{$shortcode_} = 0;
  }
  $shortcode_to_comm_{$shortcode_} += $comm_amount_;
  $shortcode_to_fee1_{$shortcode_} += $fee1_amount_;
  $shortcode_to_fee2_{$shortcode_} += $fee2_amount_;
  $shortcode_to_fee3_{$shortcode_} += $fee3_amount_;
  $shortcode_to_fee4_{$shortcode_} += $fee4_amount_;

  my $total_comm_ = ($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_ + $fee4_amount_);

  if ( index ( $shortcode_ , "NIY" ) >= 0 )
  {
     $shortcode_to_pnl_{$shortcode_} += $total_comm_* ( 1 / $JPY_TO_DOL_) ;
  }
  else
  {
    $shortcode_to_pnl_{$shortcode_} += $total_comm_ ;
  }


# Add the trade price into the pnl
  my $buysell_ = substr ($fields_[$buysell_offset_], 1, -1); $buysell_ =~ s/^\s+|\s+$//g;
  my $trade_price_ = $fields_[$trade_price_offset_]; $trade_price_ =~ s/^\s+|\s+$//g;
  my $closing_price_ = $fields_[$closing_price_offset_]; $closing_price_ =~ s/^\s+|\s+$//g;

  if( $shortcode_to_trade_time_{$shortcode_} < $trade_time_){
    $shortcode_to_last_price_{$shortcode_} =  $trade_price_ ;
    $shortcode_to_trade_time_{$shortcode_} = $trade_time_;
  }

  my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;


  print "$shortcode_".($buysell_-1)."$quantity_$trade_price_0\n";
  	
  if ($buysell_ == 1) { # Buy
    $shortcode_to_pnl_{$shortcode_} -= ($trade_price_ * $quantity_ * $number_to_dollar_);
    $shortcode_to_last_pos_{$shortcode_} += $quantity_ ;  #would we used to compute flat pnl if we hold on to some positions
  } elsif ($buysell_ == 2) { # Sell
    $shortcode_to_pnl_{$shortcode_} += ($trade_price_ * $quantity_ * $number_to_dollar_);
    $shortcode_to_last_pos_{$shortcode_} -= $quantity_ ;
  } else {
    print "Illegal buysell code : $buysell_\n";
    exit (1);
  }

  $shortcode_to_volume_{$shortcode_} += $quantity_;

  if (!exists ($shortcode_to_exchange_{$shortcode_})) {
    $shortcode_to_exchange_{$shortcode_} = $mfg_exch_future_code_to_exchange_{$exch_future_code_};
  }
if ($mfg_exch_future_code_to_exchange_{$exch_future_code_} ne "BMF" and $mfg_exch_future_code_to_exchange_{$exch_future_code_} ne "CFE"){
  my $comm_fee_ = -($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_ + $fee4_amount_ )/$quantity_;
  my $expected_comm_fee_ = GetCommissionForShortcode( $mfg_exch_future_code_to_shortcode_{$exch_future_code_} );
  if( abs($comm_fee_ - $expected_comm_fee_) > 0.1 )
  {
    if(!exists($fee_discrepancy_shortcodes_{$mfg_exch_future_code_to_shortcode_{$exch_future_code_}}))
    {
      $fee_discrepancy_shortcodes_{$mfg_exch_future_code_to_shortcode_{$exch_future_code_}} = $comm_fee_;
    }
  }
}
}

=pod
my $mail_body_ = "";
foreach my $shortcode_ ( keys %fee_discrepancy_shortcodes_ )
{
  my $expected_comm_fee_ = GetCommissionForShortcode( $shortcode_ );
  $mail_body_ = $mail_body_."Discrepancy for $shortcode_: in MFG file $fee_discrepancy_shortcodes_{$shortcode_}, expected: $expected_comm_fee_\n";
}

if( $mail_body_ )
{
  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: nseall@tworoads.co.in, sghosh\@circulumvite.com ravi.parikh\@tworoads.co.in\n";
  print MAIL "Subject: comm fee discrepancy - $expected_date_\n\n";
  print MAIL $mail_body_;
  close(MAIL);
}
=cut

my $shortcode_ = "";
foreach $shortcode_ (sort keys %shortcode_to_pnl_) {
  if ($shortcode_to_exchange_{$shortcode_} eq "EUREX") {
# Values are still in euros, convert to doller

    $shortcode_to_pnl_{$shortcode_} *= $EUR_TO_DOL_;

    if( $shortcode_to_last_pos_{$shortcode_} > 0) {  #open positions are closed with closing price , commision is not taken into account here

      $shortcode_to_pnl_{$shortcode_} += ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $EUR_TO_DOL_)  * $shortcode_to_last_pos_{$shortcode_} ) ;

    }elsif( $shortcode_to_last_pos_{$shortcode_} < 0 ){

      $shortcode_to_pnl_{$shortcode_} -= ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $EUR_TO_DOL_ ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ; 

    }

  }
  if ($shortcode_to_exchange_{$shortcode_} eq "LIFFE") {
    my $conversion_to_use_ = $EUR_TO_DOL_;

    if ( index ( $shortcode_ , "LFL" ) >= 0 ||
        index ( $shortcode_ , "LFR" ) >= 0 ||
        index ( $shortcode_ , "LFZ" ) >= 0 )
    {
# Values are still in gbp, convert to dollars
      $conversion_to_use_ = $GBP_TO_DOL_;
    }

    $shortcode_to_pnl_ { $shortcode_ } *= $conversion_to_use_;

    if( $shortcode_to_last_pos_{$shortcode_} > 0) {  #open positions are closed with closing price , commision is not taken into account here

      $shortcode_to_pnl_{$shortcode_} += ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $conversion_to_use_ )  * $shortcode_to_last_pos_{$shortcode_} ) ;

    }elsif( $shortcode_to_last_pos_{$shortcode_} < 0 ){

      $shortcode_to_pnl_{$shortcode_} -= ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $conversion_to_use_ ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ;

    }
  }

  if ($shortcode_to_exchange_{$shortcode_} eq "TMX") {
# Values are still in canadian $, convert to dollars

    $shortcode_to_pnl_{$shortcode_} *= $CD_TO_DOL_;

    if( $shortcode_to_last_pos_{$shortcode_} > 0) {  #open positions are closed with closing price

      $shortcode_to_pnl_{$shortcode_} += ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $CD_TO_DOL_)  * $shortcode_to_last_pos_{$shortcode_} ) ;

    }elsif( $shortcode_to_last_pos_{$shortcode_} < 0 ){

      $shortcode_to_pnl_{$shortcode_} -= ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $CD_TO_DOL_ ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ; 

    }

  }

  if ($shortcode_to_exchange_{$shortcode_} eq "OSE") {
# Values are still in canadian $, convert to dollars

    $shortcode_to_pnl_{$shortcode_} *= $JPY_TO_DOL_ ;

    if( $shortcode_to_last_pos_{$shortcode_} > 0) {  #open positions are closed with closing price

      $shortcode_to_pnl_{$shortcode_} += ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $JPY_TO_DOL_ )  * $shortcode_to_last_pos_{$shortcode_} ) ;

    }elsif( $shortcode_to_last_pos_{$shortcode_} < 0 ){

      $shortcode_to_pnl_{$shortcode_} -= ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $JPY_TO_DOL_ ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ; 

    }

  }

  if ($shortcode_to_exchange_{$shortcode_} eq "CME") {
# Values are in dollars

#$shortcode_to_pnl_{$shortcode_} *= 1 ;
    if ( index ( $shortcode_, "NIY" ) >= 0 ) 
    {
      $shortcode_to_pnl_{$shortcode_} *= $JPY_TO_DOL_ ;
    }

    if( $shortcode_to_last_pos_{$shortcode_} > 0) {  #open positions are closed with closing price

      $shortcode_to_pnl_{$shortcode_} += ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * 1 )  * $shortcode_to_last_pos_{$shortcode_} ) ;

    }elsif( $shortcode_to_last_pos_{$shortcode_} < 0 ){

      $shortcode_to_pnl_{$shortcode_} -= ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * 1 ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ; 

    }

  }

  if ($shortcode_to_exchange_{$shortcode_} eq "CFE") {
#Values are in dollars

#$shortcode_to_pnl_{$shortcode_} *= 1 ;

    if( $shortcode_to_last_pos_{$shortcode_} > 0) {  #open positions are closed with closing price

      $shortcode_to_pnl_{$shortcode_} += ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * 1 )  * $shortcode_to_last_pos_{$shortcode_} ) ;

    }elsif( $shortcode_to_last_pos_{$shortcode_} < 0 ){

      $shortcode_to_pnl_{$shortcode_} -= ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * 1 ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ;

    }

  }

 # print "$expected_date_,$shortcode_,$shortcode_to_pnl_{$shortcode_},$shortcode_to_volume_{$shortcode_},$shortcode_to_comm_{$shortcode_}/$shortcode_to_volume_{$shortcode_},$shortcode_to_fee1_{$shortcode_}/$shortcode_to_volume_{$shortcode_},$shortcode_to_fee2_{$shortcode_}/$shortcode_to_volume_{$shortcode_},$shortcode_to_fee3_{$shortcode_}/$shortcode_to_volume_{$shortcode_}\n";
}
