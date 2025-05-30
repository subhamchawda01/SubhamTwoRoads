#!/usr/bin/perl
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

sub GetCommissionForDI{

	my $shortcode_  = shift;
	$shortcode_ = substr ($shortcode_, 0 , 6);
	
	my $comm_ = `cat /spare/local/files/DI_commissions | grep $shortcode_ | awk -F, '{print \$2}'`;
	
	if ( $comm_ eq "")
	{
		$comm_ = `cat /spare/local/files/DI_commissions | grep OTHER | awk -F, '{print \$2}'`;
	}

	chomp($comm_);
	$comm_;

}

1;