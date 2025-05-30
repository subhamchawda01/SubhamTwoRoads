#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;
use File::Find;
use List::Util qw/max min/; # for max
use Data::Dumper;
#use autodie;

my $PSCRIPTS_DIR="/home/pengine/prod/live_scripts/";
my $PEXEC_DIR="/home/pengine/prod/live_execs";

require "$PSCRIPTS_DIR/calc_prev_business_day.pl"; # 
require "$PSCRIPTS_DIR/calc_next_business_day.pl";
use Term::ANSIColor;

my $hostname_ = `hostname`;

#my $exchange_to_unrealpnl_map_;
#C - colour, R - No colour, E - without native commission in native currency, W - with native commision in native currency, P - get commissions in native currency for every symbol

#in E & W mode, only individual symbol's pnl is in native currency. The exchange pnl is still in USD since there is no particular native currency for a exchange

my $USAGE="$0 mode trade_type(H/M) YYYYMMDD pnl_type(Q/T) [dont_load_over_night]";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $mode_ = shift;
#my $today_date_ = `date  --date='tomorrow' +"%Y%m%d"` ; chomp ( $today_date_ ) ;
my $trade_type_ = shift || '';
my $today_date_ = `date +"%Y%m%d"` ; chomp ( $today_date_ ) ;
my $input_date_ =  shift || $today_date_ ;
my $EXCH_ = '';
my $PNL_SHOW_MODE= shift || 'T';
my $LOAD_ = shift || ''; 
my $MAX_PAGE_SIZE_ = shift || 40;

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

#================== calculating remaining days in expiry for DI products =================#
my %symbol_to_noof_working_days_till_expiry_ = ( );

#=============================================================#

#================== for certain pairs the price for both should remain the same ===========#
#E.g.: {BR_DOL_0, BR_WDO_0}  
my %same_price_shc_map_ = ( );

my $secname1_ = `$PEXEC_DIR/get_exchange_symbol BR_WDO_0 $input_date_`;
my $secname2_ = `$PEXEC_DIR/get_exchange_symbol BR_DOL_0 $input_date_`;
$same_price_shc_map_{ $secname1_ } = $secname2_;
$same_price_shc_map_{ $secname2_ } = $secname1_;

$secname1_ = `$PEXEC_DIR/get_exchange_symbol BR_WIN_0 $input_date_`;
$secname2_ = `$PEXEC_DIR/get_exchange_symbol BR_IND_0 $input_date_`;
$same_price_shc_map_{ $secname1_ } = $secname2_;
$same_price_shc_map_{ $secname2_ } = $secname1_;
#=============================================================#

my %symbol_to_commish_map_ = ();
my %symbol_to_n2d_map_ = ();

my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_lots_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();
my %symbol_to_display_name_ = ();
my %display_name_to_symbol_= ();

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

#tag-query pnl maps
my %tag_shc_to_realpnl_map = ();
my %tag_shc_to_unrealpnl_map = ();
my %tag_shc_to_vol_map = ();
my %tag_shc_to_pos_map = ();
my %query_shc_to_realpnl_map = ();
my %query_shc_to_unrealpnl_map = ();
my %query_shc_to_vol_map = ();
my %query_shc_to_pos_map = ();
my %tag_to_pnl_map_ = ();
my %tag_to_vol_map_ = ();
my %query_to_pnl_map_ = ();
my %query_to_vol_map_ = ();

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

my %server_to_last_processed_time_ = ();
$server_to_last_processed_time_{"sdv-ind-srv12"} = 0; #10.23.115.62
$server_to_last_processed_time_{"sdv-ind-srv11"} = 0; #10.23.115.61
$server_to_last_processed_time_{"sdv-ind-srv13"} = 0;

#================== Load overnight positions only when no argument is provided for the same =================#
if ($LOAD_ eq '') {
    LoadOvernightPositions ( )
}
#=============================================================================================================#
#pnly process these tags
my %tags_to_process =();

my $time_eod_trades_=21000;
my $time_start_trades_=33000;
my $cron_time_ = 210800;
my $time_AS_email_ = 61000;
my $time_EU_email_ = 120000;
my $delta_dir="/spare/local/logs/pnl_data/$trade_type_/delta_files/";
my $temp_email_pnl_file="/spare/local/logs/pnl_data/$trade_type_/temp_email_ors_pnl_.txt";
#my $eod_date_ = `date  --date='tomorrow' +"%Y%m%d"` ; chomp ( $eod_date_ ) ;

sub LoadConfigFile {
    my $config="/spare/local/logs/pnl_data/hft/nse_pnl_config";
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
	}
    }
}

