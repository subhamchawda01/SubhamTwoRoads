#!/usr/bin/perl

# \file ModelScripts/choose_indicators_from_stats.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes an instructionfilename :
#
# shortcode
# TIME_DURATION = [ EU_MORN_DAY | US_MORN | US_DAY | US_MORN_DAY | EU_MORN_DAY_US_DAY ]
# BASEPX
# FUTPX 
# outfile
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MODELING_INDICATORWORK_DIR=$HOME_DIR."/indicatorwork"; # this directory is used to store the chosen strategy files
my $GLOBALRESULTSDBDIR=$HOME_DIR."/ec2_globalresults"; # Changed for DVCTrader ... so that it does not clash with others

my $hostname_s_ = `hostname -s`; chomp ( $hostname_s_ );
if ( ! ( ( $hostname_s_ eq "sdv-ny4-srv11" ) || 
	 ( $hostname_s_ eq "sdv-crt-srv11" ) ) )
{
    $GLOBALRESULTSDBDIR="/NAS1/ec2_globalresults"; # on non ny4srv11 and crtsrv11 ... look at NAS
}

sub CalcCorrSort ;
sub PrintSortedUniqOutlines ;
sub GetIndicatorString ;
sub GetStrFromPxType ;
sub IsDisallowedBookCombo ;

# start 
my $USAGE="$0 shortcode timeperiod basepx futpx min_abs_ind_corr min_sharpe_corr outfile";

