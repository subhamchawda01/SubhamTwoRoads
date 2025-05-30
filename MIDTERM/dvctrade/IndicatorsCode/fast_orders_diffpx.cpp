// =====================================================================================
//
//       Filename:  fast_orders.cpp_diffpx.cpp
//
//    Description:  Uses Order Cancels for small orders
//
//        Version:  1.0
//        Created:  01/06/2015 04:31:48 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/fast_orders_diffpx.hpp"

namespace HFSAT {
void FastOrdersDiffPx::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

FastOrdersDiffPx* FastOrdersDiffPx::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      atoi(r_tokens_[4]), atoi(r_tokens_[5]), atoi(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

FastOrdersDiffPx* FastOrdersDiffPx::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const SecurityMarketView& _indep_market_view_, int _window_msecs_,
                                                      int _num_fast_orders_, int _orders_size_,
                                                      PriceType_t t_base_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _window_msecs_ << ' ' << _num_fast_orders_
              << ' ' << _orders_size_ << PriceType_t_To_String(t_base_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, FastOrdersDiffPx*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new FastOrdersDiffPx(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                             _window_msecs_, _num_fast_orders_, _orders_size_, t_base_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

FastOrdersDiffPx::FastOrdersDiffPx(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   const SecurityMarketView& _indep_market_view_, int _window_msecs_,
                                   int _num_fast_orders_, int _orders_size_, PriceType_t t_base_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
      fast_orders_size_(_orders_size_),
      window_usecs_(_window_msecs_ * 1000),
      num_fast_orders_(_num_fast_orders_),
      min_price_increment_(_indep_market_view_.min_price_increment()),
      mid_price_(0),
      fast_best_bid_int_price_(0),
      fast_best_ask_int_price_(0),
      fast_best_bid_size_(1),
      fast_best_ask_size_(1),
      fast_mkt_price_(0),
      last_bid_price_(0),
      last_ask_price_(0),
      base_price_type_(t_base_price_type_) {
  if (!indep_market_view_.subscribe_price_type(this, base_price_type_)) {
    PriceType_t t_error_price_type_ = base_price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  if (base_price_type_ != kPriceTypeMidprice) {
    if (!indep_market_view_.subscribe_price_type(this, kPriceTypeMidprice)) {
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << kPriceTypeMidprice << std::endl;
    }
  }

  HFSAT::BaseUtils::EOBIFastOrderManager::SetFastBookListenerForAll();
}

void FastOrdersDiffPx::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void FastOrdersDiffPx::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  HFSAT::BaseUtils::EOBIFastOrderManager& eobi_fast_order_manager =
      HFSAT::BaseUtils::EOBIFastOrderManager::GetUniqueInstance(_security_id_);

  fast_best_bid_int_price_ =
      eobi_fast_order_manager.GetFastBidPrice(fast_orders_size_, num_fast_orders_, window_usecs_);
  fast_best_ask_int_price_ =
      eobi_fast_order_manager.GetFastAskPrice(fast_orders_size_, num_fast_orders_, window_usecs_);
  fast_best_bid_size_ = eobi_fast_order_manager.GetFastBidNetSize(fast_orders_size_, num_fast_orders_, window_usecs_);
  fast_best_ask_size_ = eobi_fast_order_manager.GetFastAskNetSize(fast_orders_size_, num_fast_orders_, window_usecs_);
  mid_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMidprice, _market_update_info_);

  fast_mkt_price_ = (double)((double)fast_best_bid_int_price_ * fast_best_ask_size_ +
                             fast_best_ask_int_price_ * fast_best_bid_size_) /
                    (fast_best_ask_size_ + fast_best_bid_size_);
  if ((fast_best_ask_int_price_ - fast_best_bid_int_price_) > 1) {
    fast_mkt_price_ = ((double)fast_best_bid_int_price_ + fast_best_ask_int_price_) / 2.0;
  }
  fast_mkt_price_ = fast_mkt_price_ * min_price_increment_;
  if (fast_mkt_price_ > 0 && mid_price_ > 0) {
    if (_market_update_info_.bestask_int_price_ - _market_update_info_.bestbid_int_price_ < 2) {
      indicator_value_ = fast_mkt_price_ - SecurityMarketView::GetPriceFromType(base_price_type_, _market_update_info_);
    } else {
      indicator_value_ = fast_mkt_price_ - mid_price_;
    }
    if (fast_best_bid_int_price_ > _market_update_info_.bestbid_int_price_ ||
        fast_best_ask_int_price_ < _market_update_info_.bestask_int_price_) {
      indicator_value_ = 0;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void FastOrdersDiffPx::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void FastOrdersDiffPx::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void FastOrdersDiffPx::InitializeValues() {
  fast_best_bid_int_price_ = 0;
  fast_best_ask_int_price_ = 0;
  fast_best_bid_size_ = 0;
  fast_best_ask_size_ = 0;
  fast_mkt_price_ = 0;
  last_bid_price_ = 0;
  last_ask_price_ = 0;
  mid_price_ = 0;
}
}
