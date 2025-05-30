use strict;
use warnings;

my $HOME_DIR = $ENV{'HOME'};

require $HOME_DIR . "/infracore/GenPerlLib/get_all_machines_vec.pl";

#development and trade machines
my @all_machines_;
if ($#ARGV == -1){
  @all_machines_ = GetAllMachinesVec();
}

#development machines ny11-15
elsif ($ARGV[0] eq "D"){
  @all_machines_ = GetDevMachinesVec();
}

#trade machines
elsif ($ARGV[0] eq "T"){
  @all_machines_ = GetTradeMachinesVec();
}

foreach (@all_machines_) {
  print "$_\n";
}