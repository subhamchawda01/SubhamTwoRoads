#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $INSTALL_BIN="$HOME_DIR/infracore_install/bin/";
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/get_hft_commission_discount_for_shortcode.pl"; # Get hft discount according to volume tiers for shortcode
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; # 
require "$GENPERLLIB_DIR/get_rts_ticksize.pl"; #RITickSize

my $USAGE="$0 ors_trades_filename_";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];

my $RUB_TO_DOL = 0.031 ;
my $today_date_ = `date +"%Y%m%d"` ; chomp ($today_date_);

#================================= fetching conversion rates from currency info file if available ====================#

my $yesterday_file_ = "/tmp/YESTERDAY_DATE" ;
open YESTERDAY_FILE_HANDLE, "< $yesterday_file_" ;

my @yesterday_lines_ = <YESTERDAY_FILE_HANDLE> ;
close YESTERDAY_FILE_HANDLE ;

my @ywords_ = split(' ', $yesterday_lines_[0] );
my $yesterday_date_ = $ywords_[0];

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

    if( index ( $currency_, "USDRUB" ) == 0 ){
      $RUB_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
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
my %symbol_to_noof_working_days_till_expiry_ = ();
my %symbol_to_n2d_map_ = ();

my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();

my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();


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
    if ( $buysell_ == 0 )
    { # buy


      $symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};

      $symbol_to_pos_map_{$symbol_} += $tsize_;
    }
    else
    {


      $symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};


      $symbol_to_pos_map_{$symbol_} -= $tsize_;
    }

    $symbol_to_volume_map_{$symbol_} += $tsize_;
    $symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
    $symbol_to_price_map_{$symbol_} = $tprice_;
    if (  ( exists $symbol_to_unrealpnl_map_{$symbol_} ) ){
#printf ("\n$symbol_ $buysell_ $tsize_ $tprice_  $symbol_to_volume_map_\{$symbol_\} $symbol_to_unrealpnl_map_\{$symbol_\}\n");
#      printf " PNL : %10.3f \n", $symbol_to_unrealpnl_map_{$symbol_};
    }
  }
}

use Term::ANSIColor; 

my $date_ = `date`;
printf ("\n$date_\n");

my $totalpnl_ = 0.0;
foreach my $symbol_ ( sort keys %symbol_to_price_map_ )
{
# As if we were to go flat now with the last seen (executed price ) 
  my $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};

  my $shortcode_ = "";

  print color("BOLD");
  printf "| %10s ", $symbol_;
  print color("reset");
  printf "| PNL : ";
  if ($unreal_pnl_ < 0) {
    print color("red"); print color("BOLD");
  } else {
    print color("blue"); print color("BOLD");
  }
  printf "%10.3f ", $unreal_pnl_;
  print color("reset");
  printf "| POSITION : %4d | VOLUME : %4d |\n", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_};
  $totalpnl_ += $unreal_pnl_;

  $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
  $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};
}
printf "-------------------------------------------------------------------\n";

if ( exists $exchange_to_unrealpnl_map_{"RTS"} )
{
  printf "%17s |", "RTS";
  print color("BOLD");
  print color("reset");
  printf "| PNL : ";
  if ($exchange_to_unrealpnl_map_{"RTS"} < 0) {
    print color("red"); print color("BOLD");
  } else {
    print color("blue"); print color("BOLD");
  }
  printf "%10.3f ", $exchange_to_unrealpnl_map_{"RTS"};
  print color("reset");
  printf "| VOLUME : %5d |\n", $exchange_to_volume_map_{"RTS"};

}


#printf "\n%17s |", "TOTAL";
#print color("BOLD");
#print color("reset");
#printf "| PNL : ";
#if ($totalpnl_ < 0) {
#  print color("red"); print color("BOLD");
#} else {
#  print color("blue"); print color("BOLD");
#}
#printf "%10.3f ", $totalpnl_;
#print color("reset");
#printf "| VOLUME : %5d |\n", $totalvol_;
#
exit ( 0 );

sub SetSecDef 
{
  my $symbol_ = shift;
  my $shc_ = "";

  if  ( index ( $symbol_, "Si" ) == 0 )
  {
	$shc_ = "Si_0" ;
  }
  elsif  ( index ( $symbol_, "RTS" ) == 0 )
  {
	$shc_ = "RI_0" ;
  }
  elsif  ( index ( $symbol_, "GOLD" ) == 0 )
  {
	$shc_ = "GD_0" ;
  }
  elsif  ( index ( $symbol_, "ED" ) == 0 )
  {
	$shc_ = "ED_0" ;
  }
  elsif  ( index ( $symbol_, "BR" ) == 0 )
  {
	$shc_ = "BR_0" ;
  }
  elsif  ( index ( $symbol_, "Eu" ) == 0 )
  {
	$shc_ = "Eu_0" ;
  }
  elsif  ( index ( $symbol_, "MIX" ) == 0 )
  {
	$shc_ = "MX_0" ;
  }
  elsif  ( index ( $symbol_, "SBPR" ) == 0 )
  {
	$shc_ = "SBPR_0" ;
  }
  elsif  ( index ( $symbol_, "SBRF" ) == 0 )
  {
	$shc_ = "SBRF_0" ;
  }
  
    my $commish_ = `$INSTALL_BIN/get_contract_specs $shc_ $today_date_ COMMISH | awk '{print \$2}'`; chomp ( $commish_ ) ;
    $symbol_to_commish_map_{$symbol_} = $commish_;
    
    my $n2d_ = `$INSTALL_BIN/get_contract_specs $shc_ $today_date_ N2D | awk '{print \$2}'`; chomp $n2d_ ; 
    $symbol_to_n2d_map_{$symbol_} =$n2d_ ;
    
    $symbol_to_exchange_map_{$symbol_} = "RTS";
}