sub LoadTagsSaciQidInfo {
    my $server=shift;
    my $tag_saci_qid_info_file="/spare/local/logs/risk_logs/qid_saci_tag";
    open TAGS_SACI_QID_FILEHANDLE, "< $tag_saci_qid_info_file" or print " could not open $tag_saci_qid_info_file\n";
    my @saci_tags_qid_data= <TAGS_SACI_QID_FILEHANDLE>;
    close TAGS_SACI_QID_FILEHANDLE;
    for ( my $i = 0 ; $i <= $#saci_tags_qid_data; $i ++ )
    {
	my @words_ = split ( ' ', $saci_tags_qid_data[$i] );
#print "#words_ $#words_ \n";
	if ( $#words_ == 2 )
	{
	    my $qid = $words_[0];
	    my $saci = $words_[1];
	    my $tags = $words_[2];
	   
            if(! exists $saci_to_tags_map{$saci}){
               $saci_to_qid_map{$saci}=$qid;
	       $saci_to_tags_map{$saci}=$tags;
            }
	    
#if qid_tag map is available for this product, update the maps now
	    if($saci_to_tags_map{$saci} eq "UNKNOWNTAG_".$saci) 
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
			    my @tags_for_saci = split ( ':', $tags );
			    if ( $#tags_for_saci >= 0 )
			    {
				for ( my $i = 0 ; $i <= $#tags_for_saci; $i++) 		
				{
				    if(! exists $tags_to_process{$tags_for_saci[$i]}) {next;}
				    print "debug: this tag_shc $tags_for_saci[$i].':'.$symbol \n";
				    $tag_shc_to_realpnl_map{$tags_for_saci[$i].":".$symbol} += $tag_shc_to_realpnl_map{$tag_shc};

				    $tag_shc_to_vol_map{$tags_for_saci[$i].":".$symbol} += $tag_shc_to_vol_map{$tag_shc};
				    $tag_shc_to_pos_map{$tags_for_saci[$i].":".$symbol}  += $tag_shc_to_pos_map{$tag_shc};
				    my $commission_rate_ = $symbol_to_price_map_{$exchange_symbol} * $symbol_to_commish_map_{$exchange_symbol} * $INR_TO_DOL;
				    $tag_shc_to_unrealpnl_map{$tags_for_saci[$i].":".$symbol} = $tag_shc_to_pos_map{$tags_for_saci[$i].":".$symbol} * $symbol_to_price_map_{$exchange_symbol} * $symbol_to_n2d_map_{$exchange_symbol} - abs ($tag_shc_to_pos_map{$tags_for_saci[$i].":".$symbol}) * $commission_rate_;	
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
	    if($saci_to_qid_map{$saci} eq "UNKNOWNQID_".$saci) 
	    {
		$saci_to_qid_map{$saci}=$qid;
		foreach my $query_shc ( sort keys %query_shc_to_realpnl_map ){
		    my @split_query_words = split ( ':', $query_shc );
		    my $query;
		    my $symbol;
		    if ( $#split_query_words == 1 ) {
			$query=$split_query_words[0];
			$symbol=$split_query_words[1];
			my $exchange_symbol=$display_name_to_symbol_{$symbol};
#print "query: $query shc: $symbol \n";
			if( $query eq "UNKNOWNQID_".$saci)
			{
			    $query_shc_to_realpnl_map{$qid.":".$symbol} += $query_shc_to_realpnl_map{$query_shc};

			    $query_shc_to_vol_map{$qid.":".$symbol} += $query_shc_to_vol_map{$query_shc};
			    $query_shc_to_pos_map{$qid.":".$symbol}  += $query_shc_to_pos_map{$query_shc};
			    my $commission_rate_ = $symbol_to_price_map_{$exchange_symbol} * $symbol_to_commish_map_{$exchange_symbol} * $INR_TO_DOL;
			    $query_shc_to_unrealpnl_map{$qid.":".$symbol} = $query_shc_to_pos_map{$qid.":".$symbol} * $symbol_to_price_map_{$exchange_symbol} * $symbol_to_n2d_map_{$exchange_symbol} - abs ($query_shc_to_pos_map{$qid.":".$symbol}) * $commission_rate_;

#dont need this unknownquery entries anymore
			    delete $query_shc_to_realpnl_map{$query_shc};
			    delete $query_shc_to_vol_map{$query_shc};
			    delete $query_shc_to_pos_map{$query_shc};
			}
		    }	
		}
	    } 	
	}
    }
}

#LoadConfigfile for start, end time, tags to process etc
LoadConfigFile ();
#make STDOUT HOT so that nothing is buffered
select STDOUT;
$| = 1;

while(1)
{
    LoadTagsSaciQidInfo ("unknown"); 
    my @delta_files_to_process;
    foreach my $server ( sort keys %server_to_last_processed_time_ )
    { 
	LoadTagsSaciQidInfo ($server);
	my $server_last_processed_time_=$server_to_last_processed_time_{$server};

	my @delta_file_times = `ls $delta_dir |  awk -F"_" '{if (\$2 == "$server") print \$3}'` ;
	foreach my $file_time (@delta_file_times)
	{
	    chomp ($file_time);
	    if ( int($file_time) > $server_to_last_processed_time_{$server})
	    { 
		my @temp_file_;
		find(sub  { push @temp_file_ , $File::Find::name if ( m/^(.*)$server/ and m/^(.*)$file_time$/ ) }, $delta_dir);
		@delta_files_to_process = ( @delta_files_to_process, @temp_file_);
	    }
	    if( int($file_time) > $server_last_processed_time_)
	    {
		$server_last_processed_time_=int($file_time);
	    }
	}
	$server_to_last_processed_time_{$server} = $server_last_processed_time_;
    }

    my $total_index_fut_win_ind_volume_ = 0.0 ;
    foreach my $ors_trades_filename_ (@delta_files_to_process)
    {
	open ORS_TRADES_FILE_HANDLE, "< $ors_trades_filename_" or die "calc_ors_pnl.pl could not open ors_trades_filename_ $ors_trades_filename_\n";
	my @ors_trades_file_lines_ = <ORS_TRADES_FILE_HANDLE>;
	close ORS_TRADES_FILE_HANDLE;

	for ( my $i = 0 ; $i <= $#ors_trades_file_lines_; $i ++ )
	{
#print " ors_trades_file_lines_:  $ors_trades_file_lines_[$i]\n";
	    my @words_ = split ( '', $ors_trades_file_lines_[$i] );
#print "#words_ $#words_ \n";
	    if ( $#words_ >= 7 )
	    {
		my $symbol_ = $words_[0];
		my $buysell_ = $words_[1];
		my $tsize_ = $words_[2];
		my $tprice_ = $words_[3];
		my $saos_ = $words_[4];
		my $timestamp = $words_[6];
		my $saci = $words_[7];

		if ( ! ( exists $symbol_to_unrealpnl_map_{$symbol_} ) )
		{
		    $symbol_to_unrealpnl_map_{$symbol_} = 0;
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

		$symbol_to_pos_map_{$symbol_} += ($buysell_ == 0) ? $tsize_ : (-1 * $tsize_);
		$symbol_to_volume_map_{$symbol_} += $tsize_;
		$symbol_to_price_map_{$symbol_} = $tprice_;

		if ( exists $same_price_shc_map_{ $symbol_ } ) {
		    my $tsymbol_ = $same_price_shc_map_{ $symbol_ };
		    if ( exists $symbol_to_unrealpnl_map_{ $tsymbol_ } ) {
			$symbol_to_price_map_{ $tsymbol_ } = $tprice_;
		    }
		}

		if ( index ( $symbol_ , "USD000TODTOM" ) == 0 ) {
		    SplitTodTomSpread ( );
		    next;
		}
		my $this_trade_pnl;
		if ( $buysell_ == 0 )
		{ # buy
		    if ( index ( $symbol_ , "DI" ) == 0 ) {
			$symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
			$this_trade_pnl = $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
		    }
		    else {
			$symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
			$this_trade_pnl = -1 * $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
		    }
		}
		else
		{
		    if ( index ( $symbol_ , "DI" ) == 0 ) {
			$symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
			$this_trade_pnl = -1 * $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
		    }
		    else {
			$symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
			$this_trade_pnl = $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
		    }
		}

		if(( index ( $symbol_ , "NSE" ) == 0 )){
		    $symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $tprice_ * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL);
		    $this_trade_pnl -= ($tsize_ * $tprice_ * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL);
		    $symbol_to_total_commish_map_{$symbol_} +=  ($tsize_ * $tprice_ * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL);
		}
		else{
		    $symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
		    $this_trade_pnl -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
		    $symbol_to_total_commish_map_{$symbol_} +=  $tsize_ * $symbol_to_commish_map_{$symbol_};     
		}

#account for open positions
		if( index ( $symbol_to_exchange_map_{$symbol_}, "BMF" ) == 0 )
		{
		    if ( index ( $symbol_, "IND" ) == 0 ) { $total_index_fut_win_ind_volume_ += $tsize_ * 5 ; }
		    if ( index ( $symbol_, "WIN" ) == 0 ) { $total_index_fut_win_ind_volume_ += $tsize_ ; }
		}

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

		my $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_eod_price_ * $n2d_rate_ - abs ($symbol_to_pos_map_{$symbol_}) * $commission_rate_;
# BMF has different tiers of discount for HFT commission
		if( index ( $symbol_to_exchange_map_{$symbol_}, "BMF" ) == 0 ){
		    my $shortcode_ = "";

		    if ( index ( $symbol_, "DOL" ) == 0 ) {
			$shortcode_ = "DOL" ;
			$unreal_pnl_ += ( $symbol_to_volume_map_{$symbol_} * GetHFTDiscount( $shortcode_, $symbol_to_volume_map_{$symbol_} ) * $BR_TO_DOL ) ;
		    }
		    if ( index ( $symbol_, "WDO" ) == 0 ) {
			$shortcode_ = "WDO" ;
			$unreal_pnl_ += ( $symbol_to_volume_map_{$symbol_} * GetHFTDiscount( $shortcode_, $symbol_to_volume_map_{$symbol_} ) * $BR_TO_DOL ) ;
		    }
		    if ( index ( $symbol_, "WIN" ) == 0 ) {
			$shortcode_ = "WIN" ;
			$unreal_pnl_ += ( $symbol_to_volume_map_{$symbol_} * GetHFTDiscount( $shortcode_, $total_index_fut_win_ind_volume_ ) * $BR_TO_DOL ) ;
		    }
		    if ( index ( $symbol_, "IND" ) == 0 ) {
			$shortcode_ = "IND" ;
			$unreal_pnl_ += ( $symbol_to_volume_map_{$symbol_} * GetHFTDiscount( $shortcode_, $total_index_fut_win_ind_volume_ ) * $BR_TO_DOL ) ;
		    }
		}
		elsif( index ( $symbol_to_exchange_map_{$symbol_}, "CME" ) == 0 ){
		    my $shortcode_ ="" ;

		    if ( index ( $symbol_, "ZN" ) == 0 ) { $shortcode_ = "ZN" ; }
		    if ( index ( $symbol_, "ZF" ) == 0 ) { $shortcode_ = "ZF" ; }
		    if ( index ( $symbol_, "ZB" ) == 0 ) { $shortcode_ = "ZB" ; }
		    if ( index ( $symbol_, "UB" ) == 0 ) { $shortcode_ = "UB" ; }

		    my $trade_hours_ = `date +%H`; chomp ( $trade_hours_ );

		    my $trade_hours_string_ = "EU" ;

		    if( $trade_hours_ > 11 ) {
#compute discount in commission only for US hours volumes
			$trade_hours_string_ = "US" ;
			$unreal_pnl_ += ( ( $symbol_to_volume_map_{$symbol_} - GetCMEEUVolumes( $shortcode_ ) ) * GetCMEDiscount( $shortcode_, $trade_hours_string_ ) ) ;
		    }
		}
#populate tag pnls
		if(! exists $saci_to_tags_map{$saci}) {
		    $saci_to_tags_map{$saci}="UNKNOWNTAG_".$saci;	#fake tag for this saci
		    $tags_to_process{"UNKNOWNTAG_".$saci}=1;
		}	  
		my $tags_for_saci=$saci_to_tags_map{$saci};
		my @tags = split ( ':', $tags_for_saci );
		if ( $#tags >= 0 )
		{
		    for ( my $i = 0 ; $i <= $#tags; $i++)
		    {
#process selected tags
			if(! (exists $tags_to_process{$tags[$i]})) {
			    next;
			}
			my $tag_shc=$tags[$i].':'.$symbol_to_display_name_{$symbol_};
			$tag_shc_to_pos_map{$tag_shc} += ($buysell_ == 0) ? $tsize_ : (-1 * $tsize_);
			$tag_shc_to_realpnl_map{$tag_shc} += $this_trade_pnl;
			$tag_shc_to_unrealpnl_map{$tag_shc} = $tag_shc_to_pos_map{$tag_shc} * $symbol_eod_price_ * $n2d_rate_ - abs ($tag_shc_to_pos_map{$tag_shc}) * $commission_rate_;
			$tag_shc_to_vol_map{$tag_shc} += $tsize_;
#print "debug: $tag_shc pos: $tag_shc_to_pos_map{$tag_shc} thispnl: $this_trade_pnl unrealpnl: $unreal_pnl_temp  totalpnl: $tag_shc_to_pnl_map{$tag_shc} vol: $tag_shc_to_vol_map{$tag_shc} n2D: $n2d_rate_ comm: $commission_rate_ \n";		
		    }
		}
#populate query-wise pnls
		if(! exists $saci_to_qid_map{$saci}) {
		    $saci_to_qid_map{$saci} = "UNKNOWNQID_".$saci;	#fake queryID for this saci
		}
		my $queryID=$saci_to_qid_map{$saci};
		my $queryID_shc=$queryID.':'.$symbol_to_display_name_{$symbol_};
		$query_shc_to_pos_map{$queryID_shc} += ($buysell_ == 0) ? $tsize_ : (-1 * $tsize_);
		$query_shc_to_realpnl_map{$queryID_shc} += $this_trade_pnl;
		$query_shc_to_unrealpnl_map{$queryID_shc} = $query_shc_to_pos_map{$queryID_shc} * $symbol_eod_price_ * $n2d_rate_ - abs ($query_shc_to_pos_map{$queryID_shc}) * $commission_rate_;
		$query_shc_to_vol_map{$queryID_shc} += $tsize_;    
	    }
	}
    }
# don't show the PNL Line for TODTOM Spread (since we are splitting it into TOD and TOM outrights)
    if ( exists $symbol_to_price_map_{ "USD000TODTOM" } ) {
	delete $symbol_to_price_map_{ "USD000TODTOM" };
    }
    if($PNL_SHOW_MODE eq 'T') {
	PrintTagProductPnls ('C', "STDOUT" );
	PrintTagTotalPnls ('C');
    }
    else{
	PrintQueryWisePnls ( 'C', "STDOUT");
	PrintQueryTotalPnls ('C');
    }
    sleep (30);

    my $time_now_=`date +"%H%M%S"`;
    if( int($time_now_) >= $time_eod_trades_)
    {
	my $eod_date_ = `date +"%Y%m%d"` ; chomp ( $eod_date_ ) ;
	my $eod_pnl_tag_file="/spare/local/logs/pnl_data/$trade_type_/EODPnl/nse_tag_ors_pnl_$eod_date_".".txt";
	my $eod_pnl_query_file="/spare/local/logs/pnl_data/$trade_type_/EODPnl/nse_query_ors_pnl_$eod_date_".".txt";
	my $eod_pos_file="/spare/local/logs/pnl_data/$trade_type_/EODPos/nse_overnight_pos_$eod_date_".".txt";
	my $EOD_FILE_LOCATION="/spare/local/logs/pnl_data/$trade_type_/EODPos";

        #backup risktag file with date: useful for historic pnl calculations
        foreach my $server ( sort keys %server_to_last_processed_time_ )
        {
          my $source_file="/spare/local/logs/pnl_data/hft/saci_tags_qid_files/$server"."_tag_saci_qid";
          my $dest_file= "/spare/local/logs/pnl_data/hft/saci_tags_qid_files/$server"."_tag_saci_qid_$eod_date_";
	  `cp -f $source_file $dest_file`; 
        }
        #also backup the unknwon fix saci_qid_map
        `cp -f "/spare/local/logs/pnl_data/hft/saci_tags_qid_files/unknown_tag_saci_qid" "/spare/local/logs/pnl_data/hft/saci_tags_qid_files/unknown_tag_saci_qid_$eod_date_"`;

	$mode_ = 'R';
	print "dumping pnls \n";
	if($PNL_SHOW_MODE eq 'T') {
	    open (my $EOD_PNL, '>', "$eod_pnl_tag_file") or die "Can't open $eod_pnl_tag_file $!";
	    select $EOD_PNL;
	    $| = 1;
	    if ($trade_type_ eq 'mtt') {
		print "\n \nMFT pnls: \n";
	    }
	    else {
		print "HFT Pnls: \n";
	    }
	    PrintTagProductPnls ( 'R', $EOD_PNL);
	    PrintTagTotalPnls ('R');
	}
	else{
	    open (my $EOD_PNL, '>', "$eod_pnl_query_file") or die "Can't open $eod_pnl_query_file $!";
	    select $EOD_PNL;
	    $| = 1;
	    if ($trade_type_ eq 'mtt') {
		print "\n \nMFT pnls: \n";
	    }
	    else {
		print "HFT Pnls: \n";
	    }
	    PrintQueryWisePnls ('R', $EOD_PNL);
	    PrintQueryTotalPnls ('R');
	}
	sleep(20);
	select STDOUT;
        if($PNL_SHOW_MODE eq 'T') {
	  print "proceeding to dump overnight positions \n";
	  open (my $EOD_POS_TMP, '>', "$eod_pos_file") or die "Can't open $eod_pos_file $!";
	  select $EOD_POS_TMP;
	  $| = 1;
	  DumpOrsPositions ( );
	  sleep(10);
          select STDOUT;
          print "dumped overnight positions \n";
        }
        last;
    }
}

my $exit_time=`date +"%H%M%S"` ; chomp ( $exit_time) ;
exit ( 0 );

sub PrintTagProductPnls
{	
    my $mode=shift;
	my $current_file_handle=shift;
    my $date_ = `date`;
    printf ("\n$date_\n");
    %tag_to_pnl_map_= ();
    %tag_to_vol_map_ =();
    my $page_size_=keys %tag_shc_to_pos_map;
    my $line_number_ = 0 ;
    foreach my $tag_shc ( sort keys %tag_shc_to_pos_map ){
	$line_number_ += 1 ;
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
#close open positions on latest trade price
	my $exchange_symbol=$display_name_to_symbol_{$symbol};
	my $commission_rate_ = $symbol_to_price_map_{$exchange_symbol} * $symbol_to_commish_map_{$exchange_symbol} * $INR_TO_DOL;
	$tag_shc_to_unrealpnl_map{$tag_shc} = $tag_shc_to_pos_map{$tag_shc} * $symbol_to_price_map_{$exchange_symbol} * $symbol_to_n2d_map_{$exchange_symbol} - abs ($tag_shc_to_pos_map{$tag_shc}) * $commission_rate_;

	$tag_to_pnl_map_{$tag} += ($tag_shc_to_realpnl_map{$tag_shc} + $tag_shc_to_unrealpnl_map{$tag_shc});
	$tag_to_vol_map_{$tag} += ($tag_shc_to_vol_map{$tag_shc}/$symbol_to_lots_map_{$symbol});
	
	if(index ( $tag_shc , "UNKNOWN" ) == 0 ) { #print unknown tag trades in STDERR
		select STDERR;
	    $| = 1;
	    printf ("\n$date_\n");
	}
	
	printf "| %35.35s ", $tag_shc;

	Print ($mode, $tag_shc_to_realpnl_map{$tag_shc} + $tag_shc_to_unrealpnl_map{$tag_shc} );
	if( 0 == $line_number_ % 2 || $page_size_ < $MAX_PAGE_SIZE_ || $mode_ eq 'R') {
	    printf "| POS: %6d | VOL: %6d \n", $tag_shc_to_pos_map{$tag_shc}/$symbol_to_lots_map_{$symbol},$tag_shc_to_vol_map{$tag_shc}/$symbol_to_lots_map_{$symbol};
	}
	else{
	    printf "| POS: %6d | VOL: %6d \t", $tag_shc_to_pos_map{$tag_shc}/$symbol_to_lots_map_{$symbol},  $tag_shc_to_vol_map{$tag_shc}/$symbol_to_lots_map_{$symbol};
	}
	select $current_file_handle;	#print next entries on current file handle
	$| = 1;
    }
}

sub PrintTagTotalPnls
{
    print "\n \n";
    my $mode=shift;
    my $total_tag_pnls;
    my $total_tag_vol;
    foreach my $tag ( sort keys %tag_to_pnl_map_ ){
    if(index ( $tag , "UNKNOWN" ) == 0 ) {next;} 		#do not print unknown tag in total tag pnls
	printf "%17s |", $tag;
	Print($mode, $tag_to_pnl_map_{$tag});
	$total_tag_pnls+=$tag_to_pnl_map_{$tag};
	$total_tag_vol+=$tag_to_vol_map_{$tag};
	printf "| VOLUME : %8d |\n", $tag_to_vol_map_{$tag};
    }
    print "\n";
    printf "%10s |", "NSE";
    Print($mode, $total_tag_pnls);
    printf "| VOLUME : %8d |\n\n", $total_tag_vol;
}

sub PrintQueryWisePnls
{	
    my $mode=shift;
    my $current_file_handle=shift;
    my $date_ = `date`;
    printf ("\n$date_\n");
    %query_to_pnl_map_ =();
    %query_to_vol_map_ = ();
    my $page_size_=keys %query_shc_to_pos_map;
    my $line_number_ = 0 ;
    foreach my $query_shc ( sort keys %query_shc_to_pos_map ){
	$line_number_ += 1 ;
	my @split_query_words = split ( ':', $query_shc );
	my $query;
	my $symbol;
	if ( $#split_query_words == 1 ) {
	    $query=$split_query_words[0];
	    $symbol=$split_query_words[1];
#print "query: $query shc: $symbol \n";
	}
	else {
	    print "Incorrect query:shc entry $query_shc\n";
	    next;
	}
#close open positions on latest price
	my $exchange_symbol=$display_name_to_symbol_{$symbol};
	my $commission_rate_ = $symbol_to_price_map_{$exchange_symbol} * $symbol_to_commish_map_{$exchange_symbol} * $INR_TO_DOL;
	$query_shc_to_unrealpnl_map{$query_shc} = $query_shc_to_pos_map{$query_shc} * $symbol_to_price_map_{$exchange_symbol} * $symbol_to_n2d_map_{$exchange_symbol} - abs ($query_shc_to_pos_map{$query_shc}) * $commission_rate_;

	$query_to_pnl_map_{$query} += ($query_shc_to_realpnl_map{$query_shc} + $query_shc_to_unrealpnl_map{$query_shc});
	$query_to_vol_map_{$query} += ($query_shc_to_vol_map{$query_shc}/$symbol_to_lots_map_{$symbol});
	
	if(index ( $query_shc , "UNKNOWN" ) == 0 ) { #print unknown tag trades in STDERR
		select STDERR;
	    $| = 1;
	    printf ("\n$date_\n");
	}
	printf "| %35.35s ", $query_shc;

	Print ($mode, $query_shc_to_realpnl_map{$query_shc} + $query_shc_to_unrealpnl_map{$query_shc});
	if( 0 == $line_number_ % 2 || $page_size_ < $MAX_PAGE_SIZE_ || $mode_ eq 'R') {
	    printf "| POS: %6d | VOL: %6d \n", $query_shc_to_pos_map{$query_shc}/$symbol_to_lots_map_{$symbol},  $query_shc_to_vol_map{$query_shc}/$symbol_to_lots_map_{$symbol};
	}
	else{
	    printf "| POS: %6d | VOL: %6d \t", $query_shc_to_pos_map{$query_shc}/$symbol_to_lots_map_{$symbol}, $query_shc_to_vol_map{$query_shc}/$symbol_to_lots_map_{$symbol};
	}
	select $current_file_handle;	#print next entries on current file handle, we selected STDERR incase of unknown 
	$| = 1;
    }
}
sub PrintQueryTotalPnls
{
    print "\n \n";
    my $mode=shift;
    
    my $total_query_pnls;
    my $total_query_vol;
    foreach my $query ( sort keys %query_to_pnl_map_ ){
    if(index ( $query , "UNKNOWN" ) == 0 ) {next;} 		#do not print unknown tag in total query pnls
	printf "%17s |", $query;
	Print($mode, $query_to_pnl_map_{$query});
	$total_query_pnls+=$query_to_pnl_map_{$query};
	$total_query_vol+=$query_to_vol_map_{$query};
	printf "| VOLUME : %8d |\n", $query_to_vol_map_{$query};
    }
    print "\n";
    printf "%10s |", "NSE";
    Print($mode, $total_query_pnls);
    printf "| VOLUME : %8d |\n\n", $total_query_vol;
}

sub GetMktVol
{
    my $symbol_ = shift ;

    my $is_valid_ = 0; 
    if ( index ( $hostname_ , "ip-10-0" ) < 0 ) {
	my $vol_ = `$PSCRIPTS_DIR/get_curr_mkt_vol.sh "$symbol_"` ; chomp($vol_);
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

    my $shc_ = `$PEXEC_DIR/get_shortcode_for_symbol  "$symbol_" $input_date_ ` ; chomp ( $shc_ );
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
	my $commish_ = `$PEXEC_DIR/get_contract_specs "$shc_" $input_date_ COMMISH | awk '{print \$2}' `; chomp ( $commish_ ) ;
	$symbol_to_commish_map_ {$symbol_} = $commish_;

	$symbol_to_n2d_map_{$symbol_} = 1000 ;
	$symbol_to_exchange_map_{$symbol_} = "CFE";
    }
    else
    {
	if(index ( $symbol_, "NSE" ) == 0){
	    $symbol_to_display_name_{$symbol_} = substr($shc_,4);
	    $display_name_to_symbol_{substr($shc_,4)} = $symbol_;
	}

	my $exchange_name_ = `$PEXEC_DIR/get_contract_specs "$shc_"  $input_date_ EXCHANGE | awk '{print \$2}' `; chomp ( $exchange_name_ ) ;
	if ( $exchange_name_ eq "HONGKONG" ) { $exchange_name_ = "HKEX" ; }
	if ( index ( $exchange_name_, "MICEX" ) == 0 )  { $exchange_name_ = "MICEX" ; }
	$symbol_to_exchange_map_{$symbol_}=$exchange_name_;

	my $commish_ = `$PEXEC_DIR/get_contract_specs "$shc_" $input_date_ COMMISH | awk '{print \$2}' `; chomp ( $commish_ ) ;
	$symbol_to_commish_map_ {$symbol_} = $commish_;

	my $n2d_ = `$PEXEC_DIR/get_contract_specs $shc_ $input_date_ N2D | awk '{print \$2}' `; chomp ( $n2d_ ) ;
	$symbol_to_n2d_map_{$symbol_} = $n2d_;

	my $lots_ = `$PEXEC_DIR/get_contract_specs $shc_ $input_date_ LOTSIZE | awk '{print \$2}' `; chomp ( $lots_ ) ;
	$symbol_to_lots_map_{$symbol_to_display_name_{$symbol_}} = $lots_;
    
	if ( index ( $symbol_, "DI" ) == 0 && ! exists $symbol_to_noof_working_days_till_expiry_{$symbol_}) {
    	$symbol_to_noof_working_days_till_expiry_{$symbol_} = CalcNoWorkingDaysTillExpiryForDIs ( $symbol_ , $input_date_) ;
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
	my $expiry_date = `$PEXEC_DIR/option_details  "$shc_" $input_date_ | awk '{print \$1}'` ; chomp ( $expiry_date );
	if($expiry_date <= $expiry_check_date_) {
	    $symbol_to_isExpiring_map_{$symbol_}=1;
	}
    }
    else {
	my $shc_after_2days = `$PEXEC_DIR/get_shortcode_for_symbol  "$symbol_" $expiry_check_date_ ` ; chomp ( $shc_after_2days);
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

    my $overnight_pos_file = "/spare/local/logs/pnl_data/$trade_type_/EODPos/nse_overnight_pos_$input_date_".".txt";
    if(! -e $overnight_pos_file) {
	$overnight_pos_file="/spare/local/logs/pnl_data/$trade_type_/EODPos/nse_overnight_pos_$yesterday_date_".".txt";
    }
    if (-e $overnight_pos_file) {
	open OVN_FILE_HANDLE, "< $overnight_pos_file" or die "see_ors_pnl could not open ors_trades_filename_ $overnight_pos_file\n";

	my @ovn_file_lines_ = <OVN_FILE_HANDLE>;
	close OVN_FILE_HANDLE;
	for ( my $i = 0 ; $i <= $#ovn_file_lines_; $i ++ )
	{
	    my @words_ = split ( ',', $ovn_file_lines_[$i] );
	    if ( $#words_ >= 4 )
	    {
#case1: load tagwise positions
		if($words_[0] eq "TAGWISE") {
		    my $tag = $words_[1];
		    if(index ( $tag , "UNKNOWNTAG" ) == 0 ) {next;} #dont load overnight unknown positions
		    my $symbol_ = $words_[2];
		    SetSecDef ( $symbol_ ) ;
		    chomp($words_[3]);	#pos
		    chomp($words_[4]);	#price
		    my $tag_shc=$tag.':'.$symbol_to_display_name_{$symbol_};

		    $tag_shc_to_pos_map{$tag_shc}=$words_[3];
		    $tag_shc_to_vol_map{$tag_shc}=0;
		    $symbol_to_price_map_{$symbol_} = $words_[4];
		    my $commission_rate_ = $symbol_to_commish_map_{$symbol_};
		    if ( ( index ( $symbol_ , "NSE" ) == 0 ) ) {
			$commission_rate_ = $symbol_to_price_map_{$symbol_} * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL;
		    }
		    $tag_shc_to_realpnl_map{$tag_shc}=  -$tag_shc_to_pos_map{$tag_shc} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_};
		    $tag_shc_to_unrealpnl_map{$tag_shc} = $tag_shc_to_pos_map{$tag_shc} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($tag_shc_to_pos_map{$tag_shc}) * $commission_rate_;
		    print "loadovn: $tag_shc $tag_shc_to_pos_map{$tag_shc} $symbol_to_price_map_{$symbol_} $tag_shc_to_realpnl_map{$tag_shc}\n";
		}
		elsif($words_[0] eq "QUERYWISE")
		{
		    my $query = $words_[1];
		    if(index ( $query , "UNKNOWNTAG" ) == 0 ) {next;}  #dont load overnight unknown positions
		    my $symbol_ = $words_[2];
		    SetSecDef ( $symbol_ ) ;
		    chomp($words_[3]);	#pos
		    chomp($words_[4]);	#price
		    my $query_shc=$query.':'.$symbol_to_display_name_{$symbol_};
		    $query_shc_to_pos_map{$query_shc}=$words_[3];
		    $query_shc_to_vol_map{$query_shc}=0;
		    $symbol_to_price_map_{$symbol_} = $words_[4];
		    my $commission_rate_ = $symbol_to_commish_map_{$symbol_};
		    if ( ( index ( $symbol_ , "NSE" ) == 0 ) ) {
			$commission_rate_ = $symbol_to_price_map_{$symbol_} * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL;
		    }
		    $query_shc_to_realpnl_map{$query_shc}=  -$query_shc_to_pos_map{$query_shc} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_};
		    $query_shc_to_unrealpnl_map{$query_shc} = $query_shc_to_pos_map{$query_shc} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_} - abs ($query_shc_to_pos_map{$query_shc}) * $commission_rate_;
		}
	    }
	}
    }
}

sub DumpOrsPositions
{
#dump positions only in tag mode: 2 different instances running T and Q mode
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

		    print "TAGWISE,$tag,$exchange_sym,$tag_shc_to_pos_map{$tag_shc},$symbol_to_price_map_{$exchange_sym}\n";
		}
	    }
	}

	foreach my $query_shc ( sort keys %query_shc_to_pos_map ){
	    if($query_shc_to_pos_map{$query_shc} != 0)
	    {
		my @split_query_words = split ( ':', $query_shc );
		my $query;
		my $symbol;
		if ( $#split_query_words == 1 ) {
		    $query=$split_query_words[0];
		    $symbol=$split_query_words[1];
		    my $exchange_sym=$display_name_to_symbol_{$symbol};

		    #print "QUERYWISE,$query,$exchange_sym,$query_shc_to_pos_map{$query_shc},$symbol_to_price_map_{$exchange_sym}\n";
		}
	    }
	}
}

sub SendSlackForExpiringContractsWithPositions ()
{
    my $symbol=shift;
    my $shc_ = `$PEXEC_DIR/get_shortcode_for_symbol  "$symbol" $input_date_ ` ; chomp ( $shc_ );

    if(exists ($symbol_to_isExpiring_map_{$symbol})){
	my $alert_str="Expiring product with position: $symbol $shc_ $symbol_to_pos_map_{$symbol}";
	`$PEXEC_DIR/send_slack_notification prod-issues DATA "$alert_str"`;
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

	$symbol_to_unrealpnl_map_{ "USD000UTSTOM" } += (-1 * $todtom_proj_pos_) * $tom_price_ * $symbol_to_n2d_map_{"USD000UTSTOM"} - abs($todtom_proj_pos_) * $symbol_to_commish_map_{"USD000UTSTOM"};
	$symbol_to_unrealpnl_map_{ "USD000000TOD" } += $todtom_proj_pos_ * $tod_price_ * $symbol_to_n2d_map_{"USD000000TOD"} - abs($todtom_proj_pos_) * $symbol_to_commish_map_{"USD000UTSTOM"};

	$symbol_to_volume_map_{ "USD000UTSTOM" } += abs($todtom_proj_pos_);
	$symbol_to_volume_map_{ "USD000000TOD" } += abs($todtom_proj_pos_);

	$symbol_to_pos_map_{ "USD000TODTOM" } = 0;
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
	} else {
	    print color("blue"); print color("BOLD");
	}
	printf "%10.3f ", $value;
	print color("reset");
    }
    else
    {
	printf "%10.3f ", $value;
    }
}
