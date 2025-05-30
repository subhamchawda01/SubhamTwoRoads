#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $USAGE="$0 alpes_filename_ link_invoice_filename_ YYYYMMDD";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $alpes_filename_= $ARGV[0];
my $link_filename_= $ARGV[1];  #remains unused 
my $expected_date_ = $ARGV[2];

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/get_number_of_working_days_BMF_between_two_dates.pl" ;
require "$GENPERLLIB_DIR/get_commission_for_DI.pl";
my $today_date_ = `date +"%Y%m%d"` ;

my $LAST_TRD_PRICE_SCRIPT=$HOME_DIR."/LiveExec/scripts/get_last_trade_price_for_product.sh";
my $MARGIN_TRACKER_FILE="/tmp/trades/ALL_MARGIN" ;

my %future_code_to_shortcode_ = ();
my %future_code_to_n2d_ = ();
my %symbol_to_noof_working_days_till_expiry_ = ();
my %exchsymbol_to_noof_working_days_till_expiry_ = ();
my %future_code_to_exchange_ = ();

my %shortcode_to_pnl_ = ();
my %shortcode_to_open_position_margin_requirment_ = ();
my %shortcode_to_last_pos_ = ();
my %shortcode_to_last_price_ = ();
my %shortcode_to_n2d_ = ();
my %shortcode_to_volume_ = ();
my %shortcode_to_exchange_ = ();
my %short_name_to_symbol_map_ = ();
my %fee_discrepancy_shortcodes_ = ();
my %symbol_to_commish_map_ = ();
my %mfg_symbol_to_commish_map_ = ();

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

#check if DI needs future code handling 
$future_code_to_shortcode_{"DI1"}= "BR_DI";
$future_code_to_exchange_{"DI1"} = "BMF";





my $buysell_offset_ = 4; # B = buy ; S = sell
my $quantity_offset_ = 5;
my $future_code_offset_ = 0;
my $contract_yr_month_offset_ = 2;
my $trade_price_offset_ = 10 ;

##This fiels is not utilized for alpes 
my $settlement_price_offset_ = 10 ; 

# Need to figure out how many charges are placed on us.
my $comm_amount_offset_ = 14;
my $fee1_amount_offset_ = 15;
my $fee2_amount_offset_ = 16;


# BRTODOL
my $BR_TO_DOL_ = 0.4907;

#================================= fetching conversion rates from currency info file if available ====================#

my $curr_date_ = $expected_date_ ;

my $yesterday_file_ = "/tmp/YESTERDAY_DATE" ;
open YESTERDAY_FILE_HANDLE, "< $yesterday_file_" ;

my @yesterday_lines_ = <YESTERDAY_FILE_HANDLE> ;
close YESTERDAY_FILE_HANDLE ;

my @ywords_ = split(' ', $yesterday_lines_[0] );
my $yesterday_date_ = $ywords_[0];

my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$yesterday_date_.'.txt' ;

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
open ALPES_FILE_HANDLE, "< $alpes_filename_" or die "could not open alpes_filename_ $alpes_filename_\n";
my @alpes_file_lines_ = <ALPES_FILE_HANDLE>; chomp ( @alpes_file_lines_ );
close ALPES_FILE_HANDLE;

