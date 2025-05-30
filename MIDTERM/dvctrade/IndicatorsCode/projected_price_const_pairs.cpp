/**
    \file IndicatorsCode/projected_price_const_pairs.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/projected_price_const_pairs.hpp"

namespace HFSAT {

void ProjectedPriceConstPairs::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
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

ProjectedPriceConstPairs *ProjectedPriceConstPairs::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                                      const std::vector<const char *> &r_tokens_,
                                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _source_shortcode_(can be a portfolio or shortcode)
  // _fractional_seconds_ _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           std::string(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

ProjectedPriceConstPairs *ProjectedPriceConstPairs::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                                      SecurityMarketView &_dep_market_view_,
                                                                      std::string _source_shortcode_,
                                                                      double _fractional_seconds_,
                                                                      PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _source_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ProjectedPriceConstPairs *> concise_indicator_description_map_;
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
        new ProjectedPriceConstPairs(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                     t_indep_market_view_vec_, _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ProjectedPriceConstPairs::ProjectedPriceConstPairs(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                   const std::string &concise_indicator_description_,
                                                   SecurityMarketView &_dep_market_view_,
                                                   std::vector<SecurityMarketView *> &_indep_market_view_vec_,
                                                   double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_vec_(_indep_market_view_vec_),
      price_type_(_price_type_),
      moving_avg_bid_price_vec_(1 + _indep_market_view_vec_.size(), 0),
      moving_avg_ask_price_vec_(1 + _indep_market_view_vec_.size(), 0),

      last_bid_price_vec_(1 + _indep_market_view_vec_.size(), 0),
      last_ask_price_vec_(1 + _indep_market_view_vec_.size(), 0),
      volatility_factor_vec_(1 + _indep_market_view_vec_.size(), 1),
      last_best_bid_price_(0),
      last_best_ask_price_(0),
      last_best_ask_idx_(-1),
      last_best_bid_idx_(-1),
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

  if (!dep_market_view_.subscribe_price_type(this, kPriceTypeMidprice)) {
    PriceType_t t_error_price_type_ = kPriceTypeMidprice;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << " to dep " << std::endl;
    exit(1);
  }

  security_id_to_idx_map_[dep_market_view_.security_id()] = 0;
  min_price_increment_vec_[0] = dep_market_view_.min_price_increment();
  double t_dep_stdev_ = PcaWeightsManager::GetUniqueInstance().GetShortcodeStdevs(dep_market_view_.shortcode());
  if (t_dep_stdev_ <= 0) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " : dep shc has undefined stdev " << t_dep_stdev_ << std::endl;
    exit(1);
  }
  volatility_factor_vec_[0] = 1;

  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    security_id_to_idx_map_[indep_market_view_vec_[i]->security_id()] = i + 1;
    min_price_increment_vec_[i + 1] = indep_market_view_vec_[i]->min_price_increment();
    double t_indep_stdev_ =
        PcaWeightsManager::GetUniqueInstance().GetShortcodeStdevs(indep_market_view_vec_[i]->shortcode());
    if (t_indep_stdev_ <= 0) {
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " : indep shc " << indep_market_view_vec_[i]->shortcode() << " has undefined stdev "
                << t_indep_stdev_ << std::endl;
      exit(1);
    }
    volatility_factor_vec_[i + 1] = t_dep_stdev_ / t_indep_stdev_;
    // std::cerr << i+1 << " " << t_dep_stdev_ << " " << t_indep_stdev_ << " " << volatility_factor_vec_[i+1];

    if (!indep_market_view_vec_[i]->subscribe_price_type(this, kPriceTypeMidprice)) {
      PriceType_t t_error_price_type_ = kPriceTypeMidprice;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
      exit(1);
    }
  }
}

void ProjectedPriceConstPairs::OnMarketUpdate(const unsigned int t_security_id_,
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

  if (update_idx_ == 0) {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
  }

  double current_bid_price_ = cr_market_update_info_.bestbid_price_;
  double current_ask_price_ = cr_market_update_info_.bestask_price_;

  if (!is_ready_vec_[update_idx_]) {
    InitializeValues(update_idx_);

    is_ready_vec_[update_idx_] = true;
    is_ready_ = AreAllReady();
  } else {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_vec_[update_idx_] < page_width_msecs_) {
      moving_avg_bid_price_vec_[update_idx_] +=
          inv_decay_sum_ * (current_bid_price_ - last_bid_price_vec_[update_idx_]);
      moving_avg_ask_price_vec_[update_idx_] +=
          inv_decay_sum_ * (current_ask_price_ - last_ask_price_vec_[update_idx_]);
    } else {
      int num_pages_to_add_ =
          (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_vec_[update_idx_]) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues(update_idx_);
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_bid_price_vec_[update_idx_] =
              (current_bid_price_ * inv_decay_sum_) + (moving_avg_bid_price_vec_[update_idx_] * decay_vector_[1]);
          moving_avg_ask_price_vec_[update_idx_] =
              (current_ask_price_ * inv_decay_sum_) + (moving_avg_ask_price_vec_[update_idx_] * decay_vector_[1]);
        } else {
          moving_avg_bid_price_vec_[update_idx_] =
              (current_bid_price_ * inv_decay_sum_) +
              (last_bid_price_vec_[update_idx_] * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_avg_bid_price_vec_[update_idx_] * decay_vector_[num_pages_to_add_]);
          moving_avg_ask_price_vec_[update_idx_] =
              (current_ask_price_ * inv_decay_sum_) +
              (last_ask_price_vec_[update_idx_] * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_avg_ask_price_vec_[update_idx_] * decay_vector_[num_pages_to_add_]);
        }

        last_new_page_msecs_vec_[update_idx_] += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_bid_price_vec_[update_idx_] = current_bid_price_;
    last_ask_price_vec_[update_idx_] = current_ask_price_;
  }

  if (is_ready_ && !data_interrupted_) {
    double projected_bid_price_;
    double projected_ask_price_;

    if (update_idx_ == 0) {
      projected_bid_price_ = current_bid_price_;
      projected_ask_price_ = current_ask_price_;
    } else {
      // proj_price = moving_dep_price_ + (curr_source_price_-moving_source_price_) * (scale)
      projected_bid_price_ =
          moving_avg_bid_price_vec_[0] +
          (current_bid_price_ - moving_avg_bid_price_vec_[update_idx_]) * (volatility_factor_vec_[update_idx_]);
      projected_ask_price_ =
          moving_avg_ask_price_vec_[0] +
          (current_ask_price_ - moving_avg_ask_price_vec_[update_idx_]) * (volatility_factor_vec_[update_idx_]);
    }

    // std::cerr << volatility_factor_vec_[0] << " " << volatility_factor_vec_[1] << " " <<
    // volatility_factor_vec_[0]/volatility_factor_vec_[update_idx_] << std::endl;

    // computing best_projected_prices
    if ((update_idx_ > 0) && (last_best_bid_idx_ != update_idx_) &&
        (last_best_bid_idx_ >= 0))  // if its an indep update, then we can just check if it beats the current proj price
    {
      if (projected_bid_price_ > last_best_bid_price_) {
        last_best_bid_idx_ = update_idx_;
        last_best_bid_price_ = projected_bid_price_;
      }
    } else {
      // find the best_bid
      last_best_bid_price_ = last_bid_price_vec_[0];
      last_best_bid_idx_ = 0;
      for (int i = 1; i < int(last_bid_price_vec_.size()); i++) {
        double t_projected_price_ =
            moving_avg_bid_price_vec_[0] +
            (last_bid_price_vec_[i] - moving_avg_bid_price_vec_[i]) * (volatility_factor_vec_[i]);
        if (t_projected_price_ > last_best_bid_price_) {
          last_best_bid_price_ = t_projected_price_;
          last_best_bid_idx_ = i;
        }
      }
    }

    if ((update_idx_ > 0) && (last_best_ask_idx_ != update_idx_) &&
        (last_best_ask_idx_ >= 0))  // if its an indep update, then we can just check if it beats the current proj price
    {
      if (projected_ask_price_ < last_best_ask_price_) {
        last_best_ask_idx_ = update_idx_;
        last_best_ask_price_ = projected_ask_price_;
      }
    } else {
      // find the best_ask
      last_best_ask_price_ = last_ask_price_vec_[0];
      last_best_ask_idx_ = 0;
      for (int i = 1; i < int(last_ask_price_vec_.size()); i++) {
        double t_projected_price_ =
            moving_avg_ask_price_vec_[0] +
            (last_ask_price_vec_[i] - moving_avg_ask_price_vec_[i]) * (volatility_factor_vec_[i]);
        if (t_projected_price_ < last_best_ask_price_) {
          last_best_ask_price_ = t_projected_price_;
          last_best_ask_idx_ = i;
        }
      }
    }

    if (last_best_bid_price_ >= last_best_ask_price_) {
      // std::cerr << last_best_bid_price_ << " " << last_best_ask_price_ << " " << last_bid_price_vec_[0] << " " <<
      // last_ask_price_vec_[0] << std::endl;
      // in case we have crossed best_price : can improve upon this formulation
      if (fabs(last_best_bid_price_ - last_bid_price_vec_[0]) > fabs(last_best_ask_price_ - last_ask_price_vec_[0])) {
        indicator_value_ = last_best_bid_price_ - last_bid_price_vec_[0];
      } else {
        indicator_value_ = last_best_ask_price_ - last_ask_price_vec_[0];
      }
    } else {
      indicator_value_ = (last_best_bid_price_ + last_best_ask_price_) / 2 - current_dep_price_;
    }

    if (std::isnan(indicator_value_)) {
      std::cerr << __PRETTY_FUNCTION__ << " nan " << std::endl;
      // exit ( 1 );
      // indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void ProjectedPriceConstPairs::InitializeValues(int _update_idx_) {
  if (_update_idx_ == 0) {
    last_ask_price_vec_[_update_idx_] = dep_market_view_.bestask_price();
    last_bid_price_vec_[_update_idx_] = dep_market_view_.bestbid_price();
  } else {
    last_ask_price_vec_[_update_idx_] = indep_market_view_vec_[_update_idx_ - 1]->bestask_price();
    last_bid_price_vec_[_update_idx_] = indep_market_view_vec_[_update_idx_ - 1]->bestbid_price();
  }
  moving_avg_ask_price_vec_[_update_idx_] = last_ask_price_vec_[_update_idx_];
  moving_avg_bid_price_vec_[_update_idx_] = last_bid_price_vec_[_update_idx_];

  last_new_page_msecs_vec_[_update_idx_] =
      watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
  last_best_bid_idx_ = -1;
  last_best_ask_idx_ = -1;
}

void ProjectedPriceConstPairs::OnMarketDataInterrupted(const unsigned int _security_id_,
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

void ProjectedPriceConstPairs::OnMarketDataResumed(const unsigned int _security_id_) {
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

bool ProjectedPriceConstPairs::AreAllReady() { return VectorUtils::CheckAllForValue(is_ready_vec_, true); }
}
