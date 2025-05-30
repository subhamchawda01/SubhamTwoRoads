#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $USAGE="$0 mfglobal_trades_filename_ YYYYMMDD";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $mfg_trades_filename_ = $ARGV[0];
my $expected_date_ = $ARGV[1];

my %mfg_future_code_to_shortcode_ = ();
my %mfg_future_code_to_n2d_ = ();
my %mfg_future_code_to_exchange_ = ();

my %shortcode_to_pnl_ = ();
my %shortcode_to_volume_ = ();
my %shortcode_to_exchange_ = ();

my %exchange_to_pnl_ = ();
my %exchange_to_volume_ = ();

# These instruments need to be configured with NewEdge instrument codes.
$mfg_future_code_to_shortcode_{"SD"} = "FGBS";
$mfg_future_code_to_n2d_{"SD"} = 1000;
$mfg_future_code_to_exchange_{"SD"} = "EUREX";

$mfg_future_code_to_shortcode_{"BC"} = "FGBM";
$mfg_future_code_to_n2d_{"BC"} = 1000;
$mfg_future_code_to_exchange_{"BC"} = "EUREX";

$mfg_future_code_to_shortcode_{"BM"} = "FGBL";
$mfg_future_code_to_n2d_{"BM"} = 1000;
$mfg_future_code_to_exchange_{"BM"} = "EUREX";

$mfg_future_code_to_shortcode_{"SF"} = "FESX";
$mfg_future_code_to_n2d_{"SF"} = 10;
$mfg_future_code_to_exchange_{"SF"} = "EUREX";

$mfg_future_code_to_shortcode_{"25"} = "ZF";
$mfg_future_code_to_n2d_{"25"} = 1000;
$mfg_future_code_to_exchange_{"25"} = "CME";

$mfg_future_code_to_shortcode_{"21"} = "ZN";
$mfg_future_code_to_n2d_{"21"} = 1000;
$mfg_future_code_to_exchange_{"21"} = "CME";

$mfg_future_code_to_shortcode_{"CBT 2YR T-NOTE"} = "ZT";
$mfg_future_code_to_n2d_{"CBT 2YR T-NOTE"} = 2000;
$mfg_future_code_to_exchange_{"CBT 2YR T-NOTE"} = "CME";

$mfg_future_code_to_shortcode_{"CBT US T-BONDS"} = "ZB";
$mfg_future_code_to_n2d_{"CBT US T-BONDS"} = 1000;
$mfg_future_code_to_exchange_{"CBT US T-BONDS"} = "CME";

$mfg_future_code_to_shortcode_{"BA"} = "BAX";
$mfg_future_code_to_n2d_{"BA"} = 2500;
$mfg_future_code_to_exchange_{"BA"} = "TMX";

$mfg_future_code_to_shortcode_{"CG"} = "CGB";
$mfg_future_code_to_n2d_{"CG"} = 1000;
$mfg_future_code_to_exchange_{"CG"} = "TMX";

$mfg_future_code_to_shortcode_{"?"} = "SXF";
$mfg_future_code_to_n2d_{"?"} = 200;
$mfg_future_code_to_exchange_{"?"} = "TMX";

# These offsets were obtained using the 'FIMST4F1.xls & FIMST4F1.csv' template files
# These will be used to determine the field being processed after splitting each trade line
my $record_id_offset_ = 0; # PRECID # Needed to figure out which records to use towards pnl computation.
my $date_offset_ = 11; # PTDATE
my $buysell_offset_ = 12; # PBS # 1 = buy ; 2 = sell
my $quantity_offset_ = 17; # PQTY
my $mfg_future_code_offset_ = 23; # PFC
my $exch_code_offset_ = 22; # PEXCH
my $contract_yr_month_offset_ = 6; # PCTYM  # YYYYMM
my $trade_price_offset_ = 26; # PTPRIC # Trade price may be different than price of order

# Need to figure out how many charges are placed on us.
my $comm_amount_offset_ = 36;
my $fee1_amount_offset_ = 37;
my $fee2_amount_offset_ = 38;
my $fee3_amount_offset_ = 39;

# EURTODOL
my $CD_TO_DOL_ = 0.97;
my $EUR_TO_DOL_ = 1.45;

# pnl computation from the MFG trades files are simpler, given the commission amounts
open MFG_TRADES_FILE_HANDLE, "< $mfg_trades_filename_" or die "could not open mfg_trades_filename_ $mfg_trades_filename_\n";

my @mfg_trades_file_lines_ = <MFG_TRADES_FILE_HANDLE>;

close MFG_TRADES_FILE_HANDLE;

