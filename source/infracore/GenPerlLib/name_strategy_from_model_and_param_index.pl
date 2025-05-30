# \file GenPerlLib/name_strategy_from_model_and_param_index.pl
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
use File::Basename;

sub NameStrategyFromModelAndParamIndex
{
    my ( $name_, $this_model_filename_, $param_filevec_index_, $trading_start_hhmm_, $trading_end_hhmm_ ) = @_;
    my $this_model_filename_base_ = basename ( $this_model_filename_ ); chomp ( $this_model_filename_base_ );
    my $this_model_filename_dir_ = dirname ( $this_model_filename_ ); chomp ( $this_model_filename_dir_ );

    my $this_strategy_filename_base_ = $this_model_filename_base_;
    my $find_str = "w_model";
    my $replace_str = "w_".$name_;
    $find_str = quotemeta $find_str; # escape regex metachars if present

    $this_strategy_filename_base_ =~ s/$find_str/$replace_str/g;
    $this_strategy_filename_base_ = $this_strategy_filename_base_.".tt_".$trading_start_hhmm_."_".$trading_end_hhmm_.".pfi_".$param_filevec_index_;
    my $this_strategy_filename_ = $this_model_filename_dir_."/".$this_strategy_filename_base_;
    $this_strategy_filename_;
}

1;
