#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

my $USAGE="$0 ors_trades_filename_";
if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];

my $CD_TO_DOL =  0.97; # canadian dollar

my $targetcol = 9;

my $ors_trades_file_base_ = basename ($ors_trades_filename_); chomp ($ors_trades_file_base_);

open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "add_results_to_local_database.pl could not open ors_trades_filename_ $ors_trades_filename_\n";

my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;

close ORS_TRADES_FILE_HANDLE;

my %symbol_to_commish_map_ = ();
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
      }
  }

use Term::ANSIColor; 

my $totalpnl_ = 0.0;
foreach my $symbol_ ( sort keys %symbol_to_price_map_ )
  {

      # As if we were to go flat now with the last seen (executed price ) 
      my $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};

      printf "-------------------------------------------------------------------\n";
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


if ( exists $exchange_to_unrealpnl_map_{"TMX"} )
{
    printf "\tTMX   |";
    print color("BOLD");
    print color("reset");
    printf "| PNL : ";
    if ($exchange_to_unrealpnl_map_{"TMX"} < 0) {
	print color("red"); print color("BOLD");
    } else {
	print color("blue"); print color("BOLD");
    }
    printf "%10.3f ", $exchange_to_unrealpnl_map_{"TMX"};
    print color("reset");
    printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{"TMX"};
}

exit ( 0 );

sub SetSecDef 
{
  my $symbol_ = shift;
   if ( index ( $symbol_, "CGB" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.21 * $CD_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 1000 * $CD_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "TMX";
    }
   if ( index ( $symbol_, "CGF" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.21 * $CD_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 1000 * $CD_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "TMX";
    }
   if ( index ( $symbol_, "CGZ" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.21 * $CD_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 2000 * $CD_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "TMX";
    }
   if ( index ( $symbol_, "BAX" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.21 * $CD_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 2500 * $CD_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "TMX";
    }
   if ( index ( $symbol_, "SXF" ) == 0 )
    {
      $symbol_to_commish_map_{$symbol_} = 0.21 * $CD_TO_DOL;
      $symbol_to_n2d_map_{$symbol_} = 200 * $CD_TO_DOL;
      $symbol_to_exchange_map_{$symbol_} = "TMX";
    }

}
