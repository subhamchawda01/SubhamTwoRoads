#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;
use File::Basename;
use File::Find;
use List::Util;
use Data::Dumper;
use Time::Piece;

#use autodie;
#Handling Agruments
my $USAGE="$0 YYYYMMDD <Exch-FO,CM>\n";  #TT : Tag Totals / TI: Tagwise Individual
if ( $#ARGV < 0 ) { print "USAGE: ".$USAGE."\n"; exit ( 0 ); }

my $date=shift;
my $exch=shift;
#TODO
my $NAS_EOD_PNL_FILE="/NAS1/data/MFGlobalTrades/ind_pnls/$exch"."/eod_pnls/ind_pnl_$date".".txt";
my $NAS_EOD_POS_FILE="/NAS1/data/MFGlobalTrades/ind_pnls/$exch"."/eod_positions/ind_pos_$date".".txt";
my $NAS_EOD_OPEN_POS_FILE="/NAS1/data/MFGlobalTrades/ind_pnls/$exch"."/open_positions_report/ind_ors_pos_$date".".csv";

my $NAS_REMOTE_EOD_PNL_FILE="/NAS1/data/MFGlobalTrades/ind_pnls/$exch"."/eod_pnls/ind_pnl_$date".".txt";
my $NAS_REMOTE_EOD_POS_FILE="/NAS1/data/MFGlobalTrades/ind_pnls/$exch"."/eod_positions/ind_pos_$date".".txt";
my $NAS_REMOTE_EOD_OPEN_POS_FILE="/NAS1/data/MFGlobalTrades/ind_pnls/$exch"."/open_positions_report/ind_ors_pos_$date".".csv";
my $DATASOURCE_EXCHSYMBOL_FILE="/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt";

my $TMP_DIR="/tmp/ind_pnls/settlement/$exch"."/";
`mkdir -p $TMP_DIR`; #Create the directories required

my $EOD_PNL_FILE_TMP= $TMP_DIR."ind_pnl_$date".".txt";
my $TMP_EOD_POS_RECONCILE_FILE= $TMP_DIR."ind_ors_pos_$date".".csv";
my $TMP_EOD_POS_FILE= $TMP_DIR."ind13_pos_$date".".txt";
my $PEXEC_BIN="/home/pengine/prod/live_execs";
my $SETTLEMENT_INPUT="/tmp/settlement_in_$exch".$date;
my $SETTLEMENT_OUTPUT="/tmp/settlement_out_$exch".$date;

my $have_ovn_pos=0;
open OPEN_POSITION_FILE_HANDLE, "< $NAS_EOD_PNL_FILE" or die "show_pnl_generic_tagwise.pl could not open ors_trades_filename_ $NAS_EOD_PNL_FILE\n";
my @Product_info = <OPEN_POSITION_FILE_HANDLE>;

@Product_info = grep(/LPX/,@Product_info);
open (my $EOD_PNL, '>', "$EOD_PNL_FILE_TMP") or die "Can't open $EOD_PNL_FILE_TMP $!";
open (my $EOD_POS_RECONCILE, '>', "$TMP_EOD_POS_RECONCILE_FILE") or die "Can't open $TMP_EOD_POS_RECONCILE_FILE $!";
open (my $EOD_POS, '>', "$TMP_EOD_POS_FILE") or die "Can't open $TMP_EOD_POS_FILE $!";
select $EOD_PNL;
$| = 1;
my $ExchVol = 0;
my $ExchIntradayPnl = 0;
my $ExchOvnPnl = 0;
system("awk '{print \$31;}' < $NAS_EOD_PNL_FILE >$SETTLEMENT_INPUT; >$SETTLEMENT_OUTPUT");
my $cmd_output=`/apps/anaconda/anaconda3/bin/python /home/pengine/prod/live_scripts/get_nse_details_from_symbol.py $date $SETTLEMENT_INPUT $SETTLEMENT_OUTPUT`;
#my $cmd_output=`/apps/anaconda/anaconda3/bin/python /home/dvcinfra/trash/get_nse_details_from_symbol_hardik.py $date $SETTLEMENT_INPUT $SETTLEMENT_OUTPUT`;

my $set_out_cnt=`wc -l < $SETTLEMENT_OUTPUT`;
my $set_in_cnt=`wc -l < $SETTLEMENT_INPUT`;

select STDOUT;
$| = 1;
print "$cmd_output\n";
print "Settlement price log: $set_out_cnt $set_in_cnt\n";

for ( my $i = 0 ; $i <= $#Product_info; $i ++ )
{
    my @words_ = split ( ' ', $Product_info[$i] );
    my $symbol_ = $words_[1];chomp($symbol_);
    my $intraday_pos_ = $words_[7];chomp($intraday_pos_);
    my $ovn_pos_ = $words_[13];chomp($ovn_pos_);
    my $last_price = $words_[19];chomp($last_price);
    my $ovn_pnl_ = $words_[10];chomp($ovn_pnl_);
    my $intraday_pnl_ = $words_[4];chomp($intraday_pnl_);
    my $volume = $words_[16];chomp($volume);
    my $lotsize = $words_[28];chomp($lotsize);
    my $exchange_symbol = $words_[30];chomp($exchange_symbol);
    my $settlement_price =`grep $exchange_symbol $SETTLEMENT_OUTPUT| awk '{print \$3;}'`;chomp($settlement_price);
    if($settlement_price eq ""){
       $settlement_price = $last_price;
    }
    my $expiry_date = `grep $exchange_symbol $SETTLEMENT_OUTPUT| awk '{print \$2;}'`;chomp($expiry_date);
    my $t = Time::Piece->strptime($expiry_date, '%Y%m%d');
    my @underlying = split('_',$symbol_); chomp($underlying[0]);

    $intraday_pnl_ += ($intraday_pos_*($settlement_price - $last_price));
    $ovn_pnl_ += ($ovn_pos_*($settlement_price - $last_price));
    $ExchVol += $volume;
    $ExchIntradayPnl += $intraday_pnl_;
    $ExchOvnPnl +=$ovn_pnl_;
   #print recomputed trade.
   select $EOD_PNL;
   $| = 1;
   printf "| %15s | INTRADAY_PNL: %10.3f | INTRADAY_POS: %6d | OVN_PNL: %10.3f | OVN_POS: %6d | TRADED_VAL: %10d | LPX: %10.3f | TOTAL_PNL: %10.3f | TOTAL_POS: %6d | NO_OF_LOTS: %6d | %15s |\n",$symbol_,$intraday_pnl_,$intraday_pos_,$ovn_pnl_,$ovn_pos_, $volume , $settlement_price, $intraday_pnl_ + $ovn_pnl_, $intraday_pos_ + $ovn_pos_ ,$lotsize, $exchange_symbol;

   #print to position file
   my $open_positions_ = $ovn_pos_+ $intraday_pos_;
   if($open_positions_ != 0){
#Writing to reconcilliation file
     select $EOD_POS_RECONCILE;
     $| = 1;
     if($exch eq 'FO'){
       my $fut_opt=`grep $exchange_symbol $DATASOURCE_EXCHSYMBOL_FILE | awk -F_ '{print \$3;}'`;chomp($fut_opt);
       if($fut_opt eq 'FUT'){
         printf  "%16s, %16s, %6d, %13s\n",$underlying[0],$t->strftime('%d-%b-%Y'), $open_positions_, $settlement_price;
       }elsif(($fut_opt eq 'CE') || ($fut_opt eq 'PE')){
         my $optty_stkpx=`grep $exchange_symbol $DATASOURCE_EXCHSYMBOL_FILE | awk -F_ '{print \$3"_"\$5;}'`;chomp($optty_stkpx);
         printf "%16s, %16s, %6d, %13s, %16s\n",$underlying[0],$t->strftime('%d-%b-%Y'), $open_positions_, $settlement_price, $optty_stkpx;
       }
     }
#writing to EOD position file
     select $EOD_POS;
     $| = 1;
     printf "%s,%s,%s\n",$exchange_symbol, $open_positions_, $settlement_price;


     $have_ovn_pos=1;
   }
}

if($have_ovn_pos == 0){
    select $EOD_POS_RECONCILE;
    $| = 1; 
    printf "No EOD position.\n";
}
# Update Exchange
select $EOD_PNL;
$| = 1;
printf "\n\n----------------------------------------------------------------------------------------\n\n";
printf "| %15s | INTRADAY_PNL: %10.3f | OVN_PNL: %10.3f | TRADED_VAL: %10d |  TOTAL_PNL: %10.3f |\n","NSE", $ExchIntradayPnl, $ExchOvnPnl, $ExchVol, $ExchIntradayPnl + $ExchOvnPnl;

close OPEN_POSITION_FILE_HANDLE;
close $EOD_PNL;
close $EOD_POS_RECONCILE;
close $EOD_POS;

my $TMP_EOD_POS_RECONCILE_FILE_TMP= $TMP_EOD_POS_RECONCILE_FILE."_tmp";
my $ret1=`cat $TMP_EOD_POS_RECONCILE_FILE| sort -k1 >$TMP_EOD_POS_RECONCILE_FILE_TMP; mv $TMP_EOD_POS_RECONCILE_FILE_TMP $TMP_EOD_POS_RECONCILE_FILE`;
`cp $TMP_EOD_POS_RECONCILE_FILE $NAS_REMOTE_EOD_OPEN_POS_FILE; cp $EOD_PNL_FILE_TMP $NAS_REMOTE_EOD_PNL_FILE ;cp $TMP_EOD_POS_FILE $NAS_REMOTE_EOD_POS_FILE ; `;
