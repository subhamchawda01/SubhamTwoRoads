/*
 * spread_trading_manager.hpp
 *
 *  Created on: 12-May-2014
 *      Author: archit
 */

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvctrade/ExecLogic/trading_manager.hpp"
#include "baseinfra/MarketAdapter/shortcode_spread_market_view_map.hpp"

#ifndef SPREAD_TRADING_MANAGER_HPP_
#define SPREAD_TRADING_MANAGER_HPP_

namespace HFSAT {
class SpreadTradingManagerListener {
 public:
  virtual ~SpreadTradingManagerListener() {}
  virtual void OnPositionUpdate(int _ideal_spread_position_) = 0;
  virtual unsigned int GetSecId() = 0;
  virtual const SmartOrderManager &order_manager() = 0;
};

class SpreadTradingManager : public TradingManager {
 protected:
  const SpreadMarketView &spread_market_view_;

  std::vector<std::string> shortcode_vec_;
  std::vector<int> uts_vec_;
  std::map<unsigned int, int> secid_to_idx_map_;

  std::vector<SpreadTradingManagerListener *> stm_listeners_;

 public:
  SpreadTradingManager(DebugLogger &_dbglogger_, const Watch &_watch_, const SpreadMarketView &_spread_market_view_);

  virtual ~SpreadTradingManager() {}

  bool AddStmListener(SpreadTradingManagerListener *_listener_);
  void OnPositionUpdate(unsigned int _securit_id_, int _new_position_);
  void ReportResults(HFSAT::BulkFileWriter &trades_writer_);

  inline const SpreadMarketView &GetSpreadMarketView() const { return spread_market_view_; }
};

} /* namespace HFSAT */

#endif /* SPREAD_TRADING_MANAGER_HPP_ */