if ( $#ARGV < 6 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $timeperiod_ = $ARGV[1];
my $basepx_ = $ARGV[2];
my $futpx_ = $ARGV[3];
my $min_abs_ind_corr_ = $ARGV[4];
my $min_sharpe_corr_ = $ARGV[5];
my $outfile_ = $ARGV[6];

my $timestr_ = "EST";
given ( $timeperiod_ )
{
    when ( "US_MORN_DAY" )
    {
	$timestr_ = "EST";
    }
    when ( "EU_MORN_DAY" )
    {
	$timestr_ = "CET";
    }
    default
    {
	$timestr_ = "EST";
    }
}
my $basestr_ = GetStrFromPxType ( $basepx_ );
my $futstr_ = GetStrFromPxType ( $futpx_ );

my $search_string_ = "head -q -n8 ".$MODELING_INDICATORWORK_DIR."/".$shortcode_."_\*_na_?3_".$timestr_."\*".$basestr_."_".$futstr_."/sorted_results* | $SCRIPTS_DIR/sort_abs_wzscore.pl 2";

my @search_results_ = `$search_string_`; chomp ( @search_results_ );
my @outlines_ = ();
my %print_jis_corr_sum_ ;
my %print_jis_corr_count_ ;

foreach my $ind_stat_str_ ( @search_results_ )
{
    if ( ! ( IsDisallowedBookCombo ( $ind_stat_str_ ) ) )
    {

    my @txt_words_ = split ( ' ', $ind_stat_str_ );
    if ( $#txt_words_ >= 3 )
    { #INDICATOR corr std text_
	my $ind_corr_ = $txt_words_[1];
	my $abs_ind_corr_ = abs ( $ind_corr_ );
	my $ind_corr_std_ = $txt_words_[2];
	if ( ( ( $abs_ind_corr_ >= $min_abs_ind_corr_ ) &&
	       ( $abs_ind_corr_ >= $min_sharpe_corr_ * $ind_corr_std_ ) ) ||
	     ( ( $abs_ind_corr_ >= ($min_abs_ind_corr_/2.0) ) &&
	       ( $abs_ind_corr_ >= (2.0 * $min_sharpe_corr_ * $ind_corr_std_ ) ) ) # corr thresh reduced to half and sharpe thresh doubled
	    )
	{
	    my $jis_ = GetIndicatorString ( @txt_words_ ) ;
	    my $print_jis_ = "INDICATOR 1.00 ".$jis_;

	    if ( ! exists ( $print_jis_corr_sum_ { $print_jis_ } ) )
	    {
		$print_jis_corr_sum_ { $print_jis_ } = 0;
		$print_jis_corr_count_ { $print_jis_ } = 0;
	    }
	    $print_jis_corr_sum_ { $print_jis_ } += $ind_corr_ ;
	    $print_jis_corr_count_ { $print_jis_ } ++;

	    push ( @outlines_, $print_jis_ );
	}
    }
    }
}

my @uniqoutlines_ = GetUniqueList ( @outlines_ );

my %print_jis_corr_ ;
my @sorted_uniq_outlines_ = CalcCorrSort ();

PrintSortedUniqOutlines ();

exit ( 0 );

################################ SUBS ##############################
sub CalcCorrSort
{
    for ( my $outline_idx_ = 0 ; $outline_idx_ <= $#uniqoutlines_ ; $outline_idx_ ++ ) 
    {
	my $print_jis_ = $uniqoutlines_ [ $outline_idx_ ];
	my $adj_corr_ = 0;
	if ( exists ( $print_jis_corr_sum_ { $print_jis_ } ) )
	{
	    $adj_corr_ = $print_jis_corr_sum_ { $print_jis_ } / $print_jis_corr_count_ { $print_jis_ } ;
	}
	$print_jis_corr_ { $print_jis_ } = $adj_corr_ ;
    }
    my @indicators_sorted_by_corr_ = sort { abs($print_jis_corr_{$b}) <=> abs($print_jis_corr_{$a}) } keys %print_jis_corr_;
    return @indicators_sorted_by_corr_ ;
}

sub PrintSortedUniqOutlines
{
    open ( OUTFILE, "> $outfile_" ) or PrintStacktraceAndDie ( "Could not open file $outfile_ for writing\n" );
    print OUTFILE "MODELINIT DEPBASE $shortcode_ $basestr_ $futstr_\n"; 
    print OUTFILE "MODELMATH LINEAR CHANGE\n";
    print OUTFILE "INDICATORSTART\n";
    for ( my $outline_idx_ = 0 ; $outline_idx_ <= $#sorted_uniq_outlines_ ; $outline_idx_ ++ ) 
    {
	my $print_jis_ = $sorted_uniq_outlines_ [ $outline_idx_ ];
	my $adj_corr_ = 0;
	if ( exists ( $print_jis_corr_sum_ { $print_jis_ } ) )
	{
	    $adj_corr_ = $print_jis_corr_sum_ { $print_jis_ } / $print_jis_corr_count_ { $print_jis_ } ;
	}
	printf OUTFILE "%s # %.2f\n", $print_jis_, $adj_corr_ ;
    }
    print OUTFILE "INDICATOREND\n";
    close OUTFILE;
}

sub GetIndicatorString
{
    my @txt_words_ = @_;
    my $retval_ = "";
    if ( $#txt_words_ >= 3 )
    {
	for ( my $i = 3; $i <= $#txt_words_ ; $i ++ )
	{
	    $retval_ = $retval_.$txt_words_[$i]." ";
	}
    }
    return $retval_;
}

sub GetStrFromPxType 
{
    my $pxtype_ = shift;
    my $str_ = "MktSizeWPrice";
    given ( $pxtype_ )
    {
	when ( "mkt" )
	{
	    $str_ = "MktSizeWPrice";
	}
	when ( "mid" )
	{
	    $str_ = "Midprice";
	}
	when ( "msin" )
	{
	    $str_ = "MktSinusoidal";
	}
	default
	{
	    $str_ = "MktSizeWPrice";
	}
    }
    return $str_ ;
}

sub IsDisallowedBookCombo
{
    my $ind_stat_str_ = shift;
    if ( 
#       ( $ind_stat_str_ =~ /BidAskToPayCombo/ ) ||
	( $ind_stat_str_ =~ /BookOrderCDiffCombo/ ) 
	|| ( $ind_stat_str_ =~ /BookSizeDiffCombo/ ) 
#       || ( $ind_stat_str_ =~ /DiffPairPriceTypeCombo/ )
	|| ( $ind_stat_str_ =~ /MultMidOrderPriceCombo/ ) 
	|| ( $ind_stat_str_ =~ /MultMktComplexPriceTopOffCombo/ ) 
	|| ( $ind_stat_str_ =~ /MultMktOrderPriceCombo/ ) 
	|| ( $ind_stat_str_ =~ /MultMktOrderPriceTopOffCombo/ ) 
	|| ( $ind_stat_str_ =~ /MultMktPriceCombo/ ) 
	)
    {
	return 1;
    }
    return 0;
}
