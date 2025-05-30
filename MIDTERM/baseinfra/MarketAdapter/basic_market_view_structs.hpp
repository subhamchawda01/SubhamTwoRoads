/**
   \file MarketAdapter/basic_market_view_structs.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_MARKETADAPTER_BASIC_MARKET_VIEW_STRUCTS_H
#define BASE_MARKETADAPTER_BASIC_MARKET_VIEW_STRUCTS_H

#include <vector>
#include <string>
#include <sstream>

#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/defines.hpp"
// #include "dvccode/CommonDataStructures/fixed_length_circular_vector.hpp"
// #include "dvccode/CommonDataStructures/circular_buffer.hpp"
#include "baseinfra/MDSMessages/defines.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"

namespace HFSAT {

typedef enum {
  kNoTradeMask = 0,                // the best level is the saem as book
  kTradeImpliedL1SizeChanged = 1,  // based on trades we have changed the sizes but not prices
  kTradeImpliedL1PriceChanged = 2
} BookMaskStatus_t;

/// information of a pricelevel data based market book update
/// with some preprocessing
/// used primarily as a publicly available snapshot of the full book,
/// passed as a reference
struct MarketUpdateInfoLevelStruct {
  int limit_int_price_level_;  ///< int_price difference between top level price and this price. Only computed if
  /// computing_price_levels_ = true
  int limit_int_price_;  ///< integer value of the price, computed by dividing by min price increment. Only computed if
  /// computing_price_absolute_ = true
  double limit_price_;    ///< price at this level */
  int limit_size_;        ///< cumulative size at this level */
  int limit_ordercount_;  ///< cumulative count of orders at this level */
  ttime_t mod_time_;
  int priority_size_;
  int priority_size_filled_;
  double level_fraction_;
  int mask_time_msecs_;

  explicit MarketUpdateInfoLevelStruct()
      : limit_int_price_level_(DEF_MARKET_DEPTH),
        limit_int_price_(kInvalidIntPrice),
        limit_price_(kInvalidPrice),
        limit_size_(0),
        limit_ordercount_(0),
        mod_time_(0, 0),
        priority_size_(0),
        priority_size_filled_(0),
        level_fraction_(1),
        mask_time_msecs_(0) {}

  explicit MarketUpdateInfoLevelStruct(int _limit_int_price_level_, int _limit_int_price_, double _limit_price_,
                                       int _limit_size_, int _limit_ordercount_, ttime_t tv, int _priority_size_ = 0)
      : limit_int_price_level_(_limit_int_price_level_),
        limit_int_price_(_limit_int_price_),
        limit_price_(_limit_price_),
        limit_size_(_limit_size_),
        limit_ordercount_(_limit_ordercount_),
        mod_time_(tv),
        priority_size_(_priority_size_),
        priority_size_filled_(0),
        level_fraction_(1),
        mask_time_msecs_(0) {}

  explicit MarketUpdateInfoLevelStruct(int _limit_int_price_level_, int _limit_int_price_, double _limit_price_,
                                       int _limit_size_, int _limit_ordercount_, int _priority_size_)
      : limit_int_price_level_(_limit_int_price_level_),
        limit_int_price_(_limit_int_price_),
        limit_price_(_limit_price_),
        limit_size_(_limit_size_),
        limit_ordercount_(_limit_ordercount_),
        mod_time_(0, 0),
        priority_size_(_priority_size_),
        priority_size_filled_(0),
        level_fraction_(1),
        mask_time_msecs_(0) {}

  inline void InValidateLevel() {
    limit_int_price_level_ = MAX_LEVELS;
    limit_int_price_ = kInvalidIntPrice;
    limit_price_ = kInvalidPrice;
    limit_size_ = 0;
    limit_ordercount_ = -1;
    mod_time_ = ttime_t(0, 0);
    priority_size_ = 0;
    priority_size_filled_ = 0;
    level_fraction_ = 0;
    mask_time_msecs_ = 0;
  }

  std::string GetPriceAsStr() {
    char pp[10] = {0};
    sprintf(pp, "%.7f", limit_price_);
    std::ostringstream temp_ss;
    temp_ss << pp;
    return temp_ss.str();
  }
};

struct MarketOrder {  // the basic order struct to be used to store all orders
  // It is also used in smv to store order level book info

