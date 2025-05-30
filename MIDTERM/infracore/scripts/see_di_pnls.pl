#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/get_hft_commission_discount_for_shortcode.pl"; # Get hft discount according to volume tiers for shortcode
require "$GENPERLLIB_DIR/get_cme_commission_discount_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_cme_eu_volumes_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_number_of_working_days_BMF_between_two_dates.pl" ;

my $USAGE="$0 ors_trades_filename_";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];

my $BR_TO_DOL = 0.51;
my $today_date_ = `date +"%Y%m%d"` ;

#================================= fetching conversion rates from currency info file if available ====================#

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
	    $BR_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
        }
    }
}

#======================================================================================================================#


my $targetcol = 9;

my $ors_trades_file_base_ = basename ($ors_trades_filename_); chomp ($ors_trades_file_base_);

open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "add_results_to_local_database.pl could not open ors_trades_filename_ $ors_trades_filename_\n";

my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;

close ORS_TRADES_FILE_HANDLE;

my %symbol_to_commish_map_ = ();
my %symbol_to_noof_working_days_till_expiry_ = ();

my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();

my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();

my $temp_buy_ = 0 ;
my $temp_sell_ = 0;

for ( my $i = 0 ; $i <= $#ors_trades_file_lines_; $i ++ )
{
    my @words_ = split ( '', $ors_trades_file_lines_[$i] );
    if ( $#words_ >= 4 )
    {
	my $symbol_ = $words_[0];
	my $buysell_ = $words_[1];
	my $tsize_ = $words_[2];
	my $tprice_ = $words_[3];
	my $saos_ = $words_[4];
	
	if ( ! ( exists $symbol_to_unrealpnl_map_{$symbol_} ) )
	{
	    $symbol_to_unrealpnl_map_{$symbol_} = 0;
	    $symbol_to_pos_map_{$symbol_} = 0;
	    $symbol_to_price_map_{$symbol_} = 0;
	    $symbol_to_volume_map_{$symbol_} = 0;
	    SetSecDef ( $symbol_ ) ;
	}

	if ( $buysell_ == 0 )
	{ # buy

            $symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;

	    $symbol_to_pos_map_{$symbol_} += $tsize_;

	}
	else
	{
            $symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;

	    $symbol_to_pos_map_{$symbol_} -= $tsize_;

	}

	$symbol_to_volume_map_{$symbol_} += $tsize_;
	$symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
	$symbol_to_price_map_{$symbol_} = $tprice_;
    }
}

use Term::ANSIColor; 

my $date_ = `date`;
printf ("\n$date_\n");

my $totalpnl_ = 0.0;
foreach my $symbol_ ( sort keys %symbol_to_price_map_ )
{

    my $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_noof_working_days_till_expiry_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};

    printf "-------------------------------------------------------------------\n";
    print color("BOLD");
    printf "| %10.10s ", $symbol_;
    print color("reset");
    printf "| PNL : ";
    if ($unreal_pnl_ < 0) {
	print color("red"); print color("BOLD");
    } else {
	print color("blue"); print color("BOLD");
    }
    printf "%10.3f ", $unreal_pnl_;
    print color("reset");
    my $last_closing_price = sprintf("%.4f", $symbol_to_price_map_{$symbol_});
    printf "| POSITION : %4d | VOLUME : %5d | LPX : %s |\n", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}, , $last_closing_price;
    $totalpnl_ += $unreal_pnl_;

    $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
    $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};
}
printf "-------------------------------------------------------------------\n";

$totalpnl_ = 0.0;
my $totalvol_ = 0;

if ( exists $exchange_to_unrealpnl_map_{"BMF"} )
{
    printf "%17s |", "BMF";
    print color("BOLD");
    print color("reset");
    printf "| PNL : ";
    if ($exchange_to_unrealpnl_map_{"BMF"} < 0) {
	print color("red"); print color("BOLD");
    } else {
	print color("blue"); print color("BOLD");
    }
    printf "%10.3f ", $exchange_to_unrealpnl_map_{"BMF"};
    print color("reset");
    printf "| VOLUME : %5d |\n", $exchange_to_volume_map_{"BMF"};

    $totalpnl_ += $exchange_to_unrealpnl_map_{"BMF"};
    $totalvol_ += $exchange_to_volume_map_{"BMF"}; 
}

printf "\n%17s |", "TOTAL";
print color("BOLD");
print color("reset");
printf "| PNL : ";
if ($totalpnl_ < 0) {
    print color("red"); print color("BOLD");
} else {
    print color("blue"); print color("BOLD");
}
printf "%10.3f ", $totalpnl_;
print color("reset");
printf "| VOLUME : %5d |\n", $totalvol_;

exit ( 0 );

sub SetSecDef 
{
    my $symbol_ = shift;

    if ( index ( $symbol_, "DI" ) == 0 )
    {
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "DI" ) * $BR_TO_DOL;

	if ( index ( $symbol_ , "DI1F13" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130102 ) ;
	}
	elsif ( index ( $symbol_ , "DI1F14" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140102 ) ;
	}
	elsif ( index ( $symbol_ , "DI1F15" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150102 ) ;
	}
	elsif ( index ( $symbol_ , "DI1F16" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160102 ) ;
	}
	elsif ( index ( $symbol_ , "DI1F17" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170131 ) ;
	}
	elsif ( index ( $symbol_ , "DI1F18" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20180131 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1N13" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130731 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1N14" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140731 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1N15" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150731 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1N16" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20160731 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1N17" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170731 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1N18" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20180731 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1G13" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130228 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1G14" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140228 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1G15" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20150228 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1J13" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20130430 ) ; 
	}
	elsif ( index ( $symbol_ , "DI1J14" ) >= 0 )
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20140431 ) ; 
	}
	else
	{
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = CalNumberOfWorkingDaysBMFBetweenDates ( $today_date_, 20170131 ) ; 
	}

	$symbol_to_exchange_map_{$symbol_} = "BMF";
    }
    
}
