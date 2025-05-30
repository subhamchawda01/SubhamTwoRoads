/**
   \file Indicators/sampling_shortcodes.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/
#pragma once

#include <iostream>
#include <string>

#include "dvccode/CommonDataStructures/vector_utils.hpp"

namespace HFSAT {

inline bool GetSamplingShortcodesAS(const std::string& r_dep_shortcode_,
                                    std::vector<std::string>& core_shortcode_vec_) {
  // ose
  if (!r_dep_shortcode_.compare("NK_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("NKM_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("NKM_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("TOPIX_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("JP400_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("TOPIX_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("JGBL_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("TOPIX_0"));
    return true;
  }

  if (!r_dep_shortcode_.compare("NKD_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("NIY_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    return true;
  }

  // cme
  if (!r_dep_shortcode_.compare("ZF_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("ZN_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZF_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("ZB_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZF_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("UB_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("YT_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("XT_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("IR_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    return true;
  }
  if (!r_dep_shortcode_.compare("IR_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("IR_2")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_3"));
    return true;
  }
  if (!r_dep_shortcode_.compare("IR_3")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_2"));
    return true;
  }
  if (!r_dep_shortcode_.compare("IR_4")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_3"));
    return true;
  }

  // sgx
  if (!r_dep_shortcode_.compare("SGX_NK_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    return true;
  }
  return false;
}

inline bool GetSamplingShortcodesEU(const std::string& r_dep_shortcode_,
                                    std::vector<std::string>& core_shortcode_vec_) {
  // ose
  if (!r_dep_shortcode_.compare("NK_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("NKM_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("NKM_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("TOPIX_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("JP400_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("TOPIX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("JGBL_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("TOPIX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    return true;
  }

  if (!r_dep_shortcode_.compare("NKD_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("NIY_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6J_0"));
    return true;
  }

  // cme
  if (!r_dep_shortcode_.compare("ZF_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBL_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("ZN_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZF_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBL_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("ZB_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZF_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("UB_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("YT_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("XT_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("IR_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    return true;
  }
  if (!r_dep_shortcode_.compare("IR_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("IR_2")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_3"));
    return true;
  }
  if (!r_dep_shortcode_.compare("IR_3")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_2"));
    return true;
  }
  if (!r_dep_shortcode_.compare("IR_4")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("IR_3"));
    return true;
  }
  // hk
  if (!r_dep_shortcode_.compare("HHI_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HSI_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("HSI_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HHI_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("MHI_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HHI_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HSI_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    return true;
  }
  if (!r_dep_shortcode_.compare("MCH_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HHI_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("HSI_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    return true;
  }

  // sgx
  if (!r_dep_shortcode_.compare("SGX_NK_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    return true;
  }
  return false;
}

inline bool GetSamplingShortcodesUS(const std::string& r_dep_shortcode_,
                                    std::vector<std::string>& core_shortcode_vec_) {
  // sgx
  if (!r_dep_shortcode_.compare("SGX_NK_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    return true;
  }
  return false;
}

inline void GetSamplingShortcodesDefault(const std::string& r_dep_shortcode_,
                                         std::vector<std::string>& core_shortcode_vec_) {
  // CFE
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
  if (!r_dep_shortcode_.compare("FVS_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
  }
  if (!r_dep_shortcode_.compare("FVS_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FVS_0"));
  }
  if (!r_dep_shortcode_.compare("FVS_2")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FESX_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FVS_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FVS_1"));
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
  if (!r_dep_shortcode_.compare("NKMF_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
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

  if (!r_dep_shortcode_.compare("NKD_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
  }
  if (!r_dep_shortcode_.compare("NIY_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NKM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("NK_0"));
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
  if (!r_dep_shortcode_.compare("ZB_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZF_0"));
  }
  if (!r_dep_shortcode_.compare("UB_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZB_0"));
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
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_,
                                 std::string("YFEBM_1"));  // Assuming liquidity beyond _1 is minimal/none.
  }
  if ((!r_dep_shortcode_.compare("XFC_0")) || (!r_dep_shortcode_.compare("XFC_1"))) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFC_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFC_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFRC_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFRC_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_,
                                 std::string("YFEBM_2"));  // Assuming liquidity beyond _2 is minimal/none.
  }
  if ((!r_dep_shortcode_.compare("XFRC_0")) || (!r_dep_shortcode_.compare("XFRC_1"))) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFC_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFC_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFRC_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XFRC_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YFEBM_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_,
                                 std::string("YFEBM_2"));  // Assuming liquidity beyond _2 is minimal/none.
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
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6M_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }
  if (!r_dep_shortcode_.compare("DI1F19")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F18"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_DOL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6M_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }
  if (!r_dep_shortcode_.compare("DI1F20")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F18"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F19"));
  }
  if (!r_dep_shortcode_.compare("DI1F21")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F18"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_DOL_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6M_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
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
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F18"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F21"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("6M_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ES_0"));
  }

  if (!r_dep_shortcode_.compare("LFI_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_2"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBS_0"));
  }

  if (!r_dep_shortcode_.compare("LFI_1")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_2"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBS_0"));
  }

  if (!r_dep_shortcode_.compare("LFI_2")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_1"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_3"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBS_0"));
  }

  if (!r_dep_shortcode_.compare("LFI_3")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_2"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_4"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBS_0"));
  }

  if (!r_dep_shortcode_.compare("LFI_4")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_3"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_5"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBS_0"));
  }

  if (!r_dep_shortcode_.compare("LFI_5")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_4"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_6"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBS_0"));
  }

  if (!r_dep_shortcode_.compare("LFI_6")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_4"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("LFI_5"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("FGBS_0"));
  }

  if (!r_dep_shortcode_.compare("DI1F17")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F16"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F18"));
  }
  if (!r_dep_shortcode_.compare("DI1F16")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F15"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F17"));
  }
  if (!r_dep_shortcode_.compare("DI1F18")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F16"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("BR_DOL_0"));
  }
  if (!r_dep_shortcode_.compare("DI1F17")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F16"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("DI1F18"));
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
  if (!r_dep_shortcode_.compare("Si_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("USD000UTSTOM"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("USD000000TOD"));
  }
  if (!r_dep_shortcode_.compare("USD000UTSTOM")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("Si_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("USD000000TOD"));
  }
  if (!r_dep_shortcode_.compare("USD000000TOD")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("Si_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("USD000UTSTOM"));
  }

  if (!r_dep_shortcode_.compare("XT_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
  }

  if (!r_dep_shortcode_.compare("YT_0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("ZN_0"));
  }

  if (!r_dep_shortcode_.compare("SP_XT0_YT0")) {
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("XT_0"));
    VectorUtils::UniqueVectorAdd(core_shortcode_vec_, std::string("YT_0"));
  }
}

inline void GetSamplingShortcodes(const std::string& r_dep_shortcode_, std::vector<std::string>& core_shortcode_vec_,
                                  const unsigned int start_msecs_utc_ = 12 * 60 * 60 * 1000) {
  unsigned int pre_as_msecs_ = (21 * 60 + 30) * 60 * 1000;   // UTC_2130
  unsigned int eu_start_msecs_ = (5 * 60 + 30) * 60 * 1000;  // UTC_530
  unsigned int us_start_msecs_ = (11 * 60) * 60 * 1000;      // UTC_1100 , JGBL_US hr starts at EST_700

  if (start_msecs_utc_ >= pre_as_msecs_ || start_msecs_utc_ <= eu_start_msecs_) {
    if (!GetSamplingShortcodesAS(r_dep_shortcode_, core_shortcode_vec_)) {
      GetSamplingShortcodesDefault(r_dep_shortcode_, core_shortcode_vec_);
    }
  } else if (start_msecs_utc_ < us_start_msecs_) {
    if (!GetSamplingShortcodesEU(r_dep_shortcode_, core_shortcode_vec_)) {
      GetSamplingShortcodesDefault(r_dep_shortcode_, core_shortcode_vec_);
    }
  } else {
    if (!GetSamplingShortcodesUS(r_dep_shortcode_, core_shortcode_vec_)) {
      GetSamplingShortcodesDefault(r_dep_shortcode_, core_shortcode_vec_);
    }
  }
  VectorUtils::UniqueVectorAdd(core_shortcode_vec_, r_dep_shortcode_);
}
}