  int int_price_;
  int size_;
  int order_count_;
  int level_;
  ttime_t order_entry_time_;
  bool priority_;

  MarketOrder() {}

  explicit MarketOrder(int t_int_price_, int t_size_, int t_order_count_, int t_level_, ttime_t t_entry_time_,
                       bool t_priority_ = false)
      : int_price_(t_int_price_),
        size_(t_size_),
        order_count_(t_order_count_),
        level_(t_level_),
        order_entry_time_(t_entry_time_),
        priority_(t_priority_) {}

  void set(int t_int_price_, int t_size_, int t_order_count_, int t_level_, ttime_t t_entry_time_,
           bool t_priority_ = false) {
    int_price_ = t_int_price_;
    size_ = t_size_;
    order_count_ = t_order_count_;
    level_ = t_level_;
    order_entry_time_ = t_entry_time_;
    priority_ = t_priority_;
  }

  std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << int_price_;
    t_temp_oss_.width(10);
    t_temp_oss_.width(5);
    t_temp_oss_ << size_;
    t_temp_oss_.width(3);
    t_temp_oss_ << order_count_;
    t_temp_oss_.width(3);
    return t_temp_oss_.str();
  }
};

struct MarketUpdateInfo {
  const char* secname_;       ///< full symbol set by exchange / data-server for incoming data */
  std::string shortcode_;     ///< symbol used by us in modeling */
  unsigned int security_id_;  ///< the numeric identifier for this symbol, meant to speed up computation. */
  ExchSource_t exch_source_;  ///< the exch source ./.. used in processing perhaps

  // TradeType_t buysell_highest_level_changed_ ; ///< The direction bidside/askside that was changed in the last
  // change, kTradeTypeBuy / kTradeTypeSell / kTradeTypeNoInfo  . Only referenced by PriceLevelSimMarketMaker to remove
  // masks if the price at which we should mask has changed ... should se if there is a better way to get this */
  // int highest_level_changed_ ; ///< The int-level-from-top that was changed in the last change, should always be a
  // meaningful change unless market did not change, which is never */
  // MDUType_t mdu_type_; ///< Type of update this is. Usually taken from market data. Except needs to be generated for
  // all quotes created by SMV, (i) TradeBeforeQuote markets (ii) ORSmessages based change of nonselfbestmarket */

  // following section is being written as a vector

  bool temporary_bool_checking_if_this_is_an_indexed_book_;
  bool compute_stable_levels_;
  std::vector<MarketUpdateInfoLevelStruct> bidlevels_;
  std::vector<MarketUpdateInfoLevelStruct> asklevels_;
  std::vector<MarketUpdateInfoLevelStruct> indexed_bidlevels_;
  std::vector<MarketUpdateInfoLevelStruct> indexed_asklevels_;
  //#ifdef ENABLE_USE_OF_ORDER_LEVEL_DEPTH_BOOK
  std::vector<std::vector<MarketOrder*> > bid_level_order_depth_book_;  // for approx level order book
  std::vector<std::vector<MarketOrder*> > ask_level_order_depth_book_;
  //#endif
  // keeping these as separate variables since the effects of PromOrderManager are accounted for in these.
  // most indicators and intermediate price computations ( mid_price_, etc ) and strategy and modelmath classes
  // are meant to use these variables, instead of the array variables
  // TODO make these MarketUpdateInfoLevelStruct instead of stand alone variables
  double bestbid_price_;
  int bestbid_size_;
  int bestbid_int_price_;
  int bestbid_ordercount_;

  double bestask_price_;
  int bestask_size_;
  int bestask_int_price_;
  int bestask_ordercount_;

  // computed variables START
  int spread_increments_;  ///< = ( ( bestask_int_price_ - bestbid_int_price_ )

  // The following are in the same order as their defines so
  // the following code could be used to get a faster implementation of GetPriceFromType
  //   union {
  //     double prices_ [ kPriceTypeMax ];
  //     struct {
  double mid_price_;
  double mkt_size_weighted_price_;
  double mkt_sinusoidal_price_;
  double order_weighted_price_;
  double trade_weighted_price_;
  double offline_mix_mms_price_;
  double valid_level_mid_price_;
  double trade_base_px_;
  double trade_mktsz_px_;
  double trade_mktsin_px_;
  double trade_orderw_px_;
  double trade_tradew_px_;
  double trade_omix_px_;
  double order_size_weighted_price_;
  double online_mix_price_;
  double decayed_trade_px_;
  double stable_bid_price_;
  double stable_ask_price_;
  double implied_vol_;
  double band_mkt_price_;
  double general_price_;
  double pro_rata_mkt_size_weighted_price_;
  double alpha_;
  //     };
  //   };
  // computed variables END

