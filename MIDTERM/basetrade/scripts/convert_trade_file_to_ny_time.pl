#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use File::Basename ;
use FileHandle;

my $USAGE="$0 tradesfilename";
if ( $#ARGV < 0 ) { print "$USAGE\n"; exit ( 0 ); }
my $tradesfilename_ = $ARGV[0];
my $targetcol = 9;

my $tradesfilebase_ = basename ( $tradesfilename_ ); chomp ($tradesfilebase_);

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
#my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

if ( ExistsWithSize ( $tradesfilename_ ) )
{

open PNLFILEHANDLE, "< $tradesfilename_" or PrintStacktraceAndDie ( "$0 could not open tradesfile $tradesfilename_\n" );

while ( my $inline_ = <PNLFILEHANDLE> )
{
    chomp ( $inline_ );
    my @pnlwords_ = split ( ' ', $inline_, 2 );
    if ( $#pnlwords_ >= 1 )
    {
	my $timestr_ = $pnlwords_[0];
	$pnlwords_[0] = `env TZ=America/New_York $HOME_DIR/$REPO/scripts/unixtime2localstr.pl $timestr_`; chomp ($pnlwords_[0]);
	print STDOUT join ( ' ', @pnlwords_ )."\n";
    }
}
close PNLFILEHANDLE ;

}
