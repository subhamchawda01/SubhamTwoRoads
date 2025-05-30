#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;
use File::Find;
use List::Util qw/max min/; # for max
use Data::Dumper;
#use autodie;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore_install";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $INSTALL_BIN="$HOME_DIR/$REPO/bin";
my $BASETRADE_BIN="$HOME_DIR/basetrade_install/bin";
my $LIVE_BIN="$HOME_DIR/LiveExec/bin/";

require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/get_hft_commission_discount_for_shortcode.pl"; # Get hft discount according to volume tiers for shortcode
require "$GENPERLLIB_DIR/get_cme_commission_discount_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_cme_eu_volumes_for_shortcode.pl"; # Get CME comission change based on trading hours
require "$GENPERLLIB_DIR/get_number_of_working_days_BMF_between_two_dates.pl" ;
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; # 
require "$GENPERLLIB_DIR/calc_next_business_day.pl";
require "$GENPERLLIB_DIR/get_rts_ticksize.pl"; #RITickSize
require "$GENPERLLIB_DIR/get_shortcode_from_symbol.pl"; #GetShortcodeFromSymbol
use Term::ANSIColor;

my $hostname_ = `hostname`;

#my $exchange_to_unrealpnl_map_;
#C - colour, R - No colour, E - without native commission in native currency, W - with native commision in native currency, P - get commissions in native currency for every symbol

#in E & W mode, only individual symbol's pnl is in native currency. The exchange pnl is still in USD since there is no particular native currency for a exchange

my $USAGE="$0 mode trade_type(H/M) YYYYMMDD  [dont_load_over_night]";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $mode_ = shift;
#my $today_date_ = `date  --date='tomorrow' +"%Y%m%d"` ; chomp ( $today_date_ ) ;
my $trade_type_ = shift || '';
my $today_date_ = `date +"%Y%m%d"` ; chomp ( $today_date_ ) ;
my $input_date_ =  shift || $today_date_ ;
my $EXCH_ = '';
my $LOAD_ = shift || ''; 
my $MAX_PAGE_SIZE_ = shift || 40;

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

=begin comment
#for native currency 
if($mode_ eq 'E'){
  $EUR_TO_DOL = 1;
  $CD_TO_DOL = 1;
  $BR_TO_DOL = 1;
  $GBP_TO_DOL = 1;
  $HKD_TO_DOL = 1;
  $JPY_TO_DOL = 1;
  $RUB_TO_DOL = 1;
  $AUD_TO_DOL = 1;
  $INR_TO_DOL = 1;
}
=end comment
=cut  #comment ends here
#=============================================================#


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

my $secname1_ = `$BASETRADE_BIN/get_exchange_symbol BR_WDO_0 $input_date_`;
my $secname2_ = `$BASETRADE_BIN/get_exchange_symbol BR_DOL_0 $input_date_`;
$same_price_shc_map_{ $secname1_ } = $secname2_;
$same_price_shc_map_{ $secname2_ } = $secname1_;

$secname1_ = `$BASETRADE_BIN/get_exchange_symbol BR_WIN_0 $input_date_`;
$secname2_ = `$BASETRADE_BIN/get_exchange_symbol BR_IND_0 $input_date_`;
$same_price_shc_map_{ $secname1_ } = $secname2_;
$same_price_shc_map_{ $secname2_ } = $secname1_;
#=============================================================#

my %symbol_to_commish_map_ = ();
my %symbol_to_n2d_map_ = ();

my %symbol_to_unrealpnl_map_ = ();
my %symbol_to_pos_map_ = ();
my %symbol_to_price_map_ = ();
my %symbol_to_volume_map_ = ();
my %symbol_to_exchange_map_ = ();
my %symbol_to_display_name_ = ();

my %exchange_to_unrealpnl_map_ = ();
my %exchange_to_volume_map_ = ();

my %symbol_to_mkt_vol_ =();