  // pretrade variables .. to capture state before the trade
  // In case the exchange sends trade information first like CME,
  // pretrade information is only needed in indicators to compare against current values.
  //
  // for variables like tradepx_mktpx_diff_ computed in SMV, no comparison is needed against pretrade
  // values in a listener to SMV, since the computation can be done first in SMV.
  // then market_update_info_ can be updated and then the callbacks can be called.
  bool storing_pretrade_state_;
  // START
  double pretrade_bestbid_price_;
  double pretrade_bestbid_int_price_;
  int pretrade_bestbid_size_;
  double pretrade_bestask_price_;
  double pretrade_bestask_int_price_;
  int pretrade_bestask_size_;

  double pretrade_mid_price_;
  // END

  double prebook_bestbid_price_;
  double prebook_bestbid_int_price_;
  int prebook_bestbid_size_;
  double prebook_bestask_price_;
  double prebook_bestask_int_price_;
  int prebook_bestask_size_;
  TradeType_t tradetype;

  double prequote_mkt_size_weighted_price_;

  /// It is needed in computation of last_book_tdiff_ in SecurityMarketView::OnTrade .
  /// the difference between last_book_mkt_size_weighted_price_ and mkt_size_weighted_price_ is that
  /// in case of CME ( and other trade_before_quote exchanges ) where we might have a number of
  /// back to back trade messages, the last_book_mkt_size_weighted_price_ is the snapshot of mkt_size_weighted_price_
  /// the last time the update was a book message
  double last_book_mkt_size_weighted_price_;

  /// Needed for is_ready_complex and
  /// IndicatorLogger::PrintVals
  /// TODO ... make int if even for ES this does not come close to becoming 2^32
  unsigned long long l1events_;
  bool trade_update_implied_quote_;
  std::vector<unsigned long long> trades_count_;  // Keep count for different types of trades

  MarketUpdateInfo() {}

  MarketUpdateInfo(const std::string& _shortcode_, const char* _exchange_symbol_, const unsigned int _security_id_,
                   const ExchSource_t _exch_source_)
      : secname_(_exchange_symbol_),
        shortcode_(_shortcode_),
        security_id_(_security_id_),
        exch_source_(_exch_source_),
        temporary_bool_checking_if_this_is_an_indexed_book_(false),
        compute_stable_levels_(false),
        // buysell_highest_level_changed_ ( kTradeTypeNoInfo ), highest_level_changed_ ( DEF_MARKET_DEPTH ),
        // mdu_type_ ( kMDUType_PLNone ),
        bestbid_price_(kInvalidPrice),
        bestbid_size_(0),
        bestbid_int_price_(kInvalidIntPrice),
        bestbid_ordercount_(0),
        bestask_price_(kInvalidPrice),
        bestask_size_(0),
        bestask_int_price_(kInvalidIntPrice),
        bestask_ordercount_(0),
        spread_increments_(1),
        mid_price_(kInvalidPrice),
        mkt_size_weighted_price_(kInvalidPrice),
        mkt_sinusoidal_price_(kInvalidPrice),
        order_weighted_price_(kInvalidPrice),
        trade_weighted_price_(kInvalidPrice),
        offline_mix_mms_price_(kInvalidPrice),
        valid_level_mid_price_(kInvalidPrice),
        trade_base_px_(kInvalidPrice),
        trade_mktsz_px_(kInvalidPrice),
        trade_mktsin_px_(kInvalidPrice),
        trade_orderw_px_(kInvalidPrice),
        trade_tradew_px_(kInvalidPrice),
        trade_omix_px_(kInvalidPrice),
        order_size_weighted_price_(kInvalidPrice),
        online_mix_price_(kInvalidPrice),
        decayed_trade_px_(kInvalidPrice),
        stable_bid_price_(kInvalidPrice),
        stable_ask_price_(kInvalidPrice),
        implied_vol_(kInvalidPrice),
        band_mkt_price_(kInvalidPrice),
        general_price_(kInvalidPrice),
        pro_rata_mkt_size_weighted_price_(kInvalidPrice),
        alpha_(0),
        storing_pretrade_state_(false),
        pretrade_bestbid_price_(kInvalidPrice),
        pretrade_bestbid_int_price_(kInvalidIntPrice),
        pretrade_bestbid_size_(0),
        pretrade_bestask_price_(kInvalidPrice),
        pretrade_bestask_int_price_(kInvalidIntPrice),
        pretrade_bestask_size_(0),
        pretrade_mid_price_(kInvalidPrice),
        prebook_bestbid_price_(0),
        prebook_bestbid_int_price_(0),
        prebook_bestbid_size_(0),
        prebook_bestask_price_(0),
        prebook_bestask_int_price_(0),
        prebook_bestask_size_(0),
        tradetype(kTradeTypeNoInfo),
        prequote_mkt_size_weighted_price_(kInvalidPrice),
        last_book_mkt_size_weighted_price_(kInvalidPrice),
        l1events_(0u),
        trade_update_implied_quote_(false),
        trades_count_(std::vector<unsigned long long>(3, 0)) {
    // unlike indexed_bidlevels_ and indexed_asklevels_ these will be cleared
    bidlevels_.clear();
    asklevels_.clear();

    //#ifdef ENABLE_USE_OF_ORDER_LEVEL_DEPTH_BOOK
    bid_level_order_depth_book_.clear();
    ask_level_order_depth_book_.clear();
    //#endif
  }

