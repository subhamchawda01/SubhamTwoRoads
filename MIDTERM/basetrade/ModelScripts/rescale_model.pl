#!/usr/bin/perl

# \file ModelScripts/rescale_model.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
# modelfile newmodelfile scale_constant

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 prevmodelfile newmodelfile scale_const";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $model_filename_ = $ARGV[0];
my $new_model_filename_ = $ARGV[1];
my $scale_const_ = $ARGV[2];

open MFILE, "< $model_filename_" or PrintStacktraceAndDie ( "Could not open model_filename_ $model_filename_ for reading\n" );
open NEWMFILE, "> $new_model_filename_" or PrintStacktraceAndDie ( "Could not open $new_model_filename_ for writing\n" );

my $exec_cmd_ = "cat $model_filename_ | grep MODELMATH | awk '{print \$2}'";
my $model_math_line_ = `$exec_cmd_`;
chomp($model_math_line_);
my $model_type_ = $model_math_line_;

while ( my $mline_ = <MFILE> )
{
    chomp ( $mline_ );
    if ( $mline_ =~ /INDICATOR / )
    {
	my @mwords_ = split ( ' ', $mline_ );
	if ( $#mwords_ >= 2 )
	{
	    if ( $model_type_ eq "SIGLR" || $model_type_ eq "SELECTIVESIGLR" )
	    {
		my @t_coeff_words_ = split ( ':', $mwords_[1] );
		$t_coeff_words_[1] *= $scale_const_; # multiplying only beta for scaling the model in case of SIGLR
		$mwords_[1] = join ( ":", @t_coeff_words_ );		
	    }
	    elsif ( $model_type_ eq "LINEAR" || $model_type_ eq "SELECTIVENEW" )
	    {
	        $mwords_[1] *= $scale_const_;
	    }
	    printf NEWMFILE "%s\n", join ( " ", @mwords_ );
	}
    }
    elsif($mline_ =~ /StDev: / ){
	
	my @mwords_ = split ( ' ', $mline_ );
	$mwords_[5] *= $scale_const_;
	printf NEWMFILE "%s\n", join ( " ", @mwords_ );
    }
    else
    {
	printf NEWMFILE "%s\n", $mline_;
    }

}

close NEWMFILE;
close MFILE;
