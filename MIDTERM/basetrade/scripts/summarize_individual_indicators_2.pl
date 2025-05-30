#!/usr/bin/perl

# \file scripts/summarize_individual_indicators_2.pl
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use List::Util qw/max min/; # for max
use FileHandle;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $ITEMPLATES_DIR=$HOME_DIR."/modelling/indicatorwork";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
# sub BuildIndicatorToExtensionMap;

if ( $USER eq "rkumar" ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "mayank" || $USER eq "rahul" || $USER eq "ankit" )
{
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

my %indicator_to_extension_map_ ;

my $USAGE="$0 correlation_record_file [file_prefix=results] [NUM_DAYS=45]";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $rec_file=$ARGV[0];
if ( -e $rec_file )
{
    my $file_prefix_ = "results";
    if ( $#ARGV >= 1 )
    {
	$file_prefix_ = $ARGV[1];
    }
    
    my $num_days=45;
    if ( $#ARGV >= 2 )
    {
	$num_days= min ( 200, $ARGV[2] );
	$num_days= max ( 10, $num_days );
    }

    # BuildIndicatorToExtensionMap ( );

    my $work_dir=dirname($rec_file);
    my $rec_file_basename_=basename($rec_file);

    my $summary_file_ = $work_dir."/summary_".$rec_file_basename_;
    
    my $all_indicator_temples="$ITEMPLATES_DIR/indicator_list_*";
    my @indicator_templates=`ls $all_indicator_temples` ;
    
    my $make_indicator_list_exec_="$LIVE_BIN_DIR/make_indicator_list";
    if ( ! ( -e $make_indicator_list_exec_ ) )
    {
	$make_indicator_list_exec_="$BIN_DIR/make_indicator_list";
    }

    my $cmd="$make_indicator_list_exec_ DUMMY1 $rec_file DUMMY2 DUMMY3 TODAY-$num_days TODAY-1 2 -a 1 > $summary_file_ "; # change to sort by stdev_penalized_mean
#    print STDERR "$cmd\n";
    `$cmd`;
    
    foreach my $ind ( @indicator_templates)
    {
	my @ind_parts=split(/_/,$ind);
	my $sz = $#ind_parts ;
	my $indicator_name=$ind_parts[$sz-1]."_".$ind_parts[$sz] ;
	chomp($indicator_name);
	my $regex=$ind_parts[$sz-1];
	
	my $sorted_file=$work_dir."/sorted_".$file_prefix_."_".$indicator_name;
	# my $unsorted_file=$work_dir."/unsorted_".$file_prefix_."_".$indicator_name;
	
	my $fcmd = "grep -w $regex $summary_file_ > $sorted_file ";
	`$fcmd` ;
    }

    # open SUMMARYFILEHANDLE, "< $summary_file_ " or PrintStacktraceAndDie ( "Could not open $summary_file_\n" );
    # while ( my $thisline_ = <SUMMARYFILEHANDLE> ) 
    # {
    # 	chomp ( $thisline_ );
    # 	my @stat_line_words_ = split ( ' ', $thisline_ );
    # 	if ( ( $#stat_line_words_ >= 3 ) &&
    # 	     ( $stat_line_words_[0] eq "INDICATOR" ) )
    # 	{
    # 	    my $indicator_name_ = $stat_line_words_[3];
    # 	    if ( exists $indicator_to_extension_map_{$indicator_name_} )
    # 	    {
    # 		my $indicator_extension_ = $indicator_to_extension_map_{$indicator_name_};
    # 		my $unsorted_filename_ = $work_dir."/unsorted_".$file_prefix_."_".$indicator_name_."_".$indicator_extension_;
    # 		my $sorted_filename_ = $work_dir."/sorted_".$file_prefix_."_".$indicator_name_."_".$indicator_extension_;
    # 		# write this line to the right file
    # 	    }
    # 	}
    # }

    my $gzipped_summary_file_ = $summary_file_.".gz";
    if ( -e $gzipped_summary_file_ ) { `rm -f $gzipped_summary_file_`; }
    `gzip $summary_file_`;
}

exit ( 0 );

### SUBS ###
# sub BuildIndicatorToExtensionMap
# {

# }
