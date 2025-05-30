# \file GenPerlLib/get_cs_temp_file_name.pl
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
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetCSTempFileName 
{
    my $work_dir_ = shift;
    my $cstempfile_ = "";

    my $cstempdir = $work_dir_."/cstemp";
    if ( ! -d $cstempdir )
    { `mkdir -p $cstempdir`; }
    
    my $unique_cstf_id_ = `date +%N`; chomp ( $unique_cstf_id_ ); 
    $unique_cstf_id_ = int($unique_cstf_id_) + 0;

    $cstempfile_ = $cstempdir."/cstempfile_".$unique_cstf_id_.".txt";
    my $loop_cnt_ = 0;
    while ( -e $cstempfile_ )
    {
	$loop_cnt_ ++;
	$unique_cstf_id_ = `date +%N`; chomp ( $unique_cstf_id_ ); 
	$unique_cstf_id_ = int($unique_cstf_id_) + 0;

	$cstempfile_ = $cstempdir."/cstempfile_".$unique_cstf_id_.".txt";
	if ( $loop_cnt_ >= 100 )
	{
	    last;
	}
    }

    $cstempfile_;
}

1;
