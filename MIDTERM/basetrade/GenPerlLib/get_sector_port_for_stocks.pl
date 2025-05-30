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

sub GetSectorPortForStocks
{
  my ( $shortcode_ , $fraction_ ) = @_;
  my @portfolios_ = () ;
  if ( $fraction_ == 0 )
  {
    if ( index ( $shortcode_ , "EMBR3" ) >= 0 ) { push ( @portfolios_ , "BMFFEQCgTe" ) ; }
    if ( index ( $shortcode_ , "POMO4" ) >= 0 ) { push ( @portfolios_ , "BMFFEQCgTe" ) ; }
    if ( index ( $shortcode_ , "BRFS3" ) >= 0 ) { push ( @portfolios_ , "BMFEQCncFp" ) ; }
    if ( index ( $shortcode_ , "CSAN3" ) >= 0 ) { push ( @portfolios_ , "BMFEQCncFp" ) ; }
    if ( index ( $shortcode_ , "JBSS3" ) >= 0 ) { push ( @portfolios_ , "BMFEQCncFp" ) ; }
    if ( index ( $shortcode_ , "MRFG3" ) >= 0 ) { push ( @portfolios_ , "BMFEQCncFp" ) ; }
    if ( index ( $shortcode_ , "ABEV3" ) >= 0 ) { push ( @portfolios_ , "BMFEQCnc" ) ; }
    if ( index ( $shortcode_ , "PCAR4" ) >= 0 ) { push ( @portfolios_ , "BMFEQCnc" ) ; }
    if ( index ( $shortcode_ , "CRUZ3" ) >= 0 ) { push ( @portfolios_ , "BMFEQCnc" ) ; }
    if ( index ( $shortcode_ , "NATU3" ) >= 0 ) { push ( @portfolios_ , "BMFEQCnc" ) ; }
    if ( index ( $shortcode_ , "HYPE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQCnc" ) ; }
    if ( index ( $shortcode_ , "CYRE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQConsTr" ) ; }
    if ( index ( $shortcode_ , "EVEN3" ) >= 0 ) { push ( @portfolios_ , "BMFEQConsTr" ) ; }
    if ( index ( $shortcode_ , "GFSA3" ) >= 0 ) { push ( @portfolios_ , "BMFEQConsTr" ) ; }
    if ( index ( $shortcode_ , "MRVE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQConsTr" ) ; }
    if ( index ( $shortcode_ , "PDGR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQConsTr" ) ; }
    if ( index ( $shortcode_ , "ALLL3" ) >= 0 ) { push ( @portfolios_ , "BMFEQConsTr" ) ; }
    if ( index ( $shortcode_ , "RUMO3" ) >= 0 ) { push ( @portfolios_ , "BMFEQConsTr" ) ; }
    if ( index ( $shortcode_ , "CCRO3" ) >= 0 ) { push ( @portfolios_ , "BMFEQConsTr" ) ; }
    if ( index ( $shortcode_ , "ECOR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQConsTr" ) ; }
    if ( index ( $shortcode_ , "GOLL4" ) >= 0 ) { push ( @portfolios_ , "BMFEQConsTr" ) ; }
    if ( index ( $shortcode_ , "LAME4" ) >= 0 ) { push ( @portfolios_ , "BMFEQRet" ) ; }
    if ( index ( $shortcode_ , "LREN3" ) >= 0 ) { push ( @portfolios_ , "BMFEQRet" ) ; }
    if ( index ( $shortcode_ , "HGTX3" ) >= 0 ) { push ( @portfolios_ , "BMFEQRet" ) ; }
    if ( index ( $shortcode_ , "QUAL3" ) >= 0 ) { push ( @portfolios_ , "BMFEQRet" ) ; }
    if ( index ( $shortcode_ , "ESTC3" ) >= 0 ) { push ( @portfolios_ , "BMFEQDiv" ) ; }
    if ( index ( $shortcode_ , "KROT3" ) >= 0 ) { push ( @portfolios_ , "BMFEQDiv" ) ; }
    if ( index ( $shortcode_ , "RENT3" ) >= 0 ) { push ( @portfolios_ , "BMFEQDiv" ) ; }
    if ( index ( $shortcode_ , "BRML3" ) >= 0 ) { push ( @portfolios_ , "BMFEQRlst" ) ; }
    if ( index ( $shortcode_ , "BRPR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQRlst" ) ; }
    if ( index ( $shortcode_ , "MULT3" ) >= 0 ) { push ( @portfolios_ , "BMFEQRlst" ) ; }
    if ( index ( $shortcode_ , "BRAP4" ) >= 0 ) { push ( @portfolios_ , "BMFEQRlst" ) ; }
    if ( index ( $shortcode_ , "UGPA3" ) >= 0 ) { push ( @portfolios_ , "BMFEQRlst" ) ; }
    if ( index ( $shortcode_ , "BBDC3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFI" ) ; }
    if ( index ( $shortcode_ , "BBDC4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFI" ) ; }
    if ( index ( $shortcode_ , "BBAS3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFI" ) ; }
    if ( index ( $shortcode_ , "ITSA4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFI" ) ; }
    if ( index ( $shortcode_ , "ITUB4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFI" ) ; }
    if ( index ( $shortcode_ , "SANB11" ) >= 0 ) { push ( @portfolios_ , "BMFEQFI" ) ; }
    if ( index ( $shortcode_ , "BVMF3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFI" ) ; }
    if ( index ( $shortcode_ , "BBSE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFI" ) ; }
    if ( index ( $shortcode_ , "CTIP3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFI" ) ; }
    if ( index ( $shortcode_ , "CIEL3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFI" ) ; }
    if ( index ( $shortcode_ , "DTEX3" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "FIBR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "KLBN11" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "SUZB5" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "VALE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "VALE5" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "BRKM5" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "GGBR4" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "GOAU4" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "CSNA3" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "USIM5" ) >= 0 ) { push ( @portfolios_ , "BMFEQBM" ) ; }
    if ( index ( $shortcode_ , "PETR4" ) >= 0 ) { push ( @portfolios_ , "BMFEQOGB" ) ; }
    if ( index ( $shortcode_ , "PETR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQOGB" ) ; }
    if ( index ( $shortcode_ , "OIBR4" ) >= 0 ) { push ( @portfolios_ , "BMFEQTlc" ) ; }
    if ( index ( $shortcode_ , "VIVT4" ) >= 0 ) { push ( @portfolios_ , "BMFEQTlc" ) ; }
    if ( index ( $shortcode_ , "TIMP3" ) >= 0 ) { push ( @portfolios_ , "BMFEQTlc" ) ; }
    if ( index ( $shortcode_ , "SBSP3" ) >= 0 ) { push ( @portfolios_ , "BMFEQUt" ) ; }
    if ( index ( $shortcode_ , "CMIG4" ) >= 0 ) { push ( @portfolios_ , "BMFEQUt" ) ; }
    if ( index ( $shortcode_ , "CESP6" ) >= 0 ) { push ( @portfolios_ , "BMFEQUt" ) ; }
    if ( index ( $shortcode_ , "CPLE6" ) >= 0 ) { push ( @portfolios_ , "BMFEQUt" ) ; }
    if ( index ( $shortcode_ , "CPFE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQUt" ) ; }
    if ( index ( $shortcode_ , "ELET3" ) >= 0 ) { push ( @portfolios_ , "BMFEQUt" ) ; }
    if ( index ( $shortcode_ , "ELET6" ) >= 0 ) { push ( @portfolios_ , "BMFEQUt" ) ; }
    if ( index ( $shortcode_ , "ENBR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQUt" ) ; }
    if ( index ( $shortcode_ , "LIGT3" ) >= 0 ) { push ( @portfolios_ , "BMFEQUt" ) ; }
    if ( index ( $shortcode_ , "TBLE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQUt" ) ; }
    if ( index ( $shortcode_ , "NSE_HDFC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_HDFCBANK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_SBIN_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_ICICIBANK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_AXISBANK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_KOTAKBANK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_INDUSINDBK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_BANKBARODA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_YESBANK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_IBULHSGFIN_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_ULTRACEMCO_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSTRF" ) ; } 
    if ( index ( $shortcode_ , "NSE_AMBUJACEM_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSTRF" ) ; } 
    if ( index ( $shortcode_ , "NSE_ACC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSTRF" ) ; } 
    if ( index ( $shortcode_ , "NSE_HINDUNILVR_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSNDF" ) ; } 
    if ( index ( $shortcode_ , "NSE_DABUR_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSNDF" ) ; } 
    if ( index ( $shortcode_ , "NSE_GODREJIND_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSNDF" ) ; } 
    if ( index ( $shortcode_ , "NSE_LT_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEENGGF" ) ; } 
    if ( index ( $shortcode_ , "NSE_ADANIPORTS_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEENGGF" ) ; } 
    if ( index ( $shortcode_ , "NSE_BHEL_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEENGGF" ) ; } 
    if ( index ( $shortcode_ , "NSE_SIEMENS_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEENGGF" ) ; } 
    if ( index ( $shortcode_ , "NSE_TCS_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETECHF" ) ; } 
    if ( index ( $shortcode_ , "NSE_INFY_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETECHF" ) ; } 
    if ( index ( $shortcode_ , "NSE_WIPRO_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETECHF" ) ; } 
    if ( index ( $shortcode_ , "NSE_HCLTECH_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETECHF" ) ; } 
    if ( index ( $shortcode_ , "NSE_TECHM_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETECHF" ) ; } 
    if ( index ( $shortcode_ , "NSE_COALINDIA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEMETMINF" ) ; } 
    if ( index ( $shortcode_ , "NSE_HINDZINC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEMETMINF" ) ; } 
    if ( index ( $shortcode_ , "NSE_NMDC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEMETMINF" ) ; } 
    if ( index ( $shortcode_ , "NSE_RELIANCE_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEOILGASF" ) ; } 
    if ( index ( $shortcode_ , "NSE_ONGC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEOILGASF" ) ; } 
    if ( index ( $shortcode_ , "NSE_IOC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEOILGASF" ) ; } 
    if ( index ( $shortcode_ , "NSE_BPCL_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEOILGASF" ) ; } 
    if ( index ( $shortcode_ , "NSE_SUNPHARMA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEPHARMAF" ) ; } 
    if ( index ( $shortcode_ , "NSE_LUPIN_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEPHARMAF" ) ; } 
    if ( index ( $shortcode_ , "NSE_DRREDDY_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEPHARMAF" ) ; } 
    if ( index ( $shortcode_ , "NSE_CIPLA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEPHARMAF" ) ; } 
    if ( index ( $shortcode_ , "NSE_AUROPHARMA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEPHARMAF" ) ; } 
    if ( index ( $shortcode_ , "NSE_BHARTIARTL_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETELECOMF" ) ; } 
    if ( index ( $shortcode_ , "NSE_IDEA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETELECOMF" ) ; } 
    if ( index ( $shortcode_ , "NSE_RCOM_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETELECOMF" ) ; } 
    if ( index ( $shortcode_ , "NSE_NTPC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEUTILF" ) ; } 
    if ( index ( $shortcode_ , "NSE_POWERGRID_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEUTILF" ) ; } 
    elsif ( $#portfolios_ < 0  && index ( $shortcode_, "NSE_") >= 0 ) { push ( @portfolios_,"NSENIFTY75F") ; } 
  }
  else
  {
    if ( index ( $shortcode_ , "EMBR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCgTe" ) ; }
    if ( index ( $shortcode_ , "POMO4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCgTe" ) ; }
    if ( index ( $shortcode_ , "BRFS3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCncFp" ) ; }
    if ( index ( $shortcode_ , "CSAN3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCncFp" ) ; }
    if ( index ( $shortcode_ , "JBSS3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCncFp" ) ; }
    if ( index ( $shortcode_ , "MRFG3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCncFp" ) ; }
    if ( index ( $shortcode_ , "ABEV3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCnc" ) ; }
    if ( index ( $shortcode_ , "PCAR4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCnc" ) ; }
    if ( index ( $shortcode_ , "CRUZ3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCnc" ) ; }
    if ( index ( $shortcode_ , "NATU3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCnc" ) ; }
    if ( index ( $shortcode_ , "HYPE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrCnc" ) ; }
    if ( index ( $shortcode_ , "CYRE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrConsTr" ) ; }
    if ( index ( $shortcode_ , "EVEN3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrConsTr" ) ; }
    if ( index ( $shortcode_ , "GFSA3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrConsTr" ) ; }
    if ( index ( $shortcode_ , "MRVE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrConsTr" ) ; }
    if ( index ( $shortcode_ , "PDGR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrConsTr" ) ; }
    if ( index ( $shortcode_ , "ALLL3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrConsTr" ) ; }
    if ( index ( $shortcode_ , "CCRO3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrConsTr" ) ; }
    if ( index ( $shortcode_ , "ECOR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrConsTr" ) ; }
    if ( index ( $shortcode_ , "GOLL4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrConsTr" ) ; }
    if ( index ( $shortcode_ , "LAME4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrRet" ) ; }
    if ( index ( $shortcode_ , "LREN3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrRet" ) ; }
    if ( index ( $shortcode_ , "HGTX3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrRet" ) ; }
    if ( index ( $shortcode_ , "QUAL3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrRet" ) ; }
    if ( index ( $shortcode_ , "ESTC3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrDiv" ) ; }
    if ( index ( $shortcode_ , "KROT3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrDiv" ) ; }
    if ( index ( $shortcode_ , "RENT3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrDiv" ) ; }
    if ( index ( $shortcode_ , "BRML3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrRlst" ) ; }
    if ( index ( $shortcode_ , "BRPR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrRlst" ) ; }
    if ( index ( $shortcode_ , "MULT3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrRlst" ) ; }
    if ( index ( $shortcode_ , "BRAP4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrRlst" ) ; }
    if ( index ( $shortcode_ , "UGPA3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrRlst" ) ; }
    if ( index ( $shortcode_ , "BBDC3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrFI" ) ; }
    if ( index ( $shortcode_ , "BBDC4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrFI" ) ; }
    if ( index ( $shortcode_ , "BBAS3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrFI" ) ; }
    if ( index ( $shortcode_ , "ITSA4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrFI" ) ; }
    if ( index ( $shortcode_ , "ITUB4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrFI" ) ; }
    if ( index ( $shortcode_ , "SANB11" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrFI" ) ; }
    if ( index ( $shortcode_ , "BVMF3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrFI" ) ; }
    if ( index ( $shortcode_ , "BBSE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrFI" ) ; }
    if ( index ( $shortcode_ , "CTIP3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrFI" ) ; }
    if ( index ( $shortcode_ , "CIEL3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrFI" ) ; }
    if ( index ( $shortcode_ , "DTEX3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "FIBR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "KLBN11" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "SUZB5" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "VALE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "VALE5" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "BRKM5" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "GGBR4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "GOAU4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "CSNA3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "USIM5" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrBM" ) ; }
    if ( index ( $shortcode_ , "PETR4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrOGB" ) ; }
    if ( index ( $shortcode_ , "PETR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrOGB" ) ; }
    if ( index ( $shortcode_ , "OIBR4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrTlc" ) ; }
    if ( index ( $shortcode_ , "VIVT4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrTlc" ) ; }
    if ( index ( $shortcode_ , "TIMP3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrTlc" ) ; }
    if ( index ( $shortcode_ , "SBSP3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrUt" ) ; }
    if ( index ( $shortcode_ , "CMIG4" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrUt" ) ; }
    if ( index ( $shortcode_ , "CESP6" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrUt" ) ; }
    if ( index ( $shortcode_ , "CPLE6" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrUt" ) ; }
    if ( index ( $shortcode_ , "CPFE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrUt" ) ; }
    if ( index ( $shortcode_ , "ELET3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrUt" ) ; }
    if ( index ( $shortcode_ , "ELET6" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrUt" ) ; }
    if ( index ( $shortcode_ , "ENBR3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrUt" ) ; }
    if ( index ( $shortcode_ , "LIGT3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrUt" ) ; }
    if ( index ( $shortcode_ , "TBLE3" ) >= 0 ) { push ( @portfolios_ , "BMFEQFrUt" ) ; }
    if ( index ( $shortcode_ , "BEEF3" ) >= 0 ) { push ( @portfolios_, "BMFEQCnc") ; } 
    if ( index ( $shortcode_ , "VVAR11" ) >= 0 ) { push ( @portfolios_, "BMFEQCnc") ; }
    if ( index ( $shortcode_ , "IGTA3" ) >= 0 ) { push ( @portfolios_, "BMFEQRet") ; } 
    if ( index ( $shortcode_ , "RADL3" ) >= 0 ) { push ( @portfolios_, "BMFEQRet") ; } 
    if ( index ( $shortcode_ , "MPLU3" ) >= 0 ) { push ( @portfolios_, "BMFEQDiv" ) ; } 
    if ( index ( $shortcode_ , "VLID3" ) >= 0 ) { push ( @portfolios_, "BMFEQDiv") ; } 
    if ( index ( $shortcode_ , "TOTS3" ) >= 0 ) { push ( @portfolios_, "BMFEQDiv") ; } 
    if ( index ( $shortcode_ , "MDIA3" ) >= 0 ) { push ( @portfolios_, "BMFEQDiv") ; } 
    if ( index ( $shortcode_ , "BRSR6" ) >= 0 ) { push ( @portfolios_, "BMFEQFI"); } 
    if ( index ( $shortcode_ , "SULA11" ) >= 0 ) { push ( @portfolios_, "BMFEQFI") ; } 
    if ( index ( $shortcode_ , "TAEE11" ) >= 0 ) { push ( @portfolios_, "BMFEQUt" ) ; }   
    if ( index ( $shortcode_ , "GETI4" ) >= 0 ) { push ( @portfolios_, "BMFEQUt" ) ; }   
    if ( index ( $shortcode_ , "EQTL3" ) >= 0 ) { push ( @portfolios_, "BMFEQUt" ) ; }   
    if ( index ( $shortcode_ , "WEGE3" ) >= 0 ) { push ( @portfolios_, "BMFEQUt" ) ; }   
    if ( index ( $shortcode_ , "MYPK3" ) >= 0 ) { push ( @portfolios_, "BMFEQFrConsTr" ) ; }   
    if ( index ( $shortcode_ , "ODPV3" ) >= 0 ) { push ( @portfolios_, "BMFEQFrRet" ) ; }   
    if ( index ( $shortcode_ , "SEER3" ) >= 0 ) { push ( @portfolios_, "BMFEQFrDiv" ) ; }   
    if ( index ( $shortcode_ , "ANIM3" ) >= 0 ) { push ( @portfolios_, "BMFEQFrDiv" ) ; }   
    if ( index ( $shortcode_ , "BBTG11" ) >= 0 ) { push ( @portfolios_, "BMFEQFrFI" ) ; }   
    if ( index ( $shortcode_ , "PSSA3" ) >= 0 ) { push ( @portfolios_, "BMFEQFrFI" ) ; }   
    if ( index ( $shortcode_ , "BBSE3" ) >= 0 ) { push ( @portfolios_, "BMFEQFrFI" ) ; }   
    if ( index ( $shortcode_ , "QGEP3" ) >= 0 ) { push ( @portfolios_, "BMFEQFrBM" ) ; }  
    if ( index ( $shortcode_ , "NSE_HDFC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_HDFCBANK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_SBIN_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_ICICIBANK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_AXISBANK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_KOTAKBANK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_INDUSINDBK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_BANKBARODA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_YESBANK_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_IBULHSGFIN_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEFINANCEF" ) ; } 
    if ( index ( $shortcode_ , "NSE_ULTRACEMCO_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSTRF" ) ; } 
    if ( index ( $shortcode_ , "NSE_AMBUJACEM_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSTRF" ) ; } 
    if ( index ( $shortcode_ , "NSE_ACC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSTRF" ) ; } 
    if ( index ( $shortcode_ , "NSE_HINDUNILVR_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSNDF" ) ; } 
    if ( index ( $shortcode_ , "NSE_DABUR_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSNDF" ) ; } 
    if ( index ( $shortcode_ , "NSE_GODREJIND_FUT0" ) >= 0 ) { push ( @portfolios_, "NSECONSNDF" ) ; } 
    if ( index ( $shortcode_ , "NSE_LT_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEENGGF" ) ; } 
    if ( index ( $shortcode_ , "NSE_ADANIPORTS_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEENGGF" ) ; } 
    if ( index ( $shortcode_ , "NSE_BHEL_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEENGGF" ) ; } 
    if ( index ( $shortcode_ , "NSE_SIEMENS_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEENGGF" ) ; } 
    if ( index ( $shortcode_ , "NSE_TCS_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETECHF" ) ; } 
    if ( index ( $shortcode_ , "NSE_INFY_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETECHF" ) ; } 
    if ( index ( $shortcode_ , "NSE_WIPRO_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETECHF" ) ; } 
    if ( index ( $shortcode_ , "NSE_HCLTECH_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETECHF" ) ; } 
    if ( index ( $shortcode_ , "NSE_TECHM_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETECHF" ) ; } 
    if ( index ( $shortcode_ , "NSE_COALINDIA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEMETMINF" ) ; } 
    if ( index ( $shortcode_ , "NSE_HINDZINC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEMETMINF" ) ; } 
    if ( index ( $shortcode_ , "NSE_NMDC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEMETMINF" ) ; } 
    if ( index ( $shortcode_ , "NSE_RELIANCE_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEOILGASF" ) ; } 
    if ( index ( $shortcode_ , "NSE_ONGC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEOILGASF" ) ; } 
    if ( index ( $shortcode_ , "NSE_IOC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEOILGASF" ) ; } 
    if ( index ( $shortcode_ , "NSE_BPCL_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEOILGASF" ) ; } 
    if ( index ( $shortcode_ , "NSE_SUNPHARMA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEPHARMAF" ) ; } 
    if ( index ( $shortcode_ , "NSE_LUPIN_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEPHARMAF" ) ; } 
    if ( index ( $shortcode_ , "NSE_DRREDDY_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEPHARMAF" ) ; } 
    if ( index ( $shortcode_ , "NSE_CIPLA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEPHARMAF" ) ; } 
    if ( index ( $shortcode_ , "NSE_AUROPHARMA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEPHARMAF" ) ; } 
    if ( index ( $shortcode_ , "NSE_BHARTIARTL_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETELECOMF" ) ; } 
    if ( index ( $shortcode_ , "NSE_IDEA_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETELECOMF" ) ; } 
    if ( index ( $shortcode_ , "NSE_RCOM_FUT0" ) >= 0 ) { push ( @portfolios_, "NSETELECOMF" ) ; } 
    if ( index ( $shortcode_ , "NSE_NTPC_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEUTILF" ) ; } 
    if ( index ( $shortcode_ , "NSE_POWERGRID_FUT0" ) >= 0 ) { push ( @portfolios_, "NSEUTILF" ) ; } 
    elsif ( $#portfolios_ < 0  && index ( $shortcode_, "NSE_") >= 0 ) { push ( @portfolios_,"NSENIFTY75F") ; }    
  }
    @portfolios_ ;
}

sub GetNextMajorStockInSector 
{
    my $shortcode_ = shift; 
    my @next_major_stock_ = () ;
    
    if ( index ( $shortcode_ , "EMBR3" ) >= 0 ) { push ( @next_major_stock_ , "POMO4" ) ; }
    if ( index ( $shortcode_ , "POMO4" ) >= 0 ) { push ( @next_major_stock_ , "EMBR3" ) ; }
    if ( index ( $shortcode_ , "BRFS3" ) >= 0 ) { push ( @next_major_stock_ , "JBSS3" ) ; }
    if ( index ( $shortcode_ , "CSAN3" ) >= 0 ) { push ( @next_major_stock_ , "BRFS3" ) ; }
    if ( index ( $shortcode_ , "JBSS3" ) >= 0 ) { push ( @next_major_stock_ , "BRFS3" ) ; }
    if ( index ( $shortcode_ , "MRFG3" ) >= 0 ) { push ( @next_major_stock_ , "BRFS3" ) ; }
    if ( index ( $shortcode_ , "ABEV3" ) >= 0 ) { push ( @next_major_stock_ , "PCAR4" ) ; }
    if ( index ( $shortcode_ , "PCAR4" ) >= 0 ) { push ( @next_major_stock_ , "ABEV3" ) ; }
    if ( index ( $shortcode_ , "CRUZ3" ) >= 0 ) { push ( @next_major_stock_ , "ABEV3" ) ; }
    if ( index ( $shortcode_ , "NATU3" ) >= 0 ) { push ( @next_major_stock_ , "ABEV3" ) ; }
    if ( index ( $shortcode_ , "HYPE3" ) >= 0 ) { push ( @next_major_stock_ , "ABEV3" ) ; }
    if ( index ( $shortcode_ , "CYRE3" ) >= 0 ) { push ( @next_major_stock_ , "MRVE3" ) ; }
    if ( index ( $shortcode_ , "EVEN3" ) >= 0 ) { push ( @next_major_stock_ , "MRVE3" ) ; }
    if ( index ( $shortcode_ , "GFSA3" ) >= 0 ) { push ( @next_major_stock_ , "MRVE3" ) ; }
    if ( index ( $shortcode_ , "MRVE3" ) >= 0 ) { push ( @next_major_stock_ , "CYRE3" ) ; }
    if ( index ( $shortcode_ , "PDGR3" ) >= 0 ) { push ( @next_major_stock_ , "CCRO3" ) ; }
    if ( index ( $shortcode_ , "ALLL3" ) >= 0 ) { push ( @next_major_stock_ , "CCRO3" ) ; }
    if ( index ( $shortcode_ , "RUMO3" ) >= 0 ) { push ( @next_major_stock_ , "CCRO3" ) ; }
    if ( index ( $shortcode_ , "CCRO3" ) >= 0 ) { push ( @next_major_stock_ , "MRVE3" ) ; }
    if ( index ( $shortcode_ , "ECOR3" ) >= 0 ) { push ( @next_major_stock_ , "CCRO3" ) ; }
    if ( index ( $shortcode_ , "GOLL4" ) >= 0 ) { push ( @next_major_stock_ , "CCRO3" ) ; }
    if ( index ( $shortcode_ , "LAME4" ) >= 0 ) { push ( @next_major_stock_ , "LREN3" ) ; }
    if ( index ( $shortcode_ , "LREN3" ) >= 0 ) { push ( @next_major_stock_ , "LAME4" ) ; }
    if ( index ( $shortcode_ , "HGTX3" ) >= 0 ) { push ( @next_major_stock_ , "QUAL3" ) ; }
    if ( index ( $shortcode_ , "QUAL3" ) >= 0 ) { push ( @next_major_stock_ , "HGTX3" ) ; }
    if ( index ( $shortcode_ , "ESTC3" ) >= 0 ) { push ( @next_major_stock_ , "KROT3" ) ; }
    if ( index ( $shortcode_ , "KROT3" ) >= 0 ) { push ( @next_major_stock_ , "ESTC3" ) ; }
    if ( index ( $shortcode_ , "RENT3" ) >= 0 ) { push ( @next_major_stock_ , "KROT3" ) ; }
    if ( index ( $shortcode_ , "BRML3" ) >= 0 ) { push ( @next_major_stock_ , "MULT3" ) ; }
    if ( index ( $shortcode_ , "BRPR3" ) >= 0 ) { push ( @next_major_stock_ , "BRML3" ) ; }
    if ( index ( $shortcode_ , "MULT3" ) >= 0 ) { push ( @next_major_stock_ , "BRML3" ) ; }
    if ( index ( $shortcode_ , "BRAP4" ) >= 0 ) { push ( @next_major_stock_ , "UGPA3" ) ; }
    if ( index ( $shortcode_ , "UGPA3" ) >= 0 ) { push ( @next_major_stock_ , "BRAP4" ) ; }
    if ( index ( $shortcode_ , "BBDC3" ) >= 0 ) { push ( @next_major_stock_ , "BBDC4" ) ; }
    if ( index ( $shortcode_ , "BBDC4" ) >= 0 ) { push ( @next_major_stock_ , "BBDC3" ) ; }
    if ( index ( $shortcode_ , "BBAS3" ) >= 0 ) { push ( @next_major_stock_ , "ITUB4" ) ; }
    if ( index ( $shortcode_ , "ITSA4" ) >= 0 ) { push ( @next_major_stock_ , "ITUB4" ) ; }
    if ( index ( $shortcode_ , "ITUB4" ) >= 0 ) { push ( @next_major_stock_ , "BBDC4" ) ; }
    if ( index ( $shortcode_ , "SANB11" ) >= 0 ) { push ( @next_major_stock_ , "ITUB4" ) ; }
    if ( index ( $shortcode_ , "BVMF3" ) >= 0 ) { push ( @next_major_stock_ , "CIEL3" ) ; }
    if ( index ( $shortcode_ , "BBSE3" ) >= 0 ) { push ( @next_major_stock_ , "BVMF3" ) ; }
    if ( index ( $shortcode_ , "CTIP3" ) >= 0 ) { push ( @next_major_stock_ , "BVMF3" ) ; }
    if ( index ( $shortcode_ , "CIEL3" ) >= 0 ) { push ( @next_major_stock_ , "BVMF3" ) ; }
    if ( index ( $shortcode_ , "DTEX3" ) >= 0 ) { push ( @next_major_stock_ , "FIBR3" ) ; }
    if ( index ( $shortcode_ , "FIBR3" ) >= 0 ) { push ( @next_major_stock_ , "KLBN11" ) ; }
    if ( index ( $shortcode_ , "KLBN11" ) >= 0 ) { push ( @next_major_stock_ , "FIBR3" ) ; }
    if ( index ( $shortcode_ , "SUZB5" ) >= 0 ) { push ( @next_major_stock_ , "FIBR3" ) ; }
    if ( index ( $shortcode_ , "VALE3" ) >= 0 ) { push ( @next_major_stock_ , "VALE5" ) ; }
    if ( index ( $shortcode_ , "VALE5" ) >= 0 ) { push ( @next_major_stock_ , "VALE3" ) ; }
    if ( index ( $shortcode_ , "BRKM5" ) >= 0 ) { push ( @next_major_stock_ , "GGBR4" ) ; }
    if ( index ( $shortcode_ , "GGBR4" ) >= 0 ) { push ( @next_major_stock_ , "CSNA3" ) ; }
    if ( index ( $shortcode_ , "GOAU4" ) >= 0 ) { push ( @next_major_stock_ , "GGBR4" ) ; }
    if ( index ( $shortcode_ , "CSNA3" ) >= 0 ) { push ( @next_major_stock_ , "GGBR4" ) ; }
    if ( index ( $shortcode_ , "USIM5" ) >= 0 ) { push ( @next_major_stock_ , "GGBR4" ) ; }
    if ( index ( $shortcode_ , "PETR4" ) >= 0 ) { push ( @next_major_stock_ , "PETR3" ) ; }
    if ( index ( $shortcode_ , "PETR3" ) >= 0 ) { push ( @next_major_stock_ , "PETR4" ) ; }
    if ( index ( $shortcode_ , "OIBR4" ) >= 0 ) { push ( @next_major_stock_ , "VIVT4" ) ; }
    if ( index ( $shortcode_ , "VIVT4" ) >= 0 ) { push ( @next_major_stock_ , "OIBR4" ) ; }
    if ( index ( $shortcode_ , "TIMP3" ) >= 0 ) { push ( @next_major_stock_ , "VIVT4" ) ; }
    if ( index ( $shortcode_ , "SBSP3" ) >= 0 ) { push ( @next_major_stock_ , "CMIG4" ) ; }
    if ( index ( $shortcode_ , "CMIG4" ) >= 0 ) { push ( @next_major_stock_ , "TBLE3" ) ; }
    if ( index ( $shortcode_ , "CESP6" ) >= 0 ) { push ( @next_major_stock_ , "CMIG4" ) ; }
    if ( index ( $shortcode_ , "CPLE6" ) >= 0 ) { push ( @next_major_stock_ , "CMIG4" ) ; }
    if ( index ( $shortcode_ , "CPFE3" ) >= 0 ) { push ( @next_major_stock_ , "CMIG4" ) ; }
    if ( index ( $shortcode_ , "ELET3" ) >= 0 ) { push ( @next_major_stock_ , "CMIG4" ) ; }
    if ( index ( $shortcode_ , "ELET6" ) >= 0 ) { push ( @next_major_stock_ , "CMIG4" ) ; }
    if ( index ( $shortcode_ , "ENBR3" ) >= 0 ) { push ( @next_major_stock_ , "CMIG4" ) ; }
    if ( index ( $shortcode_ , "LIGT3" ) >= 0 ) { push ( @next_major_stock_ , "CMIG4" ) ; }
    if ( index ( $shortcode_ , "TBLE3" ) >= 0 ) { push ( @next_major_stock_ , "CMIG4" ) ; }
    
    if ( index ( $shortcode_ , "BEEF3" ) >= 0 ) { push ( @next_major_stock_, "BRFS3") ; } 
    if ( index ( $shortcode_ , "VVAR11" ) >= 0 ) { push ( @next_major_stock_, "BRFS3") ; }
    if ( index ( $shortcode_ , "IGTA3" ) >= 0 ) { push ( @next_major_stock_, "LAME4") ; } 
    if ( index ( $shortcode_ , "RADL3" ) >= 0 ) { push ( @next_major_stock_, "LAME4") ; } 
    if ( index ( $shortcode_ , "MPLU3" ) >= 0 ) { push ( @next_major_stock_, "KROT3" ) ; } 
    if ( index ( $shortcode_ , "VLID3" ) >= 0 ) { push ( @next_major_stock_, "KROT3") ; } 
    if ( index ( $shortcode_ , "TOTS3" ) >= 0 ) { push ( @next_major_stock_, "KROT3") ; } 
    if ( index ( $shortcode_ , "MDIA3" ) >= 0 ) { push ( @next_major_stock_, "KROT3") ; } 
    if ( index ( $shortcode_ , "BRSR6" ) >= 0 ) { push ( @next_major_stock_, "BBDC4"); } 
    if ( index ( $shortcode_ , "SULA11" ) >= 0 ) { push ( @next_major_stock_, "BBDC4") ; } 
    if ( index ( $shortcode_ , "TAEE11" ) >= 0 ) { push ( @next_major_stock_, "ELET3" ) ; }   
    if ( index ( $shortcode_ , "GETI4" ) >= 0 ) { push ( @next_major_stock_, "ELET3" ) ; }   
    if ( index ( $shortcode_ , "EQTL3" ) >= 0 ) { push ( @next_major_stock_, "ELET3" ) ; }   
    if ( index ( $shortcode_ , "WEGE3" ) >= 0 ) { push ( @next_major_stock_, "ELET3" ) ; }   
    if ( index ( $shortcode_ , "MYPK3" ) >= 0 ) { push ( @next_major_stock_, "CCRO3" ) ; }   
    if ( index ( $shortcode_ , "ODPV3" ) >= 0 ) { push ( @next_major_stock_, "LREN3" ) ; }   
    if ( index ( $shortcode_ , "SEER3" ) >= 0 ) { push ( @next_major_stock_, "ESTC3" ) ; }   
    if ( index ( $shortcode_ , "ANIM3" ) >= 0 ) { push ( @next_major_stock_, "ESTC3" ) ; }   
    if ( index ( $shortcode_ , "BBTG11" ) >= 0 ) { push ( @next_major_stock_, "BBDC4" ) ; }   
    if ( index ( $shortcode_ , "PSSA3" ) >= 0 ) { push ( @next_major_stock_, "BBDC4" ) ; }   
    if ( index ( $shortcode_ , "BBSE3" ) >= 0 ) { push ( @next_major_stock_, "BBDC4" ) ; }   
    if ( index ( $shortcode_ , "QGEP3" ) >= 0 ) { push ( @next_major_stock_, "VALE5" ) ; }   

    return @next_major_stock_ ;

}
#if ($#ARGV <  1 )
#{
#  print "Usage: $0  SHORTCODE.\n";
#}
#else
#{
#  my @core_shc_ = GetSectorPortForStocks ( $ARGV[0] );
#  foreach my $sh_ ( @core_shc_ )
#  {
#    print $sh_." ";
#  }
#  print "\n";
#}
