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
require "$GENPERLLIB_DIR/calc_next_business_day.pl"; # for exchange rates
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # 
require "$GENPERLLIB_DIR/get_commission_for_shortcode.pl"; # 
require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr
require "$GENPERLLIB_DIR/get_commission_for_DI.pl"; # 

my $date_ = "TODAY-1" ;
if ( scalar ( @ARGV ) == 1 )
{
    $date_ = $ARGV [ 0 ] ;
}
$date_ = GetIsoDateFromStr ( $date_ );

# /NAS1/data/MFGlobalTrades/MFGFiles/GMIST4_T-1.csv
# /NAS1/data/MFGlobalTrades/MFGFiles/GMIST4_T.csv
# /NAS1/data/MFGlobalTrades/MFGFiles/GMIST4_T+1.csv
# awk -F, '{ gsub ( "\"", "" ) ; if ( $1 == "T" && $24 == "22" && $25 == "MN"  ) {  print $12" "$13" "$18" "$24" "$25" "$28" "$51 } }' /NAS1/data/MFGlobalTrades/MFGFiles/GMITRN_20150401.csv   | more

#print "InputDate: ".$date_."\n";

#print "NEWEDGE:: " ;
my $sanity_counter_ = 0 ;
my $tdate_ = $date_ ;
my $ne_tfile_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMIST4_".$tdate_.".csv" ;
while ( ! ExistsWithSize ( $ne_tfile_ ) && $sanity_counter_ < 10 )
{
    $tdate_ = CalcPrevBusinessDay ( $tdate_ ) ;
    $ne_tfile_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMIST4_".$tdate_.".csv" ;
    $sanity_counter_ = $sanity_counter_ + 1 ;
}

my $next_date_ = CalcNextBusinessDay ( $tdate_ ) ; # 
my $ne_tp1file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMIST4_".$next_date_.".csv" ;
if ( ! ExistsWithSize ( $ne_tp1file_ ) && $sanity_counter_ < 10 )
{
    $next_date_ = CalcNextBusinessDay ( $next_date_ ) ;
    $ne_tp1file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMIST4_".$next_date_.".csv" ;
    $sanity_counter_ = $sanity_counter_ + 1 ;
}

my $prev_date_ = CalcPrevBusinessDay ( $tdate_ ) ; # 
my $ne_tm1file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMIST4_".$prev_date_.".csv" ;
while ( ! ExistsWithSize ( $ne_tm1file_ ) && $sanity_counter_ < 10 )
{
    $prev_date_ = CalcPrevBusinessDay ( $prev_date_ ) ;
    $ne_tm1file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMIST4_".$prev_date_.".csv" ;
    $sanity_counter_ = $sanity_counter_ + 1 ;
}

if ( $sanity_counter_ > 10 )
{
    print $sanity_counter_." ".$prev_date_." ".$tdate_." ".$next_date_."\n";
    exit ( 0 ) ;
}

#print "Using: ".$prev_date_." ".$tdate_."\n";

my %mfg_exch_future_code_to_shortcode_ = ();
my %mfg_exch_future_code_to_n2d_ = ();
my %mfg_exch_future_code_to_exchange_ = ();


#OSE
$mfg_exch_future_code_to_shortcode_{"22MN"} = "NKM";
$mfg_exch_future_code_to_n2d_{"22MN"} = 100;
$mfg_exch_future_code_to_exchange_{"22MN"} = "OSE";


$mfg_exch_future_code_to_shortcode_{"22NI"} = "NK";   # newedge has 100000 should retreive from price_multiplier field ( 20 is the offset )
$mfg_exch_future_code_to_n2d_{"22NI"} = 100000;
$mfg_exch_future_code_to_exchange_{"22NI"} = "OSE";


$mfg_exch_future_code_to_shortcode_{"24RV"} = "JGBL";
$mfg_exch_future_code_to_n2d_{"24RV"} = 10000;
$mfg_exch_future_code_to_exchange_{"24RV"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"24TP"} = "TOPIX";
$mfg_exch_future_code_to_n2d_{"24TP"} = 10000;
$mfg_exch_future_code_to_exchange_{"24TP"} = "OSE";


#HKEX
$mfg_exch_future_code_to_shortcode_{"33MH"} = "MHI";
$mfg_exch_future_code_to_n2d_{"33MH"} = 1000;
$mfg_exch_future_code_to_exchange_{"33MH"} = "HKEX";

$mfg_exch_future_code_to_shortcode_{"33HC"} = "HHI";
$mfg_exch_future_code_to_n2d_{"33HC"} = 50;
$mfg_exch_future_code_to_exchange_{"33HC"} = "HKEX";

$mfg_exch_future_code_to_shortcode_{"33HS"} = "HSI";
$mfg_exch_future_code_to_n2d_{"33HS"} = 5000;
$mfg_exch_future_code_to_exchange_{"33HS"} = "HKEX";

#CME
$mfg_exch_future_code_to_shortcode_{"0126"} = "ZT";
$mfg_exch_future_code_to_n2d_{"0126"} = 2000;
$mfg_exch_future_code_to_exchange_{"0126"} = "CME";

$mfg_exch_future_code_to_shortcode_{"0125"} = "ZF";
$mfg_exch_future_code_to_n2d_{"0125"} = 1000;
$mfg_exch_future_code_to_exchange_{"0125"} = "CME";

$mfg_exch_future_code_to_shortcode_{"0121"} = "ZN";
$mfg_exch_future_code_to_n2d_{"0121"} = 1000;
$mfg_exch_future_code_to_exchange_{"0121"} = "CME";

$mfg_exch_future_code_to_shortcode_{"0117"} = "ZB";
$mfg_exch_future_code_to_n2d_{"0117"} = 1000;
$mfg_exch_future_code_to_exchange_{"0117"} = "CME";

$mfg_exch_future_code_to_shortcode_{"01UL"} = "UB";
$mfg_exch_future_code_to_n2d_{"01UL"} = 1000;
$mfg_exch_future_code_to_exchange_{"01UL"} = "CME";

$mfg_exch_future_code_to_shortcode_{"16N1"} = "NIY";
$mfg_exch_future_code_to_n2d_{"16N1"} = 500;
$mfg_exch_future_code_to_exchange_{"16N1"} = "CME";  # one chicago

$mfg_exch_future_code_to_shortcode_{"16NK"} = "NKD";
$mfg_exch_future_code_to_n2d_{"16NK"} = 50;
$mfg_exch_future_code_to_exchange_{"16NK"} = "CME"; # one chicago


#$mfg_exch_future_code_to_shortcode_{"01CBT 2YR T-NOTE"} = "ZT";
#$mfg_exch_future_code_to_n2d_{"01CBT 2YR T-NOTE"} = 2000;
#$mfg_exch_future_code_to_exchange_{"01CBT 2YR T-NOTE"} = "CME";


#LIFFE
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

#TMX
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

