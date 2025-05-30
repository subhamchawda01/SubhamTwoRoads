#!/usr/bin/perl

# \file ModelScripts/place_coeffs_in_model.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# output_model_filename_  indicator_list_filename_  regression_output_filename_

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_numeric_value_from_line.pl"; # GetNumericValueFromLine ;

# start 
my $USAGE="$0  output_model_filename  indicator_list_filename  regression_output_filename";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $output_model_filename_ = $ARGV[0];
my $indicator_list_filename_ = $ARGV[1];
my $regression_output_filename_ = $ARGV[2];

my $ilist_pre_indicator_text_="";
my $ilist_post_indicator_text_="";
my @indicator_trailing_text_=();

open ILIST, "< $indicator_list_filename_" or PrintStacktraceAndDie ( "Could not open indicator_list_filename_ $indicator_list_filename_ for reading\n" );
my $mode_ = -1;
while ( my $iline_ = <ILIST> )
{
    if ( $mode_ eq -1 ) 
    {
	$ilist_pre_indicator_text_ .= $iline_;

	# check if mode changing to 0 ... that in indicator reading mode
	my @iwords_ = split ' ', $iline_;
	if ( ( $#iwords_ >= 0 ) &&
	     ( $iwords_[0] eq "INDICATORSTART" ) )
	{
	    $mode_ = 0;
	}
    }
    elsif ( $mode_ eq 0 )
    {
	# process indicator line
	my @iwords_ = split ' ', $iline_;
	if ( ( $#iwords_ >= 0 ) &&
	     ( $iwords_[0] eq "INDICATOR" ) )
	{
	    splice ( @iwords_, 0, 2 );
	    push ( @indicator_trailing_text_, join ( ' ', @iwords_ ) );
	}
	elsif ( $iwords_[0] eq "INDICATOREND" )
	{
	    $mode_ = 1;
	    $ilist_post_indicator_text_ .= $iline_;
	}
    }
    elsif ( $mode_ eq 1 )
    {
	$ilist_post_indicator_text_ .= $iline_;
    }
}
close ILIST;



my @regression_coefficients_=();
open REGOUTFILE, "< $regression_output_filename_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_filename_ for reading\n" );
my ($model_corr_, $model_stdev_, $model_rsquared_, $model_corr_HDV_, $model_error_auto_corr_, $model_pred_auto_corr_ )=(-1.0, -1.0, -1.0, -1.0, -1.0 ,-1.0);
my $model_size_= 0;
while ( my $regline_ = <REGOUTFILE> )
{
    my @regwords_ = split ' ', $regline_;
    my ($this_index_, $this_coeff_, $this_init_corr_, $this_ts_, $this_auto_corr_,$this_dep_index_)=(-1,-1,-1,-1,-1,0);
    if ( ( $#regwords_ >= 2 ) &&
	 ( $regwords_[0] eq "OutCoeff" ) )
    {
	$this_index_ = int($regwords_[1]); # This refers to the index of the indicator. The assumption is that hte indicator will exist at this index.
	if ( $this_index_ > $#indicator_trailing_text_ )
	{ # invalid index
	    close REGOUTFILE ;
	    PrintStacktraceAndDie ( "PlaceCoeffsInModel : Ilist index $this_index_ from $regression_output_filename_ out of bounds of ilist $indicator_list_filename_\n" );
	}

	$this_coeff_ = $regwords_[2];

	$this_init_corr_ = GetNumericValueFromLine ( "InitCorrelation", \@regwords_ );
	$this_ts_ = GetNumericValueFromLine ( "Tstat", \@regwords_ );
	$this_auto_corr_ = GetNumericValueFromLine ( "AutoCorrelation", \@regwords_ );
	$this_dep_index_ = GetNumericValueFromLine ( "DepIndex", \@regwords_ );
#	print "vals: $this_init_corr_\t$this_ts_\t$this_auto_corr_\n";
	my $outline_ = sprintf("INDICATOR %s %s # IC: %.3f TS: %.1f AC: %.2f DI: %d\n", $this_coeff_, $indicator_trailing_text_[$this_index_], $this_init_corr_, $this_ts_, $this_auto_corr_, $this_dep_index_ );
#	my $outline_ = "INDICATOR ".$this_coeff_." ".$indicator_trailing_text_[$this_index_]." # IC: $this_init_corr_ TS: $this_ts_ AC: $this_auto_corr_ \n" ;
	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;
	$model_size_++;
    }
    elsif ( $regwords_[0] eq "RSquared" )    { $model_rsquared_ = $regwords_[1]; }
    elsif ( $regwords_[0] eq "Correlation" ) { $model_corr_     = $regwords_[1]; }
    elsif ( $regwords_[0] eq "StdevModel" )  { $model_stdev_    = $regwords_[1]; }
    elsif ( $regwords_[0] eq "HDVCorr" )	 { $model_corr_HDV_ = $regwords_[1]; }
    elsif ( $regwords_[0] eq "ErrorAutoCorrelation" ) { $model_error_auto_corr_ = $regwords_[1]; }
    elsif ( $regwords_[0] eq "PredictionAutoCorrelation" ) { $model_pred_auto_corr_ = $regwords_[1]; }
}

chomp($ilist_post_indicator_text_);
if ( $model_corr_HDV_ < 0.0 )
{
$ilist_post_indicator_text_ .= " # Corr: $model_corr_ StDev: $model_stdev_ RS: $model_rsquared_\n";
}
else
{
$ilist_post_indicator_text_ .= " # Corr: $model_corr_ HDV_Corr: $model_corr_HDV_ StDev: $model_stdev_ RS: $model_rsquared_\n";
}


### for debugging..
if ( 0 )
{
if ( $model_size_ >0 && ($model_rsquared_ <= 0  || $model_corr_ == 0.0 ) )
{
    my $instructionfilename_ = $regression_output_filename_;
    $instructionfilename_ =~ s/.*\/(.*)\/(.*\/){2}.*$/$1/g ;
    my $gen_strat_dir_ = $regression_output_filename_;
    #$gen_strat_dir_ =~ s/.*\/(.*)\/(.*\/){1}.*$/$1/g ;
    $gen_strat_dir_ = `dirname $regression_output_filename_ | xargs dirname` ; chomp ( $gen_strat_dir_ );
    my $error_bak_dir_ = `dirname $gen_strat_dir_ | xargs dirname `; chomp( $error_bak_dir_ ); $error_bak_dir_ .= "/error_dir/";
    #`mkdir -p $error_bak_dir_; cp -r $gen_strat_dir_ $error_bak_dir_;`;
    my $mail_body_ = "\n\nRegOutFile: $regression_output_filename_\n";
    $mail_body_ .= "$ilist_pre_indicator_text_"."$ilist_post_indicator_text_\n";
    #$mail_body_ .= "Data copied to $error_bak_dir_/\n";
    #$mail_body_ .= "mkdir -p $error_bak_dir_; cp -r $gen_strat_dir_ $error_bak_dir_;\n"; 
    open(MAIL, "|/usr/sbin/sendmail -t");
    my $hostname_=`hostname`;
    ## Mail Header
    print MAIL "To: nseall@tworoads.co.in\n";
    print MAIL "From: $USER\@$hostname_";
    print MAIL "Subject: ZeroCorr ERROR ( $instructionfilename_ ) $hostname_";
    ## Mail Body
    print MAIL $mail_body_ ;
    
    close(MAIL);
    
}


### for checking values of autocorrelation..
{
    my $instructionfilename_ = $regression_output_filename_;
    $instructionfilename_ =~ s/.*\/(.*)\/(.*\/){2}.*$/$1/g ;
    my $gen_strat_dir_ = $regression_output_filename_;
    $gen_strat_dir_ = `dirname $regression_output_filename_ | xargs dirname` ; chomp ( $gen_strat_dir_ );
    my $error_bak_dir_ = `dirname $gen_strat_dir_ | xargs dirname `; chomp( $error_bak_dir_ ); $error_bak_dir_ .= "/error_dir/";
    my $mail_body_ = "\n-----\nRegOutFile: $regression_output_filename_\n";
    $mail_body_ .= "$ilist_pre_indicator_text_"."$ilist_post_indicator_text_\n"."ErrorAutoCorrelation : $model_error_auto_corr_ ; PredictionsAutoCorrelation : $model_pred_auto_corr_ \n";
    open(MAIL, "|/usr/sbin/sendmail -t");
    my $hostname_=`hostname`;
    print MAIL "To: ankit\@tworoads.co.in\n";  # just for few days check
    print MAIL "From: $USER\@$hostname_";
    print MAIL "Subject: AutoCorrCheck ( $instructionfilename_ ) $hostname_";
    print MAIL $mail_body_ ;

    close(MAIL);
}
}

###  -- END -- 
close REGOUTFILE;

if ( $model_size_ > 0 ) {
    open OUTMODEL, "> $output_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $output_model_filename_ for writing\n" );
    print OUTMODEL $ilist_pre_indicator_text_;
    print OUTMODEL $ilist_post_indicator_text_;
    close OUTMODEL;
}
else {
    print "-1" ;
}

exit ( 0 );

