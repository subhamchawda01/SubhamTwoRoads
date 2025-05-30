#!/usr/bin/perl
use strict;
use warnings;
use List::Util qw /min max/;
use Fcntl qw (:flock);

use Digest::MD5 qw(md5 md5_hex md5_base64);
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_seconds_to_prep_for_shortcode.pl"; # GetDataStartTimeForStrat

if ( $#ARGV >= 1 )
{
  print GetDataStartTimeForStrat( $ARGV[0], $ARGV[1] )."\n" ;
}
else
{
  print "-1\n";
}
