/**
    \file IndicatorsCode/mult_mkt_per_order_complex_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"

#include "dvctrade/Indicators/mult_mkt_per_order_complex_price.hpp"

namespace HFSAT {

void MultMktPerOrderComplexPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                    std::vector<std::string>& _ors_source_needed_vec_,
                                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

MultMktPerOrderComplexPrice* MultMktPerOrderComplexPrice::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                            const Watch& r_watch_,
                                                                            const std::vector<const char*>& r_tokens_,
                                                                            PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_levels_ _decay_factor_ _num_events_avg_
  // _price_type_
  PriceType_t t_price_type_ = kPriceTypeMax;
  if (r_tokens_.size() < 8) {
    if (r_tokens_.size() < 7) {
      ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                  "MultMktPerOrderComplexPrice incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                  "_indep_market_view_ _num_levels_ _decay_factor_ _num_events_avg_ _price_type_ ");
    } else {
      t_dbglogger_ << "MultMktPerOrderComplexPrice incorrect syntax. Should be INDICATOR _this_weight_ "
                      "_indicator_string_ _indep_market_view_ _num_levels_ _decay_factor_ _num_events_avg_ "
                      "_price_type_ "
                   << "\n";
      t_dbglogger_.CheckToFlushBuffer();
      t_price_type_ = _basepx_pxtype_;
    }
  } else {
    if (std::string(r_tokens_[7]).compare("#") == 0) {
      t_price_type_ = _basepx_pxtype_;
    } else {
      t_price_type_ = StringToPriceType_t(r_tokens_[7]);
    }
  }

  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      std::max(0, atoi(r_tokens_[4])), atof(r_tokens_[5]), std::max(0, atoi(r_tokens_[6])), t_price_type_);
}

MultMktPerOrderComplexPrice* MultMktPerOrderComplexPrice::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _indep_market_view_,
    unsigned int _num_levels_, double _decay_factor_, int _num_events_avg_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_levels_ << ' ' << _decay_factor_
              << " " << _num_events_avg_ << " " << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, MultMktPerOrderComplexPrice*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new MultMktPerOrderComplexPrice(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                        _num_levels_, _decay_factor_, _num_events_avg_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

MultMktPerOrderComplexPrice::MultMktPerOrderComplexPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                         const std::string& concise_indicator_description_,
                                                         SecurityMarketView& _indep_market_view_,
                                                         unsigned int _num_levels_, double _decay_factor_,
                                                         int _num_events_avg_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      num_levels_(std::max(1u, _num_levels_)),
      decay_factor_(_decay_factor_),
      decay_vector_(std::max(24u, (2u * _num_levels_ + 8u)), 0),
      sum_bid_price_size_(0),
      sum_bid_size_(0),
      sum_ask_price_size_(0),
      sum_ask_size_(0),
      buffer_len_(_num_events_avg_),
      bid_mkt_size_per_order_(),
      ask_mkt_size_per_order_() {
  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(sqrt(decay_factor_), (int)i);
  }

  for (auto i = 0u; i < 2 * num_levels_; i++) {
    FixedBufferAveraging<double> bid_dummy_(buffer_len_);
    FixedBufferAveraging<double> ask_dummy_(buffer_len_);
    bid_mkt_size_per_order_.push_back(bid_dummy_);
    ask_mkt_size_per_order_.push_back(ask_dummy_);
  }

  indep_market_view_.subscribe_price_type(this, kPriceTypeMidprice);  // needed for mid_price_
  if (!_indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  indep_market_view_.subscribe_L2(this);
  indep_market_view_.ComputeIntPriceLevels();
}

void MultMktPerOrderComplexPrice::OnMarketUpdate(const unsigned int _security_id_,
                                                 const MarketUpdateInfo& cr_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready() &&
        (indep_market_view_.spread_increments() < (2 * indep_market_view_.normal_spread_increments()))) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else {
    unsigned int base_level_ = indep_market_view_.spread_increments();  // assumption is this is a positive value

    sum_bid_price_size_ = 0;
    sum_bid_size_ = 0;
    sum_ask_price_size_ = 0;
    sum_ask_size_ = 0;

    if (indep_market_view_.market_update_info_
            .temporary_bool_checking_if_this_is_an_indexed_book_) {  // better handling of bid levels in indexed book

      for (unsigned int t_current_bid_index_ = indep_market_view_.GetBaseBidMapIndex(), i = 0;
           (i < num_levels_) && (t_current_bid_index_ > 0); i++) {
        unsigned int this_bid_int_price_level_ =
            base_level_ +
            2 * (indep_market_view_.GetBaseBidMapIndex() -
                 t_current_bid_index_);  // In Indexed Book bid_int_price_level=base_bid_index_ - t_current_bid_index_ (
                                         // since levels are 1 min_price_increment_ away )
        if (this_bid_int_price_level_ >= (2 * num_levels_)) {
          break;
        }

        double this_bid_decay_factor_ = decay_vector_[this_bid_int_price_level_];
        const MarketUpdateInfoLevelStruct& this_bid_level_ =
            indep_market_view_.GetBidLevelAtIndex(t_current_bid_index_);
        double this_bid_size_ = this_bid_level_.limit_size_;
        double current_mktsize_per_order_bid_ = this_bid_size_ / this_bid_level_.limit_ordercount_;
        bid_mkt_size_per_order_[this_bid_int_price_level_].push_replace(current_mktsize_per_order_bid_);
        double bid_size_adj_ratio_ = std::min(
            1.2, std::max(bid_mkt_size_per_order_[this_bid_int_price_level_].average() / current_mktsize_per_order_bid_,
                          0.8));
        double this_bid_price_ = this_bid_level_.limit_price_;
        sum_bid_price_size_ += this_bid_decay_factor_ * this_bid_size_ * this_bid_price_ * bid_size_adj_ratio_;
        sum_bid_size_ += this_bid_decay_factor_ * this_bid_size_ * bid_size_adj_ratio_;

        // increment to next level .. will be 0 if error
        t_current_bid_index_ = indep_market_view_.GetNextBidMapIndex(t_current_bid_index_);
      }

      for (unsigned int t_current_ask_index_ = indep_market_view_.GetBaseAskMapIndex(), i = 0;
           (i < num_levels_) && (t_current_ask_index_ > 0); i++) {
        unsigned int this_ask_int_price_level_ =
            base_level_ + 2 * (indep_market_view_.GetBaseAskMapIndex() - t_current_ask_index_);  // In Indexed Book
        // ask_int_price_level=base_ask_index_ -
        // t_current_ask_index_ ( since levels are
        // 1 min_price_increment_ away )
        if (this_ask_int_price_level_ >= (2 * num_levels_)) {
          break;
        }

        double this_ask_decay_factor_ = decay_vector_[this_ask_int_price_level_];
        const MarketUpdateInfoLevelStruct& this_ask_level_ =
            indep_market_view_.GetAskLevelAtIndex(t_current_ask_index_);
        double this_ask_size_ = this_ask_level_.limit_size_;
        double current_mktsize_per_order_ask_ = this_ask_size_ / this_ask_level_.limit_ordercount_;
        ask_mkt_size_per_order_[this_ask_int_price_level_].push_replace(current_mktsize_per_order_ask_);
        double ask_size_adj_ratio_ = std::min(
            1.2, std::max(ask_mkt_size_per_order_[this_ask_int_price_level_].average() / current_mktsize_per_order_ask_,
                          0.8));
        double this_ask_price_ = this_ask_level_.limit_price_;
        sum_ask_price_size_ += this_ask_decay_factor_ * this_ask_size_ * this_ask_price_ * ask_size_adj_ratio_;
        sum_ask_size_ += this_ask_decay_factor_ * this_ask_size_ * ask_size_adj_ratio_;

        // increment to next level .. will be 0 if error
        t_current_ask_index_ = indep_market_view_.GetNextAskMapIndex(t_current_ask_index_);
      }

    } else {
      for (auto i = 0u; i < num_levels_ && i < indep_market_view_.bidlevels_size(); i++) {
        unsigned int this_bid_int_price_level_ =
            base_level_ +
            2 * indep_market_view_.bid_int_price_level(i);  // assumption bid_int_price_level is always positive
        if (this_bid_int_price_level_ >= 2 * num_levels_) break;

        double current_mktsize_per_order_bid_ = indep_market_view_.bid_size(i) / indep_market_view_.bid_order(i);
        bid_mkt_size_per_order_[this_bid_int_price_level_].push_replace(current_mktsize_per_order_bid_);
        double bid_size_adj_ratio_ = std::min(
            1.2, std::max(bid_mkt_size_per_order_[this_bid_int_price_level_].average() / current_mktsize_per_order_bid_,
                          0.8));
        double this_bid_decay_factor_ = decay_vector_[this_bid_int_price_level_];
        sum_bid_price_size_ += this_bid_decay_factor_ * indep_market_view_.bid_size(i) *
                               indep_market_view_.bid_price(i) * bid_size_adj_ratio_;
        sum_bid_size_ += this_bid_decay_factor_ * indep_market_view_.bid_size(i) * bid_size_adj_ratio_;
      }

      for (auto i = 0u; i < num_levels_ && i < indep_market_view_.asklevels_size(); i++) {
        unsigned int this_ask_int_price_level_ =
            base_level_ +
            2 * indep_market_view_.ask_int_price_level(i);  // assumption ask_int_price_level is always positive
        if (this_ask_int_price_level_ >= 2 * num_levels_) break;

        double current_mktsize_per_order_ask_ = indep_market_view_.ask_size(i) / indep_market_view_.ask_order(i);
        ask_mkt_size_per_order_[this_ask_int_price_level_].push_replace(current_mktsize_per_order_ask_);
        double ask_size_adj_ratio_ = std::min(
            1.2, std::max(ask_mkt_size_per_order_[this_ask_int_price_level_].average() / current_mktsize_per_order_ask_,
                          0.8));
        double this_ask_decay_factor_ = decay_vector_[this_ask_int_price_level_];
        sum_ask_price_size_ += this_ask_decay_factor_ * indep_market_view_.ask_size(i) *
                               indep_market_view_.ask_price(i) * ask_size_adj_ratio_;
        sum_ask_size_ += this_ask_decay_factor_ * indep_market_view_.ask_size(i) * ask_size_adj_ratio_;
      }
    }

    if ((sum_bid_size_ <= 0) || (sum_ask_size_ <= 0)) {
      indicator_value_ = 0;
    } else {
      double adjusted_price_ = (2 * indep_market_view_.mid_price()) -
                               ((sum_bid_price_size_ + sum_ask_price_size_) / (sum_bid_size_ + sum_ask_size_));
      // calling GetPriceFromType instead of indep_market_view_.price_from_type in the hope that this will be inlined to
      // a faster lookup from the memory ref
      indicator_value_ = adjusted_price_ - SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    }

    if (data_interrupted_) indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void MultMktPerOrderComplexPrice::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                          const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void MultMktPerOrderComplexPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
    bid_mkt_size_per_order_.clear();
    ask_mkt_size_per_order_.clear();
    for (auto i = 0u; i < 2 * num_levels_; i++) {
      FixedBufferAveraging<double> bid_dummy_(buffer_len_);
      FixedBufferAveraging<double> ask_dummy_(buffer_len_);
      bid_mkt_size_per_order_.push_back(bid_dummy_);
      ask_mkt_size_per_order_.push_back(ask_dummy_);
    }
  } else
    return;
}
}
