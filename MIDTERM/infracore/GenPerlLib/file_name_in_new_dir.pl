# \file GenPerlLib/file_name_new_dir.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
# Finds the full path of a filename as it would be after copying to the given directory

use strict;
use warnings;
use File::Basename; # for basename and dirname

# my $HOME_DIR=$ENV{'HOME'}; 
# my $REPO="infracore";

# my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";

sub FileNameInNewDir {
    my $input_filename_ = shift;
    my $new_dir_name_ = shift;

    my $retval_full_path_ = $new_dir_name_."/".basename ( $input_filename_ ) ;
    chomp ( $retval_full_path_ );
    $retval_full_path_;
}

1;
