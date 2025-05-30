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
require "$GENPERLLIB_DIR/calc_next_business_day.pl"; # 
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; # 


my $USAGE="$0 ors_trades_filename_ date";
if ( $#ARGV < 1 ) { print $USAGE; exit ( 0 ); }
my $ors_trades_filename_ = $ARGV[0];
my $date_ = $ARGV[1];

my $JPY_TO_DOL = 0.0104 ;

my $currency_date_ = CalcPrevBusinessDay ( $date_ ) ; # 

#================================= fetching conversion rates from currency info file if available ====================#

my $yesterday_file_ = "/tmp/YESTERDAY_DATE" ;
open YESTERDAY_FILE_HANDLE, "< $yesterday_file_" ;

my @yesterday_lines_ = <YESTERDAY_FILE_HANDLE> ;
close YESTERDAY_FILE_HANDLE ;

my @ywords_ = split(' ', $yesterday_lines_[0] );
my $yesterday_date_ = $currency_date_ ;

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
 
        if( index ( $currency_, "USDJPY" ) == 0 ){
            $JPY_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
        }

    }
}

#======================================================================================================================#


my $ors_trades_file_base_ = basename ($ors_trades_filename_); chomp ($ors_trades_file_base_);

open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "Could not open ors_trades_filename_ $ors_trades_filename_\n";
my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;
close ORS_TRADES_FILE_HANDLE;

my %symbol_to_commish_map_ = ();
my %symbol_to_noof_working_days_till_expiry_ = ();
my %symbol_to_n2d_map_ = ();
my %symbol_to_symbol_name_map_ = ();

my %symbol_to_expiry_year_ = ("0" => "2010", "1" => "2011", "2" => "2012", "3" => "2013", "4" => "2014", "12"=>"2012", "5" => "2015", "6" => "2016", "7" => "2017", "8" => "2018", "9" => "2019");
my %symbol_to_expiry_month_ = ("H" => "03", "M" => "06", "U" => "09", "Z" => "12", "G" => "02");

my %liffe_symbol_to_expiry_year_ = ("12" => "2012", "13" => "2013", "14" => "2014", "15" => "2015", "16" => "2016", "17"=>"2017", "18" => "2018", "19" => "2019");
my %ose_symbol_to_expiry_year_ = ("12" => "2012", "13" => "2013", "14" => "2014", "15" => "2015", "16" => "2016", "17"=>"2017", "18" => "2018", "19" => "2019");

my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();

my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();

$exchange_to_volume_map_{"OSE"} = 0;

$exchange_to_unrealpnl_map_{"OSE"} = 0;

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
	}

	if ($buysell_ == 0)
	{ # buy


            $symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
            $symbol_to_pos_map_{$symbol_} += $tsize_;

	}
	else
	{
            $symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
            $symbol_to_pos_map_{$symbol_} -= $tsize_;

	}

	$symbol_to_volume_map_{$symbol_} += $tsize_;
	$symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
	$symbol_to_price_map_{$symbol_} = $tprice_;
    }
}

foreach my $symbol_ ( sort keys %symbol_to_price_map_ ) {
    
    my $unreal_pnl_ = 0 ;

    $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($symbol_to_pos_map_{$symbol_}) * $symbol_to_commish_map_{$symbol_};

    $symbol_to_unrealpnl_map_{$symbol_} = $unreal_pnl_;
    $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
    $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};

}

my $yyyy = substr ($date_, 0, 4);
my $mm = substr ($date_, 4, 2);
my $dd = substr ($date_, 6, 2);

my $mfg_exchange_pnl_filename_ = "/apps/data/MFGlobalTrades/ExchangePnl/".$yyyy."/".$mm."/".$dd."/pnl.csv";
my $mfg_product_pnl_filename_ = "/apps/data/MFGlobalTrades/ProductPnl/".$yyyy."/".$mm."/".$dd."/pnl.csv";

open MFG_EXCHANGE_PNL_FILE_HANDLE, "< $mfg_exchange_pnl_filename_" or die "Could not open '$mfg_exchange_pnl_filename_'. Exiting.\n";
my @mfg_exchange_pnl_file_lines_ = <MFG_EXCHANGE_PNL_FILE_HANDLE>;
close MFG_EXCHANGE_PNL_FILE_HANDLE;

