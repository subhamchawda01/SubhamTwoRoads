/**
   \file MarketAdapterCode/security_market_view.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <typeinfo>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/simple_mempool.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "baseinfra/MarketAdapter/basic_market_view_structs.hpp"
#include "baseinfra/MarketAdapter/normal_spread_manager.hpp"
#include "baseinfra/MarketAdapter/smv_utils.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#define LOW_ACCESS_INDEX 50

namespace HFSAT {

SecurityMarketView::SecurityMarketView(DebugLogger &t_dbglogger_, const Watch &t_watch_,
                                       SecurityNameIndexer &t_sec_name_indexer_, const std::string &t_shortcode_,
                                       const char *t_exchange_symbol_, const unsigned int t_security_id_,
                                       const ExchSource_t t_exch_source_,
                                       bool t_temporary_bool_checking_if_this_is_an_indexed_book_,
                                       const std::string &t_offline_mix_mms_wts_filename_,
                                       const std::string &t_online_mix_price_const_filename_,
                                       const std::string &t_online_beta_kalman_const_filename_)
    : dbglogger_(t_dbglogger_),
      watch_(t_watch_),
      sec_name_indexer_(t_sec_name_indexer_),
      min_price_increment_(SecurityDefinitions::GetContractMinPriceIncrement(t_shortcode_, t_watch_.YYYYMMDD())),
      yield_min_price_increment_(0),
      min_order_size_(SecurityDefinitions::GetContractMinOrderSize(t_shortcode_, t_watch_.YYYYMMDD())),
      fast_price_convertor_(SecurityDefinitions::GetContractMinPriceIncrement(t_shortcode_, t_watch_.YYYYMMDD())),
      normal_spread_increments_(
          std::max(1.00, NormalSpreadManager::GetNormalSpreadIncrements(t_watch_.YYYYMMDD(), t_shortcode_))),
      normal_spread_(1),
      yield_normal_spread_(1),
      future_smv_(nullptr),
      option_(nullptr),
      computing_price_levels_(false),
      base_price_type_(kPriceTypeMidprice),
      trade_before_quote_(SecurityDefinitions::GetTradeBeforeQuote(t_shortcode_, t_watch_.YYYYMMDD())),
      is_ready_(false),
      l1_only_(false),
      offline_mix_mms_price_weights_(NUM_PRICETYPES_IN_OFFLINEMIXMMS,
                                     0.0),  // change the size from 5 if more weights/pricetypes are added to omix
      online_const_c(0.0),
      online_const_k(0.0),
      min_bid_size_to_consider_(0),
      min_ask_size_to_consider_(0),
      bid_side_valid_level_int_price_(0),
      ask_side_valid_level_int_price_(0),
      bid_side_valid_level_size_(0),
      ask_side_valid_level_size_(0),
      use_stable_bidask_levels_(false),
      band_lower_bid_int_price_(0),
      band_lower_ask_int_price_(0),
      bid_side_band_int_price_(0),
      ask_side_band_int_price_(0),
      bid_side_band_size_(0),
      ask_side_band_size_(0),
      num_non_empty_levels_in_band_(2u),
      compute_trade_prices_(true),
      market_update_info_(t_shortcode_, t_exchange_symbol_, t_security_id_,
                          SecurityDefinitions::GetContractExchSource(t_shortcode_, t_watch_.YYYYMMDD())),
      hybrid_market_update_info_(t_shortcode_, t_exchange_symbol_, t_security_id_,
                                 SecurityDefinitions::GetContractExchSource(t_shortcode_, t_watch_.YYYYMMDD())),
      trade_print_info_(),
      raw_trade_print_info_(),
      self_best_bid_ask_(),
      remove_self_orders_from_book_(false),
      smart_remove_self_orders_from_book_(false),  // Both false by default , have to be turned on explicitly.
      conf_to_market_update_msecs_(SecurityDefinitions::GetConfToMarketUpdateMsecs(t_shortcode_, t_watch_.YYYYMMDD())),

      dump_non_self_smv_on_next_update_(false),

      p_prom_order_manager_(nullptr),
      current_best_level_(),
      last_best_level_(),

      prev_bid_was_quote_(true),
      prev_ask_was_quote_(true),
      top_bid_level_to_mask_trades_on_(0),
      top_ask_level_to_mask_trades_on_(0),
      running_hit_size_vec_(),
      running_lift_size_vec_(),
      l1_changed_since_last_(false),
      l2_changed_since_last_(false),
      use_order_level_book_(false),
      last_message_was_trade_(false),
      market_orders_mempool_(nullptr),
      prob_modify_from_top_(-1),
      prob_modify_level_(90),
      min_priority_size_(10),
      max_priority_size_(500),
      l1_price_listeners_(),
      l1_size_listeners_(),
      l2_listeners_(),
      l2_only_listeners_(),
      onready_listeners_(),
      pl_change_listeners_(),
      pl_change_listeners_present_(false),
      p_smv_pretrade_listener_(nullptr),
      suspect_book_correct_(false),
      using_order_level_data_(false),
      skip_listener_notification_end_time_(0, 0),
      process_market_data_(true),
      initial_book_constructed_(false),
      int_price_bid_book_(),
      int_price_ask_book_(),
      int_price_bid_skip_delete_(),
      int_price_ask_skip_delete_(),
      // indexed_bid_book_ (),
      // indexed_ask_book_ (),
      base_bid_index_(0u),
      base_ask_index_(0u),
      base_bid_intprice_index_adjustment_(0),
      base_ask_intprice_index_adjustment_(0),
      last_base_bid_index_(0),
      last_base_ask_index_(0),
      this_int_price_(0),
      last_msg_was_quote_(false),
      running_lift_size_(0),
      running_hit_size_(0),
      lift_base_index_(0),
      hit_base_index_(0),
      initial_tick_size_(MIN_INITIAL_TICK_BASE),
      max_tick_range_(MIN_INITIAL_TICK_BASE),
      max_allowed_tick_range_(4 * MIN_INITIAL_TICK_BASE),
      this_smv_exch_source_(t_exch_source_),
      ors_exec_triggered_(false),
      last_exec_was_buy_(false),
      last_exec_was_sell_(false),
      last_ors_exec_triggered_msecs_(0),
      bestbid_size_before_ors_exec_triggered_(0),
      bestask_size_before_ors_exec_triggered_(0),
      order_sequence_to_timestamp_when_placed_(),
      order_sequence_to_size_when_placed_(),
      bid_access_bitmask_(0x0000),  // Initialize with Invalid L-mark
      ask_access_bitmask_(0x0000),
      bitmask_size_(sizeof(bid_access_bitmask_) * BYTE_SIZE),
      bid_level_change_bitmask_(0xFFFF),
      ask_level_change_bitmask_(0xFFFF),
      offline_mix_mms_wts_filename_(t_offline_mix_mms_wts_filename_),
      online_mix_price_const_filename_(t_online_mix_price_const_filename_),
      online_beta_kalman_const_filename_(t_online_beta_kalman_const_filename_),
      trade_basepx_struct_(trade_print_info_, market_update_info_, watch_,
                           HFSAT::SMVUtils::SMVUtils::GetTradeBasePriceParams(
                               "/spare/local/tradeinfo/param_trade_basepx.txt", t_shortcode_)),
      level_size_thresh_(0),
      current_market_status_(kMktTradingStatusOpen),
      best_bid_level_info_struct_(),
      best_ask_level_info_struct_() {
  best_bid_level_info_struct_.limit_int_price_level_ = 0;
  best_ask_level_info_struct_.limit_int_price_level_ = 0;
  hybrid_security_manager_ = new HybridSecurityManager(t_watch_.YYYYMMDD());
  if (hybrid_security_manager_->IsHybridSecurity(t_shortcode_)) {
    price_to_yield_map_ = hybrid_security_manager_->GetYieldsForHybridShortcode(t_shortcode_);
  }
  market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ =
      t_temporary_bool_checking_if_this_is_an_indexed_book_;
  for (PriceType_t _it_ = (PriceType_t)0; _it_ < kPriceTypeMax; _it_ = (PriceType_t)(_it_ + 1)) {
    price_type_subscribed_[_it_] = false;
  }

  normal_spread_ = normal_spread_increments_ * min_price_increment_;

  if (!price_to_yield_map_.empty()) {
    std::string act_shc_ = hybrid_security_manager_->GetActualSecurityFromHybrid(t_shortcode_);
    double last_act_price_ =
        SampleDataUtil::GetLastSampleBeforeDate(act_shc_, t_watch_.YYYYMMDD(), "AvgPrice300", 10, 0);
    if (last_act_price_ != 0) {
      double last_act_int_price_ = GetIntPx(last_act_price_);
      yield_min_price_increment_ =
          fabs(price_to_yield_map_[last_act_int_price_] - price_to_yield_map_[last_act_int_price_ - 1]);
    } else {
      yield_min_price_increment_ = min_price_increment_;
    }
    yield_normal_spread_ = normal_spread_increments_ * yield_min_price_increment_;
  }

  running_hit_size_vec_.resize(DEF_MARKET_DEPTH, 0);
  running_lift_size_vec_.resize(DEF_MARKET_DEPTH, 0);

  // default offline_mix_mms_price_weights_
  offline_mix_mms_price_weights_[kPriceTypeMidprice] = 0.00;
  offline_mix_mms_price_weights_[kPriceTypeMktSizeWPrice] = 1.00;
  offline_mix_mms_price_weights_[kPriceTypeMktSinusoidal] = 0.0;
  offline_mix_mms_price_weights_[kPriceTypeOrderWPrice] = 0.00;
  offline_mix_mms_price_weights_[kPriceTypeTradeWPrice] = 0.00;

  can_use_online_price_ = false;

  DBGLOG_TIME_CLASS_FUNC_LINE << " " << t_shortcode_ << " remove_self_orders_from_book_ = "
                              << (remove_self_orders_from_book_ ? "true" : "false")
                              << " smart_remove_self_orders_from_book_ = "
                              << (smart_remove_self_orders_from_book_ ? "true" : "false") << DBGLOG_ENDL_FLUSH;

  for (unsigned int lookup_counter_ = 0; lookup_counter_ < bitmask_size_; lookup_counter_++) {
    bitmask_lookup_table_[lookup_counter_] = pow(2, ((bitmask_size_ - 1) - lookup_counter_));
  }

  // SetUp Initial Point
  bitmask_access_lookup_table_[0] = 0x8000;  // This will be our starting point

  for (unsigned int lookup_counter_ = 1; lookup_counter_ < bitmask_size_; lookup_counter_++) {
    bitmask_access_lookup_table_[lookup_counter_] =
        (bitmask_access_lookup_table_[lookup_counter_ - 1]) + pow(2, (bitmask_size_ - 1) - lookup_counter_);
  }
  CheckComputeStablePrice();
  // clearing space and hybrid_security_manager not used due to ram issue
  delete hybrid_security_manager_;
}

void SecurityMarketView::CheckComputeStablePrice() {
  std::ifstream valid_sizes_file_;
  std::string valid_size_file_name_ = std::string(BASETRADEINFODIR) + "OfflineInfo/eq_stable_sizes.txt";
  if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Stable sizes file: " << valid_size_file_name_ << DBGLOG_ENDL_FLUSH;
  }
  valid_sizes_file_.open(valid_size_file_name_.c_str(), std::ifstream::in);
  if (valid_sizes_file_.is_open()) {
    const int kBufferSize = 1024;
    char readline_buffer_[kBufferSize];
    while (valid_sizes_file_.good()) {
      bzero(readline_buffer_, kBufferSize);
      valid_sizes_file_.getline(readline_buffer_, kBufferSize);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      PerishableStringTokenizer st_(readline_buffer_, kBufferSize);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() >= 3) {
        std::string this_shortcode_ = tokens_[0];
        if (market_update_info_.shortcode_.compare(this_shortcode_) == 0) {
          level_size_thresh_ = atoi(tokens_[1]);
          market_update_info_.compute_stable_levels_ = (atoi(tokens_[2]) != 0);
          if (!price_to_yield_map_.empty()) {
            hybrid_market_update_info_.compute_stable_levels_ = market_update_info_.compute_stable_levels_;
          }
          break;
        }
      }
    }
  }
  valid_sizes_file_.close();
}

void SecurityMarketView::subscribe_to_fast_trade_updates(FastTradeListener *t_fast_trade_listener) const {
  if (t_fast_trade_listener != NULL) {
    VectorUtils::UniqueVectorAdd(fast_trade_listeners_, t_fast_trade_listener);
  }
  return;
}

void SecurityMarketView::subscribe_tradeprints(SecurityMarketViewChangeListener *t_new_listener_) const {
  subscribe_price_type(t_new_listener_, kPriceTypeMidprice);  // listening to tradeprints means you must be interested
                                                              // in only top level price changes, not size changes
  // TODO .. make trade listeners separate so that listeners can be allowed to listen only OnTradePrint calls
}

void SecurityMarketView::subscribe_rawtradeprints(SecurityMarketViewRawTradesListener *t_new_listener_) const {
  if (t_new_listener_ != nullptr) {
    VectorUtils::UniqueVectorAdd(raw_trade_listeners_, t_new_listener_);
  }
}
bool SecurityMarketView::subscribe_price_type(SecurityMarketViewChangeListener *t_new_listener_,
                                              PriceType_t t_price_type_) const {
  bool is_successful_ = true;
  switch (t_price_type_) {
    case kPriceTypeMidprice: {
      ComputeMidPrice();

      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
      }
    } break;
    case kPriceTypeMktSizeWPrice: {
      ComputeMidPrice();  // if subscribing to mkt size weighted price then midprice needs to be computed as well
      ComputeMktPrice();

      if (t_new_listener_ != nullptr) {  // since mktpx is affected by both size and price of top level
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
    } break;
    case kPriceTypeProRataMktSizeWPrice: {
      ComputeMidPrice();  // if subscribing to pro rata mkt size weighted price then midprice needs to be computed as
                          // well
      ComputeMktPrice();  // if subscribing to pro rata mkt size weighted price then Mktprice needs to be computed as
                          // well
      ComputeProrataMktPrice();

      if (t_new_listener_ != nullptr) {  // since mktpx is affected by both size and price of top level
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
    } break;
    case kPriceTypeMktSinusoidal: {
      ComputeMidPrice();  // if subscribing to sinusoidal price then midprice needs to be computed as well
      ComputeMktSinPrice();

      if (t_new_listener_ != nullptr) {  // since mktsinpx is affected by both size and price of top level
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
    } break;
    case kPriceTypeOrderWPrice: {
      ComputeMidPrice();  // if subscribing to sinusoidal price then midprice needs to be computed as well
      ComputeOrderWPrice();

      if (t_new_listener_ != nullptr) {  // since mktsinpx is affected by both size and price of top level
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
    } break;
    case kPriceTypeTradeWPrice: {
      ComputeMidPrice();
      ComputeTradeWPrice();

      if (t_new_listener_ != nullptr) {  // since tradepx is affected by OnTradePrint only
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
      }

      price_type_subscribed_[kPriceTypeTradeWPrice] = true;
    } break;
    case kPriceTypeTradeBasePrice: {
      ComputeMidPrice();
      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
      }
      price_type_subscribed_[kPriceTypeTradeBasePrice] = true;
    } break;
    case kPriceTypeTradeMktSizeWPrice: {
      ComputeMidPrice();
      ComputeMktPrice();
      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
      price_type_subscribed_[kPriceTypeTradeMktSizeWPrice] = true;
      price_type_subscribed_[kPriceTypeMktSizeWPrice] = true;
    } break;

    case kPriceTypeTradeMktSinPrice: {
      ComputeMidPrice();
      ComputeMktPrice();
      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
      price_type_subscribed_[kPriceTypeMktSinusoidal] = true;
      price_type_subscribed_[kPriceTypeTradeMktSinPrice] = true;
    } break;
    case kPriceTypeTradeOrderWPrice: {
      ComputeMidPrice();
      ComputeOrderWPrice();
      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
      price_type_subscribed_[kPriceTypeOrderWPrice] = true;
      price_type_subscribed_[kPriceTypeTradeOrderWPrice] = true;
    } break;
    case kPriceTypeTradeTradeWPrice: {
      ComputeMidPrice();
      ComputeTradeWPrice();
      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
      }
      price_type_subscribed_[kPriceTypeTradeWPrice] = true;
      price_type_subscribed_[kPriceTypeTradeTradeWPrice] = true;
    } break;
    case kPriceTypeTradeOmixPrice: {
      if (!price_type_subscribed_[t_price_type_]) {
        SetupOfflineMixMMS();
        ComputeMidPrice();
        ComputeMktPrice();
        ComputeMktSinPrice();
        ComputeOrderWPrice();
        ComputeTradeWPrice();
        price_type_subscribed_[kPriceTypeOfflineMixMMS] = true;
        price_type_subscribed_[t_price_type_] = true;
      }
      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
    } break;
    case kPriceTypeOfflineMixMMS: {
      if (!price_type_subscribed_[t_price_type_]) {  // if subscribing to offline mix of all four prices then
        // look up the data files and setup the constants for this computation
        // and also subscribe to all four price types

        SetupOfflineMixMMS();

        // TODO_OPT
        // Strictly speaking , don't need to subscribe to all 4 types.
        // Only to types with non-zero weights.
        ComputeMidPrice();
        ComputeMktPrice();
        ComputeMktSinPrice();
        ComputeOrderWPrice();
        ComputeTradeWPrice();

        price_type_subscribed_[t_price_type_] = true;
      }

      if (t_new_listener_ != nullptr) {  // since mktpx,mktsinpx is affected by both size and price of top level
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
    } break;
    case kPriceTypeValidLevelMidPrice: {
      if (!price_type_subscribed_[t_price_type_]) {
        SetupValidLevelSizes();
        ComputeValidLevelMidPrice();
        price_type_subscribed_[kPriceTypeValidLevelMidPrice] = true;
      }

      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }

    } break;
    case kPriceTypeBandPrice: {
      if (!price_type_subscribed_[t_price_type_]) {
        SetupBands();
        ComputeBandPrice();
        price_type_subscribed_[kPriceTypeBandPrice] = true;
      }
      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }

    } break;
    case kPriceTypeBidPrice: {  // copying same as midprice
      ComputeMidPrice();

      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);  // TODO ... add additional checks to discern
                                                                            // if the prices have changed ... and then
                                                                            // remove this
      }
    } break;
    case kPriceTypeAskPrice: {  // copying same as midprice
      ComputeMidPrice();

      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);  // TODO ... add additional checks to discern
                                                                            // if the prices have changed ... and then
                                                                            // remove this
      }
    } break;

    case kPriceTypeStableBidPrice: {
      if (!price_type_subscribed_[t_price_type_]) {
        SetupValidLevelSizes();
        ComputeStableBidPrice();
        price_type_subscribed_[kPriceTypeStableBidPrice] = true;
      }

      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }

    } break;

    case kPriceTypeStableAskPrice: {
      if (!price_type_subscribed_[t_price_type_]) {
        SetupValidLevelSizes();
        ComputeStableBidPrice();
        price_type_subscribed_[kPriceTypeStableAskPrice] = true;
      }

      if (t_new_listener_ != nullptr) {
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
    } break;

    case kPriceTypeOrderSizeWPrice: {
      ComputeMidPrice();  // just incase spd > 1
      ComputeOrderSizeWPrice();

      if (t_new_listener_ != nullptr) {  // affected by both size and price of top level
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
    } break;
    case kPriceTypeOnlineMixPrice: {
      if (!price_type_subscribed_[t_price_type_]) {
        SetupOnlinePriceConstants();
      }
      ComputeMidPrice();
      ComputeOrderWPrice();
      ComputeMktPrice();
      ComputeOnlineMixPrice();
      if (t_new_listener_ != nullptr) {  // affected by both size and price of top level
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
    } break;
    case kPriceTypeImpliedVol: {
      if (!price_type_subscribed_[t_price_type_]) {
        // Initialize the future SMV and Option object
        SetupImpliedVolVariables();
      }
      ComputeMidPrice();
      ComputeImpliedVol();
      if (t_new_listener_ != nullptr) {  // affected by both size and price of top level
        VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
        // VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
      }
    } break;
    default:
      std::cerr << " SecurityMarketView::subscribe_price_type called with invalid argument!! " << t_price_type_ << "\n";
      is_successful_ = false;
      break;
  }

  return is_successful_;
}

void SecurityMarketView::subscribe_SpreadTrades(SecurityMarketViewChangeListener *t_new_listener_) {
  VectorUtils::UniqueVectorAdd(spread_trades_listeners_, t_new_listener_);
}

void SecurityMarketView::subscribe_L2(
    SecurityMarketViewChangeListener *
        t_new_listener_) {  // an l2-listner is interested in price / size updates of top and non top levels
  if (t_new_listener_ != nullptr) {
    VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
    VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
    VectorUtils::UniqueVectorAdd(l2_listeners_, t_new_listener_);
  }
}

void SecurityMarketView::subscribe_L1_Only(SecurityMarketViewChangeListener *t_new_listener_) {
  if (t_new_listener_ != nullptr) {
    VectorUtils::UniqueVectorAdd(l1_price_listeners_, t_new_listener_);
    VectorUtils::UniqueVectorAdd(l1_size_listeners_, t_new_listener_);
  }
}

void SecurityMarketView::subscribe_L2_Only(SecurityMarketViewL2ChangeListener *t_new_listener_) {
  if (t_new_listener_ != nullptr) {
    VectorUtils::UniqueVectorAdd(l2_only_listeners_, t_new_listener_);
  }
}

void SecurityMarketView::subscribe_OnReady(SecurityMarketViewOnReadyListener *t_new_listener_) const {
  if (t_new_listener_ != nullptr) {
    VectorUtils::UniqueVectorAdd(onready_listeners_, t_new_listener_);
  }
}

void SecurityMarketView::subscribe_PreTrades(SMVPreTradeListener *t_new_listener_) {
  if (t_new_listener_ != nullptr) {
    VectorUtils::UniqueVectorAdd(pre_trade_listeners_, t_new_listener_);
  }
}

void SecurityMarketView::subscribe_MktStatus(SecurityMarketViewStatusListener *t_new_listener_) const {
  if (t_new_listener_ != nullptr) {
    VectorUtils::UniqueVectorAdd(market_status_listeners_, t_new_listener_);
  }
}

void SecurityMarketView::subscribe_OffMktTrades(SMVOffMarketTradeListener *t_new_listener_) const {
  if (t_new_listener_ != nullptr) {
    VectorUtils::UniqueVectorAdd(off_market_trade_listeners_, t_new_listener_);
  }
}

// what all this unfortunate class is handling !
void SecurityMarketView::DumpStatus() {
  // misc
  if (future_smv_ != nullptr) {
    std::cerr << "Future pointer is set and computing mid price " << future_smv_->shortcode() << "\n";
  }
  if (option_ != nullptr) {
    std::cerr << "Option object is set " << option_->shortcode() << "\n";
  }

  // prices subscribed
  std::cerr << "Subcribed to following prices:";
  for (auto i = 0u; i < kPriceTypeMax; i++) {
    if (price_type_subscribed_[i]) {
      std::cerr << " " << PriceTypeStrings[i];
    }
  }
  std::cerr << "\n";
  if (computing_price_levels_) {
    std::cerr << "Computing Price Levels, not sure what this means\n";
  }
  std::cerr << "BasePrice is set to " << price_type_subscribed_[base_price_type_]
            << ", although this doesnt do anything\n";
  if (trade_before_quote_) {
    std::cerr << "Trade Before Quote is enabled\n";
  }

  if (is_ready_) {
    std::cerr << "IsReady is set to true\n";
  } else {
    std::cerr << "IsReady is set to false\n";
  }

  if (l1_only_) {
    std::cerr << "L1 only set to true";
  }

  // not interested in other

  // listeners
  std::cerr << " RawTradeListeners: " << raw_trade_listeners_.size() << " L1Listeners: " << l1_price_listeners_.size()
            << " L1SizeListeners: " << l1_size_listeners_.size() << " L2Listeners: " << l2_listeners_.size()
            << " L2OnlyListeners: " << l2_only_listeners_.size() << " OnreadyListeners: " << onready_listeners_.size()
            << " PLChangelisteners: " << pl_change_listeners_.size()
            << " OTCTradeListeners: " << otc_trades_listeners_.size()
            << " SpreadTradeListeners: " << spread_trades_listeners_.size()
            << " PreTradeListeners: " << pre_trade_listeners_.size()
            << " MarketStatusListeners: " << market_status_listeners_.size()
            << " OffMarketTradeListeners: " << off_market_trade_listeners_.size() << "\n";

  // what are all getting computed
  // current state
}

void SecurityMarketView::SetupOnlinePriceConstants() const {
  std::ifstream t_online_mix_price_infile_;
  std::string t_online_mix_price_const_filename_ = online_mix_price_const_filename_;
  if (!FileUtils::exists(t_online_mix_price_const_filename_)) {
    return;
  }
  t_online_mix_price_infile_.open(t_online_mix_price_const_filename_.c_str(), std::ifstream::in);
  if (t_online_mix_price_infile_.is_open()) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Using online-mix file " << t_online_mix_price_const_filename_ << DBGLOG_ENDL_FLUSH;

    const int kONLINEMIXLineBufferLen = 1024;
    char readline_buffer_[kONLINEMIXLineBufferLen];
    bzero(readline_buffer_, kONLINEMIXLineBufferLen);

    while (t_online_mix_price_infile_.good()) {
      bzero(readline_buffer_, kONLINEMIXLineBufferLen);
      t_online_mix_price_infile_.getline(readline_buffer_, kONLINEMIXLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kONLINEMIXLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() >= NUM_CONST_IN_ONLINEMIXPRICE + 1) {  // TOPIX_0 const1 const2
        std::string dep_indep_portfolio_code_ = tokens_[0];
        if (market_update_info_.shortcode_.compare(dep_indep_portfolio_code_) == 0) {
          online_const_c = std::max(0.0, std::min(1.0, atof(tokens_[1])));  // c [0,1]
          online_const_k = std::max(0.0, atof(tokens_[2]));                 // k > 0
          can_use_online_price_ = true;
          break;
        }
      }
    }
    t_online_mix_price_infile_.close();
  }
}

void SecurityMarketView::SetupOfflineMixMMS() const {
  std::ifstream t_offline_mix_mms_infile_;
  std::string t_offline_mix_mms_wts_filename_ = offline_mix_mms_wts_filename_;

  if (!FileUtils::exists(t_offline_mix_mms_wts_filename_)) {  // All attempts failed.
    DBGLOG_TIME_CLASS_FUNC_LINE << " Can't find omix file " << t_offline_mix_mms_wts_filename_ << DBGLOG_ENDL_FLUSH;
    t_offline_mix_mms_wts_filename_ = std::string(DEFAULT_OFFLINEMIXMMS_FILE);
  }

  t_offline_mix_mms_infile_.open(t_offline_mix_mms_wts_filename_.c_str(), std::ifstream::in);

  bool wts_found_ = false;

  if (t_offline_mix_mms_infile_.is_open()) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " Using offline-mix file " << t_offline_mix_mms_wts_filename_ << DBGLOG_ENDL_FLUSH;

    // std::cerr << " Using offline-mix file " << t_offline_mix_mms_wts_filename_ << std::endl;

    const int kOFFLINEMIXLineBufferLen = 1024;
    char readline_buffer_[kOFFLINEMIXLineBufferLen];
    bzero(readline_buffer_, kOFFLINEMIXLineBufferLen);

    while (t_offline_mix_mms_infile_.good()) {
      bzero(readline_buffer_, kOFFLINEMIXLineBufferLen);
      t_offline_mix_mms_infile_.getline(readline_buffer_, kOFFLINEMIXLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kOFFLINEMIXLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();

      if (tokens_.size() >= NUM_PRICETYPES_IN_OFFLINEMIXMMS + 1) {  // ZN_0 wt1....
        std::string dep_indep_portfolio_code_ = tokens_[0];
        if (market_update_info_.shortcode_.compare(dep_indep_portfolio_code_) == 0) {
          offline_mix_mms_price_weights_[kPriceTypeMidprice] = atof(tokens_[1]);
          offline_mix_mms_price_weights_[kPriceTypeMktSizeWPrice] = atof(tokens_[2]);
          offline_mix_mms_price_weights_[kPriceTypeMktSinusoidal] = atof(tokens_[3]);
          offline_mix_mms_price_weights_[kPriceTypeOrderWPrice] = atof(tokens_[4]);
          offline_mix_mms_price_weights_[kPriceTypeTradeWPrice] = atof(tokens_[5]);
          wts_found_ = true;
        }
        // does not break hence it keeps comparing
      }
    }
    t_offline_mix_mms_infile_.close();
  }

  if (!wts_found_) {
    std::ostringstream temp_oss_;
    temp_oss_ << "Can't find omixwts for " << shortcode() << " in file " << t_offline_mix_mms_wts_filename_ << "\n";
    // ExitVerbose(kOmixWtsNotFound, temp_oss_.str().c_str()); //not sure if we wan't to exit here.

    DBGLOG_TIME_CLASS_FUNC_LINE << temp_oss_.str() << " Using default offline-mix weights " << DBGLOG_ENDL_FLUSH;
  }
}

void SecurityMarketView::SetupValidLevelSizes() const {
  std::ifstream valid_sizes_file_;
  std::string valid_size_file_name_ = std::string(BASETRADEINFODIR) + "OfflineInfo/valid_sizes.txt";
  DBGLOG_TIME_CLASS_FUNC_LINE << " Stable sizes file: " << valid_size_file_name_ << DBGLOG_ENDL_FLUSH;
  valid_sizes_file_.open(valid_size_file_name_.c_str(), std::ifstream::in);
  if (valid_sizes_file_.is_open()) {
    const int kBufferSize = 1024;
    char readline_buffer_[kBufferSize];
    while (valid_sizes_file_.good()) {
      bzero(readline_buffer_, kBufferSize);
      valid_sizes_file_.getline(readline_buffer_, kBufferSize);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      PerishableStringTokenizer st_(readline_buffer_, kBufferSize);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() >= 3) {
        std::string this_shortcode_ = tokens_[0];
        if (market_update_info_.shortcode_.compare(this_shortcode_) == 0) {
          min_bid_size_to_consider_ = atoi(tokens_[1]);
          min_ask_size_to_consider_ = atoi(tokens_[2]);
          break;
        }
      }
    }
  }
  valid_sizes_file_.close();
}

void SecurityMarketView::SetupBands() const {
  // Read number of levels to take as band on each side of book
  std::ifstream band_file;
  std::string band_filename = std::string(BASETRADEINFODIR) + "OfflineInfo/band_levels.txt";
  DBGLOG_TIME_CLASS_FUNC_LINE << " BandSizesFile: " << band_filename << DBGLOG_ENDL_FLUSH;
  band_file.open(band_filename.c_str(), std::ifstream::in);
  if (band_file.is_open()) {
    const int kBufferSize = 1024;
    char readline_buffer_[kBufferSize];
    while (band_file.good()) {
      bzero(readline_buffer_, kBufferSize);
      band_file.getline(readline_buffer_, kBufferSize);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      PerishableStringTokenizer st_(readline_buffer_, kBufferSize);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() >= 2) {
        std::string this_shortcode_ = tokens_[0];
        if (market_update_info_.shortcode_.compare(this_shortcode_) == 0) {
          num_non_empty_levels_in_band_ = atoi(tokens_[1]);
          DBGLOG_TIME_CLASS_FUNC_LINE << " Prod: " << this_shortcode_ << " num_level: " << num_non_empty_levels_in_band_
                                      << DBGLOG_ENDL_FLUSH;
          break;
        }
      }
    }
  }
}

void SecurityMarketView::SetupImpliedVolVariables() const {
  option_ = OptionObject::GetUniqueInstance(dbglogger_, watch_, shortcode());
}

// handling of book reset for order book
void SecurityMarketView::EmptyBook() {
  if (!market_update_info_.bidlevels_.empty()) market_update_info_.bidlevels_.clear();
  if (!market_update_info_.asklevels_.empty()) market_update_info_.asklevels_.clear();
}

void SecurityMarketView::Sanitize() {
  if (!market_update_info_.bidlevels_.empty()) {
    for (auto i = 0u; (i + 1) < market_update_info_.bidlevels_.size();) {
      if (market_update_info_.bidlevels_[i + 1].limit_int_price_ >=
          market_update_info_.bidlevels_[i].limit_int_price_) {
        // dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << "Check Sanitize \n";
        // dbglogger_.CheckToFlushBuffer( );
        if (market_update_info_.bidlevels_[i + 1].mod_time_ < market_update_info_.bidlevels_[i].mod_time_) {
          if (pl_change_listeners_present_) {
            int old_size_at_this_price = market_update_info_.bidlevels_[i + 1].limit_size_;
            int old_ordercount_at_this_price = market_update_info_.bidlevels_[i + 1].limit_ordercount_;
            NotifyOnPLChangeListeners(market_update_info_.security_id_, market_update_info_, kTradeTypeBuy, i + 1,
                                      market_update_info_.bidlevels_[i + 1].limit_int_price_,
                                      market_update_info_.bidlevels_[i + 1].limit_int_price_level_,
                                      old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                      false,  // t_is_intermediate_message_ ,
                                      'D');
          }
          EraseBid(i + 1);
        } else {
          if (pl_change_listeners_present_) {
            int old_size_at_this_price = market_update_info_.bidlevels_[i].limit_size_;
            int old_ordercount_at_this_price = market_update_info_.bidlevels_[i].limit_ordercount_;
            NotifyOnPLChangeListeners(market_update_info_.security_id_, market_update_info_, kTradeTypeBuy, i,
                                      market_update_info_.bidlevels_[i].limit_int_price_,
                                      market_update_info_.bidlevels_[i].limit_int_price_level_, old_size_at_this_price,
                                      0, old_ordercount_at_this_price, 0,
                                      false,  // t_is_intermediate_message_ ,
                                      'D');
          }

          EraseBid(i);
        }
      } else {
        i++;
      }
    }
  }

  if (!market_update_info_.asklevels_.empty()) {
    for (auto i = 0u; i < market_update_info_.asklevels_.size() - 1;) {
      if (market_update_info_.asklevels_[i + 1].limit_price_ <= market_update_info_.asklevels_[i].limit_price_) {
        // dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << "Check Sanitize \n";
        // dbglogger_.CheckToFlushBuffer( );
        if (market_update_info_.asklevels_[i + 1].mod_time_ < market_update_info_.asklevels_[i].mod_time_) {
          if (pl_change_listeners_present_) {
            int old_size_at_this_price = market_update_info_.asklevels_[i + 1].limit_size_;
            int old_ordercount_at_this_price = market_update_info_.asklevels_[i + 1].limit_ordercount_;
            NotifyOnPLChangeListeners(market_update_info_.security_id_, market_update_info_, kTradeTypeSell, i + 1,
                                      market_update_info_.asklevels_[i + 1].limit_int_price_,
                                      market_update_info_.asklevels_[i + 1].limit_int_price_level_,
                                      old_size_at_this_price, 0, old_ordercount_at_this_price, 0,
                                      true,  // t_is_intermediate_message_ ,
                                      'D');
          }
          EraseAsk(i + 1);
        } else {
          if (pl_change_listeners_present_) {
            int old_size_at_this_price = market_update_info_.asklevels_[i].limit_size_;
            int old_ordercount_at_this_price = market_update_info_.asklevels_[i].limit_ordercount_;
            NotifyOnPLChangeListeners(market_update_info_.security_id_, market_update_info_, kTradeTypeSell, i,
                                      market_update_info_.asklevels_[i].limit_int_price_,
                                      market_update_info_.asklevels_[i].limit_int_price_level_, old_size_at_this_price,
                                      0, old_ordercount_at_this_price, 0,
                                      true,  // t_is_intermediate_message_ ,
                                      'D');
          }

          EraseAsk(i);
        }
      } else {
        i++;
      }
    }
  }
}

void SecurityMarketView::SanitizeBidSideWithLevel(int level_added) {
  if (market_update_info_.bidlevels_.empty() || level_added > (int)market_update_info_.bidlevels_.size()) return;

  if (!market_update_info_.bidlevels_.empty()) {
    for (unsigned int i = level_added - 1; (i + 1) < market_update_info_.bidlevels_.size();) {
      if (market_update_info_.bidlevels_[i + 1].limit_int_price_ >=
          market_update_info_.bidlevels_[i].limit_int_price_) {
        // dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << " Bid Sanitizing \n";
        // dbglogger_ << " : " << market_update_info_.bidlevels_ [i + 1].limit_int_price_ <<
        // market_update_info_.bidlevels_ [i + 1].limit_size_ <<"\n";
        // dbglogger_.CheckToFlushBuffer( );
        //                    if ( market_update_info_.bidlevels_[ i + 1 ].mod_time_ < market_update_info_.bidlevels_[
        //                    i
        //                    ].mod_time_ )
        //      {
        EraseBid(i + 1);
        //      }
        // else
        //      {
        //
        //        EraseBid( i );
        //      }
      } else {
        i++;
      }
    }
  }
}

void SecurityMarketView::SanitizeAskSideWithLevel(int level_added) {
  if (market_update_info_.asklevels_.empty() || level_added > (int)market_update_info_.asklevels_.size()) return;

  if (!market_update_info_.asklevels_.empty()) {
    for (unsigned int i = level_added - 1; i < market_update_info_.asklevels_.size() - 1;) {
      if (market_update_info_.asklevels_[i + 1].limit_price_ <= market_update_info_.asklevels_[i].limit_price_) {
        // dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << " Ask Sanitizing \n";
        // dbglogger_ << " : " << market_update_info_.bidlevels_ [i + 1].limit_int_price_ <<
        // market_update_info_.bidlevels_ [i + 1].limit_size_ <<"\n";
        // dbglogger_.CheckToFlushBuffer( );

        // dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << "Check Sanitize \n";
        // dbglogger_.CheckToFlushBuffer( );
        // if ( market_update_info_.asklevels_[ i + 1 ].mod_time_ < market_update_info_.asklevels_[ i ].mod_time_ )
        //   {
        EraseAsk(i + 1);
        //   }
        // else
        //   {
        //     EraseAsk( i );
        //   }
      } else {
        i++;
      }
    }
  }
}

// Sanitize all the bid levels which are greater than this int_price
void SecurityMarketView::SanitizeBidSideWithPrice(int this_int_price) {
  if (market_update_info_.bidlevels_.empty()) return;

  // Nothing to do all ready in order
  if (market_update_info_.asklevels_[0].limit_int_price_ > this_int_price) return;

  int size_bid_levels_ = market_update_info_.bidlevels_.size();
  int last_bid_int_price_ = market_update_info_.bidlevels_[size_bid_levels_ - 1].limit_int_price_;

  // If the last price on the bid level is higher than the first level of the ask side
  // clear whole bid levels
  if (last_bid_int_price_ >= this_int_price) {
    //        dbglogger_<< " Clearing all bid levels : " << "\n"; dbglogger_.DumpCurrentBuffer ();
    market_update_info_.bidlevels_.clear();
    return;
  }

  for (auto i = 0u; (i + 1) < market_update_info_.bidlevels_.size();) {
    if (market_update_info_.bidlevels_[i].limit_int_price_ >= this_int_price) {
      // dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << " Bid Sanitizing REmove
      // Price
      // Level\n";
      // dbglogger_ << " : " << market_update_info_.bidlevels_ [i ].limit_int_price_ << market_update_info_.bidlevels_
      // [i].limit_size_ <<"\n";
      // dbglogger_.CheckToFlushBuffer( );
      //                if ( market_update_info_.bidlevels_[ i + 1 ].mod_time_ < market_update_info_.bidlevels_[ i
      //                ].mod_time_ )
      //  {
      EraseBid(i);
      //  }
      // else
      //  {
      //
      //    EraseBid( i );
      //  }
    } else {
      i++;
    }
  }
}

void SecurityMarketView::SanitizeAskSideWithPrice(int this_int_price) {
  if (market_update_info_.asklevels_.empty()) return;

  // Nothing to do all ready in order
  if (market_update_info_.asklevels_[0].limit_int_price_ > this_int_price) return;

  int size_ask_levels_ = market_update_info_.asklevels_.size();
  int last_ask_int_price_ = market_update_info_.asklevels_[size_ask_levels_ - 1].limit_int_price_;

  if (last_ask_int_price_ <= this_int_price) {
    //        dbglogger_<< " Clearing all ask levels : " << "\n"; dbglogger_.DumpCurrentBuffer ();
    market_update_info_.asklevels_.clear();
    return;
  }

  for (auto i = 0u; (i + 1) < market_update_info_.asklevels_.size();) {
    if (market_update_info_.asklevels_[i].limit_int_price_ <= this_int_price) {
      // dbglogger_ << watch_.tv() << ' ' << typeid( *this ).name() << ':' << __func__ << " Ask Sanitizing REmove
      // Price
      // Level\n";
      // dbglogger_ << " : " << market_update_info_.asklevels_ [i ].limit_int_price_ << market_update_info_.asklevels_
      // [i].limit_size_ <<"\n";
      // dbglogger_.CheckToFlushBuffer( );
      //                if ( market_update_info_.asklevels_[ i + 1 ].mod_time_ < market_update_info_.asklevels_[ i
      //                ].mod_time_ )
      //  {
      EraseAsk(i);
      //  }
      // else
      //  {
      //
      //    EraseAsk( i );
      //  }
    } else {
      i++;
    }
  }
}

void SecurityMarketView::EraseBid(int level) {
  if (level == 0) {
    if (computing_price_levels()) {
      int to_deduct =
          market_update_info_.bidlevels_[0].limit_int_price_ - market_update_info_.bidlevels_[1].limit_int_price_;
      for (unsigned int i = 1; i < market_update_info_.bidlevels_.size(); i++) {
        market_update_info_.bidlevels_[i].limit_int_price_level_ -= to_deduct;
      }
    }
    market_update_info_.bidlevels_.erase(market_update_info_.bidlevels_.begin());
    if (use_order_level_book_ == true) {
      market_update_info_.bid_level_order_depth_book_.erase(market_update_info_.bid_level_order_depth_book_.begin());
    }

    // Bug fix - Top two levels get deleted if level == 0.
    return;
  }

  std::vector<MarketUpdateInfoLevelStruct>::iterator liter = market_update_info_.bidlevels_.begin();
  for (int i = 0; i < level && liter != market_update_info_.bidlevels_.end(); i++, liter++)
    ;

  if (liter != market_update_info_.bidlevels_.begin() && liter != market_update_info_.bidlevels_.end()) {
    market_update_info_.bidlevels_.erase(liter);
  }

  if (use_order_level_book_ == true) {
    std::vector<std::vector<MarketOrder *> >::iterator liter2 = market_update_info_.bid_level_order_depth_book_.begin();
    for (int i = 0; i < level && liter2 != market_update_info_.bid_level_order_depth_book_.end(); i++, liter2++)
      ;

    if (liter2 != market_update_info_.bid_level_order_depth_book_.begin() &&
        liter2 != market_update_info_.bid_level_order_depth_book_.end()) {
      market_update_info_.bid_level_order_depth_book_.erase(liter2);
    }
  }
}

void SecurityMarketView::EraseAsk(int level) {
  if (level == 0) {
    if (computing_price_levels()) {
      int to_deduct =
          market_update_info_.asklevels_[1].limit_int_price_ - market_update_info_.asklevels_[0].limit_int_price_;
      for (unsigned int i = 1; i < market_update_info_.asklevels_.size(); i++) {
        market_update_info_.asklevels_[i].limit_int_price_level_ -= to_deduct;
      }
    }
    market_update_info_.asklevels_.erase(market_update_info_.asklevels_.begin());
    if (use_order_level_book_ == true) {
      market_update_info_.ask_level_order_depth_book_.erase(market_update_info_.ask_level_order_depth_book_.begin());
    }

    // Bug fix - Top two levels get deleted if level == 0.
    return;
  }

  std::vector<MarketUpdateInfoLevelStruct>::iterator liter = market_update_info_.asklevels_.begin();
  for (int i = 0; i < level && liter != market_update_info_.asklevels_.end(); i++, liter++)
    ;

  if (liter != market_update_info_.asklevels_.begin() && liter != market_update_info_.asklevels_.end()) {
    market_update_info_.asklevels_.erase(liter);
  }

  if (use_order_level_book_ == true) {
    std::vector<std::vector<MarketOrder *> >::iterator liter2 = market_update_info_.ask_level_order_depth_book_.begin();
    for (int i = 0; i < level && liter2 != market_update_info_.ask_level_order_depth_book_.end(); i++, liter2++)
      ;

    if (liter2 != market_update_info_.ask_level_order_depth_book_.begin() &&
        liter2 != market_update_info_.ask_level_order_depth_book_.end()) {
      market_update_info_.ask_level_order_depth_book_.erase(liter2);
    }
  }
}

inline void SecurityMarketView::OnL1PriceUpdate() {
  Sanitize();
  if (IsBidBookEmpty() || IsAskBookEmpty()) return;
  UpdateL1Prices();

  NotifyL1PriceListeners();
  NotifyOnReadyListeners();
}

void SecurityMarketView::OnL1SizeUpdate() {}

void SecurityMarketView::OnL2Update() {
  // TODO{}  check the empty with something clever

  if (IsBidBookEmpty() || IsAskBookEmpty()) return;
  // only L2 change so no l1 prices

  NotifyL2Listeners();
  //  NotifyOnReadyListeners ( );
}

void SecurityMarketView::OnL2OnlyUpdate() {
  if (IsBidBookEmpty() || IsAskBookEmpty()) return;
  NotifyL2OnlyListeners();
}

void SecurityMarketView::RecomputeOfflineMixPrice() {
  market_update_info_.offline_mix_mms_price_ =
      (market_update_info_.mid_price_ * offline_mix_mms_price_weights_[kPriceTypeMidprice]) +
      (market_update_info_.mkt_size_weighted_price_ * offline_mix_mms_price_weights_[kPriceTypeMktSizeWPrice]) +
      (market_update_info_.mkt_sinusoidal_price_ * offline_mix_mms_price_weights_[kPriceTypeMktSinusoidal]) +
      (market_update_info_.order_weighted_price_ * offline_mix_mms_price_weights_[kPriceTypeOrderWPrice]) +
      (market_update_info_.trade_weighted_price_ * offline_mix_mms_price_weights_[kPriceTypeTradeWPrice]);
  hybrid_market_update_info_.offline_mix_mms_price_ =
      (hybrid_market_update_info_.mid_price_ * offline_mix_mms_price_weights_[kPriceTypeMidprice]) +
      (hybrid_market_update_info_.mkt_size_weighted_price_ * offline_mix_mms_price_weights_[kPriceTypeMktSizeWPrice]) +
      (hybrid_market_update_info_.mkt_sinusoidal_price_ * offline_mix_mms_price_weights_[kPriceTypeMktSinusoidal]) +
      (hybrid_market_update_info_.order_weighted_price_ * offline_mix_mms_price_weights_[kPriceTypeOrderWPrice]) +
      (hybrid_market_update_info_.trade_weighted_price_ * offline_mix_mms_price_weights_[kPriceTypeTradeWPrice]);
}

void SecurityMarketView::RecomputeOnlineMixPrice() {
  if (can_use_online_price_ && market_update_info_.bestask_ordercount_ != 0 &&
      market_update_info_.bestbid_ordercount_ != 0) {
    double ra = market_update_info_.bestask_size_ / double(market_update_info_.bestask_ordercount_);
    double rb = market_update_info_.bestbid_size_ / double(market_update_info_.bestbid_ordercount_);
    double r = std::max(ra, rb) / std::min(ra, rb);
    double a = online_const_c * (1 - std::exp(online_const_k * (1 - r)));
    market_update_info_.online_mix_price_ =
        a * market_update_info_.order_weighted_price_ + (1 - a) * market_update_info_.mkt_size_weighted_price_;
    hybrid_market_update_info_.online_mix_price_ = a * hybrid_market_update_info_.order_weighted_price_ +
                                                   (1 - a) * hybrid_market_update_info_.mkt_size_weighted_price_;
  } else {
    market_update_info_.online_mix_price_ = market_update_info_.mkt_size_weighted_price_;
    hybrid_market_update_info_.online_mix_price_ = hybrid_market_update_info_.mkt_size_weighted_price_;
  }
}

void SecurityMarketView::RecomputeValidLevelPrices() {
  int this_bid_int_price = market_update_info_.bestbid_int_price_;
  int this_ask_int_price = market_update_info_.bestask_int_price_;
  int64_t sum_bid_size = 0;  // bid_size_at_int_price(this_bid_int_price_);
  int64_t sum_ask_size = 0;  // ask_size_at_int_price(this_ask_int_price_);

  double sum_ask_px_size = 0.0;
  double sum_bid_px_size = 0.0;

  int ask_index = GetAskIndex(this_ask_int_price);
  int bid_index = GetBidIndex(this_bid_int_price);

  unsigned int count = 0;
  while (sum_bid_size < min_bid_size_to_consider_ && (count < MIN_PRICE_LEVELS_TO_ITERATE)) {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      bid_index = GetNextBidMapIndex(bid_index);
      sum_bid_size += market_update_info_.bidlevels_[bid_index].limit_size_;
      sum_bid_px_size += (market_update_info_.bidlevels_[bid_index].limit_size_ *
                          market_update_info_.bidlevels_[bid_index].limit_price_);
    } else if (count < market_update_info_.bidlevels_.size()) {
      sum_bid_size += market_update_info_.bidlevels_[count].limit_size_;
      sum_bid_px_size +=
          (market_update_info_.bidlevels_[count].limit_size_ * market_update_info_.bidlevels_[count].limit_price_);
    }
    count++;
  }

  count = 0;
  while (sum_ask_size < min_ask_size_to_consider_ && (count < MIN_PRICE_LEVELS_TO_ITERATE)) {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      ask_index = GetNextAskMapIndex(ask_index);
      sum_ask_size += market_update_info_.asklevels_[ask_index].limit_size_;
      sum_ask_px_size += (market_update_info_.asklevels_[ask_index].limit_size_ *
                          market_update_info_.asklevels_[ask_index].limit_price_);
    } else if (count < market_update_info_.asklevels_.size()) {
      sum_ask_size += market_update_info_.asklevels_[count].limit_size_;
      sum_ask_px_size +=
          (market_update_info_.asklevels_[count].limit_size_ * market_update_info_.asklevels_[count].limit_price_);
    }
    count++;
  }

  if (sum_bid_size == 0) {
    sum_bid_size = market_update_info_.bestbid_size_;
    sum_bid_px_size = market_update_info_.bestbid_size_ * market_update_info_.bestbid_price_;
  }
  if (sum_ask_size == 0) {
    sum_ask_size = market_update_info_.bestask_size_;
    sum_ask_px_size = market_update_info_.bestask_size_ * market_update_info_.bestask_price_;
  }

  double valid_level_bid_price = sum_bid_px_size / (double)sum_bid_size;
  double valid_level_ask_price = sum_ask_px_size / (double)sum_ask_size;

  bid_side_valid_level_int_price_ = GetIntPx(valid_level_bid_price);
  ask_side_valid_level_int_price_ = GetIntPx(valid_level_ask_price);
  bid_side_valid_level_size_ = sum_bid_size;
  ask_side_valid_level_size_ = sum_ask_size;
  market_update_info_.valid_level_mid_price_ = (valid_level_bid_price + valid_level_ask_price) / 2.0;

  if (!price_to_yield_map_.empty()) {
    hybrid_market_update_info_.valid_level_mid_price_ =
        (price_to_yield_map_[this_bid_int_price] + price_to_yield_map_[this_ask_int_price]) / 2.0;
  }
}

void SecurityMarketView::RecomputeBandPrices() {
  int this_bid_int_price = market_update_info_.bestbid_int_price_;
  int this_ask_int_price = market_update_info_.bestask_int_price_;
  int64_t sum_bid_size = 0;  // bid_size_at_int_price(this_bid_int_price_);
  int64_t sum_ask_size = 0;  // ask_size_at_int_price(this_ask_int_price_);

  double sum_ask_px_size = 0.0;
  double sum_bid_px_size = 0.0;

  int ask_index = GetAskIndex(this_ask_int_price);
  int bid_index = GetBidIndex(this_bid_int_price);

  std::vector<double> fact_vec(num_non_empty_levels_in_band_);
  double fact = 0.95;
  for (auto i = 0u; i < fact_vec.size(); i++) {
    fact_vec[i] = pow(fact, i);
  }

  unsigned int count = 0;
  while ((count < num_non_empty_levels_in_band_)) {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      int size = std::max(1.0, market_update_info_.bidlevels_[bid_index].limit_size_ * fact_vec[count]);
      sum_bid_size += size;
      sum_bid_px_size += size * market_update_info_.bidlevels_[bid_index].limit_price_;
      band_lower_bid_int_price_ = market_update_info_.bidlevels_[bid_index].limit_int_price_;
      bid_index = GetNextBidMapIndex(bid_index);
    } else if (count < market_update_info_.bidlevels_.size()) {
      int size = std::max(1.0, market_update_info_.bidlevels_[count].limit_size_ * fact_vec[count]);
      sum_bid_size += size;
      sum_bid_px_size += size * market_update_info_.bidlevels_[count].limit_price_;
      band_lower_bid_int_price_ = market_update_info_.bidlevels_[count].limit_int_price_;
    }
    count++;
  }

  count = 0;
  while ((count < num_non_empty_levels_in_band_)) {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      int size = std::max(1.0, market_update_info_.asklevels_[ask_index].limit_size_ * fact_vec[count]);
      sum_ask_size += size;
      sum_ask_px_size += size * market_update_info_.asklevels_[ask_index].limit_price_;
      band_lower_ask_int_price_ = market_update_info_.asklevels_[ask_index].limit_int_price_;
      ask_index = GetNextAskMapIndex(ask_index);
    } else if (count < market_update_info_.asklevels_.size()) {
      int size = std::max(1.0, market_update_info_.asklevels_[count].limit_size_ * fact_vec[count]);
      sum_ask_size += size;
      sum_ask_px_size += size * market_update_info_.asklevels_[count].limit_price_;
      band_lower_ask_int_price_ = market_update_info_.asklevels_[ask_index].limit_int_price_;
    }
    count++;
  }

  if (sum_bid_size == 0) {
    sum_bid_size = market_update_info_.bestbid_size_;
    sum_bid_px_size = market_update_info_.bestbid_size_ * market_update_info_.bestbid_price_;
  }
  if (sum_ask_size == 0) {
    sum_ask_size = market_update_info_.bestask_size_;
    sum_ask_px_size = market_update_info_.bestask_size_ * market_update_info_.bestask_price_;
  }

  double band_bid_price = sum_bid_px_size / (double)sum_bid_size;
  double band_ask_price = sum_ask_px_size / (double)sum_ask_size;

  bid_side_band_int_price_ = GetIntPx(band_bid_price);
  ask_side_band_int_price_ = GetIntPx(band_ask_price);
  bid_side_band_size_ = sum_bid_size;
  ask_side_band_size_ = sum_ask_size;
  market_update_info_.band_mkt_price_ =
      (band_bid_price * sum_ask_size + band_ask_price * sum_bid_size) / (double)(sum_bid_size + sum_ask_size);

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << sum_bid_size << " @ " << bid_side_band_int_price_ << " " << band_bid_price << " # "
                           << band_ask_price << " " << ask_side_band_int_price_ << " @ " << sum_ask_size
                           << DBGLOG_ENDL_FLUSH;
  }

  if (!price_to_yield_map_.empty()) {
    hybrid_market_update_info_.band_mkt_price_ =
        (price_to_yield_map_[this_bid_int_price] + price_to_yield_map_[this_ask_int_price]) / 2.0;
  }
}

void SecurityMarketView::RecomputeStableBidPrice() {
  int this_bid_int_price_ = market_update_info_.bestbid_int_price_;
  int sum_bid_size_ = bid_size_at_int_price(this_bid_int_price_);

  int count_ = 0;
  while (sum_bid_size_ < min_bid_size_to_consider_ && (count_ < MIN_PRICE_LEVELS_TO_ITERATE)) {
    this_bid_int_price_--;
    sum_bid_size_ += bid_size_at_int_price(this_bid_int_price_);
    count_++;
  }
  market_update_info_.stable_bid_price_ = GetDoublePx(this_bid_int_price_);
  if (!price_to_yield_map_.empty()) {
    hybrid_market_update_info_.stable_bid_price_ = price_to_yield_map_[this_bid_int_price_];
  }
}

void SecurityMarketView::RecomputeStableAskPrice() {
  int this_ask_int_price_ = market_update_info_.bestask_int_price_;
  int sum_ask_size_ = ask_size_at_int_price(this_ask_int_price_);

  int count_ = 0;
  while (sum_ask_size_ < min_ask_size_to_consider_ && (count_ < MIN_PRICE_LEVELS_TO_ITERATE)) {
    this_ask_int_price_++;
    sum_ask_size_ += ask_size_at_int_price(this_ask_int_price_);
    count_++;
  }
  market_update_info_.stable_ask_price_ = GetDoublePx(this_ask_int_price_);
  if (!price_to_yield_map_.empty()) {
    hybrid_market_update_info_.stable_ask_price_ = price_to_yield_map_[this_ask_int_price_];
  }
}

void SecurityMarketView::RecomputeImpliedVol() {
  double this_mid_price_ = market_update_info_.mid_price_;
  if (this_mid_price_ <= 0) return;
  double fut_mid_price_ = future_smv_->mid_price();
  market_update_info_.implied_vol_ = 100 * option_->MktImpliedVol(fut_mid_price_, this_mid_price_);
}

void SecurityMarketView::OnL1Trade(const double t_trade_price_, const int t_trade_size_, const TradeType_t t_buysell_) {
  if (trade_print_info_.computing_last_book_tdiff_) {
    if (prev_bid_was_quote_ && prev_ask_was_quote_) {  // the difference between last_book_mkt_size_weighted_price_ and
                                                       // mkt_size_weighted_price_ is that
      // in case of CME ( and other trade_before_quote exchanges ) where we might have a number of
      // back to back trade messages, the last_book_mkt_size_weighted_price_ is the snapshot of
      // mkt_size_weighted_price_
      // the last time the update was a book message
      market_update_info_.last_book_mkt_size_weighted_price_ =
          market_update_info_
              .mkt_size_weighted_price_;  // noting the mktpx as it was justbefore the first trade message
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.last_book_mkt_size_weighted_price_ =
            hybrid_market_update_info_.mkt_size_weighted_price_;
      }
    }
  }
  if (!market_update_info_.trade_update_implied_quote_) {
    market_update_info_.trade_update_implied_quote_ = true;
  }

  StorePreTrade();

  // set the primary variables before

  if (t_buysell_ != kTradeTypeNoInfo) {
    trade_print_info_.buysell_ = t_buysell_;
  } else {  // handling for exchanges where trade message does not have agressor side

    if (asklevels_size() > 0 && (t_trade_price_ >= ask_price(0) - 0.0001)) {
      trade_print_info_.buysell_ = kTradeTypeBuy;
    } else if (bidlevels_size() > 0 && (t_trade_price_ <= bid_price(0) + 0.0001)) {
      trade_print_info_.buysell_ = kTradeTypeSell;
    } else {
      trade_print_info_.buysell_ = kTradeTypeNoInfo;
    }
  }
  trade_print_info_.trade_price_ = t_trade_price_;
  trade_print_info_.size_traded_ = t_trade_size_;
  trade_print_info_.int_trade_price_ = GetIntPx(t_trade_price_);

  // do the conditional computations ... before the quote is adjusted for the trade

  if (trade_print_info_.computing_trade_impact_) {
    trade_print_info_.trade_impact_ = 0.0;
    switch (trade_print_info_.buysell_) {
      case kTradeTypeBuy: {
        if (market_update_info_.pretrade_bestask_size_ > 0) {
          trade_print_info_.trade_impact_ =
              (std::max(0.0, std::min(1.0, ((double)trade_print_info_.size_traded_ /
                                            (double)market_update_info_.pretrade_bestask_size_))));
        }
      } break;
      case kTradeTypeSell: {
        if (market_update_info_.pretrade_bestbid_size_ > 0) {
          trade_print_info_.trade_impact_ =
              (std::max(0.0, std::min(1.0, ((double)trade_print_info_.size_traded_ /
                                            (double)market_update_info_.pretrade_bestbid_size_))));
        }
      } break;

      default:  // kTradeTypeNoInfo
      {
        if (trade_print_info_.int_trade_price_ == market_update_info_.pretrade_bestbid_int_price_) {
          if (market_update_info_.pretrade_bestbid_size_ > 0) {
            trade_print_info_.trade_impact_ =
                (std::max(0.0, std::min(1.0, ((double)trade_print_info_.size_traded_ /
                                              (double)market_update_info_.pretrade_bestbid_size_))));
          }
        }
        if (trade_print_info_.int_trade_price_ == market_update_info_.pretrade_bestask_int_price_) {
          if (market_update_info_.pretrade_bestask_size_ > 0) {
            trade_print_info_.trade_impact_ =
                (std::max(0.0, std::min(1.0, ((double)trade_print_info_.size_traded_ /
                                              (double)market_update_info_.pretrade_bestask_size_))));
          }
        }
      }
    }
  }

  if (trade_print_info_.computing_int_trade_type_) {
    switch (trade_print_info_.buysell_) {
      case kTradeTypeBuy:
        trade_print_info_.int_trade_type_ = +1;
        break;
      case kTradeTypeSell:
        trade_print_info_.int_trade_type_ = -1;
        break;
      default:
        trade_print_info_.int_trade_type_ = 0;
    }
  }

  if (trade_print_info_.computing_sqrt_size_traded_) {
    trade_print_info_.sqrt_size_traded_ = sqrt(trade_print_info_.size_traded_);
  }

  if (trade_print_info_.computing_sqrt_trade_impact_) {
    trade_print_info_.sqrt_trade_impact_ = sqrt(trade_print_info_.trade_impact_);
  }

  if (trade_print_info_.computing_tradepx_mktpx_diff_) {
#define MAX_VALUE_TDIFF_TICKS 5
    if (trade_before_quote_) {
      if (fabs(trade_print_info_.trade_price_ - market_update_info_.mkt_size_weighted_price_) <
          MAX_VALUE_TDIFF_TICKS * min_price_increment_) {
        trade_print_info_.tradepx_mktpx_diff_ =
            trade_print_info_.trade_price_ - market_update_info_.mkt_size_weighted_price_;
      } else {
        trade_print_info_.tradepx_mktpx_diff_ = 0;
      }
    } else {
      if (fabs(trade_print_info_.trade_price_ - market_update_info_.prequote_mkt_size_weighted_price_) <
          MAX_VALUE_TDIFF_TICKS * min_price_increment_) {
        trade_print_info_.tradepx_mktpx_diff_ =
            trade_print_info_.trade_price_ - market_update_info_.prequote_mkt_size_weighted_price_;
      } else {
        trade_print_info_.tradepx_mktpx_diff_ = 0;
      }
    }
#undef MAX_VALUE_TDIFF_TICKS
  }

  if (trade_print_info_.computing_last_book_tdiff_) {
    if (market_update_info_.last_book_mkt_size_weighted_price_ > -500) {
      trade_print_info_.last_book_tdiff_ =
          trade_print_info_.trade_price_ - market_update_info_.last_book_mkt_size_weighted_price_;
    } else {
      trade_print_info_.last_book_tdiff_ = 0;
    }
  }
}

void SecurityMarketView::OnTrade(const double t_trade_price_, const int t_trade_size_, const TradeType_t t_buysell_) {
  if (trade_print_info_.computing_last_book_tdiff_) {
    if (prev_bid_was_quote_ && prev_ask_was_quote_) {  // the difference between last_book_mkt_size_weighted_price_ and
                                                       // mkt_size_weighted_price_ is that
      // in case of CME ( and other trade_before_quote exchanges ) where we might have a number of
      // back to back trade messages, the last_book_mkt_size_weighted_price_ is the snapshot of
      // mkt_size_weighted_price_
      // the last time the update was a book message
      market_update_info_.last_book_mkt_size_weighted_price_ =
          market_update_info_
              .mkt_size_weighted_price_;  // noting the mktpx as it was justbefore the first trade message
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.last_book_mkt_size_weighted_price_ =
            hybrid_market_update_info_.mkt_size_weighted_price_;
      }
    }
  }
  if (!market_update_info_.trade_update_implied_quote_) {
    market_update_info_.trade_update_implied_quote_ = true;
  }

  StorePreTrade();

  // set the primary variables before

  if (t_buysell_ != kTradeTypeNoInfo) {
    trade_print_info_.buysell_ = t_buysell_;
  } else {  // handling for exchanges where trade message does not have agressor side
    if (market_update_info_.asklevels_.size() > 0 &&
        (t_trade_price_ >= market_update_info_.asklevels_[0].limit_price_ - 0.0001)) {
      trade_print_info_.buysell_ = kTradeTypeBuy;
    } else if (market_update_info_.bidlevels_.size() > 0 &&
               (t_trade_price_ <= market_update_info_.bidlevels_[0].limit_price_ + 0.0001)) {
      trade_print_info_.buysell_ = kTradeTypeSell;
    } else {
      trade_print_info_.buysell_ = kTradeTypeNoInfo;
    }
  }
  trade_print_info_.trade_price_ = t_trade_price_;
  trade_print_info_.size_traded_ = t_trade_size_;
  trade_print_info_.int_trade_price_ = GetIntPx(t_trade_price_);

  // do the conditional computations ... before the quote is adjusted for the trade
  SetTradeVarsForIndicatorsIfRequired();

  // if trade information comes before updated quote information
  if (trade_before_quote()) {
    // interpolate quote with trade
    switch (trade_print_info_.buysell_) {
      case kTradeTypeBuy: {
        // highest_level_changed_ = 0;
        // buysell_highest_level_changed_ = kTradeTypeSell;
        SetBestLevelAskVariablesOnLift();
        if (is_ready_) {
          UpdateL1Prices();
          NotifyTradeListeners();
          NotifyOnReadyListeners();
        }
      } break;
      case kTradeTypeSell: {
        // highest_level_changed_ = 0;
        // buysell_highest_level_changed_ = kTradeTypeBuy;
        SetBestLevelBidVariablesOnHit();
        if (is_ready_) {
          UpdateL1Prices();
          NotifyTradeListeners();
          NotifyOnReadyListeners();
        }
      } break;
      default: {
        if ((this_smv_exch_source_ == HFSAT::kExchSourceRTS) || (this_smv_exch_source_ == HFSAT::kExchSourceMICEX) ||
            (this_smv_exch_source_ == HFSAT::kExchSourceMICEX_EQ) ||
            (this_smv_exch_source_ == HFSAT::kExchSourceMICEX_CR)) {  // An aggressor side couldnot be estimated.
          if (is_ready_) {
            UpdateL1Prices();
            NotifyTradeListeners();
            NotifyOnReadyListeners();
          }
        }
      }
    }
  } else {
    NotifyTradeListeners();
    NotifyOnReadyListeners();
  }
}

/**
 * called for nasdaq trade messages only
 */
