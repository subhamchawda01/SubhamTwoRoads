#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use Term::ANSIColor;
use List::Util qw[min max] ;

my $REPO="infracore_install";
my $GENPERLLIB_DIR=$ENV{"HOME"}."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # Get commission for shortcode
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; # for exchange rates
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD



my $USAGE="$0 <RETRIEVE/transfile> <TODAY/YYYYMMDD> <EU/ONLINE> ";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }

my $mfg_trans_filename_ = $ARGV[0];
my $run_ = $ARGV[0];
my $expected_date_ = $ARGV[1];
my $period_ = $ARGV[2];



my %mfg_exch_future_code_to_shortcode_ = ();
my %mfg_exch_future_code_to_n2d_ = ();
my %mfg_exch_future_code_to_exchange_ = ();

my %shortcode_to_pnl_ = ();
my %shortcode_to_last_pos_ = ();
my %shortcode_to_last_price_ = ();
my %shortcode_to_n2d_ = ();
my %shortcode_to_volume_ = ();
my %shortcode_to_pos_ = ();
my %shortcode_to_spos_ = ();
my %shortcode_to_bpos_ = ();
my %shortcode_to_exchange_ = ();

my %fee_discrepancy_shortcodes_ = ();

# These instruments need to be configured with NewEdge instrument codes.
$mfg_exch_future_code_to_shortcode_{"27SD"} = "FGBS";
$mfg_exch_future_code_to_n2d_{"27SD"} = 1000;
$mfg_exch_future_code_to_exchange_{"27SD"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27BC"} = "FGBM";
$mfg_exch_future_code_to_n2d_{"27BC"} = 1000;
$mfg_exch_future_code_to_exchange_{"27BC"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27BM"} = "FGBL";
$mfg_exch_future_code_to_n2d_{"27BM"} = 1000;
$mfg_exch_future_code_to_exchange_{"27BM"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27SF"} = "FESX";
$mfg_exch_future_code_to_n2d_{"27SF"} = 10;
$mfg_exch_future_code_to_exchange_{"27SF"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27I*"} = "FOAT";
$mfg_exch_future_code_to_n2d_{"27I*"} = 1000;
$mfg_exch_future_code_to_exchange_{"27I*"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27FE"} = "FDAX";
$mfg_exch_future_code_to_n2d_{"27FE"} = 25;
$mfg_exch_future_code_to_exchange_{"27FE"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27F7"} = "FBTP";
$mfg_exch_future_code_to_n2d_{"27F7"} = 1000;
$mfg_exch_future_code_to_exchange_{"27F7"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27FA"} = "FBTS";
$mfg_exch_future_code_to_n2d_{"27FA"} = 1000;
$mfg_exch_future_code_to_exchange_{"27FA"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27BV"} = "FGBX";
$mfg_exch_future_code_to_n2d_{"27BV"} = 1000;
$mfg_exch_future_code_to_exchange_{"27BV"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"30CA"} = "JFFCE";
$mfg_exch_future_code_to_n2d_{"30CA"} = 10;
$mfg_exch_future_code_to_exchange_{"30CA"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"28EJ"} = "KFFTI";
$mfg_exch_future_code_to_n2d_{"28EJ"} = 200;
$mfg_exch_future_code_to_exchange_{"28EJ"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"05SF"} = "LFI";
$mfg_exch_future_code_to_n2d_{"05SF"} = 2500;
$mfg_exch_future_code_to_exchange_{"05SF"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"05RJ"} = "LFL";
$mfg_exch_future_code_to_n2d_{"05RJ"} = 1250;
$mfg_exch_future_code_to_exchange_{"05RJ"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"05RH"} = "LFR";
$mfg_exch_future_code_to_n2d_{"05RH"} = 1000;
$mfg_exch_future_code_to_exchange_{"05RH"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"05FT"} = "LFZ";
$mfg_exch_future_code_to_n2d_{"05FT"} = 10;
$mfg_exch_future_code_to_exchange_{"05FT"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"25B3"} = "YFEBM";
$mfg_exch_future_code_to_n2d_{"25B3"} = 50 ;
$mfg_exch_future_code_to_exchange_{"25B3"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"14LA"} = "XFC";
$mfg_exch_future_code_to_n2d_{"14LA"} = 10 ;
$mfg_exch_future_code_to_exchange_{"14LA"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"14RC"} = "XFRC";
$mfg_exch_future_code_to_n2d_{"14RC"} = 10 ;
$mfg_exch_future_code_to_exchange_{"14RC"} = "LIFFE";

$mfg_exch_future_code_to_shortcode_{"0125"} = "ZF";
$mfg_exch_future_code_to_n2d_{"0125"} = 1000;
$mfg_exch_future_code_to_exchange_{"0125"} = "CME"; 

$mfg_exch_future_code_to_shortcode_{"16N1"} = "NIY";
$mfg_exch_future_code_to_n2d_{"16N1"} = 500;
$mfg_exch_future_code_to_exchange_{"16N1"} = "CME";  # one chicago 

$mfg_exch_future_code_to_shortcode_{"16NK"} = "NKD";
$mfg_exch_future_code_to_n2d_{"16NK"} = 50;
$mfg_exch_future_code_to_exchange_{"16NK"} = "CME"; # one chicago 

$mfg_exch_future_code_to_shortcode_{"0121"} = "ZN";
$mfg_exch_future_code_to_n2d_{"0121"} = 1000;
$mfg_exch_future_code_to_exchange_{"0121"} = "CME";

$mfg_exch_future_code_to_shortcode_{"01CBT 2YR T-NOTE"} = "ZT";
$mfg_exch_future_code_to_n2d_{"01CBT 2YR T-NOTE"} = 2000;
$mfg_exch_future_code_to_exchange_{"01CBT 2YR T-NOTE"} = "CME";

$mfg_exch_future_code_to_shortcode_{"0117"} = "ZB";
$mfg_exch_future_code_to_n2d_{"0117"} = 1000;
$mfg_exch_future_code_to_exchange_{"0117"} = "CME";

$mfg_exch_future_code_to_shortcode_{"01UL"} = "UB";
$mfg_exch_future_code_to_n2d_{"01UL"} = 1000;
$mfg_exch_future_code_to_exchange_{"01UL"} = "CME";

$mfg_exch_future_code_to_shortcode_{"10BA"} = "BAX";
$mfg_exch_future_code_to_n2d_{"10BA"} = 2500;
$mfg_exch_future_code_to_exchange_{"10BA"} = "TMX";

$mfg_exch_future_code_to_shortcode_{"10CG"} = "CGB";
$mfg_exch_future_code_to_n2d_{"10CG"} = 1000;
$mfg_exch_future_code_to_exchange_{"10CG"} = "TMX";

$mfg_exch_future_code_to_shortcode_{"10CF"} = "CGF";
$mfg_exch_future_code_to_n2d_{"10CF"} = 1000;
$mfg_exch_future_code_to_exchange_{"10CF"} = "TMX";

$mfg_exch_future_code_to_shortcode_{"10CZ"} = "CGZ";
$mfg_exch_future_code_to_n2d_{"10CZ"} = 2000;
$mfg_exch_future_code_to_exchange_{"10CZ"} = "TMX";

$mfg_exch_future_code_to_shortcode_{"10SX"} = "SXF";
$mfg_exch_future_code_to_n2d_{"10SX"} = 200;
$mfg_exch_future_code_to_exchange_{"10SX"} = "TMX";

$mfg_exch_future_code_to_shortcode_{"22MN_Y"} = "Y_NKM";
$mfg_exch_future_code_to_n2d_{"22MN_Y"} = 100;
$mfg_exch_future_code_to_exchange_{"22MN_Y"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"22MN_T"} = "T_NKM";
$mfg_exch_future_code_to_n2d_{"22MN_T"} = 100;
$mfg_exch_future_code_to_exchange_{"22MN_T"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"22NI_Y"} = "Y_NK";   # newedge has 100000 should retreive from price_multiplier field ( 20 is the offset )
$mfg_exch_future_code_to_n2d_{"22NI_Y"} = 100000;
$mfg_exch_future_code_to_exchange_{"22NI_Y"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"22NI_T"} = "T_NK";
$mfg_exch_future_code_to_n2d_{"22NI_T"} = 100000;
$mfg_exch_future_code_to_exchange_{"22NI_T"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"33MH"} = "MHI";
$mfg_exch_future_code_to_n2d_{"33MH"} = 1000;
$mfg_exch_future_code_to_exchange_{"33MH"} = "HKEX";

$mfg_exch_future_code_to_shortcode_{"33HC"} = "HHI";
$mfg_exch_future_code_to_n2d_{"33HC"} = 50;
$mfg_exch_future_code_to_exchange_{"33HC"} = "HKEX";

$mfg_exch_future_code_to_shortcode_{"33HS"} = "HSI";
$mfg_exch_future_code_to_n2d_{"33HS"} = 5000;
$mfg_exch_future_code_to_exchange_{"33HS"} = "HKEX";

$mfg_exch_future_code_to_shortcode_{"S9HKD"} = "HKDUSD";
$mfg_exch_future_code_to_n2d_{"S9HKD"} = 1;
$mfg_exch_future_code_to_exchange_{"S9HKD"} = "DVCAP";

$mfg_exch_future_code_to_shortcode_{"S9JPY"} = "JPYUSD";
$mfg_exch_future_code_to_n2d_{"S9JPY"} = 1;
$mfg_exch_future_code_to_exchange_{"S9JPY"} = "DVCAP";


# These offsets were obtained using the 'FIMST4F1.xls & FIMST4F1.csv' template files
# These will be used to determine the field being processed after splitting each trade line
#awk -F, '{print $1$12$13$18$25$24$7$28$29$39$40$41$42 }'
my $record_id_offset_ = 0; # FRECID # Needed to figure out which records to use towards pnl computation.
my $date_offset_ = 11; # FTDATE
my $buysell_offset_ = 12; # FBS # 1 = buy ; 2 = sell
my $quantity_offset_ = 17; # FQTY
my $mfg_future_code_offset_ = 24; # FFC
my $exch_code_offset_ = 23; # FEXCH
my $contract_yr_month_offset_ = 6; # FCTYM  # YYYYMM
my $trade_price_offset_ = 27; # FTPRIC # Trade price may be different than price of order
my $closing_price_offset_ = 28 ; # FCLOSE

# Need to figure out how many charges are placed on us.
my $comm_amount_offset_ = 38;
my $fee1_amount_offset_ = 39;
my $fee2_amount_offset_ = 40;
my $fee3_amount_offset_ = 41;

# EURTODOL
my $CD_TO_DOL_ = 0.97;
my $EUR_TO_DOL_ = 1.30;
my $GBP_TO_DOL_ = 1.57;
my $HKD_TO_DOL_ = 0.1289 ;
my $JPY_TO_DOL_ = 0.0104 ;

#================================= fetching conversion rates from currency info file if available ====================#
if ( $expected_date_ eq "TODAY" )
{
    $expected_date_ = `date "+%Y%m%d"` ;
    chomp ( $expected_date_ ) ;
}

my $prevdate_ = CalcPrevBusinessDay ( $expected_date_ ) ;
chomp ( $prevdate_ ) ;



my $time1_ = `date +'%H%M%S'` ;
chomp ( $time1_ ) ;

my $time_ = `date +'%T'` ;
chomp ( $time_ ) ;

my $curr_date_ = $expected_date_ ;

my $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;

my $attempt_ = 0 ;

while ( ! -e $curr_filename_  && $attempt_ < 10 ) 
{
    $curr_date_ = CalcPrevBusinessDay ( $curr_date_ ) ;
    $curr_filename_ = '/spare/local/files/CurrencyData/currency_rates_'.$curr_date_.'.txt' ;
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
          $EUR_TO_DOL_ = sprintf( "%.4f", $cwords_[1] );
        }

        if( index ( $currency_, "USDCAD" ) == 0 ){
          $CD_TO_DOL_ = sprintf( "%.4f", 1 / $cwords_[1] );
        }

	if( index ( $currency_, "GBPUSD" ) == 0 ){
	    $GBP_TO_DOL_ = sprintf( "%.4f", $cwords_[1] );
	}

	if( index ( $currency_, "USDHKD" ) == 0 ){
	    $HKD_TO_DOL_ = sprintf( "%.4f", 1/$cwords_[1] );
	}

        if( index ( $currency_, "USDJPY" ) == 0 ){
            $JPY_TO_DOL_ = sprintf( "%.4f", 1 / $cwords_[1] );
        }

      }
  }

#======================================================================================================================#
while ( $period_ eq "ONLINE" && $time1_ < 220000 )
{

    %shortcode_to_pnl_ = ();
    %shortcode_to_last_pos_ = ();
    %shortcode_to_last_price_ = ();
    %shortcode_to_n2d_ = ();
    %shortcode_to_volume_ = ();
    %shortcode_to_pos_ = ();
    %shortcode_to_spos_ = ();
    %shortcode_to_bpos_ = ();
    %shortcode_to_exchange_ = ();
    
my %fee_discrepancy_shortcodes_ = ();

    if ( $run_ eq "RETRIEVE" )
    {
	`/home/dvcinfra/infracore/scripts/newedge_pnl_per_prod_csv.sh` ;
	$mfg_trans_filename_ = "/apps/data/MFGlobalTrades/MFGFiles/GMITRN_$expected_date_.csv" ;
	
    }
    
    
    my @keys_ = `awk -F, '{ if ( \$25 != \"\\\"FFC\\\"\" ) { print \$24\$25\$7 } } ' $mfg_trans_filename_  | sort | uniq ` ;

#CASH
#awk -F, '{ if ( $24 == "\"S9\"" || $24 == "\"FEXCH\"") { print $12$13$24$18$26$112 } } ' /apps/data/MFGlobalTrades/MFGFiles/GMITRN_20130514.csv  | head -n2
 

    chomp ( @keys_ ) ;
    for ( my $idx = 0 ; $idx < scalar ( @keys_ ) ; $idx ++ )
    {
	next if ( length ( $keys_[ $idx ] ) < 5 ) ;
	
	$keys_[ $idx ] =~ s/"//g ;
	
	my $exid_ = substr $keys_[ $idx ] , 0 , 2 ;
	my $prodid_ = substr $keys_[ $idx ] , 2 , 2 ; 
	my $contract_ = substr $keys_[ $idx ] , 4 ;
	
	next if ( $exid_ eq "S9" ) ;
	
	if ( $exid_ ne "22" )
	{
	    my $key = `awk -F, '{ if ( \$25 == \"\\\"$prodid_\\\"\" &&  \$24 == \"\\\"$exid_\\\"\" && \$7 == \"\\\"$contract_\\\"\" ) { print \$24\$25\$7 } } ' $mfg_trans_filename_  | sort | uniq ` ;
	    
	    chomp ( $key ) ;
#	print $key, " " , $keys_[$idx], "\t" ;
	    $key =~ s/"//g ;
	    
	    my $bpos = 0 ;
	    my $spos = 0 ;
	    
	    
	    $bpos = `awk -F, '{ if ( \$25 == \"\\\"$prodid_\\\"\" && \$24 == \"\\\"$exid_\\\"\" && \$13 == \"\\\"1\\\"\" && \$1 == \"\\\"T\\\"\" && \$12 == \"\\\"$expected_date_\\\"\" && \$7 == \"\\\"$contract_\\\"\" )  sum+=\$18 ; } END { print sum }' $mfg_trans_filename_ `;
	    $spos = `awk -F, '{ if ( \$25 == \"\\\"$prodid_\\\"\" && \$24 == \"\\\"$exid_\\\"\" && \$13 == \"\\\"2\\\"\" && \$1 == \"\\\"T\\\"\" && \$12 == \"\\\"$expected_date_\\\"\" && \$7 == \"\\\"$contract_\\\"\" )  sum+=\$18 ; } END { print sum }' $mfg_trans_filename_ `;
	    
	    chomp ( $bpos ) ;
	    chomp ( $spos ) ;
	    
	    my $shortcode_ = $mfg_exch_future_code_to_shortcode_{$exid_.$prodid_}.$contract_ ;
	    
	    $shortcode_to_bpos_{$shortcode_} = min ( $bpos , $spos ) ;
	    $shortcode_to_spos_{$shortcode_} = min ( $bpos , $spos ) ;
#	print $mfg_exch_future_code_to_shortcode_{$exid_.$prodid_}," ",$shortcode_to_spos_{$shortcode_}," ",$shortcode_to_bpos_{$shortcode_}," ",max ( $bpos , $spos ),"\n" ;
	}
	else
	{
	    my $key = `awk -F, '{ if ( \$25 == \"\\\"$prodid_\\\"\" &&  \$24 == \"\\\"$exid_\\\"\" && \$7 == \"\\\"$contract_\\\"\" ) { print \$24\$25\$7 } } ' $mfg_trans_filename_  | sort | uniq ` ;
	    
	    chomp ( $key ) ;
#	print $key, " " , $keys_[$idx], "\t" ;
	    $key =~ s/"//g ;
	    
	    my $bpos = 0 ;
	    my $spos = 0 ;
	    
# awk -F',' '{ if ( $24 == "\"22\""  && $25 == "\"NI\"" && $13 == "\"2\"" )  { print $12" " $51" "$25" "$18; } }' /apps/data/MFGlobalTrades/MFGFiles/GMITRN_20130514.csv   | sort -k2n | head -n50 | awk '{ ORS = " " ; print $3" "$4; system ( "perl scripts/convert_datetime_tz.pl "$1" "$2" TOK NY") }' | awk '{ if ( $3 == "20130513" ) sum+=$2 ; } END { print sum  }'
	    
	
	    $bpos = `awk -F, '{ if ( \$25 == \"\\\"$prodid_\\\"\" && \$24 == \"\\\"$exid_\\\"\" && \$13 == \"\\\"1\\\"\" && \$1 == \"\\\"T\\\"\" ) { print \$12\" \"\$51\" \"\$25\" \"\$18; } }' $mfg_trans_filename_ | sort -k2n | awk '{ ORS = \" \" ; print \$3\" \"\$4 ; system ( \"perl /home/dvcinfra/infracore/scripts/convert_datetime_tz.pl \"\$1\" \"\$2\" TOK NY\" ) } ' | awk '{ if ( \$3 == $expected_date_ )  sum+=\$2 ; } END { print sum }'`;
	    
	    $spos = `awk -F, '{ if ( \$25 == \"\\\"$prodid_\\\"\" && \$24 == \"\\\"$exid_\\\"\" && \$13 == \"\\\"2\\\"\" && \$1 == \"\\\"T\\\"\" ) { print \$12\" \"\$51\" \"\$25\" \"\$18; } }' $mfg_trans_filename_ | sort -k2n | awk '{ ORS = \" \" ; print \$3\" \"\$4 ; system ( \"perl /home/dvcinfra/infracore/scripts/convert_datetime_tz.pl \"\$1\" \"\$2\" TOK NY\" ) } ' | awk '{ if ( \$3 == $expected_date_ )  sum+=\$2 ; } END { print sum }'`;
	    
	    chomp ( $bpos ) ;
	    chomp ( $spos ) ;
	    
	    my $shortcode_ = $mfg_exch_future_code_to_shortcode_{$exid_.$prodid_."_T"}.$contract_ ;
	    
	    $shortcode_to_bpos_{$shortcode_} = min ( $bpos , $spos ) ;
	    $shortcode_to_spos_{$shortcode_} = min ( $bpos , $spos ) ;
#	print $mfg_exch_future_code_to_shortcode_{$exid_.$prodid_}," ",$shortcode_to_spos_{$shortcode_}," ",$shortcode_to_bpos_{$shortcode_}," ",max ( $bpos , $spos ),"\n" ;
	    
	    
	    $bpos = `awk -F, '{ if ( \$25 == \"\\\"$prodid_\\\"\" && \$24 == \"\\\"$exid_\\\"\" && \$13 == \"\\\"1\\\"\" && \$1 == \"\\\"T\\\"\" ) { print \$12\" \"\$51\" \"\$25\" \"\$18; } }' $mfg_trans_filename_ | sort -k2n | awk '{ ORS = \" \" ; print \$3\" \"\$4 ; system ( \"perl /home/dvcinfra/infracore/scripts/convert_datetime_tz.pl \"\$1\" \"\$2\" TOK NY\" ) } ' | awk '{ if ( \$3 == $prevdate_ )  sum+=\$2 ; } END { print sum }'`;

	    $spos = `awk -F, '{ if ( \$25 == \"\\\"$prodid_\\\"\" && \$24 == \"\\\"$exid_\\\"\" && \$13 == \"\\\"2\\\"\" && \$1 == \"\\\"T\\\"\" ) { print \$12\" \"\$51\" \"\$25\" \"\$18; } }' $mfg_trans_filename_ | sort -k2n | awk '{ ORS = \" \" ; print \$3\" \"\$4 ; system ( \"perl /home/dvcinfra/infracore/scripts/convert_datetime_tz.pl \"\$1\" \"\$2\" TOK NY\" ) } ' | awk '{ if ( \$3 == $prevdate_ )  sum+=\$2 ; } END { print sum }'`;
	    
	    chomp ( $bpos ) ;
	    chomp ( $spos ) ;
	    
	    $shortcode_ = $mfg_exch_future_code_to_shortcode_{$exid_.$prodid_."_Y"}.$contract_ ;
	    
	    $shortcode_to_bpos_{$shortcode_} = min ( $bpos , $spos ) ;
	    $shortcode_to_spos_{$shortcode_} = min ( $bpos , $spos ) ;
	}
	
    }

# pnl computation from the MFG trades files are simpler, given the commission amounts
    open MFG_TRADES_FILE_HANDLE, "< $mfg_trans_filename_" or die "could not open mfg_trans_filename_ $mfg_trans_filename_\n";
    
    my @mfg_trades_file_lines_ = <MFG_TRADES_FILE_HANDLE>;
    
    close MFG_TRADES_FILE_HANDLE;
    
    for (my $i = 0; $i <= $#mfg_trades_file_lines_; $i++) {
	
	my @fields_ = split (',', $mfg_trades_file_lines_[$i]);
	
	
	if ($#fields_ != 116 && $#fields_ != 115) {
#	    print "Malformatted line : $mfg_trades_file_lines_[$i] No. of fields : $#fields_\n";
	    next;
	}
	
	if ( length ($fields_[$mfg_future_code_offset_]) < 2 ) {
#	next;
	}
	
	
	# NewEdge trade entries are listed twice in the file.
	# We only use the one specifying trade information.
	my $record_id_ = substr ($fields_[$record_id_offset_], 1, -1); $record_id_ =~ s/^\s+|\s+$//g;
	
	if ($record_id_ ne "T") {
	    next;
	}
	
	my $exch_code_ = substr ( $fields_ [ $exch_code_offset_ ] , 1 , -1 ); $exch_code_ =~ s/^\s+|\s+$//g;
	# Handling use of same future code for multiple products.
	# E.g. FESX & EURIBOR use "SF"
	my $future_code_ = "NA" ;
	my $exch_future_code_ = "NA" ;
	

	if ( $exch_code_ eq "S9" )
	{
	    #awk -F, '{ if ( $24 == "\"S9\"" || $24 != "\"FEXCH\"") { print $12$13$24$18$26$112 } } ' /apps/data/MFGlobalTrades/MFGFiles/GMITRN_20130514.csv  | head -n2
	    $future_code_ = $fields_[ 111 ]; $future_code_ =~ s/^\s+|\s+$//g ; $future_code_ =~ s/"//g ;
	    $exch_future_code_ = $exch_code_.$future_code_ ;
#	print $exch_future_code_,"\n" ;
	    
	    my $shortcode_ = $mfg_exch_future_code_to_shortcode_{$exch_future_code_};
	    my $number_to_dollar_ = $mfg_exch_future_code_to_n2d_{$exch_future_code_};
	    $shortcode_to_n2d_{$shortcode_} = $number_to_dollar_ ;
	    
	    if ( ! exists $shortcode_to_pnl_{$shortcode_} ) 
	    {
		$shortcode_to_pnl_{$shortcode_} = 0.0;
	    }
	    
	    my $buysell_ = substr ($fields_[$buysell_offset_], 1, -1); $buysell_ =~ s/^\s+|\s+$//g;
	    my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;
	    
	    if ( $buysell_ == 1 )
	    {
		$shortcode_to_pnl_{ $shortcode_ } -= ($quantity_ * $number_to_dollar_);
		$shortcode_to_volume_{$shortcode_} += 1;
		$shortcode_to_pos_{$shortcode_} = 0 ;
		$shortcode_to_last_pos_{$shortcode_} =0;
		
	    }
	    else
	    {
		$shortcode_to_pnl_{ $shortcode_ } += ($quantity_ * $number_to_dollar_);
		$shortcode_to_volume_{$shortcode_} -= 1;
		$shortcode_to_pos_{$shortcode_} = 0 ;
		$shortcode_to_last_pos_{$shortcode_} =0;
	    }
	    
	    if ( ! exists ( $shortcode_to_exchange_{$shortcode_} ) ) 
	    {
		$shortcode_to_exchange_{$shortcode_} = $mfg_exch_future_code_to_exchange_{$exch_future_code_} ;
	    }
	    
	    next ;
	}


	my $ose_b = "" ;
	
	if ( $exch_code_ eq "22" )
	{
	    my $tdate_ =  `perl ~/infracore/scripts/convert_datetime_tz.pl $fields_[ 11 ] $fields_[ 50 ] TOK NY | awk '{ print \$1 }'` ;
	    chomp ( $tdate_ ) ;
	    if ( $tdate_ eq $expected_date_ ) 
	    {
		$ose_b = "_T";
	    }
	    elsif ( $tdate_ eq $prevdate_ )
	    {
		$ose_b = "_Y";
	    }
	    else
	    {
		next ;
	    }
	}
	
	
	$future_code_ = substr ($fields_[$mfg_future_code_offset_], 1, -1); $future_code_ =~ s/^\s+|\s+$//g;
	$exch_future_code_ = $exch_code_.$future_code_.$ose_b ;

	if ( ! exists ( $mfg_exch_future_code_to_shortcode_{$exch_future_code_} ) ) 
	{
	    print "$exch_future_code_";
	    next;
	}
	
	my $date_ = substr ($fields_[$date_offset_], 1, -1); $date_ =~ s/^\s+|\s+$//g;
	
	if ($date_ ne $expected_date_) 
	{
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
	if ( ! exists $shortcode_to_pnl_{$shortcode_} ) {
	    $shortcode_to_pnl_{$shortcode_} = 0.0;
	}

	

	# Add the trade price into the pnl
	my $buysell_ = substr ($fields_[$buysell_offset_], 1, -1); $buysell_ =~ s/^\s+|\s+$//g;
	my $trade_price_ = $fields_[$trade_price_offset_]; $trade_price_ =~ s/^\s+|\s+$//g;
	my $closing_price_ = 0 ;
	
	$shortcode_to_last_price_{$shortcode_} =  $closing_price_ ; 
	my $quantity_ = $fields_[$quantity_offset_]; $quantity_ =~ s/^\s+|\s+$//g;
	
	if ( $buysell_ == 1 )  
	{ # Buy
	    
	    if ( $shortcode_to_bpos_{$shortcode_} >= $quantity_ ) 
	    {
		$shortcode_to_bpos_{$shortcode_} -= $quantity_ ; 
		$shortcode_to_pnl_{ $shortcode_ } -= ($trade_price_ * $quantity_ * $number_to_dollar_);
		$shortcode_to_last_pos_{$shortcode_} += $quantity_ ;  #would we used to compute flat pnl if we hold on to some positions
		$shortcode_to_pos_{ $shortcode_ } += 0 ;
		$shortcode_to_volume_{$shortcode_} += $quantity_;
	    }
	    elsif ( $shortcode_to_bpos_{$shortcode_} > 0 )
	    {
		my $qty_ =  $shortcode_to_bpos_ { $shortcode_ } ;
		
		$shortcode_to_bpos_{$shortcode_} -= $qty_ ; 
		$shortcode_to_pnl_{ $shortcode_ } -= ($trade_price_ * $qty_ * $number_to_dollar_);
		$shortcode_to_last_pos_{$shortcode_} += $qty_ ;  #would we used to compute flat pnl if we hold on to some positions
		$shortcode_to_pos_{ $shortcode_ } += 0 ;
		$shortcode_to_volume_{$shortcode_} += $qty_;
		
		my $qty1_ = $quantity_ - $qty_ ;
		$shortcode_to_pos_{ $shortcode_ } += $qty1_ ;
	    
	    }
	    else
	    {
		$shortcode_to_pos_{ $shortcode_ } += $quantity_ ;
	    }
	} 
	elsif ( $buysell_ == 2 ) 
	{ # Sell
	    if ( $shortcode_to_spos_{$shortcode_} >= $quantity_ ) 
	    {
		$shortcode_to_spos_{$shortcode_} -= $quantity_ ; 
		$shortcode_to_pnl_{$shortcode_} += ($trade_price_ * $quantity_ * $number_to_dollar_);
		$shortcode_to_last_pos_{$shortcode_} -= $quantity_ ;
		$shortcode_to_pos_{ $shortcode_ } -= 0 ;
		$shortcode_to_volume_{$shortcode_} += $quantity_;
	    }
	    elsif ( $shortcode_to_spos_{$shortcode_} > 0 )
	    {
		my $qty_ =  $shortcode_to_spos_ { $shortcode_ } ;
	    
		$shortcode_to_spos_{$shortcode_} -= $qty_ ; 
		$shortcode_to_pnl_{ $shortcode_ } += ($trade_price_ * $qty_ * $number_to_dollar_);
		$shortcode_to_last_pos_{$shortcode_} -= $qty_ ;  #would we used to compute flat pnl if we hold on to some positions
		$shortcode_to_pos_{ $shortcode_ } -= 0 ;
		$shortcode_to_volume_{$shortcode_} += $qty_;
		
		my $qty1_ = $quantity_ - $qty_ ;
		$shortcode_to_pos_{ $shortcode_ } -= $qty1_ ;
		
	    }
	    else
	    {
		$shortcode_to_pos_{ $shortcode_ } -= $quantity_ ;
	    }
	} 
	else 
	{
	    print " error : $buysell_, $quantity_ ;\n";
	    next ;
	    
	}

	$shortcode_to_pnl_{$shortcode_} -= GetCommissionForShortcode( $mfg_exch_future_code_to_shortcode_{$exch_code_.$future_code_} ) * $quantity_ ;
	
	if  ( !exists ( $shortcode_to_exchange_{$shortcode_} ) ) 
	{
	    $shortcode_to_exchange_{$shortcode_} = $mfg_exch_future_code_to_exchange_{$exch_future_code_};
	}
	
    }

    my $mail_body_ = "";
    foreach my $shortcode_ ( keys %fee_discrepancy_shortcodes_ )
    {
	my $expected_comm_fee_ = GetCommissionForShortcode( $shortcode_ );
	$mail_body_ = $mail_body_."Discrepancy for $shortcode_: in MFG file $fee_discrepancy_shortcodes_{$shortcode_}, expected: $expected_comm_fee_\n";
    }
    
    if( $mail_body_ )
    {
	open ( MAIL , "|/usr/sbin/sendmail -t" );
#    print MAIL "To: nseall@tworoads.co.in, sghosh\@circulumvite.com ravi.parikh\@tworoads.co.in\n";
	print MAIL "To: nseall@tworoads.co.in, kp\@circulumvite.com\n";
	print MAIL "Subject: comm fee discrepancy - $expected_date_\n\n";
	print MAIL $mail_body_;
	close(MAIL);
    }
    
    
    
    my ( $yyyy , $mm , $dd ) =  BreakDateYYYYMMDD ( $expected_date_ ) ;
    my $CSV_PROD_FILE_DIR = "/apps/data/MFGlobalTrades/ProductPnl/$yyyy/$mm/$dd" ;
    
    ( $yyyy , $mm , $dd ) =  BreakDateYYYYMMDD ( $prevdate_ ) ;
    my $CSV_HK_PROD_FILE_DIR="/apps/data/MFGlobalTrades/ProductPnl/$yyyy/$mm/$dd" ;
    
    
    `mkdir -p $CSV_PROD_FILE_DIR` ;
    `mkdir -p $CSV_HK_PROD_FILE_DIR` ;
    
    my $CSV_PROD_FILE = "$CSV_PROD_FILE_DIR/pnl_$period_.csv";
    my $CSV_HK_PROD_FILE = "$CSV_HK_PROD_FILE_DIR/pnl_$period_.csv";
    
#    open ( FH_HK, ">> $CSV_HK_PROD_FILE" ) ;
#    print FH_HK "\n\n\n\n $time_ \n" ;
    
#    open ( FH, ">> $CSV_PROD_FILE " ) ;
#    print FH "\n\n\n\n $time_ \n" ;
    
    print "\n\n\n\n", $time_ ,"\n";
    
    my $shortcode_ = "";
    foreach $shortcode_ (sort keys %shortcode_to_pnl_) 
    {
	if( $shortcode_to_last_pos_{$shortcode_} != 0) 
	{ 
	    print "non zero pos :: $shortcode_ \n"
	}
	
	if ($shortcode_to_exchange_{$shortcode_} eq "EUREX") 
	{
	    $shortcode_to_pnl_{$shortcode_} *= $EUR_TO_DOL_;
	}
	if ($shortcode_to_exchange_{$shortcode_} eq "LIFFE") 
	{
	    my $conversion_to_use_ = $EUR_TO_DOL_;
	    
	    if ( index ( $shortcode_ , "LFL" ) >= 0 ||
		 index ( $shortcode_ , "LFR" ) >= 0 ||
		 index ( $shortcode_ , "LFZ" ) >= 0 )
	    {
		# Values are still in gbp, convert to dollars
		$conversion_to_use_ = $GBP_TO_DOL_;
	    }	    
	    $shortcode_to_pnl_ { $shortcode_ } *= $conversion_to_use_;
	}
	
	if ($shortcode_to_exchange_{$shortcode_} eq "TMX") 
	{
	    $shortcode_to_pnl_{$shortcode_} *= $CD_TO_DOL_;
	}
	
	if ($shortcode_to_exchange_{$shortcode_} eq "HKEX") 
	{
	    $shortcode_to_pnl_{$shortcode_} *= $HKD_TO_DOL_;
	}
	
	if ($shortcode_to_exchange_{$shortcode_} eq "OSE") 
	{
	    $shortcode_to_pnl_{$shortcode_} *= $JPY_TO_DOL_ ;
	}

	if ($shortcode_to_exchange_{$shortcode_} eq "CME")
	{
	    if ( index ( $shortcode_, "NIY" ) >= 0 ) 
	    {
		$shortcode_to_pnl_{$shortcode_} *= $JPY_TO_DOL_ ;
	    }
	}
	
	if ( $shortcode_to_exchange_{$shortcode_} eq "DVCAP" )
	{
	
	}
	
	
	if ( $shortcode_to_exchange_{$shortcode_} eq "HKEX" || ( ( $shortcode_to_exchange_{$shortcode_} eq "OSE" ) && ( index ( $shortcode_, 'Y_' ) >  -1 ) ) )
	{
	    printf ( "| NY_DATE: %8d | ", $prevdate_ );
	    
	    printf color 'bold white' ;
	    printf ( "%12s", $shortcode_ );
	    print color 'reset' ;
	    
	    printf ( " | PNL: " ) ;
	    
	    if ( $shortcode_to_pnl_{$shortcode_} > 0 )
	    {
		print color 'bold blue' ;
	    }
	    else
	    {
		print color 'bold red' ;
	    }
	    printf ( "%11.3f",$shortcode_to_pnl_{$shortcode_} ) ;
	    print color 'reset' ;
	    
	    printf ( " | VOLUME: %8d | POSITION: %4d\n",  $shortcode_to_volume_{$shortcode_}, $shortcode_to_pos_{$shortcode_} ) ;

#	    print FH_HK "$prevdate_,$shortcode_,$shortcode_to_pnl_{$shortcode_},$shortcode_to_volume_{$shortcode_},$shortcode_to_pos_{$shortcode_}\n" ;
	    
	    
	}
	else
	{
	    printf ( "| NY_DATE: %8d | ", $expected_date_ );
	    
	    printf color 'bold white' ;
	    printf ( "%12s", $shortcode_ );
	    print color 'reset' ;
	    
	    printf ( " | PNL: " ) ;
	    
	    if ( $shortcode_to_pnl_{$shortcode_} > 0 )
	    {
		print color 'bold blue' ;
	    }
	    else
	    {
		print color 'bold red' ;
	    }
	    printf ( "%11.3f",$shortcode_to_pnl_{$shortcode_} ) ;
	    print color 'reset' ;
	    
	    printf ( " | VOLUME: %8d | POSITION: %4d\n",  $shortcode_to_volume_{$shortcode_}, $shortcode_to_pos_{$shortcode_} ) ;
	    
#	printf ( "%d \t %s \t %.4f \t %d \t %d\n",  $expected_date_,$shortcode_,$shortcode_to_pnl_{$shortcode_},$shortcode_to_volume_{$shortcode_},$shortcode_to_pos_{$shortcode_} ) ;
#	    print FH "$expected_date_,$shortcode_,$shortcode_to_pnl_{$shortcode_},$shortcode_to_volume_{$shortcode_},$shortcode_to_pos_{$shortcode_}\n" ;
	    
	    
	}



    }
    
 #   close ( FH_HK ) ;
 #   close ( FH ) ;

    sleep ( 60 ) ;
    $time1_ = `date +'%H%M%S'` ;
    chomp ( $time1_ ) ;
    $time_ = `date +'%T'` ;
    chomp ( $time_ ) ;
}

    
