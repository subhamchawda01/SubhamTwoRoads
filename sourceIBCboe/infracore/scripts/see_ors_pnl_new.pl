#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;
use List::Util qw/max min/; # for max
use Data::Dumper;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore_install";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $INSTALL_BIN="$HOME_DIR/$REPO/bin";
my $LIVE_BIN="$HOME_DIR/LiveExec/bin/";

require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/get_hft_commission_discount_for_shortcode.pl"; # Get hft discount according to volume tiers for shortcode
require "$GENPERLLIB_DIR/get_cme_commission_discount_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_cme_eu_volumes_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_number_of_working_days_BMF_between_two_dates.pl" ;
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; # 
require "$GENPERLLIB_DIR/get_rts_ticksize.pl"; #RITickSize
require "$GENPERLLIB_DIR/get_shortcode_from_symbol.pl"; #GetShortcodeFromSymbol
use Term::ANSIColor;

#my $exchange_to_unrealpnl_map_;
#C - colour, R - No colour, E - without commission

sub Print{
	my $mode=$_[0];
	my $value=$_[1];
	
	printf "| PNL : ";
	
	if($mode eq 'C' || $mode eq 'E'){
		print color("BOLD");
		print color("reset");
		if ($value < 0) {
		    print color("red"); print color("BOLD");
		  } else {
		    print color("blue"); print color("BOLD");
		  }
		printf "%10.3f ", $value;
		print color("reset");
	}
	
	else{
		printf "%10.3f ", $value;
	}
}

#sub Print_shortcode_pnl(){
#	my $mode=$_[0];
#	my $short_code=$_[1];

#	my $exchange_to_unrealpnl_map=$_[2];

#	if( exists $exchange_to_unrealpnl_map{$short_code} ){
#		printf "\n\n%17 |", $short_code;
#		Print($mode, $exchange_to_unrealpnl_map{$short_code});
#	}
#}


my $USAGE="$0 mode ors_trades_filename_ [date] [dump_overnight_pos] [exch] [dont_load_over_night]";
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $mode_=$ARGV[0];
my $ors_trades_filename_ = $ARGV[1];

my $DUMP_OVERNIGHT_POS = '';
my $EXCH_ = '' ;
my $LOAD_ = '' ;
my $MAX_PAGE_SIZE_ = 40 ;
my $today_date_ = `date +"%Y%m%d"` ; chomp ( $today_date_ ) ;
my $input_date_ = $today_date_;
 
if ( $#ARGV >= 2 ){
	$input_date_ = int( $ARGV[2] );
}
if ( $#ARGV >= 3 ){
  	$DUMP_OVERNIGHT_POS = $ARGV[3];                                                     
}
if ( $#ARGV >= 4 ){
  	$EXCH_ = $ARGV[4];
}
if ( $#ARGV >= 5 ){
	if ( $ARGV[5] != 0 )
	{
  		$LOAD_ = $ARGV[5];
	}                                                     
}
if ( $#ARGV >= 6 ){
  	$MAX_PAGE_SIZE_ = $ARGV[6];                                                     
}

my $EUR_TO_DOL = 1.30; # Should have a way of looking this up from the currency info file.
my $CD_TO_DOL =  0.97; # canadian dollar
my $BR_TO_DOL = 0.51;
my $GBP_TO_DOL = 1.57;
my $HKD_TO_DOL = 0.1289 ;
my $JPY_TO_DOL = 0.0104 ;
my $RUB_TO_DOL = 0.031 ;
my $AUD_TO_DOL = 0.819 ;
my $INR_TO_DOL = 0.152;

my %micex_bcs_fee_tiers_ = ();
$micex_bcs_fee_tiers_{"5000"} = 0.003; #0.006;
$micex_bcs_fee_tiers_{"10000"} = 0.003; # 0.005;
$micex_bcs_fee_tiers_{"15000"} = 0.003; #0.004;
$micex_bcs_fee_tiers_{"10000000000"} = 0.003;
my $micex_exchange_fee_ = 0.008 ;
my $bmfeq_exchange_fee_ = 0.00025 ;
my $bmfeq_bp_fee_ = 0.00003 ;
my $last_traded_tom_while_tod = 0;

