# \file GenPerlLib/permute_params.pl
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
require "$GENPERLLIB_DIR/strat_utils.pl"; # CheckIfRegimeParam

# Usage :
# PermuteParams (SRC_FILE, DEST_DIR)
# E.g. PermuteParams ("/home/dvcinfra/strat_options.txt", "/spare/local/dvctrader")
# will create files containing all permutations in directory '/spare/local/dvctrader'. 
# Files will be named strat_options.txt_1, strat_options.txt_2 ...

sub PermuteParamsConstrained
{
    my $src_file_ = shift;
    my $dest_dir_ = shift;
    my $thres_min_increment_ = shift ;
    my $max_values_per_fields_ = shift ;
    # Add the trailing "/" to dest if not already there.
    print $src_file_." ".$dest_dir_."\n";
    if (substr ($dest_dir_, -1, 1) ne '/') 
    {
	$dest_dir_ = $dest_dir_."/";
    }
    my $ors_indicator_string_ = "";   

    # First find only the file name (remove absolute path)
    my $base_file_name_ = basename ( $src_file_ ) ; chomp ( $src_file_ );

    my @ret_file_names_ = ();
    if ( -e $src_file_ )
    {
	open SRC_FILE_, "<", $src_file_ or PrintStacktraceAndDie ( "Could not open file : $src_file_\n" );
	

	my $total_files_ = 1;
	my $output_string_ = "";
	my %param_key_to_count_ = ();
	my @comment_lines_ = ();
        my @param_lines_ = <SRC_FILE_>;

	foreach my $line_ ( @param_lines_ ) 
	{
	    if (substr ($line_, 0, 1) eq '#')
	    {
		push ( @comment_lines_, $line_ ) ;
		next;
	    }

	    my @words_ = split (' ', $line_);

	    if ($#words_ < 2)
	    {
		next;
	    }

	    if ( $words_[1] eq "PRIMARY_ORS_INDICATOR" || $words_[1] eq "SECONDARY_ORS_INDICATOR" )
	    {
		$ors_indicator_string_ = $ors_indicator_string_.$line_."\n";
		next;
	    }

        }

        foreach my $line_ ( @param_lines_ )  
        {
          my @words_ = split (' ', $line_);

          if ($#words_ < 2)
          {
            next;
          }

          my $param_name_val_ = $words_[0]." ".$words_[1]." ";
          if ( $#words_ >= 2 ) 
          {
            if ( $words_[1] eq "ZEROPOS_KEEP" )
            {
              my $base_value_ = $words_[2] ;
              @words_ = ("PARAMVLAUE", "ZEROPOS_KEEP" ) ;
              my $num_ = 0 ;
              for ( my $j_ = 0 ; $j_ < 3 ; $j_++ )  { 
              for ( my $i_ = 0; $i_ < $max_values_per_fields_ * 5; $i_ ++ ) 
              {
                my @this_param_lines_ = () ;
                foreach my $next_line_ ( @param_lines_ ) 
                {
                  $next_line_ =~ s/^\s+|\s+$//g ;
                  if (substr ($next_line_, 0, 1) eq '#')
                  {
                    push ( @comment_lines_, $next_line_ ) ;
                    next;
                  }

                  if ( index ( $next_line_, "ZEROPOS_KEEP" ) >= 0 ) 
                  {
                    my @t_words_ = split ( " ", $next_line_ ) ;
                    if ( $#t_words_ >= 2 ) 
                    {
                      push ( @this_param_lines_,  "$t_words_[0] $t_words_[1] ".($t_words_[2] + 5 * $thres_min_increment_ * $i_) );
                    }
                  }
                  elsif ( index ( $next_line_, "PLACE_KEEP_DIFF" ) >= 0 ) 
                  {
                    my @t_words_ = split ( " ", $next_line_ ) ;
                    if ( $#t_words_ >= 2 ) 
                    {
                      push ( @this_param_lines_, "$t_words_[0] $t_words_[1] ".( $t_words_[2] + $thres_min_increment_ * $i_ ) ) ;
                    }
                  } 
                  elsif ( index ( $next_line_, "INCREASE_ZEROPOS_DIFF" ) >= 0 ) 
                  {
                    my @t_words_ = split ( " ",$next_line_ ) ;
                    if ( $#t_words_ >= 2 ) 
                    {
                      push ( @this_param_lines_, "$t_words_[0] $t_words_[1]  ".( $t_words_[2] + 2 * 1.1 * $thres_min_increment_ * $i_ )) ;
                    }
                  } 
                  elsif ( index ( $next_line_, "ZEROPOS_DECREASE_DIFF" ) >= 0 ) 
                  {
                    my @t_words_ = split ( " ", $next_line_ ) ;
                    if ( $#t_words_ >= 2 ) 
                    {
                      push ( @this_param_lines_, "$t_words_[0] $t_words_[1] ". ($t_words_[2] + 2 * 0.9 * $thres_min_increment_ * $i_  ) );
                    }
                  } 
                  elsif ( index ( $next_line_, "ALLOWED_TO_IMPROVE" ) >= 0  )
                  {
                    my @t_words_ = split ( " ", $next_line_ ) ;
                    if ( $#t_words_ >= 2 )
                    {
                      if ( $j_ == 0 ) 
                      {
                        push ( @this_param_lines_, "$t_words_[0] $t_words_[1] 0 " ) ;
                      }
                      else
                      {
                        push ( @this_param_lines_, "$t_words_[0] $t_words_[1] 1 " ) ;
                        push ( @this_param_lines_, "$t_words_[0] IMPROVE ".($j_) ) ; 
                      }
                    }
                  }
                  elsif ( index ( $next_line_, "IMPROVE" ) >= 0 )
                  {
                    push ( @this_param_lines_, $i_ * 0.2 );
                    next ;
                  }
                  else 
                  {
                    push ( @this_param_lines_, $next_line_ ) ;
                  }
                }

                my   $this_string_ = join ("\n", @this_param_lines_ );
                my $file_name_ = $dest_dir_.$base_file_name_."_".$num_;
                open FILE, ">", $file_name_ or PrintStacktraceAndDie ( "Could not write to file : $file_name_\n" );
                print FILE $this_string_;
                $num_++;
		    
                for ( my $comment_line_index_ = 0 ; $comment_line_index_ <= $#comment_lines_; $comment_line_index_ ++ )
                {
                  print FILE $comment_lines_[$comment_line_index_];
                }
                print FILE $ors_indicator_string_;
                close FILE ;
                push ( @ret_file_names_, $file_name_ ) ;
              }
              }
            }
	    # Find how many entries are to be permuted.

          }
        }
    }

    @ret_file_names_
}

1;

#my $param_ = $ARGV[0];
#my $DIR = $ARGV[1];
#my @val_ = PermuteParamsConstrained($param_, $DIR, 0.003, 20 );
#print "NumPram ".$#val_."\n";
#exit;