void SecurityMarketView::OnNonDisplayedTrade(const double t_trade_price_, const int t_trade_size_,
                                             const TradeType_t t_buysell_) {
  StorePreTrade();

  if (t_buysell_ != kTradeTypeNoInfo) {
    trade_print_info_.buysell_ = t_buysell_;
  } else {  // handling for exchanges where trade message does not have agressor side
    if (market_update_info_.asklevels_.size() > 0 &&
        (t_trade_price_ >= market_update_info_.asklevels_[0].limit_price_ - 0.0001))
      trade_print_info_.buysell_ = kTradeTypeBuy;
    else if (market_update_info_.bidlevels_.size() > 0 &&
             (t_trade_price_ <= market_update_info_.bidlevels_[0].limit_price_ + 0.0001))
      trade_print_info_.buysell_ = kTradeTypeSell;
    else
      return;
  }
  trade_print_info_.trade_price_ = t_trade_price_;
  trade_print_info_.size_traded_ = t_trade_size_;
  trade_print_info_.int_trade_price_ = GetIntPx(t_trade_price_);

  // do the conditional computations ... before the quote is adjusted for the trade

  if (trade_print_info_.computing_trade_impact_) {
    if (trade_print_info_.buysell_ == kTradeTypeBuy) {
      if (market_update_info_.pretrade_bestask_size_ > 0) {
        trade_print_info_.trade_impact_ =
            (std::max(0.0, std::min(1.0, ((double)trade_print_info_.size_traded_ /
                                          (double)market_update_info_.pretrade_bestask_size_))));
      } else {
        trade_print_info_.trade_impact_ = 0.0;
      }
    }
    if (trade_print_info_.buysell_ == kTradeTypeSell) {
      if (market_update_info_.pretrade_bestbid_size_ > 0) {
        trade_print_info_.trade_impact_ =
            (std::max(0.0, std::min(1.0, ((double)trade_print_info_.size_traded_ /
                                          (double)market_update_info_.pretrade_bestbid_size_))));
      } else {
        trade_print_info_.trade_impact_ = 0.0;
      }
    }
  }

  if (trade_print_info_.computing_int_trade_type_) {  // FIXME TODO Work for NoTradeInfo
    trade_print_info_.int_trade_type_ = ((trade_print_info_.buysell_ == kTradeTypeBuy) ? +1 : -1);
  }

  if (trade_print_info_.computing_sqrt_size_traded_) {
    trade_print_info_.sqrt_size_traded_ = sqrt(trade_print_info_.size_traded_);
  }

  if (trade_print_info_.computing_sqrt_trade_impact_) {
    trade_print_info_.sqrt_trade_impact_ = sqrt(trade_print_info_.trade_impact_);
  }

  if (trade_print_info_.computing_tradepx_mktpx_diff_) {
#define MAX_VALUE_TDIFF_TICKS 5
    if (fabs(trade_print_info_.trade_price_ - market_update_info_.mkt_size_weighted_price_) <
        MAX_VALUE_TDIFF_TICKS * min_price_increment_) {
      trade_print_info_.tradepx_mktpx_diff_ =
          trade_print_info_.trade_price_ - market_update_info_.mkt_size_weighted_price_;
    } else {
      trade_print_info_.tradepx_mktpx_diff_ = 0;
    }
#undef MAX_VALUE_TDIFF_TICKS
  }

  if (trade_print_info_.computing_last_book_tdiff_) {
    trade_print_info_.last_book_tdiff_ =
        trade_print_info_.trade_price_ - market_update_info_.last_book_mkt_size_weighted_price_;
  }

  NotifyTradeListeners();
  NotifyOnReadyListeners();
}

