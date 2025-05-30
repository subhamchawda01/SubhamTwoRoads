/*
  file IndicatorsCode/online_beta_return.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_beta_kalman.hpp"

#include <sstream>
#include <string>
#include <fstream>

namespace HFSAT {
void OnlineBetaKalman::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  std::string t_source_shortcode_ = (std::string)r_tokens_[4];
  if (IndicatorUtil::IsPortfolioShortcode(t_source_shortcode_)) {
    IndicatorUtil::AddPortfolioShortCodeVec(t_source_shortcode_, _shortcodes_affecting_this_indicator_);
  } else {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, t_source_shortcode_);
  }
}

OnlineBetaKalman* OnlineBetaKalman::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_  _source_code_ t_return_history_secs_ _t_price_type_
  if (r_tokens_.size() < 7) {
    std::cerr << "insufficient arguments to INDICATOR OnlineBetaKalman, correct syntax : _this_weight_ "
                 "_indicator_string_ _dep_market_view_  _indep_market_view_ t_return_history_secs_ "
                 "_t_price_type_"
                 " OPTIONAL _t_init_var_ _t_beta_noise _t_return_noise_ \n";
    exit(1);
  }

  PriceType_t _price_type_ = StringToPriceType_t(r_tokens_[6]);

  double _init_var_ = -1;
  if (r_tokens_.size() > 7) {
    _init_var_ = atof(r_tokens_[7]);
  }

  double _beta_noise_ = -1;
  if (r_tokens_.size() > 8) {
    _beta_noise_ = atof(r_tokens_[8]);
  }

  double _return_noise_ = -1;
  if (r_tokens_.size() > 9) {
    _return_noise_ = atof(r_tokens_[9]);
  }

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  // ShortcodeSecurityMarketViewMap::StaticCheckValid ( r_tokens_[4] );
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      (std::string)r_tokens_[4], atoi(r_tokens_[5]), _init_var_, _beta_noise_, _return_noise_, _price_type_);
}

OnlineBetaKalman* OnlineBetaKalman::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const SecurityMarketView& _dep_market_view_,
                                                      std::string _source_shortcode_,
                                                      const unsigned int t_return_history_secs_,
                                                      const double _init_var_, const double _beta_noise_,
                                                      const double _return_noise_, PriceType_t _t_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _source_shortcode_ << ' '
              << t_return_history_secs_ << ' ' << ' ' << _init_var_ << ' ' << _beta_noise_ << ' ' << _return_noise_
              << _t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineBetaKalman*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new OnlineBetaKalman(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _source_shortcode_,
        t_return_history_secs_, _init_var_, _beta_noise_, _return_noise_, _t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineBetaKalman::OnlineBetaKalman(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   const SecurityMarketView& _dep_market_view_, std::string _source_shortcode_,
                                   const unsigned int t_return_history_secs_, const double _init_var_,
                                   const double _beta_noise_, const double _return_noise_, PriceType_t _t_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),

      dep_market_view_(_dep_market_view_),
      dep_return_indicator_(*(SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_,
                                                               t_return_history_secs_, _t_price_type_))),

      return_history_msecs_(std::max(100u, 1000u * t_return_history_secs_)),
      init_var_(_init_var_),
      beta_noise_(_beta_noise_),
      return_noise_(_return_noise_),
      lrdb_(OfflineReturnsRetLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      dep_updated_(false),
      indep_updated_(false),
      source_shortcode_(_source_shortcode_),
      upper_bound_(std::numeric_limits<double>::max()),
      lower_bound_(-1 * std::numeric_limits<double>::max()) {
  dep_market_view_.GetonlineBetaKalmanFile(kalman_param_file_);
  get_kalman_params();
  watch_.subscribe_BigTimePeriod(this);
  if (IndicatorUtil::IsPortfolioShortcode(_source_shortcode_)) {
    indep_return_indicator_ = SimpleReturnsPort::GetUniqueInstance(t_dbglogger_, r_watch_, _source_shortcode_,
                                                                   t_return_history_secs_, _t_price_type_);

    // min_unbiased_l2_norm_ = min_price_increment_ * min_price_increment_ / 25.0;
  } else {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(_source_shortcode_);
    SecurityMarketView& _indep_market_view_ =
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_source_shortcode_));
    indep_return_indicator_ = SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_,
                                                               t_return_history_secs_, _t_price_type_);
    // min_unbiased_l2_norm_ =
    //    _indep_market_view_.min_price_increment() * _indep_market_view_.min_price_increment() / 25.0;
  }

  trend_history_msecs_ = std::max(20, (int)round(1000 * t_return_history_secs_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;

  SetTimeDecayWeights();
  dep_return_indicator_.add_unweighted_indicator_listener(0, this);
  indep_return_indicator_->add_unweighted_indicator_listener(1, this);

  UpdateLRInfo();
}

void OnlineBetaKalman::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_kalman_param_present_) {
    indicator_value_ = 0;
    NotifyIndicatorListeners((indicator_value_));
  } else {
    if (!is_ready_) {
      if (_indicator_index_ == 0) {
        dep_updated_ = true;
      }
      if (_indicator_index_ == 1) {
        indep_updated_ = true;
      }
      if (dep_updated_ && indep_updated_) {
        is_ready_ = true;
        last_beta_msecs_ = watch_.msecs_from_midnight();
        InitializeValues();
      }
    } else {
      curr_returns_vec_[_indicator_index_] = _new_value_;

      if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
        moving_avg_vec_[_indicator_index_] +=
            inv_decay_sum_ * (curr_returns_vec_[_indicator_index_] - last_returns_vec_[_indicator_index_]);
      } else {
        int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
        if (num_pages_to_add_ >= (int)decay_vector_.size()) {
          InitializeValues();
        } else {
          if (num_pages_to_add_ == 1) {
            moving_avg_vec_[_indicator_index_] = (curr_returns_vec_[_indicator_index_] * inv_decay_sum_) +
                                                 (moving_avg_vec_[_indicator_index_] * decay_vector_[1]);
          } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
            moving_avg_vec_[_indicator_index_] =
                (curr_returns_vec_[_indicator_index_] * inv_decay_sum_) +
                (last_returns_vec_[_indicator_index_] * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                (moving_avg_vec_[_indicator_index_] * decay_vector_[num_pages_to_add_]);
          }
          last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
        }

        last_returns_vec_[_indicator_index_] = curr_returns_vec_[_indicator_index_];
      }

      if (watch_.msecs_from_midnight() - last_beta_msecs_ > 1000) {
        compute_beta();
        indicator_value_ = curr_beta_;

        if (indicator_value_ > upper_bound_) indicator_value_ = upper_bound_;
        if (indicator_value_ < lower_bound_) indicator_value_ = lower_bound_;

        NotifyIndicatorListeners((indicator_value_));
      }

      if (data_interrupted_) {
        indicator_value_ = 0;
        NotifyIndicatorListeners((indicator_value_));
      }
    }
  }
}

void OnlineBetaKalman::InitializeValues() {
  moving_avg_vec_[0] = curr_returns_vec_[0];
  moving_avg_vec_[1] = curr_returns_vec_[1];
  last_returns_vec_[0] = curr_returns_vec_[0];
  last_returns_vec_[1] = curr_returns_vec_[1];

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;

  prev_var_ = init_var_;
  curr_var_ = init_var_;

  UpdateLRInfo();
  indicator_value_ = current_projection_multiplier_;
  prev_beta_ = current_projection_multiplier_;
  curr_beta_ = current_projection_multiplier_;
  //  last_beta_msecs_ = watch_.msecs_from_midnight();
}

void OnlineBetaKalman::UpdateLRInfo() {
  if ((last_lrinfo_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_lrinfo_updated_msecs_ > TENMINUTESMSECS)) {
    current_lrinfo_ = lrdb_.GetLRCoeff(dep_market_view_.shortcode(), source_shortcode_);
    ComputeMultiplier();

    if (dbglogger_.CheckLoggingLevel(LRDB_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "lrinfo ( " << dep_market_view_.shortcode() << ", " << source_shortcode_ << " ) "
                             << current_lrinfo_.lr_coeff_ << ' ' << current_lrinfo_.lr_correlation_ << " -> "
                             << current_projection_multiplier_ << DBGLOG_ENDL_FLUSH;
    }

    last_lrinfo_updated_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TENMINUTESMSECS);
  }
}
void OnlineBetaKalman::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OnlineBetaKalman::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}

void OnlineBetaKalman::compute_beta() {
  prev_var_ = curr_var_ + beta_noise_;

  gain_ = prev_var_ * moving_avg_vec_[1] / ((moving_avg_vec_[1] * prev_var_ * moving_avg_vec_[1]) + return_noise_);

  curr_beta_ = prev_beta_ + (gain_ * (moving_avg_vec_[0] - (prev_beta_ * moving_avg_vec_[1])));

  prev_beta_ = curr_beta_;

  curr_var_ = (1 - (gain_ * moving_avg_vec_[1])) * prev_var_;
  last_beta_msecs_ = watch_.msecs_from_midnight();
}

void OnlineBetaKalman::get_kalman_params() {
  std::string dep_code = dep_market_view_.shortcode();
  std::string indep_code = source_shortcode_;

  double init_var = -1;
  double beta_noise = -1;
  double return_noise = -1;

  std::string line;
  std::string temp_dep_code = "";
  std::string temp_indep_code = " ";

  std::ifstream infile(kalman_param_file_);

  while (std::getline(infile, line)) {
    std::istringstream iss(line);

    if (!(iss >> temp_dep_code >> temp_indep_code)) {
      std::cerr << "kalman_lrdb_file is incomplete : \n ";
      // exit(1);
    }  // error

    if ((dep_code.compare(temp_dep_code) == 0) && (indep_code.compare(temp_indep_code) == 0)) {
      if (!(iss >> init_var >> beta_noise >> return_noise)) {
        std::cerr << "kalman_lrdb_file is incomplete : \n ";
        init_var = -1;
        beta_noise = -1;
        return_noise = -1;
        // exit(1);
      }
      break;
    }
  }

  if (init_var_ == -1) {
    init_var_ = init_var;
  }
  if (beta_noise_ == -1) {
    beta_noise_ = beta_noise;
  }
  if (return_noise_ == -1) {
    return_noise_ = return_noise;
  }

  if ((init_var_ != -1) && (beta_noise_ != -1) && (return_noise_ != -1)) {
    is_kalman_param_present_ = true;
  }
}
}
