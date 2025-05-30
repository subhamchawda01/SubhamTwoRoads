#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/calc_next_business_day.pl"; #
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; #

my $USAGE="$0 YYYYMMDD";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }


my $expected_date_ = $ARGV[0];
my $ne_file1_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMITRN_".$expected_date_.".csv" ;

my $next_date_ = CalcNextBusinessDay ( $expected_date_ ) ; #

my $ne_file2_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMITRN_".$next_date_.".csv" ;

my $prev_date_ = CalcPrevBusinessDay ( $expected_date_ ) ; #

my %ne_exch_future_code_to_shortcode_ = ();
my %ne_exch_future_code_to_fee_ = ();
my %ne_exch_future_code_to_exchange_ = ();

my %shortcode_to_pnl_ = ();
my %shortcode_to_last_pos_ = ();
my %shortcode_to_last_price_ = ();
my %shortcode_to_n2d_ = ();
my %shortcode_to_volume_ = ();
my %shortcode_to_exchange_ = ();

my %fee_discrepancy_shortcodes_ = ();

$ne_exch_future_code_to_shortcode_{"33MH"} = "MHI";
$ne_exch_future_code_to_fee_{"33MH"} = 1000 ;
$ne_exch_future_code_to_exchange_{"33MH"} = "HKEX";

$ne_exch_future_code_to_shortcode_{"33HC"} = "HHI";
$ne_exch_future_code_to_fee_{"33HC"} = 5.85 ;
$ne_exch_future_code_to_exchange_{"33HC"} = "HKEX";

$ne_exch_future_code_to_shortcode_{"33HS"} = "HSI";
$ne_exch_future_code_to_fee_{"33HS"} = 12.35 ;
$ne_exch_future_code_to_exchange_{"33HS"} = "HKEX";

# These offsets were obtained using the 'FIMST4F1.xls & FIMST4F1.csv' template files
# These will be used to determine the field being processed after splitting each trade line

my $record_id_offset_ = 0; # PRECID # Needed to figure out which records to use towards pnl computation.
my $date_offset_ = 11; # PTDATE
my $buysell_offset_ = 12; # PBS # 1 = buy ; 2 = sell
my $quantity_offset_ = 17; # PQTY
my $ne_future_code_offset_ = 24; # PFC
my $exch_code_offset_ = 23; # PEXCH
my $contract_yr_month_offset_ = 6; # PCTYM  # YYYYMM
my $trade_price_offset_ = 27; # PTPRIC # Trade price may be different than price of order
my $closing_price_offset_ = 28 ;
my $time_offset_ = 50;
my $price_multiplier_offset_ = 20;

# Need to figure out how many charges are placed on us.
my $comm_amount_offset_ = 37;
my $fee1_amount_offset_ = 38;
my $fee2_amount_offset_ = 39;
my $fee3_amount_offset_ = 40;

# EURTODOL
my $JPY_TO_DOL_ = 0.1290 ;

#================================= fetching conversion rates from currency info file if available ====================#

my $curr_date_ = $prev_date_ ;

#my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;
my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$prev_date_.'.txt' ;

if ( -e $curr_filename_ )
{
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
		$JPY_TO_DOL_ = sprintf( "%.4f", 1/$cwords_[1] );
	    }

	}
    }
}

#====================================================================================================================
# pnl computation from the NE trades files are simpler, given the commission amounts




open NE_HANDLE1_, "< $ne_file1_" or die "could not open ne_file1_ $ne_file1_\n";
my @ne_trades1_ = <NE_HANDLE1_>  ;

#print $ne_file1_ ,"\n";

my @ne_trades2_ = ( ) ;
if ( $ne_file2_ ne "NA" )
{
    open NE_HANDLE2_, "< $ne_file2_" or die "could not open ne_file2_ $ne_file2_\n";
    @ne_trades2_ = <NE_HANDLE2_> ;
    close NE_HANDLE2_;
}