#EUREX
$mfg_exch_future_code_to_shortcode_{"27SD"} = "FGBS";
$mfg_exch_future_code_to_n2d_{"27SD"} = 1000;
$mfg_exch_future_code_to_exchange_{"27SD"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27BC"} = "FGBM";
$mfg_exch_future_code_to_n2d_{"27BC"} = 1000;
$mfg_exch_future_code_to_exchange_{"27BC"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27BM"} = "FGBL";
$mfg_exch_future_code_to_n2d_{"27BM"} = 1000;
$mfg_exch_future_code_to_exchange_{"27BM"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27BV"} = "FGBX";
$mfg_exch_future_code_to_n2d_{"27BV"} = 1000;
$mfg_exch_future_code_to_exchange_{"27BV"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27SF"} = "FESX";
$mfg_exch_future_code_to_n2d_{"27SF"} = 10;
$mfg_exch_future_code_to_exchange_{"27SF"} = "EUREX";

$mfg_exch_future_code_to_shortcode_{"27>O"} = "FXXP";
$mfg_exch_future_code_to_n2d_{"27>O"} = 50;
$mfg_exch_future_code_to_exchange_{"27>O"} = "EUREX";

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

$mfg_exch_future_code_to_shortcode_{"27VS"} = "FVS";
$mfg_exch_future_code_to_n2d_{"27VS"} = 1000;
$mfg_exch_future_code_to_exchange_{"27VS"} = "EUREX";


#CFE
$mfg_exch_future_code_to_shortcode_{"7BVX"} = "VX";
$mfg_exch_future_code_to_n2d_{"7BVX"} = 1000;
$mfg_exch_future_code_to_exchange_{"7BVX"} = "CFE";

#CASH
$mfg_exch_future_code_to_shortcode_{"S9HKD"} = "HKDUSD";
$mfg_exch_future_code_to_n2d_{"S9HKD"} = 1;
$mfg_exch_future_code_to_exchange_{"S9HKD"} = "DVCAP";

$mfg_exch_future_code_to_shortcode_{"S9JPY"} = "JPYUSD";
$mfg_exch_future_code_to_n2d_{"S9JPY"} = 1;



# this file should be print the following
my $check_ = 1 ;
my $print_prod_info_ = 1 ;
my $print_ose_prod_info_ = 1 ;
my $print_hk_prod_info_ = 1 ;
my $print_cfe_prod_info_ = 1 ;
my $print_bmf_prod_info_ = 1 ;
my $print_mos_prod_info_ = 1 ;
my $collect_unaccounted_ = 1 ;
my $print_commissions_ = 0 ;
#ST4
my %st4_flds_ = ( ) ;

$st4_flds_{"PRECID"} = 1;
$st4_flds_{"PACCT"} = 4;
$st4_flds_{"PCTYM"} = 7;
$st4_flds_{"PEXPDT"} = 11;
$st4_flds_{"PTDATE"} = 12;
$st4_flds_{"PBS"} = 13;
$st4_flds_{"PQTY"} = 18;
$st4_flds_{"PSDSC1"} = 19;
$st4_flds_{"PMULTF"} = 92;
$st4_flds_{"PSDATE"} = 21;
$st4_flds_{"PLTDAT"} = 22;
$st4_flds_{"PEXCH"} = 23;
$st4_flds_{"PFC"} = 24;
$st4_flds_{"PSYMBL"} = 25;
$st4_flds_{"PPTYPE"} = 26;
$st4_flds_{"PTPRIC"} = 27;
$st4_flds_{"PCLOSE"} = 28;
$st4_flds_{"PCOMM"} = 37;
$st4_flds_{"PFEE1"} = 38;
$st4_flds_{"PFEE2"} = 39;
$st4_flds_{"PFEE3"} = 40;
$st4_flds_{"PFEE4"} = 41;
$st4_flds_{"PRODCT"} = 48;
$st4_flds_{"PTIME"} = 50;

# TO CHECK PROD_DESC  # SDSC1 MULTF EXCH SYMBL COMM FEE1 FEE2 FEE3 FEE4 PRODCT
# TO COMPUTE PNLS # RECID TDATE BS QTY FC EXCH CTYM TPRICE CLOSE COMM FEE1 FEE2 FEE3 FEE4

my $filebase_name_ = `basename $ne_tfile_` ; chomp ( $filebase_name_ ) ;
my @filename_tkns_ = split ( "_" , $filebase_name_ ) ;

#print "Processing ST4 file\n";
if ( $check_ == 1 )
{
    open ( my $NEFILE , "$ne_tfile_" ) ;
    while ( my $line_ = <$NEFILE> )
    {
	chomp ( $line_ ) ;
	$line_ =~ s/"//g ; #gsub ( "\"" , "" , $line_) ;
	my @tkns_ = split ( "," , $line_ ) ;
	if ( $tkns_[$st4_flds_{"PRECID"} -1 ] eq "PRECID" &&
	     $tkns_[$st4_flds_{"PTDATE"} -1 ] eq "PTDATE" &&
	     $tkns_[$st4_flds_{"PBS"} -1 ] eq "PBS" &&
	     $tkns_[$st4_flds_{"PQTY"} -1 ] eq "PQTY" &&
	     $tkns_[$st4_flds_{"PFC"} -1 ] eq "PFC" &&
	     $tkns_[$st4_flds_{"PEXCH"} -1 ] eq "PEXCH" &&
	     $tkns_[$st4_flds_{"PCTYM"} -1 ] eq "PCTYM" &&
	     $tkns_[$st4_flds_{"PTPRIC"} -1 ] eq "PTPRIC" &&
	     $tkns_[$st4_flds_{"PCLOSE"} -1 ] eq "PCLOSE" &&
	     $tkns_[$st4_flds_{"PCOMM"} -1 ] eq "PCOMM" &&
	     $tkns_[$st4_flds_{"PFEE1"} -1 ] eq "PFEE1" &&
	     $tkns_[$st4_flds_{"PFEE2"} -1 ] eq "PFEE2" &&
	     $tkns_[$st4_flds_{"PFEE3"} -1 ] eq "PFEE3" &&
	     $tkns_[$st4_flds_{"PFEE4"} -1 ] eq "PFEE4" &&
	     $tkns_[$st4_flds_{"PSDSC1"} -1 ] eq "PSDSC1" &&
	     $tkns_[$st4_flds_{"PMULTF"} -1 ] eq "PMULTF" &&
	     $tkns_[$st4_flds_{"PEXCH"} -1 ] eq "PEXCH" &&
	     $tkns_[$st4_flds_{"PSYMBL"} -1 ] eq "PSYMBL" &&
	     $tkns_[$st4_flds_{"PCOMM"} -1 ] eq "PCOMM" &&
	     $tkns_[$st4_flds_{"PFEE1"} -1 ] eq "PFEE1" &&
	     $tkns_[$st4_flds_{"PFEE2"} -1 ] eq "PFEE2" &&
	     $tkns_[$st4_flds_{"PFEE3"} -1 ] eq "PFEE3" &&
	     $tkns_[$st4_flds_{"PFEE4"} -1 ] eq "PFEE4" &&
	     $tkns_[$st4_flds_{"PRODCT"} -1 ] eq "PRODCT" )
	{
	    #print "File has expected fields in order\n" ;
	}
	else
	{
	    print "File doent have expected fields in order\n" ;
	    print $tkns_[$st4_flds_{"PRECID"} -1 ]," PRECID\n",
	    $tkns_[$st4_flds_{"PTDATE"} -1 ]," PTDATE\n",
	    $tkns_[$st4_flds_{"PBS"} -1 ]," PBS\n",
	    $tkns_[$st4_flds_{"PQTY"} -1 ]," PQTY\n",
	    $tkns_[$st4_flds_{"PFC"} -1 ]," PFC\n",
	    $tkns_[$st4_flds_{"PEXCH"} -1 ]," PEXCH\n",
	    $tkns_[$st4_flds_{"PCTYM"} -1 ]," PCTYM\n",
	    $tkns_[$st4_flds_{"PTPRIC"} -1 ]," PTPRIC\n",
	    $tkns_[$st4_flds_{"PCLOSE"} -1 ]," PCLOSE\n",
	    $tkns_[$st4_flds_{"PCOMM"} -1 ]," PCOMM\n",
	    $tkns_[$st4_flds_{"PFEE1"} -1 ]," PFEE1\n",
	    $tkns_[$st4_flds_{"PFEE2"} -1 ]," PFEE2\n",
	    $tkns_[$st4_flds_{"PFEE3"} -1 ]," PFEE3\n",
	    $tkns_[$st4_flds_{"PFEE4"} -1 ]," PFEE4\n",
	    $tkns_[$st4_flds_{"PSDSC1"} -1 ]," PSDSC1\n",
	    $tkns_[$st4_flds_{"PMULTF"} -1 ]," PMULTF\n",
	    $tkns_[$st4_flds_{"PEXCH"} -1 ]," PEXCH\n",
	    $tkns_[$st4_flds_{"PSYMBL"} -1 ]," PSYMBL\n",
	    $tkns_[$st4_flds_{"PCOMM"} -1 ]," PCOMM\n",
	    $tkns_[$st4_flds_{"PFEE1"} -1 ]," PFEE1\n",
	    $tkns_[$st4_flds_{"PFEE2"} -1 ]," PFEE2\n",
	    $tkns_[$st4_flds_{"PFEE3"} -1 ]," PFEE3\n",
	    $tkns_[$st4_flds_{"PFEE4"} -1 ]," PFEE4\n",
	    $tkns_[$st4_flds_{"PRODCT"} -1 ]," PRODCT\n";
	}
	close ( $NEFILE ) ;
	last ;
    }
}

my %ne_dv_map_ = qw (
"NEWMILWHEAT" YFEBM
"MON CAC40 10EU" JFFCE
"AEX INDEX EURO" KFFTI
"ME 10Y CDN BND" CGB
"ME 3M BAX" BAX
"ME S&P CAN 60" SXF
"US 2YR T-NOTE" ZT
"5 YR NOTE" ZF
"10YR T-NOTES" ZN
"US T-BONDS" ZB
"CBT UL T-BONDS" UB
"IMM NIKKEI IND" NKD
"NIKKEI 225-YEN" NIY
"CFE VIX" VX
"EURX E-SCHATZ" FGBS
"EURX EURO-BOBL" FGBM
"EURX EURO-BUND" FGBL
"EURX EURO-BUXL" FGBX
"EUX FOAT" FOAT
"EUX EURO-BTP" FBTP
"EUX ST EUROBTP" FBTS
"EURX E-STXX 50" FESX
"EUX MINI VSTX" FVS
"LIF LONG GILT" LFR
"LIF 3M EURIBOR" LFI
"LIF 3M INT" LFL
"NEW FTSE 100" LFZ
"HKE H-SHARES" HHI
"OSE NIK 225" NK
"OSE MN NIK225" NKM
"OSE TOPIX" TOPIX
"OSE 10YR JGB" JGBL
);


my @prod_info_ = ( );
my @prod_ose_info_ = ( );
my @prod_hk_info_ = ( );
my @prod_cfe_info_ = ( );

my @unaccounted_ = ( ) ;

my %prods_ ;
my %prod_date_ ;
my %prod_trans_ ;
my %prod_mult_ ;
my %prod_exh_ ;
my %prod_ncode_ ;
my %prod_fee1_ ;
my %prod_fee2_ ;
my %prod_fee3_ ;
my %prod_fee4_ ;
my %prod_vol_ ;

# exceptions
# OSE
# if exch = 24 / 22 ( NK/NKM/NKMF || TOPIX/JGBL )
# ( ( time >= 900 && time < 1600 && T ) || ( time >=0 && time <= 900 ) && T+1 || ( time >=1600 T+1  ) ) 
#
# HK -> T-1 
# if exch = 33


# NON-OSE and HK
if ( $print_prod_info_ == 1 )
{
    #print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PSDSC1"} == \"PSDSC1\" ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}" | "\$$st4_flds_{"PFEE1"}" | "\$$st4_flds_{"PFEE2"}" | "\$$st4_flds_{"PFEE3"}" | "\$$st4_flds_{"PFEE4"}" | "\$$st4_flds_{"PRODCT"}}}' $ne_tfile_ | sort | uniq `;
    #print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PSDSC1"} != \"PSDSC1\" ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PRODCT"}}}' $ne_tfile_ | sort | uniq `;
    @prod_info_ = `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && \$$st4_flds_{"PEXCH"} != \"24\" && \$$st4_flds_{"PEXCH"} != \"33\" && \$$st4_flds_{"PEXCH"} != \"22\" && \$$st4_flds_{"PEXCH"} != \"7B\" ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PQTY"}}}' $ne_tfile_ `;
    #print $prod_info_[0]."\n";

    for ( my $i_ = 0 ; $i_ < scalar ( @prod_info_ ) ; $i_ ++ )
    {
	my @tks_ = split ( '\|', $prod_info_[ $i_ ] ) ;

	( my $temp_ = $tks_[2].$tks_[3] ) =~ s/\s//g ;
	#print $temp_." ".$mfg_exch_future_code_to_shortcode_{$temp_}."\n";
	# if ( any exceptions )
	# else
	if ( ! exists ( $mfg_exch_future_code_to_shortcode_{$temp_}  ) )
	{
	    if ( $collect_unaccounted_ == 1 )
	    {
		push ( @unaccounted_, $prod_info_[$i_] ) ;
		next ;
	    }
	    else
	    {
		print "unaccounted ".$prod_info_[$i_]."\n";
		exit ( 0 ) ;
	    }
	}

	my $key_ = $mfg_exch_future_code_to_shortcode_{$temp_} ;
	$prods_{ $key_ } = 1 ;

	if ( exists ( $prod_trans_{ $key_ } ) )
	{
	    $prod_trans_{ $key_ } += 1 ;
	    $prod_vol_ { $key_ } += int ( $tks_[ 9 ] ) ;
	    $prod_fee1_ { $key_ } += ( $tks_[ 4 ] - $prod_fee1_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee2_ { $key_ } += ( $tks_[ 5 ] - $prod_fee2_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee3_ { $key_ } += ( $tks_[ 6 ] - $prod_fee3_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee4_ { $key_ } += ( $tks_[ 7 ] - $prod_fee4_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_mult_ { $key_ } = int ( $tks_[ 1 ] ) ;
	    $prod_exh_ { $key_ } = $tks_[ 2 ] ;
	    $prod_ncode_ { $key_ } = $tks_[ 3 ] ;
	}
	else
	{
	    $prod_date_ { $key_ } = $tdate_ ;
	    $prod_trans_{ $key_ } = 1 ;
	    $prod_vol_ { $key_ } = $tks_[ 9 ] ;
	    $prod_fee1_ { $key_ } = $tks_[ 4 ] ;
	    $prod_fee2_ { $key_ } = $tks_[ 5 ] ;
	    $prod_fee3_ { $key_ } = $tks_[ 6 ] ;
	    $prod_fee4_ { $key_ } = $tks_[ 7 ] ;
	}
    }
}


#OSE
if ( $print_ose_prod_info_ == 1 )
{
    my $t1_ = 900 ;
    my $t2_ = 1600 ;

    my @ose_ss_ = ( ) ;

    # SH
    @ose_ss_ = `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"24\" || \$$st4_flds_{"PEXCH"} == \"22\" ) && int ( \$$st4_flds_{"PTIME"} ) >= $t1_ && int ( \$$st4_flds_{"PTIME"} ) < $t2_ ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PQTY"}}}' $ne_tm1file_ `;
    push ( @prod_ose_info_, @ose_ss_ ) ;

    # FH and TH
   @ose_ss_ = `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"24\" || \$$st4_flds_{"PEXCH"} == \"22\" ) && int ( \$$st4_flds_{"PTIME"} ) < $t1_ ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PQTY"}}}' $ne_tfile_ `;
    push ( @prod_ose_info_, @ose_ss_ ) ;

    @ose_ss_ = `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"24\" || \$$st4_flds_{"PEXCH"} == \"22\" ) && ( int ( \$$st4_flds_{"PTIME"} ) >= $t2_ ) ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PQTY"}}}' $ne_tfile_ `;
    push ( @prod_ose_info_, @ose_ss_ ) ;

    #print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"24\" || \$$st4_flds_{"PEXCH"} == \"22\" ) && ( \$$st4_flds_{"PFC"} == \"MN\" ) && int ( \$$st4_flds_{"PTIME"} ) <= 900 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tp1file_ 2>/dev/null`;
    #print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"24\" || \$$st4_flds_{"PEXCH"} == \"22\" ) && ( \$$st4_flds_{"PFC"} == \"MN\" ) && int ( \$$st4_flds_{"PTIME"} ) >= 900 && int ( \$$st4_flds_{"PTIME"} ) < 1600 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tp1file_ 2>/dev/null`;
    #print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"24\" || \$$st4_flds_{"PEXCH"} == \"22\" ) && ( \$$st4_flds_{"PFC"} == \"MN\" ) && int ( \$$st4_flds_{"PTIME"} ) >= 1600 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tp1file_ 2>/dev/null`;

    for ( my $i_ = 0 ; $i_ < scalar ( @prod_ose_info_ ) ; $i_ ++ )
    {
	my @tks_ = split ( '\|', $prod_ose_info_[ $i_ ] ) ;
	( my $temp_ = $tks_[2].$tks_[3] ) =~ s/\s//g ;
	#print $temp_." ".$mfg_exch_future_code_to_shortcode_{$temp_}."\n";
	# if ( any exceptions )
	# else
	if ( ! exists ( $mfg_exch_future_code_to_shortcode_{$temp_}  ) )
	{
	    if ( $collect_unaccounted_ == 1 )
	    {
		push ( @unaccounted_, $prod_info_[$i_] ) ;
		next ;
	    }
	    else
	    {
		print "unaccounted ".$prod_info_[$i_]."\n";
		exit ( 0 ) ;
	    }
	}
	my $key_ = $mfg_exch_future_code_to_shortcode_{$temp_} ;
	$prods_{ $key_ } = 1 ;

	if ( exists ( $prod_trans_{ $key_ } ) )
	{
	    $prod_trans_{ $key_ } += 1 ;
	    $prod_vol_ { $key_ } += int ( $tks_[ 9 ] ) ;
	    $prod_fee1_ { $key_ } += ( $tks_[ 4 ] - $prod_fee1_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee2_ { $key_ } += ( $tks_[ 5 ] - $prod_fee2_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee3_ { $key_ } += ( $tks_[ 6 ] - $prod_fee3_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee4_ { $key_ } += ( $tks_[ 7 ] - $prod_fee4_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_mult_ { $key_ } = int ( $tks_[ 1 ] ) ;
	    $prod_exh_ { $key_ } = $tks_[ 2 ] ;
	    $prod_ncode_ { $key_ } = $tks_[ 3 ] ;
	}
	else
	{
	    $prod_date_ { $key_ } = $prev_date_ ;
	    $prod_trans_{ $key_ } = 1 ;
	    $prod_vol_ { $key_ } = $tks_[ 9 ] ;
	    $prod_fee1_ { $key_ } = $tks_[ 4 ] ;
	    $prod_fee2_ { $key_ } = $tks_[ 5 ] ;
	    $prod_fee3_ { $key_ } = $tks_[ 6 ] ;
	    $prod_fee4_ { $key_ } = $tks_[ 7 ] ;
	}
    }
}

#HK
if ( $print_hk_prod_info_ == 1 )
{
    
    my $t1_ = 163000 ;
    my @hk_ss_ = ( ) ;

    @hk_ss_ = `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && \$$st4_flds_{"PEXCH"} == \"33\" && ( int ( \$$st4_flds_{"PTIME"} ) < $t1_ ) ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PQTY"}}}' $ne_tm1file_ `;
    push ( @prod_hk_info_, @hk_ss_ ) ;
    
    @hk_ss_ = `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && \$$st4_flds_{"PEXCH"} == \"33\" && ( int ( \$$st4_flds_{"PTIME"} ) > $t1_ ) ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PQTY"}}}' $ne_tfile_ `;
    push ( @prod_hk_info_, @hk_ss_ ) ;



    #print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) <= 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tm1file_ 2>/dev/null`;
#    print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) > 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tm1file_ 2>/dev/null`;

    #print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) <= 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tfile_ 2>/dev/null`;
#    print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) > 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tfile_ 2>/dev/null`;

#    print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) <= 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tp1file_ 2>/dev/null`;
#    print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) > 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tp1file_ 2>/dev/null`;


    for ( my $i_ = 0 ; $i_ < scalar ( @prod_hk_info_ ) ; $i_ ++ )
    {
	my @tks_ = split ( '\|', $prod_hk_info_[ $i_ ] ) ;

	( my $temp_ = $tks_[2].$tks_[3] ) =~ s/\s//g ;
	#print $temp_." ".$mfg_exch_future_code_to_shortcode_{$temp_}."\n";
	# if ( any exceptions )
	# else
	if ( ! exists ( $mfg_exch_future_code_to_shortcode_{$temp_}  ) )
	{
	    if ( $collect_unaccounted_ == 1 )
	    {
		push ( @unaccounted_, $prod_info_[$i_] ) ;
		next ;
	    }
	    else
	    {
		print "unaccounted ".$prod_info_[$i_]."\n";
		exit ( 0 ) ;
	    }
	}

	my $key_ = $mfg_exch_future_code_to_shortcode_{$temp_} ;
	$prods_{ $key_ } = 1 ;

	if ( exists ( $prod_trans_{ $key_ } ) )
	{
	    $prod_trans_{ $key_ } += 1 ;
	    $prod_vol_ { $key_ } += int ( $tks_[ 9 ] ) ;
	    $prod_fee1_ { $key_ } += ( $tks_[ 4 ] - $prod_fee1_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee2_ { $key_ } += ( $tks_[ 5 ] - $prod_fee2_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee3_ { $key_ } += ( $tks_[ 6 ] - $prod_fee3_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee4_ { $key_ } += ( $tks_[ 7 ] - $prod_fee4_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_mult_ { $key_ } = int ( $tks_[ 1 ] ) ;
	    $prod_exh_ { $key_ } = $tks_[ 2 ] ;
	    $prod_ncode_ { $key_ } = $tks_[ 3 ] ;
	}
	else
	{
	    $prod_date_ { $key_ } = $prev_date_ ;
	    $prod_trans_{ $key_ } = 1 ;
	    $prod_vol_ { $key_ } = $tks_[ 9 ] ;
	    $prod_fee1_ { $key_ } = $tks_[ 4 ] ;
	    $prod_fee2_ { $key_ } = $tks_[ 5 ] ;
	    $prod_fee3_ { $key_ } = $tks_[ 6 ] ;
	    $prod_fee4_ { $key_ } = $tks_[ 7 ] ;
	}
    }

}

# VX ( trades after EST_1630 ) 
if ( $print_cfe_prod_info_ == 1 ) 
{
    my $t1_ = 83000 ;
    my $t2_ = 151500 ;
    my @cfe_ss_ = ( ) ;

    #@cfe_ss_ = `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && \$$st4_flds_{"PEXCH"} == \"7B\" && ( int ( \$$st4_flds_{"PTIME"} ) < $t1_ ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PQTY"}}}' $ne_tm1file_ 2>/dev/null`;
    #push ( @prod_cfe_info_, @cfe_ss_ ) ;
    
    @cfe_ss_ = `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && \$$st4_flds_{"PEXCH"} == \"7B\" && ( int ( \$$st4_flds_{"PTIME"} ) >= $t1_ ) && ( int ( \$$st4_flds_{"PTIME"} ) <= $t2_ ) ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PQTY"}}}' $ne_tfile_ `;
    push ( @prod_cfe_info_, @cfe_ss_ ) ;

    #@cfe_ss_ = `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && \$$st4_flds_{"PEXCH"} == \"7B\" && ( int ( \$$st4_flds_{"PTIME"} ) > $t2_ ) ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PQTY"}}}' $ne_tp1file_ 2>/dev/null`;
    #push ( @prod_cfe_info_, @cfe_ss_ ) ;
    #print @cfe_ss_ ;
    #push ( @prod_cfe_info_, @cfe_ss_ ) ;



    #print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) <= 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tm1file_ 2>/dev/null`;
#    print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) > 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tm1file_ 2>/dev/null`;

    #print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) <= 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tfile_ 2>/dev/null`;
#    print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) > 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tfile_ 2>/dev/null`;

#    print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) <= 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tp1file_ 2>/dev/null`;
#    print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" && ( \$$st4_flds_{"PEXCH"} == \"33\" || \$$st4_flds_{"PEXCH"} == \"33\" ) && ( \$$st4_flds_{"PFC"} == \"HC\" ) && int ( \$$st4_flds_{"PTIME"} ) > 1630 ) { sum+= \$$st4_flds_{"PQTY"} } } END { print sum } ' $ne_tp1file_ 2>/dev/null`;


    for ( my $i_ = 0 ; $i_ < scalar ( @prod_cfe_info_ ) ; $i_ ++ )
    {
	my @tks_ = split ( '\|', $prod_cfe_info_[ $i_ ] ) ;

	( my $temp_ = $tks_[2].$tks_[3] ) =~ s/\s//g ;
	#print $temp_." ".$mfg_exch_future_code_to_shortcode_{$temp_}."\n";
	# if ( any exceptions )
	# else

	my $key_ = $mfg_exch_future_code_to_shortcode_{$temp_} ;
	$prods_{ $key_ } = 1 ;

	if ( exists ( $prod_trans_{ $key_ } ) )
	{
	    $prod_trans_{ $key_ } += 1 ;
	    $prod_vol_ { $key_ } += int ( $tks_[ 9 ] ) ;
	    $prod_fee1_ { $key_ } += ( $tks_[ 4 ] - $prod_fee1_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee2_ { $key_ } += ( $tks_[ 5 ] - $prod_fee2_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee3_ { $key_ } += ( $tks_[ 6 ] - $prod_fee3_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee4_ { $key_ } += ( $tks_[ 7 ] - $prod_fee4_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_mult_ { $key_ } = int ( $tks_[ 1 ] ) ;
	    $prod_exh_ { $key_ } = $tks_[ 2 ] ;
	    $prod_ncode_ { $key_ } = $tks_[ 3 ] ;
	}
	else
	{
	    $prod_date_ { $key_ } = $tdate_ ;
	    $prod_trans_{ $key_ } = 1 ;
	    $prod_vol_ { $key_ } = $tks_[ 9 ] ;
	    $prod_fee1_ { $key_ } = $tks_[ 4 ] ;
	    $prod_fee2_ { $key_ } = $tks_[ 5 ] ;
	    $prod_fee3_ { $key_ } = $tks_[ 6 ] ;
	    $prod_fee4_ { $key_ } = $tks_[ 7 ] ;
	}
    }
}

if ( $print_commissions_ == 1 )
{
    print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PRECID"} == \"T\" ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PRODCT"}}}' $ne_tfile_ | sort | uniq | awk -F"|" '{ print \$NF" "\$5+\$6+\$7+\$8+\$9 }' | sort -k3 `;
}

# ST4FILE
# NE_SYM NE_EX NE_N2D NE_COMM DV_SYM DV_EX DV_N2D DV_COMM
# SYM/EX/PRICE/BS/SIZE/
# These instruments need to be configured with NewEdge instrument codes.

## ALPES ##
#print "ALPES:: ";
my $adate_ = $date_ ;
my $alpes_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/InvoiceDetailed_".$adate_.".csv" ;
while ( ! ExistsWithSize ( $alpes_file_ ) )
{
    $adate_ = CalcPrevBusinessDay ( $adate_ ) ;
    $alpes_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/InvoiceDetailed_".$adate_.".csv" ;
}

#print "Using: ".$adate_."\n" ;

my %alpes_flds_ = ( ) ;
$alpes_flds_{"ASSET"} = 1; # DI1
#$alpes_flds_{"MARKET"} = 2; # FUT
$alpes_flds_{"MATURITY"} = 3; # F16
#$alpes_flds_{"ISIN_CODE"} = 4; # ISIN
#$alpes_flds_{"SIDE"} = 5; # S B
$alpes_flds_{"QUANTITY"} = 6; # 30
#$alpes_flds_{"SETTLEMENT_DATE"} = 7; # 20150406 == file_date
#$alpes_flds_{"CLIENT_NAME"} = 8; # DV Capital LLC
#$alpes_flds_{"CLIENTACCOUNT"} = 9; # 16360
#$alpes_flds_{"TRADE_ID"} = 10; # 8350471
#$alpes_flds_{"SETTLEMENT_PRICE"} = 11; # 13.3
#$alpes_flds_{"FX_RATE"} = 12; # 
#$alpes_flds_{"PRODUCT_CURRENCY"} = 13; # BRL
#$alpes_flds_{"DESIGNATION"} = 14; # DT
$alpes_flds_{"EXCHANGE_FEES"} = 15; # 3.3
$alpes_flds_{"EXCHANGE_REGISTER_FEES"} = 16; # 2.7
$alpes_flds_{"BROKERFEES"} = 17; # 0.3
$alpes_flds_{"PEL"} = 18; # -722.40 


my @prod_bmf_info_ = ( ) ;

if ( $print_bmf_prod_info_ == 1 )
{
    @prod_bmf_info_ = `awk -F ';' '{ if ( \$$alpes_flds_{"ASSET"} != "ASSET" ) { print \$$alpes_flds_{"ASSET"}" | "\$$alpes_flds_{"MATURITY"}" | "\$$alpes_flds_{"QUANTITY"}" | "\$$alpes_flds_{"EXCHANGE_FEES"}/\$$alpes_flds_{"QUANTITY"}" | "\$$alpes_flds_{"EXCHANGE_REGISTER_FEES"}/\$$alpes_flds_{"QUANTITY"}" | "\$$alpes_flds_{"BROKERFEES"}/\$$alpes_flds_{"QUANTITY"} } }' $alpes_file_ `;
    #DI1 | F17  | 30 | 5.4 | 4.5 | 0.3
    for ( my $i_ = 0 ; $i_ < scalar ( @prod_bmf_info_ ) ; $i_ ++ )
    {
	my @tks_ = split ( '\|', $prod_bmf_info_[ $i_ ] ) ;
	#print $temp_." ".$mfg_exch_future_code_to_shortcode_{$temp_}."\n";
	# if ( any exceptions )
	# else

	( my $key_ = $tks_[ 0 ].$tks_[ 1 ] ) =~ s/\s//g ;
	$prods_{ $key_ } = 1 ;

	if ( exists ( $prod_trans_{ $key_ } ) )
	{
	    $prod_trans_{ $key_ } += 1 ;
	    $prod_vol_ { $key_ } += int ( $tks_[ 2 ] ) ;
	    $prod_fee1_ { $key_ } += ( $tks_[ 3 ] - $prod_fee1_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee2_ { $key_ } += ( $tks_[ 4 ] - $prod_fee2_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee3_ { $key_ } += ( $tks_[ 5 ] - $prod_fee3_{ $key_ } ) / $prod_trans_{ $key_ } ;
	    $prod_fee4_ { $key_ } += 0 ;
	    $prod_mult_ { $key_ } = 1 ;
	    $prod_exh_ { $key_ } = "BP" ;
	    $prod_ncode_ { $key_ } = $tks_[ 0 ] ;
	}
	else
	{
	    $prod_date_ { $key_ } = $adate_ ;
	    $prod_trans_{ $key_ } = 1 ;
	    $prod_vol_ { $key_ } = int ( $tks_[ 2 ] ) ;
	    $prod_fee1_ { $key_ } = $tks_[ 3 ] ;
	    $prod_fee2_ { $key_ } = $tks_[ 4 ] ;
	    $prod_fee3_ { $key_ } = $tks_[ 5 ] ;
	    $prod_fee4_ { $key_ } = 0 ;
	}
    } 
}

## BCS ##
my %bcs_flds_ = ( ) ;
#$bcs_flds_{"TradeNum"} = 1 ; # 1084954859
#$bcs_flds_{"TradeDate"} = 2 ; # 07.04.2015
#$bcs_flds_{"TradeTime"} = 3 ; # 10:17:38
#$bcs_flds_{"TSSection_Name"} = 4 ; # FORTS
#$bcs_flds_{"BuySell"} = 5 ; # 2
#$bcs_flds_{"FunctionType"} = 6 ; # 0
#$bcs_flds_{"AgreeNum"} = 7 ; # 
#$bcs_flds_{"Subacc_SubaccCode"} = 8 ; # DV005F
#$bcs_flds_{"PutAccount_AccountCode"} = 9 ; # FORTS_82A9005
#$bcs_flds_{"PayAccount_AccountCode"} = 10 ; # BCSBROKER_FORTS_55
$bcs_flds_{"Security_SecCode"} = 11 ; # BRJ5
$bcs_flds_{"Asset_ShortName"} = 12 ; # BRJ5
$bcs_flds_{"CurrPayAsset_ShortName"} = 13 ; # RUR
#$bcs_flds_{"CPFirm_FirmShortName"} = 14 ; # National Clearing Centre
#$bcs_flds_{"Price"} = 15 ; # 57.33
$bcs_flds_{"Qty"} = 16 ; # 3.00
#$bcs_flds_{"IsAccrued"} = 17 ; # n
#$bcs_flds_{"AccruedInt"} = 18 ; # 0
$bcs_flds_{"Volume1"} = 19 ; # 94731.39
$bcs_flds_{"ClearingComission"} = 20 ; # 0
$bcs_flds_{"ExchangeComission"} = 21 ; # 3.00
$bcs_flds_{"TechCenterComission"} = 22 ; # 0
#$bcs_flds_{"PayPlannedDate"} = 23 ; # 07.04.2015
#$bcs_flds_{"PutPlannedDate"} = 24 ; # 07.04.2015
#$bcs_flds_{"IsRepo2"} = 25 ; # n
#$bcs_flds_{"RepoRate"} = 26 ; # 0
#$bcs_flds_{"BackDate"} = 27 ; # ""
#$bcs_flds_{"RepoDate2"} = 28 ; # ""
#$bcs_flds_{"BackPrice"} = 29 ; # 0
$bcs_flds_{"Volume2"} = 30 ; # 0
#$bcs_flds_{"Accruedint2"} = 31 ; # 0
#$bcs_flds_{"VarMargin"} = 32 ; # -1801.11
#$bcs_flds_{"Comment"} = 33 ; # $F1$
$bcs_flds_{"BrokerageCommission"} = 34 ; # 0
$bcs_flds_{"Taxes"} = 35 ; # 0
#$bcs_flds_{"GrossAmount"} = 36 ; # 94731.39
#$bcs_flds_{"NetAmount"} = 37 ; # 94728.39
$bcs_flds_{"TransactionType"} = 38 ; # Trade

#print "BCS_PRIME:: " ;

my $mdate_ = $date_ ;

my $acct1_ = "DV005F" ; # RTS 
my $acct2_ = "DV005" ; # MICEX


# NAS1/broker_files/BCS_MOS/.20150501./.DV005F._trades_.20150501._*.csv_srv.zip'

my $file1_ = `find /NAS1/broker_files/BCS_MOS/$mdate_/$acct1_"_trades_"$mdate_*.csv_srv.zip ` ;
$file1_  =~ s/\|?\n//g ;
my $file2_ = `find /NAS1/broker_files/BCS_MOS/$mdate_/$acct2_"_trades_"$mdate_*.csv_srv.zip ` ;
$file2_  =~ s/\|?\n//g ;

while ( ( !  ( -e $file1_ ) ) && ( ! ( -e $file2_) ) && $sanity_counter_ < 10 )
{
    $mdate_ = CalcPrevBusinessDay ( $mdate_ ) ;
    $file1_ = `find /NAS1/broker_files/BCS_MOS/$mdate_/$acct1_"_trades_"$mdate_*.csv_srv.zip` ;
    $file1_  =~ s/\|?\n//g ;
    $file2_ = `find /NAS1/broker_files/BCS_MOS/$mdate_/$acct2_"_trades_"$mdate_*.csv_srv.zip` ;
    $file2_  =~ s/\|?\n//g ;
    $sanity_counter_ ++ ;
}

#print "Using: ".$mdate_."\n" ;
if ( $sanity_counter_ > 10 )
{
    print $sanity_counter_." ".$mdate_." ".$date_."\n";
    exit ( 0 ) ;
}

my @prod_mos_info_ = ( ) ;

if ( $print_mos_prod_info_ == 1 )
{
    #print "BCS ".$file1_."\n" ;
    #print "BCS ".$file2_."\n" ;
    if ( ExistsWithSize ( $file1_ ) )
    {
	@prod_mos_info_ = `gzip -dc $file1_ \| awk -F';' '{ if ( \$$bcs_flds_{"TransactionType"} == \"Trade\" ) { print \$$bcs_flds_{"Security_SecCode"}" | "\$$bcs_flds_{"Asset_ShortName"}" | "\$$bcs_flds_{"CurrPayAsset_ShortName"}" | "\$$bcs_flds_{"Qty"}" | "\$$bcs_flds_{"Volume1"}" | "\$$bcs_flds_{"ClearingComission"}\/\$$bcs_flds_{"Qty"}" | "\$$bcs_flds_{"ExchangeComission"}/\$$bcs_flds_{"Qty"}" | "\$$bcs_flds_{"TechCenterComission"}/\$$bcs_flds_{"Qty"}" | "\$$bcs_flds_{"Volume2"}" | "\$$bcs_flds_{"BrokerageCommission"}/\$$bcs_flds_{"Qty"} } }' ` ;
	#print @prod_mos_info_ ;
#BRJ5 | BRJ5 | RUR | 2.00 | 62747.28 | 0 | 2.00 | 0 | 0 | 0

	for ( my $i_ = 0 ; $i_ < scalar ( @prod_mos_info_ ) ; $i_ ++ )
	{
	    my @tks_ = split ( '\|', $prod_mos_info_[ $i_ ] ) ;
	    
	    ( my $key_ = $tks_[ 0 ] ) =~ s/\s//g ;
	    $key_ = substr $key_ , 0 , -2 ;
	    $prods_{ $key_ } = 1 ;
	    
	    if ( exists ( $prod_trans_{ $key_ } ) )
	    {
		$prod_trans_{ $key_ } += 1 ;
		$prod_vol_ { $key_ } += int ( $tks_[ 3 ] ) ;
		#$prod_fee1_ { $key_ } += ( $tks_[ 5 ] - $prod_fee1_{ $key_ } ) / $prod_trans_{ $key_ } ;
		$prod_fee1_ { $key_ } += ( 0.5 - $prod_fee1_{ $key_ } ) / $prod_trans_{ $key_ } ;
		$prod_fee2_ { $key_ } += ( $tks_[ 6 ] - $prod_fee2_{ $key_ } ) / $prod_trans_{ $key_ } ;
		$prod_fee3_ { $key_ } += ( $tks_[ 7 ] - $prod_fee3_{ $key_ } ) / $prod_trans_{ $key_ } ;
		$prod_fee4_ { $key_ } += ( $tks_[ 9 ] - $prod_fee4_{ $key_ } ) / $prod_trans_{ $key_ } ;
		$prod_mult_ { $key_ } = 1 ;
		$prod_exh_ { $key_ } = "MS" ;
		$prod_ncode_ { $key_ } = $tks_[ 0 ] ;
	    }
	    else
	    {
		$prod_date_ { $key_ } = $mdate_ ;
		$prod_trans_{ $key_ } = 1 ;
		$prod_vol_ { $key_ } = int ( $tks_[ 3 ] ) ;
		#$prod_fee1_ { $key_ } = $tks_[ 5 ] ;
		$prod_fee1_ { $key_ } = 0.5 ; # hack till you know, where are clearing commissions
		$prod_fee2_ { $key_ } = $tks_[ 6 ] ;
		$prod_fee3_ { $key_ } = $tks_[ 7 ] ;
		$prod_fee4_ { $key_ } = $tks_[ 9 ] ;
	    }
	}
    }
    if ( ExistsWithSize ( $file2_ ) )
    {
	@prod_mos_info_ = `gzip -dc $file2_ \| awk -F';' '{ if ( \$$bcs_flds_{"TransactionType"} == \"Trade\" ) { print \$$bcs_flds_{"Security_SecCode"}" | "\$$bcs_flds_{"Asset_ShortName"}" | "\$$bcs_flds_{"CurrPayAsset_ShortName"}" | "\$$bcs_flds_{"Qty"}" | "\$$bcs_flds_{"Volume1"}" | "\$$bcs_flds_{"ClearingComission"}\/\$$bcs_flds_{"Qty"}" | "\$$bcs_flds_{"ExchangeComission"}\/\$$bcs_flds_{"Qty"}" | "\$$bcs_flds_{"TechCenterComission"}\/\$$bcs_flds_{"Qty"}" | "\$$bcs_flds_{"Volume2"}" | "\$$bcs_flds_{"BrokerageCommission"}\/\$$bcs_flds_{"Qty"} } }' ` ;
	#print STDERR @prod_mos_info_ ;
#BRJ5 | BRJ5 | RUR | 2.00 | 62747.28 | 0 | 2.00 | 0 | 0 | 0
	
	for ( my $i_ = 0 ; $i_ < scalar ( @prod_mos_info_ ) ; $i_ ++ )
	{
	    my @tks_ = split ( '\|', $prod_mos_info_[ $i_ ] ) ;
	    
	    ( my $key_ = $tks_[ 0 ] ) =~ s/\s//g ;
	    $key_ = substr $key_ , 0 , -2 ;
	    $prods_{ $key_ } = 1 ;
	    
	    if ( exists ( $prod_trans_{ $key_ } ) )
	    {
		$prod_trans_{ $key_ } += 1 ;
		$prod_vol_ { $key_ } += int ( $tks_[ 3 ] ) / 1000 ;
		#$prod_fee1_ { $key_ } += ( $tks_[ 5 ] - $prod_fee1_{ $key_ } ) / $prod_trans_{ $key_ } ;
		$prod_fee1_ { $key_ } += ( 0.5 - $prod_fee1_{ $key_ } ) / $prod_trans_{ $key_ } ;
		$prod_fee2_ { $key_ } += ( $tks_[ 6 ] - $prod_fee2_{ $key_ } ) / $prod_trans_{ $key_ } ;
		$prod_fee3_ { $key_ } += ( $tks_[ 7 ] - $prod_fee3_{ $key_ } ) / $prod_trans_{ $key_ } ;
		$prod_fee4_ { $key_ } += ( $tks_[ 9 ] - $prod_fee4_{ $key_ } ) / $prod_trans_{ $key_ } ;
		$prod_mult_ { $key_ } = 1 ;
		$prod_exh_ { $key_ } = "MS" ;
		$prod_ncode_ { $key_ } = $tks_[ 0 ] ;
	    }
	    else
	    {
		$prod_date_ { $key_ } = $mdate_ ;
		$prod_trans_{ $key_ } = 1 ;
		$prod_vol_ { $key_ } = int ( $tks_[ 3 ] ) / 1000 ;
		$prod_fee1_ { $key_ } = 0.5 ;
		$prod_fee2_ { $key_ } = $tks_[ 6 ] ;
		$prod_fee3_ { $key_ } = $tks_[ 7 ] ;
		$prod_fee4_ { $key_ } = $tks_[ 9 ] ;
	    }
	}
    }
}

# BCBCS 
printf "%-16s%-12s%-12s%-12s%-12s%-12s\n",
    "TDATE",
    "SHC",
    "BR_VOL",
    "DV_VOL",
    "BR_COM",
    "DV_COM";

my @keys_ = sort { $a cmp $b } keys %prod_exh_ ;
#foreach my $key ( sort keys %prods_ )
foreach my $key ( @keys_ )
{
## VOL
    my ( $year, $month, $day ) = BreakDateYYYYMMDD ( $date_ ) ;
    my $rgkey1 = $key ;
    my $rgkey2 = "DUMMY" ;
    my $col = 14 ;
    my $dc_ = 0 ; # for double in case of spreads 

    if ( $key eq "LFI" )
    {
	$rgkey1 = "I   FM" ;
	$col = 15 ;
    }
    elsif ( $key eq "LFL" )
    {
	$rgkey1 = "L   FM" ;
	$col = 15 ;
    }
    elsif ( $key eq "LFR" )
    {
	$rgkey1 = "R   FM" ;
	$col = 15 ;
    }
    elsif ( $key eq "LFZ" )
    {
	$rgkey1 = "Z   FM" ;
	$col = 15 ;
    }
    elsif ( $key eq "NK" )
    {
	$rgkey1 = "NK1" ;
    }
    elsif ( $key eq "UB" )
    {
	$rgkey2 = "RUB" ;
    }
    elsif ( $key eq "VX" )
    {
	$rgkey1 = "VX2" ;
	$dc_ = 1 ;
	$rgkey2 = "VX[A-Z][0-9]_VX" ;
    }
    elsif ( $key eq "BAX" )
    {
	$rgkey1 = "BAX" ;
	$rgkey2 = "BAX[A-Z][0-9][0-9]" ;
	$dc_ = 1 ;
    }

    my $tr_vol1_ = 0 ;
    my $tr_vol2_ = 0 ;

    $tr_vol1_ = `grep \"$rgkey1\"  /NAS1/data/MFGlobalTrades/EODPnl/ors_pnls_$prod_date_{$key}.txt | grep -v \"$rgkey2\" | awk '{ vol+=\$$col } END { print vol } '` ;
    chomp ( $tr_vol1_ ) ;

    if ( $tr_vol1_ eq "" )
    {
	$tr_vol1_ = 0 ;
    }
    if ( $dc_ == 1 )
    {
	$tr_vol2_ = `grep \"$rgkey2\"  /NAS1/data/MFGlobalTrades/EODPnl/ors_pnls_$prod_date_{$key}.txt | awk '{ vol+=\$$col } END { print vol } '` ;
	chomp ( $tr_vol2_ );
    }
    if ( $tr_vol2_ eq "" )
    {
	$tr_vol2_ = 0 ;
    }

# COMM
    my $comm_ = GetCommissionForShortcode ( $key , $prod_vol_{$key} );
    if ( substr ( $key , 0, 3 ) eq "DI1" )
    {
	$comm_ = GetCommissionForDI ( $key ) ;
    }
    elsif ( substr ( $key , 0, 3 ) eq "DOL" )
    {
	my ( $bro_ ) = grep { $_ =~ /WDO/ } keys %prod_exh_ ;
	$comm_ = GetCommissionForShortcode (  substr ( $key , 0, 3 ) , $prod_vol_{$key}, ( defined $bro_ ? $prod_vol_{$bro_} : 0 ) ) ;
    }
    elsif ( substr ( $key , 0, 3 ) eq "WIN" )
    {
	my ( $bro_ ) = grep { $_ =~ /IND/ } keys %prod_exh_ ;
	$comm_ = GetCommissionForShortcode (  substr ( $key , 0, 3 ) , ( defined $bro_ ? $prod_vol_{$bro_} : 0 ), $prod_vol_{$key} ) ;
    }    
    elsif ( substr ( $key , 0, 3 ) eq "IND" )
    {
	my ( $bro_ ) = grep { $_ =~ /WIN/ } keys %prod_exh_ ;
	$comm_ = GetCommissionForShortcode (  substr ( $key , 0, 3 ) , $prod_vol_{$key}, ( defined $bro_ ? $prod_vol_{$bro_} : 0 ) ) ;
    }    
    elsif ( substr ( $key , 0, 3 ) eq "WDO" )
    {
	my ( $bro_ ) = grep { $_ =~ /DOL/ } keys %prod_exh_ ;
	$comm_ = GetCommissionForShortcode (  substr ( $key , 0, 3 ) , ( defined $bro_ ? $prod_vol_{$bro_} : 0 ), $prod_vol_{$key} ) ;
    }
    
    printf "%-16s%-12s%-12s%-12s%12.3f%12.3f\n",
    $prod_date_{$key},        
    $key,
    #$prod_trans_{$key}, 
    $prod_vol_{$key},
    #$prod_mult_{$key},
    ($tr_vol1_+2*$tr_vol2_),
    #$prod_exh_{$key}, $prod_ncode_{$key},    
    abs ( ($prod_fee1_{$key}+$prod_fee2_{$key}+$prod_fee3_{$key}+$prod_fee4_{$key})),
    $comm_;
}

if ( $collect_unaccounted_ )
{
    for ( my $i = 0 ; $i < scalar ( @unaccounted_ ) ; $i++ )
    {
	print STDERR $unaccounted_[$i] ;
    }
}
