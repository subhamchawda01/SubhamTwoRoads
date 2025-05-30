/**
    \file IndicatorsCode/mult_mkt_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/mult_mkt_price.hpp"

namespace HFSAT {

void MultMktPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                     std::vector<std::string>& _ors_source_needed_vec_,
                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

MultMktPrice* MultMktPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_levels_ _decay_factor_ _price_type_
  PriceType_t t_price_type_ = kPriceTypeMax;
  double t_stdev_duration_ = 0;
  if (r_tokens_.size() < 7) {
    if (r_tokens_.size() < 6) {
      ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                  "MultMktPrice incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                  "_indep_market_view_ _num_levels_ _decay_factor_ _duration_ _price_type_ ");
    } else {
      t_dbglogger_ << "MultMktPrice incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                      "_indep_market_view_ _num_levels_ _decay_factor_ _price_type_ "
                   << "\n";
      t_dbglogger_.CheckToFlushBuffer();
      t_price_type_ = _basepx_pxtype_;
    }
  }

  t_price_type_ = _basepx_pxtype_;
  IndicatorUtil::GetLastTwoArgsFromIndicatorTokens(6, r_tokens_, t_stdev_duration_, t_price_type_);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           std::max(0, atoi(r_tokens_[4])), atof(r_tokens_[5]), t_price_type_, t_stdev_duration_);
}

MultMktPrice* MultMktPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              SecurityMarketView& _indep_market_view_, unsigned int _num_levels_,
                                              double _decay_factor_, PriceType_t _price_type_,
                                              double _stdev_duration_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_levels_ << ' ' << _decay_factor_
              << " " << _stdev_duration_ << " " << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, MultMktPrice*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new MultMktPrice(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _num_levels_,
                         _decay_factor_, _price_type_, _stdev_duration_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

MultMktPrice::MultMktPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                           const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                           unsigned int _num_levels_, double _decay_factor_, PriceType_t _price_type_,
                           double _stdev_duration_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      num_levels_(std::max((unsigned int)1, _num_levels_)),
      decay_factor_(_decay_factor_),
      stdev_duration_(_stdev_duration_),
      decay_vector_(std::max(24u, (2u * _num_levels_ + 8u)), 0),
      book_info_manager_(*(BookInfoManager::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_))) {
  book_info_manager_.ComputeSumFactorSize(num_levels_, decay_factor_, stdev_duration_);
  book_info_manager_.ComputeSumFactorPriceSize(num_levels_, decay_factor_, stdev_duration_);

  book_info_struct_ = book_info_manager_.GetBookInfoStruct(num_levels_, decay_factor_, stdev_duration_);

  if (book_info_struct_ == NULL) {
    std::cerr << "Error getting book_info_struct for MultMktPrice: " << _num_levels_ << " " << _decay_factor_
              << std::endl;
  }

  unsigned int num_exponents_ = std::max((unsigned int)16, 2 * _num_levels_);
  decay_vector_.resize(num_exponents_);

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(sqrt(decay_factor_), (int)i);
  }

  if (!_indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  indep_market_view_.subscribe_L2(this);
  indep_market_view_.ComputeIntPriceLevels();
}

void MultMktPrice::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else {
    if (cr_market_update_info_.bestbid_int_price_ != indep_market_view_.bid_int_price(0) ||
        cr_market_update_info_.bestask_int_price_ != indep_market_view_.ask_int_price(0)) {
      indicator_value_ = 0;
      return;
    }

    if ((book_info_struct_->sum_bid_factor_size_ <= 0) || (book_info_struct_->sum_ask_factor_size_ <= 0)) {
      indicator_value_ = 0;
    } else {
      double adjusted_price_ = (((book_info_struct_->sum_bid_factor_price_size_ *
                                  book_info_struct_->sum_ask_factor_size_ / book_info_struct_->sum_bid_factor_size_) +
                                 (book_info_struct_->sum_ask_factor_price_size_ *
                                  book_info_struct_->sum_bid_factor_size_ / book_info_struct_->sum_ask_factor_size_)) /
                                (book_info_struct_->sum_bid_factor_size_ + book_info_struct_->sum_ask_factor_size_));
      indicator_value_ = adjusted_price_ - SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    }

    if (std::isnan(indicator_value_)) {
      std::cerr << __PRETTY_FUNCTION__ << " nan in " << concise_indicator_description() << std::endl;
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void MultMktPrice::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void MultMktPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}
}