#print $ne_file2_ ,"\n";
close NE_HANDLE1_;


for (my $i = 0; $i <= $#ne_trades1_; $i++) {
    my @fields_ = split (',', $ne_trades1_[$i]);

    if ($#fields_ < 115) {
	print "Malformatted line : $ne_trades1_[$i] No. of fields : $#fields_\n";
	next;
    }

    if (length ($fields_[$ne_future_code_offset_]) < 2) {
	next;
    }

    # NewEdge trade entries are listed twice in the file.
    # We only use the one specifying trade information.
    my $record_id_ = substr ($fields_[$record_id_offset_], 1, -1); $record_id_ =~ s/^\s+|\s+$//g;

    if ($record_id_ ne "T") {
	next;
    }

    my $exch_code_ = substr ( $fields_ [ $exch_code_offset_ ] , 1 , -1 ); $exch_code_ =~ s/^\s+|\s+$//g;
    my $future_code_ = substr ($fields_[$ne_future_code_offset_], 1, -1); $future_code_ =~ s/^\s+|\s+$//g;

    next if ( ! exists $ne_exch_future_code_to_shortcode_ { $exch_code_.$future_code_ } ) ;

    my $time_ = $fields_[ $time_offset_ ] ;
    $time_ =~ s/"//g ;


    if ( int ( $time_ ) > 163000 )                   # yyyymmdd_
    {
    	next;
#	print "SESSION 1 :: " ;
#	print $time_ , "\n" ;
#	print "$fields_[ $date_offset_ ] $fields_[ $quantity_offset_ ] $fields_[ $buysell_offset_ ] $fields_[ $ne_future_code_offset_ ] $fields_[ $exch_code_offset_ ] $fields_[ $contract_yr_month_offset_ ] $fields_[ $trade_price_offset_ ] $fields_[ $time_offset_ ] $fields_[ $price_multiplier_offset_ ]\n";
    }


    # Handling use of same future code for multiple products.
    # E.g. FESX & EURIBOR use "SF"

    my $exch_future_code_ = $exch_code_.$future_code_;

    if (!exists ( $ne_exch_future_code_to_shortcode_{$exch_future_code_})) {
	if ($future_code_ ne "PFC") {
#	    print "No shortcode known for ne exch-future_code : $exch_future_code_\n";
	}

	next;
    }

    my $date_ = substr ($fields_[$date_offset_], 1, -1); $date_ =~ s/^\s+|\s+$//g;

    if ($date_ ne $expected_date_) {
#	print "NETrades file doesnot match date provided\n";
#	exit (1);
    }

    my $contract_date_ = substr ($fields_[$contract_yr_month_offset_], 1, -1); $contract_date_ =~ s/^\s+|\s+$//g;
    my $shortcode_ = $ne_exch_future_code_to_shortcode_{$exch_future_code_}.$contract_date_;
    my $number_to_dollar_ = $fields_ [ $price_multiplier_offset_ ];
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

    my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;

    $shortcode_to_pnl_{$shortcode_} -= $ne_exch_future_code_to_fee_{ $exch_future_code_} * $quantity_ ;

    # Add the trade price into the pnl
    my $buysell_ = substr ($fields_[$buysell_offset_], 1, -1); $buysell_ =~ s/^\s+|\s+$//g;
    my $trade_price_ = $fields_[$trade_price_offset_]; $trade_price_ =~ s/^\s+|\s+$//g;
    my $closing_price_ = $fields_[$closing_price_offset_]; $closing_price_ =~ s/^\s+|\s+$//g;

    $shortcode_to_last_price_{$shortcode_} =  $closing_price_ ;

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
	$shortcode_to_exchange_{$shortcode_} = $ne_exch_future_code_to_exchange_{$exch_future_code_};
    }

    my $comm_fee_ = $ne_exch_future_code_to_fee_{ $exch_future_code_} ;
    my $expected_comm_fee_ = GetCommissionForShortcode( $ne_exch_future_code_to_shortcode_{$exch_future_code_} );

    if( abs($comm_fee_ - $expected_comm_fee_) > 0.1 )
    {
	if(!exists($fee_discrepancy_shortcodes_{$ne_exch_future_code_to_shortcode_{$exch_future_code_}}))
	{
	    $fee_discrepancy_shortcodes_{$ne_exch_future_code_to_shortcode_{$exch_future_code_}} = $comm_fee_;
	}
    }
}


