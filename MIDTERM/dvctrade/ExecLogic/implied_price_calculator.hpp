/**
    \file ExecLogic/implied_price_calculator.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_EXECLOGIC_IMPLIED_PRICE_CALCULATOR_H
#define BASE_EXECLOGIC_IMPLIED_PRICE_CALCULATOR_H

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/ExecLogic/implied_price_calculator.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvctrade/Indicators/online_ratio_calculator.hpp"

namespace HFSAT {

enum ImpliedPriceCalculation {
  IPC_USD000000TOD = 1,
  IPC_USD000000TOD1,
  IPC_USD000UTSTOM,
  IPC_USD000UTSTOM1,
  IPC_SGX_NK_0,
  IPC_NIY_0,
  IPC_SGX_IN_0,
  IPC_NSE_NIFTY_FUT0,
  IPC_SGX_IU_0,
  IPC_NSE_USDINR_FUT0,
  IPC_VX_1,
  IPC_SP_VX0_VX1,
  IPC_Si_0,
  IPC_NK_0,
  IPC_NK_0_1,
  IPC_NKM_0,
  IPC_NKM_0_1,
  IPC_TOPIX_0,
  IPC_NKMF_0,
  IPC_HHI_0,
  IPC_HSI_0,
  IPC_BR_DOL_0,
  IPC_BR_WDO_0,
  IPC_BR_IND_0,
  IPC_BR_WIN_0,
  IPC_FDXM_0,
  IPC_DEFAULT
};

class ImpliedPriceCalculator : public virtual IndicatorListener {
 public:
  static std::vector<std::string> GetIndepShortcodesForImpliedPrice(const std::string &r_dep_shortcode_) {
    std::vector<std::string> indep_shc_vec_;
    if (!r_dep_shortcode_.compare("USD000UTSTOM")) {
      indep_shc_vec_.push_back(std::string("USD000000TOD"));
      indep_shc_vec_.push_back(std::string("USD000TODTOM"));
      indep_shc_vec_.push_back(std::string("Si_0"));
    } else if (!r_dep_shortcode_.compare("SGX_NK_0")) {
      indep_shc_vec_.push_back(std::string("NKM_0"));
    } else if (!r_dep_shortcode_.compare("NIY_0")) {
      indep_shc_vec_.push_back(std::string("NKM_0"));
    } else if (!r_dep_shortcode_.compare("FDXM_0")) {
      indep_shc_vec_.push_back(std::string("FDAX_0"));
    } else if (!r_dep_shortcode_.compare("USD000000TOD")) {
      indep_shc_vec_.push_back(std::string("USD000UTSTOM"));
      indep_shc_vec_.push_back(std::string("USD000TODTOM"));
      indep_shc_vec_.push_back(std::string("Si_0"));
    } else if (!r_dep_shortcode_.compare("SGX_IN_0")) {
      indep_shc_vec_.push_back(std::string("NSE_NIFTY_FUT0"));
    } else if (!r_dep_shortcode_.compare("NSE_NIFTY_FUT0")) {
      indep_shc_vec_.push_back(std::string("SGX_IN_0"));
    } else if (!r_dep_shortcode_.compare("SGX_IU_0")) {
      indep_shc_vec_.push_back(std::string("NSE_USDINR_FUT0"));
    } else if (!r_dep_shortcode_.compare("NSE_USDINR_FUT0")) {
      indep_shc_vec_.push_back(std::string("SGX_IU_0"));
    } else if (!r_dep_shortcode_.compare("VX_1")) {
      indep_shc_vec_.push_back(std::string("SP_VX0_VX1"));
      indep_shc_vec_.push_back(std::string("VX_0"));
    } else if (!r_dep_shortcode_.compare("SP_VX0_VX1")) {
      indep_shc_vec_.push_back(std::string("VX_1"));
      indep_shc_vec_.push_back(std::string("VX_0"));
    } else if (!r_dep_shortcode_.compare("Si_0")) {
      indep_shc_vec_.push_back(std::string("USD000UTSTOM"));
    } else if (!r_dep_shortcode_.compare("NK_0")) {
      indep_shc_vec_.push_back(std::string("NIY_0"));
      indep_shc_vec_.push_back(std::string("NKM_0"));
    } else if (!r_dep_shortcode_.compare("NKM_0")) {
      indep_shc_vec_.push_back(std::string("NIY_0"));
      indep_shc_vec_.push_back(std::string("NK_0"));
    } else if (!r_dep_shortcode_.compare("TOPIX_0")) {
      indep_shc_vec_.push_back(std::string("NKM_0"));
    } else if (!r_dep_shortcode_.compare("NKMF_0")) {
      indep_shc_vec_.push_back(std::string("NKM_0"));
    } else if (!r_dep_shortcode_.compare("HHI_0")) {
      indep_shc_vec_.push_back(std::string("HSI_0"));
    } else if (!r_dep_shortcode_.compare("HSI_0")) {
      indep_shc_vec_.push_back(std::string("HHI_0"));
    } else if (!r_dep_shortcode_.compare("BR_DOL_0")) {
      indep_shc_vec_.push_back(std::string("BR_WDO_0"));
    } else if (!r_dep_shortcode_.compare("BR_WDO_0")) {
      indep_shc_vec_.push_back(std::string("BR_DOL_0"));
    } else if (!r_dep_shortcode_.compare("BR_IND_0")) {
      indep_shc_vec_.push_back(std::string("BR_WIN_0"));
    } else if (!r_dep_shortcode_.compare("BR_WIN_0")) {
      indep_shc_vec_.push_back(std::string("BR_IND_0"));
    }
    return indep_shc_vec_;
  }

  // base variables needed by all strategy implementations, and in implementing the interface methods
  DebugLogger &dbglogger_;
  const Watch &watch_;
  const SecurityMarketView &dep_market_view_;
  ;
  ImpliedPriceCalculation implied_price_calculation_;
  double indicator_value_;

  ImpliedPriceCalculator(DebugLogger &_dbglogger_, const Watch &_watch_, const SecurityMarketView &_dep_market_view_,
                         unsigned int ipc_method_ = 0)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        dep_market_view_(_dep_market_view_),
        implied_price_calculation_(IPC_DEFAULT),
        indicator_value_(0.0) {
    if (dep_market_view_.shortcode().compare("USD000000TOD") == 0) {
      implied_price_calculation_ = IPC_USD000000TOD;
      if (ipc_method_ == 1) {
        implied_price_calculation_ = IPC_USD000000TOD1;
        const unsigned int kModelLineBufferLen = 1024;
        char buffer[kModelLineBufferLen];
        bzero(buffer, kModelLineBufferLen);
        std::string this_indicator_string = "INDICATOR 1.00 OnlineRatioCalculator USD000000TOD Si_0 100.0 Midprice";
        strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
        PerishableStringTokenizer st_(buffer, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &ind_tokens = st_.GetTokens();
        OnlineRatioCalculator::GetUniqueInstance(dbglogger_, watch_, ind_tokens, HFSAT::kPriceTypeMidprice)
            ->add_unweighted_indicator_listener(IPC_USD000000TOD1, this);
      }
    } else if (dep_market_view_.shortcode().compare("USD000UTSTOM") == 0) {
      implied_price_calculation_ = IPC_USD000UTSTOM;
      if (ipc_method_ == 1) {
        implied_price_calculation_ = IPC_USD000UTSTOM1;
        const unsigned int kModelLineBufferLen = 1024;
        char buffer[kModelLineBufferLen];
        bzero(buffer, kModelLineBufferLen);
        std::string this_indicator_string = "INDICATOR 1.00 OnlineRatioCalculator USD000UTSTOM Si_0 100.0 Midprice";
        strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
        PerishableStringTokenizer st_(buffer, kModelLineBufferLen);  // Perishable string readline_buffer_
        const std::vector<const char *> &ind_tokens = st_.GetTokens();
        OnlineRatioCalculator::GetUniqueInstance(dbglogger_, watch_, ind_tokens, HFSAT::kPriceTypeMidprice)
            ->add_unweighted_indicator_listener(IPC_USD000000TOD1, this);
      }
    } else if (dep_market_view_.shortcode().compare("SGX_NK_0") == 0) {
      implied_price_calculation_ = IPC_SGX_NK_0;
    } else if (dep_market_view_.shortcode().compare("NIY_0") == 0) {
      implied_price_calculation_ = IPC_NIY_0;
    } else if (dep_market_view_.shortcode().compare("SGX_IN_0") == 0) {
      implied_price_calculation_ = IPC_SGX_IN_0;
    } else if (dep_market_view_.shortcode().compare("NSE_NIFTY_FUT0") == 0) {
      implied_price_calculation_ = IPC_NSE_NIFTY_FUT0;
    } else if (dep_market_view_.shortcode().compare("SGX_IU_0") == 0) {
      implied_price_calculation_ = IPC_SGX_IU_0;
    } else if (dep_market_view_.shortcode().compare("NSE_USDINR_FUT0") == 0) {
      implied_price_calculation_ = IPC_NSE_USDINR_FUT0;
    } else if (dep_market_view_.shortcode().compare("VX_1") == 0) {
      implied_price_calculation_ = IPC_VX_1;
    } else if (dep_market_view_.shortcode().compare("SP_VX0_VX1") == 0) {
      implied_price_calculation_ = IPC_SP_VX0_VX1;
    } else if (dep_market_view_.shortcode().compare("FDXM_0") == 0) {
      implied_price_calculation_ = IPC_FDXM_0;
    } else if (dep_market_view_.shortcode().compare("Si_0") == 0) {
      implied_price_calculation_ = IPC_Si_0;
      const unsigned int kModelLineBufferLen = 1024;
      char buffer[kModelLineBufferLen];
      bzero(buffer, kModelLineBufferLen);
      std::string this_indicator_string = "INDICATOR 1.00 OnlineRatioCalculator Si_0 USD000UTSTOM 100.0 Midprice";
      strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
      PerishableStringTokenizer st_(buffer, kModelLineBufferLen);  // Perishable string readline_buffer_
      const std::vector<const char *> &ind_tokens = st_.GetTokens();
      OnlineRatioCalculator::GetUniqueInstance(dbglogger_, watch_, ind_tokens, HFSAT::kPriceTypeMidprice)
          ->add_unweighted_indicator_listener(IPC_USD000000TOD1, this);
    } else if (dep_market_view_.shortcode().compare("NK_0") == 0) {
      implied_price_calculation_ = IPC_NK_0;
      if (ipc_method_ == 1) {
        implied_price_calculation_ = IPC_NK_0_1;
      }
    } else if (dep_market_view_.shortcode().compare("NKM_0") == 0) {
      implied_price_calculation_ = IPC_NKM_0;
      if (ipc_method_ == 1) {
        implied_price_calculation_ = IPC_NKM_0_1;
      }
    } else if (dep_market_view_.shortcode().compare("TOPIX_0") == 0) {
      implied_price_calculation_ = IPC_TOPIX_0;
      const unsigned int kModelLineBufferLen = 1024;
      char buffer[kModelLineBufferLen];
      bzero(buffer, kModelLineBufferLen);
      std::string this_indicator_string = "INDICATOR 1.00 OnlineRatioCalculator TOPIX_0 NKM_0 100.0 MktSizeWPrice";
      strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
      PerishableStringTokenizer st_(buffer, kModelLineBufferLen);
      const std::vector<const char *> &ind_tokens = st_.GetTokens();
      OnlineRatioCalculator::GetUniqueInstance(dbglogger_, watch_, ind_tokens, HFSAT::kPriceTypeMktSizeWPrice)
          ->add_unweighted_indicator_listener(IPC_TOPIX_0, this);
    } else if (dep_market_view_.shortcode().compare("NKMF_0") == 0) {
      implied_price_calculation_ = IPC_NKMF_0;
      const unsigned int kModelLineBufferLen = 1024;
      char buffer[kModelLineBufferLen];
      bzero(buffer, kModelLineBufferLen);
      std::string this_indicator_string = "INDICATOR 1.00 OnlineRatioCalculator NKMF_0 NKM_0 100.0 MktSizeWPrice";
      strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
      PerishableStringTokenizer st_(buffer, kModelLineBufferLen);
      const std::vector<const char *> &ind_tokens = st_.GetTokens();
      OnlineRatioCalculator::GetUniqueInstance(dbglogger_, watch_, ind_tokens, HFSAT::kPriceTypeMktSizeWPrice)
          ->add_unweighted_indicator_listener(IPC_NKMF_0, this);
    } else if (dep_market_view_.shortcode().compare("HHI_0") == 0) {
      implied_price_calculation_ = IPC_HHI_0;
      const unsigned int kModelLineBufferLen = 1024;
      char buffer[kModelLineBufferLen];
      bzero(buffer, kModelLineBufferLen);
      std::string this_indicator_string = "INDICATOR 1.00 OnlineRatioCalculator HHI_0 HSI_0 100.0 MktSizeWPrice";
      strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
      PerishableStringTokenizer st_(buffer, kModelLineBufferLen);
      const std::vector<const char *> &ind_tokens = st_.GetTokens();
      OnlineRatioCalculator::GetUniqueInstance(dbglogger_, watch_, ind_tokens, HFSAT::kPriceTypeMktSizeWPrice)
          ->add_unweighted_indicator_listener(IPC_HHI_0, this);
    } else if (dep_market_view_.shortcode().compare("HSI_0") == 0) {
      implied_price_calculation_ = IPC_HSI_0;
      const unsigned int kModelLineBufferLen = 1024;
      char buffer[kModelLineBufferLen];
      bzero(buffer, kModelLineBufferLen);
      std::string this_indicator_string = "INDICATOR 1.00 OnlineRatioCalculator HSI_0 HHI_0 100.0 MktSizeWPrice";
      strncpy(buffer, this_indicator_string.c_str(), this_indicator_string.length());
      PerishableStringTokenizer st_(buffer, kModelLineBufferLen);
      const std::vector<const char *> &ind_tokens = st_.GetTokens();
      OnlineRatioCalculator::GetUniqueInstance(dbglogger_, watch_, ind_tokens, HFSAT::kPriceTypeMktSizeWPrice)
          ->add_unweighted_indicator_listener(IPC_HSI_0, this);
    } else if (dep_market_view_.shortcode().compare("BR_DOL_0") == 0) {
      implied_price_calculation_ = IPC_BR_DOL_0;
    } else if (dep_market_view_.shortcode().compare("BR_WDO_0") == 0) {
      implied_price_calculation_ = IPC_BR_WDO_0;
    } else if (dep_market_view_.shortcode().compare("BR_IND_0") == 0) {
      implied_price_calculation_ = IPC_BR_IND_0;
    } else if (dep_market_view_.shortcode().compare("BR_WIN_0") == 0) {
      implied_price_calculation_ = IPC_BR_WIN_0;
    }
  }

  ~ImpliedPriceCalculator() {}

  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_) {
    indicator_value_ = _new_value_;
  }
  void OnIndicatorUpdate(const unsigned int &_indicator_index_, const double &_new_value_decrease_,
                         const double &_new_value_nochange_, const double &_new_value_increase_) {}

  void GetImpliedPrice(const std::vector<SecurityMarketView *> indep_market_view_vec_, double &implied_bid_price_,
                       double &implied_ask_price_, int &implied_bid_size_, int &implied_ask_size_,
                       double &implied_mkt_price_) {
    implied_bid_price_ = -1000;
    implied_ask_price_ = -1000;
    implied_bid_size_ = -1000;
    implied_ask_size_ = -1000;
    implied_mkt_price_ = -1000;

    switch (implied_price_calculation_) {
      case IPC_USD000000TOD: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_ -
                             indep_market_view_vec_[1]->market_update_info_.bestask_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_ -
                             indep_market_view_vec_[1]->market_update_info_.bestbid_price_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_ -
                             indep_market_view_vec_[1]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_USD000000TOD1: {
        implied_bid_price_ = indep_market_view_vec_[2]->market_update_info_.bestbid_price_ * indicator_value_;
        implied_ask_price_ = indep_market_view_vec_[2]->market_update_info_.bestask_price_ * indicator_value_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
        implied_mkt_price_ = indep_market_view_vec_[2]->market_update_info_.mkt_size_weighted_price_ * indicator_value_;
      } break;
      case IPC_USD000UTSTOM: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_ +
                             indep_market_view_vec_[1]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_ +
                             indep_market_view_vec_[1]->market_update_info_.bestask_price_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_ +
                             indep_market_view_vec_[1]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_USD000UTSTOM1: {
        implied_bid_price_ = indep_market_view_vec_[2]->market_update_info_.bestbid_price_ * indicator_value_;
        implied_ask_price_ = indep_market_view_vec_[2]->market_update_info_.bestask_price_ * indicator_value_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
        implied_mkt_price_ = indep_market_view_vec_[2]->market_update_info_.mkt_size_weighted_price_ * indicator_value_;
      } break;
      case IPC_SGX_NK_0: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_NIY_0: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_SGX_IN_0: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_NSE_NIFTY_FUT0: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_SGX_IU_0: {
        implied_bid_price_ = 1 / indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_ask_price_ = 1 / indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
        implied_mkt_price_ = 1 / indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_NSE_USDINR_FUT0: {
        implied_bid_price_ = 1 / indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_ask_price_ = 1 / indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
        implied_mkt_price_ = 1 / indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_VX_1: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_ +
                             indep_market_view_vec_[1]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_ +
                             indep_market_view_vec_[1]->market_update_info_.bestask_price_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_ +
                             indep_market_view_vec_[1]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_SP_VX0_VX1: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_ -
                             indep_market_view_vec_[1]->market_update_info_.bestask_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_ -
                             indep_market_view_vec_[1]->market_update_info_.bestbid_price_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_ -
                             indep_market_view_vec_[1]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_Si_0: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_ * indicator_value_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_ * indicator_value_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_ * indicator_value_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
      } break;
      case IPC_NK_0: {
        // Using NIY
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_ / 2;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_ / 2;
      } break;
      case IPC_NK_0_1: {
        // Using NKM
        implied_bid_price_ = indep_market_view_vec_[1]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[1]->market_update_info_.bestask_price_;
        implied_mkt_price_ = indep_market_view_vec_[1]->market_update_info_.mkt_size_weighted_price_;
        implied_bid_size_ = indep_market_view_vec_[1]->market_update_info_.bestbid_size_ / 10;
        implied_ask_size_ = indep_market_view_vec_[1]->market_update_info_.bestask_size_ / 10;
      } break;
      case IPC_NKM_0: {
        // Using NIY
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_ * 5;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_ * 5;
      } break;
      case IPC_NKM_0_1: {
        // Using NK
        implied_bid_price_ = indep_market_view_vec_[1]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[1]->market_update_info_.bestask_price_;
        implied_mkt_price_ = indep_market_view_vec_[1]->market_update_info_.mkt_size_weighted_price_;
        implied_bid_size_ = indep_market_view_vec_[1]->market_update_info_.bestbid_size_ * 10;
        implied_ask_size_ = indep_market_view_vec_[1]->market_update_info_.bestask_size_ * 10;
      } break;
      case IPC_TOPIX_0: {
        // Using NKM
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_ * indicator_value_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_ * indicator_value_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_ * indicator_value_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
      } break;
      case IPC_NKMF_0: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_ * indicator_value_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_ * indicator_value_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_ * indicator_value_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
      } break;
      case IPC_HHI_0: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_ * indicator_value_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_ * indicator_value_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_ * indicator_value_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
      } break;
      case IPC_HSI_0: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_ * indicator_value_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_ * indicator_value_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_ * indicator_value_;
        implied_bid_size_ = -1000;
        implied_ask_size_ = -1000;
      } break;
      case IPC_FDXM_0: {
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_ * 5;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_ * 5;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
      } break;
      case IPC_BR_DOL_0: {
        // Using BR_WDO_0
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_ / 5;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_ / 5;
      } break;
      case IPC_BR_WDO_0: {
        // Using BR_DOL_0
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_ * 5;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_ * 5;
      } break;
      case IPC_BR_IND_0: {
        // Using BR_WIN_0
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_ / 5;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_ / 5;
      } break;
      case IPC_BR_WIN_0: {
        // Using BR_IND_0
        implied_bid_price_ = indep_market_view_vec_[0]->market_update_info_.bestbid_price_;
        implied_ask_price_ = indep_market_view_vec_[0]->market_update_info_.bestask_price_;
        implied_mkt_price_ = indep_market_view_vec_[0]->market_update_info_.mkt_size_weighted_price_;
        implied_bid_size_ = indep_market_view_vec_[0]->market_update_info_.bestbid_size_ * 5;
        implied_ask_size_ = indep_market_view_vec_[0]->market_update_info_.bestask_size_ * 5;
      } break;
      default: { } break; }
  }
};
}
#endif  // BASE_EXECLOGIC_IMPLIED_PRICE_CALCULATOR_H
