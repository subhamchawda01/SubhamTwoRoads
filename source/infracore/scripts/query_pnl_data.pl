#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $USAGE="query_pnl_data.pl PRODUCT/EXCHANGE START_YYYYMMDD END_YYYYMMDD    : (Including START_YYYYMMDD & excluding END_YYYYMMDD).";
my $USAGE_EG1="query_pnl_data.pl CME 20110821 20110915 < Get CME pnls for dates from 20110821 upto 20110915 (Including 20110821 & excluding 20110915)";
my $USAGE_EG2="query_pnl_data.pl ZN 20110821 20110915 < Get ZN pnls for dates from 20110821 upto 20110915 (Including 20110821 & excluding 20110915)";

if ( $#ARGV < 2 ) { print "$USAGE\n\nEXAMPLES:\n\n\t$USAGE_EG1\n\t$USAGE_EG2\n"; exit ( 0 ); }
my $product_exchange_ = $ARGV[0];
my $start_date_ = $ARGV[1];
my $end_date_ = $ARGV[2];

my $is_exchange_ = 0;

if ($product_exchange_ eq "CME" || $product_exchange_ eq "EUREX" || $product_exchange_ eq "BMF" || $product_exchange_ eq "TMX" || $product_exchange_ eq "LIFFE" || $product_exchange_ eq "HKEX" ) {
    $is_exchange_ = 1; # Overloaded to store product or exchange.
}

my $start_yyyy_ = substr ($start_date_, 0, 4);
my $start_mm_ = substr ($start_date_, 4, 2);
my $start_dd_ = substr ($start_date_, 6, 2);

my $end_yyyy_ = substr ($end_date_, 0, 4);
my $end_mm_ = substr ($end_date_, 4, 2);
my $end_dd_ = substr ($end_date_, 6, 2);

my $pnl_base_filename_ = "";

if ($is_exchange_) { # Get the directory to get the pnl csv's from.
    $pnl_base_filename_ = "/NAS1/data/MFGlobalTrades/ExchangePnl/";
} else {
    $pnl_base_filename_ = "/NAS1/data/MFGlobalTrades/ProductPnl/";
}

for (; !($start_yyyy_ == $end_yyyy_ && $start_mm_ == $end_mm_ && $start_dd_ == $end_dd_); ) 
{
    my $pnl_filename_ = $pnl_base_filename_.$start_yyyy_."/".$start_mm_."/".$start_dd_."/pnl.csv";

    if (-e $pnl_filename_) 
    {
	open PNL_FILE_HANDLE, "< $pnl_filename_" or die "Fatal error. Exiting.";
	my @pnl_file_lines_ = <PNL_FILE_HANDLE>;
	close PNL_FILE_HANDLE;

	for (my $i = 0; $i <= $#pnl_file_lines_; $i++) 
	{
	    my @fields_ = split (',', $pnl_file_lines_[$i]);
	    
	    if ( $#fields_ >= 1 )
	    {
		my $field_to_match_ = $fields_[1];
		if ($is_exchange_ == 0) {
		    $field_to_match_ = substr ($fields_[1], 0, -6);
		}
		
		if ( ( $fields_[0] eq $start_yyyy_.$start_mm_.$start_dd_ ) 
		     && ( $field_to_match_ eq $product_exchange_ ) 
		     && ( $#fields_ >= 3 ) )
		{
		    printf "%s,%s,%d,%d\n", $fields_[0], $fields_[1], $fields_[2], $fields_[3];
		}
	    }
	}
    }

    # Update yyyy mm dd correctly.
    $start_dd_++;

    if ($start_dd_ > 31) 
    {
	$start_dd_ = 1;
	$start_mm_++;

	if ($start_mm_ > 12) {
	    $start_mm_ = 1;
	    $start_yyyy_++;
	}
    }

    # Sanitize fields to find correct files
    $start_yyyy_ = sprintf ("%04d", $start_yyyy_);
    $start_mm_ = sprintf ("%02d", $start_mm_);
    $start_dd_ = sprintf ("%02d", $start_dd_);
}

