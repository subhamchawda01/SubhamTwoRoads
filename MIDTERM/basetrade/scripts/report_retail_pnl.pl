#!/usr/bin/perl
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
if( -d $LIVE_BIN_DIR) {$BINDIR=$LIVE_BIN_DIR;}
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

if ( $#ARGV < 1 ) 
{
  print "USAGE: <script> date strat sendmail=0\n";
  exit(0);
}  
my $date=shift;
if ( $date eq "TODAY" )
{
  $date=`date +%Y%m%d`; chomp($date);
}
my $strat=shift;
if ( ! -s $strat )
{
  print "strat file $strat does not exist\n";
  exit(0);
}

my $sendmail_ = 0;
if ( $#ARGV >= 0 )
{
  $sendmail_ = shift;
}

my ( $yyyy_, $mm_, $dd_ ) = BreakDateYYYYMMDD($date);
my $real_trd_file="/NAS1/logs/QueryTrades/$yyyy_/$mm_/$dd_/trades.$date.30892";

my $uid=`date +%N`; chomp($uid);

my $tmp_file_="/tmp/retail_sim_".$uid;
my $pnl_file_="/tmp/retail_pnl_".$date;
my $real_pnl_file_="/tmp/retail_real_pnl_".$date;
`echo "***********************RetailTradesRecieved************************\n" > $tmp_file_`;
`cat /NAS1/data/Retail_Trades/retail_trades_"$date" >> $tmp_file_`;
`echo "\n\n***********************SIM_PNL************************\n" >> $tmp_file_`;
`echo "ShortCode Pnl Vol" > $pnl_file_`;
`echo "ShortCode Pnl Vol" > $real_pnl_file_`;
my @shc_list_=`~/basetrade/scripts/shc_list_from_retailstrat.sh $strat`; chomp(@shc_list_);
my @results_=`~/basetrade_install/bin/sim_strategy SIM $strat $uid $date 2>/dev/null | grep SIMRESULT`; chomp(@results_);

my $t_pnl_ = 0;
my $t_vol_ = 0;

open FILE, ">> $pnl_file_" or die ( "can't open $pnl_file_" );
for ( my $i=0; $i<=$#results_ ; $i++ )
{
  my @words_ = split(' ', $results_[$i]);
  if ( $#words_>=6 && $words_[0] eq "SIMRESULT" && $words_[2] > 0 )
  {
    $t_pnl_ += $words_[1];
    $t_vol_ += $words_[2];
    
    print FILE "$shc_list_[$i] $words_[1] $words_[2]\n" ;
  }  
}

print FILE "\nEOD: $t_pnl_ $t_vol_\n" ;
close(FILE);

if ( $t_vol_ > 0 )
{
  `cat $pnl_file_ | column -t > $pnl_file_."1"; mv $pnl_file_."1" $pnl_file_`; 
  `chmod 755 $pnl_file_`;
  `rsync $pnl_file_ dvcinfra\@10.23.74.41:/apps/data/Retail_Trades/`;


  `~/basetrade/scripts/retail_pnl_from_strat_and_trdfile.sh $strat $real_trd_file > $real_pnl_file_."1"`; 
  `cat $real_pnl_file_."1" >> $real_pnl_file_`;
  `awk '{p+=\$2; v+=\$3} END{print "EOD:", p, v;}' $real_pnl_file_."1" >> $real_pnl_file_`;

  `cat $real_pnl_file_ | column -t > $real_pnl_file_."1"; mv $real_pnl_file_."1" $real_pnl_file_`; 
  `chmod 755 $real_pnl_file_`;
  `rsync $real_pnl_file_ dvcinfra\@10.23.74.41:/apps/data/Retail_Trades/`;

  my $till_date_line_=`grep "EOD:" /NAS1/data/Retail_Trades/retail_pnl_* | awk 'BEGIN{pnl=0;vol=0} {pnl+=\$(NF-1); vol+=\$NF;} END{printf"%d %d", pnl, vol}'`; chomp($till_date_line_);
  `echo "\nALL: $till_date_line_\n" >> $pnl_file_`;
  `cat $pnl_file_ | column -t >> $tmp_file_`;

  `echo "\n\n**************REAL_PNL******************\n" >> $tmp_file_`;
  `cat $real_pnl_file_ >> $tmp_file_`;
  `echo "\n\nPS: SIM and REAL PNL differ mainly because of IntExecs and in some cases SimReal Bias.\n" >> $tmp_file_`;

  
  my $fin_out_ = `cat $tmp_file_`;
  print "$fin_out_\n";
  if ( $sendmail_ > 0 )
  {
    `/bin/mail -s "RETAIL_PNL $date" -r "nseall@tworoads.co.in" "nseall@tworoads.co.in" < $tmp_file_`;
#`/bin/mail -s "RETAIL_PNL $date" -r "archit\@circulumvite.com" "archit\@circulumvite.com" < $tmp_file_`;
  }
}

`rm -f $tmp_file_`;
`rm -f $pnl_file_`;
