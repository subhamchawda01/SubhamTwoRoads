#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;
use File::Basename;
use File::Find;
use List::Util qw/max min/; # for max
use Data::Dumper;
use Switch;

#use autodie;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore_install";
my $PSCRIPTS_DIR="/home/pengine/prod/live_scripts";
my $PEXEC_BIN="/home/pengine/prod/live_execs";
my $UNKNOWN_TAG_FILE_PATH="";
my $BMF_EXPIRY_EXEC=$PEXEC_BIN."/get_di_term";
require "$PSCRIPTS_DIR/calc_prev_business_day.pl"; #
require "$PSCRIPTS_DIR/calc_next_business_day.pl";
use Term::ANSIColor;

my $hostname_ = `hostname`;


my $USAGE="$0 mode trade_type(H/M) YYYYMMDD pnl_type(TT/TI) Recovery/Normal/SEE_ORS(R/N/S) <iff SEE_ORS mode Tradefile path> [dont_load_over_night]";  #TT : Tag Totals / TI: Tagwise Individual
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $mode_ = shift;
my $trade_type_ = shift || '';
my $today_date_ = `date +"%Y%m%d"` ; chomp ( $today_date_ ) ;
my $input_date_ =  shift || $today_date_ ;
my $PNL_SHOW_MODE= shift || 'TT';
my $IsRecovery= shift || 'N'; #Keep the mode Normal by default
my $LOAD_ = '';
my $input_trade_file = '';
my $MAX_PAGE_SIZE_ = 40;
if ($IsRecovery eq "S"){
	$input_trade_file = shift||'';
}
else{
	$LOAD_ = shift || '';
	$MAX_PAGE_SIZE_ = shift ||40;
}

#EU/AS pnls flag
my $dump_AS_EU_pnl=0;
my $already_dumped_AS_pnl=1;
my $already_dumped_EU_pnl=1;

if ($trade_type_ eq 'H') { $trade_type_ = 'hft'; }
else { $trade_type_ = 'mtt'; }
#=============== Currency Cinversion Rates ==============#
my $EUR_TO_DOL = 1.30;
my $CD_TO_DOL =  0.97; # canadian dollar
my $BR_TO_DOL = 0.51;
my $GBP_TO_DOL = 1.57;
my $HKD_TO_DOL = 0.1289 ;
my $JPY_TO_DOL = 0.0104 ;
my $RUB_TO_DOL = 0.031 ;
my $AUD_TO_DOL = 0.819 ;
my $INR_TO_DOL = 0.152;

# Looking up from the currency info file.
GetCurrencyConversion ( );


my %micex_bcs_fee_tiers_ = ();
$micex_bcs_fee_tiers_{"5000"} = 0.006;
$micex_bcs_fee_tiers_{"10000"} = 0.005;
$micex_bcs_fee_tiers_{"15000"} = 0.004;
$micex_bcs_fee_tiers_{"10000000000"} = 0.003;
my $micex_exchange_fee_ = 0.008 ;
my $bmfeq_exchange_fee_ = 0.00025 ;
my $bmfeq_bp_fee_ = 0.00003 ;
my $last_traded_tom_while_tod = 0;

my %spread_to_pos_symbol_ = ();
my %spread_to_neg_symbol_ = ();
my %symbol_to_expiry_year_ = ("0" => "2010", "1" => "2011", "2" => "2012", "3" => "2013", "4" => "2014", "5" => "2015", "6" => "2016", "7" => "2017", "8" => "2018", "9" => "2019");
my %symbol_to_expiry_month_ = ("F" => "01", "G" => "02", "H" => "03", "J" => "04", "K" => "05", "M" => "06", "N" => "07", "Q" => "08", "U" => "09", "V" => "10", "X" => "11", "Z" => "12");
my %expiry_month_to_symbol_ = ("01" => "F", "02" => "G", "03" => "H", "04" => "J", "05" => "K", "06" => "M", "07" => "N", "08" => "Q", "09" => "U", "10" => "V", "11" => "X", "12" => "Z");

my ($sec,$min,$hour,$day,$month,$year,$wday,$yday,$isdst) = localtime();
my $today = sprintf "%.4d%.2d%.2d", $year+1900, $month+1, $day;

#================== calculating remaining days in expiry for DI products ==================#
my %symbol_to_noof_working_days_till_expiry_ = ( );

#==========================================================================================#

#================== for certain pairs the price for both should remain the same ===========#
#E.g.: {BR_DOL_0, BR_WDO_0}
my %same_price_shc_map_ = ( );

my $secname1_ = `$PEXEC_BIN/get_exchange_symbol BR_WDO_0 $input_date_`;
my $secname2_ = `$PEXEC_BIN/get_exchange_symbol BR_DOL_0 $input_date_`;
$same_price_shc_map_{ $secname1_ } = $secname2_;
$same_price_shc_map_{ $secname2_ } = $secname1_;

$secname1_ = `$PEXEC_BIN/get_exchange_symbol BR_WIN_0 $input_date_`;
$secname2_ = `$PEXEC_BIN/get_exchange_symbol BR_IND_0 $input_date_`;
$same_price_shc_map_{ $secname1_ } = $secname2_;
$same_price_shc_map_{ $secname2_ } = $secname1_;
#===========================================================================================#

#================================Maps to Store Pnl Information==============================#
my %symbol_to_commish_map_ = ();
my %symbol_to_n2d_map_ = ();

my %symbol_to_realpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_lots_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();
my %symbol_to_display_name_ = ();
my %display_name_to_symbol_= ();

my %exchange_to_realpnl_map_ = ();
my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();

my %symbol_to_mkt_vol_ =();
my %symbol_to_currency_map_ = ();
my %exchange_to_currency_map_ = ();
my %symbol_to_total_commish_map_ = ();
my %symbol_to_isExpiring_map_ = ();

#tag-saci-query mapping
my %saci_to_qid_map = ();
my %saci_to_tags_map = ();
my %saci_to_invalid_tags_map = ();
my %invalid_tag_to_display = ();

#tag pnl maps
my %tag_shc_to_realpnl_map = ();
my %tag_shc_to_unrealpnl_map = ();
my %tag_shc_to_vol_map = ();
my %tag_shc_to_pos_map = ();
my %tag_to_pnl_map_ = ();
my %tag_to_vol_map_ = ();

#Updating Prices of Illiquid product using spread information
my %SpreadPosProduct = ();
my %SpreadNegProduct = ();
my %ProductToSpread = ();
#===========================================================================================#

$exchange_to_currency_map_{ "EUREX" } = $EUR_TO_DOL;
$exchange_to_currency_map_{ "LIFFE" } = $GBP_TO_DOL; 	# LFI, YFEBM, JFFCE, KFFTI, KFMFA, JFMFC => EUR
$exchange_to_currency_map_{ "ICE" } = $GBP_TO_DOL; 	# LFI =>EUR,
$exchange_to_currency_map_{ "TMX" } =$CD_TO_DOL;
$exchange_to_currency_map_{ "CME" } =1; 		#NIY => JPY
$exchange_to_currency_map_{ "BMF" } =$BR_TO_DOL;
$exchange_to_currency_map_{ "HKEX" } =$HKD_TO_DOL;
$exchange_to_currency_map_{ "OSE" } =$JPY_TO_DOL;
$exchange_to_currency_map_{ "RTS" } =$RUB_TO_DOL;
$exchange_to_currency_map_{ "MICEX" } =$RUB_TO_DOL;
$exchange_to_currency_map_{ "CFE" } =1;
$exchange_to_currency_map_{ "NSE" } =$INR_TO_DOL;
$exchange_to_currency_map_{ "ASX" } =$AUD_TO_DOL;


#To Disable any server from pnls setup simply comment the entry from the
my %server_to_last_processed_time_ = ();
$server_to_last_processed_time_{"sdv-fr2-srv15"} = 0;  #10.23.102.55"
$server_to_last_processed_time_{"sdv-fr2-srv16"} = 0;   #"10.23.102.56"
$server_to_last_processed_time_{"sdv-fr2-srv13"} = 0;      #10.23.200.53
$server_to_last_processed_time_{"sdv-fr2-srv14"} = 0; #10.23.200.54
$server_to_last_processed_time_{"sdv-chi-srv15"} = 0; #10.23.82.55
$server_to_last_processed_time_{"sdv-chi-srv16"} = 0; #10.23.82.56
$server_to_last_processed_time_{"sdv-chi-srv13"} = 0; # 10.23.82.53
$server_to_last_processed_time_{"sdv-chi-srv14"} = 0; # 10.23.82.54
$server_to_last_processed_time_{"sdv-tor-srv11"} = 0; #10.23.182.51
$server_to_last_processed_time_{"sdv-tor-srv12"} = 0; #10.23.182.52
$server_to_last_processed_time_{"sdv-tor-srv13"} = 0; #10.23.182.53
$server_to_last_processed_time_{"sdv-bsl-srv11"} = 0; #10.23.52.51
$server_to_last_processed_time_{"sdv-bsl-srv12"} = 0;
$server_to_last_processed_time_{"sdv-bsl-srv13"} = 0;
$server_to_last_processed_time_{"sdv-bmf-srv11"} = 0; #10.220.65.35
$server_to_last_processed_time_{"sdv-bmf-srv12"} = 0; #10.220.65.34
$server_to_last_processed_time_{"sdv-bmf-srv15"} = 0; #10.220.65.36
$server_to_last_processed_time_{"sdv-bmf-srv13"} = 0; #10.220.65.33
$server_to_last_processed_time_{"sdv-bmf-srv14"} = 0; #10.220.65.38
#$server_to_last_processed_time_{"sdv-mos-srv11"} = 0; #172.18.244.107
$server_to_last_processed_time_{"sdv-mos-srv12"} = 0; #10.23.241.2
$server_to_last_processed_time_{"sdv-mos-srv13"} = 0; #172.26.33.226
#$server_to_last_processed_time_{"SDV-HK-SRV11"} = 0; #10.152.224.145
#$server_to_last_processed_time_{"SDV-HK-SRV12"} = 0; #10.152.224.146
$server_to_last_processed_time_{"sdv-ose-srv12"} = 0; #10.134.73.212
$server_to_last_processed_time_{"sdv-ose-srv11"} = 0; #10.134.73.211
$server_to_last_processed_time_{"sdv-ose-srv13"} = 0; #10.134.73.213
$server_to_last_processed_time_{"sdv-ose-srv14"} = 0; #10.134.73.214
$server_to_last_processed_time_{"sdv-cfe-srv11"} = 0; #10.23.74.61
$server_to_last_processed_time_{"sdv-cfe-srv12"} = 0; #10.23.74.62
#$server_to_last_processed_time_{"sdv-cfe-srv13"} = 0; #10.23.74.63
$server_to_last_processed_time_{"SDV-ASX-SRV11"} = 0; #10.23.43.51
#$server_to_last_processed_time_{"SDV-ASX-SRV12"} = 0; #10.23.43.52
#$server_to_last_processed_time_{"sdv-sgx-srv11"} = 0; #10.23.26.51
$server_to_last_processed_time_{"sdv-sgx-srv12"} = 0; #10.23.26.52
#$server_to_last_processed_time_{"sdv-ind-srv12"} = 0; #10.23.115.62
#$server_to_last_processed_time_{"sdv-ind-srv11"} = 0; #10.23.115.61
#$server_to_last_processed_time_{"sdv-ind-srv13"} = 0;
#$server_to_last_processed_time_{"sdv-ind-srv15"} = 0; #10.23.27.10

#Only process these tags
my %tags_to_process =();
my %valid_tags =();		#Tags Comming from Queries other than console trade
my %nse_tags =();
my $console_qid = 123456; #Used for Segregating Tags Recieved from Console, which might be incorrect
$valid_tags{"GBLHFT"} = 1;  #All Overnight positions are dumped to global by default so we assume GBLHFT tag will always be valid
$tags_to_process{"GBLHFT"}=1;

my $is_sgx_cn_commish_updated = 0; #Flag to check if SGX_CN commission has been discounted
my $SGX_CN_0_EXCH_SYMBOL = `$PEXEC_BIN/get_exchange_symbol  "SGX_CN_0" $input_date_ ` ; chomp ( $SGX_CN_0_EXCH_SYMBOL);
my $time_eod_trades_=21000;
my $time_start_trades_=33000;
my $cron_time_ = 210800;
my $time_AS_email_ = 61000;
my $time_EU_email_ = 120000;
my $delta_dir = "";
my $temp_email_pnl_file = "";
my $remap_tag_file = "";
if($IsRecovery eq "R"){
	$delta_dir="/spare/local/logs/pnl_data/$trade_type_/delta_recovery/";
	$remap_tag_file="/spare/local/logs/pnl_data/hft/tag_pnl/saci_maps_hist/remap_tag_".$input_date_.".txt";
}
elsif ($IsRecovery eq "N"){
	$delta_dir="/spare/local/logs/pnl_data/$trade_type_/delta_files/";
	$temp_email_pnl_file="/spare/local/logs/pnl_data/$trade_type_/tag_pnl/temp_email_ors_pnl_.txt";
	$remap_tag_file="/spare/local/logs/pnl_data/hft/tag_pnl/remap_tag.txt";
}

#==============================================================================================================================#

