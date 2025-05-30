#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr # CalcPrevWorkingDateMult

my $USAGE="$0 date shortcode";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }

my $tradingdate_ = GetIsoDateFromStr ( $ARGV[0] );
my $shortcode_ = $ARGV[1];
chomp ($shortcode_);

my $margin_file_name_ = "/spare/local/marginlogs/margin_trades_file.".$tradingdate_ ;

my $total_pnl_ = 0.0;
my $old_pnl_ = 0.0;

my $targetcol_ = 6;

my $offset_sec_ = -4 * 3600;

my $pnl_tempdir=$HOME_DIR."/pnltemp";

if ( ! -d $pnl_tempdir )
{
    `mkdir -p $pnl_tempdir`;
}

my $margin_file_basename_ = basename ($margin_file_name_); chomp ($margin_file_basename_);
my $temp_file_name_ = $pnl_tempdir."/pnl_".$shortcode_;

my $wrote_data_ = 0;

open MARGINFILEHANDLE, "< $margin_file_name_" or die "$0 could not open margin_file $margin_file_name_\n";
open TEMPFILEHANDLE, "> $temp_file_name_" or die "$0 could not create temporary file\n";

while (my $line_ = <MARGINFILEHANDLE>) {
    chomp ($line_);
    my @words_ = split (' ', $line_);

    if ($#words_ >= 16 && 
	( ( $shortcode_ eq "ALL" ) ||
	  ( $words_[2] eq $shortcode_ ) ) ) 
    {
	$words_[0] += $offset_sec_;

	my $pnl_ = $words_[8];
	if ( $shortcode_ eq "ALL" )
	{
	    $pnl_ = $words_[16] ;
	}

	my $pnl_change_this_line_ = $pnl_ - $old_pnl_;
	$old_pnl_ = $pnl_;
	$total_pnl_ += $pnl_change_this_line_;

	$words_[8] = $total_pnl_;

	print TEMPFILEHANDLE join (' ', @words_)."\n";

	$wrote_data_ = 1;
    }
}

close MARGINFILEHANDLE;
close TEMPFILEHANDLE;

if ($wrote_data_ != 1) {
    print "No trades found for $shortcode_ in $margin_file_name_\n";
    exit (-1);
}

if ( -e $temp_file_name_) {
    open (GP, "|gnuplot -persist ") or die "no gnuplot";
# force buffer to flush after each write
    GP->autoflush(1);

    print GP "set xdata time; \n set timefmt \"\%s\"; set grid \n"; # set terminal X11 size 1080,840 \n set autoscale xy \n show autoscale \n ";
    my $first = 1; # to detect if comman needed

    open (GP, "|gnuplot -persist ") or die "no gnuplot";
# force buffer to flush after each write
    GP->autoflush(1);
    print GP "set xdata time; \n set timefmt \"\%s\"; set grid; \n plot \'$temp_file_name_\' using 1:$targetcol_ with lines title \"$shortcode_\" \n; ";
    close GP;

    `rm -f $temp_file_name_`;
} else {
    print "No trades found for $shortcode_ in $margin_file_name_\n";
}
