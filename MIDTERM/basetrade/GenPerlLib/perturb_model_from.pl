# \file GenPerlLib/perturb_model_from.pl
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
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# Usage :
# PerturbModelFrom (CURRENT_BEST, SCALE, DEST_DIR)
# E.g. PerturbModelFrom (t_best_model, scale, "/spare/local/dvctrader")
# will create files containing all permutations in directory '/spare/local/dvctrader'. 
# Files will be named strat_options.txt_1, strat_options.txt_2 ...
sub PerturbModelFrom
{
    my $value_src_file_ = shift;
    my $model_scale_factor_ = shift;
    my $dest_dir_ = shift;

    # Add the trailing "/" to dest if not already there.
    if (substr ($dest_dir_, -1, 1) ne '/') 
    {
	$dest_dir_ = $dest_dir_."/";
    }

    my $orig_model_file_name_ = $dest_dir_."model_orig.txt";
    my $low_model_file_name_ = $dest_dir_."model_low.txt";
    my $high_model_file_name_ = $dest_dir_."model_high.txt";
    `cp $value_src_file_ $orig_model_file_name_`;
    
    open LOW_MODEL_FILEHANDLE, ">", $low_model_file_name_ or PrintStacktraceAndDie ( "Could not write to file : $low_model_file_name_\n" );
    open HIGH_MODEL_FILEHANDLE, ">", $high_model_file_name_ or PrintStacktraceAndDie ( "Could not write to file : $high_model_file_name_\n" );

    # temporary variables for the algorithm
    my $model_type_ = "LINEAR";

    my @created_model_filename_vec_ = ( $low_model_file_name_, $orig_model_file_name_, $high_model_file_name_ );

    if ( -e $orig_model_file_name_ ) 
      {
	open ORIG_MODEL_FILEHANDLE, "<", $orig_model_file_name_ or PrintStacktraceAndDie ( "Could not open file : $orig_model_file_name_\n" );
	
	while (my $mline_ = <ORIG_MODEL_FILEHANDLE>) 
	  {
	    chomp ( $mline_ );
	    if (substr ($mline_, 0, 1) eq '#') 
	    {
	      printf LOW_MODEL_FILEHANDLE "%s\n", $mline_ ;
	      printf HIGH_MODEL_FILEHANDLE "%s\n", $mline_ ;
	      next;
	    }
	    my @mwords_ = split (' ', $mline_);
	    if ( ( $#mwords_ >= 1 ) &&
		 ( $mwords_[0] eq "MODELMATH" ) )
	      {
		$model_type_ = $mwords_[1];
	      }
	    if ( ( $#mwords_ >= 2 ) &&
		 ( $mwords_[0] eq "INDICATOR" ) )
	      {
		if ( $model_type_ eq "SIGLR" )
		  {
		    my @t_coeff_words_ = split ( ':', $mwords_[1] );
		    if ( $#t_coeff_words_ >= 1 )
		      {
			my $orig_wt_ = $t_coeff_words_[1] ;
			
			$t_coeff_words_[1] = $orig_wt_ * $model_scale_factor_; # multiplying only beta for scaling the model in case of SIGLR
			$mwords_[1] = join ( ":", @t_coeff_words_ );		
			printf HIGH_MODEL_FILEHANDLE "%s\n", join ( " ", @mwords_ );
			
			$t_coeff_words_[1] = $orig_wt_ * 1/$model_scale_factor_; # multiplying only beta for scaling the model in case of SIGLR
			$mwords_[1] = join ( ":", @t_coeff_words_ );		
			printf LOW_MODEL_FILEHANDLE "%s\n", join ( " ", @mwords_ );
		      }
		  }
		if ( $model_type_ eq "LINEAR" )
		  {
		    my $orig_wt_ = $mwords_[1] ;

		    $mwords_[1] = $orig_wt_ * $model_scale_factor_; # multiplying only beta for scaling the model in case of SIGLR
		    printf HIGH_MODEL_FILEHANDLE "%s\n", join ( " ", @mwords_ );

		    $mwords_[1] = $orig_wt_ * 1/$model_scale_factor_; # multiplying only beta for scaling the model in case of SIGLR
		    printf LOW_MODEL_FILEHANDLE "%s\n", join ( " ", @mwords_ );
		  }
	      }
	    else
	      {
		printf LOW_MODEL_FILEHANDLE "%s\n", $mline_ ;
		printf HIGH_MODEL_FILEHANDLE "%s\n", $mline_ ;
	      }
	  }
	close ( ORIG_MODEL_FILEHANDLE );
	close ( LOW_MODEL_FILEHANDLE );
	close ( HIGH_MODEL_FILEHANDLE );
      }
    @created_model_filename_vec_;
  }

#if ( $0 eq __FILE__ ) {
#  if ( $#ARGV >= 2 ) {
#    print join ( ' ', PerturbModelFrom ( $ARGV[0], $ARGV[1], $ARGV[2] ) );
#  }
#}

1;