bool SecurityMarketView::Uncross() {
#ifdef USE_CIRCULAR_BUFFER_FOR_MKT_BOOK
  return true;  // NOthing to do
#else
  bool crossed = false;
  if (IsL1Valid()) {  // if book has both sides check if book crossed
    while (market_update_info_.bidlevels_[0].limit_int_price_ >=
           market_update_info_.asklevels_[0].limit_int_price_) {  // it is crossed
      crossed = true;
      dbglogger_ << watch_.tv() << ' ' << typeid(*this).name() << ':' << __func__ << " Crossed \n";
      dbglogger_.CheckToFlushBuffer();
      if (market_update_info_.asklevels_[0].mod_time_ <
          market_update_info_.bidlevels_[0].mod_time_) {  // since bid is newer remove top ask
        RemoveTopAsk();
      } else {
        RemoveTopBid();
      }
    }
  }
  return crossed;
#endif
}

bool SecurityMarketView::Check_Bid_Order_Level_Correctness(const int t_level_) const {
  int total_size_ = 0, total_order_count_ = 0;
  if ((int)market_update_info_.bid_level_order_depth_book_.size() > t_level_) {
    for (unsigned int index_ = 0; index_ < market_update_info_.bid_level_order_depth_book_[t_level_].size(); index_++) {
      total_size_ += market_update_info_.bid_level_order_depth_book_[t_level_][index_]->size_;
      total_order_count_ += market_update_info_.bid_level_order_depth_book_[t_level_][index_]->order_count_;
    }
    if (total_size_ == market_update_info_.bidlevels_[t_level_].limit_size_) {
      return true;
    } else {
      return false;
    }
  }
  return true;
}

bool SecurityMarketView::Check_Ask_Order_Level_Correctness(const int t_level_) const {
  int total_size_ = 0, total_order_count_ = 0;
  if ((int)market_update_info_.ask_level_order_depth_book_.size() > t_level_) {
    for (unsigned int index_ = 0; index_ < market_update_info_.ask_level_order_depth_book_[t_level_].size(); index_++) {
      total_size_ += market_update_info_.ask_level_order_depth_book_[t_level_][index_]->size_;
      total_order_count_ += market_update_info_.ask_level_order_depth_book_[t_level_][index_]->order_count_;
    }
    if (total_size_ == market_update_info_.asklevels_[t_level_].limit_size_) {
      return true;
    } else {
      return false;
    }
    return false;
  }
  return true;
}

std::string SecurityMarketView::ShowMarket() const {
  std::ostringstream t_temp_oss_;

  bool bid_correct_ = true;
  bool ask_correct_ = true;

  t_temp_oss_ << market_update_info_.secname_ << "\n";
  unsigned int m_m_levels = std::min(MinValidNumBidLevels(5), MinValidNumAskLevels(5));
  for (unsigned int t_level_ = 0; t_level_ < m_m_levels; t_level_++) {
    t_temp_oss_.width(5);
    t_temp_oss_ << bid_int_price_level(t_level_);
    t_temp_oss_ << " ";
    t_temp_oss_.width(6);
    t_temp_oss_ << bid_size(t_level_);
    t_temp_oss_ << " ";
    t_temp_oss_.width(5);
    t_temp_oss_ << bid_order(t_level_);
    t_temp_oss_ << " ";
    t_temp_oss_.width(6);
    t_temp_oss_ << bid_price(t_level_);
    t_temp_oss_ << " ";
    t_temp_oss_.width(6);
    t_temp_oss_ << bid_int_price(t_level_);
    t_temp_oss_ << " X ";
    t_temp_oss_.width(6);
    t_temp_oss_ << ask_int_price(t_level_);
    t_temp_oss_ << " ";
    t_temp_oss_.width(6);
    t_temp_oss_ << ask_price(t_level_);
    t_temp_oss_ << " ";
    t_temp_oss_.width(5);
    t_temp_oss_ << ask_order(t_level_);
    t_temp_oss_ << " ";
    t_temp_oss_.width(6);
    t_temp_oss_ << ask_size(t_level_);
    t_temp_oss_ << " ";
    t_temp_oss_ << ask_int_price_level(t_level_);

    if (use_order_level_book_ == true) {
      bid_correct_ = Check_Bid_Order_Level_Correctness(t_level_);
      ask_correct_ = Check_Ask_Order_Level_Correctness(t_level_);
      if (ask_correct_ == false || bid_correct_ == false) {
        t_temp_oss_ << " FAULTY ";
      }
      t_temp_oss_ << std::endl;

      int m_order_levels_ = std::min(market_update_info_.bid_level_order_depth_book_[t_level_].size(),
                                     market_update_info_.ask_level_order_depth_book_[t_level_].size());

      for (int order_level_ = 0; order_level_ < m_order_levels_; order_level_++) {
        t_temp_oss_.width(5);
        t_temp_oss_ << bid_int_price_level(t_level_);
        t_temp_oss_ << " ";
        t_temp_oss_.width(6);
        t_temp_oss_ << market_update_info_.bid_level_order_depth_book_[t_level_][order_level_]->size_;
        t_temp_oss_ << " ";
        t_temp_oss_.width(5);
        t_temp_oss_ << market_update_info_.bid_level_order_depth_book_[t_level_][order_level_]->order_count_;
        t_temp_oss_ << " ";
        t_temp_oss_.width(6);
        t_temp_oss_ << bid_price(t_level_);
        t_temp_oss_ << " ";
        t_temp_oss_.width(6);
        t_temp_oss_ << bid_int_price(t_level_);

        t_temp_oss_ << " X ";
        t_temp_oss_.width(6);
        t_temp_oss_ << ask_int_price(t_level_);
        t_temp_oss_ << " ";
        t_temp_oss_.width(6);
        t_temp_oss_ << ask_price(t_level_);
        t_temp_oss_ << " ";
        t_temp_oss_.width(5);
        t_temp_oss_ << market_update_info_.ask_level_order_depth_book_[t_level_][order_level_]->order_count_;
        t_temp_oss_ << " ";
        t_temp_oss_.width(6);
        t_temp_oss_ << market_update_info_.ask_level_order_depth_book_[t_level_][order_level_]->size_;
        t_temp_oss_ << " ";
        t_temp_oss_ << ask_int_price_level(t_level_);
        /*t_temp_oss_ << " "  ;
                  t_temp_oss_.width ( 10 );
                  t_temp_oss_ <<
           market_update_info_.bid_level_order_depth_book_[t_level_][order_level_]->order_entry_time_ ;
                  t_temp_oss_ << " " ;
                  t_temp_oss_.width ( 10 );
                  t_temp_oss_ <<
           market_update_info_.ask_level_order_depth_book_[t_level_][order_level_]->order_entry_time_ ;
                  t_temp_oss_ << " " ;*/
        t_temp_oss_ << std::endl;
      }
      if (market_update_info_.bid_level_order_depth_book_.size() > t_level_) {
        for (int order_level_ = m_order_levels_;
             order_level_ < (int)market_update_info_.bid_level_order_depth_book_[t_level_].size(); order_level_++) {
          t_temp_oss_.width(5);
          t_temp_oss_ << bid_int_price_level(t_level_);
          t_temp_oss_ << " ";
          t_temp_oss_.width(6);
          t_temp_oss_ << market_update_info_.bid_level_order_depth_book_[t_level_][order_level_]->size_;
          t_temp_oss_ << " ";
          t_temp_oss_.width(5);
          t_temp_oss_ << market_update_info_.bid_level_order_depth_book_[t_level_][order_level_]->order_count_;
          t_temp_oss_ << " ";
          t_temp_oss_.width(6);
          t_temp_oss_ << bid_price(t_level_);
          t_temp_oss_ << " ";
          t_temp_oss_.width(6);
          t_temp_oss_ << bid_int_price(t_level_);
          t_temp_oss_ << " X ";
          /*t_temp_oss_ << " "  ;
                      t_temp_oss_.width ( 10 );
                      t_temp_oss_ <<
             market_update_info_.bid_level_order_depth_book_[t_level_][order_level_]->order_entry_time_ ;*/
          t_temp_oss_ << std::endl;
        }
      }
      if (market_update_info_.ask_level_order_depth_book_.size() > t_level_) {
        for (int order_level_ = m_order_levels_;
             order_level_ < (int)market_update_info_.ask_level_order_depth_book_[t_level_].size(); order_level_++) {
          t_temp_oss_ << "                                  X ";
          t_temp_oss_.width(6);
          t_temp_oss_ << ask_int_price(t_level_);
          t_temp_oss_ << " ";
          t_temp_oss_.width(6);
          t_temp_oss_ << ask_price(t_level_);
          t_temp_oss_ << " ";
          t_temp_oss_.width(5);
          t_temp_oss_ << market_update_info_.ask_level_order_depth_book_[t_level_][order_level_]->order_count_;
          t_temp_oss_ << " ";
          t_temp_oss_.width(6);
          t_temp_oss_ << market_update_info_.ask_level_order_depth_book_[t_level_][order_level_]->size_;
          t_temp_oss_ << " ";
          t_temp_oss_ << ask_int_price_level(t_level_);
          /*t_temp_oss_ << " "  ;
                      t_temp_oss_.width ( 10 );
                      t_temp_oss_ <<
             market_update_info_.ask_level_order_depth_book_[t_level_][order_level_]->order_entry_time_ ;*/
          t_temp_oss_ << std::endl;
        }
      }
    }
    t_temp_oss_ << std::endl;
  }
  if (MinValidNumBidLevels(5) > m_m_levels)  // remove m_m_levels this is fro comparison testing
  {
    for (unsigned int t_level_ = m_m_levels; t_level_ < MinValidNumBidLevels(5); t_level_++) {
      t_temp_oss_.width(5);
      t_temp_oss_ << bid_int_price_level(t_level_);
      t_temp_oss_ << " ";
      t_temp_oss_.width(6);
      t_temp_oss_ << bid_size(t_level_);
      t_temp_oss_ << " ";
      t_temp_oss_.width(5);
      t_temp_oss_ << bid_order(t_level_);
      t_temp_oss_ << " ";
      t_temp_oss_.width(6);
      t_temp_oss_ << bid_price(t_level_);
      t_temp_oss_ << " ";
      t_temp_oss_.width(6);
      t_temp_oss_ << bid_int_price(t_level_);
      t_temp_oss_ << " X ";
      if (use_order_level_book_ == true) {
        bid_correct_ = Check_Bid_Order_Level_Correctness(t_level_);
        if (bid_correct_ == false) {
          t_temp_oss_ << " FAULTY ";
        }
        t_temp_oss_ << std::endl;
        if ((market_update_info_.bid_level_order_depth_book_.size()) > t_level_) {
          for (unsigned int order_level_ = 0;
               order_level_ < (unsigned int)market_update_info_.bid_level_order_depth_book_[t_level_].size();
               order_level_++) {
            t_temp_oss_.width(5);
            t_temp_oss_ << bid_int_price_level(t_level_);
            t_temp_oss_ << " ";
            t_temp_oss_.width(6);
            t_temp_oss_ << market_update_info_.bid_level_order_depth_book_[t_level_][order_level_]->size_;
            t_temp_oss_ << " ";
            t_temp_oss_.width(5);
            t_temp_oss_ << market_update_info_.bid_level_order_depth_book_[t_level_][order_level_]->order_count_;
            t_temp_oss_ << " ";
            t_temp_oss_.width(6);
            t_temp_oss_ << bid_price(t_level_);
            t_temp_oss_ << " ";
            t_temp_oss_.width(6);
            t_temp_oss_ << bid_int_price(t_level_);
            t_temp_oss_ << " X ";
            t_temp_oss_ << std::endl;
          }
        }
      }

      t_temp_oss_ << std::endl;
    }
  } else if (NumAskLevels() > (int)m_m_levels) {
    for (int t_level_ = m_m_levels; t_level_ < NumAskLevels(); t_level_++) {
      t_temp_oss_ << "                                  X ";
      t_temp_oss_.width(6);
      t_temp_oss_ << ask_int_price(t_level_);
      t_temp_oss_ << " ";
      t_temp_oss_.width(6);
      t_temp_oss_ << ask_price(t_level_);
      t_temp_oss_ << " ";
      t_temp_oss_.width(5);
      t_temp_oss_ << ask_order(t_level_);
      t_temp_oss_ << " ";
      t_temp_oss_.width(6);
      t_temp_oss_ << ask_size(t_level_);
      t_temp_oss_ << " ";
      t_temp_oss_ << ask_int_price_level(t_level_);
      if (use_order_level_book_ == true) {
        ask_correct_ = Check_Ask_Order_Level_Correctness(t_level_);
        if (ask_correct_ == false) {
          t_temp_oss_ << " FAULTY ";
        }
        t_temp_oss_ << std::endl;
        if (((int)market_update_info_.ask_level_order_depth_book_.size()) > t_level_) {
          for (unsigned int order_level_ = 0;
               order_level_ < market_update_info_.ask_level_order_depth_book_[t_level_].size(); order_level_++) {
            t_temp_oss_ << "                                  X ";
            t_temp_oss_.width(6);
            t_temp_oss_ << ask_int_price(t_level_);
            t_temp_oss_ << " ";
            t_temp_oss_.width(6);
            t_temp_oss_ << ask_price(t_level_);
            t_temp_oss_ << " ";
            t_temp_oss_.width(5);
            t_temp_oss_ << market_update_info_.ask_level_order_depth_book_[t_level_][order_level_]->order_count_;
            t_temp_oss_ << " ";
            t_temp_oss_.width(6);
            t_temp_oss_ << market_update_info_.ask_level_order_depth_book_[t_level_][order_level_]->size_;
            t_temp_oss_ << " ";
            t_temp_oss_ << ask_int_price_level(t_level_);
            t_temp_oss_ << std::endl;
          }
        }
      }
      t_temp_oss_ << std::endl;
    }
  }
  t_temp_oss_ << "\n";
  return t_temp_oss_.str();
}

bool SecurityMarketView::IsError() const {
  if (IsBidBookL2() && IsAskBookL2()) {
    if (bid_price(0) >= ask_price(0)) {
      printf("error top level %f %f \n", bid_price(0), ask_price(0));
      printf("%s\n", ShowMarket().c_str());
      return true;
    }
    for (unsigned int i = 1u; i < MinValidNumBidLevels(10); i++) {
      if (bid_price(i - 1) <= bid_price(i)) {
        printf("error bid level %d %f %d %f\n", i - 1, bid_price(i - 1), i, bid_price(i));
        printf("%s\n", ShowMarket().c_str());  // return true;
      }
    }
    for (unsigned int i = 1u; i < MinValidNumAskLevels(10); i++) {
      if (ask_price(i - 1) >= ask_price(i)) {
        printf("error ask level %d %f %d %f\n", i - 1, ask_price(i - 1), i, ask_price(i));
        printf("%s\n", ShowMarket().c_str());  // return true;
      }
    }
  }
  return false;
}

// -------------------PL Related Functions----------------
void SecurityMarketView::subscribePlEvents(SMVPLChangeListener *t_new_listener_) const {
  if (t_new_listener_ != nullptr) {
    VectorUtils::UniqueVectorAdd(pl_change_listeners_, t_new_listener_);
    pl_change_listeners_present_ = true;
  }
}

void SecurityMarketView::unsubscribePlEvents(SMVPLChangeListener *t_new_listener_) {
  if (t_new_listener_ != nullptr) {
    VectorUtils::UniqueVectorRemove(pl_change_listeners_, t_new_listener_);
    if (pl_change_listeners_.empty()) {
      pl_change_listeners_present_ = false;
    }
  }
}

// -------------------@End PL Related Functions------------

/// 1. henceforth ignore effects of trade masks and
/// 2. later if we start handling self executions, effect of last self execution can be ingored as well
///          (note that in EUREX updates are sent to public stream before they are sent to private stream and hence
///          looking at self executions should not be useful)

void SecurityMarketView::SetBestLevelBidVariablesOnQuote() {
  if (!prev_bid_was_quote_) {  // clearing trade mask
    VectorUtils::FillInValue(running_hit_size_vec_, 0);
    prev_bid_was_quote_ = true;
  }

  if (market_update_info_.trade_update_implied_quote_) {
    market_update_info_.trade_update_implied_quote_ = false;
    trade_print_info_.num_trades_++;  // move incrementing numtrades to the first quote after a trade
  }

  if (IsBidBookEmpty()) {
    market_update_info_.bestbid_price_ = kInvalidPrice;
    market_update_info_.bestbid_size_ = 0;
    market_update_info_.bestbid_int_price_ = kInvalidIntPrice;
    market_update_info_.bestbid_ordercount_ = 0;
    if (!price_to_yield_map_.empty()) {
      hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[kInvalidIntPrice];
      hybrid_market_update_info_.bestbid_size_ = 0;
      hybrid_market_update_info_.bestbid_int_price_ = kInvalidIntPrice;
      hybrid_market_update_info_.bestbid_ordercount_ = 0;
    }
    return;
  }

  if (remove_self_orders_from_book_) {
    SetBestLevelBidVariablesIfNonSelfBook();
  } else if (bid_int_price(0) != self_best_bid_ask_.best_bid_int_price_) {
    // TODO_OPT make this more efficient
    market_update_info_.bestbid_price_ = bid_price(0);
    market_update_info_.bestbid_size_ = bid_size(0);
    market_update_info_.bestbid_int_price_ = bid_int_price(0);
    market_update_info_.bestbid_ordercount_ = bid_order(0);
    if (!price_to_yield_map_.empty()) {
      hybrid_market_update_info_.bestbid_size_ = bid_size(0);
      hybrid_market_update_info_.bestbid_int_price_ = bid_int_price(0);
      hybrid_market_update_info_.bestbid_ordercount_ = bid_order(0);
      hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[hybrid_market_update_info_.bestbid_int_price_];
    }
  }

  // assign is_ready_
  if (!is_ready_) {
    if (((!IsBidBookEmpty()) && (market_update_info_.bestbid_int_price_ != kInvalidIntPrice) &&
         (market_update_info_.bestbid_size_ > 0)) &&
        ((!IsAskBookEmpty()) && (market_update_info_.bestask_int_price_ != kInvalidIntPrice) &&
         (market_update_info_.bestask_size_ > 0)) &&
        (market_update_info_.bestbid_int_price_ < market_update_info_.bestask_int_price_) && ArePricesComputed()) {
      is_ready_ = true;
    }
  }
}

// 1. henceforth ignore effects of trade masks and
// 2. later if we start handling self executions, effect of last self execution can be ingored as well
//          (note that in EUREX updates are sent to public stream before they are sent to private stream and hence
//          looking at self executions should not be useful)

void SecurityMarketView::SetBestLevelAskVariablesOnQuote() {
  if (!prev_ask_was_quote_) {  // clearing trade mask
    VectorUtils::FillInValue(running_lift_size_vec_, 0);
    prev_ask_was_quote_ = true;
  }

  if (market_update_info_.trade_update_implied_quote_) {
    market_update_info_.trade_update_implied_quote_ = false;
    trade_print_info_.num_trades_++;  // move incrementing numtrades to the first quote after a trade
  }

  if (IsAskBookEmpty()) {
    market_update_info_.bestask_price_ = kInvalidPrice;
    market_update_info_.bestask_size_ = 0;
    market_update_info_.bestask_int_price_ = kInvalidIntPrice;
    market_update_info_.bestask_ordercount_ = 0;
    if (!price_to_yield_map_.empty()) {
      hybrid_market_update_info_.bestask_price_ = price_to_yield_map_[kInvalidIntPrice];
      ;
      hybrid_market_update_info_.bestask_size_ = 0;
      hybrid_market_update_info_.bestask_int_price_ = kInvalidIntPrice;
      hybrid_market_update_info_.bestask_ordercount_ = 0;
    }
    return;
  }

  if (remove_self_orders_from_book_) {
    SetBestLevelAskVariablesIfNonSelfBook();
  } else if ((market_update_info_.asklevels_[0].limit_int_price_ != self_best_bid_ask_.best_ask_int_price_)) {
    // no need to care about self orders
    market_update_info_.bestask_price_ = market_update_info_.asklevels_[0].limit_price_;
    market_update_info_.bestask_size_ = market_update_info_.asklevels_[0].limit_size_;
    market_update_info_.bestask_int_price_ = market_update_info_.asklevels_[0].limit_int_price_;
    market_update_info_.bestask_ordercount_ = market_update_info_.asklevels_[0].limit_ordercount_;
    if (!price_to_yield_map_.empty()) {
      hybrid_market_update_info_.bestask_price_ = price_to_yield_map_[market_update_info_.bestask_int_price_];
      hybrid_market_update_info_.bestask_size_ = market_update_info_.bestask_size_;
      hybrid_market_update_info_.bestask_int_price_ = market_update_info_.bestask_int_price_;
      hybrid_market_update_info_.bestask_ordercount_ = market_update_info_.bestask_ordercount_;
    }
  }

  if (!is_ready_) {
    if ((market_update_info_.bidlevels_.size() > 0 && market_update_info_.bestbid_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestbid_size_ > 0) &&
        (market_update_info_.asklevels_.size() > 0 && market_update_info_.bestask_int_price_ != kInvalidIntPrice &&
         market_update_info_.bestask_size_ > 0) &&
        (market_update_info_.bestbid_int_price_ < market_update_info_.bestask_int_price_) && ArePricesComputed()) {
      is_ready_ = true;
    }
  }
}