my %symbol_to_currency_map_ = ();
my %exchange_to_currency_map_ = ();
my %symbol_to_total_commish_map_ = ();
my %symbol_to_isExpiring_map_ = ();

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
$server_to_last_processed_time_{"sdv-bsl-srv11"} = 0; #10.23.52.51
$server_to_last_processed_time_{"sdv-bsl-srv12"} = 0;
$server_to_last_processed_time_{"sdv-bsl-srv13"} = 0;
$server_to_last_processed_time_{"sdv-bmf-srv11"} = 0; #10.220.65.35
$server_to_last_processed_time_{"sdv-bmf-srv12"} = 0; #10.220.65.34
$server_to_last_processed_time_{"sdv-bmf-srv15"} = 0; #10.220.65.36
$server_to_last_processed_time_{"sdv-bmf-srv13"} = 0; #10.220.65.33
$server_to_last_processed_time_{"sdv-bmf-srv14"} = 0; #10.220.65.38
$server_to_last_processed_time_{"sdv-mos-srv11"} = 0; #172.18.244.107
$server_to_last_processed_time_{"sdv-mos-srv12"} = 0; #10.23.241.2
$server_to_last_processed_time_{"sdv-mos-srv13"} = 0; #172.26.33.226
$server_to_last_processed_time_{"SDV-HK-SRV11"} = 0; #10.152.224.145
$server_to_last_processed_time_{"SDV-HK-SRV12"} = 0; #10.152.224.146
$server_to_last_processed_time_{"sdv-ose-srv12"} = 0; #10.134.73.212
$server_to_last_processed_time_{"sdv-ose-srv11"} = 0; #10.134.73.211
$server_to_last_processed_time_{"sdv-ose-srv13"} = 0; #10.134.73.213
$server_to_last_processed_time_{"sdv-ose-srv14"} = 0; #10.134.73.214
$server_to_last_processed_time_{"sdv-cfe-srv11"} = 0; #10.23.74.61
$server_to_last_processed_time_{"sdv-cfe-srv12"} = 0; #10.23.74.62
$server_to_last_processed_time_{"sdv-cfe-srv13"} = 0; #10.23.74.63 
$server_to_last_processed_time_{"SDV-ASX-SRV11"} = 0; #10.23.43.51
$server_to_last_processed_time_{"SDV-ASX-SRV12"} = 0; #10.23.43.52
$server_to_last_processed_time_{"sdv-sgx-srv11"} = 0; #10.23.26.51
$server_to_last_processed_time_{"sdv-sgx-srv12"} = 0; #10.23.26.52

if ($trade_type_ eq 'mtt') {
  $server_to_last_processed_time_{"sdv-ind-srv12"} = 0; #10.23.115.62
}

#================== Load overnight positions only when no argument is provided for the same =================#
if ($LOAD_ eq '') {
  LoadOvernightPositions ( )
}
#=============================================================================================================#
my $time_eod_trades_ = 210000;
my $cron_time_ = 210800;
my $delta_dir="/spare/local/logs/pnl_data/$trade_type_/delta_files/";

