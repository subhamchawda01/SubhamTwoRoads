/**
   \file IndicatorsCode/offline_corradjusted_pairs_normalized_combo.cpp

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
#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvctrade/Indicators/offline_corradjusted_pairs_normalized_combo.hpp"

#define IMPOSSIBLY_LOW_VALUE_SUM_BETAS 0.000005

namespace HFSAT {

void OfflineCorradjustedPairsNormalizedCombo::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs,
                "OfflineCorradjustedPairsNormalizedCombo incorrect syntax. Should be INDICATOR _this_weight_ "
                "_indicator_string_ _dep_market_view_ _portfolio_descriptor_shortcode_ _fractional_seconds_ "
                "_price_type_");
  }
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

OfflineCorradjustedPairsNormalizedCombo* OfflineCorradjustedPairsNormalizedCombo::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _dep_market_view_  _portfolio_descriptor_shortcode_
  // _fractional_seconds_  _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (std::string)(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

OfflineCorradjustedPairsNormalizedCombo* OfflineCorradjustedPairsNormalizedCombo::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _dep_market_view_,
    std::string _portfolio_descriptor_shortcode_, double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineCorradjustedPairsNormalizedCombo*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    std::vector<std::string> t_shortcode_vec_;
    IndicatorUtil::GetPortfolioShortCodeVec(_portfolio_descriptor_shortcode_, t_shortcode_vec_);
    concise_indicator_description_map_[concise_indicator_description_] = new OfflineCorradjustedPairsNormalizedCombo(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _portfolio_descriptor_shortcode_,
        t_shortcode_vec_, _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineCorradjustedPairsNormalizedCombo::OfflineCorradjustedPairsNormalizedCombo(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    const SecurityMarketView& t_dep_market_view_, std::string t_portfolio_descriptor_shortcode_,
    const std::vector<std::string>& t_shortcode_vec_, double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      p_dep_price_change_indicator_(NULL),
      prev_dep_price_change_(0),
      shortcode_vec_(t_shortcode_vec_),
      normalized_weight_vec_(t_shortcode_vec_.size(), 0),
      is_ready_vec_(t_shortcode_vec_.size(), true),
      prev_value_vec_(t_shortcode_vec_.size(), 0),
      p_source_price_change_indicator_vec_(t_shortcode_vec_.size(), NULL) {
  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  bool lrdb_absent_ = false;

  std::vector<SecurityMarketView*> indep_market_view_vec_;
  (ShortcodeSecurityMarketViewMap::GetUniqueInstance())
      .GetSecurityMarketViewVec(shortcode_vec_, indep_market_view_vec_);

  std::vector<double> unnormalized_weight_vec_(shortcode_vec_.size(),
                                               0);  // initialize to 0 ... value taken if lrdb not present
  std::vector<LRInfo> current_lrinfo_vec_(shortcode_vec_.size());
  for (size_t i = 0u; i < shortcode_vec_.size(); i++) {
    if ((t_dep_market_view_.shortcode().compare(shortcode_vec_[i]) !=
         0)  // dep and source aren't the same , if so then skip this source
        && (lrdb_.LRCoeffPresent(t_dep_market_view_.shortcode(), shortcode_vec_[i])))  // lrdb has data for this
    {
      lrdb_absent_ = false;  // at least one present

      current_lrinfo_vec_[i] = lrdb_.GetLRCoeff(dep_market_view_.shortcode(), shortcode_vec_[i]);
      unnormalized_weight_vec_[i] =
          current_lrinfo_vec_[i].lr_coeff_;  // / std::max ( 0.05, fabs ( current_lrinfo_vec_[i].lr_correlation_ ) ) ) ;
    }
  }

  double sum_unnormalized_weights_ = 0;
  for (size_t i = 0u; i < unnormalized_weight_vec_.size(); i++) {
    if (fabs(unnormalized_weight_vec_[i]) >= IMPOSSIBLY_LOW_VALUE_SUM_BETAS) {
      sum_unnormalized_weights_ +=
          fabs(current_lrinfo_vec_[i].lr_correlation_);  // sum and normalize by non stdev elements
    }
  }
  // if ( sum_unnormalized_weights_ < IMPOSSIBLY_LOW_VALUE_SUM_BETAS )
  //   {
  // 	lrdb_absent_ = true;
  //   }
  sum_unnormalized_weights_ = std::max(IMPOSSIBLY_LOW_VALUE_SUM_BETAS, sum_unnormalized_weights_);

  for (size_t i = 0u; i < unnormalized_weight_vec_.size(); i++) {
    normalized_weight_vec_[i] = (unnormalized_weight_vec_[i] / sum_unnormalized_weights_);
  }

  if (lrdb_absent_) {
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    p_dep_price_change_indicator_ =
        SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(), _fractional_seconds_, _price_type_);
    if (p_dep_price_change_indicator_ == NULL) {
      indicator_value_ = 0;
      is_ready_ = true;
      return;
    }
    p_dep_price_change_indicator_->add_unweighted_indicator_listener(0u, this);

    for (size_t i = 0u; i < shortcode_vec_.size(); i++) {
      p_source_price_change_indicator_vec_[i] = NULL;  // DEFAULT
      is_ready_vec_[i] = true;                         // DEFAULT

      if ((indep_market_view_vec_[i] != NULL) &&
          (fabs(normalized_weight_vec_[i]) > IMPOSSIBLY_LOW_VALUE_SUM_BETAS))  // TODO : checking for non zero
                                                                               // normalized_weight_vec_[i] here but not
                                                                               // sure if weights can change later
      {
        p_source_price_change_indicator_vec_[i] = SimpleTrend::GetUniqueInstance(
            t_dbglogger_, r_watch_, indep_market_view_vec_[i]->shortcode(), _fractional_seconds_, _price_type_);
        p_source_price_change_indicator_vec_[i]->add_unweighted_indicator_listener(i + 1, this);
        is_ready_vec_[i] = (p_source_price_change_indicator_vec_[i]->IsIndicatorReady());
      }
    }
  }

  if (!is_ready_) {
    is_ready_ = (p_dep_price_change_indicator_->IsIndicatorReady() && AreAllReady());
    if (is_ready_) {
      indicator_value_ = 0;
    }
  }
}

void OfflineCorradjustedPairsNormalizedCombo::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                                                const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (t_indicator_index_ > 0) {  // this is a source
      is_ready_vec_[t_indicator_index_ - 1] = true;
    }

    if (p_dep_price_change_indicator_->IsIndicatorReady() && AreAllReady()) {
      is_ready_ = true;
      indicator_value_ = 0;

      bool dummy_bool = true;  // just becasue following call needs it.
      prev_dep_price_change_ = p_dep_price_change_indicator_->indicator_value(dummy_bool);
      indicator_value_ = -prev_dep_price_change_;
      for (size_t i = 0u; i < p_source_price_change_indicator_vec_.size(); i++) {
        if (p_source_price_change_indicator_vec_[i] != NULL) {
          prev_value_vec_[i] = p_source_price_change_indicator_vec_[i]->indicator_value(dummy_bool);
          indicator_value_ += (normalized_weight_vec_[i] * prev_value_vec_[i]);
        } else {
          prev_value_vec_[i] = 0;
        }
      }
    }
  } else {
    if (t_indicator_index_ == 0) {
      indicator_value_ -= (t_new_indicator_value_ - prev_dep_price_change_);
      prev_dep_price_change_ = t_new_indicator_value_;
    } else {
      int this_source_index_ = t_indicator_index_ - 1;
      indicator_value_ +=
          normalized_weight_vec_[this_source_index_] * (t_new_indicator_value_ - prev_value_vec_[this_source_index_]);
      prev_value_vec_[this_source_index_] = t_new_indicator_value_;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflineCorradjustedPairsNormalizedCombo::WhyNotReady() {
  if (!is_ready_) {
    if (!p_dep_price_change_indicator_->IsIndicatorReady()) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " SimpleTrend not ready " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    } else {
      for (size_t i = 0u; i < is_ready_vec_.size(); i++) {
        if (!is_ready_vec_[i]) {
          DBGLOG_TIME_CLASS << " Source " << i << " "
                            << p_source_price_change_indicator_vec_[i]->concise_indicator_description() << " not ready "
                            << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
          if (p_source_price_change_indicator_vec_[i] != NULL) {
            p_source_price_change_indicator_vec_[i]->WhyNotReady();
          } else {
            DBGLOG_TIME_CLASS << " Source " << i << " not ready but indicator NULL " << DBGLOG_ENDL_FLUSH;
            DBGLOG_DUMP;
          }
        }
      }
    }
  }
}

// market_interrupt_listener interface
void OfflineCorradjustedPairsNormalizedCombo::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                      const int msecs_since_last_receive_) {
  // if ( _security_id_ == indep_market_view_.security_id() )
  //   {
  //     data_interrupted_ = true;
  //     indicator_value_ = 0;
  //     NotifyIndicatorListeners ( indicator_value_ ) ;
  //   }
}

void OfflineCorradjustedPairsNormalizedCombo::OnMarketDataResumed(const unsigned int _security_id_) {
  // if ( _security_id_ == indep_market_view_.security_id() )
  //   {
  //     InitializeValues ( ) ;
  //     data_interrupted_ = false;
  //   }
}

bool OfflineCorradjustedPairsNormalizedCombo::AreAllReady() {
  return VectorUtils::CheckAllForValue(is_ready_vec_, true);
}

void OfflineCorradjustedPairsNormalizedCombo::UpdateLRInfo() {
  if ((last_lrinfo_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_lrinfo_updated_msecs_ > TENMINUTESMSECS)) {
    UpdateWeights();
    last_lrinfo_updated_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TENMINUTESMSECS);
  }
}

void OfflineCorradjustedPairsNormalizedCombo::UpdateWeights() {
  std::vector<double> unnormalized_weight_vec_(shortcode_vec_.size(),
                                               0);  // initialize to 0 ... value taken if lrdb not present
  std::vector<LRInfo> current_lrinfo_vec_(shortcode_vec_.size());
  for (size_t i = 0u; i < shortcode_vec_.size(); i++) {
    if ((dep_market_view_.shortcode().compare(shortcode_vec_[i]) !=
         0)  // dep and source aren't the same , if so then skip this source
        && (lrdb_.LRCoeffPresent(dep_market_view_.shortcode(), shortcode_vec_[i])))  // lrdb has data for this
    {
      current_lrinfo_vec_[i] = lrdb_.GetLRCoeff(dep_market_view_.shortcode(), shortcode_vec_[i]);
      unnormalized_weight_vec_[i] =
          current_lrinfo_vec_[i].lr_coeff_;  // / std::max ( 0.05, fabs ( current_lrinfo_vec_[i].lr_correlation_ ) ) ) ;
    }
  }

  double sum_unnormalized_weights_ = 0;
  for (size_t i = 0u; i < unnormalized_weight_vec_.size(); i++) {
    if (fabs(unnormalized_weight_vec_[i]) > IMPOSSIBLY_LOW_VALUE_SUM_BETAS) {
      sum_unnormalized_weights_ +=
          fabs(current_lrinfo_vec_[i].lr_correlation_);  // sum and normalize by non stdev elements
    }
  }
  sum_unnormalized_weights_ =
      std::max(IMPOSSIBLY_LOW_VALUE_SUM_BETAS, sum_unnormalized_weights_);  // to prevent divide by 0 in the next step

  if (normalized_weight_vec_.size() == unnormalized_weight_vec_.size()) {  // just a harmless check
    for (size_t i = 0u; i < unnormalized_weight_vec_.size(); i++) {
      normalized_weight_vec_[i] = (unnormalized_weight_vec_[i] / sum_unnormalized_weights_);
    }
  }
}
}
