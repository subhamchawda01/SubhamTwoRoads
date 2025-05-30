#!/usr/bin/perl
# \file GenPerlLib/is_model_corr_consistent.pl
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
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

sub GetSpareLocalDir
{

    my $spare_local_dir_ = "/spare/local/";
    my $min_avail_ = 100;
    my $min_index_ = 0;
    my $hostname_ = `hostname`;
    if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
    {
	my $exec_cmd_ = "df -H | grep dev | grep ephemeral | awk '{print \$(NF-1)\" \"substr(\$(NF),length(\$(NF)))}' | replace \"%\" \"\" | sort -k1n | head -n1 | awk '{ print \$2}'";
	$min_index_ = `$exec_cmd_`;
	chomp ( $min_index_ );
    }

    $spare_local_dir_ = "/spare$min_index_/local/";
    $spare_local_dir_;
}

1
