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
my $TMP_LOC="/tmp" ; 

my $t_=`date +%N |cut -c6-`;chomp($t_); #to allow temp files for multiple genstrats 

my $regression_output_linear_filename_temp_="$TMP_LOC/linear_reg_out.txt".$t_ ;
my $regression_output_hinged_linear_filename_temp_="$TMP_LOC/hinged_linear_reg_out.txt".$t_ ;
my $regression_output_hinged_deg2_filename_temp_="$TMP_LOC/hinged_deg2_reg_out.txt".$t_ ;

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0  output_model_linear_filename output_model_hinged_linear_filename  output_model_hinged_deg2_filename indicator_list_filename  regression_linear_output_filename regression_hinged_linear_output_filename regression_hinged_deg2_output_filename ";

if ( $#ARGV < 6 ) { print $USAGE."\n"; exit ( 0 ); }

my $output_model_linear_filename_ = $ARGV[0];
my $output_model_hinged_linear_filename_ = $ARGV[1];
my $output_model_hinged_deg2_filename_ = $ARGV[2];

my $indicator_list_filename_ = $ARGV[3];

my $regression_output_linear_filename_ = $ARGV[4];
my $regression_output_hinged_linear_filename_ = $ARGV[5];
my $regression_output_hinged_deg2_filename_ = $ARGV[6];

my $ilist_pre_indicator_text_="";
my $ilist_post_indicator_text_="";
my @indicator_trailing_text_=();

my $START_INDEX_ILIST_ = 2 ;

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
	else
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

my $common_ilist_pre_indicator_text_ = $ilist_pre_indicator_text_ ;
my $common_ilist_post_indicator_text_ = $ilist_post_indicator_text_ ;

`$HOME_DIR/$REPO/scripts/generate_linear_unscale_model_temp.sh $regression_output_linear_filename_ $regression_output_linear_filename_temp_` ;
`$HOME_DIR/$REPO/scripts/generate_hinged_linear_model_temp.sh $regression_output_hinged_linear_filename_ $regression_output_hinged_linear_filename_temp_` ;
`$HOME_DIR/$REPO/scripts/generate_hinged_deg2_model_temp.sh $regression_output_hinged_deg2_filename_ $regression_output_hinged_deg2_filename_temp_` ;

open REGOUTFILE, "< $regression_output_linear_filename_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_linear_filename_ for reading\n" );

my $ilist_comment_indicator_text_="" ;

while ( my $regline_ = <REGOUTFILE> )
{
    my @regwords_ = split ' ', $regline_;

    if ( ( $#regwords_ >= 1 ) && ( index ( $regwords_[0], "Intercept" ) != -1 ) ) 
    {

      my $intercept_ = sprintf( "%.10f", $regwords_[1] ) ; 
      $ilist_pre_indicator_text_ = $ilist_pre_indicator_text_."INTERCEPT ".$intercept_."\n" ;
      last ;

    }

}

close REGOUTFILE;



my @regression_coefficients_=();
open REGOUTFILE, "< $regression_output_linear_filename_temp_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_linear_filename_temp_ for reading\n" );
my ($model_corr_, $model_stdev_, $model_rsquared_)=(-1.0, -1.0, -1.0);
while ( my $regline_ = <REGOUTFILE> )
{
    my @regwords_ = split ' ', $regline_;

    my ($this_index_, $this_coeff_, $this_init_corr_, $this_ts_)=(-1,-1,-1,-1);

    if ( ( $#regwords_ >= 2 ) &&
	 ( $regwords_[0] eq "OutCoeff" ) ) 
    {
	$this_index_ = ( int($regwords_[1]) ) - $START_INDEX_ILIST_ ; 
	$this_coeff_ = $regwords_[2];

	my $outline_ = "INDICATOR ".$this_coeff_." ".$indicator_trailing_text_[$this_index_]." # IC: $this_init_corr_ TS: $this_ts_\n" ;
	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;
    }

}

open REGOUTFILE, "< $regression_output_linear_filename_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_linear_filename_ for reading\n" );

$ilist_comment_indicator_text_="" ;

while ( my $regline_ = <REGOUTFILE> )
{

    if ( index ( $regline_, "Importance" ) == 0 ) { $ilist_comment_indicator_text_ = $ilist_comment_indicator_text_."#".$regline_  } 
    if ( index ( $regline_, "RSq" ) != -1 ) { $ilist_comment_indicator_text_ = $ilist_comment_indicator_text_."#".$regline_ } 

}

close REGOUTFILE;

chomp($ilist_post_indicator_text_);
$ilist_post_indicator_text_ .= " # Corr: $model_corr_ StDev: $model_stdev_ RS: $model_rsquared_\n";


open OUTMODEL, "> $output_model_linear_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $output_model_linear_filename_ for writing\n" );
print OUTMODEL $ilist_pre_indicator_text_;
print OUTMODEL $ilist_post_indicator_text_;
print OUTMODEL $ilist_comment_indicator_text_;
close OUTMODEL;

