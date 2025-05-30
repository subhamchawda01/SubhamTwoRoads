/**
    \file SmartOrderRouting/prom_pnl.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_SMARTORDERROUTING_PROM_PNL_H
#define BASE_SMARTORDERROUTING_PROM_PNL_H

#include <queue>

#include "dvccode/Utils/bulk_file_writer.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {

/// \brief General class to compute PNL listening to L1 MarketData Updates and OrderRouting Updates ( Execution Updates
/// )
class PromPNL : public GlobalOrderExecListener, public SecurityMarketViewChangeListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  PromOrderManager& prom_order_manager_;
  SecurityMarketView& dep_market_view_;
  int runtime_id_;
  BulkFileWriter& trades_writer_;

  char numbered_secname_[24];  ///< secname.runtime_id_ for pnl file

  double pnl_;  ///< without accounting for position ... cash accouting
  double previous_pnl;
  double realized_pnl_;  ///< the pnl recorded when the last time we were flat or we changed signs of position
  double opentrade_unrealized_pnl_;  ///< the unrealized pnl of this trade
  double total_pnl_;  ///< realized_pnl_ + opentrade_unrealized_pnl_ ( of this trade ) ... best estimate of current pnl
  double last_closing_trade_pnl_;  ///< the pnl of only the last closed trade
  int position_;                   ///< current position
  double commish_dollars_per_unit_;
  int di_reserves_;
  double numbers_to_dollars_;

  double mid_price_;
  std::queue<std::pair<double, int> > pnl_time_queue;

 public:
  PromPNL(DebugLogger& t_dbglogger_, const Watch& r_watch_, PromOrderManager& t_prom_order_manager_,
          SecurityMarketView& t_dep_market_view_, int t_runtime_id_, BulkFileWriter& t_trades_writer_);

  ~PromPNL();

  void OnGlobalOrderExec(const unsigned int security_id_, const TradeType_t _buysell_, const int _size_,
                         const double _trade_px_);
  double get_pnl_diff(int time_diff);

  /// @brief Update opentrade_unrealized_pnl_ based on market change
  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
    mid_price_ = ( _market_update_info_.mid_price_ < 0) ? mid_price_ :  _market_update_info_.mid_price_;

    if( _market_update_info_.bestbid_price_ < 0 || _market_update_info_.bestask_price_ < 0 || mid_price_ < 0 ) return;
    if (dep_market_view_.shortcode().find("DI1") != std::string::npos)  // BR_DI_1 we have a generic_ticker
    {
      numbers_to_dollars_ = -GetDIContractNumbersToDollars();
    }

    total_pnl_ = pnl_ + (position_ * mid_price_ * numbers_to_dollars_);
    pnl_time_queue.push(std::make_pair(total_pnl_, watch_.msecs_from_midnight()));
    opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;
  }

  inline double GetDIContractNumbersToDollars() {
    double unit_price_ = 0;
    double term_ = double(di_reserves_ / 252.0);
    if (term_ > 0.000) {
      unit_price_ = 100000 / std::pow((mid_price_ / 100 + 1), term_);
    }
    return (unit_price_ * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD) / mid_price_);
  }

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline double pnl() const { return pnl_; }
  inline double realized_pnl() const { return realized_pnl_; }
  inline double opentrade_unrealized_pnl() const { return opentrade_unrealized_pnl_; }
  inline double total_pnl() const { return total_pnl_; }
  inline double last_closing_trade_pnl() const { return last_closing_trade_pnl_; }
  inline int position() const { return position_; }
  inline double commish_dollars_per_unit() const { return commish_dollars_per_unit_; }
  inline double numbers_to_dollars() const { return numbers_to_dollars_; }
  inline double min_price() const { return mid_price_; }
  inline std::string get_shortcode(){ return dep_market_view_.shortcode();}
};
}

#endif  // BASE_ORDERROUTING_PROM_PNL_H
