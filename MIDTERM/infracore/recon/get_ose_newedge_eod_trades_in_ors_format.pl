#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

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
my $ne_file1_ = "/apps/data/MFGlobalTrades/MFGFiles/GMITRN_".$expected_date_.".csv" ;

my $next_date_ = CalcNextBusinessDay ( $expected_date_ ) ; #

while(IsExchangeHoliday("OSE", $next_date_) == 1)
{
  $next_date_ = CalcNextBusinessDay ( $next_date_ ) ; 
}

my $ne_file2_ = "/apps/data/MFGlobalTrades/MFGFiles/GMITRN_".$next_date_.".csv" ;

my $prev_date_ = CalcPrevBusinessDay ( $expected_date_ ) ; #

my %broker_symbol_to_exchange_symbol_ = ();
my %fee_discrepancy_shortcodes_ = ();
my %shortcode_to_comm_ = ();
my %symbol_to_last_traded_time = ();
my %symbol_to_last_traded_price = ();
my %not_found_symbol=();
my %translation_failed_symbol=();


my %ne_exch_future_code_to_shortcode_ = GetFuturecodeToShortcodeMap();
my %ne_exch_future_code_to_exchange_ = GetFuturecodeToExchangeMap();

# These offsets were obtained using the 'FIMST4F1.xls & FIMST4F1.csv' template files
# These will be used to determine the field being processed after splitting each trade line

my $record_id_offset_ = 0; # PRECID
my $date_offset_ = 11; # PTDATE
my $buysell_offset_ = 12; # PBS # 1 = buy ; 2 = sell
my $quantity_offset_ = 17; # PQTY
my $ne_future_code_offset_ = 24; # PFC
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
my $JPY_TO_DOL_ = 0.0104 ;

#================================= fetching conversion rates from currency info file if available ====================#

my $curr_date_ = $prev_date_ ;

#my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;
my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$expected_date_.'.txt' ;

