/**
    \file CommonTradeUtilsCode/rollover_utils.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "dvccode/CommonTradeUtils/rollover_utils.hpp"
namespace HFSAT {
namespace RollOverUtils {

// returns the major maturity for the shortcode
std::string GetNearestMajorExpiry(const std::string& shortcode_) {
  std::string shc_sname_ = shortcode_;
  if (shortcode_.compare("FGBM_1") == 0 || shortcode_.compare("FGBL_1") == 0 || shortcode_.compare("FGBS_1") == 0 ||
      shortcode_.compare("FESX_1") == 0 || shortcode_.compare("FDAX_1") == 0 || shortcode_.compare("FXXP_1") == 0 ||
      shortcode_.compare("FDXM_1") == 0 || shortcode_.compare("FGBX_1") == 0 || shortcode_.compare("FOAT_1") == 0 ||
      shortcode_.compare("FBTP_1") == 0 ||
      // CME
      shortcode_.compare("ZN_1") == 0 || shortcode_.compare("ZB_1") == 0 || shortcode_.compare("ZF_1") == 0 ||
      shortcode_.compare("ZT_1") == 0 || shortcode_.compare("UB_1") == 0 || shortcode_.compare("TN_1") == 0 ||
      shortcode_.compare("ES_1") == 0 || shortcode_.compare("NQ_1") == 0 || shortcode_.compare("YM_1") == 0 ||
      shortcode_.compare("6E_1") == 0 || shortcode_.compare("6B_1") == 0 || shortcode_.compare("6A_1") == 0 ||
      shortcode_.compare("6C_1") == 0 || shortcode_.compare("6J_1") == 0 || shortcode_.compare("6S_1") == 0 ||
      shortcode_.compare("6N_1") == 0 || shortcode_.compare("GC_1") == 0 ||
      // HK
      shortcode_.compare("HHI_1") == 0 || shortcode_.compare("HSI_1") == 0 || shortcode_.compare("MHI_1") == 0 ||
      shortcode_.compare("MCH_1") == 0 ||
      // OSE
      shortcode_.compare("NK_1") == 0 || shortcode_.compare("NKM_1") == 0 || shortcode_.compare("TOPIX_1") == 0 ||
      shortcode_.compare("JGBL_1") == 0 || shortcode_.compare("TOPIXM_1") == 0 ||
      // SGX
      shortcode_.compare("SGX_CN_1") == 0 || shortcode_.compare("SGX_IN_1") == 0 ||
      shortcode_.compare("SGX_IU_1") == 0 || shortcode_.compare("SGX_TW_1") == 0) {
    shc_sname_ = shortcode_.substr(0, shortcode_.size() - 1) + "0";
  }

  if (shortcode_.size() > 9 && !shortcode_.compare(0, 4, "NSE_") &&
      !shortcode_.compare(shortcode_.size() - 5, 5, "_FUT1")) {
    shc_sname_ = shortcode_.substr(0, shortcode_.size() - 1) + "0";
  }

  if (shortcode_.compare("SP_VXE0_VXE1") == 0) {
    shc_sname_ = "SP_VX0_VX1";
  }

  if (shortcode_.compare("VXE_0") == 0) {
    shc_sname_ = "VX_0";
  }

  return shc_sname_;
}
}
}
