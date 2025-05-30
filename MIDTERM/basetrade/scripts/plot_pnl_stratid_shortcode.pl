#!/usr/bin/perl

# \file scripts/plot_pnl_stratid_shortcode.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/get_strat_id_for_shortcode.pl"; #GetStratIdForShortcode
require "$GENPERLLIB_DIR/exists_with_size.pl"; #ExistsWithSize

my $short_code = shift ;
my $trd_date = shift ;
my $strat_id = GetStratIdForShortcode( $short_code );
$strat_id =~ s/..$/??/ ;

my $split_date = $trd_date;
$split_date =~ s/(....)(..)(..).*/$1\/$2\/$3/g;

my $trd_dir = "/NAS1/logs/QueryTrades/".$split_date ;
my $trade_logs="/NAS1/logs/QueryTrades/".$split_date."/trades.".$trd_date.".".$strat_id ;

print $trd_dir."\n";

print $trade_logs."\n";



open (GP, "|gnuplot -persist ") or die "no gnuplot";

use FileHandle;
GP->autoflush(1);

print GP "set xdata time; \n set timefmt \"\%s\"; \n set grid \n";
print GP " set  style line  1 linetype  1 linewidth 1 ;\n";
print GP " set  style line  2 linetype  2 linewidth 1 ;\n";
print GP " set  style line  3 linetype  3 linewidth 1 ;\n";
print GP " set  style line  4 linetype  4 linewidth 1 ;\n";
print GP " set  style line  5 linetype  5 linewidth 1 ;\n";
print GP " set  style line  6 linetype  6 linewidth 1 ;\n";
print GP " set  style line  7 linetype  7 linewidth 1 ;\n";
print GP " set  style line  8 linetype  8 linewidth 1 ;\n";
print GP " set  style line  9 linetype  9 linewidth 2 ;\n";
print GP " set  style line 10 linetype 10 linewidth 2 ;\n";
print GP " set  style line 11 linetype 11 linewidth 2 ;\n";
print GP " set  style line 12 linetype 12 linewidth 2 ;\n";
print GP " set  style line 13 linetype 13 linewidth 2 ;\n";
print GP " set  style line 14 linetype 14 linewidth 2 ;\n";
print GP " set  style line 15 linetype 15 linewidth 2 ;\n";
print GP " set  style line 16 linetype 16 linewidth 2 ;\n";

print GP "plot ";
my $ls_files = `ls $trade_logs ` ;
my $cnt=0;
foreach (split(/\n/,$ls_files))
{
	if(!ExistsWithSize($_))
	{
		next;
	}
	if($cnt != 0)
	{
		print GP ", "
	}
	$cnt++;
	my @arr=split(/\//,$_);
        
	print GP "\"$_\" using 1:9 with lines ls $cnt title '$arr[$#arr]' ";
}
print GP " \n";
close GP;
