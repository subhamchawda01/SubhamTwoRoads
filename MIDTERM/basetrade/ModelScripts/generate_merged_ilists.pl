#!/usr/bin/env perl

# \file ModelScripts/generate_merged_ilists.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
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

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $USAGE = "$0 SHORTCODE BASE_PRICE_TYPE_1 BASE_PRICE_TYPE_2 TIMEPERIOD NUM_ILISTS NUM_INDICATORS ILIST_PREFIX DURATION_1 DURATION_2 DURATION_3 DURATION_4 ...";

if ( $#ARGV < 6 )
{
    print ( $USAGE."\n" );
    exit ( 0 );
}

my $shortcode_ = $ARGV [ 0 ];
my $base_px1_ = $ARGV [ 1 ];
my $base_px2_ = $ARGV [ 2 ];
my $timeperiod_ = $ARGV [ 3 ];
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
elsif ( index ( $timeperiod_ , "AS_MORN_STB" ) == 0 )
{
    $concise_timeperiod_ = "ASMS";
}
elsif ( index ( $timeperiod_ , "AS_MORN_VOL" ) == 0 )
{
    $concise_timeperiod_ = "ASMV";
}
elsif ( index ( $timeperiod_ , "AS_DAY_STB" ) == 0 )
{
    $concise_timeperiod_ = "ASDS";
}
elsif ( index ( $timeperiod_ , "AS_DAY_VOL" ) == 0 )
{
    $concise_timeperiod_ = "ASDV";
}
elsif ( index ( $timeperiod_ , "AS_MORN" ) == 0 )
{
    $concise_timeperiod_ = "ASM";
}
elsif ( index ( $timeperiod_ , "AS_DAY" ) == 0 )
{
    $concise_timeperiod_ = "ASD";
}

my $num_ilists_ = $ARGV [ 4 ];
my $num_indicators_ = $ARGV [ 5 ];

my $ilist_prefix_ = $ARGV [ 6 ];

my @pred_durations_ = ( );
my %indicator_to_pred_duration_to_stats_ = ( );
my %pred_duration_to_indicator_to_stats_ = ( );

my %J0_pred_duration_to_weight_ = ( );
my %Jl_pred_duration_to_weight_ = ( );

my $short_long_cut_ = 200 ;
my $short_term_ = 0 ;

for ( my $i = 7 ; $i <= $#ARGV ; $i ++ )
{
    push ( @pred_durations_ , $ARGV [ $i ] );
}

if ( $num_ilists_ == 2  ) # should be much smarter !
{
    if ( scalar ( @pred_durations_ ) % 2 == 0 )
    {
	$short_term_ = scalar ( @pred_durations_ ) / 2 ;
    }
    else
    {
	my $t = int ( scalar ( @pred_durations_ ) / 2 ) ;
	if ( ( $pred_durations_ [ $t ] - $pred_durations_[ $t - 1 ] ) > ( $pred_durations_[ $t + 1 ] - $pred_durations_[ $t ] ) )
	{
	    $short_term_ = int ( scalar ( @pred_durations_ ) / 2 ) ;
	}
	else
	{
	    $short_term_ = int ( scalar ( @pred_durations_ ) / 2 ) + 1 ;
	}
    }  
}

if ( $num_ilists_ == 2 )
{ # J0 & Jl
    print "creating two merged lists : st_duration ";
    for ( my $i = 0 ; $i < $short_term_ ; $i ++ )
    {
	print $pred_durations_[ $i ]."  ";
	$J0_pred_duration_to_weight_ { $pred_durations_ [ $i ] } = 1.0 / ( $short_term_ );
	print "J_st_pred_duration_to_weight_ {".$pred_durations_ [ $i ]." ".$J0_pred_duration_to_weight_ { $pred_durations_ [ $i ] }."\n";
    }
    
    print "\nlt_durations ";
    
    for ( my $i = $short_term_ ; $i <= $#pred_durations_ ; $i ++ )
    {
	print $pred_durations_[ $i ]." ";
	$Jl_pred_duration_to_weight_ { $pred_durations_ [ $i ] } = 1.0 / ( scalar ( @pred_durations_ ) - $short_term_ );
	print "J_lt_pred_duration_to_weight_ {".$pred_durations_ [ $i ]." ".$Jl_pred_duration_to_weight_ { $pred_durations_ [ $i ] }."\n";
    }
    print "\n" ;
}
else
{ # J0
    for ( my $i = 0 ; $i <= $#pred_durations_ ; $i ++ )
    {
	$J0_pred_duration_to_weight_ { $pred_durations_ [ $i ] } = 1.0 / ( ( $#pred_durations_ + 1 ) );
	print "J0_pred_duration_to_weight_ {".$pred_durations_ [ $i ]." ".$J0_pred_duration_to_weight_ { $pred_durations_ [ $i ] }."\n";
    }
}

GeneratePrimaryIlists ( );

#GenerateSecondaryIlists ( );

exit ( 0 );



sub GeneratePrimaryIlists
{
    foreach my $t_pred_duration_ ( @pred_durations_ )
    {
	my $mkt_mkt_ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_".PriceTypeToString($base_px1_)."_".PriceTypeToString($base_px2_)."_".$ilist_prefix_.$t_pred_duration_;
	my @mkt_mkt_ilist_content_ = `cat $mkt_mkt_ilist_file_name_`; chomp ( @mkt_mkt_ilist_content_ );

	foreach my $iline_ ( @mkt_mkt_ilist_content_ )
	{
	    if ( index ( $iline_ , "INDICATOR 1.00" ) >= 0 )
	    {
		my $indicator_name_ = "";
		my $corr_stats_ = "";

		my @iline_words_ = split ( ' ' , $iline_ );
		my $i = 2 ;
		for (  ; $i <= $#iline_words_ ; $i ++ )
		{
		    if ( $iline_words_ [ $i ] eq "#" )
		    {
			last ;
		    }
		    $indicator_name_ = $indicator_name_.$iline_words_ [ $i ]." ";
		}
		if ( ( $#iline_words_ - $i ) < 2 )
		{
		    PrintStacktraceAndDie ( "$iline_ doesnot contain corrs. statistics" );
		}

		for ( $i = $i + 1 ; $i <= $#iline_words_ ; $i ++ )
		{
		    $corr_stats_ = $corr_stats_.$iline_words_ [ $i ]." ";
		}

		$indicator_to_pred_duration_to_stats_ { $indicator_name_ } { $t_pred_duration_ } = $corr_stats_;
		$pred_duration_to_indicator_to_stats_ { $t_pred_duration_ } { $indicator_name_ } = $corr_stats_;
	    }
	}
    }

    #if ( $num_ilists_ == 1 ) 
    { # Create J0 type ilists
	my %indicator_to_score_ = ( );
	my %indicator_to_mean_correlation_ ;
	foreach my $J0_pred_duration_ ( keys %J0_pred_duration_to_weight_ )
	{
	    foreach my $J0_indicator_ ( keys % { $pred_duration_to_indicator_to_stats_ { $J0_pred_duration_ } } )
	    {
		my $corr_stats_ = $pred_duration_to_indicator_to_stats_ { $J0_pred_duration_ } { $J0_indicator_ };
		my @corr_stats_words_ = split ( ' ' , $corr_stats_ );

		my $correlation_ = $corr_stats_words_ [ 0 ];
		my $mean_correlation_ = $corr_stats_words_ [ 1 ];
		my $stdev_correlation_ = max ( 0.000001 , $corr_stats_words_ [ 2 ] );

		# Sum per the four durations { correlation * ( mean_correlation / stdev_correlation ) * weight_of_duration }
		$indicator_to_score_ { $J0_indicator_ } += ( $correlation_ * ( $mean_correlation_ / $stdev_correlation_ ) * $J0_pred_duration_to_weight_ { $J0_pred_duration_ } );
		if ( ! exists $indicator_to_mean_correlation_ { $J0_indicator_ } )
		  { $indicator_to_mean_correlation_ { $J0_indicator_ } = 0 ; }
		$indicator_to_mean_correlation_ { $J0_indicator_ } += ( $mean_correlation_ * $J0_pred_duration_to_weight_ { $J0_pred_duration_ } ) ;
	    }
	}

	my $ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_".PriceTypeToString($base_px1_)."_".PriceTypeToString($base_px2_)."_".$ilist_prefix_."0";
	if ( $num_ilists_ == 2 ) 
	{
	    $ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_".PriceTypeToString($base_px1_)."_".PriceTypeToString($base_px2_)."_".$ilist_prefix_."_st";
	}
	open ( ILIST_FILE , "> $ilist_file_name_ " ) or PrintStacktraceAndDie ( "Could not write file $ilist_file_name_" );

	print ILIST_FILE "MODELINIT DEPBASE ".$shortcode_." ".$base_px1_." ".$base_px2_."\n";
	print ILIST_FILE "MODELMATH LINEAR CHANGE\n";
	print ILIST_FILE "INDICATORSTART\n";

	my $num_indicators_printed_ = 0;
	foreach my $indicator_ ( sort { $indicator_to_score_ { $b } <=> $indicator_to_score_ { $a } }
				 keys %indicator_to_score_ )
	{
	  if ( exists $indicator_to_mean_correlation_ { $indicator_ } )
	    {
	      print ILIST_FILE "INDICATOR 1.00 ".$indicator_." # ".$indicator_to_score_ { $indicator_ }." ".$indicator_to_mean_correlation_ { $indicator_ }."\n";
	    }
	  else
	    {
	      print ILIST_FILE "INDICATOR 1.00 ".$indicator_." # ".$indicator_to_score_ { $indicator_ }." 0\n";
	    }

	    $num_indicators_printed_ ++;

	    if ( $num_indicators_printed_ == $num_indicators_ )
	    {
		last;
	    }
	}

	print ILIST_FILE "INDICATOREND"."\n";

	close ( ILIST_FILE );
    }
    
    if ( $num_ilists_ == 2 )
    { # Create Jl type ilists
	my %indicator_to_score_ = ( );
	my %indicator_to_mean_correlation_ ;
	foreach my $Jl_pred_duration_ ( keys %Jl_pred_duration_to_weight_ )
	{
	    foreach my $Jl_indicator_ ( keys % { $pred_duration_to_indicator_to_stats_ { $Jl_pred_duration_ } } )
	    {
		my $corr_stats_ = $pred_duration_to_indicator_to_stats_ { $Jl_pred_duration_ } { $Jl_indicator_ };
		my @corr_stats_words_ = split ( ' ' , $corr_stats_ );

		my $correlation_ = $corr_stats_words_ [ 0 ]; # std_penalized_mean
		my $mean_correlation_ = $corr_stats_words_ [ 1 ]; # mean 
		my $stdev_correlation_ = max ( 0.000001 , $corr_stats_words_ [ 2 ] ); # stdev

		# Sum per the four durations { correlation * ( mean_correlation / stdev_correlation ) * weight_of_duration }
		$indicator_to_score_ { $Jl_indicator_ } += ( $correlation_ * ( $mean_correlation_ / $stdev_correlation_ ) * $Jl_pred_duration_to_weight_ { $Jl_pred_duration_ } );
		if ( ! exists $indicator_to_mean_correlation_ { $Jl_indicator_ } )
		  { $indicator_to_mean_correlation_ { $Jl_indicator_ } = 0 ; }
		$indicator_to_mean_correlation_ { $Jl_indicator_ } += ( $mean_correlation_ * $Jl_pred_duration_to_weight_ { $Jl_pred_duration_ } ) ;
	    }
	}

	my $ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_".PriceTypeToString($base_px1_)."_".PriceTypeToString($base_px2_)."_".$ilist_prefix_."_lt";
	open ( ILIST_FILE , "> $ilist_file_name_ " ) or PrintStacktraceAndDie ( "Could not write file $ilist_file_name_" );

	print ILIST_FILE "MODELINIT DEPBASE ".$shortcode_." ".$base_px1_." ".$base_px2_."\n";
	print ILIST_FILE "MODELMATH LINEAR CHANGE\n";
	print ILIST_FILE "INDICATORSTART\n";

	my $num_indicators_printed_ = 0;
	foreach my $indicator_ ( sort { $indicator_to_score_ { $b } <=> $indicator_to_score_ { $a } }
				 keys %indicator_to_score_ )
	{
	  if ( exists $indicator_to_mean_correlation_ { $indicator_ } )
	    {
	      print ILIST_FILE "INDICATOR 1.00 ".$indicator_." # ".$indicator_to_score_ { $indicator_ }." ".$indicator_to_mean_correlation_ { $indicator_ }."\n";
	    }
	  else
	    {
	      print ILIST_FILE "INDICATOR 1.00 ".$indicator_." # ".$indicator_to_score_ { $indicator_ }." 0\n";
	    }

	    $num_indicators_printed_ ++;

	    if ( $num_indicators_printed_ == $num_indicators_ )
	    {
		last;
	    }
	}

	print ILIST_FILE "INDICATOREND"."\n";

	close ( ILIST_FILE );
    }
}

sub GenerateSecondaryIlists
{
    my @price_types_list_ = ( );

    push ( @price_types_list_ , "MidPrice MidPrice" );
    push ( @price_types_list_ , "MidPrice MktSizeWPrice" );
    push ( @price_types_list_ , "MktSinusoidal MktSinusoidal" );
    push ( @price_types_list_ , "OrderWPrice OrderWPrice" );
    push ( @price_types_list_ , "OfflineMixMMS OfflineMixMMS" );

    my @agg_pred_duration_ = ( "0" , "l" );

    if ( $num_ilists_ == 1 ) { @agg_pred_duration_ = ( "0" ); }

    foreach my $t_pred_duration_ ( @agg_pred_duration_ )
    {
	my $mkt_mkt_ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_".$base_px1_."_".$base_px2_."_".$ilist_prefix_.$t_pred_duration_;

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

	default { $price_string_ = ""; }

    }

    return $price_string_;
}
