#!/usr/bin/perl

# \file scripts/check_hist_pnl_prod.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script generates a contract file for nse
# inputs are 
# ( a ) last day's bhavcopy file,
# ( b ) lot size specification file
# ( c ) strike scheme file for options
# ( d ) date for which contract file needs to be generated and mode ( C/A )

use strict;
use warnings;
use FileHandle;
use List::Util qw/max min/;
use Math::Round;

sub formatDate; 
sub formatMonth;
sub checkAndAdd;

my $GENPERLLIB_DIR="/home/pengine/prod/live_scripts";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE = "$0 <bhavcopy.csv> <mktlots.csv> <strike_scheme.csv> <output_dated_contract_file> <mode , A-append> <appended_data_exchsymbol_file> [expiry_date]";

if ( $#ARGV < 6 ){ print $USAGE."\n"; exit( 0 ); }
my $bhavcopy_file_ = $ARGV[0];
my $mktlots_file_ = $ARGV[1];
my $strike_scheme_file_ = $ARGV[2];
my $output_file_ = $ARGV[3];
my $mode_ = $ARGV[4];
my $datasource_exchsymbol_file_ = $ARGV[5];
my $next_working_date_ = $ARGV[6];

my $this_month_expiry_date_ = -1;
if ( $#ARGV == 7 ){
	$this_month_expiry_date_ = $ARGV[7];
	print "Setting Expiry Day : $this_month_expiry_date_\n";
}

my $default_lotsize_file_ = "/spare/local/tradeinfo/NSE_Files/Lotsizes/fo_lotsize_default.txt";

#month representation in mktlots file
my %monthstrtonum_1 = ("JAN",1,"FEB",2,"MAR",3,"APR",4,"MAY",5,"JUN",6,"JUL",7,"AUG",8,"SEP",9,"OCT",10,"NOV",11,"DEC",12 );
my %datasource_exchsymbol_map_ = ();
my $highest_exchsymbol_offset_ = 0;

#storing all expiries per underlying
my %underlying_to_expiries_ = ();
#maintain expiry date maps per underlying for creating spread symbols 
sub AddUnderlyingExpiry
{
    ( my $this_underlying_, my $this_expiry_ ) = @_;
    if( ! exists $underlying_to_expiries_{ $this_underlying_ } )
    {
	my @exp_vectors_ = ( $this_expiry_ );
	$underlying_to_expiries_{ $this_underlying_ } = \@exp_vectors_;
    }
    else
    {
	if ( ! ( $this_expiry_ ~~ @{ $underlying_to_expiries_{ $this_underlying_ } } ) )
	{
	    push( @{ $underlying_to_expiries_{ $this_underlying_ } }, $this_expiry_ );
	}
    }
}

#add entry to datasource exchsymbol file as needed
sub checkAndAdd
{
    
    ( my $instrument_datasource_name_, my $OUTPUTFILE ) = @_;
    if( ! exists $datasource_exchsymbol_map_{ $instrument_datasource_name_} )
    {
	$highest_exchsymbol_offset_ = $highest_exchsymbol_offset_ + 1;
	my $added_entry_string_ = "NSE".$highest_exchsymbol_offset_;
	printf $OUTPUTFILE "%s\t%s\n", $added_entry_string_, $instrument_datasource_name_ ;
	$datasource_exchsymbol_map_{ $instrument_datasource_name_ } = $added_entry_string_;
    }
}
#parse date in mktlots file
sub formatMonth
{
    my ( $month_string_ ) = @_;
    my @month_tokens_ = split(/\-/,$month_string_ );
    my $month_num_ = "";
   	if ( exists( $monthstrtonum_1{uc($month_tokens_[1])})){
    	$month_num_ = 200000 + $month_tokens_[0]*100 + $monthstrtonum_1{ uc( $month_tokens_[1] ) };
     }
    else{
      	$month_num_ = 200000 + $month_tokens_[1]*100 + $monthstrtonum_1{ uc($month_tokens_[0]) };
    }
    return $month_num_;
}

#parse date in bhavcopy files
sub formatDate
{
    my ( $date_string_ ) = @_;
    my @date_tokens_ = split(/[\-\s]+/,$date_string_ );
    my $date_num_ = $date_tokens_[2]*10000 + $monthstrtonum_1{ uc ( $date_tokens_[1] ) }*100 + $date_tokens_[0];
    return $date_num_;
}

# process mktlots file
my %underlyingmonth_lot_map_ = ();
open ( FILE, "<", $mktlots_file_ ) or PrintStacktraceAndDie( "Could not open file $mktlots_file_ \n" );
my $file_line_ = <FILE>;
chomp ( $file_line_ );
my @fline_tokens_ = split(/[,\s]+/, $file_line_ );
my $month_1 = formatMonth( $fline_tokens_[2] );
my $month_2 = formatMonth( $fline_tokens_[3] );
my $month_3 = formatMonth( $fline_tokens_[4] );

while ( $file_line_ = <FILE> )
{
    chomp( $file_line_ );
    @fline_tokens_  = split(/[\s]*,/,$file_line_ );
    my $underlying_month_1 = $fline_tokens_[1]."_".$month_1;
    $underlyingmonth_lot_map_{ $underlying_month_1 } = $fline_tokens_[2];
    my $underlying_month_2 = $fline_tokens_[1]."_".$month_2;
    $underlyingmonth_lot_map_{ $underlying_month_2 } = $fline_tokens_[3];
    my $underlying_month_3 = $fline_tokens_[1]."_".$month_3;
    $underlyingmonth_lot_map_{ $underlying_month_3 } = $fline_tokens_[4];
}
close( FILE );

#load default lotsize map as well ( since we don't have lotsize maps historically ) - used only for FO 
my %underlying_default_lotsize_map_ = ();
open( FILE, "<", $default_lotsize_file_ ) or PrintStacktraceAndDie( "Could not open file $default_lotsize_file_ \n");
while( $file_line_ = <FILE> )
{
    chomp( $file_line_ );
    my @fline_tokens_ = split(' ', $file_line_ );
    my $fline_tokens_size_ = @fline_tokens_;
    if( $fline_tokens_size_ == 2 && $fline_tokens_[0] !~ /#/ )
    {	
	$underlying_default_lotsize_map_{ $fline_tokens_[0] } = $fline_tokens_[1];
    }
    else
    {
	#debug statement
	PrintStacktraceAndDie( "Weird line in default lotsize file $file_line_ \n" );
    }
}
close (FILE);


#process strike scheme map ( onlyfor stock options in F&O segment )
my %underlying_to_step_ = ();

# Option's strike-granularity can be affected by two strike files 
my %underlying_to_step_prev_ = ();

my %underlying_to_numitm_ = ();

# Num item in prev strike scheme
my %underlying_to_numitm_prev_ = ();

open( FILE, "<", $strike_scheme_file_ ) or PrintStacktraceAndDie( "Could not open file $strike_scheme_file_ \n" );
while ( $file_line_ = <FILE> )
{
    chomp( $file_line_ );
    my @fline_tokens_ = split(/[,\s]+/, $file_line_ );
    $underlying_to_step_{ $fline_tokens_[0] } = $fline_tokens_[1];
    
    # initializing prev step map
    $underlying_to_step_prev_{ $fline_tokens_[0] } = $fline_tokens_[1];
    
    my @num_strike_tokens_ = split(/[\s\-]+/, $fline_tokens_[2] );
    #we currently load only 4 strikes itm/otm for stocks
    $underlying_to_numitm_{ $fline_tokens_[0] } = $num_strike_tokens_[0];
    
    # initializing prev num items
    $underlying_to_numitm_prev_{ $fline_tokens_[0] } = $num_strike_tokens_[0];
}
close( FILE );

# Finding out previous quarter's sos file and appending in contract 
# /spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_0616.csv : month "06" will be at 62nd index
my $curr_month_ = substr($strike_scheme_file_ ,62, 2);
my $curr_year_ = substr($strike_scheme_file_ ,64, 2);
my $prev_month_ = -1;
my $prev_year_ = -1;

if ( $curr_month_ > 3 and $curr_month_ <= 12 ) {
	$prev_month_ = $curr_month_ - 3;
	$prev_year_ = $curr_year_;
}elsif($curr_month_ <= 3 and $curr_month_ >= 1 ) {
	$prev_month_ = $curr_month_ + 9;
	$prev_year_ = $curr_year_ - 1;
}else {
	PrintStacktraceAndDie( "Month in Strike Scheme File not between 1-12 \n" );
}

$prev_month_ = sprintf ("%02d", $prev_month_);

my $prev_strike_scheme_file = "/spare/local/tradeinfo/NSE_Files/StrikeSchemeFiles/sos_scheme_".$prev_month_.$prev_year_.".csv";

#print "Prev Strike File: ".$prev_strike_scheme_file ;

# setting prev step value from prev sos file
if ( -e $prev_strike_scheme_file ){
	open( FILE, "<", $prev_strike_scheme_file ) or PrintStacktraceAndDie( "Could not open file $prev_strike_scheme_file \n" );
	while ( $file_line_ = <FILE> )
	{
	    chomp( $file_line_ );
	    my @fline_tokens_ = split(/[,\s]+/, $file_line_ );
	    $underlying_to_step_prev_{ $fline_tokens_[0] } = $fline_tokens_[1];
	    
	    my @num_strike_tokens_ = split(/[\s\-]+/, $fline_tokens_[2] );
	    $underlying_to_numitm_prev_{ $fline_tokens_[0] } = $num_strike_tokens_[0];
	}
	close( FILE );
}

#map from string to closing price
my %string_to_close_ = ();
my %weekly_option_string_to_close_ = ();
my %string_to_type_ = ();
my %string_to_underlying_ = ();
my %string_to_month_ = ();
my %string_to_itm_ = ();
my %string_to_itm_prev_ = ();
my %string_to_step_ = ();
my %string_to_step_prev_ = ();
my %string_to_lotsize_ = ();
my %string_to_date_ = ();
my %string_weekly_option_ = ();

#Augment data in datasource_exchsymbol file
#Backup existing file 
my $unique_tag_ = `date +%N`;
my $datasource_exchsymbol_backup_file_name_ = "/spare/local/tradeinfo/NSE_Files/DataExchFile/datasource_exchsymbol.txt.".$unique_tag_;
system( "cp $datasource_exchsymbol_file_ $datasource_exchsymbol_backup_file_name_" ); 
open( FILE, "<", $datasource_exchsymbol_file_ ) or PrintStacktraceAndDie( "Fatal error - cannot open file $datasource_exchsymbol_file_ for reading\n" );
while( $file_line_ = <FILE> )
{
    chomp( $file_line_ );
    my @fline_tokens_ = split( /[\s]+/,$file_line_ );
    if( @fline_tokens_ == 0 || substr( $fline_tokens_[0],0,1) eq "#" ) #avoid comments and empty line in file
    {
       next;
    }
    $datasource_exchsymbol_map_{ $fline_tokens_[1] } = $fline_tokens_[0];
    my $exchsymbol_offset_ = substr( $fline_tokens_[0], 3);
    if( $exchsymbol_offset_ > $highest_exchsymbol_offset_ )
    {
	$highest_exchsymbol_offset_ = $exchsymbol_offset_;
    }   
}
close( FILE );

#process the bhavcopy file and load data into maps 
#Bhavcopy of F&O is the csv file from exchange website
#Bhavcopy of CDS obtained from ftp site
open( FILE, "<", $bhavcopy_file_ ) or PrintStacktraceAndDie( "Could not open file $bhavcopy_file_ \n" );
open( my $DEFILE, ">>", $datasource_exchsymbol_file_ ) or PrintStacktraceAndDie( "Fatal error - cannot open file $datasource_exchsymbol_file_ for writing\n" );
my $key_="";
my $date_=0;
my $optname_string_;
while( my $bfline_ = <FILE> )
{
    chomp( $bfline_ );
    my @bfline_tokens_ = split(/[\s]*,/, $bfline_ );
    #$bfline_tokens_[4] = $bfline_tokens_[4] * 1 ; 
    if( ( $bfline_tokens_[1] eq "OPTSTK" || $bfline_tokens_[1] eq "OPTIDX" ) && $bfline_tokens_[4] > 0 )
    {
	$optname_string_ = sprintf( "NSE_%s_%s_%d_%.2f", $bfline_tokens_[2], $bfline_tokens_[5], formatDate( $bfline_tokens_[3] ), $bfline_tokens_[4] );	
	checkAndAdd( $optname_string_, $DEFILE );
    }

    #handles CD segment
    #handle FUTIRC and FUTCUR entries
    if( $bfline_tokens_[1] eq "FUTIRC" || $bfline_tokens_[1] eq "FUTCUR" )
	{
	    $date_ = formatDate( $bfline_tokens_[3] );
            if($date_ <  $next_working_date_){
              print "IGNORING CD expiry date[ $date_ ] < nxt Working date[ $next_working_date_ ]\n";
              next;
            }
	    $key_ = $bfline_tokens_[1]."_".$bfline_tokens_[2]."_".$date_;
	    $string_to_close_{ $key_ } = $bfline_tokens_[10]; #settlement price actually
	    $string_to_underlying_{ $key_ } = $bfline_tokens_[2];
	    $string_to_month_{ $key_ } = int($date_/100);
	    $string_to_date_{ $key_ } = $date_;
	    $string_to_lotsize_{ $key_ } = 1;
	    if( $bfline_tokens_[1] eq "FUTIRC" )
	    {
		$string_to_type_{ $key_ } = "IRFFUT";
		$string_to_itm_{ $key_ } = 0;		
	    }
	    else
	    {		
		$string_to_type_{ $key_ } = "CURFUT";
		#supporting fewer strikes than listed 
		$string_to_itm_{ $key_ } = 12;
		$string_to_step_{ $key_ } = 0.25;
	    }
	    AddUnderlyingExpiry( $bfline_tokens_[2], $date_ );
	    
	    next; # no need to process currency further
	}
    elsif($bfline_tokens_[1] eq "OPTIDX" && ($bfline_tokens_[2] eq "BANKNIFTY" || $bfline_tokens_[2] eq "NIFTY" || $bfline_tokens_[2] eq "FINNIFTY"))
    {
        ## Support for Weekly Options
        $date_ = formatDate( $bfline_tokens_[3] );
        $key_ = "IDXOPT_".$bfline_tokens_[2]."_".$date_;
        $string_to_close_{ $key_ } = $bfline_tokens_[ 10 ];
        $string_to_type_{ $key_ } = "IDXOPT";
        $string_to_underlying_{ $key_ } = $bfline_tokens_[2];
        $string_to_month_{ $key_ } = int($date_/100);
        $string_to_date_{ $key_ } = $date_;
        $string_weekly_option_{ $key_ } = 1;
        #for NIFTY and BANKNIFTY we don't get #itm and step values in strike scheme file
        #we take fewer strikes than listed - data is from NSE website
        
        if($bfline_tokens_[2] eq "BANKNIFTY") {
		      $string_to_itm_{ $key_ } = 50; #hft we want 6 each side with 500 as multiple
#$string_to_itm_{ $key_ } = 100; #hft we want 6 each side with 500 as multiple
          $string_to_step_{ $key_ } = 100;
        }
	elsif($bfline_tokens_[2] eq "FINNIFTY") {
                $string_to_itm_{ $key_ } = 10; #hft we want 6 each side with 500 as multiple
          	$string_to_step_{ $key_ } = 100;
        }
        else {
          $string_to_itm_{ $key_ } = 30; #hft we want 6 each side with 500 as multiple
          $string_to_step_{ $key_ } = 50;
        }

        #AddUnderlyingExpiry( $bfline_tokens_[2], $date_ ); #[pjain] : Intentionally commented out. Otherwise Spread Future Contracts would be generated.
    }
    elsif( ( $bfline_tokens_[1] eq "FUTIDX" && $bfline_tokens_[2] ne "NIFTY" && $bfline_tokens_[2] ne "BANKNIFTY" && $bfline_tokens_[2] ne "FINNIFTY" ) || 
	( $bfline_tokens_[1] ne "FUTIDX" && $bfline_tokens_[1] ne "FUTSTK" ) )
    {     
        #due to different file format BhavCopy entries of CDS file will get here. // << not valid anymore
        # ignoring these rows 
        next;
    }
    elsif( $bfline_tokens_[1] eq "FUTIDX" && ( $bfline_tokens_[2] eq "NIFTY" || $bfline_tokens_[2] eq "BANKNIFTY" || $bfline_tokens_[2] eq "FINNIFTY" ) )
    {
		$date_ = formatDate( $bfline_tokens_[3] );
		$key_ = $bfline_tokens_[1]."_".$bfline_tokens_[2]."_".$date_;	
		$string_to_close_{ $key_ } = $bfline_tokens_[ 10 ];
		$string_to_type_{ $key_ } = "IDXFUT";
		$string_to_underlying_{ $key_ } = $bfline_tokens_[2];
		$string_to_month_{ $key_ } = int($date_/100);
		$string_to_date_{ $key_ } = $date_;

        my $weekly_option_key = $bfline_tokens_[1]."_".$bfline_tokens_[2]."_".$string_to_month_{ $key_ };
        $weekly_option_string_to_close_{ $weekly_option_key } = $bfline_tokens_[ 10 ];
		#for NIFTY and BANKNIFTY we don't get #itm and step values in strike scheme file
		#we take fewer strikes than listed - data is from NSE website
		$string_to_itm_{ $key_ } = 6;

    if($bfline_tokens_[2] eq "NIFTY") 
    {
      $string_to_itm_{ $key_ } = 30; 
      $string_to_step_{ $key_ } = 50;
    }elsif($bfline_tokens_[2] eq "FINNIFTY")
    {
      $string_to_itm_{ $key_ } = 10; #hft we want 6 each side with 500 as multiple
      $string_to_step_{ $key_ } = 100;

      open( FILESFINN, "<", $bhavcopy_file_ ) or PrintStacktraceAndDie( "Could not open file $bhavcopy_file_ \n" );
      my $banknifty_entry = 0;
      while( my $bfline_ffentry = <FILESFINN> )
      {
        if (index($bfline_ffentry, "BANKNIFTY") != -1 && index($bfline_ffentry, "FUTIDX") != -1){
          if(( $bfline_ffentry =~ /$bfline_tokens_[3]/ ))
          {
              $banknifty_entry = 1;
          }
        }
      }
        close( FILESFINN );

        if (!$banknifty_entry){
          delete($string_to_underlying_{$key_});
          next
        }
    }
    elsif($bfline_tokens_[2] eq "BANKNIFTY")
    {
      # chainging bnf num items to 16 from 30 earlier in accordance with exchange 
      $string_to_itm_{ $key_ } = 50; #hft we want 6 each side with 500 as multiple
#$string_to_itm_{ $key_ } = 100; #hft we want 6 each side with 500 as multiple
      $string_to_step_{ $key_ } = 100;
    }else 
    {

      if( $bfline_tokens_[ 10 ] > 2000 )
      {
        $string_to_step_{ $key_ } = 100;
      }
      else
      {
          $string_to_step_{ $key_ } = 50;
      }
    }

		AddUnderlyingExpiry( $bfline_tokens_[2], $date_ );
    }
    else
    {
		$date_ = formatDate( $bfline_tokens_[3] );
		my $expiry_month = int($date_/100)%100;
		
		$key_ = $bfline_tokens_[1]."_".$bfline_tokens_[2]."_".$date_;
		$string_to_close_{ $key_ } = $bfline_tokens_[ 10 ];
		$string_to_type_{ $key_ } = "STKFUT";
		$string_to_underlying_{ $key_ } = $bfline_tokens_[2];
		$string_to_month_{ $key_ } = int($date_/100);
		$string_to_date_{ $key_ } = $date_;
		if( exists $underlying_to_step_{ $bfline_tokens_[2] } )
		{
		    $string_to_step_{ $key_ } = $underlying_to_step_{ $bfline_tokens_[2] };
		    $string_to_itm_{ $key_ } = $underlying_to_numitm_{ $bfline_tokens_[2] };
		    
		    # if expiry month is modulo 3, it is only affected by 1 sos file
		    if ( $expiry_month % 3 == 0){
		    	$string_to_step_prev_{ $key_ } = $string_to_step_{ $key_ };
		    	$string_to_itm_prev_{ $key_ } = $string_to_itm_{ $key_ };
		    }else {
		    	$string_to_step_prev_{ $key_ } = $underlying_to_step_prev_{ $bfline_tokens_[2] };
		    	$string_to_itm_prev_{ $key_ } = $underlying_to_numitm_prev_{ $bfline_tokens_[2] };
		    }
		}
		else
		{
			if (($this_month_expiry_date_ > 0) && ($date_ eq  $this_month_expiry_date_)){
				delete($string_to_underlying_{$key_});
				next;
			}
		    PrintStacktraceAndDie(" Fatal error - could not find strike step information for $bfline_tokens_[2] : $key_\n" );
		}
		AddUnderlyingExpiry( $bfline_tokens_[2], $date_ );
    }

    my $underlying_month_str_ = $bfline_tokens_[2]."_".$string_to_month_{ $key_ };

    #if month entry exists in lotsize map then use that ( for real time and going forward )
    #else use default value ( for historical data )
    if( exists $underlyingmonth_lot_map_{ $underlying_month_str_ } )
    {
	$string_to_lotsize_{ $key_ } = $underlyingmonth_lot_map_{ $underlying_month_str_ };
    }
    elsif ( exists $underlying_default_lotsize_map_{ $bfline_tokens_[2] } )
    {
	$string_to_lotsize_{ $key_ } =  $underlying_default_lotsize_map_{ $bfline_tokens_[2] };
    }
    else
    {
    	if (($this_month_expiry_date_ > 0) && ($date_ eq  $this_month_expiry_date_)){
    		delete($string_to_underlying_{$key_});
			next;
		}
		print "$this_month_expiry_date_ : $date_  : $this_month_expiry_date_\n";
		
	PrintStacktraceAndDie( "fatal error - could not find lotsize for $bfline_tokens_[2] : $key_ \n" );
    }
    
}
close( FILE );
close( $DEFILE );

#Write data in nse_contracts file
if( $mode_ eq "A" )
{
    open ( FILE, ">>", $output_file_ ) or PrintStacktraceAndDie( "Fatal error - could not open file $output_file_ for writing\n" );    
}
else
{
    open ( FILE, ">", $output_file_ ) or PrintStacktraceAndDie( "Fatal error - could not open file $output_file_ for writing\n" );
}

printf  FILE "# TYPE\tUNDERLYING\tPREV_CLOSE\tLOT\tMIN_TICK\tEXPIRY\tNUM_ITM\tSTEP_VALUE\tSTEP_VAL_PREV\tNUM_ITM_PREV\n";
printf  FILE "# Index/Stock Futures FO Segment\n";
foreach my $string_( keys %string_to_underlying_ )
{
    if( $string_to_type_{ $string_ } eq "STKFUT" || $string_to_type_{ $string_ } eq "IDXFUT" )
    {
	printf FILE "%s\t%s\t%.2f\t%d\t%.2f\t%d\n", $string_to_type_{ $string_ }, $string_to_underlying_{ $string_ }, $string_to_close_{ $string_ }, $string_to_lotsize_{ $string_ }, 0.05, $string_to_date_{ $string_ } ;		
    }
}
printf FILE "\n# Currency/IRF Futures CDS Segment\n";
foreach my $string_( keys %string_to_underlying_ )
{
    if( $string_to_type_{ $string_ } eq "CURFUT" || $string_to_type_{ $string_ } eq "IRFFUT" )
    {
	printf FILE "%s\t%s\t%.4f\t%d\t%.4f\t%d\n", $string_to_type_{ $string_ }, $string_to_underlying_{ $string_ }, $string_to_close_{ $string_ }, $string_to_lotsize_{ $string_ }, 0.0025, $string_to_date_{ $string_ } ;		
    }
}


printf FILE "\n# Options Contracts\n";
my %is_option_added_ = (); # [pjain] : This map is reuiqred to prevent same entries to go into NSE Contracts File multiple times.
foreach my $string_( keys %string_to_underlying_ )
{
	if( not( exists $string_to_step_prev_{ $string_ })){
    	$string_to_step_prev_{ $string_ } = $string_to_step_{ $string_ };
    }
    
    if( not( exists $string_to_itm_prev_{ $string_ })){
    	$string_to_itm_prev_{ $string_ } = $string_to_itm_{ $string_ };
    }
	
    if( $string_to_type_{ $string_ } eq "IDXFUT" )
    {
        my $option_string_suffix_ = substr $string_, 6;
        my $option_string_ = "IDXOPT".$option_string_suffix_;

        if(! exists $is_option_added_{ $option_string_ }){
            printf FILE "IDXOPT\t%s\t%.2f\t%d\t%.2f\t%d\t%d\t%.2f\t%.2f\t%d\n", $string_to_underlying_{ $string_ }, $string_to_close_{ $string_ }, $string_to_lotsize_{ $string_ }, 0.05, $string_to_date_{ $string_ }, $string_to_itm_{ $string_ }, $string_to_step_{ $string_ },$string_to_step_prev_{ $string_ },$string_to_itm_prev_{ $string_ };
            $is_option_added_{ $option_string_ } = 1;
        }
        
    }
    elsif( $string_to_type_{ $string_ } eq "STKFUT" )
    {
	   printf FILE "STKOPT\t%s\t%.2f\t%d\t%.2f\t%d\t%d\t%.2f\t%.2f\t%d\n", $string_to_underlying_{ $string_ }, $string_to_close_{ $string_ }, $string_to_lotsize_{ $string_ }, 0.05, $string_to_date_{ $string_ }, $string_to_itm_{ $string_ }, $string_to_step_{ $string_ }, $string_to_step_prev_{ $string_ },$string_to_itm_prev_{ $string_ };
    }
    elsif( $string_to_type_{ $string_ } eq "CURFUT" )
    {
	   printf FILE "CUROPT\t%s\t%.4f\t%d\t%.4f\t%d\t%d\t%.2f\t%.2f\t%d\n", $string_to_underlying_{ $string_ }, $string_to_close_{ $string_ }, $string_to_lotsize_{ $string_ }, 0.0025, $string_to_date_{ $string_ }, $string_to_itm_{ $string_ }, $string_to_step_{ $string_ },$string_to_step_prev_{ $string_ },$string_to_itm_prev_{ $string_ };	
    }
    elsif($string_to_type_{ $string_ } eq "IDXOPT" && ($string_to_underlying_{ $string_ } eq "BANKNIFTY" || $string_to_underlying_{ $string_ } eq "NIFTY" || $string_to_underlying_{ $string_ } eq "FINNIFTY"))
    {
        #Weekly Options
        #[pjain] : Currently the code is written assuming that weekly options is only supported for BANKNIFTY.
        my $weekly_option_key_current_mon_future = "FUTIDX_".$string_to_underlying_{ $string_ }."_".$string_to_month_{ $string_ };
        if(exists $weekly_option_string_to_close_{ $weekly_option_key_current_mon_future })
        {
            $string_to_close_{ $string_ } = $weekly_option_string_to_close_{ $weekly_option_key_current_mon_future };
            if(! exists $is_option_added_{ $string_ })
            {
                $is_option_added_{ $string_ } = 1;
                printf FILE "IDXOPT\t%s\t%.2f\t%d\t%.2f\t%d\t%d\t%.2f\t%.2f\t%d\n", $string_to_underlying_{ $string_ }, $string_to_close_{ $string_ }, $string_to_lotsize_{ $string_ }, 0.05, $string_to_date_{ $string_ }, $string_to_itm_{ $string_ }, $string_to_step_{ $string_ },$string_to_step_prev_{ $string_ },$string_to_itm_prev_{ $string_ };    
            }
            
        }
        else
        {
            print "Weekly Options : Skipping Contract : ",$string_," in ",$output_file_ ,".\n";
            next;
        }
        
    }
}
close( FILE );

open( $DEFILE, ">>", $datasource_exchsymbol_file_ ) or PrintStacktraceAndDie( "Fatal error - cannot open file $datasource_exchsymbol_file_ for writing\n" );
my $futname_string_;
my $added_entry_string_;
my $option_strike_;
my $atm_strike_;

foreach my $string_( keys %string_to_underlying_ )
{
    if( not( exists $string_weekly_option_{ $string_ })){ #This check required for Weekly Options
        #print "Not Skipping Futures\n";
        #add future contract
        $futname_string_ = "NSE_".$string_to_underlying_{ $string_ }."_FUT_".$string_to_date_{ $string_ };
        checkAndAdd( $futname_string_, $DEFILE );
        
        #add spread contract - note multiple calls are safe
        {
           my @this_expiries_ = @{ $underlying_to_expiries_{ $string_to_underlying_{ $string_ } } };
           my @sorted_expiries_ = sort @this_expiries_;
           my $num_expiries_ = scalar @sorted_expiries_;
           if( $num_expiries_ >= 2 )
           {
               my $futname_spread_string_ = "NSE_".$string_to_underlying_{ $string_ }."_FUT_".$sorted_expiries_[0]."_".$sorted_expiries_[1];
               checkAndAdd( $futname_spread_string_, $DEFILE );
           }
        }

        if( $string_to_type_{ $string_ } eq "IRFFUT" )
        {
          next;
        }
    }
    else
    {
        #[pjain] : Currently the code is written assuming that weekly options is only supported for BANKNIFTY.
        my $weekly_option_key_current_mon_future = "FUTIDX_".$string_to_underlying_{ $string_ }."_".$string_to_month_{ $string_ };
        #print $weekly_option_key_current_mon_future, "\n";
        if(exists $weekly_option_string_to_close_{ $weekly_option_key_current_mon_future })
        {
            $string_to_close_{ $string_ } = $weekly_option_string_to_close_{ $weekly_option_key_current_mon_future };
            #print $string_," ",$string_to_close_{ $string_ },"\n";
        }
        else
        {
            print "Weekly Options : Skipping Contract : ",$string_," in ",$datasource_exchsymbol_file_,".\n";
            next;
        }
        
    }
    
    if( not( exists $string_to_step_prev_{ $string_ })){
    	$string_to_step_prev_{ $string_ } = $string_to_step_{ $string_ };
    }
    
    # since we have two steps now, we have to atm strikes
    my $atm_strike_1_ = round( $string_to_close_{ $string_ }/$string_to_step_{ $string_ } ) * $string_to_step_{ $string_ };
    my $atm_strike_2_ = round( $string_to_close_{ $string_ }/$string_to_step_prev_{ $string_ } ) * $string_to_step_prev_{ $string_ };
    
    #ATM CE for that expiry + underlying
    $optname_string_ = sprintf( "NSE_%s_CE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $atm_strike_1_ );
    checkAndAdd( $optname_string_, $DEFILE );
    
    #ATM PE for that expiry + underlying
    $optname_string_ = sprintf( "NSE_%s_PE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $atm_strike_1_ );
    checkAndAdd( $optname_string_, $DEFILE );
    
    #ATM CE for that expiry + underlying
    $optname_string_ = sprintf( "NSE_%s_CE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $atm_strike_2_ );
    checkAndAdd( $optname_string_, $DEFILE );
    
    #ATM PE for that expiry + underlying
    $optname_string_ = sprintf( "NSE_%s_PE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $atm_strike_2_ );
    checkAndAdd( $optname_string_, $DEFILE );
    
#    print "ATM : $string_ $atm_strike_ ATM1: $atm_strike_1_ ATM2: $atm_strike_2_ $string_to_step_{ $string_ } : $string_to_step_prev_{ $string_ }\n";
    
    #ITM PUT and OTM calls
    # For Current Schema
    for( my $i = 0; $i < ($string_to_itm_{ $string_ } + 2) ; $i++ )
    {
    	my $option_strike_1_ = $atm_strike_1_ + $string_to_step_{ $string_ }*( $i + 1 );
#    	print "ATM : $string_ $atm_strike_ ATM1: $atm_strike_1_ ATM2: $atm_strike_2_ $string_to_step_{ $string_ } : $string_to_step_prev_{ $string_ } OPT : $option_strike_ OPT1: $option_strike_1_ OPT2: $option_strike_2_\n";
    
        if( $option_strike_1_ > 0 ) {
	   $optname_string_ = sprintf( "NSE_%s_CE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $option_strike_1_ );
	   checkAndAdd( $optname_string_, $DEFILE );
	   
	   $optname_string_ = sprintf( "NSE_%s_PE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $option_strike_1_ );
	   checkAndAdd( $optname_string_, $DEFILE );
        }
    }
    # For Prev Schema
    for( my $i = 0; $i < $string_to_itm_prev_{ $string_ } ; $i++ )
    {
    	my $option_strike_2_ = $atm_strike_2_ + $string_to_step_prev_{ $string_ }*( $i + 1 );
#    	print "ATM : $string_ $atm_strike_ ATM1: $atm_strike_1_ ATM2: $atm_strike_2_ $string_to_step_{ $string_ } : $string_to_step_prev_{ $string_ } OPT : $option_strike_ OPT1: $option_strike_1_ OPT2: $option_strike_2_\n";
    	if( $option_strike_2_ > 0 ) {
	   $optname_string_ = sprintf( "NSE_%s_CE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $option_strike_2_ );
	   checkAndAdd( $optname_string_, $DEFILE );
	   
	   $optname_string_ = sprintf( "NSE_%s_PE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $option_strike_2_ );
	   checkAndAdd( $optname_string_, $DEFILE );
	}
    }
    
#    print "ATM : $string_ $atm_strike_ ATM1: $atm_strike_1_ ATM2: $atm_strike_2_ $string_to_step_{ $string_ } : $string_to_step_prev_{ $string_ }\n";
    
    #ITM CALLS and OTM puts
    # For Current Schema
    for( my $i = 0; $i < ($string_to_itm_{ $string_ } + 2) ; $i++ )
    {
    	my $option_strike_1_ = $atm_strike_1_ - $string_to_step_{ $string_ }*( $i + 1 );
#    	print "ATM : $string_ $atm_strike_ ATM1: $atm_strike_1_ ATM2: $atm_strike_2_ $string_to_step_{ $string_ } : $string_to_step_prev_{ $string_ } OPT : $option_strike_ OPT1: $option_strike_1_ OPT2: $option_strike_2_\n";
        if( $option_strike_1_ > 0 ) {	
	   $optname_string_ = sprintf( "NSE_%s_CE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $option_strike_1_ );
	   checkAndAdd( $optname_string_, $DEFILE );
	   
	   $optname_string_ = sprintf( "NSE_%s_PE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $option_strike_1_ );
	   checkAndAdd( $optname_string_, $DEFILE );
	}
    }
    # For Prev Schema
    for( my $i = 0; $i < $string_to_itm_prev_{ $string_ } ; $i++ )
    {
    	my $option_strike_2_ = $atm_strike_2_ - $string_to_step_prev_{ $string_ }*( $i + 1 );
#    	print "ATM : $string_ $atm_strike_ ATM1: $atm_strike_1_ ATM2: $atm_strike_2_ $string_to_step_{ $string_ } : $string_to_step_prev_{ $string_ } OPT : $option_strike_ OPT1: $option_strike_1_ OPT2: $option_strike_2_\n";
    	if( $option_strike_2_ > 0 ) {
	   $optname_string_ = sprintf( "NSE_%s_CE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $option_strike_2_ );
	   checkAndAdd( $optname_string_, $DEFILE );
	   
	   $optname_string_ = sprintf( "NSE_%s_PE_%d_%.2f", $string_to_underlying_{ $string_ }, $string_to_date_{ $string_ }, $option_strike_2_ );
	   checkAndAdd( $optname_string_, $DEFILE );
	}
    }		
}
close( $DEFILE );
