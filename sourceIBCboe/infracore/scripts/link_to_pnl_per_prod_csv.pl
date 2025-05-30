#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $USAGE="$0 link_invoice_filename_ YYYYMMDD";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $link_filename_ = $ARGV[0];
my $expected_date_ = $ARGV[1];

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $LAST_TRD_PRICE_SCRIPT=$HOME_DIR."/LiveExec/scripts/get_last_trade_price_for_product.sh";
my $MARGIN_TRACKER_FILE="/tmp/trades/ALL_MARGIN" ;

my %future_code_to_shortcode_ = ();
my %future_code_to_n2d_ = ();
my %future_code_to_exchange_ = ();

my %shortcode_to_pnl_ = ();
my %shortcode_to_open_position_margin_requirment_ = ();
my %shortcode_to_last_pos_ = ();
my %shortcode_to_last_price_ = ();
my %shortcode_to_n2d_ = ();
my %shortcode_to_volume_ = ();
my %shortcode_to_exchange_ = ();

$future_code_to_shortcode_{"WIN"}= "BR_WIN";
$future_code_to_n2d_{"WIN"} = 0.2;
$future_code_to_exchange_{"WIN"} = "BMF";

$future_code_to_shortcode_{"IND"}= "BR_IND";
$future_code_to_n2d_{"IND"} = 1;
$future_code_to_exchange_{"IND"} = "BMF";

$future_code_to_shortcode_{"WDO"}= "BR_WDO";
$future_code_to_n2d_{"WDO"} = 10;
$future_code_to_exchange_{"WDO"} = "BMF";

$future_code_to_shortcode_{"DOL"}= "BR_DOL";
$future_code_to_n2d_{"DOL"} = 50;
$future_code_to_exchange_{"DOL"} = "BMF";

$future_code_to_shortcode_{"DI1"}= "BR_DI";
$future_code_to_n2d_{"DI1"} = 10;
$future_code_to_exchange_{"DI1"} = "BMF";


my $buysell_offset_ = 4; # B = buy ; S = sell
my $quantity_offset_ = 6;
my $future_code_offset_ = 0;
my $contract_yr_month_offset_ = 2;
my $trade_price_offset_ = 5;
my $settlement_price_offset_ = 11 ;

# Need to figure out how many charges are placed on us.
my $comm_amount_offset_ = 16;
my $fee1_amount_offset_ = 17;
my $fee2_amount_offset_ = 18;
my $fee3_amount_offset_ = 19;

# BRTODOL
my $BR_TO_DOL_ = 0.51;

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

        if( index ( $currency_, "USDBRL" ) == 0 ){
          $BR_TO_DOL_ = sprintf( "%.4f", 1 / $cwords_[1] );
          last ;
        }

      }
  }

#======================================================================================================================#

# pnl computation from the MFG trades files are simpler, given the commission amounts
open LINK_FILE_HANDLE, "< $link_filename_" or die "could not open link_filename_ $link_filename_\n";
my @link_file_lines_ = <LINK_FILE_HANDLE>;
close LINK_FILE_HANDLE;

