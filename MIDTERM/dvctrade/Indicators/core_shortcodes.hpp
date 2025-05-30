/**
   \file Indicators/core_shortcodes.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_CORE_SHORTCODES_HPP
#define BASE_INDICATORS_CORE_SHORTCODES_HPP

#include <iostream>
#include <string>

#include "dvccode/CommonDataStructures/vector_utils.hpp"

namespace HFSAT {

inline void GetCoreShortcodes(const std::string& r_dep_shortcode_, std::vector<std::string>& core_shortcode_vec_) {
  // cfe
  if (!r_dep_shortcode_.compare("VX_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
  }
  if (!r_dep_shortcode_.compare("VX_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
  }
  if (!r_dep_shortcode_.compare("VX_2")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
  }
  if (!r_dep_shortcode_.compare("VX_3")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
  }
  if (!r_dep_shortcode_.compare("VX_4")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
  }
  if (!r_dep_shortcode_.compare("SP_VX0_VX1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
  }
  if (!r_dep_shortcode_.compare("SP_VX1_VX2")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("SP_VX0_VX1"));
  }
  if (!r_dep_shortcode_.compare("SP_VX2_VX3")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("VX_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("SP_VX0_VX1"));
  }

  // eurex
  if (!r_dep_shortcode_.compare("FESX_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FDAX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }

  if (!r_dep_shortcode_.compare("FDAX_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }

  if (!r_dep_shortcode_.compare("FDXM_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FDAX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }

  if (!r_dep_shortcode_.compare("FESQ_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FDAX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }

  if (!r_dep_shortcode_.compare("FXXP_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FDAX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }

  if (!r_dep_shortcode_.compare("FGBL_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBS_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFR_0"));
  }

  if (!r_dep_shortcode_.compare("FGBX_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBM_0"));
  }

  if (!r_dep_shortcode_.compare("FGBM_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBS_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
  }

  if (!r_dep_shortcode_.compare("FGBS_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
  }

  if (!r_dep_shortcode_.compare("FOAT_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFR_0"));
  }

  if (!r_dep_shortcode_.compare("FOAM_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FOAT_0"));
  }

  if (!r_dep_shortcode_.compare("FBTP_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FOAT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFR_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
  }

  if (!r_dep_shortcode_.compare("FBTS_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FBTP_0"));
  }

  // hk
  if (!r_dep_shortcode_.compare("HHI_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HSI_0"));
  }
  if (!r_dep_shortcode_.compare("HSI_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HHI_0"));
  }
  if (!r_dep_shortcode_.compare("MHI_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HHI_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HSI_0"));
  }
  if (!r_dep_shortcode_.compare("MCH_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HHI_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HSI_0"));
  }

  // sgx
  if (!r_dep_shortcode_.compare("SGX_NK_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
  }
  if (!r_dep_shortcode_.compare("SGX_NK_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_1"));
  }

  // ose
  if (!r_dep_shortcode_.compare("NK_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
  }
  if (!r_dep_shortcode_.compare("NKM_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
  }
  if (!r_dep_shortcode_.compare("NKM_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }
  if (!r_dep_shortcode_.compare("TOPIX_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
  }
  if (!r_dep_shortcode_.compare("JP400_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("TOPIX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
  }
  if (!r_dep_shortcode_.compare("JGBL_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("TOPIX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
  }

  // cme
  if (!r_dep_shortcode_.compare("ZF_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("GE_4"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("GE_5"));
  }
  if (!r_dep_shortcode_.compare("ZN_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZF_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }
  if (!r_dep_shortcode_.compare("TN_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
  }
  if (!r_dep_shortcode_.compare("ZB_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZF_0"));
  }
  if (!r_dep_shortcode_.compare("UB_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
  }

  // cme commodities
  if (!r_dep_shortcode_.compare("RB_0") || !r_dep_shortcode_.compare("RB_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BZ_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("RB_0"));
  }
  if (!r_dep_shortcode_.compare("BZ_0") || !r_dep_shortcode_.compare("BZ_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BZ_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("RB_0"));
  }
  if (!r_dep_shortcode_.compare("NG_0") || !r_dep_shortcode_.compare("NG_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NG_0"));
  }

  if (!r_dep_shortcode_.compare("ZW_0") || !r_dep_shortcode_.compare("ZW_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZW_0"));
  }
  if (!r_dep_shortcode_.compare("KE_0") || !r_dep_shortcode_.compare("KE_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("KE_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZW_0"));
  }

  // liffe
  if (!r_dep_shortcode_.compare("LFR_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
  }
  if ((!r_dep_shortcode_.compare("YFEBM_0")) || (!r_dep_shortcode_.compare("YFEBM_1")) ||
      (!r_dep_shortcode_.compare("YFEBM_2"))) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZW_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_0"));
    // Assuming liquidity beyond _1 is minimal/none.
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_1"));
  }
  if ((!r_dep_shortcode_.compare("XFC_0")) || (!r_dep_shortcode_.compare("XFC_1"))) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFC_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFC_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFRC_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFRC_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_1"));
    // Assuming liquidity beyond _2 is minimal/none.
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_2"));
  }
  if ((!r_dep_shortcode_.compare("XFRC_0")) || (!r_dep_shortcode_.compare("XFRC_1"))) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFC_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFC_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFRC_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFRC_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_1"));
    // Assuming liquidity beyond _2 is minimal/none.
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_2"));
  }
  if (!r_dep_shortcode_.compare("LFZ_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NQ_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FDAX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("JFFCE_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("KFFTI_0"));
  }
  if (!r_dep_shortcode_.compare("JFFCE_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NQ_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FDAX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFZ_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("KFFTI_0"));
  }
  if (!r_dep_shortcode_.compare("KFFTI_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NQ_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FDAX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFZ_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("JFFCE_0"));
  }

  // tmx
  if (!r_dep_shortcode_.compare("CGB_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
  }
  if (!r_dep_shortcode_.compare("CGF_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CGB_0"));
  }
  if (!r_dep_shortcode_.compare("CGZ_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CGB_0"));
  }

  // bmf
  if (!r_dep_shortcode_.compare("BR_IND_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_WIN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }
  if (!r_dep_shortcode_.compare("BR_WIN_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_IND_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }
  if (!r_dep_shortcode_.compare("BR_DOL_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6A_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6B_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6E_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6C_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6M_0"));
  }
  if (!r_dep_shortcode_.compare("DI1F16")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F17"));
  }
  if (!r_dep_shortcode_.compare("DI1F15")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F17"));
  }
  if (!r_dep_shortcode_.compare("DI1F17")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_DOL_0"));
  }
  if (!r_dep_shortcode_.compare("DI1F18")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_DOL_0"));
  }
  if (!r_dep_shortcode_.compare("DI1F19")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F18"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_DOL_0"));
  }
  if (!r_dep_shortcode_.compare("DI1F20")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F18"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F19"));
  }
  if (!r_dep_shortcode_.compare("DI1F21")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F18"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F19"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_DOL_0"));
  }
  if (!r_dep_shortcode_.compare("DI1F22")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F23"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F25"));
  }
  if (!r_dep_shortcode_.compare("DI1F23")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F25"));
  }
  if (!r_dep_shortcode_.compare("DI1F24")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F23"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F25"));
  }
  if (!r_dep_shortcode_.compare("DI1F25")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F19"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F23"));
  }
  if (!r_dep_shortcode_.compare("DI1F26")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F23"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F25"));
  }
  if (!r_dep_shortcode_.compare("DI1F27")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_DOL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F23"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F25"));
  }

  if (!r_dep_shortcode_.compare("RI_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NQ_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FDAX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("Si_0"));
  }
  if (!r_dep_shortcode_.compare("GD_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("GC_0"));
  }
  if (!r_dep_shortcode_.compare("ED_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6E_0"));
  }
  if (!r_dep_shortcode_.compare("BR_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_0"));
  }
  if (!r_dep_shortcode_.compare("Si_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("USD000UTSTOM"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("USD000000TOD"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
  }
  if (!r_dep_shortcode_.compare("USD000UTSTOM")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("Si_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("USD000000TOD"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
  }
  if (!r_dep_shortcode_.compare("USD000000TOD")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("Si_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("USD000UTSTOM"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("CL_0"));
  }

  if (!r_dep_shortcode_.compare("YT_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
  }
  if (!r_dep_shortcode_.compare("XT_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
  }
  if (!r_dep_shortcode_.compare("IR_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
  }
  if (!r_dep_shortcode_.compare("IR_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_0"));
  }
  if (!r_dep_shortcode_.compare("IR_2")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_3"));
  }
  if (!r_dep_shortcode_.compare("IR_3")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_2"));
  }
  if (!r_dep_shortcode_.compare("IR_4")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_3"));
  }
  if (!r_dep_shortcode_.compare("SGX_CN_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HHI_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HSI_0"));
  }

  VectorUtils::UniqueVectorAdd(core_shortcode_vec_, r_dep_shortcode_);
}

inline void GetSessionCoreShortcodes(const std::string& r_dep_shortcode_, std::vector<std::string>& core_shortcode_vec_,
                                     const int& r_mfm_) {
  int mfm_as_end_hours = 6 * 60 * 60 * 1000;
  int mfm_as_start_hours = 20 * 60 * 60 * 1000;  // Query starting after this time would be AS hour Query
  if (!r_dep_shortcode_.compare("XT_0") || !r_dep_shortcode_.compare("XTE_0")) {
    if (r_mfm_ > mfm_as_end_hours && r_mfm_ < mfm_as_start_hours) {
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZNY_0"));
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZF_0"));
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZFY_0"));
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZBY_0"));
    }
  }

  if (!r_dep_shortcode_.compare("YT_0") || !r_dep_shortcode_.compare("YTE_0")) {
    if (r_mfm_ > mfm_as_end_hours && r_mfm_ < mfm_as_start_hours) {
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZNY_0"));
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZF_0"));
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZFY_0"));
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZBY_0"));
    }
  }

  if (!r_dep_shortcode_.compare("AP_0")) {
    if (r_mfm_ > mfm_as_end_hours && r_mfm_ < mfm_as_start_hours) {
      VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    }
  }
  VectorUtils::UniqueVectorAdd(core_shortcode_vec_, r_dep_shortcode_);
}
}

#endif  // BASE_INDICATORS_CORE_SHORTCODES_HPP
