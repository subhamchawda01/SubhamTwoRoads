#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $USAGE="$0 mfglobal_trades_filename_ YYYYMMDD";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $mfg_trades_filename_ = $ARGV[0];
my $expected_date_ = $ARGV[1];

my %mfg_symbol_to_shortcode_ = ();
my %mfg_symbol_to_n2d_ = ();
my %mfg_symbol_to_exchange_ = ();

my %shortcode_to_pnl_ = ();
my %shortcode_to_volume_ = ();
my %shortcode_to_exchange_ = ();

# These values are obtained by manually scanning through the MFG trades files
$mfg_symbol_to_shortcode_{"EURO E-SCHATZ"} = "FGBS";
$mfg_symbol_to_n2d_{"EURO E-SCHATZ"} = 1000;
$mfg_symbol_to_exchange_{"EURO E-SCHATZ"} = "EUREX";

$mfg_symbol_to_shortcode_{"EURX EUROBOBL"} = "FGBM";
$mfg_symbol_to_n2d_{"EURX EUROBOBL"} = 1000;
$mfg_symbol_to_exchange_{"EURX EUROBOBL"} = "EUREX";

$mfg_symbol_to_shortcode_{"EURX E-BUND"} = "FGBL";
$mfg_symbol_to_n2d_{"EURX E-BUND"} = 1000;
$mfg_symbol_to_exchange_{"EURX E-BUND"} = "EUREX";

$mfg_symbol_to_shortcode_{"DTB EU STX 50"} = "FESX";
$mfg_symbol_to_n2d_{"DTB EU STX 50"} = 10;
$mfg_symbol_to_exchange_{"DTB EU STX 50"} = "EUREX";

$mfg_symbol_to_shortcode_{"CBT TNOTE 5Y"} = "ZF";
$mfg_symbol_to_n2d_{"CBT TNOTE 5Y"} = 1000;
$mfg_symbol_to_exchange_{"CBT TNOTE 5Y"} = "CME";

$mfg_symbol_to_shortcode_{"CBT TNOTE 10Y"} = "ZN";
$mfg_symbol_to_n2d_{"CBT TNOTE 10Y"} = 1000;
$mfg_symbol_to_exchange_{"CBT TNOTE 10Y"} = "CME";

$mfg_symbol_to_shortcode_{"CBT TNOTE 2Y"} = "ZT";
$mfg_symbol_to_n2d_{"CBT TNOTE 2Y"} = 2000;
$mfg_symbol_to_exchange_{"CBT TNOTE 2Y"} = "CME";

$mfg_symbol_to_shortcode_{"CBT T-BONDS"} = "ZB";
$mfg_symbol_to_n2d_{"CBT T-BONDS"} = 1000;
$mfg_symbol_to_exchange_{"CBT T-BONDS"} = "CME";

# These offsets were obtained using the 'trd_hdr.csv' template file
# These will be used to determine the field being processed after splitting each trade line
my $date_offset_ = 1;
my $buysell_offset_ = 5; # 1 = buy ; 2 = sell
my $quantity_offset_ = 6;
my $mfg_symbol_offset_ = 8;
my $contract_yr_month_offset_ = 9; # YYYYMM
my $price_offset_ = 13;
my $trade_price_offset_ = 26; # Trade price may be different than price of order
my $comm_amount_offset_ = 32;
my $fee1_amount_offset_ = 34;
my $fee2_amount_offset_ = 36;
my $fee3_amount_offset_ = 38;

# EURTODOL
my $EUR_TO_DOL_ = 1.45;

# pnl computation from the MFG trades files are simpler, given the commission amounts
open MFG_TRADES_FILE_HANDLE, "< $mfg_trades_filename_" or die "could not open mfg_trades_filename_ $mfg_trades_filename_\n";

my @mfg_trades_file_lines_ = <MFG_TRADES_FILE_HANDLE>;

close MFG_TRADES_FILE_HANDLE;

for (my $i = 0; $i <= $#mfg_trades_file_lines_; $i++) {
    my @fields_ = split (',', $mfg_trades_file_lines_[$i]);

    if ($#fields_ != 58) {
	print "Malformatted line : $mfg_trades_file_lines_[$i] No. of fields : $#fields_\n";
	next;
    }

    my $date_ = substr ($fields_[$date_offset_], 1, -1); $date_ =~ s/^\s+|\s+$//g;

    if ($date_ ne $expected_date_) {
	print "MFGTrades file doesnot match date provided\n";
	exit (1);
    }

    my $symbol_ = substr ($fields_[$mfg_symbol_offset_], 1, -1); $symbol_ =~ s/^\s+|\s+$//g;

    if (!exists ($mfg_symbol_to_shortcode_{$symbol_})) {
	print "No shortcode known for mfg symbol : $symbol_\n";
	exit (1);
    }

    my $contract_date_ = substr ($fields_[$contract_yr_month_offset_], 1, -1); $contract_date_ =~ s/^\s+|\s+$//g;
    my $shortcode_ = $mfg_symbol_to_shortcode_{$symbol_}.$contract_date_;
    my $number_to_dollar_ = $mfg_symbol_to_n2d_{$symbol_};

    my $comm_amount_ = substr ($fields_[$comm_amount_offset_], 1, -1); $comm_amount_ =~ s/^\s+|\s+$//g;
    my $fee1_amount_ = substr ($fields_[$fee1_amount_offset_], 1, -1); $fee1_amount_ =~ s/^\s+|\s+$//g;
    my $fee2_amount_ = substr ($fields_[$fee2_amount_offset_], 1, -1); $fee2_amount_ =~ s/^\s+|\s+$//g;
    my $fee3_amount_ = substr ($fields_[$fee3_amount_offset_], 1, -1); $fee3_amount_ =~ s/^\s+|\s+$//g;

    # Factor the commissions into the pnl
    # += because the commission ammounts are already -ve
    $shortcode_to_pnl_{$shortcode_} += ($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_);

    # Add the trade price into the pnl
    my $buysell_ = substr ($fields_[$buysell_offset_], 1, -1); $buysell_ =~ s/^\s+|\s+$//g;
    my $trade_price_ = substr ($fields_[$trade_price_offset_], 1, -1); $trade_price_ =~ s/^\s+|\s+$//g;
    my $price_ = substr ($fields_[$price_offset_], 1, -1); $price_ =~ s/^\s+|\s+$//g;
    my $quantity_ = substr ($fields_[$quantity_offset_], 1, -1); $quantity_ =~ s/^\s+|\s+$//g;

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
	$shortcode_to_exchange_{$shortcode_} = $mfg_symbol_to_exchange_{$symbol_};
    }
}

my $shortcode_ = "";
foreach $shortcode_ (sort keys %shortcode_to_pnl_) {
    if ($shortcode_to_exchange_{$shortcode_} eq "EUREX") {
	# Values are still in euros, convert to dollars
	$shortcode_to_pnl_{$shortcode_} *= $EUR_TO_DOL_;
    }

    print "$expected_date_\t$shortcode_\tPNL : $shortcode_to_pnl_{$shortcode_}\t VOL : $shortcode_to_volume_{$shortcode_}\n";
}
