/**
    \file IndicatorsCode/big_trade_volume_version.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/big_trade_volume_version.hpp"

namespace HFSAT {

void BigTradeVolumeVersion::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

BigTradeVolumeVersion* BigTradeVolumeVersion::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _window_msecs_ buysell_(0:buy:1:sell)
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]));
}

BigTradeVolumeVersion* BigTradeVolumeVersion::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                SecurityMarketView& _indep_market_view_,
                                                                int _window_msecs_ ) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _window_msecs_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, BigTradeVolumeVersion*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new BigTradeVolumeVersion(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _window_msecs_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

BigTradeVolumeVersion::BigTradeVolumeVersion(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             SecurityMarketView& _indep_market_view_, int _window_msecs_ )
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      window_msecs_(_window_msecs_),
      last_buysell_(-1),
      current_int_trade_price_(0),
      last_int_trade_price_(0),
      last_bestbid_int_price_(0),
      last_bestask_int_price_(0),
      current_trade_size_(0) {
  indep_market_view_.ComputeTradeImpact();
  indep_market_view_.subscribe_tradeprints(this);
}

void BigTradeVolumeVersion::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void BigTradeVolumeVersion::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                         const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  } else {  // remove trades which falls outside of window_msecs
    while (!trades_queue_.empty()) {
      TradeElem* t_trade_elem_ = trades_queue_.front();
      if (watch_.msecs_from_midnight() - t_trade_elem_->time_msecs_ < window_msecs_) {
        break;
      }
      indicator_value_ = indicator_value_ - t_trade_elem_->size_;
      trades_queue_.pop_front();
    }

    int buysell_parity_ = (int)_trade_print_info_.buysell_ > 0 ? -1 : 1;
    current_trade_size_ = _trade_print_info_.size_traded_ * buysell_parity_;
    current_int_trade_price_ = _trade_print_info_.int_trade_price_;

    if ((int)_trade_print_info_.buysell_ == last_buysell_ && current_int_trade_price_ == last_int_trade_price_ ) {
      trades_queue_.push_back(new TradeElem(watch_.msecs_from_midnight(), current_trade_size_, buysell_parity_));
      indicator_value_ += current_trade_size_;
    } else {
      while (!trades_queue_.empty()) {
        trades_queue_.pop_front();
      }      
      trades_queue_.push_back(new TradeElem(watch_.msecs_from_midnight(), current_trade_size_, buysell_parity_));
      indicator_value_ = current_trade_size_;
    }

    if (_market_update_info_.bestask_int_price_ - _market_update_info_.bestbid_int_price_ > 1 ) {
      indicator_value_ = 0;
      while (!trades_queue_.empty()) {
        trades_queue_.pop_front();
      }
    } else if (_market_update_info_.bestbid_int_price_ != last_bestbid_int_price_ || _market_update_info_.bestask_int_price_ != last_bestask_int_price_ ) {
      indicator_value_ = 0;
      while (!trades_queue_.empty()) {
        trades_queue_.pop_front();
      }
    }

    if (data_interrupted_) {
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
    last_int_trade_price_ = current_int_trade_price_;
    last_buysell_ = (int)_trade_print_info_.buysell_;
    last_bestbid_int_price_ = _market_update_info_.bestbid_int_price_;
    last_bestask_int_price_ = _market_update_info_.bestask_int_price_;
  }
}

void BigTradeVolumeVersion::OnMarketUpdate ( const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if(!is_ready_) {
    if(indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  } else {
    while (!trades_queue_.empty()) {
      TradeElem* t_trade_elem_ = trades_queue_.front();
      if ( watch_.msecs_from_midnight() - t_trade_elem_->time_msecs_ < window_msecs_ ) {
        break;
      }
      indicator_value_ = indicator_value_ - t_trade_elem_->size_ * t_trade_elem_->buysell_;      
      trades_queue_.pop_front();
    }

    if ( _market_update_info_.bestask_int_price_ - _market_update_info_.bestbid_int_price_  > 1 ) {
      indicator_value_ = 0;
      while(!trades_queue_.empty()) {
        trades_queue_.pop_front();
      }
    } else if ( _market_update_info_.bestask_int_price_ != last_bestask_int_price_ || _market_update_info_.bestbid_int_price_ != last_bestbid_int_price_ ) {
      while(!trades_queue_.empty()) {
        trades_queue_.pop_front();
      }
    }
 
    if (data_interrupted_) {
      indicator_value_ = 0;
    }
    
    NotifyIndicatorListeners(indicator_value_);    
    last_bestbid_int_price_ = _market_update_info_.bestbid_int_price_;
    last_bestask_int_price_ = _market_update_info_.bestask_int_price_;
  }
}

void BigTradeVolumeVersion::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void BigTradeVolumeVersion::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}
}
