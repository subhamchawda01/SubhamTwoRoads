#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

my $USAGE="$0 ors_trades_filename_ date";
if ( $#ARGV < 1 ) { print $USAGE; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];
my $date_ = $ARGV[1];

my $EUR_TO_DOL = 1.45; # Should have a way of looking this up from the currency info file.
my $CD_TO_DOL =  0.97; # canadian dollar

my $ors_trades_file_base_ = basename ($ors_trades_filename_); chomp ($ors_trades_file_base_);

open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "Could not open ors_trades_filename_ $ors_trades_filename_\n";

my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;

close ORS_TRADES_FILE_HANDLE;

my %symbol_to_commish_map_ = ();
my %symbol_to_n2d_map_ = ();
my %symbol_to_symbol_name_ = ();

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

$exchange_to_unrealpnl_map_{"EUREX"} = 0;
$exchange_to_unrealpnl_map_{"CME"} = 0;
$exchange_to_unrealpnl_map_{"TMX"} = 0;

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

	$symbol_to_volume_map_{$symbol_} += $tsize_;
	$symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
	$symbol_to_price_map_{$symbol_} = $tprice_;
      }
  }

foreach my $symbol_ ( sort keys %symbol_to_price_map_ ) {
    my $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};

    $symbol_to_unrealpnl_map_{$symbol_} = $unreal_pnl_;

    $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
    $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};
}

my $yyyy = substr ($date_, 0, 4);
my $mm = substr ($date_, 4, 2);
my $dd = substr ($date_, 6, 2);

my $mfg_exchange_pnl_filename_ = "/NAS1/data/MFGlobalTrades/ExchangePnl/".$yyyy."/".$mm."/".$dd."/pnl.csv";
my $mfg_product_pnl_filename_ = "/NAS1/data/MFGlobalTrades/ProductPnl/".$yyyy."/".$mm."/".$dd."/pnl.csv";

open MFG_EXCHANGE_PNL_FILE_HANDLE, "< $mfg_exchange_pnl_filename_" or die "Could not open '$mfg_exchange_pnl_filename_'. Exiting.\n";
my @mfg_exchange_pnl_file_lines_ = <MFG_EXCHANGE_PNL_FILE_HANDLE>;
close MFG_EXCHANGE_PNL_FILE_HANDLE;

open MFG_PRODUCT_PNL_FILE_HANDLE, "< $mfg_product_pnl_filename_" or die "Could not open '$mfg_product_pnl_filename_'. Exiting.\n";
my @mfg_product_pnl_file_lines_ = <MFG_PRODUCT_PNL_FILE_HANDLE>;
close MFG_PRODUCT_PNL_FILE_HANDLE;

for (my $i = 0; $i <= $#mfg_product_pnl_file_lines_; $i++) {
    my @words_ = split ( ',', $mfg_product_pnl_file_lines_[$i] );

    my $symbol_ = $words_[1];
    my $pnl_ = $words_[2];
    my $traded_vol_ = $words_[3];

    $symbol_ = substr ($symbol_, 0, -6); # Get rid of the expiry.

    if (! exists ($symbol_to_symbol_name_{$symbol_})) {
	$symbol_to_symbol_name_{$symbol_} = $symbol_;
	$symbol_to_unrealpnl_map_{$symbol_} = 0;
	$symbol_to_volume_map_{$symbol_} = 0;
    }

    $symbol_ = $symbol_to_symbol_name_{$symbol_};

    if (abs ($symbol_to_unrealpnl_map_{$symbol_} - $pnl_) >= 0.09 ||
	$symbol_to_volume_map_{$symbol_} != $traded_vol_) {
	print ("$date_\n\tORS :\t$symbol_ | PNL : $symbol_to_unrealpnl_map_{$symbol_} | VOL : $symbol_to_volume_map_{$symbol_}\n\tMFG:\t$symbol_ | PNL : $pnl_ | VOL : $traded_vol_\n\n");
    }
}

