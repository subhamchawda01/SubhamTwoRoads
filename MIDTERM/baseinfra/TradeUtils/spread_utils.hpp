/*
 * spread_utils.hpp
 *
 *  Created on: Mar 5, 2015
 *      Author: archit
 */

#ifndef UTILS_SPREAD_UTILS_HPP_
#define UTILS_SPREAD_UTILS_HPP_

#include <iostream>
#include <fstream>
#include <strings.h>
#include <vector>

#include "dvccode/CDef/security_definitions.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

namespace HFSAT {

struct TradeInfoStruct {
  char contract_[24];
  int security_id_;
  double trd_px_;
  unsigned int trd_qty_;
  TradeType_t buysell_;

  inline std::string ToString() const {
    std::ostringstream temp_oss_;
    temp_oss_ << "Contract: " << contract_ << " SID: " << security_id_ << " Trd_Px: " << trd_px_
              << " Trd_Qty: " << trd_qty_ << " BuySell: " << GetTradeTypeChar(buysell_);
    return temp_oss_.str();
  }
};

class SpreadUtils {
 public:
  static bool GetLegTradesFromSpreadTrade(const char *_spd_secname_, double _spd_trd_px_, unsigned int _spd_trd_qty_,
                                          TradeType_t _spd_buysell_, int _YYYYMMDD_, DebugLogger &dbglogger_,
                                          std::vector<TradeInfoStruct> &retval_);
  static bool GetLegTradesFromSpreadTrade(const SecurityMarketView *_smv_long_maturity_leg_,
                                          const SecurityMarketView *_smv_short_maturity_leg_, double _spd_trd_px_,
                                          unsigned int _spd_trd_qty_, TradeType_t _spd_buysell_, int _YYYYMMDD_,
                                          DebugLogger &dbglogger_, std::vector<TradeInfoStruct> &retval_);
  static bool GetLegTradesFromFlyTrade(const std::vector<const SecurityMarketView *> &_p_smv_vec_,
                                       const std::vector<double> &_size_factor_vec_,
                                       const std::vector<double> &_price_factor_vec_, double _fly_trd_px_,
                                       unsigned int _fly_trd_qty_, TradeType_t _fly_buysell_, DebugLogger &dbglogger_,
                                       std::vector<TradeInfoStruct> &retval_);

  static void GetSpreadSecname(const std::vector<const SecurityMarketView *> &_p_smv_vec_, std::string &_spd_secname_);
};

} /* namespace HFSAT */

#endif /* UTILS_SPREAD_UTILS_HPP_ */
