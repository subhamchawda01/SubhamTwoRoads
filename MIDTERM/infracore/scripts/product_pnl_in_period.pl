#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use FileHandle;

sub TimeZoneToCountryCodeHHMM ;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";


require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl";
require "$GENPERLLIB_DIR/array_ops.pl";

my $USAGE="$0 shortcode date stime etime lookback <details> <basename=0/shortcode=1>";
if ( $#ARGV < 4 ) { print "$USAGE\n"; exit ( 0 ); }

my $shortcode_ = $ARGV[0];
my $date_ = $ARGV[1];
my $stime_ = $ARGV[2];
my $etime_ = $ARGV[3];
my $lookback_ = $ARGV[4];

my $print_details_ = 0 ;

if ( $#ARGV > 4 )
{
    $print_details_ = $ARGV[5] ;
}

my $using_basename_otherwise_shortcode_ = 0 ;

if ( $#ARGV > 5 )
{
    $using_basename_otherwise_shortcode_ = $ARGV[6] ;
}

if ( index ( $shortcode_, "NKM" ) >= 0 ||  index ( $shortcode_, "JFFCE" ) >=  0 || index ( $shortcode_,  'KFFTI') >=0 || index ( $shortcode_,  'LFR') >=0 || index ( $shortcode_, 'LFZ' ) >= 0 ||
    index ( $shortcode_,"LFI" ) >= 0 || index ( $shortcode_, "LFL" ) >= 0 ) { $using_basename_otherwise_shortcode_ = 1 ; }

my %date_to_pnl_map_ = ();
my %date_to_vol_map_ = () ;

while ( $lookback_ > 0 )
{
    my $sut_ = `$SCRIPTS_DIR/get_unix_time.pl $date_ $stime_` ;
    my $eut_ = `$SCRIPTS_DIR/get_unix_time.pl $date_ $etime_` ;

    chomp ( $sut_ ) ;
    chomp ( $eut_ ) ;

#    print " start_time ".$sut_." end_time ".$eut_."\n" ;

    my $spnl_ = 0 ;
    my $epnl_ = 0 ;

    my $svol_ = 0 ;
    my $evol_ = 0 ;

    my $symbol_ = $shortcode_ ;

    if ( $using_basename_otherwise_shortcode_ != 0 )
    {
      if ( $shortcode_ eq "LFI" || $shortcode_ eq "LFL" || $shortcode_ eq "LFZ" || $shortcode_ eq "LFR" )
      {
        $symbol_ = `$HOME_DIR/infracore_install/bin/get_exchange_symbol $shortcode_"_0" $date_ | tr ' ' '~'` ;
        $symbol_ = substr  ( $symbol_, 0, 3 ) ;
      }
      elsif ( $shortcode_ eq "JFFCE" || $shortcode_ eq "KFFTI" )
      {
        $symbol_ = `$HOME_DIR/infracore_install/bin/get_exchange_symbol $shortcode_"_0" $date_ | tr ' ' '~'` ;
        $symbol_ = substr  ( $symbol_, 0, 5 ) ;
      }
      else
      {
        $symbol_ = `$HOME_DIR/infracore_install/bin/get_exchange_symbol $shortcode_ $date_ | tr ' ' '~'` ;
      }
    }

    my @pnl_ts_ = "invalid" ;

    if ( $using_basename_otherwise_shortcode_ == 0 )
    {
	@pnl_ts_ = ` $SCRIPTS_DIR/get_pnl_TS.pl $shortcode_ $date_` ;
    }
    else
    {
	@pnl_ts_ = ` $SCRIPTS_DIR/get_pnl_TS.pl $symbol_ $date_` ;
    }


    chomp ( @pnl_ts_ ) ;

    foreach my $line_ ( @pnl_ts_ )
    {
	chomp ($line_);
	my @words_ = split (' ', $line_);

	if ( scalar ( @words_ ) != 4 )
	{
	    next ;
	}

	if ( int ( $words_[0] ) <= $sut_ )
	{
	    $spnl_ = $words_[3] ;
	    $svol_ = $words_[2] ;
	}
	if ( int ( $words_[0] ) <= $eut_ )
	{
	    $epnl_ = $words_[3] ;
	    $evol_ = $words_[2] ;
	}
    }

    if ( $evol_ - $svol_ == 0 )
    {
	$date_ = CalcPrevWorkingDateMult ( $date_ , 1 ) ;
	$lookback_-- ;
	next ;
    }

    $date_to_pnl_map_{$date_} = $epnl_ - $spnl_ ;
    $date_to_vol_map_{$date_} = $evol_ - $svol_ ;

    $date_ = CalcPrevWorkingDateMult ( $date_ , 1 ) ;
    $lookback_-- ;
}

my @pseries_ = ( ) ;
my @vseries_ = ( ) ;

foreach my $key ( sort ( keys %date_to_pnl_map_ ) )
{
    if ( $print_details_ == 2 )
    {
	print "$key $date_to_pnl_map_{$key} $date_to_vol_map_{$key} \n" ;
    }
    push ( @pseries_ , $date_to_pnl_map_ { $key } );
    push ( @vseries_ , $date_to_vol_map_ { $key } );
}

my $count_ = scalar ( @pseries_ );

my $pnl_sum_ = GetSum( \@pseries_ ) ;
my $vol_sum_ = GetSum( \@vseries_ ) ;

my $pnl_gpr_ = GetGainToPainRatio( \@pseries_ ) ;

my $pnl_dd_ = GetMaxDrawdown( \@pseries_ ) ;

my $pnl_mean_ = GetAverage( \@pseries_ ) ;
my $vol_mean_ = GetAverage( \@vseries_ ) ;

my $pnl_stdev_ = GetStdev( \@pseries_ ) ;
my $vol_stdev_ = GetStdev( \@vseries_ ) ;

my $pnl_sharpe_ = 0 ;

if ( $count_ > 1 )
{
    $pnl_sharpe_ = $pnl_mean_ / $pnl_stdev_ ;
}
else
{
    $pnl_stdev_ = 0 ;
    $vol_stdev_ = 0 ;
}

if ( $print_details_ == 1 )
{
    print "PNL_SUM \t PNL_GPR \t PNL_DD \t PNL_MEAN \t PNL_STDEV \t VOL_SUM \t VOL_MEAN \t VOL_STDEV\n" ;
    printf "%.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \n", $pnl_sum_ , $pnl_gpr_ , $pnl_dd_ , $pnl_mean_ , $pnl_stdev_ , $vol_sum_ , $vol_mean_ , $vol_stdev_ ;
}

if ( $print_details_ == 0 && $vol_mean_ > 0 )
{
    if ( $pnl_dd_ > 0 )
    {
	printf ( "%s | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f", $shortcode_, $count_, $pnl_sum_, $pnl_mean_, $pnl_stdev_, $pnl_sharpe_ , $vol_mean_, $pnl_dd_, $pnl_mean_/$pnl_dd_, $pnl_gpr_ ) ;
    }
    else
    {
	printf ( "%s | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %s | %.3f", $shortcode_, $count_, $pnl_sum_, $pnl_mean_, $pnl_stdev_, $pnl_sharpe_ , $vol_mean_, $pnl_dd_, "NA", $pnl_gpr_ ) ;
    }
}

sub TimeZoneToCountryCodeHHMM
{
    my $tz_ = shift ;
    my $cc_ = "UTC" ;
    my $hhmm_ = $tz_ ;

    if ( index ( $tz_, "EST_" ) != -1 )
    {
	$cc_ = "NY" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "CST_" ) != -1 )
    {
	$cc_ = "CHI" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "CET_" ) != -1 )
    {
	$cc_ = "FR" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "BRT_" ) != -1 )
    {
	$cc_ = "BRZ" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "HKT_" ) != -1 )
    {
	$cc_ = "HK" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "IST_" ) != -1 )
    {
	$cc_ = "IND" ;
	$hhmm_ = substr $tz_ , 4 ;
    }
    elsif ( index ( $tz_, "JST_" ) != -1 )
    {
	$cc_ = "TOK" ;
	$hhmm_ = substr $tz_, 4 ;
    }
    elsif ( index ( $tz_, "UTC_" ) != -1 )
    {
	$cc_ = "UTC" ;
	$hhmm_ = substr $tz_, 4 ;
    }
    return $cc_, int ( $hhmm_ ) ;
}
