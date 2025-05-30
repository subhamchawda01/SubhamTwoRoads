#!/usr/bin/perl
#
# \file ModelScripts/find_best_model_for_strategy_var_pert.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
#

use strict;
use warnings;
use feature "switch";
use FileHandle;
use POSIX;
use List::Util qw/max min/; # for max
use File::Basename;
use Term::ANSIColor;

my $HOME = $ENV{'HOME'};
my $MODELLING_DIR = "$HOME/modelling";
my $MODELS_DIR = "$MODELLING_DIR/models";
my $STRATS_DIR = "$MODELLING_DIR/strats";
my $PARAMS_DIR = "$MODELLING_DIR/params";
my $DATABASE_DIR = "$MODELLING_DIR/database";
my $USAGE="$0 shc start_time end_time config_";
my $exchange_;
my @num_strats_to_install_;
my $t_num_strats_to_install_;
my @regime_size_;
my $regime_start;
my $regime_end;
my @regime_size;
my $regime_tag;
my $regime1_tag;
my $regime2_tag;
my @intermediate_files;
require "$HOME/basetrade_install/GenPerlLib/make_strat_vec_from_dir_and_tt.pl"; # MakeStratVecFromDirAndTT
require "$HOME/basetrade_install/GenPerlLib/get_basepx_strat_first_model.pl"; # MakeStratVecFromDirAndTT
require "$HOME/basetrade_install/GenPerlLib/strat_utils.pl"; # MakeStratVecFromDirAndTT





