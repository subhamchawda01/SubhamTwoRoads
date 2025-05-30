#!/usr/bin/perl
#
# \file scripts/rank_ilists.pl
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
use Data::Dumper;

use POSIX qw/strftime/;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_prune_strats_config.pl"; # GetPruneStratsConfig

my $USAGE="$0 SHC TIME_PERIOD [RANKING_METHOD]";
if ( $#ARGV < 1){ print $USAGE."\n"; exit ( 0 ); }

my $shc_ = $ARGV[0];
my $time_period_ = $ARGV[1];
my $rank_method_ = 2;
if($#ARGV>1)
{
	$rank_method_ = $ARGV[2];
}
my $bias_ = 1;

my @strats_ = ();
if($rank_method_==1)
{
	@strats_ =`~/basetrade/scripts/rank_hist_queries.pl $time_period_ $shc_ TODAY-30 TODAY-1 | awk '{print \$3}'`;
}
elsif ($rank_method_==0)
{
	@strats_ =`ls ~/modelling/strats/$shc_/*/* 2>/dev/null`;
}
else
{
	my $sort_algo_ = "kCNASqrtPnlVolByTTCDD";
	my ( $num_days_past_, $min_pnl_per_contract_, $min_volume_, $max_ttc_, $max_num_to_keep_, $min_num_mid_mid_, $min_num_mid_mkt_, $min_num_mkt_mkt_, @exclude_tp_dirs_ ) = GetPruneStratsConfig ( $shc_,$time_period_ );	
	@strats_ = `$BINDIR/summarize_local_results_dir_and_choose_by_algo $sort_algo_ $max_num_to_keep_ $max_num_to_keep_ $min_pnl_per_contract_ $min_volume_ $max_ttc_ 10000000 /NAS1/ec2_globalresults/$shc_ | grep STRATEGYFILEBASE | awk '{print \$2}'`; 
}

my %ilists_scores_ = ();

for( my $i=0; $i<=$#strats_; $i++)
{
	my $ilist_ = `~/basetrade/scripts/get_ilist_from_strat.pl $strats_[$i]`;
	if(exists $ilists_scores_{$ilist_})
	{
		if($rank_method_==0)
		{
			$ilists_scores_{$ilist_}+=1;
		}
		else
		{
			$ilists_scores_{$ilist_}+= 1/($bias_+$i+1);
		}
	}
	else
	{
                if($rank_method_==0)
                {
                        $ilists_scores_{$ilist_}=1;
                }
                else
                {               
                        $ilists_scores_{$ilist_}= 1/($bias_+$i+1);
                }
	}
}

my @ilists_from_stratwork_ =`ls ~/modelling/stratwork/$ARGV[0]/* 2>/dev/null | grep ilist`;

for( my $i=0; $i<=$#ilists_from_stratwork_;$i++)
{
	my @t_ilist_tokens_ = split(/\//,$ilists_from_stratwork_[$i]);
	my $ilist_ = $t_ilist_tokens_[$#t_ilist_tokens_];
	if(not(exists $ilists_scores_{$ilist_}))
	{
		$ilists_scores_{$ilist_}=0.00;
	}
}

foreach my $value (sort {$ilists_scores_{$b} <=> $ilists_scores_{$a} } keys %ilists_scores_)
{
     my $score_ = int($ilists_scores_{$value}*100)/100.0;
     print "$score_ $value";
}

