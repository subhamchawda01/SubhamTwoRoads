#ifndef MT_SPRD_EXEC_LOGIC
#define MT_SPRD_EXEC_LOGIC

#include <sstream>

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "dvccode/Utils/slack_utils.hpp"
#include "dvctrade/InitCommon/nse_spread_logic_param.hpp"

namespace MT_SPRD {
class SpreadExecLogic : public HFSAT::SecurityMarketViewChangeListener,
                        public HFSAT::PositionChangeListener,
                        public HFSAT::ExecutionListener {
 public:
  SpreadExecLogic(HFSAT::DebugLogger& dbglogger_, HFSAT::BulkFileWriter& trades_writer_, HFSAT::Watch& _watch_,
                  const int _sec_id_pass_, const int _sec_id_agg_, HFSAT::SecurityMarketView& _smv_pass_,
                  HFSAT::SecurityMarketView& _smv_agg_, HFSAT::SmartOrderManager& _om_pass_,
                  HFSAT::SmartOrderManager& _om_agg_, const int _overnight_pos_pass_, const int _overnight_pos_agg_,
                  ParamSet* t_param_, bool t_live, bool t_is_banned);

  virtual ~SpreadExecLogic(){};
  // SMV listener calls
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_infor_,
                    const HFSAT::MarketUpdateInfo& _market_update_info_){};

