=Begin
see_ors_pnl_lopr.pl:
This script takes a trade file as input and computes and prints the final product positions. It is used for TMX
EOD Positions report submissions.

Ussage:
<script> <ors_trade_file(Required)> <1 - Dump positions(Optional)> <Load Overnight Positions(Optional) - Default>
=End
=cut

#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;
use List::Util qw/max min/; # for max

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore_install";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $INSTALL_BIN="$HOME_DIR/$REPO/bin/";
my $LIVE_BIN="/home/pengine/prod/live_execs/";

require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; 							#Returns the previous working day
require "$GENPERLLIB_DIR/get_rts_ticksize.pl"; 									#RITickSize
require "$GENPERLLIB_DIR/get_shortcode_from_symbol.pl"; 						#GetShortcodeFromSymbol

#============================================ Ussage and Command Line Arguments ===================================================#

my $USAGE="$0 ors_trades_filename_ [dump_overnight_pos] [dont_load_over_night]";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];
my $DUMP_OVERNIGHT_POS = ''; 		#Doesn't Dump overnight positions by default
my $LOAD_ = '' ; 					#Loads overnight positions by default

if ( $#ARGV >= 1 )
{
  $DUMP_OVERNIGHT_POS = $ARGV[1];
}

if ( $#ARGV >= 2 )
{
   $LOAD_ =$ARGV[2];
}

#============================================ Get Previous Working Day ===================================================#

my $yesterday_file_ = "/tmp/YESTERDAY_DATE" ;		#File storing previous weekday date
open YESTERDAY_FILE_HANDLE, "< $yesterday_file_" ;

my @yesterday_lines_ = <YESTERDAY_FILE_HANDLE> ;
close YESTERDAY_FILE_HANDLE ;

my @ywords_ = split(' ', $yesterday_lines_[0] );
my $yesterday_date_ = $ywords_[0];

my $prev_date_ = $yesterday_date_ ; 

#=======================================Maps for Storing Pnl and Volumes===============================================================#
my %symbol_to_pos_map_ = ();			#Symbol to position map
my %symbol_to_price_map_ = ();			#Symbol to Last traded price map
my %symbol_to_volume_map_ = ();			#Symbol to total traded volume map

#=======================================Codes for Product Expiry Map=========================================================#
my %symbol_to_expiry_year_ = ("0" => "2010", "1" => "2011", "2" => "2012", "3" => "2013", "4" => "2014", "5" => "2015", "6" => "2016");
my %symbol_to_expiry_month_ = ("F" => "01", "G" => "02", "H" => "03", "J" => "04", "K" => "05", "M" => "06", "N" => "07", "Q" => "08", "U" => "09", "V" => "10", "X" => "11", "Z" => "12");
my %expiry_month_to_symbol_ = ("01" => "F", "02" => "G", "03" => "H", "04" => "J", "05" => "K", "06" => "M", "07" => "N", "08" => "Q", "09" => "U", "10" => "V", "11" => "X", "12" => "Z");

#===================================Fetch Trades from trade files===================================================#
my $ors_trades_file_base_ = basename ($ors_trades_filename_); chomp ($ors_trades_file_base_);

open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "add_results_to_local_database.pl could not open ors_trades_filename_ $ors_trades_filename_\n";

my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;

close ORS_TRADES_FILE_HANDLE;

#======================================================Load Previous Day EOD Positions=====================================================#
if ($LOAD_ eq '')
{
	# Load previous day EOD positions.
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
        chomp($words_[1]);
        chomp($words_[2]);
        $symbol_to_pos_map_{$symbol_} = $words_[1];
        $symbol_to_price_map_{$symbol_} = $words_[2];
        $symbol_to_volume_map_{$symbol_} = 0;
      }
    }
  }
}

#=======================================Positions Computation Logic==============================================================#
for ( my $i = 0 ; $i <= $#ors_trades_file_lines_; $i ++ )
{
  my @words_ = split ( '', $ors_trades_file_lines_[$i] );
  if ( $#words_ >= 4 )
  {
  	#Parse the trade file to fetch trade information
    my $symbol_ = $words_[0];
    my $buysell_ = $words_[1];
    my $tsize_ = $words_[2];
    my $tprice_ = $words_[3];
    my $saos_ = $words_[4];

    if ( ! ( exists $symbol_to_pos_map_{$symbol_} ) )
    {
		# Initialize symbol positions, price and volumes maps
      $symbol_to_pos_map_{$symbol_} = 0;
      $symbol_to_price_map_{$symbol_} = 0;
      $symbol_to_volume_map_{$symbol_} = 0;
    }
    if ( $buysell_ == 0 )
    { # buy
      $symbol_to_pos_map_{$symbol_} += $tsize_;
    }
    else
    { # sell
      $symbol_to_pos_map_{$symbol_} -= $tsize_;
    }
    
	# Update symbol volumes and prices
    $symbol_to_volume_map_{$symbol_} += $tsize_;
    $symbol_to_price_map_{$symbol_} = $tprice_;

  }
}

#=======================================Print EOD Positions For Product with Non Zero Positions===========================#
if($DUMP_OVERNIGHT_POS eq "1")
{
	# Print the final positions on STDOUT
  foreach my $symbol_ (sort keys %symbol_to_pos_map_)
  {
      if($symbol_to_pos_map_{$symbol_} != 0){
      	# Print products with non zero positions only
      	print "$symbol_,$symbol_to_pos_map_{$symbol_},$symbol_to_price_map_{$symbol_}\n";
      }
  }

  exit(0);
}