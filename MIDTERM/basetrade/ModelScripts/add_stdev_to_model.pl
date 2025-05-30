#!/usr/bin/perl

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

require "$GENPERLLIB_DIR/strat_utils.pl"; #GetModelL1NormVec, AddL1NormToModel

if ( $#ARGV < 2 ) 
{
  print "USAGE: <script> <modelfile> <starttime> <endtime> <startdate=TODAY-80> <enddate=TODAY-1>\n";
  exit (0);
}

my $model_ = shift;
my $start_time_ = shift;
my $end_time_ = shift;
my $sd_ = "TODAY-80";
my $ed_ = "TODAY-1";
if ( $#ARGV >= 0 )
{
  $sd_ = shift;
}
if ( $#ARGV >= 0 )
{
  $ed_ = shift;
}

if ( not IsModelScalable($model_) ) { exit(1); }
my @t_l1norm_vec_ = ();
GetModelL1NormVec($model_, $sd_ , $ed_, $start_time_ , $end_time_, \@t_l1norm_vec_);
AddL1NormToModel($model_, \@t_l1norm_vec_);

1
