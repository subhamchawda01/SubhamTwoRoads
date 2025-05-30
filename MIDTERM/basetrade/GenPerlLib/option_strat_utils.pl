#!/usr/bin/perl

# \file GenPerlLib/option_strat_utils.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#   Suite No 353, Evoma, #14, Bhattarhalli,
#   Old Madras Road, Near Garden City College,
#   KR Puram, Bangalore 560049, India
#   +91 80 4190 3551
#

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub IsOptionStrat;
sub GetModelFileName;
sub GetParamFileName;
sub GetListOfShcFromUnderlying;
sub GetListOfUnderlying;

sub IsOptionStrat {
  my $base_strat_name_ = shift; 
  if ( $#_ >= 0 )
  {
    my $shc_ = shift;
    my $string_to_remove_  = "_".$shc_."\$";
    $base_strat_name_ =~ s/$string_to_remove_//g;
  }
  my $STRAT_FILE_PATH = `ls $HOME_DIR/modelling/NSEOptionsMM/*Strats/*/$base_strat_name_ $HOME_DIR/modelling/NSEOptionsMM/*Strats/$base_strat_name_  2>/dev/null`; chomp ( $STRAT_FILE_PATH ) ;   
  if ( -e $STRAT_FILE_PATH )
  {
    return $STRAT_FILE_PATH;
  } 
  else
  {
    return "";
  }
}

sub GetModelFileName {
  my $t_full_strategy_name_ = shift;    
  my $t_full_model_name_ = "";

  open STRATFILENAME, "< $t_full_strategy_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_strategy_name_\n" );

  while ( my $thisline_ = <STRATFILENAME> ) 
  {
    next if $thisline_ =~ /^#/;
    my @t_words_ = split ( ' ', $thisline_ );
    $t_full_model_name_ = $t_words_[2];
    open MODELFILENAME, "< $t_full_model_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_model_name_\n" );
  }
  close STRATFILENAME ;
  return $t_full_model_name_;
}

sub GetParamFileName {
  my $t_full_strategy_name_ = shift;    
  my $shc_name_ = shift;
  my $t_full_param_name_ = "";

  open STRATFILENAME, "< $t_full_strategy_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_strategy_name_\n" );

  while ( my $thisline_ = <STRATFILENAME> ) 
  {
    chomp($thisline_);
    next if $thisline_ =~ /^#|^$/;
    my @t_words_ = split ( /\s+/, $thisline_ );
    next if $#t_words_ < 0 ;
    my $t_full_model_name_ = $t_words_[2];
    open MODELFILENAME, "< $t_full_model_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_model_name_\n" );
    my $found_ = 0;
    while ( my $thismline_ = <MODELFILENAME> ) 
    {
      chomp($thismline_);
      next if $thismline_ =~ /^#|^$/;
      last if $thismline_ eq "INDVIDUAL_INDICATORS";
      last if $thismline_ eq "GLOBAL_INDICATORS";
      if ( $thismline_ eq "PARAMSHCPAIR" ) {
        $found_ = 1;
        next;
      } elsif ( $found_ == 1 ) {
        my @m_words_ = split ( /\s+/, $thismline_ );
        next if $#m_words_ < 0 ;
        if($m_words_[0] eq $shc_name_) {
          $t_full_param_name_ = $m_words_[1];
          last;
        }
      } else {
        next;
      }
    }     
  }
  close STRATFILENAME ;
  return $t_full_param_name_;
}

