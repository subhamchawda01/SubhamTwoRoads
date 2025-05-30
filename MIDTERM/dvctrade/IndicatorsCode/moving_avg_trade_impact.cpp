/**
    \file IndicatorsCode/moving_avg_trade_size.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/moving_avg_trade_impact.hpp"

namespace HFSAT {

void MovingAvgTradeImpact::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                             std::vector<std::string>& _ors_source_needed_vec_,
                                             const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

MovingAvgTradeImpact* MovingAvgTradeImpact::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const std::vector<const char*>& r_tokens_,
                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _window_msecs_ buysell_(0:buy:1:sell)
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), atoi(r_tokens_[5]), atof(r_tokens_[6]));
}

MovingAvgTradeImpact* MovingAvgTradeImpact::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              SecurityMarketView& _indep_market_view_,
                                                              int _window_msecs_, int buysell_, double decay_factor_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _window_msecs_ << ' ' << buysell_
              << decay_factor_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, MovingAvgTradeImpact*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new MovingAvgTradeImpact(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                 _window_msecs_, buysell_, decay_factor_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

MovingAvgTradeImpact::MovingAvgTradeImpact(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                           const std::string& concise_indicator_description_,
                                           SecurityMarketView& _indep_market_view_, int _window_msecs_, int _buysell_,
                                           double decay_factor_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      page_width_msecs_(_window_msecs_),
      buysell_(_buysell_),
      decay_price_factor_(decay_factor_),
      decay_price_factor_vec_(),
      last_int_trade_price_(0),
      current_trade_impact_(0),
      current_int_trade_price_(0),
      sum_trades_(0),
      current_book_size_(0),
      current_trade_size_(0),
      trades_queue_mempool_() {
  RescaleFactorVec(100);
  indep_market_view_.ComputeTradeImpact();
  indep_market_view_.subscribe_tradeprints(this);
}

/**
 *
 * @param till_value
 */
void MovingAvgTradeImpact::RescaleFactorVec(int till_value) {
  auto orig_size = decay_price_factor_vec_.size();
  decay_price_factor_vec_.resize(till_value + 1, 0);
  for (auto i = (int)orig_size; i <= till_value; i++) {
    decay_price_factor_vec_[i] = pow(decay_page_factor_, i);
  }
}

void MovingAvgTradeImpact::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void MovingAvgTradeImpact::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                        const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  } else {
    if ((int)_trade_print_info_.buysell_ == buysell_) {
      current_trade_impact_ = _trade_print_info_.trade_impact_;
      current_int_trade_price_ = _trade_print_info_.int_trade_price_;
      current_trade_size_ = _trade_print_info_.size_traded_;
      while (!trades_queue_.empty()) {
        TradeElem* t_trade_elem_ = trades_queue_.front();
        if (watch_.msecs_from_midnight() - t_trade_elem_->time_msecs_ < page_width_msecs_) {
          break;
        }
        indicator_value_ = indicator_value_ - t_trade_elem_->impact_;
        sum_trades_ = sum_trades_ - t_trade_elem_->size_;
        // memory leak
        trades_queue_.pop_front();
      }
      if (current_int_trade_price_ != last_int_trade_price_) {
        sum_trades_ = 0;
        for (auto i = 0u; i < trades_queue_.size(); i++) {
          trades_queue_[i]->size_ = 0;
        }
      }

      current_book_size_ = current_trade_size_ / current_trade_impact_;
      double modified_current_trade_impact_ = double(current_trade_size_) / (sum_trades_ + current_book_size_);

      auto int_px_diff = abs(current_int_trade_price_ - last_int_trade_price_);

      if (current_int_trade_price_ == last_int_trade_price_ && current_trade_impact_ < 1) {
        indicator_value_ += modified_current_trade_impact_;
        auto trade_elem = trades_queue_mempool_.Alloc();
        trade_elem->time_msecs_ = watch_.msecs_from_midnight();
        trade_elem->impact_ = modified_current_trade_impact_;
        trade_elem->size_ = current_trade_size_;
        trades_queue_.push_back(trade_elem);
      } else {
        if (current_trade_impact_ == 1) modified_current_trade_impact_ *= decay_price_factor_;

        if (decay_price_factor_ == 0 && int_px_diff == 0) {
          for (auto i = 0u; i < trades_queue_.size(); i++) {
            trades_queue_[i]->impact_ = 0;
          }
          indicator_value_ = 0;
        } else {
          if (int_px_diff >= (int)decay_price_factor_vec_.size()) {
            RescaleFactorVec(int_px_diff + 10);
          }

          for (auto i = 0u; i < trades_queue_.size(); i++) {
            trades_queue_[i]->impact_ *= decay_price_factor_vec_[int_px_diff];
          }

          indicator_value_ = modified_current_trade_impact_ + indicator_value_ * decay_price_factor_vec_[int_px_diff];
        }

        auto trade_elem = trades_queue_mempool_.Alloc();
        trade_elem->time_msecs_ = watch_.msecs_from_midnight();
        trade_elem->impact_ = modified_current_trade_impact_;
        trade_elem->size_ = current_trade_size_;

        trades_queue_.push_back(trade_elem);
      }

      last_int_trade_price_ = current_int_trade_price_;
      sum_trades_ += current_trade_size_;
      if (current_trade_impact_ == 1) {
        if ((int)_trade_print_info_.buysell_ == 0)
          last_int_trade_price_ = current_int_trade_price_ + 1;
        else
          last_int_trade_price_ = current_int_trade_price_ - 1;
      }

      if (data_interrupted_) {
        indicator_value_ = 0;
      }

      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void MovingAvgTradeImpact::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                   const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void MovingAvgTradeImpact::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}
}