############session 2
for (my $i = 0; $i <= $#ne_trades2_; $i++) {
    my @fields_ = split (',', $ne_trades2_[$i]);

    if ($#fields_ < 115 ) {
	print "Malformatted line : $ne_trades2_[$i] No. of fields : $#fields_\n";
	next;
    }

    if (length ($fields_[$ne_future_code_offset_]) < 2) {
	next;
    }

    # NewEdge trade entries are listed twice in the file.
    # We only use the one specifying trade information.
    my $record_id_ = substr ($fields_[$record_id_offset_], 1, -1); $record_id_ =~ s/^\s+|\s+$//g;

    if ($record_id_ ne "T") {
	next;
    }

    my $exch_code_ = substr ( $fields_ [ $exch_code_offset_ ] , 1 , -1 ); $exch_code_ =~ s/^\s+|\s+$//g;
    my $future_code_ = substr ($fields_[$ne_future_code_offset_], 1, -1); $future_code_ =~ s/^\s+|\s+$//g;

    next if ( ! exists $ne_exch_future_code_to_shortcode_ { $exch_code_.$future_code_ } ) ;

    my $time_ = $fields_[ $time_offset_ ] ;
    $time_ =~ s/"//g ;

   if ( int ( $time_  ) >= 163000 )    # yyyymmdd_
    {
#	print "$fields_[ $date_offset_ ] $fields_[ $quantity_offset_ ] $fields_[ $buysell_offset_ ] $fields_[ $ne_future_code_offset_ ] $fields_[ $exch_code_offset_ ] $fields_[ $contract_yr_month_offset_ ] $fields_[ $trade_price_offset_ ] $fields_[ $time_offset_ ] $fields_[ $price_multiplier_offset_ ]\n";
#	print "SESSION 3 :: " ;
#	print $time_ , "\n" ;
    }
    else
    {
	next ;

    }

    # Handling use of same future code for multiple products.
    # E.g. FESX & EURIBOR use "SF"


    my $exch_future_code_ = $exch_code_.$future_code_;

    if (!exists ( $ne_exch_future_code_to_shortcode_{$exch_future_code_})) {
	if ($future_code_ ne "PFC") {
#	    print "No shortcode known for ne exch-future_code : $exch_future_code_\n";
	}

	next;
    }

    my $date_ = substr ($fields_[$date_offset_], 1, -1); $date_ =~ s/^\s+|\s+$//g;

    if ($date_ ne $expected_date_) {
#	print "NETrades file doesnot match date provided\n";
#	exit (1);
    }

    my $contract_date_ = substr ($fields_[$contract_yr_month_offset_], 1, -1); $contract_date_ =~ s/^\s+|\s+$//g;
    my $shortcode_ = $ne_exch_future_code_to_shortcode_{$exch_future_code_}.$contract_date_;
    my $number_to_dollar_ = $fields_ [ $price_multiplier_offset_ ];
    $shortcode_to_n2d_{$shortcode_} = $number_to_dollar_ ;

    my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;

    my $comm_amount_ = $fields_[$comm_amount_offset_]; $comm_amount_ =~ s/^\s+|\s+$//g;
    my $fee1_amount_ = $fields_[$fee1_amount_offset_]; $fee1_amount_ =~ s/^\s+|\s+$//g;
    my $fee2_amount_ = $fields_[$fee2_amount_offset_]; $fee2_amount_ =~ s/^\s+|\s+$//g;
    my $fee3_amount_ = $fields_[$fee3_amount_offset_]; $fee3_amount_ =~ s/^\s+|\s+$//g;

    $shortcode_to_pnl_{$shortcode_} -= $ne_exch_future_code_to_fee_{ $exch_future_code_} * $quantity_ ;

    # Add the trade price into the pnl
    my $buysell_ = substr ($fields_[$buysell_offset_], 1, -1); $buysell_ =~ s/^\s+|\s+$//g;
    my $trade_price_ = $fields_[$trade_price_offset_]; $trade_price_ =~ s/^\s+|\s+$//g;
    my $closing_price_ = $fields_[$closing_price_offset_]; $closing_price_ =~ s/^\s+|\s+$//g;

    $shortcode_to_last_price_{$shortcode_} =  $closing_price_ ;

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
	$shortcode_to_exchange_{$shortcode_} = $ne_exch_future_code_to_exchange_{$exch_future_code_};
    }

    my $comm_fee_ = $ne_exch_future_code_to_fee_{ $exch_future_code_} ;
    my $expected_comm_fee_ = GetCommissionForShortcode( $ne_exch_future_code_to_shortcode_{$exch_future_code_} );

    # Factor the commissions into the pnl
    # += because the commission ammounts are already -ve
    if (! exists $shortcode_to_pnl_{$shortcode_}) {
	$shortcode_to_pnl_{$shortcode_} = 0.0;
    }

    if( abs($comm_fee_ - $expected_comm_fee_) > 0.1 )
    {
	if(!exists($fee_discrepancy_shortcodes_{$ne_exch_future_code_to_shortcode_{$exch_future_code_}}))
	{
	    $fee_discrepancy_shortcodes_{$ne_exch_future_code_to_shortcode_{$exch_future_code_}} = $comm_fee_;
	}
    }
}
########################33

