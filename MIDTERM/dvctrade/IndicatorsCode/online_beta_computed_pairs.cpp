/**
    \file IndicatorsCode/online_beta_computed_pairs.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_beta_computed_pairs.hpp"

namespace HFSAT {

void OnlineBetaComputedPairs::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                std::vector<std::string>& _ors_source_needed_vec_,
                                                const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OnlineBetaComputedPairs* OnlineBetaComputedPairs::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                    const std::vector<const char*>& r_tokens_,
                                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_
  // _beta_fractional_secs_ _price_type_
  if (r_tokens_.size() < 8) {
    std::cerr << "Insufficient arguments to INDICATOR OnlineBetaComputedPairs, correct syntax : _this_weight_ "
                 "_indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_ _beta_fractional_secs_ "
                 "_price_type_\n";
    exit(1);
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

OnlineBetaComputedPairs* OnlineBetaComputedPairs::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                    SecurityMarketView& _dep_market_view_,
                                                                    SecurityMarketView& _indep_market_view_,
                                                                    double _fractional_seconds_,
                                                                    double _beta_fractional_secs_,
                                                                    PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _fractional_seconds_ << ' ' << _beta_fractional_secs_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineBetaComputedPairs*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OnlineBetaComputedPairs(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                    _indep_market_view_, _fractional_seconds_, _beta_fractional_secs_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineBetaComputedPairs::OnlineBetaComputedPairs(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                 const std::string& concise_indicator_description_,
                                                 SecurityMarketView& _dep_market_view_,
                                                 SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                                 double _beta_fractional_secs_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      dep_price_trend_(0),
      indep_price_trend_(0),
      beta_value_(0),
      beta_indicator_(NULL),
      current_projected_trend_(0),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL) {
  beta_indicator_ = OnlineBeta::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_, _indep_market_view_,
                                                  std::max(ONLINE_BETA_DURATION, _beta_fractional_secs_), _price_type_);
  beta_indicator_->add_unweighted_indicator_listener(0, this);

  if (dep_market_view_.security_id() == indep_market_view_.security_id()) {  // added this since for convenience one
                                                                             // could add a combo or portfolio as source
                                                                             // with a security
    // that is also the dependant
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    p_dep_indicator_ =
        SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_.shortcode(), _fractional_seconds_, _price_type_);
    if (p_dep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    p_indep_indicator_ =
        SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_.shortcode(), _fractional_seconds_, _price_type_);
    if (p_indep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    p_dep_indicator_->add_unweighted_indicator_listener(1u, this);
    p_indep_indicator_->add_unweighted_indicator_listener(2u, this);
  }
}

void OnlineBetaComputedPairs::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
    }
  } else {
    if (!data_interrupted_) {
      switch (_indicator_index_) {
        case 0u: {
          beta_value_ = _new_value_;
        } break;
        case 1u: {
          dep_price_trend_ = _new_value_;
        } break;
        case 2u: {
          indep_price_trend_ = _new_value_;
        } break;
      }

      indicator_value_ = beta_value_ * indep_price_trend_ - dep_price_trend_;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void OnlineBetaComputedPairs::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void OnlineBetaComputedPairs::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                      const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OnlineBetaComputedPairs::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    dep_price_trend_ = 0;
    indep_price_trend_ = 0;
    beta_value_ = 0;
    data_interrupted_ = false;
  }
}
}
