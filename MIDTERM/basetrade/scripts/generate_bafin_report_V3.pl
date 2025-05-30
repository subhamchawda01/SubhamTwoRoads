#!/usr/bin/perl
use strict;
use File::Basename;
use List::Util qw[min max];
use warnings;
use POSIX qw(strtod setlocale LC_NUMERIC);
#use File::Basename; # for basename and dirname
#use File::Copy; # for copy
#use List::Util qw/max min/; # for max
#use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $eurex_dir = "/NAS1/logs/ExchangeFiles/EUREX/";


require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/calc_next_business_day.pl"; # ExistsWithSize

sub MakeReportUsingNewedgeFile ( ) ;
sub MakeReportUsingExchangeFile ( ) ;
sub SendReportMail ( ) ;

my $USAGE = "$0 from_date to_date <output_dir>";


if ( $#ARGV < 1 )
{
    print ( $USAGE."\n" );
    exit ( 0 );
}

#hacky helpers
#my %product_list_ = ( "FESX", "FGBS", "FGBM", "FGBL", "FGBX", "FDAX", "FBTP", "FBTS", "FOAT", "FVS", "FXXP", "FEU3" );

my @pad_len_ = ( 1, 34, 1, 34, 1, 34, 1, 34, 1, 34,
		 1, 34, 8, 6, 40, 16, 1, 2, 1, 1, 
		 1, 2, 11, 3, 16, 3, 18, 6, 6, 6,
		 12, 2, 12, 6, 70, 1, 10, 8, 1, 2,
		 12, 18, 2, 3, 18, 1, 8, 8, 1, 1,
		 8, 5, 8, 6, 12, 8, 6, 16, 1, 1,
		 8 ) ;

my @type_ = ( 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 
	      'A', 'A', 'N', 'N', 'A', 'A', 'A', 'N', 'A', 'A', 
	      'A', 'A', 'A', 'A', 'A', 'A', 'N', 'N', 'N', 'A',
	      'A', 'A', 'A', 'N', 'A', 'N', 'N', 'N', 'A', 'A', 
	      'A', 'N', 'N', 'A', 'N', 'N', 'N', 'N', 'A', 'A', 
	      'N', 'A', 'N', 'N', 'A', 'N', 'N', 'A', 'A', 'A',
	      'N' ) ;

my %mon2num = qw(
              Jan 01  Feb 02  Mar 03  Apr 04  May 05  Jun 06
              Jul 07  Aug 08  Sep 09  Oct 10 Nov 11 Dec 12
    );

my %ric2newedge_ = qw( 
     FESX SF
     FDAX FE
     FDXM PN
     FGBS SD
     FGBM BC
     FGBL BM
     FGBX BV
     FOAT I*
     FBTP F7
     FBTS FA
     FVS VS
     FXXP >O
     FSTB TB
     FEU3 E4
    );

my %newedge2ric_ = qw( 
     SF FESX
     FE FDAX
     PN FDXM
     SD FGBS
     BC FGBM
     BM FGBL
     BV FGBX
     I* FOAT
     F7 FBTP
     FA FBTS
     VS FVS
     >O FXXP
     TB FSTB
     E4 FEU3
    );



# The twelve-digit ISIN consists of a two-digit country code, a nine-digit national identification code as well as a check digit. The sixth to eleventh digits represent the WKN.

my %product_isin = qw(
      FSTB DE0005705610
      FESX DE0009652388
      FGBS DE0009652669
      FGBM DE0009652651
      FGBL DE0009652644
      FGBX DE0009652636
      FDAX DE0008469594
      FDXM DE000A160WT6
      FBTP DE000A0ZW3V8
      FBTS DE000A1EZJ09
      FOAT DE000A1MAPW3
      FVS DE000A0Z3CW9
      FXXP DE000A1DKQK4
      FEU3 DE0009653147
    ); 

my %underlying_isin = qw(
      FSTB EU0009658806
      FESX EU0009658145
      FDAX DE0008469008
      FDXM DE0008469008
      FVS DE000A0C3QF1
      FXXP EU0009658202
    );

#http://www.onixs.biz/fix-dictionary/4.4/app_6_d.html
# FFDPSX # Futures FinancialFutures InterestRate/Debt Physical StandardizedTerms N/A
# FFICSX # Futures FinancialFutures Indices Cash StandardizedTerms N/A
# for futures F F 1# 2# 3# X   * == 
#1st attribute: Underlying assets (Indicates the type of underlying assets that the futures buyer receives, respectively
#				  that the seller delivers; see also Entitlements (Rights), Warrants)
#B = Basket -F-F-B-*-*-*-
#S = Stock-Equities -F-F-S-*-*-*-
#D = Debt Instruments/Interest -F-F-D-*-*-*-
#C = Currencies -F-F-C-*-*-*-
#I = Indices -F-F-I-*-*-*-*-
#O = Options (The option gives the right to buy, respectively to sell options.) -F-F-O-*-*-*-
#F = Futures (The option gives the right to buy, respectively to sell futures.) -F-F-F-*-*-*-
#W = Swaps (The option gives the right to buy, respectively to sell swaps.) -F-F-W-*-*-*-
#N = Interest rates -F-F-N-*-*-*-
#M = Others (Miscellaneous) -F-F-M-*-*-*-

#2nd attribute: Delivery(see also Options, Call options)
#P = Physical -F-F-*-P-*-*-
#C = Cash -F-F-*-C-*-*-
#N = Non-delivering -F-F-*-N-*-*-

#3rd attribute: Standardized/non-standardized (see also Options, Call options)
#S = Standardized -F-F-*-*-S-*-
#N = Non-standardized -F-F-*-*-N-*-
#F = Flexible -F-F-*-*-F-*-

my %iso_10962 = qw(
       FSTB FFICSX
       FESX FFICSX
       FDAX FFICSX
       FDXM FFICSX
       FVS FFICSX
       FGBS FFDPSX
       FGBM FFDPSX
       FGBL FFDPSX
       FGBX FFDPSX
       FBTP FFDPSX
       FBTS FFDPSX
       FOAT FFDPSX
       FXXP FFICSX
       FEU3 FFDCSX
    );

my %sec_identification = (
    "FESX", "FUT ON EURO STOXX 50",
    "FDAX", "FUT ON DAX",
    "FDXM", "MINI FUT ON DAX",
    "FVS", "FUT ON VSTOXX",
    "FGBS", "FUT 1 3/4-2 1/4Y.GOV.BONDS 6%",
    "FGBM", "FUT 4 1/2-5 1/2Y.GOV.BONDS 6%",
    "FGBL", "FUT 8 1/2-10 1/2Y.GOV.BONDS 6%",
    "FGBX", "FUT 24Y-35Y.GOV.BONDS 4%",
    "FBTP", "FUT 8 1/2-11Y.GOV.BONDS 6%",
    "FBTS", "FUT 2-3 1/4Y.GOV.BONDS 6%",
    "FOAT", "FUT 8 1/2-10 1/2.GOV.BONDS 6%",
    "FXXP", "FUT ON EURX STOX 600",
    "FSTB", "FUT ON EURX BANK STOX 600",
    "FEU3", "Three-Month EURIBOR Futures"
    );


# 1 = unit 2 = percent 3 = pro mill 4 = points 9 = other
my %unit_of_security = qw(
      FSTB 4
      FESX 4
      FDAX 4
      FDXM 4
      FVS 4
      FGBS 2
      FGBM 2
      FGBL 2
      FGBX 2
      FBTP 2
      FBTS 2
      FOAT 2
      FXXP 4
      FEU3 2
    );

# security type F/O/P
my %sec_type = qw(
      FESX F
      FDAX F
      FDXM F
      FVS F
      FGBS F
      FGBM F
      FGBL F
      FGBX F
      FOAT F
      FBTP F
      FBTS F
      FXXP F
      FSTB F
      FEU3 F
    );

# price multiplier ## value of 1.0 point
my %price_multiplier = qw(
      FESX 10
      FDAX 25
      FDXM 5
      FVS 100  
      FGBS 1000
      FGBM 1000
      FGBL 1000
      FGBX 1000
      FOAT 1000
      FBTP 1000
      FBTS 1000
      FXXP 50
      FSTB 50
      FEU3 2500
    );

## XXP currency for points !!!
## XXX no currency !!!
my %price_curr_iso4217 = qw(
      FESX XXP
      FDAX XXP
      FDXM XXP
      FVS XXP
      FGBS XXX
      FGBM XXX
      FGBL XXX
      FGBX XXX
      FOAT XXX
      FBTP XXX
      FBTS XXX
      FXXP XXP
      FSTB XXP
      FEU3 XXX
    );

my $to_address_ = "NA" ;
my $from_address_ = "NA" ;

my $msg = "";

my $USER = `echo \$USER`;
chomp ( $USER );

if ( $USER eq "dvctrader" || $USER eq "dvcinfra" )
{
    $to_address_ = "nseall@tworoads.co.in";
    $from_address_ = "BafinOnNY15\@dvctrader.com";
}
my $sendmail_ = 1;
my $debug_ = 0;
my $log_dir_ = "/spare/local/dvctrader/BLogs";

my $fdate_ = $ARGV [ 0 ] ;
my $tdate_ = $ARGV [ 1 ] ;

my $dstr = `env TZ=Europe/Berlin date +%Y%m%d`;
my $tstr = `env TZ=Europe/Berlin date +%H%M%S`;

chomp ( $dstr );
chomp ( $tstr );

my $nstr = "DV$tstr" ;
if ( $fdate_ eq $tdate_ )
{
    $nstr = "DV".substr $fdate_,2, 6 ;
}

my $header_ = "echo -n \"$nstr;$dstr;$tstr\r\n\"" ;
my $trecords_ = 2 ;

my $output_dir = "/spare/local/$USER/BafinReports";
my $serial_no_ = 1 ;

if ( $#ARGV >= 2 )
{
    $output_dir = $ARGV [ 2 ];
}

if ( $#ARGV == 3 )
{
    $serial_no_ = $ARGV [ 3 ];
}

if ( ! -d $output_dir )
{
    `mkdir -p $output_dir` ;
}
if ( ! -d $output_dir )
{
    $msg = $msg."cannot create output_dir $output_dir \n" ;
    SendReportMail ( ) ;
    exit ( 0 ) ;
}

my $counter_ = 0 ;
my $yyyymmdd_ =  $fdate_  ;

my $ric_code_ = "NA" ;
my $newedge_code_ = "NA" ;

my $eurex_file_ = "NA" ;
my $eurex_mat_file_ = "NA" ;
my $newedge_file_ = "NA" ;
my $ors_file_ = "NA" ;
my $output_file_ = "NA" ;

my $eurex_count_ = -1 ;
my $newedge_count_ = -1 ;
my $ors_count_ = -1 ;

my $bafin_report_ = $nstr;
`$header_ > $output_dir/$bafin_report_`;

for ( $yyyymmdd_ = $fdate_ ; $yyyymmdd_ <= $tdate_  ;  $yyyymmdd_ = CalcNextBusinessDay ( $yyyymmdd_ ) )
{

    print STDERR $yyyymmdd_,"\n" ;

    # reset all # just in case #
    $ric_code_ = "NA" ;
    $newedge_code_ = "NA" ;

    $eurex_file_ = "NA" ;
    $eurex_mat_file_ = "NA" ;
    $newedge_file_ = "NA" ;
    $ors_file_ = "NA" ;
    $output_file_ = "NA" ;

    $eurex_count_ = -1 ;
    $newedge_count_ = -1 ;
    $ors_count_ = -1 ;


    # get ors trades file #
    $ors_file_ = "/NAS1/logs/ORSTrades/EUREX/*/" . ( substr $yyyymmdd_,0,4 ) . "/" . (substr $yyyymmdd_,4,2 ) . "/" . ( substr $yyyymmdd_,6,2) . "/trades." .$yyyymmdd_ ;

    # get newedgeflie #
    if ( $yyyymmdd_ >= 20111101  && $yyyymmdd_ <= 20111130 )
    {
	$newedge_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20111101_20111130.csv" ;
    }
    elsif ( $yyyymmdd_ >= 20111201  && $yyyymmdd_ <= 20111231 )
    {
	$newedge_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20111201_20111231.csv" ;
    }
    elsif ( $yyyymmdd_ >= 20120101  && $yyyymmdd_ <= 20120131 )
    {
	$newedge_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20120101_20120131.csv" ;
    }
    else
    {
 	$newedge_file_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMITRN_" . $yyyymmdd_ . ".csv" ;
# 	$newedge_file_ = "/home/dvctrader/kputta/GMITRN_" . $yyyymmdd_ . ".csv" ;
    }

    # in case the file is gzipped, copy locally and then use it
    my $local_newedge_file_ = '';
    my $local_gz_newedge_file_ = '';
    
    if ( !(-e $newedge_file_)){
		my $gz_newedge_file_ = $newedge_file_.".gz";
		if ( -e $gz_newedge_file_){
			my $base_newedge_file_ = basename($gz_newedge_file_);
		    $local_gz_newedge_file_ = $output_dir.'/'.$base_newedge_file_;
		    `cp $gz_newedge_file_ $local_gz_newedge_file_`;
		    `gunzip $local_gz_newedge_file_`;
		    $local_newedge_file_ = substr($local_gz_newedge_file_, 0, -3);
		    $newedge_file_ = $local_newedge_file_;
		}
    }

    # get eurex file #  %%
    $eurex_file_ = "90RPTTE810DVCNJ".$yyyymmdd_.".XML";
    $eurex_mat_file_ = "00RPTTA111PUBLI".$yyyymmdd_.".XML";
    if ( ExistsWithSize ( "$eurex_dir/$yyyymmdd_/$eurex_file_.gz" ) )
    {
	`cp $eurex_dir/$yyyymmdd_"/"$eurex_file_".gz" $output_dir"/"`;
	`unzip -f $output_dir"/"$eurex_file_".gz"` ;
	`rm $output_dir"/"$eurex_file_".gz"`;
    }
    if ( ExistsWithSize ( "$eurex_dir/$yyyymmdd_/$eurex_file_.ZIP" ) )
    {
	`cp $eurex_dir/$yyyymmdd_"/"$eurex_file_".ZIP" $output_dir"/"`;
	`unzip $output_dir"/"$eurex_file_".ZIP"` ;
	`rm $output_dir"/"$eurex_file_".ZIP"`;
	`mv $eurex_file_ $output_dir"/"`;
	if ( ExistsWithSize ( "$eurex_dir/$yyyymmdd_/$eurex_mat_file_.ZIP" ) )
	{
	    `cp $eurex_dir/$yyyymmdd_"/"$eurex_mat_file_".ZIP" $output_dir"/"`;
	    `unzip $output_dir"/"$eurex_mat_file_".ZIP"` ;
	    `rm $output_dir"/"$eurex_mat_file_".ZIP"`;
	    `mv $eurex_mat_file_ $output_dir"/"`;
	}

    }
    elsif ( -s ( "$eurex_dir/$yyyymmdd_/$eurex_file_" ) )
    {	
	`cp $eurex_dir/$yyyymmdd_"/"$eurex_file_ $output_dir"/"`;
    }

    my @ric_codes_ = ( ) ;
    my @newedge_codes_ = ( ) ;
    if ( -e "$output_dir/$eurex_file_" )
    {
	@ric_codes_ = `grep prodId $output_dir"/"$eurex_file_ \| sort \| uniq | awk -F"\>" '{print \$2}' | awk -F"<" '{print \$1}'`;

	chomp ( @ric_codes_ ) ;

	foreach my $code_ ( @ric_codes_ )
	{
	    push ( @newedge_codes_, $ric2newedge_{ $code_ } ) ;
	}
    }
    if ( -e $newedge_file_ )
    {
	@newedge_codes_ = `awk -F',' '{ if ( ( \$1 == \"\\\"T\\\"\" ) && ( \$24 == \"\\\"27\\\"\" ) && ( \$12 == \"\\\"$yyyymmdd_\\\"\" ) ) print \$25 }' $newedge_file_ | sort | uniq ` ;
	chomp ( @newedge_codes_ ) ;
	
	if ( scalar ( @ric_codes_ ) != scalar ( @newedge_codes_ ) && scalar ( @ric_codes_ ) > 0 ) 
	{
	    @newedge_codes_ = `awk -F',' '{ if ( ( \$1 == \"T\" ) && ( \$24 == \"27\" ) && ( \$12 == \"$yyyymmdd_\" ) ) print \$25 }' $newedge_file_ | sort | uniq ` ;
	    chomp ( @newedge_codes_ ) ; 
	     if ( scalar ( @ric_codes_ ) != scalar ( @newedge_codes_ ) && scalar ( @ric_codes_ ) > 0 )
	     {
		 $msg = $msg . $yyyymmdd_ . "ERROR :: RICCODES ::" . join ( '|', @ric_codes_ ) . "NEWEDGECODES ::" . join ( '|', @newedge_codes_ ) . "\n";
		 SendReportMail ( ) ; exit ( 0 ) ;
	     }
	}

	@ric_codes_ = ( ) ; # make up your mind about using one of the if statements instead
	
	foreach my $code_ ( @newedge_codes_ )  # for ric & newedge shoud be in same order
	{
	    $code_ =~ s/"//g ;
	    push ( @ric_codes_, $newedge2ric_{ $code_ } ) ;
	}

    }
    else
    {
	my $t_ = `ls -l $ors_file_  2>/dev/null | wc -l` ;
	chomp ( $t_ ) ;
	if ( $t_ > 0 )
	{
	    if ( -e "$output_dir/$eurex_file_" )
	    {
		$msg = $msg. $yyyymmdd_." ERROR :: newedge_file missing but ors trades && eurex_file log exists\n ";
		`rm $output_dir/$eurex_file_` ;
		SendReportMail ( ) ; exit ( 0 ) ;
	    }
	    else
	    {
		$msg = $msg. $yyyymmdd_." ERROR :: newedge_file && eurex_file are missing but ors trades log exists\n ";
		SendReportMail ( ) ; exit ( 0 ) ;
	    }
	}

	next ; 
    }
    if ( scalar ( @ric_codes_ ) == 0 )
    {
	$msg = $msg. $yyyymmdd_." WARNING :: failed at newedge2ric conversion, Newedge $newedge_count_ Eurex $eurex_count_ Ors $ors_count_ \n" ;
	SendReportMail ( ) ; exit ( 0 ) ;	    
    }
    else
    {
	for ( my $idx_ = 0 ; $idx_ < scalar ( @ric_codes_ ) ; $idx_ ++ )
	{
	    $eurex_count_ = -1 ;
	    $newedge_count_ = -1 ;
	    $ors_count_ = -1 ;
	    
	    $ric_code_ =  $ric_codes_[ $idx_ ] ;
	    $newedge_code_ =  $newedge_codes_[ $idx_ ] ;
	    
	    chomp ( $ric_code_ );
	    $output_file_  = "$output_dir/bfr_".$ric_code_."_".$yyyymmdd_;
	    
	    #MakeReportUsingNewedgeFile ( ) ;
	    MakeReportUsingExchangeFile ( ) ;
	    
	    if ( -e $output_file_ )
	    {
		`cat $output_file_ >> $output_dir/$bafin_report_` ;
		`rm $output_file_` ;
		$trecords_ = $trecords_ + $eurex_count_ ;
		
		if  ( ( $newedge_count_ > 0 && $eurex_count_ > 0 ) && ( $eurex_count_ != $newedge_count_ ) )
		{
		    $msg = $msg. $yyyymmdd_." WARNING :: $ric_code_ Newedge $newedge_count_ Eurex $eurex_count_ Ors $ors_count_ \n";
		    $sendmail_ = 1 ;
		}	       
		elsif ( ( ( $ors_count_ > 0 || $newedge_count_ > 0 || $eurex_count_ > 0 ) && ( $ors_count_ != $newedge_count_  || $eurex_count_ != $newedge_count_ ) ) )
		{
		    $msg = $msg. $yyyymmdd_." WARNING :: $ric_code_ Newedge $newedge_count_ Eurex $eurex_count_ Ors $ors_count_ \n";
		    $sendmail_ = 1 ;
		}
		else
		{
		    $msg = $msg. $yyyymmdd_." $ric_code_ Newedge $newedge_count_ Eurex $eurex_count_ Ors $ors_count_ \n";
		}
	    }
	    else
	    {
		$msg = $msg. $yyyymmdd_." WARNING :: no line items created for $ric_code_ Newedge $newedge_count_ Eurex $eurex_count_ Ors $ors_count_ \n" ;
		$sendmail_ = 1 
	    }
	    
	}
    
	if ( -e "$output_dir/$eurex_file_" )
	{
	    `rm $output_dir/$eurex_file_` ;
	    `rm $output_dir/$eurex_mat_file_`;
	}
        
	if ( -e $local_newedge_file_){
	    `rm -f $local_newedge_file_`;
	}
	
	if ( -e $local_gz_newedge_file_){
	    `rm -f $local_gz_newedge_file_`;
	}
    }
}



my $tail_ = "echo -n \"$dstr;$tstr;$trecords_\"";
`$tail_ >> $output_dir/$bafin_report_`;

if ( $eurex_count_ == 0 ) {
  `rm -rf $output_dir/$bafin_report_`;
}
if ( $sendmail_ == 0 )
{
    $to_address_ = "NA" ;
}
SendReportMail ( ) ;
exit ( 0 );




sub MakeReportUsingNewedgeFile ( )
{

    #grep time date tranid orderno buysell qty price
    my @trades_ = ( );
    my @eurex_trades_ = ( );
    my @eurex_times_ = ( );

#    "FRECID"|"FCUSIP"|"FCTYM"|"FTDATE"|"FBS"|"FQTY"|"FSDSC1"|"FMULTF"|"FSDATE"|"FLTDAT"|"FEXCH"|"FFC"|"FTPRIC"|"FEXBKR"|"FTIME"|"FCHIT"
#    "FEXBKR == DVCNJ, always ?

    @trades_ = `awk -F',' '{ if ( ( \$1 == \"\\\"T\\\"\" || \$1 == \"\\\"SKIP_HEADER\\\"\" ) && ( \$24 == \"\\\"27\\\"\" || \$24 == \"\\\"FEXCH\\\"\" ) && ( \$25 == \"\\\"$newedge_code_\\\"\" || \$25 == \"\\\"FFC\\\"\") &&  ( \$12 == \"\\\"$yyyymmdd_\\\"\" || \$12 == \"\\\"FTDATE\\\"\") && ( \$34 != \"\\\"FIMFR\\\"\" ) ) print \$1"|"\$6"|"\$7"|"\$12"|"\$13"|"\$18"|"\$19"|"\$21"|"\$22"|"\$23"|"\$24"|"\$25"|"\$28"|"\$34"|"\$51"|"\$114 }' $newedge_file_ | sort -k15 -t'|' ` ;

    if ( scalar ( @trades_ ) == 0 )
	{
	    @trades_ =  `awk -F',' '{ if ( ( \$1 == \"T\" || \$1 == \"FREID\" ) && ( \$24 == 27 || \$24 == \"FEXCH\" ) && ( \$25 == \"$newedge_code_\" || \$25 == \"FFC\") &&  ( \$12 == \"$yyyymmdd_\" || \$12 == \"FTDATE\") && ( \$34 != \"FIMFR\" ) ) print \$1"|"\$6"|"\$7"|"\$12"|"\$13"|"\$18"|"\$19"|"\$21"|"\$22"|"\$23"|"\$24"|"\$25"|"\$28"|"\$34"|"\$51"|"\$114 }' $newedge_file_ | sort -k15 -t'|'` ;
	    
	}

    if ( $debug_ == 1 )
    {
	`mkdir -p /spare/local/dvctrader/BLogs/`;
	my $tempfile_ = $log_dir_."/ne_"."yyyymmdd_";
	open ( MYFILE , '>', $tempfile_ );
	print MYFILE @trades_;
	close ( MYFILE );
    }

    chomp ( @trades_ ) ;
    $newedge_count_ = scalar ( @trades_ ) ;
    
    my $t_ = `ls -l $ors_file_  2>/dev/null | wc -l` ;
    chomp ( $t_ ) ;

    if ( $t_ > 0 )
    {
	#print "awk -F'\001' '{ if ( match ( \$1, \"$ric_code_\" ) ) { print \$1" "\$2" "\$3" "\$4" "\$5 } }' $ors_file_ | wc -l";
	$ors_count_ =  `awk -F'\001' '{ if ( match ( \$1, \"$ric_code_\" ) ) { print \$1" "\$2" "\$3" "\$4" "\$5 } }' $ors_file_ | wc -l`;
	chomp ( $ors_count_ ) ;
    }
    if ( -e "$output_dir/$eurex_file_" )
    {
	print $ric_code_."\n";
	if ( $ric_code_ eq "FVS" )
	{
	    $ric_code_ = "FVS ";
	}
	@eurex_trades_ = `$BIN_DIR/retrieve_eurex_trades_v1  $output_dir/$eurex_file_ \"$ric_code_\" | awk -F'|' '{ if ( NF > 5 ) print \$0 ; }'` ;
	$eurex_count_ = `$BIN_DIR/retrieve_eurex_trades_v1  $output_dir/$eurex_file_ \"$ric_code_\" | awk -F'|' '{ if ( NF > 5 ) print \$0 ; }' | wc -l` ;
	@eurex_times_ = `$BIN_DIR/retrieve_eurex_trades_v1  $output_dir/$eurex_file_ \"$ric_code_\" | awk -F'|' '{ if ( NF > 5 ) print \$0 ; }' | awk -F'|' '{  split ( \$1, tk, \".\" ) ; split ( tk[1],tks ,\":\" ) ;print tks[1]tks[2]tks[3]; }' | sort `;
	chomp ( $eurex_count_ ) ;
	chomp ( @eurex_times_ ) ;
	if ( $ric_code_ eq "FVS " )
	{
	    $ric_code_ = "FVS";
	}
	if ( $debug_ == 1 )
	{
	    `mkdir -p /spare/local/dvctrader/BLogs/`;
	    my $tempfile_ = $log_dir_."/ex_"."yyyymmdd_";
	    open ( MYFILE , '>', $tempfile_ );
	    print MYFILE @eurex_trades_;
	    close ( MYFILE );
	}
    }

    if ( $newedge_count_ <= 0 )
    {
	return ;
    }


#common fields

    my $NF = 61;
    my @flds_ = ( ("") x $NF ) ;    

    $flds_[ 0 ] = "M" ;
    $flds_[ 1 ] = "DVCNJ" ;
    $flds_[ 6 ] = "M" ;
    $flds_[ 7 ] = "EUREX" ;
    $flds_[ 17 ] = "1" ;  # 1 = gross 3 = agrregrate
    $flds_[ 18 ] = "J" ;
    $flds_[ 19 ] = "E" ;
    $flds_[ 21 ] = "DE" ;
    $flds_[ 22 ] = "XEUR" ;
    $flds_[ 23 ] = "XXK" ; # for all traded on EUREX
    
    if ( exists ( $price_curr_iso4217 { $ric_code_ } ) )
    {
	$flds_[ 25 ] = $price_curr_iso4217 { $ric_code_ }  ;	
    }
    else 
    {
	$msg = $msg."ERROR there is no price_currency iso4217 for $ric_code_\n" ;
	SendReportMail ( ) ;
	exit  ( 0 ) ;	
    }
    
    
    if ( exists ( $iso_10962 { $ric_code_ } ) )
    {
	$flds_[ 29 ] = $iso_10962 { $ric_code_ } ; 
	
    }
    else 
    {
	$msg = $msg."ERROR there is no iso 10962  for $ric_code_ \n" ;
	SendReportMail ( ) ;
	exit ( 0 ) ;
    }

    $flds_[ 31 ] = "XP" ;
    
    if ( exists ( $sec_identification { $ric_code_ } ) )
    {
	$flds_[ 34 ] = $sec_identification { $ric_code_  } ; 

    }
    else 
    {
	
	$msg = $msg."ERROR there is no security idenfitification for $ric_code_ \n" ;
	SendReportMail ( ) ;
	exit ( 0 ) ;
    }
    
    
    if ( exists ( $unit_of_security { $ric_code_ } ) )
    {
	$flds_[ 35 ] = $unit_of_security { $ric_code_  } ; 
	
    }
    else 
    {
	$msg = $msg. "ERROR there is no unit of security for $ric_code_ \n" ;
	SendReportMail ( ) ; exit ( 0 ) ;
    }
    
    if ( exists ( $sec_type { $ric_code_ } ) )
    {
	$flds_[ 38 ] = $sec_type { $ric_code_ } ; 
	
    }
    else 
    {
	$msg = $msg. "ERROR there is no sec_type for $ric_code_ \n" ;
	SendReportMail ( ) ; exit ( 0 ) ;
    }
    
    
    if ( exists ( $underlying_isin { $ric_code_ } ) )
    {
	$flds_[ 40 ] = $underlying_isin { $ric_code_ } ;
    }
    elsif ( exists ( $product_isin { $ric_code_ } ) )
    {
	$flds_[ 40 ] = $product_isin { $ric_code_ } ;
    }
    else 
    {
	$msg = $msg. "ERROR there is no product isin for $ric_code_ \n" ;
	SendReportMail ( ) ; exit ( 0 ) ;
    }
    
    
    $flds_[ 41 ] = $price_multiplier { $ric_code_ }  ;
    $flds_[ 41 ] =~ s/\./\,/g ;
    
    
    if ( $flds_[ 38 ] eq "F" )
    {
	$flds_[ 43 ] = "" ;
	$flds_[ 44 ] = 0 ;
	$flds_[ 44 ] =~ s/\./\,/g ;

    }
    else
    {
	$msg = $msg.  "write code to get option underlying price \n" ;
	SendReportMail ( ) ; exit ( 0 );
    }
    
# N = normal A = already reported
    $flds_ [ 49 ] = "N" ;
    
    
    $flds_[50] = CalcNextBusinessDay ( $yyyymmdd_ );

    
    $flds_[ 32 ] = $ric_code_ ;
    
    $flds_ [ 51 ] = "MBOX" ;
    
    $flds_ [ 54 ] = "40014504" ;  # replace with SWIFT-BIC 
    
    my $check_and_replace_time_ = 1;
    my $capture_time_mismatch_error_ = 1;

    `mkdir -p /spare/local/dvctrader/BLogs/`;
    my $tempfile_ = $log_dir_."/ex_"."yyyymmdd_";
    open ( MYFILE , '>', $tempfile_ );
    print MYFILE @eurex_trades_;
    close ( MYFILE );

    $tempfile_ = $log_dir_."/ne_"."yyyymmdd_";
    open ( MYFILE , '>', $tempfile_ );
    print MYFILE @trades_;
    close ( MYFILE );

    if ( scalar ( @trades_ ) > 0 ) 
    {
	open FHW, "> $output_file_";
  
	for ( my $i = 0 ; $i < scalar ( @trades_ ) ; $i++ )
	{
	    
	    #   "FRECID"|"FCUSIP"|"FCTYM"|"FTDATE"|"FBS"|"FQTY"|"FSDSC1"|"FMULTF"|"FSDATE"|"FLTDAT"|"FEXCH"|"FFC"|"FTPRIC"|"FEXBKR"|"FTIME"|"FCHIT"
	    
	    my @tks_ = split ('\|' , $trades_ [ $i ] ) ;
	    
	    if ( scalar ( @tks_ )  != 16 ) 
	    {  	
		$msg = $msg.@tks_."ERROR : number of tokens in newedge file doesnt match expected number  \n" ;
		SendReportMail ( ) ; exit ( 0 );
	    }

	    #date format and time format
	    $tks_[ 3 ] =~ s/"//g ;
	    $flds_ [ 12 ] = $tks_[ 3 ] ;
	    $tks_[ 14 ] =~ s/"//g ;
	    $flds_ [ 13 ] = $tks_[ 14 ] ;

	    if ( $check_and_replace_time_ == 1 )
	    {
		if ( abs ( int ( $flds_[13] )  - int ( $eurex_times_[$i] ) ) > 2 )  # greater than 2 minutes is fine but still place eurex time stamp
		{
		    if ( $capture_time_mismatch_error_ == 1 )
		    {
			$msg = $msg."newedge transaction time doesnt match with eurex time stamp, please check, replacing and continuing, $eurex_times_[0] $flds_[ 13 ]\n";
			$capture_time_mismatch_error_ = 0;
		    }
		}
		# $check_time_ = 0;
		$flds_[13] = $eurex_times_[$i];
		#$flds_[13] = $flds_[13] + 20000 ;
		#$flds_[13] = sprintf ( "%06d" , $flds_[13] ) ;
	    }

	    #buysell format
	    $tks_[ 4 ] =~s/"//g ;
	    $flds_[ 16 ] = ( ( $tks_[ 4 ] == 1 )? 'K' : 'V' ) ;
	    
	    # transaction ID
	    $tks_[ 2 ] =~s/"//g ;
	    $flds_[ 14 ] =  sprintf ( "%06d", ( $i + 1 ) ) . " " . $flds_[ 13 ] . " " .$ric_code_.$tks_[ 2 ] . " " .$flds_[ 16 ] ;
	    
	    #qty format
	    $tks_[ 5 ] =~ s/"//g ;
	    $flds_[ 24 ] = sprintf ( "%d" , $tks_[ 5 ] ) ;
	    $flds_[ 24 ] =~ s/\./\,/g ;
	
	    #price format
	    $tks_[ 12 ] =~ s/"//g ;
	    $tks_[ 12 ] =~ s/\+//g ; 
	    $flds_[ 26 ] = sprintf ( "%g" , $tks_ [ 12 ] ) ;
	    $flds_[ 26 ] =~ s/\./\,/g ;
	    
	    #ltd 
	    $tks_[ 9 ] =~ s/"//g ;
	    $flds_[ 46 ] = $tks_[ 9 ] ;
	    
	    $flds_ [ 53 ] = $serial_no_ + $counter_ ; # serial number starting from 1 for each report per person per day
	    $counter_ ++ ;	
	    
	    print FHW join(';', @flds_ ),"\r\n" ;	
	}
	
	close FHW ;
    }
	
}



sub MakeReportUsingExchangeFile ( )
{

    my @trades_ = ();
    @trades_ = `awk -F',' '{ if ( ( \$1 == \"\\\"T\\\"\" || \$1 == \"\\\"SKIP_HEADER\\\"\" ) && ( \$24 == \"\\\"27\\\"\" || \$24 == \"\\\"FEXCH\\\"\" ) && ( \$25 == \"\\\"$newedge_code_\\\"\" || \$25 == \"\\\"FFC\\\"\") &&  ( \$12 == \"\\\"$yyyymmdd_\\\"\" || \$12 == \"\\\"FTDATE\\\"\") && ( \$34 != \"\\\"FIMFR\\\"\" ) ) print \$1"|"\$6"|"\$7"|"\$12"|"\$13"|"\$18"|"\$19"|"\$21"|"\$22"|"\$23"|"\$24"|"\$25"|"\$28"|"\$34"|"\$51"|"\$114 }' $newedge_file_ | sort -k15 -t'|' ` ;

    if ( scalar ( @trades_ ) == 0 )
	{
	    @trades_ =  `awk -F',' '{ if ( ( \$1 == \"T\" || \$1 == \"FREID\" ) && ( \$24 == 27 || \$24 == \"FEXCH\" ) && ( \$25 == \"$newedge_code_\" || \$25 == \"FFC\") &&  ( \$12 == \"$yyyymmdd_\" || \$12 == \"FTDATE\") && ( \$34 != \"FIMFR\" ) ) print \$1"|"\$6"|"\$7"|"\$12"|"\$13"|"\$18"|"\$19"|"\$21"|"\$22"|"\$23"|"\$24"|"\$25"|"\$28"|"\$34"|"\$51"|"\$114 }' $newedge_file_ | sort -k15 -t'|'` ;
	}

    if ( $debug_ == 1 )
    {
	`mkdir -p /spare/local/dvctrader/BLogs/`;
	my $tempfile_ = $log_dir_."/ne_"."yyyymmdd_";
	open ( MYFILE , '>', $tempfile_ );
	print MYFILE @trades_;
	close ( MYFILE );
    }

    chomp ( @trades_ ) ;
    $newedge_count_ = scalar ( @trades_ ) ;
    
    my $t_ = `ls -l $ors_file_  2>/dev/null | wc -l` ;
    chomp ( $t_ ) ;

    if ( $t_ > 0 )
    {
	#print "awk -F'\001' '{ if ( match ( \$1, \"$ric_code_\" ) ) { print \$1" "\$2" "\$3" "\$4" "\$5 } }' $ors_file_ | wc -l";
	$ors_count_ =  `awk -F'\001' '{ if ( match ( \$1, \"$ric_code_\" ) ) { print \$1" "\$2" "\$3" "\$4" "\$5 } }' $ors_file_ | wc -l`;
	chomp ( $ors_count_ ) ;
    }
    if ( $ric_code_ eq "FVS" )
    {
#	$ric_code_ = "FVS ";
    }
    $eurex_count_ = `$BIN_DIR/retrieve_eurex_trades_v1  $output_dir/$eurex_file_ \"$ric_code_\" | awk -F'|' '{ if ( NF > 5 ) print \$0 ; }' | wc -l` ;
    chomp ( $eurex_count_);
    if ( $ric_code_ eq "FVS " )
    {
	$ric_code_ = "FVS";
    }
##
    my @eurex_trades_ = ( );
    my @mdates_= ( );
    my %mat_dates_ = ( );
    if ( -e "$output_dir/$eurex_file_" )
    {
	if ( $ric_code_ eq "FVS" )
	{
#	    $ric_code_ = "FVS ";
	}
	@eurex_trades_ = `$BIN_DIR/retrieve_eurex_trades_v1  $output_dir/$eurex_file_ \"$ric_code_\" | awk -F'|' '{ if ( NF > 5 ) print \$0 ; }'` ;
	if ( ExistsWithSize ( "$output_dir/$eurex_mat_file_" ) )
	{
	    @mdates_ = `$BIN_DIR/retrieve_eurex_MatDat  $output_dir/$eurex_mat_file_ \"$ric_code_\"` ;
	    chomp(@mdates_);
	    for ( my $k = 0 ; $k < scalar ( @mdates_ ) ; $k++)
	    {
		my @ttks_ = split ( ' ', $mdates_[$k] );
		$mat_dates_{$ttks_[0].$ttks_[1]} = $ttks_[2];
	    }
	}

	if ( $ric_code_ eq "FVS " )
	{
#	    $ric_code_ = "FVS";
	}
	if ( $debug_ == 1 )
	{
	    `mkdir -p /spare/local/dvctrader/BLogs/`;
	    my $tempfile_ = $log_dir_."/ex_"."yyyymmdd_";
	    open ( MYFILE , '>', $tempfile_ );
	    print MYFILE @eurex_trades_;
	    close ( MYFILE );
	}
    }


#LTrading
    my %ne_mtdate_;
    my @nltd_ = `awk -F',' '{ gsub(\"\\\"\", \"\") ;  if ( \$25 == \"$newedge_code_\") print \$25\" \"\$23 }' $newedge_file_ | sort | uniq` ;
    chomp(@nltd_);
    for( my $j=0; $j<scalar(@nltd_);$j++)
    {
	my @tttks_= split(" ", $nltd_[$j]);
	if ( scalar(@tttks_) < 2 )
	{
	    next ;
	}
	$ne_mtdate_{$ric_code_.int(substr $tttks_[1],4,2).(substr $tttks_[1],0,4)}=$tttks_[1];
    }
#common fields

    my $NF = 61;
    my @flds_ = ( ("") x $NF ) ;    

    $flds_[ 0 ] = "M" ;
    $flds_[ 1 ] = "DVCNJ" ;
    $flds_[ 6 ] = "M" ;
    $flds_[ 7 ] = "EUREX" ;
    $flds_[ 17 ] = "1" ;  # 1 = gross 3 = agrregrate
    $flds_[ 18 ] = "J" ;
    $flds_[ 19 ] = "E" ;
    $flds_[ 21 ] = "DE" ;
    $flds_[ 22 ] = "XEUR" ;
    $flds_[ 23 ] = "XXK" ; # for all traded on EUREX
    
    if ( exists ( $price_curr_iso4217 { $ric_code_ } ) )
    {
	$flds_[ 25 ] = $price_curr_iso4217 { $ric_code_ }  ;	
    }
    else 
    {
	$msg = $msg."ERROR there is no price_currency iso4217 for $ric_code_\n" ;
	SendReportMail ( ) ;
	exit  ( 0 ) ;	
    }
    
    
    if ( exists ( $iso_10962 { $ric_code_ } ) )
    {
	$flds_[ 29 ] = $iso_10962 { $ric_code_ } ; 
	
    }
    else 
    {
	$msg = $msg."ERROR there is no iso 10962  for $ric_code_ \n" ;
	SendReportMail ( ) ;
	exit ( 0 ) ;
    }

    $flds_[ 31 ] = "XP" ;
    
    if ( exists ( $sec_identification { $ric_code_ } ) )
    {
	$flds_[ 34 ] = $sec_identification { $ric_code_  } ; 

    }
    else 
    {
	
	$msg = $msg."ERROR there is no security idenfitification for $ric_code_ \n" ;
	SendReportMail ( ) ;
	exit ( 0 ) ;
    }
    
    
    if ( exists ( $unit_of_security { $ric_code_ } ) )
    {
	$flds_[ 35 ] = $unit_of_security { $ric_code_  } ; 
	
    }
    else 
    {
	$msg = $msg. "ERROR there is no unit of security for $ric_code_ \n" ;
	SendReportMail ( ) ; exit ( 0 ) ;
    }
    
    if ( exists ( $sec_type { $ric_code_ } ) )
    {
	$flds_[ 38 ] = $sec_type { $ric_code_ } ; 
	
    }
    else 
    {
	$msg = $msg. "ERROR there is no sec_type for $ric_code_ \n" ;
	SendReportMail ( ) ; exit ( 0 ) ;
    }
    
    
    if ( exists ( $underlying_isin { $ric_code_ } ) )
    {
	$flds_[ 40 ] = $underlying_isin { $ric_code_ } ;
    }
    elsif ( exists ( $product_isin { $ric_code_ } ) )
    {
	$flds_[ 40 ] = $product_isin { $ric_code_ } ;
    }
    else 
    {
	$msg = $msg. "ERROR there is no product isin for $ric_code_ \n" ;
	SendReportMail ( ) ; exit ( 0 ) ;
    }
    
    
    $flds_[ 41 ] = $price_multiplier { $ric_code_ }  ;
    $flds_[ 41 ] =~ s/\./\,/g ;
    
    
    if ( $flds_[ 38 ] eq "F" )
    {
	$flds_[ 43 ] = "" ;
	$flds_[ 44 ] = 0 ;
	$flds_[ 44 ] =~ s/\./\,/g ;

    }
    else
    {
	$msg = $msg.  "write code to get option underlying price \n" ;
	SendReportMail ( ) ; exit ( 0 );
    }
    
# N = normal A = already reported
    $flds_ [ 49 ] = "N" ;
    
    $flds_[50] = CalcNextBusinessDay ( $yyyymmdd_ );
    
    $flds_[ 32 ] = $ric_code_ ;
    
    $flds_[ 51 ] = "MBOX" ;
    
    $flds_[ 54 ] = "40014504" ;  # replace with SWIFT-BIC     
    
    if ( scalar ( @eurex_trades_ ) > 0 )
    {
	open FHW, "> $output_file_";
	for ( my $i = 0 ; $i < scalar ( @eurex_trades_ ) ; $i++ )
	{
	    my @tks_= split ('\|', $eurex_trades_[ $i ] );
	    #date
	    $tks_[ 14 ] =~ s/-//g;
	    $flds_ [ 12 ] = $tks_[ 14 ] ;
	    #time
	    $tks_[ 0 ] =~ s/://g;
	    $tks_[ 0 ] = substr $tks_[ 0 ], 0, 6 ;
	    $flds_ [ 13 ] = $tks_[ 0 ] ;
	    #BS
	    $flds_[ 16 ] = ( ( $tks_[ 3 ] eq 'B' )? 'K' : 'V' ) ;
	    #TID
	    $flds_[ 14 ] =  sprintf ( "%06d", ( $i+1 ) ). " " . $flds_[ 13 ] . " " .$ric_code_.$tks_[22].(sprintf("%02d", $tks_[21])). " " .$flds_[ 16 ] ;
	    #QTY
	    $flds_[ 24 ] = sprintf ( "%d" , $tks_[ 5 ] ) ;
	    #PRICE
	    $tks_[7] =~ s/\+//g;
	    $flds_[ 26 ] = sprintf ( "%g" , $tks_ [ 7 ] ) ;
	    $flds_[ 26 ] =~ s/\./\,/g ;

	    #MATURITY
	    my $matdate_=$mat_dates_{$ric_code_.$tks_[21].$tks_[22]};
	    #print STDOUT $matdate_." ".$ne_mtdate_{$ric_code_.$tks_[21].$tks_[22]}."\n";
	    if ( ! $matdate_ || length($matdate_) < 2 )
	    {
		$matdate_ = $ne_mtdate_{$ric_code_.$tks_[21].$tks_[22]};
		if ( length ( $matdate_ )  < 2 )
		{
		    $msg = $msg. "ERROR there is no maturity date for $ric_code_.$tks_[21].$tks_[22] \n" ;
		    SendReportMail ( ) ; exit ( 0 ) ;
		}
	    }
	    $matdate_=~ s/-//g;
	    $flds_[ 46 ] = $matdate_ ;

	    #COUNTER
	    $flds_ [ 53 ] = $serial_no_ + $counter_ ; # serial number starting from 1 for each report per person per day
	    $counter_ ++ ; 
	    print FHW join(';', @flds_ ),"\r\n" ;
	}
	close FHW;
    }
}

sub SendReportMail ( )
{
    print STDOUT $msg;

    if ( $to_address_ eq "NA" )
    {
	return;
    }
    else
    {
	if ( $to_address_ && $msg )
	{
	    open ( MAIL , "|/usr/sbin/sendmail -t" );
	    
	    print MAIL "To: $to_address_\n";
	    print MAIL "From: $from_address_ \n";
	    print MAIL "Subject: Daily Job Report : generate_bafin_report  $fdate_ $tdate_  \n\n";
	    print MAIL $msg ;
	    
	    close(MAIL);
	}
    }
}