  std::string ToString() const {
    std::ostringstream t_oss_;

    t_oss_ << "[ " << shortcode_ << " " << ((bidlevels_.size() >= 1) ? bidlevels_[0].limit_ordercount_ : 0) << " "
           << ((bidlevels_.size() >= 1) ? bidlevels_[0].limit_size_ : 0) << " "
           << ((bidlevels_.size() >= 1) ? bidlevels_[0].limit_price_ : 0) << " "
           << ((asklevels_.size() >= 1) ? asklevels_[0].limit_price_ : 0) << " "
           << ((asklevels_.size() >= 1) ? asklevels_[0].limit_size_ : 0) << " "
           << ((asklevels_.size() >= 1) ? asklevels_[0].limit_ordercount_ : 0) << " ] "
           << " [ "
           << " " << bestbid_ordercount_ << " " << bestbid_size_ << " " << bestbid_price_ << " " << bestask_price_
           << " " << bestask_size_ << " " << bestask_ordercount_ << " ]";

    return t_oss_.str();
  }
};

/// Summary of the information in the trade
/// And some preprocessed math
/// In future we can check if any variable is only needed in one place then we can
/// try pushing the math to where it is needed
struct TradePrintInfo {
  TradeType_t buysell_;  ///< TradeType_t of the aggressor party */
  double trade_price_;   ///< trade price */
  int size_traded_;      ///< size of this execution update */
  int int_trade_price_;  ///< = GetIntPx ( trade_price_ ) ... always computed ... needed in many cases ... only for
  /// markets which are not TradeBeforeQuote perhaps might not be needed */

  bool computing_trade_impact_;
  double trade_impact_;  ///< ratio of trade size to the amount previously there at that level .

  bool computing_int_trade_type_;
  int int_trade_type_;  ///< Set this to sign of position change

  bool computing_sqrt_size_traded_;
  double sqrt_size_traded_;

  bool computing_sqrt_trade_impact_;
  double sqrt_trade_impact_;

  bool computing_tradepx_mktpx_diff_;
  double tradepx_mktpx_diff_;  ///< Diff of current trade price with the mkt_size_weighted_price_ computed after the
  /// most recent UpdateL1Prices ( bookupdate or interpolated after

  bool computing_last_book_tdiff_;
  double last_book_tdiff_;  ///< Diff of current trade price with the mkt_size_weighted_price_ in the last bookupdate

  /// Number of trades seen since the start of data
  unsigned int num_trades_;
  unsigned int num_levels_cleared_;

  // OTC TRADES INFO -> should we instead have vector collecting all otc_trades/sizes or leave it to listener to
  // collect..?
  double otc_trade_price_;
  int otc_trade_size_;

  bool is_intermediate_;

