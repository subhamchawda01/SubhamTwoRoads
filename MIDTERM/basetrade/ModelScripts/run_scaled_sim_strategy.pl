#!/usr/bin/perl

# \file ModelScripts/run_scaled_sim_strategy.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This takes a strategy file as input. expects 1 line of input.
# Creates another strategy file with 13 different strategy lines with each lines pointing to a different model
# all the models essentially are scaled from the intial model file by a factor of .60, .80, .90, .95, .98, .99, 1.00, 1.01, 1.02, 1.05, 1.10, 1.20, 1.40  

use strict;
use warnings;

use File::Basename;
use IO::File;
use FileHandle;

if( $#ARGV <3 )
{	
	print "usage: strat_file prog_id date_yyyymmdd market_model_index\n";
	exit 0 ;
}
my $strat_file=shift;
my $prog_id=shift;
my $date_yyyymmdd=shift;
my $market_model_index=shift;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $rnd = int(rand(10000000));
my $work_dir = $SPARE_HOME."RSSS/tmp_data".$rnd;
`mkdir $work_dir`;


my($filename, $directories) = fileparse($strat_file);
open(FILE, $strat_file);
my $f=$work_dir."/".$filename;
open (my $write_fh, ">", $f) or die "cannot open > $f $!";

my $ln_no = 0 ;
while(<FILE>)
{
	my @words=split(" ", $_);
	if($#words < 7)
	{
		next;
	}
	$ln_no++;
	my $fac_cnt=0;
	my $alpha = 1.13;
	foreach my $i (-20.0 .. 20.0){
	my $factor = $alpha ** $i;
		$fac_cnt++;
		my $ind_file = $work_dir."/ind_".$ln_no."_".$factor;
		my $exec_cmd = "cat $words[3] | awk '{if(\$1==\"INDICATOR\") \$2=\$2* $factor ; print \$0 }' > $ind_file";
		`$exec_cmd`; 
		foreach my $cnt(0 .. $#words)
		{
			if($cnt == 3)
			{#scaled_ind_file
				print $write_fh "$ind_file ";
			}
			elsif($cnt == $#words)
			{
				print $write_fh ($words[$cnt]*100 + $fac_cnt)."\n";
				
			}
			else
			{#prog_id
				print $write_fh "$words[$cnt] ";
			}
		}
	};
} 
close($write_fh);
close (FILE);
my $exec_cmd = "$BIN_DIR/sim_strategy SIM $f $prog_id $date_yyyymmdd $market_model_index ADD_DBG_CODE -1";
my @res=`$exec_cmd`;
#print "@res";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$date_yyyymmdd.".".int($prog_id);
$exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_";
my @res2=`$exec_cmd`;

foreach my $i( 0 .. 40 ){
	print "factor $i :: $res[$i]\n";
}

foreach my $i(0 .. $#res2){
	print "$i :: $res2[$i]\n";
}

`rm -rf $work_dir`;