if ( -e $curr_filename_ )
{
    open CURR_FILE_HANDLE, "< $curr_filename_" ;
    my @curr_file_lines_ = <CURR_FILE_HANDLE>;
    close CURR_FILE_HANDLE;

    for ( my $itr = 0 ; $itr <= $#curr_file_lines_; $itr ++ )
    {

	my @cwords_ = split ( ' ', $curr_file_lines_[$itr] );
	if ( $#cwords_ >= 1 )
	{
	    my $currency_ = $cwords_[0] ;

	    if( index ( $currency_, "USDJPY" ) == 0 ){
		$JPY_TO_DOL_ = sprintf( "%.4f", 1/$cwords_[1] );
	    }

	}
    }
}

#====================================================================================================================
# pnl computation from the NE trades files are simpler, given the commission amounts




open NE_HANDLE1_, "< $ne_file1_" or die "could not open ne_file1_ $ne_file1_\n";
my @ne_trades1_file_lines_ = <NE_HANDLE1_>  ;

#print $ne_file1_ ,"\n";

my @ne_trades2_file_lines_ = ( ) ;
if ( $ne_file2_ ne "NA" )
{
    open NE_HANDLE2_, "< $ne_file2_" or die "could not open ne_file2_ $ne_file2_\n";
    @ne_trades2_file_lines_ = <NE_HANDLE2_> ;
    close NE_HANDLE2_;
}

#print $ne_file2_ ,"\n";
close NE_HANDLE1_;

############session 1
for (my $i = 0; $i <= $#ne_trades1_file_lines_; $i++) {
  my @fields_ = split (',', $ne_trades1_file_lines_[$i]);

  if ($#fields_ != 120) {
    next;
  }

  if (length ($fields_[$ne_future_code_offset_]) < 2) {
    next;
  }

  my $record_id_ = substr ($fields_[$record_id_offset_], 1, -1); $record_id_ =~ s/^\s+|\s+$//g;

  if ($record_id_ ne "T") {
    next;
  }
  
  my $trade_time_ = $fields_[$time_offset_]; $trade_time_ =~ s/\"//g;
  
  if ( int ( $trade_time_  ) >= 845 && int ( $trade_time_ ) < 1515 ){}
  else{next;}
  

  my $exch_code_ = substr ( $fields_ [ $exch_code_offset_ ] , 1 , -1 ); $exch_code_ =~ s/^\s+|\s+$//g;
  my $future_code_ = substr ($fields_[$ne_future_code_offset_], 1, -1); $future_code_ =~ s/^\s+|\s+$//g;

  my $exch_future_code_ = $exch_code_.$future_code_;

  if (!exists ( $ne_exch_future_code_to_shortcode_{$exch_future_code_})) {
    if(! exists $not_found_symbol{$exch_future_code_}){
      print STDERR "RECON : EXCHANGE_FUTURE_CODE_NOT_FOUND $exch_future_code_\n";
      $not_found_symbol{$exch_future_code_} = 1;
    }
    next;
  }
  
  if ( $ne_exch_future_code_to_exchange_{$exch_future_code_} ne "OSE"){
    next;
  }
  
  my $date_ = substr ($fields_[$date_offset_], 1, -1); $date_ =~ s/^\s+|\s+$//g;

  if ($date_ ne $expected_date_) {
 #   next;
  }

  my $contract_date_ = substr ($fields_[$contract_yr_month_offset_], 1, -1); $contract_date_ =~ s/^\s+|\s+$//g;
  my $broker_symbol = $ne_exch_future_code_to_shortcode_{$exch_future_code_}.$contract_date_;
 
  if(exists $not_found_symbol{$exch_future_code_}){
   next;
  }
  if (exists  $translation_failed_symbol{$exch_future_code_}{$contract_date_}) {
  	next;
  }
  if(! exists $broker_symbol_to_exchange_symbol_{$broker_symbol}){
    my $exchange_symbol = GetSymbolFromFuturecode($broker_symbol, $contract_date_, $ne_exch_future_code_to_exchange_{$exch_future_code_},$expected_date_);
    if($exchange_symbol eq "")
    {
      print STDERR "RECON : TRANSLATION FAILED $broker_symbol\n";
      $translation_failed_symbol{$exch_future_code_}{$contract_date_} = 1;
      next;
    }
    $broker_symbol_to_exchange_symbol_{$broker_symbol} = $exchange_symbol;
  }
  
  my $exchange_symbol =  $broker_symbol_to_exchange_symbol_{$broker_symbol};
  
  my $comm_amount_ = $fields_[$comm_amount_offset_]; $comm_amount_ =~ s/^\s+|\s+$//g;
  my $fee1_amount_ = $fields_[$fee1_amount_offset_]; $fee1_amount_ =~ s/^\s+|\s+$//g;
  my $fee2_amount_ = $fields_[$fee2_amount_offset_]; $fee2_amount_ =~ s/^\s+|\s+$//g;
  my $fee3_amount_ = $fields_[$fee3_amount_offset_]; $fee3_amount_ =~ s/^\s+|\s+$//g;
  my $fee4_amount_ = $fields_[$fee4_amount_offset_]; $fee4_amount_ =~ s/^\s+|\s+$//g;

  my $total_comm_ = ($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_ + $fee4_amount_);


# Add the trade price into the pnl
  my $buysell_ = substr ($fields_[$buysell_offset_], 1, -1); $buysell_ =~ s/^\s+|\s+$//g;
  my $trade_price_ = $fields_[$trade_price_offset_]; $trade_price_ =~ s/^\s+|\s+$//g;
  $trade_price_ = $trade_price_ * GetPriceMultiplier ($exchange_symbol, $ne_exch_future_code_to_exchange_{$exch_future_code_});
  
  my $closing_price_ = $fields_[$closing_price_offset_]; $closing_price_ =~ s/^\s+|\s+$//g;
  my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;

  if((! exists $symbol_to_last_traded_time{$exchange_symbol} ) || $symbol_to_last_traded_time{$exchange_symbol} < $trade_time_){
    $symbol_to_last_traded_time{$exchange_symbol} = $trade_time_;
    $symbol_to_last_traded_price{$exchange_symbol} = $trade_price_;
  }
  
  print "$exchange_symbol".($buysell_-1)."$quantity_$trade_price_0\n";
}

foreach my $key (keys %symbol_to_last_traded_price) { 
    $symbol_to_last_traded_time{$key} = 0;
}

############session 2
for (my $i = 0; $i <= $#ne_trades2_file_lines_; $i++) {
  my @fields_ = split (',', $ne_trades2_file_lines_[$i]);

  if ($#fields_ != 120) {
    next;
  }

  if (length ($fields_[$ne_future_code_offset_]) < 2) {
    next;
  }

  my $record_id_ = substr ($fields_[$record_id_offset_], 1, -1); $record_id_ =~ s/^\s+|\s+$//g;

  if ($record_id_ ne "T") {
    next;
  }
  
  my $trade_time_ = $fields_[$time_offset_]; $trade_time_ =~ s/\"//g;
  
  if ( int ( $trade_time_ ) < 845 || int ( $trade_time_  ) >= 1630){}
  else{ next ;}
  

  my $exch_code_ = substr ( $fields_ [ $exch_code_offset_ ] , 1 , -1 ); $exch_code_ =~ s/^\s+|\s+$//g;
  my $future_code_ = substr ($fields_[$ne_future_code_offset_], 1, -1); $future_code_ =~ s/^\s+|\s+$//g;

  my $exch_future_code_ = $exch_code_.$future_code_;

  if (!exists ( $ne_exch_future_code_to_shortcode_{$exch_future_code_})) {
    if(! exists $not_found_symbol{$exch_future_code_}){
      print STDERR "RECON : EXCHANGE_FUTURE_CODE_NOT_FOUND $exch_future_code_\n";
      $not_found_symbol{$exch_future_code_} = 1;
    }
    next;
  }
  
  if ( $ne_exch_future_code_to_exchange_{$exch_future_code_} ne "OSE"){
    next;
  }
  
  my $date_ = substr ($fields_[$date_offset_], 1, -1); $date_ =~ s/^\s+|\s+$//g;

  if ($date_ ne $expected_date_) {
    #next;
  }

  my $contract_date_ = substr ($fields_[$contract_yr_month_offset_], 1, -1); $contract_date_ =~ s/^\s+|\s+$//g;
  my $broker_symbol = $ne_exch_future_code_to_shortcode_{$exch_future_code_}.$contract_date_;
 
  if(exists $not_found_symbol{$exch_future_code_}){
   next;
  }
  if (exists  $translation_failed_symbol{$exch_future_code_}{$contract_date_}) {
  	next;
  }
   
  if(! exists $broker_symbol_to_exchange_symbol_{$broker_symbol}){
    my $exchange_symbol = GetSymbolFromFuturecode($broker_symbol, $contract_date_, $ne_exch_future_code_to_exchange_{$exch_future_code_}, $expected_date_);
    if($exchange_symbol eq "")
    {
      print STDERR "RECON : TRANSLATION FAILED $broker_symbol\n";
      $translation_failed_symbol{$exch_future_code_}{$contract_date_} = 1;
      next;
    }
    $broker_symbol_to_exchange_symbol_{$broker_symbol} = $exchange_symbol;
  }
  
  my $exchange_symbol =  $broker_symbol_to_exchange_symbol_{$broker_symbol};
  
  my $comm_amount_ = $fields_[$comm_amount_offset_]; $comm_amount_ =~ s/^\s+|\s+$//g;
  my $fee1_amount_ = $fields_[$fee1_amount_offset_]; $fee1_amount_ =~ s/^\s+|\s+$//g;
  my $fee2_amount_ = $fields_[$fee2_amount_offset_]; $fee2_amount_ =~ s/^\s+|\s+$//g;
  my $fee3_amount_ = $fields_[$fee3_amount_offset_]; $fee3_amount_ =~ s/^\s+|\s+$//g;
  my $fee4_amount_ = $fields_[$fee4_amount_offset_]; $fee4_amount_ =~ s/^\s+|\s+$//g;

  my $total_comm_ = ($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_ + $fee4_amount_);


# Add the trade price into the pnl
  my $buysell_ = substr ($fields_[$buysell_offset_], 1, -1); $buysell_ =~ s/^\s+|\s+$//g;
  my $trade_price_ = $fields_[$trade_price_offset_]; $trade_price_ =~ s/^\s+|\s+$//g;
  $trade_price_ = $trade_price_ * GetPriceMultiplier ($exchange_symbol, $ne_exch_future_code_to_exchange_{$exch_future_code_});
  my $closing_price_ = $fields_[$closing_price_offset_]; $closing_price_ =~ s/^\s+|\s+$//g;
  my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;

  if((! exists $symbol_to_last_traded_time{$exchange_symbol} ) || $symbol_to_last_traded_time{$exchange_symbol} < $trade_time_){
    $symbol_to_last_traded_time{$exchange_symbol} = $trade_time_;
    $symbol_to_last_traded_price{$exchange_symbol} = $trade_price_;
  }
  
  print "$exchange_symbol".($buysell_-1)."$quantity_$trade_price_0\n";
}

#to get last trading price (will change pnls in case of open positions)
my $newedge_pnl=$HOME_DIR."/recon/newedge_ors_pnl";
foreach my $symbol (sort keys %symbol_to_last_traded_price)
{
  if(-e $newedge_pnl) {
  	my $last_price=`grep $symbol $newedge_pnl | awk -F'|' '{print \$7}' | awk -F':' '{print \$2}' | tr -d ' '`; chomp ($last_price);
    print "$symbol00$last_price0\n";
  }
}