#!/usr/bin/perl

# \file ModelScripts/generate_base_px_ilists.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $HOME_DIR = $ENV { 'HOME' };
my $USER = $ENV { 'USER' };
my $REPO = "basetrade";

my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";

sub GeneratePrimaryIlists;
sub GenerateSecondaryIlists;

sub PriceTypeToString;

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_number_from_vec.pl"; # FindNumberFromVec
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "kputta" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $USAGE = "$0 USER BASE-PRICE-1 BASE-PRICE-2 PREDALGO FILTER ILIST_PREFIX SHORTCODE TIMEPERIOD STARTDATE ENDDATE CUTOFFCORR MAKEINDMEASURE NUMINDICATORS DURATION1 DURATION2 DURATION3 ... -e exlucde_ilist";

if ( $#ARGV < 10 )
{
    print ( $USAGE."\n" );
    exit ( 0 );
}

my $user_ = $ARGV [ 0 ];
my $base_px_1_ = $ARGV [ 1 ];
my $base_px_2_ = $ARGV [ 2 ];
my $pred_algo_ = $ARGV [ 3 ];
my $filter_ = $ARGV [ 4 ];
if ( $filter_ ne "f0" &&
     $filter_ ne "fst1" &&
     $filter_ ne "fsg1" &&
     $filter_ ne "fsr.5_3" &&
     $filter_ ne "fv" )
{ # invalid/no-filter
    $filter_ = "";
}

my $ilist_prefix_ = $ARGV [ 5 ];
my $shortcode_ = $ARGV [ 6 ];
my $timeperiod_ = $ARGV [ 7 ];
my $concise_timeperiod_ = "";

if ( index ( $timeperiod_ , "US_MORN_DAY" ) == 0 )
{
    $concise_timeperiod_ = "US";
}
elsif ( index ( $timeperiod_ , "US_MORN" ) == 0 )
{
    $concise_timeperiod_ = "USM";
}
elsif ( index ( $timeperiod_ , "US" ) == 0 )
{
    $concise_timeperiod_ = "US";
}
elsif ( index ( $timeperiod_ , "EU" ) == 0 )
{
    $concise_timeperiod_ = "EU";
}
elsif ( index ( $timeperiod_ , "AS_MORN_VOL" ) == 0 )
{
    $concise_timeperiod_ = "ASMV";
}
elsif ( index ( $timeperiod_ , "AS_MORN_STB" ) == 0 )
{
    $concise_timeperiod_ = "ASMS";
}
elsif ( index ( $timeperiod_ , "AS_DAY_VOL" ) == 0 )
{
    $concise_timeperiod_ = "ASDV";
}
elsif ( index ( $timeperiod_ , "AS_DAY_STB" ) == 0 )
{
    $concise_timeperiod_ = "ASDS";
}
elsif ( index ( $timeperiod_ , "AS_MORN" ) == 0 )
{
    $concise_timeperiod_ = "ASM";
}
elsif ( index ( $timeperiod_ , "AS_DAY" ) == 0 )
{
    $concise_timeperiod_ = "ASD";
}

my $start_date_ = $ARGV [ 8 ];
my $end_date_ = $ARGV [ 9 ];
my $corr_cutoff_ = $ARGV [ 10 ];
my $measure_ = $ARGV [ 11 ];
my $num_indicators_ = $ARGV [ 12 ];

if ( $num_indicators_ < 5 )
{
    print ( "num_indicators_=".$num_indicators_." too low\n" );
    exit ( 0 );
}
my @pred_durations_ = ( );
my $exclude_filename_ = "NA";

for ( my $i = 13 ; $i <= $#ARGV ; $i ++ )
{
    if ( $i < $#ARGV && $ARGV [$i] eq "-e" ) 
    {
	$exclude_filename_ = $ARGV [ ($i + 1) ];
	last;
	    
    }
    elsif ( ! FindNumberFromVec ( $ARGV [ $i ] , @pred_durations_ ) )
    {
	push ( @pred_durations_ , $ARGV [ $i ] );
    }
}

#print ( $exclude_filename_ );
#print ( scalar(@pred_durations_) );
    
if ( $measure_ eq "SHARPE" )
{
    $measure_ = 2;
}
else
{
    $measure_ = 1;
}

GeneratePrimaryIlists ( );

GenerateSecondaryIlists ( );

exit ( 0 );

sub GeneratePrimaryIlists
{
    my $make_ilist_exec_ = $LIVE_BIN_DIR."/make_indicator_list";

    foreach my $t_pred_duration_ ( @pred_durations_ )
    { # FESX_0_288_fv_na_e3_US_MORN_DAY_OfflineMixMMS_OfflineMixMMS
	my $indicator_corr_filename_ = "";

	if ( $filter_ )
	{
	    $indicator_corr_filename_ = "/home/sghosh/indicatorwork/".$shortcode_."_".$t_pred_duration_."_".$filter_."_".$pred_algo_."_".$timeperiod_."_".$base_px_1_."_".$base_px_2_."/indicator_corr_record_file.txt.gz";
	}
	else
	{
	    $indicator_corr_filename_ = "/home/sghosh/indicatorwork/".$shortcode_."_".$t_pred_duration_."_".$pred_algo_."_".$timeperiod_."_".$base_px_1_."_".$base_px_2_."/indicator_corr_record_file.txt.gz";
	}
	
	my $count = 0 ;
	if ( -e $indicator_corr_filename_ ) 
	{
	    $count = `wc -l $indicator_corr_filename_ | awk '{print \$1}'` ;
	    chomp ( $count ) ;	    
	}

	if ( $count == 0  )
	{
	    my $t_indicator_corr_filename_ = "";
	    
	    if ( $filter_ )
	    {
		$t_indicator_corr_filename_ = "/home/sghosh/indicatorwork/".$shortcode_."_".$t_pred_duration_."_".$filter_."_".$pred_algo_."_".$timeperiod_."_".$base_px_1_."_".$base_px_2_."/indicator_corr_record_file.txt";
	    }
	    else
	    {
		$t_indicator_corr_filename_ = "/home/sghosh/indicatorwork/".$shortcode_."_".$t_pred_duration_."_".$pred_algo_."_".$timeperiod_."_".$base_px_1_."_".$base_px_2_."/indicator_corr_record_file.txt";
	    }

	    if ( -e $t_indicator_corr_filename_ ) 
	    {
		$count = `wc -l $t_indicator_corr_filename_ | awk '{print \$1}'` ;
		chomp ( $count ) ;	    
	    }
	    
	    if ( $count == 0  )
	    {
		PrintStacktraceAndDie ( "Could not find $indicator_corr_filename_ or $t_indicator_corr_filename_" );
	    }
	    
	    $indicator_corr_filename_ = $t_indicator_corr_filename_;
	}
	
	


	my $ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_".PriceTypeToString ( $base_px_1_ )."_".PriceTypeToString ( $base_px_2_ )."_".$ilist_prefix_.$t_pred_duration_;

	my $exec_cmd_;
	
	if( $exclude_filename_ ne "NA")
	{
	    $exec_cmd_ = $make_ilist_exec_." ".$shortcode_." ".$indicator_corr_filename_." ".$base_px_1_." ".$base_px_2_." ".$start_date_." ".$end_date_." ".$corr_cutoff_." -a ".$measure_." -e ".$exclude_filename_." 2>/dev/null";
	}
	else
	{
	    $exec_cmd_ = $make_ilist_exec_." ".$shortcode_." ".$indicator_corr_filename_." ".$base_px_1_." ".$base_px_2_." ".$start_date_." ".$end_date_." ".$corr_cutoff_." -a ".$measure_." 2>/dev/null";
	}

	print $exec_cmd_."\n";
	print "\t >> ".$ilist_file_name_."\n";

	my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

	if ( $#exec_output_ <  0.25 * ( $num_indicators_ ) )
	{
	   PrintStacktraceAndDie ( "make_indicator_list returned only $#exec_output_. The set cutoff is 0.25 * $num_indicators_. exiting\n" );
	    
	}

	if ( $#exec_output_ < ( $num_indicators_ + 2 ) )
	{
	    #PrintStacktraceAndDie ( "make_indicator_list returned less than minimum indicators" );
	    print "Note: make_indicator_list only returned $#exec_output_ indicators. You have asked for $num_indicators_. Will go ahead and print all available in the file \n"
	    
	}


	open ( ILIST_FILE , "> $ilist_file_name_ " ) or PrintStacktraceAndDie ( "Could not write file $ilist_file_name_" );

	for ( my $i = 0 ; $i <= 2 ; $i ++ )
	{
	    print ILIST_FILE $exec_output_ [ $i ]."\n";
	}

	for ( my $i = 3 ; $i <= min ( ( $#exec_output_ - 1 ), ( $num_indicators_ + 2 ) ) ; $i ++ )
	{
	    print ILIST_FILE $exec_output_ [ $i ]."\n";
	}

	print ILIST_FILE "INDICATOREND"."\n";

	close ( ILIST_FILE );
    }
}

sub GenerateSecondaryIlists
{
    my @price_types_list_ = ( );

    if ( ! ( ( $base_px_1_ eq "MidPrice" || $base_px_1_ eq "Midprice" ) && 
	     ( $base_px_2_ eq "MidPrice" || $base_px_2_ eq "Midprice" ) ) )
    { # Mid_Mid
	push ( @price_types_list_ , "MidPrice MidPrice" );
    }

    if ( ! ( ( $base_px_1_ eq "MidPrice" || $base_px_1_ eq "Midprice" ) &&
	     ( $base_px_2_ eq "MktSizeWPrice" ) ) )
    {
	push ( @price_types_list_ , "MidPrice MktSizeWPrice" );
    }

    if ( ! ( ( $base_px_1_ eq "MktSizeWPrice" ) &&
	     ( $base_px_2_ eq "MktSizeWPrice" ) ) )
    {
	push ( @price_types_list_ , "MktSizeWPrice MktSizeWPrice" );
    }

    if ( ! ( ( $base_px_1_ eq "MktSinusoidal" ) &&
	     ( $base_px_2_ eq "MktSinusoidal" ) ) )
    {
	push ( @price_types_list_ , "MktSinusoidal MktSinusoidal" );
    }

    if ( ! ( ( $base_px_1_ eq "OrderWPrice" ) &&
	     ( $base_px_2_ eq "OrderWPrice" ) ) )
    {
	push ( @price_types_list_ , "OrderWPrice OrderWPrice" );
    }

    if ( ! ( ( $base_px_1_ eq "OfflineMixMMS" ) &&
	     ( $base_px_2_ eq "OfflineMixMMS" ) ) )
    {
	push ( @price_types_list_ , "OfflineMixMMS OfflineMixMMS" );
    }

    if ( ! ( ( $base_px_1_ eq "TradeWPrice" ) &&
	     ( $base_px_2_ eq "TradeWPrice" ) ) )
    {
	push ( @price_types_list_ , "TradeWPrice TradeWPrice" );
    }

    foreach my $t_pred_duration_ ( @pred_durations_ )
    {
	my $mkt_mkt_ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_".PriceTypeToString ( $base_px_1_ )."_".PriceTypeToString ( $base_px_2_ )."_".$ilist_prefix_.$t_pred_duration_;
	my @mkt_mkt_ilist_content_ = `cat $mkt_mkt_ilist_file_name_`; chomp ( @mkt_mkt_ilist_content_ );

	foreach my $t_price_type_ ( @price_types_list_ )
	{
	    my @t_price_type_words_ = split ( ' ' , $t_price_type_ );

	    my $t_price_dep_type_ = $t_price_type_words_ [ 0 ];
	    my $t_price_indep_type_ = $t_price_type_words_ [ 1 ];

	    my $t_price_dep_string_ = PriceTypeToString ( $t_price_dep_type_ );
	    my $t_price_indep_string_ = PriceTypeToString ( $t_price_indep_type_ );

	    my $ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_".$t_price_dep_string_."_".$t_price_indep_string_."_".$ilist_prefix_.$t_pred_duration_;
	    print "\t >> ".$ilist_file_name_."\n";

	    open ( ILIST_FILE , "> $ilist_file_name_ " ) or PrintStacktraceAndDie ( "Could not write file $ilist_file_name_" );

	    print ILIST_FILE "MODELINIT DEPBASE ".$shortcode_." ".$t_price_dep_type_." ".$t_price_indep_type_."\n";
	    for ( my $i = 1 ; $i <= $#mkt_mkt_ilist_content_ ; $i ++ )
	    {
		print ILIST_FILE $mkt_mkt_ilist_content_ [ $i ]."\n";
	    }

	    close ( ILIST_FILE );
	}
    }
}

sub PriceTypeToString
{
    my ( $price_type_ ) = @_;

    my $price_string_ = "";

    given ( $price_type_ )
    {
	when ( "MidPrice" ) { $price_string_ = "Mid"; }

	when ( "Midprice" ) { $price_string_ = "Mid"; }

	when ( "MktSizeWPrice" ) { $price_string_ = "Mkt"; }

	when ( "MktSinusoidal" ) { $price_string_ = "Sin"; }

	when ( "OrderWPrice" ) { $price_string_ = "Owp"; }

	when ( "OfflineMixMMS" ) { $price_string_ = "OMix"; }

	when ( "TradeWPrice" ) { $price_string_ = "Twp"; }

	default { $price_string_ = ""; }
    }

    return $price_string_;
}
