#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $TRADELOG_DIR = "/spare/local/logs/tradelogs/" ;
my $tradingdate_ = "20101115" ;
my $unique_gsm_id_ = "072171116" ;

my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);

printf "%s\n", $this_tradesfilename_ ;