if ( $#ARGV < 3 ){ print "USAGE : ".$USAGE."\n"; exit(0);}

my $shortcode_ = $ARGV[ 0 ];
my $start_time_ = $ARGV[ 1 ];
my $end_time_ = $ARGV[ 2 ];
my $regime_ind_list_;
my $set_1_;
my $set_2_;
my $regime1_folder;
my $regime2_folder;
my ($config_file_) = $ARGV[ 3 ];
my ($regime1_config_);
my ($regime2_config_);
my ($regime_config_);
LoadConfigFile($config_file_);
my $px_type_ = "ALL";
print "$0 $shortcode_ $start_time_ $end_time_ $regime_ind_list_ $set_1_ $set_2_ called\n";

$MODELS_DIR = $MODELS_DIR."/".$shortcode_."/".$start_time_."-".$end_time_;
$STRATS_DIR = $STRATS_DIR."/".$shortcode_."/".$start_time_."-".$end_time_;
$PARAMS_DIR = $PARAMS_DIR."/".$shortcode_."/".$start_time_."-".$end_time_;
$DATABASE_DIR = $DATABASE_DIR."/".$shortcode_."/".$start_time_."-".$end_time_;
if ( !( -e $MODELS_DIR ) )
{
    `mkdir -p $MODELS_DIR`;
}
if ( !( -e $STRATS_DIR ) )
{
    `mkdir -p $STRATS_DIR`;
}
if ( !( -e $PARAMS_DIR ) )
{
    `mkdir -p $PARAMS_DIR`;
}
if ( !( -e $DATABASE_DIR ) )
{
    `mkdir -p $DATABASE_DIR`;
}

my $DATABASE_FILE = $DATABASE_DIR."/mapping.txt";
if ( -e $DATABASE_FILE )
{
    `rm $DATABASE_FILE`;
}
my @index_to_strat_ = ();

open (SET, "<", $set_1_ ) or PrintStacktraceAndDie ( "Could not open $set_1_ for reading" );
my @set_lines_1_ = <SET>; chomp ( @set_lines_1_ );
close(SET);
open (SET, "<", $set_2_ ) or PrintStacktraceAndDie ( "Could not open $set_2_ for reading" );
my @set_lines_2_ = <SET>; chomp ( @set_lines_2_ );
close(SET);

my @temp_all_strats_in_dir_1 = MakeStratVecFromDirAndTT("/home/dvctrader/modelling/strats/".$shortcode_, $regime1_folder);
my @temp_all_strats_in_dir_2 = MakeStratVecFromDirAndTT("/home/dvctrader/modelling/strats/".$shortcode_, $regime2_folder);
my $timeperiod_ = "$start_time_-$end_time_";

print " ".$#temp_all_strats_in_dir_1." ";
if ( $#temp_all_strats_in_dir_1 < 0 )
{
    print "No strats in desired pool\n";
    exit ( 0 );
}
my @all_strats_in_dir_1_ = ( );
my @all_strats_in_dir_2_ = ( );
for ( my $i=0; $i<=$#temp_all_strats_in_dir_1; $i++ )
{
    my $base_p = `basename $temp_all_strats_in_dir_1[ $i ]`;
    chomp($base_p);
    for ( my $j=0; $j<=$#set_lines_1_; $j++ )
{
    if ( $base_p eq $set_lines_1_[ $j ] )
{
    push (@all_strats_in_dir_1_, $temp_all_strats_in_dir_1[ $i ]);
}
}
}
for ( my $i=0; $i<=$#temp_all_strats_in_dir_2; $i++ )
{
    my $base_p = `basename $temp_all_strats_in_dir_2[ $i ]`;
    chomp($base_p);
    for ( my $j=0; $j<=$#set_lines_2_; $j++ )
{
    if ( $base_p eq $set_lines_2_[ $j ] )
{
    push (@all_strats_in_dir_2_, $temp_all_strats_in_dir_2[ $i ]);
}
}
}
my @used_strats2;
my @used_strats1;
my @unused_regime_1;
my @unused_regime_2;
my @regime_strats;
for ( my $i=0; $i<=$#all_strats_in_dir_1_; $i++ )
{
    for ( my $j=0; $j<=$#all_strats_in_dir_2_; $j++ )
{
    if ( !( $all_strats_in_dir_2_[ $j ] ~~ @used_strats2) && CanMergeForRegime($all_strats_in_dir_1_[ $i ], $all_strats_in_dir_2_[ $j ] )  )
{
    print "Can merge".$all_strats_in_dir_1_[ $i ]." ".$all_strats_in_dir_2_[ $j ]." ".$i." ".$j."\n";
    MergeStratsWithRegime( $shortcode_, $all_strats_in_dir_1_[ $i ], $all_strats_in_dir_2_[ $j ], $i, $j, $regime_ind_list_, $start_time_, $end_time_ );
    push( @used_strats1, $all_strats_in_dir_1_[ $i ]);
    push( @used_strats2, $all_strats_in_dir_2_[ $j ]);
    last;
}
else
{
    print "Can't merge $all_strats_in_dir_1_[ $i ] $all_strats_in_dir_2_[ $j ]\n";
}
}
}
for ( my $j=0; $j<=$#all_strats_in_dir_2_; $j++ )
{
    if(!($all_strats_in_dir_2_[ $j ] ~~ @used_strats2))
{
    push( @unused_regime_2, $all_strats_in_dir_2_[ $j ]);
}
}
for ( my $j=0; $j<=$#all_strats_in_dir_1_; $j++ )
{
    if(!($all_strats_in_dir_1_[ $j ] ~~ @used_strats1))
    {
	push( @unused_regime_1, $all_strats_in_dir_1_[ $j ]);
    } 
}


sub CanMergeForRegime
{
    my ($strat_1, $strat_2) = @_;
    print $strat_1."\n";
    my $type_1 = GetStrattype($strat_1);
    print $strat_2."\n";
    my $type_2 = GetStrattype($strat_2);
    if ($type_1 ne $type_2) { return 0; }
    my $regress_1 = GetRegression($strat_1);
    my $regress_2 = GetRegression($strat_2);
    if ( $regress_1 ne "LINEAR" || $regress_1 ne $regress_2 ) { return 0; }
    my $basepx_1 = GetBasepxStratFirstModel($strat_1);
    my $basepx_2 = GetBasepxStratFirstModel($strat_2);
    if ( $basepx_1 ne $basepx_2 ) { return 0; }
    if ( IsRollStrat($strat_1) || IsRollStrat($strat_2) ) { return 0; }
    if ( IsRegimeStrat($strat_1) || IsRegimeStrat($strat_2) ) { return 0; }
    return 1;
}

sub MergeStratsWithRegime
{
    my ($shc_, $strat_1_, $strat_2_, $idx_1_, $idx_2_, $regime_ind_file_, $start_time_, $end_time_ ) = @_;
    open ( REGIMES , "<" , $regime_ind_file_ ) or PrintStacktraceAndDie ( "Could not open $regime_ind_file_ for reading" );
    my @regimes_ = <REGIMES>; chomp ( @regimes_ );
    close (REGIMES);
    open ( FILE, ">>", $DATABASE_FILE ) or PrintStacktraceAndDie ( "Could not open $DATABASE_FILE for writing" );

    my $model_1_ = GetModel ( $strat_1_ );
    my $model_2_ = GetModel ( $strat_2_ );
    my $param_1_ = GetParam ( $strat_1_ );
    my $param_2_ = GetParam ( $strat_2_ );
    open (M1, "<", $model_1_ ) or PrintStacktraceAndDie ( "Could not open $model_1_ for reading" );
    my @model_1_lines_ = <M1>; chomp ( @model_1_lines_ );
    close(M1);
    open (M2, "<", $model_2_ ) or PrintStacktraceAndDie ( "Could not open $model_2_ for reading" );
    my @model_2_lines_ = <M2>; chomp ( @model_2_lines_ );
    close(M2);
    my $type_ = GetStrattype( $strat_1_ );
    for ( my $i=0; $i<=$#regimes_; $i++ )
{
    my $strat_1_2_ = $STRATS_DIR."/strat_".$idx_1_."_".$idx_2_."_".$i;
#my $strat_2_1_ = $STRATS_DIR."/strat_".$idx_2_."_".$idx_1_."_".$i;
    my $model_1_2_ = $MODELS_DIR."/model_".$idx_1_."_".$idx_2_."_".$i;
#my $model_2_1_ = $MODELS_DIR."/model_".$idx_2_."_".$idx_1_."_".$i;
    my $param_1_2_ = $PARAMS_DIR."/param_".$idx_1_."_".$idx_2_."_".$i;
#my $param_2_1_ = $PARAMS_DIR."/param_".$idx_2_."_".$idx_1_."_".$i;
    print FILE basename($strat_1_2_)." ".basename($strat_1_)." ".basename($strat_2_)."\n";
#print FILE basename($strat_2_1_)." ".basename($strat_2_)." ".basename($strat_1_)."\n";
    open ( STRAT, ">" , $strat_1_2_ ) or PrintStacktraceAndDie ( "Could not open $strat_1_2_ for writing" );
    print STRAT "STRATEGYLINE $shc_ $type_ $model_1_2_ $param_1_2_ $start_time_ $end_time_ 1111\n";
    close(STRAT);
#open ( STRAT, ">" , $strat_2_1_ ) or PrintStacktraceAndDie ( "Could not open $strat_2_1_ for writing" );
#print STRAT "STRATEGYLINE $shc_ $type_ $model_2_1_ $param_2_1_ $start_time_ $end_time_ 1111\n";
#close(STRAT);
    push(@regime_strats, $strat_1_2_);
    open ( MODEL, ">" , $model_1_2_ ) or PrintStacktraceAndDie ( "Could not open $model_1_2_ for writing" );
    for ( my $line = 0; $line <=$#model_1_lines_-1; $line++ )
{
    if ( $line == 2 )
{
    print MODEL "REGIMEINDICATOR 1.00 $regimes_[$i]\n";
}
if ( $line == 1 )
{
    print MODEL "MODELMATH SELECTIVENEW CHANGE\n";
    next;
}
print MODEL "$model_1_lines_[ $line ]\n";
}
print MODEL "INDICATORINTERMEDIATE\n";
for ( my $line = 3; $line <=$#model_2_lines_-1; $line++ )
{
    print MODEL "$model_2_lines_[ $line ]\n";
}
print MODEL "INDICATOREND\n";
close(MODEL);
=for comment
open ( MODEL, ">" , $model_2_1_ ) or PrintStacktraceAndDie ( "Could not open $model_2_1_ for writing" );
for ( my $line = 0; $line <=$#model_2_lines_-1; $line++ )
{
    if ( $line == 2 )
{
    print MODEL "REGIMEINDICATOR 1.00 $regimes_[$i]\n";
}
if ( $line == 1 )
{
    print MODEL "MODELMATH SELECTIVENEW CHANGE\n";
    next;
}
print MODEL "$model_2_lines_[ $line ]\n";
}
print MODEL "INDICATORINTERMEDIATE\n";
for ( my $line = 3; $line <=$#model_1_lines_-1; $line++ )
{
    print MODEL "$model_1_lines_[ $line ]\n";
}
print MODEL "INDICATOREND\n";
close(MODEL);
=cut

open ( PARAM, ">" , $param_1_2_ ) or PrintStacktraceAndDie ( "Could not open $param_1_2_ for writing" );
print PARAM "PARAMFILELIST $param_1_\n";
print PARAM "PARAMFILELIST $param_2_\n";
print PARAM "INDICATOR 1.00 $regimes_[$i]\n";
close(PARAM);
push (@intermediate_files, $param_1_2_);
push (@intermediate_files, $strat_1_2_);
push (@intermediate_files, $model_1_2_);
=for comment
    open ( PARAM, ">" , $param_2_1_ ) or PrintStacktraceAndDie ( "Could not open $param_2_1_ for writing" );
print PARAM "PARAMFILELIST $param_2_\n";
print PARAM "PARAMFILELIST $param_1_\n";
print PARAM "INDICATOR 1.00 $regimes_[$i]\n";
close(PARAM);
=cut
}
close(FILE);
}
my $config_ = $regime_config_;
if($#regime_strats > -1)
{
    $regime_start = 0;
    $regime_end = 1;
    MakeNewPickStratsConfig(@regime_strats);
    my $cmd = "/home/dvctrader/sushant/pick_strats_and_install.pl ".$regime_tag." ".$regime_config_.".temp";
    print "\n".$cmd."\n";
}
$config_ = $regime1_config_;
print "1. ".$regime1_config_."\n";
if($#unused_regime_1 > -1)
{
    $regime_start = 0;
    $regime_end = 0;
    MakeNewPickStratsConfig(@unused_regime_1);
    my $cmd = "/home/dvctrader/sushant/pick_strats_and_install.pl ".$regime1_tag." ".$regime1_config_.".temp";
    print "\n".$cmd."\n";
}
$config_ = $regime2_config_;
print "2. ".$regime2_config_."\n";
if($#unused_regime_2 > -1)
{
    $regime_start = 1;
    $regime_end = 1;
    MakeNewPickStratsConfig(@unused_regime_2);
    my $cmd = "/home/dvctrader/sushant/pick_strats_and_install.pl ".$regime2_tag." ".$regime2_config_.".temp";
    print "\n".$cmd."\n";
}

for( my $j = 0; $j<= $#intermediate_files; $j++)
{
#  my $cmd = `rm $intermediate_files[$j]`;
}

sub MakeNewPickStratsConfig
{ 
    my $has_printed_strats = "";
    my $file_handle_ = FileHandle->new;
    my $regime1_config_temp_ = $config_.".temp";
    my @strats_ = @_;
    $file_handle_->open ( "> $regime1_config_temp_" ) or PrintStacktraceAndDie ( "Could not open $regime1_config_temp_ for writing\n" );
    open ( REGIME1_FILE , "<" , $config_ ) or PrintStacktraceAndDie ( "Could not open config file $config_" );
    my @config_file_lines_ = <REGIME1_FILE>; chomp ( @config_file_lines_ );
    close ( REGIME1_FILE );

    my $current_param_ = "";
    foreach my $config_file_lines_ ( @config_file_lines_ )
    {
	if ( index ( $config_file_lines_ , "#" ) == 0 ) # not ignoring lines with # not at the beginning
	{
	    next;
	}
	my @t_words_ = split ( ' ' , $config_file_lines_ );
	if ( $#t_words_ < 0 )
	{
	    $current_param_ = "";
            print $file_handle_ "\n";
	    next;
	}
	if ( ! $current_param_ )
	{
	    $current_param_ = $t_words_ [ 0 ];
            print $file_handle_ $config_file_lines_."\n";
	    next;
	}
	else
	{
	    given ( $current_param_ )
	    {
                when ( "STRATS_TO_KEEP" )
		{
                  if(! $has_printed_strats)
                  {
		    for ( my $j=0; $j<=$#strats_; $j++ )
		    {
			print $file_handle_ `basename $strats_[$j]`;
		    }
                    $has_printed_strats = 'true';
                  }

                }
                when ( "NUM_STRATS_TO_INSTALL")
                {
                    my $number = $#strats_ + 1;
		    print $file_handle_ "$number"."\n";
	        }
                when ( "TOTAL_SIZES_TO_RUN_TIME_BASED")
                {
                    for(my $i = $regime_start ;$i <= $regime_end; $i++)
                    {
			my $size_to_run_ = $regime_size_[$i] * ($#strats_+1);
			print $file_handle_ $size_to_run_." ";
                    }
                     print $file_handle_ "\n";
                }
                when ( "TOTAL_SIZE_TO_RUN" )
                {
                    for(my $i = $regime_start ;$i <= $regime_end; $i++)
		    {
			my $size_to_run_ = $regime_size_[$i] * ($#strats_+1);
			print $file_handle_ $size_to_run_." ";
		    }
                    print $file_handle_ "\n";
                }
                default
                {
		    print $file_handle_ $config_file_lines_."\n";
                }
                
            }        
        }
    }
}

sub LoadConfigFile
{
    my ( $t_config_file_ ) = @_;

    open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
    my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
    close ( CONFIG_FILE );

    my $current_param_ = "";
    foreach my $config_file_lines_ ( @config_file_lines_ )
    {
	if ( index ( $config_file_lines_ , "#" ) == 0 ) # not ignoring lines with # not at the beginning
	{
	    next;
	}
	my @t_words_ = split ( ' ' , $config_file_lines_ );
	if ( $#t_words_ < 0 )
	{
	    $current_param_= "";
	    next;
	}
	if ( ! $current_param_)
	{
	    $current_param_= $t_words_ [ 0 ];
	    next;
	}
	else
	{
	    given ( $current_param_)
	    {
                when ( "SHORTCODE" )
		{
		    my $t_shortcode_ = $t_words_ [ 0 ];
		    if ( $t_shortcode_ ne $shortcode_ )
		    {
			PrintStacktraceAndDie ( "$t_shortcode_ in config file != $shortcode_" );
		    }
		}
                when ( "REGIME1FOLDER")
                {
                    $regime1_folder = $t_words_[0];
                }
                when ( "REGIME2FOLDER")
                {
                    $regime2_folder = $t_words_[0];
                }
                when ("REGIME1STRATS")
                {
                    $set_1_ = $t_words_ [0];
                }
                when ("REGIME2STRATS")
                {
                    $set_2_ = $t_words_ [0];
                }
		when ( "REGIME1TAG")
                {
                    $regime1_tag = $t_words_ [0];
                }
                when ( "REGIME2TAG")
                {
		    $regime2_tag = $t_words_ [0];
                }
                when ( "REGIMETAG")
                {
                    $regime_tag = $t_words_ [0];
                }   

		when ( "EXCHANGE" )
		{
		    $exchange_ = $t_words_ [ 0 ];
		}
                when ( "REGIMEINDICATORLIST" )
                {
                    $regime_ind_list_ = $t_words_ [0];
                }
		when ( "REGIME1CONFIG")
		{
		    $regime1_config_ = $t_words_ [ 0 ];
                    print $regime1_config_."\n";
		}

		when ( "REGIME2CONFIG" )
		{
		    $regime2_config_ = $t_words_ [ 0 ];
                    print $regime2_config_."\n";

                }
                when ( "REGIMECONFIG" )
		{
		    $regime_config_ = $t_words_ [ 0 ];
                    print $regime_config_."\n";
		}
                when ( "REGIMESIZES" )
                {
                    @regime_size_ = @t_words_;
                }
	    }
	}

    }
}
