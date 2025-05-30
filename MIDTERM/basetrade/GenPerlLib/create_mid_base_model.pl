# \file GenPerlLib/name_strategy_from_model_and_param_index.pl
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
use File::Basename;

sub CreateMidBaseModel
{
    my ( $this_model_file_, $strategyname_ ) = @_;
    my $this_model_filename_base_ = basename ( $this_model_file_ ); chomp ($this_model_filename_base_);
    my $this_model_filename_dir_ = dirname ( $this_model_file_ ); chomp ( $this_model_filename_dir_ );
    my $temp_mid_base_model_file_ = $this_model_filename_dir_."/".$this_model_filename_base_."_temp_mid_model" ;

    my $base_pxtype_ = "" ;

    if ( -e $this_model_file_ )
     {
       open ( MFILE_HANDLE, "< $this_model_file_ " ) or PrintStacktraceAndDie ( " Could not open file $this_model_file_ for reading \n" );
       while ( my $mline_ = <MFILE_HANDLE> )
       {
         my @words_ = split(' ', $mline_);
         if ( ( $#words_ >= 3) &&
             ( $words_[0] eq "MODELINIT" ) )
         {
           $base_pxtype_ = $words_[3];
           last; 
         }
       }
       close ( MFILE_HANDLE );
     }

    my $to_set_mid_base_ = 0;

    if ( ( $strategyname_ eq "DirectionalAggressiveTrading" ) && ( ( $base_pxtype_ ne "MidPrice" ) && ( $base_pxtype_ ne "Midprice" ) ) )
    {
      $to_set_mid_base_ = 1;
      my $count_ = `grep -c USE_MID_AS_BASE $this_model_file_`;
      if ( $count_ > 0 )
      {
        $to_set_mid_base_ = 0;
      }
    }
    
    if ( $to_set_mid_base_ )
    {
      if ( -e $this_model_file_ )
      {
	open IMODEL_FILEHANDLE, "< $this_model_file_";
	open OMODEL_FILEHANDLE, "> $temp_mid_base_model_file_";
	while ( my $mline_ = <IMODEL_FILEHANDLE> )
	{
	  chomp ( $mline_ );
	  my @rwords_ = split ' ', $mline_;
	  if ( ( $#rwords_ >= 0 ) && ( $rwords_[0] eq "MODELMATH" ) )
	  {	
	    print OMODEL_FILEHANDLE $mline_."\n";
            print OMODEL_FILEHANDLE "USE_MID_AS_BASE\n";
          }
          else
	  {
	    print OMODEL_FILEHANDLE $mline_."\n";
	  }
	}
	close OMODEL_FILEHANDLE;
	close IMODEL_FILEHANDLE;
      }
      `mv $temp_mid_base_model_file_ $this_model_file_`;
    }
}

1;
