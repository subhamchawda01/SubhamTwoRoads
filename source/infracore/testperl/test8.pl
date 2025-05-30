#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 
my $MODELSCRIPTS_DIR=$HOME_DIR."/infracore_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";

my $const_string_ = "FGBS_0^FGBM_0^FGBL_0";
my @this_const_vec_ = split ( /\^/, $const_string_ );

printf ( "%d %s\n", $#this_const_vec_, join ( ' ', @this_const_vec_ ) );
