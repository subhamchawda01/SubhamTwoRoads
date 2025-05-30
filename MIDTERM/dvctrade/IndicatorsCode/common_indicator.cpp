/**
    \file IndicatorsCode/common_indicator.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvctrade/Indicators/common_indicator.hpp"

namespace HFSAT {

int32_t CommonIndicator::global_trading_start_mfm_ = 0;
int32_t CommonIndicator::global_trading_end_mfm_ = 86400000;

std::map<std::string, CommonIndicator*> CommonIndicator::global_concise_indicator_description_map_;

CommonIndicator::CommonIndicator(DebugLogger& _dbglogger_, const Watch& _watch_,
                                 const std::string& r_concise_indicator_description_)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      concise_indicator_description_(r_concise_indicator_description_),
      indicator_listener_pairs_(),
      indicator_listener_pairs_logit_(),
      unweighted_indicator_listener_pairs_(),
      basepx_pxtype_(kPriceTypeMidprice),
      trading_start_mfm_(global_trading_start_mfm_),
      trading_end_mfm_(global_trading_end_mfm_),
      trend_history_msecs_(0),
      last_new_page_msecs_(0),
      page_width_msecs_(500),
      decay_page_factor_(0.95),
      inv_decay_sum_(0.05),
      is_ready_(false),
      data_interrupted_(false),
      indicator_value_(0),
      tolerance_value_(0.05),
      prev_indicator_value_(0),
      last_mkt_status_(kMktTradingStatusOpen) {
  // std::ifstream ifs;
  // ifs.open ( "/home/ashwin/ashwin/indicator_threshold.txt" , std::ifstream::in );
  // if ( ifs.is_open ( ) )
  //{
  //	ifs >> tolerance_value_;
  //	ifs.close ( );
  //}
}

void CommonIndicator::set_start_mfm(int32_t t_start_mfm_) { trading_start_mfm_ = t_start_mfm_; }

void CommonIndicator::set_end_mfm(int32_t t_end_mfm_) { trading_end_mfm_ = t_end_mfm_; }

void CommonIndicator::set_global_start_mfm(int32_t t_global_start_mfm_) {
  global_trading_start_mfm_ = t_global_start_mfm_;
}

void CommonIndicator::set_global_end_mfm(int32_t t_global_end_mfm_) { global_trading_end_mfm_ = t_global_end_mfm_; }

void CommonIndicator::add_indicator_listener(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                                             double _node_value_) {
  if (_indicator_listener__ != NULL) {
    if (IsWeightedListenerPresent(_indicator_listener__) != true) {
      IndicatorListenerPair _new_indicator_listener_pair_(_indicator_index_, _indicator_listener__, _node_value_);
      indicator_listener_pairs_.push_back(_new_indicator_listener_pair_);
    }
  }
}

void CommonIndicator::add_indicator_listener(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                                             double _node_value_, bool _check_existance_) {
  if (_indicator_listener__ != NULL && !_check_existance_) {
    IndicatorListenerPair _new_indicator_listener_pair_(_indicator_index_, _indicator_listener__, _node_value_);
    indicator_listener_pairs_.push_back(_new_indicator_listener_pair_);
  }
}

void CommonIndicator::add_indicator_listener(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                                             double _node_value_decrease_, double _node_value_nochange_,
                                             double _node_value_increase_) {
  if (_indicator_listener__ != NULL) {
    if (IsWeightedListenerPresent(_indicator_listener__) != true) {
      IndicatorListenerPairLogit _new_indicator_listener_pair_logit_(_indicator_index_, _indicator_listener__,
                                                                     _node_value_decrease_, _node_value_nochange_,
                                                                     _node_value_increase_);
      indicator_listener_pairs_logit_.push_back(_new_indicator_listener_pair_logit_);
    }
  }
}

void CommonIndicator::add_indicator_listener(unsigned int _indicator_index_, IndicatorListener* _indicator_listener__,
                                             double _node_value_alpha_, double _node_value_beta_) {
  if (_indicator_listener__ != NULL) {
    if (IsWeightedListenerPresent(_indicator_listener__) != true) {
      IndicatorListenerPairSigmoid _new_indicator_listener_pair_sigmoid_(_indicator_index_, _indicator_listener__,
                                                                         _node_value_alpha_, _node_value_beta_);
      indicator_listener_pairs_sigmoid_.push_back(_new_indicator_listener_pair_sigmoid_);
    }
  }
}

void CommonIndicator::UpdateIndicatorListenerWeight(IndicatorListener* _indicator_listener__, double _node_value_) {
  for (auto i = 0u; i < indicator_listener_pairs_.size(); i++) {
    if (indicator_listener_pairs_[i].indicator_listener__ == _indicator_listener__) {
      indicator_listener_pairs_[i].node_value_ = _node_value_;
    }
  }
}

void CommonIndicator::MultiplyIndicatorListenerWeight(IndicatorListener* _indicator_listener__,
                                                      double _node_value_mult_factor_) {
  for (auto i = 0u; i < indicator_listener_pairs_.size(); i++) {
    if (indicator_listener_pairs_[i].indicator_listener__ == _indicator_listener__) {
      indicator_listener_pairs_[i].node_value_ *= _node_value_mult_factor_;
    }
  }
}

double CommonIndicator::GetIndicatorListenerWeight(IndicatorListener* _indicator_listener__) {
  for (auto i = 0u; i < indicator_listener_pairs_.size(); i++) {
    if (indicator_listener_pairs_[i].indicator_listener__ == _indicator_listener__) {
      return (indicator_listener_pairs_[i].node_value_);
    }
  }
  return (-100000000);
}

bool CommonIndicator::IsWeightedListenerPresent(IndicatorListener* _indicator_listener__) {
  for (auto i = 0u; i < indicator_listener_pairs_.size(); i++) {
    if (indicator_listener_pairs_[i].indicator_listener__ == _indicator_listener__) {
      return true;
    }
  }
  return false;
}

void CommonIndicator::add_unweighted_indicator_listener(unsigned int _indicator_index_,
                                                        IndicatorListener* _indicator_listener__) {
  if (_indicator_listener__ != NULL) {
    if (IsUnweightedListenerPresent(_indicator_listener__) != true) {
      UnweightedIndicatorListenerPair _new_unweighted_indicator_listener_pair_(_indicator_index_,
                                                                               _indicator_listener__);
      unweighted_indicator_listener_pairs_.push_back(_new_unweighted_indicator_listener_pair_);
    }
  }
}

bool CommonIndicator::IsUnweightedListenerPresent(IndicatorListener* _indicator_listener__) {
  for (auto i = 0u; i < unweighted_indicator_listener_pairs_.size(); i++) {
    if (unweighted_indicator_listener_pairs_[i].indicator_listener__ == _indicator_listener__) {
      return true;
    }
  }
  return false;
}

void CommonIndicator::OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {
  if (_new_market_status_ == kMktTradingStatusReserved || _new_market_status_ == kMktTradingStatusClosed) {
    indicator_value_ = 0;
    OnMarketDataInterrupted(_security_id_, 900 * 1000);
    DBGLOG_TIME_CLASS_FUNC << "Market Status: " << _new_market_status_ << DBGLOG_ENDL_FLUSH;
  } else {
    OnMarketDataResumed(_security_id_);
  }
  last_mkt_status_ = _new_market_status_;

  // Need common handling for all products
}

void CommonIndicator::SetTimeDecayWeights() {
  const unsigned int kDecayLength =
      20;  ///< here number of samples are not required to be very high and hence the decaylength target is just 20
  const unsigned int kMinPageWidth = 10;
  const unsigned int kMaxPageWidth =
      200;  ///< keeping kMaxPageWidth low makes the number_fadeoffs_ pretty high and hence keeps lots of sample points
  page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (trend_history_msecs_ / kDecayLength)));

  int number_fadeoffs_ = std::max(1, (int)ceil(trend_history_msecs_ / page_width_msecs_));

  decay_page_factor_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

  decay_vector_.resize(2 * number_fadeoffs_);
  decay_vector_sums_.resize(2 * number_fadeoffs_);

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(decay_page_factor_, (int)i);
  }

  decay_vector_sums_[0] = 0;
  for (unsigned int i = 1; i < decay_vector_sums_.size(); i++) {
    decay_vector_sums_[i] = decay_vector_sums_[i - 1] + decay_vector_[i];
  }

  inv_decay_sum_ = (1 - decay_page_factor_);
}
double CommonIndicator::GetPageWidth() { return page_width_msecs_; }
double CommonIndicator::GetDecayPageFactor() { return decay_page_factor_; }
}
