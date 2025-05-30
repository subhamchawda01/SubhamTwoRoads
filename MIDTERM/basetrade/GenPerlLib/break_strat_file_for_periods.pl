# \file GenPerlLib/break_strat_file_for_periods.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite No 353, Evoma, #14, Bhattarhalli,
#        Old Madras Road, Near Garden City College,
#        KR Puram, Bangalore 560049, India
#        +91 80 4190 3551
#

use strict;
use warnings;
use POSIX;
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use POSIX qw/floor/;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; #GetBasepxStratFirstModel
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_matchbase.pl"; #MakeStratVecFromDirMatchBase
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; #GetUTCHHMMStr



my $running_duration = 100 ; 
#my $temp_strat_dir = "$SPARE_HOME/temp_strat_dir/" ;


sub BreakStratFileForPeriods
{
  my ($top_directory_, $basepx_pxtype_, $temp_strat_dir) = @_;
  my @strategy_filevec_ = MakeStratVecFromDirMatchBase ($top_directory_, $basepx_pxtype_);
  
  `rm -rf $temp_strat_dir` ;
  if ( ! ( -d $temp_strat_dir ) )
  {
  	`mkdir -p $temp_strat_dir` ;
  }

  foreach  my $temp_strat_ ( @strategy_filevec_ )
  {
    my $orig_start_time = `awk '{print \$6}' $temp_strat_`;
    my $orig_end_time = `awk '{print \$7}' $temp_strat_`;
    my $start_time = GetUTCHHMMStr ( $orig_start_time, "" );
    my $end_time = GetUTCHHMMStr ( $orig_end_time, "" );
    
    my $no_of_periods = floor ( ( $end_time - $start_time ) / $running_duration ) ;
    my $this_strat_basename_ = basename ( $temp_strat_ ) ; chomp ( $this_strat_basename_ ) ;
   
    for ( my $i = 0 ; $i < $no_of_periods ; $i++ )
    {
    	my $new_start_time = $start_time + $i*$running_duration ;
    	my $new_end_time = $new_start_time + $running_duration ;
    	if ( $i ==  $no_of_periods - 1 ) { $new_end_time = $end_time ; }
    	
    	my $new_strat = $temp_strat_dir."$this_strat_basename_"."_Start_".$new_start_time."_End_".$new_end_time ;
    	`cp $temp_strat_ $new_strat` ;
    	`sed -i 's/ \$orig_start_time \$orig_end_time / \$new_start_time \$new_end_time /g' $new_strat` ;
    }
  }
}

1