my $mail_body_ = "";
foreach my $shortcode_ ( keys %fee_discrepancy_shortcodes_ )
{
    my $expected_comm_fee_ = GetCommissionForShortcode( $shortcode_ );
    $mail_body_ = $mail_body_."Discrepancy for $shortcode_: in NE file $fee_discrepancy_shortcodes_{$shortcode_}, expected: $expected_comm_fee_\n";
}


my $shortcode_ = "";
foreach $shortcode_ (sort keys %shortcode_to_pnl_) {

    if ($shortcode_to_exchange_{$shortcode_} eq "HKEX") {

	$shortcode_to_pnl_{$shortcode_} *= $JPY_TO_DOL_;

	if( $shortcode_to_last_pos_{$shortcode_} != 0) {  #open positions are closed with closing price

	    $mail_body_ = $mail_body_." ALERT :: Open Positions for $shortcode_: $shortcode_to_last_pos_{$shortcode_} \n";
	    #print $mail_body_,"\n";

	    $shortcode_to_pnl_{$shortcode_} += ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $JPY_TO_DOL_)  * $shortcode_to_last_pos_{$shortcode_} ) ;

        }elsif( $shortcode_to_last_pos_{$shortcode_} < 0 ){

          $shortcode_to_pnl_{$shortcode_} -= ( $shortcode_to_last_price_{$shortcode_} * ( $shortcode_to_n2d_{$shortcode_} * $JPY_TO_DOL_ ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ;

        }

    }


    print "$expected_date_,$shortcode_,$shortcode_to_pnl_{$shortcode_},$shortcode_to_volume_{$shortcode_}\n";
}

my $mail_ = 1 ;

if ( $mail_body_ )
{
    if ( $mail_ != 0 )
    {
	open ( MAIL , "|/usr/sbin/sendmail -t" );
	print MAIL "To: nseall@tworoads.co.in\n";
	print MAIL "Subject: open pos / comm fee discrepancy - $expected_date_\n\n";
	print MAIL $mail_body_;
	close(MAIL);
    }
    else
    {
	print $mail_body_,"\n" ;
    }
}
