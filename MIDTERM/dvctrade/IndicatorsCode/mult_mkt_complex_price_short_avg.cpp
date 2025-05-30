/**
    \file IndicatorsCode/mult_mkt_complex_price_short_avg.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvctrade/Indicators/mult_mkt_complex_price_short_avg.hpp"

namespace HFSAT {

void MultMktComplexPriceShortAvg::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                    std::vector<std::string>& _ors_source_needed_vec_,
                                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

MultMktComplexPriceShortAvg* MultMktComplexPriceShortAvg::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                            const Watch& r_watch_,
                                                                            const std::vector<const char*>& r_tokens_,
                                                                            PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_levels_ _decay_factor_ _price_type_
  PriceType_t t_price_type_ = kPriceTypeMax;
  double t_stdev_duration_ = 0.0;
  if (r_tokens_.size() < 7) {
    if (r_tokens_.size() < 6) {
      ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                  "MultMktComplexPriceShortAvg incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                  "_indep_market_view_ _num_levels_ _decay_factor_ _price_type_");
    } else {
      t_dbglogger_ << "MultMktComplexPriceShortAvg incorrect syntax. Should be INDICATOR _this_weight_ "
                      "_indicator_string_ _indep_market_view_ _num_levels_ _decay_factor_ _price_type_"
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

MultMktComplexPriceShortAvg* MultMktComplexPriceShortAvg::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _indep_market_view_,
    unsigned int _num_levels_, double _decay_factor_, PriceType_t _price_type_, double stdev_duration) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_levels_ << ' ' << _decay_factor_
              << " " << stdev_duration << " " << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, MultMktComplexPriceShortAvg*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new MultMktComplexPriceShortAvg(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                        _num_levels_, _decay_factor_, _price_type_, stdev_duration);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

MultMktComplexPriceShortAvg::MultMktComplexPriceShortAvg(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                         const std::string& concise_indicator_description_,
                                                         SecurityMarketView& _indep_market_view_,
                                                         unsigned int _num_levels_, double _decay_factor_,
                                                         PriceType_t _price_type_, double stdev_duration)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      num_levels_(std::max(1u, _num_levels_)),
      decay_factor_(_decay_factor_),
      decay_vector_(std::max(24u, (5 * 2u * _num_levels_ + 8u)), 0),  // 5 times required size
      short_history_averaging_(5),
      current_dep_stdev_ratio_(1.0),
      last_decay_factor_update_msecs_(0) {  // kComplexPriceShortAvgLength = 5
                                            //

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    int fact = (int)((double)i / current_dep_stdev_ratio_);
    decay_vector_[i] = pow(sqrt(decay_factor_), (int)fact);
  }

  indep_market_view_.subscribe_price_type(this, kPriceTypeMidprice);  // needed for mid_price_
  if (!indep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  if (stdev_duration > 0) {
    StdevRatioCalculator* stdev_ratio_calculator =
        StdevRatioCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_, stdev_duration, false);
    stdev_ratio_calculator->AddStdevRatioListener(0u, this);
  }
  indep_market_view_.subscribe_L2(this);
  indep_market_view_.ComputeIntPriceLevels();
}

void MultMktComplexPriceShortAvg::OnMarketUpdate(const unsigned int _security_id_,
                                                 const MarketUpdateInfo& cr_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready() &&
        (indep_market_view_.spread_increments() < (2 * indep_market_view_.normal_spread_increments()))) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else if (!data_interrupted_) {
    if (cr_market_update_info_.bestbid_int_price_ != indep_market_view_.bid_int_price(0) ||
        cr_market_update_info_.bestask_int_price_ != indep_market_view_.ask_int_price(0)) {
      indicator_value_ = 0;
      return;
    }

    unsigned int base_level_ = indep_market_view_.spread_increments();

    double sum_bid_price_size_ = 0;
    double sum_bid_size_ = 0;
    double sum_ask_price_size_ = 0;
    double sum_ask_size_ = 0;

    unsigned int new_num_levels = (unsigned int)(num_levels_ * current_dep_stdev_ratio_);
    if (indep_market_view_.market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      // better handling of bid levels in indexed book

      double this_bid_size_ = indep_market_view_.bestbid_size();
      double this_bid_price_ = indep_market_view_.bestbid_price();
      unsigned t_current_bid_index_ = indep_market_view_.GetBidIndex(indep_market_view_.bestbid_int_price());
      unsigned int this_bid_int_price_level_ = base_level_;
      double this_bid_decay_factor_ = decay_vector_[this_bid_int_price_level_];
      unsigned bid_levels_so_far_ = 0;

      do {
        sum_bid_price_size_ += this_bid_decay_factor_ * this_bid_size_ * this_bid_price_;
        sum_bid_size_ += this_bid_decay_factor_ * this_bid_size_;

        t_current_bid_index_ = indep_market_view_.GetNextBidMapIndex(t_current_bid_index_);
        this_bid_int_price_level_ = base_level_ +
                                    2 * (indep_market_view_.GetBidIndex(indep_market_view_.bestbid_int_price()) -
                                         t_current_bid_index_);  // In Indexed Book bid_int_price_level=base_bid_index_
                                                                 // - t_current_bid_index_ ( since levels are 1
                                                                 // min_price_increment_ away )
        if (this_bid_int_price_level_ >= (2 * new_num_levels)) {
          break;
        }
        this_bid_decay_factor_ = decay_vector_[this_bid_int_price_level_];
        const MarketUpdateInfoLevelStruct& this_bid_level_ =
            indep_market_view_.GetBidLevelAtIndex(t_current_bid_index_);
        this_bid_size_ = this_bid_level_.limit_size_;
        this_bid_price_ = this_bid_level_.limit_price_;

      } while ((bid_levels_so_far_ < new_num_levels) && (t_current_bid_index_ > 0));

      double this_ask_size_ = indep_market_view_.bestask_size();
      double this_ask_price_ = indep_market_view_.bestask_price();
      unsigned t_current_ask_index_ = indep_market_view_.GetAskIndex(indep_market_view_.bestask_int_price());
      unsigned int this_ask_int_price_level_ = base_level_;
      double this_ask_decay_factor_ = decay_vector_[this_ask_int_price_level_];
      unsigned ask_levels_so_far_ = 0;

      do {
        sum_ask_price_size_ += this_ask_decay_factor_ * this_ask_size_ * this_ask_price_;
        sum_ask_size_ += this_ask_decay_factor_ * this_ask_size_;

        t_current_ask_index_ = indep_market_view_.GetNextAskMapIndex(t_current_ask_index_);
        this_ask_int_price_level_ = base_level_ +
                                    2 * (indep_market_view_.GetAskIndex(indep_market_view_.bestask_int_price()) -
                                         t_current_ask_index_);  // In Indexed Book ask_int_price_level=base_ask_index_
                                                                 // - t_current_ask_index_ ( since levels are 1
                                                                 // min_price_increment_ away )
        if (this_ask_int_price_level_ >= (2 * new_num_levels)) {
          break;
        }
        this_ask_decay_factor_ = decay_vector_[this_ask_int_price_level_];
        const MarketUpdateInfoLevelStruct& this_ask_level_ =
            indep_market_view_.GetAskLevelAtIndex(t_current_ask_index_);
        this_ask_size_ = this_ask_level_.limit_size_;
        this_ask_price_ = this_ask_level_.limit_price_;

      } while ((ask_levels_so_far_ < new_num_levels) && (t_current_ask_index_ > 0));

    } else {
      unsigned bid_index_ = 0;
      double this_bid_size_ = indep_market_view_.bestbid_size();
      double this_bid_price_ = indep_market_view_.bestbid_price();
      while (bid_index_ < indep_market_view_.bidlevels_size() &&
             indep_market_view_.bid_int_price(bid_index_) > indep_market_view_.bestbid_int_price()) {
        bid_index_++;
      }
      int best_bid_int_price_level_ = indep_market_view_.bid_int_price_level(bid_index_);
      unsigned bid_level_ = bid_index_;
      unsigned bid_levels_so_far_ = 0;

      do {
        int t_bid_int_price_level_ = indep_market_view_.bid_int_price_level(bid_level_);
        int bid_int_price_level_ = t_bid_int_price_level_ - best_bid_int_price_level_;  // always b +ve number ?
        double this_bid_int_price_level_ = base_level_ + 2 * bid_int_price_level_;
        if (this_bid_int_price_level_ >= 2 * new_num_levels) {
          break;
        }

        double this_bid_decay_factor_ = decay_vector_[this_bid_int_price_level_];
        sum_bid_price_size_ += this_bid_decay_factor_ * this_bid_size_ * this_bid_price_;
        sum_bid_size_ += this_bid_decay_factor_ * this_bid_size_;

        bid_level_++;
        bid_levels_so_far_++;
        this_bid_size_ = indep_market_view_.bid_size(bid_level_);
        this_bid_price_ = indep_market_view_.bid_price(bid_level_);

      } while (bid_levels_so_far_ < new_num_levels && bid_level_ < indep_market_view_.bidlevels_size());

      unsigned ask_index_ = 0;
      double this_ask_size_ = indep_market_view_.bestask_size();
      double this_ask_price_ = indep_market_view_.bestask_price();
      while (ask_index_ < indep_market_view_.asklevels_size() &&
             indep_market_view_.ask_int_price(ask_index_) < indep_market_view_.bestask_int_price()) {
        ask_index_++;
      }
      unsigned best_ask_int_price_level_ = indep_market_view_.ask_int_price_level(ask_index_);

      unsigned ask_level_ = ask_index_;
      unsigned ask_levels_so_far_ = 0;

      do {
        int t_ask_int_price_level_ = indep_market_view_.ask_int_price_level(ask_level_);
        int this_ask_int_price_level_ = t_ask_int_price_level_ - best_ask_int_price_level_;
        double this_ask_decay_factor_ = decay_vector_[this_ask_int_price_level_];
        sum_ask_price_size_ += this_ask_decay_factor_ * this_ask_size_ * this_ask_price_;
        sum_ask_size_ += this_ask_decay_factor_ * indep_market_view_.ask_size(ask_level_);

        ask_level_++;
        ask_levels_so_far_++;
        this_ask_size_ = indep_market_view_.ask_size(ask_level_);
        this_ask_price_ = indep_market_view_.ask_price(ask_level_);

      } while (ask_levels_so_far_ < new_num_levels && ask_level_ < indep_market_view_.asklevels_size());
    }

    double this_indicator_value_ = 0;
    if ((sum_bid_size_ > 0) && (sum_ask_size_ > 0)) {
      double adjusted_price_ = (2 * indep_market_view_.mid_price()) -
                               ((sum_bid_price_size_ + sum_ask_price_size_) / (sum_bid_size_ + sum_ask_size_));
      this_indicator_value_ =
          adjusted_price_ - SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    }

    short_history_averaging_.push_replace(this_indicator_value_);
    indicator_value_ = short_history_averaging_.average();

    NotifyIndicatorListeners(indicator_value_);
  }
}

void MultMktComplexPriceShortAvg::OnStdevRatioUpdate(const unsigned int index_to_send,
                                                     const double& r_new_scaled_volume_value) {
  if (index_to_send == 0u) {
    current_dep_stdev_ratio_ = r_new_scaled_volume_value;
    current_dep_stdev_ratio_ = std::min(STDEV_RATIO_CAP, current_dep_stdev_ratio_);
  }
  if (watch_.msecs_from_midnight() - last_decay_factor_update_msecs_ >= 600000) {
    for (auto i = 0u; i < decay_vector_.size(); i++) {
      int fact = (int)((double)i / current_dep_stdev_ratio_);
      decay_vector_[i] = pow(sqrt(decay_factor_), (int)fact);
    }
    last_decay_factor_update_msecs_ = watch_.msecs_from_midnight();
  }
}
void MultMktComplexPriceShortAvg::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                          const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void MultMktComplexPriceShortAvg::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
    // kind of initialize
    for (int i = 0; i < 5; i++)  // kComplexPriceShortAvgLength = 5
      short_history_averaging_.push_replace(0.0);
  } else
    return;
}
}
