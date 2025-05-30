# \file GenPerlLib/calc_prev_date.pl
# #
# # \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
# #  Address:
# #        Suite 217, Level 2, Prestige Omega,
# #        No 104, EPIP Zone, Whitefield,
# #        Bangalore - 560066, India
# #        +91 80 4060 0717
# #
#

use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
if( -d $LIVE_BIN_DIR)   {$BINDIR=$LIVE_BIN_DIR;}

sub GetCoreShortcodes
{
  my ( $shortcode_ ) = @_;
  my @core_shortcodes_ = ();
  if ( index ( $shortcode_ , "FESX") >= 0 )
  {
    push (@core_shortcodes_, "FDAX_0" );
    push (@core_shortcodes_, "ES_0" );
  }

  if ( index ( $shortcode_, "FDAX" ) >= 0 )
  {
    push (@core_shortcodes_, "FESX_0" );
    push (@core_shortcodes_, "ES_0" );
  }
  
  if ( index ( $shortcode_, "FGBL" ) >= 0 )
  {
    push ( @core_shortcodes_, "FGBS_0" );
    push ( @core_shortcodes_, "FGBM_0" ); 
    push ( @core_shortcodes_, "ZN_0" );
    push ( @core_shortcodes_, "LFR_0" );
  }

  if ( index ( $shortcode_, "FGBX" ) >= 0 )
  {
    push ( @core_shortcodes_, "FGBL_0" );
    push ( @core_shortcodes_, "FGBM_0" );
  }
  
  if ( index ( $shortcode_, "FGBM" ) >= 0 )
  {
    push ( @core_shortcodes_, "FGBS_0" );
    push ( @core_shortcodes_, "FGBL_0" );
    push ( @core_shortcodes_, "ZN_0" );
  }

  if ( index ( $shortcode_, "FGBS" ) >= 0 )
  {
    push ( @core_shortcodes_, "FGBL_0" );
    push ( @core_shortcodes_, "FGBM_0" );
    push ( @core_shortcodes_, "ZN_0" );
  }

  if ( index ( $shortcode_, "FOAT") >= 0 )
  {
    push ( @core_shortcodes_, "FGBL_0" );
    push ( @core_shortcodes_, "FGBM_0" );
    push ( @core_shortcodes_, "ZN_0" );
    push ( @core_shortcodes_, "LFR_0" );
  }

  if ( index ( $shortcode_, "FOAM") >= 0 )
  {
    push ( @core_shortcodes_, "FOAT_0" );
  }

  if ( index ( $shortcode_, "FBTP") >= 0 )
  {
    push ( @core_shortcodes_, "FOAT_0" );
    push ( @core_shortcodes_, "FGBL_0" );
    push ( @core_shortcodes_, "FGBM_0" );
    push ( @core_shortcodes_, "ZN_0" );
    push ( @core_shortcodes_, "LFR_0" );
    push (@core_shortcodes_, "FESX_0" );
    push (@core_shortcodes_, "ES_0" );
  }

  if ( index ( $shortcode_, "FBTS" ) >= 0 )
  {
    push ( @core_shortcodes_, "FBTP_0" );
  }

  if ( index ( $shortcode_, "HHI" ) >= 0 )
  {
    push ( @core_shortcodes_, "HSI_0" );
  }


  if ( index ( $shortcode_, "HSI" )  >= 0 )
  {
    push ( @core_shortcodes_, "HHI_0" );
  }

  if ( index ( $shortcode_, "MHI") >= 0 )
  {
    push ( @core_shortcodes_, "HSI_0" );
    push ( @core_shortcodes_, "HHI_0" );
  }

  if ( index ( $shortcode_, "MCH") >= 0 )
  {
    push ( @core_shortcodes_, "HHI_0" );
    push ( @core_shortcodes_, "HSI_0" );
  }

  if ( $shortcode_ eq "NK_0" )
  {
    push ( @core_shortcodes_, "NKM_0" );
  }

  if ( index ( $shortcode_, "NKM_0") >= 0 )
  {
    push ( @core_shortcodes_, "NK_0" );
  }

  if ( index ( $shortcode_, "NKM_1") >= 0 )
  {
    push ( @core_shortcodes_, "NKM_0" );
    push ( @core_shortcodes_, "NK_0" );
  }

  if ( index ( $shortcode_, "ZF_0") >=0 )
  {
    push ( @core_shortcodes_, "ZN_0" );
    push ( @core_shortcodes_, "ZB_0" );
    push ( @core_shortcodes_, "GE_0" );
  }

  if ( index ( $shortcode_, "ZN_0") >=0 )
  {
    push ( @core_shortcodes_, "FGBL_0" );
    push ( @core_shortcodes_, "ZB_0" );
    push ( @core_shortcodes_, "ZF_0" );
    push ( @core_shortcodes_, "ES_0" );
  }

  if ( index ( $shortcode_, "ZB_0") >=0 )
  {
    push ( @core_shortcodes_, "ES_0" );
    push ( @core_shortcodes_, "ZN_0" );
    push ( @core_shortcodes_, "ZF_0" );
  }

  if ( index ( $shortcode_, "UB_0") >=0 )
  {
    push ( @core_shortcodes_, "ZN_0" );
    push ( @core_shortcodes_, "ZB_0" );
  }


  if ( index ( $shortcode_, "LFR_0") >=0 )
  {
    push ( @core_shortcodes_, "FGBL_0" );
    push ( @core_shortcodes_, "FGBM_0" );
    push ( @core_shortcodes_, "ZN_0" );
  }
  
  if ( index ( $shortcode_, "YFEBM") >=0 )
  {
    push ( @core_shortcodes_, "ZW_0" );
    push ( @core_shortcodes_, "YFEBM_0" );
    push ( @core_shortcodes_, "YFEBM_1" );
  }

  if ( index ( $shortcode_, "XFC") >=0 )
  {
    push ( @core_shortcodes_, "XFC_0" );
    push ( @core_shortcodes_, "XFC_1" );
    push ( @core_shortcodes_, "XFRC_0" );
    push ( @core_shortcodes_, "XFRC_1" );
    push ( @core_shortcodes_, "YFEBM_0" );
    push ( @core_shortcodes_, "YFEBM_1" );
    push ( @core_shortcodes_, "YFEBM_2" );
  }
  
  if ( index ( $shortcode_, "LFZ_0") >=0 )
  {
    push ( @core_shortcodes_, "ES_0" );
    push ( @core_shortcodes_, "NQ_0" );
    push ( @core_shortcodes_, "YM_0" );
    push ( @core_shortcodes_, "FDAX_0" );
    push ( @core_shortcodes_, "FESX_0" );
    push ( @core_shortcodes_, "JFFCE_0" );
    push ( @core_shortcodes_, "KFFTI_0" );
  }

  if ( index ( $shortcode_, "JFFCE_0") >=0 )
  {
    push ( @core_shortcodes_, "ES_0" );
    push ( @core_shortcodes_, "NQ_0" );
    push ( @core_shortcodes_, "YM_0" );
    push ( @core_shortcodes_, "FDAX_0" );
    push ( @core_shortcodes_, "LFZ_0" );
    push ( @core_shortcodes_, "KFFTI_0" );
  }

  if ( index ( $shortcode_, "KFFTI_0") >=0 )
  {
    push ( @core_shortcodes_, "ES_0" );
    push ( @core_shortcodes_, "NQ_0" );
    push ( @core_shortcodes_, "YM_0" );
    push ( @core_shortcodes_, "FDAX_0" );
    push ( @core_shortcodes_, "FESX_0" );
    push ( @core_shortcodes_, "FESX_0" );
    push ( @core_shortcodes_, "LFZ_0" );
    push ( @core_shortcodes_, "JFFCE_0" );
  }

  if ( index ( $shortcode_, "CGB_0") >=0 )
  {
    push ( @core_shortcodes_, "ZN_0" );
    push ( @core_shortcodes_, "ZB_0" );
  }
  
  if ( index ( $shortcode_, "CGF_0") >=0 )
  {
    push ( @core_shortcodes_, "CGB_0" );
  }

  if ( index ( $shortcode_, "CGZ_0") >=0 )
  {
    push ( @core_shortcodes_, "CGB_0" );
  }

  if ( index ( $shortcode_, "BR_IND_0") >=0 )
  {
    push ( @core_shortcodes_, "BR_WIN_0" );
    push ( @core_shortcodes_, "ES_0" );
  }

  if ( index ( $shortcode_, "BR_WIN_0") >=0 )
  {
    push ( @core_shortcodes_, "BR_WIN_0" );
    push ( @core_shortcodes_, "ES_0" );
  }
  
  if ( index ( $shortcode_, "BR_DOL_0") >=0 )
  {
    push ( @core_shortcodes_, "ES_0" );
    push ( @core_shortcodes_, "6A_0" );
    push ( @core_shortcodes_, "6B_0" );
    push ( @core_shortcodes_, "6E_0" );
    push ( @core_shortcodes_, "6C_0" );
    push ( @core_shortcodes_, "6M_0" );
  }

  if ( index ( $shortcode_, "DI1F17") >=0 )
  {
    push ( @core_shortcodes_, "DI1F16" );
    push ( @core_shortcodes_, "DI1F18" );
  }

  if ( index ( $shortcode_, "DI1F16") >=0 )
  {
    push ( @core_shortcodes_, "DI1F15" );
    push ( @core_shortcodes_, "DI1F18" );
  }

  if ( index ( $shortcode_, "DI1F18") >=0 )
  {
    push ( @core_shortcodes_, "DI1F16" );
    push ( @core_shortcodes_, "BR_DOL_0" );
  }

  if ( index ( $shortcode_, "RI_0") >=0 )
  {
    push ( @core_shortcodes_, "ES_0" );
    push ( @core_shortcodes_, "NQ_0" );
    push ( @core_shortcodes_, "YM_0" );
    push ( @core_shortcodes_, "FESX_0" ); 
    push ( @core_shortcodes_, "FDAX_0" );
    push ( @core_shortcodes_, "Si_0" );
  }

  if ( index ( $shortcode_, "GD_0") >=0 )
  {
    push ( @core_shortcodes_, "GC_0" );
  }

  if ( index ( $shortcode_, "ED_0") >=0 )
  {
    push ( @core_shortcodes_, "6E_0" );
  }

  if ( index ( $shortcode_, "Si_0") >=0 )
  {
    push ( @core_shortcodes_, "USD000UTSTOM" );
    push ( @core_shortcodes_, "USD000000TOD" );
  }

  if ( index ( $shortcode_, "USD000UTSTOM" ) >=0 )
  {
    push ( @core_shortcodes_, "Si_0" );
    push ( @core_shortcodes_, "USD000000TOD" );
  }

  if ( index ( $shortcode_, "USD000000TOD" ) >=0 )
  {
    push ( @core_shortcodes_, "Si_0" );
    push ( @core_shortcodes_, "USD000UTSTOM" );
  }
  
  if ( $shortcode_ eq "XT_0" )
  {
    push ( @core_shortcodes_, "YT_0" );
  }
  elsif ( $shortcode_ eq "YT_0" )
  {
    push ( @core_shortcodes_, "XT_0" );
  }
  elsif ( index( $shortcode_, "IR_") == 0 )
  {
    push ( @core_shortcodes_, "YT_0" );
    push ( @core_shortcodes_, "XT_0" );
    if ( $shortcode_ eq "IR_0" ) 
    { 
      push ( @core_shortcodes_, "IR_1"); 
    }
    elsif ( $shortcode_ eq "IR_1" ) 
    { 
      push ( @core_shortcodes_, "IR_0"); 
    }
    elsif ( $shortcode_ eq "IR_2" ) 
    { 
      push ( @core_shortcodes_, "IR_1"); 
      push ( @core_shortcodes_, "IR_3"); 
    }
    elsif ( $shortcode_ eq "IR_3" ) 
    { 
      push ( @core_shortcodes_, "IR_1"); 
      push ( @core_shortcodes_, "IR_2"); 
    }
    elsif ( $shortcode_ eq "IR_4" ) 
    { 
      push ( @core_shortcodes_, "IR_1"); 
      push ( @core_shortcodes_, "IR_3"); 
    }
  }


 @core_shortcodes_;
  
}

if ($#ARGV <  0 )
{
  print "Usage: $0  SHORTCODE.\n";
}
else
{
  my @core_shc_ = GetCoreShortcodes ( $ARGV[0] );
  foreach my $sh_ ( @core_shc_ )
  {
    print $sh_." ";
  }
  print "\n";
}
