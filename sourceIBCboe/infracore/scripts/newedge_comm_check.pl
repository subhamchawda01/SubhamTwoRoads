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

my $USAGE="$0 file";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }

my $mfg_filename_ = $ARGV[0];

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

# this file should be print the following
my $check_ = 1 ;
my $print_prod_info_ = 1 ;
my $print_commissions_ = 1 ;
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

#TRN
my %trn_flds_ = ( ) ;

$trn_flds_{"FRECID"} = 1;
$trn_flds_{"FACCT"} = 4;
$trn_flds_{"FCTYM"} = 7;
$trn_flds_{"FEXPDT"} = 11;
$trn_flds_{"FTDATE"} = 12;
$trn_flds_{"FBS"} = 13;
$trn_flds_{"FQTY"} = 18;
$trn_flds_{"FSDSC1"} = 19;
$trn_flds_{"FMULTF"} = 21;
$trn_flds_{"FSDATE"} = 22;
$trn_flds_{"FLTDAT"} = 23;
$trn_flds_{"FEXCH"} = 24;
$trn_flds_{"FFC"} = 25;
$trn_flds_{"FSYMBL"} = 26;
$trn_flds_{"FPTYPE"} = 27;
$trn_flds_{"FTPRIC"} = 28;
$trn_flds_{"FCLOSE"} = 29;
$trn_flds_{"FCOMM"} = 38;
$trn_flds_{"FFEE1"} = 39;
$trn_flds_{"FFEE2"} = 40;
$trn_flds_{"FFEE3"} = 41;
$trn_flds_{"FFEE4"} = 42;
$trn_flds_{"PRODCT"} = 49;
$trn_flds_{"FTIME"} = 51;


# TO CHECK PROD_DESC  # SDSC1 MULTF EXCH SYMBL COMM FEE1 FEE2 FEE3 FEE4 PRODCT
# TO COMPUTE PNLS # RECID TDATE BS QTY FC EXCH CTYM TPRICE CLOSE COMM FEE1 FEE2 FEE3 FEE4

my $filebase_name_ = `basename $mfg_filename_` ; chomp ( $filebase_name_ ) ;
my @filename_tkns_ = split ( "_" , $filebase_name_ ) ;

