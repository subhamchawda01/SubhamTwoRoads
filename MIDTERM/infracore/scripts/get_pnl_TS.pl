#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;


my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

my $USAGE="$0 shortcode date";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }

my $shortcode_ = $ARGV[0];
my $date_ = $ARGV[1];
my ( $year, $month, $day ) = BreakDateYYYYMMDD ( $date_ );

my $total_pnl_ = 0.0;
my $old_pnl_ = 0.0;

my $total_vol_ = -1 ;

#my @lq_ids_ = `grep -h $shortcode_ /NAS1/logs/QueryIds/live_trade_ids.$date_`;

my $cmd_ = "for f in `ls /NAS1/logs/QueryTrades/$year/$month/$day/trades.$date_.* 2>/dev/null` ; do if `head -n2  \$f | grep -q -m 1 $shortcode_` ; then echo \$f ;  fi ; done" ;
print "DEBUG_INFO ".$cmd_."\n";

my @trades_files_  = `$cmd_` ;
chomp ( @trades_files_ ) ;

if ( scalar ( @trades_files_ ) < 1 ) 
{
    exit ( 0 ) ;
}

my @trades_ = `grep -h $shortcode_ @trades_files_ 2>/dev/null | sort -k1n ` ;
my %secname_to_pnl_map_ = ( ) ;

foreach my $line_ ( @trades_ ) 
{
    chomp ($line_);
    my @words_ = split (' ', $line_);

    if ( $#words_ >= 15 && ( index ( $words_[2] ,  $shortcode_ ) == 0 ) && #Check if line corresponds to the given shortcode
      ( ( $shortcode_ ne "NK" ) || ( index ( $words_[2] ,  "NKM" ) != 0 ) ) ) #Fix for "NK" matching with "NKM"
    {
	my $numbered_secname_ = $words_[2];
	my $pnl_ = $words_[8];
	if ( ! exists $secname_to_pnl_map_{ $numbered_secname_ } )
	{
	    $secname_to_pnl_map_{$numbered_secname_} = 0 ;
	}
	my $pnl_change_this_line_ = $pnl_ - $secname_to_pnl_map_{$numbered_secname_} ;
	$secname_to_pnl_map_{$numbered_secname_} = $pnl_;
	$total_pnl_ += $pnl_change_this_line_ ;
	$total_vol_ += $words_[4] ;
	$words_[8] = $total_pnl_ ;
	$words_[4] = $total_vol_ ;
	print join (' ', $words_[0], $words_[2], $words_[4], $words_[8])."\n" ;

    }
}

