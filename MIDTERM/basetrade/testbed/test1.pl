#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/get_spare_local_dir_for_aws.pl"; # GetSpareLocalDir
my $SPARE_LOCAL="/spare/local/";
my $hostname_ = `hostname`;
if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
{
  $SPARE_LOCAL = GetSpareLocalDir();
}
print $SPARE_LOCAL."\n";
