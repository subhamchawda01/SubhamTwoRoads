#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."/scripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="query_pnl_data_mult.pl YYYYMMDD NUM_OF_PREV_DAYS [\"ALL\"/ PRODUCT/EXCHANGE_1 PRODUCT/EXCHANGE_2 ...-1 TIMEPERIOD[US/EU/ALL]  : (Including START_YYYYMMDD & excluding END_YYYYMMDD).";
my $USAGE_EG1="query_pnl_data_mult.pl 20110821 20 CME EUREX CME ";
my $USAGE_EG2="query_pnl_data_mult.pl 20110821 25 ZF ZN ZB EU ";

my @product_exchange_vec_ =();

if ( $#ARGV < 3 ) { print "$USAGE\n\nEXAMPLES:\n\n\t$USAGE_EG1\n\t$USAGE_EG2\n"; exit ( 0 ); }


my $yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[0] );
my $num_of_days_ = $ARGV [ 1 ] ;

my $idx = 2;

for ( ; $idx <= $#ARGV ; $idx ++ )
{
    if ( $ARGV[$idx] eq "-1" )
    {
	last ;
    }
    else
    {
	push ( @product_exchange_vec_, $ARGV[$idx] );
    }
}

my $period_ = "ALL";

$idx ++ ;

if ( $idx < scalar ( @ARGV ) )
{
    $period_ = $ARGV [ $idx ] ;
}

my @period_start_string_ = ( ) ;
my @period_end_string_ = ( ) ;
my @period_ = ( ) ;

#Usage : SHORTCODE YYYYMMDD NUM_OF_DAYS START_TIME END_TIME SUMMARY/DETAILS
#Example : HSI_0 20130214 30 HKT_800 HKT_1500 DETAILS
my $pnl_by_time_script  =  "$BIN_DIR/get_pnl_for_shortcode_bytime_stats";

if ( $period_ eq "US" || $period_ eq "ALL" )
{
    push ( @period_, "US" ) ;
    push ( @period_start_string_ , "EST_0800" ) ;  # EST RANGE 0010 1955 == UTC 0410 UTC 2355
    push ( @period_end_string_ ,  "EST_1700" ) ;  
}
if ( $period_ eq "EU" || $period_ eq "ALL" )
{
    push ( @period_, "EU" ) ;
    push ( @period_start_string_ , "EST_0100" ) ;
    push ( @period_end_string_ , "EST_0800" ) ;
}


my $stat_line_ = ( ) ;


for ( my $idx = 0 ; $idx < scalar ( @period_ ) ; $idx ++ )
{
    printf "%-13s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s",
    "$period_[ $idx]_SHCODE","$period_[ $idx]_PNLSUM","$period_[ $idx]_PNLAVG","$period_[ $idx]_PNLSTD","$period_[ $idx]_PNLSHRP","$period_[ $idx]_VOLAVG","$period_[ $idx]_MAXDD","$period_[ $idx]_AVG/DD","$period_[ $idx]_GPR";
}

print "\n" ;

foreach my $product_exchange_ ( @product_exchange_vec_ ) 
{
    my $is_exchange_ = 0;

    if ( $product_exchange_ eq "CME" || $product_exchange_ eq "EUREX" || $product_exchange_ eq "BMF" || $product_exchange_ eq "TMX" || $product_exchange_ eq "LIFFE" || $product_exchange_ eq "HKEX" ) 
    {

    }
    else
    {

	for ( my $idx = 0 ; $idx < scalar ( @period_ ) ; $idx ++ )
	{
	    $stat_line_  = `$pnl_by_time_script $product_exchange_ $yyyymmdd_ $num_of_days_ $period_start_string_[ $idx ] $period_end_string_[ $idx ] SUMMARY`  ;

	    chomp ( $stat_line_ ) ;

	    my @tokens_ = split ( '\|' , $stat_line_ ) ;
	    if ( scalar ( @tokens_ ) >= 9 )
	    {
		printf "%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s ",
		$tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ] ;
	    }
	}
    }
    print "\n" ;
    
#    if ( $volume_ > 0 ) { my $pnl_per_c_ = $pnl_/$volume_; printf "%s,%d,%d,%.2f\n", $tradingdate_, $pnl_, $volume_, $pnl_per_c_; }
}