# $/ = " ";
for (my $i = 1; $i <= $#alpes_file_lines_; $i++) {
    my @fields_ = split ( ";", $alpes_file_lines_[$i]);

    if ($#fields_ < 17) {
        print "Malformatted line : $alpes_file_lines_[$i] No. of fields : $#fields_\n";
        next;
    }

    my $future_code_ = $fields_[$future_code_offset_]; chomp ($future_code_);

    if (!exists ($future_code_to_shortcode_{$future_code_})) {
        print "No shortcode known for future_code : $future_code_\n";

        next;
    }

    my $contract_date_ = $fields_[$contract_yr_month_offset_]; chomp ($contract_date_); $contract_date_ = $contract_date_."   ";
    my $shortcode_ = $future_code_.$contract_date_; chomp ($shortcode_) ;
    my $number_to_dollar_ = 0 ; 

    if ( !exists ( $symbol_to_noof_working_days_till_expiry_ { $shortcode_ } ) ) {

        if ( index ( $shortcode_ , "DI1F13" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130102 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1F14" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140102 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1F15" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150102 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1F16" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160102 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1F17" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170131 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1F18" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20180131 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1N13" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130731 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1N14" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140731 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1N15" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150731 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1N16" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160731 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1N17" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170731 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1N18" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20180731 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1G13" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130228 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1G14" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140228 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1G15" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150228 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1J13" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130430 ) ;
        }
        elsif ( index ( $shortcode_ , "DI1J14" ) >= 0 )
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140431 ) ;
        }
        else
        {
            $symbol_to_noof_working_days_till_expiry_{$shortcode_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170131 ) ;
        }

    }

    if ( index ( $future_code_, "DI1" ) == 0 ) {

	$exchsymbol_to_noof_working_days_till_expiry_{ $shortcode_ } = $symbol_to_noof_working_days_till_expiry_{$shortcode_} ;
	$number_to_dollar_ = 0 ;

    }else{

	$number_to_dollar_ = $future_code_to_n2d_{$future_code_}; 

    }

    $shortcode_to_n2d_{$shortcode_} = $number_to_dollar_ ;

    my $comm_amount_ = $fields_[$comm_amount_offset_]; chomp ($comm_amount_);
    my $fee1_amount_ = $fields_[$fee1_amount_offset_]; chomp ($fee1_amount_);
    my $fee2_amount_ = $fields_[$fee2_amount_offset_]; chomp ($fee2_amount_);
    my $fee3_amount_ = 0 ;

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

        if ( ( index ( $shortcode_, "DI" ) >= 0 ) ) {

	    $shortcode_to_pnl_{$shortcode_} += $quantity_ * ( 100000 / ( $trade_price_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$shortcode_} / 252 ) ) ;

        }else{

	    $shortcode_to_pnl_{$shortcode_} -= ($trade_price_ * $quantity_ * $number_to_dollar_);

        }

	
        $shortcode_to_last_pos_{$shortcode_} += $quantity_ ;  #would we used to compute flat pnl if we hold on to some positions 
    } elsif ($buysell_ eq "S") { # Sell

        if ( ( index ( $shortcode_, "DI" ) >= 0 ) ) {

	    $shortcode_to_pnl_{$shortcode_} -= $quantity_ * ( 100000 / ( $trade_price_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$shortcode_} / 252 ) ) ;

        }else{

	    $shortcode_to_pnl_{$shortcode_} += ($trade_price_ * $quantity_ * $number_to_dollar_);

        }

        $shortcode_to_last_pos_{$shortcode_} -= $quantity_ ;

    } else {
        print "Illegal buysell code : $buysell_\n";
        exit (1);
    }
    
    my $comm_fee_ = ($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_) / $quantity_ ;
    if ( !exists ( $mfg_symbol_to_commish_map_{$shortcode_}))
    {
           $mfg_symbol_to_commish_map_{$shortcode_} = $comm_fee_;
    }
    else
    {
        if (abs($comm_fee_ -  $mfg_symbol_to_commish_map_{$shortcode_}) > 0.1 )
        {
            open ( MAIL , "|/usr/sbin/sendmail -t" );
            print MAIL "To: vedant\@tworoads.co.in\n";
            print MAIL "Subject: diff comm for same trade - $expected_date_\n\n";
            print MAIL "$shortcode_ : $comm_fee_ and  $mfg_symbol_to_commish_map_{$shortcode_}";
            close(MAIL);
        }
    }
   
    $shortcode_to_volume_{$shortcode_} += $quantity_;

    if (!exists ($shortcode_to_exchange_{$shortcode_})) {
        $shortcode_to_exchange_{$shortcode_} = $future_code_to_exchange_{$future_code_};
    }
}

# my $shortcode_ = ""

