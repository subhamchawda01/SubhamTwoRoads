#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;
use Fcntl qw (:flock);
use File::Path qw(mkpath);

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $SCRIPTNAME="$0";

my $QUEUEBASEDIR=$HOME_DIR."/gis/" ;
my $HOST=`hostname -s` ; chomp ( $HOST ) ;
my $QUEUEFILE=$QUEUEBASEDIR.$HOST ;
my $RUNNINGFILE=$QUEUEBASEDIR."running_now_".$HOST ;
my $PROCESSEDFILE=$QUEUEBASEDIR."processed_".$HOST ;

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/comb_config_files_exist.pl"; # CombConfigFilesExist

if ( $#ARGV >= 0 )
{
    my $this_product_ = $ARGV[0];
    my $comb_config_files_exist_ = CombConfigFilesExist ( $HOME_DIR."/indicatorwork/prod_configs", $this_product_ );
    print STDERR "$comb_config_files_exist_\n";

}
