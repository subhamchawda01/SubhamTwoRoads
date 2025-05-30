#!/usr/bin/perl

# \file scripts/performance_summary.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";


require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr


#my $pnl_by_time_script  =  "$BIN_DIR/get_pnl_for_shortcode_bytime_stats";
my $pnl_by_time_script  =  "$SCRIPTS_DIR/product_pnl_in_period.pl";

my $date_ = "TODAY" ;
if ( scalar ( @ARGV ) == 1 )
{
    $date_ = $ARGV [ 0 ] ;
}

my $yyyymmdd_ = GetIsoDateFromStr ( $date_ );

my @NUM_DAYS_PAST_VEC=();
#push ( @NUM_DAYS_PAST_VEC, 1 );
push ( @NUM_DAYS_PAST_VEC, 5 );
push ( @NUM_DAYS_PAST_VEC, 20 );
push ( @NUM_DAYS_PAST_VEC, 60 );
#push ( @NUM_DAYS_PAST_VEC, 180 );
#push ( @NUM_DAYS_PAST_VEC, 500 );


#add shc like a key to grep from, given all trades files.
my @tmx_shortcodes_ = ( 'CGB', 'CGF', 'CGZ', 'BAX', 'SXF' ) ; #tmx
my @cme_shortcodes_ = ( 'ZT','ZF', 'ZN', 'ZB', 'UB', 'NKD', 'NIY', 'GE' ) ; #cme
my @cfe_shortcodes_ = ( 'VX' ) ; #cfe
my @eurex_shortcodes_ = ( 'FGBS', 'FGBM', 'FGBL', 'FESX', 'FDAX', 'FOAT', 'FBTP', 'FBTS', 'FGBX', 'FSMI', 'FVS' ) ; #eurex
my @bmf_shortcodes_ = ( 'DOL', 'IND', 'WIN', 'DI1', 'WDO' ) ; #bmf
my @liffe_shortcodes_ = ( 'JFFCE_0', 'KFFTI_0', 'LFR_0', 'LFZ_0', 'LFI', 'LFL','YFEBM' ) ; #liffe
my @hkex_shortcodes_ = ( 'HHI', 'HSI', 'MHI', 'MCH' ) ; #hkex
my @ose_shortcodes_ = ( 'NKM_0',  'NKMF_0', 'NK1', 'JGBL', 'TOPIX') ; #ose
my @rts_shortcodes_ = ( 'Si', 'RI', 'GD', 'MX', 'BR', 'ED' ) ; #rts
my @micex_shortcodes_ = ( 'USD000UTSTOM', 'USD000000TOD', 'EUR_RUB__TOD', 'EUR_RUB__TOM' ) ; #micex


my @cme_periods_strings_ =  ( 'UTC_0005 UTC_0555', 'EST_0100 EST_0800', 'EST_0800 EST_1700' ) ;
my %cme_periods_shortname_ =  ( 'UTC_0005 UTC_0555', 'FS', 'EST_0100 EST_0800', 'SS', 'EST_0800 EST_1700', 'TS' ) ;

my @cfe_periods_strings_ =  ( 'EST_300 EST_915', 'EST_915 EST_1700' ) ;
my %cfe_periods_shortname_ =  ( 'EST_300 EST_915', 'SS', 'EST_915 EST_1700', 'TS' ) ;

my @eurex_periods_strings_ = ( 'EST_0100 EST_0800', 'EST_0800 EST_1700' ) ;
my %eurex_periods_shortname_ =  ( 'EST_0100 EST_0800', 'SS', 'EST_0800 EST_1700', 'TS' ) ;

my @tmx_periods_strings_ = ( 'EST_0600 EST_1700' ) ;
my %tmx_periods_shortname_ =  ( 'EST_0600 EST_1700', 'TS' ) ;

my @bmf_periods_strings_ = ( 'EST_0100 EST_0500', 'BRT_0900 BRT_1800' ) ;
my %bmf_periods_shortname_ =  ( 'EST_0100 EST_0500', 'SS', 'BRT_0900 BRT_1800', 'TS' ) ;

my @liffe_periods_strings_ = ( 'EST_0100 EST_0800', 'EST_0800 EST_1700' ) ;
my %liffe_periods_shortname_ =  ( 'EST_0100 EST_0800', 'SS', 'EST_0800 EST_1700', 'TS' ) ;