foreach my $shortcode_ (sort keys %shortcode_to_pnl_) {
if ($shortcode_to_exchange_{$shortcode_} eq "BMF") {

    if ( index ( $shortcode_, "DOL" ) >= 0 ) {
        $short_name_to_symbol_map_{"DOL"} = $shortcode_;
    }
     if ( index ( $shortcode_, "WDO" ) >= 0 ) {
        $short_name_to_symbol_map_{"WDO"} = $shortcode_;
    }
     if ( index ( $shortcode_, "IND" ) >= 0 ) {
        $short_name_to_symbol_map_{"IND"} = $shortcode_;
    }
     if ( index ( $shortcode_, "WIN" ) >= 0 ) {
        $short_name_to_symbol_map_{"WIN"} = $shortcode_;
    }
}
}

my $mail_body_ = "";
foreach my $shortcode_ (sort keys %shortcode_to_pnl_) {
if ($shortcode_to_exchange_{$shortcode_} eq "BMF") {

    if ( index ( $shortcode_, "DI1" ) >= 0 ) {
      $symbol_to_commish_map_{$shortcode_} = GetCommissionForDI($shortcode_);
    }
    if ( index ( $shortcode_, "DOL" ) == 0 ) {
      my $wdo_volume_ = $shortcode_to_volume_{$short_name_to_symbol_map_{"WDO"}};
      $symbol_to_commish_map_{$shortcode_} = GetCommissionForShortcode( "DOL" , $shortcode_to_volume_{$shortcode_},$wdo_volume_) ;      
    }
    if ( index ( $shortcode_, "WDO" ) == 0 ) {
      
      my $dol_volume_ = $shortcode_to_volume_{$short_name_to_symbol_map_{"DOL"}};
      $symbol_to_commish_map_{$shortcode_} = GetCommissionForShortcode( "WDO" , $shortcode_to_volume_{$shortcode_},$dol_volume_) ;
    }
    if ( index ( $shortcode_, "WIN" ) == 0 ) {
      
      my $ind_volume_ = $shortcode_to_volume_{$short_name_to_symbol_map_{"IND"}};
     $symbol_to_commish_map_{$shortcode_} = GetCommissionForShortcode( "WIN" , $shortcode_to_volume_{$shortcode_},$ind_volume_) ;
    }
    if ( index ( $shortcode_, "IND" ) == 0 ) {
     
      my $win_volume_ = $shortcode_to_volume_{$short_name_to_symbol_map_{"WIN"}};
     $symbol_to_commish_map_{$shortcode_} = GetCommissionForShortcode( "IND" , $shortcode_to_volume_{$shortcode_}, $win_volume_) ;
    }

    if (abs($symbol_to_commish_map_{$shortcode_} - $mfg_symbol_to_commish_map_{$shortcode_}) > 0.1 )
    {
        $mail_body_ = $mail_body_."Discrepancy for $shortcode_: in MFG file $mfg_symbol_to_commish_map_{$shortcode_}, expected: $symbol_to_commish_map_{$shortcode_} \n";
    }

    }
}

if( $mail_body_ )
{
     open ( MAIL , "|/usr/sbin/sendmail -t" );
     print MAIL "To: nseall@tworoads.co.in\n";
     print MAIL "Subject: comm fee discrepancy - $expected_date_\n\n";
     print MAIL $mail_body_;
     close(MAIL);
}
#$buysell_offset_ = 4; # B = buy ; S = sell
#$quantity_offset_ = 6;
#$future_code_offset_ = 0;
#$contract_yr_month_offset_ = 2;
#$trade_price_offset_ = 5;
#$settlement_price_offset_ = 11 ;

## Need to figure out how many charges are placed on us.
#$comm_amount_offset_ = 16;
#$fee1_amount_offset_ = 17;
#$fee2_amount_offset_ = 18;
#my $fee3_amount_offset_ = 19;

## pnl computation from the MFG trades files are simpler, given the commission amounts
#open LINK_FILE_HANDLE, "< $link_filename_" or die "could not open link_filename_ $link_filename_\n";
#my @link_file_lines_ = <LINK_FILE_HANDLE>; chomp ( @link_file_lines_ );
#close LINK_FILE_HANDLE;