open MFG_PRODUCT_PNL_FILE_HANDLE, "< $mfg_product_pnl_filename_" or die "Could not open '$mfg_product_pnl_filename_'. Exiting.\n";
my @mfg_product_pnl_file_lines_ = <MFG_PRODUCT_PNL_FILE_HANDLE>;
close MFG_PRODUCT_PNL_FILE_HANDLE;


#
#for my $symbol_ ( keys %symbol_to_unrealpnl_map_ )
#{
#    print ("$date_\n\tORS :\t$symbol_ | PNL : $symbol_to_unrealpnl_map_{$symbol_} | VOL : $symbol_to_volume_map_{$symbol_}\n");
#}

for (my $i = 0; $i <= $#mfg_product_pnl_file_lines_; $i++) {

    my @words_ = split ( ',', $mfg_product_pnl_file_lines_[$i] );
    my $symbol_ = $words_[1]; 

    $symbol_ =~ s/^\s+//; # Remove leading/trailing white spaces.
    $symbol_ =~ s/\s+$//;

    if ( index ( $symbol_, "NK" ) == 0 ) {

	    my $pnl_ = $words_[2];
	    my $traded_vol_ = $words_[3];

	    if (!exists $symbol_to_unrealpnl_map_{$symbol_}) {
#		print "symbol_to_unrealpnl_map_ does not have |$symbol_|\n";
	    }
	    if (abs ($symbol_to_unrealpnl_map_{$symbol_} - $pnl_) >= 20.00 ||
		$symbol_to_volume_map_{$symbol_} != $traded_vol_) {
		print ("$date_\n\tORS :\t$symbol_ | PNL : $symbol_to_unrealpnl_map_{$symbol_} | VOL : $symbol_to_volume_map_{$symbol_}\n\tMFG:\t$symbol_ | PNL : $pnl_ | VOL : $traded_vol_\n\n");
	    }

    }
}

for (my $i = 0; $i <= $#mfg_exchange_pnl_file_lines_; $i++) {
    my @words_ = split ( ',', $mfg_exchange_pnl_file_lines_[$i] );

    my $symbol_ = $words_[1];

    if ( index ( $symbol_, "OSE" ) == 0 ) {

	    my $pnl_ = $words_[2];
	    my $traded_vol_ = $words_[3];

	    if ($exchange_to_volume_map_{$symbol_} != $traded_vol_ ||
		abs ($exchange_to_unrealpnl_map_{$symbol_} - $pnl_) >= 100.00) {
		print ("$date_\n\tORS :\t$symbol_ | PNL : $exchange_to_unrealpnl_map_{$symbol_} | VOL : $exchange_to_volume_map_{$symbol_}\n\tMFG:\t$symbol_ | PNL : $pnl_ | VOL : $traded_vol_\n\n");
	    }

    }
}

exit (0);

sub SetSecDef 
{
    my $symbol_ = shift;

    if ( index ( $symbol_, "NKM" ) == 0 )
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

    }elsif ( index ( $symbol_, "JGBL" ) == 0 ) {

        my $base_symbol_length_ = length ("JGBL");
        my $expiry_year_offset_ = $base_symbol_length_;
        my $expiry_month_offset_ = $expiry_year_offset_ + 2;
        
        my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$ose_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_,2)}.substr ($symbol_, $expiry_month_offset_, 2);
        $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;
        
        $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "JGBL" ) * $JPY_TO_DOL ;
        $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 10000 * $JPY_TO_DOL ;
        $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "OSE";

    }elsif ( index ( $symbol_, "TOPIX" ) == 0 ) {

        my $base_symbol_length_ = length ("TOPIX");
        my $expiry_year_offset_ = $base_symbol_length_;
        my $expiry_month_offset_ = $expiry_year_offset_ + 2;
        
        my $symbol_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$ose_symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_,2)}.substr ($symbol_, $expiry_month_offset_, 2);
        $symbol_to_symbol_name_map_{$symbol_} = $symbol_name_with_expiry_;
        
        $symbol_to_commish_map_{$symbol_name_with_expiry_} = GetCommissionForShortcode( "TOPIX" ) * $JPY_TO_DOL ;
        $symbol_to_n2d_map_{$symbol_name_with_expiry_} = 100 * $JPY_TO_DOL ;
        $symbol_to_exchange_map_{$symbol_name_with_expiry_} = "OSE";

    }else {

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
