#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;


my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD


my $USAGE="$0 shortcode date start_time end_time ";
if ( $#ARGV < 3 ) { print "$USAGE\n"; exit ( 0 ); }

my $shortcode_ = `$BIN_DIR/get_exchange_symbol $ARGV[0] $ARGV[1]`;
chomp( $shortcode_ );
my $date_ = $ARGV[1];
my ( $year, $month, $day ) = BreakDateYYYYMMDD ( $date_ );

my $start_time_ = `$SCRIPTS_DIR/get_unix_time.pl $date_ $ARGV[2]` ;
my $end_time_ = `$SCRIPTS_DIR/get_unix_time.pl $date_ $ARGV[3]` ;

#get list of all trades files
my $cmd_ = "for f in `ls /NAS1/logs/QueryTrades/$year/$month/$day/trades.$date_.* 2>/dev/null` ; do if `head -n2  \$f | grep -q -m 1 $shortcode_` ; then echo \$f ;  fi ; done" ;

my @trades_files_  = `$cmd_` ;
chomp ( @trades_files_ ) ;

if ( scalar ( @trades_files_ ) < 1 ) 
{
    exit ( 0 ) ;
}

my $total_uts_ = 0;
#for each file check if it is in the relevant timezone
foreach my $file_ ( @trades_files_ )
{
    chomp( $file_ );
    my $line_ = `head -1 $file_`;
    chomp( $line_ );
    my @tokens_ = split( ' ', $line_ );
    chomp( @tokens_ );
    if ( $tokens_[ 0 ] >= $start_time_ && $tokens_[ 0 ] <= $end_time_ )
    {
	$cmd_ = "sort -k5g $file_ | tail -1 ";        
	my $line_ = `$cmd_`;
        chomp( $line_ );
        my @tokens_= split(' ', $line_ );
        chomp( @tokens_ ); 
	$total_uts_ = $total_uts_ + $tokens_[4];
    }
}

printf ( "%d\t %d\n", $date_, $total_uts_ );
