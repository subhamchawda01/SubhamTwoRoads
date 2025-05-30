# \file GenPerlLib/sync_to_all_machines.pl
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

my $HOME_DIR = $ENV{'HOME'};

sub GetDevMachinesVec {
  my $only_locations_ref_ = shift;
  my $exclude_locations_ref_ = shift;

  $only_locations_ref_ = [] if ! defined $only_locations_ref_;

  push ( @$only_locations_ref_, "ny4" );

  my @machines_vec_ = GetAllMachinesVec ( $only_locations_ref_, $exclude_locations_ref_ );

  return @machines_vec_;
}

sub GetTradeMachinesVec {
  my $only_locations_ref_ = shift;
  my $exclude_locations_ref_ = shift;

  $exclude_locations_ref_ = [] if ! defined $exclude_locations_ref_;

  push ( @$exclude_locations_ref_, "ny4" );

  my @machines_vec_ = GetAllMachinesVec ( $only_locations_ref_, $exclude_locations_ref_ );

  return @machines_vec_;
}

sub GetAllMachinesVec {
  my $only_locations_ref_ = shift;
  my $exclude_locations_ref_ = shift;

  my @machines_vec_ = ();

  my $servers_filename = '/spare/local/tradeinfo/all_servers.cfg';

  open( my $fh, '<:encoding(UTF-8)', $servers_filename )
    or die "Could not open file '$servers_filename' $!";

  my $read = 0;

  while ( my $row = <$fh> ) {
    chomp $row;

    if ( $row =~ /^#/ ) {
      $read = 0;
      $row =~ s/^#//;
      if ( ! (defined $only_locations_ref_ && $#$only_locations_ref_ >= 0)
          || grep { $row eq $_ } @$only_locations_ref_ ) {

        if ( ! (defined $exclude_locations_ref_ && $#$exclude_locations_ref_ >= 0)
            || ! grep { $row eq $_ } @$exclude_locations_ref_ ) {
          $read = 1;
        }
      }
    }
    elsif ( $read == 1 && $row !~ /^#/ ) {
      push( @machines_vec_, $row );
    }
  }
  return @machines_vec_;
}

1;