void SecurityMarketView::SetBestLevelBidVariablesIfNonSelfBook() {
  if (remove_self_orders_from_book_ && smart_remove_self_orders_from_book_ && conf_to_market_update_msecs_ &&
      p_prom_order_manager_) {  // Smart non-self
    current_best_level_ = market_update_info_;
    // Try to match the oldest ors update with received market update.
    std::deque<BestBidAskInfo> &t_mfm_ordered_bid_ask_updates_ = p_prom_order_manager_->mfm_ordered_bid_ask_updates();
    BestBidAskInfo &t_mkt_affirmed_bid_ask_ = p_prom_order_manager_->mkt_affirmed_bid_ask();
    const int t_current_mfm_ = watch_.msecs_from_midnight();

    for (std::deque<BestBidAskInfo>::iterator _itr_ = t_mfm_ordered_bid_ask_updates_.begin();
         _itr_ != t_mfm_ordered_bid_ask_updates_.end();) {
      if (t_current_mfm_ - _itr_->msecs_from_midnight_ >= conf_to_market_update_msecs_) {
        // This update was not "recognized" in the mkt updates within
        // expected time durations.
        // Proceed to apply this to the mask.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " expired @ " << watch_.msecs_from_midnight()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);
        _itr_ = t_mfm_ordered_bid_ask_updates_.begin();

        continue;
      }

      const BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      if (t_diff_ == *_itr_) {  // Matched.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " matched with " << t_diff_.ToString()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);

        break;  // Do not match a single market update with more than one self update.
      } else {
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " unmatched with " << t_diff_.ToString()
                                      << ", discarding." << DBGLOG_ENDL_FLUSH;
        }
      }

      break;
    }

    if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
      BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      DBGLOG_TIME_CLASS_FUNC_LINE << " CURRENT_BEST_LEVEL_ : " << current_best_level_.ToString()
                                  << " LAST_BEST_LEVEL_ : " << last_best_level_.ToString()
                                  << " DIFF_ : " << t_diff_.ToString() << DBGLOG_ENDL_FLUSH;
    }

    last_best_level_.best_bid_int_price_ = current_best_level_.best_bid_int_price_;
    last_best_level_.best_bid_size_ = current_best_level_.best_bid_size_;
    last_best_level_.best_bid_orders_ = current_best_level_.best_bid_orders_;

    self_best_bid_ask_ = t_mkt_affirmed_bid_ask_;
  } else if (remove_self_orders_from_book_) {  // Dumb non-self
    self_best_bid_ask_ = p_prom_order_manager_->ors_best_bid_ask();
  }

  if (remove_self_orders_from_book_ && (bid_int_price(0) == self_best_bid_ask_.best_bid_int_price_)) {
    int t_non_self_best_bid_size_ = (bid_size(0) - self_best_bid_ask_.best_bid_size_);
    int t_non_self_best_bid_order_count_ = (bid_order(0) - self_best_bid_ask_.best_bid_orders_);

    if (t_non_self_best_bid_size_ <= 0) {
      // remove level
      if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
        unsigned int second_bid_index = IndexedBookGetNextNonEmptyBidMapIndex(base_bid_index_);
        if (second_bid_index > 0) {
          const MarketUpdateInfoLevelStruct &second_bid_level = GetBidLevelAtIndex(second_bid_index);
          market_update_info_.bestbid_price_ = second_bid_level.limit_price_;
          market_update_info_.bestbid_size_ = second_bid_level.limit_size_;
          market_update_info_.bestbid_int_price_ = second_bid_level.limit_int_price_;
          market_update_info_.bestbid_ordercount_ = second_bid_level.limit_ordercount_;
          if (!price_to_yield_map_.empty()) {
            hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[second_bid_level.limit_int_price_];
            hybrid_market_update_info_.bestbid_size_ = second_bid_level.limit_size_;
            hybrid_market_update_info_.bestbid_int_price_ = second_bid_level.limit_int_price_;
            hybrid_market_update_info_.bestbid_ordercount_ = second_bid_level.limit_ordercount_;
          }
        }
      } else {
        market_update_info_.bestbid_price_ = ((market_update_info_.bidlevels_.size() > 1 &&
                                               market_update_info_.bidlevels_[1].limit_price_ > kInvalidPrice)
                                                  ? market_update_info_.bidlevels_[1].limit_price_
                                                  : market_update_info_.bidlevels_[0].limit_price_);
        market_update_info_.bestbid_size_ =
            ((market_update_info_.bidlevels_.size() > 1 && market_update_info_.bidlevels_[1].limit_size_ > 0)
                 ? market_update_info_.bidlevels_[1].limit_size_
                 : market_update_info_.bidlevels_[0].limit_size_);
        market_update_info_.bestbid_int_price_ =
            ((market_update_info_.bidlevels_.size() > 1 &&
              market_update_info_.bidlevels_[1].limit_int_price_ > kInvalidIntPrice)
                 ? market_update_info_.bidlevels_[1].limit_int_price_
                 : market_update_info_.bidlevels_[0].limit_int_price_);
        market_update_info_.bestbid_ordercount_ =
            ((market_update_info_.bidlevels_.size() > 1 && market_update_info_.bidlevels_[1].limit_ordercount_ > 0)
                 ? market_update_info_.bidlevels_[1].limit_ordercount_
                 : market_update_info_.bidlevels_[0].limit_ordercount_);
        if (!price_to_yield_map_.empty()) {
          hybrid_market_update_info_.bestbid_size_ =
              ((market_update_info_.bidlevels_.size() > 1 && market_update_info_.bidlevels_[1].limit_size_ > 0)
                   ? market_update_info_.bidlevels_[1].limit_size_
                   : market_update_info_.bidlevels_[0].limit_size_);
          hybrid_market_update_info_.bestbid_int_price_ =
              ((market_update_info_.bidlevels_.size() > 1 &&
                market_update_info_.bidlevels_[1].limit_int_price_ > kInvalidIntPrice)
                   ? market_update_info_.bidlevels_[1].limit_int_price_
                   : market_update_info_.bidlevels_[0].limit_int_price_);
          hybrid_market_update_info_.bestbid_ordercount_ =
              ((market_update_info_.bidlevels_.size() > 1 && market_update_info_.bidlevels_[1].limit_ordercount_ > 0)
                   ? market_update_info_.bidlevels_[1].limit_ordercount_
                   : market_update_info_.bidlevels_[0].limit_ordercount_);
          hybrid_market_update_info_.bestbid_price_ =
              price_to_yield_map_[hybrid_market_update_info_.bestbid_int_price_];
        }
      }
    } else {
      // assign level 0 with reduced size
      market_update_info_.bestbid_price_ = bid_price(0);
      market_update_info_.bestbid_size_ = t_non_self_best_bid_size_;
      market_update_info_.bestbid_int_price_ = bid_int_price(0);
      market_update_info_.bestbid_ordercount_ = (t_non_self_best_bid_order_count_ > 0)
                                                    ? t_non_self_best_bid_order_count_
                                                    : 1;  // Transient bad cases reduce order_count to 0.
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestbid_size_ = t_non_self_best_bid_size_;
        hybrid_market_update_info_.bestbid_int_price_ = bid_int_price(0);
        hybrid_market_update_info_.bestbid_ordercount_ = (t_non_self_best_bid_order_count_ > 0)
                                                             ? t_non_self_best_bid_order_count_
                                                             : 1;  // Transient bad cases reduce order_count to 0.
        hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[hybrid_market_update_info_.bestbid_int_price_];
      }
    }
  }

  // if we have both indexed book and older books then
  // we should use functions like bid_int_price (0) and not
  // market_update_info_.bidlevels_[0].limit_int_price_

  if (remove_self_orders_from_book_ &&
      (dbglogger_.CheckLoggingLevel(SMVSELF_INFO) || dump_non_self_smv_on_next_update_) && (!(IsBidBookEmpty())) &&
      (!(IsAskBookEmpty()))) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " " << market_update_info_.shortcode_ << " MKT : " << bid_order(0) << " "
                                << bid_size(0) << " " << bid_int_price(0) << " - " << ask_int_price(0) << " "
                                << ask_size(0) << " " << ask_order(0)
                                << " SELF-MKT : " << self_best_bid_ask_.best_bid_orders_ << " "
                                << self_best_bid_ask_.best_bid_size_ << " " << self_best_bid_ask_.best_bid_int_price_
                                << " - " << self_best_bid_ask_.best_ask_int_price_ << " "
                                << self_best_bid_ask_.best_ask_size_ << " " << self_best_bid_ask_.best_ask_orders_
                                << " NON-SELF-MKT : " << market_update_info_.bestbid_ordercount_ << " "
                                << market_update_info_.bestbid_size_ << " " << market_update_info_.bestbid_int_price_
                                << " - " << market_update_info_.bestask_int_price_ << " "
                                << market_update_info_.bestask_size_ << " " << market_update_info_.bestask_ordercount_
                                << DBGLOG_ENDL_FLUSH;

    dump_non_self_smv_on_next_update_ = false;
  }
}

void SecurityMarketView::SetBestLevelAskVariablesIfNonSelfBook() {
  if (remove_self_orders_from_book_ && smart_remove_self_orders_from_book_ && conf_to_market_update_msecs_ &&
      p_prom_order_manager_) {  // Smart non-self
    current_best_level_ = market_update_info_;

    // Try to match the oldest ors update with received market update.
    std::deque<BestBidAskInfo> &t_mfm_ordered_bid_ask_updates_ = p_prom_order_manager_->mfm_ordered_bid_ask_updates();
    BestBidAskInfo &t_mkt_affirmed_bid_ask_ = p_prom_order_manager_->mkt_affirmed_bid_ask();
    const int t_current_mfm_ = watch_.msecs_from_midnight();

    for (std::deque<BestBidAskInfo>::iterator _itr_ = t_mfm_ordered_bid_ask_updates_.begin();
         _itr_ != t_mfm_ordered_bid_ask_updates_.end();) {
      if (t_current_mfm_ - _itr_->msecs_from_midnight_ >= conf_to_market_update_msecs_) {
        // This update was not "recognized" in the mkt updates within
        // expected time durations.
        // Proceed to apply this to the mask.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " expired @ " << watch_.msecs_from_midnight()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);
        _itr_ = t_mfm_ordered_bid_ask_updates_.begin();

        continue;
      }

      const BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      if (t_diff_ == *_itr_) {  // Matched.
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " matched with " << t_diff_.ToString()
                                      << ", applying to " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mkt_affirmed_bid_ask_ = t_mkt_affirmed_bid_ask_ + *_itr_;

        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " t_mkt_affirmed_bid_ask_ " << t_mkt_affirmed_bid_ask_.ToString() << "."
                                      << DBGLOG_ENDL_FLUSH;
        }

        t_mfm_ordered_bid_ask_updates_.erase(_itr_);

        break;  // Do not match a single market update with more than one self update.
      } else {
        if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
          DBGLOG_TIME_CLASS_FUNC_LINE << " " << _itr_->ToString() << " unmatched with " << t_diff_.ToString()
                                      << ", discarding." << DBGLOG_ENDL_FLUSH;
        }
      }

      break;
    }

    if (dbglogger_.CheckLoggingLevel(SMVSELF_INFO)) {
      BestBidAskInfo t_diff_ = current_best_level_ - last_best_level_;
      DBGLOG_TIME_CLASS_FUNC_LINE << " CURRENT_BEST_LEVEL_ : " << current_best_level_.ToString()
                                  << " LAST_BEST_LEVEL_ : " << last_best_level_.ToString()
                                  << " DIFF_ : " << t_diff_.ToString() << DBGLOG_ENDL_FLUSH;
    }

    last_best_level_.best_ask_int_price_ = current_best_level_.best_ask_int_price_;
    last_best_level_.best_ask_size_ = current_best_level_.best_ask_size_;
    last_best_level_.best_ask_orders_ = current_best_level_.best_ask_orders_;

    self_best_bid_ask_ = t_mkt_affirmed_bid_ask_;
  } else if (remove_self_orders_from_book_) {  // Dumb non-self
    self_best_bid_ask_ = p_prom_order_manager_->ors_best_bid_ask();
  }

  if (remove_self_orders_from_book_ &&
      (market_update_info_.asklevels_[0].limit_int_price_ == self_best_bid_ask_.best_ask_int_price_)) {
    int t_non_self_best_ask_size_ = (market_update_info_.asklevels_[0].limit_size_ - self_best_bid_ask_.best_ask_size_);
    int t_non_self_best_ask_order_count_ =
        (market_update_info_.asklevels_[0].limit_ordercount_ - self_best_bid_ask_.best_ask_orders_);

    if (t_non_self_best_ask_size_ <= 0) {  // if apart from us no one is at this level
      // set bestlevel to the second level
      market_update_info_.bestask_price_ =
          ((market_update_info_.asklevels_.size() > 1 && market_update_info_.asklevels_[1].limit_price_ > kInvalidPrice)
               ? market_update_info_.asklevels_[1].limit_price_
               : market_update_info_.asklevels_[0].limit_price_);
      market_update_info_.bestask_size_ =
          ((market_update_info_.asklevels_.size() > 1 && market_update_info_.asklevels_[1].limit_size_ > 0)
               ? market_update_info_.asklevels_[1].limit_size_
               : market_update_info_.asklevels_[0].limit_size_);
      market_update_info_.bestask_int_price_ = ((market_update_info_.asklevels_.size() > 1 &&
                                                 market_update_info_.asklevels_[1].limit_int_price_ > kInvalidIntPrice)
                                                    ? market_update_info_.asklevels_[1].limit_int_price_
                                                    : market_update_info_.asklevels_[0].limit_int_price_);
      market_update_info_.bestask_ordercount_ =
          ((market_update_info_.asklevels_.size() > 1 && market_update_info_.asklevels_[1].limit_ordercount_ > 0)
               ? market_update_info_.asklevels_[1].limit_ordercount_
               : market_update_info_.asklevels_[0].limit_ordercount_);
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestask_price_ = price_to_yield_map_[market_update_info_.bestask_int_price_];
        hybrid_market_update_info_.bestask_size_ = market_update_info_.bestask_size_;
        hybrid_market_update_info_.bestask_int_price_ = market_update_info_.bestask_int_price_;
        hybrid_market_update_info_.bestask_ordercount_ = market_update_info_.bestask_ordercount_;
      }
    } else {  // if there are other orders at this level aprt from us
      // assign level 0 with reduced size
      market_update_info_.bestask_price_ = market_update_info_.asklevels_[0].limit_price_;
      market_update_info_.bestask_size_ = t_non_self_best_ask_size_;
      market_update_info_.bestask_int_price_ = market_update_info_.asklevels_[0].limit_int_price_;
      market_update_info_.bestask_ordercount_ = (t_non_self_best_ask_order_count_ > 0)
                                                    ? t_non_self_best_ask_order_count_
                                                    : 1;  // Transient bad cases reduce order_count to 0.
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestask_price_ = price_to_yield_map_[market_update_info_.bestask_int_price_];
        hybrid_market_update_info_.bestask_size_ = market_update_info_.bestask_size_;
        hybrid_market_update_info_.bestask_int_price_ = market_update_info_.bestask_int_price_;
        hybrid_market_update_info_.bestask_ordercount_ = market_update_info_.bestask_ordercount_;
      }
    }
  }
  if (remove_self_orders_from_book_ &&
      (dbglogger_.CheckLoggingLevel(SMVSELF_INFO) || dump_non_self_smv_on_next_update_) &&
      market_update_info_.bidlevels_.size() >= 1 && market_update_info_.asklevels_.size() >= 1) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " " << market_update_info_.shortcode_
                                << " MKT : " << market_update_info_.bidlevels_[0].limit_ordercount_ << " "
                                << market_update_info_.bidlevels_[0].limit_size_ << " "
                                << market_update_info_.bidlevels_[0].limit_int_price_ << " - "
                                << market_update_info_.asklevels_[0].limit_int_price_ << " "
                                << market_update_info_.asklevels_[0].limit_size_ << " "
                                << market_update_info_.asklevels_[0].limit_ordercount_
                                << " SELF-MKT : " << self_best_bid_ask_.best_bid_orders_ << " "
                                << self_best_bid_ask_.best_bid_size_ << " " << self_best_bid_ask_.best_bid_int_price_
                                << " - " << self_best_bid_ask_.best_ask_int_price_ << " "
                                << self_best_bid_ask_.best_ask_size_ << " " << self_best_bid_ask_.best_ask_orders_
                                << " NON-SELF-MKT : " << market_update_info_.bestbid_ordercount_ << " "
                                << market_update_info_.bestbid_size_ << " " << market_update_info_.bestbid_int_price_
                                << " - " << market_update_info_.bestask_int_price_ << " "
                                << market_update_info_.bestask_size_ << " " << market_update_info_.bestask_ordercount_
                                << DBGLOG_ENDL_FLUSH;

    dump_non_self_smv_on_next_update_ = false;
  }
}

void SecurityMarketView::SetTradeVarsForIndicatorsIfRequired() {
  if ((price_type_subscribed_[kPriceTypeTradeBasePrice]) || (price_type_subscribed_[kPriceTypeTradeMktSizeWPrice]) ||
      (price_type_subscribed_[kPriceTypeTradeMktSinPrice]) || (price_type_subscribed_[kPriceTypeTradeOrderWPrice]) ||
      (price_type_subscribed_[kPriceTypeTradeTradeWPrice]) || (price_type_subscribed_[kPriceTypeTradeOmixPrice])) {
    trade_basepx_struct_.OnTradeUpdate();
  }

  if (trade_print_info_.computing_trade_impact_) {
    trade_print_info_.trade_impact_ = 0.0;
    switch (trade_print_info_.buysell_) {
      case kTradeTypeBuy: {
        if (market_update_info_.pretrade_bestask_size_ > 0) {
          trade_print_info_.trade_impact_ =
              (std::max(0.0, std::min(1.0, ((double)trade_print_info_.size_traded_ /
                                            (double)market_update_info_.pretrade_bestask_size_))));
        }
      } break;
      case kTradeTypeSell: {
        if (market_update_info_.pretrade_bestbid_size_ > 0) {
          trade_print_info_.trade_impact_ =
              (std::max(0.0, std::min(1.0, ((double)trade_print_info_.size_traded_ /
                                            (double)market_update_info_.pretrade_bestbid_size_))));
        }
      } break;
      default:  // kTradeTypeNoInfo
      {
        if (trade_print_info_.int_trade_price_ == market_update_info_.pretrade_bestbid_int_price_) {
          if (market_update_info_.pretrade_bestbid_size_ > 0) {
            trade_print_info_.trade_impact_ =
                (std::max(0.0, std::min(1.0, ((double)trade_print_info_.size_traded_ /
                                              (double)market_update_info_.pretrade_bestbid_size_))));
          }
        }
        if (trade_print_info_.int_trade_price_ == market_update_info_.pretrade_bestask_int_price_) {
          if (market_update_info_.pretrade_bestask_size_ > 0) {
            trade_print_info_.trade_impact_ =
                (std::max(0.0, std::min(1.0, ((double)trade_print_info_.size_traded_ /
                                              (double)market_update_info_.pretrade_bestask_size_))));
          }
        }
      }
    }
  }

  if (trade_print_info_.computing_int_trade_type_) {
    switch (trade_print_info_.buysell_) {
      case kTradeTypeBuy:
        trade_print_info_.int_trade_type_ = +1;
        break;
      case kTradeTypeSell:
        trade_print_info_.int_trade_type_ = -1;
        break;
      default:
        trade_print_info_.int_trade_type_ = 0;
    }
  }

  if (trade_print_info_.computing_sqrt_size_traded_) {
    trade_print_info_.sqrt_size_traded_ = sqrt(trade_print_info_.size_traded_);
  }

  if (trade_print_info_.computing_sqrt_trade_impact_) {
    trade_print_info_.sqrt_trade_impact_ = sqrt(trade_print_info_.trade_impact_);
  }

  if (trade_print_info_.computing_tradepx_mktpx_diff_) {
#define MAX_VALUE_TDIFF_TICKS 5
    if (trade_before_quote_) {
      if (fabs(trade_print_info_.trade_price_ - market_update_info_.mkt_size_weighted_price_) <
          MAX_VALUE_TDIFF_TICKS * min_price_increment_) {
        trade_print_info_.tradepx_mktpx_diff_ =
            trade_print_info_.trade_price_ - market_update_info_.mkt_size_weighted_price_;
      } else {
        trade_print_info_.tradepx_mktpx_diff_ = 0;
      }
    } else {
      if (fabs(trade_print_info_.trade_price_ - market_update_info_.prequote_mkt_size_weighted_price_) <
          MAX_VALUE_TDIFF_TICKS * min_price_increment_) {
        trade_print_info_.tradepx_mktpx_diff_ =
            trade_print_info_.trade_price_ - market_update_info_.prequote_mkt_size_weighted_price_;
      } else {
        trade_print_info_.tradepx_mktpx_diff_ = 0;
      }
    }
#undef MAX_VALUE_TDIFF_TICKS
  }

  if (trade_print_info_.computing_last_book_tdiff_) {
    if (market_update_info_.last_book_mkt_size_weighted_price_ > -500) {
      trade_print_info_.last_book_tdiff_ =
          trade_print_info_.trade_price_ - market_update_info_.last_book_mkt_size_weighted_price_;
    } else {
      trade_print_info_.last_book_tdiff_ = 0;
    }
  }
}

void SecurityMarketView::UpdateL1Prices() {
  //@ravi - there is no need to go for double comparison

  if ((market_update_info_.bestbid_int_price_ == (kInvalidIntPrice)) ||
      (market_update_info_.bestask_int_price_ == (kInvalidIntPrice))) {
    is_ready_ = false;  // This is to ensure the listeners don't use Invalid prices if the side is cleared after
                        // setting is_ready at first
    return;
  }

  // The int price computed should always be in sync with the double values when calls are made for SetBestBid/Ask,
  // double comparison should be avoided unless required  - @ravi
  // In this case we are just not updating the computed prices but we are still sending ahead the update
  if (market_update_info_.bestask_int_price_ <= market_update_info_.bestbid_int_price_) {
    return;
  }

  UpdateBestBidInfoStruct();
  UpdateBestAskInfoStruct();

  market_update_info_.spread_increments_ =
      market_update_info_.bestask_int_price_ - market_update_info_.bestbid_int_price_;

  // update precomputed prices
  if (price_type_subscribed_[kPriceTypeMidprice]) {
    market_update_info_.mid_price_ = (market_update_info_.bestbid_price_ + market_update_info_.bestask_price_) / 2.0;
    hybrid_market_update_info_.mid_price_ =
        (hybrid_market_update_info_.bestbid_price_ + hybrid_market_update_info_.bestask_price_) / 2.0;

    if (!is_ready_ &&  // SMV not ready
        price_type_subscribed_[kPriceTypeTradeBasePrice] &&
        market_update_info_.trade_base_px_ < (kInvalidPrice + 1.5))  // Waiting on TradeWPrice ?
    {
      market_update_info_.trade_base_px_ = market_update_info_.mid_price_;
      hybrid_market_update_info_.trade_base_px_ = hybrid_market_update_info_.mid_price_;
    }
  }

  if (price_type_subscribed_[kPriceTypeMktSizeWPrice]) {  // if mktwpx subscribed to ... then compute it

    // this is added just for quote_before_trade exchanges like LIFFE
    if ((!trade_before_quote_) && (trade_print_info_.computing_tradepx_mktpx_diff_)) {  // in case we need to compute
                                                                                        // tdiff, this saves the
                                                                                        // mkt_size_weighted_price_
      // the last time UpdateL1Prices has been called .
      // Hence we we get the following updates : q(1):100 - 101, q(2):100 - 102, trd(1)101
      // then while processing q2, we save prequote_mkt_size_weighted_price_ based on q(1)
      market_update_info_.prequote_mkt_size_weighted_price_ = market_update_info_.mkt_size_weighted_price_;
    }

    // TODO change to normal_spread
    if (market_update_info_.spread_increments_ <= 1) {  // if spread is 1 tick then this
      market_update_info_.mkt_size_weighted_price_ =
          (market_update_info_.bestbid_price_ * market_update_info_.bestask_size_ +
           market_update_info_.bestask_price_ * market_update_info_.bestbid_size_) /
          (market_update_info_.bestbid_size_ + market_update_info_.bestask_size_);
      hybrid_market_update_info_.mkt_size_weighted_price_ =
          (hybrid_market_update_info_.bestbid_price_ * hybrid_market_update_info_.bestask_size_ +
           hybrid_market_update_info_.bestask_price_ * hybrid_market_update_info_.bestbid_size_) /
          (hybrid_market_update_info_.bestbid_size_ + hybrid_market_update_info_.bestask_size_);
    } else {  // else 50% midprice_ 50% mkt_size_weighted_price_
      market_update_info_.mkt_size_weighted_price_ =
          ((market_update_info_.bestbid_price_ * market_update_info_.bestask_size_ +
            market_update_info_.bestask_price_ * market_update_info_.bestbid_size_) /
               (market_update_info_.bestbid_size_ + market_update_info_.bestask_size_) +
           (market_update_info_.mid_price_)) /
          2.0;
      hybrid_market_update_info_.mkt_size_weighted_price_ =
          ((hybrid_market_update_info_.bestbid_price_ * hybrid_market_update_info_.bestask_size_ +
            hybrid_market_update_info_.bestask_price_ * hybrid_market_update_info_.bestbid_size_) /
               (hybrid_market_update_info_.bestbid_size_ + hybrid_market_update_info_.bestask_size_) +
           (hybrid_market_update_info_.mid_price_)) /
          2.0;
    }

    if (!is_ready_ &&  // SMV not ready
        price_type_subscribed_[kPriceTypeTradeMktSizeWPrice] &&
        market_update_info_.trade_mktsz_px_ < (kInvalidPrice + 1.5))  // Waiting on TradeWPrice ?
    {
      market_update_info_.trade_mktsz_px_ = market_update_info_.mkt_size_weighted_price_;
      hybrid_market_update_info_.trade_mktsz_px_ = hybrid_market_update_info_.mkt_size_weighted_price_;
    }
  }

  if (price_type_subscribed_[kPriceTypeProRataMktSizeWPrice]) {  // if Proratamktwpx subscribed to ... then compute it

    double ask_side_fraction = 1;
    double bid_side_fraction = 1;
    unsigned int ask_index = GetAskIndex(market_update_info_.bestask_int_price_);
    unsigned int prebook_ask_index = GetAskIndex(market_update_info_.prebook_bestask_int_price_);
    unsigned int bid_index = GetBidIndex(market_update_info_.bestbid_int_price_);
    unsigned int prebook_bid_index = GetBidIndex(market_update_info_.prebook_bestbid_int_price_);

    switch (market_update_info_.tradetype) {
      case kTradeTypeBuy: {
        // If level has not changed, update the level_fraction_, else just use the value stored.
        if (bid_index >= 0 && bid_index < market_update_info_.bidlevels_.size()) {
          if (market_update_info_.bestbid_price_ ==
              market_update_info_.prebook_bestbid_price_) {  // Level has not changed
            double size_changed = market_update_info_.bestbid_size_ - market_update_info_.prebook_bestbid_size_;
            // Size is decreased (order is deleted) because of last seen market size. One is not concerned about the
            // market size afer the order is deleted.
            // While in placing order, one is concered about the size after adding his order as he will get fill
            // according to the new size not old one.
            double size_accounted_while_change =
                (size_changed >= 0) ? market_update_info_.bestbid_size_ : market_update_info_.prebook_bestbid_size_;
            market_update_info_.bidlevels_[bid_index].level_fraction_ +=
                (double)size_changed / (double)size_accounted_while_change;
          } else {  // Level may have changed to some other existing level or a new level is formed
            if (prebook_bid_index >= 0 && prebook_bid_index < market_update_info_.bidlevels_.size()) {
              if (market_update_info_.bidlevels_[prebook_bid_index].limit_size_ <=
                  0) {  // Reset the fraction at last level which is now cleared
                market_update_info_.bidlevels_[prebook_bid_index].level_fraction_ = 1;
              }
            }
          }
          bid_side_fraction = market_update_info_.bidlevels_[bid_index].level_fraction_;
        }
        ask_side_fraction = market_update_info_.asklevels_[ask_index].level_fraction_;
      } break;
      case kTradeTypeSell: {
        if (ask_index >= 0 && ask_index < market_update_info_.asklevels_.size()) {
          if (market_update_info_.bestask_price_ ==
              market_update_info_.prebook_bestask_price_) {  // Level has not changed
            double size_changed = market_update_info_.bestask_size_ - market_update_info_.prebook_bestask_size_;
            double size_accounted_while_change =
                (size_changed >= 0) ? market_update_info_.bestask_size_ : market_update_info_.prebook_bestask_size_;
            market_update_info_.asklevels_[ask_index].level_fraction_ +=
                (double)size_changed / (double)size_accounted_while_change;
          } else {  // Level may have changed to some other existing level or a new level is formed
            if (prebook_ask_index >= 0 && prebook_ask_index < market_update_info_.asklevels_.size()) {
              if (market_update_info_.asklevels_[prebook_ask_index].limit_size_ <=
                  0) {  // Reset the fraction at last level which is now cleared
                market_update_info_.asklevels_[prebook_ask_index].level_fraction_ = 1;
              }
            }
          }
          ask_side_fraction = market_update_info_.asklevels_[ask_index].level_fraction_;
        }
        bid_side_fraction = market_update_info_.bidlevels_[bid_index].level_fraction_;
      } break;
      default: {
        bid_side_fraction = market_update_info_.bidlevels_[bid_index].level_fraction_;
        ask_side_fraction = market_update_info_.asklevels_[ask_index].level_fraction_;
      } break;
    }
    bid_side_fraction = (bid_side_fraction < 0) ? 0 : bid_side_fraction;
    ask_side_fraction = (ask_side_fraction < 0) ? 0 : ask_side_fraction;

    if ((bid_side_fraction == 1 && ask_side_fraction == 1) ||
        (bid_side_fraction == 0 && ask_side_fraction == 0)) {  // Revert to MktWtPrice if the fractions on both side are
                                                               // 1. Because 1 can be because of the default value which
                                                               // we set.
      market_update_info_.pro_rata_mkt_size_weighted_price_ = market_update_info_.mkt_size_weighted_price_;
      hybrid_market_update_info_.pro_rata_mkt_size_weighted_price_ =
          hybrid_market_update_info_.mkt_size_weighted_price_;
    } else {  // Use fractions in place of sizes to compute the price as compared to MktSizeWPrice
      // TODO change to normal_spread
      if (market_update_info_.spread_increments_ <= 1) {  // if spread is 1 tick then this
        market_update_info_.pro_rata_mkt_size_weighted_price_ =
            (market_update_info_.bestbid_price_ * ask_side_fraction +
             market_update_info_.bestask_price_ * bid_side_fraction) /
            (bid_side_fraction + ask_side_fraction);
        hybrid_market_update_info_.pro_rata_mkt_size_weighted_price_ =
            (hybrid_market_update_info_.bestbid_price_ * ask_side_fraction +
             hybrid_market_update_info_.bestask_price_ * bid_side_fraction) /
            (bid_side_fraction + ask_side_fraction);
      } else {  // else 50% midprice_ 50% pro_rata_mkt_size_weighted_price_
        market_update_info_.pro_rata_mkt_size_weighted_price_ =
            ((market_update_info_.bestbid_price_ * ask_side_fraction +
              market_update_info_.bestask_price_ * bid_side_fraction) /
                 (bid_side_fraction + ask_side_fraction) +
             (market_update_info_.mid_price_)) /
            2.0;
        hybrid_market_update_info_.pro_rata_mkt_size_weighted_price_ =
            ((hybrid_market_update_info_.bestbid_price_ * ask_side_fraction +
              hybrid_market_update_info_.bestask_price_ * bid_side_fraction) /
                 (bid_side_fraction + ask_side_fraction) +
             (hybrid_market_update_info_.mid_price_)) /
            2.0;
      }
    }
  }

  if (price_type_subscribed_[kPriceTypeMktSinusoidal]) {  // if mkt - sinusoidal price needed then compute it

    // TODO change to normal_spread
    if (market_update_info_.spread_increments_ <= 1) {  // if spread is 1 tick then this

      /// tilt ... top level book pressure ... if > 0 then bid is heavier and hence price shoudl be towards offer
      register double size_tilt = (double)(market_update_info_.bestbid_size_ - market_update_info_.bestask_size_) /
                                  (double)(market_update_info_.bestbid_size_ + market_update_info_.bestask_size_);

      market_update_info_.mkt_sinusoidal_price_ =
          (market_update_info_.mid_price_) +
          (((market_update_info_.bestask_price_ - market_update_info_.bestbid_price_) / 2.0) * size_tilt * size_tilt *
           size_tilt);
      hybrid_market_update_info_.mkt_sinusoidal_price_ =
          (hybrid_market_update_info_.mid_price_) +
          (((hybrid_market_update_info_.bestask_price_ - hybrid_market_update_info_.bestbid_price_) / 2.0) * size_tilt *
           size_tilt * size_tilt);
    } else {  // else just mid_price_
      market_update_info_.mkt_sinusoidal_price_ = market_update_info_.mid_price_;
      hybrid_market_update_info_.mkt_sinusoidal_price_ = hybrid_market_update_info_.mid_price_;
    }

    if (!is_ready_ &&  // SMV not ready
        price_type_subscribed_[kPriceTypeTradeMktSinPrice] &&
        market_update_info_.trade_mktsin_px_ < (kInvalidPrice + 1.5))  // Waiting on TradeWPrice ?
    {
      market_update_info_.trade_mktsin_px_ = market_update_info_.mkt_sinusoidal_price_;
      hybrid_market_update_info_.trade_mktsin_px_ = hybrid_market_update_info_.mkt_sinusoidal_price_;
    }
  }

  if (price_type_subscribed_[kPriceTypeOrderWPrice]) {  // if orderwprice price needed then compute it

    // TODO change to normal_spread
    if (market_update_info_.spread_increments_ <= 1) {  // if spread is 1 tick then this

      /// tilt ... top level book pressure ... if > 0 then bid is heavier and hence price shoudl be towards offer
      register double size_tilt =
          (double)(market_update_info_.bestbid_ordercount_ - market_update_info_.bestask_ordercount_) /
          (double)(market_update_info_.bestbid_ordercount_ + market_update_info_.bestask_ordercount_);

      market_update_info_.order_weighted_price_ =
          market_update_info_.mid_price_ +
          (((market_update_info_.bestask_price_ - market_update_info_.bestbid_price_) / 2.0) * size_tilt);
      hybrid_market_update_info_.order_weighted_price_ =
          hybrid_market_update_info_.mid_price_ +
          (((hybrid_market_update_info_.bestask_price_ - hybrid_market_update_info_.bestbid_price_) / 2.0) * size_tilt);
    } else {  // else just mid_price_
      market_update_info_.order_weighted_price_ = market_update_info_.mid_price_;
      hybrid_market_update_info_.order_weighted_price_ = hybrid_market_update_info_.mid_price_;
    }

    if (!is_ready_ &&  // SMV not ready
        price_type_subscribed_[kPriceTypeTradeOrderWPrice] &&
        market_update_info_.trade_orderw_px_ < (kInvalidPrice + 1.5))  // Waiting on TradeWPrice ?
    {
      market_update_info_.trade_orderw_px_ = market_update_info_.order_weighted_price_;
      hybrid_market_update_info_.trade_orderw_px_ = hybrid_market_update_info_.order_weighted_price_;
    }
  }

  if (price_type_subscribed_[kPriceTypeTradeWPrice]) {
    if (!is_ready_ ||                                                 // SMV not ready
        ((trade_print_info_.trade_price_ < (kInvalidPrice + 1.5)) &&  // Waiting on Trade
         (price_to_yield_map_.empty() || (hybrid_trade_print_info_.trade_price_ <
                                          (kInvalidPrice + 1.5)))))  // Waiting on hybrid trade if map is non-empty
    {
      market_update_info_.trade_weighted_price_ = market_update_info_.mid_price_;
      hybrid_market_update_info_.trade_weighted_price_ = hybrid_market_update_info_.mid_price_;
    } else {
      double t_trade_price_ = std::max(market_update_info_.bestbid_price_,
                                       std::min(market_update_info_.bestask_price_, trade_print_info_.trade_price_));
      market_update_info_.trade_weighted_price_ = (0.5 * t_trade_price_ + 0.5 * market_update_info_.mid_price_);
      if (!price_to_yield_map_.empty()) {
        double t_hybrid_trade_price_ =
            std::max(hybrid_market_update_info_.bestbid_price_,
                     std::min(hybrid_market_update_info_.bestask_price_,
                              price_to_yield_map_[GetIntPx(hybrid_trade_print_info_.trade_price_)]));
        hybrid_market_update_info_.trade_weighted_price_ =
            (0.5 * t_hybrid_trade_price_ + 0.5 * hybrid_market_update_info_.mid_price_);
      }
    }

    if (!is_ready_ &&  // SMV not ready
        price_type_subscribed_[kPriceTypeTradeTradeWPrice] &&
        market_update_info_.trade_tradew_px_ < (kInvalidPrice + 1.5))  // Waiting on TradeWPrice ?
    {
      market_update_info_.trade_tradew_px_ = market_update_info_.trade_weighted_price_;
      hybrid_market_update_info_.trade_tradew_px_ = hybrid_market_update_info_.trade_weighted_price_;
    }
  }

  if (price_type_subscribed_[kPriceTypeOfflineMixMMS]) {
    RecomputeOfflineMixPrice();
    if (!is_ready_ &&  // SMV not ready
        price_type_subscribed_[kPriceTypeTradeOmixPrice] &&
        market_update_info_.trade_omix_px_ < (kInvalidPrice + 1.5))  // Waiting on TradeWPrice ?
    {
      market_update_info_.trade_omix_px_ = market_update_info_.offline_mix_mms_price_;
      hybrid_market_update_info_.trade_omix_px_ = hybrid_market_update_info_.offline_mix_mms_price_;
    }
  }
  if (price_type_subscribed_[kPriceTypeOnlineMixPrice]) {
    // It should be computed after Omix. Beacuse in default(when no const are present in file) case it will be same
    // as
    // omix px
    RecomputeOnlineMixPrice();
  }
  if (price_type_subscribed_[kPriceTypeValidLevelMidPrice]) {
    RecomputeValidLevelPrices();
  }

  if (price_type_subscribed_[kPriceTypeBandPrice]) {
    RecomputeBandPrices();
  }
  if (price_type_subscribed_[kPriceTypeStableBidPrice]) {
    RecomputeStableBidPrice();
  }

  if (price_type_subscribed_[kPriceTypeStableAskPrice]) {
    RecomputeStableAskPrice();
  }

  if (price_type_subscribed_[kPriceTypeImpliedVol]) {
    RecomputeImpliedVol();
  }

  if (price_type_subscribed_[kPriceTypeTradeBasePrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }

      market_update_info_.trade_base_px_ = (1 - alpha) * market_update_info_.mid_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_base_px_ = market_update_info_.mid_price_;
    }
    hybrid_market_update_info_.trade_base_px_ = hybrid_market_update_info_.mid_price_;
  }
  if (price_type_subscribed_[kPriceTypeTradeMktSizeWPrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }

      market_update_info_.trade_mktsz_px_ =
          (1 - alpha) * market_update_info_.mkt_size_weighted_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_mktsz_px_ = market_update_info_.mkt_size_weighted_price_;
    }
    hybrid_market_update_info_.trade_mktsz_px_ = hybrid_market_update_info_.mkt_size_weighted_price_;
  }
  if (price_type_subscribed_[kPriceTypeTradeMktSinPrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }

      market_update_info_.trade_mktsin_px_ =
          (1 - alpha) * market_update_info_.mkt_sinusoidal_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_mktsin_px_ = market_update_info_.mkt_sinusoidal_price_;
    }
    hybrid_market_update_info_.trade_mktsin_px_ = hybrid_market_update_info_.mkt_sinusoidal_price_;
  }
  if (price_type_subscribed_[kPriceTypeTradeOrderWPrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }

      market_update_info_.trade_orderw_px_ =
          (1 - alpha) * market_update_info_.order_weighted_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_orderw_px_ = market_update_info_.order_weighted_price_;
    }
    hybrid_market_update_info_.trade_orderw_px_ = hybrid_market_update_info_.order_weighted_price_;
  }
  if (price_type_subscribed_[kPriceTypeTradeTradeWPrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }

      market_update_info_.trade_tradew_px_ =
          (1 - alpha) * market_update_info_.trade_weighted_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_tradew_px_ = market_update_info_.trade_weighted_price_;
    }
    hybrid_market_update_info_.trade_tradew_px_ = hybrid_market_update_info_.trade_weighted_price_;
  }
  if (price_type_subscribed_[kPriceTypeTradeOmixPrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }

      market_update_info_.trade_omix_px_ =
          (1 - alpha) * market_update_info_.offline_mix_mms_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_omix_px_ = market_update_info_.offline_mix_mms_price_;
    }
    hybrid_market_update_info_.trade_omix_px_ = hybrid_market_update_info_.offline_mix_mms_price_;
  }
  if (price_type_subscribed_[kPriceTypeOrderSizeWPrice]) {  // if ordersizewprice price needed then compute it
    // 5SizeOrder Vs 25SizeOrder
    // TODO change to normal_spread
    double a_push_ = (market_update_info_.bestask_size_ / std::max(1, market_update_info_.bestask_ordercount_));
    double b_push_ = (market_update_info_.bestbid_size_ / std::max(1, market_update_info_.bestbid_ordercount_));

    a_push_ = std::max(a_push_, 1.00);  // psarthy April 11th 2016 - to make sure this number is never zero
    b_push_ = std::max(b_push_, 1.00);

    if (market_update_info_.spread_increments_ <= 1) {  // if spread is 1 tick then this
      market_update_info_.order_size_weighted_price_ =
          (b_push_ * market_update_info_.bestask_price_ + a_push_ * market_update_info_.bestbid_price_) /
          (a_push_ + b_push_);
    } else {  // else share with mid_price_
      market_update_info_.order_size_weighted_price_ =
          market_update_info_.mid_price_ * 0.5 +
          (b_push_ * market_update_info_.bestask_price_ + a_push_ * market_update_info_.bestbid_price_) /
              (a_push_ + b_push_) * 0.5;
    }
    hybrid_market_update_info_.order_size_weighted_price_ =
        hybrid_market_update_info_.mid_price_ * 0.5 +
        (b_push_ * hybrid_market_update_info_.bestask_price_ + a_push_ * hybrid_market_update_info_.bestbid_price_) /
            (a_push_ + b_push_) * 0.5;
  }
}