for (my $i = 0; $i <= $#mfg_trades_file_lines_; $i++) {
    my @fields_ = split (',', $mfg_trades_file_lines_[$i]);

    if ($#fields_ != 106) {
	print "Malformatted line : $mfg_trades_file_lines_[$i] No. of fields : $#fields_\n";
	next;
    }

    if (length ($fields_[$mfg_future_code_offset_]) < 2) {
	next;
    }

    # NewEdge trade entries are listed twice in the file.
    # We only use the one specifying trade information.
    my $record_id_ = substr ($fields_[$record_id_offset_], 1, -1); $record_id_ =~ s/^\s+|\s+$//g;

    if ($record_id_ ne "T") {
	next;
    }

    my $future_code_ = substr ($fields_[$mfg_future_code_offset_], 1, -1); $future_code_ =~ s/^\s+|\s+$//g;

    if (!exists ($mfg_future_code_to_shortcode_{$future_code_})) {
	if ($future_code_ ne "PFC") {
	    print "No shortcode known for mfg future_code : $future_code_\n";
	}

	next;
    }

    my $date_ = substr ($fields_[$date_offset_], 1, -1); $date_ =~ s/^\s+|\s+$//g;

    if ($date_ ne $expected_date_) {
	print "MFGTrades file doesnot match date provided\n";
	exit (1);
    }

    my $contract_date_ = substr ($fields_[$contract_yr_month_offset_], 1, -1); $contract_date_ =~ s/^\s+|\s+$//g;
    my $shortcode_ = $mfg_future_code_to_shortcode_{$future_code_}.$contract_date_;
    my $number_to_dollar_ = $mfg_future_code_to_n2d_{$future_code_};

    my $comm_amount_ = $fields_[$comm_amount_offset_]; $comm_amount_ =~ s/^\s+|\s+$//g;
    my $fee1_amount_ = $fields_[$fee1_amount_offset_]; $fee1_amount_ =~ s/^\s+|\s+$//g;
    my $fee2_amount_ = $fields_[$fee2_amount_offset_]; $fee2_amount_ =~ s/^\s+|\s+$//g;
    my $fee3_amount_ = $fields_[$fee3_amount_offset_]; $fee3_amount_ =~ s/^\s+|\s+$//g;

    # Factor the commissions into the pnl
    # += because the commission ammounts are already -ve
    if (! exists $shortcode_to_pnl_{$shortcode_}) {
	$shortcode_to_pnl_{$shortcode_} = 0.0;
    }
    $shortcode_to_pnl_{$shortcode_} += ($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_);

    # Add the trade price into the pnl
    my $buysell_ = substr ($fields_[$buysell_offset_], 1, -1); $buysell_ =~ s/^\s+|\s+$//g;
    my $trade_price_ = $fields_[$trade_price_offset_]; $trade_price_ =~ s/^\s+|\s+$//g;
    my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;

    if ($buysell_ == 1) { # Buy
	$shortcode_to_pnl_{$shortcode_} -= ($trade_price_ * $quantity_ * $number_to_dollar_);
    } elsif ($buysell_ == 2) { # Sell
	$shortcode_to_pnl_{$shortcode_} += ($trade_price_ * $quantity_ * $number_to_dollar_);
    } else {
	print "Illegal buysell code : $buysell_\n";
	exit (1);
    }

    $shortcode_to_volume_{$shortcode_} += $quantity_;

    if (!exists ($shortcode_to_exchange_{$shortcode_})) {
	$shortcode_to_exchange_{$shortcode_} = $mfg_future_code_to_exchange_{$future_code_};
    }
}

my $shortcode_ = "";
foreach $shortcode_ (sort keys %shortcode_to_pnl_) {
    if ($shortcode_to_exchange_{$shortcode_} eq "EUREX") {
	# Values are still in euros, convert to dollars
	$shortcode_to_pnl_{$shortcode_} *= $EUR_TO_DOL_;
    }
    if ($shortcode_to_exchange_{$shortcode_} eq "TMX") {
	# Values are still in canadian $, convert to dollars
	$shortcode_to_pnl_{$shortcode_} *= $CD_TO_DOL_;
    }

    $exchange_to_pnl_{$shortcode_to_exchange_{$shortcode_}} += $shortcode_to_pnl_{$shortcode_};
    $exchange_to_volume_{$shortcode_to_exchange_{$shortcode_}} += $shortcode_to_volume_{$shortcode_};
}

my $exchange_ = "";
foreach $exchange_ (sort keys %exchange_to_pnl_) {
    print "$expected_date_,$exchange_,$exchange_to_pnl_{$exchange_},$exchange_to_volume_{$exchange_}\n";
}