my @hkex_periods_strings_ = ( 'HKT_0900 HKT_1200', 'HKT_1300 HKT_1630', 'HKT_1700 HKT_2255' ) ;
my %hkex_periods_shortname_ =  (  'HKT_0900 HKT_1200', 'FS', 'HKT_1300 HKT_1630', 'SS', 'HKT_1700 HKT_2255', 'TS' ) ;

my @ose_periods_strings_ = ( 'JST_0900 JST_1500', 'JST_1700 EST_800' , 'EST_800 UTC_1730' ) ;
my %ose_periods_shortname_ =  ( 'JST_0900 JST_1500', 'FS', 'JST_1700 EST_800', 'SS', 'EST_800 UTC_1730', 'TS' ) ;

my @rts_periods_strings_ = ( 'MSK_1000 EST_800', 'EST_800 MSK_2345' ) ;
my %rts_periods_shortname_ =  ( 'MSK_1000 EST_800', 'SS', 'EST_800 MSK_2345', 'TS' ) ;

my @micex_periods_strings_ = ( 'MSK_1000 EST_800', 'EST_800 MSK_2345' ) ;
my %micex_periods_shortname_ =  ( 'MSK_1000 EST_800', 'SS', 'EST_800 MSK_2345', 'TS' ) ;

my $stat_line_ = "" ;

my $tmx_flag_ = 1 ;
my $cme_flag_ = 1 ;
my $cfe_flag_ = 1 ;
my $bmf_flag_ = 1 ;
my $liffe_flag_ = 1 ;
my $eurex_flag_ = 1 ;
my $hkex_flag_ = 1 ;
my $ose_flag_ = 1 ;
my $rts_flag_ = 1 ;
my $micex_flag_ = 1 ;

my %weekly_avg_matrix_ = ( ) ; # ( ( exchange, session ) -> avg )
my %monthly_avg_matrix_ = ( ) ; # ( ( exchange, session ) -> avg )
my %quarterly_avg_matrix_ = ( ); # ( ( exchange, session ) -> avg )
my $print_mat_ = 1 ;

