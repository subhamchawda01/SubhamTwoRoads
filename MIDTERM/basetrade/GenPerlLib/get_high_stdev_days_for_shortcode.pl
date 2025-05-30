# \file GenPerlLib/get_high_stdev_days_for_shortcode.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
use strict;
use warnings;
use feature "switch";

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $STORE="/spare/local/tradeinfo";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetHighStdevDaysForShortcode
{

    # return top 20% of the days
    my $start_date_ = 20100101;
    my $shortcode_ = shift;
    my $time_period_ = shift;
    my $lookahead_ = shift; # one of 2,10,30,60,120,300,600
 
    my $fraction_to_pick_ = 0.3;
   
    my $t_column_ = 1;

    if ( $lookahead_ == 2 ) {
	$t_column_ = 2;
    } elsif ( $lookahead_ == 10 ) {
	$t_column_ = 3;
    } elsif ( $lookahead_ == 30 ) {
        $t_column_ = 4;
    } elsif ( $lookahead_ == 60 ) {
	$t_column_ = 5;
    } elsif ( $lookahead_ == 120 ){
	$t_column_ = 6;
    } elsif ( $lookahead_ == 300 ){
	$t_column_ = 7;
    } elsif ( $lookahead_ == 600 ){
	$t_column_ = 8;
    }
  
    my ($array_ref) = shift;
    if ( $#_ >= 0 )
    {
      $start_date_ = shift;
    }

    if ( $#_ >= 0)
    {
      $fraction_to_pick_ =shift;
    }

    my $stdev_file_ = $STORE."/datageninfo/stdev_".$shortcode_."_".$time_period_; 
   
    my $cmd_ = "cat $stdev_file_ | awk -vsd=$start_date_ '{if(\$1 >= sd){print \$1;}}' | wc -l";

    my $lines_ = `$cmd_`; chomp($lines_);
   
    my $lines_to_choose_ = int($fraction_to_pick_ * $lines_);

    $cmd_ = "cat $stdev_file_ | awk -vsd=$start_date_ '{if(\$1 >= sd){print \$1,\$$t_column_}}' | sort -k2 -gr | head -n$lines_to_choose_ | awk '{print \$1}'";
   
    my @date_lines_= `$cmd_`;
    
    for ( my $i=0; $i <= $#date_lines_; $i++ )
    {
	my $date_ = $date_lines_[$i];
	chomp ( $date_ );	
	push ( @$array_ref, $date_ );
    }
}

1
