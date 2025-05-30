#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore_install";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $INSTALL_BIN="$HOME_DIR/$REPO/bin/";
my $LIVE_BIN="$HOME_DIR/LiveExec/bin/";

my $USAGE="$0 ors_trades_filename_";
if ( $#ARGV < 0 ) { print $USAGE; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];

my $LOAD_ = '' ;

my $CD_TO_DOL =  0.97; # canadian dollar

#================================= fetching conversion rates from currency info file if available ====================#

my $yesterday_file_ = "/tmp/YESTERDAY_DATE" ;
open YESTERDAY_FILE_HANDLE, "< $yesterday_file_" ;

my @yesterday_lines_ = <YESTERDAY_FILE_HANDLE> ;
close YESTERDAY_FILE_HANDLE ;

my @ywords_ = split(' ', $yesterday_lines_[0] );
my $yesterday_date_ = $ywords_[0];

my $prev_date_ = $yesterday_date_ ; 

my $today_date_ = `date +"%Y%m%d"` ; chomp ( $today_date_ ) ;

my $curr_filename_ = '/home/dvcinfra/infracore_install/SysInfo/CurrencyInfo/currency_info_'.$yesterday_date_.'.txt' ;

open CURR_FILE_HANDLE, "< $curr_filename_" ;

my @curr_file_lines_ = <CURR_FILE_HANDLE>;
close CURR_FILE_HANDLE;

for ( my $itr = 0 ; $itr <= $#curr_file_lines_; $itr ++ )
  {

    my @cwords_ = split ( ' ', $curr_file_lines_[$itr] );
    if ( $#cwords_ >= 1 )
      {
        my $currency_ = $cwords_[0] ;

        if( index ( $currency_, "USDCAD" ) == 0 ){
          $CD_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
          last ;
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

my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();

my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();


#======================================================================================================================================#
if ($LOAD_ eq '')
{
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
        my $symbol_ = substr($words_[0],0,4)."1".substr($words_[0],4,1);
        SetSecDef ( $symbol_ ) ;
        if ( ! ( exists ( $symbol_to_exchange_map_{$symbol_} ) ) ) 
        {
          next;
        }
        chomp($words_[1]);
        chomp($words_[2]);
        $symbol_to_pos_map_{$symbol_} = $words_[1];
        $symbol_to_price_map_{$symbol_} = $words_[2];
         
        $symbol_to_unrealpnl_map_{$symbol_} = -$symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_};
        $symbol_to_volume_map_{$symbol_} = 0;
        
      }
    }
  }
}
#=============================================================================================================================#

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

my $date_ = `date`;
printf ("\n$date_\n");

my $totalpnl_ = 0.0;
foreach my $symbol_ ( sort keys %symbol_to_price_map_ )
  {

      # As if we were to go flat now with the last seen (executed price ) 
      my $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};

      print color("BOLD");
      printf "| %12.12s ", $symbol_;
      print color("reset");
      printf "| PNL : ";
      if ($unreal_pnl_ < 0) {
	  print color("red"); print color("BOLD");
      } else {
	  print color("blue"); print color("BOLD");
      }
      printf "%10.3f ", $unreal_pnl_;
      print color("reset");
      my $last_closing_price_ = sprintf ( "%.6f", $symbol_to_price_map_ { $symbol_ } ) ;
      printf "| POSITION : %4d | VOLUME : %5d | LPX : %s | \n", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}, , $last_closing_price_;
      $totalpnl_ += $unreal_pnl_;

      $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
      $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};
  }


if ( exists $exchange_to_unrealpnl_map_{"TMX"} )
{
    printf "\n\n\t%17s   |", "TMX";
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
    printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{"TMX"};
}

exit ( 0 );

sub SetSecDef 
{
    my  $symbol_ = shift ;
    my $sec_def_symbol_ = substr ( $symbol_, 0, length ( $symbol_ ) - 2 ).substr($symbol_, -1, 1 );

    my $shc_ = `$INSTALL_BIN/get_shortcode_for_symbol  "$sec_def_symbol_" $today_date_  2>/dev/null` ; chomp ( $shc_ ) ;
    my $exchange_name_ = `$INSTALL_BIN/get_contract_specs "$shc_"  $today_date_ EXCHANGE 2>/dev/null | awk '{print \$2}'`; chomp ( $exchange_name_ ) ;
    if ( $exchange_name_ eq "TMX" )
    {
        $symbol_to_exchange_map_{$symbol_}=$exchange_name_;
        my $commish_ = `$INSTALL_BIN/get_contract_specs "$shc_" $today_date_ COMMISH | awk '{print \$2}'`; chomp ( $commish_ ) ;
        $symbol_to_commish_map_ {$symbol_} = $commish_;#  GetCommissionForShortcode ( $symbol_ ) ;
        my $n2d_ = `$INSTALL_BIN/get_contract_specs $shc_ $today_date_ N2D | awk '{print \$2}'`; chomp $n2d_ ; 
        $symbol_to_n2d_map_{$symbol_} = $n2d_ ;
    }
}