sub GetListOfShcFromUnderlying {
  my $t_full_strategy_name_ = shift;    
  my $underlying_ = shift;
  my @list_shc_ = ();
  my $shc_name_ = "NSE_".$underlying_."_FUT0";

  my $tag_ = "";
  my $num_contracts_ = 3;
  my $step_size_ = 1;
  my $moneyness_tag_ = "";

  open STRATFILENAME, "< $t_full_strategy_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_strategy_name_\n" );

  while ( my $thisline_ = <STRATFILENAME> ) 
  {
    chomp($thisline_);
    next if $thisline_ =~ /^#|^$/;
    my @t_words_ = split ( /\s+/, $thisline_ );
    next if $#t_words_ < 0 ;
    my $t_full_model_name_ = $t_words_[2];
    open MODELFILENAME, "< $t_full_model_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_model_name_\n" );
    my $found_ = 0;
    while ( my $thismline_ = <MODELFILENAME> )
    {
      chomp($thismline_);
      next if $thismline_ =~ /^#|^$/;
      last if $thismline_ eq "INDVIDUAL_INDICATORS";
      last if $thismline_ eq "GLOBAL_INDICATORS";
      if ( $thismline_ eq "PARAMSHCPAIR" ) {
        $found_ = 1;
        next;
      } elsif ( $found_ == 1 ) {
        my @m_words_ = split ( /\s+/, $thismline_ );
        next if $#m_words_ < 0 ;
        if($m_words_[0] eq $shc_name_) {
          $tag_ = $m_words_[2];
          $num_contracts_ = $m_words_[3];
          $step_size_ = $m_words_[4];         
          last;
        }
      } else {
        next;
      }
    }

    if (index($tag_, "OTM") != -1) {
      $moneyness_tag_ = "O";
    }

    if (index($tag_, "ITM") != -1) {
      $moneyness_tag_ = "I";
    }

    my $total_contracts_ = $num_contracts_*$step_size_;
    
    for(my $i = 1 ; $i <= 2*$total_contracts_ ; $i++) {
      my $temp_call_shc_ = "NSE_".$underlying_."_C0_".$moneyness_tag_.$i;
      my $temp_put_shc_ = "NSE_".$underlying_."_P0_".$moneyness_tag_.$i;
      push(@list_shc_,$temp_call_shc_);    
      push(@list_shc_,$temp_put_shc_); 
    }

    if (index($tag_, "ExATM") == -1) {
      my $temp_call_shc_ = "NSE_".$underlying_."C0_A";
      my $temp_put_shc_ = "NSE_".$underlying_."P0_A";
      push(@list_shc_,$temp_call_shc_);    
      push(@list_shc_,$temp_put_shc_); 
    }

  }
  close STRATFILENAME ;
  return @list_shc_;
}


sub GetListOfUnderlying {
  my $t_full_strategy_name_ = shift;
  my @list_underlying_ = ();

  open STRATFILENAME, "< $t_full_strategy_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_strategy_name_\n" );

  while ( my $thisline_ = <STRATFILENAME> )
  {
    chomp($thisline_);
    next if $thisline_ =~ /^#|^$/;
    my @t_words_ = split ( /\s+/, $thisline_ );
    next if $#t_words_ < 0 ;
    my $t_full_model_name_ = $t_words_[2];
    open MODELFILENAME, "< $t_full_model_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_model_name_\n" );
    my $found_ = 0;
    while ( my $thismline_ = <MODELFILENAME> )
    {
      chomp($thismline_);
      next if $thismline_ =~ /^#|^$/;
      last if $thismline_ eq "INDVIDUAL_INDICATORS";
      last if $thismline_ eq "GLOBAL_INDICATORS";
      if ( $thismline_ eq "PARAMSHCPAIR" ) {
        $found_ = 1;
        next;
      } elsif ( $found_ == 1 ) {
        my @m_words_ = split ( /\s+/, $thismline_ );
        next if $#m_words_ < 0 ;
        my @uwords_ = split ( '_', $m_words_[0] );
        if ( $#uwords_ >= 1 )
        {
          push(@list_underlying_ , $uwords_[1]) ;
        }     
      } else {
        next;
      }
    }

  }
  close STRATFILENAME ;
  return @list_underlying_;
}


sub GetFullStratPathFromBaseOptions {
  my $t_strategy_base_name_ = shift;
  my $exec_cmd_ = "find /home/dvctrader/modelling/NSEOptionsMM/*Strats -name $t_strategy_base_name_";
  my $t_full_strategy_name_ = `$exec_cmd_`;
  return $t_full_strategy_name_;
}

1;
