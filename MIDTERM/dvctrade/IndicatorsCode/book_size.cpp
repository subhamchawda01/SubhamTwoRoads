/*
 * book_size.cpp
 *
 *  Created on: 22-Mar-2016
 *      Author: raghuram
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/book_size.hpp"

namespace HFSAT {

void BookSize::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                 std::vector<std::string>& _ors_source_needed_vec_,
                                 const std::vector<const char*>& _tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)_tokens_[3]);
}

BookSize* BookSize::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                      const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _sec_market_view_ level_ _side_
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight BookSize _sec_market_view_ _level_ _side_");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), std::string(r_tokens_[5]), _basepx_pxtype_);
}

BookSize* BookSize::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                      const SecurityMarketView& _sec_market_view_, unsigned int _level_,
                                      std::string _side_, PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _sec_market_view_.secname() << ' ' << _level_ << ' ' << _side_
              << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, BookSize*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new BookSize(
        t_dbglogger_, r_watch_, concise_indicator_description_, _sec_market_view_, _level_, _side_, _basepx_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

BookSize::BookSize(DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
                   const SecurityMarketView& _sec_market_view_, unsigned int _level_, std::string _side_,
                   PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      sec_market_view_(_sec_market_view_),
      level_(_level_),
      side_(_side_),
      size_(0) {
  if (!sec_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void BookSize::WhyNotReady() {
  if (!is_ready_) {
    if (!(sec_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << sec_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void BookSize::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (sec_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (side_ == "Bid") {
      index_ = sec_market_view_.GetBidIndex(sec_market_view_.bestbid_int_price());
    } else {
      index_ = sec_market_view_.GetAskIndex(sec_market_view_.bestask_int_price());
    }

    for (auto i = 0u; i < level_; i++) {
      if (side_ == "Bid") {
        index_ = sec_market_view_.IndexedBookGetNextNonEmptyBidMapIndex(index_);
      } else {
        index_ = sec_market_view_.IndexedBookGetNextNonEmptyAskMapIndex(index_);
      }
    }

    if (side_ == "Bid") {
      const MarketUpdateInfoLevelStruct& this_bid_level_ = sec_market_view_.GetBidLevelAtIndex(index_);
      size_ = this_bid_level_.limit_size_;
    } else {
      const MarketUpdateInfoLevelStruct& this_ask_level_ = sec_market_view_.GetAskLevelAtIndex(index_);
      size_ = this_ask_level_.limit_size_;
    }
  }
  indicator_value_ = size_;
  NotifyIndicatorListeners(indicator_value_);
}
}

// market_interrupt_listener interface
