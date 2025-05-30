# \file GenPerlLib/read_shc_machine_mapping.pl
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

my $product_machine_file="/home/pengine/prod/live_configs/machine_shc.txt";
my %shc_to_serv_ = ( );
my %serv_to_shcvec_ = ( );

open FHANDLE, "< $product_machine_file" or PrintStacktraceAndDie ( "Could not open $product_machine_file for reading" );
my @config_lines_ = <FHANDLE>; chomp ( @config_lines_ );
close FHANDLE;

foreach my $line_ ( @config_lines_ ) {
  my @twords_ = split(" ", $line_ );
  if ( $#twords_ < 1 ) { next; }
  $shc_to_serv_ { $twords_[1] } = $twords_[0];
  push ( @{$serv_to_shcvec_{ $twords_[0] } }, $twords_[1] );
}

sub GetMachineForProduct {
  my $input_product=shift;
  return $shc_to_serv_{ $input_product };
}

sub GetAllProductsForMachine {
  my $input_machine=shift;
  my $products_vec_ref_ = shift;

  if ( defined $serv_to_shcvec_{ $input_machine } ) {
    @$products_vec_ref_ = @{ $serv_to_shcvec_{ $input_machine } };
  }
}

1;
