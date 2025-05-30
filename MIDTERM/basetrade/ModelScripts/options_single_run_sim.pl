#!/usr/bin/perl

# \file ModelScripts/add_results_to_global_database.pl
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

my $TRADELOG_DIR="/spare/local/logs/tradelogs/";
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $SCRIPTS_BASE_DIR=$HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/basetrade_install/bin";

require "$GENPERLLIB_DIR/lock_utils.pl"; # TakeLock, RemoveLock
my $USAGE="$0 strat_file_ unique_sim_id_ tradingdate_ resultsfilename_ stratname_ \n";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }

my $strat_file_ = $ARGV[0];
my $unique_sim_id_ = $ARGV[1];
my $tradingdate_  = $ARGV[2];
my $resultsfilename_ = $ARGV[3];
my $stratname_ = $ARGV[4];

my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
my $this_log_filename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".$unique_sim_id_;
my $this_resultline_filename_ = $TRADELOG_DIR."/results.".$tradingdate_.".".$unique_sim_id_;

my $cmd_ = "$LIVE_BIN_DIR/sim_strategy_options SIM $strat_file_ $unique_sim_id_ $tradingdate_ ADD_DBG_CODE -1 2>/dev/null 1>/dev/null ; $MODELSCRIPTS_DIR/get_pnl_stats_options.pl $this_trades_filename_ X > $this_resultline_filename_; rm $this_trades_filename_ $this_log_filename_";

my @output_lines_ =  `$cmd_`;
chomp( @output_lines_ );
print "$_ \n" for @output_lines_;

open RLH, "< $this_resultline_filename_" or PrintStacktraceAndDie ( "Could not open file $this_resultline_filename_ for reading" );
my @result_lines_ = <RLH>;
close RLH;

my @results_ = ();
for  ( my $rl_ = 0; $rl_ < scalar ( @result_lines_ ); $rl_ ++ )
{
	my @resultline_words_ = split ( ' ', $result_lines_[$rl_]);
	my $strat_id_ = splice ( @resultline_words_, 0, 1 );
	my $shc_ = splice ( @resultline_words_, 0, 1 );
        my $suffix_ = "";

#        if(index ($shc_,"_DELTA") != -1)
#        {
#          $suffix_ = "_DELTA";
#        }
#
#        if (index ($shc_,"_VEGA") != -1)
#        {
#          $suffix_ = "_VEGA";
#        } 
#
#        $shc_ =~ s/$suffix_//g;
       
        my $tmp_stratname_ = $stratname_; 
#       if(!($shc_ eq "ALL"))
#       {
	  $tmp_stratname_ = $stratname_."_".$shc_; #-- Need to change this to file based when clubbing introduced
#	}

#        $tmp_stratname_ = $tmp_stratname_.$suffix_;
        my $result_line_ = join(' ', @resultline_words_);
	push ( @results_, $tmp_stratname_." ".$tradingdate_." ".$result_line_ );
}

print localtime()."\n";
my $id=`date +%N`;
chomp($id);
#TakeLock($resultsfilename_);
open ( RESULTSDBFILE, ">> $resultsfilename_.$id" );
print RESULTSDBFILE $_."\n" foreach @results_;
close ( RESULTSDBFILE );
#RemoveLock($resultsfilename_);
`rm $this_resultline_filename_`;
print localtime()."\n";
