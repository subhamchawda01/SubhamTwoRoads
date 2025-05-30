#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl";

# start 
my $USAGE="$0 tdf";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $tdf_ = $ARGV[0];

my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( 15, "na_e1", $tdf_ );

print "$this_pred_counters_\n";
