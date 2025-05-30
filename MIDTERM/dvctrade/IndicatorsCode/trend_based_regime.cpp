/**
   \file IndicatorsCode/trend_based_regime.cpp

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

#include "dvctrade/Indicators/trend_based_regime.hpp"
#include "dvctrade/Indicators/simple_trend.hpp"

namespace HFSAT {

void TrendBasedRegime::CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator, (std::string)r_tokens_[3]);
  if (IndicatorUtil::IsPortfolioShortcode(r_tokens_[4])) {
    std::vector<std::string> shortcode_vec;
    IndicatorUtil::GetPortfolioShortCodeVec(r_tokens_[4], shortcode_vec);
    VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator, shortcode_vec);
  } else {
    VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator, (std::string)r_tokens_[4]);
  }
}

TrendBasedRegime* TrendBasedRegime::GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& r_watch,
                                                      const std::vector<const char*>& r_tokens,
                                                      PriceType_t basepx_pxtype) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _indep_market_view_  fractional_seconds
  // _volume_measure_seconds_  _trend_weight_  _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens[3]);

  if (r_tokens.size() >= 8) {
    return GetUniqueInstance(t_dbglogger, r_watch,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens[3])), r_tokens[4],
                             atof(r_tokens[5]), atof(r_tokens[6]), StringToPriceType_t(r_tokens[7]));
  } else {
    return NULL;
  }
}

TrendBasedRegime* TrendBasedRegime::GetUniqueInstance(DebugLogger& t_dbglogger, const Watch& r_watch,
                                                      const SecurityMarketView& dep_market_view,
                                                      const std::string& indep_name, double fractional_seconds,
                                                      double threshold, PriceType_t price_type) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << dep_market_view.secname() << ' ' << indep_name << ' ' << fractional_seconds << ' '
              << threshold << ' ' << PriceType_t_To_String(price_type);
  std::string t_concise_indicator_description(t_temp_oss_.str());

  static std::map<std::string, TrendBasedRegime*> t_concise_indicator_descriptionmap_;

  if (t_concise_indicator_descriptionmap_.find(t_concise_indicator_description) ==
      t_concise_indicator_descriptionmap_.end()) {
    t_concise_indicator_descriptionmap_[t_concise_indicator_description] =
        new TrendBasedRegime(t_dbglogger, r_watch, t_concise_indicator_description, dep_market_view, indep_name,
                             fractional_seconds, threshold, price_type);
  }
  return t_concise_indicator_descriptionmap_[t_concise_indicator_description];
}

TrendBasedRegime::TrendBasedRegime(DebugLogger& t_dbglogger, const Watch& r_watch,
                                   const std::string& t_concise_indicator_description,
                                   const SecurityMarketView& t_dep_market_view, const std::string& indep_name,
                                   double fractional_seconds, double threshold, PriceType_t price_type)
    : CommonIndicator(t_dbglogger, r_watch, t_concise_indicator_description),
      dep_market_view_(&t_dep_market_view),
      indep_market_view_vec_(),
      dep_price_trend_(0),
      indep_price_trend_(0),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      dep_l1_norm_ratio_(1.0),
      indep_l1_norm_ratio_(1.0),
      dep_mean_l1_norm_(1.0),
      indep_mean_l1_norm_(1.0),
      threshold_(threshold),
      inverse_threshold_(0.0),
      pred_mode_(1u),
      p_dep_indicator_(nullptr),
      p_indep_indicator_(nullptr),
      p_indep_indicator_port_(nullptr) {
  ///

  watch_.subscribe_FifteenSecondPeriod(this);  // for UpdateLRInfo and updating trend adjustment

  if (threshold > 1.00) {
    inverse_threshold_ = 1.00 / threshold;
  }

  int idx = 1;

  p_dep_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger, r_watch, t_dep_market_view.shortcode(), fractional_seconds, price_type);
  if (p_dep_indicator_ == NULL) {
    indicator_value_ = 0;
    is_ready_ = true;
    return;
  }

  dep_mean_l1_norm_ = LoadMeanL1Norm(r_watch.YYYYMMDD(), t_dep_market_view.shortcode(), NUM_DAYS_HISTORY);
  p_dep_indicator_->add_unweighted_indicator_listener(idx, this);

  idx++;
  if (IndicatorUtil::IsPortfolioShortcode(indep_name)) {
    /// If its portfolio then calculate portfolio volume-ratio
    std::vector<std::string> shortcode_vec;
    IndicatorUtil::GetPortfolioShortCodeVec(indep_name, shortcode_vec);
    (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
        .GetSecurityMarketViewVec(shortcode_vec, indep_market_view_vec_);
    p_indep_indicator_port_ =
        SimpleTrendPort::GetUniqueInstance(t_dbglogger, r_watch, indep_name, fractional_seconds, price_type);
  } else {
    indep_market_view_vec_.push_back(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(indep_name));
    p_indep_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger, r_watch, indep_market_view_vec_[0]->shortcode(),
                                                        fractional_seconds, price_type);
    if (p_indep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }
    indep_mean_l1_norm_ = LoadMeanL1Norm(r_watch.YYYYMMDD(), indep_market_view_vec_[0]->shortcode(), NUM_DAYS_HISTORY);
  }

  if (p_indep_indicator_) {
    p_indep_indicator_->add_unweighted_indicator_listener(idx, this);
  } else if (p_indep_indicator_port_) {
    p_indep_indicator_port_->add_unweighted_indicator_listener(idx, this);
  }
}

void TrendBasedRegime::OnTimePeriodUpdate(const int num_pages_to_add_) {
  dep_l1_norm_ratio_ = fabs(dep_price_trend_) / dep_mean_l1_norm_;
  indep_l1_norm_ratio_ = fabs(indep_price_trend_) / indep_mean_l1_norm_;

  double dep_by_indep_l1_norm = 0;
  if (indep_l1_norm_ratio_ != 0) {
    dep_by_indep_l1_norm = dep_l1_norm_ratio_ / indep_l1_norm_ratio_;
  }

  if (dep_by_indep_l1_norm >= threshold_) {
    pred_mode_ = 0u;
  } else if (dep_by_indep_l1_norm < inverse_threshold_) {
    pred_mode_ = 1u;
  } else {
    pred_mode_ = 2u;
  }

  indicator_value_ = pred_mode_ + 1;
  NotifyIndicatorListeners(indicator_value_);
}

void TrendBasedRegime::OnIndicatorUpdate(const unsigned int& t_indicator_index_, const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (dep_market_view_->is_ready_complex(2)) {
      is_ready_ = true;
      for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
        is_ready_ = is_ready_ && indep_market_view_vec_[i]->is_ready_complex(2);
      }
      if (is_ready_) {
        InitializeValues();
      }
    }
  } else {
    if (!data_interrupted_) {
      switch (t_indicator_index_) {
        case 1u: {
          dep_price_trend_ = t_new_indicator_value_;
        } break;
        case 2u: {
          indep_price_trend_ = t_new_indicator_value_;
        } break;
        default: { } break; }
    }
  }
}

void TrendBasedRegime::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_->is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_->secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    for (auto smv : indep_market_view_vec_)
      if (!(smv->is_ready_complex(2))) {
        DBGLOG_TIME_CLASS << smv->secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
  }
}

// market_interrupt_listener interface
void TrendBasedRegime::OnMarketDataInterrupted(const unsigned int security_id, const int msecs_since_last_receive) {
  for (auto smv : indep_market_view_vec_) {
    if (security_id == smv->security_id()) {
      data_interrupted_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
      break;
    }
  }
}

void TrendBasedRegime::OnMarketDataResumed(const unsigned int security_id) {
  for (auto smv : indep_market_view_vec_) {
    if (security_id == smv->security_id()) {
      InitializeValues();
      data_interrupted_ = false;
      break;
    }
  }
}

void TrendBasedRegime::InitializeValues() { indicator_value_ = 0; }
}
