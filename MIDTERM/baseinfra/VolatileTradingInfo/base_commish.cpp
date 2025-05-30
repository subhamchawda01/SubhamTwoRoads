/**
 \file CDefCode/base_commish.cpp
 1
 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066, India
 +91 80 4060 0717
 */
#include "dvccode/CDef/bse_security_definition.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"

namespace HFSAT {

CommishMap BaseCommish::commish_map_;
CommishMap BaseCommish::retail_commish_map_;
CommishTiers BaseCommish::di1_retail_commish_tiers_;

void BaseCommish::InitCommishMap(CommishMap& _commish_map_) {
#define CLEARING_COMMISH 0.03
#define CME_CLEARING_COMMISH_DOLLARS 0.02  // 2 cents US
#define EUREX_CLEARING_COMMISH_EUROS 0.02  // 2 cents EU
#define LIFFE_CLEARING_COMMISH_EUROS 0.03  // 3 cents EU
#define LIFFE_BROKER_COMMISH_EUROS 0.02    // 2 cents EU
#define IDEM_CLEARING_COMMISH_EUROS 0.10   // 10 cents EU
#define TMX_CLEARING_COMMISH_CAD 0.10      // 10 cents CAD
#define TMX_BROKER_COMMISH_CAD 0.05        // 5 cents CAD
#define HKFE_CLEARING_COMMISH_HKD 1.00     // 1 HKD to 1.75 HKD
#define RTS_CLEARING_COMMISH_RUB 0.30      // 50 cents RUB
#define SI_CLEARING_COMMISH_RUB 0.05       // 5 cents RUB
#define RI_CLEARING_COMMISH_RUB 0.20       // 20 cents RUB
#define ED_CLEARING_COMMISH_RUB 0.05       // 5 cents RUB
#define EU_CLEARING_COMMISH_RUB 0.05       // 5 cents RUB
#define BR_CLEARING_COMMISH_RUB 0.10       // 10 cents RUB
#define SR_CLEARING_COMMISH_RUB 0.02       // 2 cents RUB
#define MX_CLEARING_COMMISH_RUB 0.20       // 25 cents RUB
#define LK_CLEARING_COMMISH_RUB 0.04       // 4 cents RUB
#define VB_CLEARING_COMMISH_RUB 0.01       // 1 cent RUB
#define GZ_CLEARING_COMMISH_RUB 0.03       // 3 cents RUB
#define GD_CLEARING_COMMISH_RUB 0.10       // 10 cents RUB
#define MICEX_CLEARING_COMMISH_RUB 0.18    // this is 0.06 bps , need to figure out approximate RUB values
#define ASX_BROKERAGE 0.10                 // ABN fee
#define SGX_NK_LICENSING_FEE 0.15
#define SGX_CN_LICENSING_FEE 0.025
#define SGX_TW_LICENSING_FEE 0.05
#define SGX_SG_LICENSING_FEE 0.20  // in SGD
#define SGX_TAIWAN_LICENSING_FEE 0.05
#define SGX_NIFTY_LICENSING_FEE 0.175
#define SGX_BROKERAGE 0.10
#define CFE_EXCHANGE_FEE 0.55
#define CFE_REGULATORY_FEE 0.04
#define CFE_BROKER_FEE 0.02
#define CFE_CLEARING_FEE 0.04

  /* CME */
  _commish_map_["GE_0"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_1"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_2"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_3"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_4"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_5"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_6"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_7"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_8"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_9"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_10"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_11"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_12"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_13"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_14"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_15"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GE_16"] = 0.44 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZT_0"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZT_1"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;

  _commish_map_["SP_GE0_GE1"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE1_GE2"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE2_GE3"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE3_GE4"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE4_GE5"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE5_GE6"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE6_GE7"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE7_GE8"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE8_GE9"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE9_GE10"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE10_GE11"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE11_GE12"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE12_GE13"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE13_GE14"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE14_GE15"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);
  _commish_map_["SP_GE15_GE16"] = 2 * (0.44 + CME_CLEARING_COMMISH_DOLLARS);

  _commish_map_["ZF_0"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZF_1"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZN_0"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZN_1"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["TN_0"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["TN_1"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZB_0"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZB_1"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["UB_0"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["UB_1"] = 0.12 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ES_0"] = 0.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ES_1"] = 0.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["NQ_0"] = 0.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["NQ_1"] = 0.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["YM_0"] = 0.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["YM_1"] = 0.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6A_0"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6A_1"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6B_0"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6B_1"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6C_0"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6C_1"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6E_0"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6E_1"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6J_0"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6J_1"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6M_0"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6M_1"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6N_0"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6N_1"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6S_0"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["6S_1"] = 0.32 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["CL_0"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["CL_1"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["CL_2"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["CL_3"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["SP_CL0_CL1"] = 2 * ( 1.50 + CME_CLEARING_COMMISH_DOLLARS );
  _commish_map_["SP_CL1_CL2"] = 2 * ( 1.50 + CME_CLEARING_COMMISH_DOLLARS );
  _commish_map_["SP_CL2_CL3"] = 2 * ( 1.50 + CME_CLEARING_COMMISH_DOLLARS );
  _commish_map_["RB_0"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["RB_1"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["RB_2"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["BZ_0"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["BZ_1"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["BZ_2"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["NG_0"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["NG_1"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["NN_0"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["QM_0"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["HO_0"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["HO_1"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["RB_0"] = 1.50 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["GC_0"] = 0.55 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["HG_0"] = 0.55 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["SI_0"] = 0.55 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["PA_0"] = 0.55 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["PD_0"] = 0.55 + CME_CLEARING_COMMISH_DOLLARS;

  _commish_map_["ZW_0"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZW_1"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZW_2"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZW_3"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["KE_0"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["KE_1"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["KE_2"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZC_0"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZC_1"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZC_2"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZS_0"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZS_1"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZS_2"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZL_0"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["ZM_0"] = 0.81 + CME_CLEARING_COMMISH_DOLLARS;

  _commish_map_["NKD_0"] = 1.02 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["NKD_1"] = 1.02 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["NIY_0"] = 1.03 + CME_CLEARING_COMMISH_DOLLARS;
  _commish_map_["NIY_1"] = 1.03 + CME_CLEARING_COMMISH_DOLLARS;

  /* CFE */
  _commish_map_["VX_0"] = CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE;
  _commish_map_["VXE_0"] = CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE;
  _commish_map_["VX_1"] = CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE;
  _commish_map_["VX_2"] = CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE;
  _commish_map_["VX_3"] = CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE;
  _commish_map_["VX_4"] = CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE;
  _commish_map_["VX_5"] = CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE;
  _commish_map_["VX_6"] = CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE;
  _commish_map_["VX_7"] = CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE;
  _commish_map_["SP_VX0_VX1"] = (CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE) * 2;
  _commish_map_["SP_VXE0_VXE1"] = (CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE) * 2;
  _commish_map_["SP_VX1_VX2"] = (CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE) * 2;
  _commish_map_["SP_VX2_VX3"] = (CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE) * 2;
  _commish_map_["SP_VX3_VX4"] = (CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE) * 2;
  _commish_map_["SP_VX4_VX5"] = (CFE_EXCHANGE_FEE + CFE_REGULATORY_FEE + CFE_BROKER_FEE + CFE_CLEARING_FEE) * 2;

  /* EUREX */
  _commish_map_["FGBS_0"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FGBS_1"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FGBM_0"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FGBM_1"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FGBL_0"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FGBL_1"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FBTP_0"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FBTP_1"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FBTS_0"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FBTS_1"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["FOAT_0"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FOAT_1"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["FBON_0"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FOAM_0"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FXXP_0"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FXXP_1"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FSTB_0"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FSTG_0"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FSMI_0"] = 0.40 * CurrencyConvertor::Convert(kCurrencyCHF, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FSMI_1"] = 0.40 * CurrencyConvertor::Convert(kCurrencyCHF, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["FGBX_0"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FGBX_1"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["FESX_0"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FESX_1"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["FESQ_0"] =
      0.50 + (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["FESB_0"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FESB_1"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["FDAX_0"] = 0.50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FDAX_1"] = 0.50 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["FDXM_0"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FDXM_1"] = 0.20 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["FVS_0"] = (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FVS_1"] = (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FVS_2"] = (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FVS_3"] = (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FVS_4"] = (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FVS_5"] = (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FVS_6"] = (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FVS_7"] = (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["SP_FVS0_FVS1"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS0_FVS2"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;

  _commish_map_["SP_FVS0_FVS3"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS0_FVS4"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS1_FVS2"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS1_FVS3"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS1_FVS4"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS1_FVS5"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS2_FVS3"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS2_FVS4"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS2_FVS5"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS2_FVS6"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS3_FVS4"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS3_FVS5"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS3_FVS6"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS3_FVS7"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS4_FVS5"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS4_FVS6"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS4_FVS7"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS5_FVS6"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS5_FVS7"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;
  _commish_map_["SP_FVS6_FVS7"] =
      (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD)) * 2;

  _commish_map_["FCEU_0"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FCEU_1"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FCEU_2"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FCPU_0"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FCPU_1"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FCPU_2"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FCUF_0"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FCUF_1"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FCUF_2"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  _commish_map_["FEU3_0"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_1"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_2"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_3"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_4"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_5"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_6"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_7"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_8"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_9"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                            (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_10"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                             (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_11"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                             (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_12"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                             (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_13"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                             (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_14"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                             (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_15"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                             (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_16"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                             (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_17"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                             (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_18"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                             (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["FEU3_19"] = 0.30 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
                             (EUREX_CLEARING_COMMISH_EUROS * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));

  /* LIFFE */
  _commish_map_["JFFCE_0"] =
      0.27 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["KFFTI_0"] =
      0.44 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFZ_0"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFR_0"] = 0.23 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_0"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_1"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_2"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_3"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_4"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_5"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_6"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_7"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_8"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_9"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_10"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_11"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFL_12"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_0"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_1"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_2"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_3"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_4"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_5"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_6"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_7"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_8"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_9"] = 0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_10"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_11"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_12"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_13"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_14"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_15"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_16"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_17"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_18"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["LFI_19"] =
      0.28 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;

  _commish_map_["SP_LFI0_LFI1"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI1_LFI2"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI2_LFI3"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI3_LFI4"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI4_LFI5"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI5_LFI6"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI6_LFI7"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI7_LFI8"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI8_LFI9"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI0_LFI2"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI1_LFI3"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI2_LFI4"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI3_LFI5"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI4_LFI6"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI5_LFI7"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI6_LFI8"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI7_LFI9"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI8_LFI10"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI9_LFI11"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFI10_LFI12"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL0_LFL1"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL1_LFL2"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL2_LFL3"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL3_LFL4"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL4_LFL5"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL5_LFL6"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL6_LFL7"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL7_LFL8"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL8_LFL9"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL0_LFL2"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL1_LFL3"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL2_LFL4"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL3_LFL5"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL4_LFL6"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL5_LFL7"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL6_LFL8"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL7_LFL9"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL8_LFL10"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL9_LFL11"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["SP_LFL10_LFL12"] =
      0.28 * 2 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + 2 * LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["YFEBM_0"] =
      0.25 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
      ((0.7 + LIFFE_BROKER_COMMISH_EUROS) * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["YFEBM_1"] =
      0.25 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
      ((0.7 + LIFFE_BROKER_COMMISH_EUROS) * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["YFEBM_2"] =
      0.25 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
      ((0.7 + LIFFE_BROKER_COMMISH_EUROS) * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["YFEBM_3"] =
      0.25 * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD) +
      ((0.7 + LIFFE_BROKER_COMMISH_EUROS) * CurrencyConvertor::Convert(kCurrencyEUR, kCurrencyUSD));
  _commish_map_["XFC_0"] = 0.54 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["XFC_1"] = 0.54 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["XFRC_0"] =
      0.82 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;
  _commish_map_["XFRC_1"] =
      0.82 * CurrencyConvertor::Convert(kCurrencyGBP, kCurrencyUSD) + LIFFE_CLEARING_COMMISH_EUROS;

  /* TMX */
  _commish_map_["CGB_0"] = (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["CGB_1"] = (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);

  _commish_map_["CGF_0"] = (0.08 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["CGZ_0"] = (0.08 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SXF_0"] = (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["EMF_0"] =
      0.21 + (TMX_CLEARING_COMMISH_CAD *
              CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));  // need to decide on changes if needed
  _commish_map_["BAX_0"] = (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["BAX_1"] = (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["BAX_2"] = (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["BAX_3"] = (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["BAX_4"] = (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["BAX_5"] = (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["BAX_6"] = (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["BAX_7"] = (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["BAX_8"] = (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["BAX_9"] = (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                           CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX0_BAX1"] = 2 * (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX0_BAX2"] = 2 * (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX0_BAX3"] = 2 * (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX0_BAX4"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX0_BAX5"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX0_BAX6"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX0_BAX7"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX0_BAX8"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX0_BAX9"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX1_BAX2"] = 2 * (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX1_BAX3"] = 2 * (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX1_BAX4"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX1_BAX5"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX1_BAX6"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX1_BAX7"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX1_BAX8"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX1_BAX9"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX2_BAX3"] = 2 * (0.11 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX2_BAX4"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX2_BAX5"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX2_BAX6"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX2_BAX7"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX2_BAX8"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX2_BAX9"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX3_BAX4"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX3_BAX5"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX3_BAX6"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX3_BAX7"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX3_BAX8"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX3_BAX9"] = (0.11 + 0.05 +
                                   2 * (TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                       CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["SP_BAX4_BAX5"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX4_BAX6"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX4_BAX7"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX4_BAX8"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX4_BAX9"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX5_BAX6"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX5_BAX7"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX5_BAX8"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX5_BAX9"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX6_BAX7"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX6_BAX8"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX6_BAX9"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX7_BAX8"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX7_BAX9"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["SP_BAX8_BAX9"] = 2 * (0.05 + TMX_CLEARING_COMMISH_CAD + TMX_BROKER_COMMISH_CAD) *
                                  CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD);
  _commish_map_["FLY_BAX0_BAX1_BAX2"] =
      0.21 * 4 + (TMX_CLEARING_COMMISH_CAD * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["FLY_BAX0_BAX1_BAX3"] =
      0.21 * 4 + (TMX_CLEARING_COMMISH_CAD * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["FLY_BAX0_BAX2_BAX4"] =
      0.21 * 4 + (TMX_CLEARING_COMMISH_CAD * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["FLY_BAX1_BAX2_BAX3"] =
      0.21 * 4 + (TMX_CLEARING_COMMISH_CAD * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["FLY_BAX2_BAX3_BAX4"] =
      0.21 * 4 + (TMX_CLEARING_COMMISH_CAD * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["FLY_BAX3_BAX4_BAX5"] =
      0.21 * 4 + (TMX_CLEARING_COMMISH_CAD * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["FLY_BAX4_BAX5_BAX6"] =
      0.21 * 4 + (TMX_CLEARING_COMMISH_CAD * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["FLY_BAX5_BAX6_BAX7"] =
      0.21 * 4 + (TMX_CLEARING_COMMISH_CAD * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["FLY_BAX6_BAX7_BAX8"] =
      0.21 * 4 + (TMX_CLEARING_COMMISH_CAD * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));
  _commish_map_["FLY_BAX7_BAX8_BAX9"] =
      0.21 * 4 + (TMX_CLEARING_COMMISH_CAD * CurrencyConvertor::Convert(kCurrencyCAD, kCurrencyUSD));

  /* BMF */
  _commish_map_["BR_IND_0"] = 0.33 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BR_IND_1"] = 0.33 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BR_WIN_0"] = 0.03 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BR_WIN_1"] = 0.03 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BR_DOL_0"] = 0.17 + (0.166) * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BR_DOL_1"] = 0.17 + (0.166) * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["DR1_0"] =
      0.34 +
      (0.332) *
          CurrencyConvertor::Convert(
              kCurrencyBRL, kCurrencyUSD);  // TODO: This is mostly likely 2 * dol_commish. Check and update (hardik).
  _commish_map_["BR_WDO_0"] = 0.0305 + (0.01) * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BR_WDO_1"] = 0.0305 + (0.01) * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);

  _commish_map_["BR_ISP_0"] = (0.22 + 0.20);
  _commish_map_["BR_CCM_0"] = 0.3 * (0.33 + 0.27) * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BR_ICF_0"] = 0.3 * (0.31 + 0.41);
  _commish_map_["BR_SJC_0"] = 0.3 * (0.19 + 0.20);
  _commish_map_["BR_SJC_1"] = 0.3 * (0.19 + 0.20);
  _commish_map_["BR_SJC_2"] = 0.3 * (0.19 + 0.20);

  _commish_map_["VAGR3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["OGXP3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["PDGR3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["PETR4"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["VALE5"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["GFSA3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["ITSA4"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["JBSS3"] = 0.0017 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BBAS3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["ITUB4"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["TIMP3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BVMF3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["MRVE3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["CYRE3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["GGBR4"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["RSID3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["CSNA3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BBDC4"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["USIM5"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["MMXM3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["CCRO3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["MRFG3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["BISA3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["PETR3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["SANB11"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["ELET3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["CMIG4"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["ELET6"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["CIEL3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  _commish_map_["VALE3"] = 0.0 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);

  // HKEX - tier 2 commish - exch-fee + commish-levy + newedge-commish
  _commish_map_["HSI_0"] = (10.0 + 0.54 + 1.50) * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD);
  _commish_map_["HSI_1"] = (10.0 + 0.54 + 1.50) * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD);
  _commish_map_["MHI_0"] = (3.5 + 0.12 + 0.40) * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD);  // Mini HSI
  _commish_map_["MHI_1"] = (3.5 + 0.12 + 0.40) * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD);  // Mini HSI
  _commish_map_["HHI_0"] = (3.5 + 0.54 + 1.50) * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD);
  _commish_map_["HHI_1"] = (3.5 + 0.54 + 1.50) * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD);
  // Mini H-shares
  _commish_map_["MCH_0"] = (2.0 + 0.12 + 0.40) * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD);
  _commish_map_["MCH_1"] = (2.0 + 0.12 + 0.40) * CurrencyConvertor::Convert(kCurrencyHKD, kCurrencyUSD);

  // OSE - worst fees
  _commish_map_["NKMF_0"] = (11) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);    // Mini Nikkei
  _commish_map_["TOPIX_0"] = (82) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);   // Topix futures
  _commish_map_["TOPIX_1"] = (82) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);   // Topix futures
  _commish_map_["TOPIXM_0"] = (10) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);  // JGB futures
  _commish_map_["TOPIXM_1"] = (10) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);  // JGB futures
  _commish_map_["NK_0"] = (90) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);      // Nikkei
  _commish_map_["NK_1"] = (90) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);      // Nikkei
  _commish_map_["NKM_0"] = (11) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);     // Mini Nikkei
  _commish_map_["NKM_1"] = (11) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);     // Mini Nikkei
  _commish_map_["JP400_0"] = (15) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);   // guess
  _commish_map_["JP400_1"] = (15) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);   // guess
  _commish_map_["JN400_0"] =
      (15) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);  // TODO: same as JP400, remove one entry?
  _commish_map_["JN400_1"] =
      (15) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);  // TODO: same as JP400, remove one entry?
  _commish_map_["JGBL_0"] = (189) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);  //
  _commish_map_["JGBL_1"] = (189) * CurrencyConvertor::Convert(kCurrencyJPY, kCurrencyUSD);  //

  /* RTS */
  _commish_map_["RI_0"] = (1.25 + RI_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["MX_0"] = (2.00 + MX_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);

  _commish_map_["Si_0"] = (0.465 + SI_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["ED_0"] = (0.51 + ED_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["Eu_0"] = (0.525 + EU_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["OX_0"] = (0.25 + RTS_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["OV_0"] = (0.25 + RTS_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["O2_0"] = (0.25 + RTS_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["O4_0"] = (0.25 + RTS_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["O6_0"] = (0.25 + RTS_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);

  _commish_map_["GD_0"] = (1.71 + GD_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["BR_0"] = (0.605 + BR_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["BR_1"] = (0.605 + BR_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);

  _commish_map_["LK_0"] = (0.92 + LK_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["VB_0"] = (0.25 + VB_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["GZ_0"] = (0.50 + GZ_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["SR_0"] = (0.25 + SR_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["GM_0"] = (0.50 + RTS_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["RN_0"] = (0.50 + RTS_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);

  /* MICEX */
  _commish_map_["USD000000TOD"] = 0.011;
  _commish_map_["USD000TODTOM"] = 2.2;
  _commish_map_["USD000UTSTOM"] = 0.011;
  _commish_map_["EUR_RUB__TOD"] = 0.011;
  _commish_map_["EUR_RUB__TOM"] = 0.011;

  _commish_map_["SBER"] = (0 + MICEX_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["VTBR"] = (0 + MICEX_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["MGNT"] = (0 + MICEX_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["LKOH"] = (0 + MICEX_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["GAZP"] = (0 + MICEX_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["GMKN"] = (0 + MICEX_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["ROSN"] = (0 + MICEX_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["SNGS"] = (0 + MICEX_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);
  _commish_map_["TATN"] = (0 + MICEX_CLEARING_COMMISH_RUB) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD);

  // ASX
  /*
  _commish_map_["XT_0"] = 0.9 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD) + ASX_BROKERAGE ;
  _commish_map_["YT_0"] = 0.9 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD) + ASX_BROKERAGE ;
  _commish_map_["AP_0"] = 1 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD) + ASX_BROKERAGE ;
  _commish_map_["IR_0"] = 0.9 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD) + ASX_BROKERAGE ;
  _commish_map_["IR_1"] = 0.9 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD) + ASX_BROKERAGE ;
  _commish_map_["IR_2"] = 0.9 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD) + ASX_BROKERAGE ;
  _commish_map_["IR_3"] = 0.9 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD) + ASX_BROKERAGE ;
  _commish_map_["IR_4"] = 0.9 * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD) + ASX_BROKERAGE ;
  */

  // Adding new trader commissions for now to increase volumes

  _commish_map_["XT_0"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["YT_0"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["XTE_0"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["YTE_0"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["XTE_1"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["XT_1"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["YTE_1"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);

  _commish_map_["AP_0"] = (0.75 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["IR_0"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["IR_1"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["IR_2"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["IR_3"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
  _commish_map_["IR_4"] = (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);

  // Approximate dv01 neutral # of contracts for now
  _commish_map_["SP_XT0_YT0"] =
      (10 + 34) * (0.675 + ASX_BROKERAGE) * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);

  // SGX Commissions Considering 25% rebate for CN as of now.
  _commish_map_["SGX_CN_0"] = 0.30 + SGX_CN_LICENSING_FEE + SGX_BROKERAGE;
  _commish_map_["SGX_CN_1"] = 0.30 + SGX_CN_LICENSING_FEE + SGX_BROKERAGE;

  _commish_map_["SGX_TW_0"] = 0.50 + SGX_TW_LICENSING_FEE + SGX_BROKERAGE;

  _commish_map_["SGX_IN_0"] = 0.20 + SGX_NIFTY_LICENSING_FEE + SGX_BROKERAGE;
  _commish_map_["SGX_IN_1"] = 0.20 + SGX_NIFTY_LICENSING_FEE + SGX_BROKERAGE;

  _commish_map_["SGX_SG_0"] =
      (0.20 + SGX_SG_LICENSING_FEE) * CurrencyConvertor::Convert(kCurrencySGD, kCurrencyUSD) + SGX_BROKERAGE;

  _commish_map_["SGX_AJ_0"] = 0.35 + SGX_BROKERAGE;
  _commish_map_["SGX_AJ_1"] = 0.35 + SGX_BROKERAGE;

  _commish_map_["SGX_AU_0"] = 0.35 + SGX_BROKERAGE;
  _commish_map_["SGX_AU_1"] = 0.35 + SGX_BROKERAGE;

  _commish_map_["SGX_US_0"] = 0.45 + SGX_BROKERAGE;
  _commish_map_["SGX_US_1"] = 0.45 + SGX_BROKERAGE;

  _commish_map_["SGX_IU_0"] = 0.35 + SGX_BROKERAGE;
  _commish_map_["SGX_IU_1"] = 0.35 + SGX_BROKERAGE;

  _commish_map_["SGX_KU_0"] = 0.35 + SGX_BROKERAGE;
  _commish_map_["SGX_KU_1"] = 0.35 + SGX_BROKERAGE;

  // Removing NK_0 commisions as of now to check performance with rebate
  _commish_map_["SGX_NK_0"] = 0.0 + SGX_NK_LICENSING_FEE + SGX_BROKERAGE;
  _commish_map_["SGX_NK_1"] = 0.5 + SGX_NK_LICENSING_FEE + SGX_BROKERAGE;

  _commish_map_["SP_SGX_NK0_NK1"] = 2 * (0.5 + SGX_NK_LICENSING_FEE + SGX_BROKERAGE);

  _commish_map_["SGX_NKF_0"] = 0.50 + SGX_NK_LICENSING_FEE + SGX_BROKERAGE;
  _commish_map_["SGX_NKF_1"] = 0.50 + SGX_NK_LICENSING_FEE + SGX_BROKERAGE;

  // KRX Commissions
  _commish_map_["KOSPI_0"] =
      828 * CurrencyConvertor::Convert(
                kCurrencyKRW, kCurrencyUSD);  // Notional * 0.065bp%.This includes all the exchange fees and brokerage.
}

double BaseCommish::GetDI1Commish(const std::string& _shortcode_, int _YYYYMMDD_) {
  int t_term_ = std::min(290, SecurityDefinitions::GetDIReserves(_YYYYMMDD_, _shortcode_));
  if (t_term_ > 0) {
    // http://www.bmf.com.br/bmfbovespa/pages/boletim2/pdfs/Exchange-Fee-20150406.pdf
    // exchange_fees = 10^5 * ( (1 + p/100)^(t/252) - 1 ) = 10^3*p*t/252 BRL , bcoz p ~ 0 => (1+p)^x ~ 1+px
    // only 35% for HFT/Day-Trade, assuming a conservative volume tier of 7k to 47k, p = 0.0005049
    // broker_fees = 0.01 BRL
    if (t_term_ > 290) t_term_ = 290;
    return (0.01 + 0.35 * 1000 * (0.0005049 + 0.0004112) * t_term_ / 252) *
           CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  }
  return LARGE_COMMISH;
}

double BaseCommish::GetCommishPerContract(const std::string& _shortcode_, int _YYYYMMDD_) {
  if (commish_map_.empty()) {
    InitCommishMap(commish_map_);
  }
  if (commish_map_.find(_shortcode_) != commish_map_.end()) {
    return commish_map_[_shortcode_];
  } else if (_shortcode_.find("DI1") == 0) {
    commish_map_[_shortcode_] = GetDI1Commish(_shortcode_, _YYYYMMDD_);
    return commish_map_[_shortcode_];
  } else if (_shortcode_.find("NSE") == 0) {
    commish_map_[_shortcode_] = NSESecurityDefinitions::GetNSECommission(_shortcode_);
    return commish_map_[_shortcode_];
  }

  return LARGE_COMMISH;
}

void BaseCommish::SetSgxCNCommishPerContract() {
  if (commish_map_.empty()) {
    InitCommishMap(commish_map_);
  }
  commish_map_["SGX_CN_0"] = -0.40 + SGX_CN_LICENSING_FEE + SGX_BROKERAGE;
  commish_map_["SGX_CN_1"] = -0.40 + SGX_CN_LICENSING_FEE + SGX_BROKERAGE;
}

void BaseCommish::InitRetailCommishMap(CommishMap& _commish_map_) {
  std::vector<std::vector<std::string>> tokens_;
  PerishableStringTokenizer::ParseConfigLines(std::string(RETAIL_COMMISH_FILE), tokens_);
  for (auto i = 0u; i < tokens_.size(); i++) {
    // SHC COMMISH CURR
    // DI1F17 0.396 BRL
    if (tokens_[i].size() >= 3) {
      _commish_map_[tokens_[i][0]] =
          atof(tokens_[i][1].c_str()) *
          CurrencyConvertor::Convert(GetCurrencyFromString(tokens_[i][2].c_str()), kCurrencyUSD);
    }
  }
}

void BaseCommish::LoadDI1RetailCommishTiers(CommishTiers& _commish_tiers_) {
  std::vector<std::vector<std::string>> tokens_;
  PerishableStringTokenizer::ParseConfigLines(std::string(DI1_RETAIL_COMMISH_TIERS_FILE), tokens_);
  _commish_tiers_.clear();
  for (auto i = 0u; i < tokens_.size(); i++) {
    // starting_expiry ending_expiry COMMISH [CURR=BRL]
    // 0 7 0.2 BRL
    if (tokens_[i].size() >= 3) {
      Currency_t t_curr_ = kCurrencyBRL;
      if (tokens_[i].size() >= 4) {
        t_curr_ = GetCurrencyFromString(tokens_[i][3].c_str());
      }
      double t_commish_usd_ = atof(tokens_[i][2].c_str()) * CurrencyConvertor::Convert(t_curr_, kCurrencyUSD);
      _commish_tiers_.emplace_back(atoi(tokens_[i][1].c_str()), t_commish_usd_);
    }
  }
  sort(_commish_tiers_.begin(), _commish_tiers_.end());
}

double BaseCommish::GetDI1RetailCommish(const std::string& _shortcode_, int _YYYYMMDD_) {
  if (di1_retail_commish_tiers_.empty()) {
    LoadDI1RetailCommishTiers(di1_retail_commish_tiers_);
  }
  int t_exp_num_ = CurveUtils::GetExpiryNumber(_shortcode_, _YYYYMMDD_);
  if (t_exp_num_ >= 0) {
    for (auto i = 0u; i < di1_retail_commish_tiers_.size(); i++) {
      if (di1_retail_commish_tiers_[i].first >= t_exp_num_) {
        return di1_retail_commish_tiers_[i].second;
      }
    }
  }
  return LARGE_COMMISH;
}

double BaseCommish::GetRetailCommishPerContract(const std::string& _shortcode_, int _YYYYMMDD_) {
  if (retail_commish_map_.empty()) {
    InitRetailCommishMap(retail_commish_map_);
  }
  if (retail_commish_map_.find(_shortcode_) != retail_commish_map_.end()) {
    return retail_commish_map_[_shortcode_];
  } else if (_shortcode_.find("DI1") == 0) {
    retail_commish_map_[_shortcode_] = GetDI1RetailCommish(_shortcode_, _YYYYMMDD_);
    return retail_commish_map_[_shortcode_];
  }
  return LARGE_COMMISH;
}

// getting commission for a given trade. default values of qty and price are 1 to return commission per contract.
double BaseCommish::GetCommission(const std::string& _shortcode_, int _YYYYMMDD, const double _trade_price_,
                                  const int _qty_, const int volume) {
  HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(_shortcode_, _YYYYMMDD);

  double commission_per_contract = GetCommishPerContract(_shortcode_, _YYYYMMDD);

  if (exch_src_ == kExchSourceBMFEQ) {
    commission_per_contract = _trade_price_ * 0.00028 * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  } else if (exch_src_ == kExchSourceNSE) {
    commission_per_contract *= _trade_price_ * CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD);
  }

  commission_per_contract = SwitchSlab(_shortcode_, commission_per_contract, volume);

  return _qty_ * commission_per_contract;
};

double BaseCommish::SwitchSlab(const std::string& _shortcode_, double commission_per_contract, const int volume) {
  if (volume == 0) return commission_per_contract;

  return commission_per_contract;
}

double BaseCommish::GetBMFEquityCommishPerContract(const std::string& _shortcode_, const double _trade_price_,
                                                   const int _qty_, const int _tier_) {
  if (_tier_ == 1) {
    return (_trade_price_ * _qty_ * (0.00025 + 0.00003)) * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  } else if (_tier_ == 2) {
    return (_trade_price_ * _qty_ * (0.00023 + 0.00003)) * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  } else if (_tier_ == 3) {
    return (_trade_price_ * _qty_ * (0.00020 + 0.00003)) * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  } else if (_tier_ == 4) {
    return (_trade_price_ * _qty_ * (0.00018 + 0.00003)) * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  } else if (_tier_ == 5) {
    return (_trade_price_ * _qty_ * (0.00016 + 0.00003)) * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD);
  }
  return LARGE_COMMISH;
}

double BaseCommish::GetNSECommishPerContract(const std::string& _shortcode_, const double _trade_price_,
                                             const int _qty_) {
  return (_trade_price_ * _qty_ * NSESecurityDefinitions::GetNSECommission(_shortcode_) *
          CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD));
}

double BaseCommish::GetBSECommishPerContract(const std::string& _shortcode_, const double _trade_price_,
                                             const int _qty_) {
  return (_trade_price_ * _qty_ * BSESecurityDefinitions::GetUniqueInstance().GetBSECommission(_shortcode_) *
          CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD));
}

double BaseCommish::GetCommishPerContract(const std::string& _shortcode_, const double _trade_price_, const int _qty_,
                                          const int _tier_) {
  if (_shortcode_.compare("USD000000TOD") == 0 || _shortcode_.compare("USD000UTSTOM") == 0) {
    if (_tier_ == 1)  // 0.011
    {
      return ((0.011 * _trade_price_ * _qty_) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD));
    } else if (_tier_ == 2)  // 0.012
    {
      return ((0.012 * _trade_price_ * _qty_) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD));
    } else if (_tier_ == 3)  // 0.013
    {
      return ((0.013 * _trade_price_ * _qty_) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD));
    } else if (_tier_ == 4)  // 0.014
    {
      return ((0.014 * _trade_price_ * _qty_) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD));
    }
  } else if (_shortcode_.compare("USD000TODTOM") == 0) {
    if (_tier_ == 1)  // 0.0011
    {
      // Calculation : 2 * 0.0011% * 100000 * ( USD/RUB ) * ( RUB/USD )
      return (2 * 1.1 * _qty_);  // approximate using currency file rate instead of current spot price
    } else if (_tier_ == 2)      // 0.0012
    {
      return (2 * 1.2 * _qty_);  // approximate using currency file rate instead of current spot price
    } else if (_tier_ == 3)      // 0.0013
    {
      return (2 * 1.3 * _qty_);  // approximate using currency file rate instead of current spot price
    } else if (_tier_ == 4)      // 0.0014
    {
      return (2 * 1.4 * _qty_);  // approximate using currency file rate instead of current spot price
    }
  } else if ((_shortcode_.compare("EUR_RUB__TOD") == 0) || (_shortcode_.compare("EUR_RUB__TOM") == 0)) {
    if (_tier_ == 1) {
      return ((0.011 * _trade_price_ * _qty_) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD) *
              CurrencyConvertor::Convert(kCurrencyUSD, kCurrencyEUR));
    } else if (_tier_ == 2) {
      return ((0.012 * _trade_price_ * _qty_) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD) *
              CurrencyConvertor::Convert(kCurrencyUSD, kCurrencyEUR));
    } else if (_tier_ == 3) {
      return ((0.013 * _trade_price_ * _qty_) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD) *
              CurrencyConvertor::Convert(kCurrencyUSD, kCurrencyEUR));
    } else if (_tier_ == 4) {
      return ((0.014 * _trade_price_ * _qty_) * CurrencyConvertor::Convert(kCurrencyRUB, kCurrencyUSD) *
              CurrencyConvertor::Convert(kCurrencyUSD, kCurrencyEUR));
    }
  }
  return LARGE_COMMISH;
}
}