int SecurityMarketView::AddNonTopAsk(int t_level_added_, MarketUpdateInfoLevelStruct &new_level_) {
  if (t_level_added_ > DEF_MARKET_DEPTH) {
    return -1;
  }

  int i = 1;
  std::vector<MarketUpdateInfoLevelStruct>::iterator t_iter_ = market_update_info_.asklevels_.begin();
  for (i = 1; ((i < t_level_added_) && (t_iter_ != market_update_info_.asklevels_.end()) &&
               (new_level_.limit_int_price_ > t_iter_->limit_int_price_));
       i++, t_iter_++) {
    // t_iter_ ++;
  }

  market_update_info_.asklevels_.insert(t_iter_, new_level_);
  if (market_update_info_.asklevels_.size() > DEF_MARKET_DEPTH) {
    market_update_info_.asklevels_.pop_back();
  }
  return i;
}

int SecurityMarketView::AddNonTopBid(int t_level_added_, MarketUpdateInfoLevelStruct &new_level_) {
  if (t_level_added_ > DEF_MARKET_DEPTH) {
    return -1;
  }

  std::vector<MarketUpdateInfoLevelStruct>::iterator t_iter_ = market_update_info_.bidlevels_.begin();
  int i;
  for (i = 1; ((i < t_level_added_) && (t_iter_ != market_update_info_.bidlevels_.end()) &&
               (new_level_.limit_int_price_ < t_iter_->limit_int_price_));
       i++, t_iter_++) {
    // t_iter_ ++;
  }

  market_update_info_.bidlevels_.insert(t_iter_, new_level_);
  if (market_update_info_.bidlevels_.size() > DEF_MARKET_DEPTH) {
    market_update_info_.bidlevels_.pop_back();
  }
  return i;
}

/// unlike NumAskLevels this function has an input max beyond with the value is not of interest
/// used to optimize for indexed book
unsigned int SecurityMarketView::MinValidNumAskLevels(unsigned int t_max_interested_value_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    unsigned int num_valid_levels_ = 0;
    for (unsigned int current_ask_index_ = GetBaseAskMapIndex(); current_ask_index_ > 0;
         current_ask_index_--) {  // not optimized
      if (market_update_info_.asklevels_[current_ask_index_].limit_size_ > 0) {
        num_valid_levels_++;
      }
      if (num_valid_levels_ >= t_max_interested_value_) {
        return num_valid_levels_;
      }
    }
    return num_valid_levels_;
  } else {
    return std::min((long unsigned int)t_max_interested_value_, market_update_info_.asklevels_.size());
  }
}

/// unlike NumBidLevels this function has an input max beyond with the value is not of interest
/// used to optimize for indexed book
unsigned int SecurityMarketView::MinValidNumBidLevels(unsigned int t_max_interested_value_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    unsigned int num_valid_levels_ = 0;
    for (unsigned int current_bid_index_ = GetBaseBidMapIndex(); current_bid_index_ > 0;
         current_bid_index_--) {  // not optimized
      if (market_update_info_.bidlevels_[current_bid_index_].limit_size_ > 0) {
        num_valid_levels_++;
      }
      if (num_valid_levels_ >= t_max_interested_value_) {
        return num_valid_levels_;
      }
    }
    return num_valid_levels_;
  } else {
    return std::min((long unsigned int)t_max_interested_value_, market_update_info_.bidlevels_.size());
  }
}

void SecurityMarketView::StorePreBook(TradeType_t t_buysell_) {
  if (price_type_subscribed_[kPriceTypeProRataMktSizeWPrice]) {
    market_update_info_.tradetype = t_buysell_;  // tradetype is stored for the current trade only not of previous state
                                                 // as opposed to the function definition.
    switch (t_buysell_) {
      case kTradeTypeBuy: {
        market_update_info_.prebook_bestbid_size_ = market_update_info_.bestbid_size_;
        market_update_info_.prebook_bestbid_price_ = market_update_info_.bestbid_price_;
        market_update_info_.prebook_bestbid_int_price_ = market_update_info_.bestbid_int_price_;
      } break;
      case kTradeTypeSell: {
        market_update_info_.prebook_bestask_size_ = market_update_info_.bestask_size_;
        market_update_info_.prebook_bestask_price_ = market_update_info_.bestask_price_;
        market_update_info_.prebook_bestask_int_price_ = market_update_info_.bestask_int_price_;
      } break;
      default:
        break;
    }
  }
}

void SecurityMarketView::StorePreTrade() {
  if (market_update_info_.storing_pretrade_state_) {
    market_update_info_.pretrade_bestbid_price_ = market_update_info_.bestbid_price_;
    market_update_info_.pretrade_bestbid_int_price_ = market_update_info_.bestbid_int_price_;
    market_update_info_.pretrade_bestbid_size_ = market_update_info_.bestbid_size_;
    market_update_info_.pretrade_bestask_price_ = market_update_info_.bestask_price_;
    market_update_info_.pretrade_bestask_int_price_ = market_update_info_.bestask_int_price_;
    market_update_info_.pretrade_bestask_size_ = market_update_info_.bestask_size_;
    market_update_info_.pretrade_mid_price_ = market_update_info_.mid_price_;
    hybrid_market_update_info_.pretrade_bestbid_price_ = hybrid_market_update_info_.bestbid_price_;
    hybrid_market_update_info_.pretrade_bestbid_int_price_ = hybrid_market_update_info_.bestbid_int_price_;
    hybrid_market_update_info_.pretrade_bestbid_size_ = hybrid_market_update_info_.bestbid_size_;
    hybrid_market_update_info_.pretrade_bestask_price_ = hybrid_market_update_info_.bestask_price_;
    hybrid_market_update_info_.pretrade_bestask_int_price_ = hybrid_market_update_info_.bestask_int_price_;
    hybrid_market_update_info_.pretrade_bestask_size_ = hybrid_market_update_info_.bestask_size_;
    hybrid_market_update_info_.pretrade_mid_price_ = hybrid_market_update_info_.mid_price_;
  }
}

void SecurityMarketView::SetBestLevelBidVariablesOnHit() {
  if (prev_bid_was_quote_) {
    VectorUtils::FillInValue(running_hit_size_vec_, 0);
    prev_bid_was_quote_ = false;
    top_bid_level_to_mask_trades_on_ = 0;
  }

  if (market_update_info_.bidlevels_.size() == 0) return;

  while ((trade_print_info_.int_trade_price_ <
          market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_int_price_) &&
         (top_bid_level_to_mask_trades_on_ <
          std::min(DEF_MARKET_DEPTH - 1, (int)market_update_info_.bidlevels_.size() - 1)))
    top_bid_level_to_mask_trades_on_++;

  if (trade_print_info_.int_trade_price_ ==
      market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_int_price_) {
    running_hit_size_vec_[top_bid_level_to_mask_trades_on_] += trade_print_info_.size_traded_;

    int t_trade_masked_best_bid_size_ = (market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_size_ -
                                         running_hit_size_vec_[top_bid_level_to_mask_trades_on_]);
    if (ors_exec_triggered_) {
      t_trade_masked_best_bid_size_ = std::min(t_trade_masked_best_bid_size_, market_update_info_.bestbid_size_);
    }
    if (t_trade_masked_best_bid_size_ <= 0) {  // even second level cleared
      // so set to third level
      if (top_bid_level_to_mask_trades_on_ <
          std::min(DEF_MARKET_DEPTH - 1, (int)market_update_info_.bidlevels_.size() - 1)) {
        top_bid_level_to_mask_trades_on_++;
        market_update_info_.bestbid_price_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_price_;
        market_update_info_.bestbid_size_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_size_;
        market_update_info_.bestbid_int_price_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_int_price_;
        market_update_info_.bestbid_ordercount_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_ordercount_;
        if (!price_to_yield_map_.empty()) {
          market_update_info_.bestbid_price_ =
              price_to_yield_map_[market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_price_];
          market_update_info_.bestbid_size_ =
              market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_size_;
          market_update_info_.bestbid_int_price_ =
              market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_int_price_;
          market_update_info_.bestbid_ordercount_ =
              market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_ordercount_;
        }
      } else {
        market_update_info_.bestbid_price_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_price_;
        market_update_info_.bestbid_size_ = 1;  // no change in prices
        market_update_info_.bestbid_int_price_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_int_price_;
        market_update_info_.bestbid_ordercount_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_ordercount_;
        if (!price_to_yield_map_.empty()) {
          market_update_info_.bestbid_price_ =
              price_to_yield_map_[market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_price_];
          market_update_info_.bestbid_size_ = 1;  // no change in prices
          market_update_info_.bestbid_int_price_ =
              market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_int_price_;
          market_update_info_.bestbid_ordercount_ =
              market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_ordercount_;
        }
      }
    } else {
      market_update_info_.bestbid_price_ =
          market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_price_;
      market_update_info_.bestbid_size_ = t_trade_masked_best_bid_size_;  // no change in prices
      market_update_info_.bestbid_int_price_ =
          market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_int_price_;
      market_update_info_.bestbid_ordercount_ =
          market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_ordercount_;
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestbid_price_ =
            price_to_yield_map_[market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_price_];
        hybrid_market_update_info_.bestbid_size_ = t_trade_masked_best_bid_size_;  // no change in prices
        hybrid_market_update_info_.bestbid_int_price_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_int_price_;
        hybrid_market_update_info_.bestbid_ordercount_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_ordercount_;
      }
    }
  } else if (trade_print_info_.int_trade_price_ <
                 market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_].limit_int_price_ &&
             market_update_info_.exch_source_ == HFSAT::kExchSourceHONGKONG)  // do this only for hk, ose has data
  // problems and this mod makes it worse
  {  // happens for highly volatile products with an aggressive trade at a non-existant price-level.
    if ((top_bid_level_to_mask_trades_on_ + 1) <
        std::min(DEF_MARKET_DEPTH - 1, (int)market_update_info_.bidlevels_.size() - 1)) {
      market_update_info_.bestbid_price_ =
          market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_ + 1].limit_price_;
      market_update_info_.bestbid_size_ =
          market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_ + 1].limit_size_;
      market_update_info_.bestbid_int_price_ =
          market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_ + 1].limit_int_price_;
      market_update_info_.bestbid_ordercount_ =
          market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_ + 1].limit_ordercount_;
      if (!price_to_yield_map_.empty()) {
        market_update_info_.bestbid_price_ =
            price_to_yield_map_[market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_ + 1].limit_price_];
        market_update_info_.bestbid_size_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_ + 1].limit_size_;
        market_update_info_.bestbid_int_price_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_ + 1].limit_int_price_;
        market_update_info_.bestbid_ordercount_ =
            market_update_info_.bidlevels_[top_bid_level_to_mask_trades_on_ + 1].limit_ordercount_;
      }
    } else {  // Trade price is outside the entire book !
      // produce a dummy bid level, one level below the traded price.
      market_update_info_.bestbid_price_ = trade_print_info_.trade_price_ - min_price_increment_;
      market_update_info_.bestbid_size_ = 1;
      market_update_info_.bestbid_int_price_ = trade_print_info_.int_trade_price_ - 1;
      market_update_info_.bestbid_ordercount_ = 1;
      if (!price_to_yield_map_.empty()) {
        market_update_info_.bestbid_price_ = price_to_yield_map_[trade_print_info_.trade_price_ - min_price_increment_];
        market_update_info_.bestbid_size_ = 1;
        market_update_info_.bestbid_int_price_ = trade_print_info_.int_trade_price_ - 1;
        market_update_info_.bestbid_ordercount_ = 1;
      }
    }
  }
}

/// If aggressive side of the Trade is kTradeTypeBuy
/// deduct running_lift_size_ from top level ask
void SecurityMarketView::SetBestLevelAskVariablesOnLift() {
  if (prev_ask_was_quote_) {
    VectorUtils::FillInValue(running_lift_size_vec_, 0);
    prev_ask_was_quote_ = false;
    top_ask_level_to_mask_trades_on_ = 0;
  }

  if (market_update_info_.asklevels_.size() == 0) {
    return;
  }

  while ((trade_print_info_.int_trade_price_ >
          market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_) &&
         (top_ask_level_to_mask_trades_on_ <
          std::min(DEF_MARKET_DEPTH - 1, (int)market_update_info_.asklevels_.size() - 1)))
    top_ask_level_to_mask_trades_on_++;

  if (trade_print_info_.int_trade_price_ ==
      market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_) {
    running_lift_size_vec_[top_ask_level_to_mask_trades_on_] += trade_print_info_.size_traded_;

    int t_trade_masked_best_ask_size_ = (market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_size_ -
                                         running_lift_size_vec_[top_ask_level_to_mask_trades_on_]);
    if (ors_exec_triggered_) {
      t_trade_masked_best_ask_size_ = std::min(t_trade_masked_best_ask_size_, market_update_info_.bestask_size_);
    }

    if (t_trade_masked_best_ask_size_ <= 0) {  // even second level cleared
      // so set to third level
      if (top_ask_level_to_mask_trades_on_ <
          std::min(DEF_MARKET_DEPTH - 1, (int)market_update_info_.asklevels_.size() - 1)) {  // no lower level ready
        top_ask_level_to_mask_trades_on_++;
        market_update_info_.bestask_price_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_price_;
        market_update_info_.bestask_size_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_size_;
        market_update_info_.bestask_int_price_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_;
        market_update_info_.bestask_ordercount_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_ordercount_;
        if (!price_to_yield_map_.empty()) {
          //            std::cout <<
          //            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_
          //            << "\n";
          hybrid_market_update_info_.bestask_price_ =
              price_to_yield_map_[market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_price_];
          hybrid_market_update_info_.bestask_size_ =
              market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_size_;
          hybrid_market_update_info_.bestask_int_price_ =
              market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_;
          hybrid_market_update_info_.bestask_ordercount_ =
              market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_ordercount_;
        }

        // if (dbglogger_.CheckLoggingLevel (BOOK_MINIMAL_TEST))
        //   {
        //     dbglogger_ << "masked_best_size < 0 whole level got cleared ?" << "\n";
        //     dbglogger_ <<market_update_info_.bestask_price_ << " " <<
        //    market_update_info_.bestask_size_ <<"  "<<
        //    market_update_info_.bestask_int_price_ <<" "<<
        //    market_update_info_.bestask_ordercount_ <<" "<<"\n";
        //     dbglogger_ << "@end multi level clear" << "\n";
        //     dbglogger_.DumpCurrentBuffer();
        //   }

      } else {
        market_update_info_.bestask_price_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_price_;
        market_update_info_.bestask_size_ = 1;  // no change in prices
        market_update_info_.bestask_int_price_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_;
        market_update_info_.bestask_ordercount_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_ordercount_;
        if (!price_to_yield_map_.empty()) {
          //            std::cout <<
          //            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_
          //            << "\n";
          hybrid_market_update_info_.bestask_price_ =
              price_to_yield_map_[market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_price_];
          hybrid_market_update_info_.bestask_size_ = 1;  // no change in prices
          hybrid_market_update_info_.bestask_int_price_ =
              market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_;
          hybrid_market_update_info_.bestask_ordercount_ =
              market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_ordercount_;
        }
      }
    } else {
      market_update_info_.bestask_price_ =
          market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_price_;
      market_update_info_.bestask_size_ = t_trade_masked_best_ask_size_;  // no change in prices
      market_update_info_.bestask_int_price_ =
          market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_;
      market_update_info_.bestask_ordercount_ =
          market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_ordercount_;
      if (!price_to_yield_map_.empty()) {
        //          std::cout << market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_
        //          <<
        //          "\n";
        hybrid_market_update_info_.bestask_price_ =
            price_to_yield_map_[market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_price_];
        hybrid_market_update_info_.bestask_size_ = t_trade_masked_best_ask_size_;  // no change in prices
        hybrid_market_update_info_.bestask_int_price_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_;
        hybrid_market_update_info_.bestask_ordercount_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_ordercount_;
      }
      // if (dbglogger_.CheckLoggingLevel (BOOK_MINIMAL_TEST))
      //        {
      //          dbglogger_ << "update bestvars" << "\n";
      //          dbglogger_ <<market_update_info_.bestask_price_ << " " <<
      //            market_update_info_.bestask_size_ <<"  "<<
      //            market_update_info_.bestask_int_price_ <<" "<<
      //            market_update_info_.bestask_ordercount_ <<" "<<"\n";
      //          dbglogger_ << "@end update best vars" << "\n";
      //          dbglogger_.DumpCurrentBuffer();
      //        }
    }
  } else if (trade_print_info_.int_trade_price_ >
                 market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_].limit_int_price_ &&
             market_update_info_.exch_source_ == HFSAT::kExchSourceHONGKONG)  // do this only for hk, ose has data
  // problems and this mod makes it worse
  {  // happens for highly volatile products with an aggressive trade at a non-existant price-level.
    if ((top_ask_level_to_mask_trades_on_ + 1) <
        std::min(DEF_MARKET_DEPTH - 1, (int)market_update_info_.asklevels_.size() - 1)) {
      market_update_info_.bestask_price_ =
          market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_ + 1].limit_price_;
      market_update_info_.bestask_size_ =
          market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_ + 1].limit_size_;
      market_update_info_.bestask_int_price_ =
          market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_ + 1].limit_int_price_;
      market_update_info_.bestask_ordercount_ =
          market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_ + 1].limit_ordercount_;
      if (!price_to_yield_map_.empty()) {
        //          std::cout << market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_ +
        //          1].limit_int_price_
        //          << "\n";
        hybrid_market_update_info_.bestask_price_ =
            price_to_yield_map_[market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_ + 1].limit_price_];
        hybrid_market_update_info_.bestask_size_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_ + 1].limit_size_;
        hybrid_market_update_info_.bestask_int_price_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_ + 1].limit_int_price_;
        hybrid_market_update_info_.bestask_ordercount_ =
            market_update_info_.asklevels_[top_ask_level_to_mask_trades_on_ + 1].limit_ordercount_;
      }
    } else {  // Trade price is outside the entire book !
      // produce a dummy ask level, one level above the traded price.
      market_update_info_.bestask_price_ = trade_print_info_.trade_price_ + min_price_increment_;
      market_update_info_.bestask_size_ = 1;
      market_update_info_.bestask_int_price_ = trade_print_info_.int_trade_price_ + 1;
      market_update_info_.bestask_ordercount_ = 1;
      if (!price_to_yield_map_.empty()) {
        //          std::cout << trade_print_info_.trade_price_ + min_price_increment_ << "\n";
        hybrid_market_update_info_.bestask_price_ =
            price_to_yield_map_[trade_print_info_.trade_price_ + min_price_increment_];
        hybrid_market_update_info_.bestask_size_ = 1;
        hybrid_market_update_info_.bestask_int_price_ = trade_print_info_.int_trade_price_ + 1;
        hybrid_market_update_info_.bestask_ordercount_ = 1;
      }
    }
  }
}

void SecurityMarketView::NotifyFastTradeListeners(const unsigned int t_security_id_, TradeType_t t_aggressive_side,
                                                  double t_last_traded_price) {
  for (auto it : fast_trade_listeners_) {
    it->OnFastTradeUpdate(t_security_id_, t_aggressive_side, t_last_traded_price);
  }
}

void SecurityMarketView::NotifyOnPLChangeListeners(const unsigned int t_security_id_,
                                                   const MarketUpdateInfo &r_market_update_info_,
                                                   const TradeType_t t_buysell_, const int t_level_changed_,
                                                   const int t_int_price_, const int t_int_price_level_,
                                                   const int t_old_size_, const int t_new_size_,
                                                   const int t_old_ordercount_, const int t_new_ordercount_,
                                                   const bool t_is_intermediate_message_, const char t_pl_notif)

{
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif
  // no callbacks until given time
  if (using_order_level_data_) {
    if (watch_.tv() <= skip_listener_notification_end_time_) return;
  }

  if (!pl_change_listeners_present_) return;

  switch (t_pl_notif) {
    case 'N':  // New
    {
      for (std::vector<SMVPLChangeListener *>::iterator iter = pl_change_listeners_.begin();
           iter != pl_change_listeners_.end(); iter++) {
        (*iter)->OnPLNew(t_security_id_, r_market_update_info_, t_buysell_, t_level_changed_, t_old_size_, t_new_size_,
                         t_old_ordercount_, t_new_ordercount_, t_int_price_, t_int_price_level_,
                         t_is_intermediate_message_);
      }
    } break;
    case 'D':  // Delete
    {
      for (std::vector<SMVPLChangeListener *>::iterator iter = pl_change_listeners_.begin();
           iter != pl_change_listeners_.end(); iter++) {
        (*iter)->OnPLDelete(t_security_id_, r_market_update_info_, t_buysell_, t_level_changed_, t_old_size_,
                            t_old_ordercount_, t_int_price_, t_int_price_level_, t_is_intermediate_message_);
      }
    } break;
    case 'C':  // change
    {
      for (std::vector<SMVPLChangeListener *>::iterator iter = pl_change_listeners_.begin();
           iter != pl_change_listeners_.end(); iter++) {
        (*iter)->OnPLChange(t_security_id_, r_market_update_info_, t_buysell_, t_level_changed_, t_int_price_,
                            t_int_price_level_, t_old_size_, t_new_size_, t_old_ordercount_, t_new_ordercount_,
                            t_is_intermediate_message_);
      }

    } break;
  }
}

void SecurityMarketView::NotifyOnReadyListeners() {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif
  if (!is_ready_) {
    return;
  }

  // no callbacks until given time
  if (using_order_level_data_) {
    if (watch_.tv() <= skip_listener_notification_end_time_) {
      return;
    }
  }

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(25);
#endif

  for (auto onready_listener : onready_listeners_) {
    onready_listener->SMVOnReady();
  }
  // SMVOnReady listeners usually exits if the by checking if they are ready
  // for further processing. In that case to the tag remains dangling and may
  // cause errors in profiling.
  if (HFSAT::CpucycleProfiler::GetUniqueInstance().current_tag_ == 25) {
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(25);
  }
}

void SecurityMarketView::NotifyL2OnlyListeners() {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif
  if (!is_ready_) {
    return;
  }

  // no callbacks until given time
  if (using_order_level_data_) {
    if (watch_.tv() <= skip_listener_notification_end_time_) {
      return;
    }
  }

  for (auto l2_only_listener : l2_only_listeners_) {
    if (!price_to_yield_map_.empty()) {
      l2_only_listener->OnMarketUpdateL2(security_id(), hybrid_market_update_info_);
    } else {
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
#endif
      l2_only_listener->OnMarketUpdateL2(security_id(), market_update_info_);
    }
  }
}

void SecurityMarketView::NotifyL2Listeners() {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif

  if (!is_ready_) {
    return;
  }
#ifdef L1_NOTIFICATION_CHANGE
  if (l1_notified_) {
    l1_notified_ = false;
    return;
  }
#endif
  // no callbacks until given time
  if (using_order_level_data_) {
    if (watch_.tv() <= skip_listener_notification_end_time_) {
      return;
    }
  }

  for (auto l2_listener : l2_listeners_) {
    if (!price_to_yield_map_.empty()) {
      l2_listener->OnMarketUpdate(security_id(), hybrid_market_update_info_);
    } else {
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
#endif
      l2_listener->OnMarketUpdate(security_id(), market_update_info_);
    }
  }
}

void SecurityMarketView::NotifyL1SizeListeners() {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif

  if (!is_ready_) {
    return;
  }

  // no callbacks until given time
  if (using_order_level_data_) {
    if (watch_.tv() <= skip_listener_notification_end_time_) {
      return;
    }
  }

  for (auto l1_sz_listener : l1_size_listeners_) {
    if (price_to_yield_map_.empty()) {
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
#endif
      l1_sz_listener->OnMarketUpdate(security_id(), market_update_info_);
    } else {
      l1_sz_listener->OnMarketUpdate(security_id(), hybrid_market_update_info_);
    }
  }

  market_update_info_.l1events_++;
  hybrid_market_update_info_.l1events_++;
#ifdef L1_NOTIFICATION_CHANGE
  l1_notified_ = true;
#endif
}

