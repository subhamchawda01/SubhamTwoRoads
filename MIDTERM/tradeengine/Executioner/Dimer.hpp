#ifndef _EXECUTIONER_DIMER_HPP
#define _EXECUTIONER_DIMER_HPP

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvccode/CDef/math_utils.hpp"
#include "tradeengine/Executioner/BaseExecutioner.hpp"

class Dimer : public BaseExecutioner, public HFSAT::ThrottleNumberListener {
 private:
  std::string identifier_;
  int max_depth_;

  double bid_percentage_offset_;
  double ask_percentage_offset_;
  double bid_offset_;
  double ask_offset_;
  int dimer_bid_size_;
  int dimer_ask_size_;

  int max_bid_join_size_;
  int max_ask_join_size_;
  int min_bid_dime_size_;
  int min_ask_dime_size_;

  bool agg_sqoff_fav_invalid_;
  double max_bid_offset_;
  double max_ask_offset_;
  double max_bid_offset_update_;
  double max_ask_offset_update_;
  int max_bid_offset_int_;
  int max_ask_offset_int_;
  int max_bid_offset_update_int_;
  int max_ask_offset_update_int_;

  /*double max_bid_offset_nojoindime_;
  double max_ask_offset_nojoindime_;
  double max_bid_offset_nojoindime_update_;
  double max_ask_offset_nojoindime_update_;*/

  bool bid_overruled_;
  bool ask_overruled_;
  double overruled_leaway_percent_;
  double overruled_leaway_offset_;

  int delay_between_orders_;

  int drag_tighten_delay_;
  int config_drag_tighten_delay_;
  int min_drag_tighten_delay_;
  int max_drag_tighten_delay_;
  int drag_widen_delay_;
  int order_timeout_;

  bool use_spread_threshold_;
  double spread_threshold_;

  bool passive_only_;
  bool allow_orders_beyond_maxoffset_;

  double dimer_bid_price_;
  int dimer_bid_int_price_;
  double dimer_ask_price_;
  int dimer_ask_int_price_;

  double previous_bid_price_;
  int previous_bid_int_price_;
  double previous_ask_price_;
  int previous_ask_int_price_;

  bool bid_order_at_maxoffset_;
  bool ask_order_at_maxoffset_;

  int last_new_bid_order_time_;
  int last_modify_bid_order_time_;
  int last_new_ask_order_time_;
  int last_modify_ask_order_time_;

  int price_upper_limit_;
  int price_lower_limit_;
  bool cancel_on_maxoffset_;
  bool unselective_if_maxoffset_;
  int cancel_on_sweep_;

  double safe_order_offset_percent_;  // Offset from order range limit at which we will keep safe orders in case of big
                                      // trade
  int safe_order_offset_int_;
  bool dime_two_ticks_;

  HFSAT::BaseOrder* bid_order_;
  HFSAT::BaseOrder* ask_order_;

  bool disable_one_percent_order_limit_;
  double order_to_trade_ratio_percent_;
  double order_to_trade_ratio_percent_upper_multiplier_;
  double order_to_trade_ratio_percent_lower_multiplier_;

  int min_levels_to_clear_for_cancel_on_big_trade_;

  void LoadParams();
  void UpdateOrders();
  void UpdateBidOrders();
  void UpdateAskOrders();
  void SendBidOrder(int current_order_bid_int_price_);
  void SendAskOrder(int current_order_bid_int_price_);

 public:
  Dimer(std::string _exec_param_file, HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_,
        HFSAT::SecurityMarketView* _secondary_smv, HFSAT::BasicOrderManager* _base_om, bool _livetrading_,
        bool _is_modify_before_confirmation_, bool _is_cancellable_before_confirmation_, TheoValues& theo_values);
  ~Dimer() {}

