/**
  \file IndicatorsCode/level_size_per_order.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
Address:
Suite No 353, Evoma, #14, Bhattarhalli,
Old Madras Road, Near Garden City College,
KR Puram, Bangalore 560049, India
+91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"
#include <string>
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/level_size_per_order.hpp"

namespace HFSAT {

void LevelSizePerOrder::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                          std::vector<std::string>& _ors_source_needed_vec_,
                                          const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() < 4) {
    std::cerr << "Invalid Tokens to LevelSizePerOrder::CollectShortCodes \n";
    exit(1);
  }
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

LevelSizePerOrder* LevelSizePerOrder::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const std::vector<const char*>& r_tokens_, bool _is_ask) {
  if (r_tokens_.size() < 6) {
    std::cerr << "Invalid Args to LevelSizePerOrder\n";
    exit(1);
  }
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ book_level_ _is_ask
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), atoi(r_tokens_[5]) > 0);
}

LevelSizePerOrder* LevelSizePerOrder::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        SecurityMarketView& _indep_market_view_, int book_level_,
                                                        bool _is_ask) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << book_level_ << ' ' << _is_ask;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, LevelSizePerOrder*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new LevelSizePerOrder(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, book_level_, _is_ask);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

LevelSizePerOrder::LevelSizePerOrder(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     const std::string& concise_indicator_description_,
                                     SecurityMarketView& _indep_market_view_, int _book_level_, bool is_ask)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      book_level_(_book_level_),
      _is_ask(is_ask) {
  indep_market_view_.subscribe_L2(this);
  is_ready_ = false;
  indicator_value_ = 0;

#if EQUITY_INDICATORS_ALWAYS_READY
  if (IndicatorUtil::IsEquityShortcode(indep_market_view_.shortcode())) {
    is_ready_ = true;
    InitializeValues();
  }
#endif
}

void LevelSizePerOrder::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void LevelSizePerOrder::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready() &&
        (indep_market_view_.spread_increments() < (2 * indep_market_view_.normal_spread_increments()))) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else if (!data_interrupted_) {
    if (!_is_ask) {
      if (indep_market_view_.bid_order(book_level_) > 0)
        indicator_value_ = indep_market_view_.bid_size(book_level_) / (double)indep_market_view_.bid_order(book_level_);
      else
        indicator_value_ = 0;
    } else {
      if (indep_market_view_.ask_order(book_level_) > 0)
        indicator_value_ = indep_market_view_.ask_size(book_level_) / (double)indep_market_view_.ask_order(book_level_);
      else
        indicator_value_ = 0;
    }

    if (std::isnan(indicator_value_)) {
      std::cerr << __PRETTY_FUNCTION__ << " nan in " << concise_indicator_description() << std::endl;
      indicator_value_ = 0;
    }

    if (indicator_value_ < 0) {
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void LevelSizePerOrder::InitializeValues() { indicator_value_ = 0; }

// market_interrupt_listener interface
void LevelSizePerOrder::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void LevelSizePerOrder::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
