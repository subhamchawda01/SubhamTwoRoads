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

sub cartesian_product
{
  my $sets_ = shift @_;
  return [[]] unless @$sets_;
  my $first_set_ = shift @$sets_;
  my $partial_products_ = cartesian_product ( $sets_ );
  my $products_ = [];
  for my $item_ ( @$first_set_ )
  {
    for my $product_ ( @$partial_products_ )
    {
      push @$products_, [ $item_, @$product_ ];
    }
  }
  return $products_;
}

sub GetSubsetsofSize
{
  my ( $set_ , $size_ ) = @_ ;
  my @set_of_sets_ = ();
  for ( my $i=0; $i<$size_; $i++ )
  {
    push ( @set_of_sets_, [ @$set_ ] );
  }
  return cartesian_product ( \@set_of_sets_ );
}

sub CheckSubsetElementUniqueness
{
  my ( $set_, $num_rep_ ) = @_;
  my %set_element_to_count_map_ = ( );
  for my $element_ ( @$set_ )
  {
    if ( exists $set_element_to_count_map_{ $element_ } )
    {
      $set_element_to_count_map_{ $element_ } += 1;
    }
    else
    {
      $set_element_to_count_map_{ $element_ } = 1;
    }
    if ( $set_element_to_count_map_{ $element_ } > $num_rep_ )
    {
      return 0;
    }
  }
  return 1;
}


