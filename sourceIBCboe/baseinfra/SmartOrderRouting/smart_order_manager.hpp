/**
   \file SmartOrderRouting/smart_order_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_SMARTORDERROUTING_SMART_ORDER_MANAGER_H
#define BASE_SMARTORDERROUTING_SMART_ORDER_MANAGER_H

#include "baseinfra/OrderRouting/base_order_manager.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

#include "baseinfra/SmartOrderRouting/base_pnl.hpp"

namespace HFSAT {

/// \brief extends BaseOrderManager with information of SMV to (i)adjust for our own presence (ii)calculate place in
/// line of orders
class SmartOrderManager : public BaseOrderManager, public SecurityMarketViewChangeListener {
 protected:
  SecurityMarketView& dep_market_view_;
  const bool livetrading_;
  BasePNL* p_base_pnl_;

  int num_whole_trades_;
  bool last_update_was_quote_;
  bool this_update_is_trade_;

 public:
  SmartOrderManager(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityNameIndexer& _sec_name_indexer_,
                    BaseTrader& _base_trader_, SecurityMarketView& _dep_market_view_, int runtime_id_,
                    const bool r_livetrading_, int _first_client_assigned_order_sequence_);

  ~SmartOrderManager() {}

  /// @brief SecurityMarketViewChangeListener callback : update place in line of all orders at the best market
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  void UpdateQueuePosition(int t_int_price_);
  void UpdateBidQueuePosition(int t_int_price_);
  void UpdateAskQueuePosition(int t_int_price_);

  // advanced
  inline void GetBestNonSelfBid(const double _max_self_ratio_at_level_, double& _best_nonself_bid_price_,
                                int& _best_nonself_bid_int_price_, int& _best_nonself_bid_size_) {
    if (dep_market_view_.use_stable_bidask_levels_) {
      _best_nonself_bid_price_ = dep_market_view_.GetDoublePx(dep_market_view_.bid_side_valid_level_int_price_);
      _best_nonself_bid_int_price_ = dep_market_view_.bid_side_valid_level_int_price_;
      _best_nonself_bid_size_ = dep_market_view_.bid_side_valid_level_size_;
    } else {
      if (_max_self_ratio_at_level_ < 0.99) {
        int _our_size_ = GetTotalBidSizeOrderedAtIntPx(dep_market_view_.bestbid_int_price());
        double min_size_to_cmp = (_max_self_ratio_at_level_ * dep_market_view_.bestbid_size());
        if (livetrading_) {
          min_size_to_cmp -= _our_size_;
        }  // this will be a problem if we receive ou confirm faster than market data sees the order
        if ((_our_size_ > min_size_to_cmp) && (dep_market_view_.bid_int_price(1) > 0)) {
          _best_nonself_bid_price_ = dep_market_view_.bid_price(1);
          _best_nonself_bid_int_price_ = dep_market_view_.bid_int_price(1);
          _best_nonself_bid_size_ = dep_market_view_.bestbid_size() + dep_market_view_.bid_size(1) -
                                    _our_size_;  // add sizes of first 2 levels not equals
          return;
        }
      }
      _best_nonself_bid_price_ = dep_market_view_.bestbid_price();
      _best_nonself_bid_int_price_ = dep_market_view_.bestbid_int_price();
      _best_nonself_bid_size_ = dep_market_view_.bestbid_size();
    }
  }

  inline void GetBestNonSelfAsk(const double _max_self_ratio_at_level_, double& _best_nonself_ask_price_,
                                int& _best_nonself_ask_int_price_, int& _best_nonself_ask_size_) {
    if (dep_market_view_.use_stable_bidask_levels_) {
      _best_nonself_ask_price_ = dep_market_view_.GetDoublePx(dep_market_view_.ask_side_valid_level_int_price_);
      _best_nonself_ask_int_price_ = dep_market_view_.ask_side_valid_level_int_price_;
      _best_nonself_ask_size_ = dep_market_view_.bid_side_valid_level_size_;
    } else {
      if (_max_self_ratio_at_level_ < 0.99) {
        int _our_size_ = GetTotalAskSizeOrderedAtIntPx(dep_market_view_.bestask_int_price());
        double min_size_to_cmp = (_max_self_ratio_at_level_ * dep_market_view_.bestask_size());
        if (livetrading_) {
          min_size_to_cmp -= _our_size_;
        }
        if ((_our_size_ > min_size_to_cmp) && (dep_market_view_.ask_int_price(1) > 0)) {
          _best_nonself_ask_price_ = dep_market_view_.ask_price(1);
          _best_nonself_ask_int_price_ = dep_market_view_.ask_int_price(1);
          _best_nonself_ask_size_ =
              dep_market_view_.bestask_size() + dep_market_view_.ask_size(1) - _our_size_;  // add not equals
          return;
        }
      }

      _best_nonself_ask_price_ = dep_market_view_.bestask_price();
      _best_nonself_ask_int_price_ = dep_market_view_.bestask_int_price();
      _best_nonself_ask_size_ = dep_market_view_.bestask_size();
    }
  }

  inline void SetBasePNL(BasePNL* t_p_base_pnl_) {
    p_base_pnl_ = t_p_base_pnl_;
    AddExecutionListener(t_p_base_pnl_);
  }

  inline BasePNL& base_pnl() const { return *p_base_pnl_; }

  virtual void ComputeQueueSizes();

  const int TradeParticipationPercentage(void) const {
    return ((num_whole_trades_ > 0) ? ((num_self_trades_ * 100) / num_whole_trades_) : (100));
  }

  const int global_position() const { return global_position_; }

  void ShowOrderBook() {
    int unconfirmed_bid_index_ = unconfirmed_top_bid_index_;
    int confirmed_bid_index_ = confirmed_top_bid_index_;

    int unconfirmed_ask_index_ = unconfirmed_top_ask_index_;
    int confirmed_ask_index_ = confirmed_top_ask_index_;

    /// vectors to store values
    std::vector<int> my_comb_bidpxs_;
    std::vector<int> my_comb_bidszs_;
    std::vector<int> my_comb_askpxs_;
    std::vector<int> my_comb_askszs_;

    /// current level bid stats
    while ((unconfirmed_bottom_bid_index_ != -1 && unconfirmed_bid_index_ >= unconfirmed_bottom_bid_index_) ||
           (confirmed_bottom_bid_index_ != -1 && confirmed_bid_index_ >= confirmed_bottom_bid_index_)) {
      int current_bidpx_ = 0, current_bidsz_ = 0;
      int temp_bidpx_ = 0, temp_bidsz_ = 0;
      if (unconfirmed_bottom_bid_index_ != -1 && unconfirmed_bid_index_ >= unconfirmed_bottom_bid_index_) {
        temp_bidpx_ = GetBidIntPrice(unconfirmed_bid_index_);
        temp_bidsz_ = sum_bid_unconfirmed_[unconfirmed_bid_index_];
      }
      if (confirmed_bottom_bid_index_ != -1 && confirmed_bid_index_ >= confirmed_bottom_bid_index_) {
        /// current best level is confirmed
        if (temp_bidpx_ && GetBidIntPrice(confirmed_bid_index_) > temp_bidpx_) {
          current_bidpx_ = GetBidIntPrice(confirmed_bid_index_);
          current_bidsz_ = sum_bid_confirmed_[confirmed_bid_index_];
          confirmed_bid_index_--;
        }
        /// current best level is unconfirmed
        else if (temp_bidpx_ && GetBidIntPrice(confirmed_bid_index_) < temp_bidpx_) {
          current_bidpx_ = temp_bidpx_;
          current_bidsz_ = temp_bidsz_;
          unconfirmed_bid_index_--;
        }
        /// current best level is confirmed ( + unconfirmed if unconfirmed_iter is valid )
        else {
          if (temp_bidpx_) {
            current_bidsz_ = temp_bidsz_;
            unconfirmed_bid_index_--;
          }
          current_bidsz_ += sum_bid_confirmed_[confirmed_bid_index_];
          current_bidpx_ = GetBidIntPrice(confirmed_bid_index_);
          confirmed_bid_index_--;
        }
      }
      /// case where confirmed iter is invalid
      else {
        current_bidpx_ = temp_bidpx_;
        current_bidsz_ = temp_bidsz_;
        unconfirmed_bid_index_--;
      }
      /// handle map references with 0 sizes
      if (current_bidsz_ > 0) {
        my_comb_bidpxs_.push_back(current_bidpx_);
        my_comb_bidszs_.push_back(current_bidsz_);
      }
    }

    /// current level ask stats
    while ((unconfirmed_top_ask_index_ != -1 && unconfirmed_ask_index_ >= unconfirmed_bottom_ask_index_) ||
           (confirmed_top_ask_index_ != -1 && confirmed_ask_index_ >= confirmed_bottom_ask_index_)) {
      int current_askpx_ = 0, current_asksz_ = 0;
      int temp_askpx_ = 0, temp_asksz_ = 0;
      if (unconfirmed_top_ask_index_ != -1 && unconfirmed_ask_index_ >= unconfirmed_bottom_ask_index_) {
        temp_askpx_ = GetAskIntPrice(unconfirmed_ask_index_);
        temp_asksz_ = sum_ask_unconfirmed_[unconfirmed_ask_index_];
      }
      if (confirmed_top_ask_index_ != -1 && confirmed_ask_index_ >= confirmed_bottom_ask_index_) {
        /// current best level is confirmed
        if (temp_askpx_ && GetAskIntPrice(confirmed_ask_index_) < temp_askpx_) {
          current_askpx_ = GetAskIntPrice(confirmed_ask_index_);
          current_asksz_ = sum_ask_confirmed_[confirmed_ask_index_];
          confirmed_ask_index_--;
        }
        /// current best level is unconfirmed
        else if (temp_askpx_ && GetAskIntPrice(confirmed_ask_index_) > temp_askpx_) {
          current_askpx_ = temp_askpx_;
          current_asksz_ = temp_asksz_;
          unconfirmed_ask_index_--;
        }
        /// current best level is confirmed ( + unconfirmed if unconfirmed_iter is valid )
        else {
          if (temp_askpx_) {
            current_asksz_ = temp_asksz_;
            unconfirmed_ask_index_--;
          }
          current_asksz_ += sum_ask_confirmed_[confirmed_ask_index_];
          current_askpx_ = GetAskIntPrice(confirmed_ask_index_);
          confirmed_ask_index_--;
        }
      }
      /// case where confirmed iter is invalid
      else {
        current_askpx_ = temp_askpx_;
        current_asksz_ = temp_asksz_;
        unconfirmed_ask_index_--;
      }
      /// handle map references with 0 sizes
      if (current_asksz_ > 0) {
        my_comb_askpxs_.push_back(current_askpx_);
        my_comb_askszs_.push_back(current_asksz_);
      }
    }

    /// display the results
    dbglogger_ << "Full Orders dump @ " << watch_.tv() << '\n';
    dbglogger_ << "Market " << dep_market_view_.bestbid_size() << " @ " << dep_market_view_.bestbid_int_price()
               << " ----- " << dep_market_view_.bestask_int_price() << " @ " << dep_market_view_.bestask_size()
               << "\n\n";
    for (uint t_counter_ = 0; t_counter_ < std::max(my_comb_bidpxs_.size(), my_comb_askpxs_.size()); t_counter_++) {
      if (t_counter_ < my_comb_bidpxs_.size()) {
        dbglogger_ << my_comb_bidszs_[t_counter_] << " @ " << my_comb_bidpxs_[t_counter_];
      }
      dbglogger_ << "   -----   ";
      if (t_counter_ < my_comb_askpxs_.size()) {
        dbglogger_ << my_comb_askpxs_[t_counter_] << " @ " << my_comb_askszs_[t_counter_];
      }
      dbglogger_ << '\n';
    }
    dbglogger_ << "\n\n-------------------------------------------------------------------------------------\n";
  }

 protected:
  inline double GetMidPrice() { return dep_market_view_.mid_price(); }

  void InitialQueueBid(BaseOrder* p_newly_confirmed_bid_order_);
  void InitialQueueAsk(BaseOrder* p_newly_confirmed_ask_order_);
};
}
#endif  // BASE_SMARTORDERROUTING_SMART_ORDER_MANAGER_H