foreach my $num_of_days_ ( @NUM_DAYS_PAST_VEC )
{
#do tmx

    print "\nPERFORMANCE FOR PAST ",$num_of_days_," DAYS\n";

    printf "%-20s%-12s%-13s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
    "TPSTRING","TPSNAME","SHCODE","DCOUNT","PNLSUM","PNLAVG","PNLSTD","PNLSHRP","VOLAVG","MAXDD","AVG/DD","GPR";

    if ( $tmx_flag_ == 1 )
    {
	foreach my $shortcode_ ( @tmx_shortcodes_ )
	{
	    foreach my $tpstring_  ( @tmx_periods_strings_ )
	    {
		#$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY` ;
		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $tpstring_ $num_of_days_ 0` ;

		my @tokens_ = split ( '\|' , $stat_line_ ) ;
		if ( scalar ( @tokens_ ) >= 9 && $tokens_[ 6 ] > 0 )  #avgvolume > 0
		{
		    printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
		    $tpstring_, $tmx_periods_shortname_{ $tpstring_}, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ], $tokens_[ 9 ] ;

		    if ( $num_of_days_ == 5 ) # fill weekly matrix
		    {
			$weekly_avg_matrix_{"TMX"}{$tmx_periods_shortname_{$tpstring_}} += $tokens_[2];
		    }
		    elsif ( $num_of_days_ == 20 ) # fill monthly matrix
		    {
			$monthly_avg_matrix_{"TMX"}{$tmx_periods_shortname_{$tpstring_}} += $tokens_[2];
		    }
                    elsif ( $num_of_days_ == 60 ) # fill quarterly matrix ( 60 days )
                    {
                        $quarterly_avg_matrix_{"TMX"}{$tmx_periods_shortname_{$tpstring_}} += $tokens_[2];
                    }
		}
#		print $stat_line_ ,"\n";
	    }
	}
    }
#do cme
    if ( $cme_flag_ == 1 )
    {
	foreach my $shortcode_ ( @cme_shortcodes_ )
	{
	    foreach my $tpstring_  ( @cme_periods_strings_ )
	    {
		#$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY`  ;
		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $tpstring_ $num_of_days_ 0` ;
		#print $stat_line_."\n";

		my @tokens_ = split ( '\|' , $stat_line_ ) ;
		if ( scalar ( @tokens_ ) >= 9 && $tokens_[ 6 ] > 0 )  #avgvolume > 0
		{
		    printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
		    $tpstring_, $cme_periods_shortname_{ $tpstring_}, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ], $tokens_[ 9 ] ;
		}

		if ( $num_of_days_ == 5 ) # fill weekly matrix
		{
		    $weekly_avg_matrix_{"CME"}{$cme_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
		elsif ( $num_of_days_ == 20 ) # fill monthly matrix
		{
		    $monthly_avg_matrix_{"CME"}{$cme_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
                elsif ( $num_of_days_ == 60 ) # fill quarterly matrix ( 60 days )
                {
                    $quarterly_avg_matrix_{"CME"}{$cme_periods_shortname_{$tpstring_}} += $tokens_[2];
                }
#		print $stat_line_ ,"\n";

	    }
	}
    }
#do cfe
    if ( $cfe_flag_ == 1 )
    {
	foreach my $shortcode_ ( @cfe_shortcodes_ )
	{
	    foreach my $tpstring_  ( @cfe_periods_strings_ )
	    {
		#$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY`  ;
		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $tpstring_ $num_of_days_ 0` ;

		my @tokens_ = split ( '\|' , $stat_line_ ) ;
		if ( scalar ( @tokens_ ) >= 9 && $tokens_[ 6 ] > 0 )  #avgvolume > 0
		{
		    printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
		    $tpstring_, $cfe_periods_shortname_{ $tpstring_}, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ], $tokens_[ 9 ] ;
		}

		if ( $num_of_days_ == 5 ) # fill weekly matrix
		{
		    $weekly_avg_matrix_{"CFE"}{$cfe_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
		elsif ( $num_of_days_ == 20 ) # fill monthly matrix
		{
		    $monthly_avg_matrix_{"CFE"}{$cfe_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
                elsif ( $num_of_days_ == 60 ) # fill quarterly matrix ( 60 days )
                {
                    $quarterly_avg_matrix_{"CFE"}{$cfe_periods_shortname_{$tpstring_}} += $tokens_[2];
                }
#		print $stat_line_ ,"\n";

	    }
	}
    }


#do bmf
    if ( $bmf_flag_ == 1 )
    {
	foreach my $shortcode_ ( @bmf_shortcodes_ )
	{
	    foreach my $tpstring_  ( @bmf_periods_strings_ )
	    {
		#$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY`  ;
		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $tpstring_ $num_of_days_ 0` ;

		my @tokens_ = split ( '\|' , $stat_line_ ) ;
		if ( scalar ( @tokens_ ) >= 9 && $tokens_[ 6 ] > 0 )  #avgvolume > 0
		{
		    printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
		    $tpstring_, $bmf_periods_shortname_{ $tpstring_}, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ], $tokens_[ 9 ] ;
		}

		if ( $num_of_days_ == 5 ) # fill weekly matrix
		{
		    $weekly_avg_matrix_{"BMF"}{$bmf_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
		elsif ( $num_of_days_ == 20 ) # fill monthly matrix
		{
		    $monthly_avg_matrix_{"BMF"}{$bmf_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
                elsif ( $num_of_days_ == 60 ) # fill quarterly matrix 
                {
                    $quarterly_avg_matrix_{"BMF"}{$bmf_periods_shortname_{$tpstring_}} += $tokens_[2];
                }
#		print $stat_line_ ,"\n";

	    }
	}
    }
#do liffe
    if ( $liffe_flag_ == 1 )
    {
	foreach my $shortcode_ ( @liffe_shortcodes_ )
	{
	    foreach my $tpstring_  ( @liffe_periods_strings_ )
	    {
		#$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY`  ;
		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $tpstring_ $num_of_days_ 0` ;

		my @tokens_ = split ( '\|' , $stat_line_ ) ;
		if ( scalar ( @tokens_ ) >= 9 && $tokens_[ 6 ] > 0 )  #avgvolume > 0
		{
		    printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
		    $tpstring_, $liffe_periods_shortname_{ $tpstring_}, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ], $tokens_[ 9 ] ;
		}

		if ( $num_of_days_ == 5 ) # fill weekly matrix
		{
		    $weekly_avg_matrix_{"LIFFE"}{$liffe_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
		elsif ( $num_of_days_ == 20 ) # fill monthly matrix
		{
		    $monthly_avg_matrix_{"LIFFE"}{$liffe_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
                elsif ( $num_of_days_ == 60 ) # fill quarterly matrix 
                {
                    $quarterly_avg_matrix_{"LIFFE"}{$liffe_periods_shortname_{$tpstring_}} += $tokens_[2];
                }
#		print $stat_line_ ,"\n";
	    }
	}
    }
#do eurex
    if ( $eurex_flag_ == 1 )
    {
	foreach my $shortcode_ ( @eurex_shortcodes_ )
	{
	    foreach my $tpstring_  ( @eurex_periods_strings_ )
	    {
		#$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY`  ;
		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $tpstring_ $num_of_days_ 0` ;

		my @tokens_ = split ( '\|' , $stat_line_ ) ;
		if ( scalar ( @tokens_ ) >= 9 && $tokens_[ 6 ] > 0 )  #avgvolume > 0
		{
		    printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
		    $tpstring_, $eurex_periods_shortname_{ $tpstring_}, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ], $tokens_[ 9 ] ;
		}

		if ( $num_of_days_ == 5 ) # fill weekly matrix
		{
		    $weekly_avg_matrix_{"EUREX"}{$eurex_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
		elsif ( $num_of_days_ == 20 ) # fill monthly matrix
		{
		    $monthly_avg_matrix_{"EUREX"}{$eurex_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
                elsif ( $num_of_days_ == 60 ) # fill quarterly matrix 
                {
                    $monthly_avg_matrix_{"EUREX"}{$eurex_periods_shortname_{$tpstring_}} += $tokens_[2];
                }
#		print $stat_line_ ,"\n";
	    }
	}
    }

#do hkex
    if ( $hkex_flag_ == 1 )
    {
	foreach my $shortcode_ ( @hkex_shortcodes_ )
	{
	    foreach my $tpstring_  ( @hkex_periods_strings_ )
	    {
		#$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY`  ;
		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $tpstring_ $num_of_days_ 0` ;

		my @tokens_ = split ( '\|' , $stat_line_ ) ;
		if ( scalar ( @tokens_ ) >= 9 && $tokens_[ 6 ] > 0 )  #avgvolume > 0
		{
		    printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
		    $tpstring_, $hkex_periods_shortname_{ $tpstring_}, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ], $tokens_[ 9 ] ;
		}

		if ( $num_of_days_ == 5 ) # fill weekly matrix
		{
		    $weekly_avg_matrix_{"HKEX"}{$hkex_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
		elsif ( $num_of_days_ == 20 ) # fill monthly matrix
		{
		    $monthly_avg_matrix_{"HKEX"}{$hkex_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
                elsif ( $num_of_days_ == 60 ) # fill quarterly matrix 
                {
                    $quarterly_avg_matrix_{"HKEX"}{$hkex_periods_shortname_{$tpstring_}} += $tokens_[2];
                }

#		print $stat_line_ ,"\n";
	    }
	}
    }
#do ose
    if ( $ose_flag_ == 1 )
    {
	foreach my $shortcode_ ( @ose_shortcodes_ )
	{
	    foreach my $tpstring_  ( @ose_periods_strings_ )
	    {
#		print "$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY\n"  ;
#		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY`  ;
		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $tpstring_ $num_of_days_ 0` ;

		my @tokens_ = split ( '\|' , $stat_line_ ) ;
		if ( scalar ( @tokens_ ) >= 9 && $tokens_[ 6 ] > 0 )  #avgvolume > 0
		{
		    printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
		    $tpstring_, $ose_periods_shortname_{ $tpstring_}, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ], $tokens_[ 9 ] ;
		}

		if ( $num_of_days_ == 5 ) # fill weekly matrix
		{
		    $weekly_avg_matrix_{"OSE"}{$ose_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
		elsif ( $num_of_days_ == 20 ) # fill monthly matrix
		{
		    $monthly_avg_matrix_{"OSE"}{$ose_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
                elsif ( $num_of_days_ == 60 ) # fill quarterly matrix 
                {
                    $quarterly_avg_matrix_{"OSE"}{$ose_periods_shortname_{$tpstring_}} += $tokens_[2];
                }
		#print $stat_line_ ,"\n";
	    }
	}
    }

#do rts
    if ( $rts_flag_ == 1 )
    {
	foreach my $shortcode_ ( @rts_shortcodes_ )
	{
	    foreach my $tpstring_  ( @rts_periods_strings_ )
	    {
#		print "$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY"  ;
#		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY`  ;
		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $tpstring_ $num_of_days_ 0` ;

		my @tokens_ = split ( '\|' , $stat_line_ ) ;
		if ( scalar ( @tokens_ ) >= 9 && $tokens_[ 6 ] > 0 )  #avgvolume > 0
		{
		    printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
		    $tpstring_, $rts_periods_shortname_{ $tpstring_}, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ], $tokens_[ 9 ] ;
		}

		if ( $num_of_days_ == 5 ) # fill weekly matrix
		{
		    $weekly_avg_matrix_{"RTS"}{$rts_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
		elsif ( $num_of_days_ == 20 ) # fill monthly matrix
		{
		    $monthly_avg_matrix_{"RTS"}{$rts_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
                elsif ( $num_of_days_ == 20 ) # fill quarterly matrix
                {
                    $quarterly_avg_matrix_{"RTS"}{$rts_periods_shortname_{$tpstring_}} += $tokens_[2];
                }
		#print $stat_line_ ,"\n";
	    }
	}
    }

#do micex
    if ( $micex_flag_ == 1 )
    {
	foreach my $shortcode_ ( @micex_shortcodes_ )
	{
	    foreach my $tpstring_  ( @micex_periods_strings_ )
	    {
#		print "$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY"  ;
#		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $num_of_days_ $tpstring_ SUMMARY`  ;
		$stat_line_  = `$pnl_by_time_script $shortcode_ $yyyymmdd_ $tpstring_ $num_of_days_ 0` ;

		my @tokens_ = split ( '\|' , $stat_line_ ) ;
		if ( scalar ( @tokens_ ) >= 9 && $tokens_[ 6 ] > 0 )  #avgvolume > 0
		{
		    printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
		    $tpstring_, $micex_periods_shortname_{ $tpstring_}, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ], $tokens_[ 9 ] ;
		}

		if ( $num_of_days_ == 5 ) # fill weekly matrix
		{
		    $weekly_avg_matrix_{"MICEX"}{$micex_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
		elsif ( $num_of_days_ == 20 ) # fill monthly matrix
		{
		    $monthly_avg_matrix_{"MICEX"}{$micex_periods_shortname_{$tpstring_}} += $tokens_[2];
		}
                elsif ( $num_of_days_ == 60 ) # fill quarterly matrix
                {
                    $quarterly_avg_matrix_{"MICEX"}{$micex_periods_shortname_{$tpstring_}} += $tokens_[2];
                }

		#print $stat_line_ ,"\n";
		#print $stat_line_ ,"\n";
	    }
	}
    }

}
#do cme
#do bmf
#do liffe
#do eurex
#do hkex
#do ose
#did


#    printf "%-13s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s",
 #   "$period_[ $idx]_SHCODE","$period_[ $idx]_PNLSUM","$period_[ $idx]_PNLAVG","$period_[ $idx]_PNLSTD","$period_[ $idx]_PNLSHRP","$period_[ $idx]_VOLAVG","$period_[ $idx]_MAXDD","$period_[ $idx]_AVG/DD","$period_[ $idx]_GPR";

#$stat_line_  = `$pnl_by_time_script $product_exchange_ $yyyymmdd_ $num_of_days_ $period_start_string_[ $idx ] $period_end_string_[ $idx ] SUMMARY`  ;

#chomp ( $stat_line_ ) ;

#my @tokens_ = split ( '\|' , $stat_line_ ) ;
#if ( scalar ( @tokens_ ) >= 9 )
#{
#   printf "%-20s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s ",
#   $tpstring_, $tokens_[ 0 ], $tokens_[ 1 ], $tokens_[ 2 ], $tokens_[ 3 ], $tokens_[ 4 ], $tokens_[ 5 ], $tokens_[ 6 ], $tokens_[ 7 ], $tokens_[ 8 ] ;
#}

if ( $print_mat_ == 1 )
{
    print "\n";
    printf "%-12s%-12s%-12s%-12s%-12s%-12s\n", "WKEY", "EXCH", "AS", "EU", "US", "ALL";
    my %per_ss_ ;
    my $all_ = 0;
    for my $exch_ ( "TMX", "CME", "CFE" , "BMF" , "LIFFE" , "EUREX" , "HKEX" , "OSE" , "RTS", "MICEX" )
    {
	my $per_exch_ = 0;
	printf "%-12s%-12s" , "W", $exch_;
	for my $sess_ ( "FS", "SS", "TS" )
	{
	    if ( exists ( $weekly_avg_matrix_{$exch_} {$sess_} ) )
	    {
		printf "%-12s", int ( $weekly_avg_matrix_{$exch_}{$sess_}/5 ) ;
		$per_exch_ += $weekly_avg_matrix_{$exch_}{$sess_}/5 ;
		$per_ss_{$sess_} += $weekly_avg_matrix_{$exch_}{$sess_}/5 ;
		$all_ += $weekly_avg_matrix_{$exch_}{$sess_}/5 ;
	    }
	    else
	    {
		printf "%-12s", 0 ;
	    }
	}
	printf "%-12s", int($per_exch_) ;
	printf "\n";
    }
    printf "%-12s%-12s%-12s%-12s%-12s%-12s\n", "W", "ALL", int($per_ss_{"FS"}) ,int($per_ss_{"SS"}),int($per_ss_{"TS"}), int($all_) ;
    print "\n";

    printf "%-12s%-12s%-12s%-12s%-12s%-12s\n", "MKEY", "EXCH", "AS", "EU", "US", "ALL";
    %per_ss_ = {};
    $all_ = 0;
    for my $exch_ ( "TMX", "CME", "CFE" , "BMF" , "LIFFE" , "EUREX" , "HKEX" , "OSE" , "RTS", "MICEX" )
    {
	my $per_exch_ = 0;
	printf "%-12s%-12s" , "M", $exch_;
	for my $sess_ ( "FS", "SS", "TS" )
	{
	    if ( exists ( $monthly_avg_matrix_{$exch_} {$sess_} ) )
	    {
		printf "%-12s", int($monthly_avg_matrix_{$exch_}{$sess_}/20) ;
		$per_exch_ += $monthly_avg_matrix_{$exch_}{$sess_}/20 ;
		$per_ss_{$sess_} += $monthly_avg_matrix_{$exch_}{$sess_}/20 ;
		$all_ += $monthly_avg_matrix_{$exch_}{$sess_}/20 ;
	    }
	    else
	    {
		printf "%-12s", 0 ;
	    }
	}
	printf "%-12s", int($per_exch_) ;
	printf "\n";
    }
    printf "%-12s%-12s%-12s%-12s%-12s%-12s\n", "M", "ALL", int($per_ss_{"FS"}),int($per_ss_{"SS"}),int($per_ss_{"TS"}), int($all_) ;
    print "\n";

    printf "%-12s%-12s%-12s%-12s%-12s%-12s\n", "QKEY", "EXCH", "AS", "EU", "US", "ALL";
    %per_ss_ = {};
    $all_ = 0;
    for my $exch_ ( "TMX", "CME", "CFE" , "BMF" , "LIFFE" , "EUREX" , "HKEX" , "OSE" , "RTS", "MICEX" )
    {
	my $per_exch_ = 0;
	printf "%-12s%-12s" , "M", $exch_;
	for my $sess_ ( "FS", "SS", "TS" )
	{
	    if ( exists ( $quarterly_avg_matrix_{$exch_} {$sess_} ) )
	    {
		printf "%-12s", int($quarterly_avg_matrix_{$exch_}{$sess_}/60) ;
		$per_exch_ += $quarterly_avg_matrix_{$exch_}{$sess_}/60 ;
		$per_ss_{$sess_} += $quarterly_avg_matrix_{$exch_}{$sess_}/60 ;
		$all_ += $quarterly_avg_matrix_{$exch_}{$sess_}/60 ;
	    }
	    else
	    {
		printf "%-12s", 0 ;
	    }
	}
	printf "%-12s", int($per_exch_) ;
	printf "\n";
    }
    printf "%-12s%-12s%-12s%-12s%-12s%-12s\n", "M", "ALL", int($per_ss_{"FS"}),int($per_ss_{"SS"}),int($per_ss_{"TS"}), int($all_) ;
    print "\n";

}
