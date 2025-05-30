#!/usr/bin/perl
# \file scripts/get_periodwise_pnls.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $SUMMARIZE_GLOBALRESULTS_EXEC=$BIN_DIR."/summarize_strategy_results";
my $PORTFOLIO_OPTIMIZATION_SCRIPT=$SCRIPTS_DIR."/portfolio_optimization.R";

my $USAGE="$0 shortcode strats_dir results_dir start_date end_date result_pnl_file result_strat_file";

if ($#ARGV < 6) { 
    printf "$USAGE\n"; 
    exit (0); 
}

my $shortcode_ = $ARGV[0];
my $strats_dir = $ARGV[1];
my $results_dir = $ARGV[2];
my $start_date = $ARGV[3];
my $end_date = $ARGV[4];
my $result_pnl_file = $ARGV[5];
my $result_strat_file = $ARGV[6];


`$SUMMARIZE_GLOBALRESULTS_EXEC $shortcode_ $strats_dir $results_dir $start_date $end_date INVALIDFILE kCNAPnlAdjAverage 0 INVALIDFILE 0 > tmp_per_pnl` ;
`awk '{if(\$1=="STRATEGYFILEBASE"){n=1} if(\$1=="STATISTICS"){n=0; print "";} if(n>1){printf "%s ", \$2;} if(n==1){n++;}}' tmp_per_pnl > $result_pnl_file`;
`awk '{if(\$1=="STRATEGYFILEBASE"){print \$2}}' tmp_per_pnl > $result_strat_file`;
`rm tmp_per_pnl`;
my @result_lines = `$PORTFOLIO_OPTIMIZATION_SCRIPT $result_pnl_file` ;
print "@result_lines\n" ;