if ( $filename_tkns_[0] eq "GMITRN" )
{
    print "Processing TRN file\n";
    if ( $check_ == 1 )
    {
	open ( my $NEFILE , "$mfg_filename_" ) ;
	while ( my $line_ = <$NEFILE> )
	{
	    chomp ( $line_ ) ;
	    $line_ =~ s/"//g ; #gsub ( "\"" , "" , $line_) ;
	    my @tkns_ = split ( "," , $line_ ) ;
	    if ( $tkns_[$trn_flds_{"FRECID"} -1 ] eq "FRECID" &&
		$tkns_[$trn_flds_{"FTDATE"} -1 ] eq "FTDATE" &&
		$tkns_[$trn_flds_{"FBS"} -1 ] eq "FBS" &&
		$tkns_[$trn_flds_{"FQTY"} -1 ] eq "FQTY" &&
		$tkns_[$trn_flds_{"FFC"} -1 ] eq "FFC" &&
		$tkns_[$trn_flds_{"FEXCH"} -1 ] eq "FEXCH" &&
		$tkns_[$trn_flds_{"FCTYM"} -1 ] eq "FCTYM" &&
		$tkns_[$trn_flds_{"FTPRIC"} -1 ] eq "FTPRIC" &&
		$tkns_[$trn_flds_{"FCLOSE"} -1 ] eq "FCLOSE" &&
		$tkns_[$trn_flds_{"FCOMM"} -1 ] eq "FCOMM" &&
		$tkns_[$trn_flds_{"FFEE1"} -1 ] eq "FFEE1" &&
		$tkns_[$trn_flds_{"FFEE2"} -1 ] eq "FFEE2" &&
		$tkns_[$trn_flds_{"FFEE3"} -1 ] eq "FFEE3" &&
		$tkns_[$trn_flds_{"FFEE4"} -1 ] eq "FFEE4" &&
		$tkns_[$trn_flds_{"FSDSC1"} -1 ] eq "FSDSC1" &&
		$tkns_[$trn_flds_{"FMULTF"} -1 ] eq "FMULTF" &&
		$tkns_[$trn_flds_{"FEXCH"} -1 ] eq "FEXCH" &&
		$tkns_[$trn_flds_{"FSYMBL"} -1 ] eq "FSYMBL" &&
		$tkns_[$trn_flds_{"FCOMM"} -1 ] eq "FCOMM" &&
		$tkns_[$trn_flds_{"FFEE1"} -1 ] eq "FFEE1" &&
		$tkns_[$trn_flds_{"FFEE2"} -1 ] eq "FFEE2" &&
		$tkns_[$trn_flds_{"FFEE3"} -1 ] eq "FFEE3" &&
		$tkns_[$trn_flds_{"FFEE4"} -1 ] eq "FFEE4" &&
		$tkns_[$trn_flds_{"PRODCT"} -1 ] eq "PRODCT" )
	    {
		print "File has expected fields in order\n" ;
	    }
	    else
	    {
		print "File doent have expected fields in order\n" ;
		print $tkns_[$trn_flds_{"FRECID"} -1 ]," FRECID\n",
		$tkns_[$trn_flds_{"FTDATE"} -1 ]," FTDATE\n",
		$tkns_[$trn_flds_{"FBS"} -1 ]," FBS\n",
		$tkns_[$trn_flds_{"FQTY"} -1 ]," FQTY\n",
		$tkns_[$trn_flds_{"FFC"} -1 ]," FFC\n",
		$tkns_[$trn_flds_{"FEXCH"} -1 ]," FEXCH\n",
		$tkns_[$trn_flds_{"FCTYM"} -1 ]," FCTYM\n",
		$tkns_[$trn_flds_{"FTPRIC"} -1 ]," FTPRIC\n",
		$tkns_[$trn_flds_{"FCLOSE"} -1 ]," FCLOSE\n",
		$tkns_[$trn_flds_{"FCOMM"} -1 ]," FCOMM\n",
		$tkns_[$trn_flds_{"FFEE1"} -1 ]," FFEE1\n",
		$tkns_[$trn_flds_{"FFEE2"} -1 ]," FFEE2\n",
		$tkns_[$trn_flds_{"FFEE3"} -1 ]," FFEE3\n",
		$tkns_[$trn_flds_{"FFEE4"} -1 ]," FFEE4\n",
		$tkns_[$trn_flds_{"FSDSC1"} -1 ]," FSDSC1\n",
		$tkns_[$trn_flds_{"FMULTF"} -1 ]," FMULTF\n",
		$tkns_[$trn_flds_{"FEXCH"} -1 ]," FEXCH\n",
		$tkns_[$trn_flds_{"FSYMBL"} -1 ]," FSYMBL\n",
		$tkns_[$trn_flds_{"FCOMM"} -1 ]," FCOMM\n",
		$tkns_[$trn_flds_{"FFEE1"} -1 ]," FFEE1\n",
		$tkns_[$trn_flds_{"FFEE2"} -1 ]," FFEE2\n",
		$tkns_[$trn_flds_{"FFEE3"} -1 ]," FFEE3\n",
		$tkns_[$trn_flds_{"FFEE4"} -1 ]," FFEE4\n",
		$tkns_[$trn_flds_{"PRODCT"} -1 ]," PRODCT\n";
	    }
	    close ( $NEFILE ) ;
	    last ;
	}
    }
    if ( $print_prod_info_ == 1 )
    {
	print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; print \$$trn_flds_{"FSDSC1"}" | "\$$trn_flds_{"FMULTF"}" | "\$$trn_flds_{"FEXCH"}" | "\$$trn_flds_{"FFC"}" | "\$$trn_flds_{"FCOMM"}" | "\$$trn_flds_{"FFEE1"}" | "\$$trn_flds_{"FFEE2"}" | "\$$trn_flds_{"FFEE3"}" | "\$$trn_flds_{"FFEE4"}" | "\$$trn_flds_{"PRODCT"}}' $mfg_filename_ | sort | uniq `;
    }
}
elsif ( $filename_tkns_[0] eq "GMIST4" )
{
    print "Processing ST4 file\n";
    if ( $check_ == 1 )
    {
	open ( my $NEFILE , "$mfg_filename_" ) ;
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
		print "File has expected fields in order\n" ;
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
    if ( $print_prod_info_ == 1 )
    {
	#print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}" | "\$$st4_flds_{"PFEE1"}" | "\$$st4_flds_{"PFEE2"}" | "\$$st4_flds_{"PFEE3"}" | "\$$st4_flds_{"PFEE4"}" | "\$$st4_flds_{"PRODCT"}" | "\$$st4_flds_{"PQTY"}}' $mfg_filename_ | uniq `;
	print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PSDSC1"} == \"PSDSC1\" &&  \$$st4_flds_{"PSDSC1"} != 0 ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}" | "\$$st4_flds_{"PFEE1"}" | "\$$st4_flds_{"PFEE2"}" | "\$$st4_flds_{"PFEE3"}" | "\$$st4_flds_{"PFEE4"}" | "\$$st4_flds_{"PRODCT"}}}' $mfg_filename_ | sort | uniq `;
	print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PSDSC1"} != \"PSDSC1\" && \$$st4_flds_{"PQTY"} != 0 ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PRODCT"}}}' $mfg_filename_ | sort | uniq `;
    }
    if ( $print_commissions_ == 1 )
    {
	print `awk -F, '{ gsub ( \"\\\"\", \"\" ) ; if ( \$$st4_flds_{"PSDSC1"} != \"PSDSC1\" && \$$st4_flds_{"PQTY"} != 0 ) { print \$$st4_flds_{"PSDSC1"}" | "\$$st4_flds_{"PMULTF"}" | "\$$st4_flds_{"PEXCH"}" | "\$$st4_flds_{"PFC"}" | "\$$st4_flds_{"PCOMM"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE1"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE2"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE3"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PFEE4"}/\$$st4_flds_{"PQTY"}" | "\$$st4_flds_{"PRODCT"}}}' $mfg_filename_ | sort | uniq | awk -F"|" '{ print \$NF" "\$5+\$6+\$7+\$8+\$9 }' | sort -k3 `;
    }
    exit ( 0 ) ;
}
else
{
    print "Unknown filetype\n" ;
    exit ( 0 ) ;
}


