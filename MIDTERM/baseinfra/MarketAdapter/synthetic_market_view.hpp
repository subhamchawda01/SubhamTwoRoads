/*
 * spread_market_view.hpp
 *
 *  Created on: 05-Jun-2014
 *      Author: archit
 */

#ifndef SYNTHETIC_MARKET_VIEW_HPP_
#define SYNTHETIC_MARKET_VIEW_HPP_

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/MarketAdapter/book_interface.hpp"
#include "dvccode/Utils/synthetic_security_manager.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "dvccode/CDef/math_utils.hpp"

namespace HFSAT {

class SyntheticMarketView : public SecurityMarketViewChangeListener, public TimePeriodListener {
 protected:
  DebugLogger &dbglogger_;
  const Watch &watch_;
  std::string spread_shc_;
  std::vector<std::string> shortcode_vec_;
  std::vector<double> weights_vec_;
  std::vector<SecurityMarketView *> smv_vec_;
  SecurityMarketView *synth_smv_;
  std::map<unsigned int, int> secid_to_idx_map_;
  std::vector<bool> ready_vec_;
  bool is_ready_;
  bool is_dv01_not_updated_;
  bool is_di_spread_;

  double bestbid_price_;
  double bestask_price_;
  unsigned int bestbid_size_;
  unsigned int bestask_size_;

  std::vector<double> last_bestbid_price_;
  std::vector<double> last_bestask_price_;
  std::vector<int> last_bestbid_size_;
  std::vector<int> last_bestask_size_;
  std::vector<double> dv01_vec_;
  int tradingdate_;

  void InitializeBestVars();

  void NotifyListeners();

 public:
  virtual ~SyntheticMarketView() {}

  SyntheticMarketView(DebugLogger &_dbglogger_, const Watch &_watch_, const std::string &_spread_shc_);

  inline std::string shortcode() const { return spread_shc_; }
  inline bool is_ready() const { return is_ready_; }

  // BookInterface

  void ShowBook() const;
  void PrintBook() const;

  // SecurityMarketViewChangeListener
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                    const MarketUpdateInfo &_market_update_info_);

  void OnTimePeriodUpdate(const int num_pages_to_add_);
};
} /* namespace HFSAT */

#endif /* SYNTHETIC_MARKET_VIEW_HPP_ */
