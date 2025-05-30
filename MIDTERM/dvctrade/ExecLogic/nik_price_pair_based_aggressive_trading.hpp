/**
    \file ExecLogic/nik_price_pair_based_aggressive_trading.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_EXECLOGIC_NIK_PRICE_PAIR_BASED_AGGRESSIVE_TRADING_H
#define BASE_EXECLOGIC_NIK_PRICE_PAIR_BASED_AGGRESSIVE_TRADING_H

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/nik_trading_manager.hpp"

namespace HFSAT {

/// Class to trade with a target price that is very dependant on top level nonself marketprice
///
/// the base price moves sharply with market price and so does the target price
/// the targetprice is thus compared to bid price and ask price ( for getting an opinion to place trades )
/// and not midprice or a price that is much more stable than market weighted price
class NikPricePairBasedAggressiveTrading : public virtual BaseTrading, public NikTradingManagerListener {
 protected:
  NikTradingManager* niktm_;
  unsigned int last_pair_bid_cancel_msecs_;
  unsigned int last_pair_ask_cancel_msecs_;

 public:
  NikPricePairBasedAggressiveTrading(
      DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
      SmartOrderManager& _order_manager_, const std::string& _paramfilename_, const bool _livetrading_,
      MulticastSenderSocket* _p_strategy_param_sender_socket_, EconomicEventsManager& t_economic_events_manager_,
      const int t_trading_start_utc_mfm_, const int t_trading_end_utc_mfm_, const int t_runtime_id_,
      const std::vector<std::string> _this_model_source_shortcode_vec_, NikTradingManager* niktm_);

  ~NikPricePairBasedAggressiveTrading(){};  ///< destructor not made virtual ... please do so when making child classes

  static std::string StrategyName() {
    return "NikPricePairBasedAggressiveTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  void OnTimePeriodUpdate(const int num_pages_to_add_);

  inline void OnControlUpdate(const HFSAT::ControlMessage& _control_message_, const char* symbol, int trader_id) {
    BaseTrading::OnControlUpdate(_control_message_, symbol, trader_id);
  }

  inline void SetModelMathComponent(BaseModelMath* t_base_model_math_) {
    p_base_model_math_ = t_base_model_math_;
    for (auto i = 0u; i < param_set_vec_.size(); i++) {
      if (param_set_vec_[i].read_own_base_px_) {
        p_base_model_math_->SetOwnBasePx(StringToPriceType_t(param_set_vec_[i].own_base_px_));
      }
    }
  }
  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_);

 protected:
  void UpdatePairParams(ParamSet& _param_set_);

  void TradingLogic();  ///< All the strategy based trade execution is written here

  void OrderPlacingLogic();

  void PrintFullStatus();

  inline void UpdateCombinedPosition(int _new_combined_position_) { my_combined_position_ = _new_combined_position_; }

  ///< Function to map position to map_index and then the map_index is used to load the
  void TradeVarSetLogic(int t_position);

  /// current_tradevarset_

  void OnExec(const int t_new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);

  bool ShouldBeGettingFlat();

  void GetFlatTradingLogic();  ///< Trade Selection and execution during getflat mode

  inline void CancelBestBid() {
    top_bid_keep_ = false;
    top_bid_place_ = false;
    last_pair_bid_cancel_msecs_ = watch_.msecs_from_midnight();
    OrderPlacingLogic();
  }

  inline void CancelBestAsk() {
    top_ask_keep_ = false;
    top_ask_place_ = false;
    last_pair_ask_cancel_msecs_ = watch_.msecs_from_midnight();
    OrderPlacingLogic();
  }

  inline void AggressiveBuy() {
    top_ask_lift_ = true;
    OrderPlacingLogic();
  }

  inline void AggressiveSell() {
    top_bid_hit_ = true;
    OrderPlacingLogic();
  }

  inline SmartOrderManager& order_manager() { return order_manager_; }
};
}
#endif  // BASE_EXECLOGIC_NIK_PRICE_PAIR_BASED_AGGRESSIVE_TRADING_H
