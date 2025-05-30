/**
    \file IndicatorsCode/book_order_cdiff.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/book_order_cdiff.hpp"

namespace HFSAT {

void BookOrderCDiff::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                       std::vector<std::string>& _ors_source_needed_vec_,
                                       const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

BookOrderCDiff* BookOrderCDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  const std::vector<const char*>& r_tokens_,
                                                  PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_levels_ _decay_factor_
  double t_stdev_duration_ = 0;
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "BookOrderCDiff incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                "_indep_market_view_ _num_levels_ _decay_factor_ ");
  } else if (r_tokens_.size() >= 7) {
    if (std::string(r_tokens_[6]).compare("#") != 0) {
      t_stdev_duration_ = atof(r_tokens_[6]);
    }
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           std::max(0, atoi(r_tokens_[4])), atof(r_tokens_[5]), _basepx_pxtype_, t_stdev_duration_);
}

BookOrderCDiff* BookOrderCDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                  SecurityMarketView& _indep_market_view_, unsigned int _num_levels_,
                                                  double _decay_factor_, PriceType_t _basepx_pxtype_,
                                                  double _stdev_duration_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_levels_ << ' ' << _decay_factor_
              << " " << _stdev_duration_ << PriceType_t_To_String(_basepx_pxtype_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, BookOrderCDiff*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new BookOrderCDiff(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _num_levels_,
                           _decay_factor_, _stdev_duration_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

BookOrderCDiff::BookOrderCDiff(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                               const std::string& concise_indicator_description_,
                               SecurityMarketView& _indep_market_view_, unsigned int _num_levels_,
                               double _decay_factor_, double _stdev_duration_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      num_levels_(_num_levels_),
      decay_factor_(_decay_factor_),
      stdev_duration_(_stdev_duration_),
      decay_vector_(std::max(24u, (2u * _num_levels_ + 8u)), 0),
      book_info_manager_(*(BookInfoManager::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_))) {
  book_info_manager_.ComputeSumFactor(num_levels_, decay_factor_, stdev_duration_);
  book_info_manager_.ComputeSumFactorOrder(num_levels_, decay_factor_, stdev_duration_);

  book_info_struct_ = book_info_manager_.GetBookInfoStruct(num_levels_, decay_factor_, stdev_duration_);

  if (book_info_struct_ == NULL) {
    std::cerr << "Error getting book_info_struct for MultMktPrice: " << _num_levels_ << " " << _decay_factor_
              << std::endl;
  }

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(sqrt(decay_factor_), (int)i);
  }

  indep_market_view_.subscribe_L2(this);
  indep_market_view_.ComputeIntPriceLevels();
}

void BookOrderCDiff::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else  // if ( !data_interrupted_ )
  {
    if (cr_market_update_info_.bestbid_int_price_ != indep_market_view_.bid_int_price(0) ||
        cr_market_update_info_.bestask_int_price_ != indep_market_view_.ask_int_price(0)) {
      indicator_value_ = 0;
      return;
    }

#define MIN_SIGNIFICANT_FACTOR 1.00
    if ((book_info_struct_->sum_bid_factor_ <= MIN_SIGNIFICANT_FACTOR) ||
        (book_info_struct_->sum_ask_factor_ <= MIN_SIGNIFICANT_FACTOR)) {  // this means there are no valid entries in
                                                                           // that side since oterwise even top level
                                                                           // makes this sum >= 1.0
      indicator_value_ = 0;
    } else {
      register double avg_bid_ordercount_ =
          (book_info_struct_->sum_bid_factor_order_ / book_info_struct_->sum_bid_factor_);
      register double avg_ask_ordercount_ =
          (book_info_struct_->sum_ask_factor_order_ / book_info_struct_->sum_ask_factor_);
      if (avg_bid_ordercount_ + avg_ask_ordercount_ > 1) {
        indicator_value_ = (avg_bid_ordercount_ - avg_ask_ordercount_) / (avg_bid_ordercount_ + avg_ask_ordercount_);
      } else {
        indicator_value_ = 0;
      }
    }
#undef MIN_SIGNIFICANT_FACTOR

    if (std::isnan(indicator_value_)) {
      DBGLOG_TIME_CLASS_FUNC << "iv=nan : " << concise_indicator_description() << DBGLOG_ENDL_FLUSH;
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void BookOrderCDiff::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void BookOrderCDiff::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}
}
