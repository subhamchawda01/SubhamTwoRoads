/**
    \file IndicatorsCode/bid_ask_to_pay.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/bid_ask_to_pay_l1.hpp"

namespace HFSAT {

void BidAskToPayL1::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                      std::vector<std::string>& _ors_source_needed_vec_,
                                      const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

BidAskToPayL1* BidAskToPayL1::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                const std::vector<const char*>& r_tokens_,
                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_levels_ _size_to_seek_ _price_type_
  PriceType_t t_price_type_ = kPriceTypeMax;
  if (r_tokens_.size() < 7) {
    if (r_tokens_.size() < 6) {
      ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                  "BidAskToPayL1 incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                  "_indep_market_view_ _num_levels_ _size_to_seek_ _price_type_ ");
    } else {
      t_dbglogger_ << " BidAskToPayL1 incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                      "_indep_market_view_ _num_levels_ _size_to_seek_ _price_type_ "
                   << "\n";
      t_dbglogger_.CheckToFlushBuffer();
    }
    t_price_type_ = _basepx_pxtype_;
  } else {
    if (std::string(r_tokens_[6]).compare("#") == 0) {
      t_price_type_ = _basepx_pxtype_;
    } else {
      t_price_type_ = StringToPriceType_t(r_tokens_[6]);
    }
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           std::max(0, atoi(r_tokens_[4])), std::max(0, atoi(r_tokens_[5])), t_price_type_);
}

BidAskToPayL1* BidAskToPayL1::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                SecurityMarketView& _indep_market_view_, unsigned int _num_levels_,
                                                unsigned int _size_to_seek_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_levels_ << ' ' << _size_to_seek_
              << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, BidAskToPayL1*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new BidAskToPayL1(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _num_levels_,
                          _size_to_seek_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

BidAskToPayL1::BidAskToPayL1(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                             const std::string& concise_indicator_description_,
                             SecurityMarketView& t_indep_market_view_, unsigned int _num_levels_,
                             unsigned int _size_to_seek_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(t_indep_market_view_),
      price_type_(_price_type_),
      num_levels_(_num_levels_),
      size_to_seek_(_size_to_seek_),
      l1_size_ind_(NULL) {
  t_indep_market_view_.subscribe_price_type(this, kPriceTypeMidprice);  // KP : not sure if we need this
  l1_size_ind_ =
      L1SizeTrend::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_, 600, kPriceTypeMktSizeWPrice);
  if (!t_indep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  t_indep_market_view_.subscribe_L2(this);
}

void BidAskToPayL1::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_) {
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

    int sum_bid_size_ = 0;
    double sum_bid_px_size_ = 0;
    int sum_ask_size_ = 0;
    double sum_ask_px_size_ = 0;
    const unsigned int current_avg_l1_sz_ = l1_size_ind_->GetL1Factor();
    int bid_size_left_to_seek_ = std::max(current_avg_l1_sz_, size_to_seek_);
    int ask_size_left_to_seek_ = bid_size_left_to_seek_;

    if (indep_market_view_.market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      for (unsigned int t_current_bid_index_ = indep_market_view_.GetBaseBidMapIndex(), i = 0;
           (i < num_levels_) && (t_current_bid_index_ > 0); i++) {
        const MarketUpdateInfoLevelStruct& this_bid_level_ =
            indep_market_view_.GetBidLevelAtIndex(t_current_bid_index_);
        double this_bid_size_ = this_bid_level_.limit_size_;
        double this_bid_price_ = this_bid_level_.limit_price_;

        if (this_bid_size_ >= bid_size_left_to_seek_) {
          sum_bid_size_ += bid_size_left_to_seek_;
          sum_bid_px_size_ += bid_size_left_to_seek_ * this_bid_price_;
          bid_size_left_to_seek_ = 0;
          break;
        } else {
          sum_bid_size_ += this_bid_size_;
          sum_bid_px_size_ += this_bid_size_ * this_bid_price_;
          bid_size_left_to_seek_ -= this_bid_size_;
        }

        // increment to next level .. will be 0 if error
        t_current_bid_index_ = indep_market_view_.GetNextBidMapIndex(t_current_bid_index_);
      }
      for (unsigned int t_current_ask_index_ = indep_market_view_.GetBaseAskMapIndex(), i = 0;
           (i < num_levels_) && (t_current_ask_index_ > 0); i++) {
        const MarketUpdateInfoLevelStruct& this_ask_level_ =
            indep_market_view_.GetAskLevelAtIndex(t_current_ask_index_);
        double this_ask_size_ = this_ask_level_.limit_size_;
        double this_ask_price_ = this_ask_level_.limit_price_;

        if (this_ask_size_ >= ask_size_left_to_seek_) {
          sum_ask_size_ += ask_size_left_to_seek_;
          sum_ask_px_size_ += ask_size_left_to_seek_ * this_ask_price_;
          ask_size_left_to_seek_ = 0;
          break;
        } else {
          sum_ask_size_ += this_ask_size_;
          sum_ask_px_size_ += this_ask_size_ * this_ask_price_;
          ask_size_left_to_seek_ -= this_ask_size_;
        }

        // increment to next level .. will be 0 if error
        t_current_ask_index_ = indep_market_view_.GetNextAskMapIndex(t_current_ask_index_);
      }
    } else {
      for (auto i = 0u; i < num_levels_ && i < indep_market_view_.bidlevels_size(); i++) {
        if (indep_market_view_.bid_size(i) >= bid_size_left_to_seek_) {
          sum_bid_size_ += bid_size_left_to_seek_;
          sum_bid_px_size_ += bid_size_left_to_seek_ * indep_market_view_.bid_price(i);
          bid_size_left_to_seek_ = 0;
          break;
        } else {
          sum_bid_size_ += indep_market_view_.bid_size(i);
          sum_bid_px_size_ += indep_market_view_.bid_size(i) * indep_market_view_.bid_price(i);
          bid_size_left_to_seek_ -= indep_market_view_.bid_size(i);
        }
      }

      for (auto i = 0u; i < num_levels_ && i < indep_market_view_.asklevels_size(); i++) {
        if (indep_market_view_.ask_size(i) >= ask_size_left_to_seek_) {
          sum_ask_size_ += ask_size_left_to_seek_;
          sum_ask_px_size_ += ask_size_left_to_seek_ * indep_market_view_.ask_price(i);
          ask_size_left_to_seek_ = 0;
          break;
        } else {
          sum_ask_size_ += indep_market_view_.ask_size(i);
          sum_ask_px_size_ += indep_market_view_.ask_size(i) * indep_market_view_.ask_price(i);
          ask_size_left_to_seek_ -= indep_market_view_.ask_size(i);
        }
      }
    }

#define MIN_SIGNIFICANT_FACTOR 1
    if ((sum_bid_size_ <= MIN_SIGNIFICANT_FACTOR) ||
        (sum_ask_size_ <= MIN_SIGNIFICANT_FACTOR)) {  // this means there are no valid entries in that side since
                                                      // oterwise even top level makes this sum >= 1.0
      indicator_value_ = 0;
    } else {
      double proj_price_ =
          std::max(cr_market_update_info_.mid_price_ - indep_market_view_.min_price_increment(),
                   std::min(cr_market_update_info_.mid_price_ + indep_market_view_.min_price_increment(),
                            (((sum_bid_px_size_ / sum_bid_size_) + (sum_ask_px_size_ / sum_ask_size_)) / 2.00)));
      indicator_value_ = proj_price_ - SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    }
#undef MIN_SIGNIFICANT_FACTOR

    //      if(data_interrupted_)
    //       indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void BidAskToPayL1::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void BidAskToPayL1::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}
}