my %spread_to_pos_symbol_ =();
my %spread_to_neg_symbol_ =();
my %symbol_to_expiry_year_ = ("0" => "2010", "1" => "2011", "2" => "2012", "3" => "2013", "4" => "2014", "5" => "2015", "6" => "2016", "7" => "2017", "8" => "2018", "9" => "2019");
my %symbol_to_expiry_month_ = ("F" => "01", "G" => "02", "H" => "03", "J" => "04", "K" => "05", "M" => "06", "N" => "07", "Q" => "08", "U" => "09", "V" => "10", "X" => "11", "Z" => "12");
my %expiry_month_to_symbol_ = ("01" => "F", "02" => "G", "03" => "H", "04" => "J", "05" => "K", "06" => "M", "07" => "N", "08" => "Q", "09" => "U", "10" => "V", "11" => "X", "12" => "Z");

my %symbol_to_noof_working_days_till_expiry_ = ();

#================================ file for storing remaining days in expiry for DI products =========================#
my ($sec,$min,$hour,$day,$month,$year,$wday,$yday,$isdst) = localtime();
my $today = sprintf "%.4d%.2d%.2d", $year+1900, $month+1, $day;
my $remaining_days_filename = "$HOME_DIR/.DI_remaining_days.$today";
if (-f $remaining_days_filename && -s $remaining_days_filename){
#file already exists and it is non-empty
  open(TODAY_FILE_HANDLE, "+<$remaining_days_filename") || die "Couldn't open existing file: $remaining_days_filename, $!";
  while(<TODAY_FILE_HANDLE>){
    my @values = split(" ",$_);
    $symbol_to_noof_working_days_till_expiry_{$values[0]} = $values[1];  #loading map
  }
  close TODAY_FILE_HANDLE ;
}
else {
  open(TODAY_FILE_HANDLE,">>$remaining_days_filename") || die "Couldn't open new file: $remaining_days_filename, $!";
  close TODAY_FILE_HANDLE ;
}

my $yesterday_date_ = `/home/dvcinfra/LiveExec/bin/calc_prev_week_day $input_date_`;
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
    }

    if( index ( $currency_, "USDAUD" ) == 0 ){
      $AUD_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
    }
    
    if(index ( $currency_, "USDINR" ) == 0){
    	$INR_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
    }

  }
}

#for native currency 
if($mode_ eq 'E'){
	$EUR_TO_DOL = 1;
	$CD_TO_DOL = 1;
	$BR_TO_DOL = 1;
	$GBP_TO_DOL = 1;
	$HKD_TO_DOL = 1;
	$JPY_TO_DOL = 1;
	$RUB_TO_DOL = 1;
	$AUD_TO_DOL = 1;
	$INR_TO_DOL = 1;
}
#======================================================================================================================#

my $targetcol = 9;

my $mkt_volume_file = "$HOME_DIR/trades/ALL_MKT_VOLUMES" ;

my $ors_trades_file_base_ = basename ($ors_trades_filename_); chomp ($ors_trades_file_base_);

open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "see_ors_pnl_new.pl could not open ors_trades_filename_ $ors_trades_filename_\n";

my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;

close ORS_TRADES_FILE_HANDLE;

my %symbol_to_commish_map_ = ();
my %symbol_to_n2d_map_ = ();

my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();
my %symbol_to_display_name_ = ();

my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();

my %symbol_to_mkt_vol_ =();

my $wdo_secname = `$LIVE_BIN/get_exchange_symbol BR_WDO_0 $input_date_`;
my $dol_secname = `$LIVE_BIN/get_exchange_symbol BR_DOL_0 $input_date_`;
my $win_secname = `$LIVE_BIN/get_exchange_symbol BR_WIN_0 $input_date_`;
my $ind_secname = `$LIVE_BIN/get_exchange_symbol BR_IND_0 $input_date_`;

#======================================================================================================================================#

