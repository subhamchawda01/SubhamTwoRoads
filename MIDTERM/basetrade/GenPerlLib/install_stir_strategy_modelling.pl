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
my $MODELING_STIR_STRATS_DIR=$MODELING_BASE_DIR."/stir_strats"; # this directory is used to store the chosen strategy files
my $MODELING_MODELS_DIR=$MODELING_BASE_DIR."/models"; # this directory is used to store the chosen model files
my $MODELING_PARAMS_DIR=$MODELING_BASE_DIR."/params"; # this directory is used to store the chosen param files

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/exists_and_same.pl"; #ExistsAndSame
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_id_for_shortcode.pl"; #GetStratIdForShortcode

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub InstallStirStrategyModelling {
# USAGE : src_strategy_file shortcode datagen_start_end_str trading_start_end_str";
    
    my $prod_install_ = 0 ;
    my $t_temp_strategy_filename_ = shift ;
    my $shortcode_ = shift ;
    my $datagen_start_end_str_ = shift ;
    my $trading_start_end_str_ = shift ;
    $prod_install_ = defined $_[0] ?  $_[0] : 0 ;

    if ( int ( $prod_install_ )  > 0  )
    {
	$MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats";
    }

    my $debug_ = 0;
    
    if ( ( $t_temp_strategy_filename_ ) && ExistsWithSize ( $t_temp_strategy_filename_ ) )
    {
      my $strat_name_ = `cat $t_temp_strategy_filename_ | awk '{print \$2 }'`; chomp ( $strat_name_ ) ;
      my $dest_stir_strats_dir_ = $MODELING_STIR_STRATS_DIR."/".$shortcode_."/".$trading_start_end_str_ ;
      my $dest_stir_strat_name_ = FileNameInNewDir ( $t_temp_strategy_filename_, $dest_stir_strats_dir_ ) ;
      CreateEnclosingDirectory ( $dest_stir_strat_name_ ) ;
      if ( -e $dest_stir_strat_name_ ) 
      {
        print STDERR "A strat with name $dest_stir_strat_name_ already exists, Can't continue \n" ;
      }

      if ( ( $strat_name_ ) && ExistsWithSize ( $strat_name_ ) ) 
      {      
        open STRAT_CONTENT , "< $strat_name_ " or PrintStacktraceAndDie ( "Could not open file for reading\n" ) ;
        my @strat_lines_ = <STRAT_CONTENT>;
        close STRAT_CONTENT ;

        my $dest_strats_dir_ = $MODELING_STRATS_DIR."/".$shortcode_."/".$trading_start_end_str_ ;
        my $final_strat_name_ = FileNameInNewDir ( $strat_name_, $dest_strats_dir_ ) ;
        CreateEnclosingDirectory ( $final_strat_name_ ) ;
        open FINAL_STRAT, "> $final_strat_name_" or PrintStacktraceAndDie ( "Could not open the file $final_strat_name_ for writing\n" ) ;

        foreach my $line_ ( @strat_lines_ )
        {
          $line_ =~ s/^\s+|\s+$//g ;
          my @line_words_ = split ( " ", $line_ );
          if ( index ( $line_, "STRATEGYLINE" ) >= 0 && $#line_words_ >= 3 ) 
          {
            my $prod_ = $line_words_[1] ;
            my $model_name_ = $line_words_ [2] ;
            my $param_name_ = $line_words_ [3] ; 
            my $dest_models_dir_ = $MODELING_MODELS_DIR."/".$shortcode_."/".$prod_."/".$datagen_start_end_str_ ;
            my $dest_params_dir_ = $MODELING_PARAMS_DIR."/".$shortcode_."/".$prod_ ;
            my $final_model_name_ = FileNameInNewDir ( $model_name_, $dest_models_dir_ ) ;
            my $final_param_name_ = FileNameInNewDir ( $param_name_, $dest_params_dir_ ) ;

            CreateEnclosingDirectory ( $final_model_name_ );
            CreateEnclosingDirectory ( $final_param_name_ );

              if ( -e $final_model_name_  )
            {
              if ( $debug_ == 1 ) 
              { print STDERR "A model file with the name $final_model_name_ already exists! skipping $final_model_name_ creation\n"; }
              $final_model_name_ = $final_model_name_."s";
            }
            
            if ( -e $final_param_name_ )
            {
              if ( $debug_ == 1 ) 
              { print STDERR "A param file with the name $final_param_name_ already exists! \n"; }
            }

            `cp $model_name_ $final_model_name_`;
            `cp $param_name_ $final_param_name_`;
            print FINAL_STRAT "STRATEGYLINE $prod_ $final_model_name_ $final_param_name_\n";
          }
          elsif ( index ( $line_, "STRUCTURED_TRADING") >= 0 )
          {
          	# Common Line 
          	my $common_param_product_ = $line_words_[1] ; # ideally should be same as $shortcode_
          	my $common_param_name_ = $line_words_[3];
          	my $dest_params_dir_ = $MODELING_PARAMS_DIR."/".$common_param_product_ ;
          	my $final_common_param_name_ = FileNameInNewDir ( $common_param_name_, $dest_params_dir_ ) ;	 
          	CreateEnclosingDirectory ( $final_common_param_name_ );
          	if ( -e $final_common_param_name_ )
          	{
          		if ( $debug_ ==  1) 
          		{
          			print STDERR "A common param file with the name $final_common_param_name_ already exists! skipping the $final_common_param_name_ creation\n";
          		}
          	}
          	
          	`cp $common_param_name_ $final_common_param_name_ `;
          	print FINAL_STRAT "$line_words_[0] $line_words_[1] $line_words_[2] $final_common_param_name_ $line_words_[4] $line_words_[5] $line_words_[6]\n";
          	
          }
          else
          {
            print FINAL_STRAT $line_."\n";
          }
        }
        close FINAL_STRAT ;
        `cat $t_temp_strategy_filename_ | awk '{\$2="$final_strat_name_"; print }' > $dest_stir_strat_name_ `;
      }
    }
}
1;

#my $strat_ = $ARGV[0];
#my $shortcode_ = $ARGV[1];
#my $datagen_time_period_ = $ARGV[2];
#my $trading_start_end_str_ = $ARGV [3];
#my @val_ = InstallStirStrategyModelling ($strat_, $shortcode_, $datagen_time_period_, $trading_start_end_str_  );
##print "NumPram ".$#val_."\n";
##exit;
#
