#!/usr/bin/perl

use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
require "$GENPERLLIB_DIR/calc_next_business_day.pl"; # ExistsWithSize

my $fdate_= $ARGV [ 0 ] ;
my $tdate_= $ARGV [ 1 ] ;

my $newedgecode_= $ARGV [ 2 ] ;
my $riccode_= $ARGV [ 3 ] ;

for ( my $yyyymmdd_ = $fdate_ ; $yyyymmdd_ <= $tdate_ ; $yyyymmdd_ = CalcNextBusinessDay ( $yyyymmdd_ ) )
{

    my $orsfile_ = "/NAS1/logs/ORSTrades/EUREX/*/" . ( substr $yyyymmdd_,0,4 ) . "/" . (substr $yyyymmdd_,4,2 ) . "/" . ( substr $yyyymmdd_,6,2) . "/trades." .$yyyymmdd_ ;
    
    my $newedgefile_ = "NA" ;
    
    if ( $yyyymmdd_>= 20111101  && $yyyymmdd_ <= 20111130 )
    {
	$newedgefile_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20111101_20111130.csv" ;
    }
    elsif ( $yyyymmdd_>= 20111201  && $yyyymmdd_ <= 20111231 )
    {
	$newedgefile_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20111201_20111231.csv" ;
    }
    elsif ( $yyyymmdd_>= 20120101  && $yyyymmdd_ <= 20120131 )
    {
	$newedgefile_ = "/NAS1/data/MFGlobalTrades/MFGFiles/FIMTRNTEXT20120101_20120131.csv" ;
    }
    else
    {
 	$newedgefile_ = "/NAS1/data/MFGlobalTrades/MFGFiles/GMITRN_" . $yyyymmdd_ . ".csv" ;
    }

    my $eurex_file_ = "/NAS1/data/MFGlobalTrades/EUREX810/00RPTTC810DVCNJ".$yyyymmdd_."FIMFR.XML.gz";

    my $t_ = `ls -l $orsfile_  2>/dev/null | wc -l` ;

    chomp ( $t_ ) ;

    if ( ( $t_ == 0 ) &&  ( ! -e $newedgefile_ ) )
    {
	print "$orsfile_ $newedgefile_ for $yyyymmdd_ are missing\n" ;
	next ;
    }    
     
#print `echo "SYMBL BUYSELL QTY SAOS" ; awk -F'\001' '{ if ( match ( \$1, \"$riccode_\" ) ) { print \$1" "\$2" "\$3" "\$4" "\$5 } }' $orsfile_ | wc -l`;
    #print "ors no: ";

    my $orsn_ = -1 ;
    if ( $t_ != 0 )
    {
	$orsn_ =  `awk -F'\001' '{ if ( match ( \$1, \"$riccode_\" ) ) { print \$1" "\$2" "\$3" "\$4" "\$5 } }' $orsfile_ | wc -l`;
	chomp ( $orsn_ ) ;
    }

    my $nen_ = -1 ;
    if ( -e $newedgefile_ )
    {
#print `awk -F',' '{ if ( ( \$1 == \"\\\"T\\\"\" || \$1 == \"\\\"FRECID\\\"\" ) && ( \$24 == \"\\\"27\\\"\" || \$24 == \"\\\"FEXCH\\\"\" ) && ( \$25 == \"\\\"$newedgecode_\\\"\" || \$25 == \"\\\"FFC\\\"\") ) print \$1"|"\$6"|"\$7"|"\$12"|"\$13"|"\$18"|"\$19"|"\$21"|"\$22"|"\$23"|"\$24"|"\$25"|"\$28"|"\$34"|"\$51"|"\$114"|"\$116 }' $newedgefile_ | sort -k15 -t'|' | wc -l` ;
	$nen_ =  `awk -F',' '{ if ( ( \$1 == \"\\\"T\\\"\" || \$1 == \"\\\"FREID\\\"\" ) && ( \$24 == \"\\\"27\\\"\" || \$24 == \"\\\"FEXCH\\\"\" ) && ( \$25 == \"\\\"$newedgecode_\\\"\" || \$25 == \"\\\"FFC\\\"\") &&  ( \$12 == \"\\\"$yyyymmdd_\\\"\" || \$12 == \"\\\"FTDATE\\\"\") ) print \$1"|"\$6"|"\$7"|"\$12"|"\$13"|"\$18"|"\$19"|"\$21"|"\$22"|"\$23"|"\$24"|"\$25"|"\$28"|"\$34"|"\$51"|"\$114"|"\$116 }' $newedgefile_ | sort -k15 -t'|' | wc -l` ;

	chomp ( $nen_ );
	
	if ( $nen_ == 0 )
	{
	    $nen_ =  `awk -F',' '{ if ( ( \$1 == \"T\" || \$1 == \"FREID\" ) && ( \$24 == 27 || \$24 == \"FEXCH\" ) && ( \$25 == \"$newedgecode_\" || \$25 == \"FFC\") &&  ( \$12 == \"$yyyymmdd_\" || \$12 == \"FTDATE\") ) print \$1"|"\$6"|"\$7"|"\$12"|"\$13"|"\$18"|"\$19"|"\$21"|"\$22"|"\$23"|"\$24"|"\$25"|"\$28"|"\$34"|"\$51"|"\$114"|"\$116 }' $newedgefile_ | sort -k15 -t'|' | wc -l` ;
	    chomp ( $nen_ );
	    
	}

    }

    my $en_ = -1 ;

    if ( -e $eurex_file_ )
    {
	`cp $eurex_file_ .` ;
	my $f_ = "00RPTTC810DVCNJ".$yyyymmdd_."FIMFR.XML";
	`gunzip $f_`;

	$en_ = `$BIN_DIR/retrieve_eurex_trades  $f_ $riccode_ | awk -F'|' '{ if ( NF > 5 ) print \$0 ; }' | wc -l` ;
	chomp ( $en_ ) ;
	`rm $f_`;
    }
    else
    {
	$eurex_file_ = "/NAS1/data/MFGlobalTrades/EUREX810/00RPTTC810DVCNJ".$yyyymmdd_."FIMFR.XML";
	if ( -e $eurex_file_ )
	{
	    `cp $eurex_file_ .` ;
	    my $f_ = "00RPTTC810DVCNJ".$yyyymmdd_."FIMFR.XML";

	    $en_ = `$BIN_DIR/retrieve_eurex_trades  $f_ $riccode_ | awk -F'|' '{ if ( NF > 5 ) print \$0 ; }' | wc -l` ;
	    chomp ( $en_ ) ;
	    `rm $f_`;
	
	}
    }

    
    if ( ( ( $orsn_ > 0 || $nen_ > 0 || $en_ > 0 ) && ( $orsn_ != $nen_  && $orsn_ != $en_ ) ) || ( ( $nen_ > 0 && $en_ > 0 ) && ( $en_ != $nen_ ) ) ) 
    {
	print "WARNING :: $yyyymmdd_ :: $orsn_ \t NE :: $nen_ \t E :: $en_ \n";
    }
    
#    print "$yyyymmdd_ :: $orsn_ \t NE :: $nen_ \t E :: $en_ \n";

}


