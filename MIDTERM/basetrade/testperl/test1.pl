#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

#my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_unique_list.pl";
require "$GENPERLLIB_DIR/install_strategy_modelling.pl";

my @list = qw/a b c d d a e b a b d e f/;
my @uniq = GetUniqueList ( @list );



print "@list\n@uniq\n";

InstallStrategyModelling ( "/home/dvctrader/kputta/kp.strat" , "KP" , "EST_830" , "EST_1600" ) ;
InstallStrategyModelling ( "/home/dvctrader/kputta/kp.strat" , "KP" , "EST_830" , "EST_1600" , 1 ) ;