# ST4FILE
# NE_SYM NE_EX NE_N2D NE_COMM DV_SYM DV_EX DV_N2D DV_COMM
# SYM/EX/PRICE/BS/SIZE/
# These instruments need to be configured with NewEdge instrument codes.

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

#OSE
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


$mfg_exch_future_code_to_shortcode_{"24RV_Y"} = "Y_JGBL";
$mfg_exch_future_code_to_n2d_{"24RV_Y"} = 10000;
$mfg_exch_future_code_to_exchange_{"24RV_Y"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"24RV_T"} = "T_TOPIX";
$mfg_exch_future_code_to_n2d_{"24RV_T"} = 10000;
$mfg_exch_future_code_to_exchange_{"24RV_T"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"24TP_Y"} = "Y_TOPIX";
$mfg_exch_future_code_to_n2d_{"24TP_Y"} = 10000;
$mfg_exch_future_code_to_exchange_{"24TP_Y"} = "OSE";

$mfg_exch_future_code_to_shortcode_{"24TP_T"} = "T_TOPIX";
$mfg_exch_future_code_to_n2d_{"24TP_T"} = 10000;
$mfg_exch_future_code_to_exchange_{"24TP_T"} = "OSE";

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

#CME
$mfg_exch_future_code_to_shortcode_{"0125"} = "ZF";
$mfg_exch_future_code_to_n2d_{"0125"} = 1000;
$mfg_exch_future_code_to_exchange_{"0125"} = "CME";

$mfg_exch_future_code_to_shortcode_{"0126"} = "ZT";
$mfg_exch_future_code_to_n2d_{"0126"} = 2000;
$mfg_exch_future_code_to_exchange_{"0126"} = "CME";

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