if ($LOAD_ eq '')
{
  my $overnight_pos_file = "/spare/local/files/EODPositions/overnight_pnls_$prev_date_.txt";
  if (-e $overnight_pos_file) {
    open OVN_FILE_HANDLE, "< $overnight_pos_file" or die "see_ors_pnl_new could not open ors_trades_filename_ $overnight_pos_file\n";

    my @ovn_file_lines_ = <OVN_FILE_HANDLE>;
    close OVN_FILE_HANDLE;
    for ( my $i = 0 ; $i <= $#ovn_file_lines_; $i ++ )
    {
      my @words_ = split ( ',', $ovn_file_lines_[$i] );
      if ( $#words_ >= 2 )
      {

        my $symbol_ = $words_[0];
        SetSecDef ( $symbol_ ) ;
        if ( ($symbol_to_exchange_map_{$symbol_} eq $EXCH_) or ($EXCH_ eq '') )
        {
          ;
        }
        else
        {
          next;
        }
        chomp($words_[1]);
        chomp($words_[2]);
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
#=============================================================================================================================#

#print "ors_size  $#ors_trades_file_lines_ \n";

for ( my $i = 0 ; $i <= $#ors_trades_file_lines_; $i ++ )
{
  #print " ors_trades_file_lines_:  $ors_trades_file_lines_[$i]\n";
  my @words_ = split ( '', $ors_trades_file_lines_[$i] );
  #print "#words_ $#words_ \n";
  if ( $#words_ >= 4 )
  {
    my $symbol_ = $words_[0];
    my $buysell_ = $words_[1];
    my $tsize_ = $words_[2];
    my $tprice_ = $words_[3];
    my $saos_ = $words_[4];
	
	#print " $words_[0] $words_[1] $words_[2] $words_[3] $words_[4]\n";

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
      $symbol_to_commish_map_{$symbol_} = ( $bmfeq_exchange_fee_ + $bmfeq_bp_fee_ ) * $tprice_ * $tsize_ * $BR_TO_DOL ;
      $symbol_to_commish_map_{$symbol_} /= $tsize_ ;
    }

	if($mode_ eq 'E'){
		$symbol_to_commish_map_{$symbol_} = 0;
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
	$symbol_to_price_map_{$symbol_} = $tprice_;
	
	if(( index ( $symbol_ , "NSE" ) == 0 )){
		$symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $tprice_ * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL);
	}
	else{
    	$symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
  	}
  	
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

if($DUMP_OVERNIGHT_POS eq "1")
{
  foreach my $symbol_ (sort keys %symbol_to_unrealpnl_map_)
  {
    if (exists ($spread_to_pos_symbol_{$symbol_})){
      $symbol_to_pos_map_{$spread_to_pos_symbol_{$symbol_}}+=$symbol_to_pos_map_{$symbol_};
      $symbol_to_pos_map_{$spread_to_neg_symbol_{$symbol_}}-=$symbol_to_pos_map_{$symbol_};
    }
    
    if ($symbol_ eq "USD000TODTOM"){
    	if(!exists($symbol_to_pos_map_{"USD000000TOD"})){
      		$symbol_to_pos_map_{"USD000000TOD"}=0;
    	}
      	$symbol_to_pos_map_{"USD000000TOD"}-=100 * $symbol_to_pos_map_{$symbol_};
      
      	if(!exists($symbol_to_pos_map_{"USD000UTSTOM"})){
      		$symbol_to_pos_map_{"USD000UTSTOM"}=0;
    	}
      	$symbol_to_pos_map_{"USD000UTSTOM"}+=100 * $symbol_to_pos_map_{$symbol_};
      	
      	$symbol_to_pos_map_{$symbol_}=0;
    }

    if (index($symbol_, "NK") == 0 )
    {
      foreach my $symbol_1_ ( sort keys %symbol_to_unrealpnl_map_)
      {
        if (index ($symbol_1_,"NKM") == 0)
        {
          if (substr($symbol_,2,4) eq substr($symbol_1_,3,4))
          {
            $symbol_to_pos_map_{$symbol_1_} += 10*$symbol_to_pos_map_{$symbol_};
            $symbol_to_pos_map_{$symbol_} = 0;
          }
        }
      }

    }
	 if (index($symbol_, "DOL") == 0 )
    {
      foreach my $symbol_1_ ( sort keys %symbol_to_unrealpnl_map_)
      {
        if (index ($symbol_1_,"WDO") == 0)
        {
          if (substr($symbol_,3,3) eq substr($symbol_1_,3,3))
          {
            $symbol_to_pos_map_{$symbol_1_} += 5*$symbol_to_pos_map_{$symbol_};
            $symbol_to_pos_map_{$symbol_} = 0;
          }
        }
      }
    }

    if (index($symbol_, "IND") == 0 )
    {
      foreach my $symbol_1_ ( sort keys %symbol_to_unrealpnl_map_)
      {
        if (index ($symbol_1_,"WIN") == 0)
        {
          if (substr($symbol_,3,3) eq substr($symbol_1_,3,3))
          {
                if ( $symbol_to_pos_map_{$symbol_1_} == -5*$symbol_to_pos_map_{$symbol_}  )
                {
                        $symbol_to_pos_map_{$symbol_} = 0;
                        $symbol_to_pos_map_{$symbol_1_} = 0;
                }
          }
        }
      }
    }

  }

  foreach my $symbol_ (sort keys %symbol_to_unrealpnl_map_)
  {
    if (exists ($spread_to_pos_symbol_{$symbol_})){
      next;
    }
    else{
      if($symbol_to_pos_map_{$symbol_} != 0){
        print "$symbol_,$symbol_to_pos_map_{$symbol_},$symbol_to_price_map_{$symbol_}\n";
      }
    }
  }

  exit(0);
}
		
my $date_ = `date`;
printf ("\n$date_\n");

my $total_index_fut_win_ind_volume_ = 0.0 ;

foreach my $symbol_ ( sort keys %symbol_to_price_map_ )
{

  if( index ( $symbol_to_exchange_map_{$symbol_}, "BMF" ) == 0 ){

    if ( index ( $symbol_, "IND" ) == 0 ) { $total_index_fut_win_ind_volume_ += $symbol_to_volume_map_{$symbol_} * 5 ; }
    if ( index ( $symbol_, "WIN" ) == 0 ) { $total_index_fut_win_ind_volume_ += $symbol_to_volume_map_{$symbol_} ; }

  }

}

my $totalpnl_ = 0.0;

my $line_number_ = 0 ;
my $page_size_ = keys %symbol_to_price_map_ ;

#print "#### %symbol_to_price_map_\n";
#print Dumper(\%symbol_to_price_map_);


foreach my $symbol_ ( sort keys %symbol_to_price_map_ )
{
 # print "#### $symbol_\n";
  my $unreal_pnl_ = 0 ;


  if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {
    $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} - $symbol_to_pos_map_{$symbol_} * ( ( 100000 / ( $symbol_to_price_map_{$symbol_} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ) - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};
  }elsif(( index ( $symbol_ , "NSE" ) == 0 )){
  	$unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_})* $symbol_to_price_map_{$symbol_} * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL;
  }elsif(( index ( $symbol_ , "USD000UTSTOM" ) == 0 )){
  	if(exists ($symbol_to_pos_map_{"USD000TODTOM"}) && exists ($symbol_to_pos_map_{"USD000000TOD"}) && (($hour < 7 || ($hour > 14 && $min >15) || $input_date_ != $today_date_))){
  		my $actual_pos = $symbol_to_pos_map_{$symbol_} + 100 *$symbol_to_pos_map_{"USD000TODTOM"};
  		$unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $actual_pos * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($actual_pos) * $symbol_to_commish_map_{$symbol_};
  		$unreal_pnl_ += -100 * $symbol_to_pos_map_{"USD000TODTOM"} * ($last_traded_tom_while_tod) * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{"USD000TODTOM"} * 100) * $symbol_to_commish_map_{$symbol_};
  	}else{
  		$unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};
  	}
  }
  else{
    $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};
  }
  if(( index ( $symbol_ , "USD000000TOD" ) == 0 )){
    $last_traded_tom_while_tod = $symbol_to_price_map_{"USD000UTSTOM"};
  }


# BMF has different tiers of discount for HFT commission
  if( index ( $symbol_to_exchange_map_{$symbol_}, "BMF" ) == 0 ){

    my $shortcode_ = "";

    if ( index ( $symbol_, "DOL" ) == 0 ) {

      $shortcode_ = "DOL" ;
	  $unreal_pnl_ += ( $symbol_to_volume_map_{$symbol_} * GetHFTDiscount( $shortcode_, $symbol_to_volume_map_{$symbol_} ) * $BR_TO_DOL ) ;

    }

    if ( index ( $symbol_, "WDO" ) == 0 ) {

      $shortcode_ = "WDO" ;
      $unreal_pnl_ += ( $symbol_to_volume_map_{$symbol_} * GetHFTDiscount( $shortcode_, $symbol_to_volume_map_{$symbol_} ) * $BR_TO_DOL ) ;

    }

    if ( index ( $symbol_, "WIN" ) == 0 ) {

      $shortcode_ = "WIN" ;
      $unreal_pnl_ += ( $symbol_to_volume_map_{$symbol_} * GetHFTDiscount( $shortcode_, $total_index_fut_win_ind_volume_ ) * $BR_TO_DOL ) ;

    }

    if ( index ( $symbol_, "IND" ) == 0 ) {

      $shortcode_ = "IND" ;
      $unreal_pnl_ += ( $symbol_to_volume_map_{$symbol_} * GetHFTDiscount( $shortcode_, $total_index_fut_win_ind_volume_ ) * $BR_TO_DOL ) ;

    }

  }elsif( index ( $symbol_to_exchange_map_{$symbol_}, "CME" ) == 0 ){

    my $shortcode_ ="" ;

    if ( index ( $symbol_, "ZN" ) == 0 ) { $shortcode_ = "ZN" ; }
    if ( index ( $symbol_, "ZF" ) == 0 ) { $shortcode_ = "ZF" ; }
    if ( index ( $symbol_, "ZB" ) == 0 ) { $shortcode_ = "ZB" ; }
    if ( index ( $symbol_, "UB" ) == 0 ) { $shortcode_ = "UB" ; }

    my $trade_hours_ = `date +%H`; chomp ( $trade_hours_ );

    my $trade_hours_string_ = "EU" ;

    if( $trade_hours_ > 11 ) {
#compute discount in commission only for US hours volumes
      $trade_hours_string_ = "US" ;
      $unreal_pnl_ += ( ( $symbol_to_volume_map_{$symbol_} - GetCMEEUVolumes( $shortcode_ ) ) * GetCMEDiscount( $shortcode_, $trade_hours_string_ ) ) ;
    }

  }

  if ( ( ! $EXCH_ )  && ( index ( $symbol_to_exchange_map_{$symbol_}, "BMFEQ" ) == 0 ) )
  {
    $totalpnl_ += $unreal_pnl_;
    $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
    $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};
    next;
  }
  
  $line_number_ += 1 ;
  GetMktVol($symbol_);
  
  if($mode_  eq  'C' || $mode_ eq 'E'){
  	  print color("BOLD");
	  if (index($symbol_,"LFL") == 0 )
	  {
	    printf "| %12.12s ", substr($symbol_to_display_name_{$symbol_},0,8).substr($symbol_to_display_name_{$symbol_},11,4);
	  }
	  else
	  {
	    printf "| %12.12s ", $symbol_to_display_name_{$symbol_};
	  }
	  print color("reset");	
  }
  
  elsif($mode_  eq  'R'){
  	 printf "| %16s ", $symbol_to_display_name_{$symbol_};
  }
     
   Print($mode_, $unreal_pnl_);
   my $last_closing_price = sprintf("%.6f", $symbol_to_price_map_{$symbol_});
	
	if($mode_ eq 'R'){
		printf "| POS : %4d | VOL : %4d | v/V: %.1f | LPX : %s |\n", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_},$symbol_to_volume_map_{$symbol_}/$symbol_to_mkt_vol_{$symbol_}, $last_closing_price;
	}
	elsif ( $mode_ eq 'E' || (0 == $line_number_ % 2 || $page_size_ < $MAX_PAGE_SIZE_) ) {
		printf "| POS: %6d | VOL: %7d | v/V: %5.1f | LPX: %13s |\n", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}/$symbol_to_mkt_vol_{$symbol_} , $last_closing_price;
	}
	else{
		printf "| POS: %6d | VOL: %7d | v/V: %5.1f | LPX: %13s |\t", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}/$symbol_to_mkt_vol_{$symbol_}, $last_closing_price;
	}
  
    $totalpnl_ += $unreal_pnl_;

  $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
  $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};
}
printf "\n\n----------------------------------------------------------------------------------------\n\n";

