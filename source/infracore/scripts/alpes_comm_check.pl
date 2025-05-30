#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $USAGE="$0 S_YYYYMMDD E_YYYYMMDD";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $start_date_ = $ARGV[0];
my $end_date_ = $ARGV[1];

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; #

#ST4
my %alpes_flds_ = ( ) ;

$alpes_flds_{"ASSET"} = 0;
$alpes_flds_{"MARKET"} = 1;
$alpes_flds_{"MATURITY"} = 2;
$alpes_flds_{"ISIN_CODE"} = 3;
$alpes_flds_{"SIDE"} = 4;
$alpes_flds_{"QUANTITY"} = 5;
$alpes_flds_{"SETTLEMENT_DATE"} = 6;
$alpes_flds_{"CLIENT_NAME"} = 7;
$alpes_flds_{"CLIENTACCOUNT"} = 8;
$alpes_flds_{"TRADE_ID"} = 9;
$alpes_flds_{"SETTLEMENT_PRICE"} = 10;
$alpes_flds_{"FX_RATE"} = 11;
$alpes_flds_{"PRODUCT_CURRENCY"} = 12;
$alpes_flds_{"DESIGNATION"} = 13;
$alpes_flds_{"EXCHANGE_FEES"} = 14;
$alpes_flds_{"EXCHANGE_REGISTER_FEES"} = 15;
$alpes_flds_{"BROKERFEES"} = 16;
$alpes_flds_{"PEL"} = 17;

#ASSET DOL
#MARKET FUT
#MATURITY U14
#ISIN_CODE BRBMEFDOL454
#SIDE S
#QUANTITY 5
#SETTLEMENT_DATE 20140822
#CLIENT_NAME DV CAPITAL LLC
#CLIENTACCOUNT 4505
#TRADE_ID 4000249
#SETTLEMENT_PRICE 2277
#FX_RATE
#PRODUCT_CURRENCY BRL
#DESIGNATION DT
#EXCHANGE_FEES 2.70
#EXCHANGE_REGISTER_FEES 3.58
#BROKERFEES 0.25
#PEL -1841.03

#grep DOL /NAS1/data/MFGlobalTrades/EODPnl/ors_pnls_20140821.txt  | awk '{ print $14 }'
#awk -F";" '{ if ( int ( $6 )  > 0 ) { commission[$1]+=int ( ($15+$16+$17) * 1000 )/1000; volume[$1]+=$6}  }END { for ( i in volume ) print "Z "i" "volume[i]" "commission[i]/volume[i]  } ' InvoiceDetailed_ALL_20140822.csv | sort | uniq
#"/NAS1/data/MFGlobalTrades/MFGFiles/InvoiceDetailed_ALL_YYYYMMDD.csv"
my $current_date_ = $end_date_ ;
while ( $current_date_ >= $start_date_ )
{
    my $alpes_filename_ = "/NAS1/data/MFGlobalTrades/MFGFiles/InvoiceDetailed_ALL_".$current_date_.".csv";
    if ( -e $alpes_filename_ )
    {
	open ALPES_FILE_HANDLE, "< $alpes_filename_" or die "could not open alpes_filename_ $alpes_filename_\n";
	my @alpes_file_lines_ = <ALPES_FILE_HANDLE>; chomp ( @alpes_file_lines_ );
	close ALPES_FILE_HANDLE;
	my %shortcode_to_volume_ = ();
	my %shortcode_to_avg_commision_ = ();
	my $verified_header_ = 0 ;

	for ( my $i = 0; $i <= $#alpes_file_lines_; $i++ )
	{
	    my @fields_ = split ( ";", $alpes_file_lines_[$i]);

	    if ( $verified_header_ == 0 )
	    {
		if ( $fields_[$alpes_flds_{"ASSET"}] eq "ASSET" &&
		     $fields_[$alpes_flds_{"MARKET"}] eq "MARKET" &&
		     $fields_[$alpes_flds_{"MATURITY"}] eq "MATURITY" &&
		     $fields_[$alpes_flds_{"ISIN_CODE"}] eq "ISIN_CODE" &&
		     $fields_[$alpes_flds_{"SIDE"}] eq "SIDE" &&
		     $fields_[$alpes_flds_{"QUANTITY"}] eq "QUANTITY" &&
		     $fields_[$alpes_flds_{"SETTLEMENT_DATE"}] eq "SETTLEMENT_DATE" &&
		     $fields_[$alpes_flds_{"CLIENT_NAME"}] eq "CLIENT_NAME" &&
		     $fields_[$alpes_flds_{"CLIENTACCOUNT"}] eq "CLIENTACCOUNT" &&
		     $fields_[$alpes_flds_{"TRADE_ID"}] eq "TRADE_ID" &&
		     $fields_[$alpes_flds_{"SETTLEMENT_PRICE"}] eq "SETTLEMENT_PRICE" &&
		     $fields_[$alpes_flds_{"FX_RATE"}] eq "FX_RATE" &&
		     $fields_[$alpes_flds_{"PRODUCT_CURRENCY"}] eq "PRODUCT_CURRENCY" &&
		     $fields_[$alpes_flds_{"DESIGNATION"}] eq "DESIGNATION" &&
		     $fields_[$alpes_flds_{"EXCHANGE_FEES"}] eq "EXCHANGE_FEES" &&
		     $fields_[$alpes_flds_{"EXCHANGE_REGISTER_FEES"}] eq "EXCHANGE_REGISTER_FEES" &&
		     $fields_[$alpes_flds_{"BROKERFEES"}] eq "BROKERFEES" &&
		     $fields_[$alpes_flds_{"PEL"}] eq "PEL" )
		{
		    $verified_header_ = 1 ;
		}
		else
		{
		    print $fields_[$alpes_flds_{"ASSET"}] ;
		    print "headers not in expected order\n";
		    exit ( 0 ) ;
		}
	    }
	    else
	    {
		$shortcode_to_volume_{$fields_[$alpes_flds_{"ASSET"}]}+=$fields_[$alpes_flds_{"QUANTITY"}];
		$shortcode_to_avg_commision_{$fields_[$alpes_flds_{"ASSET"}]}+= ( $fields_[$alpes_flds_{"EXCHANGE_FEES"}] +
										  $fields_[$alpes_flds_{"EXCHANGE_REGISTER_FEES"}] +
										  $fields_[$alpes_flds_{"BROKERFEES"}] ) ;
	    }
	}
	print $current_date_."\t";
	foreach my $key ( keys %shortcode_to_volume_ )
	{
	    print "( ".$key." ".$shortcode_to_volume_{$key}." ".(int($shortcode_to_avg_commision_{$key}/$shortcode_to_volume_{$key}*1000)/1000).")\t";
	}
	print "\n";
    }
    $current_date_ = CalcPrevBusinessDay ( $current_date_ ) ; #
}
