/**
   \file IndicatorsCode/offline_computed_pairs_mult.cpp

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
#include "dvctrade/Indicators/offline_computed_returns_mult.hpp"
#include "dvctrade/Indicators/simple_returns.hpp"

namespace HFSAT {

void OfflineComputedReturnsMult::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                   std::vector<std::string>& _ors_source_needed_vec_,
                                                   const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  // IndicatorUtil::AddReturnsPortfolioShortCodeVec ( std::string ( r_tokens_[4] ) ,
  // _shortcodes_affecting_this_indicator_ ) ;
  OfflineReturnsPairsDB::CollectSourceShortcodes(r_tokens_[3], _shortcodes_affecting_this_indicator_);
}

OfflineComputedReturnsMult* OfflineComputedReturnsMult::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                          const Watch& r_watch_,
                                                                          const std::vector<const char*>& r_tokens_,
                                                                          PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ t_dep_market_view_ t_indep_market_view_ t_fractional_seconds_
  // t_price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

OfflineComputedReturnsMult* OfflineComputedReturnsMult::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                          const Watch& r_watch_,
                                                                          SecurityMarketView& t_dep_market_view_,
                                                                          double t_fractional_seconds_,
                                                                          PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << t_fractional_seconds_ << ' '
              << t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflineComputedReturnsMult*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OfflineComputedReturnsMult(t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_,
                                       t_fractional_seconds_, t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflineComputedReturnsMult::OfflineComputedReturnsMult(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                       const std::string& concise_indicator_description_,
                                                       SecurityMarketView& t_dep_market_view_,
                                                       double t_fractional_seconds_, PriceType_t t_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      dep_price_trend_(0),
      indep_price_trend_(0),
      price_type_(t_price_type_),
      ret_pairs_db_(OfflineReturnsPairsDB::GetUniqueInstance(t_dbglogger_, r_watch_)),
      last_lrinfo_updated_msecs_(0),
      current_returns_info_(),
      current_projection_multiplier_(0.0),
      current_projected_trend_(0),
      p_dep_indicator_(NULL),
      p_indep_indicator_vec_() {
  watch_.subscribe_BigTimePeriod(this);  // for UpdateLRInfo and updating volume adjustment

  OfflineReturnsPairsDB::CollectSourceShortcodes(dep_market_view_.shortcode(), source_shortcode_vec_);

  Initialize();
  {
    p_dep_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, t_dep_market_view_.shortcode(), t_fractional_seconds_,
                                                      t_price_type_);
    if (p_dep_indicator_ == NULL) {
      std::cerr << __FUNCTION__ << " Could not created returns Indicator " << t_dep_market_view_.shortcode() << " "
                << t_fractional_seconds_ << " " << PriceType_t(t_price_type_) << std::endl;
      ExitVerbose(kExitErrorCodeGeneral);
    }

    p_dep_indicator_->add_unweighted_indicator_listener(1u, this);

    for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
      p_indep_indicator_vec_.push_back(SimpleReturns::GetUniqueInstance(
          t_dbglogger_, r_watch_, *indep_market_view_vec_[i], t_fractional_seconds_, t_price_type_));
      if (p_indep_indicator_vec_[i] == NULL) {
        std::cerr << __FUNCTION__ << " Could not created returns Indicator " << indep_market_view_vec_[i]->shortcode()
                  << " " << t_fractional_seconds_ << " " << PriceType_t(t_price_type_) << std::endl;
        ExitVerbose(kExitErrorCodeGeneral);
      }
      p_indep_indicator_vec_[i]->add_unweighted_indicator_listener(i + 2u, this);
      // std::cerr << " Indep: " << source_shortcode_vec_[i] << " weithg: " << source_weight_vec_[i] << "idx_ " << i <<"
      // updateidx_: " << i+2u << std::endl;
    }
  }
}

void OfflineComputedReturnsMult::OnIndicatorUpdate(const unsigned int& t_indicator_index_,
                                                   const double& t_new_indicator_value_) {
  if (!is_ready_) {
    if (watch_.tv().tv_sec != 0 &&
        (last_lrinfo_updated_msecs_ == 0 || watch_.msecs_from_midnight() - last_lrinfo_updated_msecs_ > 100000)) {
      portfolio_discriptor_shortcode_ = ret_pairs_db_.GetSourcePortfolioForShortcode(dep_market_view_.shortcode());
      LoadSourceWeights(portfolio_discriptor_shortcode_);
      last_lrinfo_updated_msecs_ = watch_.msecs_from_midnight();
    }

    if (dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
    }
    for (unsigned idx_ = 0; idx_ < indep_market_view_vec_.size(); idx_++) {
      is_ready_ = is_ready_ && ((!readiness_required_vec_[idx_]) || indep_market_view_vec_[idx_]->is_ready_complex(2));
    }

  } else {
    if (!data_interrupted_) {
      switch (t_indicator_index_) {
        case 1u: {
          dep_price_trend_ = t_new_indicator_value_;
        } break;
        default: {
          int indep_index_ = t_indicator_index_ - 2;  // indices are as follows ( 1 ( dep), 2, 3... )
          indep_price_trend_ = t_new_indicator_value_;
          current_projected_trend_ +=
              (indep_price_trend_ - prev_value_vec_[indep_index_]) * source_weight_vec_[indep_index_];
          prev_value_vec_[indep_index_] = indep_price_trend_;
          // std::cerr << " indicatorIdx_ " << t_indicator_index_ << " indep_index_ " << indep_index_ << "
          // source_shortcode_vec_" << source_shortcode_vec_ [ indep_index_ ] << std::endl ;
        } break;
      }

      double current_dep_price_ = dep_market_view_.price_from_type(price_type_);
      indicator_value_ = current_projected_trend_ * (current_dep_price_ - dep_price_trend_) - dep_price_trend_;

      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void OfflineComputedReturnsMult::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    for (unsigned idx_ = 0; idx_ < indep_market_view_vec_.size(); idx_++) {
      if (!(indep_market_view_vec_[idx_]->is_ready_complex(2))) {
        DBGLOG_TIME_CLASS << indep_market_view_vec_[idx_]->secname() << " is_ready_complex(2) = false "
                          << DBGLOG_ENDL_FLUSH;
      }
    }
    DBGLOG_DUMP;
  }
}

// market_interrupt_listener interface
void OfflineComputedReturnsMult::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                         const int msecs_since_last_receive_) {
  for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
    if (_security_id_ == indep_market_view_vec_[i]->security_id()) {
      // data interrupted for this security id, reload the weights ?
      source_weight_vec_[i] = 0.0;
      RecomputeWeights(i, true);
    }
  }
  if (data_interrupted_) {
    indicator_value_ = true;
  }
}

void OfflineComputedReturnsMult::OnMarketDataResumed(const unsigned int _security_id_) {
  for (unsigned i = 0; i < indep_market_view_vec_.size(); i++) {
    if (_security_id_ == indep_market_view_vec_[i]->security_id()) {
      RecomputeWeights(i, false);
      data_interrupted_ = false;
    }
  }
}

// TODO Tell me why

void OfflineComputedReturnsMult::RecomputeWeights(const int updated_idx_, bool t_remove_) {
  std::string t_new_portfolio_discriptor_shortcode_ = "";
  for (unsigned i = 0; i < source_shortcode_vec_.size(); i++) {
    if (!(i == (unsigned)updated_idx_ && t_remove_)) {
      std::string shc_str_ = source_shortcode_vec_[i];
      if (shc_str_.find("_") != std::string::npos) shc_str_.replace(shc_str_.find("_"), 1, "");
      if (shc_str_.find(" ") != std::string::npos) shc_str_.replace(shc_str_.find(" "), 1, "");
      // std::replace ( shc_str_.begin () , shc_str_.end(), '_', '');
      // std::replace ( shc_str_.begin (), shc_str_.end(), ' ', '');

      t_new_portfolio_discriptor_shortcode_ = t_new_portfolio_discriptor_shortcode_ + shc_str_;
    }
  }

  DBGLOG_CLASS_FUNC_LINE << " Loading weights for " << t_new_portfolio_discriptor_shortcode_ << DBGLOG_ENDL_FLUSH;
  if (ret_pairs_db_.ReturnsCoeffPresent(dep_market_view_.shortcode(), t_new_portfolio_discriptor_shortcode_)) {
    // Only
    ReturnsInfo this_returns_info_ =
        ret_pairs_db_.GetReturnsCoeff(dep_market_view_.shortcode(), t_new_portfolio_discriptor_shortcode_);
    for (unsigned i = 0; i < this_returns_info_.shortcode_vec_.size(); i++) {
      unsigned int shc_index_ = 0;
      while (shc_index_ < source_shortcode_vec_.size()) {
        if (source_shortcode_vec_[shc_index_].compare(this_returns_info_.shortcode_vec_[i]) == 0) {
          source_weight_vec_[shc_index_] = this_returns_info_.returns_coeff_vec_[i];
          shc_index_++;
          break;
        }
        shc_index_++;
      }
      DBGLOG_CLASS_FUNC_LINE << " Source: " << this_returns_info_.shortcode_vec_[i]
                             << " Weights : " << this_returns_info_.returns_coeff_vec_[i] << DBGLOG_ENDL_FLUSH;
    }
  } else {
    DBGLOG_CLASS_FUNC_LINE << " Weights not present for this dep-portfolio price" << DBGLOG_ENDL_FLUSH;
    data_interrupted_ = true;
  }
}

void OfflineComputedReturnsMult::LoadSourceWeights(std::string t_portfolio_discriptor_shortcode_) {
  if (!ret_pairs_db_.ReturnsCoeffPresent(dep_market_view_.shortcode(), t_portfolio_discriptor_shortcode_)) {
    is_ready_ = true;
    indicator_value_ = 0;
  } else {
    ReturnsInfo this_returns_info_ =
        ret_pairs_db_.GetReturnsCoeff(dep_market_view_.shortcode(), t_portfolio_discriptor_shortcode_);
    // source_shortcode_vec_ = this_returns_info_.shortcode_vec_ ;
    // source_weight_vec_ = this_returns_info_.returns_coeff_vec_;
    DBGLOG_TIME_CLASS_FUNC_LINE << " Total number of indeps: " << source_shortcode_vec_.size() << " "
                                << t_portfolio_discriptor_shortcode_ << DBGLOG_ENDL_FLUSH;
    for (unsigned i = 0; i < this_returns_info_.shortcode_vec_.size(); i++) {
      for (unsigned j = 0; j < source_shortcode_vec_.size(); j++) {
        if (source_shortcode_vec_[j].compare(this_returns_info_.shortcode_vec_[i]) == 0) {
          source_weight_vec_[j] = this_returns_info_.returns_coeff_vec_[i];
          readiness_required_vec_[i] = true;
          DBGLOG_TIME_CLASS_FUNC_LINE << " Source " << source_shortcode_vec_[j] << " Weight: " << source_weight_vec_[j]
                                      << DBGLOG_ENDL_FLUSH;
        }
      }
    }
  }
  prev_value_vec_.resize(source_weight_vec_.size(), 0.0);
}

void OfflineComputedReturnsMult::Initialize() {
  for (unsigned i = 0; i < source_shortcode_vec_.size(); i++) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(source_shortcode_vec_[i]);
    indep_market_view_vec_.push_back(
        ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(source_shortcode_vec_[i]));
  }
  prev_value_vec_.resize(source_shortcode_vec_.size(), 0.0);
  source_weight_vec_.resize(source_shortcode_vec_.size(), 0.0);
  readiness_required_vec_.resize(source_shortcode_vec_.size(), false);
}
}
