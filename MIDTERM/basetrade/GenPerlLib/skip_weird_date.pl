# \file GenPerlLib/skip_weird_date.pl
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
use Carp qw/longmess/; 
use Data::Dumper;

sub SkipWeirdDate 
{
    my @this_words_ = @_;
    if ( $#this_words_ < 0 )
    {
	die "skip_weird_date called with no args\n";
	return 1;
    }

    my $input_date = shift;
    if ( ! defined $input_date || $input_date eq "" || ! ($input_date =~ /^\d+$/ ) ){
	my $mess = longmess();
	print "StackTrace: " . Dumper( $mess ). "$input_date\n";
	return 1;
    }
    if ( $input_date == 20110704 
	 || $input_date == 20110712  
	 || $input_date == 20110728  
	 || $input_date == 20110826 # lost data again
	 || $input_date == 20110905 # labor day
	 || $input_date == 20111010 # columbus day
	 || $input_date == 20111111 # veteran's day
	 || $input_date == 20111124 # thanksgiving
	 || $input_date == 20111125 # thanksgiving
	 || $input_date == 20111222 # christmas
	 || $input_date == 20111223 # christmas
	 || $input_date == 20111226 # christmas
	 || $input_date == 20111227 # christmas
	 || $input_date == 20111229 # new years
	 || $input_date == 20111230 # new years
	 || $input_date == 20120102 # new years
	 || $input_date == 20120116 # martin luther king day
	 || $input_date == 20120220 # president's day
	 || $input_date == 20120228 # bad data day 1
	 || $input_date == 20120314 # some data missing
	 || $input_date == 20120406 # good friday
	 || $input_date == 20120409 # easter monday
	 || $input_date == 20120501 # europe labor day
	 || $input_date == 20120528 # memorial day
	 || $input_date == 20120704 # july 4th
	 || $input_date == 20120903 # labor day US
	 || $input_date == 20121224 # christmas
	 || $input_date == 20121225 # christmas
	 || $input_date == 20121226 # christmas
	 || $input_date == 20121227 # new years
	 || $input_date == 20121228 # new years
	 || $input_date == 20121231 # new years
	 || $input_date == 20130101 # new years
	)
    {
	1;
    }
    else 
    {
	0;
    }
}

1;
