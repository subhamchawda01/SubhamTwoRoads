/**
    \file IndicatorsCode/base_implied_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/base_implied_price.hpp"

namespace HFSAT {

void BaseImpliedPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

BaseImpliedPrice* BaseImpliedPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _price_type_
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])), StringToPriceType_t(r_tokens_[6]));
}

BaseImpliedPrice* BaseImpliedPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      SecurityMarketView& _dep_market_view_,
                                                      SecurityMarketView& _indep_market_view_,
                                                      PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, BaseImpliedPrice*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new BaseImpliedPrice(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _indep_market_view_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

BaseImpliedPrice::BaseImpliedPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   SecurityMarketView& _dep_market_view_, SecurityMarketView& _indep_market_view_,
                                   PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      dep_interrupted_(false),
      indep_interrupted_(false),
      base_implied_price_(0.0),
      compute_mkt_px_(false),
      compute_owp_px_(false),
      compute_mid_px_(false),
      compute_sin_px_(false),
      compute_omix_px_(false),
      mkt_px_(0),
      owp_px_(0),
      mid_px_(0),
      sin_px_(0),
      omix_px_(0) {
  if (dep_market_view_.security_id() == indep_market_view_.security_id()) {  // added this since for convenience one
                                                                             // could add a combo or portfolio as source
                                                                             // with a security
    // that is also the dependent
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
    SubscribePrice(price_type_);
  }
}

void BaseImpliedPrice::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex(2) = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void BaseImpliedPrice::SubscribePrice(PriceType_t t_price_type_) {
  if (!dep_market_view_.subscribe_price_type(this, t_price_type_)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_price_type_ << " to DEP " << std::endl;
  }

  if (!indep_market_view_.subscribe_price_type(this, t_price_type_)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_price_type_ << std::endl;
  }

  if (t_price_type_ == kPriceTypeMktSizeWPrice) {
    compute_mkt_px_ = true;
  }

  if (t_price_type_ == kPriceTypeMidprice) {
    compute_mid_px_ = true;
  }

  if (t_price_type_ == kPriceTypeOrderWPrice) {
    compute_owp_px_ = true;
  }

  if (t_price_type_ == kPriceTypeMktSinusoidal) {
    compute_sin_px_ = true;
    compute_mkt_px_ = true;
  }

  if (t_price_type_ == kPriceTypeOfflineMixMMS) {
    compute_mkt_px_ = true;
    compute_mid_px_ = true;
    compute_owp_px_ = true;
    compute_sin_px_ = true;
    compute_omix_px_ = true;
  }
}

void BaseImpliedPrice::OnMarketUpdate(const unsigned int _security_id_,
                                      const MarketUpdateInfo& cr_market_update_info_) {
  if (!dep_market_view_.is_ready() || !indep_market_view_.is_ready()) {
    is_ready_ = false;
  }

  // TODO observe and remove
  // the fllowing has been added since above we could be potentially be accessing the price when the
  // SMV is not ready
  if (dep_market_view_.IsBidBookEmpty() || dep_market_view_.IsAskBookEmpty() || indep_market_view_.IsBidBookEmpty() ||
      indep_market_view_.IsAskBookEmpty()) {
    return;
  }

  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (compute_mkt_px_) {
      if (dep_market_view_.spread_increments() > 1) {
        mkt_px_ = dep_market_view_.price_from_type(kPriceTypeMktSizeWPrice);
      } else {
        double effective_bid_price_ = dep_market_view_.bestbid_price();
        double effective_bid_size_ = dep_market_view_.bestbid_size();
        double effective_ask_price_ = dep_market_view_.bestask_price();
        double effective_ask_size_ = dep_market_view_.bestask_size();

        if (dep_market_view_.bestbid_price() == indep_market_view_.bestbid_price()) {
          effective_bid_price_ = dep_market_view_.bestbid_price();
          effective_bid_size_ = dep_market_view_.bestbid_size() * 10.0 + ((double)indep_market_view_.bestbid_size());
        } else {
          if (indep_market_view_.bestbid_price() > dep_market_view_.bestbid_price()) {
            double size_px_ = dep_market_view_.bestbid_price() * dep_market_view_.bestbid_size() * 10.0 +
                              indep_market_view_.bestbid_price() * indep_market_view_.bestbid_size() +
                              indep_market_view_.bid_price(1) * indep_market_view_.bid_size(1);
            effective_bid_size_ = dep_market_view_.bestbid_size() * 10.0 + ((double)indep_market_view_.bestbid_size()) +
                                  ((double)indep_market_view_.bid_size(1));
            effective_bid_price_ = size_px_ / effective_bid_size_;
          } else {
            // Error?
            // Do log this case
            effective_bid_size_ = dep_market_view_.bestbid_size() * 10;
            effective_bid_price_ = dep_market_view_.bestbid_price();
          }
        }

        if (dep_market_view_.bestask_price() == indep_market_view_.bestask_price()) {
          effective_ask_price_ = dep_market_view_.bestask_price();
          effective_ask_size_ = dep_market_view_.bestask_size() * 10.0 + ((double)indep_market_view_.bestask_size());
        } else {
          if (indep_market_view_.bestask_price() < dep_market_view_.bestask_price()) {
            double size_px_ = dep_market_view_.bestask_price() * dep_market_view_.bestask_size() * 10.0 +
                              indep_market_view_.bestask_price() * indep_market_view_.bestask_size() +
                              indep_market_view_.ask_price(1) * indep_market_view_.ask_size(1);
            effective_ask_size_ = dep_market_view_.bestask_size() * 10.0 + ((double)indep_market_view_.bestask_size()) +
                                  ((double)indep_market_view_.ask_size(1));
            effective_ask_price_ = size_px_ / effective_ask_size_;
          } else {
            // Rare Possibility - best to send some sane price - ankit
            // Going with the assumption that NK leads and not looking at NKM in that case.
            // Error?
            // Do log this case
            effective_ask_size_ = dep_market_view_.bestask_size() * 10;
            effective_ask_price_ = dep_market_view_.bestask_price();
          }
        }

        mkt_px_ = (effective_bid_price_ * effective_ask_size_ + effective_ask_price_ * effective_bid_size_) /
                  (effective_ask_size_ + effective_bid_size_);

        //                DBGLOG_TIME_CLASS_FUNC << "new: " << mkt_px_ << " old: " << dep_market_view_.price_from_type (
        //                kPriceTypeMktSizeWPrice )
        //                    << " mkt: [ " << dep_market_view_.bestbid_size ( ) << " " <<
        //                    dep_market_view_.bestbid_price ( ) << " * " << dep_market_view_.bestask_price ( )
        //                    << " " << dep_market_view_.bestask_size ( ) << " ] [ " << indep_market_view_.bestbid_size
        //                    ( ) << " " << indep_market_view_.bestbid_price ( ) << " * " <<
        //                    indep_market_view_.bestask_price ( ) << " " << indep_market_view_.bestask_size ( ) << " ]
        //                    [ "
        //                    << effective_bid_size_ << " " << effective_bid_price_ << " * " << effective_ask_price_ <<
        //                    " " << effective_ask_size_ << " ]"
        //                    << DBGLOG_ENDL_FLUSH;
      }
    }

    if (compute_mid_px_) {
      mid_px_ = (dep_market_view_.bestbid_price() + dep_market_view_.bestask_price() +
                 indep_market_view_.bestbid_price() + indep_market_view_.bestask_price()) /
                4;
    }

    if (compute_owp_px_) {
      if (dep_market_view_.spread_increments() > 1) {
        owp_px_ = dep_market_view_.price_from_type(kPriceTypeOrderWPrice);
      } else {
        double effective_bid_price_ = dep_market_view_.bestbid_price();
        double effective_bid_order_ = dep_market_view_.bestbid_ordercount();
        double effective_ask_price_ = dep_market_view_.bestask_price();
        double effective_ask_order_ = dep_market_view_.bestask_ordercount();

        if (dep_market_view_.bestbid_price() == indep_market_view_.bestbid_price()) {
          effective_bid_price_ = dep_market_view_.bestbid_price();
          effective_bid_order_ =
              dep_market_view_.bestbid_ordercount() * 10.0 + ((double)indep_market_view_.bestbid_ordercount());
        } else {
          if (indep_market_view_.bestbid_price() > dep_market_view_.bestbid_price()) {
            double size_px_ = dep_market_view_.bestbid_price() * dep_market_view_.bestbid_ordercount() * 10.0 +
                              indep_market_view_.bestbid_price() * indep_market_view_.bestbid_ordercount() +
                              indep_market_view_.bid_price(1) * indep_market_view_.bid_order(1);
            effective_bid_order_ = dep_market_view_.bestbid_ordercount() * 10.0 +
                                   ((double)indep_market_view_.bestbid_ordercount()) +
                                   ((double)indep_market_view_.bid_order(1));
            effective_bid_price_ = size_px_ / effective_bid_order_;
          } else {
            // Error?
            // Do log this case
            effective_bid_order_ = dep_market_view_.bestbid_ordercount() * 10;
            effective_bid_price_ = dep_market_view_.bestbid_price();
          }
        }

        if (dep_market_view_.bestask_price() == indep_market_view_.bestask_price()) {
          effective_ask_price_ = dep_market_view_.bestask_price();
          effective_ask_order_ =
              dep_market_view_.bestask_ordercount() + ((double)indep_market_view_.bestask_ordercount()) / 10;
        } else {
          if (indep_market_view_.bestask_price() < dep_market_view_.bestask_price()) {
            double size_px_ = dep_market_view_.bestask_price() * dep_market_view_.bestask_ordercount() +
                              indep_market_view_.bestask_price() * indep_market_view_.bestask_ordercount() / 10 +
                              indep_market_view_.ask_price(1) * indep_market_view_.ask_order(1) / 10;
            effective_ask_order_ = dep_market_view_.bestask_ordercount() +
                                   ((double)indep_market_view_.bestask_ordercount()) / 10 +
                                   ((double)indep_market_view_.ask_order(1)) / 10;
            effective_ask_price_ = size_px_ / effective_ask_order_;
          } else {
            // Error?
            // Do log this case
            effective_ask_order_ = dep_market_view_.bestask_ordercount() * 10;
            effective_ask_price_ = dep_market_view_.bestask_price();
          }
        }

        owp_px_ = (effective_bid_price_ * effective_ask_order_ + effective_ask_price_ * effective_bid_order_) /
                  (effective_ask_order_ + effective_bid_order_);

        //                DBGLOG_TIME_CLASS_FUNC << "new: " << mkt_px_ << " old: " << dep_market_view_.price_from_type (
        //                kPriceTypeMktSizeWPrice )
        //                    << " mkt: [ " << dep_market_view_.bestbid_size ( ) << " " <<
        //                    dep_market_view_.bestbid_price ( ) << " * " << dep_market_view_.bestask_price ( )
        //                    << " " << dep_market_view_.bestask_size ( ) << " ] [ " << indep_market_view_.bestbid_size
        //                    ( ) << " " << indep_market_view_.bestbid_price ( ) << " * " <<
        //                    indep_market_view_.bestask_price ( ) << " " << indep_market_view_.bestask_size ( ) << " ]
        //                    [ "
        //                    << effective_bid_size_ << " " << effective_bid_price_ << " * " << effective_ask_price_ <<
        //                    " " << effective_ask_size_ << " ]"
        //                    << DBGLOG_ENDL_FLUSH;
      }
    }

    if (compute_sin_px_) {
      sin_px_ = mkt_px_;
    }

    if (compute_omix_px_) {
      omix_px_ = mkt_px_ * dep_market_view_.offline_mix_mms_price_weights_[kPriceTypeMktSizeWPrice] +
                 mid_px_ * dep_market_view_.offline_mix_mms_price_weights_[kPriceTypeMidprice] +
                 owp_px_ * dep_market_view_.offline_mix_mms_price_weights_[kPriceTypeOrderWPrice] +
                 sin_px_ * dep_market_view_.offline_mix_mms_price_weights_[kPriceTypeMktSinusoidal] +
                 owp_px_ * dep_market_view_.offline_mix_mms_price_weights_[kPriceTypeTradeWPrice];
    }

    switch (price_type_) {
      case kPriceTypeMktSizeWPrice: {
        base_implied_price_ = mkt_px_;
      } break;
      case kPriceTypeMidprice: {
        base_implied_price_ = mid_px_;
      } break;
      case kPriceTypeOrderWPrice: {
        base_implied_price_ = owp_px_;
      } break;
      case kPriceTypeMktSinusoidal: {
        base_implied_price_ = sin_px_;
      } break;
      case kPriceTypeOfflineMixMMS: {
        base_implied_price_ = omix_px_;
      } break;
      default: { base_implied_price_ = mkt_px_; } break;
    }

    indicator_value_ =
        base_implied_price_ - SecurityMarketView::GetPriceFromType(price_type_, dep_market_view_.market_update_info_);

    NotifyIndicatorListeners(indicator_value_);
  }
}

void BaseImpliedPrice::InitializeValues() { indicator_value_ = 0; }

double BaseImpliedPrice::GetBaseImpliedPrice() { return base_implied_price_; }

double BaseImpliedPrice::GetBaseImpliedPrice(PriceType_t t_price_type_) {
  double t_implied_price_ = base_implied_price_;

  switch (t_price_type_) {
    case kPriceTypeMktSizeWPrice: {
      t_implied_price_ = mkt_px_;
    } break;
    case kPriceTypeMidprice: {
      t_implied_price_ = mid_px_;
    } break;
    case kPriceTypeOrderWPrice: {
      t_implied_price_ = owp_px_;
    } break;
    case kPriceTypeMktSinusoidal: {
      t_implied_price_ = sin_px_;
    } break;
    case kPriceTypeOfflineMixMMS: {
      t_implied_price_ = omix_px_;
    } break;
    default:
      break;
  }

  return t_implied_price_;
}

// market_interrupt_listener interface
void BaseImpliedPrice::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    indep_interrupted_ = true;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = true;
  }
  if (indep_interrupted_ || dep_interrupted_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void BaseImpliedPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (indep_market_view_.security_id() == _security_id_) {
      indep_interrupted_ = false;
    } else if (dep_market_view_.security_id() == _security_id_) {
      dep_interrupted_ = false;
    }
    if (!(dep_interrupted_ || indep_interrupted_)) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