## $/ = " ";
#for (my $i = 1; $i <= $#link_file_lines_; $i++) {
#my @fields_ = split ( ";", $link_file_lines_[$i]);

#if ($#fields_ < 19) {
#print "Malformatted line : $link_file_lines_[$i] No. of fields : $#fields_\n";
#next;
#}

#my $future_code_ = $fields_[$future_code_offset_]; chomp ($future_code_);

#if (!exists ($future_code_to_shortcode_{$future_code_})) {
#print "No shortcode known for future_code : $future_code_\n";

#next;
#}

#my $contract_date_ = $fields_[$contract_yr_month_offset_]; chomp ($contract_date_); $contract_date_ = $contract_date_."   ";

#if ( index ( $contract_date_ , "F14" ) >= 0 ){

#$future_code_to_n2d_{"DI1"} = 1150 ;

#}elsif( index ( $contract_date_ , "F13" ) >= 0 ){

#$future_code_to_n2d_{"DI1"} = 393 ;

#}elsif( index ( $contract_date_ , "F15" ) >= 0 ){

#$future_code_to_n2d_{"DI1"} = 1800 ;

#}elsif( index ( $contract_date_ , "F17" ) >= 0 ){

#$future_code_to_n2d_{"DI1"} = 2790 ;

#}elsif( index ( $contract_date_ , "F16" ) >= 0 ){

#$future_code_to_n2d_{"DI1"} = 2300 ;

#}elsif( index ( $contract_date_ , "N13" ) >= 0 ){

#$future_code_to_n2d_{"DI1"} = 700;

#}
#else{

#$future_code_to_n2d_{"DI1"} = 100 ;

#}

#my $shortcode_ = $future_code_.$contract_date_;
#my $number_to_dollar_ = $future_code_to_n2d_{$future_code_};
#$shortcode_to_n2d_{$shortcode_} = $number_to_dollar_ ;

#my $comm_amount_ = $fields_[$comm_amount_offset_]; chomp ($comm_amount_);
#my $fee1_amount_ = $fields_[$fee1_amount_offset_]; chomp ($fee1_amount_);
#my $fee2_amount_ = $fields_[$fee2_amount_offset_]; chomp ($fee2_amount_);
#my $fee3_amount_ = $fields_[$fee3_amount_offset_]; chomp ($fee3_amount_);

#if (! exists $shortcode_to_pnl_{$shortcode_}) {
#$shortcode_to_pnl_{$shortcode_} = 0.0;
#}
#$shortcode_to_pnl_{$shortcode_} -= ($comm_amount_ + $fee1_amount_ + $fee2_amount_ + $fee3_amount_);


## Add the trade price into the pnl
#my $buysell_ = $fields_[$buysell_offset_]; chomp ($buysell_);
#my $trade_price_ = $fields_[$trade_price_offset_]; chomp ($trade_price_);
#my $settlement_price_ = $fields_[$settlement_price_offset_]; chomp ($settlement_price_);
#my $quantity_ = $fields_[$quantity_offset_]; chomp ($quantity_);

#$shortcode_to_last_price_{$shortcode_} =  $settlement_price_ ;

#if ($buysell_ eq "B") { # Buy
#$shortcode_to_pnl_{$shortcode_} -= ($trade_price_ * $quantity_ * $number_to_dollar_);
#$shortcode_to_last_pos_{$shortcode_} += $quantity_ ;  #would we used to compute flat pnl if we hold on to some positions 
#} elsif ($buysell_ eq "S") { # Sell
#$shortcode_to_pnl_{$shortcode_} += ($trade_price_ * $quantity_ * $number_to_dollar_);
#$shortcode_to_last_pos_{$shortcode_} -= $quantity_ ;
#} else {
#print "Illegal buysell code : $buysell_\n";
#exit (1);
#}

#$shortcode_to_volume_{$shortcode_} += $quantity_;

