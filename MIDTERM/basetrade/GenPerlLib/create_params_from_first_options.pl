# \file GenPerlLib/create_params_from_first_options.pl
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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# Usage :
# CreateParamsFromFirstOptions ( FILE_WITH_PERTURBATIONS, OUTPUT_FILE )
sub CreateParamsFromFirstOptions
{
    my $permutaion_src_file_ = shift;
    my $output_param_filename_ = shift;

    if ( -e $permutaion_src_file_ )
      {
	open OUTPUT_PARAM_FILEHANDLE, ">", $output_param_filename_ or PrintStacktraceAndDie ( "Could not open file : $output_param_filename_\n" );

	open PERMUTATION_SRC_FILEHANDLE, "<", $permutaion_src_file_ or PrintStacktraceAndDie ( "Could not open file : $permutaion_src_file_\n" );
	while (my $line_ = <PERMUTATION_SRC_FILEHANDLE>) 
	  {
	    chomp ( $line_ );
	    # my ( $precomment_text_, $postcomment_part_ ) = split ('#', $line_, 2);
	    # try splitting on '#'
	    
	    if (substr ($line_, 0, 1) eq '#') 
	    {
		print OUTPUT_PARAM_FILEHANDLE "$line_\n";
		next;
	    }
	    my @words_ = split (' ', $line_);
	    if ($#words_ < 2) 
	    { 
		print OUTPUT_PARAM_FILEHANDLE "$line_\n";
		next;
	    }
	    if (substr ($words_[0], 0, 1) eq '#') 
	    {
		print OUTPUT_PARAM_FILEHANDLE "$line_\n";
		next;
	    }
	    
	    if ( $words_[0] eq "PARAMVALUE" ) {
		if ( $#words_ >= 2 ) {
		    printf OUTPUT_PARAM_FILEHANDLE "%s %s %s\n", $words_[0], $words_[1], $words_[2];
		}
		else {
		    print OUTPUT_PARAM_FILEHANDLE "$line_\n";
		    next;
		}
	    } else {
		print OUTPUT_PARAM_FILEHANDLE "$line_\n";
		next;
	    }
	  }
	close ( PERMUTATION_SRC_FILEHANDLE );
	close ( OUTPUT_PARAM_FILEHANDLE );
      }
}

#if ( $0 eq __FILE__ ) {
#  if ( $#ARGV >= 2 ) {
#    print join ( ' ', CreateParamsFromFirstOptions ( $ARGV[0], $ARGV[1] ) );
#  }
#}

1;
