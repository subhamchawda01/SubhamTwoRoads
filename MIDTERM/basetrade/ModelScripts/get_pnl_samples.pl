#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;
use Scalar::Util qw(looks_like_number);

my $USAGE="$0 tradesfilename";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }
my $tradesfilename_ = $ARGV[0];

my $tradesfilebase_ = basename ( $tradesfilename_ ); chomp ($tradesfilebase_);

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

open PNLFILEHANDLE, "< $tradesfilename_" or PrintStacktraceAndDie ( "$0 could not open tradesfile $tradesfilename_\n" );

while ( my $inline_ = <PNLFILEHANDLE> )
{
    chomp ( $inline_ );
    my @pnlwords_ = split ( ' ', $inline_ );
    if ( $pnlwords_[0] eq "PNLSAMPLES"  )
    {
        splice ( @pnlwords_, 0, 1 );
        my $pnlsamples_line_ = join ( ' ', @pnlwords_ );
        print $pnlsamples_line_."\n" ;
    }
}

close PNLFILEHANDLE ;
