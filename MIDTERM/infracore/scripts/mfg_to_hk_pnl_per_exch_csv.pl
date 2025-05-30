#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $USAGE="$0 mfglobal_trades_filename_ YYYYMMDD";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $mfg_trades_filename_ = $ARGV[0];
my $expected_date_ = $ARGV[1];

my %mfg_exch_future_code_to_shortcode_ = ();
my %mfg_exch_future_code_to_n2d_ = ();
my %mfg_exch_future_code_to_exchange_ = ();

my %shortcode_to_pnl_ = ();
my %shortcode_to_last_pos_ = ();
my %shortcode_to_last_price_ = ();
my %shortcode_to_n2d_ = ();
my %shortcode_to_volume_ = ();
my %shortcode_to_exchange_ = ();

my %exchange_to_pnl_ = ();
my %exchange_to_volume_ = ();

# These instruments need to be configured with NewEdge instrument codes.
$mfg_exch_future_code_to_shortcode_{"33MH"} = "MHI";
$mfg_exch_future_code_to_n2d_{"33MH"} = 1000;
$mfg_exch_future_code_to_exchange_{"33MH"} = "HKEX";

$mfg_exch_future_code_to_shortcode_{"33HC"} = "HHI";
$mfg_exch_future_code_to_n2d_{"33HC"} = 50;
$mfg_exch_future_code_to_exchange_{"33HC"} = "HKEX";

$mfg_exch_future_code_to_shortcode_{"33HS"} = "HSI";
$mfg_exch_future_code_to_n2d_{"33HS"} = 5000;
$mfg_exch_future_code_to_exchange_{"33HS"} = "HKEX";

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
my $closing_price_offset_ = 27; # will used to settle open positions if any

# Need to figure out how many charges are placed on us.
my $comm_amount_offset_ = 36;
my $fee1_amount_offset_ = 37;
my $fee2_amount_offset_ = 38;
my $fee3_amount_offset_ = 39;

# EURTODOL
my $HKD_TO_DOL_ = 0.1289 ;

#================================= fetching conversion rates from currency info file if available ====================#

my $curr_date_ = $expected_date_ ;

my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;

open CURR_FILE_HANDLE, "< $curr_filename_" ;

my @curr_file_lines_ = <CURR_FILE_HANDLE>;
close CURR_FILE_HANDLE;

for ( my $itr = 0 ; $itr <= $#curr_file_lines_; $itr ++ )
  {

    my @cwords_ = split ( ' ', $curr_file_lines_[$itr] );
    if ( $#cwords_ >= 1 )
      {
        my $currency_ = $cwords_[0] ;

        if( index ( $currency_, "USDHKD" ) == 0 ){
            $HKD_TO_DOL_ = sprintf( "%.4f", 1/$cwords_[1] );
        }

      }
  }

#======================================================================================================================#



# pnl computation from the MFG trades files are simpler, given the commission amounts
open MFG_TRADES_FILE_HANDLE, "< $mfg_trades_filename_" or die "could not open mfg_trades_filename_ $mfg_trades_filename_\n";

my @mfg_trades_file_lines_ = <MFG_TRADES_FILE_HANDLE>;

close MFG_TRADES_FILE_HANDLE;

for (my $i = 0; $i <= $#mfg_trades_file_lines_; $i++) {
    my @fields_ = split (',', $mfg_trades_file_lines_[$i]);

    if ($#fields_ != 107 && $#fields_ != 106) {
#	print "Malformatted line : $mfg_trades_file_lines_[$i] No. of fields : $#fields_\n";
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

    # Handling use of same future code for multiple products.
    # E.g. FESX & EURIBOR use "SF"

    my $exch_code_ = substr ( $fields_ [ $exch_code_offset_ ] , 1 , -1 ); $exch_code_ =~ s/^\s+|\s+$//g;
    my $future_code_ = substr ($fields_[$mfg_future_code_offset_], 1, -1); $future_code_ =~ s/^\s+|\s+$//g;

    my $exch_future_code_ = $exch_code_.$future_code_;

    if (!exists ( $mfg_exch_future_code_to_shortcode_{$exch_future_code_})) {
	if ($future_code_ ne "PFC") {
#	    print "No shortcode known for mfg exch-future_code : $exch_future_code_\n";
	}

	next;
    }

    my $date_ = substr ($fields_[$date_offset_], 1, -1); $date_ =~ s/^\s+|\s+$//g;

    if ($date_ ne $expected_date_) {
#	print "MFGTrades file doesnot match date provided\n";
#	exit (1);
    }

    my $contract_date_ = substr ($fields_[$contract_yr_month_offset_], 1, -1); $contract_date_ =~ s/^\s+|\s+$//g;
    my $shortcode_ = $mfg_exch_future_code_to_shortcode_{$exch_future_code_}.$contract_date_;
    my $number_to_dollar_ = $mfg_exch_future_code_to_n2d_{$exch_future_code_};
    $shortcode_to_n2d_{$shortcode_} = $number_to_dollar_ ;

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
    my $closing_price_ = $fields_[$closing_price_offset_]; $closing_price_ =~ s/^\s+|\s+$//g;

    $shortcode_to_last_price_{$shortcode_} =  $closing_price_ ;

    my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;

    if ($buysell_ == 1) { # Buy
	$shortcode_to_pnl_{$shortcode_} -= ($trade_price_ * $quantity_ * $number_to_dollar_);
        $shortcode_to_last_pos_{$shortcode_} += $quantity_ ;  #would we used to compute flat pnl if we hold on to some positions
    } elsif ($buysell_ == 2) { # Sell
	$shortcode_to_pnl_{$shortcode_} += ($trade_price_ * $quantity_ * $number_to_dollar_);
        $shortcode_to_last_pos_{$shortcode_} -= $quantity_ ; 
    } else {
	print "Illegal buysell code : $buysell_\n";
	exit (1);
    }

    $shortcode_to_volume_{$shortcode_} += $quantity_;

    if (!exists ($shortcode_to_exchange_{$shortcode_})) {
	$shortcode_to_exchange_{$shortcode_} = $mfg_exch_future_code_to_exchange_{$exch_future_code_};
    }
}

my $shortcode_ = "";
foreach $shortcode_ (sort keys %shortcode_to_pnl_) {

    if ($shortcode_to_exchange_{$shortcode_} eq "HKEX") {
        # Values are still in canadian $, convert to dollars

        $shortcode_to_pnl_{$shortcode_} *= $HKD_TO_DOL_;

        if( $shortcode_to_last_pos_{$shortcode_} > 0) {  #open positions are closed with closing price

          $shortcode_to_pnl_{$shortcode_} += ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $HKD_TO_DOL_)  * $shortcode_to_last_pos_{$shortcode_} ) ;

        }elsif( $shortcode_to_last_pos_{$shortcode_} < 0 ){

          $shortcode_to_pnl_{$shortcode_} -= ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $HKD_TO_DOL_ ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ;

        }

    }

    $exchange_to_pnl_{$shortcode_to_exchange_{$shortcode_}} += $shortcode_to_pnl_{$shortcode_};
    $exchange_to_volume_{$shortcode_to_exchange_{$shortcode_}} += $shortcode_to_volume_{$shortcode_};
}

my $exchange_ = "";
foreach $exchange_ (sort keys %exchange_to_pnl_) {
    print "$expected_date_,$exchange_,$exchange_to_pnl_{$exchange_},$exchange_to_volume_{$exchange_}\n";
}
