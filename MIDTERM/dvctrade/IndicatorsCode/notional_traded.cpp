/**
    \file IndicatorsCode/notional_traded.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvctrade/Indicators/notional_traded.hpp"
#include "dvccode/CDef/security_definitions.hpp"

namespace HFSAT {

void NotionalTraded::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                       std::vector<std::string>& _ors_source_needed_vec_,
                                       const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

NotionalTraded* NotionalTraded::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::vector<const char*>& r_tokens_,
                                                  PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_dep_market_view_ trade duration

  if (r_tokens_.size() < 5) {
    std::cerr << "Insufficient arguments to INDICATOR NotionalTraded, correct syntax : _this_weight_ "
                 "_indicator_string_ t_dep_market_view_ trade duration";
    exit(1);
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]));
}

NotionalTraded* NotionalTraded::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  SecurityMarketView& t_dep_market_view_, double _trade_duration_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << _trade_duration_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, NotionalTraded*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new NotionalTraded(
        t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_, _trade_duration_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

NotionalTraded::NotionalTraded(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                               const std::string& concise_indicator_description_,
                               SecurityMarketView& t_dep_market_view_, double _trade_duration_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      dep_time_decayed_trade_info_manager_(*(TimeDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, t_dep_market_view_, _trade_duration_))) {
  dep_n2d_ = SecurityDefinitions::contract_specification_map_[dep_market_view_.shortcode()].numbers_to_dollars_;
  dep_time_decayed_trade_info_manager_.compute_sumpxsz();
  dep_market_view_.subscribe_tradeprints(this);
}

void NotionalTraded::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                  const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (!data_interrupted_) {
      indicator_value_ = dep_n2d_ * dep_time_decayed_trade_info_manager_.sumpxsz_;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void NotionalTraded::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void NotionalTraded::InitializeValues() { indicator_value_ = 0; }
}
