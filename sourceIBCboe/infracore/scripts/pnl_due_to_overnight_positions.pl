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


my $USAGE="$0 file_yesterday file_today date_today\n";
if ( $#ARGV < 2 ) { print $USAGE; exit ( 0 ); }

my $today_date_ = `date +"%Y%m%d"` ;
my $ors_file_yesterday_ = $ARGV[0];
my $ors_file_today_ = $ARGV[1];
my $date_ = $ARGV[2];

my $EUR_TO_DOL = 1.30; # Should have a way of looking this up from the currency info file.
my $CD_TO_DOL =  0.97; # canadian dollar
my $BR_TO_DOL = 0.51; # Brazil real
my $GBP_TO_DOL = 1.57;
my $HKD_TO_DOL = 0.1289 ;
my $JPY_TO_DOL = 0.0104 ;

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
        if ( index ( $currency_, "EURUSD" ) == 0 ){
            $EUR_TO_DOL = sprintf( "%.4f", $cwords_[1] );
        }

        if( index ( $currency_, "USDCAD" ) == 0 ){
            $CD_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
        }

        if( index ( $currency_, "USDBRL" ) == 0 ){
            $BR_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
        }

        if( index ( $currency_, "GBPUSD" ) == 0 ){
            $GBP_TO_DOL = sprintf( "%.4f", $cwords_[1] );
        }

        if( index ( $currency_, "USDHKD" ) == 0 ){
            $HKD_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
        }

        if( index ( $currency_, "USDJPY" ) == 0 ){
            $JPY_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
        }


    }
}

#======================================================================================================================#

my $ors_trades_file_base_ = basename ($ors_file_yesterday_); chomp ($ors_trades_file_base_);
open ORS_TRADES_FILE_HANDLE, "< $ors_file_yesterday_" or die "Could not open ors_file_yesterday_ $ors_file_yesterday_\n";
my @ors_trades_yesterday_lines_ = <ORS_TRADES_FILE_HANDLE>;
close ORS_TRADES_FILE_HANDLE;

$ors_trades_file_base_ = basename ($ors_file_today_); chomp ($ors_trades_file_base_);
open ORS_TRADES_FILE_HANDLE, "< $ors_file_today_" or die "Could not open ors_file_today_ $ors_file_today_\n";
my @ors_trades_today_lines_ = <ORS_TRADES_FILE_HANDLE>;
close ORS_TRADES_FILE_HANDLE;

my %symbol_to_commish_map_ = ();
my %symbol_to_noof_working_days_till_expiry_ = ();
my %symbol_to_n2d_map_ = ();
my %symbol_to_symbol_name_map_ = ();

my %symbol_to_expiry_year_ = ("0" => "2010", "1" => "2011", "2" => "2012", "3" => "2013", "4" => "2014", "12"=>"2012");
my %symbol_to_expiry_month_ = ("H" => "03", "M" => "06", "U" => "09", "Z" => "12", "G" => "02");

my %liffe_symbol_to_expiry_year_ = ("12" => "2012", "13" => "2013", "14" => "2014", "15" => "2015", "16" => "2016", "17"=>"2017", "18" => "2018", "19" => "2019");
my %ose_symbol_to_expiry_year_ = ("12" => "2012", "13" => "2013", "14" => "2014", "15" => "2015", "16" => "2016", "17"=>"2017", "18" => "2018", "19" => "2019");

my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();

my %symbol_to_last_px_yesterday_ = ();
my %symbol_to_first_px_today_ = ();

my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();

$exchange_to_volume_map_{"EUREX"} = 0;
$exchange_to_volume_map_{"CME"} = 0;
$exchange_to_volume_map_{"TMX"} = 0;
$exchange_to_volume_map_{"BMF"} = 0;
$exchange_to_volume_map_{"LIFFE"} = 0;
$exchange_to_volume_map_{"HKEX"} = 0;
$exchange_to_volume_map_{"OSE"} = 0;

