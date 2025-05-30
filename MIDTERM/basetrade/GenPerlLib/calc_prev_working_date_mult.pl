# \file GenPerlLib/calc_prev_working_date_mult.pl
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
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl";
require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec

#implement cache to store already computed results. introduced for speed
my %prev_working_day_cache_ ;

sub CalcPrevWorkingDateMult 
{
  my @this_words_ = @_;
  my $arg_line = "";
  my $retval = 20110101;
  
  my @ADDITIONAL_EXEC_PATHS=();
  my $calc_prev_week_day = SearchExec ( "calc_prev_week_day", @ADDITIONAL_EXEC_PATHS ) ;
  if ( ! $calc_prev_week_day ) {
  exit(0);
  }

  if ( $#this_words_ >= 1 )
  {

    $arg_line = join('_',@this_words_);  
    if (exists $prev_working_day_cache_{$arg_line} ) 
    {      
      return $prev_working_day_cache_{$arg_line};
    }

    my $input_date = $this_words_[0];
    my $num_times = $this_words_[1];
    $retval = $input_date;

    if($#this_words_ < 2)
    { # <= 2 arguments
      if ( $num_times > 0 )
      {
        $retval = `$calc_prev_week_day $input_date $num_times`; # no chomp required
        die "calc_prev_week_day returned Weird String : $retval from $input_date $num_times" if length ( $retval ) != 8
      }
    }
    else 
    {
      my $shortcode_ = $this_words_[2];
      my @skip_dates_ = ();
      if ( $#this_words_ >= 3)
      {
        if ( -s $this_words_[3] )
        {
          @skip_dates_ = `cat $this_words_[3]`; chomp(@skip_dates_);
        }
      }

      if ( !$shortcode_ && !@skip_dates_ )
      { #shortcode give but empty or undefined, and no skip dates - falling back to 2 args
        $retval = CalcPrevWorkingDateMult( $retval, $num_times );
      }
      else
      {
        while ( $num_times > 0 )
        {
          last if ( ! ValidDate ( $retval ) ); 
          
          $retval = `$calc_prev_week_day $retval`; 
          next if ( ( $shortcode_ && IsProductHoliday ( $retval, $shortcode_) ) || ( grep {$_ eq $retval} @skip_dates_ ) ) ;
          $num_times --;    
        }
      }
    }
  }
  else
  {
    die "CalcPrevWorkingDateMult called with less than 2 args: $#this_words_";
  }

  if ( ! ( $retval ) )
  {
    $retval = 20110101;
  }
  $prev_working_day_cache_{$arg_line} = $retval;
  $retval;	
}

# removed this ... now please use scripts/call_calc_prev_working_date_mult.pl
#if ( __FILE__ eq $0 ){
#    print "$ARGV[0] - ".CalcPrevWorkingDateMult ( @ARGV )."\n";
#}


1;
