#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

my $USAGE="query_pnl_data.pl PRODUCT/EXCHANGE START_YYYYMMDD END_YYYYMMDD    : (Including START_YYYYMMDD & excluding END_YYYYMMDD).";
my $USAGE_EG1="query_pnl_data.pl CME 20110821 20110915 < Get CME pnls for dates from 20110821 upto 20110915 (Including 20110821 & excluding 20110915)";
my $USAGE_EG2="query_pnl_data.pl ZN 20110821 20110915 < Get ZN pnls for dates from 20110821 upto 20110915 (Including 20110821 & excluding 20110915)";

if ( $#ARGV < 2 ) { print "$USAGE\n\nEXAMPLES:\n\n\t$USAGE_EG1\n\t$USAGE_EG2\n"; exit ( 0 ); }
my $product_exchange_ = $ARGV[0];
my $start_date_ = $ARGV[1];
my $end_date_ = $ARGV[2];

my $is_exchange_ = 0;

if ($product_exchange_ eq "CME" || $product_exchange_ eq "EUREX" || $product_exchange_ eq "BMF" || $product_exchange_ eq "TMX") {
    $is_exchange_ = 1; # Overloaded to store product or exchange.
}

my ($start_yyyy_, $start_mm_, $start_dd_) = BreakDateYYYYMMDD ( $start_date_ );
my ($end_yyyy_, $end_mm_, $end_dd_) = BreakDateYYYYMMDD ( $end_date_ );

my $pnl_base_filename_ = "";

if ($is_exchange_) { # Get the directory to get the pnl csv's from.
    $pnl_base_filename_ = "/NAS1/data/MFGlobalTrades/ExchangePnl/";
} else {
    $pnl_base_filename_ = "/NAS1/data/MFGlobalTrades/ProductPnl/";
}

for ( my $tradingdate_ = $start_date_; $tradingdate_ <= $end_date_ ; )
{
    my ($tradingdate_yyyy_, $tradingdate_mm_, $tradingdate_dd_) = BreakDateYYYYMMDD ( $tradingdate_ );

#for (; !($tradingdate_yyyy_ == $end_yyyy_ && $tradingdate_mm_ == $end_mm_ && $tradingdate_dd_ == $end_dd_); ) {
    my $pnl_filename_ = $pnl_base_filename_.$tradingdate_yyyy_."/".$tradingdate_mm_."/".$tradingdate_dd_."/pnl.csv";

    if (-e $pnl_filename_) {
	open PNL_FILE_HANDLE, "< $pnl_filename_" or die "Fatal error. Exiting.";
	my @pnl_file_lines_ = <PNL_FILE_HANDLE>;
	close PNL_FILE_HANDLE;

	for (my $i = 0; $i <= $#pnl_file_lines_; $i++) {
	    my @fields_ = split (',', $pnl_file_lines_[$i]);

	    my $field_to_match_ = $fields_[1];
	    if ($is_exchange_ == 0) {
		$field_to_match_ = substr ($fields_[1], 0, -6);
	    }

	    if ( ( $fields_[0] eq $tradingdate_ ) && ( $field_to_match_ eq $product_exchange_ ) ) {
		printf "%s,%s,%d,%d\n", $fields_[0], $fields_[1], $fields_[2], $fields_[3];
	    }
	}
    }

    $tradingdate_ = CalcNextDate ( $tradingdate_ );
}

