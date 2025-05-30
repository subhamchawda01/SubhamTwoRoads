#!/usr/bin/perl

# \file scripts/rank_hist_queries.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes as input :
# product and period

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
##my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage GetStdev
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

#start
my $USAGE="$0 TIMEOFDAY SHORTCODE START_DATE END_DATE";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $timeperiod_ = $ARGV[0];
my $shortcode_ = $ARGV[1];
my $start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[3] );

#print STDERR "$SCRIPTS_DIR/rank_hist_queries.sh $timeperiod_ $shortcode_ $start_yyyymmdd_ $end_yyyymmdd_\n";
my @result_lines_ = `$SCRIPTS_DIR/rank_hist_queries.sh $timeperiod_ $shortcode_ $start_yyyymmdd_ $end_yyyymmdd_`;
chomp ( @result_lines_ );

my %stratbase2pnlsum_;
my %stratbase2volsum_;
my %stratbase2count_;


for ( my $i = 0 ; $i <= $#result_lines_; $i ++ )
{
    # format : pnl volume stratbase date
    my $thisline_ = $result_lines_[$i];
    my @this_words_ = split ( ' ', $thisline_ );
    if ( $#this_words_ >= 3 )
    {
	my $pnl_ = $this_words_[0];
	my $volume_ = $this_words_[1];
	my $stratbase_ = $this_words_[2];
	
	if ( $volume_ > 0 )
	{
	    my @stratbase_parts_ = split ( '_', $stratbase_ );
	    if ( ( $#stratbase_parts_ >= 1 )
		 && ( ( $stratbase_parts_ [ $#stratbase_parts_ ] > 0 ) && 
		      ( $stratbase_parts_ [ $#stratbase_parts_ ] < 1000000 ) ) )
	    {
		splice ( @stratbase_parts_, $#stratbase_parts_, 1 );
		$stratbase_ = basename ( join ( "_", @stratbase_parts_ ) );
	    }
	    
	    if ( ! exists $stratbase2count_{$stratbase_} )
	    {
		$stratbase2pnlsum_{$stratbase_} = 0;
		$stratbase2volsum_{$stratbase_} = 0;
		$stratbase2count_{$stratbase_} = 0;
	    }
	    
	    $stratbase2pnlsum_{$stratbase_} += $pnl_ ;
	    $stratbase2volsum_{$stratbase_} += $volume_ ;
	    $stratbase2count_{$stratbase_} ++ ;
	}
    }
}

#normalize
foreach my $stratbase_ ( keys %stratbase2pnlsum_ )
{
    if ( $stratbase2count_{$stratbase_} > 0 )
    {
	$stratbase2pnlsum_{$stratbase_} = $stratbase2pnlsum_{$stratbase_}/$stratbase2count_{$stratbase_};
	$stratbase2volsum_{$stratbase_} = $stratbase2volsum_{$stratbase_}/$stratbase2count_{$stratbase_};
    }
}

sub SBPCmp ($$) 
{
    my($sb1, $sb2) = @_;
    return $stratbase2pnlsum_{$sb2} <=> $stratbase2pnlsum_{$sb1};
}

my @sorted_stratbase_vec_ = sort SBPCmp ( keys %stratbase2pnlsum_ ) ;

foreach my $stratbase_ ( @sorted_stratbase_vec_ )
{
    printf "%d %d %s %d\n", $stratbase2pnlsum_{$stratbase_}, $stratbase2volsum_{$stratbase_}, $stratbase_, $stratbase2count_{$stratbase_};
}

exit ( 0 );
