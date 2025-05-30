/*
 * price_regime.cpp
 *
 *  Created on: 04-Mar-2016
 *      Author: raghuram
 */

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/price_regime.hpp"
#include "dvctrade/Indicators/exponential_moving_average.hpp"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

namespace HFSAT {

void PriceRegime::CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& r_ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(r_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

PriceRegime* PriceRegime::GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  /*
        if (r_tokens_.size() < 9) {
    ExitVerbose(
        kModelCreationIndicatorLineLessArgs, r_dbglogger_,
        "INDICATOR weight PriceRegime _sec_market_view_ half_life _threshold_vec_"
        "_tolerance_  price_type_");
  }
*/
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  std::vector<double> _thres_vec_ = {};

  for (unsigned int i = 5; i < r_tokens_.size() - 2; i++) {
    _thres_vec_.push_back(atof(r_tokens_[i]));
  }

  return GetUniqueInstance(r_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), _thres_vec_, atof(r_tokens_[r_tokens_.size() - 2]),
                           StringToPriceType_t(r_tokens_[r_tokens_.size() - 1]));
}

PriceRegime* PriceRegime::GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                            SecurityMarketView& _sec_market_view_, double _half_life_,
                                            std::vector<double> _threshold_vec_, double _tolerance_,
                                            PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  if (_threshold_vec_.size() == 1) {
    t_temp_oss_ << VarName() << ' ' << _sec_market_view_.secname() << ' ' << _half_life_ << ' ' << _threshold_vec_[0]
                << ' ' << _tolerance_ << ' ' << PriceType_t_To_String(_price_type_);
  }

  if (_threshold_vec_.size() == 2) {
    t_temp_oss_ << VarName() << ' ' << _sec_market_view_.secname() << ' ' << _half_life_ << ' ' << _threshold_vec_[0]
                << ' ' << _threshold_vec_[1] << ' ' << _tolerance_ << ' ' << PriceType_t_To_String(_price_type_);
  }

  if (_threshold_vec_.size() == 3) {
    t_temp_oss_ << VarName() << ' ' << _sec_market_view_.secname() << ' ' << _half_life_ << ' ' << _threshold_vec_[0]
                << ' ' << _threshold_vec_[1] << ' ' << _threshold_vec_[2] << ' ' << _tolerance_ << ' '
                << PriceType_t_To_String(_price_type_);
  }

  if (_threshold_vec_.size() == 4) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, r_dbglogger_, "threshold vector size should be less than three");
  }

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, PriceRegime*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new PriceRegime(r_dbglogger_, r_watch_, concise_indicator_description_, _sec_market_view_, _half_life_,
                        _threshold_vec_, _tolerance_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

PriceRegime::PriceRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_, SecurityMarketView& _sec_market_view_,
                         double _half_life_, std::vector<double> _threshold_vec_, double _tolerance_,
                         PriceType_t _price_type_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      sec_market_view_(_sec_market_view_),
      half_life_(_half_life_),
      threshold_vec_(_threshold_vec_),
      tolerance_(_tolerance_),
      price_type_(_price_type_) {
  ExponentialMovingAverage* ema =
      ExponentialMovingAverage::GetUniqueInstance(dbglogger_, watch_, _sec_market_view_, half_life_, price_type_);
  ema->add_unweighted_indicator_listener(0, this);
}

void PriceRegime::WhyNotReady() {
  if (!is_ready_) {
    if (!(sec_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << sec_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void PriceRegime::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    indicator_ready_ = true;
    is_ready_ = true;
    current_price_ = _new_value_;
  } else {
    current_price_ = _new_value_;
    current_regime_index_ = threshold_vec_.size() + 1;

    for (auto i = 0u; i < threshold_vec_.size(); i++) {
      if (current_price_ < threshold_vec_[i]) {
        current_regime_index_ = i + 1;
        break;
      }
    }

    indicator_value_ = current_regime_index_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void PriceRegime::InitializeValues() { indicator_value_ = 1; }
}
