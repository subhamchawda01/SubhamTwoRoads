#!/usr/bin/perl

# Check if for a given date, the config has shortcodes from given exchange
# if no then accept
# if yes then print corresponding shortcode/portfolio and reject
# 
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $WF_SCRIPTS_DIR=$HOME_DIR."/".$REPO."/walkforward";

require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl"; # GetExchFromSHC
require "$GENPERLLIB_DIR/get_port_constituents.pl"; # GetPortConstituents

if ( $#ARGV < 1 )
{
  print "USAGE: <script> strat_basename exchange [tradingdate] [print_all_rejects]\n";
  exit(1);
}


my $strat_name_ = $ARGV [ 0 ];
my $exchange_ = $ARGV [ 1 ];
my $tradingdate = 19700101;
my $print_all_rejects_ = "";
my $rejected_ = 0;

if ($#ARGV >= 2){
	# date is optional for type 3 configs or configs where model doesn't change across days
	$tradingdate = $ARGV [ 2 ];
	if ($#ARGV >= 3){
		$print_all_rejects_ = $ARGV[3];		
	}

}

my $exec_cmd_ = $WF_SCRIPTS_DIR."/print_strat_from_config.py -c $strat_name_ -d $tradingdate ";
my $strategy_line = `$exec_cmd_`; chomp($exec_cmd_);
my @strategy_words_ = split(' ', $strategy_line);

# get the temporary model and param both so they can be deleted at later stage
my $model_path_ = "INVALID";
my $param_path_ = "INVALID";

if ( $#strategy_words_ >= 4){
	$model_path_ = $strategy_words_[3];
	$param_path_ = $strategy_words_[4]; 
}


open MODEL_FILE, "< $model_path_";

while ( my $model_line_ = <MODEL_FILE> )
{
    chomp($model_line_);
    my @model_words_ = split ' ', $model_line_;
    if($model_words_[0] eq "INDICATOR")
    {
	my $prod_exch_ = GetExchFromSHC($model_words_[3]);
	if ($prod_exch_ eq "" ){
            #probably a portfolio, check for portfolio shortcodes and get exchange for that
	    my @port_constituents = GetPortConstituents($model_words_[3]);
	    foreach my $port_constituent ( @port_constituents)
	    {
		if(GetExchFromSHC($port_constituent) eq $exchange_)
		{
		    printf("Reject at %s \n",$model_words_[3]);
		    $rejected_ = 1;
		    if (!$print_all_rejects_){
			    close ( MODEL_FILE );
			    exit(0);
		    }
		}
	    }
	}
	else
	{
	    if($prod_exch_ eq $exchange_)
	    {
		printf("Reject at %s \n",$model_words_[3]);
		$rejected_ = 1;
		if (!$print_all_rejects_){
			close ( MODEL_FILE );
			exit(0);
		}
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
			$rejected_ = 1;
			if (!$print_all_rejects_){
				close ( MODEL_FILE );
				exit(0);
			}
		    }
		}
	    }
	    else
	    {
		if($prod_exch_ eq $exchange_)
		{
			# reject if the shortcode from given exchange was in model
		    printf("Reject at %s \n",$model_words_[4]);
		    $rejected_ = 1;
		    if (!$print_all_rejects_){
			    close ( MODEL_FILE );
			    exit(0);
		    }
		}
	    }
	}
    }
}

close ( MODEL_FILE );

# delete the model-param
`rm -rf $model_path_ 2>/dev/null`;
`rm -rf $param_path_ 2>/dev/null`;

if ( $rejected_ == 0 ){
	print("Accept\n");
}
