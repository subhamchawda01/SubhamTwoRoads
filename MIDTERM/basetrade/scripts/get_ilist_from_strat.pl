#!/usr/bin/perl
#
# \file scripts/get_ilist_from_strat.pl
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

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";


    my $strat_name_ = shift;
    my $full_strat_name_ = "$HOME_DIR/modelling/strats/**/**/$strat_name_";
    if( -e $strat_name_ )
    { 
 	$full_strat_name_ = $strat_name_;
    }
    
    {
	my @strat_tokens_ = split(/_/,$strat_name_);
	my $start_token_ = 0;
	my $end_token_ = 0;
        for(my $i=0;$i<=$#strat_tokens_;$i++)
        {
               if($strat_tokens_[$i]=~/ilist/)
               {
               		$start_token_ = $i;
 	       }

	       if($strat_tokens_[$i]=~/na/ || $strat_tokens_[$i]=~/ac/)
               {
                       $end_token_ = $i;
               }
        }
        my @ilist_tokens_ = @strat_tokens_[$start_token_..($end_token_-2)];
        my $ilist_ = join('_',@ilist_tokens_);
        print $ilist_."\n";
    }