#if (!exists ($shortcode_to_exchange_{$shortcode_})) {
#$shortcode_to_exchange_{$shortcode_} = $future_code_to_exchange_{$future_code_};
#}
#}

# my $shortcode_ = "";
foreach my $shortcode_ (sort keys %shortcode_to_pnl_) {
    if ($shortcode_to_exchange_{$shortcode_} eq "BMF") {
        # Values are still in br, convert to dollars
        $shortcode_to_pnl_{$shortcode_} *= $BR_TO_DOL_;

        if( $shortcode_to_last_pos_{$shortcode_} > 0) {  #open positions are closed with closing price , commision is not taken into account here

            my $temp_shortcode_ = $shortcode_ ;
            $temp_shortcode_ =~ s/^\s+|\s+$//g ;
            $temp_shortcode_ = $temp_shortcode_."_";
	
            my $last_trd_price_for_shortcode_ = `$LAST_TRD_PRICE_SCRIPT $temp_shortcode_ $shortcode_to_exchange_{$shortcode_} $expected_date_` ;

	    if ( index ( $shortcode_, "DI1" ) >= 0 ) {

		$shortcode_to_pnl_{$shortcode_} -= $shortcode_to_last_pos_{$shortcode_} * ( 100000 / ( $last_trd_price_for_shortcode_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$shortcode_} / 252 ) ) * $BR_TO_DOL_ ; 

	    }else{

		$shortcode_to_pnl_{$shortcode_} += ( $last_trd_price_for_shortcode_ * ( $shortcode_to_n2d_{$shortcode_} * $BR_TO_DOL_)  * $shortcode_to_last_pos_{$shortcode_} ) ;

	    }


	    #if the positions are open the shortcode to last price column will show the margin value 
	    $shortcode_to_open_position_margin_requirment_{ $shortcode_ } = $shortcode_to_last_price_{$shortcode_} * $BR_TO_DOL_ ;

	    `echo "Date : "$expected_date_ " Product : "$shortcode_ " OPEN : +"$shortcode_to_last_pos_{$shortcode_} " MARGIN UTILIZED : "$shortcode_to_open_position_margin_requirment_{ $shortcode_ }  >>$MARGIN_TRACKER_FILE ` ;

        }elsif( $shortcode_to_last_pos_{$shortcode_} < 0 ){

            my $temp_shortcode_ = $shortcode_ ;
            $temp_shortcode_ =~ s/^\s+|\s+$//g ; 
            $temp_shortcode_ = $temp_shortcode_."_";

	    my $last_trd_price_for_shortcode_ = `$LAST_TRD_PRICE_SCRIPT $temp_shortcode_ $shortcode_to_exchange_{$shortcode_} $expected_date_` ;
	    
	    if ( index ( $shortcode_, "DI1" ) >= 0 ) {

		$shortcode_to_pnl_{$shortcode_} += abs($shortcode_to_last_pos_{$shortcode_}) * ( 100000 / ( $last_trd_price_for_shortcode_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$shortcode_} / 252 ) ) * $BR_TO_DOL_ ; 

	    }else{

		$shortcode_to_pnl_{$shortcode_} -= ( $last_trd_price_for_shortcode_ * ( $shortcode_to_n2d_{$shortcode_} * $BR_TO_DOL_ ) * abs($shortcode_to_last_pos_{$shortcode_}) ) ;
		
	    }


	    #if the positions are open the shortcode to last price column will show the margin value 
	    $shortcode_to_open_position_margin_requirment_{ $shortcode_ } = $shortcode_to_last_price_{$shortcode_} * $BR_TO_DOL_ ;


	    `echo "Date : "$expected_date_ " Product : "$shortcode_ " OPEN : "$shortcode_to_last_pos_{$shortcode_} " MARGIN UTILIZED : "$shortcode_to_open_position_margin_requirment_{ $shortcode_ }  >>$MARGIN_TRACKER_FILE ` ;

        }

    }

    print "$expected_date_,$shortcode_,$shortcode_to_pnl_{$shortcode_},$shortcode_to_volume_{$shortcode_}\n";
}

