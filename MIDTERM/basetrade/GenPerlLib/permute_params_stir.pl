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

sub PermuteParamsStir
{
    my $strat_file_ = $_[0];
    my @params_indices = @{ $_[1] };
    my @params = @{ $_[2] };
    my $dest_dir_ = $_[3];
    my @retval_ = ();

    # Add the trailing "/" to dest if not already there.
    if (substr ($dest_dir_, -1, 1) ne '/') 
    {
	$dest_dir_ = $dest_dir_."/";
    }

    for ( my $param_ind = 0 ; $param_ind <= $#params_indices ; $param_ind++ )
    {
      my $param_index = $params_indices [ $param_ind ];
      my $src_file_ = $params [ $param_ind ];


      my $nested_dest_dir_ = $dest_dir_.$param_index."/";

      if ( -e $nested_dest_dir_ )
      {
        print "Surprising but the directory for param of ouright index ".$param_index." exists.. deleting it\n";
        my $exec_cmd1_ = "rm -r $nested_dest_dir_";
        `$exec_cmd1_`;
      }

      mkdir $nested_dest_dir_ or PrintStacktraceAndDie ( "Error creating directory: $src_file_\n" );

      print $src_file_." ".$nested_dest_dir_."\n";
      my $ors_indicator_string_ = "";   

      # First find only the file name (remove absolute path)
      my $base_file_name_ = basename ( $src_file_ ) ; chomp ( $src_file_ );
  
      my @ret_file_names_ = ();
      my %orig_vals_ = ();

      if ( ! ($strat_file_ eq "INVALID") && -e $strat_file_ )
      {
        my $exec_cmd_ = "cat $strat_file_ | awk 'FNR == ($param_index + 1) { print \$4 }'";
        my $orig_src_file_ = `$exec_cmd_`;
        print "$orig_src_file_ \n";
        chomp($orig_src_file_);

        if ( -e $orig_src_file_ )
        {
          open ORIG_SRC_FILE_, "<", $orig_src_file_ or PrintStacktraceAndDie ( "Could not open file : $src_file_\n" );
  
          while (my $line_ = <ORIG_SRC_FILE_>)
          {
              if (substr ($line_, 0, 1) eq '#')
              {
                  next;
              }
  
              my @words_ = split (' ', $line_);
  
              if ($#words_ < 2)
              {
                  next;
              }
  
              $orig_vals_{ $words_[1] } = $words_[2] ;

#print "orig_vals: $words_[1] : $orig_vals_{ $words_[1] } \n";
          } 
        }
      }

      if ( -e $src_file_ )
      {
  	open SRC_FILE_, "<", $src_file_ or PrintStacktraceAndDie ( "Could not open file : $src_file_\n" );
  	
  
  	my $total_files_ = 1;
  	my @output_string_ = ();
  	my @orig_output_string_ = ();
  	my @comment_lines_ = ();
        my $nopermuteparams_ = 0;
  
  	while (my $line_ = <SRC_FILE_>)
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
  
  	    my $param_name_val_ = $words_[0]." ".$words_[1]." ";
  
  	    # Find how many entries are to be permuted.
  	    my $useable_entries_;
  	    for ($useable_entries_ = 2; $useable_entries_ <= $#words_ && substr ($words_[$useable_entries_], 0, 1) ne '#'; $useable_entries_++)
  	    {
  	    }
  
  	    # Actual useable entries are -2 this due to PARAMVALUE & KEY_NAME.
  	    $useable_entries_ -= 2;
  
  	    my $comment_line_ = "";
  	    for ( my $count_ = 2 + $useable_entries_; $count_ <= $#words_; $count_++ ) 
  	    {
  		$comment_line_ = $comment_line_." ".$words_[$count_];
  	    }
  
  	    if ( $words_[1] eq "PRIMARY_ORS_INDICATOR" || $words_[1] eq "SECONDARY_ORS_INDICATOR" )
  	    {
  		$ors_indicator_string_ = $ors_indicator_string_.$line_."\n";
  		next;
  	    }
  
  	    # Based on how many params-values follow this key, the no. of total perms may go up.
  	    # Simple permutation calc :
  	    $total_files_ = $total_files_ + ($useable_entries_ - 1);
            
            my $this_orig_val_ = "";
            if ( exists $orig_vals_ { $words_[1] } )
            {
              $this_orig_val_ = $orig_vals_ { $words_[1] };
# print "here $words_[1] : $this_orig_val_\n";
            }
            elsif ( $useable_entries_ > 0 )
            {
              $this_orig_val_ = $words_[2];
            }
#           print "orig $words_[1] : $this_orig_val_\n";
            push ( @orig_output_string_ , $param_name_val_.$this_orig_val_.$comment_line_ );
            
            my $this_output_string_ = "";
  	    for (my $count_ = 1; $count_ < $useable_entries_ + 2; $count_++) 
  	    {
              if ( $words_[$count_] eq $this_orig_val_ )
              {
                next;
              }
  	      $this_output_string_ = $this_output_string_.$param_name_val_.$words_[$count_].$comment_line_."|";
  	    }
            push ( @output_string_ , $this_output_string_ );
  	}
  
  	close SRC_FILE_;
  	my $MAX_FILES_TO_ALLOW = 500;
	if ( $total_files_ < $MAX_FILES_TO_ALLOW )
	{
	    # This map is used to store content for each of the total_files no. of files.
	    my %file_no_to_string_ = ();
            $file_no_to_string_{1} = join("\n", @orig_output_string_);
	    my $file_count_ = 2;
            for ( my $paramno_ = 0 ; $paramno_ <= $#output_string_ ; $paramno_++)
            {
              my @data_lines_ = split ('\|', $output_string_[ $paramno_ ]);

              if ($#data_lines_ == 0 )
              {
                next;
              }

	      for ( my $index_ = 1; $index_ <= $#data_lines_; $index_++ )
	      {
#                print "$param_ind $paramno_ $index_ $data_lines_[$index_]\n";
                my @cp_output_string_ = @orig_output_string_;
                $cp_output_string_ [$paramno_] = $data_lines_[$index_];

	        $file_no_to_string_{$file_count_} = join( "\n", @cp_output_string_ );
                $file_count_ ++;    
	      }
            }

	    # Open total_files no. of files with appropriate names.
	    # Array of file handles!
	    my $num_ = 1;
	    for (my $count_ = 1; $count_ <= $total_files_; $count_++) 
	    {
		my $file_name_ = "";

		if ($total_files_ == 1)
		{ # For 1 output file, dest file name is same as src file name.
		    $file_name_ = $nested_dest_dir_.$base_file_name_;
		}
		else
		{
		    $file_name_ = $nested_dest_dir_.$base_file_name_."_".$num_;
		}

		# if ( CheckValidParam( $file_no_to_string_{$count_}) )
		{
		    open FILE, ">", $file_name_ or PrintStacktraceAndDie ( "Could not write to file : $file_name_\n" );
		    print FILE $file_no_to_string_{$count_};
		    $num_++;
		    
		    for ( my $comment_line_index_ = 0 ; $comment_line_index_ <= $#comment_lines_; $comment_line_index_ ++ )
		    {
			print FILE $comment_lines_[$comment_line_index_];
		    }
		    print FILE $ors_indicator_string_;
		    close FILE;
		    push ( @ret_file_names_, $file_name_ );
		}
	    }
	}
      }
      push ( @retval_ , \@ret_file_names_ );
    }
    @retval_;
}

sub CheckValidParam
{
    my ($param_string_) = @_;
    my @param_lines_ = split('\n', $param_string_);
    my $increase_place_ = 100;
    my $increase_keep_ =  100;
    my $decrease_place_ = 100;
    my $decrease_keep_ = -00;
    my $zeropos_place_ = 100;
    my $zeropos_keep_ = 100;
    my $place_keep_diff_ = 100;
    my $increase_zeropos_diff_ = 100;
    my $zeropos_decrease_diff_ = 100;

    foreach my $line_ (@param_lines_)
    {
	if ( index ($line_, "INCREASE_PLACE") >= 0 ) 
	{
	    my @param_words_ = split(' ', $line_);	  
	    $increase_place_ = $param_words_[2];
	}

	if ( index ($line_, "INCREASE_KEEP") >= 0 )
	{
	    my @param_words_ = split(' ', $line_);
	    $increase_keep_ = $param_words_[2];
	}

	if ( index ($line_, "DECREASE_PLACE") >= 0 )
	{
	    my @param_words_ = split(' ', $line_);
	    $decrease_place_ = $param_words_[2];
	}
	
	if ( index ($line_, "DECREASE_KEEP") >= 0 )
	{
	    my @param_words_ = split(' ', $line_);
	    $decrease_keep_ = $param_words_[2];
	}
	
	if ( index ($line_, "ZEROPOS_PLACE") >= 0 )
	{
	    my @param_words_ = split(' ', $line_);
	    $zeropos_place_ = $param_words_[2];
	}
	
	if ( index ($line_, "ZEROPOS_KEEP") >= 0 )
	{
	    my @param_words_ = split(' ', $line_);
	    $zeropos_keep_ = $param_words_[2];
	}

	if ( index ( $line_, "INCREASE_ZEROPOS_DIFF" ) >= 0 )
	{
	    my @param_words_ = split(' ', $line_);
	    $increase_zeropos_diff_ = $param_words_[2];
	}

	if ( index ( $line_, "ZEROPOS_DECREASE_DIFF") >= 0 )
	{
	    my @param_words_ = split(' ', $line_);
	    $zeropos_decrease_diff_ = $param_words_[2];
	}

	if ( index ( $line_, "PLACE_KEEP_DIFF") >= 0 )
	{
	    my @param_words_ = split(' ', $line_);
	    $place_keep_diff_ = $param_words_[2];
	}
    }
    
    if ( ( $increase_keep_ != 100 && $increase_place_ != 100 ) && $increase_keep_ >= $increase_place_) 
    {
	print "increase keep greater than increase place skipping\n" ; 
	return "" ;
    }
    elsif ( ( $decrease_keep_ != 100 && $decrease_place_ != 100 ) && $decrease_keep_ >= $decrease_place_) 
    {
	print "decrease keep greater than decrease place skipping\n" ;
	return "" ;
    }
    elsif ( ( $zeropos_keep_ != 100 && $zeropos_place_ != 100 ) && $zeropos_keep_ >= $zeropos_place_ ) 
    {
	print "zeropos keep greater than zeropos place skipping\n" ; 
	return "" ;
    }
    elsif ( ( $increase_place_ != 100 && $zeropos_place_ != 100 ) && $increase_place_ < $zeropos_place_) 
    {
	print "increase place less than zeropos place skipping\n" ; 
	return "" ;
    }
    elsif ( ( $zeropos_place_ != 100 && $decrease_place_ != 100 ) && $zeropos_place_ < $decrease_place_) 
    {
	print "zeropos place less than increase place skipping\n" ; 
	return "" ;
    }
    elsif ( $increase_zeropos_diff_ < 0 ) 
    {
	print "increase zeropos diff less than zero skipping\n" ; 
	return "" ;
    }
    elsif ( $zeropos_decrease_diff_ < 0 ) 
    {
	print "zeropos decrease diff less than zero skipping\n" ; 
	return "" ;
    }
    elsif ( $place_keep_diff_ < 0 ) 
    {
	print "place keep diff greater than zero skipping\n" ; 
	return "" ; 
    }
    else 
    {
	return "true";
    }
}
1;

#my $param_ = $ARGV[0];
#my $DIR = $ARGV[1];
#my @val_ = PermuteParams($param_, $DIR);
#print "NumPram ".$#val_."\n";
#exit;
