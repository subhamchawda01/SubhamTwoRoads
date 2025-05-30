#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="query_pnl_data_mult.pl START_YYYYMMDD END_YYYYMMDD [\"ALL\"/ PRODUCT/EXCHANGE_1 PRODUCT/EXCHANGE_2 ... ]  : (Including START_YYYYMMDD & excluding END_YYYYMMDD).";
my $USAGE_EG1="query_pnl_data_mult.pl 20110821 20110915 CME EUREX < Get CME,EUREX pnls for dates from 20110821 upto 20110915 (Including 20110821 & excluding 20110915)";
my $USAGE_EG2="query_pnl_data_mult.pl 20110821 20110915 ZF ZN ZB < Get ZF,ZN,ZB pnls for dates from 20110821 upto 20110915 (Including 20110821 & excluding 20110915)";

my @product_exchange_vec_ =();

if ( $#ARGV < 2 ) { print "$USAGE\n\nEXAMPLES:\n\n\t$USAGE_EG1\n\t$USAGE_EG2\n"; exit ( 0 ); }
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[0] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStr ( $ARGV[1] );
for ( my $i = 2; $i <= $#ARGV; $i ++ )
{
    if ( $ARGV[$i] eq "ALL" )
    {
	@product_exchange_vec_ = ();
	push ( @product_exchange_vec_, "TMX" );
	push ( @product_exchange_vec_, "EUREX" );
	push ( @product_exchange_vec_, "CME" );
	push ( @product_exchange_vec_, "BMF" );
	push ( @product_exchange_vec_, "LIFFE" );
	last;
    }
    else
    {
	push ( @product_exchange_vec_, $ARGV[$i] );
    }
}

my $tradingdate_ = $trading_end_yyyymmdd_;
my $max_days_at_a_time_ = 2000;
for ( my $j = 0 ; $j < $max_days_at_a_time_ ; $j ++ ) 
{
    if ( ( ! ValidDate ( $tradingdate_ ) ) ||
	 ( $tradingdate_ < $trading_start_yyyymmdd_ ) )
      { last; }

    my ( $tradingdate_YYYY_, $tradingdate_MM_, $tradingdate_DD_) = BreakDateYYYYMMDD ( $tradingdate_ );

    my $pnl_ = 0;
    my $volume_ = 0;
    foreach my $product_exchange_ ( @product_exchange_vec_ ) 
    {
	my $pnl_base_filename_ = "/NAS1/data/MFGlobalTrades/ProductPnl/";
	my $is_exchange_ = 0;
	if ( $product_exchange_ eq "CME" || $product_exchange_ eq "EUREX" || $product_exchange_ eq "BMF" || $product_exchange_ eq "TMX" || $product_exchange_ eq "LIFFE" ) 
	{
	    $pnl_base_filename_ = "/NAS1/data/MFGlobalTrades/ExchangePnl/";
	    $is_exchange_ = 1; # Overloaded to store product or exchange.
	}
	my $pnl_filename_ = $pnl_base_filename_.$tradingdate_YYYY_."/".$tradingdate_MM_."/".$tradingdate_DD_."/pnl.csv";
#	print STDERR "Trying $tradingdate_, $product_exchange_ $pnl_filename_\n";
	
	if (-e $pnl_filename_) 
	{
	    open PNL_FILE_HANDLE, "< $pnl_filename_" or PrintStacktraceAndDie ( "Fatal error. Exiting." );
	    my @pnl_file_lines_ = <PNL_FILE_HANDLE>;
	    close PNL_FILE_HANDLE;
	    chomp ( @pnl_file_lines_ );
	    for (my $i = 0; $i <= $#pnl_file_lines_; $i++) 
	    {
		my @fields_ = split (',', $pnl_file_lines_[$i]);
		if ( $#fields_ >= 3 )
		{
		    my $field_to_match_ = $fields_[1];
#		    if ($is_exchange_ == 0) 
#		    {
#			$field_to_match_ = substr ($fields_[1], 0, -6);
#		    }
#		    print STDERR "$product_exchange_ $field_to_match_ ".$pnl_file_lines_[$i]."\n";
#		    print STDERR "$tradingdate_ ".$fields_[0]."\n";
		    if ( ( $fields_[0] eq $tradingdate_ ) && 
			 ( index ( $field_to_match_, $product_exchange_ ) == 0 ) )
		    {
			$pnl_ += int ( $fields_[2] );
			$volume_ += int ( $fields_[3] );
#			print STDERR "ADDING $pnl_ $volume_\n";
		    }
		}
	    }
	    
	}
    }
    if ( $volume_ > 0 ) { my $pnl_per_c_ = $pnl_/$volume_; printf "%s,%d,%d,%.2f\n", $tradingdate_, $pnl_, $volume_, $pnl_per_c_; }
    $tradingdate_ = CalcPrevDate ( $tradingdate_ );
}