$totalpnl_ = 0.0;
my $totalvol_ = 0;

#width of R mode can be same as C and E, but just seperated it just incase if any scripts are running assuming width 4.
if ( exists $exchange_to_unrealpnl_map_{"EUREX"} )
{
  printf "%17s |", "EUREX";
  
  Print($mode_, $exchange_to_unrealpnl_map_{"EUREX"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"EUREX"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"EUREX"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"EUREX"};
  $totalvol_ += $exchange_to_volume_map_{"EUREX"};
}

if ( exists $exchange_to_unrealpnl_map_{"LIFFE"} )
{
  printf "%17s |", "LIFFE";
  
  Print($mode_, $exchange_to_unrealpnl_map_{"LIFFE"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"LIFFE"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"LIFFE"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"LIFFE"};
  $totalvol_ += $exchange_to_volume_map_{"LIFFE"};
}

if ( exists $exchange_to_unrealpnl_map_{"ICE"} )
{
  printf "%17s |", "ICE";
  
  Print($mode_, $exchange_to_unrealpnl_map_{"ICE"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"ICE"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"ICE"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"ICE"};
  $totalvol_ += $exchange_to_volume_map_{"ICE"};
}

if ( exists $exchange_to_unrealpnl_map_{"TMX"} )
{
  printf "%17s |", "TMX";
  
  Print($mode_, $exchange_to_unrealpnl_map_{"TMX"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"TMX"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"TMX"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"TMX"};
  $totalvol_ += $exchange_to_volume_map_{"TMX"};
}

if ( exists $exchange_to_unrealpnl_map_{"CME"} )
{
  printf "%17s |", "CME";
  
  Print($mode_, $exchange_to_unrealpnl_map_{"CME"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"CME"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"CME"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"CME"};
  $totalvol_ += $exchange_to_volume_map_{"CME"};
}

if ( exists $exchange_to_unrealpnl_map_{"BMF"} )
{
  printf "%17s |", "BMF";
  
  Print($mode_, $exchange_to_unrealpnl_map_{"BMF"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"BMF"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"BMF"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"BMF"};
  $totalvol_ += $exchange_to_volume_map_{"BMF"};
}

if ( exists $exchange_to_unrealpnl_map_{"HKEX"} )
{
  printf "%17s |", "HKEX";
  
  Print($mode_, $exchange_to_unrealpnl_map_{"HKEX"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"HKEX"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"HKEX"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"HKEX"};
  $totalvol_ += $exchange_to_volume_map_{"HKEX"};
}

if ( exists $exchange_to_unrealpnl_map_{"OSE"} )
{
  printf "%17s |", "OSE";
  
  Print($mode_, $exchange_to_unrealpnl_map_{"OSE"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"OSE"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"OSE"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"OSE"};
  $totalvol_ += $exchange_to_volume_map_{"OSE"};
}

if ( exists $exchange_to_unrealpnl_map_{"RTS"} )
{
  printf "%17s |", "RTS";
  
  Print($mode_, $exchange_to_unrealpnl_map_{"RTS"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"RTS"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"RTS"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"RTS"};
  $totalvol_ += $exchange_to_volume_map_{"RTS"};
}

if ( exists $exchange_to_unrealpnl_map_{"MICEX"} )
{
  printf "%17s |", "MICEX";
  
  Print($mode_, $exchange_to_unrealpnl_map_{"MICEX"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"MICEX"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"MICEX"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"MICEX"};
  $totalvol_ += $exchange_to_volume_map_{"MICEX"};
}

if ( exists $exchange_to_unrealpnl_map_{"CFE"} )
{
  printf "%17s |", "CFE";

  Print($mode_, $exchange_to_unrealpnl_map_{"CFE"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"CFE"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"CFE"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"CFE"};
  $totalvol_ += $exchange_to_volume_map_{"CFE"};
}

if ( exists $exchange_to_unrealpnl_map_{"NSE"} )
{
  printf "%17s |", "NSE";

  Print($mode_, $exchange_to_unrealpnl_map_{"NSE"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"NSE"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"NSE"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"NSE"};
  $totalvol_ += $exchange_to_volume_map_{"NSE"};
}

if ( exists $exchange_to_unrealpnl_map_{"ASX"} )
{
  printf "%17s |", "ASX";

  Print($mode_, $exchange_to_unrealpnl_map_{"ASX"});
  
  if ( $mode_  eq 'C' || $mode_  eq 'E' ){
  	printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"ASX"};
  }
  
  if ( $mode_ eq 'R' ){
  	printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"ASX"};
  }
  
  $totalpnl_ += $exchange_to_unrealpnl_map_{"ASX"};
  $totalvol_ += $exchange_to_volume_map_{"ASX"};
}

printf "\n%17s |", "TOTAL";

Print($mode_, $totalpnl_);

printf "| VOLUME : %8d |\n", $totalvol_;

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
  $symbol_to_display_name_{$symbol_} = $symbol_;
  
  if ( index ( $symbol_, "DI" ) == 0 )
  {
    my $commish_ = `$INSTALL_BIN/get_contract_specs "$symbol_" $input_date_ COMMISH | awk '{print \$2}'`; chomp ( $commish_ ) ;
    $symbol_to_commish_map_ {$symbol_} = $commish_;#  GetCommissionForShortcode ( $symbol_ ) ;

    if ( exists $symbol_to_noof_working_days_till_expiry_{$symbol_} ) {
#Already calculated remaining days. Do nothing!
    }
    elsif ( index ( $symbol_ , "DI1F13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20130102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20140102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F15" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20150102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F16" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20160102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F17" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20170102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F18" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20180102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F19" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20190102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F20" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20200102 ) ;
    }
     elsif ( index ( $symbol_ , "DI1F21" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20210102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F22" ) >= 0 )
    {
        $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20220102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F23" ) >= 0 )
    {
        $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20230102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F24" ) >= 0 )
    {
        $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20240102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1F25" ) >= 0 )
    {
        $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20250102 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20130701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20140701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N15" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20150701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N16" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20160701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N17" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20170701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1N18" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20180701 ) ;
    }
    elsif ( index ( $symbol_ , "DI1G13" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20130201 ) ;
    }
    elsif ( index ( $symbol_ , "DI1G14" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20140201 ) ;
    }
    elsif ( index ( $symbol_ , "DI1J15" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20150401 ) ;
    }
    elsif ( index ( $symbol_ , "DI1J16" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20160401 ) ;
    }
    elsif ( index ( $symbol_ , "DI1J17" ) >= 0 )
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20170401 ) ;
    }
    else
    {
      $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20170131 ) ;
    }
	#Save the new result to file (so that we don't have to calculate this again today)
    open(TODAY_FILE_HANDLE,">>$remaining_days_filename") || die "Couldn't open existing file: $remaining_days_filename, $!";
    print TODAY_FILE_HANDLE "$symbol_ $symbol_to_noof_working_days_till_expiry_{$symbol_}\n";
    close TODAY_FILE_HANDLE;

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
    my $shc_ = `$INSTALL_BIN/get_shortcode_for_symbol  "$symbol_" $input_date_ ` ; chomp ( $shc_ ) ;
    
    if(index ( $symbol_, "NSE" ) == 0){
    	$symbol_to_display_name_{$symbol_} = substr($shc_,4);
    }
    
    my $exchange_name_ = `$INSTALL_BIN/get_contract_specs "$shc_"  $input_date_ EXCHANGE | awk '{print \$2}' `; chomp ( $exchange_name_ ) ;
    
    if ( $exchange_name_ eq "HONGKONG" ) { $exchange_name_ = "HKEX" ; }
    if ( index ( $exchange_name_, "MICEX" ) == 0 )  { $exchange_name_ = "MICEX" ; }
    $symbol_to_exchange_map_{$symbol_}=$exchange_name_;
    my $commish_ = `$INSTALL_BIN/get_contract_specs "$shc_" $input_date_ COMMISH | awk '{print \$2}' `; chomp ( $commish_ ) ;
    $symbol_to_commish_map_ {$symbol_} = $commish_;#  GetCommissionForShortcode ( $symbol_ ) ;
    
    my $n2d_ = `$INSTALL_BIN/get_contract_specs $shc_ $input_date_ N2D | awk '{print \$2}' `; chomp ( $n2d_ ) ;
    $symbol_to_n2d_map_{$symbol_} = $n2d_;
  }
}
 
    
    


