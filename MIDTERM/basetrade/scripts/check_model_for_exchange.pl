#!/usr/bin/perl
use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl"; # GetExchFromSHC
require "$GENPERLLIB_DIR/get_port_constituents.pl"; # GetPortConstituents

my $USAGE = "$0 MODELPATH EXCHANGE\n";
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $model_path_ = $ARGV [ 0 ];
my $exchange_ = $ARGV [ 1 ];

open MODEL_FILE, "< $model_path_";
while ( my $model_line_ = <MODEL_FILE> )
{
    chomp($model_line_);
    my @model_words_ = split ' ', $model_line_;
    if($model_words_[0] eq "INDICATOR")
    {
	my $prod_exch_ = GetExchFromSHC($model_words_[3]);
	if ($prod_exch_ eq "" ){
            #probably a portfolio
	    my @port_constituents = GetPortConstituents($model_words_[3]);
	    foreach my $port_constituent ( @port_constituents)
	    {
		if(GetExchFromSHC($port_constituent) eq $exchange_)
		{
		    printf("Reject at %s \n",$model_words_[3]);
		    close ( MODEL_FILE );
		    exit(0);
		}
	    }
	}
	else
	{
	    if($prod_exch_ eq $exchange_)
	    {
		printf("Reject at %s \n",$model_words_[3]);
		close ( MODEL_FILE );
		exit(0);
	    }
	}

	if ( $model_words_[4] =~ /[A-Za-z]/ && !($model_words_[4] =~ /Midprice|MktSinusoidal|MktSizeWPrice|OrderWPrice|TradeWPrice|OfflineMixMMS/ ) && ! ($model_words_[4] =~ /^[-+]?[0-9]*\.?[0-9]+$/) )
	{
	    $prod_exch_ = GetExchFromSHC($model_words_[4]);

	    if ($prod_exch_ eq "" ){
                #probably a portfolio
		my @port_constituents = GetPortConstituents($model_words_[4]);
		foreach my $port_constituent ( @port_constituents)
		{
		    if(GetExchFromSHC($port_constituent) eq $exchange_)
		    {
			printf("Reject at %s \n",$model_words_[4]);
			close ( MODEL_FILE );
			exit(0);
		    }
		}
	    }
	    else
	    {
		if($prod_exch_ eq $exchange_)
		{
		    printf("Reject at %s \n",$model_words_[4]);
		    close ( MODEL_FILE );
		    exit(0);
		}
	    }
	}
    }
}

close ( MODEL_FILE );

print("Accept\n");
