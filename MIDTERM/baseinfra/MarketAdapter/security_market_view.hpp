/**
   \file MarketAdapter/security_market_view.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include <vector>
#include <deque>
#include <string>
#define CCPROFILING_TRADEINIT 1

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/safe_array.hpp"

#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_book_oputils.hpp"
#include "dvccode/CDef/random_number_generator.hpp"
#include "baseinfra/MarketAdapter/trade_base_px.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"
#include "baseinfra/OptionsUtils/option_object.hpp"
#include "baseinfra/MarketAdapter/sparse_index_book_pxlevel_manager.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define probability_delete_front_ 30

#define MIN_PRICE_LEVELS_TO_ITERATE 20
#define LOW_ACCESS_INDEX 50

namespace HFSAT {

/// Holds the book information.
/// Changed by EUREXPriceLevelMarketViewManager / FullBookMarketViewManager on market data updates
struct SecurityMarketView {
  DebugLogger &dbglogger_;  ///< for debugging
  const Watch &watch_;
  const SecurityNameIndexer &sec_name_indexer_;
  HybridSecurityManager *hybrid_security_manager_;
  SafeArray<double> price_to_yield_map_;
  // static loaded info
  double min_price_increment_;
  double yield_min_price_increment_;
  int min_order_size_;
  const FastPriceConvertor fast_price_convertor_;
  double normal_spread_increments_;  ///< normal value of spread_increments i.e. normal value of (askpx - bidpx) /
  /// min_price_increment
  double normal_spread_;  ///< normal value of spread i.e. normal value of (askpx - bidpx), got from
  /// NormalSpreadManager::GetNormalSpread
  double yield_normal_spread_;

  // Variables required to compute Implied Vol
  mutable SecurityMarketView *future_smv_;
  mutable OptionObject *option_;
  mutable bool price_type_subscribed_[kPriceTypeMax];  ///< flags indicating wqhich prices to compute

  bool computing_price_levels_;
  PriceType_t base_price_type_;
  const bool trade_before_quote_;
  bool is_ready_;
  bool l1_only_;

  mutable std::vector<double>
      offline_mix_mms_price_weights_;  ///< made mutable so that subscribe_price_type can change it
  mutable double online_const_c;
  mutable double online_const_k;

  mutable bool can_use_online_price_;

  mutable int min_bid_size_to_consider_;
  mutable int min_ask_size_to_consider_;
  mutable int bid_side_valid_level_int_price_;
  mutable int ask_side_valid_level_int_price_;
  mutable int bid_side_valid_level_size_;
  mutable int ask_side_valid_level_size_;
  mutable bool use_stable_bidask_levels_;

  mutable int band_lower_bid_int_price_;
  mutable int band_lower_ask_int_price_;
  mutable int bid_side_band_int_price_;
  mutable int ask_side_band_int_price_;
  mutable int bid_side_band_size_;
  mutable int ask_side_band_size_;
  mutable unsigned int num_non_empty_levels_in_band_;

  bool compute_trade_prices_;

  MarketUpdateInfo market_update_info_;
  MarketUpdateInfo hybrid_market_update_info_;
  TradePrintInfo trade_print_info_;
  TradePrintInfo raw_trade_print_info_;
  TradePrintInfo hybrid_trade_print_info_;

  BestBidAskInfo self_best_bid_ask_;

  mutable bool remove_self_orders_from_book_;

  mutable bool smart_remove_self_orders_from_book_;
  const int conf_to_market_update_msecs_;

  mutable bool dump_non_self_smv_on_next_update_;

  mutable const PromOrderManager *p_prom_order_manager_;

  BestBidAskInfo current_best_level_;
  BestBidAskInfo last_best_level_;

  //=============================================================================//
  // these vars are only used for setting best level after masking trades
  bool prev_bid_was_quote_;
  bool prev_ask_was_quote_;
  int top_bid_level_to_mask_trades_on_;
  int top_ask_level_to_mask_trades_on_;
  std::vector<int> running_hit_size_vec_;
  std::vector<int> running_lift_size_vec_;
  //=============================================================================//

  bool l1_changed_since_last_;
  bool l2_changed_since_last_;
  bool l1_price_changed_;
  bool l1_size_changed_;

  bool use_order_level_book_;

  //=============================================================================== Approximate Book Changes
  //===============================================================================//

  bool last_message_was_trade_;  // these are used to update order level book appropriately in case of Trades
  int last_traded_size_;
  double last_traded_price_;
  TradeType_t last_traded_tradetype_;
  std::vector<MarketOrder *> dummy_vec_to_return_;
  SimpleMempool<MarketOrder> *market_orders_mempool_;
  int prob_modify_from_top_;
  int prob_modify_level_;
  //===============================================================================
  //===============================================================================//

  //=============================================================================== Pro Rata Book Changes
  //===============================================================================//
  int min_priority_size_;
  int max_priority_size_;
  //===============================================================================
  //===============================================================================//

  /// made mutable so that subscribe_price_type can change it
  mutable FastTradeListenerVec fast_trade_listeners_;
  mutable SMVRawTradeListenerVec raw_trade_listeners_;  // listens to all the trades in the market without filtering any
  mutable SMVChangeListenerVec l1_price_listeners_;  // only listen to trades and changes to best market prices i.e. new
                                                     // levels created or old levels being deleted
  mutable SMVChangeListenerVec l1_size_listeners_;   // l1 price/size change
  mutable SMVChangeListenerVec l2_listeners_;        // any change till multiple levels
  mutable SMVL2ChangeListenerVec l2_only_listeners_;  // Should be interested in levels > 0 and levels != 0
  mutable SMVOnReadyListenerVec onready_listeners_;   // list of listeners who are not in the abovecategories and are
                                                      // alerted at the end of every update .. primarily meant for
                                                      // modelaggregators
  mutable SMVPLChangeListenerVec
      pl_change_listeners_;  ///< list of indicators which are listening to OnPLNew, OnPLDelete, OnPLChange
  mutable SMVChangeListenerVec otc_trades_listeners_;     // only listen to otc_trades
  mutable SMVChangeListenerVec spread_trades_listeners_;  // only listen to spread trades
  mutable SMVPreTradeListenerVec pre_trade_listeners_;
  mutable SMVStatusListenerVec market_status_listeners_;
  mutable SMVOffMarketTradeListenerVec off_market_trade_listeners_;

  mutable bool
      pl_change_listeners_present_;  ///< For optimization ... calc and call plchangelisteners only if this is true.
  SMVPreTradeListener *p_smv_pretrade_listener_;
  bool suspect_book_correct_;

  // holds time for order level data until callbacks can be made
  bool using_order_level_data_;  // flag indicator
  ttime_t skip_listener_notification_end_time_;

  bool process_market_data_;  // smv created but should we process data? needed for LIFFE in risk manager since mkt data
                              // for all prods on same port

  bool initial_book_constructed_;  // this is only used in indexed books ... ideally should have been in an internal
                                   // data structure

  // what does this refer to ? //
  std::vector<int> int_price_bid_book_;
  std::vector<int> int_price_ask_book_;

  std::vector<bool> int_price_bid_skip_delete_;
  std::vector<bool> int_price_ask_skip_delete_;

  unsigned int base_bid_index_;
  unsigned int base_ask_index_;

  int base_bid_intprice_index_adjustment_;  // difference between intprice and base_bid_index. this can be negative
  int base_ask_intprice_index_adjustment_;

  unsigned int last_base_bid_index_;
  unsigned int last_base_ask_index_;

  int this_int_price_;

  bool last_msg_was_quote_;
  int running_lift_size_;
  int running_hit_size_;

  int lift_base_index_;
  int hit_base_index_;

#ifdef L1_NOTIFICATION_CHANGE
  bool l1_notified_;
#endif
  unsigned int initial_tick_size_;
  unsigned int max_tick_range_;
  unsigned int max_allowed_tick_range_;

  ExchSource_t this_smv_exch_source_;
  //    bool using_order_level_data_; //can't use indexed book here
  MarketUpdateInfoLevelStruct dummy_market_update_info_level_struct_;  // just an error retval

  bool ors_exec_triggered_;
  bool last_exec_was_buy_;
  bool last_exec_was_sell_;

  unsigned int last_ors_exec_triggered_msecs_;
  unsigned int bestbid_size_before_ors_exec_triggered_;
  unsigned int bestask_size_before_ors_exec_triggered_;
  std::map<unsigned int, unsigned int> order_sequence_to_timestamp_when_placed_;
  std::map<unsigned int, unsigned int> order_sequence_to_size_when_placed_;

#define BYTE_SIZE 8
#define BIT_RESET_ALL 0x0000;

  // Faster Technique to Acess elements - Make sure we always use the same no.of bits for Bid and Ask - ravi
  uint16_t bid_access_bitmask_;
  uint16_t ask_access_bitmask_;
  uint16_t bitmask_lookup_table_[BYTE_SIZE * sizeof(bid_access_bitmask_)];         // Prepare a lookup table
  uint16_t bitmask_access_lookup_table_[BYTE_SIZE * sizeof(bid_access_bitmask_)];  // Prepare a lookup table for access
  uint16_t bitmask_size_;

  uint16_t bid_level_change_bitmask_;
  uint16_t ask_level_change_bitmask_;
  std::string offline_mix_mms_wts_filename_;
  std::string online_mix_price_const_filename_;

  std::string online_beta_kalman_const_filename_;

  // TradeBasePx struct
  TradeBasePxDetails trade_basepx_struct_;
  int level_size_thresh_;
  MktStatus_t current_market_status_;

  MarketUpdateInfoLevelStruct best_bid_level_info_struct_;
  MarketUpdateInfoLevelStruct best_ask_level_info_struct_;

  // Synthetic variables. Used for keeping uncrossed best variables in case the books allowes itself to be crossed.
  // Used only in NSE as of now.
  bool is_book_crossed_;
  bool using_predictive_uncross_;
  MarketUpdateInfoLevelStruct *syn_best_bid_level_info_;
  MarketUpdateInfoLevelStruct *syn_best_ask_level_info_;
  unsigned int syn_bestbid_index_;
  double syn_bestbid_price_;
  int syn_bestbid_size_;
  int syn_bestbid_int_price_;
  int syn_bestbid_ordercount_;

  unsigned int syn_bestask_index_;
  double syn_bestask_price_;
  int syn_bestask_size_;
  int syn_bestask_int_price_;
  int syn_bestask_ordercount_;

  // Price limit of order
  int upper_int_price_limit_;
  int lower_int_price_limit_;
  int upper_int_trade_range_limit_;
  int lower_int_trade_range_limit_;

  // Book with px_level_manager_support
  bool is_px_managed_smv_;
  SparseIndexBookPxLevelManager *px_level_manager_;

  // functions
  SecurityMarketView(DebugLogger &t_dbglogger_, const Watch &t_watch_, SecurityNameIndexer &t_sec_name_indexer_,
                     const std::string &t_shortcode_, const char *t_exchange_symbol_, const unsigned int t_security_id_,
                     const ExchSource_t t_exch_source_,
                     bool t_temporary_bool_checking_if_this_is_an_indexed_book_ = false,
                     const std::string &t_offline_mix_mms_wts_filename_ = DEFAULT_OFFLINEMIXMMS_FILE,
                     const std::string &t_online_mix_price_const_filename_ = DEFAULT_ONLINE_MIX_PRICE_FILE,
                     const std::string &t_online_beta_kalman_const_filename_ = DEFAULT_ONLINE_BETA_KALMAN_FILE);

  virtual ~SecurityMarketView(){};

  void GetonlineBetaKalmanFile(std::string &_file_name_) const { _file_name_ = online_beta_kalman_const_filename_; }

  // Specific Call to Setup the SMV for Indexed books
  void InitializeSMVForIndexedBook();

  bool operator==(const SecurityMarketView &t_other_) const { return (shortcode().compare(t_other_.shortcode()) == 0); }

  double min_price_increment() const { return min_price_increment_; }
  //  double min_price_increment() const {
  //    return ((price_to_yield_map_.empty()) ? min_price_increment_ : yield_min_price_increment_);
  //  }

  int min_order_size() const { return min_order_size_; }
  double normal_spread_increments() const {
    return normal_spread_increments_;
  }  ///< returns normal value of spread_increments i.e. normal value of (askpx - bidpx) / min_price_increment
  double normal_spread() const { return normal_spread_; }
  //  double normal_spread() const { return ((price_to_yield_map_.empty()) ? normal_spread_ : yield_normal_spread_); }
  int spread_increments() const { return market_update_info_.spread_increments_; }
  bool SpreadWiderThanNormal() const { return (market_update_info_.spread_increments_ > normal_spread_increments_); }
  bool UseOrderLevelBook() { return use_order_level_book_; }

  void SetUseOrderLevelBook() { use_order_level_book_ = true; }
  void UnsetUseOrderLevelBook() { use_order_level_book_ = false; }
  void SetProbModifyFromTop(int t_prob_modify_from_top_) { prob_modify_from_top_ = t_prob_modify_from_top_; }
  void SetProbModifyLevel(int t_prob_modify_level_) { prob_modify_level_ = t_prob_modify_level_; }
  void SetMinPriorityForProRata(int t_min_priority_) { min_priority_size_ = t_min_priority_; }
  void SetMaxPriorityForProRata(int t_max_priority_) { max_priority_size_ = t_max_priority_; }

  // for NSE currently
  void SetPxLevelManager(SparseIndexBookPxLevelManager *px_mgr) {
    is_px_managed_smv_ = true;
    px_level_manager_ = px_mgr;
    market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ = false;
  }

  int conf_to_market_update_msecs() const { return conf_to_market_update_msecs_; }
  bool remove_self_orders_from_book() const { return remove_self_orders_from_book_; }
  bool smart_remove_self_orders_from_book() const { return smart_remove_self_orders_from_book_; }

  void set_skip_listener_notification_end_time(const ttime_t r_start_time_) {
    using_order_level_data_ = true;  // only used by order book manager
    skip_listener_notification_end_time_ = r_start_time_;
  }

  void SetBasePriceType(PriceType_t t_base_price_type_) { base_price_type_ = t_base_price_type_; }
  bool ArePricesComputed() const;

  void SetSelfOrdersFromBook(const bool t_remove_self_) const;

  void SetSmartSelfOrdersFromBook(const bool t_smart_remove_self_) const;

  void DumpNonSelfSMV() const { dump_non_self_smv_on_next_update_ = true; }

  void SetPromOrderManager(const PromOrderManager *t_prom_order_manager_) {
    p_prom_order_manager_ = t_prom_order_manager_;
  }

  void SetL1OnlyFlag(bool _l1only_) { l1_only_ = _l1only_; }
  bool trade_before_quote() const { return trade_before_quote_; }
  bool is_ready() const { return is_ready_; }  ///< if the book is a consistent state
  MarketUpdateInfo &market_update_info() { return market_update_info_; }
  const MarketUpdateInfo &market_update_info() const { return market_update_info_; }
  TradePrintInfo &trade_print_info() { return trade_print_info_; }
  const TradePrintInfo &trade_print_info() const { return trade_print_info_; }

  int self_bestbid_int_price() const { return self_best_bid_ask_.best_bid_int_price_; }
  int self_bestbid_size() const { return self_best_bid_ask_.best_bid_size_; }
  int self_bestask_int_price() const { return self_best_bid_ask_.best_ask_int_price_; }
  int self_bestask_size() const { return self_best_bid_ask_.best_ask_size_; }
  void SetMarketOrdersMempool(SimpleMempool<MarketOrder> *t_market_orders_mempool_) {
    market_orders_mempool_ = t_market_orders_mempool_;
  }
  bool trade_update_implied_quote() const { return market_update_info_.trade_update_implied_quote_; }

  virtual bool is_ready_complex(int t_num_increments_) const {
#define MIN_L1EVENTS_TO_MAKE_READY_ON 5u
    return (is_ready_ &&
            (l1_only_ || ((market_update_info_.l1events_ > MIN_L1EVENTS_TO_MAKE_READY_ON) &&
                          (market_update_info_.spread_increments_ < (t_num_increments_ * normal_spread_increments_)))));
#undef MIN_L1EVENTS_TO_MAKE_READY_ON
  }

  virtual bool is_ready_complex2(unsigned long long t_num_trades_) const {
    return (is_ready_ && ((market_update_info_.trades_count_[kTradeTypeBuy] > t_num_trades_) &&
                          (market_update_info_.trades_count_[kTradeTypeSell] > t_num_trades_)));
  }

  const char *secname() const {
    return market_update_info_.secname_;
  }  ///< returns the exchange symbol for this instrument
  const std::string &shortcode() const {
    return market_update_info_.shortcode_;
  }  ///< returns the internal symbol (like "ZN_0") for this instrument
  unsigned int security_id() const { return market_update_info_.security_id_; }
  ExchSource_t exch_source() const { return market_update_info_.exch_source_; }

  void SetPreTradeListener(SMVPreTradeListener *_new_listener_) { p_smv_pretrade_listener_ = _new_listener_; }

  /**
   * The best level variables like bestbid_int_price_ could be different than bid_levels_[0].limit_int_price_
   *[could be due to
   *     (i) OnTrade interpolation
   *     (ii) from PromOrderManager seeing our size to remove_self_orders_from_book
   *     (iii) from PromOrderManager potentially listening to self executions at prices below best level
   *]
   *
   * Hence the book visible through SecurityMarketView::bid_int_price(i) call could appear inconsistent
   * i.e. two consecutive levels with same prices and different sizes.
   * In all such cases top level will be accurate
   */

  void set_compute_stable_prices(bool _set_value_) { market_update_info_.compute_stable_levels_ = _set_value_; }
  void set_level_size_thresh(int _set_value_) { level_size_thresh_ = _set_value_; }

  int GetBidIndex(int int_price) const {
    return (int)base_bid_index_ - (market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ - int_price);
  }

  int GetAskIndex(int int_price) const {
    return (int)base_ask_index_ + (market_update_info_.asklevels_[base_ask_index_].limit_int_price_ - int_price);
  }

  int GetBidIntPrice(int index) { return market_update_info_.bidlevels_[index].limit_int_price_; }

  int GetAskIntPrice(int index) { return market_update_info_.asklevels_[index].limit_int_price_; }

  int GetBidSize(int index) { return market_update_info_.bidlevels_[index].limit_size_; }

  int GetAskSize(int index) { return market_update_info_.asklevels_[index].limit_size_; }

  double GetBidPrice(int index) { return market_update_info_.bidlevels_[index].limit_price_; }

  double GetAskPrice(int index) { return market_update_info_.asklevels_[index].limit_price_; }

  int GetBidOrders(int index) { return market_update_info_.bidlevels_[index].limit_ordercount_; }

  int GetAskOrders(int index) { return market_update_info_.asklevels_[index].limit_ordercount_; }

  void UpdateBestBid(int index);
  void UpdateBestAsk(int index);

  int GetL1BidIntPrice() { return GetBidIntPrice(base_bid_index_); }
  int GetL1AskIntPrice() { return GetAskIntPrice(base_ask_index_); }

  int GetL1BidSize() {
    // We assume that base_bid_index is always a sensible number
    return GetBidSize(base_bid_index_);
  }

  int GetL1AskSize() {
    // We assume that base_ask_index is always a sensible number
    return GetAskSize(base_ask_index_);
  }

  int GetL1BidOrders() { return GetBidOrders(base_bid_index_); }

  int GetL1AskOrders() { return GetAskOrders(base_ask_index_); }

  double GetL1BidPrice() { return GetBidPrice(base_bid_index_); }

  double GetL1AskPrice() { return GetAskPrice(base_ask_index_); }

  void ResetBidLevel(int index);
  void ResetAskLevel(int index);

  void UpdateBidLevel(int index, int size, int ordercount);
  void UpdateAskLevel(int index, int size, int ordercount);

  // Shift down for new level
  void DownBidBitmask(int index) {
    if (index - base_bid_index_ < bitmask_size_) {
      bid_access_bitmask_ >>= (index - base_bid_index_);

    } else {
      bid_access_bitmask_ = BIT_RESET_ALL;
    }
  }

  // Shift down for new level
  void DownAskBitmask(int index) {
    if (index - base_ask_index_ < bitmask_size_) {
      ask_access_bitmask_ >>= (index - base_ask_index_);

    } else {
      ask_access_bitmask_ = BIT_RESET_ALL;
    }
  }

  // Shift up for l1 level delete
  void UpBidBitmask(int index) {
    if (base_bid_index_ - index < bitmask_size_) {
      bid_access_bitmask_ <<= (base_bid_index_ - index);

    } else {
      bid_access_bitmask_ = BIT_RESET_ALL;
    }
  }

  // Shift up for l1 level delete
  void UpAskBitmask(int index) {
    if (base_ask_index_ - index < bitmask_size_) {
      ask_access_bitmask_ <<= (base_ask_index_ - index);

    } else {
      ask_access_bitmask_ = BIT_RESET_ALL;
    }
  }

  // Set a bit for add level
  void AddBidBitmask(int index) {
    if (base_bid_index_ - index < bitmask_size_) {
      bid_access_bitmask_ |= bitmask_lookup_table_[base_bid_index_ - index];
    }
  }

  // Set a bit for add level
  void AddAskBitmask(int index) {
    if (base_ask_index_ - index < bitmask_size_) {
      ask_access_bitmask_ |= bitmask_lookup_table_[base_ask_index_ - index];
    }
  }

  // Unset a bit for delete level
  void RemoveBidBitmask(int index) {
    if (base_bid_index_ - index < bitmask_size_) {
      bid_access_bitmask_ &= (~bitmask_lookup_table_[base_bid_index_ - index]);
    }
  }

  // Unset a bit for delete level
  void RemoveAskBitmask(int index) {
    if (base_ask_index_ - index < bitmask_size_) {
      ask_access_bitmask_ &= (~bitmask_lookup_table_[base_ask_index_ - index]);
    }
  }

  void NewBidLevelmask(int index) {
    if (index > (int)base_bid_index_) {
      bid_level_change_bitmask_ = 0xFFFF;

    } else {
      bid_level_change_bitmask_ = bid_level_change_bitmask_ | 0x0001;
    }
  }

  void NewAskLevelmask(int index) {
    if (index > (int)base_ask_index_) {
      ask_level_change_bitmask_ = 0xFFFF;

    } else {
      ask_level_change_bitmask_ = ask_level_change_bitmask_ | 0x0001;
    }
  }

  void SanitizeBidLevel1(int level, int index);
  void SanitizeAskLevel1(int level, int index);
  void SanitizeBidLastLevel(int level, int last_level, int index);
  void SanitizeAskLastLevel(int level, int last_level, int index);

  // Sanitize bid side given the ask int price
  void SanitizeBidSide(int int_price);
  // Sanitize ask side given the bid int price
  void SanitizeAskSide(int int_price);

  void UpdateBestBidVariablesUsingOurOrders();
  void UpdateBestAskVariablesUsingOurOrders();

  void NotifyDeleteListeners(TradeType_t buysell, int int_price, int level_removed, int old_size, int old_ordercount,
                             bool intermediate);

  // Returns false - if we need to return with notifying the listeners
  // Returns true - if we need to notify the listeners based on change flags
  bool DeleteBid(int int_price);
  bool DeleteAsk(int int_price);

  void RebuildIndexLowAccess(TradeType_t buysell, int new_int_price);

  unsigned int get_next_stable_bid_level_index(unsigned int current_bid_index_, int _level_size_thresh_,
                                               int &size_seen_, int &orders_seen_) const;

  unsigned int bidlevels_size() const;

  bool IsL1Valid() const { return ((!IsBidBookEmpty()) && (!IsAskBookEmpty())); }
  bool IsBidBookEmpty() const;
  bool IsBidBookL2() const;

  int bid_int_price_level(unsigned int t_level_) const;

  int bid_int_price(unsigned int t_level_) const;
  double bid_price(unsigned int t_level_) const;

  int bid_level_at_int_price(int t_int_price_) const;
  int next_bid_level_int_price(int t_int_price_) const;

  /// TODO : evaluate and make special case for L1
  int bid_size_at_int_price(int t_int_price_) const;

  int bid_size(unsigned int t_level_) const;
  int bid_order_at_int_price(int t_int_price_) const;
  int bid_order(unsigned int t_level_) const;
  MarketUpdateInfoLevelStruct *bid_level_info(unsigned int t_level_);

  inline MarketUpdateInfoLevelStruct *bid_info(unsigned int t_level_) {
    if (this_smv_exch_source_ == kExchSourceNSE) {
      return px_level_manager_->GetSynBidPL(market_update_info_.security_id_, t_level_);
    } else if ((this_smv_exch_source_ == kExchSourceBMF_FPGA) || (this_smv_exch_source_ == kExchSourceBMF)) {
      return bid_level_info(t_level_);
    }
    return nullptr;
  }

  inline MarketUpdateInfoLevelStruct *ask_info(unsigned int t_level_) {
    if (this_smv_exch_source_ == kExchSourceNSE) {
      return px_level_manager_->GetSynAskPL(market_update_info_.security_id_, t_level_);
    } else if ((this_smv_exch_source_ == kExchSourceBMF_FPGA) || (this_smv_exch_source_ == kExchSourceBMF)) {
      return ask_level_info(t_level_);
    }
    return nullptr;
  }

  unsigned int GetBaseBidMapIndex() const {
    if (is_ready_) {
      return GetBidIndex(market_update_info_.bestbid_int_price_);
    } else {
      return base_bid_index_;
    }
  }

  // Indexed Book faster access functions
  // takes an input the exact input index for indexed book
  // finds the next index that is valid
  unsigned int GetNextBidMapIndex(unsigned int current_bid_index_) const;

  unsigned int IncrementBidMapIndex(unsigned int current_bid_index_) const {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      current_bid_index_--;
    } else {
      current_bid_index_++;
    }
    return current_bid_index_;
  }

  unsigned int IndexedBookGetNextNonEmptyBidMapIndex(unsigned int current_bid_index_) const;

  // generally this function is to access a level for some computation ..
  // hence const reference access
  const MarketUpdateInfoLevelStruct &GetBidLevelAtIndex(unsigned int current_bid_index_) const;

  /**
   * The best level variables like bestbid_int_price_ could be different than bid_levels_[0].limit_int_price_
   *[could be due to
   *     (i) OnTrade interpolation
   *     (ii) from PromOrderManager seeing our size to remove_self_orders_from_book
   *     (iii) from PromOrderManager potentially listening to self executions at prices below best level
   *]
   *
   * Hence the book visible through SecurityMarketView::bid_int_price(i) call could appear inconsistent
   * i.e. two consecutive levels with same prices and different sizes.
   * In all such cases top level will be accurate
   */

  unsigned int get_next_stable_ask_level_index(unsigned int current_ask_index_, int _level_size_thresh_,
                                               int &size_seen_, int &orders_seen_) const;

  unsigned int asklevels_size() const;
  bool IsAskBookL2() const;
  bool IsAskBookEmpty() const;

  int ask_int_price_level(unsigned int t_level_) const;
  int ask_int_price(unsigned int t_level_) const;
  double ask_price(unsigned int t_level_) const;

  int next_ask_level_int_price(int t_int_price_) const;
  int ask_level_at_int_price(int t_int_price_) const;

  /// TODO : evaluate and make special case for L1
  int ask_size_at_int_price(int t_int_price_) const;
  int ask_size(unsigned int t_level_) const;
  int ask_order_at_int_price(int t_int_price_) const;
  int ask_order(unsigned int t_level_) const;
  MarketUpdateInfoLevelStruct *ask_level_info(unsigned int t_level_);

  unsigned int GetBaseAskMapIndex() const {
    if (is_ready_) {
      return GetAskIndex(market_update_info_.bestask_int_price_);
    } else {
      return base_ask_index_;
    }
  }

  // Indexed Book faster access functions
  // takes an input the exact input index for indexed book
  // finds the next index that is valid
  unsigned int GetNextAskMapIndex(unsigned int current_ask_index_) const {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      if (market_update_info_.compute_stable_levels_) {
        int size_seen_ = 0, orders_seen_ = 0;
        return get_next_stable_ask_level_index(current_ask_index_, level_size_thresh_, size_seen_, orders_seen_);
      }
      return IndexedBookGetNextNonEmptyAskMapIndex(current_ask_index_);
    } else {
      return current_ask_index_++;
    }
  }

  unsigned int IncrementAskMapIndex(unsigned int current_ask_index_) const {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      current_ask_index_--;
    } else {
      current_ask_index_++;
    }
    return current_ask_index_;
  }

  unsigned int IndexedBookGetNextNonEmptyAskMapIndex(unsigned int current_ask_index_) const {
    if ((int)current_ask_index_ <= 0) return 0u;
    do {
      current_ask_index_--;
    } while ((current_ask_index_ > 0) && (market_update_info_.asklevels_[current_ask_index_].limit_size_ <= 0));

    return current_ask_index_;
  }

  const MarketUpdateInfoLevelStruct &GetAskLevelAtIndex(unsigned int current_ask_index_) const {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ &&
        (current_ask_index_ < market_update_info_.asklevels_.size())) {
      if (is_ready_ && (GetAskIndex(market_update_info_.bestask_int_price_) == (int)current_ask_index_)) {
        return best_ask_level_info_struct_;
      } else {
        return market_update_info_.asklevels_[current_ask_index_];
      }
    } else {
      if (current_ask_index_ < market_update_info_.asklevels_.size()) {
        return market_update_info_.asklevels_[current_ask_index_];
      } else {
        return dummy_market_update_info_level_struct_;
      }
    }
  }

  bool IsTopIndexedLevelValid() const {
    return (base_bid_index_ && (market_update_info_.bidlevels_[base_bid_index_].limit_size_ > 0) && base_ask_index_ &&
            (market_update_info_.asklevels_[base_ask_index_].limit_size_ > 0));
  }

  /**
   * The best level variables like bestbid_int_price_ could be different than bid_levels_[0].limit_int_price_
   *[could be due to
   *     (i) OnTrade interpolation
   *     (ii) from PromOrderManager seeing our size to remove_self_orders_from_book
   *     (iii) from PromOrderManager potentially listening to self executions at prices below best level
   *]
   *
   * Hence the book visible through SecurityMarketView::bid_int_price(i) call could appear inconsistent
   * i.e. two consecutive levels with same prices and different sizes.
   * In all such cases top level will be accurate
   */

  double bestbid_price() const {
    return price_to_yield_map_.empty() ? market_update_info_.bestbid_price_ : hybrid_market_update_info_.bestbid_price_;
  }

  double bestask_price() const {
    return price_to_yield_map_.empty() ? market_update_info_.bestask_price_ : hybrid_market_update_info_.bestask_price_;
  }

  int bestbid_size() const { return market_update_info_.bestbid_size_; }
  int bestask_size() const { return market_update_info_.bestask_size_; }
  int bestbid_int_price() const { return market_update_info_.bestbid_int_price_; }
  int bestask_int_price() const { return market_update_info_.bestask_int_price_; }
  int bestbid_ordercount() const { return market_update_info_.bestbid_ordercount_; }
  int bestask_ordercount() const { return market_update_info_.bestask_ordercount_; }
  std::string MarketUpdateInfoToString();

  double mid_price() const {
    return price_to_yield_map_.empty() ? market_update_info_.mid_price_ : hybrid_market_update_info_.mid_price_;
  }

  double mkt_size_weighted_price() const {
    return price_to_yield_map_.empty() ? market_update_info_.mkt_size_weighted_price_
                                       : hybrid_market_update_info_.mkt_size_weighted_price_;
  }

  double mkt_sinusoidal_price() const {
    return price_to_yield_map_.empty() ? market_update_info_.mkt_sinusoidal_price_
                                       : hybrid_market_update_info_.mkt_sinusoidal_price_;
  }
  double order_weighted_price() const {
    return price_to_yield_map_.empty() ? market_update_info_.order_weighted_price_
                                       : hybrid_market_update_info_.order_weighted_price_;
  }

  double offline_mix_mms_price() const {
    return price_to_yield_map_.empty() ? market_update_info_.offline_mix_mms_price_
                                       : hybrid_market_update_info_.offline_mix_mms_price_;
  }

  double valid_level_mid_price() const {
    return price_to_yield_map_.empty() ? market_update_info_.valid_level_mid_price_
                                       : hybrid_market_update_info_.valid_level_mid_price_;
  }

  double band_mkt_price() const {
    return price_to_yield_map_.empty() ? market_update_info_.band_mkt_price_
                                       : hybrid_market_update_info_.band_mkt_price_;
  }

  double trade_base_price_() const {
    return price_to_yield_map_.empty() ? market_update_info_.trade_base_px_ : hybrid_market_update_info_.trade_base_px_;
  }

  double trade_mktsz_price_() const {
    return price_to_yield_map_.empty() ? market_update_info_.trade_mktsz_px_
                                       : hybrid_market_update_info_.trade_mktsz_px_;
  }

  double trade_mktsin_price_() const {
    return price_to_yield_map_.empty() ? market_update_info_.trade_mktsin_px_
                                       : hybrid_market_update_info_.trade_mktsin_px_;
  }

  double trade_orderw_price_() const {
    return price_to_yield_map_.empty() ? market_update_info_.trade_orderw_px_
                                       : hybrid_market_update_info_.trade_orderw_px_;
  }

  double trade_tradew_price_() const {
    return price_to_yield_map_.empty() ? market_update_info_.trade_tradew_px_
                                       : hybrid_market_update_info_.trade_tradew_px_;
  }

  double trade_omix_price_() const {
    return price_to_yield_map_.empty() ? market_update_info_.trade_omix_px_ : hybrid_market_update_info_.trade_omix_px_;
  }

  double order_size_weighted_price_() const {
    return price_to_yield_map_.empty() ? market_update_info_.order_size_weighted_price_
                                       : hybrid_market_update_info_.order_size_weighted_price_;
  }

  double stable_bid_price_() const {
    return price_to_yield_map_.empty() ? market_update_info_.stable_bid_price_
                                       : hybrid_market_update_info_.stable_bid_price_;
  }

  double stable_ask_price_() const {
    return price_to_yield_map_.empty() ? market_update_info_.stable_ask_price_
                                       : hybrid_market_update_info_.stable_ask_price_;
  }

  static double GetPriceFromType(const PriceType_t t_price_type_, const MarketUpdateInfo &this_market_update_info_);

  static const double &GetPriceRefFromType(const PriceType_t t_price_type_,
                                           const MarketUpdateInfo &this_market_update_info_);

  double price_from_type(const PriceType_t t_price_type_) const;

  const double &GetPriceRef(const PriceType_t t_price_type_) const;

  bool computing_price_levels() const { return computing_price_levels_; }

  void CheckComputeStablePrice();

  void subscribe_to_fast_trade_updates(FastTradeListener *t_fast_trade_listener) const;
  virtual void subscribe_tradeprints(SecurityMarketViewChangeListener *t_new_listener_) const;
  void subscribe_rawtradeprints(SecurityMarketViewRawTradesListener *t_new_listener_) const;
  bool subscribe_price_type(SecurityMarketViewChangeListener *t_new_listener_, PriceType_t price_type_) const;
  void subscribe_l1_price(SecurityMarketViewChangeListener *t_new_listener_) const;
  void subscribe_l1(SecurityMarketViewChangeListener *t_new_listener_) const;
  void subscribe_L2(SecurityMarketViewChangeListener *t_new_listener_);
  void subscribe_L2_Only(SecurityMarketViewL2ChangeListener *t_new_listener_);
  void subscribe_SpreadTrades(SecurityMarketViewChangeListener *t_new_listener_);
  virtual void subscribe_L1_Only(SecurityMarketViewChangeListener *t_new_listener_);

  virtual void subscribe_OnReady(SecurityMarketViewOnReadyListener *t_new_listener_) const;

  void subscribe_PreTrades(SMVPreTradeListener *t_new_listener_);
  void subscribe_MktStatus(SecurityMarketViewStatusListener *t_new_listener_) const;
  void subscribe_OffMktTrades(SMVOffMarketTradeListener *t_new_listener_) const;

  virtual void ComputeMidPrice() const { price_type_subscribed_[kPriceTypeMidprice] = true; }
  virtual void ComputeMktPrice() const { price_type_subscribed_[kPriceTypeMktSizeWPrice] = true; }
  virtual void ComputeProrataMktPrice() const { price_type_subscribed_[kPriceTypeProRataMktSizeWPrice] = true; }
  virtual void ComputeMktSinPrice() const { price_type_subscribed_[kPriceTypeMktSinusoidal] = true; }
  virtual void ComputeOrderWPrice() const { price_type_subscribed_[kPriceTypeOrderWPrice] = true; }
  virtual void ComputeTradeWPrice() const { price_type_subscribed_[kPriceTypeTradeWPrice] = true; }
  virtual void ComputeValidLevelMidPrice() const { price_type_subscribed_[kPriceTypeValidLevelMidPrice] = true; }
  virtual void ComputeBandPrice() const { price_type_subscribed_[kPriceTypeBandPrice] = true; }
  virtual void ComputeTradeBasePrice() const { price_type_subscribed_[kPriceTypeTradeBasePrice] = true; }
  virtual void ComputeTradeMktSizePrice() const { price_type_subscribed_[kPriceTypeTradeMktSizeWPrice] = true; }
  virtual void ComputeTradeMktSinPrice() const { price_type_subscribed_[kPriceTypeTradeMktSinPrice] = true; }
  virtual void ComputeTradeOrderWPrice() const { price_type_subscribed_[kPriceTypeTradeOrderWPrice] = true; }
  virtual void ComputeTradeTradeWPrice() const { price_type_subscribed_[kPriceTypeTradeTradeWPrice] = true; }
  virtual void ComputeTradeOmixPrice() const { price_type_subscribed_[kPriceTypeTradeOmixPrice] = true; }
  virtual void ComputeOrderSizeWPrice() const { price_type_subscribed_[kPriceTypeOrderSizeWPrice] = true; }
  virtual void ComputeOnlineMixPrice() const { price_type_subscribed_[kPriceTypeOnlineMixPrice] = true; }
  virtual void ComputeStableBidPrice() const { price_type_subscribed_[kPriceTypeStableBidPrice] = true; }
  virtual void ComputeStableAskPrice() const { price_type_subscribed_[kPriceTypeStableAskPrice] = true; }
  virtual void ComputeImpliedVol() const { price_type_subscribed_[kPriceTypeImpliedVol] = true; }

  // given there are multiple guys accessing this class
  // we need a way to know
  // all listeners
  // what all getting computed
  // what is the current state
  // for debug purpose ofcourse
  // just dump to log file
  void DumpStatus();

  virtual void ComputeIntPriceLevels() { computing_price_levels_ = true; }

  virtual void ComputeTradepxMktpxDiff() {
    trade_print_info_.computing_tradepx_mktpx_diff_ = true;
    ComputeMktPrice();
  }
  virtual void ComputeLBTDiff() {
    trade_print_info_.computing_last_book_tdiff_ = true;
    ComputeMktPrice();
  }
  virtual void ComputeIntTradeType() { trade_print_info_.computing_int_trade_type_ = true; }
  virtual void ComputeSqrtSizeTraded() { trade_print_info_.computing_sqrt_size_traded_ = true; }
  virtual void ComputeTradeImpact() {
    SetStorePreTrade();
    trade_print_info_.computing_trade_impact_ = true;
  }
  virtual void ComputeSqrtTradeImpact() {
    ComputeTradeImpact();
    trade_print_info_.computing_sqrt_trade_impact_ = true;
  }

  // In case the exchange sends trade information first like CME,
  // pretrade information is only needed in indicators to compare against current values.
  //
  // for variables like tradepx_mktpx_diff_ computed in SMV, no comparison is needed against pretrade
  // values in a listener to SMV, since the computation can be done first in SMV.
  // then market_update_info_ can be updated and then the callbacks can be called.
  void SetStorePreTrade() { market_update_info_.storing_pretrade_state_ = true; }
  bool GetStorePreTrade() const { return market_update_info_.storing_pretrade_state_; }

  double GetDoublePx(const int t_int_price_) const { return min_price_increment_ * t_int_price_; }

  int GetIntPx(const double &t_price_) const { return fast_price_convertor_.GetFastIntPx(t_price_); }
  bool DblPxCompare(const double &t_price1_, const double &t_price2_) const {
    return fast_price_convertor_.DblPxCompare(t_price1_, t_price2_);
  }
  int DblPxDiffSign(const double &t_price1_, const double &t_price2_) const {
    return fast_price_convertor_.DblPxDiffSign(t_price1_, t_price2_);
  }

  void RecomputeOfflineMixPrice();
  void RecomputeOnlineMixPrice();
  void RecomputeValidLevelPrices();
  void RecomputeBandPrices();
  void RecomputeStableBidPrice();
  void RecomputeStableAskPrice();
  void RecomputeImpliedVol();
  void SetFutureSMV(SecurityMarketView *_future_smv_) {
    future_smv_ = _future_smv_;
    if (!future_smv_->subscribe_price_type(nullptr, kPriceTypeMidprice)) {
      PriceType_t t_error_price_type_ = kPriceTypeMidprice;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed "
                << t_error_price_type_ << std::endl;
    }
  }

  void OnTrade(const double t_trade_price_, const int t_trade_size_,
               const TradeType_t t_buysell_);  ///< called by EUREXPriceLevelMarketViewManager or
  /// FullBookMarketViewManager on a Trade notification.
  void OnL1Trade(const double t_trade_price_, const int t_trade_size_,
                 const TradeType_t t_buysell_);  ///< called by EUREXPriceLevelMarketViewManager or
  /// FullBookMarketViewManager on a Trade notification.
  void OnNonDisplayedTrade(
      const double t_trade_price_, const int t_trade_size_,
      const TradeType_t t_buysell_);  /// Called for NASDAQ Trade messages which happens for non displayed orders
  void OnL1PriceUpdate();  ///< called by EUREXPriceLevelMarketViewManager or FullBookMarketViewManager when the update
  /// from the market is of a top level price change... updates all internal prices and calls
  /// listeners
  void OnL1SizeUpdate();
  void OnL2Update();
  void OnL2OnlyUpdate();

  void EmptyBook();  // for order book

  void EraseBid(int level);
  void EraseAsk(int level);
  void Sanitize();
  void SanitizeBidSideWithLevel(int level_added);
  void SanitizeAskSideWithLevel(int level_added);
  void SanitizeBidSideWithPrice(int this_int_price);
  void SanitizeAskSideWithPrice(int this_int_price);

  void SetupOfflineMixMMS() const;
  void SetupOnlinePriceConstants() const;
  void SetupValidLevelSizes() const;
  void SetupBands() const;
  void SetupImpliedVolVariables() const;
  // these functions just call listeners

  void NotifyFastTradeListeners(const unsigned int t_security_id_, TradeType_t t_aggressive_side,
                                double t_last_traded_price);
  void NotifyOTCTradeListeners(const double t_otc_trade_price_, const int t_otc_trade_size_);
  void NotifySpreadTradeListeners();
  void NotifyTradeListeners();
  void NotifyRawTradeListeners();
  void NotifyL1PriceListeners();
  void NotifyL1SizeListeners();
  void NotifyL2Listeners();
  void NotifyL2OnlyListeners();
  void NotifyOnReadyListeners();

  void NotifyOnPLChangeListeners(const unsigned int t_security_id_, const MarketUpdateInfo &r_market_update_info_,
                                 const TradeType_t t_buysell_, const int t_level_changed_, const int t_int_price_,
                                 const int t_int_price_level_, const int t_old_size_, const int t_new_size_,
                                 const int t_old_ordercount_, const int t_new_ordercount_,
                                 const bool t_is_intermediate_message_, const char t_pl_notif);

  void NotifyPreTradeListeners(TradePrintInfo &trade_print_) {
    for (auto i = 0u; i < pre_trade_listeners_.size(); i++) {
      pre_trade_listeners_[i]->OnSMVPreTrade(market_update_info_.security_id_, trade_print_, market_update_info_);
    }
  }

  void NotifyMarketStatusListeners(const unsigned int t_security_id_, const MktStatus_t _this_market_status_);

  void NotifyOffMarketTradeListeners(const unsigned int t_security_id_, const double _price_, const int _int_price_,
                                     const int _trade_size_, const TradeType_t _buysell_, int _lvl_);

  void subscribePlEvents(SMVPLChangeListener *t_new_listener_) const;
  void unsubscribePlEvents(SMVPLChangeListener *t_new_listener_);

  /// 1. henceforth ignore effects of trade masks and
  /// 2. later if we start handling self executions, effect of last self execution can be ingored as well
  ///          (note that in EUREX updates are sent to public stream before they are sent to private stream and hence
  ///          looking at self executions should not be useful)

  void SetBestLevelBidVariablesOnQuote();

  void SetBestLevelAskVariablesOnQuote();

  void SetBestLevelBidVariablesIfNonSelfBook();
  void SetBestLevelAskVariablesIfNonSelfBook();

  /// If aggressive side of the Trade is kTradeTypeBuy
  /// deduct running_lift_size_ from top level ask
  void SetBestLevelAskVariablesOnLift();
  void SetBestLevelBidVariablesOnHit();

  void StorePreBook(TradeType_t t_buysell_);

  /// StorePreTrade stores the market variables for use in the next quote.
  /// Right now this is only called from OnTrade but in markets wehre quote is updated before trade
  /// We will need to call this on an L1 quote update as well
  void StorePreTrade();

  int NumBidLevels() const {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      return DEF_MARKET_DEPTH;
    }
    return market_update_info_.bidlevels_.size();
  }

  int NumAskLevels() const {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      return DEF_MARKET_DEPTH;
    }
    return market_update_info_.asklevels_.size();
  }

  unsigned long long l1events() const { return market_update_info_.l1events_; }
  unsigned int num_trades() const { return trade_print_info_.num_trades_; }

  /// unlike NumBidLevels this function has an input max beyond with the value is not of interest
  /// used to optimize for indexed book
  unsigned int MinValidNumBidLevels(unsigned int t_max_interested_value_) const;

  /// unlike NumAskLevels this function has an input max beyond with the value is not of interest
  /// used to optimize for indexed book
  unsigned int MinValidNumAskLevels(unsigned int t_max_interested_value_) const;

  void AddTopBid(MarketUpdateInfoLevelStruct &new_level_);

  void AddTopAsk(MarketUpdateInfoLevelStruct &new_level_);

  // To be used only by L1 manager
  void ReplaceTopBid(MarketUpdateInfoLevelStruct &new_level_);

  // To be used only by L1 manager
  void ReplaceTopAsk(MarketUpdateInfoLevelStruct &new_level_);

  int AddNonTopBid(int t_level_added_, MarketUpdateInfoLevelStruct &new_level_);
  int AddNonTopAsk(int t_level_added_, MarketUpdateInfoLevelStruct &new_level_);

  void RemoveTopBid() {
    if (market_update_info_.bidlevels_.size() > 0) {
      market_update_info_.bidlevels_.erase(market_update_info_.bidlevels_.begin());
    }
  }

  void RemoveTopAsk() {
    if (market_update_info_.asklevels_.size() > 0) {
      market_update_info_.asklevels_.erase(market_update_info_.asklevels_.begin());
    }
  }

  void RemoveNonTopBid(int t_level_removed_);
  void RemoveNonTopAsk(int t_level_removed_);

  //=================================================== LIFFE =====================================================//

  void AddBestBidPrice(MarketUpdateInfoLevelStruct &new_level_);
  void AddBestAskPrice(MarketUpdateInfoLevelStruct &new_level_);
  int AddBidPrice(MarketUpdateInfoLevelStruct &new_level_);
  int AddAskPrice(MarketUpdateInfoLevelStruct &new_level_);

  void RemoveBestBidPrice() {
    if (market_update_info_.bidlevels_.size() > 0) {
      market_update_info_.bidlevels_.erase(market_update_info_.bidlevels_.begin());
    }
  }

  void RemoveBestAskPrice() {
    if (market_update_info_.asklevels_.size() > 0) {
      market_update_info_.asklevels_.erase(market_update_info_.asklevels_.begin());
    }
  }

  int RemoveBidPrice(int t_int_price_);

  int RemoveAskPrice(int t_int_price_);

  //=================================================================================================================//

  //=============================================================================== Approximate Book Changes
  //===============================================================================//

  void AddNewLevelBidOrderDepth(int t_level_added_, MarketOrder *p_market_order_);
  void AddNewLevelAskOrderDepth(int t_level_added_, MarketOrder *p_market_order_);

  void DeleteBidOrderDepth(int t_level_deleted_) {
    if ((int)market_update_info_.bid_level_order_depth_book_.size() >= t_level_deleted_) {
      market_update_info_.bid_level_order_depth_book_.erase(market_update_info_.bid_level_order_depth_book_.begin() +
                                                            t_level_deleted_ - 1);
    }
  }

  void DeleteAskOrderDepth(int t_level_deleted_) {
    if ((int)market_update_info_.ask_level_order_depth_book_.size() >= t_level_deleted_) {
      market_update_info_.ask_level_order_depth_book_.erase(market_update_info_.ask_level_order_depth_book_.begin() +
                                                            t_level_deleted_ - 1);
    }
  }

  std::vector<MarketOrder *> &get_bid_order_book_at_this_level(int t_level_) {
    if (t_level_ >= 0 && t_level_ < (int)market_update_info_.bid_level_order_depth_book_.size()) {
      return market_update_info_.bid_level_order_depth_book_[t_level_];
    }
    return dummy_vec_to_return_;
  }

  std::vector<MarketOrder *> &get_ask_order_book_at_this_level(int t_level_) {
    if (t_level_ >= 0 && t_level_ < (int)market_update_info_.ask_level_order_depth_book_.size()) {
      return market_update_info_.ask_level_order_depth_book_[t_level_];
    }
    return dummy_vec_to_return_;
  }

  std::vector<MarketOrder *> &get_bid_order_book_at_this_price(int t_bid_price_) {
    for (int x = 0; x < (int)market_update_info_.bid_level_order_depth_book_.size(); x++) {
      if (market_update_info_.bidlevels_[x].limit_int_price_ == t_bid_price_) {
        return market_update_info_.bid_level_order_depth_book_[x];
      }
    }
    return dummy_vec_to_return_;
  }

  std::vector<MarketOrder *> &get_ask_order_book_at_this_price(int t_ask_price_) {
    for (int x = 0; x < (int)market_update_info_.ask_level_order_depth_book_.size(); x++) {
      if (market_update_info_.asklevels_[x].limit_int_price_ == t_ask_price_) {
        return market_update_info_.ask_level_order_depth_book_[x];
      }
    }
    return dummy_vec_to_return_;
  }

  int CheckIfSamePriceExistsOnAsk(int t_level_added_, int t_int_price_);

  int CheckIfSamePriceExistsOnBid(int t_level_added_, int t_int_price_);

  void CopyOrderLevelDepthBookOnAsk(int t_level_added_, int index_to_copy_);
  void CopyOrderLevelDepthBookOnBid(int t_level_added_, int index_to_copy_);

  void DeleteBidSidePriorities();
  void DeleteAskSidePriorities();
  MarketUpdateInfoLevelStruct GetBidPriceLevelBookAtPrice(int t_bid_price_);

  MarketUpdateInfoLevelStruct GetAskPriceLevelBookAtPrice(int t_ask_price_);

  // This is used to generate seed fro Random Nos
  __inline__ unsigned long GetCpucycleCount(void) {
    uint32_t lo, hi;
    __asm__ __volatile__(  // serialize
        "xorl %%eax,%%eax \n        cpuid" ::
            : "%rax", "%rbx", "%rcx", "%rdx");
    /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (long)hi << 32 | lo;
    /*
        unsigned a, d;
        asm volatile("rdtsc" : "=a" (a), "=d" (d));
        return ((cyclecount_t)a) | (((cyclecount_t)d) << 32);
     */
  }

  //========================================================================================
  //========================================================================================//

  bool Uncross();

  std::string ShowMarket() const;
  bool Check_Bid_Order_Level_Correctness(const int t_level_) const;
  bool Check_Ask_Order_Level_Correctness(const int t_level_) const;

  void UpdateBestBidInfoStruct() {
    best_bid_level_info_struct_.limit_int_price_ = market_update_info_.bestbid_int_price_;
    best_bid_level_info_struct_.limit_price_ = market_update_info_.bestbid_price_;
    best_bid_level_info_struct_.limit_size_ = market_update_info_.bestbid_size_;
    best_bid_level_info_struct_.limit_ordercount_ = market_update_info_.bestbid_ordercount_;
  }
  void UpdateBestAskInfoStruct() {
    best_ask_level_info_struct_.limit_int_price_ = market_update_info_.bestask_int_price_;
    best_ask_level_info_struct_.limit_price_ = market_update_info_.bestask_price_;
    best_ask_level_info_struct_.limit_size_ = market_update_info_.bestask_size_;
    best_ask_level_info_struct_.limit_ordercount_ = market_update_info_.bestask_ordercount_;
  }

  void UpdateL1Prices();

  void SetTradeVarsForIndicatorsIfRequired();

  bool IsError() const;

  void SetProcessMarketDataBool(const bool pmd_) { process_market_data_ = pmd_; }
  bool GetProcessMarketDataBool() { return (process_market_data_); }

  void SanityCheckForBestAsk(int int_bid_price_);
  void SanityCheckForBestBid(int int_ask_price_);
  void SanityCheckOnTrade(int int_trade_price_);
  void SetSyntheticBestBid(const unsigned int t_bid_index_);
  void SetSyntheticBestAsk(const unsigned int t_bid_index_);
  void ComputeBestVarsFromSyntheticVars(const TradeType_t t_buysell_);
  void ComputeSyntheticVarsFromBaseVars(const unsigned int previous_best_index_, const TradeType_t t_buysell_);
  void SetSynVarsAsInvalid(const TradeType_t t_buysell_);
};

typedef std::vector<SecurityMarketView *> SecurityMarketViewPtrVec;

///< stored here as a static sid to smv* map so that Indicators can use this directly
static inline SecurityMarketViewPtrVec &sid_to_security_market_view_map() {
  static SecurityMarketViewPtrVec sid_to_security_market_view_map_;
  return sid_to_security_market_view_map_;
}
}