sub LoadConfigFile {
    my $config="/spare/local/logs/pnl_data/hft/pnl_config";
    open CONFIG_FILEHANDLE, "< $config" or die " could not open $config\n";
    my @config_data= <CONFIG_FILEHANDLE>;
    close CONFIG_FILEHANDLE;
    for ( my $i = 0 ; $i <= $#config_data; $i ++ )
    {
        my @line = split ( ' ', $config_data[$i] );
        if($#line >= 1)
        {
            if($line[0] eq "TRADE_ENDTIME")
            {
                $time_eod_trades_=$line[1];
            }
            elsif($line[0] eq "ENDTIME")
            {
                $cron_time_=$line[1];
            }
        }
    }
}
#make STDOUT HOT so that nothing is buffered
select STDOUT;
$| = 1;

LoadConfigFile();

while(1)
{
  my $time_now_=`date +"%H%M%S"` ; chomp ( $time_now_) ;
  if( int($time_now_) >= $time_eod_trades_ && int($time_now_) < $cron_time_)
  {
    my $eod_date_ = `date +"%Y%m%d"` ; chomp ( $eod_date_ ) ;
    my $eod_pnl_file="/spare/local/logs/pnl_data/$trade_type_/EODPnl/ors_pnl_$eod_date_".".txt";
    my $eod_pos_file_tmp="/spare/local/logs/pnl_data/$trade_type_/EODPos/overnight_pos_$eod_date_"."_tmp.txt";
    my $eod_pos_file="/spare/local/logs/pnl_data/$trade_type_/EODPos/overnight_pos_$eod_date_".".txt";
    my $EOD_FILE_LOCATION="/spare/local/logs/pnl_data/$trade_type_/EODPos";
    my $TMX_POS_COMPUTE="/home/dvcinfra/LiveExec/scripts/compute_tmx_positions.pl";

    $mode_ = 'R';
    open (my $EOD_PNL, '>', "$eod_pnl_file") or die "Can't open $eod_pnl_file $!";
    select $EOD_PNL;
    $| = 1;
    if ($trade_type_ eq 'mtt') {
      print "\n \nMFT pnls: \n";
    }
    else {
      print "HFT Pnls: \n";
    }
    PrintPnls ( );
    PrintExchPnls ( );
    sleep(20);
    select STDOUT;

    #proceeding to dump overnight positions
    open (my $EOD_POS_TMP, '>', "$eod_pos_file_tmp") or die "Can't open $eod_pos_file_tmp $!";
    select $EOD_POS_TMP;
    DumpOrsPositions ( );
    sleep(20);
    select STDOUT;
    $| = 1;
  `perl -w $TMX_POS_COMPUTE $EOD_FILE_LOCATION/overnight_pos_"$eod_date_"_tmp.txt > $EOD_FILE_LOCATION/overnight_pos_"$eod_date_".txt 2>>/spare/local/logs/pnl_data/"$trade_type_"/pnl_log`;
    sleep(10);
    last;
  }
  my @delta_files_to_process;
  foreach my $server ( sort keys %server_to_last_processed_time_ )
  { 
      #print "for $server: beginning after $server_to_last_processed_time_{$server}  ";
      my $server_last_processed_time_=$server_to_last_processed_time_{$server};
      #ls | awk -F"_" '{if ( $2 == "102310255" ) print $3 }'
       
      my @delta_file_times = `ls $delta_dir |  awk -F"_" '{if (\$2 == "$server") print \$3}'` ;
      foreach my $file_time (@delta_file_times)
      {
        chomp ($file_time);
        if ( int($file_time) > $server_to_last_processed_time_{$server})
        { 
          my @temp_file_;
          find(sub  { push @temp_file_ , $File::Find::name if ( m/^(.*)$server/ and m/^(.*)$file_time$/ ) }, $delta_dir);
          #print "@temp_file_ \n";
          @delta_files_to_process = ( @delta_files_to_process, @temp_file_);
        }
        if( int($file_time) > $server_last_processed_time_)
        {
          $server_last_processed_time_=int($file_time);
        }
      }
      $server_to_last_processed_time_{$server} = $server_last_processed_time_;
      #print "Till $server_to_last_processed_time_{$server} \n";
  }
  
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
      if ( $#words_ >= 4 )
      {
        my $symbol_ = $words_[0];
        my $buysell_ = $words_[1];
        my $tsize_ = $words_[2];
        my $tprice_ = $words_[3];
        my $saos_ = $words_[4];

#print " $words_[0] $words_[1] $words_[2] $words_[3] $words_[4]\n";

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

        if($mode_ eq 'E'){
          $symbol_to_commish_map_{$symbol_} = 0;
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

        if ( $buysell_ == 0 )
        { # buy
          if ( index ( $symbol_ , "DI" ) == 0 ) {
            $symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
          }
          else {
            $symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
          }
        }
        else
        {
          if ( index ( $symbol_ , "DI" ) == 0 ) {
            $symbol_to_unrealpnl_map_{$symbol_} -= $tsize_ * ( 100000 / ( $tprice_ / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
          }
          else {
            $symbol_to_unrealpnl_map_{$symbol_} += $tsize_ * $tprice_ * $symbol_to_n2d_map_{$symbol_};
          }
        }

        if(( index ( $symbol_ , "NSE" ) == 0 )){
          $symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $tprice_ * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL);
          $symbol_to_total_commish_map_{$symbol_} +=  ($tsize_ * $tprice_ * $symbol_to_commish_map_{$symbol_} * $INR_TO_DOL);
        }
        else{
          $symbol_to_unrealpnl_map_{$symbol_} -= ($tsize_ * $symbol_to_commish_map_{$symbol_});
          $symbol_to_total_commish_map_{$symbol_} +=  $tsize_ * $symbol_to_commish_map_{$symbol_};     
        }
      }
    }
  }  
    my $total_index_fut_win_ind_volume_ = 0.0 ;

    foreach my $symbol_ ( sort keys %symbol_to_price_map_ )
    {
      if( index ( $symbol_to_exchange_map_{$symbol_}, "BMF" ) == 0 )
      {
        if ( index ( $symbol_, "IND" ) == 0 ) { $total_index_fut_win_ind_volume_ += $symbol_to_volume_map_{$symbol_} * 5 ; }
        if ( index ( $symbol_, "WIN" ) == 0 ) { $total_index_fut_win_ind_volume_ += $symbol_to_volume_map_{$symbol_} ; }
      }
      $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} = 0;
      $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} = 0;
    }

# don't show the PNL Line for TODTOM Spread (since we are splitting it into TOD and TOM outrights)
    if ( exists $symbol_to_price_map_{ "USD000TODTOM" } ) {
      delete $symbol_to_price_map_{ "USD000TODTOM" };
    }

    my $totalpnl_ = 0.0;

    my $page_size_ = keys %symbol_to_price_map_ ;

    foreach my $symbol_ ( sort keys %symbol_to_price_map_ )
    {
# print "#### $symbol_\n";
      my $unreal_pnl_ = 0 ;
      
      #for mft, we need to consider the last closing price for the settlement trade
      # i.e. when we consider positions left after reading the entire trade file
      # Today's closing file is available around 15:00 GMT, with date stamp of next day
      # wait until file is available and pick that as last trading price. EVerything else will 
      # be taken care of.      
      if ($trade_type_ eq 'mtt' && $symbol_to_exchange_map_{$symbol_} eq 'NSE')
      {
        my $next_day = CalcNextBusinessDay ( $input_date_ );
        my $nse_contract_file="/spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts.$next_day";
        if(-e $nse_contract_file){
          my $shc_ = `$BASETRADE_BIN/get_shortcode_for_symbol  "$symbol_" $input_date_ ` ; chomp ( $shc_ ) ;
          my $last_close_price;
	  if(index ( $shc_, "_FUT" ) != -1 ) {
	  	$last_close_price= `$LIVE_BIN/get_contract_specs "$shc_" $next_day LAST_CLOSE_PRICE | awk '{print \$2}'`; chomp ( $last_close_price );
	  }
	  else {
		$last_close_price= `$LIVE_BIN/get_contract_specs "$shc_" $next_day LAST_CLOSE_PRICE_OPTIONS | awk '{print \$2}'`; chomp ( $last_close_price );
	  }
          #if last close px not found, consider last traded price as last_close_price
          if( $last_close_price ne '' && $last_close_price > 0) {
          	$symbol_to_price_map_{$symbol_} = $last_close_price;
          }
         }
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

      $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_eod_price_ * $n2d_rate_ - abs ($symbol_to_pos_map_{$symbol_}) * $commission_rate_;

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

      if ( ( ! $EXCH_ )  && ( index ( $symbol_to_exchange_map_{$symbol_}, "BMFEQ" ) == 0 ) )
      {
        $totalpnl_ += $unreal_pnl_;
        $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
        $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};
        next;
      }

      GetMktVol($symbol_);

      $totalpnl_ += $unreal_pnl_;
      $exchange_to_unrealpnl_map_{$symbol_to_exchange_map_{$symbol_}} += $unreal_pnl_;
      $exchange_to_volume_map_{$symbol_to_exchange_map_{$symbol_}} += $symbol_to_volume_map_{$symbol_};
    }
  
    PrintPnls ( );
    PrintExchPnls ( );
    sleep (30);
}

my $exit_time=`date +"%H%M%S"` ; chomp ( $exit_time) ;
exit ( 0 );

sub PrintPnls
{
  my $date_ = `date`;
  printf ("\n$date_\n");
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

    $unreal_pnl_ = $symbol_to_unrealpnl_map_{$symbol_} + $symbol_to_pos_map_{$symbol_} * $symbol_eod_price_ * $n2d_rate_ - abs ($symbol_to_pos_map_{$symbol_}) * $commission_rate_;
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
    if($symbol_to_mkt_vol_{$symbol_} == 0){
      $symbol_to_mkt_vol_{$symbol_} = -1000000000;
      print "DEBUG: $symbol_ \n";
    }
    if($mode_ eq 'R' || $mode_  eq  'E' || $mode_  eq  'W'){
      printf "| POS : %4d | VOL : %4d | v/V: %.1f | LPX : %s |\n", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_},$symbol_to_volume_map_{$symbol_}/$symbol_to_mkt_vol_{$symbol_}, $last_closing_price;
    }
    elsif ( $mode_ eq 'C') {
      if( 0 == $line_number_ % 2 || $page_size_ < $MAX_PAGE_SIZE_){
        printf "| POS: %6d | VOL: %7d | v/V: %5.1f | LPX: %13s |\n", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}/$symbol_to_mkt_vol_{$symbol_} , $last_closing_price;
      }
      else{
        printf "| POS: %6d | VOL: %7d | v/V: %5.1f | LPX: %13s |\t", $symbol_to_pos_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}, $symbol_to_volume_map_{$symbol_}/$symbol_to_mkt_vol_{$symbol_}, $last_closing_price;
      }
    }

    }
}

