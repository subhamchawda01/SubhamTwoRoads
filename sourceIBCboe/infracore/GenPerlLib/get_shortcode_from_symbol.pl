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
my $REPO="infracore";

my $INSTALL_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin/";

sub GetShortcodeFromSymbol
{
    my $symbol_ = shift;
    my $date_ = shift;
    my $shortcode_ = $symbol_;
    my @lfi_shc_array_ = ( "LFI_0", "LFI_1", "LFI_2", "LFI_3","LFI_4","LFI_5","LFI_6","LFI_7","LFI_8","LFI_9","LFI_10","LFI_11","LFI_12","LFI_13","LFI_14","LFI_15",
                       "SP_LFI0_LFI1", "SP_LFI1_LFI2", "SP_LFI2_LFI3", "SP_LFI3_LFI4", "SP_LFI4_LFI5", "SP_LFI5_LFI6", "SP_LFI6_LFI7", "SP_LFI7_LFI8", "SP_LFI8_LFI9", "SP_LFI9_LFI10" ,
                       "SP_LFI10_LFI11", "SP_LFI11_LFI12", "SP_LFI0_LFI2" , "SP_LFI1_LFI3" , "SP_LFI2_LFI4" , "SP_LFI3_LFI5" , "SP_LFI4_LFI6" , "SP_LFI5_LFI7" , "SP_LFI6_LFI8" , "SP_LFI7_LFI9" ,
                       "SP_LFI8_LFI10", "SP_LFI9_LFI11", "SP_LFI10_LFI12", "B_LFI0_LFI1_LFI2" , "B_LFI1_LFI2_LFI3", "B_LFI2_LFI3_LFI4", "B_LFI3_LFI4_LFI5" , "B_LFI4_LFI5_LFI6" ) ;

    my @lfl_shc_array_ = ( "LFL_0", "LFL_1", "LFL_2", "LFL_3","LFL_4","LFL_5","LFL_6","LFL_7","LFL_8","LFL_9","LFL_10","LFL_11","LFL_12","LFL_13","LFL_14","LFL_15" );

    my @vx_shc_array_ = ( "VX_0", "VX_1", "VX_2","VX_3","VX_4","VX_5","VX_6","VX_7","SP_VX0_VX1","SP_VX1_VX2","SP_VX2_VX3","SP_VX0_VX2","SP_VX0_VX3","SP_VX1_VX3","SP_VX0_VX4",
                       "SP_VX1_VX4", "SP_VX0_VX5", "SP_VX3_VX4", "SP_VX1_VX5", "SP_VX2_VX4", "SP_VX2_VX5", "SP_VX4_VX5", "SP_VX0_VX6", "SP_VX3_VX5", "SP_VX1_VX6", "SP_VX2_VX6",
                       "SP_VX5_VX6", "SP_VX3_VX6", "SP_VX4_VX6", "SP_VX7_VX0", "SP_VX7_VX3", "SP_VX7_VX1", "SP_VX7_VX4", "SP_VX7_VX5", "SP_VX7_VX2","SP_VX7_VX6","SP_VX0_VX1_VX2" );

    my @fvs_array_ = ("FVS_0", "FVS_1", "FVS_2", "FVS_3", "FVS_4", "FVS_5", "FVS_6","FVS_7" );
    my @bax_array_ = ("BAX_0","BAX_1","BAX_2","BAX_3","BAX_1","BAX_2","BAX_3","BAX_4","BAX_5","BAX_6","BAX_7","BAX_8","BAX_9","SP_BAX0_BAX1","SP_BAX0_BAX2","SP_BAX0_BAX3","SP_BAX1_BAX2","SP_BAX2_BAX3","SP_BAX3_BAX4","SP_BAX1_BAX3","SP_BAX2_BAX4","SP_BAX3_BAX5","SP_BAX1_BAX2","SP_BAX2_BAX3","SP_BAX3_BAX4","SP_BAX1_BAX3","SP_BAX2_BAX4","SP_BAX3_BAX5","SP_BAX4_BAX5","SP_BAX5_BAX6","SP_BAX6_BAX7","SP_BAX7_BAX8","SP_BAX4_BAX6","SP_BAX5_BAX7","SP_BAX6_BAX8","SP_BAX7_BAX9");
    my @ge_array_ = ( "GE_0","GE_1","GE_2","GE_3","GE_4","GE_5","GE_6","GE_7","GE_8","GE_9","GE_10","GE_11","GE_12","GE_13","GE_14","GE_15","GE_16","SP_GE0_GE1","SP_GE1_GE2","SP_GE2_GE3","SP_GE3_GE4","SP_GE4_GE5","SP_GE5_GE6","SP_GE6_GE7","SP_GE7_GE8","SP_GE8_GE9","SP_GE9_GE10","SP_GE10_GE11","SP_GE11_GE12","SP_GE12_GE13","SP_GE13_GE14","SP_GE14_GE15","SP_GE15_GE16" ) ;
    
    my @zt_array_ = ( "ZT_0","ZT_1" );
    my @zf_array_ = ( "ZF_0", "ZF_1" );
    my @zn_array_ = ( "ZN_0", "ZN_1" );
    my @zb_array_ = ( "ZB_0", "ZB_0" );
    my @a6_array_ = ( "6A_0","6A_1" ) ;
    my @b6_array_ = ( "6B_0","6B_1" ) ;
    my @c6_array_ = ( "6C_0","6C_1" ) ; 
    my @e6_array_ = ( "6E_0","6E_1" ) ; 
    my @j6_array_ = ( "6J_0","6J_1" ) ;
    my @l6_array_ = ( "6L_0","6L_1" ) ; 
    my @m6_array_ = ( "6M_0","6M_1" ) ; 
    my @n6_array_ = ( "6N_0","6N_1" ) ; 
    my @r6_array_ = ( "6R_0","6R_1" ) ; 
    my @s6_array_ = ( "6S_0","6S_1" ) ; 
    my @z6_array_ = ( "6Z_0","6Z_1" ) ; 
    my @gc_array_ = ( "GC_0","GC_1","GC_2" ) ;
    my @ym_array_ = ( "YM_0","YM_1" ) ;
    my @cl_array_ = ( "CL_0","CL_1","CL_2" );
    my @es_array_ = ( "ES_0","ES_1" ) ;
    
    my @yt_array_ = ( "YT_0","YT_1" ) ;
    my @xt_array_ = ( "XT_0","XT_1" ) ;
    my @ap_array_ = ( "AP_0","AP_1" ) ;
    
    my @fgbs_array_ = ( "FGBS_0", "FGBS_1"); 
    my @fgbl_array_ = ( "FGBL_0", "FGBL_1");
    my @fgbm_array_ = ( "FGBM_0", "FGBM_1");
    my @fesx_array_ = ( "FESX_0", "FESX_1");
    
    my @vsw_array_ = ( "VSW_0", "VSW_1", "VSW_2", "VSW_3") ;
    
    my @hsi_array_ = ( "HSI_0","HSI_1","HSI_2","HSI_3" ) ; 
    my @hhi_array_ = ( "HHI_0","HHI_1","HHI_2","HHI_3" ) ;
    my @mhi_array_ = ( "MHI_0","MHI_1" ) ;
    my @mch_array_ = ( "MCH_0","MCH_1" ) ;
    
    my @ir_array_ = ( "IR_0","IR_1","IR_2","IR_3","IR_4" ) ;
    
    my @jgbl_array_ = ( "JGBL_0","JGBL_1") ;
    my @jgbsl_array_ = ( "JGBSL_0","JGBSL_1") ;
    my @jgblm_array_ = ( "JGBLM_0","JGBLM_1") ;
    my @jp400_array_ = ( "JP400_0","JP400_1","JP400_2","JP400_3","JP400_0", ) ;
    my @nkm_array_ = ( "NKM_0","NKM_1","NKM_2","NKM_3","NKM_4", "NKMF_0") ;
    my @nk_array_ = ( "NK_0","NK_1","NK_2","NK_3","NK_4" ) ;
    my @topix_array_ = ( "TOPIX_0","TOPIX_1" ) ;
    my @topixm_array_ = ( "TOPIXM_0","TOPIXM_1" ) ;
    
    my @lfz_array_ = ( "LFZ_0","LFZ_1" ) ;
    my @sek_array_ = ( "SEK_0","SEK_1" ) ;
    
    my @yfebm_array_ = ( "YFEBM_0","YFEBM_1","YFEBM_2","YFEBM_3","YFEBM_4","YFEBM_5" ) ;
    my @xfc_array_ =  ( "XFC_0", "XFC_1", "XFC_2", "XFC_3", "XFC_4" ) ;
    
    my @cgb_array_ = ( "CGB_0","CGB_1","CGB_2","CGB_3" ) ;
    my @cgf_array_ = ( "CGF_0","CGF_1" ) ; 
    my @cgz_array_ = ( "CGZ_0","CGZ_1" ) ;
      
    my @shc_array_ = () ;
     
    if ( substr ( $symbol_, 0, 3 ) eq "DJ1" ) { $shortcode_ = "DJ1_0"; }
    if ( substr ( $symbol_, 0, 3 ) eq "CGB" ) { $shortcode_ = "CGB_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "SXF" ) { $shortcode_ = "SXF_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "DI1F" ) { 
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
	$shortcode_ = "DI1F16"; # lazy
    }
    elsif ( substr ( $symbol_, 0, 4 ) eq "DI1N" ) { 
# switch to using shortcode to symbol tool
#      for ( my $roll_number_ = 0; $roll_number_ <= 9; $roll_number_ ++ ) {
#	
#      }
	$shortcode_ = "DI1F16"; # lazy
    }
    elsif ( substr ( $symbol_, 0, 3 ) eq "NKD" ) { $shortcode_ = "NKD_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "NIY" ) { $shortcode_ = "NIY_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "ZW" ) { $shortcode_ = "ZW_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "ZC" ) { $shortcode_ = "ZC_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "ZS" ) { $shortcode_ = "ZS_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "XW" ) { $shortcode_ = "XW_0"; }
    
    elsif ( substr ( $symbol_, 0, 4 ) eq "FBTP" ) { $shortcode_ = "FBTP_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FBTS" ) { $shortcode_ = "FBTS_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FESX" ) { $shortcode_ = "FESX_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FDAX" ) { $shortcode_ = "FDAX_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FGBS" ) { $shortcode_ = "FGBS_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FGBM" ) { $shortcode_ = "FGBM_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FGBL" ) { $shortcode_ = "FGBL_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FGBX" ) { $shortcode_ = "FGBX_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FOAT" ) { $shortcode_ = "FOAT_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FGBX" ) { $shortcode_ = "FBBX_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FBTS" ) { $shortcode_ = "FBTS_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FBTP" ) { $shortcode_ = "FBTP_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FBTM" ) { $shortcode_ = "FBTM_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FOAT" ) { $shortcode_ = "FOAT_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FOAM" ) { $shortcode_ = "FOAM_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "CONF" ) { $shortcode_ = "CONF_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FDAX" ) { $shortcode_ = "FDAX_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FSMI" ) { $shortcode_ = "FSMI_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FXXP" ) { $shortcode_ = "FXXP_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FEXF" ) { $shortcode_ = "FEXF_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FESB" ) { $shortcode_ = "FESB_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FSTB" ) { $shortcode_ = "FSTB_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FSTS" ) { $shortcode_ = "FSTS_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FSTO" ) { $shortcode_ = "FSTO_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FSTG" ) { $shortcode_ = "FSTG_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FSTI" ) { $shortcode_ = "FSTI_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FSTM" ) { $shortcode_ = "FSTM_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FEXD" ) { $shortcode_ = "FEXD_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FRDX" ) { $shortcode_ = "FRDX_0"; }
    elsif ( substr ( $symbol_, 0, 4 ) eq "FEU3" ) { $shortcode_ = "FEU3_0"; }
    
    elsif ( substr ( $symbol_, 0, 5 ) eq "JFFCE" ) { $shortcode_ = "JFFCE_0"; }
    elsif ( substr ( $symbol_, 0, 5 ) eq "KFFTI" ) { $shortcode_ = "KFFTI_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "LFR" ) { $shortcode_ = "LFR_0"; }

    elsif ( substr ( $symbol_, 0, 2 ) eq "RI" ) { $shortcode_ = "RI_0"; }
    elsif ( substr ( $symbol_, 0, 2 ) eq "Si" ) { $shortcode_ = "Si_0"; }
    elsif ( substr ( $symbol_, 0, 12 ) eq "USD000UTSTOM" ) { $shortcode_ = "USD000UTSTOM"; }

    elsif ( substr ( $symbol_, 0, 3 ) eq "WIN" ) { $shortcode_ = "BR_WIN_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "IND" ) { $shortcode_ = "BR_IND_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "DOL" ) { $shortcode_ = "BR_DOL_0"; }
    elsif ( substr ( $symbol_, 0, 3 ) eq "WDO" ) { $shortcode_ = "BR_DOL_0"; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "LFI" ) ) { @shc_array_ = @lfi_shc_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "LFL" ) ){ @shc_array_ = @lfl_shc_array_; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "VX" ) ) { @shc_array_ = @vx_shc_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq  "NKM" ) ) { @shc_array_ = @nkm_array_ ;}
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "FVS" ) )  { @shc_array_ = @fvs_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "BAX" ) ) { @shc_array_ = @bax_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "GE" ) ) { @shc_array_ = @ge_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "ZT" ) ) {  @shc_array_ = @zt_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "ZF" ) ) { @shc_array_ = @zf_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "ZN" ) ) { @shc_array_ = @zn_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "ZB" ) ) { @shc_array_ = @zb_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6A" ) ) { @shc_array_ = @a6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6B" ) ) { @shc_array_ = @b6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6C" ) ) { @shc_array_ = @c6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6E" ) ) { @shc_array_ = @e6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6J" ) ) { @shc_array_ = @j6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6L" ) ) { @shc_array_ = @l6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6M" ) ) { @shc_array_ = @m6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6N" ) ) { @shc_array_ = @n6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6R" ) ) { @shc_array_ = @r6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6S" ) ) { @shc_array_ = @s6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "6Z" ) ) { @shc_array_ = @z6_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "GC" ) ) { @shc_array_ = @gc_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "YM" ) ) { @shc_array_ = @ym_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "CL" ) ) { @shc_array_ = @cl_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "ES" ) ) { @shc_array_ = @es_array_ ; }
    
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "YT" ) ) { @shc_array_ = @yt_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "XT" ) ) { @shc_array_ = @xt_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "AP" ) ) { @shc_array_ = @ap_array_ ; }
    
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "HSI" ) ) { @shc_array_ = @hsi_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "HHI" ) ) { @shc_array_ = @hhi_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "MHI" ) ) { @shc_array_ = @mhi_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 4 ) eq "MCH" ) ) { @shc_array_ = @mch_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 2 ) eq "IR" ) ) { @shc_array_ = @ir_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 4 ) eq "JGBL" ) ) { @shc_array_ = @jgbl_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 5 ) eq "JGBSL" ) ) { @shc_array_ = @jgbsl_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 5 ) eq "JGBLM" ) ) { @shc_array_ = @jgblm_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 5 ) eq "JP400" ) ) { @shc_array_ = @jp400_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "NKM" ) ) { @shc_array_ = @nkm_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "NK1" ) ) { @shc_array_ = @nk_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 5 ) eq "TOPIX" ) ) { @shc_array_ = @topix_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 6 ) eq "TOPIXM" ) ) { @shc_array_ = @topixm_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "LFZ" ) ) { @shc_array_ = @lfz_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "SEK" ) ) { @shc_array_ = @sek_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 5 ) eq "YFEBM" ) ) { @shc_array_ = @yfebm_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 5 ) eq "XFC" ) ) { @shc_array_ = @xfc_array_ ; }
    
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "CGB" ) ) { @shc_array_ = @cgb_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "CGF" ) ) { @shc_array_ = @cgf_array_ ; }
    elsif ( ( substr ( $symbol_, 0, 3 ) eq "CGZ" ) ) { @shc_array_ = @cgz_array_ ; }    
        

	if ( $#shc_array_ >= 0 )
	{
		foreach my $shc_ ( @shc_array_ )
		{
			my $exec_cmd_ = "$INSTALL_BIN_DIR/get_exchange_symbol $shc_ $date_:"; 
			my $exchange_symbol_ = `$exec_cmd_`; chomp ( $exchange_symbol_ ) ;
			$exchange_symbol_ =~ s/ /~/g ;
			if ( index ( $symbol_, $exchange_symbol_ ) >= 0 )  { return $shc_ ; }
		}
    }
    $shortcode_;
}

if ( $0 eq __FILE__ ) {
  if ( $#ARGV >= 0 ) {
    print GetShortcodeFromSymbol ( $ARGV[0], $ARGV[1] );
  }
}

1
