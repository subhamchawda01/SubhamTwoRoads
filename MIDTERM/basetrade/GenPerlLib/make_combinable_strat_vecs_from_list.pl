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

require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; #GetBasepxStratFirstModel GetOmixFileStratFirstModel
require "$GENPERLLIB_DIR/get_shortcode_from_stratfile.pl"; #GetShortcodeFromStratFile
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/is_strat_valid.pl"; #IsStratValid

sub MakeCombinableStratVecsFromList
{
  my $strat_list_filename_ = shift; 
  my $basepx_pxtype_ = "ALL";
  if ( @_ ) { $basepx_pxtype_ = shift; }

  my $shortcode_ = "ALL";
  if ( @_ ) { $shortcode_ = shift; }

  my %combine_key_to_files_ ;
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
        if ( -f $sfile_ ) 
        {
          my @retval = IsStratValid($sfile_);
          my $message=$retval[0];
          my $exit_code_=$retval[1];
          if ($exit_code_ != 0) {
            print "Skipping strat_file: $sfile_ Reason: $message\n";    
          }
          else {
            if ( $shortcode_ ne "ALL" ) {
              my $tshc_ = GetShortcodeFromStratFile ( $sfile_ );
              next if ( $tshc_ ne "" && $tshc_ ne $shortcode_ );
            }
            my $t_basepx_pxtype_ = GetBasepxStratFirstModel ( $sfile_ );
#necessary to keep the watch in sync
            my $t_start_time_ = `awk '{print \$6}' $sfile_`; chomp($t_start_time_);
            if( ( $basepx_pxtype_ eq "ALL" ) || ( $t_basepx_pxtype_ eq $basepx_pxtype_ ) ) 
            {
              my $t_omix_file_ = GetOmixFileStratFirstModel ( $sfile_ );
              my $t_onlinemix_file_ = GetOnlineMixFileStratFirstModel($sfile_);

              my $t_key_ = $t_basepx_pxtype_.$t_omix_file_.$t_onlinemix_file_."_".$t_start_time_;
              push ( @{$combine_key_to_files_{$t_key_}}, $sfile_ );
            }
          }
        }
      }
    }

    close STRAT_LIST_FILEHANDLE;
  }
  \%combine_key_to_files_ ;
}

1
