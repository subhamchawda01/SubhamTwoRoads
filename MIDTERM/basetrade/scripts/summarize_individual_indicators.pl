#!/usr/bin/perl

# \file scripts/summarize_individual_indicators.pl
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite No 353, Evoma, #14, Bhattarhalli,
#        Old Madras Road, Near Garden City College,
#        KR Puram, Bangalore 560049, India
#        +91 80 4190 3551

use warnings;
use strict;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $work_dir=shift;

my $num_days=45;
my $all_indicator_temples="~/modelling/indicatorwork/indicator_list_*";
my @indicator_templates=`ls $all_indicator_temples` ;
my $random_number = rand(100000000);
my $tmp_file="/tmp/indicator_summary"."$random_number";

my $rec_file=$work_dir."/indicator_corr_record_file.txt";

my $make_indicator_list_exec_=$BIN_DIR."/make_indicator_list";
if ( ( -e $rec_file ) && ( -e $make_indicator_list_exec_ ) )
{
$cmd="$make_indicator_list_exec_ DUMMY1 $rec_file DUMMY2 DUMMY3 TODAY-$num_days TODAY-1 2 > $tmp_file ";
`$cmd`;

foreach my $ind ( @indicator_templates)
{
	my @ind_parts=split(/_/,$ind);
	my $sz = $#ind_parts ;
	my $indicator_name=$ind_parts[$sz-1]."_".$ind_parts[$sz] ;
	chomp($indicator_name);
	my $regex=$ind_parts[$sz-1];

	$sorted_file=$work_dir."/sorted_results_".$indicator_name;
	$unsorted_file=$work_dir."/unsorted_results_".$indicator_name;
	
	$cmd = "grep -w $regex $tmp_file > $sorted_file ";
	`$cmd` ;
	#$cmd="sort -nrk 2,2 $unsorted_file > $sorted_file";
	#`$cmd `;

}

`rm -f $tmp_file`;
}