@regression_coefficients_=();
my $new_indicator_output_="" ;
my %index_to_indicator_ = () ;
my $index_counter_ = 0 ;

$ilist_pre_indicator_text_=$common_ilist_pre_indicator_text_ ;
$ilist_comment_indicator_text_="" ;
$ilist_post_indicator_text_=$common_ilist_post_indicator_text_ ;

$ilist_pre_indicator_text_=~s/LINEAR/NONLINEAR/g;

open REGOUTFILE, "< $regression_output_hinged_linear_filename_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_hinged_linear_filename_ for reading\n" );

while ( my $regline_ = <REGOUTFILE> )
{

    my @regwords_ = split ' ', $regline_;

    if ( ( $#regwords_ >= 1 ) && ( index ( $regwords_[0], "Intercept" ) != -1 ) ) 
    {

      my $intercept_ = sprintf( "%.10f", $regwords_[1] ) ; 
      $ilist_pre_indicator_text_ = $ilist_pre_indicator_text_."INTERCEPT ".$intercept_."\n" ;
      last ;

    }

}

close REGOUTFILE;

open REGOUTFILE, "< $regression_output_hinged_linear_filename_temp_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_hinged_linear_filename_temp_ for reading\n" );
($model_corr_, $model_stdev_, $model_rsquared_)=(-1.0, -1.0, -1.0);

while ( my $regline_ = <REGOUTFILE> )
{
    my @regwords_ = split ' ', $regline_;

    my ($this_index_, $constant, $this_coeff_, $this_init_corr_, $this_ts_)=(-1,-1,-1,-1,-1);

    if ( ( $#regwords_ >= 3 ) &&
	 ( $regwords_[0] eq "OutCoeff" ) ) 
    {
	$this_index_ = ( int($regwords_[1]) ) - $START_INDEX_ILIST_ ;
        $constant = sprintf( "%.10f", $regwords_[2] ) ;
	$this_coeff_ = sprintf( "%.10f", $regwords_[3] ) ;

	my $outline_ = "INDICATOR 1.00"." ".$indicator_trailing_text_[$this_index_]." # IC: $this_init_corr_ TS: $this_ts_\n" ;

	if ( exists $index_to_indicator_{ $this_index_ } ) {

	    next ;

	}

        $index_to_indicator_{ $this_index_ } = $index_counter_ ;
        $index_counter_ = $index_counter_ + 1 ;

	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;


    }

    elsif ( ( $#regwords_ >= 3 ) && ( $regwords_[1] eq "OutCoeff" ) ) 
    {

	$this_index_ = ( int($regwords_[2]) ) - $START_INDEX_ILIST_ ;
        $constant = sprintf( "%.10f", $regwords_[0] ) ;
	$this_coeff_ = sprintf( "%.10f", $regwords_[3] ) ;

	my $outline_ = "INDICATOR 1.00"." ".$indicator_trailing_text_[$this_index_]." # IC: $this_init_corr_ TS: $this_ts_\n" ;

	if ( exists $index_to_indicator_{ $this_index_ } ) {

            next ;

	}

        $index_to_indicator_{ $this_index_ } = $index_counter_ ;
        $index_counter_ = $index_counter_ + 1 ;
	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;

    }

}

close REGOUTFILE;


open REGOUTFILE, "< $regression_output_hinged_linear_filename_temp_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_hinged_linear_filename_temp_ for reading\n" );
($model_corr_, $model_stdev_, $model_rsquared_)=(-1.0, -1.0, -1.0);

my $index_map_ = 0 ;

while ( my $regline_ = <REGOUTFILE> )
{
    my @regwords_ = split ' ', $regline_;

    my ($this_index_, $constant, $this_coeff_, $this_init_corr_, $this_ts_)=(-1,-1,-1,-1,-1);

    if ( ( $#regwords_ >= 3 ) &&
	 ( $regwords_[0] eq "OutCoeff" ) ) 
    {
	$this_index_ = ( int($regwords_[1]) ) - $START_INDEX_ILIST_ ;
        $constant = sprintf( "%.10f",-($regwords_[2]) ) ;  #sign reversed due to dash 
	$this_coeff_ = sprintf( "%.10f", $regwords_[3] ) ; 

        $index_map_ = $index_to_indicator_{ $this_index_ } ;

        my @indicator_words_ = split ' ', $indicator_trailing_text_[$this_index_] ;
	my $outline_ = "NONLINEARCOMPONENT ".$this_coeff_." ".$indicator_words_[0]." ".$index_map_." ".$constant." "."-1\n" ;

	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;

    }

    elsif ( ( $#regwords_ >= 3 ) && ( $regwords_[1] eq "OutCoeff" ) ) 
    {

	$this_index_ = ( int($regwords_[2]) ) - $START_INDEX_ILIST_ ; 
        $constant = sprintf( "%.10f", $regwords_[0] ) ;
	$this_coeff_ = sprintf( "%.10f", $regwords_[3] ) ; 

        $index_map_ = $index_to_indicator_{ $this_index_ } ;

        my @indicator_words_ = split ' ', $indicator_trailing_text_[$this_index_] ;
	my $outline_ = "NONLINEARCOMPONENT ".$this_coeff_." ".$indicator_words_[0]." ".$index_map_." ".$constant." "."1\n" ;

	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;

    }

}

close REGOUTFILE;

open REGOUTFILE, "< $regression_output_hinged_linear_filename_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_hinged_linear_filename_ for reading\n" );

$ilist_comment_indicator_text_="" ;

while ( my $regline_ = <REGOUTFILE> )
{

    if ( index ( $regline_, "Importance" ) == 0 ) { $ilist_comment_indicator_text_ = $ilist_comment_indicator_text_."#".$regline_  } 
    if ( index ( $regline_, "RSq" ) != -1 ) { $ilist_comment_indicator_text_ = $ilist_comment_indicator_text_."#".$regline_ } 

}


close REGOUTFILE;

chomp($ilist_post_indicator_text_);
$ilist_post_indicator_text_ .= " # Corr: $model_corr_ StDev: $model_stdev_ RS: $model_rsquared_\n";


open OUTMODEL, "> $output_model_hinged_linear_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $output_model_hinged_linear_filename_ for writing\n" );
print OUTMODEL $ilist_pre_indicator_text_;
print OUTMODEL $ilist_post_indicator_text_;
print OUTMODEL $ilist_comment_indicator_text_;
close OUTMODEL;

@regression_coefficients_=();
%index_to_indicator_ = () ;
$index_counter_ = 0 ;

$ilist_pre_indicator_text_=$common_ilist_pre_indicator_text_ ;
$ilist_comment_indicator_text_="" ;
$ilist_post_indicator_text_=$common_ilist_post_indicator_text_ ;

$ilist_pre_indicator_text_=~s/LINEAR/NONLINEAR/g;

open REGOUTFILE, "< $regression_output_hinged_deg2_filename_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_hinged_deg2_filename_ for reading\n" );

$ilist_comment_indicator_text_="" ;

while ( my $regline_ = <REGOUTFILE> )
{
    my @regwords_ = split ' ', $regline_;

    if ( ( $#regwords_ >= 1 ) && ( index ( $regwords_[0], "Intercept" ) != -1 ) ) 
    {

      my $intercept_ = sprintf( "%.10f", $regwords_[1] ) ; 
      $ilist_pre_indicator_text_ = $ilist_pre_indicator_text_."INTERCEPT ".$intercept_."\n" ;
      last ;

    }


}

close REGOUTFILE;



open REGOUTFILE, "< $regression_output_hinged_deg2_filename_temp_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_hinged_deg2_filename_temp_ for reading\n" );
($model_corr_, $model_stdev_, $model_rsquared_)=(-1.0, -1.0, -1.0);

while ( my $regline_ = <REGOUTFILE> )
{
    my @regwords_ = split ' ', $regline_;

    my $temp_word_ = $#regwords_ ;

    my ($this_index_, $constant, $this_coeff_, $this_init_corr_, $this_ts_)=(-1,-1,-1,-1,-1);

    if ( ( $#regwords_ == 3 ) &&
	 ( $regwords_[0] eq "OutCoeff" ) ) 
    {
	$this_index_ = ( int($regwords_[1]) ) - $START_INDEX_ILIST_ ;
        $constant = sprintf( "%.10f", -($regwords_[2] ) ) ;#sign reversed on replacing dash 
	$this_coeff_ = sprintf( "%.10f", ($regwords_[3]) ) ;  

	my $outline_ = "INDICATOR 1.00"." ".$indicator_trailing_text_[$this_index_]." # IC: $this_init_corr_ TS: $this_ts_\n" ;

        if ( exists $index_to_indicator_{ $this_index_ } ) {

            next ;

        }

        $index_to_indicator_{ $this_index_ } = $index_counter_ ;
        $index_counter_ = $index_counter_ + 1 ;
	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;


    }

    elsif ( ( $#regwords_ == 3 ) && ( $regwords_[1] eq "OutCoeff" ) ) 
    {

	$this_index_ = ( int($regwords_[2]) ) - $START_INDEX_ILIST_ ;
        $constant = sprintf( "%.10f", $regwords_[0] ) ;
	$this_coeff_ = sprintf( "%.10f", $regwords_[3] ) ;

	my $outline_ = "INDICATOR 1.00"." ".$indicator_trailing_text_[$this_index_]." # IC: $this_init_corr_ TS: $this_ts_\n" ;

        if ( exists $index_to_indicator_{ $this_index_ } ) {

            next ;

        }


        $index_to_indicator_{ $this_index_ } = $index_counter_ ;
        $index_counter_ = $index_counter_ + 1 ;
	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;

    }

    elsif ( ( $#regwords_ > 3 ) ) 
    {

	my $total_hinged_tokens_ = ( $#regwords_ - 1 ) % 3 == 0 ;

	if ( $total_hinged_tokens_ != 0 ) {

	    print "Malformatted Line : $regline_\n" ;
	    exit ; 

	}

	for ( my $counter_ = 0 ; $counter_ < $#regwords_ ; $counter_ += 3 ) {

	    if ( 
		( $regwords_[ $counter_ ] eq "OutCoeff" ) ) 
	    {
		$this_index_ = ( int($regwords_[ $counter_ + 1 ]) ) - $START_INDEX_ILIST_ ; 
		$constant = sprintf( "%.10f", -($regwords_[ $counter_ + 2 ]) ) ;

		my $outline_ = "INDICATOR 1.00"." ".$indicator_trailing_text_[$this_index_]." # IC: $this_init_corr_ TS: $this_ts_\n" ;

		if ( exists $index_to_indicator_{ $this_index_ } ) {

                    next ;

		}


                $index_to_indicator_{ $this_index_ } = $index_counter_ ;
                $index_counter_ = $index_counter_ + 1 ;
		$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;


	    }

	    elsif ( ( $regwords_[ $counter_ + 1 ] eq "OutCoeff" ) ) 
	    {

		$this_index_ = ( int($regwords_[ $counter_ + 2 ]) ) - $START_INDEX_ILIST_ ; 
		$constant = sprintf( "%.10f", $regwords_[ $counter_ ] ) ;

		my $outline_ = "INDICATOR 1.00"." ".$indicator_trailing_text_[$this_index_]." # IC: $this_init_corr_ TS: $this_ts_\n" ;

		if ( exists $index_to_indicator_{ $this_index_ } ) {

                    next ;

		}

		$index_to_indicator_{ $this_index_ } = $index_counter_ ;
		$index_counter_ = $index_counter_ + 1 ;
		$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;

	    }

	}

    }

}

close REGOUTFILE;

open REGOUTFILE, "< $regression_output_hinged_deg2_filename_temp_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_hinged_deg2_filename_temp_ for reading\n" );
($model_corr_, $model_stdev_, $model_rsquared_)=(-1.0, -1.0, -1.0);

$index_map_ = 0 ;

while ( my $regline_ = <REGOUTFILE> )
{
    my @regwords_ = split ' ', $regline_;

    my ($this_index_, $constant, $this_coeff_, $this_init_corr_, $this_ts_)=(-1,-1,-1,-1,-1);

    if ( ( $#regwords_ == 3 ) &&
	 ( $regwords_[0] eq "OutCoeff" ) ) 
    {
	$this_index_ = ( int($regwords_[1]) ) - $START_INDEX_ILIST_ ; 
        $constant = sprintf( "%.10f", -($regwords_[2]) ) ;
	$this_coeff_ = sprintf( "%.10f", ($regwords_[3]) ) ;

        $index_map_ = $index_to_indicator_{ $this_index_ } ;

        my @indicator_words_ = split ' ', $indicator_trailing_text_[$this_index_] ;
	my $outline_ = "NONLINEARCOMPONENT ".$this_coeff_." ".$indicator_words_[0]." ".$index_map_." ".$constant." "."-1\n" ;

	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;

    }

    elsif ( ( $#regwords_ == 3 ) && ( $regwords_[1] eq "OutCoeff" ) ) 
    {

	$this_index_ = ( int($regwords_[2]) ) - $START_INDEX_ILIST_ ; 
        $constant = sprintf( "%.10f", $regwords_[0] ) ;
	$this_coeff_ = sprintf( "%.10f", $regwords_[3] ) ;

        $index_map_ = $index_to_indicator_{ $this_index_ } ;

        my @indicator_words_ = split ' ', $indicator_trailing_text_[$this_index_] ;
	my $outline_ = "NONLINEARCOMPONENT ".$this_coeff_." ".$indicator_words_[0]." ".$index_map_." ".$constant." "."1\n" ;

	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;

    }

    elsif ( ( $#regwords_ > 3 ) ) 
    {

	my $total_hinged_tokens_ = ( $#regwords_ - 1 ) % 3 == 0 ;

	if ( $total_hinged_tokens_ != 0 ) {

	    print "Malformatted Line : $regline_\n" ;
	    exit ; 

	}

        $ilist_pre_indicator_text_ = $ilist_pre_indicator_text_."NONLINEARCOMPONENT" ;

        $this_coeff_= sprintf( "%.10f", $regwords_[ $#regwords_ ] ) ;
	$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_." ".$this_coeff_." " ;

	for ( my $counter_ = 0 ; $counter_ < $#regwords_ ; $counter_ += 3 ) {

	    if ( 
		( $regwords_[ $counter_ ] eq "OutCoeff" ) ) 
	    {
		$this_index_ = ( int($regwords_[ $counter_ + 1 ]) ) - $START_INDEX_ILIST_ ;
		$constant = sprintf( "%.10f", -($regwords_[ $counter_ + 2 ] ) ) ;

		$index_map_ = $index_to_indicator_{ $this_index_ } ;

		my @indicator_words_ = split ' ', $indicator_trailing_text_[$this_index_] ;
		my $outline_ = $indicator_words_[0]." ".$index_map_." ".$constant." "."-1"." " ;

		$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;


	    }

	    elsif ( ( $regwords_[ $counter_ + 1 ] eq "OutCoeff" ) ) 
	    {

		$this_index_ = ( int($regwords_[ $counter_ + 2 ]) ) - $START_INDEX_ILIST_ ; 
		$constant = sprintf( "%.10f", $regwords_[ $counter_ ] ) ;

		$index_map_ = $index_to_indicator_{ $this_index_ } ;

		my @indicator_words_ = split ' ', $indicator_trailing_text_[$this_index_] ;
		my $outline_ = $indicator_words_[0]." ".$index_map_." ".$constant." "."1"." " ;

		$ilist_pre_indicator_text_ = $ilist_pre_indicator_text_.$outline_;

	    }

	}

        $ilist_pre_indicator_text_ = $ilist_pre_indicator_text_."\n" ;

    }


}

close REGOUTFILE;

open REGOUTFILE, "< $regression_output_hinged_deg2_filename_" or PrintStacktraceAndDie ( "Could not open regression_output_filename_ $regression_output_hinged_deg2_filename_ for reading\n" );

$ilist_comment_indicator_text_="" ;

while ( my $regline_ = <REGOUTFILE> )
{

    if ( index ( $regline_, "Importance" ) == 0 ) { $ilist_comment_indicator_text_ = $ilist_comment_indicator_text_."#".$regline_  } 
    if ( index ( $regline_, "RSq" ) != -1 ) { $ilist_comment_indicator_text_ = $ilist_comment_indicator_text_."#".$regline_ } 

}

close REGOUTFILE;

chomp($ilist_post_indicator_text_);
$ilist_post_indicator_text_ .= " # Corr: $model_corr_ StDev: $model_stdev_ RS: $model_rsquared_\n";


open OUTMODEL, "> $output_model_hinged_deg2_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $output_model_hinged_deg2_filename_ for writing\n" );
print OUTMODEL $ilist_pre_indicator_text_;
print OUTMODEL $ilist_post_indicator_text_;
print OUTMODEL $ilist_comment_indicator_text_;
close OUTMODEL;

`rm -rf $regression_output_linear_filename_temp_`;
`rm -rf $regression_output_hinged_linear_filename_temp_`;
`rm -rf $regression_output_hinged_deg2_filename_temp_` ;



