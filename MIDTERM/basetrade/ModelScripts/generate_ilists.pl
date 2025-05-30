#!/usr/bin/perl

# \file ModelScripts/generate_ilists.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

use strict;
#use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $HOME_DIR = $ENV { 'HOME' };
my $USER = $ENV { 'USER' };
my $REPO = "basetrade";

my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $DESTINATION_DIR = $HOME_DIR."/modelling/stratwork";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_number_from_vec.pl"; # FindNumberFromVec
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $BIN_DIR;
}

#my $USAGE = "$0 -un dvctrader -sc MHI_0 -tp AS_MORN -b1 Midprice -b2 Midprice -dl 32 -fl NA -sd TODAY-50 -ed TODAY-1 -pa na_e3 -cc 0.85 -ic 0.5 -ss SHARPE -im 120 -mm 250 -rm 60 -nm 1 -el ~/excludelist.txt -ii 5 -st BRT_1750 -et BRT_1900 -uf #~/params.txt -cs TODAY-5 -ce TODAY-1";

my $USAGE = "perl ~/basetrade/ModelScripts/generate_ilists.pl -un dvctrader -sc LFR_0 -uf  /home/dvctrader/indicatorwork/prod_configs/comb_config_LFR_0_na_e3_US_MORN_DAY_OMix_OMix -im 15 -rm 60 -sd TODAY-20 -ed TODAY-1 -cc 0.85 -ic 0.8 -ss SHARPE -cs TODAY-5 -ce TODAY-4 -mm 5 -nm 1 -el exclude_ilist [-hv]";


my $NUM_OF_PARAMS =  25;
my @PARAMS_FLAGS_ = ( (0) x $NUM_OF_PARAMS ); 
my $user_name_ = $USER;
my $short_code_;
my $time_period_;
my $base_px1_;
my $base_px2_;
my @duration_list_ = ( );
my $duration_list_string_;
my @filter_list_ = ( );
my $filter_list_string_;
my $start_date_;
my $end_date_;
my $pred_algo_;
my $cor_of_corr_cutoff_;
my $indep_indep_corr_cutoff_;
my $sort_strategy_;
my $i_max_indicators_;
my $m_max_indicators_;
my $r_max_indicators_;
my $num_merged_lists_;
my $excludelist_filename_ = "NA";
my $include_index_;
my $datagen_start_time_;
my $datagen_end_time_;
my $datagen_start_date_;
my $datagen_end_date_;
my $use_high_volume_days_ = 0;
my $filename_suffix_ = "_r";

# my $ilist_prefix_; $filter_.J

print $USAGE."\n";
print `date` , "\n" ;

my $this_work_dir_ = "$HOME_DIR/indicatorwork/ilists/";
`mkdir -p $this_work_dir_` ;
chdir ( $this_work_dir_ ) ;

my $modelling_dir_ = "$HOME_DIR/modelling";