#CFE
$mfg_exch_future_code_to_shortcode_{"7BVX"} = "VIX";
$mfg_exch_future_code_to_n2d_{"7BVX"} = 1000;
$mfg_exch_future_code_to_exchange_{"7BVX"} = "CFE";

$mfg_exch_future_code_to_shortcode_{"S9HKD"} = "HKDUSD";
$mfg_exch_future_code_to_n2d_{"S9HKD"} = 1;
$mfg_exch_future_code_to_exchange_{"S9HKD"} = "DVCAP";

$mfg_exch_future_code_to_shortcode_{"S9JPY"} = "JPYUSD";
$mfg_exch_future_code_to_n2d_{"S9JPY"} = 1;
$mfg_exch_future_code_to_exchange_{"S9JPY"} = "DVCAP";


# These offsets were obtained using the 'FIMST4F1.xls & FIMST4F1.csv' template files
# These will be used to determine the field being processed after splitting each trade line
#awk -F, '{print $1$12$13$18$25$24$7$28$29$39$40$41$42 }'
my $record_id_offset_ = $trn_flds_{"FRECID"} -1 ; # FRECID # Needed to figure out which records to use towards pnl computation.
my $date_offset_ = $trn_flds_{"FTDATE"} - 1 ; # FTDATE
my $buysell_offset_ = $trn_flds_{"FBS"} - 1 ; # FBS # 1 = buy ; 2 = sell
my $quantity_offset_ = $trn_flds_{"FQTY"} - 1; # FQTY
my $mfg_future_code_offset_ = $trn_flds_{"FFC"} - 1 ; # FFC
my $exch_code_offset_ = $trn_flds_{"FEXCH"} - 1 ; # FEXCH
my $contract_yr_month_offset_ = $trn_flds_{"FCTYM"} - 1; # FCTYM  # YYYYMM
my $trade_price_offset_ = $trn_flds_{"FTPRIC"} - 1 ; # FTPRIC # Trade price may be different than price of order
my $closing_price_offset_ = $trn_flds_{"FCLOSE"} - 1  ; # FCLOSE
my $comm_amount_offset_ = $trn_flds_{"FCOMM"} -1 ;
my $fee1_amount_offset_ = $trn_flds_{"FFEE1"} -1 ;
my $fee2_amount_offset_ = $trn_flds_{"FFEE2"} -1 ;
my $fee3_amount_offset_ = $trn_flds_{"FFEE3"} -1 ;

# EURTODOL
my $CD_TO_DOL_ = 0.97;
my $EUR_TO_DOL_ = 1.35;
my $GBP_TO_DOL_ = 1.57;
my $HKD_TO_DOL_ = 0.1289 ;
my $JPY_TO_DOL_ = 0.0104 ;
my $RUB_TO_DOL_ = 0.02862 ;

my $expected_date_ = `date "+%Y%m%d"` ;
chomp ( $expected_date_ ) ;
my $prevdate_ = CalcPrevBusinessDay ( $expected_date_ ) ;
chomp ( $prevdate_ ) ;
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

        if( index ( $currency_, "USDRUB" ) == 0 ){
            $RUB_TO_DOL_ = sprintf( "%.4f", 1 / $cwords_[1] );
        }

      }
  }



open MFG_TRADES_FILE_HANDLE, "< $mfg_filename_" or die "could not open mfg_trans_filename_ $mfg_filename_\n";
my @mfg_trades_file_lines_ = <MFG_TRADES_FILE_HANDLE>;
close MFG_TRADES_FILE_HANDLE;

for ( my $i = 0; $i <= $#mfg_trades_file_lines_; $i++ )
{
    my @fields_ = split (',', $mfg_trades_file_lines_[$i]);
    if ( $#fields_ < 55 )  # atleast ?
    {
	print "Malformatted line : $mfg_trades_file_lines_[$i] No. of fields : $#fields_\n";
	next;
    }

    my $record_id_ = substr ($fields_[$record_id_offset_], 1, -1); $record_id_ =~ s/^\s+|\s+$//g;

    if ( $record_id_ ne "T" )
    {
	print "Ignoring non-T records\n" ;
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
    if ( ! exists $shortcode_to_pnl_{$shortcode_} )
    {
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
