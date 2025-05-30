#!/usr/bin/perl

# \file GenPerlLib/install_strategy_modelling.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $MODELING_STAGED_STRATS_DIR=$MODELING_BASE_DIR."/staged_strats"; # this directory is used to store the chosen strategy files
my $MODELING_MODELS_DIR=$MODELING_BASE_DIR."/models"; # this directory is used to store the chosen model files
my $MODELING_PARAMS_DIR=$MODELING_BASE_DIR."/params"; # this directory is used to store the chosen param files

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/exists_and_same.pl"; #ExistsAndSame
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_id_for_shortcode.pl"; #GetStratIdForShortcode

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetPoolSize {
  my $shortcode_ = shift;
  my $timeperiod_ = shift;

  my @pool_vec_ = MakeStratVecFromDir ( $MODELING_STRATS_DIR."/".$shortcode_."/".$timeperiod_ );
  my $poolsize_ = $#pool_vec_ + 1;

  return $poolsize_;
}

sub GetStagedPoolSize {
  my $shortcode_ = shift;
  my $timeperiod_ = shift;

  my @pool_vec_ = MakeStratVecFromDir ( $MODELING_STAGED_STRATS_DIR."/".$shortcode_."/".$timeperiod_ );
  my $poolsize_ = $#pool_vec_ + 1;

  return $poolsize_;
}

sub InstallStrategyModelling {
# USAGE : src_strategy_file shortcode datagen_start_end_str trading_start_end_str";
    
    my $prod_install_ = 0 ;
    my $t_temp_strategy_filename_ = shift ;
    my $shortcode_ = shift ;
    my $datagen_start_end_str_ = shift ;
    my $trading_start_end_str_ = shift ;
    $prod_install_ = defined $_[0] ?  $_[0] : 0 ;

    my $strats_dir_ = $MODELING_STAGED_STRATS_DIR;
    

    $strats_dir_ = $MODELING_STRATS_DIR if ( int($prod_install_) > 0 );

    my $debug_ = 0;
    
    if ( ( $t_temp_strategy_filename_ ) && ExistsWithSize ( $t_temp_strategy_filename_ ) )
    {
	my ( $t_temp_model_filename_, $t_temp_param_filename_ ) = GetModelAndParamFileNames ( $t_temp_strategy_filename_ ) ;
	
	my $dest_models_dir_ = $MODELING_MODELS_DIR."/".$shortcode_."/".$datagen_start_end_str_ ;
	my $dest_params_dir_ = $MODELING_PARAMS_DIR."/".$shortcode_ ;
	my $dest_strats_dir_ = $strats_dir_."/".$shortcode_."/".$trading_start_end_str_ ;

	
	
	# now we have to move this to $dest_strats_dir_, $dest_models_dir_, $dest_params_dir_
	my $final_strat_filename_ = FileNameInNewDir ( $t_temp_strategy_filename_, $dest_strats_dir_ );
	my $final_model_filename_ = FileNameInNewDir ( $t_temp_model_filename_, $dest_models_dir_ );
	my $final_param_filename_ = FileNameInNewDir ( $t_temp_param_filename_, $dest_params_dir_ );
	
	CreateEnclosingDirectory ( $final_strat_filename_ );
	CreateEnclosingDirectory ( $final_model_filename_ );
	CreateEnclosingDirectory ( $final_param_filename_ );
	
	if ( -e $final_strat_filename_ ) 
	{ # for whatever reason a stratfile with that name exists, so cannot go ahead
	    if ( $debug_ == 1 ) 
	    { print STDERR "A strat file with the name $final_strat_filename_ already exists! skipping $final_strat_filename_ creation\n"; }
	}
	else
	{
	    my $need_to_copy_model_ = 1;
	    if ( -e $final_model_filename_ ) 
	    { # for whatever reason a modelfile with that name exists, so cannot go ahead
		if ( ExistsAndSame ( $t_temp_model_filename_, $final_model_filename_ ) == 0 )
		{
		    if ( $debug_ == 1 ) 
		    { print STDERR "A different model file with the name $final_model_filename_ already exists! \n"; }
		    $final_model_filename_ = $final_model_filename_."s";
		    while ( -e $final_model_filename_ )
		    {
			$final_model_filename_ = $final_model_filename_."s";
		    }
		}
		else
		{
		    if ( $debug_ == 1 )
		    { print STDERR "An identical model file with the name $final_model_filename_ already exists, so not copying over\n"; }
		    $need_to_copy_model_ = 0;
		}
	    }
	    
	    my $cont_searching_ = 1;
	    while ( $cont_searching_ == 1 )
	    {
		$cont_searching_ = 0; # only set to 1 later if we are searching a new file
		if ( -e $final_param_filename_ ) 
		{ # for whatever reason a paramfile with that name exists, so cannot go ahead
		    if ( ExistsAndSame ( $t_temp_param_filename_, $final_param_filename_ ) == 0 )
		    {
			if ( $debug_ == 1 )
			{ print STDERR "A different param file with the name $final_param_filename_ already exists! \n"; }
			$final_param_filename_ = $final_param_filename_."s";
			$cont_searching_ = 1;
		    }
		    else
		    {
			if ( $debug_ == 1 )
			{ print STDERR "An identical param file with the name $final_param_filename_ already exists, so not copying over\n"; }
		    }
		}
		else
		{
		    my $exec_cmd = "cp $t_temp_param_filename_ $final_param_filename_";
		    if ( $debug_ == 1 )
		    { print STDERR "$exec_cmd\n"; }
		    `$exec_cmd`;
		}
	    }
	    
	    if ( $need_to_copy_model_ == 1 )
	    {
		my $exec_cmd = "cp $t_temp_model_filename_ $final_model_filename_";
		if ( $debug_ == 1 ) { print STDERR "$exec_cmd\n"; }
		`$exec_cmd`;
	    }
	    
	    open FSTRATF, "> $final_strat_filename_" or PrintStacktraceAndDie ( "Could not open $final_strat_filename_ for writing\n" );
	    
	    open TEMPSTRATFILEHANDLE, "< $t_temp_strategy_filename_ " or PrintStacktraceAndDie ( "Could not open $t_temp_strategy_filename_\n" );
	    while ( my $thisline_ = <TEMPSTRATFILEHANDLE> ) 
	    {
		my @t_words_ = split ( ' ', $thisline_ );
		if ( $#t_words_ >= 4 )
		{ 
		    $t_words_[3] = $final_model_filename_;
		    $t_words_[4] = $final_param_filename_;
		    $t_words_[7] = GetStratIdForShortcode ( $shortcode_ );
		    print FSTRATF join ( ' ', @t_words_ )."\n";
		}
	    }
	    close TEMPSTRATFILEHANDLE ;
	    
	    close FSTRATF ;

	    

	}
        return $final_strat_filename_;
    }
}

1