for ( my $i = 0 ; $i <= $#ARGV ; $i ++ )
{    
    #print ( $ARGV[$i]."\n" );    
    given ( $ARGV[$i] )
    {
	when ( "-un" )
	{
	    if ($PARAMS_FLAGS_[1] == 0)
	    {
		$i = $i + 1;
		$user_name_ = $ARGV[$i];
		$PARAMS_FLAGS_ [1] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }
	}
	when ( "-sc" )
	{
	    if ($PARAMS_FLAGS_[2] == 0)
	    {
		$i = $i + 1;
		$short_code_ = $ARGV[ $i ];
		$PARAMS_FLAGS_ [2] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-tp" )
	{
	    if ($PARAMS_FLAGS_[3] == 0)
	    {
		$i = $i + 1;
		$time_period_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [3] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-b1" )
	{
	    if ( $PARAMS_FLAGS_[4] == 0)
	    {
		$i = $i + 1;
		$base_px1_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [4] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-b2" )
	{
	    if ( $PARAMS_FLAGS_[5] ==0 )
	    {
		$i = $i + 1;
		$base_px2_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [5] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-dl" )
	{
	    if ($PARAMS_FLAGS_[6] == 0)
	    {
		$i = $i + 1;
		
		@duration_list_ = ( );
		
		while ( ( substr $ARGV [$i], 0, 1 ) ne "-" && $i < scalar(@ARGV) )
		{
		    if ( ! FindNumberFromVec ( $ARGV [ $i ] , @duration_list_ ) )
		    {
			push( @duration_list_, $ARGV [ $i ] );
			
			if (scalar(@duration_list_) == 1)
			{
			    $duration_list_string_ = $ARGV [$i];
			}
			else
			{
			    $duration_list_string_ = $duration_list_string_." ".$ARGV [$i];
			}	
		    }		

		    $i = $i + 1;
		}	
    
		if ( scalar(@duration_list_) > 0 )
		{
		    #$duration_list_string_ = chomp ($duration_list_string_);
		    $PARAMS_FLAGS_ [6] = 1;
		}

		$i = $i - 1;
	    }

	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-fl" )
	{
	    if ($PARAMS_FLAGS_[7] == 0)
	    {
		$i = $i + 1;

		@filter_list_ = ( );

		while ( ( substr $ARGV [$i], 0, 1 ) ne "-" && $i < scalar(@ARGV) )
		{
		    if ( $ARGV [ $i ] eq "f0" ||
			 $ARGV [ $i ] eq "fst1" ||
			 $ARGV [ $i ] eq "fsg1" ||
			 $ARGV [ $i ] eq "fsr.5_3" ||
			 $ARGV [ $i ] eq "fv" )
		    {
			push( @filter_list_,$ARGV [ $i ] );
			
			if ( scalar (@filter_list_) == 1)
			{
			    $filter_list_string_ = $ARGV [$i];
			}
			else
			{
			    $filter_list_string_ = $filter_list_string_." ".$ARGV [$i];
			}
		    }
		    
		    $i = $i + 1;		
		}
	    
		if ( scalar (@filter_list_) > 0 )
		{
		    $PARAMS_FLAGS_ [7] = 1;
		}

		$i = $i - 1;
	    }
	    
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-sd" )
	{
	    if ($PARAMS_FLAGS_[8] == 0)
	    {
		
		$i = $i + 1;
		$start_date_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [8] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }	
	}	
	when ( "-ed" )
	{
	    if ($PARAMS_FLAGS_[9] == 0)
	    {
		$i = $i + 1;		    
		$end_date_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [9] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-pa" )
	{
	    if ($PARAMS_FLAGS_[10] == 0)
	    {
		$i = $i + 1;		    
		$pred_algo_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [10] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-cc" )
	{
	    if ($PARAMS_FLAGS_[11] == 0)
	    {
		$i = $i + 1;
		$cor_of_corr_cutoff_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [11] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-ss" )
	{
	    if ($PARAMS_FLAGS_[12] == 0)
	    {
		$i = $i + 1;
		$sort_strategy_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [12] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-im" )
	{
	    if ($PARAMS_FLAGS_[13] == 0)
	    {
		$i = $i + 1;
		$i_max_indicators_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [13] = 1;
	    }

	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-mm" )
	{
	    if ($PARAMS_FLAGS_[14] == 0)
	    {
	    
		$i = $i + 1;
		$m_max_indicators_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [14] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
       	when ( "-nm" )
	{
	    if ($PARAMS_FLAGS_[15] == 0)
	    {
		$i = $i + 1;
		$num_merged_lists_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [15] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-el" )
	{
	    if ($PARAMS_FLAGS_[16] == 0)
	    {

		$i = $i + 1;
		$excludelist_filename_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [16] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-ii" )
	{
	    if ($PARAMS_FLAGS_[17] == 0)
	    {
		$i = $i + 1;		    
		$include_index_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [17] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-st" )
	{
	    if ($PARAMS_FLAGS_[18] == 0)
	    {
		$i = $i + 1;
		$datagen_start_time_ = $ARGV[$i];
		$PARAMS_FLAGS_[18] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);    	
	    }
	    
	}
	when ( "-et" )
	{
	    if ($PARAMS_FLAGS_[19] == 0)
	    {
		$i = $i + 1;
		$datagen_end_time_ = $ARGV[$i];
		$PARAMS_FLAGS_[19] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);    	
	    }	    
	}
	when ( "-uf" )
	{
	    $i = $i + 1;
	    my $config_filename_ = $ARGV[$i];
	    open CONFIGFILEHANDLE, "<$config_filename_ " or Printstacktraceanddie ( "Could not open $config_filename_\n" );
	    
	    while ( my $thisline_ = <CONFIGFILEHANDLE> )
	    {
		chomp ($thisline_);
		my @config_line_words_ = split (' ', $thisline_);

		if ( scalar(@config_line_words_) > 1)
		{
		    given ( $config_line_words_[0] )
		    {
			when ("TIMEPERIODSTRING")
			{
			    if ($PARAMS_FLAGS_[3] == 0)
			    {
				$time_period_ = $config_line_words_[1];
			    }
			}
			when ("PREDALGO")
			{
			    if ($PARAMS_FLAGS_[10] == 0)
			    {
				$pred_algo_ = $config_line_words_[1];
			    }
			}
			when ("PREDDURATION")
			{
			    if ($PARAMS_FLAGS_[6] == 0)
			    {
				for(my $i_ = 1 ; $i_ < scalar(@config_line_words_) ; $i_++)
				{
				    if ( ! FindNumberFromVec ( $config_line_words_[ $i_ ] , @duration_list_ ) )
				    {
					push( @duration_list_, $config_line_words_[ $i_ ] );
					
					if (scalar(@duration_list_) == 1)
					{
					    $duration_list_string_ = $config_line_words_[ $i_ ];
					}
					else
					{
					    $duration_list_string_ = $duration_list_string_." ".$config_line_words_[ $i_ ];
					}	
				    }
				}
			    }
			}
			when ("DATAGEN_START_HHMM")
			{
			    if ($PARAMS_FLAGS_[18] == 0)
			    {	  
				$datagen_start_time_ = $config_line_words_[1];
			    }
			}
			when ("DATAGEN_END_HHMM")
			{
			    if ($PARAMS_FLAGS_[19] == 0)
			    {
				$datagen_end_time_ = $config_line_words_[1];
			    }
			}
			when ("DATAGEN_BASE_FUT_PAIR")
			{
			    if ($PARAMS_FLAGS_[4] == 0)
			    {				 
				$base_px1_ = $config_line_words_[1];
			    }
			    if ($PARAMS_FLAGS_[5] == 0)
			    {
				$base_px2_ = $config_line_words_[2];
			    }
			}
			when ("FILTER")
			{
			    if ($PARAMS_FLAGS_[7] == 0)
			    {
				for(my $i_ = 1 ; $i_ < scalar(@config_line_words_) ; $i_++)
				{
				    if ( $config_line_words_ [ $i_ ] eq "f0" ||
					 $config_line_words_ [ $i_ ] eq "fst1" ||
					 $config_line_words_ [ $i_ ] eq "fsg1" ||
					 $config_line_words_ [ $i_ ] eq "fsr.5_3" ||
					 $config_line_words_ [ $i_ ] eq "fv" )
				    { # invalid/no-filter
					push( @filter_list_,$config_line_words_[ $i_ ] );
					
					if ( scalar (@filter_list_) == 1)
					{
					    $filter_list_string_ = $config_line_words_ [$i_];
					}
					else
					{
					    $filter_list_string_ = $filter_list_string_." ".$config_line_words_ [$i_];
					}
				    }
				}
			    }
			}
		    }
		}
	    }
	    $PARAMS_FLAGS_[0] = 1;
	}
	when ( "-ic" )
	{
	    if ($PARAMS_FLAGS_[20] == 0)
	    {
		$i = $i + 1;
		$indep_indep_corr_cutoff_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [20] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ("-cs")
	{
	    if ($PARAMS_FLAGS_[21] == 0)
	    {
		$i = $i + 1;
		$datagen_start_date_ = $ARGV[$i];
		$PARAMS_FLAGS_[21] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);    	
	    }
	    
	}
	when ("-ce")
	{
	    if ($PARAMS_FLAGS_[22] == 0)
	    {
		$i = $i + 1;
		$datagen_end_date_ = $ARGV[$i];
		$PARAMS_FLAGS_[22] = 1;
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);    	
	    }	    
	}
	when ( "-rm" )
	{
	    if ($PARAMS_FLAGS_[23] == 0)
	    {
		$i = $i + 1;
		$r_max_indicators_ = $ARGV [ $i ];
		$PARAMS_FLAGS_ [23] = 1;	
	    }
	    else
	    {
		print ("options entered twice".$ARGV[$i]);
		exit (0);
	    }

	}
	when ( "-hv" )
	{
	    if ( $PARAMS_FLAGS_[24] == 0)
	    {
		$i = $i + 1;
		$PARAMS_FLAGS_[24] = 1;
		$use_high_volume_days_ = 1;
	    }
	}
	default
	{
	    print ("Unknown option ".$ARGV[$i]."\n");
	    print ($USAGE."\n");
	    exit (0);
	}
    }

}

my $concise_timeperiod_ = "";

if ( index ( $time_period_ , "US_MORN_DAY" ) == 0 )
{
    $concise_timeperiod_ = "US";
}
elsif ( index ( $time_period_ , "US_MORN" ) == 0 )
{
    $concise_timeperiod_ = "USM";
}
elsif ( index ( $time_period_ , "US" ) == 0 )
{
    $concise_timeperiod_ = "US";
}
elsif ( index ( $time_period_ , "EU" ) == 0 )
{
    $concise_timeperiod_ = "EU";
}
elsif ( index ( $time_period_ , "AS_MORN_STB" ) == 0 )
{
    $concise_timeperiod_ = "ASMS";
}
elsif ( index ( $time_period_ , "AS_MORN_VOL" ) == 0 )
{
    $concise_timeperiod_ = "ASMV";
}
elsif ( index ( $time_period_ , "AS_DAY_STB" ) == 0 )
{
    $concise_timeperiod_ = "ASDS";
}
elsif ( index ( $time_period_ , "AS_DAY_VOL" ) == 0 )
{
    $concise_timeperiod_ = "ASDV";
}
elsif ( index ( $time_period_ , "AS_MORN" ) == 0 )
{
    $concise_timeperiod_ = "ASM";
}
elsif ( index ( $time_period_ , "AS_DAY" ) == 0 )
{
    $concise_timeperiod_ = "ASD";
}

if ( $i_max_indicators_ < 5 )
{
    print ( "max_indicators_=".$i_max_indicators_." too low\n" );
    exit ( 0 );
}

if ( $excludelist_filename_ eq "NA" )
{
    if ( -e "$modelling_dir_/stratwork/$short_code_/exclude_list" )
    {
	$excludelist_filename_ = "$modelling_dir_/stratwork/$short_code_/exclude_list" ;
	print "using $excludelist_filename_\n" ;
    }
}

print (" user_name_                 \t".$user_name_.
       "\n short_code_              \t".$short_code_.
       "\n time_period_             \t".$time_period_.
       "\n base_px1_                \t".$base_px1_.
       "\n base_px2_                \t".$base_px2_.
       "\n duration_list_string_    \t".$duration_list_string_.
       "\n filter_list_string_      \t".$filter_list_string_.
       "\n start_date_              \t".$start_date_.
       "\n end_date_                \t".$end_date_.
       "\n pred_algo_               \t".$pred_algo_.
       "\n cor_of_corr_cutoff_      \t".$cor_of_corr_cutoff_.
       "\n indep_indep_corr_cutoff_ \t".$indep_indep_corr_cutoff_.
       "\n sort_strategy_           \t".$sort_strategy_.
       "\n i_max_indicators_        \t".$i_max_indicators_.
       "\n m_max_indicators_        \t".$m_max_indicators_.
       "\n r_max_indicators_        \t".$r_max_indicators_.
       "\n num_merged_lists_        \t".$num_merged_lists_.
       "\n excludelist_filename_    \t".$excludelist_filename_.
       "\n include_index_           \t".$include_index_.
       "\n datagen_startime_        \t".$datagen_start_time_.
       "\n datageb_endtime_         \t".$datagen_end_time_.
       "\n datagen_stardate_        \t".$datagen_start_date_.
       "\n datageb_enddate_         \t".$datagen_end_date_.      
       "\n use_high_volume_days_    \t".$use_high_volume_days_. 
       "\n"
    );

if ( $PARAMS_FLAGS_[1] != 1 ||  
     $PARAMS_FLAGS_[2] != 1 ||  
     $PARAMS_FLAGS_[8] != 1 ||
     $PARAMS_FLAGS_[9] != 1 ||
     $PARAMS_FLAGS_[11] != 1 ||
     $PARAMS_FLAGS_[20] != 1 ||
     $PARAMS_FLAGS_[12] != 1 ||
     $PARAMS_FLAGS_[13] != 1 ||
     $PARAMS_FLAGS_[14] != 1 ||
     $PARAMS_FLAGS_[15] != 1 ||
     $PARAMS_FLAGS_[21] != 1 ||
     $PARAMS_FLAGS_[22] != 1
     )
{
    for (my $j =0; $j < scalar(@PARAMS_FLAGS_); $j ++)
    {
	print($PARAMS_FLAGS_[$j]."\n");
    }
    print ( "required arguments missing:\n\tusage: ".$USAGE."\n" );
    exit ( 0 );
}


my $step_1_exec_ = $MODELSCRIPTS_DIR."/generate_base_px_ilists.pl";
my $step_2_exec_ = $MODELSCRIPTS_DIR."/generate_merged_ilists.pl";
my $step_3_exec_ = $MODELSCRIPTS_DIR."/remove_correlated_indicators_wrapper.pl";

my $i_;
my $j_;
my $step_1_cmd_;
my $step_2_cmd_;
my $step_3_cmd_;

for ($i_ = 0; $i_ < scalar(@filter_list_); $i_++ )
{

    print "RUNNING FILTER $filter_list_[ $i_ ] \n";
    $step_1_cmd_ = $step_1_exec_." "
	.$user_name_." "
	.$base_px1_." "
	.$base_px2_." "
	.$pred_algo_." "
	.$filter_list_[$i_]." "
	.$filter_list_[$i_].".J "
	.$short_code_." "
	.$time_period_." "
	.$start_date_." "
	.$end_date_." "
	.$cor_of_corr_cutoff_." "
	.$sort_strategy_." "
	.$i_max_indicators_." "
	.$duration_list_string_." "
	."-e ".$excludelist_filename_;      

    if ( $use_high_volume_days_ == 1 )
    {
	$step_1_cmd_ = $step_1_cmd_." -hv";
	$filename_suffix_ = "_hv.r";
    }    

    print $step_1_cmd_."\n";
    my $step_1_result_ = `$step_1_cmd_`;
    
    my $duration_list_string2_ = -1;
    my $count_ = 0 ;

    for ( my $ii = 0 ; $ii < scalar(@duration_list_) ; $ii++ )
    {
	if ( -e "ilist_".$short_code_."_".$concise_timeperiod_."_".PriceTypeToString($base_px1_)."_".PriceTypeToString($base_px2_)."_".$filter_list_[$i_].".J".$duration_list_[$ii] )
	{
	    if ($duration_list_string2_ == -1)
	    {
		$duration_list_string2_ = $duration_list_[ $ii ];
		$count_ ++ ;
	    }
	    else
	    {
		$duration_list_string2_ = $duration_list_string2_." ".$duration_list_[ $ii ];
		$count_ ++ ;
	    }	
	}
	else
	{
	    print "note : $duration_list_[ $ii ] file is not generated after step1 \n" ;
	}
    }

    next if ( $duration_list_string2_ == -1 ) ;

    if ( $count_ > 4 )
    {
	$num_merged_lists_ = 2 ;
    }

    $step_2_cmd_ = $step_2_exec_." "
	.$short_code_." "
	.$base_px1_ ." "
	.$base_px2_ ." "
	.$time_period_." "
	.$num_merged_lists_." "
	.$m_max_indicators_." "
	.$filter_list_[$i_].".J "
	.$duration_list_string2_." ";

    print $step_2_cmd_."\n";
    my $step_2_result_ = `$step_2_cmd_`;
    

    # !<->! #using the filename direclty would be suffice
    my $tcmd_="";
    if ( $num_merged_lists_ == 2 )
    {
	$tcmd_  = "ls ilist_".$short_code_."_".$concise_timeperiod_."_".PriceTypeToString($base_px1_)."_".PriceTypeToString($base_px2_)."_".$filter_list_[$i_].".J_st".
	               " ilist_".$short_code_."_".$concise_timeperiod_."_".PriceTypeToString($base_px1_)."_".PriceTypeToString($base_px2_)."_".$filter_list_[$i_].".J_lt";
    }
    else 
    {
	$tcmd_  = "ls ilist_".$short_code_."_".$concise_timeperiod_."_".PriceTypeToString($base_px1_)."_".PriceTypeToString($base_px2_)."_".$filter_list_[$i_].".J0";

    }

     #print $tcmd_."\n";    
    my @merged_files_ = `$tcmd_`; 
    chomp ( @merged_files_);

	
    for ( my $j = 0 ; $j < scalar(@merged_files_) ; $j++ )
    {	
	if (`wc -l $merged_files_[$j] | awk '{print $1}'` < 5 )
	{
	    print "$merged_files_[$j] has less indicators or no indicators \n" ;
	    next ;
	}
	
	$step_3_cmd_ = $step_3_exec_." "
	    .$short_code_." "
	    .$merged_files_[$j]." "
	    .$indep_indep_corr_cutoff_." "
	    .$r_max_indicators_." "
	    .$datagen_start_date_." "
	    .$datagen_end_date_." "
	    .$datagen_start_time_." "
	    .$datagen_end_time_;


	print $step_3_cmd_."\n";
	my @step_3_result_ = `$step_3_cmd_`;

	chomp ( @step_3_result_ );

	my $ilist_refined_file_name_ = $merged_files_ [ $j].$filename_suffix_;
	print "\t >> ".$ilist_refined_file_name_."\n";

	open ( ILIST_FILE , "> $ilist_refined_file_name_ " ) or PrintStacktraceAndDie ( "Could not write file $ilist_refined_file_name_" );
	
	for ( my $k = 0 ; $k <= $#step_3_result_ ; $k ++ )
	{
	    print ILIST_FILE $step_3_result_[ $k ]."\n";
	}
	
	close ( ILIST_FILE );
	if ( `wc -l $ilist_refined_file_name_ | awk '{ print \$1}'`  > 5 ) 
	{
	    `cp $ilist_refined_file_name_ $DESTINATION_DIR/$short_code_/`;
	}
    }
}    

GenerateSecondaryIlists ( );
print `date` , "\n" ;

##************************************EOS***************************************************************************##

sub PriceTypeToString
{
    my ( $price_type_ ) = @_;

    my $price_string_ = "";
    
    given ( $price_type_ )
    {
	when ( "MidPrice" ) { $price_string_ = "Mid"; }
	
	when ( "Midprice" ) { $price_string_ = "Mid"; }

	when ( "MktSizeWPrice" ) { $price_string_ = "Mkt"; }

	when ( "MktSinusoidal" ) { $price_string_ = "Sin"; }

	when ( "OrderWPrice" ) { $price_string_ = "Owp"; }

	when ( "OfflineMixMMS" ) { $price_string_ = "OMix"; }

	default { $price_string_ = ""; }
    }

    return $price_string_;
}


sub GenerateSecondaryIlists
{
    my @price_types_list_ = ( );

    if ( ( $base_px1_ eq "MidPrice" || $base_px1_ eq "Midprice" ) &&
	 ( $base_px2_ eq "OfflineMixMMS" ) )
    {

	push ( @price_types_list_ , "MidPrice MidPrice" );
	push ( @price_types_list_ , "MidPrice MktSizeWPrice" );
    }
    
    if ( ( $base_px1_ eq "OfflineMixMMS" ) && 
	 ( $base_px2_ eq "OfflineMixMMS" ) )
    {

	push ( @price_types_list_ , "MktSizeWPrice MktSizeWPrice" );
	push ( @price_types_list_ , "MktSinusoidal MktSinusoidal" );
	push ( @price_types_list_ , "OrderWPrice OrderWPrice" );
	
    }
    
    my @agg_pred_duration_ = ( "0" , "l" );

    if ( $num_merged_lists_ == 1 ) { @agg_pred_duration_ = ( "0" ); }

    for (my $l_ = 0; $l_ < scalar(@filter_list_); $l_++ )
    {
	foreach my $t_pred_duration_ ( @agg_pred_duration_ )
	{
	    my $bp1_bp2_ilist_file_name_ = "ilist_".$short_code_."_".$concise_timeperiod_."_".PriceTypeToString($base_px1_)."_".PriceTypeToString($base_px2_)."_".$filter_list_[$l_].".J".$t_pred_duration_.$filename_suffix_;
	    
	    
	    if ( -e $bp1_bp2_ilist_file_name_ )
	    {
		my @bp1_bp2_ilist_content_ = `cat $bp1_bp2_ilist_file_name_`; chomp ( @bp1_bp2_ilist_content_ );
	    
		foreach my $t_price_type_ ( @price_types_list_ )
		{
		    my @t_price_type_words_ = split ( ' ' , $t_price_type_ );
		    
		    my $t_price_dep_type_ = $t_price_type_words_ [ 0 ];
		    my $t_price_indep_type_ = $t_price_type_words_ [ 1 ];
		    
		    my $t_price_dep_string_ = PriceTypeToString ( $t_price_dep_type_ );
		    my $t_price_indep_string_ = PriceTypeToString ( $t_price_indep_type_ );
		    
		    my $ilist_file_name_ = "ilist_".$short_code_."_".$concise_timeperiod_."_".$t_price_dep_string_."_".$t_price_indep_string_."_".$filter_list_[$l_].".J".$t_pred_duration_.$filename_suffix_;
		    print "\t >> ".$ilist_file_name_."\n";
		    
		    open ( ILIST_FILE , "> $ilist_file_name_ " ) or PrintStacktraceAndDie ( "Could not write file $ilist_file_name_" );
		    
		    print ILIST_FILE "MODELINIT DEPBASE ".$short_code_." ".$t_price_dep_type_." ".$t_price_indep_type_."\n";
		    for ( my $i = 1 ; $i <= $#bp1_bp2_ilist_content_ ; $i ++ )
		    {
			print ILIST_FILE $bp1_bp2_ilist_content_ [ $i ]."\n";
		    }
		    
		    close ( ILIST_FILE );
		    if ( `wc -l $ilist_file_name_ | awk '{ print \$1}'`  > 5 )
		    {
			`cp $ilist_file_name_ $DESTINATION_DIR/$short_code_/`;
		    }
		}
	    }
	}
    }
}


    
