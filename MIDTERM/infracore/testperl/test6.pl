#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $MODELSCRIPTS_DIR=$HOME_DIR."/infracore_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

my $TRADELOG_DIR = "/spare/local/logs/tradelogs/" ;
my $tradingdate_ = "20101115" ;
my $unique_gsm_id_ = "072171116" ;

my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);

printf "%s\n", $this_tradesfilename_ ;
