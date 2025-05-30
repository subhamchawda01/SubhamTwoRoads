#!/usr/bin/perl

# \file ModelScripts/call_optimize_risk.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes an instructionfilename :
# startdate enddate
# constraints

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub ComputeResults ;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; #FindItemFromVec


# start 
my $USAGE="$0 instructionfilename start_date end_date\n";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $instructionfilename_ = $ARGV[0];
my $start_date =  $ARGV[1];
my $end_date = $ARGV[2];


ComputeResults ();

exit ( 0 );



sub ComputeResults
{
	my @day_vec_ = (); #just a list of start days for optimize_risk script, this is not same as days over wich avging will be done in the called script
	my @result_file_vec_ = ();
	my @real_pnls = ();
	my @optimal_pnls = ();
	
	my $datagen_date_ = $end_date ;
	my $max_days_at_a_time_ = 180;
	for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
	{
	    if ( ( ! ValidDate ( $datagen_date_ ) ) || ( $datagen_date_ < $start_date ) )
	    {
		last;
	    }
	    if ( SkipWeirdDate ( $datagen_date_ ) )
	    {
		$datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
		next;
	    }
	    {   
		push ( @day_vec_, $datagen_date_ ) ;
	    }
	    $datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
	}
	
	print "working_date\t real_pnls\t optimal_pnls\n";
	for ( my $i = 0; $i <= $#day_vec_; $i++ )
	{
		my $working_date = $day_vec_[$i];
		my $exec_cmd = "$HOME_DIR/infracore/scripts/optimize_risk.pl $instructionfilename_ $working_date 2>/dev/null";
		my $result_line = `$exec_cmd`; # should return result file name, real_pnl and optimal_pnl
		my @result_words = split ( ' ', $result_line );
		if ( $#result_words == 5 )
		{
			push ( @result_file_vec_, $result_words[1] );
			push ( @real_pnls, $result_words[3] );
			push ( @optimal_pnls, $result_words[5] );
		}
		else
		{
			push ( @result_file_vec_, "NA" );
			push ( @real_pnls, 0 );
			push ( @optimal_pnls, 0 );
		}
		print "$working_date\t $real_pnls[$i]\t $optimal_pnls[$i]\n";
	}
	print "Complete Results in @result_file_vec_\n";
}
