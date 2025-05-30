#!/usr/bin/perl
#
# \file ModelScripts/find_best_model_for_strategy_var_pert.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
#

use strict;
use warnings;
use feature "switch";
use FileHandle;
use POSIX;
use List::Util qw/max min/; # for max
use File::Basename;
use Term::ANSIColor; 
my $HOME = $ENV{'HOME'};
my $USAGE="$0 shc sd ed num_strats dir sort_algo config [minvol=0] [maxttc=10000]";
if ( $#ARGV < 6 ){ print "USAGE : ".$USAGE."\n"; exit(0);}

my $shc= $ARGV [ 0 ]; 
my $given_sd= $ARGV [ 1 ]; 
my $ed= $ARGV [ 2 ]; 
my $num_strats= $ARGV [ 3 ];
my $dir= $ARGV [ 4 ];
my $sort_algo= $ARGV [ 5 ];
my $config_file_ = $ARGV [ 6 ];
my $sfreq= 5;
my $hfreq=100;
my $mvol=0;
my $max_ttc=10000;

if ( $#ARGV >= 7 ) { $mvol= $ARGV [ 7 ]; }
if ( $#ARGV >= 8 ) { $max_ttc= $ARGV [ 8 ]; }

#print "shc=$shc given_sd=$given_sd ed=$ed num_strats=$num_strats dir=$dir sort_algo=$sort_algo sfreq=$sfreq hfreq=$hfreq mvol=$mvol\n";
my $uid=`date +%N`;
chomp ($uid);
my $tfile_c="tmp_".$uid;
my $ofile_c="out_".$uid;
my $sd=$ed;
my $summ_ed=`$HOME/basetrade_install/bin/calc_prev_week_day $sd 1`; 
my $summ_sd=`$HOME/basetrade_install/bin/calc_prev_week_day $sd $sfreq`; 
my $days=0;
my @days_to_test_vec_ = ( );
my @weights_for_days_vec_ = ( );
my @days_to_test_ = ( );
my @weights_for_days_ = ( );
my %strat_to_stat_map_ = ( );

ReadConfigFile ( $config_file_, \@days_to_test_vec_, \@weights_for_days_vec_ );

for ( my $idx=0; $idx<=$#days_to_test_vec_; $idx++ )
{
#my $tref_ = $days_to_test_vec_[ $idx ];
    @days_to_test_ = @{$days_to_test_vec_[ $idx ]};
#$tref_ = $weights_for_days_vec_[ $idx ];
    @weights_for_days_ = @{$weights_for_days_vec_[ $idx ]};
#print "IDX $idx\n";
#for ( my $id=0; $id<=$#days_to_test_; $id++ )
#{
#print "$days_to_test_[$id] $weights_for_days_[$id]\n";
#}
#next;
    %strat_to_stat_map_ = ( );
    $days=0;
    $sd=$ed;
    my $tfile=$tfile_c.$idx;
    my $ofile=$ofile_c.$idx;
    `rm -f $tfile`;
    `rm -f $ofile`;

    while ( $sd >= $given_sd )
{
    $days=$days + 1;
    $summ_ed=`$HOME/basetrade_install/bin/calc_prev_week_day $sd 1`; 
    for ( my $i=0; $i<=$#days_to_test_; $i++ )
    {
	my $num_days_= $days_to_test_[ $i ];
	$summ_sd=`$HOME/basetrade_install/bin/calc_prev_week_day $sd $num_days_`;  
        my $newtfile = $tfile.$num_days_;
	my $cmd="$HOME/basetrade_install/bin/summarize_strategy_results $shc $dir $HOME/ec2_globalresults $summ_sd $summ_ed INVALIDFILE $sort_algo | awk -vvol=$mvol -vttc=$max_ttc '{if((\$5>vol) && (\$10<ttc)) {print \$2, \$NF}}' > $newtfile" ; 
	#print "$cmd\n";
	`$cmd`;
	$cmd="cat $newtfile";
	
	my @summ_res_=`$cmd`;
	for ( my $j=0; $j <= $#summ_res_; $j++ ){
	    my $res_line_ = $summ_res_[ $j ];
	    chomp ($res_line_);
	    my @res_words_ = split(' ',$res_line_);
	    if ( ! exists $strat_to_stat_map_{$res_words_[ 0 ]} )
	    {
		$strat_to_stat_map_{$res_words_[ 0 ]} = $res_words_[ $#res_words_ ] * $weights_for_days_[ $i ];
	    }
	    else
	    {
		$strat_to_stat_map_{$res_words_[ 0 ]} += $res_words_[ $#res_words_ ] * $weights_for_days_[ $i ];
	    }
	}
        
    }
    my $num_=1;
    `echo DATE $sd >> $ofile`;
    foreach my $strat_ (sort { $strat_to_stat_map_{$b} <=> $strat_to_stat_map_{$a} } keys %strat_to_stat_map_)
    {
	#print $strat_."\n";
	my $cmd="$HOME/basetrade_install/bin/summarize_strategy_results $shc $dir $HOME/ec2_globalresults $sd $sd | awk -vstrat=$strat_ '{if(\$2==strat){print \$_}}' >> $ofile"; 
	#print "$cmd\n";
	`$cmd`;
        if ( $num_ >= $num_strats ) { last; }
	else { $num_ ++; }
    }
    $sd=$summ_ed; 
}

print "TDAYS: ".$days."\n";
my $cmd="cat $ofile | grep STRAT | awk 'BEGIN{pnl=0; vol=0} {pnl+=\$3; vol+=\$5} END{print \"ST AVGPNL,VOL:\", $num_strats*(pnl/NR), $num_strats*(vol/NR)}'";
my $out_line_=`$cmd`;
print "Config $idx:\n";
for ( my $i=0; $i<=$#days_to_test_; $i++ )
{
print "$days_to_test_[$i]: $weights_for_days_[ $i ]\t";
}
print "\n";
print $out_line_;
}



sub ReadConfigFile 
{
    my ( $t_config_file_, $t_days_vec_vec_, $t_weights_vec_vec_ ) = @_;
    my @t_days_vec_ = ( );
    my @t_weights_vec_ = ( );
    open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
    my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
    close ( CONFIG_FILE );
    foreach my $config_file_lines_ ( @config_file_lines_ )
{
    my @t_words_ = split ( ' ' , $config_file_lines_ );
    if ( $#t_words_ < 0 )
{
    next;
}
else
{
    if ($t_words_[ 0 ] eq "CONFIGSTART")
{
    @t_days_vec_ = ( );
    @t_weights_vec_ = ( );
}
elsif ($t_words_[ 0 ] eq "CONFIGEND")
{
    push (@$t_days_vec_vec_, [ @t_days_vec_ ] );
    push (@$t_weights_vec_vec_, [ @t_weights_vec_ ] );
}
else
{
    if ($#t_words_ == 1)
{
    push ( @t_days_vec_, $t_words_[ 0 ]);
    push ( @t_weights_vec_, $t_words_[ 1 ]);
}
else
{
    next;
}
}
}

}

return;
}