$exchange_to_unrealpnl_map_{"EUREX"} = 0;
$exchange_to_unrealpnl_map_{"CME"} = 0;
$exchange_to_unrealpnl_map_{"TMX"} = 0;
$exchange_to_unrealpnl_map_{"BMF"} = 0;
$exchange_to_unrealpnl_map_{"LIFFE"} = 0;
$exchange_to_unrealpnl_map_{"HKEX"} = 0;
$exchange_to_unrealpnl_map_{"OSE"} = 0;


for ( my $i = 0 ; $i <= $#ors_trades_yesterday_lines_; $i ++ )
{
    my @words_ = split ( '', $ors_trades_yesterday_lines_[$i] );
    if ( $#words_ >= 4 )
    {
	my $symbol_ = $words_[0];
	my $buysell_ = $words_[1];
	my $tsize_ = $words_[2];
	my $tprice_ = $words_[3];
	my $saos_ = $words_[4];

	if (!exists ($symbol_to_symbol_name_map_{$symbol_})) {
	    SetSecDef ($symbol_);
	}

	# Map symbol to a complete name with expiries.
	# E.g. BAXH3 => BAX201303
	$symbol_ = $symbol_to_symbol_name_map_{$symbol_};

	if ( ! ( exists $symbol_to_unrealpnl_map_{$symbol_} ) )
	{
	    $symbol_to_unrealpnl_map_{$symbol_} = 0;
	    $symbol_to_pos_map_{$symbol_} = 0;
	    $symbol_to_price_map_{$symbol_} = 0;
	    $symbol_to_volume_map_{$symbol_} = 0;
	    $symbol_to_last_px_yesterday_{$symbol_} = 0;
	}

	if ($buysell_ == 0)
	{ # buy
        $symbol_to_pos_map_{$symbol_} += $tsize_;
	}
	else
	{
        #if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {
        #    $symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
        #}else {
        #    $symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
        #}
        $symbol_to_pos_map_{$symbol_} -= $tsize_;
	}

	$symbol_to_last_px_yesterday_{$symbol_} = $tprice_;
    }
}

for ( my $i = 0 ; $i <= $#ors_trades_today_lines_; $i ++ )
{
    my @words_ = split ( '', $ors_trades_today_lines_[$i] );
    if ( $#words_ >= 4 )
    {
	my $symbol_ = $words_[0];
	my $buysell_ = $words_[1];
	my $tsize_ = $words_[2];
	my $tprice_ = $words_[3];
	my $saos_ = $words_[4];

	if (!exists ($symbol_to_symbol_name_map_{$symbol_})) {
	    SetSecDef ($symbol_);
	}

	# Map symbol to a complete name with expiries.
	# E.g. BAXH3 => BAX201303
	$symbol_ = $symbol_to_symbol_name_map_{$symbol_};

	if ( ! ( exists $symbol_to_first_px_today_{$symbol_} ) )
	{
		$symbol_to_first_px_today_{$symbol_} = $tprice_;
	}
    }
}

# Load previous open positions
my $open_position_filename_ = "/tmp/trades/OPEN_POSITIONS";
open OPEN_POSITIONS_FILE_HANDLE, "< $open_position_filename_" or die "Could not open open_position_filename_ $open_position_filename_\n";
my @open_positions_lines_ = <OPEN_POSITIONS_FILE_HANDLE>;
close OPEN_POSITIONS_FILE_HANDLE;

for ( my $i = 0; $i <= $#open_positions_lines_; $i++ )
{
	my @words_ = split ( ' ', $open_positions_lines_[$i] );

	if ( $#words_ >= 2 )
	{
		my $symbol_ = $words_[0];
		my $open_position_ = $words_[2];
		my $last_trade_price_ = $words_[8];
		$symbol_to_pos_map_{$symbol_} += $open_position_;

		if ( ! ( exists $symbol_to_last_px_yesterday_{$symbol_} ) )
		{
			$symbol_to_last_px_yesterday_{$symbol_} = $last_trade_price_;
		}
	}
}

foreach my $symbol_ ( sort keys %symbol_to_pos_map_ )
{
	if ( $symbol_to_pos_map_{$symbol_} != 0 )
	{
		if ( ! ( exists $symbol_to_first_px_today_{$symbol_} ) )
		{
			$symbol_to_first_px_today_{$symbol_} = 0.0;
			$symbol_to_unrealpnl_map_{$symbol_} = 0.0;
		}
		else
		{
            if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {
		        $symbol_to_unrealpnl_map_{$symbol_} = $symbol_to_pos_map_{$symbol_} * ( 100000 / ( $symbol_to_first_px_today_{$symbol_} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL -
		                                              $symbol_to_pos_map_{$symbol_} * ( 100000 / ( $symbol_to_last_px_yesterday_{$symbol_} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL;
	        }else {
				$symbol_to_unrealpnl_map_{$symbol_} = $symbol_to_pos_map_{$symbol_} * ( $symbol_to_first_px_today_{$symbol_} - $symbol_to_last_px_yesterday_{$symbol_} ) * $symbol_to_n2d_map_{$symbol_};
	        }
		}

		print $symbol_ . " Position: " . $symbol_to_pos_map_{$symbol_} . "\tPnl: " . $symbol_to_unrealpnl_map_{$symbol_}
		      . "\tpx_today: " . $symbol_to_first_px_today_{$symbol_} . "\tprev_px: " . $symbol_to_last_px_yesterday_{$symbol_} ."\n";
	}
}

exit (0);

sub SetSecDef
{
    my $symbol_ = shift;
    if ( index ( $symbol_, "ES" ) == 0 )
    {
	my $base_symbol_length_ = length ("ES");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "ES" );
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 0.50;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
    }
    elsif ( index ( $symbol_, "NIY" ) == 0 )
    {
	my $base_symbol_length_ = length ("NIY");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "NIY" );
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 500 * $JPY_TO_DOL ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
    }
    elsif ( index ( $symbol_, "NKD" ) == 0 )
    {
	my $base_symbol_length_ = length ("NKD");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "NKD" );
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 5 ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
    }
    elsif ( index ( $symbol_, "UB" ) == 0 )
    {
	my $base_symbol_length_ = length ("UB");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "UB" );
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
    }
    elsif ( index ( $symbol_, "ZB" ) == 0 )
    {
	my $base_symbol_length_ = length ("ZB");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "ZB" );
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
    }
    elsif ( index ( $symbol_, "ZN" ) == 0 )
    {
	my $base_symbol_length_ = length ("ZN");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "ZN" );
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
    }
    elsif ( index ( $symbol_, "ZF" ) == 0 )
    {
	my $base_symbol_length_ = length ("ZF");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "ZF" );
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
    }
    elsif ( index ( $symbol_, "ZT" ) == 0 )
    {
	my $base_symbol_length_ = length ("ZT");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "ZT" );
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 2000;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "CME";
    }
    elsif ( index ( $symbol_, "FGBS" ) == 0 )
    {
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FGBS" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "EUREX";

	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    }
    elsif ( index ( $symbol_, "FGBM" ) == 0 )
    {
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FGBM" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "EUREX";

	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    }
    elsif ( index ( $symbol_, "FGBL" ) == 0 )
    {
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FGBL" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "EUREX";

	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    }
    elsif ( index ( $symbol_, "FGBX" ) == 0 )
    {
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FGBX" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "EUREX";

	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    }
    elsif ( index ( $symbol_, "FESX" ) == 0 )
    {
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FESX" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 10 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "EUREX";

	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    }
    elsif ( index ( $symbol_, "FOAT" ) == 0 )
    {
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FOAT" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "EUREX";

	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    }
    elsif ( index ( $symbol_, "FDAX" ) == 0 )
    {
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FDAX" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 25 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "EUREX";

	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    }
    elsif ( index ( $symbol_, "FBTP" ) == 0 )
    {
	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FBTP" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "EUREX";

	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    }
    elsif ( index ( $symbol_, "FBTS" ) == 0 )
    {
        $symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "FBTS" ) * $EUR_TO_DOL;
        $symbol_to_n2d_map_{$symbol_} = 1000 * $EUR_TO_DOL;
        $symbol_to_exchange_map_{$symbol_} = "EUREX";

        $symbol_to_symbol_name_map_{$symbol_} = $symbol_;
    }

    elsif ( index ( $symbol_, "JFFCE" ) == 0 )
    {
	my $base_symbol_length_ = length ("JFFCE");
	my $expiry_year_offset_ = $base_symbol_length_ ;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "JFFCE" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

    }
    elsif ( index ( $symbol_, "KFFTI" ) == 0 )
    {
	my $base_symbol_length_ = length ("KFFTI");
	my $expiry_year_offset_ = $base_symbol_length_ ;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "KFFTI" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 200 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

    }
    elsif ( index ( $symbol_, "LFZ" ) == 0 )
    {
	my $base_symbol_length_ = length ("LFZ");
	my $expiry_year_offset_ = $base_symbol_length_ + 2;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "LFZ" ) * $GBP_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10 * $GBP_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

    }
    elsif ( index ( $symbol_, "LFL" ) == 0 )
    {
	my $base_symbol_length_ = length ("LFL");
	my $expiry_year_offset_ = $base_symbol_length_ + 2;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "LFL" ) * $GBP_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1250 * $GBP_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

    }
    elsif ( index ( $symbol_, "LFR" ) == 0 )
    {

	my $base_symbol_length_ = length ("LFR");
	my $expiry_year_offset_ = $base_symbol_length_ + 2;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "LFR" ) * $GBP_TO_DOL ;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000 * $GBP_TO_DOL ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

    }
    elsif ( index ( $symbol_, "YFEBM" ) == 0 )
    {

	my $base_symbol_length_ = length ("YFEBM");
	my $expiry_year_offset_ = $base_symbol_length_ ;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "YFEBM" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 50 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

    }
    elsif ( index ( $symbol_, "XFC" ) == 0 )
    {
	my $base_symbol_length_ = length ("XFC");
	my $expiry_year_offset_ = $base_symbol_length_ + 2;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "XFC" ) * $GBP_TO_DOL ;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10 * $GBP_TO_DOL ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

    }
    elsif ( index ( $symbol_, "XFRC" ) == 0 )
    {
	my $base_symbol_length_ = length ("XFRC");
	my $expiry_year_offset_ = $base_symbol_length_ + 1;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "XFRC" ) * $GBP_TO_DOL ;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10 ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

    }
    elsif ( index ( $symbol_, "LFI" ) == 0 )
    {
	my $base_symbol_length_ = length ("LFI");
	my $expiry_year_offset_ = $base_symbol_length_ + 2 ;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "LFI" ) * $EUR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 2500 * $EUR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "LIFFE" ;

    }
    elsif ( index ( $symbol_, "CGB" ) == 0 )
    {
	my $base_symbol_length_ = length ("CGB");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "CGB" ) * $CD_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000 * $CD_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "TMX";
    }
    elsif ( index ( $symbol_, "CGF" ) == 0 )
    {
	my $base_symbol_length_ = length ("CGF");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "CGF" ) * $CD_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000 * $CD_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "TMX";
    }
    elsif ( index ( $symbol_, "CGZ" ) == 0 )
    {
	my $base_symbol_length_ = length ("CGZ");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "CGZ" ) * $CD_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000 * $CD_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "TMX";
    }
    elsif ( index ( $symbol_, "BAX" ) == 0 )
    {
	my $base_symbol_length_ = length ("BAX");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "BAX" ) * $CD_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 2500 * $CD_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "TMX";
    }
    elsif ( index ( $symbol_, "SXF" ) == 0 )
    {
	my $base_symbol_length_ = length ("SXF");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "SXF" ) * $CD_TO_DOL;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 200 * $CD_TO_DOL;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "TMX";
    }
    elsif ( index ( $symbol_, "WIN" ) == 0 )
    {
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;

	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "WIN" ) * $BR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 0.2 * $BR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "BMF";
    }
    elsif ( index ( $symbol_, "IND" ) == 0 )
    {
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;

	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "IND" ) * $BR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 1.0 * $BR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "BMF";
    }
    elsif ( index ( $symbol_, "DOL" ) == 0 )
    {
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;

	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "DOL" ) * $BR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 50.0 * $BR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "BMF";
    }
    elsif ( index ( $symbol_, "WDO" ) == 0 )
    {
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_;

	$symbol_to_commish_map_{$symbol_} = GetCommissionForShortcode( "WDO" )  * $BR_TO_DOL;
	$symbol_to_n2d_map_{$symbol_} = 10.0 * $BR_TO_DOL;
	$symbol_to_exchange_map_{$symbol_} = "BMF";
    }
    elsif ( index ( $symbol_, "DI" ) == 0 )
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
        $symbol_to_symbol_name_map_{$symbol_} = $symbol_ ;
    }
    elsif ( index ( $symbol_, "HSI" ) == 0 )
    {

	my $base_symbol_length_ = length ("HSI");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "HSI" ) * $HKD_TO_DOL ;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 50.0 * $HKD_TO_DOL ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "HKEX";

    }
    elsif ( index ( $symbol_, "HHI" ) == 0 )
    {

	my $base_symbol_length_ = length ("HHI");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "HHI" ) * $HKD_TO_DOL ;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 50.0 * $HKD_TO_DOL ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "HKEX";

    }
    elsif ( index ( $symbol_, "MHI" ) == 0 )
    {
	my $base_symbol_length_ = length ("MHI");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "MHI" ) * $HKD_TO_DOL ;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10.0 * $HKD_TO_DOL ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "HKEX";

    }
    elsif ( index ( $symbol_, "MCH" ) == 0 )
    {
	my $base_symbol_length_ = length ("MCH");
	my $expiry_month_offset_ = $base_symbol_length_;
	my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "MCH" ) * $HKD_TO_DOL ;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10.0 * $HKD_TO_DOL ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "HKEX";
    }
    elsif ( index ( $symbol_, "NKM" ) == 0 )
    {

	my $base_symbol_length_ = length ("NKM");
	my $expiry_year_offset_ = $base_symbol_length_;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2 ;

#	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$ose_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2)}.substr ( $symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "NKM" ) * $JPY_TO_DOL ;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 100 * $JPY_TO_DOL ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "OSE";

    }
    elsif ( index ( $symbol_, "JGBL" ) == 0 )
    {
	my $base_symbol_length_ = length ("JGBL");
	my $expiry_year_offset_ = $base_symbol_length_;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2 ;

#	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$ose_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2)}.substr ( $symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "JGBL" ) * $JPY_TO_DOL ;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10000 * $JPY_TO_DOL ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "OSE";

    }
    elsif ( index ( $symbol_, "TOPIX" ) == 0 )
    {
	my $base_symbol_length_ = length ("TOPIX");
	my $expiry_year_offset_ = $base_symbol_length_;
	my $expiry_month_offset_ = $expiry_year_offset_ + 2 ;

#	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$liffe_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2 )}.substr ($symbol_, $expiry_month_offset_, 2) ;

	my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$ose_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_, 2)}.substr ( $symbol_, $expiry_month_offset_, 2) ;
	$symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	$symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "TOPIX" ) * $JPY_TO_DOL ;
	$symbol_to_n2d_map_{$symbol_name_with_expiry_} = 100 * $JPY_TO_DOL ;
	$symbol_to_exchange_map_{$symbol_name_with_expiry_} = "OSE";

    }
    else {

      if  ( index ( $symbol_, "NK" ) == 0 )
      {
	  my $base_symbol_length_ = length ("NK");
	  my $expiry_year_offset_ = $base_symbol_length_;
	  my $expiry_month_offset_ = $expiry_year_offset_ + 2;

	  my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$ose_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_,2)}.substr ($symbol_, $expiry_month_offset_, 2);
	  $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;

	  $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "NK" ) * $JPY_TO_DOL ;
	  $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 1000 * $JPY_TO_DOL ;
	  $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "OSE";

      }

    }

}
