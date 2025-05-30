# \file GenPerlLib/get_basepx_strat_first_model.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

#use feature "switch"; # for given, when

my $HOME_DIR=$ENV{'HOME'}; 

my $USER=$ENV{'USER'};
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetBasepxStratFirstModel
{
  my $strategy_file_ = shift;
  my $basepx_pxtype_first_model_ = "UNDEF"; # so that strats with no model don't match

  if ( -e $strategy_file_ )
  {
    my $MFILE_ = "";
    open ( SFILE_HANDLE, "< $strategy_file_" ) or PrintStacktraceAndDie ( " Could not open file $strategy_file_ for reading \n" );
    while ( my $sline_ = <SFILE_HANDLE> )
    { 
      my @words_ = split ( ' ', $sline_ );
      if ( $#words_ >= 3 )
      {
        $MFILE_=$words_[3];
        last; # first model
      }
    }
    close ( SFILE_HANDLE );

    if ( $MFILE_ )
    {
      $basepx_pxtype_first_model_ = GetBasepxModel($MFILE_);

    }
  }

  $basepx_pxtype_first_model_;
}

sub GetShcListKeyForMRTModel
{
  my $MFILE_ = shift;
  my $shclist_catted_ = "UNDEF";
  my @shclist_ = ();

  if ( -e $MFILE_ )
  {
    open ( MFILE_HANDLE, "< $MFILE_" ) or PrintStacktraceAndDie ( " Could not open file $MFILE_ for reading \n" );
    while ( my $mline_ = <MFILE_HANDLE> )
    {
      chomp ($mline_);
      next if ($mline_ =~ /^#/ || $mline_ eq "");
      push (@shclist_, $mline_);
    }

    $shclist_catted_ = join("\n", @shclist_);
    close ( SMFILE_HANDLE );
  }

  $shclist_catted_;
}


sub GetBasepxModel
{
  my $MFILE_ = shift;
  my $basepx_pxtype_first_model_ = "UNDEF";
  if ( -e $MFILE_ )
  {
    open ( MFILE_HANDLE, "< $MFILE_" ) or PrintStacktraceAndDie ( " Could not open file $MFILE_ for reading \n" );
    while ( my $mline_ = <MFILE_HANDLE> )
    {
      my @words_ = split ( ' ', $mline_ );
      if ( $#words_ >= 3 && $words_[0] eq "MODELINIT" )
      { # MODELINIT DEPBASE ZT_0 MktSizeWPrice MktSizeWPrice
        $basepx_pxtype_first_model_=$words_[3];
        last; # first line
      }
    }
    close ( MFILE_HANDLE );
  }
  $basepx_pxtype_first_model_;
}

sub GetOmixFileStratFirstModel
{
  my $strategy_file_ = shift;
  my $omix_file_ = "/spare/local/tradeinfo/OfflineInfo/offline_mix_mms.txt";
  if ( -e $strategy_file_ )
  {
    my $MFILE_ = "";
    open ( SFILE_HANDLE, "< $strategy_file_" ) or PrintStacktraceAndDie ( " Could not open file $strategy_file_ for reading \n" );
    while ( my $sline_ = <SFILE_HANDLE> )
    { 
      my @words_ = split ( ' ', $sline_ );
      if ( $#words_ >= 3 )
      {
        $MFILE_=$words_[3];
        last; # first model
      }
    }
    close ( SFILE_HANDLE );

    if ( $MFILE_ )
    {
      $omix_file_ = GetOmixFileModel($MFILE_);
    }
  }

  $omix_file_;
}

sub GetOmixFileModel
{
  my $MFILE_ = shift;
  my $omix_file_ = "/spare/local/tradeinfo/OfflineInfo/offline_mix_mms.txt";

  if ( -e $MFILE_ )
  {
    open ( MFILE_HANDLE, "< $MFILE_" ) or PrintStacktraceAndDie ( " Could not open file $MFILE_ for reading \n" );
    while ( my $mline_ = <MFILE_HANDLE> )
    {
      my @words_ = split ( ' ', $mline_ );
      if ( $#words_ >= 1 && $words_[0] eq "OFFLINEMIXMMS_FILE" )
      { # MODELINIT DEPBASE ZT_0 MktSizeWPrice MktSizeWPrice
        $omix_file_=$words_[1];
        last; # first line
      }
    }
    close ( MFILE_HANDLE );
  }

  $omix_file_;
}

sub GetOnlineMixFileStratFirstModel
{
  my $strategy_file_ = shift;
  my $onlinemix_file_ = "/spare/local/tradeinfo/OnlineInfo/online_price.txt";
  if ( -e $strategy_file_ )
  {
    my $MFILE_ = "";
    open ( SFILE_HANDLE, "< $strategy_file_" ) or PrintStacktraceAndDie ( " Could not open file $strategy_file_ for reading \n" );
    while ( my $sline_ = <SFILE_HANDLE> )
    {
      my @words_ = split ( ' ', $sline_ );
      if ( $#words_ >= 3 )
      {
        $MFILE_=$words_[3];
        last;
      }
    }
    close ( SFILE_HANDLE );

    if ( $MFILE_ )
    {
    	$onlinemix_file_ = GetOnlineMixFileModel($MFILE_);
    }
  }
  $onlinemix_file_;
}


sub GetOnlineMixFileModel
{
  my $MFILE_ = shift;
  my $onlinemix_file_ = "/spare/local/tradeinfo/OnlineInfo/online_price.txt";
  if ( -e $MFILE_ )
  {
    open ( MFILE_HANDLE, "< $MFILE_" ) or PrintStacktraceAndDie ( " Could not open file $MFILE_ for reading \n" );
    while ( my $mline_ = <MFILE_HANDLE> )
    {
      my @words_ = split ( ' ', $mline_ );
      if ( $#words_ >= 1 && $words_[0] eq "ONLINEMIXPRICE_FILE" )
      {
        $onlinemix_file_=$words_[1];
        last;
      }
    }
    close ( MFILE_HANDLE );
  }
  $onlinemix_file_;
}

1
