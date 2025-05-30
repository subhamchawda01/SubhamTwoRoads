#!/usr/bin/perl

# \file testperl/test29.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use strict;
use warnings;
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/create_sub_model_files.pl"; # CreateSubModelFiles

my $USAGE="$0 MODELFILENAME";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $input_model_filename_ = $ARGV[0];

my @sub_model_files_ = CreateSubModelFiles ( $input_model_filename_ );
for ( my $i = 0 ; $i <= $#sub_model_files_ ; $i ++ )
{
    print $sub_model_files_[$i]."\n";
}
exit(0);