sub ProcessInternalExec {
#This function takes an array of internal execution tradefile lines and a flag to identify whether to add trades or remove remove them as arguments and updates the pnl maps.
	my $ors_trade_file_lines_ref= shift;
	my @ors_trades_file_lines_ = @{$ors_trade_file_lines_ref};
	my $add_remove_flag = shift; #0 - Add the trade to the maps, #1 - Remove trades from the maps
	for ( my $i = 0 ; $i <= $#ors_trades_file_lines_; $i ++ )
	{
	    my @words_ = split ( '', $ors_trades_file_lines_[$i] );

	    if ( $#words_ >= 7 )
	    {
		my $symbol_ = $words_[0];
		my $buysell_ = $words_[1];
		my $tsize_ = $words_[2];
		my $tprice_ = $words_[3];
		my $saos_ = $words_[4];
		my $timestamp = $words_[6];
		my $saci = $words_[7];

		#Check if Trade Corresponds to Invalid Tags and Set Invalid Tag for display
		if(exists($saci_to_invalid_tags_map{$saci})){
		    $invalid_tag_to_display{$saci_to_invalid_tags_map{$saci}}=1;
		}

		if ( ! ( exists $symbol_to_pos_map_{$symbol_} ) )
		{
		    $symbol_to_realpnl_map_{$symbol_} = 0;
		    $symbol_to_pos_map_{$symbol_} = 0;
		    $symbol_to_price_map_{$symbol_} = 0;
		    $symbol_to_volume_map_{$symbol_} = 0;
		    $symbol_to_total_commish_map_{$symbol_} = 0;
		    SetSecDef ( $symbol_ ) ;
		}
		my $this_trade_pnl;
		if ( $buysell_ == 0 )
		{ # buy
		    if ( index ( $symbol_ , "DI" ) == 0 ) {
			$this_trade_pnl = $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
		    }
		    else {
			$this_trade_pnl = -1 * $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
		    }
		}
		else
		{
		    if ( index ( $symbol_ , "DI" ) == 0 ) {
			$this_trade_pnl = -1 * $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
		    }
		    else {
			$this_trade_pnl = $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
		    }
		}
		if($add_remove_flag==1){
			$this_trade_pnl=-1*$this_trade_pnl;
			$tsize_=-1*$tsize_;
		}
		if(( index ( $symbol_ , "NSE" ) == 0 )){
		    #Handling for considering trades of specific NSE Tags only.
		    if(exists($saci_to_tags_map{$saci})){
			my $tags_for_saci = $saci_to_tags_map{$saci};
			my @tags = split ( ':', $tags_for_saci );
			my $process_trade = 0;
			if ( $#tags >= 0 )
			{
			    for ( my $i = 0 ; $i <= $#tags; $i++)
			    {
				if(exists($nse_tags{$tags[$i]})){
				    $process_trade = 1;
				}
			    }
			}
			if($process_trade == 0){
			    #If tags are not there in nse_tags skip the trade
			    next;
			}
		    }
		    else{
			; #In case of unknown tags don't add trade to %symbol_to_real_pnl but keep it in tag_shc_map
		    }
		}
#populate tag pnls
		if(! exists $saci_to_tags_map{$saci}) {
		    $saci_to_tags_map{$saci}="UNKNOWNTAG_".$saci;	#fake tag for this saci
		    $tags_to_process{"UNKNOWNTAG_".$saci}=1;
		}
		my $tags_for_saci=$saci_to_tags_map{$saci};
#		print "$tags_for_saci:$symbol_to_display_name_{$symbol_}\n";
		my @tags = split ( ':', $tags_for_saci );
		if ( $#tags >= 0 )
		{
		    for ( my $i = 0 ; $i <= $#tags; $i++)
		    {
#process All tags instead of selected tags
#			if(! (exists $tags_to_process{$tags[$i]})) {
#			    next;
#			}
			my $tag_shc=$tags[$i].':'.$symbol_to_display_name_{$symbol_};
			if(! exists $tag_shc_to_pos_map{$tag_shc}){
			    $tag_shc_to_pos_map{$tag_shc} =0;
			    $tag_shc_to_realpnl_map{$tag_shc} =0;
			    $tag_shc_to_unrealpnl_map{$tag_shc} =0;
			    $tag_shc_to_vol_map{$tag_shc} = 0;

            }
			$tag_shc_to_pos_map{$tag_shc} += ($buysell_ == 0) ? $tsize_ : (-1 * $tsize_);
			$tag_shc_to_realpnl_map{$tag_shc} += $this_trade_pnl;
			$tag_shc_to_vol_map{$tag_shc} += $tsize_;
		    }
		}
	    }
	}
}

sub ProcessTrades {
#Takes any array of tradelines and a flag to identify whether to add trades or remove remove them, updates the pnl maps.
my $ors_trade_file_lines_ref= shift;
my @ors_trades_file_lines_ = @{$ors_trade_file_lines_ref};
my $add_remove_flag = shift; #0 - Add the trade to the maps, #1 - Remove trades from the maps

	for ( my $i = 0 ; $i <= $#ors_trades_file_lines_; $i ++ )
	{
	    
	    my @words_ = split ( '', $ors_trades_file_lines_[$i] );

	    if ( $#words_ >= 7 )
	    {
		my $symbol_ = $words_[0];
		my $buysell_ = $words_[1];
		my $tsize_ = $words_[2];
		my $tprice_ = $words_[3];
		my $saos_ = $words_[4];
		my $timestamp = $words_[6];
		my $saci = $words_[7];

		#Check if Trade Corresponds to Invalid Tags and Set Invalid Tag for display
		if(exists($saci_to_invalid_tags_map{$saci})){
		    $invalid_tag_to_display{$saci_to_invalid_tags_map{$saci}}=1;
		}

		if ( ! ( exists $symbol_to_pos_map_{$symbol_} ) )
		{
		    $symbol_to_realpnl_map_{$symbol_} = 0;
		    $symbol_to_pos_map_{$symbol_} = 0;
		    $symbol_to_price_map_{$symbol_} = 0;
		    $symbol_to_volume_map_{$symbol_} = 0;
		    $symbol_to_total_commish_map_{$symbol_} = 0;
		    SetSecDef ( $symbol_ ) ;
		}
		if( index ( $symbol_to_exchange_map_{$symbol_}, "MICEX" ) == 0 )
		{
		    if($tsize_ > 0){
			$symbol_to_commish_map_{$symbol_} = ( max ( 1 , $micex_exchange_fee_ * $tprice_ * $tsize_ ) + $micex_bcs_fee_tiers_{10000000000} * $tprice_ * $tsize_ ) * $RUB_TO_DOL ;
			$symbol_to_commish_map_{$symbol_} /= $tsize_ ;
		    }
		}
		if( index ( $symbol_to_exchange_map_{$symbol_}, "BMFEQ" ) == 0 )
		{
		    if($tsize_ > 0){
			$symbol_to_commish_map_{$symbol_} = ( $bmfeq_exchange_fee_ + $bmfeq_bp_fee_ ) * $tprice_ * $tsize_ * $BR_TO_DOL ;
			$symbol_to_commish_map_{$symbol_} /= $tsize_ ;
		    }
		}

		if(index ($symbol_, $SGX_CN_0_EXCH_SYMBOL) == 0){
			#Special handling for computing commision for SGX_CN.
			UpdateSgxCnCommission ($timestamp) ;
		}

		if ( exists $same_price_shc_map_{ $symbol_ } ) {
		    my $tsymbol_ = $same_price_shc_map_{ $symbol_ };
		    if ( exists $symbol_to_realpnl_map_{ $tsymbol_ } ) {
			$symbol_to_price_map_{ $tsymbol_ } = $tprice_;
		    }
		}

		if ( index ( $symbol_ , "USD000TODTOM" ) == 0 ) {
		    SplitTodTomSpread ( );
		    SplitTodTomSpread_Tag ();
		    next;
		}
		my $this_trade_pnl;
		if ( $buysell_ == 0 )
		{ # buy
		    if ( index ( $symbol_ , "DI" ) == 0 ) {
			$this_trade_pnl = $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
		    }
		    else {
			$this_trade_pnl = -1 * $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
		    }
		}
		else
		{
		    if ( index ( $symbol_ , "DI" ) == 0 ) {
			$this_trade_pnl = -1 * $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
		    }
		    else {
			$this_trade_pnl = $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
		    }
		}

		if($add_remove_flag==1){
			$this_trade_pnl=-1*$this_trade_pnl;
			$tsize_=-1*$tsize_;
		}

		if(( index ( $symbol_ , "NSE" ) == 0 )){
		    $this_trade_pnl -= ($tsize_ * $tprice_ * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL);
		    #Handling for considering trades of specific NSE Tags only.
		    if(exists($saci_to_tags_map{$saci}) && index($saci_to_tags_map{$saci}, "UNKNOWNTAG")){
			my $tags_for_saci = $saci_to_tags_map{$saci};
			my @tags = split ( ':', $tags_for_saci );
			my $process_trade = 0;
			if ( $#tags >= 0 )
			{
			    for ( my $i = 0 ; $i <= $#tags; $i++)
			    {
				if(exists($nse_tags{$tags[$i]})){
				    $process_trade = 1;
				}
			    }
			}
			if($process_trade == 0){
			    #If tags are not there in nse_tags skip the trade
			    next;
			}
			$symbol_to_total_commish_map_{$symbol_} +=  ($tsize_ * $tprice_ * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL);
			$symbol_to_realpnl_map_{$symbol_} += $this_trade_pnl;

			$symbol_to_pos_map_{$symbol_} += ($buysell_ == 0) ? $tsize_ : (-1 * $tsize_);
			$symbol_to_volume_map_{$symbol_} += $tsize_;
			$symbol_to_price_map_{$symbol_} = $tprice_;
		    }
		    else{
			; #In case of unknown tags don't add trade to %symbol_to_real_pnl but keep it in tag_shc
			$symbol_to_price_map_{$symbol_} = $tprice_;
			$symbol_to_total_commish_map_{$symbol_} +=  ($tsize_ * $tprice_ * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL);
		    }
		}
		else{
		    #Non NSE Exchanges
		    $this_trade_pnl -= ($tsize_ * $symbol_to_commish_map_{$symbol_} );
		    $symbol_to_total_commish_map_{$symbol_} +=  $tsize_ * $symbol_to_commish_map_{$symbol_};
		    $symbol_to_realpnl_map_{$symbol_} += $this_trade_pnl;

		    $symbol_to_pos_map_{$symbol_} += ($buysell_ == 0) ? $tsize_ : (-1 * $tsize_);
		    $symbol_to_volume_map_{$symbol_} += $tsize_;
		    $symbol_to_price_map_{$symbol_} = $tprice_;
		}
		#Update the price of Illiquid Products
		if(exists($ProductToSpread{$symbol_})){
			my $IsPosProduct = ($symbol_ eq $SpreadPosProduct{$ProductToSpread{$symbol_}});
			if(exists($symbol_to_price_map_{$ProductToSpread{$symbol_}})){
				if($IsPosProduct){
					if(exists($symbol_to_pos_map_{$SpreadNegProduct{$ProductToSpread{$symbol_}}})){
						$symbol_to_price_map_{$SpreadNegProduct{$ProductToSpread{$symbol_}}} = $symbol_to_price_map_{$symbol_} - $symbol_to_price_map_{$ProductToSpread{$symbol_}};
					}
				}
				else{
					if(exists($symbol_to_pos_map_{$SpreadPosProduct{$ProductToSpread{$symbol_}}})){
						$symbol_to_price_map_{$SpreadPosProduct{$ProductToSpread{$symbol_}}} = $symbol_to_price_map_{$symbol_} + $symbol_to_price_map_{$ProductToSpread{$symbol_}};
					}
				}
			}
		}
#populate tag pnls
		if(! exists $saci_to_tags_map{$saci}) {
		    $saci_to_tags_map{$saci}="UNKNOWNTAG_".$saci;	#fake tag for this saci
		    $tags_to_process{"UNKNOWNTAG_".$saci}=1;
		}
		my $tags_for_saci=$saci_to_tags_map{$saci};
#		print "$tags_for_saci:$symbol_to_display_name_{$symbol_}\n";
		my @tags = split ( ':', $tags_for_saci );
		if ( $#tags >= 0 )
		{
		    for ( my $i = 0 ; $i <= $#tags; $i++)
		    {
#process All tags instead of selected tags
#			if(! (exists $tags_to_process{$tags[$i]})) {
#			    next;
#			}
			my $tag_shc=$tags[$i].':'.$symbol_to_display_name_{$symbol_};
			if(! exists $tag_shc_to_pos_map{$tag_shc}){
			    $tag_shc_to_pos_map{$tag_shc} =0;
			    $tag_shc_to_realpnl_map{$tag_shc} =0;
			    $tag_shc_to_unrealpnl_map{$tag_shc} =0;
			    $tag_shc_to_vol_map{$tag_shc} = 0;

                        }
			$tag_shc_to_pos_map{$tag_shc} += ($buysell_ == 0) ? $tsize_ : (-1 * $tsize_);
			$tag_shc_to_realpnl_map{$tag_shc} += $this_trade_pnl;
			$tag_shc_to_vol_map{$tag_shc} += $tsize_;
		    }
		}
	    }
	}
}

sub RemoveTradesFromHost {
	my $host = shift;
	my $restart_time = `ls $delta_dir | grep restart| grep $host| awk -F"_" '{ print \$3}'`;
	my $cmd = "rm ".$delta_dir.$host."_restart_".$restart_time;
	system($cmd);
	my @host_trade_files = `ls $delta_dir | grep $host| grep -v int_exec` ;
	my @host_int_exec_files = `ls $delta_dir | grep $host | grep int_exec`;
	foreach (@host_trade_files)
	{
		my $ors_trades_filename_ = $delta_dir.$_;chomp($ors_trades_filename_); #$_ is iterator on @host_trade_files
		my $current_file_ts = (split '_', $ors_trades_filename_)[-1];
		if($current_file_ts > $restart_time){
			next;
		}
		open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "show_pnl_generic_tagwise.pl could not open ors_trades_filename_ $ors_trades_filename_\n";
		my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;
		close ORS_TRADES_FILE_HANDLE;
		system("rm $ors_trades_filename_");
		ProcessTrades(\@ors_trades_file_lines_, 1);
		
	}
	foreach (@host_int_exec_files){
		my $ors_trades_filename_ = $delta_dir.$_;chomp($ors_trades_filename_);
		my $current_file_ts = (split '_', $ors_trades_filename_)[-1];
		if($current_file_ts > $restart_time){
			next;
		}
		open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "show_pnl_generic_tagwise.pl could not open ors_trades_filename_ $ors_trades_filename_\n";
		my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;
		close ORS_TRADES_FILE_HANDLE;
		system("rm $ors_trades_filename_");
		ProcessInternalExec(\@ors_trades_file_lines_, 1);
	}
}

sub RestartHostIfAny {
  my @hosts_to_restart = `ls $delta_dir| grep _restart`; 
  foreach (@hosts_to_restart){
		my $host= (split '_', $_)[0];
                RemoveTradesFromHost($host);
	}
}
sub PairProductForPriceUpdate {
	#Usage: Product_Pos_shc, Product_Neg_shc, Spread_shc
	my $shc_0 = shift;
	my $shc_1 = shift;
	my $spread_shc = shift;
	my $exch_sym_0 = `$PEXEC_BIN/get_exchange_symbol $shc_0 $input_date_`; chomp ($exch_sym_0);
	my $exch_sym_1 = `$PEXEC_BIN/get_exchange_symbol $shc_1 $input_date_`; chomp ($exch_sym_1);
	my $spread_exch_sym = `$PEXEC_BIN/get_exchange_symbol $spread_shc $input_date_`; chomp ($spread_exch_sym);
	$SpreadPosProduct{$spread_exch_sym} = $exch_sym_0;
	$SpreadNegProduct{$spread_exch_sym} = $exch_sym_1;
	$ProductToSpread{$exch_sym_0} = $spread_exch_sym;
	$ProductToSpread{$exch_sym_1} = $spread_exch_sym;
}
sub UpdateSgxCnCommission {
#Updates SGX_CN commission for trades executed after 09:00 UTC.
	my $trade_time = shift; #Trade time is of format seconds.milliseconds
	my @time = split(/\./, $trade_time); #Splits $trade_time by '.'
	my ($sec,$min,$hour) = localtime($time[0]);
	if($hour >=9 && ($is_sgx_cn_commish_updated==0)){
		$is_sgx_cn_commish_updated = 1;
		$symbol_to_commish_map_{$SGX_CN_0_EXCH_SYMBOL} = $symbol_to_commish_map_{$SGX_CN_0_EXCH_SYMBOL} - 0.8;
	}
	if(($hour >=9 && ($is_sgx_cn_commish_updated==1)) || ($hour <9 && ($is_sgx_cn_commish_updated==0))){
	}
	if($hour <9 && ($is_sgx_cn_commish_updated==1)){
		#This is a wierd case, Should not be happening, Added for robustness.
		# We have received a trade with timestamp >= 9 hr UTC and then received an older trade.
		$is_sgx_cn_commish_updated = 0;
		$symbol_to_commish_map_{$SGX_CN_0_EXCH_SYMBOL} = $symbol_to_commish_map_{$SGX_CN_0_EXCH_SYMBOL} + 0.8;
	}
}
sub SendSlackNotification {
	my $channel = shift;
	my $alert_str = shift;
	`$PEXEC_BIN/send_slack_notification "$channel" DATA "$alert_str"`;
}
sub SendEodSlack {
	ResetExchPnls();
	my $totalpnl_ = 0.0;
	my $totalvol_ = 0;
	my $eod_pos_flag = 0;
	my $alert = "Eod Positions: \n";
	SendSlackNotification("tag_based_pnl", $alert);
	SendSlackNotification("prod-issues", $alert); #Send alert on prod-issues for Non-NSE products with positions
#width of R mode can be same as C and E, but just seperated it just incase if any scripts are running assuming width 4.
	my @all_exchanges=("EUREX", "LIFFE", "ICE" , "TMX", "CME", "BMF", "HKEX", "OSE", "RTS", "MICEX", "CFE", "ASX", "SGX", "NSE");

	for(my $i = 0; $i < scalar @all_exchanges; $i++) {
	    my $exchange=$all_exchanges[$i];
	    foreach my $symbol_1 (keys(%symbol_to_pos_map_)){
			if($symbol_to_exchange_map_{$symbol_1} eq $exchange){
		    my $symbol_eod_price_ = $symbol_to_price_map_{$symbol_1};
		    my $n2d_rate_ = $symbol_to_n2d_map_{$symbol_1};
		    if ( ( index ( $symbol_1 , "DI" ) == 0 ) ) {
				$symbol_eod_price_ = -1 * ( 100000 / ( $symbol_to_price_map_{$symbol_1} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_1} / 252 ) );
				$n2d_rate_ = $BR_TO_DOL;
		    }
		    $exchange_to_realpnl_map_{$exchange} += $symbol_to_realpnl_map_{$symbol_1};
		    my $commission_rate_ = $symbol_to_commish_map_{$symbol_1};
		    if(index($symbol_1, "NSE") == 0){
			$commission_rate_ = $symbol_to_price_map_{$symbol_1} * $symbol_to_commish_map_{$symbol_1} * $INR_TO_DOL;
			$exchange_to_volume_map_{$exchange} += $symbol_to_volume_map_{$symbol_1}/$symbol_to_lots_map_{$symbol_to_display_name_{$symbol_1}};
		    }
                    else{
		        $exchange_to_volume_map_{$exchange} += $symbol_to_volume_map_{$symbol_1};
                    }
		    $exchange_to_unrealpnl_map_{$exchange} += $symbol_to_pos_map_{$symbol_1} * $symbol_eod_price_ * $n2d_rate_ - abs ($symbol_to_pos_map_{$symbol_1}) * $commission_rate_;
			}
			if($i==0 && ($symbol_to_pos_map_{$symbol_1} != 0)){
				$alert = "$symbol_1 : $symbol_to_pos_map_{$symbol_1} \n";
				SendSlackNotification("tag_based_pnl", $alert);
				if(index($symbol_1, "NSE")!=0){
					SendSlackNotification("prod-issues", $alert); #Send alert on prod-issues for Non-NSE products with positions
					$eod_pos_flag = 1;
				}
			}
	    }
	}
	if($eod_pos_flag==0){
		$alert ="NO Eod Positions";
		SendSlackNotification("tag_based_pnl", $alert);
		SendSlackNotification("prod-issues", $alert); #Send alert on prod-issues for Non-NSE products with positions
	}
	$alert = " ";
	SendSlackNotification("tag_based_pnl", $alert);
	$alert = "Exchange Pnls:";
	SendSlackNotification("tag_based_pnl", $alert);
	for(my $i = 0; $i < scalar @all_exchanges; $i++) {
	    my $exchange=$all_exchanges[$i];
		    if (( ($exchange_to_unrealpnl_map_{$exchange} + $exchange_to_realpnl_map_{$exchange})!= 0) || $exchange_to_volume_map_{$exchange} != 0)
	    {
                $alert = "$exchange : ".($exchange_to_unrealpnl_map_{$exchange} + $exchange_to_realpnl_map_{$exchange})." | VOLUME : $exchange_to_volume_map_{$exchange}" ;
                SendSlackNotification("tag_based_pnl", $alert);
                $totalpnl_ += $exchange_to_unrealpnl_map_{$exchange}+ $exchange_to_realpnl_map_{$exchange};
                $totalvol_ += $exchange_to_volume_map_{$exchange};
	    }
	}
    $alert = "TOTAL: $totalpnl_ | VOLUME : $totalvol_";
    SendSlackNotification("tag_based_pnl", $alert);
}
sub LoadConfigFile {
    my $config="/spare/local/logs/pnl_data/hft/tag_pnl/tagwise_pnl_config";
    open CONFIG_FILEHANDLE, "< $config" or die " could not open $config\n";
    my @config_data= <CONFIG_FILEHANDLE>;
    close CONFIG_FILEHANDLE;
    for ( my $i = 0 ; $i <= $#config_data; $i ++ )
    {
	my @line = split ( ' ', $config_data[$i] );
	if($#line == 1)
	{
	    if($line[0] eq "STARTTIME")
	    {
		$time_start_trades_=$line[1];
	    }
	    elsif($line[0] eq "ENDTIME")
	    {
		$time_eod_trades_=$line[1];
	    }
	    elsif($line[0] eq "TAGS")
	    {
		my @tags = split ( ':', $line[1]);
		if ( $#tags >= 0 )
		{
		    for ( my $k = 0 ; $k <= $#tags; $k++)
		    {
			$tags_to_process{$tags[$k]}=1;
		    }
		}
	    }
	    elsif($line[0] eq "CONSOLE_QID"){
		$console_qid=$line[1];
	    }
	    elsif($line[0] eq "NSE_TAGS"){
		my @tags = split ( ':', $line[1]);
		if ( $#tags >= 0 )
		{
		    for ( my $k = 0 ; $k <= $#tags; $k++)
		    {
			$nse_tags{$tags[$k]}=1;
			$tags_to_process{$tags[$k]}=1;
		    }
		}
	    }
	}
    }
}

sub RemapTags{
    open REMAP_TAGS_FILEHANDLE, "< $remap_tag_file" or print " could not open $remap_tag_file\n";
    my @remap_tags_data= <REMAP_TAGS_FILEHANDLE>;
    close REMAP_TAGS_FILEHANDLE;
    for ( my $i = 0 ; $i <= $#remap_tags_data; $i ++ )
    {
	my @words_ = split ( ' ', $remap_tags_data[$i] );
	if($#words_ == 1){
	    chomp($words_[0]);chomp($words_[1]);
	    my @incorrect_tags = split(':',uc $words_[0]);
	    my @correct_tags = split(':',uc $words_[1]);
	    my $c_flag = 0;    #Flag to check that new mapping is correct
	    foreach my $c_tag (@correct_tags){
		if(!exists $valid_tags{$c_tag}){
		    $c_flag = 1;
		    last;
		}
	    }
	    if($c_flag == 1){
		select STDERR;
		$| = 1;
		print "ERR:REMAP: Invalid Replacement Tags in $words_[1] \n";
		select STDOUT;
		$| = 1;
		next;
	    }
	    my $saci = 0;
	    my $tag_flag = 0;
#Check if given invalid mapping exists
	    foreach $saci (sort(keys(%saci_to_invalid_tags_map))){
		if($saci_to_invalid_tags_map{$saci} eq $words_[0]){
		    $tag_flag = 1;
		    $saci_to_tags_map{$saci} = $words_[1];
		    if(exists($invalid_tag_to_display{$saci_to_invalid_tags_map{$saci}})){
			delete $invalid_tag_to_display{$saci_to_invalid_tags_map{$saci}};
		    }
		    delete $saci_to_invalid_tags_map{$saci};
		    last;
		}
	    }
	    if($tag_flag == 0){
		select STDERR;
		$| = 1;
		print "ERR:REMAP: Tag Entry Does not exists in invalid_map $words_[0] \n";
		select STDOUT;
		$| = 1;
		next;
	    }
	    if(@incorrect_tags>=1){
		my $old_tag = $incorrect_tags[0];
#Transfer positions and pnls to new tag
		foreach my $old_tag_shc (sort(keys(%tag_shc_to_pos_map))){
#	print "Compare($old_tag_shc : $old_tag)\n";
		    my $symbol;
		    my $tag;
		    my @split_tag_words =split(':', $old_tag_shc);
		    if ( $#split_tag_words == 1 ) {
			$tag = $split_tag_words[0];
			$symbol=$split_tag_words[1];
		    }
		    if($tag ne $old_tag){
			next;
		    }
		    else{
			foreach my $new_tag (@correct_tags){
			    if (!exists($tag_shc_to_pos_map{$new_tag.":".$symbol})){
				$tag_shc_to_pos_map{$new_tag.":".$symbol} = 0;
				$tag_shc_to_realpnl_map{$new_tag.":".$symbol} = 0;
				$tag_shc_to_unrealpnl_map{$new_tag.":".$symbol} = 0;
				$tag_shc_to_vol_map{$new_tag.":".$symbol} = 0;

			    }
			    $tag_shc_to_pos_map{$new_tag.":".$symbol} += $tag_shc_to_pos_map{$old_tag_shc};
			    $tag_shc_to_realpnl_map{$new_tag.":".$symbol} += $tag_shc_to_realpnl_map{$old_tag_shc};
			    $tag_shc_to_vol_map{$new_tag.":".$symbol}+= $tag_shc_to_vol_map{$old_tag_shc};
			}
		    }
		}
	    }
	    #Deleting old_tag_shc pnl data
	    foreach my $old_tag (@incorrect_tags){
		foreach my $old_tag_shc (sort(keys(%tag_shc_to_pos_map))){
		    my $tag;
		    my @split_tag_words =split(':', $old_tag_shc);
		    $tag = $split_tag_words[0];
		    if($tag ne $old_tag){
			next;
		    }
		    delete $tag_shc_to_pos_map{$old_tag_shc};
		    delete $tag_shc_to_realpnl_map{$old_tag_shc};
		    delete $tag_shc_to_unrealpnl_map{$old_tag_shc};
		    delete $tag_shc_to_vol_map{$old_tag_shc};
		}
	    }
	}
	else{
	    select STDERR;
	    $| = 1;
	    print "ERR:REMAP: Wrong Format \n ";
	    select STDOUT;
	    $| = 1;
	    next;
	}
    }
}

sub LoadQidSaciTagsInfo {
#Special handling for retail trades
    my $retail_saci = 799997;
    my $retail_tag = "RETAIL";
    if(! exists $saci_to_tags_map{$retail_saci}){
	$saci_to_qid_map{$retail_saci}=$retail_saci;
	$saci_to_tags_map{$retail_saci}=$retail_tag;
    }

    my $tag_saci_qid_info_file=shift;
    open TAGS_SACI_QID_FILEHANDLE, "< $tag_saci_qid_info_file" or print " could not open $tag_saci_qid_info_file\n";
    my @saci_tags_qid_data= <TAGS_SACI_QID_FILEHANDLE>;
    close TAGS_SACI_QID_FILEHANDLE;
    for ( my $i = 0 ; $i <= $#saci_tags_qid_data; $i ++ )
    {
	my @words_ = split ( ',', $saci_tags_qid_data[$i] );
#print "#words_ $#words_ \n";
	if ( $#words_ >= 2 )
	{
	    my $qid = $words_[0];$qid =~ s/\s+//g;
	    my $saci = $words_[1];$saci =~ s/\s+//g;
	    my $tags =uc $words_[2];$tags =~ s/\s+//g;

#Add Non Console Query Tags to %valid_tags map

	    if($qid != $console_qid){
		my @tags_for_saci = split ( ':', $tags );
		foreach my $tag (@tags_for_saci){
		    $valid_tags{$tag} = 1;
		}
	    }

#if saci_tag map is available for this product, update the maps now
	    if(exists($saci_to_tags_map{$saci}) && $saci_to_tags_map{$saci} eq "UNKNOWNTAG_".$saci)
	    {
		$saci_to_tags_map{$saci}=$tags;
		foreach my $tag_shc ( sort keys %tag_shc_to_realpnl_map ){
		    my @split_tag_words = split ( ':', $tag_shc );
		    my $tag;
		    my $symbol;
		    if ( $#split_tag_words == 1 ) {
			$tag=$split_tag_words[0];
			$symbol=$split_tag_words[1];
			my $exchange_symbol=$display_name_to_symbol_{$symbol};
#print "tag: $tag shc: $symbol \n";
			if( $tag eq "UNKNOWNTAG_".$saci)
			{
			    if(index($exchange_symbol, "NSE")== 0){
				my @tags_for_saci = split ( ':', $tags );
				my $transfer_pnls = 0;
				foreach my $tag (@tags_for_saci){
				    if(exists($nse_tags{$tag})){
					$transfer_pnls = 1;
				    }
				}
				if($transfer_pnls == 0){
				    #If none of the tags is from nse_tags delete entry from tags map
				    delete $tag_shc_to_realpnl_map{$tag_shc};
				    delete $tag_shc_to_unrealpnl_map{$tag_shc};
				    delete $tag_shc_to_vol_map{$tag_shc};
				    delete $tag_shc_to_pos_map{$tag_shc};
				    last;
				}
				else{
				    #Transfer positions to corrected tag_shc
				    foreach my $tag (@tags_for_saci){
					if(exists($nse_tags{$tag})){
						if(! exists $tag_shc_to_pos_map{$tag.":".$symbol}){
			    			$tag_shc_to_pos_map{$tag.":".$symbol} =0;
			    			$tag_shc_to_realpnl_map{$tag.":".$symbol} =0;
			    			$tag_shc_to_unrealpnl_map{$tag.":".$symbol} =0;
			   				$tag_shc_to_vol_map{$tag.":".$symbol} = 0;
                    	}
					    $tag_shc_to_realpnl_map{$tag.":".$symbol} += $tag_shc_to_realpnl_map{$tag_shc};
					    $tag_shc_to_vol_map{$tag.":".$symbol} += $tag_shc_to_vol_map{$tag_shc};
					    $tag_shc_to_pos_map{$tag.":".$symbol}  += $tag_shc_to_pos_map{$tag_shc};


					}
				    }
				    #Transfer Positions to the symbol maps
				    if (!exists($symbol_to_pos_map_{$exchange_symbol})){
				    	$symbol_to_realpnl_map_{$exchange_symbol} = 0;
				    	$symbol_to_pos_map_{$exchange_symbol} =0;
				    	$symbol_to_volume_map_{$exchange_symbol} =0;
		    			$symbol_to_price_map_{$exchange_symbol} = 0;
		    			$symbol_to_total_commish_map_{$exchange_symbol} = 0;
		    			SetSecDef ( $exchange_symbol ) ;
				    }
				    $symbol_to_realpnl_map_{$exchange_symbol} += $tag_shc_to_realpnl_map{$tag_shc};
				    $symbol_to_pos_map_{$exchange_symbol} += $tag_shc_to_pos_map{$tag_shc};
				    $symbol_to_volume_map_{$exchange_symbol} += $tag_shc_to_vol_map{$tag_shc};
				    
				    #Delete unknown tag entry
				    delete $tag_shc_to_realpnl_map{$tag_shc};
				    delete $tag_shc_to_unrealpnl_map{$tag_shc};
				    delete $tag_shc_to_vol_map{$tag_shc};
				    delete $tag_shc_to_pos_map{$tag_shc};
				}
			    }
			    else{
				#Non NSE Exchanges
				my @tags_for_saci = split ( ':', $tags );
				if ( $#tags_for_saci >= 0 )
				{
				    for ( my $i = 0 ; $i <= $#tags_for_saci; $i++)
				    {
					#if(! exists $tags_to_process{$tags_for_saci[$i]}) {next;}
					#print "debug: this tag_shc $tags_for_saci[$i].':'.$symbol \n";
					$tag_shc_to_realpnl_map{$tags_for_saci[$i].":".$symbol} += $tag_shc_to_realpnl_map{$tag_shc};

					$tag_shc_to_vol_map{$tags_for_saci[$i].":".$symbol} += $tag_shc_to_vol_map{$tag_shc};
					$tag_shc_to_pos_map{$tags_for_saci[$i].":".$symbol}  += $tag_shc_to_pos_map{$tag_shc};
				    }
#dont need this unknowntag entries anymore
				    delete $tag_shc_to_realpnl_map{$tag_shc};
				    delete $tag_shc_to_unrealpnl_map{$tag_shc};
				    delete $tag_shc_to_vol_map{$tag_shc};
				    delete $tag_shc_to_pos_map{$tag_shc};
				}
			    }
			}
		    }
		}
	    }

	    my @tags_for_saci = split ( ':', $tags );
	    if ( $#tags_for_saci >= 0 )
	    {
		my $illegal_tags= 1;
		for ( my $i = 0 ; $i <= $#tags_for_saci; $i++)
		{
		    if(exists $valid_tags{$tags_for_saci[$i]}) {
			$illegal_tags = 0;
		    }
		}
		if(exists($saci_to_invalid_tags_map{$saci})){
		    delete $saci_to_invalid_tags_map{$saci};
		}
		if($illegal_tags == 1){
		    $saci_to_invalid_tags_map{$saci} = $tags;
		}
		$saci_to_qid_map{$saci}=$qid;
		$saci_to_tags_map{$saci}=$tags;
	    }
	}
    }
}

sub TestLoadQidSaciTagsInfo {
    print "Mapping stored\n";
    foreach my $saci (keys %saci_to_tags_map){
        print "$saci_to_tags_map{$saci} $saci $saci_to_qid_map{$saci}\n";
    }
}


#==============================================SEE ORS MODE HANDLING==========================================#
if ($IsRecovery eq "S"){

		open ORS_TRADES_FILE_HANDLE, "< $input_trade_file" or die "show_pnl_generic_tagwise.pl could not open ors_trades_filename_ $input_trade_file\n";
		my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;
		close ORS_TRADES_FILE_HANDLE;
		ProcessTrades( \@ors_trades_file_lines_, 0);
    	PrintTagProductPnls ( 'R' );
	    PrintExchPnls();
	    exit(0);
}
#=============================================================================================================#

##================== Load overnight positions only when no argument is provided for the same =================#
if ($LOAD_ eq '') {
    LoadOvernightPositions ( )
}
#=============================================================================================================#
#LoadConfigfile for start, end time, tags to process etc
LoadConfigFile ();
PairProductForPriceUpdate("SGX_NK_1","SGX_NK_0","SP_SGX_NK0_NK1");
($sec,$min,$hour) = localtime();

#make STDOUT HOT so that nothing is buffered
select STDOUT;
$| = 1;

while(1)
{
	if($IsRecovery eq "R"){
		$UNKNOWN_TAG_FILE_PATH="/spare/local/logs/pnl_data/hft/tag_pnl/saci_maps_hist/unknown_qid_saci_tag_"."$input_date_";
	}
	elsif ($IsRecovery eq "N"){
		$UNKNOWN_TAG_FILE_PATH="/spare/local/logs/pnl_data/hft/tag_pnl/unknown_qid_saci_tag";
	}

#LoadQidSaciTagsInfo ($OLD_TAG_FILE_PATH);
	foreach my $server ( sort keys %server_to_last_processed_time_ )
    {
    	my $TAG_DIR="/spare/local/logs/pnl_data/hft/tag_pnl/saci_maps_hist/";
    	my $qid_saci_tag_map_file=`ls $TAG_DIR| grep "$server" | grep "$input_date_"`;  #Assumption there is only one unique file for each server and date
#        print "$server: $TAG_DIR.$qid_saci_tag_map_file\n";
       if($qid_saci_tag_map_file ne ""){
          LoadQidSaciTagsInfo ($TAG_DIR.$qid_saci_tag_map_file);
       }
    }

    LoadQidSaciTagsInfo ($UNKNOWN_TAG_FILE_PATH); #Deliberately placed after all mapping so wrong console mappings can be overwritten
    RemapTags ();
    RestartHostIfAny();
#TestLoadQidSaciTagsInfo;
    my @delta_files_to_process;
    foreach my $server ( sort keys %server_to_last_processed_time_ )
    {
	my @delta_file_times = `ls $delta_dir |  awk -F"_" '{if (\$2 == "$server") print \$3}'| sort` ;
	foreach my $file_time (@delta_file_times)
	{
	    chomp ($file_time);
	    if ( int($file_time) > $server_to_last_processed_time_{$server})
	    {
		my @temp_file_;
		find(sub  { push @temp_file_ , $File::Find::name if ( m/^(.*)$server/ and m/^(.*)$file_time/ ) }, $delta_dir);
                @delta_files_to_process = ( @delta_files_to_process, @temp_file_);
	    }
	    if( int($file_time) > $server_to_last_processed_time_{$server})
	    {
		$server_to_last_processed_time_{$server}=int($file_time);
	    }
	}
    }

    my $total_index_fut_win_ind_volume_ = 0.0 ;
    foreach my $ors_trades_filename_ (@delta_files_to_process)
    {
    if(index($ors_trades_filename_, "int_execution")!=-1){
    	#Internal Exec Trade File
		open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "show_pnl_generic_tagwise.pl could not open ors_trades_filename_ $ors_trades_filename_\n";
		my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;
		close ORS_TRADES_FILE_HANDLE;
        ProcessInternalExec(\@ors_trades_file_lines_, 0);
    	next;
    }
	open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "show_pnl_generic_tagwise.pl could not open ors_trades_filename_ $ors_trades_filename_\n";
	my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;
	close ORS_TRADES_FILE_HANDLE;
	ProcessTrades( \@ors_trades_file_lines_, 0);
    }
# don't show the PNL Line for TODTOM Spread (since we are splitting it into TOD and TOM outrights)
    if ( exists $symbol_to_price_map_{ "USD000TODTOM" } ) {
	delete $symbol_to_price_map_{ "USD000TODTOM" };
    }

    PrintTagProductPnls ('C');
    PrintTagTotalPnls ('C');
    PrintExchPnls();
    PrintInvalidTags();
    sleep (30);

    my $time_now_=`date +"%H%M%S"`;
    if( ($IsRecovery eq "R") || (int($time_now_) >= ($time_eod_trades_) && (int($time_now_) <=$time_start_trades_)) ) #Handling To Avoid Dumping by TI mode
    {
	if($PNL_SHOW_MODE eq 'TT'){
		my $eod_date_  = "";
		if($IsRecovery eq "R"){
			$eod_date_ = $input_date_ ;
		}
		elsif ($IsRecovery eq "N"){
			$eod_date_ = `date +"%Y%m%d"` ; chomp ( $eod_date_ ) ;
		}
	    my $eod_pnl_tag_file="/spare/local/logs/pnl_data/$trade_type_/tag_pnl/EODPnl/all_exch_tag_ors_pnl_$eod_date_".".txt";
	    my $eod_pnl_query_file="/spare/local/logs/pnl_data/$trade_type_/tag_pnl/EODPnl/all_exch_query_ors_pnl_$eod_date_".".txt";
	    my $eod_pos_file="/spare/local/logs/pnl_data/$trade_type_/tag_pnl/EODPos/all_exch_overnight_pos_$eod_date_".".txt";
	    my $EOD_FILE_LOCATION="/spare/local/logs/pnl_data/$trade_type_/tag_pnl/EODPos";

	    $mode_ = 'R';
	    #print "dumping pnls \n";
	    open (my $EOD_PNL, '>', "$eod_pnl_tag_file") or die "Can't open $eod_pnl_tag_file $!";
	    select $EOD_PNL;
	    $| = 1;
	    if ($trade_type_ eq 'mtt') {
		print "\n \nMFT pnls: \n";
	    }
	    else {
		print "HFT Pnls: \n";
	    }
	    PrintTagProductPnls ( 'R' );
	    PrintTagTotalPnls ('R');
	    PrintExchPnls();
	    sleep(20);
	    select STDOUT;
	    $| = 1;
	    print "proceeding to dump overnight positions \n";
	    open (my $EOD_POS_TMP, '>', "$eod_pos_file") or die "Can't open $eod_pos_file $!";
	    select $EOD_POS_TMP;
	    $| = 1;
	    DumpOrsPositions ( );
	    if($IsRecovery eq "N"){
			SendEodSlack ( );
	    }
	    sleep(10);
	}
        last;
    }
      #pnl dump for AS and EU pnls
  if((int($time_now_) < $time_AS_email_) && ($already_dumped_AS_pnl == 1)) {
	$already_dumped_AS_pnl = 0;
  }
  elsif((int($time_now_) >= $time_AS_email_) && ($already_dumped_AS_pnl == 0)) {
	$dump_AS_EU_pnl=1;
	$already_dumped_AS_pnl=1;
  }
  if((int($time_now_) < $time_EU_email_) && ($already_dumped_EU_pnl == 1)) {
	$already_dumped_EU_pnl = 0;
  }
  elsif((int($time_now_) >= $time_EU_email_) && ($already_dumped_EU_pnl == 0)) {
	$dump_AS_EU_pnl=1;
	$already_dumped_EU_pnl=1;
  }
       #for EU/AS emails, Dump this calculated pnls and dump into respective files
    if($dump_AS_EU_pnl == 1) {
		$mode_ = 'R';
	    open (my $EOD_PNL_EMAIL, '>', "$temp_email_pnl_file") or die "Can't open $temp_email_pnl_file $!";
	    select $EOD_PNL_EMAIL;
	    $| = 1;
	    if ($trade_type_ eq 'mtt') {
	      print "\n \nMFT pnls: \n";
	    }
	    else {
	      print "HFT Pnls: \n";
	    }
	    PrintTagProductPnls ( 'R' );
	    PrintTagTotalPnls ('R');
	    PrintExchPnls();
	    select STDOUT;
	    $mode_ = 'C';
		$| = 1;
		$dump_AS_EU_pnl = 0;
    }
}

exit ( 0 );



sub PrintTagProductPnls
{
    my $date_ = `date`;
    printf ("\n$date_\n");

    my $mode=shift;

#    my $date_ = `date`;
#    printf ("\n$date_\n");
    %tag_to_pnl_map_= ();
    %tag_to_vol_map_ =();
    my $page_size_=keys %tag_shc_to_pos_map;
    my $line_number_ = 0 ;
    foreach my $tag_shc ( sort keys %tag_shc_to_pos_map ){
	my @split_tag_words = split ( ':', $tag_shc );
	my $tag;
	my $symbol;
	if ( $#split_tag_words == 1 ) {
	    $tag=$split_tag_words[0];
	    $symbol=$split_tag_words[1];
#print "tag: $tag shc: $symbol \n";
	}
	else {
	    print "Incorrect tag:shc entry $tag_shc\n";
	    next;
	}
	if(! (exists $tags_to_process{$tag})) {
	    next;
	}
	$line_number_ += 1 ;
#close open positions on latest trade price
	my $exchange_symbol=$display_name_to_symbol_{$symbol};
	#print "DBG: ".$symbol." : ".$symbol_to_exchange_map_{$symbol}." : ".$display_name_to_symbol_{$symbol}." : ".$display_name_to_symbol_{$exchange_symbol}."\n";
        my $exch_ = $symbol_to_exchange_map_{$symbol};
        
        my $symbol_eod_price_ = $symbol_to_price_map_{$exchange_symbol};
        my $n2d_rate_ = $symbol_to_n2d_map_{$exchange_symbol};
        if ( ( index ( $exchange_symbol , "DI" ) == 0 ) ) {
            $symbol_eod_price_ = -1 * ( 100000 / ( $symbol_to_price_map_{$exchange_symbol} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$exchange_symbol} / 252 ) );
	    $n2d_rate_ = $BR_TO_DOL;
        }

	my $commission_rate_ = $symbol_to_commish_map_{$exchange_symbol};

        if($exch_ eq "NSE"){
	    $commission_rate_ = $symbol_to_price_map_{$exchange_symbol} * $symbol_to_commish_map_{$exchange_symbol} * $INR_TO_DOL;
	    $tag_to_vol_map_{$tag} += $tag_shc_to_vol_map{$tag_shc}/$symbol_to_lots_map_{$exchange_symbol}; #NSE volumes are divided by lot size
        }
        else{
	    $tag_to_vol_map_{$tag} += $tag_shc_to_vol_map{$tag_shc};
        }
        $tag_shc_to_unrealpnl_map{$tag_shc} = $tag_shc_to_pos_map{$tag_shc} * $symbol_eod_price_ * $n2d_rate_ - abs ($tag_shc_to_pos_map{$tag_shc}) * $commission_rate_;

        $tag_to_pnl_map_{$tag} += $tag_shc_to_realpnl_map{$tag_shc} + $tag_shc_to_unrealpnl_map{$tag_shc};

	if(($exch_ ne "NSE")&&$PNL_SHOW_MODE eq 'TI'){ #Handling for not displaying NSE products
	    printf "| %35.35s ", $tag_shc;
	    Print ($mode, $tag_shc_to_realpnl_map{$tag_shc} + $tag_shc_to_unrealpnl_map{$tag_shc} );
	    if( 0 == $line_number_ % 2 || $page_size_ < $MAX_PAGE_SIZE_ || $mode_ eq 'R') {
	    	printf "| POS: %6d | VOL: %6d \n", $tag_shc_to_pos_map{$tag_shc},$tag_shc_to_vol_map{$tag_shc};
	    }
	    else{
		printf "| POS: %6d | VOL: %6d \t", $tag_shc_to_pos_map{$tag_shc},  $tag_shc_to_vol_map{$tag_shc};
	    }
	}

    }
    if ($PNL_SHOW_MODE eq 'TT'){
	PrintProductPnls () ;
    }
}

sub PrintProductPnls{
    my $line_number_ = 0 ;
    my $page_size_ = keys %symbol_to_price_map_ ;

    foreach my $symbol_ ( sort keys %symbol_to_price_map_ )
    {
	my $unreal_pnl_ = 0 ;
	my $symbol_eod_price_ = $symbol_to_price_map_{$symbol_};
	my $n2d_rate_ = $symbol_to_n2d_map_{$symbol_};
	if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {
	    $symbol_eod_price_ = -1 * ( 100000 / ( $symbol_to_price_map_{$symbol_} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) );
	    $n2d_rate_ = $BR_TO_DOL;
	}

	my $commission_rate_ = $symbol_to_commish_map_{$symbol_};
	if ( ( index ( $symbol_ , "NSE" ) == 0 ) ) {
	    $commission_rate_ = $symbol_to_price_map_{$symbol_} * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL;
	}
	if($symbol_to_exchange_map_{$symbol_} eq "NSE"){ #Handling for not displaying NSE Products
	    next;
	}
	$unreal_pnl_ = $symbol_to_realpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_eod_price_ * $n2d_rate_ - abs ($symbol_to_pos_map_{$symbol_}) * $commission_rate_;
	$line_number_ += 1 ;
	if($mode_  eq  'C'){
	    print color("BOLD");
	    if(exists $symbol_to_isExpiring_map_{$symbol_}) {
		print color("yellow");
	    }
	    if (index($symbol_,"LFL") == 0 )
	    {
		printf "| %15.15s ", substr($symbol_to_display_name_{$symbol_},0,8).substr($symbol_to_display_name_{$symbol_},11,4);
	    }
	    else
	    {
		printf "| %15.15s ", $symbol_to_display_name_{$symbol_};
	    }
	    print color("reset");
	}

	elsif($mode_  eq  'R' || $mode_  eq  'E' || $mode_  eq  'W'){
	    printf "| %16s ", $symbol_to_display_name_{$symbol_};
	}
	else #mode is P- only native commissions
	{
#printf "%s|%s|%s|%s|%s\n", $symbol_, $symbol_to_total_commish_map_{$symbol_}/$symbol_to_currency_map_{$symbol_}, $symbol_to_commish_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}, $symbol_to_currency_map_{$symbol_};
	    printf "%s|%s\n", $symbol_, $symbol_to_total_commish_map_{$symbol_}/$symbol_to_currency_map_{$symbol_};
	}

	if($mode_  eq  'E' || $mode_  eq  'W')
	{
	    my $native_enreal_pnl_=$unreal_pnl_/$symbol_to_currency_map_{$symbol_};
	    Print($mode_, $native_enreal_pnl_);
	}
	elsif($mode_ eq 'C' || $mode_ eq 'R') #not needed in native currency
	{
	    Print($mode_, $unreal_pnl_);
	}
	my $last_closing_price = sprintf("%.6f", $symbol_to_price_map_{$symbol_});
	if($mode_ eq 'R' || $mode_  eq  'E' || $mode_  eq  'W'){
	    printf "| POS : %4d | VOL : %4d | LPX : %s |\n", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}, $last_closing_price;
	}
	elsif ( $mode_ eq 'C') {
	    if( 0 == $line_number_ % 2 || $page_size_ < $MAX_PAGE_SIZE_){
		printf "| POS: %6d | VOL: %7d | LPX: %13s |\n", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}, $last_closing_price;
	    }
	    else{
		printf "| POS: %6d | VOL: %7d | LPX: %13s |\t", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}, $last_closing_price;
	    }
	}

    }
}

sub PrintTagTotalPnls
{
    print "\n \n";
    my $mode=shift;
    my $total_tag_pnls;
    my $total_tag_vol;
    foreach my $tag ( sort keys %tag_to_pnl_map_ ){
	if(! (exists $tags_to_process{$tag})) {next;}   #Skip tags which are not there in config
	if(index ( $tag , "UNKNOWN" ) == 0 ) {next;}
	printf "%17s |", $tag;
	Print($mode, $tag_to_pnl_map_{$tag});
	$total_tag_pnls+=$tag_to_pnl_map_{$tag};
	$total_tag_vol+=$tag_to_vol_map_{$tag};
	printf "| VOLUME : %8d |\n", $tag_to_vol_map_{$tag};
    }
    print "\n";
}

sub ResetExchPnls{
    my @all_exchanges=("EUREX", "LIFFE", "ICE" , "TMX", "CME", "BMF", "HKEX", "OSE", "RTS", "MICEX", "CFE", "ASX", "SGX", "NSE");
    foreach my $exch_(@all_exchanges){
        $exchange_to_unrealpnl_map_{$exch_} = 0;
        $exchange_to_realpnl_map_{$exch_} = 0;
        $exchange_to_volume_map_{$exch_} = 0;
    }
}

sub PrintExchPnls
{
    if($mode_ ne 'P'){
	ResetExchPnls();
	printf "\n\n----------------------------------------------------------------------------------------\n\n";

	my $totalpnl_ = 0.0;
	my $totalvol_ = 0;

#width of R mode can be same as C and E, but just seperated it just incase if any scripts are running assuming width 4.
	my @all_exchanges=("EUREX", "LIFFE", "ICE" , "TMX", "CME", "BMF", "HKEX", "OSE", "RTS", "MICEX", "CFE", "ASX", "SGX", "NSE");

	for(my $i = 0; $i < scalar @all_exchanges; $i++) {
	    my $exchange=$all_exchanges[$i];
#        print " Initial UPnl:$exchange : $exchange_to_unrealpnl_map_{$exchange}:RPnl: $exchange_to_realpnl_map_{$exchange}\n";
	    foreach my $symbol_1 (keys(%symbol_to_pos_map_)){
        	if($symbol_to_exchange_map_{$symbol_1} eq $exchange){
#print "$symbol_1: $symbol_to_pos_map_{$symbol_1} \n";
		    my $symbol_eod_price_ = $symbol_to_price_map_{$symbol_1};
		    my $n2d_rate_ = $symbol_to_n2d_map_{$symbol_1};
		    if ( ( index ( $symbol_1 , "DI" ) == 0 ) ) {
            		$symbol_eod_price_ = -1 * ( 100000 / ( $symbol_to_price_map_{$symbol_1} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_1} / 252 ) );
                  	$n2d_rate_ = $BR_TO_DOL;
		    }
		    $exchange_to_realpnl_map_{$exchange} += $symbol_to_realpnl_map_{$symbol_1};

		    my $commission_rate_ = $symbol_to_commish_map_{$symbol_1};
		    if(index($symbol_1, "NSE") == 0){
			$commission_rate_ = $symbol_to_price_map_{$symbol_1} * $symbol_to_commish_map_{$symbol_1} * $INR_TO_DOL;
			$exchange_to_volume_map_{$exchange} += $symbol_to_volume_map_{$symbol_1}/$symbol_to_lots_map_{$symbol_to_display_name_{$symbol_1}};

		    }
                    else{
		        $exchange_to_volume_map_{$exchange} += $symbol_to_volume_map_{$symbol_1};
                    }
		    $exchange_to_unrealpnl_map_{$exchange} += $symbol_to_pos_map_{$symbol_1} * $symbol_eod_price_ * $n2d_rate_ - abs ($symbol_to_pos_map_{$symbol_1}) * $commission_rate_;
		    #print " UPnl:$exchange : $exchange_to_unrealpnl_map_{$exchange}\n";
        	}
	    }
	    if (( ($exchange_to_unrealpnl_map_{$exchange} + $exchange_to_realpnl_map_{$exchange})!= 0) || $exchange_to_volume_map_{$exchange} != 0)
	    {
                printf "%17s |", $exchange;

                Print($mode_, $exchange_to_unrealpnl_map_{$exchange} + $exchange_to_realpnl_map_{$exchange});

                if ( $mode_  eq 'C' ){
		    printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{$exchange};
                }

                if ( $mode_ eq 'R' || $mode_  eq 'E' || $mode_  eq 'W'){
		    printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{$exchange};
                }

                $totalpnl_ += $exchange_to_unrealpnl_map_{$exchange}+ $exchange_to_realpnl_map_{$exchange};
                $totalvol_ += $exchange_to_volume_map_{$exchange};
	    }
	}
	if ($trade_type_ eq 'mtt') {
	    printf "\n%17s |", "MFT_TOTAL";
	}
	else {
            printf "\n%17s |", "TOTAL";
	}

	Print($mode_, $totalpnl_);

	printf "| VOLUME : %8d |\n", $totalvol_;
    }
#   foreach my $s (keys(%symbol_to_exchange_map_)) {
#     print "$s : $symbol_to_exchange_map_{$s} \n";
#}

}
sub PrintInvalidTags
{
    if(%invalid_tag_to_display){
	print "----------------------------------------------------------------------------------------\n";
	print "Invalid Tags: Add correct mapping to:".color("yellow")." /spare/local/logs/pnl_data/hft/tag_pnl/remap_tag.txt".color("reset")." ]\n";
	foreach my $invalid_tags (keys(%invalid_tag_to_display)){
	    print color("red"); print color("BOLD");
	    print "$invalid_tags\t";
	    print color("reset");
	}
	print "\n----------------------------------------------------------------------------------------\n";
    }
}
sub GetMktVol
{
    my $symbol_ = shift ;

    my $is_valid_ = 0;
    if ( index ( $hostname_ , "ip-10-0" ) < 0 ) {
	my $vol_ = `$HOME_DIR/infracore/scripts/get_curr_mkt_vol.sh "$symbol_"` ; chomp($vol_);
	$vol_ =~ s/^\s+|\s+$//g;

	if ( $vol_ ne "" && $vol_ =~ /^-?\d+\.?\d*\z/ ) {
	    $is_valid_ = 1;
	}
	if ( $is_valid_ ) {
	    $symbol_to_mkt_vol_{$symbol_} = ( $vol_ + 1 ) / 100 ;
	}
    }
    if ( ! $is_valid_ && ! exists( $symbol_to_mkt_vol_{$symbol_} ) ) {
	$symbol_to_mkt_vol_{$symbol_} = -1000000000 ;
    }
}

sub SetSecDef
{
    my $symbol_ = shift;

    $symbol_to_display_name_{$symbol_} = $symbol_;
    $display_name_to_symbol_{$symbol_} = $symbol_;
    my $shc_ = `$PEXEC_BIN/get_shortcode_for_symbol  "$symbol_" $input_date_ ` ; chomp ( $shc_ );
    MarkExpiringProducts($symbol_, $shc_);
    if ( index ( $symbol_, "VX" ) == 0 )
    {
	if( index ( $symbol_, "_") != -1)
	{
	    my $base_symbol_length_ = length ("VX");
	    my $expiry_month_offset_ = $base_symbol_length_;
	    my $expiry_year_offset_ = $expiry_month_offset_ + 1;

	    my $symbol_1_name_with_expiry_ = substr ($symbol_, 0, $base_symbol_length_).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_,1)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	    $spread_to_neg_symbol_{$symbol_}=$symbol_1_name_with_expiry_;
	    $base_symbol_length_ = length ("VX-----");
	    $expiry_month_offset_ = $base_symbol_length_;
	    $expiry_year_offset_ = $expiry_month_offset_ + 1;

	    my $symbol_2_name_with_expiry_ = substr ($symbol_, 0, length("VX")).$symbol_to_expiry_year_{substr ($symbol_, $expiry_year_offset_)}.$symbol_to_expiry_month_{substr ($symbol_, $expiry_month_offset_, 1)};
	    $spread_to_pos_symbol_{$symbol_}=$symbol_2_name_with_expiry_;
	}
	my $commish_ = `$PEXEC_BIN/get_contract_specs "$shc_" $input_date_ COMMISH | awk '{print \$2}' `; chomp ( $commish_ ) ;
	$symbol_to_commish_map_ {$symbol_} = $commish_;

	$symbol_to_n2d_map_{$symbol_} = 1000 ;
	$symbol_to_exchange_map_{$symbol_} = "CFE";
    }
    else
    {
	if(index ( $symbol_, "NSE" ) == 0){
	    $symbol_to_display_name_{$symbol_} = substr($shc_,4);
	    $display_name_to_symbol_{substr($shc_,4)} = $symbol_;
	    $symbol_to_exchange_map_{$symbol_} = "NSE";
	    $symbol_to_exchange_map_{$symbol_to_display_name_{$symbol_}} = "NSE";
	}
	my @contract_specs =`$PEXEC_BIN/get_contract_specs "$shc_" $input_date_ ALL`; #Get All specs from at once
	my $commish_; my $exchange_name_; my $n2d_; my $lots_;
	foreach my $spec (@contract_specs){
    my @spec_split = split(":", $spec);
    chomp($spec_split[0]);
    chomp($spec_split[1]);
    $spec_split[0]=~ s/^\s*(.*?)\s*$/$1/;
    $spec_split[1]=~ s/^\s*(.*?)\s*$/$1/;
    switch($spec_split[0]){
      case "COMMISH" {$commish_ = $spec_split[1];}
      case "EXCHANGE" {$exchange_name_ = $spec_split[1];}
      case "N2D" {$n2d_ = $spec_split[1];}
      case "LOTSIZE" {$lots_ = $spec_split[1];}
      case "TICKSIZE" {}
      else {}
  }
  }

	#my $exchange_name_ = `$PEXEC_BIN/get_contract_specs "$shc_"  $input_date_ EXCHANGE | awk '{print \$2}' `; chomp ( $exchange_name_ ) ;
	if ( $exchange_name_ eq "HONGKONG" ) { $exchange_name_ = "HKEX" ; }
	if ( index ( $exchange_name_, "MICEX" ) == 0 )  { $exchange_name_ = "MICEX" ; }
	$symbol_to_exchange_map_{$symbol_}=$exchange_name_;

	#my $commish_ = `$PEXEC_BIN/get_contract_specs "$shc_" $input_date_ COMMISH | awk '{print \$2}' `; chomp ( $commish_ ) ;
	$symbol_to_commish_map_ {$symbol_} = $commish_;

	#my $n2d_ = `$PEXEC_BIN/get_contract_specs $shc_ $input_date_ N2D | awk '{print \$2}' `; chomp ( $n2d_ ) ;
	$symbol_to_n2d_map_{$symbol_} = $n2d_;

	#my $lots_ = `$PEXEC_BIN/get_contract_specs $shc_ $input_date_ LOTSIZE | awk '{print \$2}' `; chomp ( $lots_ ) ;
	$symbol_to_lots_map_{$symbol_to_display_name_{$symbol_}} = $lots_;
    $symbol_to_lots_map_{$symbol_} = $lots_;
	if ( index ( $symbol_, "DI" ) == 0 && ! exists $symbol_to_noof_working_days_till_expiry_{$symbol_}) {
	    $symbol_to_noof_working_days_till_expiry_{$symbol_} = `$BMF_EXPIRY_EXEC $symbol_  $input_date_` ;
	}
    }
#initialising symbol to currency map: symbol_to_currency_map_
    my $exchange_name_=$symbol_to_exchange_map_{$symbol_};
    my $curr=$exchange_to_currency_map_{$exchange_name_};
#'EUREX' 'TMX' 'BMF' 'HKEX''OSE' 'RTS' 'MICEX' 'CFE' 'NSE' ASX
#all products in native currency for these exchanges : handled in else part
    if ($exchange_name_ eq 'LIFFE')
    {
	if(index ($symbol_,'LFI') != -1 || index ($symbol_,'YFEBM') != -1 || index ($symbol_,'JFFCE') != -1 || index ($symbol_,'KFFTI') != -1 || index ($symbol_,'KFMFA') != -1 || index ($symbol_,'JFMFC') != -1)
	{
	    $symbol_to_currency_map_{$symbol_} = $EUR_TO_DOL;
	}
	else
	{
	    $symbol_to_currency_map_{$symbol_} = $GBP_TO_DOL;
	}
    }
    elsif ($exchange_name_ eq 'ICE')
    {
	if(index ($symbol_,'LFI') != -1)
	{
	    $symbol_to_currency_map_{$symbol_} = $EUR_TO_DOL;
	}
	else
	{
	    $symbol_to_currency_map_{$symbol_} = $GBP_TO_DOL;
	}

    }
    elsif ($exchange_name_ eq 'CME')
    {
	if(index ($symbol_,'NIY') != -1)
	{
	    $symbol_to_currency_map_{$symbol_} = $JPY_TO_DOL;
	}
	else
	{
	    $symbol_to_currency_map_{$symbol_} = 1;
	}
    }
    else
    {
	$symbol_to_currency_map_{$symbol_} = $curr;
    }
}

sub MarkExpiringProducts
{
#check if this shortcode is expiring in 2 days. Lets mark it.
    my $symbol_=shift;
    my $shc_=shift;
    my $attempt_ = 0 ;
    my $expiry_check_date_= $input_date_;
    while ( $attempt_ < 2 )
    {
	$expiry_check_date_ = CalcNextBusinessDay ( $expiry_check_date_ );
	$attempt_ ++ ;
    }
    if(index ( $symbol_, "NSE" ) == 0){
	my $expiry_date = `$PEXEC_BIN/option_details  "$shc_" $input_date_ | awk '{print \$1}'` ; chomp ( $expiry_date );
	if($expiry_date <= $expiry_check_date_) {
	    $symbol_to_isExpiring_map_{$symbol_}=1;
	}
    }
    else {
	my $shc_after_2days = `$PEXEC_BIN/get_shortcode_for_symbol  "$symbol_" $expiry_check_date_ ` ; chomp ( $shc_after_2days);
	if($shc_after_2days eq '') {
	    $symbol_to_isExpiring_map_{$symbol_}=1;
	}
    }
}

sub GetCurrencyConversion
{
    my $yesterday_date_ = CalcPrevBusinessDay ( $input_date_ );

    my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$yesterday_date_.'.txt' ;
    my $attempt_ = 1 ;
    while ( ! -e $curr_filename_ && $attempt_ < 10 )
    {
	$yesterday_date_ = CalcPrevBusinessDay ( $yesterday_date_ );
	$curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$yesterday_date_.'.txt' ;
	$attempt_ ++ ;
    }
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

	    if( index ( $currency_, "USDRUB" ) == 0 ){
		$RUB_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
	    }

	    if( index ( $currency_, "USDAUD" ) == 0 ){
		$AUD_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
	    }

	    if(index ( $currency_, "USDINR" ) == 0){
		$INR_TO_DOL = sprintf( "%.4f", 1 / $cwords_[1] );
	    }
	}
    }
}

sub LoadOvernightPositions
{
    my $yesterday_date_ = CalcPrevBusinessDay ( $input_date_ );

	my $overnight_pos_file="/spare/local/logs/pnl_data/$trade_type_/tag_pnl/EODPos/all_exch_overnight_pos_$yesterday_date_".".txt";
    if (-e $overnight_pos_file) {
	open OVN_FILE_HANDLE, "< $overnight_pos_file" or die "see_ors_pnl could not open ors_trades_filename_ $overnight_pos_file\n";

	my @ovn_file_lines_ = <OVN_FILE_HANDLE>;
	close OVN_FILE_HANDLE;
	for ( my $i = 0 ; $i <= $#ovn_file_lines_; $i ++ )
	{
	    my @words_ = split ( ',', $ovn_file_lines_[$i] );
	    if ( $#words_ >= 4 )
	    {
#load tagwise positions
		if($words_[0] eq "TAGWISE") {
		    my $tag = $words_[1];
		    my $symbol_ = $words_[2];
		    chomp($words_[3]);	#tag:shc pos
		    chomp($words_[4]);	#price
		    chomp($words_[5]); #product pos
		    if ( ( index ( $symbol_ , "NSE" ) == 0 ) ) {
		    	#Handling to ignore NSE overnight positions
		    	next;
				#$commission_rate_ = $symbol_to_price_map_{$symbol_} * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL;
		    }
		    if ($words_[5] == 0){next;} #Handling to prevent loading of products with net 0 positions
		    else{
			SetSecDef ( $symbol_ ) ; #Call SetSecDef iff Net Position of product in non-zero
		    }
#Load Product positions for computing Exchange numbers
		    $symbol_to_pos_map_{$symbol_} = $words_[5];
		    $symbol_to_price_map_{$symbol_} = $words_[4];
		    $symbol_to_volume_map_{$symbol_} = 0;
		    my $symbol_eod_price_ = $symbol_to_price_map_{$symbol_};
		    my $n2d_rate_ = $symbol_to_n2d_map_{$symbol_};
		    if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {
			$symbol_eod_price_ = -1 * ( 100000 / ( $symbol_to_price_map_{$symbol_} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) );
			$n2d_rate_ = $BR_TO_DOL;
		    }
		    my $commission_rate_ = $symbol_to_commish_map_{$symbol_};

		    $symbol_to_realpnl_map_{$symbol_}= -$symbol_to_pos_map_{$symbol_}* $symbol_eod_price_ * $n2d_rate_;

#Load Previous days positions to GBLHFT tag
		    my $tag_shc="GBLHFT".':'.$symbol_to_display_name_{$symbol_};
		    $tag_shc_to_pos_map{$tag_shc}=$words_[5];
		    $tag_shc_to_vol_map{$tag_shc}=0;
		    $tag_shc_to_realpnl_map{$tag_shc}= -$tag_shc_to_pos_map{$tag_shc} * $symbol_eod_price_ * $n2d_rate_;

# Don't Load Tag Specific Positions
#		    if(index ( $tag , "UNKNOWNTAG" ) == 0 ) {next;} #dont load overnight unknown positions
#		    my $tag_shc=$tag.':'.$symbol_to_display_name_{$symbol_};
#		    $tag_shc_to_pos_map{$tag_shc}=$words_[3];
#		    $tag_shc_to_vol_map{$tag_shc}=0;
#		    $tag_shc_to_realpnl_map{$tag_shc}= -$tag_shc_to_pos_map{$tag_shc} * $symbol_eod_price_ * $n2d_rate_;
#		    $tag_shc_to_unrealpnl_map{$tag_shc} = $tag_shc_to_pos_map{$tag_shc} * $symbol_eod_price_ * $n2d_rate_ - abs ($tag_shc_to_pos_map{$tag_shc}) * $commission_rate_;
#		    print "loadovn: $tag_shc $tag_shc_to_pos_map{$tag_shc} $symbol_to_price_map_{$symbol_} $tag_shc_to_realpnl_map{$tag_shc}\n";
		}
	    }
	}
    }
}
sub SplitTodTomSpread
{
    my ($tom_price_, $tod_price_);
    if ( exists $symbol_to_price_map_{ "USD000UTSTOM" } ) {
	$tom_price_ = $symbol_to_price_map_{ "USD000UTSTOM" };
	$tod_price_ = $tom_price_ - $symbol_to_price_map_{ "USD000TODTOM" };
    }
    elsif ( exists $symbol_to_price_map_{ "USD000000TOD" } ) {
	$tod_price_ = $symbol_to_price_map_{ "USD000000TOD" };
	$tom_price_ = $tod_price_ + $symbol_to_price_map_{ "USD000TODTOM" };
    }
    if ( defined $tom_price_ && defined $tod_price_ ) {
	my $todtom_proj_pos_ = 100 * $symbol_to_pos_map_{ "USD000TODTOM" };
	$symbol_to_pos_map_{ "USD000UTSTOM" } += $todtom_proj_pos_;
	$symbol_to_pos_map_{ "USD000000TOD" } -= $todtom_proj_pos_;

	$symbol_to_realpnl_map_{ "USD000UTSTOM" } += (-1 * $todtom_proj_pos_) * $tom_price_ * $symbol_to_n2d_map_{"USD000UTSTOM"} - abs($todtom_proj_pos_) * $symbol_to_commish_map_{"USD000UTSTOM"};
	$symbol_to_realpnl_map_{ "USD000000TOD" } += $todtom_proj_pos_ * $tod_price_ * $symbol_to_n2d_map_{"USD000000TOD"} - abs($todtom_proj_pos_) * $symbol_to_commish_map_{"USD000UTSTOM"};

	$symbol_to_volume_map_{ "USD000UTSTOM" } += abs($todtom_proj_pos_);
	$symbol_to_volume_map_{ "USD000000TOD" } += abs($todtom_proj_pos_);

	$symbol_to_pos_map_{ "USD000TODTOM" } = 0;
    }
}

sub SplitTodTomSpread_Tag
{
    my ($tom_price_, $tod_price_);
    foreach my $tag (sort keys %valid_tags){
        if(exists $tag_shc_to_pos_map{$tag.":USD000TODTOM"}){
            if ( exists $symbol_to_price_map_{ "USD000UTSTOM" } ) {
	        $tom_price_ = $symbol_to_price_map_{ "USD000UTSTOM" };
	        $tod_price_ = $tom_price_ - $symbol_to_price_map_{ "USD000TODTOM" };
            }
            elsif ( exists $symbol_to_price_map_{ "USD000000TOD" } ) {
                $tod_price_ = $symbol_to_price_map_{ "USD000000TOD" };
	        $tom_price_ = $tod_price_ + $symbol_to_price_map_{ "USD000TODTOM" };
            }
            if ( defined $tom_price_ && defined $tod_price_ ) {
                my $todtom_proj_pos_ = 100 * $tag_shc_to_pos_map{$tag.":USD000TODTOM" };
                $tag_shc_to_pos_map{$tag.":USD000UTSTOM" } += $todtom_proj_pos_;
                $tag_shc_to_pos_map{$tag.":USD000000TOD" } -= $todtom_proj_pos_;

	        $tag_shc_to_realpnl_map{$tag.":USD000UTSTOM" } += (-1 * $todtom_proj_pos_) * $tom_price_ * $symbol_to_n2d_map_{"USD000UTSTOM"} - abs($todtom_proj_pos_) * $symbol_to_commish_map_{"USD000UTSTOM"};
	        $tag_shc_to_realpnl_map{$tag.":USD000000TOD" } += $todtom_proj_pos_ * $tod_price_ * $symbol_to_n2d_map_{"USD000000TOD"} - abs($todtom_proj_pos_) * $symbol_to_commish_map_{"USD000UTSTOM"};

                $tag_shc_to_vol_map{$tag.":USD000UTSTOM" } += abs($todtom_proj_pos_);
	        $tag_shc_to_vol_map{$tag.":USD000000TOD" } += abs($todtom_proj_pos_);
	        $tag_shc_to_vol_map{$tag.":USD000TODTOM" } = 0;
            }
        }
    }
}
sub TransferNKPosToSpread_Tag()
{
    my $shc_0 = "SGX_NK_0";
    my $shc_1 = "SGX_NK_1";
    my $spread_exch_sym= `$PEXEC_BIN/get_exchange_symbol "SP_SGX_NK0_NK1"  $input_date_`; chomp ($spread_exch_sym); #spread symbol
    my $exch_sym_0= `$PEXEC_BIN/get_exchange_symbol $shc_0 $input_date_`; chomp ($exch_sym_0);
    my $exch_sym_1= `$PEXEC_BIN/get_exchange_symbol $shc_1 $input_date_`; chomp ($exch_sym_1);
    foreach my $tag (sort keys %valid_tags){
	if (exists ($tag_shc_to_pos_map{$tag.":".$exch_sym_0}) && exists ($tag_shc_to_pos_map{$tag.":".$exch_sym_1})) {
	    my $max_pos=0;
	    if($tag_shc_to_pos_map{$tag.":".$exch_sym_0} *  $tag_shc_to_pos_map{$tag.":".$exch_sym_1} <0)
	    {
		if(abs($tag_shc_to_pos_map{$tag.":".$exch_sym_0}) >= abs($tag_shc_to_pos_map{$tag.":".$exch_sym_1})) {	## 16 & -15, transfer -15 to spread | ## -16 & 15, transfer 15 to spread
		    $max_pos=$tag_shc_to_pos_map{$tag.":".$exch_sym_1};
		    $tag_shc_to_pos_map{$tag.":".$exch_sym_0}+=$tag_shc_to_pos_map{$tag.":".$exch_sym_1};
		    $tag_shc_to_pos_map{$tag.":".$exch_sym_1}=0;
		}
		else {					## 16 & -18, transfer -16 to spread | ## -16 & 18
		    $max_pos=-1*$tag_shc_to_pos_map{$tag.":".$exch_sym_0};
		    $tag_shc_to_pos_map{$tag.":".$exch_sym_1}+=$tag_shc_to_pos_map{$tag.":".$exch_sym_0};
		    $tag_shc_to_pos_map{$tag.":".$exch_sym_0}=0;
		}
	    }
	    #transfer this max position to spread contract
	    if(exists ($tag_shc_to_pos_map{$tag.":".$spread_exch_sym}))
	    {
		$tag_shc_to_pos_map{$tag.":".$spread_exch_sym}+=$max_pos;
	    }
	    else {
		$tag_shc_to_pos_map{$tag.":".$spread_exch_sym}=$max_pos;
	    }
	}
    }
}
sub TransferNKPosToSpread()
{
    my $shc_0 = "SGX_NK_0";
    my $shc_1 = "SGX_NK_1";
    my $exch_sym_0= `$PEXEC_BIN/get_exchange_symbol $shc_0 $input_date_`; chomp ($exch_sym_0);
    my $exch_sym_1= `$PEXEC_BIN/get_exchange_symbol $shc_1 $input_date_`; chomp ($exch_sym_1);
    if (exists ($symbol_to_pos_map_{$exch_sym_0}) && exists ($symbol_to_pos_map_{$exch_sym_1} )) {
	my $max_pos=0;
	if($symbol_to_pos_map_{$exch_sym_0} *  $symbol_to_pos_map_{$exch_sym_1} <0)
	{
	    if(abs($symbol_to_pos_map_{$exch_sym_0}) >= abs($symbol_to_pos_map_{$exch_sym_1})) {	## 16 & -15, transfer -15 to spread | ## -16 & 15, transfer 15 to spread
		$max_pos=$symbol_to_pos_map_{$exch_sym_1};
		$symbol_to_pos_map_{$exch_sym_0}+=$symbol_to_pos_map_{$exch_sym_1};
		$symbol_to_pos_map_{$exch_sym_1}=0;
	    }
	    else {					## 16 & -18, transfer -16 to spread | ## -16 & 18
		$max_pos=-1*$symbol_to_pos_map_{$exch_sym_0};
		$symbol_to_pos_map_{$exch_sym_1}+=$symbol_to_pos_map_{$exch_sym_0};
		$symbol_to_pos_map_{$exch_sym_0}=0;
	    }
	}
	#transfer this max position to spread contract
	my $spread_exch_sym= `$PEXEC_BIN/get_exchange_symbol "SP_SGX_NK0_NK1"  $input_date_`; chomp ($spread_exch_sym); #spread symbol
	if(exists ($symbol_to_pos_map_{$spread_exch_sym}))
	{
	    $symbol_to_pos_map_{$spread_exch_sym}+=$max_pos;
	}
	else {
	    $symbol_to_pos_map_{$spread_exch_sym}=$max_pos;
	}
    }
}
sub TransferVXSpreadPos()
{
	foreach my $sym_tmp (sort keys %symbol_to_pos_map_){
		if(exists ($spread_to_pos_symbol_{$sym_tmp})){
		    if(!exists($symbol_to_pos_map_{$spread_to_neg_symbol_{$sym_tmp}}) && (!exists($symbol_to_pos_map_{$spread_to_pos_symbol_{$sym_tmp}}))){
		    	return;
		    }
		    if(!exists($symbol_to_pos_map_{$spread_to_pos_symbol_{$sym_tmp}}) && (exists($symbol_to_pos_map_{$spread_to_neg_symbol_{$sym_tmp}}))){
		    	SetSecDef($spread_to_pos_symbol_{$sym_tmp});
				$symbol_to_realpnl_map_{$spread_to_pos_symbol_{$sym_tmp}} = 0;
				$symbol_to_price_map_{$spread_to_pos_symbol_{$sym_tmp}} = 0;
				$symbol_to_volume_map_{$spread_to_pos_symbol_{$sym_tmp}} = 0;
				$symbol_to_total_commish_map_{$spread_to_pos_symbol_{$sym_tmp}} = 0;
		    	$symbol_to_pos_map_{$spread_to_pos_symbol_{$sym_tmp}} = 0;
		    	$symbol_to_price_map_{$spread_to_pos_symbol_{$sym_tmp}}  = $symbol_to_price_map_{$sym_tmp} + $symbol_to_price_map_{$spread_to_neg_symbol_{$sym_tmp}};
		    }
		   elsif(!exists($symbol_to_pos_map_{$spread_to_neg_symbol_{$sym_tmp}}) && (exists($symbol_to_pos_map_{$spread_to_pos_symbol_{$sym_tmp}}))){
		    	SetSecDef($spread_to_neg_symbol_{$sym_tmp});
				$symbol_to_realpnl_map_{$spread_to_pos_symbol_{$sym_tmp}} = 0;
				$symbol_to_price_map_{$spread_to_pos_symbol_{$sym_tmp}} = 0;
				$symbol_to_volume_map_{$spread_to_pos_symbol_{$sym_tmp}} = 0;
				$symbol_to_total_commish_map_{$spread_to_pos_symbol_{$sym_tmp}} = 0;
		    	$symbol_to_pos_map_{$spread_to_pos_symbol_{$sym_tmp}} = 0;
		    	$symbol_to_price_map_{$spread_to_neg_symbol_{$sym_tmp}} = $symbol_to_price_map_{$spread_to_pos_symbol_{$sym_tmp}} - $symbol_to_price_map_{$sym_tmp};
		   }
		    $symbol_to_pos_map_{$spread_to_pos_symbol_{$sym_tmp}}+=$symbol_to_pos_map_{$sym_tmp};
		    $symbol_to_pos_map_{$spread_to_neg_symbol_{$sym_tmp}}-=$symbol_to_pos_map_{$sym_tmp};
		    $symbol_to_pos_map_{$sym_tmp} = 0;
		}
	}
}

sub DumpOrsPositions
{
#dump positions only in tag mode: 2 different instances running T and Q mode

    #Special Handling for TMX Pnls
    my @instr_              = ( "BAX", "CGB", "CGF", "SXF", "CGZ" );
    my %tag_prod_to_position_   = ();
    my %prod_to_position_ = ();
    my %prod_to_last_price_ = ();
    TransferNKPosToSpread ();
    TransferNKPosToSpread_Tag ();
    TransferVXSpreadPos();
    foreach my $tag_shc ( sort keys %tag_shc_to_pos_map ){
	if($tag_shc_to_pos_map{$tag_shc} != 0)
	{
	    my @split_tag_words = split ( ':', $tag_shc );
	    my $tag;
	    my $symbol;
	    if ( $#split_tag_words == 1 ) {
		$tag=$split_tag_words[0];
		$symbol=$split_tag_words[1];
		my $exchange_sym=$display_name_to_symbol_{$symbol};
     		if (exists ($spread_to_pos_symbol_{$exchange_sym})){
     			if(exists($symbol_to_pos_map_{$spread_to_pos_symbol_{$exchange_sym}}) && exists($symbol_to_pos_map_{$spread_to_neg_symbol_{$exchange_sym}})){
					#This check is only valid iff TransferVXSpreadPos() is called before.
					#Checks whether we have trasfered the spread positions to legs.
     			if(!exists($tag_shc_to_pos_map{$tag.':'.$spread_to_pos_symbol_{$exchange_sym}})){
		    		$tag_shc_to_pos_map{$tag.':'.$spread_to_pos_symbol_{$exchange_sym}} = 0;
		    	}
		    	if(!exists($tag_shc_to_pos_map{$tag.':'.$spread_to_neg_symbol_{$exchange_sym}})){
		    		$tag_shc_to_pos_map{$tag.':'.$spread_to_neg_symbol_{$exchange_sym}} = 0;
		    	}
		    $tag_shc_to_pos_map{$tag.':'.$spread_to_pos_symbol_{$exchange_sym}}+=$tag_shc_to_pos_map{$tag.':'.$exchange_sym};
		    $tag_shc_to_pos_map{$tag.':'.$spread_to_neg_symbol_{$exchange_sym}}-=$tag_shc_to_pos_map{$tag.':'.$exchange_sym};
		    $tag_shc_to_pos_map{$tag.':'.$exchange_sym}=0;
		}
     	}
		if (index($exchange_sym, "NK") == 0 )
		{
		    foreach my $tag_shc_1 ( sort keys %tag_shc_to_pos_map )
		    {
			my @split_tag_words_1 = split ( ':', $tag_shc );
			my $tag_1;
			my $symbol_1_;
			if ( $#split_tag_words_1 == 1 ) {
			    $tag_1=$split_tag_words_1[0];
			    $symbol_1_=$split_tag_words[1];
			    my $exchange_sym_1=$display_name_to_symbol_{$symbol_1_};
			    if(index($tag, $tag_1)==0){     #check if the tags match
				if (index ($exchange_sym_1,"NKM") == 0)
				{
				    if (substr($exchange_sym,2,4) eq substr($exchange_sym_1,3,4))
				    {
					$symbol_to_pos_map_{$exchange_sym_1} += 10*$symbol_to_pos_map_{$exchange_sym};
					$symbol_to_pos_map_{$exchange_sym} = 0;
					$tag_shc_to_pos_map{$tag_1.':'.$exchange_sym_1}+=10*$tag_shc_to_pos_map{$tag.':'.$exchange_sym};
					$tag_shc_to_pos_map{$tag.':'.$exchange_sym} = 0;
				    }
				}
			    }
			}
		    }
		}
		if (index($exchange_sym, "DOL") == 0 )
    		{
		    foreach my $tag_shc_1 ( sort keys %tag_shc_to_pos_map )
		    {
			my @split_tag_words_1 = split ( ':', $tag_shc );
			my $tag_1;
			my $symbol_1_;
			if ( $#split_tag_words_1 == 1 ) {
			    $tag_1=$split_tag_words_1[0];
			    $symbol_1_=$split_tag_words[1];
			    my $exchange_sym_1=$display_name_to_symbol_{$symbol_1_};
			    if(index($tag, $tag_1)==0){     #check if the tags match
				if (index ($exchange_sym_1,"WDO") == 0)
				{
				    if (substr($exchange_sym,3,3) eq substr($exchange_sym_1,3,3))
				    {
					$symbol_to_pos_map_{$exchange_sym_1} += 5*$symbol_to_pos_map_{$exchange_sym};
					$symbol_to_pos_map_{$exchange_sym} = 0;
					$tag_shc_to_pos_map{$tag_1.':'.$exchange_sym_1}+=10*$tag_shc_to_pos_map{$tag.':'.$exchange_sym};
					$tag_shc_to_pos_map{$tag.':'.$exchange_sym} = 0;
				    }
				}
			    }
			}
		    }
    		}
		if (index($exchange_sym, "IND") == 0 )
    		{
		    foreach my $tag_shc_1 ( sort keys %tag_shc_to_pos_map )
		    {
			my @split_tag_words_1 = split ( ':', $tag_shc );
			my $tag_1;
			my $symbol_1_;
			if ( $#split_tag_words_1 == 1 ) {
			    $tag_1=$split_tag_words_1[0];
			    $symbol_1_=$split_tag_words[1];
			    my $exchange_sym_1=$display_name_to_symbol_{$symbol_1_};
			    if(index($tag, $tag_1)==0){     #check if the tags match
				if (index ($exchange_sym_1,"WIN") == 0)
				{
				    if (substr($exchange_sym,3,3) eq substr($exchange_sym_1,3,3))
				    {
					if($tag_shc_to_pos_map{$tag_1.':'.$exchange_sym_1} == -5*$tag_shc_to_pos_map{$tag.':'.$exchange_sym}){
					    $tag_shc_to_pos_map{$tag_1.':'.$exchange_sym_1} = 0;
					    $tag_shc_to_pos_map{$tag.':'.$exchange_sym} = 0;
					}
				    }
				}
			    }
			}
		    }
    		}
    		foreach my $prod_ (@instr_) {
		    if ( index( $exchange_sym, $prod_ ) >= 0 ) {
			my $count_ = () = $exchange_sym =~ /$prod_/g;
			if ( $count_ == 1 ) {
			    if ( exists $tag_prod_to_position_{$tag.':'.$exchange_sym} ) {
				$tag_prod_to_position_{$tag.':'.$exchange_sym} += $tag_shc_to_pos_map{$tag.':'.$exchange_sym};
				$prod_to_last_price_{$tag.':'.$exchange_sym} = $symbol_to_price_map_{$exchange_sym};
			    }
			    else {
				# should not come here
				$tag_prod_to_position_{$tag.':'.$exchange_sym} = $tag_shc_to_pos_map{$tag.':'.$exchange_sym};
				$prod_to_last_price_{$tag.':'.$exchange_sym} = $symbol_to_price_map_{$exchange_sym};
			    }
			}
			elsif ( $count_ == 2 ) {
			    # BAXM15BAXU15 => BAXM5, BAXU5
			    my $first_len_ = length($prod_) + 3;
			    my $prod1_ = substr( $exchange_sym, 0, length($prod_) + 1 ).substr( $exchange_sym, length($prod_) + 2, 1 );
			    if ( exists $tag_prod_to_position_{$prod1_} ) {
				$tag_prod_to_position_{$prod1_} += int($tag_shc_to_pos_map{$tag.':'.$exchange_sym});
			    }
			    else { $tag_prod_to_position_{$prod1_} = int($tag_shc_to_pos_map{$tag.':'.$exchange_sym}); }
			    my $prod2_ = substr( $exchange_sym, $first_len_, length($prod_) + 1 ).substr( $exchange_sym, $first_len_ + length($prod_) + 2,1 );
			    if ( exists $tag_prod_to_position_{$prod2_} ) {
				$tag_prod_to_position_{$prod2_} -= int($tag_shc_to_pos_map{$tag.':'.$exchange_sym});
			    }
			    else { $tag_prod_to_position_{$prod2_} = -int( $tag_shc_to_pos_map{$tag.':'.$exchange_sym}); }
			}
			$tag_shc_to_pos_map{$tag.':'.$exchange_sym} = 0; #Settle the postion in the tag_shc_to_pos_map
		    }
		}
	    }
	}
    }
    foreach my $symbol_1 (keys %symbol_to_pos_map_){ #Spreads to legs
      if (index($symbol_1, "NK") == 0 ){
	    foreach my $symbol_2( sort keys %symbol_to_pos_map_)
	    {
		if (index ($symbol_2,"NKM") == 0){
		    if (substr($symbol_1,2,4) eq substr($symbol_2,3,4))
		    {
			$symbol_to_pos_map_{$symbol_2} += 10*$symbol_to_pos_map_{$symbol_1};
			$symbol_to_pos_map_{$symbol_1} = 0;
		    }
		}
	    }
	}
	if (index($symbol_1, "DOL") == 0 ){
	    foreach my $symbol_2 ( sort keys %symbol_to_pos_map_){
		if (index ($symbol_2,"WDO") == 0)
		{
		    if (substr($symbol_1,3,3) eq substr($symbol_2,3,3))
		    {
			$symbol_to_pos_map_{$symbol_2} += 5*$symbol_to_pos_map_{$symbol_1};
			$symbol_to_pos_map_{$symbol_1} = 0;
		    }
		}
	    }
    	}
    	if (index($symbol_1, "IND") == 0 )
    	{
	    foreach my $symbol_2 ( sort keys %symbol_to_pos_map_)
	    {
		if (index ($symbol_2,"WIN") == 0)
		{
		    if (substr($symbol_1,3,3) eq substr($symbol_2,3,3))
		    {
			if ( $symbol_to_pos_map_{$symbol_2} == -5*$symbol_to_pos_map_{$symbol_1}  )
			{
			    $symbol_to_pos_map_{$symbol_1} = 0;
			    $symbol_to_pos_map_{$symbol_2} = 0;
			}
		    }
		}
	    }
    	}
    	foreach my $prod_ (@instr_) {
	    if ( index( $symbol_1, $prod_ ) >= 0 ) {
		my $count_ = () = $symbol_1 =~ /$prod_/g;
		if ( $count_ == 1 ) {
		    if ( exists $prod_to_position_{$symbol_1} ) {
			$prod_to_position_{$symbol_1} += $symbol_to_pos_map_{$symbol_1};
			$prod_to_last_price_{$symbol_1} = $symbol_to_price_map_{$symbol_1};
		    }
		    else {
			# should not come here
			$prod_to_position_{$symbol_1} = $symbol_to_pos_map_{$symbol_1};
			$prod_to_last_price_{$symbol_1} = $symbol_to_price_map_{$symbol_1};
		    }
		}
		elsif ( $count_ == 2 ) {
		    # BAXM15BAXU15 => BAXM5, BAXU5
		    my $first_len_ = length($prod_) + 3;
		    my $prod1_ = substr( $symbol_1, 0, length($prod_) + 1 ).substr( $symbol_1, length($prod_) + 2, 1 );
		    if ( exists $prod_to_position_{$prod1_} ) {
			$prod_to_position_{$prod1_} += int($symbol_to_pos_map_{$symbol_1});
		    }
		    else { $prod_to_position_{$prod1_} = int($symbol_to_pos_map_{$symbol_1}); }
		    my $prod2_ = substr( $symbol_1, $first_len_, length($prod_) + 1 ).substr( $symbol_1, $first_len_ + length($prod_) + 2,1 );
		    if ( exists $prod_to_position_{$prod2_} ) {
			$prod_to_position_{$prod2_} -= int($symbol_to_pos_map_{$symbol_1});
		    }
		    else { $prod_to_position_{$prod2_} = -int( $symbol_to_pos_map_{$symbol_1}); }
		}
	    }
    	}
    }
    foreach my $tag_shc ( sort keys %tag_shc_to_pos_map ){

	my @split_tag_words = split ( ':', $tag_shc );
	my $tag;
	my $symbol;
	if ( $#split_tag_words == 1 ) {
	$tag=$split_tag_words[0];
	$symbol=$split_tag_words[1];
	my $exchange_sym=$display_name_to_symbol_{$symbol};
	if($tag_shc_to_pos_map{$tag_shc} != 0 || $symbol_to_pos_map_{$exchange_sym} != 0)
	{
		if (exists ($spread_to_pos_symbol_{$exchange_sym})){
		    next;
		}
		else{
		    if(($tag_shc_to_pos_map{$tag_shc} != 0 || $symbol_to_pos_map_{$exchange_sym} != 0|| index ($exchange_sym, "BAX") != -1)){
			print "TAGWISE,$tag,$exchange_sym,$tag_shc_to_pos_map{$tag_shc},$symbol_to_price_map_{$exchange_sym},$symbol_to_pos_map_{$exchange_sym}\n";
		    }
		}
	    }
	}
    }

    foreach my $tag_shc ( sort keys %tag_prod_to_position_ ){
	my @split_tag_words = split ( ':', $tag_shc );
	my $tag;
	my $symbol;
	if ( $#split_tag_words == 1 ) {
	    $tag=$split_tag_words[0];
	    $symbol=$split_tag_words[1];
	    my $exchange_sym=$display_name_to_symbol_{$symbol};
	    print "TAGWISE,$tag,$exchange_sym,$tag_prod_to_position_{$tag_shc},$prod_to_last_price_{$tag_shc},$symbol_to_pos_map_{$exchange_sym}\n";
	}
    }
    foreach my $sym_temp (sort keys %symbol_to_pos_map_){
		SendSlackForExpiringContractsWithPositions ($sym_temp);
    }
}

sub SendSlackForExpiringContractsWithPositions ()
{
    my $symbol = shift;
    my $shc_ = `$PEXEC_BIN/get_shortcode_for_symbol  "$symbol" $input_date_ ` ; chomp ( $shc_ );

    if(exists ($symbol_to_isExpiring_map_{$symbol}) && (exists ($symbol_to_pos_map_{$symbol})) && ($symbol_to_pos_map_{$symbol}!=0)){
	my $alert_str="Expiring product with position: $symbol $shc_ $symbol_to_pos_map_{$symbol}";
	`$PEXEC_BIN/send_slack_notification prod-issues DATA "$alert_str"`;
    }
}

sub Print
{
    my $mode=$_[0];
    my $value=$_[1];
    printf "| PNL : ";

    if($mode eq 'C'){
	print color("BOLD");
	print color("reset");
	if ($value < 0) {
	    print color("red"); print color("BOLD");
	}
	else {
	    print color("blue"); print color("BOLD");
	}
	printf "%10.3f ", $value;
	print color("reset");
    }
    else{
	printf "%10.3f ", $value;
    }
}