void SecurityMarketView::NotifyL1PriceListeners() {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif

  if (!is_ready_) {
    return;
  }

  // no callbacks until given time
  if (using_order_level_data_) {
    if (watch_.tv() <= skip_listener_notification_end_time_) {
      return;
    }
  }

  for (auto l1_px_listener : l1_price_listeners_) {
    if (price_to_yield_map_.empty()) {
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
#endif
      l1_px_listener->OnMarketUpdate(security_id(), market_update_info_);

    } else {
      l1_px_listener->OnMarketUpdate(security_id(), hybrid_market_update_info_);
    }
  }

  market_update_info_.l1events_++;
  hybrid_market_update_info_.l1events_++;

#ifdef L1_NOTIFICATION_CHANGE
  l1_notified_ = true;
#endif
}

void SecurityMarketView::NotifyRawTradeListeners() {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif

  if (!is_ready_) {
    return;
  }
  for (auto raw_trade_listener : raw_trade_listeners_) {
    if (price_to_yield_map_.empty()) {
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
#endif
      raw_trade_listener->OnRawTradePrint(security_id(), raw_trade_print_info_, market_update_info_);
    } else {
      raw_trade_listener->OnRawTradePrint(security_id(), raw_trade_print_info_, hybrid_market_update_info_);
    }
  }
}

void SecurityMarketView::NotifyTradeListeners() {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif

  if (!is_ready_) {
    return;
  }

  // This will ensure that mid price or other prices which are used here are set
  if ((market_update_info_.bestbid_int_price_ == (kInvalidIntPrice)) ||
      (market_update_info_.bestask_int_price_ == (kInvalidIntPrice))) {
    is_ready_ = false;  // This is to ensure the listeners don't use Invalid prices if the side is cleared after
                        // setting is_ready at first
    return;
  }

  // no callbacks until given time
  if (using_order_level_data_) {
    if (watch_.tv() <= skip_listener_notification_end_time_) {
      return;
    }
  }

  if (price_type_subscribed_[kPriceTypeTradeWPrice]) {
    double t_trade_price_ = std::max(market_update_info_.bestbid_price_,
                                     std::min(market_update_info_.bestask_price_, trade_print_info_.trade_price_));
    market_update_info_.trade_weighted_price_ = (0.5 * t_trade_price_ + 0.5 * market_update_info_.mid_price_);
    if (!price_to_yield_map_.empty()) {
      double t_hybrid_trade_price_ =
          std::max(hybrid_market_update_info_.bestbid_price_,
                   std::min(hybrid_market_update_info_.bestask_price_,
                            price_to_yield_map_[GetIntPx(hybrid_trade_print_info_.trade_price_)]));
      hybrid_market_update_info_.trade_weighted_price_ =
          (0.5 * t_hybrid_trade_price_ + 0.5 * hybrid_market_update_info_.mid_price_);
    }
  }
  if (price_type_subscribed_[kPriceTypeTradeBasePrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }
      market_update_info_.trade_base_px_ = (1 - alpha) * market_update_info_.mid_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_base_px_ = market_update_info_.mid_price_;
    }
    hybrid_market_update_info_.trade_base_px_ = hybrid_market_update_info_.mid_price_;
  }
  if (price_type_subscribed_[kPriceTypeTradeMktSizeWPrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }

      market_update_info_.trade_mktsz_px_ =
          (1 - alpha) * market_update_info_.mkt_size_weighted_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_mktsz_px_ = market_update_info_.mkt_size_weighted_price_;
    }
    hybrid_market_update_info_.trade_mktsz_px_ = hybrid_market_update_info_.mkt_size_weighted_price_;
  }
  if (price_type_subscribed_[kPriceTypeTradeMktSinPrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }
      market_update_info_.trade_mktsin_px_ =
          (1 - alpha) * market_update_info_.mkt_sinusoidal_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_mktsin_px_ = market_update_info_.mkt_sinusoidal_price_;
    }
    hybrid_market_update_info_.trade_mktsin_px_ = hybrid_market_update_info_.mkt_sinusoidal_price_;
  }
  if (price_type_subscribed_[kPriceTypeTradeOrderWPrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }

      market_update_info_.trade_orderw_px_ =
          (1 - alpha) * market_update_info_.order_weighted_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_orderw_px_ = market_update_info_.order_weighted_price_;
    }
    hybrid_market_update_info_.trade_orderw_px_ = hybrid_market_update_info_.order_weighted_price_;
  }
  if (price_type_subscribed_[kPriceTypeTradeTradeWPrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }

      market_update_info_.trade_tradew_px_ =
          (1 - alpha) * market_update_info_.trade_weighted_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_tradew_px_ = market_update_info_.trade_weighted_price_;
    }
    hybrid_market_update_info_.trade_tradew_px_ = hybrid_market_update_info_.trade_weighted_price_;
  }
  if (price_type_subscribed_[kPriceTypeOfflineMixMMS]) {  // Since OMix price also include TradeWPrice, it should be
                                                          // recomputed here.
    RecomputeOfflineMixPrice();
  }
  if (price_type_subscribed_[kPriceTypeTradeOmixPrice]) {
    if (compute_trade_prices_) {
      trade_basepx_struct_.UpdateVolumeAdjustedAlpha();
      double alpha = trade_basepx_struct_.alpha;
      double t_trade_px_ = trade_basepx_struct_.decayed_trade_px_;
      if (t_trade_px_ > market_update_info_.bestask_price_) {
        t_trade_px_ = market_update_info_.bestask_price_;
      } else if (t_trade_px_ < market_update_info_.bestbid_price_) {
        t_trade_px_ = market_update_info_.bestbid_price_;
      }
      market_update_info_.trade_omix_px_ =
          (1 - alpha) * market_update_info_.offline_mix_mms_price_ + alpha * t_trade_px_;
    } else {
      market_update_info_.trade_omix_px_ = market_update_info_.offline_mix_mms_price_;
    }
    hybrid_market_update_info_.trade_omix_px_ = hybrid_market_update_info_.offline_mix_mms_price_;
  }

  for (auto l1_px_listener : l1_price_listeners_) {
    if (price_to_yield_map_.empty()) {
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
#endif
      l1_px_listener->OnTradePrint(security_id(), trade_print_info_, market_update_info_);
    } else {
      l1_px_listener->OnTradePrint(security_id(), trade_print_info_, hybrid_market_update_info_);
    }
  }

  market_update_info_.l1events_++;
  market_update_info_.trades_count_[trade_print_info_.buysell_]++;
  hybrid_market_update_info_.l1events_++;
  hybrid_market_update_info_.trades_count_[trade_print_info_.buysell_]++;
}

void SecurityMarketView::NotifySpreadTradeListeners() {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif

  if (!is_ready_) {
    return;
  }

  // no callbacks until given time
  if (using_order_level_data_) {
    if (watch_.tv() <= skip_listener_notification_end_time_) {
      return;
    }
  }

  for (auto spread_listener : spread_trades_listeners_) {
    if (price_to_yield_map_.empty()) {
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
#endif
      spread_listener->OnTradePrint(security_id(), trade_print_info_, market_update_info_);
    } else {
      spread_listener->OnTradePrint(security_id(), trade_print_info_, hybrid_market_update_info_);
    }
  }
}

void SecurityMarketView::NotifyOTCTradeListeners(const double t_otc_trade_price_, const int t_otc_trade_size_) {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif

  if (!is_ready_) {
    return;
  }

  // no callbacks until given time
  if (using_order_level_data_) {
    if (watch_.tv() <= skip_listener_notification_end_time_) {
      return;
    }
  }

  for (auto otc_listener : otc_trades_listeners_) {
    if (price_to_yield_map_.empty()) {
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
#endif
      otc_listener->OnTradePrint(security_id(), trade_print_info_, market_update_info_);
    } else {
      otc_listener->OnTradePrint(security_id(), trade_print_info_, hybrid_market_update_info_);
    }
  }
}

const double &SecurityMarketView::GetPriceRef(const PriceType_t t_price_type_) const {
  switch (t_price_type_) {
    case kPriceTypeMidprice:
      return price_to_yield_map_.empty() ? market_update_info_.mid_price_ : hybrid_market_update_info_.mid_price_;
      break;
    case kPriceTypeMktSizeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.mkt_size_weighted_price_
                                         : hybrid_market_update_info_.mkt_size_weighted_price_;
      break;
    case kPriceTypeMktSinusoidal:
      return price_to_yield_map_.empty() ? market_update_info_.mkt_sinusoidal_price_
                                         : hybrid_market_update_info_.mkt_sinusoidal_price_;
      break;
    case kPriceTypeOrderWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.order_weighted_price_
                                         : hybrid_market_update_info_.order_weighted_price_;
      break;
    case kPriceTypeTradeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_weighted_price_
                                         : hybrid_market_update_info_.trade_weighted_price_;
      break;
    case kPriceTypeOfflineMixMMS:
      return price_to_yield_map_.empty() ? market_update_info_.offline_mix_mms_price_
                                         : hybrid_market_update_info_.offline_mix_mms_price_;
      break;
    case kPriceTypeValidLevelMidPrice:
      return price_to_yield_map_.empty() ? market_update_info_.valid_level_mid_price_
                                         : hybrid_market_update_info_.valid_level_mid_price_;
      break;
    case kPriceTypeBandPrice:
      return price_to_yield_map_.empty() ? market_update_info_.band_mkt_price_
                                         : hybrid_market_update_info_.band_mkt_price_;
    case kPriceTypeBidPrice:
      return price_to_yield_map_.empty() ? market_update_info_.bestbid_price_
                                         : hybrid_market_update_info_.bestbid_price_;
      break;
    case kPriceTypeAskPrice:
      return price_to_yield_map_.empty() ? market_update_info_.bestask_price_
                                         : hybrid_market_update_info_.bestask_price_;
      break;
    case kPriceTypeTradeBasePrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_base_px_
                                         : hybrid_market_update_info_.trade_base_px_;
      break;
    case kPriceTypeTradeMktSizeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_mktsz_px_
                                         : hybrid_market_update_info_.trade_mktsz_px_;
      break;
    case kPriceTypeTradeMktSinPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_mktsin_px_
                                         : hybrid_market_update_info_.trade_mktsin_px_;
      break;
    case kPriceTypeTradeOrderWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_orderw_px_
                                         : hybrid_market_update_info_.trade_orderw_px_;
      break;
    case kPriceTypeTradeTradeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_tradew_px_
                                         : hybrid_market_update_info_.trade_tradew_px_;
      break;
    case kPriceTypeTradeOmixPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_omix_px_
                                         : hybrid_market_update_info_.trade_omix_px_;
      break;
    case kPriceTypeOrderSizeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.order_size_weighted_price_
                                         : hybrid_market_update_info_.order_size_weighted_price_;
      break;
    case kPriceTypeOnlineMixPrice:
      return price_to_yield_map_.empty() ? market_update_info_.online_mix_price_
                                         : hybrid_market_update_info_.online_mix_price_;
      break;
    case kPriceTypeStableBidPrice:
      return price_to_yield_map_.empty() ? market_update_info_.stable_bid_price_
                                         : hybrid_market_update_info_.stable_bid_price_;
      break;
    case kPriceTypeStableAskPrice:
      return price_to_yield_map_.empty() ? market_update_info_.stable_ask_price_
                                         : hybrid_market_update_info_.stable_ask_price_;
      break;
    case kPriceTypeImpliedVol:
      return market_update_info_.implied_vol_;
      break;
    case kPriceTypeProRataMktSizeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.pro_rata_mkt_size_weighted_price_
                                         : hybrid_market_update_info_.pro_rata_mkt_size_weighted_price_;
      break;
    default:
      return price_to_yield_map_.empty() ? market_update_info_.mid_price_ : hybrid_market_update_info_.mid_price_;
  }
  return price_to_yield_map_.empty() ? market_update_info_.mid_price_ : hybrid_market_update_info_.mid_price_;
}

double SecurityMarketView::price_from_type(const PriceType_t t_price_type_) const {
  switch (t_price_type_) {
    case kPriceTypeMidprice:
      return price_to_yield_map_.empty() ? market_update_info_.mid_price_ : hybrid_market_update_info_.mid_price_;
      break;
    case kPriceTypeMktSizeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.mkt_size_weighted_price_
                                         : hybrid_market_update_info_.mkt_size_weighted_price_;
      break;
    case kPriceTypeMktSinusoidal:
      return price_to_yield_map_.empty() ? market_update_info_.mkt_sinusoidal_price_
                                         : hybrid_market_update_info_.mkt_sinusoidal_price_;
      break;
    case kPriceTypeOrderWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.order_weighted_price_
                                         : hybrid_market_update_info_.order_weighted_price_;
      break;
    case kPriceTypeTradeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_weighted_price_
                                         : hybrid_market_update_info_.trade_weighted_price_;
      break;
    case kPriceTypeOfflineMixMMS:
      return price_to_yield_map_.empty() ? market_update_info_.offline_mix_mms_price_
                                         : hybrid_market_update_info_.offline_mix_mms_price_;
      break;
    case kPriceTypeValidLevelMidPrice:
      return price_to_yield_map_.empty() ? market_update_info_.valid_level_mid_price_
                                         : hybrid_market_update_info_.valid_level_mid_price_;
      break;
    case kPriceTypeBandPrice:
      return price_to_yield_map_.empty() ? market_update_info_.band_mkt_price_
                                         : hybrid_market_update_info_.band_mkt_price_;
    case kPriceTypeBidPrice:
      return price_to_yield_map_.empty() ? market_update_info_.bestbid_price_
                                         : hybrid_market_update_info_.bestbid_price_;
      break;
    case kPriceTypeAskPrice:
      return price_to_yield_map_.empty() ? market_update_info_.bestask_price_
                                         : hybrid_market_update_info_.bestask_price_;
      break;
    case kPriceTypeTradeBasePrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_base_px_
                                         : hybrid_market_update_info_.trade_base_px_;
      break;
    case kPriceTypeTradeMktSizeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_mktsz_px_
                                         : hybrid_market_update_info_.trade_mktsz_px_;
      break;
    case kPriceTypeTradeMktSinPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_mktsin_px_
                                         : hybrid_market_update_info_.trade_mktsin_px_;
      break;
    case kPriceTypeTradeOrderWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_orderw_px_
                                         : hybrid_market_update_info_.trade_orderw_px_;
      break;
    case kPriceTypeTradeTradeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_tradew_px_
                                         : hybrid_market_update_info_.trade_tradew_px_;
      break;
    case kPriceTypeTradeOmixPrice:
      return price_to_yield_map_.empty() ? market_update_info_.trade_omix_px_
                                         : hybrid_market_update_info_.trade_omix_px_;
      break;
    case kPriceTypeOrderSizeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.order_size_weighted_price_
                                         : hybrid_market_update_info_.order_size_weighted_price_;
      break;
    case kPriceTypeOnlineMixPrice:
      return price_to_yield_map_.empty() ? market_update_info_.online_mix_price_
                                         : hybrid_market_update_info_.online_mix_price_;
      break;
    case kPriceTypeStableBidPrice:
      return price_to_yield_map_.empty() ? market_update_info_.stable_bid_price_
                                         : hybrid_market_update_info_.stable_bid_price_;
      break;
    case kPriceTypeStableAskPrice:
      return price_to_yield_map_.empty() ? market_update_info_.stable_ask_price_
                                         : hybrid_market_update_info_.stable_ask_price_;
      break;
    case kPriceTypeImpliedVol:
      return market_update_info_.implied_vol_;
      break;

    case kPriceTypeProRataMktSizeWPrice:
      return price_to_yield_map_.empty() ? market_update_info_.pro_rata_mkt_size_weighted_price_
                                         : hybrid_market_update_info_.pro_rata_mkt_size_weighted_price_;
      break;
    default:
      return price_to_yield_map_.empty() ? market_update_info_.mid_price_ : hybrid_market_update_info_.mid_price_;
  }
  return price_to_yield_map_.empty() ? market_update_info_.mid_price_ : hybrid_market_update_info_.mid_price_;
}

const double &SecurityMarketView::GetPriceRefFromType(const PriceType_t t_price_type_,
                                                      const MarketUpdateInfo &this_market_update_info_) {
  switch (t_price_type_) {
    case kPriceTypeMidprice:
      return this_market_update_info_.mid_price_;
      break;
    case kPriceTypeMktSizeWPrice:
      return this_market_update_info_.mkt_size_weighted_price_;
      break;
    case kPriceTypeMktSinusoidal:
      return this_market_update_info_.mkt_sinusoidal_price_;
      break;
    case kPriceTypeOrderWPrice:
      return this_market_update_info_.order_weighted_price_;
      break;
    case kPriceTypeTradeWPrice:
      return this_market_update_info_.trade_weighted_price_;
      break;
    case kPriceTypeOfflineMixMMS:
      return this_market_update_info_.offline_mix_mms_price_;
      break;
    case kPriceTypeValidLevelMidPrice:
      return this_market_update_info_.valid_level_mid_price_;
      break;
    case kPriceTypeBandPrice:
      return this_market_update_info_.band_mkt_price_;
      break;
    case kPriceTypeBidPrice:
      return this_market_update_info_.bestbid_price_;
      break;
    case kPriceTypeAskPrice:
      return this_market_update_info_.bestask_price_;
      break;
    case kPriceTypeTradeBasePrice:
      return this_market_update_info_.trade_base_px_;
      break;
    case kPriceTypeTradeMktSizeWPrice:
      return this_market_update_info_.trade_mktsz_px_;
      break;
    case kPriceTypeTradeMktSinPrice:
      return this_market_update_info_.trade_mktsin_px_;
      break;
    case kPriceTypeTradeOrderWPrice:
      return this_market_update_info_.trade_orderw_px_;
      break;
    case kPriceTypeTradeTradeWPrice:
      return this_market_update_info_.trade_tradew_px_;
      break;
    case kPriceTypeTradeOmixPrice:
      return this_market_update_info_.trade_omix_px_;
      break;
    case kPriceTypeOrderSizeWPrice:
      return this_market_update_info_.order_size_weighted_price_;
      break;
    case kPriceTypeOnlineMixPrice:
      return this_market_update_info_.online_mix_price_;
      break;
    case kPriceTypeStableBidPrice:
      return this_market_update_info_.stable_bid_price_;
      break;
    case kPriceTypeStableAskPrice:
      return this_market_update_info_.stable_ask_price_;
      break;
    case kPriceTypeImpliedVol:
      return this_market_update_info_.implied_vol_;
      break;
    case kPriceTypeProRataMktSizeWPrice:
      return this_market_update_info_.pro_rata_mkt_size_weighted_price_;
      break;
    default:
      return this_market_update_info_.mid_price_;
  }
  return this_market_update_info_.mid_price_;
}

double SecurityMarketView::GetPriceFromType(const PriceType_t t_price_type_,
                                            const MarketUpdateInfo &this_market_update_info_) {
  switch (t_price_type_) {
    case kPriceTypeMidprice:
      return this_market_update_info_.mid_price_;
      break;
    case kPriceTypeMktSizeWPrice:
      return this_market_update_info_.mkt_size_weighted_price_;
      break;
    case kPriceTypeMktSinusoidal:
      return this_market_update_info_.mkt_sinusoidal_price_;
      break;
    case kPriceTypeOrderWPrice:
      return this_market_update_info_.order_weighted_price_;
      break;
    case kPriceTypeTradeWPrice:
      return this_market_update_info_.trade_weighted_price_;
      break;
    case kPriceTypeOfflineMixMMS:
      return this_market_update_info_.offline_mix_mms_price_;
      break;
    case kPriceTypeValidLevelMidPrice:
      return this_market_update_info_.valid_level_mid_price_;
      break;
    case kPriceTypeBandPrice:
      return this_market_update_info_.band_mkt_price_;
      break;
    case kPriceTypeBidPrice:
      return this_market_update_info_.bestbid_price_;
      break;
    case kPriceTypeAskPrice:
      return this_market_update_info_.bestask_price_;
      break;
    case kPriceTypeTradeBasePrice:
      return this_market_update_info_.trade_base_px_;
      break;
    case kPriceTypeTradeMktSizeWPrice:
      return this_market_update_info_.trade_mktsz_px_;
      break;
    case kPriceTypeTradeMktSinPrice:
      return this_market_update_info_.trade_mktsin_px_;
      break;
    case kPriceTypeTradeOrderWPrice:
      return this_market_update_info_.trade_orderw_px_;
      break;
    case kPriceTypeTradeTradeWPrice:
      return this_market_update_info_.trade_tradew_px_;
      break;
    case kPriceTypeTradeOmixPrice:
      return this_market_update_info_.trade_omix_px_;
      break;
    case kPriceTypeOrderSizeWPrice:
      return this_market_update_info_.order_size_weighted_price_;
      break;
    case kPriceTypeOnlineMixPrice:
      return this_market_update_info_.online_mix_price_;
      break;
    case kPriceTypeStableBidPrice:
      return this_market_update_info_.stable_bid_price_;
      break;
    case kPriceTypeStableAskPrice:
      return this_market_update_info_.stable_ask_price_;
      break;
    case kPriceTypeImpliedVol:
      return this_market_update_info_.implied_vol_;
      break;
    case kPriceTypeProRataMktSizeWPrice:
      return this_market_update_info_.pro_rata_mkt_size_weighted_price_;
      break;
    default:
      return this_market_update_info_.mid_price_;
  }
  return this_market_update_info_.mid_price_;
}

std::string SecurityMarketView::MarketUpdateInfoToString() {
  std::ostringstream t_oss_;

  t_oss_ << std::setprecision(10) << "[" << this->shortcode() << " " << ((!IsBidBookEmpty()) ? this->bid_order(0) : 0)
         << " " << ((!IsBidBookEmpty()) ? this->bid_size(0) : 0) << " "
         << ((!IsBidBookEmpty()) ? this->bid_price(0) : 0) << " " << ((!IsAskBookEmpty()) ? this->ask_price(0) : 0)
         << " " << ((!IsAskBookEmpty()) ? this->ask_size(0) : 0) << " "
         << ((!IsAskBookEmpty()) ? this->ask_order(0) : 0) << "] "
         << "["
         << " " << market_update_info_.bestbid_ordercount_ << " " << market_update_info_.bestbid_size_ << " "
         << market_update_info_.bestbid_price_ << " " << market_update_info_.bestask_price_ << " "
         << market_update_info_.bestask_size_ << " " << market_update_info_.bestask_ordercount_ << "]";

  return t_oss_.str();
}

int SecurityMarketView::ask_order(unsigned int t_level_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (market_update_info_.compute_stable_levels_) {
      int size_seen_ = 0, orders_seen_ = 0;
      unsigned int current_ask_index_ =
          get_next_stable_ask_level_index(base_ask_index_, level_size_thresh_, size_seen_, orders_seen_);
      for (unsigned int level_counter_ = 0; current_ask_index_ > 0; level_counter_++) {
        if (level_counter_ >= t_level_) {
          return orders_seen_;
          // return market_update_info_.asklevels_[current_ask_index_] . limit_ordercount_;
        } else {
          size_seen_ = 0;
          orders_seen_ = 0;
          // get_next_stable_ask_level_index is called with decrementing 1 value to ignore this level from counting
          // Can't do this in the function itself because bid_level_ (0) might not be same in mkt_udpate_info_ and
          // here
          current_ask_index_ =
              get_next_stable_ask_level_index(current_ask_index_ - 1, level_size_thresh_, size_seen_, orders_seen_);
        }
      }
      return 0;
    } else {
      if (t_level_ == 0 && is_ready_) {
        return market_update_info_.bestask_ordercount_;
      }

      // Finds out the diff between the bestbid variable and the bestbid in the smv book.
      int level_diff = 0;
      if (is_ready_) {
        level_diff =
            market_update_info_.bestask_int_price_ - market_update_info_.asklevels_[base_ask_index_].limit_int_price_;
      }

      // If level_diff is < 0 ( possible ?), the smv book might not have info for that level.
      if (level_diff > 0) {
        t_level_ += level_diff;
      }

      if ((ask_access_bitmask_ & bitmask_access_lookup_table_[t_level_]) == bitmask_access_lookup_table_[t_level_]) {
        return (market_update_info_.asklevels_[base_ask_index_ - t_level_].limit_ordercount_);
      }

      if (base_ask_index_ == 0) return 0;
      if (t_level_ == 0) {  // top level
        return (market_update_info_.asklevels_[base_ask_index_].limit_ordercount_);
      } else {
        unsigned int current_ask_index_ = IndexedBookGetNextNonEmptyAskMapIndex(base_ask_index_);
        for (unsigned int level_counter_ = 1u; current_ask_index_ > 0; level_counter_++) {
          if (level_counter_ >= t_level_) {
            return market_update_info_.asklevels_[current_ask_index_].limit_ordercount_;
          } else {
            current_ask_index_ = IndexedBookGetNextNonEmptyAskMapIndex(current_ask_index_);
          }
        }
        return 0;
      }
    }
  } else {
    if (t_level_ < market_update_info_.asklevels_.size()) {
      return market_update_info_.asklevels_[t_level_].limit_ordercount_;
    }
  }
  return 0;
}

int SecurityMarketView::ask_order_at_int_price(int t_int_price_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    unsigned int temp_index_ =
        base_ask_index_ - (t_int_price_ - market_update_info_.asklevels_[base_ask_index_].limit_int_price_);
    if (temp_index_ < market_update_info_.asklevels_.size() && temp_index_ >= 0) {
      return market_update_info_.asklevels_[temp_index_].limit_ordercount_;
    }
  } else {
    for (int i = 0; i < NumAskLevels(); i++) {
      if (ask_int_price(i) == t_int_price_) {
        return ask_order(i);
      }
    }
  }
  return 0;
}

int SecurityMarketView::ask_size(unsigned int t_level_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (market_update_info_.compute_stable_levels_) {
      int size_seen_ = 0, orders_seen_ = 0;
      unsigned int current_ask_index_ =
          get_next_stable_ask_level_index(base_ask_index_, level_size_thresh_, size_seen_, orders_seen_);
      for (unsigned int level_counter_ = 0; current_ask_index_ > 0; level_counter_++) {
        if (level_counter_ >= t_level_) {
          return size_seen_;
          // return market_update_info_.asklevels_[current_ask_index_] . limit_size_;
        } else {
          size_seen_ = 0;
          orders_seen_ = 0;
          current_ask_index_ =
              get_next_stable_ask_level_index(current_ask_index_ - 1, level_size_thresh_, size_seen_, orders_seen_);
        }
      }
      return 0;
    } else {
      if (t_level_ == 0 && is_ready_) {
        return market_update_info_.bestask_size_;
      }

      // Finds out the diff between the bestbid variable and the bestbid in the smv book.
      int level_diff = 0;
      if (is_ready_) {
        level_diff =
            market_update_info_.bestask_int_price_ - market_update_info_.asklevels_[base_ask_index_].limit_int_price_;
      }

      // If level_diff is < 0 ( possible ?), the smv book might not have info for that level.
      if (level_diff > 0) {
        t_level_ += level_diff;
      }

      if ((ask_access_bitmask_ & bitmask_access_lookup_table_[t_level_]) == bitmask_access_lookup_table_[t_level_]) {
        return (market_update_info_.asklevels_[base_ask_index_ - t_level_].limit_size_);
      }

      if (base_ask_index_ == 0) return 0;
      if (t_level_ == 0) {  // top level
        return (market_update_info_.asklevels_[base_ask_index_].limit_size_);
      } else {
        unsigned int current_ask_index_ = IndexedBookGetNextNonEmptyAskMapIndex(base_ask_index_);
        for (unsigned int level_counter_ = 1u; current_ask_index_ > 0; level_counter_++) {
          if (level_counter_ >= t_level_) {
            return market_update_info_.asklevels_[current_ask_index_].limit_size_;
          } else {
            current_ask_index_ = IndexedBookGetNextNonEmptyAskMapIndex(current_ask_index_);
          }
        }
        return 0;
      }
    }
  } else {
    if (t_level_ < market_update_info_.asklevels_.size()) {
      return market_update_info_.asklevels_[t_level_].limit_size_;
    }
  }
  return 0;
}

/// TODO : evaluate and make special case for L1
int SecurityMarketView::ask_size_at_int_price(int t_int_price_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    unsigned int temp_index_ =
        base_ask_index_ - (t_int_price_ - market_update_info_.asklevels_[base_ask_index_].limit_int_price_);
    if (temp_index_ < market_update_info_.asklevels_.size() && temp_index_ >= 0) {
      return market_update_info_.asklevels_[temp_index_].limit_size_;
    }
  } else {
    for (int i = 0; i < NumAskLevels(); i++) {
      if (ask_int_price(i) == t_int_price_) {
        return ask_size(i);
      }
    }
  }
  return 0;
}

int SecurityMarketView::ask_level_at_int_price(int t_int_price_) const {
  int t_level_ = 0;
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ &&
      market_update_info_.compute_stable_levels_) {
    int this_index_ = base_ask_index_;
    int size_seen_ = 0, orders_seen_ = 0;
    while (t_int_price_ > market_update_info_.asklevels_[this_index_].limit_int_price_) {
      t_level_++;
      this_index_ = get_next_stable_ask_level_index(this_index_, level_size_thresh_, size_seen_, orders_seen_);
    }
  }
  return t_level_;
}

int SecurityMarketView::next_ask_level_int_price(int t_int_price_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ &&
      market_update_info_.compute_stable_levels_) {
    int size_seen_ = 0, orders_seen_ = 0;
    int this_index_ = get_next_stable_ask_level_index(base_ask_index_, level_size_thresh_, size_seen_, orders_seen_);
    while (t_int_price_ >= market_update_info_.asklevels_[this_index_].limit_int_price_ && this_index_ > 0) {
      this_index_ = get_next_stable_ask_level_index(this_index_ - 1, level_size_thresh_, size_seen_, orders_seen_);
    }
    return market_update_info_.asklevels_[this_index_].limit_int_price_;
  }
  return kInvalidIntPrice;
}

double SecurityMarketView::ask_price(unsigned int t_level_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (market_update_info_.compute_stable_levels_) {
      int size_seen_ = 0;
      int orders_seen_ = 0;
      unsigned int current_ask_index_ =
          get_next_stable_ask_level_index(base_ask_index_, level_size_thresh_, size_seen_, orders_seen_);
      for (unsigned int level_counter_ = 0; current_ask_index_ > 0; level_counter_++) {
        if (level_counter_ >= t_level_) {
          return market_update_info_.asklevels_[current_ask_index_].limit_price_;
        } else {
          size_seen_ = 0;
          orders_seen_ = 0;
          current_ask_index_ =
              get_next_stable_ask_level_index(current_ask_index_ - 1, level_size_thresh_, size_seen_, orders_seen_);
        }
      }
      return kInvalidPrice;
    } else {
      if (t_level_ == 0 && is_ready_) {
        return market_update_info_.bestask_price_;
      }

      // Finds out the diff between the bestbid variable and the bestbid in the smv book.
      int level_diff = 0;
      if (is_ready_) {
        level_diff =
            market_update_info_.bestask_int_price_ - market_update_info_.asklevels_[base_ask_index_].limit_int_price_;
      }

      // If level_diff is < 0 ( possible ?), the smv book might not have info for that level.
      if (level_diff > 0) {
        t_level_ += level_diff;
      }

      if ((ask_access_bitmask_ & bitmask_access_lookup_table_[t_level_]) == bitmask_access_lookup_table_[t_level_]) {
        return (market_update_info_.asklevels_[base_ask_index_ - t_level_].limit_price_);
      }

      if (base_ask_index_ == 0) return kInvalidPrice;
      if (t_level_ == 0) {  // top level
        return (market_update_info_.asklevels_[base_ask_index_].limit_price_);
      } else {
        unsigned int current_ask_index_ = IndexedBookGetNextNonEmptyAskMapIndex(base_ask_index_);
        for (unsigned int level_counter_ = 1u; current_ask_index_ > 0; level_counter_++) {
          if (level_counter_ >= t_level_) {
            return market_update_info_.asklevels_[current_ask_index_].limit_price_;
          } else {
            current_ask_index_ = IndexedBookGetNextNonEmptyAskMapIndex(current_ask_index_);
          }
        }
        return kInvalidPrice;
      }
    }
  } else {
    if (t_level_ < market_update_info_.asklevels_.size()) {
      return market_update_info_.asklevels_[t_level_].limit_price_;
    }
  }
  return kInvalidPrice;
}

