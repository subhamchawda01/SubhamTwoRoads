/**
   \file VolatileTradingInfo/shortcode_ezone_vec.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_VOLATILETRADINGINFO_SHORTCODE_EZONE_VEC_H
#define BASE_VOLATILETRADINGINFO_SHORTCODE_EZONE_VEC_H

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"

namespace HFSAT {

inline EconomicZone_t GetTrZoneForShortcode(const std::string& r_shortcode_) {
  if (r_shortcode_.substr(0, 5).compare("FOAT_") == 0) {
    return EZ_TFOAT;
  }
  if (r_shortcode_.substr(0, 5).compare("FBON_") == 0) {
    return EZ_TFBON;
  }
  if (r_shortcode_.substr(0, 5).compare("FBTP_") == 0) {
    return EZ_TFBTP;
  }

  if (r_shortcode_.substr(0, 5).compare("FGBL_") == 0) {
    return EZ_TFGBL;
  }

  if (r_shortcode_.substr(0, 5).compare("FGBM_") == 0) {
    return EZ_TFGBM;
  }

  if (r_shortcode_.substr(0, 5).compare("FGBS_") == 0) {
    return EZ_TFGBS;
  }

  if (r_shortcode_.compare("BR_DOL_0") == 0) {
    return EZ_TDOL;
  }

  if (r_shortcode_.compare("BR_IND_0") == 0) {
    return EZ_TIND;
  }

  if (r_shortcode_.compare("CGB_0") == 0) {
    return EZ_TCGB;
  }

  if (r_shortcode_.compare("LFR_0") == 0) {
    return EZ_TLFR;
  }

  else {
    return EZ_MAX;
  }
}

inline void GetEZVecForShortcode(const std::string& r_shortcode_, const int& r_mfm_,
                                 std::vector<EconomicZone_t>& ezone_vec_) {
  const char* t_us_ezone_time_ = "EST_800";  // changing from 12 utc to EST_800
  int mfm_us_hours_ = HFSAT::GetMsecsFromMidnightFromHHMMSS(HFSAT::DateTime::GetUTCHHMMSSFromTZHHMMSS(
      HFSAT::DateTime::GetCurrentIsoDateLocal(), HFSAT::DateTime::GetHHMMSSTime(t_us_ezone_time_ + 4),
      t_us_ezone_time_));
  int mfm_as_end_hours_ = 6 * 60 * 60 * 1000;  // mfm corresponds to UTC_600
  int mfm_asx_as_end_hours_ =
      30 * 60 * 60 * 1000;  // to take care of interday trading.trading end mfm would be 30 for asx as session
  int mfm_asx_as_start_hours_ = 21 * 60 * 60 * 1000;  // query starting after this time would be as session query

  const char* t_cme_ezone_time_ = "EST_915";  // changing from 13:15 utc to EST_915
  int mfm_cme_us_ezone_ = HFSAT::GetMsecsFromMidnightFromHHMMSS(HFSAT::DateTime::GetUTCHHMMSSFromTZHHMMSS(
      HFSAT::DateTime::GetCurrentIsoDateLocal(), HFSAT::DateTime::GetHHMMSSTime(t_cme_ezone_time_ + 4),
      t_cme_ezone_time_));

  if (r_shortcode_.substr(0, 5).compare("FGBL_") == 0 || r_shortcode_.substr(0, 5).compare("FGBX_") == 0 ||
      r_shortcode_.substr(0, 5).compare("FGBM_") == 0 || r_shortcode_.substr(0, 5).compare("FGBS_") == 0) {
    if (r_mfm_ < mfm_us_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);
    } else {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else if (r_shortcode_.substr(0, 5).compare("FESX_0") == 0 || r_shortcode_.substr(0, 5).compare("FXXP_") == 0 ||
             r_shortcode_.substr(0, 5).compare("FSTB_0") == 0 || r_shortcode_.substr(0, 5).compare("FSTG_") == 0) {
    if (r_mfm_ < mfm_us_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);

    } else {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }

  } else if ((r_shortcode_.compare("FVS_0") == 0) || (r_shortcode_.compare("FVS_1") == 0) ||
             (r_shortcode_.compare("FVS_2") == 0) || (r_shortcode_.compare("FVS_3") == 0) ||
             (r_shortcode_.compare("FVS_4") == 0) || (r_shortcode_.compare("FVS_5") == 0) ||
             (r_shortcode_.compare("FVS_6") == 0) || (r_shortcode_.compare("FVS_7") == 0)) {
    if (r_mfm_ < mfm_us_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);

    } else {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }

  } else if ((r_shortcode_.substr(0, 5).compare("FOAT_") == 0) || (r_shortcode_.substr(0, 5).compare("FOAM_") == 0)) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_FRA);
    if (r_mfm_ < mfm_us_hours_) {
      // Add GER in EU
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
    } else {
      // Add USD in US
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else if ((r_shortcode_.substr(0, 5).compare("FBON_") == 0)) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_SPA);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);
    if (r_mfm_ < mfm_us_hours_) {
      // Add GER in EU
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
    } else {
      // Add USD in US
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else if (r_shortcode_.substr(0, 5).compare("FBTP_") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_ITA);

    if (r_mfm_ < mfm_us_hours_) {
      // Add GER in EU
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
    } else {
      // Add USD in US
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else if (r_shortcode_.substr(0, 5).compare("FBTS_") == 0) {
    // No default stopping for FBTS
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_ITA);

    if (r_mfm_ < mfm_us_hours_) {
      // Add GER in EU
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
    } else {
      // Add USD in US
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
    return;

  } else if (r_shortcode_.substr(0, 5).compare("FSMI_") == 0) {
    if (r_mfm_ < mfm_us_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CHF);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);

    } else {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CHF);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }

  } else if (r_shortcode_.substr(0, 5).compare("FDAX_") == 0 || r_shortcode_.substr(0, 5).compare("FDXM_") == 0) {
    if (r_mfm_ < mfm_us_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);

    } else {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else if (r_shortcode_.substr(0, 3).compare("ZT_") == 0) {
    if (r_mfm_ >= mfm_cme_us_ezone_ /*13:15 UTC ... wanted to make it 9:15am EST */) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    } else if (r_mfm_ < mfm_as_end_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CNY);
    } else if (r_mfm_ < mfm_us_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);

    } else {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else if (r_shortcode_.substr(0, 3).compare("ZF_") == 0 || r_shortcode_.substr(0, 3).compare("ZN_") == 0 ||
             r_shortcode_.substr(0, 3).compare("TN_") == 0 || r_shortcode_.substr(0, 3).compare("ZB_") == 0 ||
             r_shortcode_.substr(0, 3).compare("UB_") == 0) {
    if (r_mfm_ >= mfm_cme_us_ezone_ /*13:15 UTC ... wanted to make it 9:15am EST */) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    } else if (r_mfm_ < mfm_as_end_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CNY);
    } else if (r_mfm_ < mfm_us_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);

    } else {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else if (r_shortcode_.substr(0, 3).compare("ES_") == 0 || r_shortcode_.substr(0, 3).compare("6B_") == 0 ||
             r_shortcode_.substr(0, 3).compare("6E_") == 0 || r_shortcode_.substr(0, 3).compare("6S_") == 0 ||
             r_shortcode_.substr(0, 3).compare("NQ_") == 0 || r_shortcode_.substr(0, 3).compare("YM_") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
  } else if (r_shortcode_.substr(0, 3).compare("6J_") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_JPY);
  } else if (r_shortcode_.substr(0, 3).compare("GC_") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
  } else if (r_shortcode_.substr(0, 3).compare("6M_") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_MXN);
  } else if (r_shortcode_.substr(0, 3).compare("6N_") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_NZD);
  } else if (r_shortcode_.substr(0, 3).compare("6A_") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_AUD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
  } else if (r_shortcode_.substr(0, 3).compare("6S_") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CHF);
  } else if (r_shortcode_.compare("CGB_0") == 0 || r_shortcode_.compare("CGF_0") == 0 ||
             r_shortcode_.compare("CGZ_0") == 0 || r_shortcode_.compare("6C_0") == 0 ||
             r_shortcode_.compare("BAX_0") == 0 || r_shortcode_.compare("BAX_1") == 0 ||
             r_shortcode_.compare("BAX_2") == 0 || r_shortcode_.compare("BAX_3") == 0 ||
             r_shortcode_.compare("BAX_4") == 0 || r_shortcode_.compare("BAX_5") == 0 ||
             r_shortcode_.compare("BAX_6") == 0 || r_shortcode_.compare("BAX_7") == 0 ||
             r_shortcode_.compare("BAX_8") == 0 || r_shortcode_.compare("BAX_9") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CAD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
  } else if (r_shortcode_.compare("SXF_0") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CAD);
    // VectorUtils::UniqueVectorAdd ( ezone_vec_, EZ_USD );
  } else if (r_shortcode_.compare("BR_WIN_0") == 0 || r_shortcode_.compare("BR_IND_0") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_BRL);
    //	VectorUtils::UniqueVectorAdd ( ezone_vec_, EZ_USD );
  } else if (r_shortcode_.compare("BR_DOL_0") == 0 || r_shortcode_.compare("BR_WDO_0") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_BRL);
    //	VectorUtils::UniqueVectorAdd ( ezone_vec_, EZ_USD );
  } else if (r_shortcode_.compare("VAGR3") == 0 || r_shortcode_.compare("OGXP3") == 0 ||
             r_shortcode_.compare("PDGR3") == 0 || r_shortcode_.compare("PETR4") == 0 ||
             r_shortcode_.compare("VALE5") == 0 || r_shortcode_.compare("GFSA3") == 0 ||
             r_shortcode_.compare("ITSA4") == 0 || r_shortcode_.compare("JBSS3") == 0 ||
             r_shortcode_.compare("BBAS3") == 0 || r_shortcode_.compare("ITUB4") == 0 ||
             r_shortcode_.compare("TIMP3") == 0 || r_shortcode_.compare("BVMF3") == 0 ||
             r_shortcode_.compare("MRVE3") == 0 || r_shortcode_.compare("CYRE3") == 0 ||
             r_shortcode_.compare("GGBR4") == 0 || r_shortcode_.compare("RSID3") == 0 ||
             r_shortcode_.compare("CSNA3") == 0 || r_shortcode_.compare("BBDC4") == 0 ||
             r_shortcode_.compare("USIM5") == 0 || r_shortcode_.compare("MMXM3") == 0 ||
             r_shortcode_.compare("CCRO3") == 0 || r_shortcode_.compare("MRFG3") == 0 ||
             r_shortcode_.compare("BISA3") == 0 || r_shortcode_.compare("PETR3") == 0 ||
             r_shortcode_.compare("SANB11") == 0 || r_shortcode_.compare("ELET3") == 0 ||
             r_shortcode_.compare("CMIG4") == 0 || r_shortcode_.compare("ELET6") == 0 ||
             r_shortcode_.compare("CIEL3") == 0 || r_shortcode_.compare("VALE3") == 0 ||
             r_shortcode_.compare("BOVA11") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_BRL);
  } else if (r_shortcode_.compare(0, 3, "DI1") == 0) {
    // For DI1s we are not stopping be default on any events
    // TODO : look at whethere we shoudl stop on auctions
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_BMFDI);
  } else if (r_shortcode_.compare("JFFCE_0") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_FRA);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
  } else if (r_shortcode_.compare("KFFTI_0") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_DUT);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
  } else if (r_shortcode_.compare("LFR_0") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);
    if (r_mfm_ < mfm_us_hours_) {
      // Add GER in EU
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
    } else {
      // Add USD in US
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else if (r_shortcode_.compare("LFZ_0") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);
  } else if (r_shortcode_.compare(0, 4, "LFL_") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);
  } else if (r_shortcode_.compare(0, 4, "LFI_") == 0 || r_shortcode_.compare(0, 6, "SP_LFI") == 0) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_LFI);  // some ECB specific events only
  } else if ((r_shortcode_.compare("NK_0") == 0) || (r_shortcode_.compare("NKM_0") == 0) ||
             (r_shortcode_.compare("NKMF_0") == 0) || (r_shortcode_.compare("JGBL_0") == 0) ||
             (r_shortcode_.compare("TOPIX_0") == 0) || (r_shortcode_.compare("NKM_1") == 0) ||
             (r_shortcode_.compare("JP400_0") == 0)) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_JPY);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CNY);
    if (r_mfm_ >= mfm_us_hours_) {
      // adding stopping for US hours
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else if ((r_shortcode_.compare("NKD_0") == 0) || (r_shortcode_.compare("NIY_0") == 0)) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_JPY);
  } else if ((r_shortcode_.compare("HHI_0") == 0) || (r_shortcode_.compare("MCH_0") == 0) ||
             (r_shortcode_.compare("HSI_0") == 0) || (r_shortcode_.compare("MHI_0") == 0)) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CNY);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_HKD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_JPY);
  } else if ((r_shortcode_.compare("RI_0") == 0) || (r_shortcode_.compare("Si_0") == 0) ||
             (r_shortcode_.compare("ED_0") == 0) || (r_shortcode_.compare("MX_0") == 0) ||
             (r_shortcode_.compare("GD_0") == 0) || (r_shortcode_.compare("VB_0") == 0) ||
             (r_shortcode_.compare("LK_0") == 0) || (r_shortcode_.compare("BR_0") == 0) ||
             (r_shortcode_.compare("Eu_0") == 0) || (r_shortcode_.compare("GZ_0") == 0) ||
             (r_shortcode_.compare("SR_0") == 0) || (r_shortcode_.compare("GM_0") == 0) ||
             (r_shortcode_.compare("RN_0") == 0) || (r_shortcode_.compare("SBER") == 0) ||
             (r_shortcode_.compare("ROSN") == 0) || (r_shortcode_.compare("MGNT") == 0) ||
             (r_shortcode_.compare("GMKN") == 0) || (r_shortcode_.compare("LKOH") == 0) ||
             (r_shortcode_.compare("GAZP") == 0) || (r_shortcode_.compare("VTBR") == 0) ||
             (r_shortcode_.compare("SNGS") == 0) || (r_shortcode_.compare("TATN") == 0) ||
             (r_shortcode_.compare("USD000000TOD") == 0) || (r_shortcode_.compare("USD000TODTOM") == 0) ||
             (r_shortcode_.compare("USD000UTSTOM") == 0) || (r_shortcode_.compare("EUR_RUB__TOD") == 0) ||
             (r_shortcode_.compare("EUR_RUB__TOM") == 0)) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_RUB);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_RTS);

    if (r_shortcode_.compare("ED_0") == 0) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
    }
  } else if ((r_shortcode_.compare("YFEBM_0") == 0) || (r_shortcode_.compare("YFEBM_1") == 0) ||
             (r_shortcode_.compare("YFEBM_2") == 0) || (r_shortcode_.compare("YFEBM_3") == 0) ||
             (r_shortcode_.compare("YFEBM_4") == 0) || (r_shortcode_.compare("XFC_0") == 0) ||
             (r_shortcode_.compare("XFC_1") == 0) || (r_shortcode_.compare("XFC_2") == 0) ||
             (r_shortcode_.compare("XFC_3") == 0) || (r_shortcode_.compare("XFC_4") == 0) ||
             (r_shortcode_.compare("XFRC_0") == 0) || (r_shortcode_.compare("XFRC_1") == 0) ||
             (r_shortcode_.compare("XFRC_2") == 0) || (r_shortcode_.compare("XFRC_3") == 0) ||
             (r_shortcode_.compare("XFRC_4") == 0)) {
    // NO default stops for FEBM, XFC and XFRC only manual handling
    return;
  } else if ((r_shortcode_.compare("GE_0") == 0) || (r_shortcode_.compare("GE_1") == 0) ||
             (r_shortcode_.compare("GE_2") == 0) || (r_shortcode_.compare("GE_3") == 0) ||
             (r_shortcode_.compare("GE_4") == 0) || (r_shortcode_.compare("GE_5") == 0) ||
             (r_shortcode_.compare("GE_6") == 0) || (r_shortcode_.compare("GE_7") == 0) ||
             (r_shortcode_.compare("GE_8") == 0) || (r_shortcode_.compare("GE_9") == 0) ||
             (r_shortcode_.compare("GE_10") == 0) || (r_shortcode_.compare("GE_11") == 0) ||
             (r_shortcode_.compare("GE_12") == 0) || (r_shortcode_.compare("GE_13") == 0) ||
             (r_shortcode_.compare("GE_14") == 0) || (r_shortcode_.compare("GE_15") == 0) ||
             (r_shortcode_.compare("GE_16") == 0) || (r_shortcode_.compare("SP_GE0_GE1") == 0) ||
             (r_shortcode_.compare("SP_GE1_GE2") == 0) || (r_shortcode_.compare("SP_GE2_GE3") == 0) ||
             (r_shortcode_.compare("SP_GE3_GE4") == 0) || (r_shortcode_.compare("SP_GE4_GE5") == 0) ||
             (r_shortcode_.compare("SP_GE5_GE6") == 0) || (r_shortcode_.compare("SP_GE6_GE7") == 0) ||
             (r_shortcode_.compare("SP_GE7_GE8") == 0) || (r_shortcode_.compare("SP_GE8_GE9") == 0) ||
             (r_shortcode_.compare("SP_GE9_GE10") == 0) || (r_shortcode_.compare("SP_GE10_GE11") == 0) ||
             (r_shortcode_.compare("SP_GE11_GE12") == 0) || (r_shortcode_.compare("SP_GE12_GE13") == 0) ||
             (r_shortcode_.compare("SP_GE13_GE14") == 0) || (r_shortcode_.compare("SP_GE14_GE15") == 0) ||
             (r_shortcode_.compare("SP_GE15_GE16") == 0)) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    // Severity to stop on would be 3 , change in adjustSeverity function
  }

  else if ((r_shortcode_.compare("VX_0") == 0) || (r_shortcode_.compare("VX_1") == 0) ||
           (r_shortcode_.compare("VX_2") == 0) || (r_shortcode_.compare("VX_3") == 0) ||
           (r_shortcode_.compare("VX_4") == 0) || (r_shortcode_.compare("VX_5") == 0) ||
           (r_shortcode_.compare("VX_6") == 0) || (r_shortcode_.compare("VX_7") == 0) ||
           (r_shortcode_.compare("SP_VX0_VX1") == 0) || (r_shortcode_.compare("SP_VX1_VX2") == 0) ||
           (r_shortcode_.compare("SP_VX2_VX3") == 0) || (r_shortcode_.compare("SP_VX0_VX2") == 0) ||
           (r_shortcode_.compare("SP_VX0_VX3") == 0) || (r_shortcode_.compare("SP_VX1_VX3") == 0) ||
           (r_shortcode_.compare("SP_VX0_VX4") == 0) || (r_shortcode_.compare("SP_VX1_VX4") == 0) ||
           (r_shortcode_.compare("SP_VX0_VX5") == 0) || (r_shortcode_.compare("SP_VX3_VX4") == 0) ||
           (r_shortcode_.compare("SP_VX1_VX5") == 0) || (r_shortcode_.compare("SP_VX2_VX4") == 0) ||
           (r_shortcode_.compare("SP_VX2_VX5") == 0) || (r_shortcode_.compare("SP_VX4_VX5") == 0) ||
           (r_shortcode_.compare("SP_VX0_VX6") == 0) || (r_shortcode_.compare("SP_VX3_VX5") == 0) ||
           (r_shortcode_.compare("SP_VX1_VX6") == 0) || (r_shortcode_.compare("SP_VX2_VX6") == 0) ||
           (r_shortcode_.compare("SP_VX5_VX6") == 0) || (r_shortcode_.compare("SP_VX3_VX6") == 0) ||
           (r_shortcode_.compare("SP_VX4_VX6") == 0) || (r_shortcode_.compare("SP_VX7_VX0") == 0) ||
           (r_shortcode_.compare("SP_VX7_VX3") == 0) || (r_shortcode_.compare("SP_VX7_VX1") == 0) ||
           (r_shortcode_.compare("SP_VX7_VX4") == 0) || (r_shortcode_.compare("SP_VX7_VX5") == 0) ||
           (r_shortcode_.compare("SP_VX7_VX2") == 0) || (r_shortcode_.compare("SP_VX7_VX6") == 0) ||
           (r_shortcode_.compare("SP_VX0_VX1_VX2") == 0) || (r_shortcode_.compare("VSW1_0") == 0) ||
           (r_shortcode_.compare("VSW2_0") == 0) || (r_shortcode_.compare("VSW3_0") == 0)) {
    // not stopping VX at any event
  } else if ((r_shortcode_.compare("XT_0") == 0) || (r_shortcode_.compare("YT_0") == 0) ||
             (r_shortcode_.compare("XTE_0") == 0) || (r_shortcode_.compare("YTE_0") == 0) ||
             (r_shortcode_.compare("AP_0") == 0) || (r_shortcode_.compare("IR_0") == 0) ||
             (r_shortcode_.compare("IR_1") == 0) || (r_shortcode_.compare("IR_2") == 0) ||
             (r_shortcode_.compare("IR_3") == 0) || (r_shortcode_.compare("IR_4") == 0) ||
             (r_shortcode_.compare("SP_XT0_YT0") == 0)) {
    if (r_mfm_ > mfm_asx_as_start_hours_ && r_mfm_ < mfm_asx_as_end_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_AUD);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CNY);
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_JPY);
    }
  } else if ((r_shortcode_.compare("SGX_NK_0") == 0) || (r_shortcode_.compare("SGX_CN_0") == 0) ||
             (r_shortcode_.compare("SGX_TW_0") == 0) || (r_shortcode_.compare("SGX_SG_0") == 0) ||
             (r_shortcode_.compare("SGX_NK_1") == 0) || (r_shortcode_.compare("SP_SGX_NK0_NK1") == 0) ||
             (r_shortcode_.compare("SGX_CN_1") == 0)) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_JPY);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CNY);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_HKD);
    if (r_mfm_ >= mfm_us_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else if ((r_shortcode_.compare("SGX_IU_0") == 0) || (r_shortcode_.compare("SGX_IN_0") == 0)) {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_INR);
    if (r_mfm_ >= mfm_us_hours_) {
      VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
    }
  } else {
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_AUD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CAD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CNY);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_EUR);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GER);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_JPY);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_NZD);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_CHF);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_GBP);
    VectorUtils::UniqueVectorAdd(ezone_vec_, EZ_USD);
  }
}
}

#endif  // BASE_VOLATILETRADINGINFO_SHORTCODE_EZONE_VEC_H
