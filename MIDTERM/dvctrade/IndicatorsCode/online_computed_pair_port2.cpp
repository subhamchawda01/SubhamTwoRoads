/**
    \file IndicatorsCode/online_computed_pair_port.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_computed_pair_port2.hpp"
#define PAIRSTRADEFILE "/spare/local/tradeinfo/PairTradeInfo/portfolio_inputs.txt"

namespace HFSAT {

void OnlineComputedPairPort2::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                std::vector<std::string>& _ors_source_needed_vec_,
                                                const std::vector<const char*>& r_tokens_) {
  // VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);

  int t_is_indep_portfolio = atoi(r_tokens_[7]);

  if (t_is_indep_portfolio) {
    IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
  } else {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
  }

  std::string t_portfolio_descriptor_shortcode_ = r_tokens_[3];
  std::string line;
  std::ifstream Portfoliofile(PAIRSTRADEFILE);
  bool flag_port_found_ = false;
  if (Portfoliofile.is_open()) {
    while (getline(Portfoliofile, line)) {
      std::istringstream buf(line);
      std::istream_iterator<std::string> beg(buf), end;

      std::vector<std::string> tokens(beg, end);
      if (tokens.size() != 0) {
        if (tokens[0] == t_portfolio_descriptor_shortcode_) {
          flag_port_found_ = true;
          for (unsigned int index = 1; index <= tokens.size() / 2; index++) {
            _shortcodes_affecting_this_indicator_.push_back(tokens[index]);
            // std :: cout<<tokens[index]<<std ::endl;
          }
          break;
        }
      }
    }
  } else {
    std::cerr << "PricePortfolio::Portfolio file not accessible" << t_portfolio_descriptor_shortcode_ << std::endl;
    ExitVerbose(kPortfolioConstituentManagerMissingArgs, t_portfolio_descriptor_shortcode_.c_str());
  }
  if (flag_port_found_ == false) {
    std::cerr << "PricePortfolio::PortfolioMissing" << t_portfolio_descriptor_shortcode_ << std::endl;
    ExitVerbose(kPortfolioConstituentManagerMissingArgs, t_portfolio_descriptor_shortcode_.c_str());
  }
}

OnlineComputedPairPort2* OnlineComputedPairPort2::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                    const std::vector<const char*>& r_tokens_,
                                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _portfolio_descriptor_shortcode_ _fractional_seconds_
  // _price_type_
  // ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_, (std::string)(r_tokens_[3]), (std::string)(r_tokens_[4]),
                           atof(r_tokens_[5]), atof(r_tokens_[6]), atoi(r_tokens_[7]),
                           StringToPriceType_t(r_tokens_[8]));
}

OnlineComputedPairPort2* OnlineComputedPairPort2::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, std::string _dep_portfolio_shortcode_,
    std::string _portfolio_descriptor_shortcode_, double _fractional_seconds_, double _fractional_seconds_port_,
    int _is_indep_portfolio_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_portfolio_shortcode_ << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _fractional_seconds_ << ' ' << _fractional_seconds_port_ << ' ' << _is_indep_portfolio_ << ' '
              << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineComputedPairPort2*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OnlineComputedPairPort2(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_portfolio_shortcode_,
                                    _portfolio_descriptor_shortcode_, _fractional_seconds_, _fractional_seconds_port_,
                                    _is_indep_portfolio_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineComputedPairPort2::OnlineComputedPairPort2(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                 const std::string& concise_indicator_description_,
                                                 std::string _dep_portfolio_shortcode_,
                                                 std::string _portfolio_descriptor_shortcode_,
                                                 double _fractional_seconds_, double _fractional_seconds_port_,
                                                 int _is_indep_portfolio_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_portfolio_price__(PricePortfolio::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_portfolio_shortcode_,
                                                              _fractional_seconds_port_, _price_type_)),
      price_type_(_price_type_),
      moving_avg_dep_price_(0),
      moving_avg_indep_price_(0),
      moving_avg_dep_indep_price_(0),
      moving_avg_indep_indep_price_(0),
      last_dep_price_recorded_(0),
      last_indep_price_recorded_(0),
      current_dep_price_(0),
      current_indep_price_(0),
      is_indep_portfolio_(_is_indep_portfolio_) {
  trend_history_msecs_ = std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  // std:: cout<<"Constructor Called"<<std::endl;
  if (is_indep_portfolio_ == 1) {
    indep_portfolio_price__ =
        PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_);
    indep_portfolio_price__->AddPriceChangeListener(this);
  } else {
    indep_market_view_ =
        (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_portfolio_descriptor_shortcode_));
    if (!indep_market_view_->subscribe_price_type(this, _price_type_)) {
      PriceType_t t_error_price_type_ = _price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  }
  dep_portfolio_price__->add_unweighted_indicator_listener(0, this);
}

void OnlineComputedPairPort2::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  // 1 we want to use model file with which has less/no indep's indicators
  // 2 we want to use model file with decent/high indep's indicators
  // std:: cout<<"IndicatorUpdateGeneric"<<std::endl;
  if (_indicator_index_ == 0) {
    current_dep_price_ = _new_value_;
    if (!is_ready_) {
      if (is_indep_portfolio_ == 1) {
        if (indep_portfolio_price__->is_ready() && dep_portfolio_price__->is_ready()) {
          is_ready_ = true;

          if (abs(current_dep_price_) < 0.1 || abs(current_indep_price_) < 0.1) return;
          InitializeValues();
        }
      } else {
        if (indep_market_view_->is_ready_complex(2) && dep_portfolio_price__->is_ready()) {
          is_ready_ = true;
          if (abs(current_dep_price_) < 0.1 || abs(current_indep_price_) < 0.1) return;
          InitializeValues();
        }
      }

    } else {
      UpdateComputedVariables();
    }
  }

  return;
}

void OnlineComputedPairPort2::OnPortfolioPriceChange(double _new_price_) {
  current_indep_price_ = _new_price_;

  if (!is_ready_) {
    if (is_indep_portfolio_ == 1) {
      if (indep_portfolio_price__->is_ready() && dep_portfolio_price__->is_ready()) {
        is_ready_ = true;
        if (abs(current_dep_price_) < 0.1 || abs(current_indep_price_) < 0.1) return;
        InitializeValues();
      }
    } else {
      if (indep_market_view_->is_ready_complex(2) && dep_portfolio_price__->is_ready()) {
        is_ready_ = true;
        if (abs(current_dep_price_) < 0.1 || abs(current_indep_price_) < 0.1) return;
        InitializeValues();
      }
    }
  } else {
    UpdateComputedVariables();
  }
}

void OnlineComputedPairPort2::OnMarketUpdate(const unsigned int _security_id_,
                                             const MarketUpdateInfo& cr_market_update_info_) {
  // this must be an update of dependant
  // hence using GetPriceFromType and not dep_market_view_.price_from_type
  // current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
  if (!is_indep_portfolio_) {
    if (indep_market_view_->security_id() == _security_id_) {
      current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    }
  }
  if (!is_ready_) {
    if (!is_indep_portfolio_) {
      if (indep_market_view_->is_ready_complex(2) && dep_portfolio_price__->is_ready()) {
        is_ready_ = true;
        if (abs(current_dep_price_) < 0.1 || abs(current_indep_price_) < 0.1) return;
        InitializeValues();
      }
    } else {
      if (indep_portfolio_price__->is_ready() && dep_portfolio_price__->is_ready()) {
        is_ready_ = true;
        if (abs(current_dep_price_) < 0.1 || abs(current_indep_price_) < 0.1) return;
        InitializeValues();
      }
    }
  } else {
    UpdateComputedVariables();
  }
}

void OnlineComputedPairPort2::UpdateComputedVariables() {
  if (abs(current_dep_price_) < 0.1 || abs(current_indep_price_) < 0.1) return;
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
    moving_avg_dep_price_ += inv_decay_sum_ * (current_dep_price_ - last_dep_price_recorded_);
    moving_avg_indep_price_ += inv_decay_sum_ * (current_indep_price_ - last_indep_price_recorded_);
    moving_avg_dep_indep_price_ += inv_decay_sum_ * ((current_dep_price_ * current_indep_price_) -
                                                     (last_dep_price_recorded_ * last_indep_price_recorded_));
    moving_avg_indep_indep_price_ += inv_decay_sum_ * ((current_indep_price_ * current_indep_price_) -
                                                       (last_indep_price_recorded_ * last_indep_price_recorded_));

  } else {
    int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      if (abs(current_dep_price_) < 0.1 || abs(current_indep_price_) < 0.1) return;
      InitializeValues();
    } else {
      if (num_pages_to_add_ == 1) {
        moving_avg_dep_price_ = (current_dep_price_ * inv_decay_sum_) + (moving_avg_dep_price_ * decay_page_factor_);
        moving_avg_indep_price_ =
            (current_indep_price_ * inv_decay_sum_) + (moving_avg_indep_price_ * decay_page_factor_);
        moving_avg_dep_indep_price_ = (current_dep_price_ * current_indep_price_ * inv_decay_sum_) +
                                      (moving_avg_dep_indep_price_ * decay_page_factor_);
        moving_avg_indep_indep_price_ = (current_indep_price_ * current_indep_price_ * inv_decay_sum_) +
                                        (moving_avg_indep_indep_price_ * decay_page_factor_);
      } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
        moving_avg_dep_price_ = (current_dep_price_ * inv_decay_sum_) + (last_dep_price_recorded_ * inv_decay_sum_ *
                                                                         decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                (moving_avg_dep_price_ * decay_vector_[num_pages_to_add_]);
        moving_avg_indep_price_ =
            (current_indep_price_ * inv_decay_sum_) +
            (last_indep_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
            (moving_avg_indep_price_ * decay_vector_[num_pages_to_add_]);
        moving_avg_dep_indep_price_ = (current_dep_price_ * current_indep_price_ * inv_decay_sum_) +
                                      (last_dep_price_recorded_ * last_indep_price_recorded_ * inv_decay_sum_ *
                                       decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                      (moving_avg_dep_indep_price_ * decay_vector_[num_pages_to_add_]);
        moving_avg_indep_indep_price_ = (current_indep_price_ * current_indep_price_ * inv_decay_sum_) +
                                        (last_indep_price_recorded_ * last_indep_price_recorded_ * inv_decay_sum_ *
                                         decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                        (moving_avg_indep_indep_price_ * decay_vector_[num_pages_to_add_]);
      }
      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }

  if (moving_avg_indep_indep_price_ > 1.00) {  // added to prevent nan

    // Main Difference from OnlineComputedPairsPort
    double indep_to_proj_value_ = (current_indep_price_ - moving_avg_indep_price_);
    double dep_to_proj_value_ = (current_dep_price_ - moving_avg_dep_price_);

    indicator_value_ =
        ((moving_avg_dep_indep_price_ * indep_to_proj_value_) / moving_avg_indep_indep_price_) - dep_to_proj_value_;

  } else {
    indicator_value_ = 0;
  }

  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);

  last_dep_price_recorded_ = current_dep_price_;
  last_indep_price_recorded_ = current_indep_price_;
}

void OnlineComputedPairPort2::OnPortfolioPriceReset(double t_new_portfolio_price_, double t_old_portfolio_price_,
                                                    unsigned int is_data_interrupted_) {
  if (is_data_interrupted_ == 1u) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else if (is_data_interrupted_ == 2u) {
    InitializeValues();
    data_interrupted_ = false;
  } else if (is_data_interrupted_ == 0u) {
    current_indep_price_ = t_new_portfolio_price_;
    double jump_ = t_new_portfolio_price_ - t_old_portfolio_price_;
    moving_avg_indep_indep_price_ = moving_avg_indep_indep_price_ + jump_ * (2 * moving_avg_indep_price_ + jump_);
    moving_avg_indep_price_ = moving_avg_indep_price_ + jump_;
    moving_avg_dep_indep_price_ = moving_avg_dep_indep_price_ + (moving_avg_dep_price_ * jump_);

    if (moving_avg_indep_indep_price_ > 1.00) {  // added to prevent nan

      // Main Difference from OnlineComputedPairsPort
      double indep_to_proj_value_ = (current_indep_price_ - moving_avg_indep_price_);
      double dep_to_proj_value_ = (current_dep_price_ - moving_avg_dep_price_);

      indicator_value_ =
          ((moving_avg_dep_indep_price_ * indep_to_proj_value_) / moving_avg_indep_indep_price_) - dep_to_proj_value_;

    } else {
      indicator_value_ = 0;
    }
    if (data_interrupted_) indicator_value_ = 0;

    last_indep_price_recorded_ = current_indep_price_;
  }
}

void OnlineComputedPairPort2::InitializeValues() {
  moving_avg_dep_price_ = current_dep_price_;
  moving_avg_indep_price_ = current_indep_price_;
  moving_avg_dep_indep_price_ = current_dep_price_ * current_indep_price_;
  moving_avg_indep_indep_price_ = current_indep_price_ * current_indep_price_;

  last_dep_price_recorded_ = current_dep_price_;
  last_indep_price_recorded_ = current_indep_price_;

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}
void OnlineComputedPairPort2::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                      const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OnlineComputedPairPort2::OnMarketDataResumed(const unsigned int _security_id_) {
  if (abs(current_dep_price_) < 0.1 || abs(current_indep_price_) < 0.1) return;
  InitializeValues();
  data_interrupted_ = false;
}
}
