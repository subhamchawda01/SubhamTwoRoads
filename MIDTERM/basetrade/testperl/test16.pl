#!/usr/bin/perl
use strict;
use warnings;
use feature "switch"; # for given, when
# use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames

if ( $#ARGV < 0 )
{
    print "USAGE: stratfile\n";
    exit(0);   
}

my ( $t_model_filename_, $t_param_filename_ ) = GetModelAndParamFileNames ( $ARGV[0] ) ;
printf STDOUT "files : %s %s\n", $t_model_filename_, $t_param_filename_;