  void OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnTheoUpdate();
  void OnOrderChange(HFSAT::BaseOrder* _order_);
  void OnOrderReject(HFSAT::BaseOrder* _order_);
  void CheckOrderStatus(HFSAT::BaseOrder* _order_);
  void DisableBid();
  void CancelBid();
  void CancelAsk();
  void DisableAsk();
  std::string GetOrderCountString(){
      return identifier_ + " " + std::to_string(order_count_) + " ";
  }
  void SetMaxOffset(double max_offset_bid, double max_offset_ask, bool _agg_sqoff_fav_invalid_ = false) {
    max_bid_offset_ = max_offset_bid;
    max_ask_offset_ = max_offset_ask;
    max_bid_offset_int_ = HFSAT::MathUtils::RoundOff(double(max_bid_offset_) * double(inverse_tick_size_));
    max_ask_offset_int_ = HFSAT::MathUtils::RoundOff(double(max_ask_offset_) * double(inverse_tick_size_));
    agg_sqoff_fav_invalid_ = _agg_sqoff_fav_invalid_;
    /*DBGLOG_TIME_CLASS_FUNC << " setting max offset to " << max_offset_bid
            << " ask " << max_offset_ask << " int " << max_bid_offset_int_
            << " "  << max_ask_offset_int_ << " inv_tick_size "
            << inverse_tick_size_ << DBGLOG_ENDL_FLUSH;*/
  }

  void OnThrottleChange(int _num_throttle_, const unsigned int security_id_) {
    if (min_throttles_per_min_ > _num_throttle_) {
      drag_tighten_delay_ = std::max((int)(drag_tighten_delay_ * 0.5), min_drag_tighten_delay_);
      //			DBGLOG_TIME_CLASS_FUNC << identifier_ << " MIN THROTTLE HIT NUM_THROTTLE: " <<
      //_num_throttle_
      //					<< " NEW TIGHTEN DELAY: " <<  drag_tighten_delay_ << DBGLOG_ENDL_FLUSH;
    } else if (max_throttles_per_min_ < _num_throttle_) {
      drag_tighten_delay_ = std::min(drag_tighten_delay_ * 2 + 1, max_drag_tighten_delay_);
      DBGLOG_TIME_CLASS_FUNC << identifier_ << " MAX THROTTLE HIT NUM_THROTTLE: " << _num_throttle_
                             << " NEW TIGHTEN DELAY: " << drag_tighten_delay_ << DBGLOG_ENDL_FLUSH;
    }
  }

