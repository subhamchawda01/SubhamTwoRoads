#!/usr/bin/perl

# \file GenPerlLib/make_indicator_lists.pl
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


require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_number_from_vec.pl"; # FindNumberFromVec
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

#require $user_
#        $base_px_1_
#        $base_px_2_
#        $pred_algo_
#        $filter_
#        $ilist_prefix_
#        $shortcode_
#        $timeperiod_
#        $concise_timeperiod_
#        $start_date_
#        $end_date_
#        $corr_cutoff_
#        $measure_
#        $num_indicators_       
#        @pred_durations_
#        $exclude_filename_
#

sub GeneratePrimaryIlistsUsingCorrelationStatistics
{
    my $make_ilist_exec_ = $LIVE_BIN_DIR."/make_indicator_list";

    foreach my $t_pred_duration_ ( @pred_durations_ )
    { # FESX_0_288_fv_na_e3_US_MORN_DAY_OfflineMixMMS_OfflineMixMMS
	my $indicator_corr_filename_ = "";

	if ( $filter_ )
	{
	    $indicator_corr_filename_ = "/home/".$user_."/indicatorwork/".$shortcode_."_".$t_pred_duration_."_".$filter_."_".$pred_algo_."_".$timeperiod_."_".$base_px_1_."_".$base_px_2_."/indicator_corr_record_file.txt.gz";
	}
	else
	{
	    $indicator_corr_filename_ = "/home/".$user_."/indicatorwork/".$shortcode_."_".$t_pred_duration_."_".$pred_algo_."_".$timeperiod_."_".$base_px_1_."_".$base_px_2_."/indicator_corr_record_file.txt.gz";
	}

	if ( ! ExistsWithSize ( $indicator_corr_filename_ ) )
	{
	    my $t_indicator_corr_filename_ = "";

	    if ( $filter_ )
	    {
		$t_indicator_corr_filename_ = "/home/".$user_."/indicatorwork/".$shortcode_."_".$t_pred_duration_."_".$filter_."_".$pred_algo_."_".$timeperiod_."_".$base_px_1_."_".$base_px_2_."/indicator_corr_record_file.txt";
	    }
	    else
	    {
		$t_indicator_corr_filename_ = "/home/".$user_."/indicatorwork/".$shortcode_."_".$t_pred_duration_."_".$pred_algo_."_".$timeperiod_."_".$base_px_1_."_".$base_px_2_."/indicator_corr_record_file.txt";
	    }

	    if ( ! ExistsWithSize ( $t_indicator_corr_filename_ ) )
	    {
		PrintStacktraceAndDie ( "Could not find $indicator_corr_filename_ or $t_indicator_corr_filename_" );
	    }

	    $indicator_corr_filename_ = $t_indicator_corr_filename_;
	}

	my $ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_".PriceTypeToString ( $base_px_1_ )."_".PriceTypeToString ( $base_px_2_ )."_".$ilist_prefix_.$t_pred_duration_;

	my $exec_cmd_ = $make_ilist_exec_." ".$shortcode_." ".$indicator_corr_filename_." ".$base_px_1_." ".$base_px_2_." ".$start_date_." ".$end_date_." ".$corr_cutoff_." -a ".$measure_." -e ".$exclude_filename_." 2>/dev/null";

	print $exec_cmd_."\n";
	print "\t >> ".$ilist_file_name_."\n";

	my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

	if ( $#exec_output_ < ( $num_indicators_ + 2 ) )
	{
	    PrintStacktraceAndDie ( "make_indicator_list returned less than minimum indicators" );
	}

	open ( ILIST_FILE , "> $ilist_file_name_ " ) or PrintStacktraceAndDie ( "Could not write file $ilist_file_name_" );

	for ( my $i = 0 ; $i <= 2 ; $i ++ )
	{
	    print ILIST_FILE $exec_output_ [ $i ]."\n";
	}

	for ( my $i = 3 ; $i <= ( $num_indicators_ + 2 ) ; $i ++ )
	{
	    print ILIST_FILE $exec_output_ [ $i ]."\n";
	}

	print ILIST_FILE "INDICATOREND"."\n";

	close ( ILIST_FILE );
    }
}

sub GenerateSecondaryIlistsUsingCorrelationStatistics
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

sub CombineAllDurationsPrimaryIlists
{
    foreach my $t_pred_duration_ ( @pred_durations_ )
    {
	my $mkt_mkt_ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_Mkt_Mkt_".$ilist_prefix_.$t_pred_duration_;
	my @mkt_mkt_ilist_content_ = `cat $mkt_mkt_ilist_file_name_`; chomp ( @mkt_mkt_ilist_content_ );

	foreach my $iline_ ( @mkt_mkt_ilist_content_ )
	{
	    if ( index ( $iline_ , "INDICATOR 1.00" ) >= 0 )
	    {
		my $indicator_name_ = "";
		my $corr_stats_ = "";

		my @iline_words_ = split ( ' ' , $iline_ );
		for ( my $i = 2 ; $i <= $#iline_words_ - 4 ; $i ++ )
		{
		    $indicator_name_ = $indicator_name_.$iline_words_ [ $i ]." ";
		}

		if ( $iline_words_ [ $#iline_words_ - 3 ] ne "#" )
		{
		    PrintStacktraceAndDie ( "$iline_ doesnot contain corrs. statistics" );
		}

		for ( my $i = $#iline_words_ - 2 ; $i <= $#iline_words_ ; $i ++ )
		{
		    $corr_stats_ = $corr_stats_.$iline_words_ [ $i ]." ";
		}

		$indicator_to_pred_duration_to_stats_ { $indicator_name_ } { $t_pred_duration_ } = $corr_stats_;
		$pred_duration_to_indicator_to_stats_ { $t_pred_duration_ } { $indicator_name_ } = $corr_stats_;
	    }
	}
    }

    { # Create J0 type ilists
	my %indicator_to_score_ = ( );
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
	    }
	}

	my $ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_Mkt_Mkt_".$ilist_prefix_."0";
	open ( ILIST_FILE , "> $ilist_file_name_ " ) or PrintStacktraceAndDie ( "Could not write file $ilist_file_name_" );

	print ILIST_FILE "MODELINIT DEPBASE ".$shortcode_." MktSizeWPrice MktSizeWPrice\n";
	print ILIST_FILE "MODELMATH LINEAR CHANGE\n";
	print ILIST_FILE "INDICATORSTART\n";

	my $num_indicators_printed_ = 0;
	foreach my $indicator_ ( sort { $indicator_to_score_ { $b } <=> $indicator_to_score_ { $a } }
				 keys %indicator_to_score_ )
	{
	    print ILIST_FILE "INDICATOR 1.00 ".$indicator_." # ".$indicator_to_score_ { $indicator_ }."\n";

	    $num_indicators_printed_ ++;

	    if ( $num_indicators_printed_ == $num_indicators_ )
	    {
		last;
	    }
	}

	print ILIST_FILE "INDICATOREND"."\n";

	close ( ILIST_FILE );
    }

    if ( $num_ilists_ > 1 )
    { # Create Jl type ilists
	my %indicator_to_score_ = ( );
	foreach my $Jl_pred_duration_ ( keys %Jl_pred_duration_to_weight_ )
	{
	    foreach my $Jl_indicator_ ( keys % { $pred_duration_to_indicator_to_stats_ { $Jl_pred_duration_ } } )
	    {
		my $corr_stats_ = $pred_duration_to_indicator_to_stats_ { $Jl_pred_duration_ } { $Jl_indicator_ };
		my @corr_stats_words_ = split ( ' ' , $corr_stats_ );

		my $correlation_ = $corr_stats_words_ [ 0 ];
		my $mean_correlation_ = $corr_stats_words_ [ 1 ];
		my $stdev_correlation_ = max ( 0.000001 , $corr_stats_words_ [ 2 ] );

		# Sum per the four durations { correlation * ( mean_correlation / stdev_correlation ) * weight_of_duration }
		$indicator_to_score_ { $Jl_indicator_ } += ( $correlation_ * ( $mean_correlation_ / $stdev_correlation_ ) * $Jl_pred_duration_to_weight_ { $Jl_pred_duration_ } );
	    }
	}

	my $ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_Mkt_Mkt_Jl";
	open ( ILIST_FILE , "> $ilist_file_name_ " ) or PrintStacktraceAndDie ( "Could not write file $ilist_file_name_" );

	print ILIST_FILE "MODELINIT DEPBASE ".$shortcode_." MktSizeWPrice MktSizeWPrice\n";
	print ILIST_FILE "MODELMATH LINEAR CHANGE\n";
	print ILIST_FILE "INDICATORSTART\n";

	my $num_indicators_printed_ = 0;
	foreach my $indicator_ ( sort { $indicator_to_score_ { $b } <=> $indicator_to_score_ { $a } }
				 keys %indicator_to_score_ )
	{
	    print ILIST_FILE "INDICATOR 1.00 ".$indicator_." # ".$indicator_to_score_ { $indicator_ }."\n";

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

sub CombineAllDurationsSecondaryIlists
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
	my $mkt_mkt_ilist_file_name_ = "ilist_".$shortcode_."_".$concise_timeperiod_."_Mkt_Mkt_".$ilist_prefix_.$t_pred_duration_;

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

1