int SecurityMarketView::ask_int_price(unsigned int t_level_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (market_update_info_.compute_stable_levels_) {
      int size_seen_ = 0;
      int orders_seen_ = 0;
      unsigned int current_ask_index_ =
          get_next_stable_ask_level_index(base_ask_index_, level_size_thresh_, size_seen_, orders_seen_);
      for (unsigned int level_counter_ = 0; current_ask_index_ > 0; level_counter_++) {
        if (level_counter_ >= t_level_) {
          return market_update_info_.asklevels_[current_ask_index_].limit_int_price_;
        } else {
          size_seen_ = 0;
          orders_seen_ = 0;
          current_ask_index_ =
              get_next_stable_ask_level_index(current_ask_index_ - 1, level_size_thresh_, size_seen_, orders_seen_);
        }
      }
      return kInvalidIntPrice;
    } else {
      if (t_level_ == 0 && is_ready_) {
        return market_update_info_.bestask_int_price_;
      }

      // Finds out the diff between the bestbid variable and the bestbid in the smv book.
      int level_diff = 0;
      if (is_ready_) {
        level_diff =
            market_update_info_.bestask_int_price_ - market_update_info_.asklevels_[base_ask_index_].limit_int_price_;
      }

      // If level_diff is < 0 ( possible ?), the smv book might not have info for that level.
      if (level_diff > 0) {
        t_level_ += level_diff;
      }

      if ((ask_access_bitmask_ & bitmask_access_lookup_table_[t_level_]) == bitmask_access_lookup_table_[t_level_]) {
        return (market_update_info_.asklevels_[base_ask_index_ - t_level_].limit_int_price_);
      }

      if (base_ask_index_ == 0) return kInvalidIntPrice;
      if (t_level_ == 0) {
        return (market_update_info_.asklevels_[base_ask_index_].limit_int_price_);
      } else {
        unsigned int current_ask_index_ = IndexedBookGetNextNonEmptyAskMapIndex(base_ask_index_);
        for (unsigned int level_counter_ = 1u; current_ask_index_ > 0; level_counter_++) {
          if (level_counter_ >= t_level_) {
            return market_update_info_.asklevels_[current_ask_index_].limit_int_price_;
          } else {
            current_ask_index_ = IndexedBookGetNextNonEmptyAskMapIndex(current_ask_index_);
          }
        }
      }
    }
  } else {
    if (t_level_ < market_update_info_.asklevels_.size()) {
      return market_update_info_.asklevels_[t_level_].limit_int_price_;
    }
  }
  return kInvalidIntPrice;
}

int SecurityMarketView::ask_int_price_level(unsigned int t_level_) const {
  // top level is always at 0 int_price_level
  if (t_level_ == 0) {
    return 0;
  } else {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      if (base_ask_index_ == 0) return kInvalidIntPrice;
      // NOT VERY EFFICIENT ... better to call GetAskLevelAtIndex, and then calc
      // base_ask_index_.limit_int_price_ - current_level_int_price_
      if (is_ready_) {
        return (ask_int_price(t_level_) - market_update_info_.bestask_int_price_);
      } else {
        return (ask_int_price(t_level_) - market_update_info_.asklevels_[base_ask_index_].limit_int_price_);
      }
    } else {
      if (t_level_ < market_update_info_.asklevels_.size()) {
        return market_update_info_.asklevels_[t_level_].limit_int_price_level_;
      }
    }
  }
  return kInvalidIntPrice;
}

bool SecurityMarketView::IsAskBookEmpty() const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    int index = GetBaseAskMapIndex();
    if (index >= 0 && index < (int)market_update_info_.asklevels_.size()) {
      return (market_update_info_.asklevels_[index].limit_size_ <= 0);
    } else {
      return true;
    }
  } else {
    return market_update_info_.asklevels_.empty();
  }
}

bool SecurityMarketView::IsAskBookL2() const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (IndexedBookGetNextNonEmptyAskMapIndex(base_ask_index_) > 0) {
      return true;
    } else {
      return false;
    }
  } else {
    return (NumAskLevels() >= 2);
  }
}

unsigned int SecurityMarketView::asklevels_size() const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    unsigned int num_valid_levels_ = 0;
    int size_seen_so_far_ = 0;
    for (unsigned int current_ask_index_ = GetBaseAskMapIndex(); current_ask_index_ > 0;
         current_ask_index_--) {  // not optimized
      if (market_update_info_.compute_stable_levels_) {
        size_seen_so_far_ += market_update_info_.bidlevels_[current_ask_index_].limit_size_;
        if (size_seen_so_far_ >= level_size_thresh_) {
          num_valid_levels_++;
          size_seen_so_far_ = 0;
        }
      } else {
        if (market_update_info_.asklevels_[current_ask_index_].limit_size_ > 0) {
          num_valid_levels_++;
        }
      }
    }
    return num_valid_levels_;
  } else {
    return market_update_info_.asklevels_.size();
  }
}

unsigned int SecurityMarketView::get_next_stable_ask_level_index(unsigned int current_ask_index_,
                                                                 int _level_size_thresh_, int &size_seen_,
                                                                 int &orders_seen_) const {
  size_seen_ = 0;
  orders_seen_ = 0;
  for (; current_ask_index_ > 0; current_ask_index_--) {
    size_seen_ += market_update_info_.asklevels_[current_ask_index_].limit_size_;
    orders_seen_ += market_update_info_.asklevels_[current_ask_index_].limit_ordercount_;
    if (size_seen_ >= _level_size_thresh_) {
      return current_ask_index_;
    }
  }
  return 0;
}

unsigned int SecurityMarketView::IndexedBookGetNextNonEmptyBidMapIndex(unsigned int current_bid_index_) const {
  if (current_bid_index_ == 0u) {
    return 0u;
  }
  do {
    current_bid_index_--;
  } while ((current_bid_index_ > 0) && (market_update_info_.bidlevels_[current_bid_index_].limit_size_ <= 0));

  return current_bid_index_;
}

// generally this function is to access a level for some computation ..
// hence const reference access
const MarketUpdateInfoLevelStruct &SecurityMarketView::GetBidLevelAtIndex(unsigned int current_bid_index_) const {
  // market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_
  if (current_bid_index_ < market_update_info_.bidlevels_.size()) {
    if (is_ready_ && (GetBidIndex(market_update_info_.bestbid_int_price_) == (int)current_bid_index_)) {
      return best_bid_level_info_struct_;
    } else {
      return market_update_info_.bidlevels_[current_bid_index_];
    }
  } else {
    return dummy_market_update_info_level_struct_;
  }
}

// Indexed Book faster access functions
// takes an input the exact input index for indexed book
// finds the next index that is valid
unsigned int SecurityMarketView::GetNextBidMapIndex(unsigned int current_bid_index_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (market_update_info_.compute_stable_levels_) {
      int size_seen_ = 0, orders_seen_ = 0;
      return get_next_stable_bid_level_index(current_bid_index_, level_size_thresh_, size_seen_, orders_seen_);
    } else {
      return IndexedBookGetNextNonEmptyBidMapIndex(current_bid_index_);
    }
  } else {
    return current_bid_index_++;
  }
}

int SecurityMarketView::bid_order(unsigned int t_level_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (market_update_info_.compute_stable_levels_) {
      int size_seen_ = 0, orders_seen_ = 0;
      unsigned int current_bid_index_ =
          get_next_stable_bid_level_index(base_bid_index_, level_size_thresh_, size_seen_, orders_seen_);
      for (unsigned int level_counter_ = 0; current_bid_index_ > 0; level_counter_++) {
        if (level_counter_ >= t_level_) {
          return orders_seen_;
          // return market_update_info_.bidlevels_[current_bid_index_] . limit_ordercount_;
        } else {
          size_seen_ = 0;
          orders_seen_ = 0;
          current_bid_index_ =
              get_next_stable_bid_level_index(current_bid_index_ - 1, level_size_thresh_, size_seen_, orders_seen_);
        }
      }
      return 0;
    } else {
      if (t_level_ == 0 && is_ready_) {
        return market_update_info_.bestbid_ordercount_;
      }

      // Finds out the diff between the bestbid variable and the bestbid in the smv book.
      int level_diff = 0;
      if (is_ready_) {
        level_diff =
            market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ - market_update_info_.bestbid_int_price_;
      }

      // If level_diff is < 0 ( possible ?), the smv book might not have info for that level.
      if (level_diff > 0) {
        t_level_ += level_diff;
      }

      if ((bid_access_bitmask_ & bitmask_access_lookup_table_[t_level_]) == bitmask_access_lookup_table_[t_level_]) {
        return (market_update_info_.bidlevels_[base_bid_index_ - t_level_].limit_ordercount_);
      }

      if (base_bid_index_ == 0) return 0;
      if (t_level_ == 0) {  // top level
        return (market_update_info_.bidlevels_[base_bid_index_].limit_ordercount_);
      } else {
        unsigned int current_bid_index_ = IndexedBookGetNextNonEmptyBidMapIndex(base_bid_index_);
        for (unsigned int level_counter_ = 1u; current_bid_index_ > 0; level_counter_++) {
          if (level_counter_ >= t_level_) {
            return market_update_info_.bidlevels_[current_bid_index_].limit_ordercount_;
          } else {
            current_bid_index_ = IndexedBookGetNextNonEmptyBidMapIndex(current_bid_index_);
          }
        }
        return 0;
      }
    }
  } else {
    if (t_level_ < market_update_info_.bidlevels_.size()) {
      return market_update_info_.bidlevels_[t_level_].limit_ordercount_;
    }
  }
  return 0;
}

int SecurityMarketView::bid_order_at_int_price(int t_int_price_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    unsigned int temp_index_ =
        base_bid_index_ - (market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ - t_int_price_);
    if (temp_index_ < market_update_info_.bidlevels_.size() && temp_index_ >= 0) {
      return market_update_info_.bidlevels_[temp_index_].limit_ordercount_;
    }
  } else {
    for (auto i = 0u; i < market_update_info_.bidlevels_.size(); i++) {
      if (bid_int_price(i) == t_int_price_) {
        return bid_order(i);
      }
    }
  }
  return 0;
}

int SecurityMarketView::bid_size(unsigned int t_level_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (market_update_info_.compute_stable_levels_) {
      int size_seen_ = 0, orders_seen_ = 0;
      unsigned int current_bid_index_ =
          get_next_stable_bid_level_index(base_bid_index_, level_size_thresh_, size_seen_, orders_seen_);
      for (unsigned int level_counter_ = 0; current_bid_index_ > 0; level_counter_++) {
        if (level_counter_ >= t_level_) {
          return size_seen_;
          // return market_update_info_.bidlevels_[current_bid_index_] . limit_size_;
        } else {
          size_seen_ = 0;
          orders_seen_ = 0;
          current_bid_index_ =
              get_next_stable_bid_level_index(current_bid_index_ - 1, level_size_thresh_, size_seen_, orders_seen_);
        }
      }
      return 0;
    } else {
      if (t_level_ == 0 && is_ready_) {
        return market_update_info_.bestbid_size_;
      }

      // Finds out the diff between the bestbid variable and the bestbid in the smv book.
      int level_diff = 0;
      if (is_ready_) {
        level_diff =
            market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ - market_update_info_.bestbid_int_price_;
      }

      // If level_diff is < 0 ( possible ?), the smv book might not have info for that level.
      if (level_diff > 0) {
        t_level_ += level_diff;
      }

      if ((bid_access_bitmask_ & bitmask_access_lookup_table_[t_level_]) == bitmask_access_lookup_table_[t_level_]) {
        return (market_update_info_.bidlevels_[base_bid_index_ - t_level_].limit_size_);
      }

      if (base_bid_index_ == 0) return 0;
      if (t_level_ == 0) {  // top level
        return (market_update_info_.bidlevels_[base_bid_index_].limit_size_);
      } else {
        unsigned int current_bid_index_ = IndexedBookGetNextNonEmptyBidMapIndex(base_bid_index_);
        for (unsigned int level_counter_ = 1u; current_bid_index_ > 0; level_counter_++) {
          if (level_counter_ >= t_level_) {
            return market_update_info_.bidlevels_[current_bid_index_].limit_size_;
          } else {
            current_bid_index_ = IndexedBookGetNextNonEmptyBidMapIndex(current_bid_index_);
          }
        }
        return 0;
      }
    }
  } else {
    if (t_level_ < market_update_info_.bidlevels_.size()) {
      return market_update_info_.bidlevels_[t_level_].limit_size_;
    }
  }
  return 0;
}

/// TODO : evaluate and make special case for L1
int SecurityMarketView::bid_size_at_int_price(int t_int_price_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    unsigned int temp_index_ =
        base_bid_index_ - (market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ - t_int_price_);
    if (temp_index_ < market_update_info_.bidlevels_.size() && temp_index_ >= 0) {
      return market_update_info_.bidlevels_[temp_index_].limit_size_;
    }
  } else {
    for (auto i = 0u; i < market_update_info_.bidlevels_.size(); i++) {
      if (bid_int_price(i) == t_int_price_) {
        return bid_size(i);
      }
    }
  }
  return 0;
}

int SecurityMarketView::bid_level_at_int_price(int t_int_price_) const {
  int t_level_ = 0;
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ &&
      market_update_info_.compute_stable_levels_) {
    int this_index_ = base_bid_index_;
    int size_seen_ = 0, orders_seen_ = 0;
    while (t_int_price_ < market_update_info_.bidlevels_[this_index_].limit_int_price_) {
      t_level_++;
      this_index_ = get_next_stable_bid_level_index(this_index_, level_size_thresh_, size_seen_, orders_seen_);
    }
  }
  return t_level_;
}

int SecurityMarketView::next_bid_level_int_price(int t_int_price_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_ &&
      market_update_info_.compute_stable_levels_) {
    int size_seen_ = 0, orders_seen_ = 0;
    int this_index_ = get_next_stable_bid_level_index(base_bid_index_, level_size_thresh_, size_seen_, orders_seen_);
    while (t_int_price_ <= market_update_info_.bidlevels_[this_index_].limit_int_price_ && this_index_ > 0) {
      this_index_ = get_next_stable_bid_level_index(this_index_ - 1, level_size_thresh_, size_seen_, orders_seen_);
    }
    return market_update_info_.bidlevels_[this_index_].limit_int_price_;
  }
  return kInvalidIntPrice;
}

double SecurityMarketView::bid_price(unsigned int t_level_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (market_update_info_.compute_stable_levels_) {
      int size_seen_ = 0, orders_seen_ = 0;
      unsigned int current_bid_index_ =
          get_next_stable_bid_level_index(base_bid_index_, level_size_thresh_, size_seen_, orders_seen_);
      for (unsigned int level_counter_ = 0; current_bid_index_ > 0; level_counter_++) {
        if (level_counter_ >= t_level_) {
          return market_update_info_.bidlevels_[current_bid_index_].limit_price_;
        } else {
          size_seen_ = 0;
          orders_seen_ = 0;
          current_bid_index_ =
              get_next_stable_bid_level_index(current_bid_index_ - 1, level_size_thresh_, size_seen_, orders_seen_);
        }
      }
      return kInvalidPrice;
    } else {
      if (t_level_ == 0 && is_ready_) {
        return market_update_info_.bestbid_price_;
      }

      // Finds out the diff between the bestbid variable and the bestbid in the smv book.
      int level_diff = 0;
      if (is_ready_) {
        level_diff =
            market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ - market_update_info_.bestbid_int_price_;
      }

      // If level_diff is < 0 ( possible ?), the smv book might not have info for that level.
      if (level_diff > 0) {
        t_level_ += level_diff;
      }

      if ((bid_access_bitmask_ & bitmask_access_lookup_table_[t_level_]) == bitmask_access_lookup_table_[t_level_]) {
        return (market_update_info_.bidlevels_[base_bid_index_ - t_level_].limit_price_);
      }

      // NOT VERY EFFICIENT ... better to call GetBidLevelAtIndex
      if (base_bid_index_ == 0) return kInvalidPrice;
      if (t_level_ == 0) {  // top level
        return (market_update_info_.bidlevels_[base_bid_index_].limit_price_);
      } else {
        unsigned int current_bid_index_ = IndexedBookGetNextNonEmptyBidMapIndex(base_bid_index_);
        for (unsigned int level_counter_ = 1u; current_bid_index_ > 0; level_counter_++) {
          if (level_counter_ >= t_level_) {
            return market_update_info_.bidlevels_[current_bid_index_].limit_price_;
          } else {
            current_bid_index_ = IndexedBookGetNextNonEmptyBidMapIndex(current_bid_index_);
          }
        }
        return kInvalidPrice;
      }
    }
  } else {
    if (t_level_ < market_update_info_.bidlevels_.size()) {
      return market_update_info_.bidlevels_[t_level_].limit_price_;
    }
  }
  return kInvalidPrice;
}

int SecurityMarketView::bid_int_price(unsigned int t_level_) const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (market_update_info_.compute_stable_levels_) {
      // Calling it before loop and starting loop with 0
      // because even best variables might not be considered as level (0)
      int size_seen_ = 0, orders_seen_ = 0;
      unsigned int current_bid_index_ =
          get_next_stable_bid_level_index(base_bid_index_, level_size_thresh_, size_seen_, orders_seen_);

      for (unsigned int level_counter_ = 0; current_bid_index_ > 0; level_counter_++) {
        if (level_counter_ >= t_level_) {
          return market_update_info_.bidlevels_[current_bid_index_].limit_int_price_;
        } else {
          // Adding from next level
          size_seen_ = 0;
          orders_seen_ = 0;
          current_bid_index_ =
              get_next_stable_bid_level_index(current_bid_index_ - 1, level_size_thresh_, size_seen_, orders_seen_);
        }
      }
      return kInvalidIntPrice;
    } else {
      if (t_level_ == 0 && is_ready_) {
        return market_update_info_.bestbid_int_price_;
      }

      // Finds out the diff between the bestbid variable and the bestbid in the smv book.
      int level_diff = 0;
      if (is_ready_) {
        level_diff =
            market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ - market_update_info_.bestbid_int_price_;
      }

      // If level_diff is < 0 ( possible ?), the smv book might not have info for that level.
      if (level_diff > 0) {
        t_level_ += level_diff;
      }

      if ((bid_access_bitmask_ & bitmask_access_lookup_table_[t_level_]) == bitmask_access_lookup_table_[t_level_]) {
        return (market_update_info_.bidlevels_[base_bid_index_ - t_level_].limit_int_price_);
      }

      if (base_bid_index_ == 0) {
        return kInvalidIntPrice;
      }  // TODO_OPT not sure why this is needed
      if (t_level_ == 0) {
        return (market_update_info_.bidlevels_[base_bid_index_].limit_int_price_);
      } else {
        unsigned int current_bid_index_ = IndexedBookGetNextNonEmptyBidMapIndex(base_bid_index_);
        for (unsigned int level_counter_ = 1u; current_bid_index_ > 0; level_counter_++) {
          if (level_counter_ >= t_level_) {
            return market_update_info_.bidlevels_[current_bid_index_].limit_int_price_;
          } else {
            current_bid_index_ = IndexedBookGetNextNonEmptyBidMapIndex(current_bid_index_);
          }
        }
      }
    }
  } else {
    if (t_level_ < market_update_info_.bidlevels_.size()) {
      return market_update_info_.bidlevels_[t_level_].limit_int_price_;
    }
  }
  return kInvalidIntPrice;
}

int SecurityMarketView::bid_int_price_level(unsigned int t_level_) const {
  // top level is always at 0 int_price_level
  if (t_level_ == 0) {
    return 0;
  } else {
    if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      // NOT VERY EFFICIENT ... better to call GetBidLevelAtIndex, and then calc
      // base_bid_index_.limit_int_price_ - current_level_int_price_
      if (market_update_info_.compute_stable_levels_) {
      } else {
        if (is_ready_) {
          return (market_update_info_.bestbid_int_price_ - bid_int_price(t_level_));
        } else {
          return (market_update_info_.bidlevels_[base_bid_index_].limit_int_price_ - bid_int_price(t_level_));
        }
      }
    } else {
      if (t_level_ < market_update_info_.bidlevels_.size()) {
        return market_update_info_.bidlevels_[t_level_].limit_int_price_level_;
      }
    }
  }
  return kInvalidIntPrice;
}

bool SecurityMarketView::IsBidBookEmpty() const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    // commented the following since this can't ever happen
    // if (base_bid_index_ > market_update_info_.bidlevels_.size()) { return true; }
    int index = GetBaseBidMapIndex();
    if (index >= 0 && index < (int)market_update_info_.bidlevels_.size()) {
      return (market_update_info_.bidlevels_[index].limit_size_ <= 0);
    } else {
      return true;
    }
  } else {
    return market_update_info_.bidlevels_.empty();
  }
}

bool SecurityMarketView::IsBidBookL2() const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    if (IndexedBookGetNextNonEmptyBidMapIndex(base_bid_index_) > 0) {
      return true;
    } else {
      return false;
    }
  } else {
    return (market_update_info_.bidlevels_.size() >= 2);
  }
}

unsigned int SecurityMarketView::get_next_stable_bid_level_index(unsigned int current_bid_index_,
                                                                 int _level_size_thresh_, int &size_seen_,
                                                                 int &orders_seen_) const {
  size_seen_ = 0;
  orders_seen_ = 0;
  for (; current_bid_index_ > 0; current_bid_index_--) {
    size_seen_ += market_update_info_.bidlevels_[current_bid_index_].limit_size_;
    orders_seen_ += market_update_info_.bidlevels_[current_bid_index_].limit_ordercount_;
    if (size_seen_ >= _level_size_thresh_) {
      return current_bid_index_;
    }
  }
  return 0;
}

unsigned int SecurityMarketView::bidlevels_size() const {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    unsigned int num_valid_levels_ = 0;
    int size_seen_so_far_ = 0;
    for (unsigned int current_bid_index_ = GetBaseBidMapIndex(); current_bid_index_ > 0;
         current_bid_index_--) {  // not optimized
      if (market_update_info_.compute_stable_levels_) {
        size_seen_so_far_ += market_update_info_.bidlevels_[current_bid_index_].limit_size_;
        if (size_seen_so_far_ >= level_size_thresh_) {
          num_valid_levels_++;
          size_seen_so_far_ = 0;
        }
      } else {
        if (market_update_info_.bidlevels_[current_bid_index_].limit_size_ > 0) {
          num_valid_levels_++;
        }
      }
    }
    return num_valid_levels_;
  } else {
    return market_update_info_.bidlevels_.size();
  }
}

void SecurityMarketView::RebuildIndexLowAccess(TradeType_t buysell, int new_int_price) {
  switch (buysell) {
    case kTradeTypeBuy: {
      bid_access_bitmask_ = BIT_RESET_ALL;

      int offset_ = market_update_info_.bidlevels_[initial_tick_size_].limit_int_price_ - new_int_price;

      if (offset_ <= 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << secname() << " offset should be +ve valued but is " << offset_
                                    << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        std::cerr << " offset should be +ve valued but is " << offset_ << std::endl;
        std::cerr << " exiting now" << std::endl;
        return;
      }

      for (int index = market_update_info_.bidlevels_.size() - 1; index >= offset_; index--) {
        market_update_info_.bidlevels_[index].limit_int_price_ =
            market_update_info_.bidlevels_[index - offset_].limit_int_price_;
        market_update_info_.bidlevels_[index].limit_ordercount_ =
            market_update_info_.bidlevels_[index - offset_].limit_ordercount_;
        market_update_info_.bidlevels_[index].limit_size_ = market_update_info_.bidlevels_[index - offset_].limit_size_;
        market_update_info_.bidlevels_[index].limit_price_ =
            market_update_info_.bidlevels_[index - offset_].limit_price_;
      }

      base_bid_index_ = initial_tick_size_;

      // Offset can be quit huge, restrict size/price/ordercount resetting to the size of the array
      offset_ = std::min(offset_, (int)market_update_info_.bidlevels_.size());

      for (int index = 0; index < offset_; index++) {
        market_update_info_.bidlevels_[index].limit_int_price_ = new_int_price - (base_bid_index_ - index);
        market_update_info_.bidlevels_[index].limit_ordercount_ = 0;
        market_update_info_.bidlevels_[index].limit_size_ = 0;
        market_update_info_.bidlevels_[index].limit_price_ =
            GetDoublePx(market_update_info_.bidlevels_[index].limit_int_price_);
      }
      break;
    }

    case kTradeTypeSell: {
      ask_access_bitmask_ = BIT_RESET_ALL;

      int offset = new_int_price - market_update_info_.asklevels_[initial_tick_size_].limit_int_price_;

      if (offset <= 0) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " " << secname() << " offset should be +ve valued but is " << offset
                                    << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
        std::cerr << " offset should be +ve valued but is " << offset << std::endl;
        std::cerr << " exiting now" << std::endl;
        return;
      }

      for (int index = market_update_info_.asklevels_.size() - 1; index >= offset; index--) {
        market_update_info_.asklevels_[index].limit_int_price_ =
            market_update_info_.asklevels_[index - offset].limit_int_price_;
        market_update_info_.asklevels_[index].limit_ordercount_ =
            market_update_info_.asklevels_[index - offset].limit_ordercount_;
        market_update_info_.asklevels_[index].limit_size_ = market_update_info_.asklevels_[index - offset].limit_size_;
        market_update_info_.asklevels_[index].limit_price_ =
            market_update_info_.asklevels_[index - offset].limit_price_;
      }

      base_ask_index_ = initial_tick_size_;

      // Offset can be quit huge, restrict size/price/ordercount resetting to the size of the array
      offset = std::min(offset, (int)market_update_info_.asklevels_.size());

      for (int index = 0; index < offset; index++) {
        market_update_info_.asklevels_[index].limit_int_price_ = new_int_price + (base_ask_index_ - index);
        market_update_info_.asklevels_[index].limit_ordercount_ = 0;
        market_update_info_.asklevels_[index].limit_size_ = 0;
        market_update_info_.asklevels_[index].limit_price_ =
            GetDoublePx(market_update_info_.asklevels_[index].limit_int_price_);
      }
      break;
    }

    default:
      break;
  }
}

bool SecurityMarketView::DeleteAsk(int int_price) {
  int ask_index = GetAskIndex(int_price);

  // If ask_index is not a valid one, notify the listeners
  if (ask_index < 0 || ask_index > (int)base_ask_index_) {
    return true;
  }

  // If there is nothing to be deleted, notify the listeners and return
  if (GetAskSize(ask_index) == 0) {
    return true;
  }

  ResetAskLevel(ask_index);

  ask_level_change_bitmask_ = 0xFFFF;

  if (ask_index == (int)base_ask_index_) {
    l1_price_changed_ = true;

    // Find next base_bid_index
    int next_index = (int)base_ask_index_ - 1;
    for (; next_index >= 0 && GetAskSize(next_index) <= 0; next_index--)
      ;

    // Return from here, if next_index is invalid
    if (next_index < 0) {
      ask_access_bitmask_ = BIT_RESET_ALL;
      is_ready_ = false;
      return true;
    }

    UpAskBitmask(next_index);

    base_ask_index_ = (unsigned int)next_index;

    if (base_ask_index_ < LOW_ACCESS_INDEX) {
      RebuildIndexLowAccess(kTradeTypeSell, GetAskIntPrice(base_ask_index_));
    }

    UpdateBestAsk(base_ask_index_);
    UpdateBestAskVariablesUsingOurOrders();
  } else {
    RemoveAskBitmask(ask_index);
    l2_changed_since_last_ = true;
  }

  return true;
}

bool SecurityMarketView::DeleteBid(int int_price) {
  int bid_index = GetBidIndex(int_price);

  // If bid_index is not a valid one, notify the listeners
  if (bid_index < 0 || bid_index > (int)base_bid_index_) {
    return true;
  }

  // If there is nothing to be deleted, notify the listeners and return
  if (GetBidSize(bid_index) == 0) {
    return true;
  }

  ResetBidLevel(bid_index);

  bid_level_change_bitmask_ = 0xFFFF;

  if (bid_index == (int)base_bid_index_) {
    l1_price_changed_ = true;

    // Find next base_bid_index
    int next_index = (int)base_bid_index_ - 1;
    for (; next_index >= 0 && GetBidSize(next_index) <= 0; next_index--)
      ;

    // Return from here, if next_index is invalid
    if (next_index < 0) {
      bid_access_bitmask_ = BIT_RESET_ALL;
      is_ready_ = false;
      return true;
    }

    UpBidBitmask(next_index);

    base_bid_index_ = (unsigned int)next_index;

    if (base_bid_index_ < LOW_ACCESS_INDEX) {
      RebuildIndexLowAccess(kTradeTypeBuy, GetBidIntPrice(base_bid_index_));
    }

    UpdateBestBid(base_bid_index_);
    UpdateBestBidVariablesUsingOurOrders();
  } else {
    RemoveBidBitmask(bid_index);
    l2_changed_since_last_ = true;
  }

  return true;
}

void SecurityMarketView::NotifyDeleteListeners(TradeType_t buysell, int int_price, int level_removed, int old_size,
                                               int old_ordercount, bool intermediate) {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif

  if (l1_price_changed_ || l1_size_changed_) {
    UpdateL1Prices();
  }

  if (!is_ready_ && GetL1BidSize() > 0 && GetL1AskSize() > 0 && ArePricesComputed()) {
    is_ready_ = true;
  }

  if (!is_ready_ || intermediate) {
    return;
  }

  if (pl_change_listeners_present_) {
    if (buysell == kTradeTypeBuy) {
      int bid_index = GetBidIndex(int_price);
      if ((int)base_bid_index_ >= bid_index && base_bid_index_ - bid_index <= 10) {
        int int_price_level = base_bid_index_ - bid_index;
        NotifyOnPLChangeListeners(security_id(), market_update_info_, buysell, level_removed - 1, int_price,
                                  int_price_level, old_size, 0, old_ordercount, 0, intermediate, 'D');
      }
    } else if (buysell == kTradeTypeSell) {
      int ask_index = GetAskIndex(int_price);
      if ((int)base_ask_index_ >= ask_index && base_ask_index_ - ask_index <= 10) {
        int int_price_level_ = base_ask_index_ - ask_index;
        NotifyOnPLChangeListeners(security_id(), market_update_info_, buysell, level_removed - 1, int_price,
                                  int_price_level_, old_size, 0, 1, 0, intermediate, 'D');
      }
    }
  }

  if (l1_price_changed_) {
    NotifyL1PriceListeners();
    l1_price_changed_ = false;
    l1_size_changed_ = false;
  } else if (l1_size_changed_) {
    NotifyL1SizeListeners();
    l1_size_changed_ = false;
  } else {
    NotifyL2Listeners();
  }

  if (l2_changed_since_last_) {
    NotifyL2OnlyListeners();
    l2_changed_since_last_ = false;
  }

  NotifyOnReadyListeners();

  bid_level_change_bitmask_ = 0x0000;
  ask_level_change_bitmask_ = 0x0000;
}