for (my $i = 0; $i <= $#mfg_exchange_pnl_file_lines_; $i++) {
    my @words_ = split ( ',', $mfg_exchange_pnl_file_lines_[$i] );

    my $symbol_ = $words_[1];
    my $pnl_ = $words_[2];
    my $traded_vol_ = $words_[3];

    if ($exchange_to_volume_map_{$symbol_} != $traded_vol_ ||
	abs ($exchange_to_unrealpnl_map_{$symbol_} - $pnl_) >= 0.09) {
	print ("$date_\n\tORS :\t$symbol_ | PNL : $exchange_to_unrealpnl_map_{$symbol_} | VOL : $exchange_to_volume_map_{$symbol_}\n\tMFG:\t$symbol_ | PNL : $pnl_ | VOL : $traded_vol_\n\n");
    }
}

exit (0);

sub SetSecDef 
{
  my $symbol_ = shift;
  if ( index ( $symbol_, "ES" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.15;
      $symbol_to_n2d_map_{$symbol_} = 0.50;
      $symbol_to_exchange_map_{$symbol_} = "CME";
      $symbol_to_symbol_name_{"ES"} = $symbol_;
    }
  if ( index ( $symbol_, "UB" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.11;
      $symbol_to_n2d_map_{$symbol_} = 1000;
      $symbol_to_exchange_map_{$symbol_} = "CME";
      $symbol_to_symbol_name_{"UB"} = $symbol_;
    }
  if ( index ( $symbol_, "ZB" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.14;
      $symbol_to_n2d_map_{$symbol_} = 1000;
      $symbol_to_exchange_map_{$symbol_} = "CME";
      $symbol_to_symbol_name_{"ZB"} = $symbol_;
    }
  if ( index ( $symbol_, "ZN" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.14;
      $symbol_to_n2d_map_{$symbol_} = 1000;
      $symbol_to_exchange_map_{$symbol_} = "CME";
      $symbol_to_symbol_name_{"ZN"} = $symbol_;
    }
  if ( index ( $symbol_, "ZF" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.14;
      $symbol_to_n2d_map_{$symbol_} = 1000;
      $symbol_to_exchange_map_{$symbol_} = "CME";
      $symbol_to_symbol_name_{"ZF"} = $symbol_;
    }
  if ( index ( $symbol_, "ZT" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.14;
      $symbol_to_n2d_map_{$symbol_} = 2000;
      $symbol_to_exchange_map_{$symbol_} = "CME";
      $symbol_to_symbol_name_{"ZT"} = $symbol_;
    }
  if ( index ( $symbol_, "FGBS" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.22 * $EUR_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "EUREX";
      $symbol_to_symbol_name_{"FGBS"} = $symbol_;
    }
  if ( index ( $symbol_, "FGBM" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.22 * $EUR_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "EUREX";
      $symbol_to_symbol_name_{"FGBM"} = $symbol_;
    }
  if ( index ( $symbol_, "FGBL" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.22 * $EUR_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "EUREX";
      $symbol_to_symbol_name_{"FGBL"} = $symbol_;
    }
  if ( index ( $symbol_, "FGBX" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.2 * $EUR_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "EUREX";
      $symbol_to_symbol_name_{"FGBX"} = $symbol_;
    }
  if ( index ( $symbol_, "FESX" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.32 * $EUR_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 10 * $EUR_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "EUREX";
      $symbol_to_symbol_name_{"FESX"} = $symbol_;
    }
  if ( index ( $symbol_, "FDAX" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.5 * $EUR_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 25 * $EUR_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "EUREX";
      $symbol_to_symbol_name_{"FDAX"} = $symbol_;
    }
  if ( index ( $symbol_, "CGB" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.21 * $CD_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 1000 * $CD_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "TMX";
      $symbol_to_symbol_name_{"CGB"} = $symbol_;
    }
   if ( index ( $symbol_, "BAX" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.21 * $CD_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 2500 * $CD_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "TMX";
      $symbol_to_symbol_name_{"BAX"} = $symbol_;
    }
   if ( index ( $symbol_, "SXF" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.21 * $CD_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 200 * $CD_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "TMX";
      $symbol_to_symbol_name_{"SXF"} = $symbol_;
    }

}
