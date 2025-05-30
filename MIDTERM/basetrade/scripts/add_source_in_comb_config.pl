#!/usr/bin/perl

# \file GenPerlLib/gen_ind_utils.pl
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
use feature "switch";

my $HOME_DIR=$ENV{'HOME'};
my $USER=$ENV{'USER'};

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";


require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE = "$0 comb_file_ source_id_(SOURCE|SOURCECOMBO) source_shc_\n";

if($#ARGV < 2)
{
  print $USAGE;
  exit(1);
}

my $comb_file_ = shift;
my $source_id_ = shift;
my $source_shc_ = shift;

my $uid_ = `date +%N`; chomp($uid_);
my $tmp_file_ = "tmp_".$uid_;

open COMB_FILE, "< $comb_file_" or PrintStacktraceAndDie("can't open file $comb_file_ for reading\n");
open TMP_FILE, "> $tmp_file_" or PrintStacktraceAndDie("can't open file $tmp_file_ for writing\n");

while( my $line_ = <COMB_FILE> )
{
  chomp($line_);
  my @words_ = split( ' ', $line_);
  if($#words_ >= 0)
  {
    if($source_id_ eq $words_[0]) 
    {
      my $found_ = 0;
      for(my $i=1; $i<=$#words_; $i++)
      {
        if($source_shc_ eq $words_[$i])
        {
          $found_ = 1;
          last;
        }
      }
      if(! $found_)
      {
        $line_ = $line_." $source_shc_";
      }
    }
  }
  print TMP_FILE "$line_\n"; 
}
close(COMB_FILE);
close(TMP_FILE);

`mv $tmp_file_ $comb_file_`;


