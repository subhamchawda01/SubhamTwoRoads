# \file GenPerlLib/get_market_model_for_shortcode.pl
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
use feature "switch";

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/get_port_constituents.pl"; # IsValidPort, GetPortConstituents

sub ExpiredDIContract
{
  my $shortcode_ = shift;
  my $date_ = `date +%Y%m%d`; chomp($date_);
  $date_ = shift if ( @_ > 0 );

  my @shc_vec_ = ($shortcode_);
  if ( IsValidPort( $shortcode_) ) 
  {
    @shc_vec_ = GetPortConstituents($shortcode_);
  }

  foreach my $t_shc_ ( @shc_vec_ )
  {
    next if ( substr( $t_shc_ , 0 , 3 ) ne "DI1" );
    return 1 if ( GetDI1Term($t_shc_,$date_) <= 0 );
  }

  return 0;
}

sub GetDI1Term
{
  my $shortcode_ = shift;
  my $date_ = `date +%Y%m%d`; chomp($date_);
  $date_ = shift if ( @_ > 0 );

  return -1 if ( substr( $shortcode_ , 0 , 3 ) ne "DI1" );

  my @exec_cmd_ = `~/basetrade_install/bin/get_di_term $shortcode_ $date_`; chomp ( @exec_cmd_ );
  return @exec_cmd_ > 0 ? int($exec_cmd_[0]) : -1 ;
}

1
