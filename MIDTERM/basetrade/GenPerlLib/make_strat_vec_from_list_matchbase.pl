# \file GenPerlLib/make_strat_vec_from_list_matchbase.pl
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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; #GetBasepxStratFirstModel

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub MakeStratVecFromListMatchBase
{
  my ( $strat_list_filename_, $basepx_pxtype_) = @_;
  my @files_ = ();
  if ( -f $strat_list_filename_ )
  {
    open STRAT_LIST_FILEHANDLE, "< $strat_list_filename_ " or PrintStacktraceAndDie ( "Could not open $strat_list_filename_\n" );
    while ( my $thisline_ = <STRAT_LIST_FILEHANDLE> ) 
    {
      chomp ( $thisline_ );
      my @t_words_ = split ( ' ', $thisline_ );
      if ( ( $#t_words_ >= 0 ) &&
          ( ! ( $t_words_[0] =~ /#/ ) ) )
      { 
        my $sfile_ = $t_words_[0] ;
        if ( ( $basepx_pxtype_ eq "ALL" ) || ( GetBasepxStratFirstModel ( $sfile_ ) eq $basepx_pxtype_ ) )
        {
          push ( @files_, $sfile_ );
        }
      }
    }

    close STRAT_LIST_FILEHANDLE;
  }
  @files_ ;
}

1