sub PermuteParams
{
    my $src_file_ = shift;
    my $dest_dir_ = shift;

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

	    if ( $words_[1] eq "PRIMARY_ORS_INDICATOR" || $words_[1] eq "SECONDARY_ORS_INDICATOR" || $words_[1] eq "TRADEINDICATOR" || $words_[1] eq "REGIMEINDICATOR" || $words_[1] eq "REGIMES_TO_TRADE" || $words_[1] eq "CANCELINDICATOR" || $words_[1] eq "IMPLIEDMKTINDICATOR")
	    {
		$ors_indicator_string_ = $ors_indicator_string_.$line_."\n";
		next;
	    }

	    # Based on how many params-values follow this key, the no. of total perms may go up.
	    # Simple permutation calc :
	    $total_files_ = $total_files_ * ($useable_entries_);

	    # Add the param-key with its count in the hash.
	    $param_key_to_count_{$words_[1]} = ($useable_entries_);

	    for (my $count_ = 2; $count_ < $useable_entries_ + 2; $count_++) 
	    {
		$output_string_ = $output_string_.$param_name_val_.$words_[$count_].$comment_line_."|";
	    }
	}

	close SRC_FILE_;
	my $MAX_FILES_TO_ALLOW = 5000;
	if ( $total_files_ < $MAX_FILES_TO_ALLOW ||
	     $USER eq "kputta" )
	{

	    # This map is used to store content for each of the total_files no. of files.
	    my %file_no_to_string_ = ();

	    for (my $count_ = 1; $count_ <= $total_files_; $count_++) 
	    {
		$file_no_to_string_{$count_} = "";
	    }

	    my @data_lines_ = split ('\|', $output_string_);

	    # Looks complex, not sure what I'm doing in here.
	    my $num_repeats_ = $total_files_;
	    for ( my $index_ = 0; $index_ <= $#data_lines_; $index_++ )
	    {
		my @words_ = split (' ', $data_lines_[$index_]);

		$num_repeats_ = $num_repeats_ / ($param_key_to_count_{$words_[1]});
		my $key_value_no_ = 0;
		for (my $file_no_ = 1; $file_no_ <= $total_files_; ) 
		{
		    for (my $count_ = 0; $count_ < $num_repeats_; $count_++) 
		    {
			# Write to the correct file.
			$file_no_to_string_{$file_no_} = $file_no_to_string_{$file_no_}.$data_lines_[($index_ + $key_value_no_)]."\n";
			$file_no_++;
		    }

		    $key_value_no_++;
		    if ($key_value_no_ == $param_key_to_count_{$words_[1]}) 
		    {
			$key_value_no_ = 0;
		    }
		}

		$index_ = $index_ + ($param_key_to_count_{$words_[1]} - 1);
	    }

	    # Open total_files no. of files with appropriate names.
	    # Array of file handles!
	    my $num_ = 1;
	    for (my $count_ = 1; $count_ <= $total_files_; $count_++) 
	    {
		my $file_name_ = "";

		if ($total_files_ == 1)
		{ # For 1 output file, dest file name is same as src file name.
		    $file_name_ = $dest_dir_.$base_file_name_;
		}
		else
		{
		    $file_name_ = $dest_dir_.$base_file_name_."_".$num_;
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
    @ret_file_names_;
}

sub PermuteParamsRegime
{
    print "Permute params regime called\n";
    my $src_file_ = shift;
    my $dest_dir_ = shift;
    print $src_file_." ".$dest_dir_."\n";
    my @list_of_param_files_ = ();
    my @params_vec_vec_ = ();
    my $regime_indicator_ = "NONE";
    my @ret_file_names_;
    if ( -e $src_file_ )
    {
	open SRC_FILE_, "<", $src_file_ or PrintStacktraceAndDie ( "Could not open file : $src_file_\n" );
	while ( my $line_ = <SRC_FILE_>)
	{
	    my @words_ = split (' ', $line_);
	    if ( index ( $words_[ 0 ], "PARAMFILELIST" ) >= 0 )
            {
		push ( @list_of_param_files_, $words_[ 1 ] );
            }
            else
            {
		$regime_indicator_ = $line_;
                chomp ($regime_indicator_);
            }
	    
	}
        for( my $i=0; $i <= $#list_of_param_files_; $i++ )
        {
	    my $base_ = basename ( $list_of_param_files_[ $i ] );
	    my $dir_ = $dest_dir_."/".$base_;
	    if ( ! ( -d $dir_ ) ) { `mkdir -p $dir_`; }
	    my @param_filevec_ = PermuteParams ( $list_of_param_files_[ $i ], $dir_ );
            push (@params_vec_vec_, [ @param_filevec_ ] );
        }

        my $permutations_ = cartesian_product( \@params_vec_vec_ );
        for my $param_ ( @$permutations_ )
        {
          my $param_name_ = $dest_dir_."/param";
          for my $x ( @$param_ )
          {
            my $base_ = basename( $x );
            my @words_ = split ('_', $base_);
            my $idx_ = $words_[$#words_];
            $param_name_ = $param_name_."_".$idx_;
          }
          open OUT_PARAM, ">", $param_name_ or PrintStacktraceAndDie ( "Could not open file for writing : $param_name_\n" );
          for my $x ( @$param_ )
          {
            print OUT_PARAM "PARAMFILELIST $x\n";
          }
          print OUT_PARAM "$regime_indicator_\n";
          close OUT_PARAM;
          push ( @ret_file_names_, $param_name_ );
        }


    }
    @ret_file_names_;
}


sub ParallelPermuteParamsRegime
{
    my $regime_index_ = shift;
    my $param_file_list_ = shift;
    my $src_file_ = shift;
    my $dest_dir_ = shift;

    print $regime_index_." ".$param_file_list_." ".$src_file_." ".$dest_dir_."\n";
    if (substr ($dest_dir_, -1, 1) ne '/')
    {
        $dest_dir_ = $dest_dir_."/";
    }
    my $ors_indicator_string_ = "";

    my $base_file_name_ = basename ( $src_file_ ) ; chomp ( $src_file_ );

    my @ret_file_names_ = ();
    if ( -e $src_file_ )
    {
        open SRC_FILE_, "<", $src_file_ or PrintStacktraceAndDie ( "Could not open file : $src_file_\n" );


        my $total_files_ = 1;
        my $output_string_ = "";
        my %param_key_to_count_ = ();
        my @comment_lines_ = ();

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

            my $useable_entries_;
            for ($useable_entries_ = 2; $useable_entries_ <= $#words_ && substr ($words_[$useable_entries_], 0, 1) ne '#'; $useable_entries_++)
            {
            }

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

            $total_files_ = $total_files_ * ($useable_entries_);

            $param_key_to_count_{$words_[1]} = ($useable_entries_);

            for (my $count_ = 2; $count_ < $useable_entries_ + 2; $count_++)
            {
                $output_string_ = $output_string_.$param_name_val_.$words_[$count_].$comment_line_."|";
            }
        }
        close SRC_FILE_;
        my $MAX_FILES_TO_ALLOW = 5000;
        if ( $total_files_ < $MAX_FILES_TO_ALLOW ||
             $USER eq "sghosh" )
        {

            my %file_no_to_string_ = ();

            for (my $count_ = 1; $count_ <= $total_files_; $count_++)
            {
                $file_no_to_string_{$count_} = "";
            }

            my @data_lines_ = split ('\|', $output_string_);

            my $num_repeats_ = $total_files_;
            for ( my $index_ = 0; $index_ <= $#data_lines_; $index_++ )
            {
                my @words_ = split (' ', $data_lines_[$index_]);

                $num_repeats_ = $num_repeats_ / ($param_key_to_count_{$words_[1]});
                my $key_value_no_ = 0;
                for (my $file_no_ = 1; $file_no_ <= $total_files_; )
                {
                    for (my $count_ = 0; $count_ < $num_repeats_; $count_++)
                    {
                        $file_no_to_string_{$file_no_} = $file_no_to_string_{$file_no_}.$data_lines_[($index_ + $key_value_no_)]."\n";
                        $file_no_++;
                    }

                    $key_value_no_++;
                    if ($key_value_no_ == $param_key_to_count_{$words_[1]})
                    {
                        $key_value_no_ = 0;
                    }
                }

                $index_ = $index_ + ($param_key_to_count_{$words_[1]} - 1);
            }

            my $num_ = 1;
            for (my $count_ = 1; $count_ <= $total_files_; $count_++)
            {
                my $file_name_ = "";

                if ($total_files_ == 1)
                { # For 1 output file, dest file name is same as src file name.
                    $file_name_ = $dest_dir_.$base_file_name_;
                }
                else
                {
                    $file_name_ = $dest_dir_.$base_file_name_."_".$num_;
                }

                {
                    open FILE, ">", $file_name_ or PrintStacktraceAndDie ( "Could not write to file : $file_name_\n" );
                    print FILE $file_no_to_string_{$count_};
                    $num_++;

                    for ( my $comment_line_index_ = 0 ; $comment_line_index_ <= $#comment_lines_; $comment_line_index_ ++ )
                    {
                        print FILE $comment_lines_[$comment_line_index_];
                    }
                    print FILE $ors_indicator_string_;
                    my $file_name_list_ = $file_name_."_list";
                    my $exec_cmd_ = "cat $param_file_list_ | awk -v var=\"$regime_index_\" '{if(NR==var) { print \"PARAMFILELIST $file_name_\" } else { print \$0} }' > $file_name_list_";
                    `$exec_cmd_`;
                    close FILE;
                    push ( @ret_file_names_, $file_name_list_ );
                }
            }
        }
    }
    @ret_file_names_;
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
