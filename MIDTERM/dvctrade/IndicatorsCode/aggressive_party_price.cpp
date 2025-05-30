/**
    \file IndicatorsCode/aggressive_party_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/aggressive_party_price.hpp"

namespace HFSAT {

void AggressivePartyPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                             std::vector<std::string>& _ors_source_needed_vec_,
                                             const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

AggressivePartyPrice* AggressivePartyPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const std::vector<const char*>& r_tokens_,
                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _trade_seconds_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), _basepx_pxtype_);
}

AggressivePartyPrice* AggressivePartyPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              SecurityMarketView& _indep_market_view_,
                                                              double _trade_seconds_, PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _trade_seconds_ << " "
              << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, AggressivePartyPrice*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new AggressivePartyPrice(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _trade_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

AggressivePartyPrice::AggressivePartyPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                           const std::string& concise_indicator_description_,
                                           SecurityMarketView& _indep_market_view_, double _trade_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      projected_mktpx_(0),
      projected_bidsz_(0),
      projected_asksz_(0),
      projected_bid_int_price_(0),
      projected_ask_int_price_(0),
      projected_bid_price_(0),
      projected_ask_price_(0),
      projected_bidsz_depletion_(0),
      projected_asksz_depletion_(0),
      last_decayed_msecs_(0),
      trade_history_halflife_msecs_(
          std::max(WATCH_DEFAULT_PAGE_WIDTH_MSECS,
                   (int)(std::max(0.0, _trade_seconds_ * 1000)))),  // DON"T BE LAZY. Allow less then 0.1 seconds !!!
      prev_best_bid_size_(0),
      prev_best_ask_size_(0) {
  _indep_market_view_.subscribe_tradeprints(this);

  if (!indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
    PriceType_t t_error_price_type_ = kPriceTypeMktSizeWPrice;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  watch_.subscribe_TimePeriod(this);  // every WATCH_DEFAULT_PAGE_WIDTH_MSECS
}

void AggressivePartyPrice::ReduceTradeEffect() {
  if (projected_bidsz_depletion_ > 0) {
    int projected_bidsz_increment_ = (projected_bidsz_depletion_ + 1) / 2;    /// a way of doing ceil
    if (projected_bid_int_price_ < indep_market_view_.bestbid_int_price()) {  // first level removed
      int second_level_depletion_ = std::max(0, indep_market_view_.bid_size(1) - projected_bidsz_);
      if (second_level_depletion_ >= projected_bidsz_increment_) {  // no change in projected_bid_price_
        projected_bidsz_ += projected_bidsz_increment_;
        projected_bidsz_depletion_ = projected_bidsz_depletion_ - projected_bidsz_increment_;
      } else {
        projected_bidsz_ =
            std::min(indep_market_view_.bestbid_size(), (projected_bidsz_increment_ - second_level_depletion_));
        projected_bid_int_price_ = indep_market_view_.bestbid_int_price();
        projected_bid_price_ = indep_market_view_.bestbid_price();
        projected_bidsz_depletion_ = indep_market_view_.bestbid_size() - projected_bidsz_;
      }
    } else {
      projected_bidsz_ = std::min(indep_market_view_.bestbid_size(), (projected_bidsz_ + projected_bidsz_increment_));
      projected_bidsz_depletion_ = indep_market_view_.bestbid_size() - projected_bidsz_;
    }
  } else {
    if (projected_bid_int_price_ < indep_market_view_.bestbid_int_price()) {
      projected_bidsz_ = indep_market_view_.bestbid_size();
      projected_bid_int_price_ = indep_market_view_.bestbid_int_price();
      projected_bid_price_ = indep_market_view_.bestbid_price();
      projected_bidsz_depletion_ = 0;
    }
  }

  if (projected_asksz_depletion_ > 0) {
    int projected_asksz_increment_ = (projected_asksz_depletion_ + 1) / 2;    /// a way of doing ceil
    if (projected_ask_int_price_ > indep_market_view_.bestask_int_price()) {  // first level removed
      int second_level_depletion_ = std::max(0, indep_market_view_.ask_size(1) - projected_asksz_);
      if (second_level_depletion_ >= projected_asksz_increment_) {  // no change in projected_ask_price_
        projected_asksz_ += projected_asksz_increment_;
        projected_asksz_depletion_ = projected_asksz_depletion_ - projected_asksz_increment_;
      } else {
        projected_asksz_ =
            std::min(indep_market_view_.bestask_size(), (projected_asksz_increment_ - second_level_depletion_));
        projected_ask_int_price_ = indep_market_view_.bestask_int_price();
        projected_ask_price_ = indep_market_view_.bestask_price();
        projected_asksz_depletion_ = indep_market_view_.bestask_size() - projected_asksz_;
      }
    } else {
      projected_asksz_ = std::min(indep_market_view_.bestask_size(), (projected_asksz_ + projected_asksz_increment_));
      projected_asksz_depletion_ = indep_market_view_.bestask_size() - projected_asksz_;
    }
  } else {
    if (projected_ask_int_price_ > indep_market_view_.bestask_int_price()) {
      projected_asksz_ = indep_market_view_.bestask_size();
      projected_ask_int_price_ = indep_market_view_.bestask_int_price();
      projected_ask_price_ = indep_market_view_.bestask_price();
      projected_asksz_depletion_ = 0;
    }
  }

  projected_mktpx_ = ((projected_bidsz_ * projected_ask_price_) + (projected_asksz_ * projected_bid_price_)) /
                     (projected_asksz_ + projected_bidsz_);

  if (std::isnan(projected_mktpx_) || projected_bidsz_ < 1 || projected_asksz_ < 1) {
    DBGLOG_TIME_CLASS_FUNC << "iv=nan : " << concise_indicator_description() << DBGLOG_ENDL_FLUSH;
    projected_mktpx_ = indep_market_view_.mkt_size_weighted_price();
  }

  indicator_value_ = projected_mktpx_ - indep_market_view_.mkt_size_weighted_price();
  NotifyIndicatorListeners(indicator_value_);
}

void AggressivePartyPrice::TestReduceTradeEffect() {
  if (watch_.msecs_from_midnight() - last_decayed_msecs_ > trade_history_halflife_msecs_) {
    if (watch_.msecs_from_midnight() - last_decayed_msecs_ >
        2 * trade_history_halflife_msecs_) {  // if a long time has elapsed
      projected_bidsz_ = indep_market_view_.bestbid_size();
      projected_asksz_ = indep_market_view_.bestask_size();
      projected_bid_int_price_ = indep_market_view_.bestbid_int_price();
      projected_ask_int_price_ = indep_market_view_.bestask_int_price();
      projected_bid_price_ = indep_market_view_.bestbid_price();
      projected_ask_price_ = indep_market_view_.bestask_price();
      projected_bidsz_depletion_ = 0;
      projected_asksz_depletion_ = 0;
      projected_mktpx_ = indep_market_view_.mkt_size_weighted_price();
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    } else {
      ReduceTradeEffect();
    }

    last_decayed_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % trade_history_halflife_msecs_;
  }
}

void AggressivePartyPrice::OnTimePeriodUpdate(const int num_pages_to_add_) {
  if (is_ready_) {
    TestReduceTradeEffect();
  }
}

void AggressivePartyPrice::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& cr_trade_print_info_,
                                        const MarketUpdateInfo& cr_market_update_info_) {
  if (is_ready_) {
    // TestReduceTradeEffect ( ); // not doing this just for speed

    if (projected_bid_int_price_ > cr_market_update_info_.bestbid_int_price_) {
      projected_bidsz_ = indep_market_view_.bestbid_size();
      projected_bid_int_price_ = indep_market_view_.bestbid_int_price();
      projected_bid_price_ = indep_market_view_.bestbid_price();
      projected_bidsz_depletion_ = 0;
    }

    if (projected_ask_int_price_ < cr_market_update_info_.bestask_int_price_) {
      projected_asksz_ = indep_market_view_.bestask_size();
      projected_ask_int_price_ = indep_market_view_.bestask_int_price();
      projected_ask_price_ = indep_market_view_.bestask_price();
      projected_asksz_depletion_ = 0;
    }

    if ((cr_trade_print_info_.buysell_ == kTradeTypeSell) &&  // this means the agg party was selling
        (projected_bid_int_price_ >=
         cr_trade_print_info_
             .int_trade_price_))  // this makes sure that we are masking at a price that woud be affected by the trade
    {
      int projected_bidsz_decrement_ = cr_trade_print_info_.size_traded_;
      if (projected_bid_int_price_ < cr_market_update_info_.bestbid_int_price_)  // need not do this all the time
      {                                                                          // first level removed

        projected_bidsz_ = std::max(1, projected_bidsz_ - projected_bidsz_decrement_);
        projected_bidsz_depletion_ =
            indep_market_view_.bestbid_size() + indep_market_view_.bid_size(1) - projected_bidsz_;
      } else {                                                // still at first level
        if (projected_bidsz_ > projected_bidsz_decrement_) {  // no levels cleared this time
          projected_bidsz_ = projected_bidsz_ - projected_bidsz_decrement_;
          projected_bidsz_depletion_ = indep_market_view_.bestbid_size() - projected_bidsz_;
        } else {
          if (indep_market_view_.bid_int_price(1) >
              0)  // second level exists // TODO make this better .. the SMV will know this easily
          {
            projected_bidsz_decrement_ -= projected_bidsz_;  // since this was eaten away in clearing first level

            projected_bid_int_price_ = indep_market_view_.bid_int_price(1);  // hence int_price is set to second level
            projected_bid_price_ = indep_market_view_.bid_price(1);          // hence price is set to second level

            if (cr_trade_print_info_.int_trade_price_ >
                projected_bid_int_price_) {    // means that the that the trade was at a better price than current price
              projected_bidsz_decrement_ = 0;  // so that there is no more depletion at lower levels
            }
            projected_bidsz_ =
                std::max(1, (indep_market_view_.bid_size(1) -
                             projected_bidsz_decrement_));  // remaining part of the trade eats into second level

            projected_bidsz_depletion_ =
                indep_market_view_.bestbid_size() + indep_market_view_.bid_size(1) - projected_bidsz_;
          } else {
            projected_bidsz_ = 1;
            projected_bidsz_depletion_ = indep_market_view_.bestbid_size() - projected_bidsz_;
          }
        }
      }
    }

    if ((cr_trade_print_info_.buysell_ == kTradeTypeBuy) &&
        (cr_trade_print_info_.int_trade_price_ >= projected_ask_int_price_)) {
      int projected_asksz_decrement_ = cr_trade_print_info_.size_traded_;
      if (projected_ask_int_price_ > cr_market_update_info_.bestask_int_price_) {  // first level removed
        projected_asksz_ = std::max(1, projected_asksz_ - projected_asksz_decrement_);
        projected_asksz_depletion_ =
            indep_market_view_.bestask_size() + indep_market_view_.ask_size(1) - projected_asksz_;
      } else {                                                // still at first level
        if (projected_asksz_ > projected_asksz_decrement_) {  // no levels cleared this time
          projected_asksz_ = projected_asksz_ - projected_asksz_decrement_;
          projected_asksz_depletion_ = indep_market_view_.bestask_size() - projected_asksz_;
        } else {
          if (indep_market_view_.ask_int_price(1) > 0) {
            projected_asksz_decrement_ -= projected_asksz_;  // since this was easten away in clearing first level

            projected_ask_int_price_ = indep_market_view_.ask_int_price(1);  // hence int_price is set to second level
            projected_ask_price_ = indep_market_view_.ask_price(1);          // hence price is set to second level

            if (cr_trade_print_info_.int_trade_price_ < projected_ask_int_price_) {
              projected_asksz_decrement_ = 0;  // so that there is no more depletion at lower levels
            }
            projected_asksz_ =
                std::max(1, (indep_market_view_.ask_size(1) -
                             projected_asksz_decrement_));  // remaining part of he trade eats into second level

            projected_asksz_depletion_ =
                indep_market_view_.bestask_size() + indep_market_view_.ask_size(1) - projected_asksz_;
          } else {
            projected_asksz_ = 1;
            projected_asksz_depletion_ = indep_market_view_.bestask_size() - projected_asksz_;
          }
        }
      }
    }

    projected_mktpx_ = ((projected_bidsz_ * projected_ask_price_) + (projected_asksz_ * projected_bid_price_)) /
                       (projected_asksz_ + projected_bidsz_);
    if (std::isnan(projected_mktpx_) || projected_bidsz_ < 1 || projected_asksz_ < 1) {
      DBGLOG_TIME_CLASS_FUNC << "iv=nan : " << concise_indicator_description() << DBGLOG_ENDL_FLUSH;
      projected_mktpx_ = indep_market_view_.mkt_size_weighted_price();
    }

    indicator_value_ = projected_mktpx_ - indep_market_view_.mkt_size_weighted_price();
    NotifyIndicatorListeners(indicator_value_);
  }
}

void AggressivePartyPrice::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void AggressivePartyPrice::OnMarketUpdate(const unsigned int _security_id_,
                                          const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
#if EQUITY_INDICATORS_ALWAYS_READY
    if (indep_market_view_.is_ready_complex(2) || IndicatorUtil::IsEquityShortcode(indep_market_view_.shortcode()))
#else
    if (indep_market_view_.is_ready_complex(2))
#endif
    {

      projected_bidsz_ = indep_market_view_.bestbid_size();
      projected_asksz_ = indep_market_view_.bestask_size();
      projected_bid_int_price_ = indep_market_view_.bestbid_int_price();
      projected_ask_int_price_ = indep_market_view_.bestask_int_price();
      projected_bid_price_ = indep_market_view_.bestbid_price();
      projected_ask_price_ = indep_market_view_.bestask_price();
      projected_bidsz_depletion_ = 0;
      projected_asksz_depletion_ = 0;
      projected_mktpx_ = indep_market_view_.mkt_size_weighted_price();
      prev_best_bid_size_ =
          indep_market_view_
              .bestbid_size();  // can't think of a better/cleaner way to get incremental updates in bid/ask sizes
      prev_best_ask_size_ = indep_market_view_.bestask_size();
      indicator_value_ = 0;

      is_ready_ = true;

      NotifyIndicatorListeners(indicator_value_);
    }
  } else if (!data_interrupted_) {
    // n2c removed because indicator_value is to be updated on every update
    // bool n2c = false;

    if ((projected_bidsz_depletion_ > 0) && (projected_bid_int_price_ < indep_market_view_.bestbid_int_price())) {
      projected_bidsz_ = indep_market_view_.bestbid_size();
      projected_bid_int_price_ = indep_market_view_.bestbid_int_price();
      projected_bid_price_ = indep_market_view_.bestbid_price();
      projected_bidsz_depletion_ = 0;

      // n2c = true;
    } else if (prev_best_bid_size_ !=
               indep_market_view_
                   .bestbid_size()) {  // if the level is not changed then we update the top projected top level
      projected_bidsz_ = std::max(
          1, projected_bidsz_ + indep_market_view_.bestbid_size() - prev_best_bid_size_);  // not very sure about this
      projected_bidsz_depletion_ = indep_market_view_.bestbid_size() - projected_bidsz_;
    }

    if ((projected_asksz_depletion_ > 0) && (projected_ask_int_price_ > indep_market_view_.bestask_int_price())) {
      projected_asksz_ = indep_market_view_.bestask_size();
      projected_ask_int_price_ = indep_market_view_.bestask_int_price();
      projected_ask_price_ = indep_market_view_.bestask_price();
      projected_asksz_depletion_ = 0;

      // n2c = true;
    } else if (prev_best_ask_size_ !=
               indep_market_view_
                   .bestask_size()) {  // if the level is not changed then we update the top projected top level
      projected_asksz_ = std::max(
          1, projected_asksz_ + indep_market_view_.bestask_size() - prev_best_ask_size_);  // not very sure about this
      projected_asksz_depletion_ = indep_market_view_.bestask_size() - projected_asksz_;
    }

    // if ( n2c )
    {
      projected_mktpx_ = ((projected_bidsz_ * projected_ask_price_) + (projected_asksz_ * projected_bid_price_)) /
                         (projected_asksz_ + projected_bidsz_);
      if (std::isnan(projected_mktpx_) || projected_bidsz_ < 1 || projected_asksz_ < 1) {
        DBGLOG_TIME_CLASS_FUNC << "iv=na/inf : " << concise_indicator_description() << DBGLOG_ENDL_FLUSH;
        projected_mktpx_ = indep_market_view_.mkt_size_weighted_price();
      }

      indicator_value_ = projected_mktpx_ - indep_market_view_.mkt_size_weighted_price();
      NotifyIndicatorListeners(indicator_value_);
    }
  }
  prev_best_bid_size_ = indep_market_view_.bestbid_size();
  prev_best_ask_size_ = indep_market_view_.bestask_size();
}

void AggressivePartyPrice::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                   const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void AggressivePartyPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}
}
