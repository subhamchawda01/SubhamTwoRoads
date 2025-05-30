#!/usr/bin/perl
# \file ModelScripts/td_to_multiple_stats_records.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#      Suite 217, Level 2, Prestige Omega,
#      No 104, EPIP Zone, Whitefield,
#      Bangalore - 560066, India
#      +91 80 4060 0717
#
# This script is used at the end of generate_indicator_stats_records
# It takes input an ilist file, a_timed_data_file, set of filters, set of prediction_duration, trade_volume_file



use strict;
use warnings;
use feature "switch"; # for given, when
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;


my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # for GetPredCountersForThisPredAlgo


# start
exit 0; 
my $SCRIPTNAME="$0";

my $USAGE="$0 YYYYMMDD output_directory_format model-file timed-file min_price_increment norm-algo filters(csv) pred-duration(csv) [if fv filter, then daily_volume_file_path should be provided] [fsudm_level]";


if ( $#ARGV < 7 ) { print $USAGE."\n"; exit ( 0 ); }
my $arg_indx = 0;
my $yyyymmdd = $ARGV[$arg_indx ++];
my $output_directory = $ARGV[$arg_indx ++];
my $ilist_file = $ARGV[$arg_indx ++];
my $time_data_file = $ARGV[$arg_indx ++];
my $min_px_incr = $ARGV[$arg_indx ++];
my $norm_algo = $ARGV[$arg_indx ++];
my $filters_csv = $ARGV[$arg_indx ++];
my $pred_duration_csv = $ARGV[$arg_indx ++];

my $trd_vol_file="INVALIDFILE";
if ($#ARGV >= $arg_indx){
    $trd_vol_file = $ARGV[$arg_indx ++];    
}

my $fsudm_level= 0;
if ( $#ARGV >= $arg_indx )
{
    $fsudm_level = $ARGV[$arg_indx ++];
}

$filters_csv =~ s/\s+//g ;
my @filters = split( ',', $filters_csv) ;
 
$pred_duration_csv=~ s/\s+//g ;
my @pred_duration = split( ',', $pred_duration_csv) ;
#scale @pred_duration for the desired algo
my @pred_duration_scaled_arr = @pred_duration;
my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( "" , $pred_duration[0], $norm_algo, $time_data_file);
for ( my $pred_cntr_indx =0; $pred_cntr_indx <= $#pred_duration; $pred_cntr_indx = $pred_cntr_indx + 1){
	$pred_duration_scaled_arr[$pred_cntr_indx] = int ( $pred_duration[$pred_cntr_indx] * $this_pred_counters_ / $pred_duration[0] );
}

#check if model file exists
#check if timed_data file exists

my $exec_cmd = $LIVE_BIN_DIR."/timed_data_to_stats_record $ilist_file $time_data_file $min_px_incr $norm_algo ".
            (1+$#filters)." ".join(" ", @filters)." ".(1+$#pred_duration)." ".join(" ", @pred_duration_scaled_arr)." $trd_vol_file $fsudm_level" ;

print "$exec_cmd\n" ;
            
my @stats_output=` $exec_cmd `;

chomp ( @stats_output ) ;

my$num_filters=$#filters + 1;
if ( ! ($trd_vol_file eq "INVALIDFILE") ){
	$num_filters ++;	
	push ( @filters, "fv");
}
if ( $fsudm_level > 0 )
{
    $num_filters ++ ;
    push ( @filters , "fsudm" ) ;
}
my$num_pred_dur = $#pred_duration + 1;
#assert ( $num_pred_dur * $num_filters == $#stats_output +1);


my @ind_list=`grep -w "^INDICATOR" $ilist_file`;
my$num_inds = $#ind_list + 1;


if ( scalar ( @stats_output ) != ( $num_filters * $num_pred_dur * $num_inds ) )
{
    print STDOUT  scalar ( @stats_output ), " "  ;
    print STDOUT $num_filters * $num_pred_dur * $num_inds , "\n" ;
    exit ( 0 ) ;    
}


for ( my $di = 0 ; $di < $num_pred_dur ; $di++ )
{
    my $d_jump_ = $di * $num_filters * $num_inds ;

    for ( my $fi = 0 ; $fi < $num_filters ; $fi++ )
    {
	my $f_jump_ =  $d_jump_ + $fi  * $num_inds ;

	my $indicator_stats_record_file_ = $output_directory;
	$indicator_stats_record_file_ =~ s/FILTER/$filters[ $fi ]/ ;
	$indicator_stats_record_file_ =~ s/DURATION/$pred_duration[ $di ]/ ;
	$indicator_stats_record_file_ = $indicator_stats_record_file_."/indicator_stats_record_file.txt";
	CreateEnclosingDirectory ($indicator_stats_record_file_);
	if ( ! ExistsWithSize ( $indicator_stats_record_file_ ) )
	{
	    `touch $indicator_stats_record_file_`;
	}
	open OUTFILE,  ">> $indicator_stats_record_file_ " or PrintStacktraceAndDie ( "Could not open $indicator_stats_record_file_\n" );
	my $str="";
	
	for ( my $ii = 0 ; $ii < $num_inds ; $ii++ )
	{
	    #the following logic is based on the output generated by the cpp exec timed_data_to_stats_record	    
	    my @ind_words = split ( /\s+/, $ind_list [ $ii ] );
	    $ind_words[0] = $yyyymmdd." ".$ind_words[0];
	    $ind_words[1] = $stats_output [ $f_jump_ + $ii ];
	    next if ( $ind_words[1] eq "" ) ;
	    $str = $str.join(" ", @ind_words)."\n";
	}
 
	print OUTFILE $str ;
	close ( OUTFILE );    
    }
}
