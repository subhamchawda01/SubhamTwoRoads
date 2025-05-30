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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# Usage :
# PermuteParams (SRC_FILE, DEST_DIR)
# E.g. PermuteParams ("/home/dvcinfra/strat_options.txt", "/spare/local/gchak")
# will create files containing all permutations in directory '/spare/local/gchak'. 
# Files will be named strat_options_1.txt, strat_options_2.txt ...
sub PermuteParams
{
    my $src_file_ = shift;
    my $dest_dir_ = shift;

    # Add the trailing "/" to dest if not already there.
    if (substr ($dest_dir_, -1, 1) ne '/') {
	$dest_dir_ = $dest_dir_."/";
    }

    # First find only the file name (remove absolute path)
    my @file_name_only_ = split ('\/', $src_file_);
    # Seperate into name & extension.
    my @file_name_parts_ = split ('\\.', $file_name_only_[-1]);

    my $base_file_name_ = "";
    my $ext_file_name_ = "";

    $base_file_name_ = $file_name_parts_[0];
    if ($#file_name_parts_ >= 1) {
	$ext_file_name_ = $file_name_parts_[$#file_name_parts_];
    }

    open SRC_FILE_, "<", $src_file_ or PrintStacktraceAndDie ( "Could not open file : $src_file_\n" );

    my $total_files_ = 1;
    my $output_string_ = "";
    my %param_key_to_count_ = ();
    my @comment_lines_ = ();

    my @ret_file_names_ = ();

    while (my $line_ = <SRC_FILE_>) {
	if (substr ($line_, 0, 1) eq '#') {
	    push ( @comment_lines_, $line_ ) ;
	    next;
	}

	my @words_ = split (' ', $line_);

	if ($#words_ < 2) {
	    printf "Ignoring Malformed line : %s\n", $line_;
	    next;
	}

	my $param_name_val_ = $words_[0]." ".$words_[1]." ";

	# Find how many entries are to be permuted.
	my $useable_entries_;
	for ($useable_entries_ = 2; $useable_entries_ <= $#words_ && substr ($words_[$useable_entries_], 0, 1) ne '#'; $useable_entries_++) {
	}

	# Actual useable entries are -2 this due to PARAMVALUE & KEY_NAME.
	$useable_entries_ -= 2;

	my $comment_line_ = "";
	for (my $count_ = 2 + $useable_entries_; $count_ <= $#words_; $count_++) {
	    $comment_line_ = $comment_line_." ".$words_[$count_];
	}

        # Based on how many params-values follow this key, the no. of total perms may go up.
	# Simple permutation calc :
	$total_files_ = $total_files_ * ($useable_entries_);

	# Add the param-key with its count in the hash.
	$param_key_to_count_{$words_[1]} = ($useable_entries_);

	for (my $count_ = 2; $count_ < $useable_entries_ + 2; $count_++) {
	    $output_string_ = $output_string_.$param_name_val_.$words_[$count_].$comment_line_."|";
	}
    }

    close SRC_FILE;

    # This map is used to store content for each of the total_files no. of files.
    my %file_no_to_string_ = ();

    for (my $count_ = 1; $count_ <= $total_files_; $count_++) {
	$file_no_to_string_{$count_} = "";
    }

    my @data_lines_ = split ('\|', $output_string_);

    # Looks complex, not sure what I'm doing in here.
    my $num_repeats_ = $total_files_;
    for (my $index_ = 0; $index_ <= $#data_lines_; $index_++) {
	my @words_ = split (' ', $data_lines_[$index_]);

	$num_repeats_ = $num_repeats_ / ($param_key_to_count_{$words_[1]});

	my $key_value_no_ = 0;
	for (my $file_no_ = 1; $file_no_ <= $total_files_; ) {
	    for (my $count_ = 0; $count_ < $num_repeats_; $count_++) {
                # Write to the correct file.
		$file_no_to_string_{$file_no_} = $file_no_to_string_{$file_no_}.$data_lines_[($index_ + $key_value_no_)]."\n";
		$file_no_++;
	    }

	    $key_value_no_++;
	    if ($key_value_no_ == $param_key_to_count_{$words_[1]}) {
		$key_value_no_ = 0;
	    }
	}

	$index_ = $index_ + ($param_key_to_count_{$words_[1]} - 1);
    }

    # Open total_files no. of files with appropriate names.
    # Array of file handles!
    for (my $count_ = 1; $count_ <= $total_files_; $count_++) {
	my $file_name_ = "";

	if ($total_files_ == 1) { # For 1 output file, dest file name is same as src file name.
	    $file_name_ = $dest_dir_.$base_file_name_;
	} else {
	    $file_name_ = $dest_dir_.$base_file_name_."_".$count_;
	}

	if ( $ext_file_name_ )
	{ $file_name_ = $file_name_.".".$ext_file_name_; }

	open FILE, ">", $file_name_ or PrintStacktraceAndDie ( "Could not write to file : $file_name_\n" );

	print FILE $file_no_to_string_{$count_};
	for ( my $comment_line_index_ = 0 ; $comment_line_index_ <= $#comment_lines_; $comment_line_index_ ++ )
	{
	    print FILE $comment_lines_[$comment_line_index_];
	}
	close FILE;
	push ( @ret_file_names_, $file_name_ );
    }
    @ret_file_names_;
}

1;