void SecurityMarketView::UpdateBestAskVariablesUsingOurOrders() {
  if (!remove_self_orders_from_book_ || !p_prom_order_manager_) {
    return;
  }

  BestBidAskInfo ors_bid_ask_info_ = p_prom_order_manager_->ors_best_bid_ask();

  // Change the best ask variables only if the prices match
  // if the ors price is lower, the current best ask price is already taking care of our orders.
  // if the current best price is lower, we may not want to change our best price.
  if (ors_bid_ask_info_.best_ask_int_price_ == market_update_info_.bestask_int_price_) {
    if (ors_bid_ask_info_.best_ask_size_ >= market_update_info_.bestask_size_) {
      // If the ors size is at least the best ask size
      // Find the next best price
      int ask_index_ = GetAskIndex(market_update_info_.bestask_int_price_);
      int next_ask_index_ = ask_index_ - 1;
      for (; next_ask_index_ >= 0 && GetAskSize(next_ask_index_) <= 0; next_ask_index_--)
        ;

      if (next_ask_index_ >= 0) {
        UpdateBestAsk(next_ask_index_);

      } else {
        // if the next level doesn't exist, then don't change the best variables. (This case should rarely occur.)
      }
    } else {
      market_update_info_.bestask_ordercount_ =
          market_update_info_.bestask_ordercount_ > ors_bid_ask_info_.best_ask_orders_
              ? market_update_info_.bestask_ordercount_ - ors_bid_ask_info_.best_ask_orders_
              : 1;
      market_update_info_.bestask_size_ -= ors_bid_ask_info_.best_ask_size_;
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestask_ordercount_ = market_update_info_.bestask_ordercount_;
        hybrid_market_update_info_.bestask_size_ = market_update_info_.bestask_size_;
      }
    }
  }
}

void SecurityMarketView::UpdateBestBidVariablesUsingOurOrders() {
  if (!remove_self_orders_from_book_ || !p_prom_order_manager_) {
    return;
  }

  BestBidAskInfo ors_bid_ask_info_ = p_prom_order_manager_->ors_best_bid_ask();

  // Change the best variables only if the prices match.
  // If the ors price is higher, the current best price is already taking care of our orders.
  // If the current best price is higher, we may not want to change our best price.
  if (ors_bid_ask_info_.best_bid_int_price_ == market_update_info_.bestbid_int_price_) {
    if (ors_bid_ask_info_.best_bid_size_ >= market_update_info_.bestbid_size_) {
      // If the ors size is at least the best bid size
      // Find the next best price
      int bid_index_ = GetBidIndex(market_update_info_.bestbid_int_price_);
      int next_bid_index_ = bid_index_ - 1;
      for (; next_bid_index_ >= 0 && GetBidSize(next_bid_index_) <= 0; next_bid_index_--)
        ;

      if (next_bid_index_ >= 0) {
        UpdateBestBid(next_bid_index_);

      } else {
        // If the next level doesn't exist, then don't change the best variables. (This case should rarely occur.)
      }
    } else {
      market_update_info_.bestbid_ordercount_ =
          market_update_info_.bestbid_ordercount_ > ors_bid_ask_info_.best_bid_orders_
              ? market_update_info_.bestbid_ordercount_ - ors_bid_ask_info_.best_bid_orders_
              : 1;
      market_update_info_.bestbid_size_ -= ors_bid_ask_info_.best_bid_size_;
      if (!price_to_yield_map_.empty()) {
        hybrid_market_update_info_.bestbid_ordercount_ = market_update_info_.bestbid_ordercount_;
        hybrid_market_update_info_.bestbid_size_ = market_update_info_.bestbid_size_;
      }
    }
  }
}

// Sanitize ask side given the bid int price
void SecurityMarketView::SanitizeAskSide(int int_price) {
  if (int_price < GetL1AskIntPrice()) {
    return;
  }

  if (GetL1AskSize() <= 0) {
    // We should not do sanitization, if the other side is not ready
    is_ready_ = false;

    // Set the ask price, size appropriately so that classes not using smv.is_ready would not get misled
    int new_base_ask = GetAskIndex(int_price + 1);

    if (new_base_ask < LOW_ACCESS_INDEX) {
      RebuildIndexLowAccess(kTradeTypeSell, int_price + 1);

    } else {
      base_ask_index_ = new_base_ask;
    }

    UpdateBestAsk(base_ask_index_);
    return;
  }

  // Sanitize ASK side
  int temp_index = base_ask_index_;
  int last_base_index = temp_index;

  for (; temp_index >= 0; temp_index--) {
    if (GetAskIntPrice(temp_index) > int_price && GetAskSize(temp_index) > 0) {
      base_ask_index_ = temp_index;
      break;
    }

    ResetAskLevel(temp_index);
  }

  // The ask side is empty
  if (temp_index < 0) {
    ask_access_bitmask_ = BIT_RESET_ALL;
    is_ready_ = false;
    DBGLOG_TIME_CLASS_FUNC_LINE << " " << secname() << " Ask side empty after sanitization " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return;
  }

  UpAskBitmask(last_base_index);

  // Check if we need to re-align the index
  if (base_ask_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(kTradeTypeSell, GetL1AskIntPrice());
  }

  // Update ASK side best variables
  UpdateBestAsk(base_ask_index_);
  UpdateBestAskVariablesUsingOurOrders();
}

// Sanitize bid side given the ask int price
void SecurityMarketView::SanitizeBidSide(int int_price) {
  if (int_price > GetL1BidIntPrice()) {
    return;
  }

  if (GetL1BidSize() <= 0) {
    // We should not do sanitization, if the other side is not ready
    is_ready_ = false;

    // Set the bid price, size appropriately so that classes not using smv.is_ready would not get misled
    int new_base_bid = GetBidIndex(int_price - 1);

    if (new_base_bid < LOW_ACCESS_INDEX) {
      RebuildIndexLowAccess(kTradeTypeBuy, int_price - 1);

    } else {
      base_bid_index_ = new_base_bid;
    }

    UpdateBestBid(base_bid_index_);
    return;
  }

  // Sanitise BID side
  int temp_index = base_bid_index_;
  int last_base_index = temp_index;

  for (; temp_index >= 0; temp_index--) {
    if (GetBidIntPrice(temp_index) < int_price && GetBidSize(temp_index) > 0) {
      base_bid_index_ = temp_index;
      break;
    }

    ResetBidLevel(temp_index);
  }

  // bid side is empty
  if (temp_index < 0) {
    is_ready_ = false;
    bid_access_bitmask_ = BIT_RESET_ALL;
    DBGLOG_TIME_CLASS_FUNC_LINE << " " << secname() << " Bid side empty after sanitization " << DBGLOG_ENDL_FLUSH;
    DBGLOG_DUMP;
    return;
  }

  UpBidBitmask(last_base_index);

  // Check if we need to re-align the centre
  if (base_bid_index_ < LOW_ACCESS_INDEX) {
    RebuildIndexLowAccess(kTradeTypeBuy, GetL1BidIntPrice());
  }

  // Update the BID side best variables
  UpdateBestBid(base_bid_index_);
  UpdateBestBidVariablesUsingOurOrders();
}

void SecurityMarketView::SanitizeAskLastLevel(int level, int last_level, int index) {
  if (level != last_level) {
    return;
  }

  int temp_level = 1;
  int second_last = (int)base_ask_index_ - 1;

  for (; second_last > index; second_last--) {
    if (GetAskSize(second_last) > 0) {
      temp_level++;
    }

    if (temp_level == last_level - 1) {
      break;
    }
  }

  second_last--;

  for (; second_last > index; second_last--) {
    // Delete this ask level
    ResetAskLevel(second_last);
    RemoveAskBitmask(second_last);
  }
}

void SecurityMarketView::SanitizeBidLastLevel(int level, int last_level, int index) {
  if (level != last_level) {
    return;
  }

  int temp_level = 1;
  int second_last = (int)base_bid_index_ - 1;

  for (; second_last > index; second_last--) {
    if (GetBidSize(second_last) > 0) {
      temp_level++;
    }

    if (temp_level == last_level - 1) {
      break;
    }
  }

  second_last--;

  for (; second_last > index; second_last--) {
    // Delete this bid level
    ResetBidLevel(second_last);
    RemoveBidBitmask(second_last);
  }
}

void SecurityMarketView::SanitizeBidLevel1(int level, int index) {
  if (level == 1 && index < (int)base_bid_index_) {
    for (int temp_index = base_bid_index_; temp_index > index; temp_index--) {
      ResetBidLevel(temp_index);
    }

    UpBidBitmask(index);
    base_bid_index_ = index;

    if (base_bid_index_ < LOW_ACCESS_INDEX) {
      RebuildIndexLowAccess(kTradeTypeBuy, GetBidIntPrice(index));
    }
  }
}

void SecurityMarketView::SanitizeAskLevel1(int level, int index) {
  if (level == 1 && index < (int)base_ask_index_) {
    for (int temp_index = base_ask_index_; temp_index > index; temp_index--) {
      ResetAskLevel(temp_index);
    }

    UpAskBitmask(index);
    base_ask_index_ = index;

    if (base_ask_index_ < LOW_ACCESS_INDEX) {
      RebuildIndexLowAccess(kTradeTypeSell, GetAskIntPrice(index));
    }
  }
}

void SecurityMarketView::UpdateBidLevel(int index, int size, int ordercount) {
  market_update_info_.bidlevels_[index].limit_size_ = size;
  market_update_info_.bidlevels_[index].limit_ordercount_ = ordercount;
  if (!price_to_yield_map_.empty()) {
    hybrid_market_update_info_.bidlevels_[index].limit_size_ = size;
    hybrid_market_update_info_.bidlevels_[index].limit_ordercount_ = ordercount;
  }
}

void SecurityMarketView::UpdateAskLevel(int index, int size, int ordercount) {
  market_update_info_.asklevels_[index].limit_size_ = size;
  market_update_info_.asklevels_[index].limit_ordercount_ = ordercount;
  if (!price_to_yield_map_.empty()) {
    hybrid_market_update_info_.asklevels_[index].limit_size_ = size;
    hybrid_market_update_info_.asklevels_[index].limit_ordercount_ = ordercount;
  }
}

void SecurityMarketView::ResetBidLevel(int index) {
  market_update_info_.bidlevels_[index].limit_size_ = 0;
  market_update_info_.bidlevels_[index].limit_ordercount_ = 0;
  if (!price_to_yield_map_.empty()) {
    hybrid_market_update_info_.bidlevels_[index].limit_size_ = 0;
    hybrid_market_update_info_.bidlevels_[index].limit_ordercount_ = 0;
  }
}

void SecurityMarketView::ResetAskLevel(int index) {
  market_update_info_.asklevels_[index].limit_size_ = 0;
  market_update_info_.asklevels_[index].limit_ordercount_ = 0;
  if (!price_to_yield_map_.empty()) {
    hybrid_market_update_info_.asklevels_[index].limit_size_ = 0;
    hybrid_market_update_info_.asklevels_[index].limit_ordercount_ = 0;
  }
}

void SecurityMarketView::UpdateBestBid(int index) {
  market_update_info_.bestbid_int_price_ = GetBidIntPrice(index);
  market_update_info_.bestbid_ordercount_ = GetBidOrders(index);
  market_update_info_.bestbid_size_ = GetBidSize(index);
  market_update_info_.bestbid_price_ = GetBidPrice(index);
  if (!price_to_yield_map_.empty()) {
    hybrid_market_update_info_.bestbid_int_price_ = GetBidIntPrice(index);
    hybrid_market_update_info_.bestbid_ordercount_ = GetBidOrders(index);
    hybrid_market_update_info_.bestbid_size_ = GetBidSize(index);
    hybrid_market_update_info_.bestbid_price_ = price_to_yield_map_[hybrid_market_update_info_.bestbid_int_price_];
  }
}

void SecurityMarketView::UpdateBestAsk(int index) {
  market_update_info_.bestask_int_price_ = GetAskIntPrice(index);
  market_update_info_.bestask_ordercount_ = GetAskOrders(index);
  market_update_info_.bestask_size_ = GetAskSize(index);
  market_update_info_.bestask_price_ = GetAskPrice(index);
  if (!price_to_yield_map_.empty()) {
    hybrid_market_update_info_.bestask_int_price_ = GetAskIntPrice(index);
    hybrid_market_update_info_.bestask_ordercount_ = GetAskOrders(index);
    hybrid_market_update_info_.bestask_size_ = GetAskSize(index);
    //      std::cout << hybrid_market_update_info_.bestask_int_price_ << " " << market_update_info_.bestask_price_ <<
    //      "\n";
    hybrid_market_update_info_.bestask_price_ = price_to_yield_map_[hybrid_market_update_info_.bestask_int_price_];
  }
}

void SecurityMarketView::SetSmartSelfOrdersFromBook(const bool t_smart_remove_self_) const {
  if (t_smart_remove_self_ == false) {
    smart_remove_self_orders_from_book_ = false;
  }

  if ((SecurityDefinitions::GetRemoveSelfOrdersFromBook(shortcode(), watch_.YYYYMMDD()) == false) ||
      (SecurityDefinitions::GetConfToMarketUpdateMsecs(shortcode(), watch_.YYYYMMDD()) == false)) {
    DBGLOG_TIME_CLASS_FUNC_LINE
        << " For " << shortcode() << " GetRemoveSelfOrdersFromBook = "
        << (SecurityDefinitions::GetRemoveSelfOrdersFromBook(shortcode(), watch_.YYYYMMDD()) ? "true" : "false")
        << " GetConfToMarketUpdateMsecs = "
        << (SecurityDefinitions::GetConfToMarketUpdateMsecs(shortcode(), watch_.YYYYMMDD()) ? "true" : "false")
        << " hence not using smart_non_self." << DBGLOG_ENDL_FLUSH;
    return;
  }

  if (!p_prom_order_manager_) {
    p_prom_order_manager_ = PromOrderManager::GetCreatedInstance(shortcode());
  }

  if (!p_prom_order_manager_) {
    DBGLOG_TIME_CLASS_FUNC_LINE
        << " For " << shortcode() << " GetRemoveSelfOrdersFromBook = "
        << (SecurityDefinitions::GetRemoveSelfOrdersFromBook(shortcode(), watch_.YYYYMMDD()) ? "true" : "false")
        << " GetConfToMarketUpdateMsecs = "
        << (SecurityDefinitions::GetConfToMarketUpdateMsecs(shortcode(), watch_.YYYYMMDD()) ? "true" : "false")
        << " p_prom_order_manager_ = nullptr hence not using smart_non_self." << DBGLOG_ENDL_FLUSH;
    return;
  } else {
    if (t_smart_remove_self_) {
      p_prom_order_manager_->ManageOrdersAlso();
    }
  }

  smart_remove_self_orders_from_book_ = t_smart_remove_self_;
  p_prom_order_manager_->SetSmartNonSelf(smart_remove_self_orders_from_book_);

  DBGLOG_TIME_CLASS_FUNC_LINE << " " << shortcode() << " remove_self_orders_from_book_ = "
                              << (remove_self_orders_from_book_ ? "true" : "false")
                              << " smart_remove_self_orders_from_book_ = "
                              << (smart_remove_self_orders_from_book_ ? "true" : "false")
                              << ((remove_self_orders_from_book_ && smart_remove_self_orders_from_book_) ? " " : " not")
                              << " using smart_non_self" << DBGLOG_ENDL_FLUSH;

  DBGLOG_DUMP;
}

void SecurityMarketView::SetSelfOrdersFromBook(const bool t_remove_self_) const {
  if (t_remove_self_ == false) {
    remove_self_orders_from_book_ = false;
  }

  if (!SecurityDefinitions::GetRemoveSelfOrdersFromBook(shortcode(), watch_.YYYYMMDD())) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " For " << shortcode() << " GetRemoveSelfOrdersFromBook = "
                                << (SecurityDefinitions::GetRemoveSelfOrdersFromBook(shortcode(), watch_.YYYYMMDD())
                                        ? "true"
                                        : "false") << " not using non_self." << DBGLOG_ENDL_FLUSH;
    return;
  }

  if (!p_prom_order_manager_) {
    p_prom_order_manager_ = PromOrderManager::GetCreatedInstance(shortcode());
  }

  if (!p_prom_order_manager_) {
    DBGLOG_TIME_CLASS_FUNC_LINE << " For " << shortcode() << " GetRemoveSelfOrdersFromBook = "
                                << (SecurityDefinitions::GetRemoveSelfOrdersFromBook(shortcode(), watch_.YYYYMMDD())
                                        ? "true"
                                        : "false") << " p_prom_order_manager_ = nullptr"
                                << " hence not using non_self." << DBGLOG_ENDL_FLUSH;
    return;
  } else {
    if (t_remove_self_) {
      p_prom_order_manager_->ManageOrdersAlso();
    }
  }

  remove_self_orders_from_book_ = t_remove_self_;

  DBGLOG_TIME_CLASS_FUNC_LINE << " " << shortcode() << " remove_self_orders_from_book_ = "
                              << (remove_self_orders_from_book_ ? "true" : "false")
                              << " smart_remove_self_orders_from_book_ = "
                              << (smart_remove_self_orders_from_book_ ? "true" : "false") << " using non_self"
                              << DBGLOG_ENDL_FLUSH;
  DBGLOG_DUMP;
}

bool SecurityMarketView::ArePricesComputed() const {
  return (
      (!price_type_subscribed_[kPriceTypeMidprice] || market_update_info_.mid_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeMktSizeWPrice] ||
       market_update_info_.mkt_size_weighted_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeMktSinusoidal] ||
       market_update_info_.mkt_sinusoidal_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeOrderWPrice] ||
       market_update_info_.order_weighted_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeOfflineMixMMS] ||
       market_update_info_.offline_mix_mms_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeTradeWPrice] ||
       market_update_info_.trade_weighted_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeValidLevelMidPrice] ||
       market_update_info_.valid_level_mid_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeBandPrice] || market_update_info_.band_mkt_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeTradeBasePrice] ||
       market_update_info_.trade_base_px_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeTradeMktSizeWPrice] ||
       market_update_info_.trade_mktsz_px_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeTradeMktSinPrice] ||
       market_update_info_.trade_mktsin_px_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeTradeOrderWPrice] ||
       market_update_info_.trade_orderw_px_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeTradeTradeWPrice] ||
       market_update_info_.trade_tradew_px_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeTradeOmixPrice] ||
       market_update_info_.trade_omix_px_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeOrderSizeWPrice] ||
       market_update_info_.order_size_weighted_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeOnlineMixPrice] ||
       market_update_info_.online_mix_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeStableBidPrice] ||
       market_update_info_.stable_bid_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeStableAskPrice] ||
       market_update_info_.stable_ask_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeProRataMktSizeWPrice] ||
       market_update_info_.pro_rata_mkt_size_weighted_price_ > (kInvalidPrice + 1.5)) &&
      (!price_type_subscribed_[kPriceTypeImpliedVol] ||
       market_update_info_.implied_vol_ > (kInvalidPrice + 1.5)));  // For each price, check that either the
                                                                    // price-type is not subscribed to OR the
                                                                    // price-type is subscribed to AND the
                                                                    // price is valid.
}

void SecurityMarketView::InitializeSMVForIndexedBook() {
  if (market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
    initial_tick_size_ = IndexedBookOputils::GetUniqueInstance().GetInitialBaseBidIndex(market_update_info_.shortcode_);
    max_allowed_tick_range_ = IndexedBookOputils::GetUniqueInstance().GetMaxTickRange(market_update_info_.shortcode_);

    // Increasing this for NSE by default
    if (exch_source() == kExchSourceNSE) {
      initial_tick_size_ *= 8;
    }

    max_tick_range_ = (2 * initial_tick_size_ + 1);

    market_update_info_.bidlevels_.clear();
    market_update_info_.asklevels_.clear();

    is_ready_ = false;
    initial_book_constructed_ = false;

    // Initialize the Indexed Book pool
    market_update_info_.bidlevels_.resize(
        max_tick_range_, MarketUpdateInfoLevelStruct(0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv()));
    market_update_info_.asklevels_.resize(
        max_tick_range_, MarketUpdateInfoLevelStruct(0, kInvalidIntPrice, kInvalidPrice, 0, 0, watch_.tv()));

    int_price_bid_book_.resize(max_tick_range_, 0);
    int_price_ask_book_.resize(max_tick_range_, 0);

    int_price_bid_skip_delete_.resize(max_tick_range_, false);
    int_price_ask_skip_delete_.resize(max_tick_range_, false);

  } else {
    std::cerr << "Invalid Function Call to Setup SMV for Indexed Book, First Set the Variable\n";
    exit(-1);
  }
}

void SecurityMarketView::DeleteBidSidePriorities() {
  for (int x = 0; x < (int)market_update_info_.bidlevels_.size(); x++) {
    market_update_info_.bidlevels_[x].priority_size_ = 0;
    if ((int)market_update_info_.bid_level_order_depth_book_.size() > x &&
        market_update_info_.bid_level_order_depth_book_[x].size() > 0) {
      market_update_info_.bid_level_order_depth_book_[x][0]->priority_ = false;
    }
  }
}

void SecurityMarketView::DeleteAskSidePriorities() {
  for (int x = 0; x < (int)market_update_info_.asklevels_.size(); x++) {
    market_update_info_.asklevels_[x].priority_size_ = 0;
    if ((int)market_update_info_.ask_level_order_depth_book_.size() > x &&
        market_update_info_.ask_level_order_depth_book_[x].size() > 0) {
      market_update_info_.ask_level_order_depth_book_[x][0]->priority_ = false;
    }
  }
}

MarketUpdateInfoLevelStruct SecurityMarketView::GetBidPriceLevelBookAtPrice(int t_bid_price_) {
  for (int x = 0; x < (int)market_update_info_.bidlevels_.size(); x++) {
    if (market_update_info_.bidlevels_[x].limit_int_price_ == t_bid_price_) {
      return market_update_info_.bidlevels_[x];
    }
  }
  MarketUpdateInfoLevelStruct dummy;
  return dummy;
}

MarketUpdateInfoLevelStruct SecurityMarketView::GetAskPriceLevelBookAtPrice(int t_ask_price_) {
  for (int x = 0; x < (int)market_update_info_.asklevels_.size(); x++) {
    if (market_update_info_.asklevels_[x].limit_int_price_ == t_ask_price_) {
      return market_update_info_.asklevels_[x];
    }
  }
  MarketUpdateInfoLevelStruct dummy;
  return dummy;
}

int SecurityMarketView::CheckIfSamePriceExistsOnAsk(int t_level_added_,
                                                    int t_int_price_)  // when exchange sends a new price level
                                                                       // for an existing price level we copy
                                                                       // the previous order book to the new
                                                                       // level
{
  for (int x = 0; x < (int)market_update_info_.asklevels_.size(); x++) {
    if (x != t_level_added_ - 1) {
      if (market_update_info_.asklevels_[x].limit_int_price_ == t_int_price_) return x;
    }
  }
  return -1;
}

int SecurityMarketView::CheckIfSamePriceExistsOnBid(int t_level_added_, int t_int_price_) {
  for (int x = 0; x < (int)market_update_info_.bidlevels_.size(); x++) {
    if (x != t_level_added_ - 1) {
      if (market_update_info_.bidlevels_[x].limit_int_price_ == t_int_price_) return x;
    }
  }
  return -1;
}

void SecurityMarketView::AddNewLevelBidOrderDepth(int t_level_added_, MarketOrder *p_market_order_) {
  std::vector<MarketOrder *> new_order_vec_;
  new_order_vec_.push_back(p_market_order_);

  if ((int)market_update_info_.bid_level_order_depth_book_.size() >= t_level_added_ - 1) {
    market_update_info_.bid_level_order_depth_book_.insert(
        market_update_info_.bid_level_order_depth_book_.begin() + t_level_added_ - 1, new_order_vec_);
  }
  if ((int)market_update_info_.bid_level_order_depth_book_.size() > DEF_MARKET_DEPTH) {
    market_update_info_.bid_level_order_depth_book_.pop_back();
  }
}

void SecurityMarketView::AddNewLevelAskOrderDepth(int t_level_added_, MarketOrder *p_market_order_) {
  std::vector<MarketOrder *> new_order_vec_;
  new_order_vec_.push_back(p_market_order_);
  if ((int)market_update_info_.ask_level_order_depth_book_.size() >= t_level_added_ - 1) {
    market_update_info_.ask_level_order_depth_book_.insert(
        market_update_info_.ask_level_order_depth_book_.begin() + t_level_added_ - 1, new_order_vec_);
  }
  if (market_update_info_.ask_level_order_depth_book_.size() > DEF_MARKET_DEPTH) {
    market_update_info_.ask_level_order_depth_book_.pop_back();
  }
}

int SecurityMarketView::RemoveBidPrice(int t_int_price_) {
  int index_ = 1;
  std::vector<MarketUpdateInfoLevelStruct>::iterator t_iter_ = market_update_info_.bidlevels_.begin();

  for (; ((t_iter_ != market_update_info_.bidlevels_.end()) && (t_int_price_ != t_iter_->limit_int_price_));
       t_iter_++, index_++)
    ;

  if (t_iter_ != market_update_info_.bidlevels_.end()) market_update_info_.bidlevels_.erase(t_iter_);
  return index_;
}

int SecurityMarketView::RemoveAskPrice(int t_int_price_) {
  int index_ = 1;
  std::vector<MarketUpdateInfoLevelStruct>::iterator t_iter_ = market_update_info_.asklevels_.begin();

  for (; ((t_iter_ != market_update_info_.asklevels_.end()) && (t_int_price_ != t_iter_->limit_int_price_));
       t_iter_++, index_++)
    ;

  if (t_iter_ != market_update_info_.asklevels_.end()) market_update_info_.asklevels_.erase(t_iter_);
  return index_;
}

int SecurityMarketView::AddBidPrice(MarketUpdateInfoLevelStruct &new_level_) {
  std::vector<MarketUpdateInfoLevelStruct>::iterator t_iter_ = market_update_info_.bidlevels_.begin();
  int i = 1;
  for (;
       ((t_iter_ != market_update_info_.bidlevels_.end()) && (t_iter_->limit_int_price_ > new_level_.limit_int_price_));
       t_iter_++, i++) {
    // t_iter_ ++;
  }

  market_update_info_.bidlevels_.insert(t_iter_, new_level_);

  if (market_update_info_.bidlevels_.size() > DEF_MARKET_DEPTH) {
    market_update_info_.bidlevels_.pop_back();
  }
  return i;
}

int SecurityMarketView::AddAskPrice(MarketUpdateInfoLevelStruct &new_level_) {
  std::vector<MarketUpdateInfoLevelStruct>::iterator t_iter_ = market_update_info_.asklevels_.begin();
  int i = 1;
  for (;
       ((t_iter_ != market_update_info_.asklevels_.end()) && (t_iter_->limit_int_price_ < new_level_.limit_int_price_));
       t_iter_++, i++) {
    // t_iter_ ++;
  }

  market_update_info_.asklevels_.insert(t_iter_, new_level_);

  if (market_update_info_.asklevels_.size() > DEF_MARKET_DEPTH) {
    market_update_info_.asklevels_.pop_back();
  }
  return i;
}

void SecurityMarketView::RemoveNonTopAsk(int t_level_removed_) {
  if (t_level_removed_ > DEF_MARKET_DEPTH || t_level_removed_ > (int)market_update_info_.asklevels_.size()) {
    return;
  }
  std::vector<MarketUpdateInfoLevelStruct>::iterator t_iter_ = market_update_info_.asklevels_.begin();
  for (int i = 1; ((i < t_level_removed_) && (t_iter_ != market_update_info_.asklevels_.end())); i++, t_iter_++)
    ;

  market_update_info_.asklevels_.erase(t_iter_);
}

//=================================================== LIFFE =====================================================//

void SecurityMarketView::AddBestBidPrice(MarketUpdateInfoLevelStruct &new_level_) {
  market_update_info_.bidlevels_.insert(market_update_info_.bidlevels_.begin(),
                                        new_level_);  // complexity .. O (number of elements after t_iter_)
  if (market_update_info_.bidlevels_.size() > DEF_MARKET_DEPTH) {
    market_update_info_.bidlevels_.pop_back();
  }
}

void SecurityMarketView::AddBestAskPrice(MarketUpdateInfoLevelStruct &new_level_) {
  market_update_info_.asklevels_.insert(market_update_info_.asklevels_.begin(),
                                        new_level_);  // complexity .. O (number of elements after t_iter_)
  if (market_update_info_.asklevels_.size() > DEF_MARKET_DEPTH) {
    market_update_info_.asklevels_.pop_back();
  }
}

void SecurityMarketView::RemoveNonTopBid(int t_level_removed_) {
  if (t_level_removed_ > DEF_MARKET_DEPTH || t_level_removed_ > (int)market_update_info_.bidlevels_.size()) {
    return;
  }
  std::vector<MarketUpdateInfoLevelStruct>::iterator t_iter_ = market_update_info_.bidlevels_.begin();
  for (int i = 1; ((i < t_level_removed_) && (t_iter_ != market_update_info_.bidlevels_.end())); i++, t_iter_++)
    ;

  market_update_info_.bidlevels_.erase(t_iter_);
}

// To be used only by L1 manager
void SecurityMarketView::ReplaceTopBid(MarketUpdateInfoLevelStruct &new_level_) {
  if (market_update_info_.bidlevels_.size() >= 1)
    market_update_info_.bidlevels_[0] = new_level_;
  else
    market_update_info_.bidlevels_.insert(market_update_info_.bidlevels_.begin(),
                                          new_level_);  // complexity .. O (number of elements after t_iter_)
}

// To be used only by L1 manager
void SecurityMarketView::ReplaceTopAsk(MarketUpdateInfoLevelStruct &new_level_) {
  if (market_update_info_.asklevels_.size() >= 1)
    market_update_info_.asklevels_[0] = new_level_;
  else
    market_update_info_.asklevels_.insert(market_update_info_.asklevels_.begin(),
                                          new_level_);  // complexity .. O (number of elements after t_iter_)
}

void SecurityMarketView::AddTopBid(MarketUpdateInfoLevelStruct &new_level_) {
  market_update_info_.bidlevels_.insert(market_update_info_.bidlevels_.begin(),
                                        new_level_);  // complexity .. O (number of elements after t_iter_)
  if (market_update_info_.bidlevels_.size() > DEF_MARKET_DEPTH) {
    market_update_info_.bidlevels_.pop_back();
  }
}

void SecurityMarketView::AddTopAsk(MarketUpdateInfoLevelStruct &new_level_) {
  market_update_info_.asklevels_.insert(market_update_info_.asklevels_.begin(),
                                        new_level_);  // complexity .. O (number of elements after t_iter_)
  if (market_update_info_.asklevels_.size() > DEF_MARKET_DEPTH) {
    market_update_info_.asklevels_.pop_back();
  }
}

void SecurityMarketView::NotifyMarketStatusListeners(const unsigned int t_security_id_,
                                                     const MktStatus_t _this_market_status_) {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif

  if (current_market_status_ != _this_market_status_) {
    // Notify only when the status changes
    current_market_status_ = _this_market_status_;
    for (auto i = 0u; i < market_status_listeners_.size(); i++) {
#if CCPROFILING_TRADEINIT
      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
#endif
      market_status_listeners_[i]->OnMarketStatusChange(t_security_id_, _this_market_status_);
    }
  }
}

void SecurityMarketView::NotifyOffMarketTradeListeners(const unsigned int t_security_id_, const double _price_,
                                                       const int _int_price_, const int _trade_size_,
                                                       const TradeType_t _buysell_, int _lvl_) {
#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::SetUniqueInstance(30);
  HFSAT::CpucycleProfiler::GetUniqueInstance().End(22);
#endif

  for (unsigned i = 0; i < off_market_trade_listeners_.size(); i++) {
#if CCPROFILING_TRADEINIT
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(23);
#endif
    off_market_trade_listeners_[i]->OnOffMarketTrade(t_security_id_, _price_, _int_price_, _trade_size_, _buysell_,
                                                     _lvl_);
  }
}
}
