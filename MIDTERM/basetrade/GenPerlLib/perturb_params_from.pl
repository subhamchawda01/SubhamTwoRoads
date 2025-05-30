# \file GenPerlLib/perturb_params_from.pl
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
# PerturbParamsFrom (CURRENT_BEST, STEP_SRC_FILE, DEST_DIR)
# E.g. PerturbParamsFrom (t_best_param, orig_param, "/spare/local/dvctrader")
# will create files containing all permutations in directory '/spare/local/dvctrader'. 
# Files will be named strat_options.txt_1, strat_options.txt_2 ...
sub PerturbParamsFrom
{
    my $value_src_file_ = shift;
    my $permutaion_src_file_ = shift;
    my $dest_dir_ = shift;

    # Add the trailing "/" to dest if not already there.
    if (substr ($dest_dir_, -1, 1) ne '/') 
    {
	$dest_dir_ = $dest_dir_."/";
    }

    # First find only the file name (remove absolute path)
    my $base_file_name_ = basename ( $value_src_file_ ) ;

    # temporary variables for the algorithm
    my %param_to_current_best_value_map_ ; # for non comment lines
    my %param_to_current_trailing_text_map_ ; # for non comment lines
    my @param_vec_ = (); # to preserve order of PARAMVALUE lines
    my @comment_lines_ = (); # non PARAMVALUE lines

    my @created_param_filename_vec_ = ();
    if ( ( -e $value_src_file_ ) &&
	 ( -e $permutaion_src_file_ ) )
      {
	open VALUE_SRC_FILEHANDLE, "<", $value_src_file_ or PrintStacktraceAndDie ( "Could not open file : $value_src_file_\n" );

	while (my $line_ = <VALUE_SRC_FILEHANDLE>) 
	{
	    chomp ( $line_ );
	    if (substr ($line_, 0, 1) eq '#') 
	    {
		push ( @comment_lines_, $line_ ) ;
		next;
	    }
	    my @words_ = split (' ', $line_);
	    
	    if ($#words_ < 2) 
	    {
#		printf "Ignoring Malformed line : %s\n", $line_;
		next;
	    }
	    if ( (substr ($words_[0], 0, 1) eq '#') ||
		 ( $words_[0] ne "PARAMVALUE" ) )
	    {
		push ( @comment_lines_, $line_ ) ;
		next;
	    }

	    # words_[0] == PARAMVALUE
	    my $param_name_ = $words_[1] ;
	    my $param_val_ = $words_[2] ;
	    push ( @param_vec_, $param_name_ );
	    $param_to_current_best_value_map_ { $param_name_ } = $param_val_ ;

	    # Find how many entries are to be perturbed.
	    # Actual useable entries are precomment words -2 this due to PARAMVALUE & KEY_NAME.
	    my $useable_entries_ = 0;
	    for ($useable_entries_ = 0; $useable_entries_ + 2 <= $#words_ && substr ($words_[$useable_entries_ + 2], 0, 1) ne '#'; $useable_entries_++) 
	    {
	    }

	    my $trailing_text_in_line_ = "";
	    for (my $comment_word_index_ = 2 + $useable_entries_; $comment_word_index_ <= $#words_; $comment_word_index_++) 
	    {
		$trailing_text_in_line_ = $trailing_text_in_line_." ".$words_[$comment_word_index_];
	    }
	  if ( length ( $trailing_text_in_line_ ) >= 1 )
	    {
	      $param_to_current_trailing_text_map_ { $param_name_ } = $trailing_text_in_line_ ;
	    }
	}
	close VALUE_SRC_FILEHANDLE;

	my $printed_current_best_already_ = 0; # a boo set to 0 as long as current best has not been printed

	open PERMUTATION_SRC_FILEHANDLE, "<", $permutaion_src_file_ or PrintStacktraceAndDie ( "Could not open file : $permutaion_src_file_\n" );
	while (my $line_ = <PERMUTATION_SRC_FILEHANDLE>) 
	  {
	    chomp ( $line_ );
	    # my ( $precomment_text_, $postcomment_part_ ) = split ('#', $line_, 2);
	    # try splitting on '#'

	    if (substr ($line_, 0, 1) eq '#') 
	      {
		next;
	      }
	    my @words_ = split (' ', $line_);
	    if ($#words_ < 2) 
	      { 
		next;
	      }
	    if (substr ($words_[0], 0, 1) eq '#') 
	      {
		next;
	      }
	    my $this_param_name_ = $words_[1] ;
	    my %param_to_try_value_map_ = %param_to_current_best_value_map_;
	    for ( my $param_value_index_ = 0 ; 2 + $param_value_index_ <= $#words_ ; $param_value_index_ ++ )
	      {
		# for each word test if it is a '#'
		# else
		# update the %param_to_try_value_map_
		# write a new param file
		
		my $this_param_val_ = $words_[2 + $param_value_index_] ;
		if ( substr ( $this_param_val_, 0, 1 ) eq '#' )
		  { # break out of loop 
		    last; 
		  }
		if ( exists ( $param_to_current_best_value_map_ { $this_param_name_ } ) )
		  { # this should be true for all
		    if ( $param_to_current_best_value_map_ { $this_param_name_ } eq $this_param_val_ )
		      {
			if ( $printed_current_best_already_  == 1 )
			  {
			    next; # skip this since it has been printed already
			  }
			else
			  {
			    $printed_current_best_already_ = 1; # print best only once 
			  }
		      }
		    $param_to_try_value_map_{$this_param_name_} = $this_param_val_;
		    
		    # output to file
		    my $this_param_file_name_ = $dest_dir_.$base_file_name_."_".( $#created_param_filename_vec_ + 1 );
		    
		    open THIS_OUTPUT_PARAM_FILEHANDLE, ">", $this_param_file_name_ or PrintStacktraceAndDie ( "Could not write to file : $this_param_file_name_\n" );
		    for ( my $param_vec_index_ = 0 ; $param_vec_index_ <= $#param_vec_; $param_vec_index_ ++ )
		      {
			my $pr_param_name_ = $param_vec_[$param_vec_index_];
			if ( exists $param_to_try_value_map_{$pr_param_name_} )
			  {
			    if ( exists $param_to_current_trailing_text_map_{$pr_param_name_} )
			      {
				printf THIS_OUTPUT_PARAM_FILEHANDLE "PARAMVALUE %s %s # %s\n", $pr_param_name_, $param_to_try_value_map_ { $pr_param_name_ }, $param_to_current_trailing_text_map_ { $pr_param_name_ } ;
			      }
			    else
			      {
				printf THIS_OUTPUT_PARAM_FILEHANDLE "PARAMVALUE %s %s\n", $pr_param_name_, $param_to_try_value_map_ { $pr_param_name_ } ;
			      }
			  }
		      }
		    # once we are done with params, print comments
		    for ( my $comment_line_index_ = 0 ; $comment_line_index_ <= $#comment_lines_; $comment_line_index_ ++ )
		      {
			printf THIS_OUTPUT_PARAM_FILEHANDLE "%s\n", $comment_lines_[$comment_line_index_];
		      }
		    close THIS_OUTPUT_PARAM_FILEHANDLE;
		    push ( @created_param_filename_vec_, $this_param_file_name_ );
		  }
	      }
	  }
      }
    @created_param_filename_vec_;
  }

#if ( $0 eq __FILE__ ) {
#  if ( $#ARGV >= 2 ) {
#    print join ( ' ', PerturbParamsFrom ( $ARGV[0], $ARGV[1], $ARGV[2] ) );
#  }
#}

1;