  TradePrintInfo()
      : buysell_(kTradeTypeNoInfo),
        trade_price_(kInvalidPrice),
        size_traded_(0),
        int_trade_price_(kInvalidIntPrice),
        computing_trade_impact_(false),
        trade_impact_(0),
        computing_int_trade_type_(false),
        int_trade_type_(0),
        computing_sqrt_size_traded_(false),
        sqrt_size_traded_(0),
        computing_sqrt_trade_impact_(false),
        sqrt_trade_impact_(0),
        computing_tradepx_mktpx_diff_(false),
        tradepx_mktpx_diff_(0),
        computing_last_book_tdiff_(false),
        last_book_tdiff_(0),
        num_trades_(0u),
        num_levels_cleared_(0),
        otc_trade_price_(kInvalidIntPrice),
        otc_trade_size_(0u),
        is_intermediate_(false) {}

  std::string ToString() const {
    std::ostringstream t_oss_;
    t_oss_ << std::setprecision(10) << "[ " << GetTradeTypeChar(buysell_) << " " << size_traded_ << " @ "
           << trade_price_ << " ]";

    return t_oss_.str();
  }
};

struct BestBidAskInfo {
 public:
  BestBidAskInfo()
      : best_bid_int_price_(0),
        best_ask_int_price_(0),
        best_bid_size_(0),
        best_ask_size_(0),
        best_bid_orders_(0),
        best_ask_orders_(0),
        msecs_from_midnight_(0) {}

  BestBidAskInfo(const int t_best_bid_int_price_, const int t_best_ask_int_price_, const int t_best_bid_size_,
                 const int t_best_ask_size_, const int t_best_bid_orders_, const int t_best_ask_orders_,
                 const int t_msecs_from_midnight_ = 0)
      : best_bid_int_price_(t_best_bid_int_price_),
        best_ask_int_price_(t_best_ask_int_price_),
        best_bid_size_(t_best_bid_size_),
        best_ask_size_(t_best_ask_size_),
        best_bid_orders_(t_best_bid_orders_),
        best_ask_orders_(t_best_ask_orders_),
        msecs_from_midnight_(t_msecs_from_midnight_) {}

  // Note that this copies over int_price and not price
  BestBidAskInfo(const MarketUpdateInfo& t_market_update_info_)
      : best_bid_int_price_(
            t_market_update_info_.bidlevels_.size() >= 1 ? t_market_update_info_.bidlevels_[0].limit_int_price_ : 0),
        best_ask_int_price_(
            t_market_update_info_.asklevels_.size() >= 1 ? t_market_update_info_.asklevels_[0].limit_int_price_ : 0),
        best_bid_size_(t_market_update_info_.bidlevels_.size() >= 1 ? t_market_update_info_.bidlevels_[0].limit_size_
                                                                    : 0),
        best_ask_size_(t_market_update_info_.asklevels_.size() >= 1 ? t_market_update_info_.asklevels_[0].limit_size_
                                                                    : 0),
        best_bid_orders_(
            t_market_update_info_.bidlevels_.size() >= 1 ? t_market_update_info_.bidlevels_[0].limit_ordercount_ : 0),
        best_ask_orders_(
            t_market_update_info_.asklevels_.size() >= 1 ? t_market_update_info_.asklevels_[0].limit_ordercount_ : 0),
        msecs_from_midnight_(0) {}

  const std::string ToString() const {
    std::ostringstream t_oss_;
    t_oss_ << " " << msecs_from_midnight_ << " " << best_bid_size_ << " " << best_bid_orders_ << " "
           << best_bid_int_price_ << " X " << best_ask_int_price_ << " " << best_ask_size_ << " " << best_ask_orders_;
    return t_oss_.str();
  }

  bool operator==(const BestBidAskInfo& t_best_bid_ask_info_) const {
    bool is_equal_ = true;

    if (best_bid_int_price_ && t_best_bid_ask_info_.best_bid_int_price_) {
      if (best_bid_int_price_ != t_best_bid_ask_info_.best_bid_int_price_ ||
          best_bid_size_ != t_best_bid_ask_info_.best_bid_size_ ||
          best_bid_orders_ != t_best_bid_ask_info_.best_bid_orders_) {
        is_equal_ = false;
      }
    }

    if (best_ask_int_price_ && t_best_bid_ask_info_.best_ask_int_price_) {
      if (best_ask_int_price_ != t_best_bid_ask_info_.best_ask_int_price_ ||
          best_ask_size_ != t_best_bid_ask_info_.best_ask_size_ ||
          best_ask_orders_ != t_best_bid_ask_info_.best_ask_orders_) {
        is_equal_ = false;
      }
    }

    return is_equal_;
  }

