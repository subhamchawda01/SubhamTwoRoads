/*
 * regime_slope_indicator.cpp
 *
 *  Created on: 11-Nov-2015
 *      Author: raghuram


\file IndicatorsCode /
    regime_online_offline_stedv_ratio.cpp

    \author : (c)Copyright Two Roads Technological Solutions Pvt Ltd 2011 Address : Suite No 353,
    Evoma, #14, Bhattarhalli, Old Madras Road, Near Garden City College, KR Puram, Bangalore 560049,
    India +
        91 80 4190 3551
        */
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/regime_slope.hpp"

namespace HFSAT {

void RegimeSlope::CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& r_ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(r_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(r_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
  VectorUtils::UniqueVectorAdd(r_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);
}

RegimeSlope* RegimeSlope::GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 9) {
    ExitVerbose(
        kModelCreationIndicatorLineLessArgs, r_dbglogger_,
        "INDICATOR weight RegimeSlope _indep1_market_view_ _indep2_market_view_ _indep3_market_view_ _threshold_"
        "_tolerance_  price_type_");
  }

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(r_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           atof(r_tokens_[6]), atof(r_tokens_[7]), StringToPriceType_t(r_tokens_[8]));
}

RegimeSlope* RegimeSlope::GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                            SecurityMarketView& _indep1_market_view_,
                                            SecurityMarketView& _indep2_market_view_,
                                            SecurityMarketView& _indep3_market_view_, double t_threshold_,
                                            double t_tolerance_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep1_market_view_.secname() << ' ' << _indep2_market_view_.secname() << ' '
              << _indep3_market_view_.secname() << ' ' << t_threshold_ << ' ' << t_tolerance_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RegimeSlope*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RegimeSlope(r_dbglogger_, r_watch_, concise_indicator_description_, _indep1_market_view_,
                        _indep2_market_view_, _indep3_market_view_, t_threshold_, t_tolerance_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RegimeSlope::RegimeSlope(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_, SecurityMarketView& _indep1_market_view_,
                         SecurityMarketView& _indep2_market_view_, SecurityMarketView& _indep3_market_view_,
                         double t_threshold_, double t_tolerance_, PriceType_t _price_type_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep1_market_view_(_indep1_market_view_),
      indep2_market_view_(_indep2_market_view_),
      indep3_market_view_(_indep3_market_view_),
      price_type_(_price_type_),
      threshold_(t_threshold_),
      tolerance_(t_tolerance_) {
  CMVFL1Price* first_maturity_price_ =
      CMVFL1Price::GetUniqueInstance(dbglogger_, watch_, _indep1_market_view_, _indep2_market_view_, price_type_);
  first_maturity_price_->add_unweighted_indicator_listener(0, this);

  CMVFL1Price* second_maturity_price_ =
      CMVFL1Price::GetUniqueInstance(dbglogger_, watch_, _indep2_market_view_, _indep3_market_view_, price_type_);
  second_maturity_price_->add_unweighted_indicator_listener(1, this);
}

void RegimeSlope::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep1_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep1_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep2_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep2_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep3_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep3_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void RegimeSlope::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  // 1 we want to use model file with which has less/no indep's indicators
  // 2 we want to use model file with decent/high indep's indicators
  if (!is_ready_) {
    if (_indicator_index_ == 0) {
      indicator1_ready_ = true;
    }

    if (_indicator_index_ == 1) {
      indicator2_ready_ = true;
    }

    if (indicator1_ready_ && indicator2_ready_) {
      InitializeValues();
      is_ready_ = true;
    }
  } else {
    if (_indicator_index_ == 0) {
      first_maturity_price_ = _new_value_;
    }

    if (_indicator_index_ == 1) {
      second_maturity_price_ = _new_value_;
    }

    if (indicator_value_ == 1 && (first_maturity_price_ - second_maturity_price_) >= threshold_ + tolerance_) {
      indicator_value_ = 2;
    } else if (indicator_value_ == 2 && (first_maturity_price_ - second_maturity_price_) <= threshold_ - tolerance_) {
      indicator_value_ = 1;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeSlope::InitializeValues() { indicator_value_ = 1; }
}
