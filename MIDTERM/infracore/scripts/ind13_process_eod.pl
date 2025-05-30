#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;
use File::Basename;
use File::Find;
use List::Util qw/max min/; # for max
use Data::Dumper;
use Time::Piece;

#use autodie;
#Handling Agruments
my $USAGE="$0 YYYYMMDD\n";  #TT : Tag Totals / TI: Tagwise Individual
if ( $#ARGV < 0 ) { print "USAGE: ".$USAGE."\n"; exit ( 0 ); }

my $date=shift;

my $EOD_PNL_FILE="/spare/local/logs/pnl_data/hft/tag_pnl/EODPnl/ind13_ors_pnl_$date".".txt";
my $EOD_PNL_FILE_TMP="/spare/local/logs/pnl_data/hft/tag_pnl/EODPnl/ind13_ors_pnl_$date".".txt_tmp";
my $EOD_POS_RECONCILE_FILE="/spare/local/logs/pnl_data/hft/tag_pnl/ind13_eod_pos/ind13_ors_pos_$date".".txt";
my $EOD_POS_FILE="/spare/local/logs/pnl_data/hft/tag_pnl/EODPos/ind13_pos_$date".".txt";
my $PEXEC_BIN="/home/pengine/prod/live_execs";

open OPEN_POSITION_FILE_HANDLE, "< $EOD_PNL_FILE" or die "show_pnl_generic_tagwise.pl could not open ors_trades_filename_ $EOD_PNL_FILE\n";
my @Product_info = <OPEN_POSITION_FILE_HANDLE>;
@Product_info = grep(/LPX/,@Product_info);
open (my $EOD_PNL, '>', "$EOD_PNL_FILE_TMP") or die "Can't open $EOD_PNL_FILE_TMP $!";
open (my $EOD_POS_RECONCILE, '>', "$EOD_POS_RECONCILE_FILE") or die "Can't open $EOD_POS_RECONCILE_FILE $!";
open (my $EOD_POS, '>', "$EOD_POS_FILE") or die "Can't open $EOD_POS_FILE $!";
select $EOD_PNL;
$| = 1;
my $ExchVol = 0;
my $ExchPnl = 0;
for ( my $i = 0 ; $i <= $#Product_info; $i ++ )
{
    my @words_ = split ( ' ', $Product_info[$i] );
    my $symbol_ = $words_[1];chomp($symbol_);
    my $open_positions_ = $words_[9];chomp($open_positions_);
    my $last_price = $words_[17];chomp($last_price);
    my $eod_pnl_ = $words_[5];chomp($eod_pnl_);
    my $volume = $words_[13];chomp($volume);
#    my $contract_specs =`$PEXEC_BIN/get_contract_specs "NSE_$symbol_" $date N2D`;
    my $exchange_sym = `$PEXEC_BIN/nse_details "NSE_$symbol_" $date 3`;chomp($exchange_sym);
#    my @n2d_array = split(' ',$contract_specs);chomp($n2d_array[1]);
    my $settlement_price = `$PEXEC_BIN/nse_details "NSE_$symbol_" $date 1`;chomp($settlement_price);
    my $expiry_date = `$PEXEC_BIN/nse_details "NSE_$symbol_" $date 2`;chomp($expiry_date);
    my $t = Time::Piece->strptime($expiry_date, '%Y%m%d');
    my @underlying = split('_',$symbol_); chomp($underlying[0]);
    $eod_pnl_ += ($open_positions_*($settlement_price - $last_price));
    $ExchVol += $volume;
    $ExchPnl += $eod_pnl_;
   #print recomputed trade.
   select $EOD_PNL;
   $| = 1;
   printf "| %16s | PNL : %10.3f | POS: %6d | VOL: %7d | LPX: %13s |\n",$symbol_,$eod_pnl_, $open_positions_,$volume, $settlement_price;

   #print to position file
   if($open_positions_ != 0){
#Writing to reconcilliation file
     select $EOD_POS_RECONCILE;
     $| = 1;
     printf "%16s %16s %6d %13s\n",$underlying[0],$t->strftime('%d-%b-%Y'), $open_positions_, $settlement_price;

#writing to EOD position file
     select $EOD_POS;
     $| = 1;
     printf "TAGWISE,GBLHFT,%s,%d,%s,%d\n",$exchange_sym, $open_positions_, $settlement_price,$open_positions_;

   }
}

# Update Exchange
select $EOD_PNL;
$| = 1;
printf "\n\n----------------------------------------------------------------------------------------\n\n";
printf "%17s | PNL : %10.3f | VOLUME : %4d |\n","NSE", $ExchPnl, $ExchVol;

close OPEN_POSITION_FILE_HANDLE;
close $EOD_PNL;
close $EOD_POS_RECONCILE;


my $ret=`mv $EOD_PNL_FILE_TMP $EOD_PNL_FILE`;
#system("mv $EOD_PNL_FILE_TMP $EOD_PNL_FILE");