  BestBidAskInfo operator-(const BestBidAskInfo& t_best_bid_ask_info_) const {
    BestBidAskInfo t_diff_best_bid_ask_info_(*this);

    if (best_bid_int_price_ == t_best_bid_ask_info_.best_bid_int_price_) {
      t_diff_best_bid_ask_info_.best_bid_int_price_ = best_bid_int_price_;
      t_diff_best_bid_ask_info_.best_bid_size_ = best_bid_size_ - t_best_bid_ask_info_.best_bid_size_;
      t_diff_best_bid_ask_info_.best_bid_orders_ = best_bid_orders_ - t_best_bid_ask_info_.best_bid_orders_;
    }

    if (best_ask_int_price_ == t_best_bid_ask_info_.best_ask_int_price_) {
      t_diff_best_bid_ask_info_.best_ask_int_price_ = best_ask_int_price_;
      t_diff_best_bid_ask_info_.best_ask_size_ = best_ask_size_ - t_best_bid_ask_info_.best_ask_size_;
      t_diff_best_bid_ask_info_.best_ask_orders_ = best_ask_orders_ - t_best_bid_ask_info_.best_ask_orders_;
    }

    return t_diff_best_bid_ask_info_;
  }

  BestBidAskInfo operator+(const BestBidAskInfo& t_best_bid_ask_info_) const {
    BestBidAskInfo t_sum_best_bid_ask_info_(*this);

    if (best_bid_int_price_ == t_best_bid_ask_info_.best_bid_int_price_) {
      t_sum_best_bid_ask_info_.best_bid_int_price_ = best_bid_int_price_;
      t_sum_best_bid_ask_info_.best_bid_size_ = best_bid_size_ + t_best_bid_ask_info_.best_bid_size_;
      t_sum_best_bid_ask_info_.best_bid_orders_ = best_bid_orders_ + t_best_bid_ask_info_.best_bid_orders_;
    } else if (!best_bid_int_price_ || (best_bid_int_price_ < t_best_bid_ask_info_.best_bid_int_price_ &&
                                        t_best_bid_ask_info_.best_bid_int_price_)) {
      t_sum_best_bid_ask_info_.best_bid_int_price_ = t_best_bid_ask_info_.best_bid_int_price_;
      t_sum_best_bid_ask_info_.best_bid_size_ = t_best_bid_ask_info_.best_bid_size_;
      t_sum_best_bid_ask_info_.best_bid_orders_ = t_best_bid_ask_info_.best_bid_orders_;
    }

    if (best_ask_int_price_ == t_best_bid_ask_info_.best_ask_int_price_) {
      t_sum_best_bid_ask_info_.best_ask_int_price_ = best_ask_int_price_;
      t_sum_best_bid_ask_info_.best_ask_size_ = best_ask_size_ + t_best_bid_ask_info_.best_ask_size_;
      t_sum_best_bid_ask_info_.best_ask_orders_ = best_ask_orders_ + t_best_bid_ask_info_.best_ask_orders_;
    } else if (!best_ask_int_price_ || (best_ask_int_price_ > t_best_bid_ask_info_.best_ask_int_price_ &&
                                        t_best_bid_ask_info_.best_ask_int_price_)) {
      t_sum_best_bid_ask_info_.best_ask_int_price_ = t_best_bid_ask_info_.best_ask_int_price_;
      t_sum_best_bid_ask_info_.best_ask_size_ = t_best_bid_ask_info_.best_ask_size_;
      t_sum_best_bid_ask_info_.best_ask_orders_ = t_best_bid_ask_info_.best_ask_orders_;
    }

    return t_sum_best_bid_ask_info_;
  }

 public:
  int best_bid_int_price_;
  int best_ask_int_price_;
  int best_bid_size_;
  int best_ask_size_;
  int best_bid_orders_;
  int best_ask_orders_;

  int msecs_from_midnight_;
};
}

#endif  // BASE_MARKETADAPTER_BASIC_MARKET_VIEW_STRUCTS_H