$/ = " ";
for (my $i = 1; $i <= $#link_file_lines_; $i++) {
    my @fields_ = split (';', $link_file_lines_[$i]);

    if ($#fields_ < 19) {
	print "Malformatted line : $link_file_lines_[$i] No. of fields : $#fields_\n";
	next;
    }

    my $future_code_ = $fields_[$future_code_offset_]; chomp ($future_code_);

    if (!exists ($future_code_to_shortcode_{$future_code_})) {
	print "No shortcode known for future_code : $future_code_\n";

	next;
    }

    my $contract_date_ = $fields_[$contract_yr_month_offset_]; chomp ($contract_date_); $contract_date_ = $contract_date_."   ";
    my $shortcode_ = $future_code_.$contract_date_;
    my $number_to_dollar_ = $future_code_to_n2d_{$future_code_};
    $shortcode_to_n2d_{$shortcode_} = $number_to_dollar_ ;

    my $comm_amount_ = $fields_[$comm_amount_offset_]; chomp ($comm_amount_);
    my $fee1_amount_ = $fields_[$fee1_amount_offset_]; chomp ($fee1_amount_);
    my $fee2_amount_ = $fields_[$fee2_amount_offset_]; chomp ($fee2_amount_);
    my $fee3_amount_ = $fields_[$fee3_amount_offset_]; chomp ($fee3_amount_);

    if (! exists $shortcode_to_pnl_{$shortcode_}) {
	$shortcode_to_pnl_{$shortcode_} = 0.0;
    }
    $shortcode_to_pnl_{$shortcode_} -= ($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_);


    # Add the trade price into the pnl
    my $buysell_ = $fields_[$buysell_offset_]; chomp ($buysell_);
    my $trade_price_ = $fields_[$trade_price_offset_]; chomp ($trade_price_);
    my $settlement_price_ = $fields_[$settlement_price_offset_]; chomp ($settlement_price_);
    my $quantity_ = $fields_[$quantity_offset_]; chomp ($quantity_);

    $shortcode_to_last_price_{$shortcode_} =  $settlement_price_ ;

    if ($buysell_ eq "B") { # Buy
	$shortcode_to_pnl_{$shortcode_} -= ($trade_price_ * $quantity_ * $number_to_dollar_);
        $shortcode_to_last_pos_{$shortcode_} += $quantity_ ;  #would we used to compute flat pnl if we hold on to some positions 
    } elsif ($buysell_ eq "S") { # Sell
	$shortcode_to_pnl_{$shortcode_} += ($trade_price_ * $quantity_ * $number_to_dollar_);
        $shortcode_to_last_pos_{$shortcode_} -= $quantity_ ;
    } else {
	print "Illegal buysell code : $buysell_\n";
	exit (1);
    }

    $shortcode_to_volume_{$shortcode_} += $quantity_;

    if (!exists ($shortcode_to_exchange_{$shortcode_})) {
	$shortcode_to_exchange_{$shortcode_} = $future_code_to_exchange_{$future_code_};
    }
}

my $shortcode_ = "";
foreach $shortcode_ (sort keys %shortcode_to_pnl_) {
    if ($shortcode_to_exchange_{$shortcode_} eq "BMF") {
	# Values are still in br, convert to dollars
	$shortcode_to_pnl_{$shortcode_} *= $BR_TO_DOL_;

        if( $shortcode_to_last_pos_{$shortcode_} > 0) {  #open positions are closed with closing price , commision is not taken into account here

          my $last_trd_price_for_shortcode_ = `$LAST_TRD_PRICE_SCRIPT $shortcode_ $shortcode_to_exchange_{$shortcode_} $expected_date_` ; 

          $shortcode_to_pnl_{$shortcode_} += ( $last_trd_price_for_shortcode_ * ( $shortcode_to_n2d_{$shortcode_} * $BR_TO_DOL_)  * $shortcode_to_last_pos_{$shortcode_} ) ;

          #if the positions are open the shortcode to last price column will show the margin value 
          $shortcode_to_open_position_margin_requirment_{ $shortcode_ } = $shortcode_to_last_price_{$shortcode_} * $BR_TO_DOL_ ;

          `echo "Date : "$expected_date_ " Product : "$shortcode_ " OPEN : +"$shortcode_to_last_pos_{$shortcode_} " MARGIN UTILIZED : "$shortcode_to_open_position_margin_requirment_{ $shortcode_ }  >>$MARGIN_TRACKER_FILE ` ;

        }elsif( $shortcode_to_last_pos_{$shortcode_} < 0 ){

          my $last_trd_price_for_shortcode_ = `$LAST_TRD_PRICE_SCRIPT $shortcode_ $shortcode_to_exchange_{$shortcode_} $expected_date_` ; 

          #if the positions are open the shortcode to last price column will show the margin value 
          $shortcode_to_open_position_margin_requirment_{ $shortcode_ } = $shortcode_to_last_price_{$shortcode_} * $BR_TO_DOL_ ;

          $shortcode_to_pnl_{$shortcode_} -= ( $last_trd_price_for_shortcode_ * ( $shortcode_to_n2d_{$shortcode_} * $BR_TO_DOL_ ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ;

          `echo "Date : "$expected_date_ " Product : "$shortcode_ " OPEN : "$shortcode_to_last_pos_{$shortcode_} " MARGIN UTILIZED : "$shortcode_to_open_position_margin_requirment_{ $shortcode_ }  >>$MARGIN_TRACKER_FILE ` ;

        }
     
    }

    print "$expected_date_,$shortcode_,$shortcode_to_pnl_{$shortcode_},$shortcode_to_volume_{$shortcode_}\n";
}