  // OM listener calls
  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_);
  void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
              const double _price_, const int r_int_price_, const int _security_id_);

  // spread_trader calls
  void SetDesiredPositions(const int _pos_pass_leg_, const int _pos_agg_leg_, const double _tgt_sprd_,
                           const double _sprd_alpha_, const double _sprd_beta_) {
    DBGLOG_TIME_CLASS_FUNC << " desired pos " << shortcode_pass_ << ' ' << _pos_pass_leg_ << " --- " << shortcode_agg_
                           << ' ' << _pos_agg_leg_ << '\n';
    desired_pos_pass_ = _pos_pass_leg_;
    desired_pos_agg_ = _pos_agg_leg_;
    target_spread_ = _tgt_sprd_;
    spread_alpha_ = _sprd_alpha_;
    spread_beta_ = _sprd_beta_;
    OrderManagement();
  };

  void SetAllVariables(int _t_des_pos_pass_, int _t_des_pos_agg_, double _t_tgt_sprd_, double _t_sprd_alpha_,
                       double _t_sprd_beta_, int _t_pos_pass_leg_, int _t_pos_agg_leg_, double _t_hedge_ratio_) {
    desired_pos_pass_ = _t_des_pos_pass_;
    desired_pos_agg_ = _t_des_pos_agg_;
    target_spread_ = _t_tgt_sprd_;
    spread_alpha_ = _t_sprd_alpha_;
    spread_beta_ = _t_sprd_beta_;
    overnight_position_pass_ = _t_pos_pass_leg_;
    overnight_position_agg_ = _t_pos_agg_leg_;
    position_pass_ = _t_pos_pass_leg_;
    position_agg_ = _t_pos_agg_leg_;
    hedge_ratio_ = _t_hedge_ratio_;
  };

  void GetAllVariables(int& _t_des_pos_pass_, int& _t_des_pos_agg_, double& _t_tgt_sprd_, double& _t_sprd_alpha_,
                       double& _t_sprd_beta_, int& _t_pos_pass_leg_, int& _t_pos_agg_leg_, double& _t_hedge_ratio_) {
    _t_des_pos_pass_ = desired_pos_pass_;
    _t_des_pos_agg_ = desired_pos_agg_;
    _t_tgt_sprd_ = target_spread_;
    _t_sprd_alpha_ = spread_alpha_;
    _t_sprd_beta_ = spread_beta_;
    _t_pos_pass_leg_ = position_pass_;
    _t_pos_agg_leg_ = position_agg_;
    _t_hedge_ratio_ = hedge_ratio_;
  };

  void SetOMPositions(const int _pos_pass_leg_, const int _pos_agg_leg_) {
    overnight_position_pass_ = _pos_pass_leg_;
    overnight_position_agg_ = _pos_agg_leg_;
    position_pass_ = _pos_pass_leg_;
    position_agg_ = _pos_agg_leg_;
    slack_pos_pass = _pos_pass_leg_ + 5;
    slack_pos_agg = _pos_agg_leg_ + 5;
  };

  void SetStdevValues(const double _stdev_pass_leg_, const double _stdev_agg_leg_) {
    stdev_pass_ = _stdev_pass_leg_;
    stdev_agg_ = _stdev_agg_leg_;
  };

  void GetOMPositions(int& _pass_position_, int& _agg_position_) {
    _pass_position_ = position_pass_;
    _agg_position_ = position_agg_;
  };

  bool HasPosition() {
    bool ret_val = (position_pass_ != 0 || position_agg_ != 0);
    return ret_val;
  }

  void UpdateSpreadParameters(double _sprd_alpha_, double _sprd_beta_) {
    spread_alpha_ = _sprd_alpha_;
    spread_beta_ = _sprd_beta_;
  };

  void UpdateHedgeRatio(double _hedge_ratio_) { hedge_ratio_ = _hedge_ratio_; };

  void SetGetflatMode(bool _t_mode_) { getflat_mode_ = _t_mode_; };

  // on data end - trigger trade for open position
  void OnAllEventsConsumed();

 private:
  // watch
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::BulkFileWriter& tradesfile_;
  HFSAT::Watch& watch_;

  // security id for agg and pass legs
  unsigned int sec_id_pass_;
  unsigned int sec_id_agg_;
  std::string shortcode_pass_;
  std::string shortcode_agg_;

  // security market views
  HFSAT::SecurityMarketView& mkt_view_pass_;
  HFSAT::SecurityMarketView& mkt_view_agg_;

  // order managers
  HFSAT::SmartOrderManager& om_pass_;
  HFSAT::SmartOrderManager& om_agg_;

  // overnight positioons - need to be set to invoking class
  int overnight_position_pass_;
  int overnight_position_agg_;

  /// bid and ask prices for two legs
  double best_bid_price_pass_leg_;
  double best_ask_price_pass_leg_;
  int best_bid_int_price_pass_leg_;
  int best_ask_int_price_pass_leg_;
  double best_bid_price_agg_leg_;
  double best_ask_price_agg_leg_;
  int best_bid_int_price_agg_leg_;
  int best_ask_int_price_agg_leg_;

  // target prices
  int bid_int_price_to_place_at_pass_leg_;
  int ask_int_price_to_place_at_pass_leg_;
  int bid_int_price_to_place_at_agg_leg_;
  int ask_int_price_to_place_at_agg_leg_;

  // time of placing/modifying orders
  uint64_t time_bid_order_placed_pass_leg_;
  uint64_t time_ask_order_placed_pass_leg_;
  uint64_t time_bid_order_placed_agg_leg_;
  uint64_t time_ask_order_placed_agg_leg_;

  // price stdev values - for use in changing order price
  double stdev_pass_;
  double stdev_agg_;

  // current positions in lots of instrument 1/2
  int position_pass_;
  int position_agg_;

  // desired positions in lots - set by spread_trader
  // test with anticipatory position - TODO
  int desired_pos_pass_;
  int desired_pos_agg_;

  // parameters of spread algo -- some of these might be irrelevant for
  // specific asset/spread comp modes
  double target_spread_;  // semantic interpretation of field depends on desired pos value
  double spread_alpha_;
  double spread_beta_;

  // parameters to limit frequency with which order placing function is called
  uint64_t last_order_handling_call_time_;
  double last_order_handling_pass_leg_midpx_;
  double last_order_handling_agg_leg_midpx_;

  // hedge ratio - fractional lots of agg leg equivalent to one lot of pass leg
  double hedge_ratio_;

  // getflat mode check - used to overwrite target checks in decisions to place
  // orders - needed for rollover mainly
  bool getflat_mode_;

  // is ready check - to prevent order placing before mkt books are ready
  bool is_ready_;

  ParamSet* param_;

  bool live;

  bool is_banned;

  int slack_pos_pass;
  int slack_pos_agg;

  // Order Management functions
  void OrderManagement();
  void PlaceSingleBuyOrder(HFSAT::SmartOrderManager& om_, int order_px_, uint64_t& time_order_placed_, bool is_pass_);
  void PlaceSingleSellOrder(HFSAT::SmartOrderManager& om_, int order_px_, uint64_t& time_order_placed_, bool is_pass_);

  // for passive leg this returns true if passive fill followed by agg fill on other
  // leg meets spread criteria
  bool PriceCheckPassed();
};
}
#endif