  void OnLargeDirectionalTrades(const HFSAT::TradeType_t t_trade_type_, const int t_min_price_levels_cleared_,
                                const bool is_valid_book_end_) {
    if (t_min_price_levels_cleared_ >= min_levels_to_clear_for_cancel_on_big_trade_) {
      if (t_trade_type_ == HFSAT::TradeType_t::kTradeTypeSell) {
        if (bid_order_) {
          int current_order_bid_int_price_ = secondary_smv_->lower_int_price_limit_;
          int safe_dimer_price_int_ = current_order_bid_int_price_ + safe_order_offset_int_;
          if ((safe_dimer_price_int_ <= dimer_bid_int_price_) && (theo_values_.is_valid_)) {
            if (bid_order_->int_price_ != current_order_bid_int_price_) {
              double current_order_bid_price_ = current_order_bid_int_price_ * tick_size_;
              bool is_modified_ = basic_om_->Modify(bid_order_, current_order_bid_price_, current_order_bid_int_price_,
                                                    dimer_bid_size_, use_reserve_msg_);
              if (is_modified_) {
                /*								dbglogger_ << watch_.tv() << " "
                                                                                        << sec_shortcode_ << " " <<
                   identifier_
                                                                                        << "SEND MODIFYORDER BUY " <<
                   current_order_bid_price_
                                                                                        << " size: " << dimer_bid_size_
                   << " intpx: "
                                                                                        << current_order_bid_int_price_
                   << " theo["
                                                                                        << theo_values_.theo_bid_price_
                   << " X " << theo_values_.theo_ask_price_ << "]"
                                                                                        << " mkt[" <<
                   secondary_smv_->market_update_info().bestbid_price_
                                                                                        << " X " <<
                   secondary_smv_->market_update_info().bestask_price_ << "]"
                                                                                        << " dimerintpx " <<
                   dimer_bid_int_price_ << DBGLOG_ENDL_FLUSH;*/
                previous_bid_price_ = current_order_bid_price_;
                previous_bid_int_price_ = current_order_bid_int_price_;
                last_modify_bid_order_time_ = watch_.msecs_from_midnight();
                order_count_++;
              }
            }
          } else {
            /*						dbglogger_ << watch_.tv() << " "
                                                                    << sec_shortcode_ << " " << identifier_
                                                                    << "SEND CANCEL BUY " <<
               current_order_bid_int_price_
                                                                    << " size: " << dimer_bid_size_ << " intpx: "
                                                                    << current_order_bid_int_price_ << " theo["
                                                                    << theo_values_.theo_bid_price_ << " X " <<
               theo_values_.theo_ask_price_ << "]"
                                                                    << " mkt[" <<
               secondary_smv_->market_update_info().bestbid_price_
                                                                    << " X " <<
               secondary_smv_->market_update_info().bestask_price_ << "]"
                                                                    << " dimerintpx " << dimer_bid_int_price_ <<
               DBGLOG_ENDL_FLUSH;*/
            basic_om_->Cancel(*bid_order_, use_reserve_msg_);
          }
        }
      } else {
        if (ask_order_) {
          int current_order_ask_int_price_ = secondary_smv_->upper_int_price_limit_;
          int safe_dimer_price_int_ = current_order_ask_int_price_ - safe_order_offset_int_;
          if ((safe_dimer_price_int_ >= dimer_ask_int_price_) && (theo_values_.is_valid_)) {
            if (ask_order_->int_price_ != current_order_ask_int_price_) {
              double current_order_ask_price_ = current_order_ask_int_price_ * tick_size_;
              bool is_modified_ = basic_om_->Modify(ask_order_, current_order_ask_price_, current_order_ask_int_price_,
                                                    dimer_ask_size_, use_reserve_msg_);
              if (is_modified_) {
                /*								dbglogger_ << watch_.tv() << " "
                                                                                        << sec_shortcode_ << " " <<
                   identifier_
                                                                                        << "SEND MODIFYORDER SELL " <<
                   current_order_ask_price_
                                                                                        << " size: " << dimer_ask_size_
                   << " intpx: "
                                                                                        << current_order_ask_int_price_
                   << " theo["
                                                                                        << theo_values_.theo_ask_price_
                   << " X " << theo_values_.theo_ask_price_ << "]"
                                                                                        << " mkt[" <<
                   secondary_smv_->market_update_info().bestbid_price_
                                                                                        << " X " <<
                   secondary_smv_->market_update_info().bestask_price_ << "]"
                                                                                        << " dimerintpx " <<
                   dimer_ask_int_price_ << DBGLOG_ENDL_FLUSH;*/
                previous_ask_price_ = current_order_ask_price_;
                previous_ask_int_price_ = current_order_ask_int_price_;
                last_modify_ask_order_time_ = watch_.msecs_from_midnight();
                order_count_++;
              }
            }
          } else {
            /*						dbglogger_ << watch_.tv() << " "
                                                                    << sec_shortcode_ << " " << identifier_
                                                                    << "SEND CANCEL SELL " <<
               current_order_ask_int_price_
                                                                    << " size: " << dimer_ask_size_ << " intpx: "
                                                                    << current_order_ask_int_price_ << " theo["
                                                                    << theo_values_.theo_ask_price_ << " X " <<
               theo_values_.theo_ask_price_ << "]"
                                                                    << " mkt[" <<
               secondary_smv_->market_update_info().bestbid_price_
                                                                    << " X " <<
               secondary_smv_->market_update_info().bestask_price_ << "]"
                                                                    << " dimerintpx " << dimer_ask_int_price_ <<
               DBGLOG_ENDL_FLUSH;*/
            basic_om_->Cancel(*ask_order_, use_reserve_msg_);
          }
        }
      }
    }
  }
};

#endif  // _EXECUTIONER_DIMER_HPP
