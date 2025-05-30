# \file GenPerlLib/get_shortcode_from_symbol.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
use strict;
use warnings;
use feature "switch";
my $HOME_DIR=$ENV{'HOME'};
my $USER=$ENV{'USER'};
my $REPO="basetrade";

my $INSTALL_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin/";

sub GetShortcodeFromSymbol
{
    my $symbol_ = shift;
    my $date_ = shift;
    my $shortcode_ = "";
    my @lfi_shc_array_ = ( "LFI_0", "LFI_1", "LFI_2", "LFI_3","LFI_4","LFI_5","LFI_6","LFI_7","LFI_8","LFI_9","LFI_10","LFI_11","LFI_12","LFI_13","LFI_14","LFI_15",
                       "SP_LFI0_LFI1", "SP_LFI1_LFI2", "SP_LFI2_LFI3", "SP_LFI3_LFI4", "SP_LFI4_LFI5", "SP_LFI5_LFI6", "SP_LFI6_LFI7", "SP_LFI7_LFI8", "SP_LFI8_LFI9", "SP_LFI9_LFI10" ,
                       "SP_LFI10_LFI11", "SP_LFI11_LFI12", "SP_LFI0_LFI2" , "SP_LFI1_LFI3" , "SP_LFI2_LFI4" , "SP_LFI3_LFI5" , "SP_LFI4_LFI6" , "SP_LFI5_LFI7" , "SP_LFI6_LFI8" , "SP_LFI7_LFI9" ,
                       "SP_LFI8_LFI10", "SP_LFI9_LFI11", "SP_LFI10_LFI12", "B_LFI0_LFI1_LFI2" , "B_LFI1_LFI2_LFI3", "B_LFI2_LFI3_LFI4", "B_LFI3_LFI4_LFI5" , "B_LFI4_LFI5_LFI6" ) ;

    my @lfl_shc_array_ = ( "LFL_0", "LFL_1", "LFL_2", "LFL_3","LFL_4","LFL_5","LFL_6","LFL_7","LFL_8","LFL_9","LFL_10","LFL_11","LFL_12","LFL_13","LFL_14","LFL_15" );

    my @vx_shc_array_ = ( "VX_0", "VX_1", "VX_2","VX_3","VX_4","VX_5","VX_6","VX_7","SP_VX0_VX1","SP_VX1_VX2","SP_VX2_VX3","SP_VX0_VX2","SP_VX0_VX3","SP_VX1_VX3","SP_VX0_VX4",
                       "SP_VX1_VX4", "SP_VX0_VX5", "SP_VX3_VX4", "SP_VX1_VX5", "SP_VX2_VX4", "SP_VX2_VX5", "SP_VX4_VX5", "SP_VX0_VX6", "SP_VX3_VX5", "SP_VX1_VX6", "SP_VX2_VX6",
                       "SP_VX5_VX6", "SP_VX3_VX6", "SP_VX4_VX6", "SP_VX7_VX0", "SP_VX7_VX3", "SP_VX7_VX1", "SP_VX7_VX4", "SP_VX7_VX5", "SP_VX7_VX2","SP_VX7_VX6","SP_VX0_VX1_VX2" );

    my @fvs_array_ = ("FVS_0", "FVS_1", "FVS_2", "FVS_3", "FVS_4", "FVS_5", "FVS_6","FVS_7" );
    my @nkm_array_ = ("NKM_0", "NKMF_0" );
    my @bax_array_ = ("BAX_0","BAX_1","BAX_2","BAX_3","BAX_1","BAX_2","BAX_3","BAX_4","BAX_5","BAX_6","BAX_7","BAX_8","BAX_9","SP_BAX0_BAX1","SP_BAX0_BAX2","SP_BAX0_BAX3","SP_BAX1_BAX2","SP_BAX2_BAX3","SP_BAX3_BAX4","SP_BAX1_BAX3","SP_BAX2_BAX4","SP_BAX3_BAX5","SP_BAX1_BAX2","SP_BAX2_BAX3","SP_BAX3_BAX4","SP_BAX1_BAX3","SP_BAX2_BAX4","SP_BAX3_BAX5","SP_BAX4_BAX5","SP_BAX5_BAX6","SP_BAX6_BAX7","SP_BAX7_BAX8","SP_BAX4_BAX6","SP_BAX5_BAX7","SP_BAX6_BAX8","SP_BAX7_BAX9");
    my @ir_array_ = ("IR_0", "IR_1", "IR_2", "IR_3", "IR_4");
    if ( substr ( $symbol_, 0, 3 ) eq "CGB" ) { $shortcode_ = "CGB_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "SXF" ) { $shortcode_ = "SXF_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "DI1F" ) { 
# switch to using shortcode to symbol tool
#      for ( my $roll_number_ = 0; $roll_number_ <= 9; $roll_number_ ++ ) {
#	
#      }
      if ( substr ( $symbol_, 0, 6 ) eq "DI1F14" ) { $shortcode_ = "DI1F15"; }
      elsif ( substr ( $symbol_, 0, 6 ) eq "DI1F15" ) { $shortcode_ = "DI1F16"; }
      elsif ( substr ( $symbol_, 0, 6 ) eq "DI1F16" ) { $shortcode_ = "DI1F17"; }
      elsif ( substr ( $symbol_, 0, 6 ) eq "DI1F17" ) { $shortcode_ = "DI1F18"; }
      elsif ( substr ( $symbol_, 0, 6 ) eq "DI1F18" ) { $shortcode_ = "DI1F19"; }
      elsif ( substr ( $symbol_, 0, 6 ) eq "DI1F19" ) { $shortcode_ = "DI1F20"; }
      elsif ( substr ( $symbol_, 0, 6 ) eq "DI1F20" ) { $shortcode_ = "DI1F21"; }
      elsif ( substr ( $symbol_, 0, 6 ) eq "DI1F21" ) { $shortcode_ = "DI1F22"; }
      
    }
    elsif ( substr ( $symbol_, 0, 4 ) eq "DI1J" ) { 
# switch to using shortcode to symbol tool
#      for ( my $roll_number_ = 0; $roll_number_ <= 9; $roll_number_ ++ ) {
#	
#      }
	$shortcode_ = "DI1F16"; # lazy
    }
    elsif ( substr ( $symbol_, 0, 4 ) eq "DI1N" ) { 
# switch to using shortcode to symbol tool
#      for ( my $roll_number_ = 0; $roll_number_ <= 9; $roll_number_ ++ ) {
#	
#      }
	$shortcode_ = "DI1F16"; # lazy
    }
    elsif ( substr ( $symbol_, 0, 3 ) eq "DOL" ) { $shortcode_ = "BR_DOL_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FBTP" ) { $shortcode_ = "FBTP_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FBTS" ) { $shortcode_ = "FBTS_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FESX" ) { $shortcode_ = "FESX_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FDAX" ) { $shortcode_ = "FDAX_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FGBS" ) { $shortcode_ = "FGBS_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FGBM" ) { $shortcode_ = "FGBM_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FGBL" ) { $shortcode_ = "FGBL_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FGBX" ) { $shortcode_ = "FGBX_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FOAT" ) { $shortcode_ = "FOAT_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "MHI" ) { $shortcode_ = "MHI_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "HHI" ) { $shortcode_ = "HHI_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "HSI" ) { $shortcode_ = "HSI_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "IND" ) { $shortcode_ = "BR_IND_0"; }
    elsif ( substr ( $symbol_, 0, 5 ) eq "JFFCE" ) { $shortcode_ = "JFFCE_0"; }
    elsif ( substr ( $symbol_, 0, 5 ) eq "KFFTI" ) { $shortcode_ = "KFFTI_0"; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "LFI" )  || ( substr ( $symbol_, 0, 3 ) eq "LFL" ) || (  substr ( $symbol_, 0, 2 ) eq "VX" ) || ( substr ( $symbol_, 0 , 3 ) eq "FVS") ||
            substr ( $symbol_, 0 , 3 ) eq "NKM" || ( substr ( $symbol_, 0, 3 ) eq "BAX" ) || ( substr ( $symbol_, 0, 2 ) eq "IR"  ) )  
    {
      my @shc_array_ = () ;
      if ( ( substr ( $symbol_, 0, 3 ) eq "LFI" ) )
      {
        @shc_array_ = @lfi_shc_array_ ;
      }
      elsif ( ( substr ( $symbol_, 0, 3 ) eq "LFL" ) ) 
      {
        @shc_array_ = @lfl_shc_array_
      }
      elsif ( (  substr ( $symbol_, 0, 2 ) eq "VX" ) )
      {
        @shc_array_ = @vx_shc_array_ ;
      }
      elsif ( ( substr ( $symbol_, 0, 3 ) eq  "NKM" ) ) 
      {
        @shc_array_ = @nkm_array_ ;
      }
      elsif ( ( substr ( $symbol_, 0, 3 ) eq "FVS" ) ) 
      {
        @shc_array_ = @fvs_array_ ;
      }
      elsif ( ( substr ( $symbol_, 0, 3 ) eq "BAX" ) ) 
      {
        @shc_array_ = @bax_array_ ;
      }
      elsif ( ( substr ( $symbol_, 0, 2 ) eq "IR" ) ) 
      {
        @shc_array_ = @ir_array_ ;
      }

      foreach my $shc_ ( @shc_array_ )
      {
        my $exec_cmd_ = "$INSTALL_BIN_DIR/get_exchange_symbol $shc_ $date_:"; 
        my $exchange_symbol_ = `$exec_cmd_`; chomp ( $exchange_symbol_ );
        $exchange_symbol_ =~ s/ /~/g;
        if ( index ( $symbol_, $exchange_symbol_ ) >= 0 ) 
        {
          return $shc_ ;
        }
      }
    }
    elsif ( substr ( $symbol_, 0, 3 ) eq "LFR" ) { $shortcode_ = "LFR_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "LFZ" ) { $shortcode_ = "LFZ_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "NKM" ) { $shortcode_ = "NKM_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "NK1" ) { $shortcode_ = "NK_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "RI" ) { $shortcode_ = "RI_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "Si" ) { $shortcode_ = "Si_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "ZT" ) { $shortcode_ = "ZT_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "ZF" ) { $shortcode_ = "ZF_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "ZN" ) { $shortcode_ = "ZN_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "ZB" ) { $shortcode_ = "ZB_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "UB" ) { $shortcode_ = "UB_0"; }
    elsif ( substr ( $symbol_, 0, 12 ) eq "USD000UTSTOM" ) { $shortcode_ = "USD000UTSTOM"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "WIN" ) { $shortcode_ = "BR_WIN_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "XT" ) { $shortcode_ = "XT_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "YT" ) { $shortcode_ = "YT_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "AP" ) { $shortcode_ = "AP_0"; }

    $shortcode_;
}

if ( $0 eq __FILE__ ) {
  if ( $#ARGV >= 0 ) {
    print GetShortcodeFromSymbol ( $ARGV[0], $ARGV[1] );
  }
}

1
