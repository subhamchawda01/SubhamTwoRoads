#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $IWORK_DIR=$HOME_DIR."/basetrade/indicatorwork";
my $indicator_list_prefix_ = $IWORK_DIR."/indicator_list_";
my $results_dirname_ = $IWORK_DIR."/FGBL_0_30_na_t1_630_1200";
my $unsorted_results_prefix_ = $results_dirname_."/unsorted_results_";

my $this_self_indicator_filename_ = $IWORK_DIR."/indicator_list_ABC";

my $this_results_filename_ = $this_self_indicator_filename_;
$this_results_filename_ =~ s#$indicator_list_prefix_#$unsorted_results_prefix_#g ;

print "s $indicator_list_prefix_ $unsorted_results_prefix_ g $this_self_indicator_filename_ $this_results_filename_ \n";
