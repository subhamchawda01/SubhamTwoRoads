/**
    \file IndicatorsCode/projected_pricetype_const_pairs.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/projected_pricetype_const_pairs.hpp"

namespace HFSAT {

void ProjectedPriceTypeConstPairs::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                                     std::vector<std::string> &_ors_source_needed_vec_,
                                                     const std::vector<const char *> &r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  std::string t_source_shortcode_ = (std::string)r_tokens_[4];
  if (IndicatorUtil::IsPortfolioShortcode(t_source_shortcode_)) {
    IndicatorUtil::AddPortfolioShortCodeVec(t_source_shortcode_, _shortcodes_affecting_this_indicator_);
  } else {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, t_source_shortcode_);
  }
}

ProjectedPriceTypeConstPairs *ProjectedPriceTypeConstPairs::GetUniqueInstance(
    DebugLogger &t_dbglogger_, const Watch &r_watch_, const std::vector<const char *> &r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _source_shortcode_(can't be a portfolio)
  // _fractional_seconds_ _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           std::string(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

ProjectedPriceTypeConstPairs *ProjectedPriceTypeConstPairs::GetUniqueInstance(
    DebugLogger &t_dbglogger_, const Watch &r_watch_, const SecurityMarketView &_dep_market_view_,
    std::string _source_shortcode_, double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _source_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ProjectedPriceTypeConstPairs *> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    std::vector<SecurityMarketView *> t_indep_market_view_vec_;

    if (IndicatorUtil::IsPortfolioShortcode(_source_shortcode_)) {
      IndicatorUtil::GetPortfolioSMVVec(_source_shortcode_, t_indep_market_view_vec_);
    } else {
      t_indep_market_view_vec_.push_back(
          ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_source_shortcode_));
    }
    t_indep_market_view_vec_.erase(
        std::remove(t_indep_market_view_vec_.begin(), t_indep_market_view_vec_.end(), &_dep_market_view_),
        t_indep_market_view_vec_.end());
    concise_indicator_description_map_[concise_indicator_description_] =
        new ProjectedPriceTypeConstPairs(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                         t_indep_market_view_vec_, _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ProjectedPriceTypeConstPairs::ProjectedPriceTypeConstPairs(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                           const std::string &concise_indicator_description_,
                                                           const SecurityMarketView &_dep_market_view_,
                                                           std::vector<SecurityMarketView *> &_indep_market_view_vec_,
                                                           double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      price_type_(_price_type_),
      moving_avg_price_vec_(1 + _indep_market_view_vec_.size(), 0),

      last_price_vec_(1 + _indep_market_view_vec_.size(), 0),
      volatility_factor_vec_(1 + _indep_market_view_vec_.size(), 1),
      last_new_page_msecs_vec_(1 + _indep_market_view_vec_.size(), 0),
      is_ready_vec_(1 + _indep_market_view_vec_.size(), false),
      min_price_increment_vec_(1 + _indep_market_view_vec_.size(), 1) {
  if (indep_market_view_vec_.empty()) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " has emtpy source " << std::endl;
    exit(1);
  }
  trend_history_msecs_ = std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  if (!dep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << " to dep " << std::endl;
    exit(1);
  }

  SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode(),
                                         std::max(100 * 1000, trend_history_msecs_),
                                         0.1)->AddSlowStdevCalculatorListener(this);

  security_id_to_idx_map_[dep_market_view_.security_id()] = 0;
  min_price_increment_vec_[0] = dep_market_view_.min_price_increment();
  volatility_factor_vec_[0] = dep_market_view_.min_price_increment();
  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    security_id_to_idx_map_[indep_market_view_vec_[i]->security_id()] = i + 1;
    min_price_increment_vec_[i + 1] = indep_market_view_vec_[i]->min_price_increment();
    volatility_factor_vec_[i + 1] = indep_market_view_vec_[i]->min_price_increment();
    if (!indep_market_view_vec_[i]->subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
      exit(1);
    }

    SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, (indep_market_view_vec_[i])->shortcode(),
                                           std::max(100 * 1000, trend_history_msecs_),
                                           0.1)->AddSlowStdevCalculatorListener(this);
  }
}

void ProjectedPriceTypeConstPairs::OnStdevUpdate(const unsigned int _security_id_, const double &_new_stdev_value_) {
  int update_idx_ = security_id_to_idx_map_[_security_id_];
  volatility_factor_vec_[update_idx_] = _new_stdev_value_;
}

void ProjectedPriceTypeConstPairs::OnMarketUpdate(const unsigned int t_security_id_,
                                                  const MarketUpdateInfo &cr_market_update_info_) {
  /*
  if(security_id_to_idx_map_.find(t_security_id_) == security_id_to_idx_map_.end())
  {
    std::cerr << typeid ( *this ).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
  << " got update from unknown secid : " << t_security_id_ << " secname : " <<
  (SecurityNameIndexer::GetUniqueInstance()).GetSecurityNameFromId(t_security_id_) << std::endl;
    return;
  }
  */

  int update_idx_ = security_id_to_idx_map_[t_security_id_];

  double current_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);

  if (!is_ready_vec_[update_idx_]) {
    InitializeValues(update_idx_);

    is_ready_vec_[update_idx_] = true;
    is_ready_ = AreAllReady();
  } else {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_vec_[update_idx_] < page_width_msecs_) {
      moving_avg_price_vec_[update_idx_] += inv_decay_sum_ * (current_price_ - last_price_vec_[update_idx_]);
    } else {
      int num_pages_to_add_ =
          (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_vec_[update_idx_]) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues(update_idx_);
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_price_vec_[update_idx_] =
              (current_price_ * inv_decay_sum_) + (moving_avg_price_vec_[update_idx_] * decay_vector_[1]);
        } else {
          moving_avg_price_vec_[update_idx_] =
              (current_price_ * inv_decay_sum_) +
              (last_price_vec_[update_idx_] * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_avg_price_vec_[update_idx_] * decay_vector_[num_pages_to_add_]);
        }

        last_new_page_msecs_vec_[update_idx_] += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_price_vec_[update_idx_] = current_price_;
  }

  if (is_ready_ && !data_interrupted_) {
    // proj_price = moving_dep_price_ + (curr_source_price_-moving_source_price_) * (scale)
    update_idx_ = 1;
    double projected_price_ =
        moving_avg_price_vec_[0] +
        (last_price_vec_[1] - moving_avg_price_vec_[1]) * (volatility_factor_vec_[0] / volatility_factor_vec_[1]);
    indicator_value_ =
        projected_price_ - last_price_vec_[0];  // projected_price - curr_dep_price(same as last_price_vec_[0])

    if (std::isnan(indicator_value_)) {
      std::cerr << __PRETTY_FUNCTION__ << " nan " << std::endl;
      // exit ( 1 );
      // indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void ProjectedPriceTypeConstPairs::InitializeValues(int _update_idx_) {
  if (_update_idx_ == 0) {
    last_price_vec_[_update_idx_] = dep_market_view_.price_from_type(price_type_);
  } else {
    last_price_vec_[_update_idx_] = indep_market_view_vec_[_update_idx_ - 1]->price_from_type(price_type_);
  }
  moving_avg_price_vec_[_update_idx_] = last_price_vec_[_update_idx_];
  volatility_factor_vec_[_update_idx_] = min_price_increment_vec_[_update_idx_];

  last_new_page_msecs_vec_[_update_idx_] =
      watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

void ProjectedPriceTypeConstPairs::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                           const int msecs_since_last_receive_) {
  if (dep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    is_ready_vec_[0] = false;

  } else {
    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      if (indep_market_view_vec_[i]->security_id() == _security_id_) {
        data_interrupted_ = true;
        is_ready_vec_[i + 1] = false;
        break;
      }
    }
  }

  is_ready_ = false;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void ProjectedPriceTypeConstPairs::OnMarketDataResumed(const unsigned int _security_id_) {
  if (dep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;  // this can be set false even if some sources yet not resumed/ready but is_ready_ would
                                // always be consistent
  } else {
    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      if (indep_market_view_vec_[i]->security_id() == _security_id_) {
        data_interrupted_ = false;
        return;
      }
    }
  }
}

bool ProjectedPriceTypeConstPairs::AreAllReady() { return VectorUtils::CheckAllForValue(is_ready_vec_, true); }
}
