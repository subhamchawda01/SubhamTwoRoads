/**
    \file IndicatorsCode/offline_computed_pairs.cpp

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
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvctrade/Indicators/mult_based_delta.hpp"
#include "dvctrade/Indicators/simple_returns.hpp"

namespace HFSAT {

void MultBasedDelta::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                       std::vector<std::string>& _ors_source_needed_vec_,
                                       const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);
}

MultBasedDelta* MultBasedDelta::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::vector<const char*>& r_tokens_,
                                                  PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_dep_market_view_ t_indep1_market_view_ t_indep2_market_view_
  // t_fractional_seconds_ t_price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[5]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

MultBasedDelta* MultBasedDelta::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  SecurityMarketView& t_dep_market_view_,
                                                  SecurityMarketView& t_indep1_market_view_,
                                                  SecurityMarketView& t_indep2_market_view_,
                                                  double t_fractional_seconds_, PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << t_indep1_market_view_.secname() << ' '
              << t_indep2_market_view_.secname() << ' ' << t_fractional_seconds_ << ' ' << t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, MultBasedDelta*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new MultBasedDelta(t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_,
                           t_indep1_market_view_, t_indep2_market_view_, t_fractional_seconds_, t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

MultBasedDelta::MultBasedDelta(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                               const std::string& concise_indicator_description_,
                               SecurityMarketView& t_dep_market_view_, SecurityMarketView& t_indep1_market_view_,
                               SecurityMarketView& t_indep2_market_view_, double t_fractional_seconds_,
                               PriceType_t t_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep1_market_view_(t_indep1_market_view_),
      indep2_market_view_(t_indep2_market_view_),
      dep_ret_trend_(0),
      indep1_ret_trend_(0),
      indep2_ret_trend_(0),
      price_type_(t_price_type_) {
  if (!dep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << " to DEP " << std::endl;
  }

  if ((t_dep_market_view_.security_id() == t_indep1_market_view_.security_id()) &&
      (t_dep_market_view_.security_id() ==
       t_indep2_market_view_.security_id())) {  // added this since for convenience one could add a combo or portfolio
                                                // as source with a security
    // that is also the dependant
    indicator_value_ = 0;
    is_ready_ = true;
    return;
  } else {
    dep_ret_indicator_ = SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_,
                                                          t_fractional_seconds_, t_price_type_);
    indep1_ret_indicator_ = SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, t_indep1_market_view_,
                                                             t_fractional_seconds_, t_price_type_);
    indep2_ret_indicator_ = SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, t_indep2_market_view_,
                                                             t_fractional_seconds_, t_price_type_);
    if (dep_ret_indicator_ == NULL || indep1_ret_indicator_ == NULL || indep2_ret_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    dep_ret_indicator_->add_unweighted_indicator_listener(1u, this);
    indep1_ret_indicator_->add_unweighted_indicator_listener(2u, this);
    indep2_ret_indicator_->add_unweighted_indicator_listener(3u, this);
  }
}

void MultBasedDelta::OnIndicatorUpdate(const unsigned int& t_indicator_index_, const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep1_market_view_.is_ready_complex(2) &&
        indep2_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (!data_interrupted_) {
      switch (t_indicator_index_) {
        case 1u: {
          dep_ret_trend_ = t_new_indicator_value_;
        } break;
        case 2u: {
          indep1_ret_trend_ = t_new_indicator_value_;
        } break;
        case 3u: {
          indep2_ret_trend_ = t_new_indicator_value_;
        } break;
      }

      indicator_value_ =
          (indep1_ret_trend_ + indep2_ret_trend_ - dep_ret_trend_) * dep_market_view_.price_from_type(price_type_);
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void MultBasedDelta::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep1_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep1_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep2_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep2_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

// market_interrupt_listener interface
void MultBasedDelta::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (_security_id_ == indep1_market_view_.security_id() || _security_id_ == indep2_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void MultBasedDelta::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == indep1_market_view_.security_id() || _security_id_ == indep2_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void MultBasedDelta::InitializeValues() { indicator_value_ = 0; }
}