sub PrintExchPnls
{
if($mode_ ne 'P'){
      printf "\n\n----------------------------------------------------------------------------------------\n\n";

      my $totalpnl_ = 0.0;
      my $totalvol_ = 0;

#width of R mode can be same as C and E, but just seperated it just incase if any scripts are running assuming width 4.
      my @all_exchanges=("EUREX", "LIFFE", "ICE" , "TMX", "CME", "BMF", "HKEX", "OSE", "RTS", "MICEX", "CFE", "NSE", "ASX", "SGX");

      for(my $i = 0; $i < scalar @all_exchanges; $i++) {
        my $exchange=$all_exchanges[$i];
        if ( exists $exchange_to_unrealpnl_map_{$exchange} )
        {
		printf "%17s |", $exchange;

		Print($mode_, $exchange_to_unrealpnl_map_{$exchange});

		if ( $mode_  eq 'C' ){
			printf "| VOLUME : %8d |\n", $exchange_to_volume_map_{$exchange};
		}

		if ( $mode_ eq 'R' || $mode_  eq 'E' || $mode_  eq 'W'){
			printf "| VOLUME : %4d |\n", $exchange_to_volume_map_{$exchange};
		}

		$totalpnl_ += $exchange_to_unrealpnl_map_{$exchange};
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
	my $shc_ = `$BASETRADE_BIN/get_shortcode_for_symbol  "$symbol_" $input_date_ ` ; chomp ( $shc_ );
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
    		my $commish_ = `$LIVE_BIN/get_contract_specs "$shc_" $input_date_ COMMISH | awk '{print \$2}' `; chomp ( $commish_ ) ;
    		$symbol_to_commish_map_ {$symbol_} = $commish_;
		
		$symbol_to_n2d_map_{$symbol_} = 1000 ;
		$symbol_to_exchange_map_{$symbol_} = "CFE";
	}
	else
	{
		if(index ( $symbol_, "NSE" ) == 0){
			$symbol_to_display_name_{$symbol_} = substr($shc_,4);
		}

		my $exchange_name_ = `$LIVE_BIN/get_contract_specs "$shc_"  $input_date_ EXCHANGE | awk '{print \$2}' `; chomp ( $exchange_name_ ) ;
		if ( $exchange_name_ eq "HONGKONG" ) { $exchange_name_ = "HKEX" ; }
		if ( index ( $exchange_name_, "MICEX" ) == 0 )  { $exchange_name_ = "MICEX" ; }
		$symbol_to_exchange_map_{$symbol_}=$exchange_name_;

		my $commish_ = `$LIVE_BIN/get_contract_specs "$shc_" $input_date_ COMMISH | awk '{print \$2}' `; chomp ( $commish_ ) ;
		$symbol_to_commish_map_ {$symbol_} = $commish_;

		my $n2d_ = `$LIVE_BIN/get_contract_specs $shc_ $input_date_ N2D | awk '{print \$2}' `; chomp ( $n2d_ ) ;
		$symbol_to_n2d_map_{$symbol_} = $n2d_;
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
		my $expiry_date = `$LIVE_BIN/option_details  "$shc_" $input_date_ | awk '{print \$1}'` ; chomp ( $expiry_date );
		if($expiry_date <= $expiry_check_date_) {
			$symbol_to_isExpiring_map_{$symbol_}=1;
		}
	}
	else {
		my $shc_after_2days = `$BASETRADE_BIN/get_shortcode_for_symbol  "$symbol_" $expiry_check_date_ ` ; chomp ( $shc_after_2days);
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

  my $overnight_pos_file="/spare/local/logs/pnl_data/$trade_type_/EODPos/overnight_pos_$yesterday_date_".".txt";
  if (-e $overnight_pos_file) {
    open OVN_FILE_HANDLE, "< $overnight_pos_file" or die "see_ors_pnl could not open ors_trades_filename_ $overnight_pos_file\n";

    my @ovn_file_lines_ = <OVN_FILE_HANDLE>;
    close OVN_FILE_HANDLE;
    for ( my $i = 0 ; $i <= $#ovn_file_lines_; $i ++ )
    {
      my @words_ = split ( ',', $ovn_file_lines_[$i] );
      if ( $#words_ >= 2 )
      {

        my $symbol_ = $words_[0];
        SetSecDef ( $symbol_ ) ;
        if ( ($symbol_to_exchange_map_{$symbol_} eq $EXCH_) or ( ! $EXCH_ ) )
        {
          ;
        }
        else
        {
          next;
        }
        chomp($words_[1]);
        chomp($words_[2]);
        $symbol_to_pos_map_{$symbol_} = $words_[1];
        $symbol_to_price_map_{$symbol_} = $words_[2];
       
        if ( ( index ( $symbol_ , "DI" ) == 0 ) ) {
          $symbol_to_unrealpnl_map_{$symbol_} = $symbol_to_pos_map_{$symbol_} * ( 100000 / ( $symbol_to_price_map_{$symbol_} / 100 + 1 ) ** ( $symbol_to_noof_working_days_till_expiry_{$symbol_} / 252 ) ) * $BR_TO_DOL ;
        }
        else
        {
          $symbol_to_unrealpnl_map_{$symbol_} = -$symbol_to_pos_map_{$symbol_} * $symbol_to_price_map_{$symbol_} * $symbol_to_n2d_map_{$symbol_};
        }
        $symbol_to_volume_map_{$symbol_} = 0;
      }
    }
  }
}

sub DumpOrsPositions
{
  TransferNKPosToSpread ();	#transfers sgx_nk_0 sgx_nk_1 hedged positions to sgx_nk0_nk1 spread
  foreach my $symbol_ (sort keys %symbol_to_pos_map_)
  {
    if (exists ($spread_to_pos_symbol_{$symbol_})){
      $symbol_to_pos_map_{$spread_to_pos_symbol_{$symbol_}}+=$symbol_to_pos_map_{$symbol_};
      $symbol_to_pos_map_{$spread_to_neg_symbol_{$symbol_}}-=$symbol_to_pos_map_{$symbol_};
    }

    if (index($symbol_, "NK") == 0 )
    {
      foreach my $symbol_1_ ( sort keys %symbol_to_unrealpnl_map_)
      {
        if (index ($symbol_1_,"NKM") == 0)
        {
          if (substr($symbol_,2,4) eq substr($symbol_1_,3,4))
          {
            $symbol_to_pos_map_{$symbol_1_} += 10*$symbol_to_pos_map_{$symbol_};
            $symbol_to_pos_map_{$symbol_} = 0;
          }
        }
      }

    }
    if (index($symbol_, "DOL") == 0 )
    {
      foreach my $symbol_1_ ( sort keys %symbol_to_unrealpnl_map_)
      {
        if (index ($symbol_1_,"WDO") == 0)
        {
          if (substr($symbol_,3,3) eq substr($symbol_1_,3,3))
          {
            $symbol_to_pos_map_{$symbol_1_} += 5*$symbol_to_pos_map_{$symbol_};
            $symbol_to_pos_map_{$symbol_} = 0;
          }
        }
      }
    }

    if (index($symbol_, "IND") == 0 )
    {
      foreach my $symbol_1_ ( sort keys %symbol_to_unrealpnl_map_)
      {
        if (index ($symbol_1_,"WIN") == 0)
        {
          if (substr($symbol_,3,3) eq substr($symbol_1_,3,3))
          {
            if ( $symbol_to_pos_map_{$symbol_1_} == -5*$symbol_to_pos_map_{$symbol_}  )
            {
              $symbol_to_pos_map_{$symbol_} = 0;
              $symbol_to_pos_map_{$symbol_1_} = 0;
            }
          }
        }
      }
    }
  }

  foreach my $symbol_ (sort keys %symbol_to_unrealpnl_map_)
  {
    if (exists ($spread_to_pos_symbol_{$symbol_})){
      next;
    }
    else{
      if(($symbol_to_pos_map_{$symbol_} != 0 || index ($symbol_, "BAX") != -1)){
        print "$symbol_,$symbol_to_pos_map_{$symbol_},$symbol_to_price_map_{$symbol_}\n";
      }
    }
  }
}

#special handling for SGX_NK_0 and SGX_NK_1 contract
sub TransferNKPosToSpread()
{
	my $shc_0 = "SGX_NK_0";
	my $shc_1 = "SGX_NK_1";
	my $exch_sym_0= `$BASETRADE_BIN/get_exchange_symbol $shc_0 $input_date_`; chomp ($exch_sym_0);
	my $exch_sym_1= `$BASETRADE_BIN/get_exchange_symbol $shc_1 $input_date_`; chomp ($exch_sym_1);
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
		my $spread_exch_sym= `$BASETRADE_BIN/get_exchange_symbol "SP_SGX_NK0_NK1"  $input_date_`; chomp ($spread_exch_sym); #spread symbol
		if(exists ($symbol_to_pos_map_{$spread_exch_sym}))
		{
			$symbol_to_pos_map_{$spread_exch_sym}+=$max_pos;
		}
		else {
			$symbol_to_pos_map_{$spread_exch_sym}=$max_pos;
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






