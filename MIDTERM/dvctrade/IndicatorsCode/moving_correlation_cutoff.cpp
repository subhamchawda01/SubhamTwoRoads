/**
   \file IndicatorsCode/moving_correlation_cutoff.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/moving_correlation_cutoff.hpp"

namespace HFSAT {
void MovingCorrelationCutOff::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                std::vector<std::string>& _ors_source_needed_vec_,
                                                const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

MovingCorrelationCutOff* MovingCorrelationCutOff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                    const std::vector<const char*>& r_tokens_,
                                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_  _indep_market_view_ t_trend_history_secs_
  // _t_price_type_ indicator_threshold _indicator_return_type
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  int return_type = 0;
  if (r_tokens_.size() > 8) return_type = atoi(r_tokens_[8]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atoi(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]), atof(r_tokens_[7]), return_type);
}

MovingCorrelationCutOff* MovingCorrelationCutOff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                    const SecurityMarketView& _dep_market_view_,
                                                                    const SecurityMarketView& _indep_market_view_,
                                                                    const unsigned int t_trend_history_secs_,
                                                                    PriceType_t _t_price_type_, double _indicator_thres,
                                                                    int _indicator_return_type) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << t_trend_history_secs_ << ' ' << _t_price_type_ << ' ' << _indicator_thres << ' '
              << _indicator_return_type;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, MovingCorrelationCutOff*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new MovingCorrelationCutOff(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _indep_market_view_,
        t_trend_history_secs_, _t_price_type_, _indicator_thres, _indicator_return_type);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

MovingCorrelationCutOff::MovingCorrelationCutOff(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                 const std::string& concise_indicator_description_,
                                                 const SecurityMarketView& _dep_market_view_,
                                                 const SecurityMarketView& _indep_market_view_,
                                                 const unsigned int t_trend_history_secs_, PriceType_t _t_price_type_,
                                                 double _indicator_thres, int _indicator_return_type)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      moving_sum_of_product_of_normalised_change_in_price_(0),
      moving_sum_of_square_of_normalised_price_(0),
      dep_slow_stdev_calculator_(*(SlowStdevCalculator::GetUniqueInstance(
          t_dbglogger_, r_watch_, _dep_market_view_.shortcode(), 1000 * t_trend_history_secs_))),
      indep_slow_stdev_calculator_(*(SlowStdevCalculator::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_.shortcode(), 1000 * t_trend_history_secs_))),
      dep_price_change_(0),
      indep_price_change_(0),
      dep_last_price_change_(0),
      indep_last_price_change_(0),
      dep_std_dev_(1.00),
      indep_std_dev_(1.00),
      dep_updated(false),
      indep_updated(false),
      cutoff_threshold(_indicator_thres),
      last_dep_price_recorded_(0),
      last_indep_price_recorded_(0),
      t_price_type_(_t_price_type_),
      current_dep_price_(0),
      current_indep_price_(0),
      dep_moving_avg_price_(0),
      indep_moving_avg_price_(0),
      _indicator_return_type_(_indicator_return_type),
      moving_correlation_cutoff_listener_ptr_vec_() {
  if (cutoff_threshold <= 0) {
    update_threshold = true;
  } else
    update_threshold = false;
  trend_history_msecs_ = std::max(20, (int)round(1000 * t_trend_history_secs_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  SetThresholdTimeDecayWeights();
  dep_slow_stdev_calculator_.AddSlowStdevCalculatorListener(this);
  indep_slow_stdev_calculator_.AddSlowStdevCalculatorListener(this);
  dep_market_view_.subscribe_price_type(this, _t_price_type_);
  indep_market_view_.subscribe_price_type(this, _t_price_type_);
}

void MovingCorrelationCutOff::OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) {
  if (_security_id_ == dep_market_view_.security_id())
    dep_std_dev_ = _new_stdev_value_;
  else
    indep_std_dev_ = _new_stdev_value_;
}

void MovingCorrelationCutOff::OnMarketUpdate(const unsigned int _security_id_,
                                             const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    current_dep_price_ = dep_market_view_.GetPriceFromType(t_price_type_, _market_update_info_);
    dep_updated = true;
  }
  if (_security_id_ == indep_market_view_.security_id()) {
    current_indep_price_ = indep_market_view_.GetPriceFromType(t_price_type_, _market_update_info_);
    indep_updated = true;
  }

  if (dep_updated && indep_updated) {
    if (!is_ready_) {
      if (indep_market_view_.is_ready_complex(2) && dep_market_view_.is_ready_complex(2)) {
        is_ready_ = true;
        InitializeValues();
        InitializeValues2();
      }
    } else {
      if (update_threshold) {
        dep_price_change2_ = (current_dep_price_ - dep_moving_avg_price2_) / dep_std_dev_;
        indep_price_change2_ = (current_indep_price_ - indep_moving_avg_price2_) / indep_std_dev_;

        if (watch_.msecs_from_midnight() - last_new_page_msecs2_ < page_width_msecs2_) {
          moving_sum_of_product_of_normalised_change_in_price2_ +=
              inv_decay_sum2_ *
              (dep_price_change2_ * indep_price_change2_ - dep_last_price_change2_ * indep_last_price_change2_);
          moving_sum_of_square_of_normalised_price2_ +=
              inv_decay_sum2_ *
              ((dep_price_change2_ * dep_price_change2_ + indep_price_change2_ * indep_price_change2_) -
               (dep_last_price_change2_ * dep_last_price_change2_ +
                indep_last_price_change2_ * indep_last_price_change2_));
          dep_moving_avg_price2_ += inv_decay_sum2_ * (current_dep_price_ - last_dep_price_recorded_);
          indep_moving_avg_price2_ += inv_decay_sum2_ * (current_indep_price_ - last_indep_price_recorded_);
        } else {  // new page(s)
          int num_pages_to_add_ =
              (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs2_) / page_width_msecs2_);
          if (num_pages_to_add_ >= (int)decay_vector2_.size()) {
            InitializeValues2();
          } else {
            if (num_pages_to_add_ == 1) {
              moving_sum_of_product_of_normalised_change_in_price2_ =
                  (dep_price_change2_ * indep_price_change2_ * inv_decay_sum2_) +
                  (moving_sum_of_product_of_normalised_change_in_price2_ * decay_page_factor2_);
              moving_sum_of_square_of_normalised_price2_ =
                  inv_decay_sum2_ *
                      (dep_price_change2_ * dep_price_change2_ + indep_price_change2_ * indep_price_change2_) +
                  (moving_sum_of_square_of_normalised_price2_ * decay_page_factor2_);
              dep_moving_avg_price2_ =
                  (current_dep_price_ * inv_decay_sum2_) + (dep_moving_avg_price2_ * decay_page_factor2_);
              indep_moving_avg_price2_ =
                  (current_indep_price_ * inv_decay_sum2_) + (indep_moving_avg_price2_ * decay_page_factor2_);
            } else {  // num_pages_to_add_ >= 2 < decay_vector2_.size ( )
              moving_sum_of_product_of_normalised_change_in_price2_ =
                  inv_decay_sum2_ * dep_price_change2_ * indep_price_change2_ +
                  (dep_last_price_change2_ * indep_last_price_change2_ * inv_decay_sum2_ *
                   decay_vector_sums2_[(num_pages_to_add_ - 1)]) +
                  (moving_sum_of_product_of_normalised_change_in_price2_ * decay_vector2_[num_pages_to_add_]);
              moving_sum_of_square_of_normalised_price2_ =
                  (inv_decay_sum2_ *
                   (dep_price_change2_ * dep_price_change2_ + indep_price_change2_ * indep_price_change2_)) +
                  ((dep_last_price_change2_ * dep_last_price_change2_ +
                    indep_last_price_change2_ * indep_last_price_change2_) *
                   inv_decay_sum2_ * decay_vector_sums2_[(num_pages_to_add_ - 1)]) +
                  (moving_sum_of_square_of_normalised_price2_ * decay_vector2_[num_pages_to_add_]);
              dep_moving_avg_price2_ =
                  (current_dep_price_ * inv_decay_sum2_) +
                  (last_dep_price_recorded_ * inv_decay_sum2_ * decay_vector_sums2_[(num_pages_to_add_ - 1)]) +
                  (dep_moving_avg_price2_ * decay_vector2_[num_pages_to_add_]);
              indep_moving_avg_price2_ =
                  (current_indep_price_ * inv_decay_sum2_) +
                  (last_indep_price_recorded_ * inv_decay_sum2_ * decay_vector_sums2_[(num_pages_to_add_ - 1)]) +
                  (indep_moving_avg_price2_ * decay_vector2_[num_pages_to_add_]);
            }

            last_new_page_msecs2_ += (num_pages_to_add_ * page_width_msecs2_);
          }
        }
        dep_last_price_change2_ = dep_price_change2_;
        indep_last_price_change2_ = indep_price_change2_;

        cutoff_threshold = sqrt(fabs(2 * moving_sum_of_product_of_normalised_change_in_price2_ /
                                     moving_sum_of_square_of_normalised_price2_));
      }

      dep_price_change_ = (current_dep_price_ - dep_moving_avg_price_) / dep_std_dev_;
      indep_price_change_ = (current_indep_price_ - indep_moving_avg_price_) / indep_std_dev_;

      if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
        moving_sum_of_product_of_normalised_change_in_price_ +=
            inv_decay_sum_ *
            (dep_price_change_ * indep_price_change_ - dep_last_price_change_ * indep_last_price_change_);
        moving_sum_of_square_of_normalised_price_ +=
            inv_decay_sum_ *
            ((dep_price_change_ * dep_price_change_ + indep_price_change_ * indep_price_change_) -
             (dep_last_price_change_ * dep_last_price_change_ + indep_last_price_change_ * indep_last_price_change_));
        dep_moving_avg_price_ += inv_decay_sum_ * (current_dep_price_ - last_dep_price_recorded_);
        indep_moving_avg_price_ += inv_decay_sum_ * (current_indep_price_ - last_indep_price_recorded_);
      } else {  // new page(s)
        int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
        if (num_pages_to_add_ >= (int)decay_vector_.size()) {
          InitializeValues();
        } else {
          if (num_pages_to_add_ == 1) {
            moving_sum_of_product_of_normalised_change_in_price_ =
                (dep_price_change_ * indep_price_change_ * inv_decay_sum_) +
                (moving_sum_of_product_of_normalised_change_in_price_ * decay_page_factor_);
            moving_sum_of_square_of_normalised_price_ =
                inv_decay_sum_ * (dep_price_change_ * dep_price_change_ + indep_price_change_ * indep_price_change_) +
                (moving_sum_of_square_of_normalised_price_ * decay_page_factor_);
            dep_moving_avg_price_ =
                (current_dep_price_ * inv_decay_sum_) + (dep_moving_avg_price_ * decay_page_factor_);
            indep_moving_avg_price_ =
                (current_indep_price_ * inv_decay_sum_) + (indep_moving_avg_price_ * decay_page_factor_);
          } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
            moving_sum_of_product_of_normalised_change_in_price_ =
                inv_decay_sum_ * dep_price_change_ * indep_price_change_ +
                (dep_last_price_change_ * indep_last_price_change_ * inv_decay_sum_ *
                 decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                (moving_sum_of_product_of_normalised_change_in_price_ * decay_vector_[num_pages_to_add_]);
            moving_sum_of_square_of_normalised_price_ =
                (inv_decay_sum_ * (dep_price_change_ * dep_price_change_ + indep_price_change_ * indep_price_change_)) +
                ((dep_last_price_change_ * dep_last_price_change_ +
                  indep_last_price_change_ * indep_last_price_change_) *
                 inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                (moving_sum_of_square_of_normalised_price_ * decay_vector_[num_pages_to_add_]);
            dep_moving_avg_price_ =
                (current_dep_price_ * inv_decay_sum_) +
                (last_dep_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                (dep_moving_avg_price_ * decay_vector_[num_pages_to_add_]);
            indep_moving_avg_price_ =
                (current_indep_price_ * inv_decay_sum_) +
                (last_indep_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                (indep_moving_avg_price_ * decay_vector_[num_pages_to_add_]);
          }

          last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
        }
      }
      dep_last_price_change_ = dep_price_change_;
      indep_last_price_change_ = indep_price_change_;
      last_indep_price_recorded_ = current_indep_price_;
      last_dep_price_recorded_ = current_dep_price_;
      indicator_value_ = 0;
      double temp = 0;

      if (moving_sum_of_square_of_normalised_price_ > 0) {
        indicator_value_ = sqrt(
            fabs(2 * moving_sum_of_product_of_normalised_change_in_price_ / moving_sum_of_square_of_normalised_price_));
        temp = indicator_value_;
        if (indicator_value_ > cutoff_threshold) {
          indicator_value_ = 1;
        } else {
          indicator_value_ = 0;
        }
      }
      indicator_value_++;
      if (_indicator_return_type_ == -1) indicator_value_ = temp;
      dep_updated = false;
      indep_updated = false;
      if (data_interrupted_) {
        indicator_value_ = 0;
      }
      // std::cout<<cutoff_threshold<<" " <<indicator_value_ << std::endl;
      NotifyIndicatorListeners((indicator_value_));
    }
  }
}

void MovingCorrelationCutOff::SetThresholdTimeDecayWeights() {
  const unsigned int kDecayLength =
      100;  ///< here number of samples are not required to be very high and hence the decaylength target is just 30
  const unsigned int kMinPageWidth = 10;
  const unsigned int kMaxPageWidth = 10000;  ///< really relaxed since the time duration is 300 seconds here
  page_width_msecs2_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (trend_history_msecs_ * 3 / kDecayLength)));

  int number_fadeoffs_ = std::max(1, (int)ceil(trend_history_msecs_ * 3 / page_width_msecs2_));

  decay_page_factor2_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

  decay_vector2_.resize(2 * number_fadeoffs_);
  decay_vector_sums2_.resize(2 * number_fadeoffs_);

  for (auto i = 0u; i < decay_vector2_.size(); i++) {
    decay_vector2_[i] = pow(decay_page_factor2_, (int)i);
  }
  decay_vector_sums2_[0] = 0;
  for (unsigned int i = 1; i < decay_vector_sums2_.size(); i++) {
    decay_vector_sums2_[i] = decay_vector_sums2_[i - 1] + decay_vector2_[i];
  }
  inv_decay_sum2_ = (1 - decay_page_factor2_);
}

void MovingCorrelationCutOff::InitializeValues() {
  moving_sum_of_product_of_normalised_change_in_price_ = 0;
  moving_sum_of_square_of_normalised_price_ = 0;
  dep_last_price_change_ = 0;
  indep_last_price_change_ = 0;
  dep_moving_avg_price_ = current_dep_price_;
  indep_moving_avg_price_ = current_indep_price_;
  last_indep_price_recorded_ = current_indep_price_;
  last_dep_price_recorded_ = current_dep_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % page_width_msecs_);
  indicator_value_ = 0;
}

void MovingCorrelationCutOff::InitializeValues2() {
  moving_sum_of_product_of_normalised_change_in_price2_ = 0;
  moving_sum_of_square_of_normalised_price2_ = 0;
  dep_last_price_change2_ = 0;
  indep_last_price_change2_ = 0;
  dep_moving_avg_price2_ = current_dep_price_;
  indep_moving_avg_price2_ = current_indep_price_;
  last_new_page_msecs2_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % page_width_msecs2_);
}

void MovingCorrelationCutOff::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                      const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void MovingCorrelationCutOff::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
