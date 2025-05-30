/**
    \file IndicatorsCode/returns_beta_computed_serban.cpp

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

#include "dvctrade/Indicators/returns_beta_computed_serban.hpp"

namespace HFSAT {

void ReturnsBetaComputedSerban::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                                  std::vector<std::string> &_ors_source_needed_vec_,
                                                  const std::vector<const char *> &r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

ReturnsBetaComputedSerban *ReturnsBetaComputedSerban::GetUniqueInstance(DebugLogger &t_dbglogger_,
                                                                        const Watch &r_watch_,
                                                                        const std::vector<const char *> &r_tokens_,
                                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_dep_market_view_ t_indep_market_view_ t_fractional_seconds_
  // t_indep_trend_momentum_fractional_seconds_ t_indep_trend_momentum_factor_ t_spread_reversion_factor_
  // t_spread_trend_momentum_fractional_seconds_ t_spread_trend_momentum_factor_ t_price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), atof(r_tokens_[6]), atof(r_tokens_[7]), atof(r_tokens_[8]),
                           atof(r_tokens_[9]), atof(r_tokens_[10]), StringToPriceType_t(r_tokens_[11]));
}

ReturnsBetaComputedSerban *ReturnsBetaComputedSerban::GetUniqueInstance(
    DebugLogger &t_dbglogger_, const Watch &r_watch_, SecurityMarketView &t_dep_market_view_,
    SecurityMarketView &t_indep_market_view_, double t_fractional_seconds_,
    double t_indep_trend_momentum_fractional_seconds_, double t_indep_trend_momentum_factor_,
    double t_spread_reversion_factor_, double t_spread_trend_momentum_fractional_seconds_,
    double t_spread_trend_momentum_factor_, PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << t_indep_market_view_.secname() << ' '
              << t_fractional_seconds_ << ' ' << t_indep_trend_momentum_fractional_seconds_ << ' '
              << t_indep_trend_momentum_factor_ << ' ' << t_spread_reversion_factor_ << ' '
              << t_spread_trend_momentum_fractional_seconds_ << ' ' << t_spread_trend_momentum_factor_ << ' '
              << t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ReturnsBetaComputedSerban *> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new ReturnsBetaComputedSerban(
        t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_, t_indep_market_view_,
        t_fractional_seconds_, t_indep_trend_momentum_fractional_seconds_, t_indep_trend_momentum_factor_,
        t_spread_reversion_factor_, t_spread_trend_momentum_fractional_seconds_, t_spread_trend_momentum_factor_,
        t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ReturnsBetaComputedSerban::ReturnsBetaComputedSerban(
    DebugLogger &t_dbglogger_, const Watch &r_watch_, const std::string &concise_indicator_description_,
    SecurityMarketView &t_dep_market_view_, SecurityMarketView &t_indep_market_view_, double t_fractional_seconds_,
    double t_indep_trend_momentum_fractional_seconds_, double t_indep_trend_momentum_factor_,
    double t_spread_reversion_factor_, double t_spread_trend_momentum_fractional_seconds_,
    double t_spread_trend_momentum_factor_, PriceType_t t_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_market_view_(t_indep_market_view_),
      dep_price_trend_(0),
      indep_price_trend_(0),
      returns_beta_(0.0),
      current_projected_trend_(0),
      p_dep_indicator_(NULL),
      p_indep_indicator_(NULL),
      current_projected_spread_(0),
      indep_trend_momentum_factor_(t_indep_trend_momentum_factor_),
      short_term_indep_trend_indicator_(SimpleReturns::GetUniqueInstance(
          t_dbglogger_, r_watch_, t_indep_market_view_, t_indep_trend_momentum_fractional_seconds_, t_price_type_)),
      short_term_indep_trend_(0),
      current_dep_price_(0.0),
      spread_reversion_factor_(t_spread_reversion_factor_),
      spread_trend_momentum_factor_(t_spread_trend_momentum_factor_),
      moving_avg_spread_(0),
      last_spread_recorded_(0) {
  if (!dep_market_view_.subscribe_price_type(this, t_price_type_)) {
    PriceType_t t_error_price_type_ = t_price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  trend_history_msecs_ = std::max(20, (int)round(1000 * t_spread_trend_momentum_fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;

  SetTimeDecayWeights();

  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  bool returns_beta_info_found_ = false;
  returns_beta_info_found_ = InitializeBetaValues();

  if (!returns_beta_info_found_) {
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    p_dep_indicator_ = SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_,
                                                        t_fractional_seconds_, t_price_type_);
    if (p_dep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    p_indep_indicator_ = SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, t_indep_market_view_,
                                                          t_fractional_seconds_, t_price_type_);
    if (p_indep_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }

    p_dep_indicator_->add_unweighted_indicator_listener(1u, this);
    p_indep_indicator_->add_unweighted_indicator_listener(2u, this);
    short_term_indep_trend_indicator_->add_unweighted_indicator_listener(3u, this);
  }
}

void ReturnsBetaComputedSerban::OnIndicatorUpdate(const unsigned int &t_indicator_index_,
                                                  const double &t_new_indicator_value_) {
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (!data_interrupted_) {
      switch (t_indicator_index_) {
        case 1u: {
          dep_price_trend_ = t_new_indicator_value_;
        } break;
        case 2u: {
          indep_price_trend_ = t_new_indicator_value_;
          current_projected_trend_ = indep_price_trend_ * returns_beta_;
        } break;
        case 3u: {
          short_term_indep_trend_ = t_new_indicator_value_;
        } break;
      }

      current_projected_spread_ = current_projected_trend_ - dep_price_trend_;

      {
        if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
          moving_avg_spread_ += inv_decay_sum_ * (current_projected_spread_ - last_spread_recorded_);
        } else {
          int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
          if (num_pages_to_add_ >= (int)decay_vector_.size()) {
            InitializeValues();
          } else {
            if (num_pages_to_add_ == 1) {
              moving_avg_spread_ =
                  (current_projected_spread_ * inv_decay_sum_) + (moving_avg_spread_ * decay_vector_[1]);
            } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
              moving_avg_spread_ =
                  (current_projected_spread_ * inv_decay_sum_) +
                  (last_spread_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                  (moving_avg_spread_ * decay_vector_[num_pages_to_add_]);
            }
            last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
          }
        }

        last_spread_recorded_ = current_projected_spread_;
      }

      indicator_value_ =
          current_dep_price_ * ((indep_trend_momentum_factor_ * returns_beta_ * short_term_indep_trend_) +
                                (spread_reversion_factor_ * current_projected_spread_) -
                                (spread_trend_momentum_factor_ * (current_projected_spread_ - moving_avg_spread_)));
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void ReturnsBetaComputedSerban::WhyNotReady() {
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

// market_interrupt_listener interface
void ReturnsBetaComputedSerban::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                        const int msecs_since_last_receive_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ReturnsBetaComputedSerban::OnMarketDataResumed(const unsigned int _security_id_) {
  if (_security_id_ == indep_market_view_.security_id()) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void ReturnsBetaComputedSerban::InitializeValues() {
  moving_avg_spread_ = current_projected_spread_;
  last_spread_recorded_ = current_projected_spread_;
  indicator_value_ = 0;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
}

bool ReturnsBetaComputedSerban::InitializeBetaValues() {
  std::ifstream t_returns_beta_file_;
  t_returns_beta_file_.open("/spare/local/tradeinfo/OfflineInfo/returns_beta_info.txt", std::ifstream::in);

  bool t_returns_beta_info_found_ = false;

  if (t_returns_beta_file_.is_open()) {
    const int kL1AvgBufferLen = 1024;
    char readline_buffer_[kL1AvgBufferLen];
    bzero(readline_buffer_, kL1AvgBufferLen);
    while (t_returns_beta_file_.good()) {
      bzero(readline_buffer_, kL1AvgBufferLen);
      t_returns_beta_file_.getline(readline_buffer_, kL1AvgBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() == 4) {
        std::string dep_shortcode_ = tokens_[0];
        std::string indep_shortcode_ = tokens_[1];
        if ((dep_market_view_.shortcode().compare(dep_shortcode_) == 0) &&
            (indep_market_view_.shortcode().compare(indep_shortcode_) == 0)) {
          returns_beta_ = atof(tokens_[2]);
          t_returns_beta_info_found_ = true;
        }
      }
    }
  }
  return t_returns_beta_info_found_;
}
}
