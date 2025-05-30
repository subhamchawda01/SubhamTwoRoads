#!/usr/bin/perl

#BhavCopy : /spare/local/tradeinfo/NSE_Files/BhavCopy/fo/MMYY/DDMMfo_0000.md
#NORMAL?, 1
#INSTRUMENT, 2
#SYMBOL, 3
#EXPIRY_DT, 4
#STRIKE_PR, 5
#OPTION_TYP, 6
#OPEN, 7
#HIGH, 8
#LOW, 9
#CLOSE, 10
#SETTLE_PR, 11
#CONTRACTS, 12
#VAL_INLAKH, 13
#OPEN_INT, 14
#CHG_IN_OI 15


# HistVol of Future
# Using DailySettlementPrice
# daily_stdev( = ln((St-St-1)/St)) => ann_stdev = sqrt ( 252 ) * daily_stdev
# awk -F, '{gsub(" ",""); if ($2=="FUTIDX" && $3=="NIFTY") print $11}' 0706fo_0000.md | head -n1
use strict;
use warnings;
use List::Util qw/max min/; # for max
#use log;

sub makeShortcodes;
sub collectSettlePx;
sub makeLnReturns;
 
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; #BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetStdev

my $BHAVFILE_="/spare/local/tradeinfo/NSE_Files/BhavCopy/fo/";
my @symbols_ = ();
my @shortcodes_ = ();
my @IDX_STK_ = ();
my %spx_;
my $count_ = 0;
my %rets_;

my $feature_ = "HistVol";

my $start_date_= GetIsoDateFromStrMin1 ( "TODAY-10" );
my $end_date_ = GetIsoDateFromStrMin1 ( "TODAY-1" );

my $USAGE="$0 start_date_ end_date_ feature_ symbol1_ symbol2_";
if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

$start_date_ = GetIsoDateFromStrMin1($ARGV[0]);
$end_date_ = GetIsoDateFromStrMin1($ARGV[1]);
$feature_ = $ARGV[2];

for ( my $i = 3 ; $i <= $#ARGV ; $i++ )
{
    push ( @symbols_, $ARGV[$i] );
}

makeShortcodes();
collectSettlePx();
makeLnReturns();

sub makeShortcodes
{
    for ( my $i = 0; $i < scalar ( @symbols_ ); $i++ )
    {
	push ( @shortcodes_, "NSE_".$symbols_[$i]."_FUT0" );
	if ( $symbols_[$i] eq "BANKNIFTY" ||
	     $symbols_[$i] eq "DJIA" ||
	     $symbols_[$i] eq "FTSE100" ||
	     $symbols_[$i] eq "NIFTY" ||
	     $symbols_[$i] eq "NIFTYINFRA" ||
	     $symbols_[$i] eq "NIFTYIT" ||
	     $symbols_[$i] eq "NIFTYMID50" ||
	     $symbols_[$i] eq "NIFTYPSE" ||
	     $symbols_[$i] eq "S&P500" )
	{
	    push ( @IDX_STK_, "FUTIDX" );
	} else {
	    push ( @IDX_STK_, "FUTSTK" );
	}
	
    }

}

sub collectSettlePx
{
    my $date_ = $start_date_;
    while ( $date_ <= $end_date_ )
    {
	my ( $yyyy_ , $mm_ , $dd_ ) = BreakDateYYYYMMDD ( $date_ );
	my $yy_ = $yyyy_ % 100;
	my $bhavfile_ = $BHAVFILE_.$mm_.$yy_."/".$dd_.$mm_."fo_0000.md";

	if ( ExistsWithSize ( $bhavfile_ ) )
	{
	    $count_++;
	    for ( my $i = 0; $i < scalar ( @shortcodes_ ); $i++ )
	    {
		my $settle_px = `awk -F, '{gsub(\" \",\"\"); if (\$2==\"$IDX_STK_[$i]\" && \$3==\"$symbols_[$i]\") print \$11}' $bhavfile_ | head -n1`;
		chomp($settle_px);
		if ( ! defined($settle_px) )
		{
		    print "no price found for ".$symbols_[$i]." for date"." ".$date_."\n";
		    exit(0);
		}
		push ( @{$spx_{$symbols_[$i]}}, $settle_px);
	    }
	}
	$date_ = CalcNextWorkingDateMult ( $date_, 1 );
    }
}

sub  makeLnReturns
{
    for ( my $j = 0; $j < scalar ( @symbols_ ); $j++ )
    {
	my $k = 0;
	my $i = 1;
	for ( ; $i < $count_ ; $i++, $k++ )
	{
	    my $ret_ = ( $spx_{$symbols_[$j]}[$i] - $spx_{$symbols_[$j]}[$k] ) / $spx_{$symbols_[$j]}[$k] ;
	    push ( @{$rets_{$symbols_[$j]}}, ( $ret_ ) );
	}
	print $symbols_[$j]." ".RoundOff(GetStdev(\@{$rets_{$symbols_[$j]}})*16, 4)."\n";
    }
}

sub RoundOff
{

    my ( $number , $accuracy ) = @_;
    return ( int ( $number * ( 10 ** $accuracy ) ) / ( 10 ** $accuracy ) )
}
