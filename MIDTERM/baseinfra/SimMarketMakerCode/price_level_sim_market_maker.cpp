/**
   \file SimMarketMakerCode/price_level_sim_market_maker.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include "dvccode/Profiler/cpucycle_profiler.hpp"

#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "baseinfra/SimMarketMaker/price_level_sim_market_maker.hpp"
#include "baseinfra/OrderRouting/market_model_manager.hpp"
#include "baseinfra/SimMarketMaker/trade_ratio_calculator.hpp"
#include "baseinfra/LoggedSources/cfe_logged_message_filenamer.hpp"

#define POSTPONE_MSECS 100
// #define USING_AGGRESSIVE_FRONT_CANCELATION

#define USING_ONLY_FULL_MKT_FOR_SIM

namespace HFSAT {

PriceLevelSimMarketMaker* PriceLevelSimMarketMaker::GetUniqueInstance(DebugLogger& dbglogger, Watch& watch,
                                                                      SecurityMarketView& dep_market_view,
                                                                      int market_model_index,
                                                                      SimTimeSeriesInfo& sim_time_series_info) {
  MarketModel this_market_model;

  MarketModelManager::GetMarketModel(dep_market_view.shortcode(), market_model_index, this_market_model,
                                     watch.YYYYMMDD());

  std::ostringstream temp_oss;
  temp_oss << "PriceLevelSimMarketMaker " << dep_market_view.shortcode() << ' ' << market_model_index;
  std::string plsmm_description(temp_oss.str());

  static std::map<std::string, PriceLevelSimMarketMaker*> SMM_description_map;
  if (SMM_description_map.find(plsmm_description) == SMM_description_map.end()) {
    SMM_description_map[plsmm_description] =
        new PriceLevelSimMarketMaker(dbglogger, watch, this_market_model, dep_market_view, sim_time_series_info);
  }
  return SMM_description_map[plsmm_description];
}

PriceLevelSimMarketMaker* PriceLevelSimMarketMaker::GetInstance(DebugLogger& dbglogger, Watch& watch,
                                                                SecurityMarketView& dep_market_view,
                                                                int market_model_index,
                                                                SimTimeSeriesInfo& sim_time_series_info) {
  MarketModel this_market_model;
  MarketModelManager::GetMarketModel(dep_market_view.shortcode(), market_model_index, this_market_model,
                                     watch.YYYYMMDD());

  std::ostringstream temp_oss;
  temp_oss << "PriceLevelSimMarketMaker " << dep_market_view.shortcode() << ' ' << market_model_index;
  std::string plsmm_description(temp_oss.str());
  PriceLevelSimMarketMaker* plsmm_ =
      new PriceLevelSimMarketMaker(dbglogger, watch, this_market_model, dep_market_view, sim_time_series_info);
  return plsmm_;
}

PriceLevelSimMarketMaker::PriceLevelSimMarketMaker(DebugLogger& dbglogger, Watch& watch, MarketModel market_model,
                                                   SecurityMarketView& dep_market_view,
                                                   HFSAT::SimTimeSeriesInfo& sim_time_series_info)
    : BaseSimMarketMaker(dbglogger, watch, dep_market_view, market_model, sim_time_series_info),
      dep_market_view_(dep_market_view),
      masked_bids_(false),
      masked_from_market_data_bids_map_(),
      masked_asks_(false),
      masked_from_market_data_asks_map_(),
      bestbid_int_price_(kInvalidIntPrice),
      bestask_int_price_(kInvalidIntPrice),
      bestbid_size_(0),
      bestask_size_(0),
      last_all_levels_queued_(0),
      current_ask_trade_time_(),
      current_bid_trade_time_(),

      current_ask_trade_ratio_(0),
      current_bid_trade_ratio_(0),
      askside_trade_size_(0),
      bidside_trade_size_(0),
      current_ask_trade_ratio_vec_(),
      current_bid_trade_ratio_vec_(),

      time_decayed_trade_info_manager_(
          *(TimeDecayedTradeInfoManager::GetUniqueInstance(dbglogger, watch, dep_market_view, 30))),
      w1(config_.tgt_sim_wt1_),
      w2(config_.tgt_sim_wt2_),
      prev_dep_price_change_(-1),
      low_likelihood_thresh_(config_.low_likelihood_thresh_),
      is_dummy_trade_print_(false),
      lookahead_msecs_(5),
      last_ask_size_change_msecs_(0),
      last_bid_size_change_msecs_(0),
      bid_side_priority_order_exists_map_(),
      bid_side_priority_order_size_map_(),
      ask_side_priority_order_exists_map_(),
      ask_side_priority_order_size_map_(),
      is_mov_avg_ready_(false) {
  dep_market_view_.ComputeMidPrice();
  dep_market_view_.ComputeMktPrice();

  ///< to get TimePeriod called every msec
  watch.subscribe_first_SmallTimePeriod(this);

  GetMatchingAlgoForShortcode(dep_market_view.shortcode(), watch.YYYYMMDD());

  if ((current_matching_algo_ == kSimpleProRata || current_matching_algo_ == kNewTimeProRata) && w1 != 0 && w2 == 0) {
    w2 = w1;
  }

  if (config_.use_tgt_sim_market_maker_ || config_.use_baseprice_tgt_sim_market_maker_) {
    moving_avg_util_ = new MovingAverage(dbglogger, watch, 30);
    time_decayed_trade_info_manager_.compute_onelvlsumpxsz();
    time_decayed_trade_info_manager_.compute_onelvlsumsz();
  } else if (config_.use_fgbm_sim_market_maker_) {
  } else {
    double percentile_ = 0.8;
    TradeRatioCalculator::BidAskTradeRatio(dep_market_view, watch, percentile_, bid_tr_ratio_, ask_tr_ratio_);
  }

  if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
    DBGLOG_TIME_CLASS_FUNC << "Bid Trade Ratio: " << bid_tr_ratio_ << "Ask Trade Ratio: " << ask_tr_ratio_
                           << DBGLOG_ENDL_FLUSH;
  }

  // Initializing lookahead reader (for fetching upcoming trades): Only required for CFE
  if (dep_market_view_.this_smv_exch_source_ == kExchSourceCFE) {
    lookahead_reader_ = new HFSAT::BulkFileReader();
    HFSAT::TradingLocation_t trade_location_cfe_ = HFSAT::kTLocCFE;
    std::string t_cfe_filename_ = CFELoggedMessageFileNamer::GetName(
        dep_market_view_.secname(), (unsigned int)watch_.YYYYMMDD(), trade_location_cfe_);
    lookahead_reader_->open(t_cfe_filename_);
  }
}

PriceLevelSimMarketMaker::~PriceLevelSimMarketMaker() {
  // De-allocating lookahead reader: Only required for CFE
  if (dep_market_view_.this_smv_exch_source_ == kExchSourceCFE) {
    lookahead_reader_->close();
    delete lookahead_reader_;
  }

  std::cout << "szie: " << all_requests_.size() << " " << pending_requests_.size() << std::endl;
}

void PriceLevelSimMarketMaker::SubscribeL2Events(SecurityMarketView& dep_market_view) {
  dep_market_view.subscribe_L2(this);
}

void PriceLevelSimMarketMaker::OnTimePeriodUpdate(const int num_pages_to_add) {
  if (config_.max_conf_orders_above_best_level_ >= 0) {
    CxlOrdersAboveBestLevel(config_.max_conf_orders_above_best_level_);
  }

  if (!all_requests_.empty()) {
    ProcessRequestQueue();
  }
}

void PriceLevelSimMarketMaker::InitializeOnMktUpdate(const MarketUpdateInfo& market_update_info) {
  int old_bestbid_int_price_ = bestbid_int_price_;
  int old_bestbid_size_ = bestbid_size_;
  int old_bestask_int_price_ = bestask_int_price_;
  int old_bestask_size_ = bestask_size_;

  if (bestbid_size_ < market_update_info.bestbid_size_) {
    last_bid_size_change_msecs_ = watch_.msecs_from_midnight();
  }

  if (bestask_size_ < market_update_info.bestask_size_) {
    last_ask_size_change_msecs_ = watch_.msecs_from_midnight();
  }

  if (old_bestask_int_price_ > market_update_info.bestask_int_price_) {
    if (dep_market_view_.ask_order(0) == 1) {
      VectorUtils::FillInValue(ask_side_priority_order_exists_map_, true);
      VectorUtils::FillInValue(ask_side_priority_order_size_map_, dep_market_view_.ask_size(0));
    } else {
      VectorUtils::FillInValue(ask_side_priority_order_exists_map_, false);
      VectorUtils::FillInValue(ask_side_priority_order_size_map_, 0);
    }
  }

  if (old_bestbid_int_price_ < market_update_info.bestbid_int_price_) {
    if (dep_market_view_.bid_order(0) == 1) {
      VectorUtils::FillInValue(bid_side_priority_order_exists_map_, true);
      VectorUtils::FillInValue(bid_side_priority_order_size_map_, dep_market_view_.bid_size(0));
    } else {
      VectorUtils::FillInValue(bid_side_priority_order_exists_map_, false);
      VectorUtils::FillInValue(bid_side_priority_order_size_map_, 0);
    }
  }

  // currently strictly removing rhe proority order if queue size changes
  //  Can change if sim -real is still bad
  if (old_bestask_int_price_ == dep_market_view_.ask_int_price(0) && old_bestask_size_ > dep_market_view_.ask_size(0)) {
    VectorUtils::FillInValue(ask_side_priority_order_exists_map_, false);
    VectorUtils::FillInValue(ask_side_priority_order_size_map_, 0);
  }

  if (old_bestbid_int_price_ == dep_market_view_.bid_int_price(0) && old_bestbid_size_ > dep_market_view_.bid_size(0)) {
    VectorUtils::FillInValue(bid_side_priority_order_exists_map_, false);
    VectorUtils::FillInValue(bid_side_priority_order_size_map_, 0);
  }

  if (config_.max_conf_orders_above_best_level_ >= 0) {
    CxlOrdersAboveBestLevel(config_.max_conf_orders_above_best_level_);
  }

  if (!all_requests_.empty()) {
    ProcessRequestQueue();
  }

  if (config_.using_only_full_mkt_for_sim_) {
    if (bestbid_int_price_ != dep_market_view_.bid_int_price(0)) {
      bestbid_int_price_ = dep_market_view_.bid_int_price(0);
      // reset masks on the bidside

      if (masked_bids_) {  // this means that masks had been set, hence the need to unmask
        VectorUtils::FillInValue(masked_from_market_data_bids_map_, 0);
      }
    }
    bestbid_size_ = dep_market_view_.bid_size(0);
  } else {
    if (bestbid_int_price_ != dep_market_view_.market_update_info_.bestbid_int_price_) {
      bestbid_int_price_ = dep_market_view_.market_update_info_.bestbid_int_price_;
      // reset masks on the bidside
      if (masked_bids_) {  // this means that masks had been set, hence the need to unmask
        VectorUtils::FillInValue(masked_from_market_data_bids_map_, 0);
      }
    }
    bestbid_size_ = dep_market_view_.market_update_info_.bestbid_size_;
  }

  if (config_.using_only_full_mkt_for_sim_) {
    if (bestask_int_price_ != dep_market_view_.ask_int_price(0)) {
      bestask_int_price_ = dep_market_view_.ask_int_price(0);
      // reset masks on the askside
      if (masked_asks_) {  // this means that masks had been set, hence the need to unmask
        VectorUtils::FillInValue(masked_from_market_data_asks_map_, 0);
      }
    }
    bestask_size_ = dep_market_view_.ask_size(0);
  } else {
    if (bestask_int_price_ != dep_market_view_.market_update_info_.bestask_int_price_) {
      bestask_int_price_ = dep_market_view_.market_update_info_.bestask_int_price_;
      // reset masks on the askside
      if (masked_asks_) {  // this means that masks had been set, hence the need to unmask
        VectorUtils::FillInValue(masked_from_market_data_asks_map_, 0);
      }
    }
    bestask_size_ = dep_market_view_.market_update_info_.bestask_size_;
  }

  // Matching this Market quote with future trades (only for CFE)
  if (dep_market_view_.this_smv_exch_source_ == kExchSourceCFE) {
    last_market_quote_time_ = watch_.tv();
    CSM_MDS::CSMCommonStruct next_event_;
    while (lookahead_reader_->is_open())  // 2ms into future
    {
      size_t available_len_ = lookahead_reader_->read((void*)&next_event_, sizeof(CSM_MDS::CSMCommonStruct));
      if (available_len_ >= sizeof(next_event_)) {
        if (next_event_.isTradeMsg()) {
          if (dep_market_view_.shortcode().substr(0, 3) == "SP_" ||
              next_event_.data_.csm_trds_.trade_condition[0] != 'S')  // Ensure that this is not a spread trade
          {
            upcoming_trades_.push_back(next_event_);
          }
        }
        ttime_t temp_tv_ = watch_.tv();
        temp_tv_.addmsecs(lookahead_msecs_);  // Looking for time difference less than 2 ms
        if (ttime_t(next_event_.time_) > temp_tv_) {
          break;
        }
      } else
        break;
    }
    for (size_t trade_ctr_ = 0; trade_ctr_ < upcoming_trades_.size(); trade_ctr_++) {
      if (ttime_t(upcoming_trades_[trade_ctr_].time_) <
          watch_.tv()) {  // Might be an implied trade (since TradePrint not called for this trade)
        upcoming_trades_.erase(upcoming_trades_.begin() + trade_ctr_);
        trade_ctr_--;
      } else {
        ttime_t temp_tv_ = watch_.tv();
        temp_tv_.addmsecs(lookahead_msecs_);
        if (temp_tv_ < ttime_t(upcoming_trades_[trade_ctr_].time_)) {  // Trade very far from current time
          break;
        }
        TradeType_t estimated_type_ = MatchTradeAndQuote(
            old_bestask_int_price_, old_bestask_size_, old_bestbid_int_price_, old_bestbid_size_, bestask_int_price_,
            bestask_size_, bestbid_int_price_, bestbid_size_, upcoming_trades_[trade_ctr_]);
        if (estimated_type_ == HFSAT::kTradeTypeBuy ||
            estimated_type_ == HFSAT::kTradeTypeSell) {  // This trade matches with the quote
          TradePrintInfo dummy_trade_print_info_;
          if (estimated_type_ == HFSAT::kTradeTypeBuy) {
            upcoming_trades_[trade_ctr_].data_.csm_trds_.agg_side_ = 1;
            dummy_trade_print_info_.buysell_ = HFSAT::kTradeTypeBuy;
          } else {
            upcoming_trades_[trade_ctr_].data_.csm_trds_.agg_side_ = 2;
            dummy_trade_print_info_.buysell_ = HFSAT::kTradeTypeSell;
          }
          dummy_trade_print_info_.size_traded_ = upcoming_trades_[trade_ctr_].data_.csm_trds_.trd_qty_;
          dummy_trade_print_info_.int_trade_price_ =
              dep_market_view_.GetIntPx(upcoming_trades_[trade_ctr_].data_.csm_trds_.trd_px_);
          is_dummy_trade_print_ = true;  // setting this to true for easy differentiation
          // between Original OnTrade and dummy OnTrade
          OnTradePrint(dep_market_view_.security_id(), dummy_trade_print_info_, market_update_info);
          break;  // break so that we only consider 1st trade in the vec
        }
        break;
      }
    }
  }

  if (config_.use_tgt_sim_market_maker_ || config_.use_baseprice_tgt_sim_market_maker_) {
    if (!is_mov_avg_ready_) {
      if (dep_market_view_.is_ready_complex(2)) {
        is_mov_avg_ready_ = true;
        double t_current_indep_price_ =
            SecurityMarketView::GetPriceFromType(kPriceTypeMktSizeWPrice, market_update_info);
        moving_avg_util_->CalculateValue(t_current_indep_price_);
      }
    } else {
      double t_current_indep_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMktSizeWPrice, market_update_info);
      moving_avg_util_->CalculateValue(t_current_indep_price_);
      prev_dep_price_change_ = t_current_indep_price_ - moving_avg_util_->moving_avg_;
    }
  }

  UpdateAggSizes();
}

void PriceLevelSimMarketMaker::BidUpdateBest(const MarketUpdateInfo& market_update_info, int bid_int_price,
                                             std::vector<BaseSimOrder*>& bid_vector) {
  for (auto p_sim_order_ : bid_vector) {
    if (p_sim_order_ == NULL) {
      continue;
    }

    const int prev_size_ = p_sim_order_->queue_size_behind_ + p_sim_order_->queue_size_ahead_;
    int new_size_ = config_.using_only_full_mkt_for_sim_ ? dep_market_view_.bid_size(0)
                                                         : dep_market_view_.market_update_info_.bestbid_size_;
    int new_ordercount_ = config_.using_only_full_mkt_for_sim_
                              ? dep_market_view_.bid_order(0)
                              : dep_market_view_.market_update_info_.bestbid_ordercount_;

    if (!dep_market_view_.trade_before_quote() && (new_size_ != prev_size_ || p_sim_order_->num_events_seen_ == 1)) {
      BackupQueueSizes(p_sim_order_);
    }

    if (p_sim_order_->num_events_seen_ == 0) {  // first time ... before this do not fill
      p_sim_order_->queue_size_behind_ = 0;
      p_sim_order_->queue_size_ahead_ = config_.using_only_full_mkt_for_sim_
                                            ? dep_market_view_.bid_size(0)
                                            : dep_market_view_.market_update_info_.bestbid_size_;
      p_sim_order_->num_events_seen_ = 1;
    } else {  // not the first time
      switch (current_matching_algo_) {
        case kFIFO: {
          if (config_.use_tgt_sim_market_maker_) {
            UpdateQueueSizesTargetPrice(new_size_, prev_size_, p_sim_order_);
          } else if (config_.use_baseprice_tgt_sim_market_maker_) {
            UpdateQueueSizesBasePriceBasedTargetPrice(new_size_, prev_size_, p_sim_order_);
          } else if (config_.use_fgbm_sim_market_maker_) {
            UpdateQueueSizesTradeBased(new_size_, prev_size_, p_sim_order_);
          } else {
            UpdateQueueSizes(new_size_, prev_size_, p_sim_order_);
          }
          break;
        }

        case kSimpleProRata: {
          bool cancel_priority_order_ = false;
          if (bid_side_priority_order_exists_map_[p_sim_order_->server_assigned_client_id()] &&
              dep_market_view_.mkt_size_weighted_price() - dep_market_view_.mid_price() <=
                  -w2 * dep_market_view_.min_price_increment()) {
            // market price is close to bid price , high chances of cancellation by the priority order
            cancel_priority_order_ = true;
          }

          bool cancel_from_behind_ = false;
          if (dep_market_view_.mkt_size_weighted_price() - dep_market_view_.mid_price() >=
              w1 * dep_market_view_.min_price_increment()) {
            cancel_from_behind_ = true;
          }

          p_sim_order_->EnqueueSimpleProRata(
              new_size_, bid_side_priority_order_size_map_[p_sim_order_->server_assigned_client_id()],
              cancel_from_behind_, cancel_priority_order_);
          if (bid_side_priority_order_size_map_[p_sim_order_->server_assigned_client_id()] == 0) {
            bid_side_priority_order_exists_map_[p_sim_order_->server_assigned_client_id()] = false;
          }
        } break;
        case kTimeProRata: {
          if (dep_market_view_.shortcode().substr(0, 3) == "SP_") {
            if (p_sim_order_->queue_size_ahead_ + p_sim_order_->queue_size_behind_ < new_size_) {
              p_sim_order_->queue_size_behind_ -=
                  ((p_sim_order_->queue_size_ahead_ + p_sim_order_->queue_size_behind_) - new_size_);
            }
            p_sim_order_->queue_size_ahead_ = new_size_ - p_sim_order_->queue_size_behind_;

            if (p_sim_order_->queue_size_ahead_ < 0) {
              p_sim_order_->queue_size_ahead_ = 0;
            }

            p_sim_order_->num_events_seen_++;
          } else {
            p_sim_order_->EnqueueLIFFETimeProRata(new_size_);
          }

        } break;
        case kNewTimeProRata: {
          bool cancel_from_behind_ = false;
          if (dep_market_view_.mkt_size_weighted_price() - dep_market_view_.mid_price() >=
              w1 * dep_market_view_.min_price_increment()) {
            cancel_from_behind_ = true;
          }
          p_sim_order_->EnqueueLIFFENewTimeProRata(new_size_, cancel_from_behind_);
        } break;
        case kSplitFIFOProRata: {
          bool cancel_from_behind_ = false;
          if (watch_.msecs_from_midnight() - last_bid_size_change_msecs_ < MAX_MSECS_TO_CANCEL) {
            cancel_from_behind_ = true;
          }
          if (dep_market_view_.mkt_size_weighted_price() - dep_market_view_.mid_price() >=
              w1 * dep_market_view_.min_price_increment()) {
            cancel_from_behind_ = true;
          }
          // std::cerr << watch_.tv() << " ";
          p_sim_order_->EnqueueSplitFIFOProRata(new_size_, new_ordercount_, fifo_matching_fraction_,
                                                cancel_from_behind_);
        } break;
        default: { break; } break;
      }
    }

    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "Order " << p_sim_order_->security_name() << ' '
                             << GetTradeTypeChar(p_sim_order_->buysell()) << ' ' << p_sim_order_->price() << ' '
                             << p_sim_order_->size_remaining() << " sE:" << p_sim_order_->size_executed() << ' '
                             << ToString(p_sim_order_->order_status()) << "[" << p_sim_order_->queue_size_ahead() << "-"
                             << p_sim_order_->queue_size_behind() << '-' << p_sim_order_->num_events_seen() << ']'
                             << " CAOS: " << p_sim_order_->client_assigned_order_sequence()
                             << " SAOS: " << p_sim_order_->server_assigned_order_sequence() << " ["
                             << dep_market_view_.bestbid_size() << " " << dep_market_view_.bestbid_price() << " * "
                             << dep_market_view_.bestask_price() << " " << dep_market_view_.bestask_size() << "]"
                             << DBGLOG_ENDL_FLUSH;
    }
  }
}

void PriceLevelSimMarketMaker::AskUpdateBest(const MarketUpdateInfo& market_update_info, int ask_int_price,
                                             std::vector<BaseSimOrder*>& ask_vector) {
  for (auto sim_order : ask_vector) {
    if (sim_order == NULL) {
      continue;
    }

    const int prev_size = sim_order->queue_size_behind_ + sim_order->queue_size_ahead_;
    int new_size = config_.using_only_full_mkt_for_sim_ ? dep_market_view_.ask_size(0)
                                                        : dep_market_view_.market_update_info_.bestask_size_;
    int new_ordercount = config_.using_only_full_mkt_for_sim_
                             ? dep_market_view_.ask_order(0)
                             : dep_market_view_.market_update_info_.bestask_ordercount_;

    if (!dep_market_view_.trade_before_quote() && (new_size != prev_size || sim_order->num_events_seen_ == 1)) {
      BackupQueueSizes(sim_order);
    }

    if (sim_order->num_events_seen_ == 0)  // first time ... before this do not fill
    {
      sim_order->queue_size_behind_ = 0;
      sim_order->queue_size_ahead_ = config_.using_only_full_mkt_for_sim_
                                         ? dep_market_view_.ask_size(0)
                                         : dep_market_view_.market_update_info_.bestask_size_;
      sim_order->num_events_seen_ = 1;
    } else  // not the first time
    {
      switch (current_matching_algo_) {
        case kFIFO: {
          if (config_.use_tgt_sim_market_maker_) {
            UpdateQueueSizesTargetPrice(new_size, prev_size, sim_order);
          } else if (config_.use_baseprice_tgt_sim_market_maker_) {
            UpdateQueueSizesBasePriceBasedTargetPrice(new_size, prev_size, sim_order);
          } else if (config_.use_fgbm_sim_market_maker_) {
            UpdateQueueSizesTradeBased(new_size, prev_size, sim_order);
          } else {
            UpdateQueueSizes(new_size, prev_size, sim_order);
          }
        } break;
        case kSimpleProRata: {
          bool cancel_priority_order_ = false;
          if (ask_side_priority_order_exists_map_[sim_order->server_assigned_client_id()] &&
              dep_market_view_.mkt_size_weighted_price() - dep_market_view_.mid_price() >=
                  w2 * dep_market_view_.min_price_increment()) {
            // market price is close to bid price , high chances of cancellation by the priority order
            cancel_priority_order_ = true;
          }

          bool cancel_from_behind_ = false;
          if (dep_market_view_.mkt_size_weighted_price() - dep_market_view_.mid_price() <=
              -w1 * dep_market_view_.min_price_increment()) {
            cancel_from_behind_ = true;
          }
          sim_order->EnqueueSimpleProRata(new_size,
                                          ask_side_priority_order_size_map_[sim_order->server_assigned_client_id()],
                                          cancel_from_behind_, cancel_priority_order_);
          if (ask_side_priority_order_size_map_[sim_order->server_assigned_client_id()] == 0) {
            ask_side_priority_order_exists_map_[sim_order->server_assigned_client_id()] = false;
          }
        } break;
        case kTimeProRata: {
          if (dep_market_view_.shortcode().substr(0, 3) == "SP_") {
            if (sim_order->queue_size_ahead_ + sim_order->queue_size_behind_ < new_size) {
              sim_order->queue_size_behind_ -=
                  (sim_order->queue_size_ahead_ + sim_order->queue_size_behind_) - new_size;
            }
            sim_order->queue_size_ahead_ = new_size - sim_order->queue_size_behind_;
            if (sim_order->queue_size_ahead_ < 0) {
              sim_order->queue_size_ahead_ = 0;
            }
            sim_order->num_events_seen_++;
          } else {
            sim_order->EnqueueLIFFETimeProRata(new_size);
          }
        } break;
        case kNewTimeProRata: {
          bool cancel_from_behind = false;
          if (dep_market_view_.mkt_size_weighted_price() - dep_market_view_.mid_price() <=
              -w1 * dep_market_view_.min_price_increment()) {
            cancel_from_behind = true;
          }

          sim_order->EnqueueLIFFENewTimeProRata(new_size, cancel_from_behind);
        } break;
        case kSplitFIFOProRata: {
          bool cancel_from_behind = false;
          if (watch_.msecs_from_midnight() - last_ask_size_change_msecs_ < MAX_MSECS_TO_CANCEL) {
            cancel_from_behind = true;
          }
          if (dep_market_view_.mkt_size_weighted_price() - dep_market_view_.mid_price() <=
              w1 * dep_market_view_.min_price_increment()) {
            cancel_from_behind = true;
          }

          sim_order->EnqueueSplitFIFOProRata(new_size, new_ordercount, fifo_matching_fraction_, cancel_from_behind);
        } break;
        default: { break; } break;
      }
    }

    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "Order " << sim_order->security_name() << ' ' << GetTradeTypeChar(sim_order->buysell())
                             << ' ' << sim_order->price() << ' ' << sim_order->size_remaining()
                             << " sE:" << sim_order->size_executed() << ' ' << ToString(sim_order->order_status())
                             << " [" << sim_order->queue_size_ahead() << "-" << sim_order->queue_size_behind() << '-'
                             << sim_order->num_events_seen() << ']'
                             << " CAOS: " << sim_order->client_assigned_order_sequence()
                             << " SAOS: " << sim_order->server_assigned_order_sequence() << " ["
                             << dep_market_view_.bestbid_size() << " " << dep_market_view_.bestbid_price() << " * "
                             << dep_market_view_.bestask_price() << " " << dep_market_view_.bestask_size() << "]"
                             << DBGLOG_ENDL_FLUSH;
    }
  }
}

void PriceLevelSimMarketMaker::BidUpdateImprove(std::vector<BaseSimOrder*>& bid_vector) {
  for (auto sim_order : bid_vector) {
    switch (current_matching_algo_) {
      case kFIFO:
        sim_order->Enqueue(0);
        break;
      case kTimeProRata:
        sim_order->EnqueueLIFFETimeProRata(0);
        break;
      case kNewTimeProRata:
        sim_order->EnqueueLIFFENewTimeProRata(0);
        break;
      case kSplitFIFOProRata: {
        sim_order->EnqueueSplitFIFOProRata(0, 0, fifo_matching_fraction_);
        break;
      }
      default:
        break;
    }
  }
}

void PriceLevelSimMarketMaker::AskUpdateImprove(std::vector<BaseSimOrder*>& ask_vector) {
  for (auto sim_order : ask_vector) {
    switch (current_matching_algo_) {
      case kFIFO:
        sim_order->Enqueue(0);
        break;
      case kSimpleProRata: {
        int p_size_ = 0;
        sim_order->EnqueueSimpleProRata(0, p_size_);
      } break;
      case kTimeProRata:
        sim_order->EnqueueLIFFETimeProRata(0);
        break;
      case kNewTimeProRata:
        sim_order->EnqueueLIFFENewTimeProRata(0);
        break;
      case kSplitFIFOProRata: {
        // std::cerr << watch_.tv() << " ";
        sim_order->EnqueueSplitFIFOProRata(0, 0, fifo_matching_fraction_);
      } break;
      default:
        break;
    }
  }
}

void PriceLevelSimMarketMaker::BidUpdateAggress(int bid_int_price, std::vector<BaseSimOrder*>& bid_vector) {
  for (BaseSimOrderPtrVec::iterator t_sim_order_vec_iter_ = bid_vector.begin();
       t_sim_order_vec_iter_ != bid_vector.end();) {
    BaseSimOrder* sim_order = *t_sim_order_vec_iter_;

    int available_size_for_exec_ = 999999;  // Very high value
    int this_size_executed_ = 0;

    if (bid_int_price == bestask_int_price_) {
      available_size_for_exec_ =
          std::max(bestask_size_ - masked_from_market_data_asks_map_[sim_order->server_assigned_client_id()], 0);
    }

    if (available_size_for_exec_ >= sim_order->size_remaining()) {
      this_size_executed_ = sim_order->ExecuteRemaining();
    } else {
      this_size_executed_ = sim_order->MatchPartial(available_size_for_exec_);
    }

    client_position_map_[sim_order->server_assigned_client_id()] += this_size_executed_;
    global_position_to_send_map_[sim_order->server_assigned_client_id()] += this_size_executed_;

    masked_from_market_data_asks_map_[sim_order->server_assigned_client_id()] += this_size_executed_;
    masked_asks_ = true;

    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "NowAggExecutedOrder " << sim_order->security_name() << ' '
                             << GetTradeTypeChar(sim_order->buysell()) << ' ' << sim_order->price() << ' '
                             << sim_order->size_remaining() << " sE:" << sim_order->size_executed() << ' '
                             << ToString(sim_order->order_status()) << " [" << sim_order->queue_size_ahead() << "-"
                             << sim_order->queue_size_behind() << '-' << sim_order->num_events_seen() << ']'
                             << " CAOS: " << sim_order->client_assigned_order_sequence()
                             << " SAOS: " << sim_order->server_assigned_order_sequence()
                             << " SACI: " << sim_order->server_assigned_client_id() << DBGLOG_ENDL_FLUSH;
    }

    BroadcastExecNotification(sim_order->server_assigned_client_id(), sim_order);

    if (sim_order->size_remaining() == 0) {
      // Dealloc order and erase from the vector
      basesimorder_mempool_.DeAlloc(sim_order);
      t_sim_order_vec_iter_ = bid_vector.erase(t_sim_order_vec_iter_);
    } else {
      t_sim_order_vec_iter_++;
    }
  }
}

void PriceLevelSimMarketMaker::AskUpdateAggress(int ask_int_price, std::vector<BaseSimOrder*>& ask_vector) {
  for (BaseSimOrderPtrVec::iterator t_sim_order_vec_iter_ = ask_vector.begin();
       t_sim_order_vec_iter_ != ask_vector.end();) {
    BaseSimOrder* sim_order = *t_sim_order_vec_iter_;

    int available_size_for_exec_ = 999999;  // Very high value
    int this_size_executed_ = 0;

    if (ask_int_price == bestbid_int_price_) {
      available_size_for_exec_ =
          std::max(bestbid_size_ - masked_from_market_data_bids_map_[sim_order->server_assigned_client_id()], 0);
    }

    if (available_size_for_exec_ >= sim_order->size_remaining()) {
      this_size_executed_ = sim_order->ExecuteRemaining();
    } else {
      this_size_executed_ = sim_order->MatchPartial(available_size_for_exec_);
    }

    client_position_map_[sim_order->server_assigned_client_id()] -= this_size_executed_;
    global_position_to_send_map_[sim_order->server_assigned_client_id()] -= this_size_executed_;

    masked_from_market_data_bids_map_[sim_order->server_assigned_client_id()] += this_size_executed_;
    masked_bids_ = true;

    if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "NowAggExecutedOrder " << sim_order->security_name() << ' '
                             << GetTradeTypeChar(sim_order->buysell()) << ' ' << sim_order->price() << ' '
                             << sim_order->size_remaining() << " sE:" << sim_order->size_executed() << ' '
                             << ToString(sim_order->order_status()) << " [" << sim_order->queue_size_ahead() << "-"
                             << sim_order->queue_size_behind() << '-' << sim_order->num_events_seen() << ']'
                             << " CAOS: " << sim_order->client_assigned_order_sequence()
                             << " SAOS: " << sim_order->server_assigned_order_sequence()
                             << " SACI: " << sim_order->server_assigned_client_id() << DBGLOG_ENDL_FLUSH;
    }

    BroadcastExecNotification(sim_order->server_assigned_client_id(), sim_order);

    if (sim_order->size_remaining() <= 0) {
      // Dealloc order and erase iterator from the vector

      basesimorder_mempool_.DeAlloc(sim_order);
      t_sim_order_vec_iter_ = ask_vector.erase(t_sim_order_vec_iter_);
    } else {
      t_sim_order_vec_iter_++;
    }
  }
}

void PriceLevelSimMarketMaker::OnMarketUpdate(const unsigned int _security_id_,
                                              const MarketUpdateInfo& _market_update_info_) {
  if (!process_mkt_updates_) return;

  InitializeOnMktUpdate(_market_update_info_);

  // Check if any orders have become aggressive if so fill them
  // Call Enqueue on orders at or above best market
  for (BidPriceSimOrderMapIter_t i2bov_iter_ = intpx_bid_order_map_.begin();
       i2bov_iter_ != intpx_bid_order_map_.end() && i2bov_iter_->first >= bestbid_int_price_; i2bov_iter_++) {
    if (i2bov_iter_->first >= bestask_int_price_) {
      BidUpdateAggress(i2bov_iter_->first, i2bov_iter_->second);
    } else if (i2bov_iter_->first > bestbid_int_price_) {
      BidUpdateImprove(i2bov_iter_->second);
    } else if (i2bov_iter_->first == bestbid_int_price_) {
      BidUpdateBest(_market_update_info_, bestbid_int_price_, i2bov_iter_->second);
    } else {
      break;
    }
  }

  for (AskPriceSimOrderMapIter_t i2aov_iter_ = intpx_ask_order_map_.begin();
       (i2aov_iter_ != intpx_ask_order_map_.end()) && (i2aov_iter_->first <= bestask_int_price_); i2aov_iter_++) {
    if (i2aov_iter_->first <= bestbid_int_price_) {
      AskUpdateAggress(i2aov_iter_->first, i2aov_iter_->second);
    } else if (i2aov_iter_->first < bestask_int_price_) {
      AskUpdateImprove(i2aov_iter_->second);
    } else if (i2aov_iter_->first == bestask_int_price_) {
      AskUpdateBest(_market_update_info_, bestask_int_price_, i2aov_iter_->second);
    } else {
      break;
    }
  }

  if (dep_market_view_.this_smv_exch_source_ == kExchSourceHONGKONG ||
      dep_market_view_.this_smv_exch_source_ == kExchSourceJPY) {
    HKOSENonBestHandling();
  }
}

void PriceLevelSimMarketMaker::HKOSENonBestHandling() {
#define ALL_LEVELS_QUEUE_INTERVAL 0
  if ((last_all_levels_queued_ == 0) ||
      (watch_.msecs_from_midnight() - last_all_levels_queued_ > ALL_LEVELS_QUEUE_INTERVAL)) {
    last_all_levels_queued_ = watch_.msecs_from_midnight();

    unsigned int lower_level_qsa_current_bid_index_ = 0;
    unsigned int lower_level_qsa_current_ask_index_ = 0;
    if (dep_market_view_.market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
      lower_level_qsa_current_bid_index_ =
          dep_market_view_.IndexedBookGetNextNonEmptyBidMapIndex(dep_market_view_.base_bid_index_);
      lower_level_qsa_current_ask_index_ =
          dep_market_view_.IndexedBookGetNextNonEmptyAskMapIndex(dep_market_view_.base_ask_index_);
    } else {
      lower_level_qsa_current_bid_index_ = 1;  // second level
      lower_level_qsa_current_ask_index_ = 1;  // second level
    }

    for (BidPriceSimOrderMapIter_t i2bov_iter_ = intpx_bid_order_map_.begin();
         (i2bov_iter_ != intpx_bid_order_map_.end()); i2bov_iter_++) {
      if (i2bov_iter_->first >= bestbid_int_price_) {
        continue;
      }

      // now at non-best level
      int size_at_level_ = 0;
      if (dep_market_view_.market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
        // use get size at price
        while ((lower_level_qsa_current_bid_index_ > 0) &&
               (dep_market_view_.market_update_info_.bidlevels_[lower_level_qsa_current_bid_index_].limit_int_price_ >
                i2bov_iter_->first)) {
          lower_level_qsa_current_bid_index_ =
              dep_market_view_.IndexedBookGetNextNonEmptyBidMapIndex(lower_level_qsa_current_bid_index_);
        }
        if ((lower_level_qsa_current_bid_index_ > 0) &&
            (dep_market_view_.market_update_info_.bidlevels_[lower_level_qsa_current_bid_index_].limit_int_price_ ==
             i2bov_iter_->first)) {
          size_at_level_ =
              dep_market_view_.market_update_info_.bidlevels_[lower_level_qsa_current_bid_index_].limit_size_;
        }
      } else {
        while ((lower_level_qsa_current_bid_index_ < (unsigned int)dep_market_view_.NumBidLevels()) &&
               (dep_market_view_.market_update_info_.bidlevels_[lower_level_qsa_current_bid_index_].limit_int_price_ >
                (int)i2bov_iter_->first)) {
          lower_level_qsa_current_bid_index_++;
        }
        if ((lower_level_qsa_current_bid_index_ < (unsigned int)dep_market_view_.NumBidLevels()) &&
            (dep_market_view_.market_update_info_.bidlevels_[lower_level_qsa_current_bid_index_].limit_int_price_ ==
             (int)i2bov_iter_->first)) {
          size_at_level_ =
              dep_market_view_.market_update_info_.bidlevels_[lower_level_qsa_current_bid_index_].limit_size_;
        }
      }

      if (size_at_level_ > 0) {
        for (size_t i = 0; i < (i2bov_iter_->second).size(); i++) {
          BaseSimOrder* p_sim_order_ = (i2bov_iter_->second)[i];
          if (p_sim_order_ == NULL) continue;
          if (p_sim_order_->queue_size_ahead_ > size_at_level_) {
            p_sim_order_->queue_size_ahead_ = size_at_level_;
          }
          p_sim_order_->queue_size_behind_ = size_at_level_ - p_sim_order_->queue_size_ahead_;

          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "SupOrder " << p_sim_order_->security_name() << ' '
                                   << GetTradeTypeChar(p_sim_order_->buysell()) << ' ' << p_sim_order_->price() << ' '
                                   << p_sim_order_->size_remaining() << " sE:" << p_sim_order_->size_executed() << ' '
                                   << ToString(p_sim_order_->order_status()) << " [" << p_sim_order_->queue_size_ahead()
                                   << "-" << p_sim_order_->queue_size_behind() << '-' << p_sim_order_->num_events_seen()
                                   << ']' << " CAOS: " << p_sim_order_->client_assigned_order_sequence()
                                   << " SAOS: " << p_sim_order_->server_assigned_order_sequence() << " ["
                                   << dep_market_view_.bestbid_size() << " " << dep_market_view_.bestbid_price()
                                   << " * " << dep_market_view_.bestask_price() << " "
                                   << dep_market_view_.bestask_size() << "]" << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }

    for (AskPriceSimOrderMapIter_t i2aov_iter_ = intpx_ask_order_map_.begin();
         (i2aov_iter_ != intpx_ask_order_map_.end()); i2aov_iter_++) {
      if (i2aov_iter_->first <= bestask_int_price_) {
        continue;
      }

      // now at non-best level
      int size_at_level_ = 0;
      if (dep_market_view_.market_update_info_.temporary_bool_checking_if_this_is_an_indexed_book_) {
        while ((lower_level_qsa_current_ask_index_ > 0) &&
               (dep_market_view_.market_update_info_.asklevels_[lower_level_qsa_current_ask_index_].limit_int_price_ <
                i2aov_iter_->first)) {
          lower_level_qsa_current_ask_index_ =
              dep_market_view_.IndexedBookGetNextNonEmptyAskMapIndex(lower_level_qsa_current_ask_index_);
        }
        if ((lower_level_qsa_current_ask_index_ > 0) &&
            (dep_market_view_.market_update_info_.asklevels_[lower_level_qsa_current_ask_index_].limit_int_price_ ==
             i2aov_iter_->first)) {
          size_at_level_ =
              dep_market_view_.market_update_info_.asklevels_[lower_level_qsa_current_ask_index_].limit_size_;
        }
      } else {
        while ((lower_level_qsa_current_ask_index_ < (unsigned int)dep_market_view_.NumAskLevels()) &&
               (dep_market_view_.market_update_info_.asklevels_[lower_level_qsa_current_ask_index_].limit_int_price_ <
                (int)i2aov_iter_->first)) {
          lower_level_qsa_current_ask_index_++;
        }
        if ((lower_level_qsa_current_ask_index_ < (unsigned int)dep_market_view_.NumAskLevels()) &&
            (dep_market_view_.market_update_info_.asklevels_[lower_level_qsa_current_ask_index_].limit_int_price_ ==
             (int)i2aov_iter_->first)) {
          size_at_level_ =
              dep_market_view_.market_update_info_.asklevels_[lower_level_qsa_current_ask_index_].limit_size_;
        }
      }

      if (size_at_level_ > 0) {
        for (size_t i = 0; i < (i2aov_iter_->second).size(); i++) {
          BaseSimOrder* p_sim_order_ = (i2aov_iter_->second)[i];
          if (p_sim_order_ == NULL) continue;
          if (p_sim_order_->queue_size_ahead_ > size_at_level_) {
            p_sim_order_->queue_size_ahead_ = size_at_level_;
          }
          p_sim_order_->queue_size_behind_ = size_at_level_ - p_sim_order_->queue_size_ahead_;

          if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
            DBGLOG_TIME_CLASS_FUNC << "SupOrder " << p_sim_order_->security_name() << ' '
                                   << GetTradeTypeChar(p_sim_order_->buysell()) << ' ' << p_sim_order_->price() << ' '
                                   << p_sim_order_->size_remaining() << " sE:" << p_sim_order_->size_executed() << ' '
                                   << ToString(p_sim_order_->order_status()) << " [" << p_sim_order_->queue_size_ahead()
                                   << "-" << p_sim_order_->queue_size_behind() << '-' << p_sim_order_->num_events_seen()
                                   << ']' << " CAOS: " << p_sim_order_->client_assigned_order_sequence()
                                   << " SAOS: " << p_sim_order_->server_assigned_order_sequence() << " ["
                                   << dep_market_view_.bestbid_size() << " " << dep_market_view_.bestbid_price()
                                   << " * " << dep_market_view_.bestask_price() << " "
                                   << dep_market_view_.bestask_size() << "]" << DBGLOG_ENDL_FLUSH;
          }
        }
      }
    }
  }
#undef ALL_LEVELS_QUEUE_INTERVAL
}

void PriceLevelSimMarketMaker::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                            const MarketUpdateInfo& _market_update_info_) {
  // Check if this trade was already considered (after getting matched to a market quote): only for CFE
  if (!process_mkt_updates_) return;

  if (config_.use_tgt_sim_market_maker_ || config_.use_baseprice_tgt_sim_market_maker_) {
    double t_current_indep_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMktSizeWPrice, _market_update_info_);
    moving_avg_util_->CalculateValue(t_current_indep_price_);
    prev_dep_price_change_ = t_current_indep_price_ - moving_avg_util_->moving_avg_;
  }

  // When we use simple sim, we award executions only based on price level changes
  if (config_.use_simple_sim_) {
    return;
  }

  if (dep_market_view_.this_smv_exch_source_ == kExchSourceCFE) {
    size_t trade_ctr_;
    for (trade_ctr_ = 0; trade_ctr_ < upcoming_trades_.size(); trade_ctr_++) {
      // Check equality condition
      if ((int)upcoming_trades_[trade_ctr_].data_.csm_trds_.trd_qty_ == _trade_print_info_.size_traded_ &&
          dep_market_view_.GetIntPx(upcoming_trades_[trade_ctr_].data_.csm_trds_.trd_px_) ==
              _trade_print_info_.int_trade_price_ &&
          is_dummy_trade_print_) {  // This Trade has not been considered yet
        upcoming_trades_.erase(upcoming_trades_.begin() + trade_ctr_);
        is_dummy_trade_print_ = false;
        break;
      }
    }
    if (trade_ctr_ == upcoming_trades_.size()) {  // Trade not found in upcoming_trades_
      ttime_t temp_tv_ = last_market_quote_time_;
      temp_tv_.addmsecs(lookahead_msecs_);
      if (watch_.tv() <= temp_tv_) {  // There were market quotes in previous 2 ms, so this trade should have been
                                      // present in upcoming_trades_
        return;  // This trade has been already considered before (due to a successful match with a previous quote)
      }
    }
  }

  if (!all_requests_.empty()) {
    ProcessRequestQueue();
  }

  TradeType_t t_trade_print_info_buysell_ = _trade_print_info_.buysell_;

  if ((t_trade_print_info_buysell_ == kTradeTypeBuy) ||
      (t_trade_print_info_buysell_ == kTradeTypeNoInfo)) {  // aggressor side was Buy
    askside_trade_size_ = _trade_print_info_.size_traded_;
    if (!(config_.use_tgt_sim_market_maker_ || config_.use_baseprice_tgt_sim_market_maker_)) {
      if (!dep_market_view_.trade_before_quote()) {
        current_ask_trade_ratio_vec_[_trade_print_info_.int_trade_price_] =
            double(askside_trade_size_) /
            double(dep_market_view_.ask_size_at_int_price(_trade_print_info_.int_trade_price_) + askside_trade_size_);
      } else {
        if (dep_market_view_.ask_size_at_int_price(_trade_print_info_.int_trade_price_) > 0)
          current_ask_trade_ratio_vec_[_trade_print_info_.int_trade_price_] =
              double(askside_trade_size_) /
              double(dep_market_view_.ask_size_at_int_price(_trade_print_info_.int_trade_price_));
        else
          current_ask_trade_ratio_vec_[_trade_print_info_.int_trade_price_] = 1.0;
      }
      if (config_.use_fgbm_sim_market_maker_) {
        current_ask_trade_time_[_trade_print_info_.int_trade_price_] = watch_.tv();
      }
    }

    if (masked_asks_) {
      masked_asks_ = false;
      if (bestask_int_price_ == _trade_print_info_.int_trade_price_) {
        for (size_t i = 0; i < masked_from_market_data_asks_map_.size(); i++) {
          masked_from_market_data_asks_map_[i] =
              std::max(masked_from_market_data_asks_map_[i] - _trade_print_info_.size_traded_, 0);

          if (masked_from_market_data_asks_map_[i] > 0) {
            masked_asks_ = true;
          }
        }
      } else {
        VectorUtils::FillInValue(masked_from_market_data_asks_map_, 0);
      }
    }

    if (config_.using_only_full_mkt_for_sim_) {
      if ((!(dep_market_view_.IsAskBookEmpty())) &&
          (dep_market_view_.ask_int_price(0) <
           _trade_print_info_.int_trade_price_)) {  // latest aggressive buy order received at a price higher than
                                                    // normalized bestask in market data
        // weirdness

        if (dbglogger_.CheckLoggingLevel(PLSMM_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "Weirdness latest aggressive buy order received at a price higher than current "
                                    "best matching limit order" << DBGLOG_ENDL_FLUSH;
        }
        // ignoring these trades for now ... we can see fi this happens often
        return;
      }
    } else {
      if (dep_market_view_.market_update_info_.bestask_int_price_ < _trade_print_info_.int_trade_price_)

      {  // latest aggressive buy order received at a price higher than normalized bestask in market data
        // weirdness

        if (dbglogger_.CheckLoggingLevel(PLSMM_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << "Weirdness latest aggressive buy order received at a price higher than current "
                                    "best matching limit order" << DBGLOG_ENDL_FLUSH;
        }
        // ignoring these trades for now ... we can see fi this happens often
        return;
      }
    }

    for (AskPriceSimOrderMapIter_t i2aov_iter_ = intpx_ask_order_map_.begin();
         (i2aov_iter_ != intpx_ask_order_map_.end()) && (i2aov_iter_->first <= _trade_print_info_.int_trade_price_);
         i2aov_iter_++) {
      if (i2aov_iter_->first < _trade_print_info_.int_trade_price_) {  ///< Limit Ask Order at a higher level than Lift
        /// Price (currently not checking masks .. simply
        /// filling !)

        if (!((i2aov_iter_->second).empty())) {  // non-zero orders at this price level
          for (size_t i = 0; i < (i2aov_iter_->second).size(); i++) {
            BaseSimOrder* p_sim_order_ = (i2aov_iter_->second)[i];
            int this_size_executed_ = p_sim_order_->ExecuteRemaining();
            client_position_map_[p_sim_order_->server_assigned_client_id()] -= this_size_executed_;
            global_position_to_send_map_[p_sim_order_->server_assigned_client_id()] -= this_size_executed_;

            LogExec(p_sim_order_, this_size_executed_);

            BroadcastExecNotification(p_sim_order_->server_assigned_client_id(), p_sim_order_);

            // dealloc every order and set pointer value to null

            basesimorder_mempool_.DeAlloc(p_sim_order_);
            (i2aov_iter_->second)[i] = NULL;
          }
          // clear the vector
          (i2aov_iter_->second).clear();
        }
      } else {  ///< (i2aov_iter_->first == _trade_print_info_.int_trade_price_) ... Limit Ask Order at a same
        /// level/price than Lift trade in market. (Check to see if executed, and Enqueue if not finished)

        if (!((i2aov_iter_->second).empty())) {  // there are orders at this price level
          /// an estimate of the total_market_non_self_size at this level after this trade, we can use it to adjust
          /// queue_size_ahead_ and queue_size_behind_
          int t_posttrade_asksize_at_trade_price_;
          t_posttrade_asksize_at_trade_price_ =
              ((dep_market_view_.market_update_info_.bestask_int_price_ > _trade_print_info_.int_trade_price_)
                   ? 0
                   : dep_market_view_.market_update_info_.bestask_size_);

          /// an estimate of the total_market_non_self_size at this level after this trade, we can use it to adjust
          /// queue_size_ahead_ and queue_size_behind_

          BaseSimOrderPtrVec& t_this_sim_order_vec_ = i2aov_iter_->second;

          for (size_t i = 0; i < saci_to_executed_size_.size(); i++) {
            saci_to_executed_size_[i] = 0;
          }

          for (BaseSimOrderPtrVec::iterator t_sim_order_vec_iter_ = t_this_sim_order_vec_.begin();
               t_sim_order_vec_iter_ !=
                   t_this_sim_order_vec_.end();) {  // check which orders are executed, send message and
            // deallocate order, nullify the pointer, erase from vector.
            // Note the iterator does not need to be incremented since we either break out of loop or erase the iterator
            // and hence increment it.

            BaseSimOrder* const p_sim_order_ = (*t_sim_order_vec_iter_);

            int trd_size_ = _trade_print_info_.size_traded_;
            if (!dep_market_view_.trade_before_quote())
              trd_size_ = RestoreQueueSizes(p_sim_order_, t_posttrade_asksize_at_trade_price_, trd_size_);

            int trade_size_to_be_used_ =
                _trade_print_info_.size_traded_ - saci_to_executed_size_[p_sim_order_->server_assigned_client_id()];

            if (trade_size_to_be_used_ <= 0) {
              if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << "Trade size exhausted for this client id: "
                                       << p_sim_order_->server_assigned_client_id() << DBGLOG_ENDL_FLUSH;
              }
              t_sim_order_vec_iter_++;
              continue;
            }

            if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "Order " << p_sim_order_->security_name() << ' '
                                     << GetTradeTypeChar(p_sim_order_->buysell()) << ' ' << p_sim_order_->price() << ' '
                                     << p_sim_order_->size_remaining() << " sE:" << p_sim_order_->size_executed() << ' '
                                     << ToString(p_sim_order_->order_status()) << " ["
                                     << p_sim_order_->queue_size_ahead() << "-" << p_sim_order_->queue_size_behind()
                                     << '-' << p_sim_order_->num_events_seen() << ']'
                                     << " CAOS: " << p_sim_order_->client_assigned_order_sequence()
                                     << " SAOS: " << p_sim_order_->server_assigned_order_sequence() << " HCT ("
                                     << _trade_print_info_.size_traded_ << ") "
                                     << " [" << dep_market_view_.bestbid_size() << " "
                                     << dep_market_view_.bestbid_price() << " * " << dep_market_view_.bestask_price()
                                     << " " << dep_market_view_.bestask_size() << "]" << DBGLOG_ENDL_FLUSH;
            }

            int t_size_executed_ = 0;
            switch (current_matching_algo_) {
              case kFIFO: {
                t_size_executed_ =
                    p_sim_order_->HandleCrossingTrade(trade_size_to_be_used_, t_posttrade_asksize_at_trade_price_);
              } break;
              case kSimpleProRata: {
                // std::cerr << watch_.tv() << " ";
                t_size_executed_ = p_sim_order_->HandleCrossingTradeSimpleProrata(
                    trade_size_to_be_used_, t_posttrade_asksize_at_trade_price_,
                    ask_side_priority_order_size_map_[p_sim_order_->server_assigned_client_id()],
                    ask_side_priority_order_exists_map_[p_sim_order_->server_assigned_client_id()]);
                if (ask_side_priority_order_size_map_[p_sim_order_->server_assigned_client_id()] <= 0) {
                  ask_side_priority_order_exists_map_[p_sim_order_->server_assigned_client_id()] = false;
                }
              } break;
              case kTimeProRata: {
                t_size_executed_ = p_sim_order_->HandleCrossingTradeTimeProrataLIFFE(
                    trade_size_to_be_used_, t_posttrade_asksize_at_trade_price_,
                    ask_side_priority_order_size_map_[p_sim_order_->server_assigned_client_id()],
                    ask_side_priority_order_exists_map_[p_sim_order_->server_assigned_client_id()]);
              } break;
              case kNewTimeProRata: {
                t_size_executed_ = p_sim_order_->HandleCrossingTradeNewTimeProrataLIFFE(
                    trade_size_to_be_used_, t_posttrade_asksize_at_trade_price_,
                    ask_side_priority_order_size_map_[p_sim_order_->server_assigned_client_id()],
                    ask_side_priority_order_exists_map_[p_sim_order_->server_assigned_client_id()]);
              } break;
              case kSplitFIFOProRata: {
                t_size_executed_ = p_sim_order_->HandleCrossingTradeSplitFIFOProRata(
                    trade_size_to_be_used_, t_posttrade_asksize_at_trade_price_, fifo_matching_fraction_);
              } break;
              default: { break; } break;
            }
            if (t_size_executed_ > 0) {
              client_position_map_[p_sim_order_->server_assigned_client_id()] -= t_size_executed_;
              global_position_to_send_map_[p_sim_order_->server_assigned_client_id()] -= t_size_executed_;
              saci_to_executed_size_[p_sim_order_->server_assigned_client_id()] += t_size_executed_;
              LogExec(p_sim_order_, t_size_executed_);

              BroadcastExecNotification(p_sim_order_->server_assigned_client_id(), p_sim_order_);
              if (p_sim_order_->size_remaining() <= 0) {
                basesimorder_mempool_.DeAlloc(p_sim_order_);
                t_sim_order_vec_iter_ = t_this_sim_order_vec_.erase(t_sim_order_vec_iter_);
              } else {
                if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
                  DBGLOG_TIME_CLASS_FUNC << "QueueOrder SACI: " << p_sim_order_->server_assigned_client_id() << ' '
                                         << p_sim_order_->security_name() << ' '
                                         << GetTradeTypeChar(p_sim_order_->buysell()) << ' ' << p_sim_order_->price()
                                         << ' ' << p_sim_order_->size_remaining()
                                         << " sE:" << p_sim_order_->size_executed() << ' '
                                         << ToString(p_sim_order_->order_status()) << " ["
                                         << p_sim_order_->queue_size_ahead() << "-" << p_sim_order_->queue_size_behind()
                                         << '-' << p_sim_order_->num_events_seen() << ']'
                                         << " CAOS: " << p_sim_order_->client_assigned_order_sequence()
                                         << " SAOS: " << p_sim_order_->server_assigned_order_sequence() << " HCT ("
                                         << _trade_print_info_.size_traded_ << ") "
                                         << " [" << dep_market_view_.bestbid_size() << " "
                                         << dep_market_view_.bestbid_price() << " * "
                                         << dep_market_view_.bestask_price() << " " << dep_market_view_.bestask_size()
                                         << "]"
                                         << " size_executed: " << t_size_executed_ << DBGLOG_ENDL_FLUSH;
                }

                // // again like below if this order is not cleared others in the queue below this won't be either
                // break;
                t_sim_order_vec_iter_++;
              }
            } else {
              // // if this order was not executed, it is likely that others after this would not be executed as well.
              // // only way this breaks if size was modified of an order and hence it was put back in queue.
              // // we just need to make sure that if the vector has more than 1 order, when an order is modified up in
              // size
              // // we actually remove the order at put it back in the vector as well.
              // break;
              t_sim_order_vec_iter_++;
            }
          }
        }
      }
    }
  }

  if ((t_trade_print_info_buysell_ == kTradeTypeSell) ||
      (t_trade_print_info_buysell_ == kTradeTypeNoInfo)) {  // trade was a HIT, i.e. removing liquidity on the bid side
    bidside_trade_size_ = _trade_print_info_.size_traded_;
    // if (HFSAT::SecurityDefinitions::GetContractExchSource (dep_market_view_.shortcode(), watch_.YYYYMMDD()) ==
    // HFSAT::kExchSourceLIFFE)
    if (!(config_.use_tgt_sim_market_maker_ || config_.use_baseprice_tgt_sim_market_maker_)) {
      if (!dep_market_view_.trade_before_quote()) {
        current_bid_trade_ratio_vec_[_trade_print_info_.int_trade_price_] =
            double(bidside_trade_size_) /
            double(dep_market_view_.bid_size_at_int_price(_trade_print_info_.int_trade_price_) + bidside_trade_size_);
      } else {
        if (dep_market_view_.bid_size_at_int_price(_trade_print_info_.int_trade_price_) > 0) {
          current_bid_trade_ratio_vec_[_trade_print_info_.int_trade_price_] =
              double(bidside_trade_size_) /
              double(dep_market_view_.bid_size_at_int_price(_trade_print_info_.int_trade_price_));
        } else {
          current_bid_trade_ratio_vec_[_trade_print_info_.int_trade_price_] = 1.0;
        }
      }
      if (config_.use_fgbm_sim_market_maker_) {
        current_bid_trade_time_[_trade_print_info_.int_trade_price_] = watch_.tv();
      }
    }

    if (masked_bids_) {
      masked_bids_ = false;
      if (bestbid_int_price_ == _trade_print_info_.int_trade_price_) {
        for (size_t i = 0; i < masked_from_market_data_bids_map_.size(); i++) {
          masked_from_market_data_bids_map_[i] =
              std::max(masked_from_market_data_bids_map_[i] - _trade_print_info_.size_traded_, 0);

          if (masked_from_market_data_bids_map_[i] > 0) {
            masked_bids_ = true;
          }
        }
      } else {
        VectorUtils::FillInValue(masked_from_market_data_bids_map_, 0);
      }
    }

    if (config_.using_only_full_mkt_for_sim_) {
      // weirdness
      if (!dep_market_view_.IsBidBookEmpty() &&
          dep_market_view_.bid_int_price(0) > _trade_print_info_.int_trade_price_) {
        if (dbglogger_.CheckLoggingLevel(PLSMM_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << " Weirdness latest aggressive sell order received at a price lower than current "
                                    "best matching limit order" << DBGLOG_ENDL_FLUSH;
        }
        // ignoring these trades for now ... we can see fi this happens often
        return;
      }
    } else {
      // weirdness
      if (dep_market_view_.market_update_info_.bestbid_int_price_ > _trade_print_info_.int_trade_price_) {
        if (dbglogger_.CheckLoggingLevel(PLSMM_ERROR)) {
          DBGLOG_TIME_CLASS_FUNC << " Weirdness latest aggressive sell order received at a price lower than current "
                                    "best matching limit order" << DBGLOG_ENDL_FLUSH;
        }
        // ignoring these trades for now ... we can see fi this happens often
        return;
      }
    }

    for (BidPriceSimOrderMapIter_t i2bov_iter_ = intpx_bid_order_map_.begin();
         (i2bov_iter_ != intpx_bid_order_map_.end()) && (i2bov_iter_->first >= _trade_print_info_.int_trade_price_);
         i2bov_iter_++) {
      if (i2bov_iter_->first > _trade_print_info_.int_trade_price_) {  // Aggressive Order at a lower level (currently
                                                                       // not checking masks .. simply filling !)

        if (!((i2bov_iter_->second).empty())) {
          for (size_t i = 0; i < (i2bov_iter_->second).size(); i++) {
            BaseSimOrder* p_sim_order_ = (i2bov_iter_->second)[i];
            int this_size_executed_ = p_sim_order_->ExecuteRemaining();
            client_position_map_[p_sim_order_->server_assigned_client_id()] += this_size_executed_;
            global_position_to_send_map_[p_sim_order_->server_assigned_client_id()] += this_size_executed_;
            LogExec(p_sim_order_, this_size_executed_);

            BroadcastExecNotification(p_sim_order_->server_assigned_client_id(), p_sim_order_);

            // dealloc every order and set pointer value to null

            basesimorder_mempool_.DeAlloc(p_sim_order_);
            (i2bov_iter_->second)[i] = NULL;
          }
          // clear the vector
          (i2bov_iter_->second).clear();
        }
      } else {  // (i2bov_iter_->first == _trade_print_info_.int_trade_price_) ... trade at best-nonself-market. Check
                // to see if executed, and Enqueue if not finished
        if (!((i2bov_iter_->second).empty())) {
          /// an estimate of the total_market_non_self_size at this level after this trade, we can use it to adjust
          /// queue_size_ahead_ and queue_size_behind_
          int t_posttrade_bidsize_at_trade_price_;
          t_posttrade_bidsize_at_trade_price_ =
              ((dep_market_view_.market_update_info_.bestbid_int_price_ < _trade_print_info_.int_trade_price_)
                   ? 0
                   : dep_market_view_.market_update_info_.bestbid_size_);

          /// an estimate of the total_market_non_self_size at this level after this trade, we can use it to adjust
          /// queue_size_ahead_ and queue_size_behind_

          BaseSimOrderPtrVec& t_this_sim_order_vec_ = i2bov_iter_->second;

          for (size_t i = 0; i < saci_to_executed_size_.size(); i++) {
            saci_to_executed_size_[i] = 0;
          }

          for (BaseSimOrderPtrVec::iterator t_sim_order_vec_iter_ = t_this_sim_order_vec_.begin();
               t_sim_order_vec_iter_ !=
                   t_this_sim_order_vec_.end();) {  // check which orders are executed, send message and
            // deallocate order, nullify the pointer, erase from vector.
            // Note the iterator does not need to be incremented since we either break out of loop or erase the iterator
            // and hence increment it.

            BaseSimOrder* const p_sim_order_ = (*t_sim_order_vec_iter_);

            int trade_size_to_be_used_ =
                _trade_print_info_.size_traded_ - saci_to_executed_size_[p_sim_order_->server_assigned_client_id()];

            if (trade_size_to_be_used_ <= 0) {
              if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
                DBGLOG_TIME_CLASS_FUNC << "Trade size exhausted for this client id: "
                                       << p_sim_order_->server_assigned_client_id() << DBGLOG_ENDL_FLUSH;
              }
              t_sim_order_vec_iter_++;
              continue;
            }

            int trd_size_ = _trade_print_info_.size_traded_;
            // if (HFSAT::SecurityDefinitions::GetContractExchSource (dep_market_view_.shortcode(), watch_.YYYYMMDD())
            // == HFSAT::kExchSourceLIFFE)
            if (!dep_market_view_.trade_before_quote())
              trd_size_ = RestoreQueueSizes(p_sim_order_, t_posttrade_bidsize_at_trade_price_, trd_size_);

            if (dbglogger_.CheckLoggingLevel(PLSMM_INFO)) {
              DBGLOG_TIME_CLASS_FUNC << "Order " << p_sim_order_->security_name() << ' '
                                     << GetTradeTypeChar(p_sim_order_->buysell()) << ' ' << p_sim_order_->price() << ' '
                                     << p_sim_order_->size_remaining() << " sE:" << p_sim_order_->size_executed() << ' '
                                     << ToString(p_sim_order_->order_status()) << " ["
                                     << p_sim_order_->queue_size_ahead() << "-" << p_sim_order_->queue_size_behind()
                                     << '-' << p_sim_order_->num_events_seen() << ']'
                                     << " CAOS: " << p_sim_order_->client_assigned_order_sequence()
                                     << " SAOS: " << p_sim_order_->server_assigned_order_sequence() << " HCT ("
                                     << _trade_print_info_.size_traded_ << ")"
                                     << " [" << dep_market_view_.bestbid_size() << " "
                                     << dep_market_view_.bestbid_price() << " * " << dep_market_view_.bestask_price()
                                     << " " << dep_market_view_.bestask_size() << "]" << DBGLOG_ENDL_FLUSH;
            }

            int t_size_executed_ = 0;
            switch (current_matching_algo_) {
              case kFIFO: {
                t_size_executed_ =
                    p_sim_order_->HandleCrossingTrade(trade_size_to_be_used_, t_posttrade_bidsize_at_trade_price_);
              } break;
              case kSimpleProRata: {
                // std::cerr << watch_.tv() << " ";
                t_size_executed_ = p_sim_order_->HandleCrossingTradeSimpleProrata(
                    trade_size_to_be_used_, t_posttrade_bidsize_at_trade_price_,
                    bid_side_priority_order_size_map_[p_sim_order_->server_assigned_client_id()],
                    bid_side_priority_order_exists_map_[p_sim_order_->server_assigned_client_id()]);
                if (bid_side_priority_order_size_map_[p_sim_order_->server_assigned_client_id()] <= 0) {
                  bid_side_priority_order_exists_map_[p_sim_order_->server_assigned_client_id()] = false;
                }
              } break;
              case kTimeProRata: {
                t_size_executed_ = p_sim_order_->HandleCrossingTradeTimeProrataLIFFE(
                    trade_size_to_be_used_, t_posttrade_bidsize_at_trade_price_,
                    bid_side_priority_order_size_map_[p_sim_order_->server_assigned_client_id()],
                    bid_side_priority_order_exists_map_[p_sim_order_->server_assigned_client_id()]);
              } break;
              case kNewTimeProRata: {
                t_size_executed_ = p_sim_order_->HandleCrossingTradeNewTimeProrataLIFFE(
                    trade_size_to_be_used_, t_posttrade_bidsize_at_trade_price_,
                    bid_side_priority_order_size_map_[p_sim_order_->server_assigned_client_id()],
                    bid_side_priority_order_exists_map_[p_sim_order_->server_assigned_client_id()]);
              } break;
              case kSplitFIFOProRata: {
                t_size_executed_ = p_sim_order_->HandleCrossingTradeSplitFIFOProRata(
                    trade_size_to_be_used_, t_posttrade_bidsize_at_trade_price_, fifo_matching_fraction_);
              } break;
              default:
                break;
            }

            if (t_size_executed_ > 0) {
              client_position_map_[p_sim_order_->server_assigned_client_id()] += t_size_executed_;
              global_position_to_send_map_[p_sim_order_->server_assigned_client_id()] += t_size_executed_;
              saci_to_executed_size_[p_sim_order_->server_assigned_client_id()] += t_size_executed_;
              LogExec(p_sim_order_, t_size_executed_);

              BroadcastExecNotification(p_sim_order_->server_assigned_client_id(), p_sim_order_);
              if (p_sim_order_->size_remaining() <= 0) {
                basesimorder_mempool_.DeAlloc(p_sim_order_);
                t_sim_order_vec_iter_ = t_this_sim_order_vec_.erase(t_sim_order_vec_iter_);
              } else {
                // // again like below if this order is not cleared others in the queue below this won't be either
                // break;
                t_sim_order_vec_iter_++;
              }
            } else {
              // // if this order was not executed, it is likely that others after this would not be executed as well.
              // // only way this breaks if size was modified of an order and hence it was put back in queue.
              // // we just need to make sure that if the vector has more than 1 order, when an order is modified up in
              // size
              // // we actually remove the order at put it back in the vector as well.
              // break;
              t_sim_order_vec_iter_++;
            }
          }
        }
      }
    }
  }
}

int PriceLevelSimMarketMaker::Connect() {
  int saci = BaseSimMarketMaker::Connect();

  // allocate space for mask vectors
  if ((int)masked_from_market_data_bids_map_.size() <= saci) {
    masked_from_market_data_bids_map_.resize(saci + 1, 0);
  }
  if ((int)masked_from_market_data_asks_map_.size() <= saci) {
    masked_from_market_data_asks_map_.resize(saci + 1, 0);
  }

  if ((int)bid_side_priority_order_exists_map_.size() <= saci) {
    bid_side_priority_order_exists_map_.resize(saci + 1, false);
  }
  if ((int)bid_side_priority_order_size_map_.size() <= saci) {
    bid_side_priority_order_size_map_.resize(saci + 1, 0);
  }
  if ((int)ask_side_priority_order_exists_map_.size() <= saci) {
    ask_side_priority_order_exists_map_.resize(saci + 1, false);
  }
  if ((int)ask_side_priority_order_size_map_.size() <= saci) {
    ask_side_priority_order_size_map_.resize(saci + 1, 0);
  }

  return saci;
}

void PriceLevelSimMarketMaker::CxlOrdersAboveBestLevel(const int max_conf_orders_above_best_level_) {
  // (i) : Get a count of orders above best level for each unique SACI.
  std::map<int, int> saci_to_above_best_ask_orders_count_ = std::map<int, int>();
  for (AskPriceSimOrderMapIter_t i2aov_iter_ = intpx_ask_order_map_.begin();
       (i2aov_iter_ != intpx_ask_order_map_.end()) &&
           (i2aov_iter_->first < bestask_int_price_ &&
            i2aov_iter_->first > bestbid_int_price_);  // non-aggressive , above-best orders
       i2aov_iter_++) {
    if (!((i2aov_iter_->second).empty())) {
      for (std::vector<BaseSimOrder*>::iterator _itr_ = (i2aov_iter_->second).begin();
           _itr_ != (i2aov_iter_->second).end(); ++_itr_) {
        BaseSimOrder* p_sim_order_ = *_itr_;

        if (p_sim_order_->IsConfirmed() && p_sim_order_->num_events_seen() >= 1) {  // a candidate for cancellation
          saci_to_above_best_ask_orders_count_[p_sim_order_->server_assigned_client_id_]++;
        }
      }
    }
  }

  std::map<int, int> saci_to_above_best_bid_orders_count_ = std::map<int, int>();
  for (BidPriceSimOrderMapIter_t i2bov_iter_ = intpx_bid_order_map_.begin();
       (i2bov_iter_ != intpx_bid_order_map_.end()) &&
           (i2bov_iter_->first > bestbid_int_price_ &&
            i2bov_iter_->first < bestask_int_price_);  // non-aggressive , above-best orders
       i2bov_iter_++) {
    if (!((i2bov_iter_->second).empty())) {
      for (std::vector<BaseSimOrder*>::iterator _itr_ = (i2bov_iter_->second).begin();
           _itr_ != (i2bov_iter_->second).end(); ++_itr_) {
        BaseSimOrder* p_sim_order_ = *_itr_;

        if (p_sim_order_->IsConfirmed() && p_sim_order_->num_events_seen() >= 1) {  // a candidate for cancellation
          saci_to_above_best_bid_orders_count_[p_sim_order_->server_assigned_client_id_]++;
        }
      }
    }
  }

  // (ii) : Leaving max_conf_orders_above_best_level_
  //          for each SACI on each side (bid & ask)
  //          cxl the remaining orders
  for (AskPriceSimOrderMapIter_t i2aov_iter_ = intpx_ask_order_map_.begin();
       (i2aov_iter_ != intpx_ask_order_map_.end()) &&
           (i2aov_iter_->first < bestask_int_price_ &&
            i2aov_iter_->first > bestbid_int_price_);  // non-aggressive , above-best orders
       i2aov_iter_++) {
    if (!((i2aov_iter_->second).empty())) {
      for (std::vector<BaseSimOrder*>::iterator _itr_ = (i2aov_iter_->second).begin();
           _itr_ != (i2aov_iter_->second).end();) {
        BaseSimOrder* p_sim_order_ = *_itr_;

        if (p_sim_order_->IsConfirmed() && p_sim_order_->num_events_seen() >= 1 &&
            saci_to_above_best_ask_orders_count_[p_sim_order_->server_assigned_client_id_] >
                max_conf_orders_above_best_level_) {
          saci_to_above_best_ask_orders_count_[p_sim_order_->server_assigned_client_id_]--;

          VectorUtils::UniqueVectorRemove(intpx_ask_order_map_[p_sim_order_->int_price()],
                                          p_sim_order_);  ///< remove from map
          BroadcastCancelNotification(p_sim_order_->server_assigned_client_id_,
                                      p_sim_order_);  ///< send cancel notification

          basesimorder_mempool_.DeAlloc(p_sim_order_);
          _itr_ = (i2aov_iter_->second).begin();
        } else {
          ++_itr_;
        }
      }
    }
  }

  for (BidPriceSimOrderMapIter_t i2bov_iter_ = intpx_bid_order_map_.begin();
       (i2bov_iter_ != intpx_bid_order_map_.end()) &&
           (i2bov_iter_->first > bestbid_int_price_ &&
            i2bov_iter_->first < bestask_int_price_);  // non-aggressive , above-best orders
       i2bov_iter_++) {
    if (!((i2bov_iter_->second).empty())) {
      for (std::vector<BaseSimOrder*>::iterator _itr_ = (i2bov_iter_->second).begin();
           _itr_ != (i2bov_iter_->second).end();) {
        BaseSimOrder* p_sim_order_ = *_itr_;

        if (p_sim_order_->IsConfirmed() && p_sim_order_->num_events_seen() >= 1 &&
            saci_to_above_best_bid_orders_count_[p_sim_order_->server_assigned_client_id_] >
                max_conf_orders_above_best_level_) {
          saci_to_above_best_bid_orders_count_[p_sim_order_->server_assigned_client_id_]--;

          VectorUtils::UniqueVectorRemove(intpx_bid_order_map_[p_sim_order_->int_price()],
                                          p_sim_order_);  ///< remove from map
          BroadcastCancelNotification(p_sim_order_->server_assigned_client_id_,
                                      p_sim_order_);  ///< send cancel notification

          basesimorder_mempool_.DeAlloc(p_sim_order_);

          _itr_ = (i2bov_iter_->second).begin();
        } else {
          ++_itr_;
        }
      }
    }
  }
}

void PriceLevelSimMarketMaker::UpdateQueueSizes(int new_size_, int prev_size_, BaseSimOrder* p_sim_order_) {
  /*
   * UPdate queue sizes based on trade ratios, i.es high cancellation from fromnt in high vol time and proportionate
   * cancellations in
   * otherwise
   * */
  if (new_size_ < prev_size_ &&
      !config_.use_no_cxl_from_front_) {  // currently only in this case are we doing anythng about place in line
    int del_size_ = 0;
    del_size_ = prev_size_ - new_size_;
    if (del_size_ > 0) {
      double trade_ratio_ = 0.0;
      if (p_sim_order_->buysell_ == kTradeTypeBuy) {
        trade_ratio_ = current_bid_trade_ratio_vec_[p_sim_order_->int_price_];
        if (trade_ratio_ >= bid_tr_ratio_) {
          // For NSE, we need to round off, as cancellations mostly happen in 1 lot, and the decrement is floored to 0
          if (dep_market_view_.min_order_size() > 5) {
            p_sim_order_->queue_size_ahead_ -= MathUtils::RoundOff(
                (round(del_size_ * ((trade_ratio_ - bid_tr_ratio_) * p_sim_order_->queue_size_ahead_ / prev_size_ +
                                    ((1 - trade_ratio_ + bid_tr_ratio_) * p_sim_order_->queue_size_ahead_ *
                                     p_sim_order_->queue_size_ahead_) /
                                        double(prev_size_ * prev_size_)))),
                dep_market_view_.min_order_size());
          } else {
            p_sim_order_->queue_size_ahead_ -= MathUtils::GetFlooredMultipleOf(
                (round(del_size_ * ((trade_ratio_ - bid_tr_ratio_) * p_sim_order_->queue_size_ahead_ / prev_size_ +
                                    ((1 - trade_ratio_ + bid_tr_ratio_) * p_sim_order_->queue_size_ahead_ *
                                     p_sim_order_->queue_size_ahead_) /
                                        double(prev_size_ * prev_size_)))),
                dep_market_view_.min_order_size());
          }
        } else {
          // For NSE, we need to round off, as cancellations mostly happen in 1 lot, and the decrement is floored to 0
          if (dep_market_view_.min_order_size() > 5) {
            double fill_fraction = (double)p_sim_order_->queue_size_ahead_ / prev_size_;
            p_sim_order_->queue_size_ahead_ -=
                MathUtils::RoundOff(round(fill_fraction * del_size_), dep_market_view_.min_order_size());
          } else {
            p_sim_order_->queue_size_ahead_ -= MathUtils::GetFlooredMultipleOf(
                round((((p_sim_order_->queue_size_ahead_ * p_sim_order_->queue_size_ahead_) * del_size_)) /
                      double((prev_size_ * prev_size_))),
                dep_market_view_.min_order_size());
          }
        }
      } else  // sell order
      {
        trade_ratio_ = current_ask_trade_ratio_vec_[p_sim_order_->int_price_];
        if (trade_ratio_ >= ask_tr_ratio_) {
          // For NSE, we need to round off, as cancellations mostly happen in 1 lot, and the decrement is floored to 0
          if (dep_market_view_.min_order_size() > 5) {
            p_sim_order_->queue_size_ahead_ -= MathUtils::RoundOff(
                (round(del_size_ *
                       ((trade_ratio_ - ask_tr_ratio_) * p_sim_order_->queue_size_ahead_ / double(prev_size_) +
                        ((1 - trade_ratio_ + ask_tr_ratio_) * p_sim_order_->queue_size_ahead_ *
                         p_sim_order_->queue_size_ahead_) /
                            double(prev_size_ * prev_size_)))),
                dep_market_view_.min_order_size());
          } else {
            p_sim_order_->queue_size_ahead_ -= MathUtils::GetFlooredMultipleOf(
                (round(del_size_ *
                       ((trade_ratio_ - ask_tr_ratio_) * p_sim_order_->queue_size_ahead_ / double(prev_size_) +
                        ((1 - trade_ratio_ + ask_tr_ratio_) * p_sim_order_->queue_size_ahead_ *
                         p_sim_order_->queue_size_ahead_) /
                            double(prev_size_ * prev_size_)))),
                dep_market_view_.min_order_size());
          }
        } else {
          // For NSE, we need to round off, as cancellations mostly happen in 1 lot, and the decrement is floored to 0
          if (dep_market_view_.min_order_size() > 5) {
            double fill_fraction = (double)p_sim_order_->queue_size_ahead_ / prev_size_;
            p_sim_order_->queue_size_ahead_ -=
                MathUtils::RoundOff(round(fill_fraction * del_size_), dep_market_view_.min_order_size());
          } else {
            p_sim_order_->queue_size_ahead_ -= MathUtils::GetFlooredMultipleOf(
                round(((p_sim_order_->queue_size_ahead_ * p_sim_order_->queue_size_ahead_) * del_size_) /
                      double(prev_size_ * prev_size_)),
                dep_market_view_.min_order_size());
          }
        }
      }
    }
  }

  if (p_sim_order_->queue_size_ahead_ > new_size_) {
    p_sim_order_->queue_size_ahead_ = new_size_;
  }

  p_sim_order_->queue_size_behind_ = new_size_ - p_sim_order_->queue_size_ahead_;
  p_sim_order_->num_events_seen_++;
}

void PriceLevelSimMarketMaker::UpdateQueueSizesTradeBased(int new_size_, int prev_size_, BaseSimOrder* p_sim_order_) {
  /* Remaned FGBM simmarket maker
   * we cancell only if there is a trade in market some time before
   * */
  if (new_size_ < prev_size_) {  // currently only in this case are we doing anythng about place in line
    int del_size_ = prev_size_ - new_size_;
    ttime_t time_diff_ = ttime_t(3, 0);
    if (p_sim_order_->buysell_ == kTradeTypeSell) {
      if (current_ask_trade_ratio_vec_.find(p_sim_order_->int_price_) != current_ask_trade_ratio_vec_.end()) {
        time_diff_ = watch_.tv() - current_ask_trade_time_[p_sim_order_->int_price_];
      }
    } else {
      if (current_bid_trade_ratio_vec_.find(p_sim_order_->int_price_) != current_bid_trade_ratio_vec_.end()) {
        time_diff_ = watch_.tv() - current_bid_trade_time_[p_sim_order_->int_price_];
      }
    }

    if (time_diff_ < ttime_t(2, 0)) {
      p_sim_order_->queue_size_ahead_ -= MathUtils::GetFlooredMultipleOf(
          round(((p_sim_order_->queue_size_ahead_ * p_sim_order_->queue_size_ahead_) * del_size_) /
                double(prev_size_ * prev_size_)),
          dep_market_view_.min_order_size());
      if (p_sim_order_->queue_size_ahead_ > new_size_) {
        p_sim_order_->queue_size_ahead_ = new_size_;
      }
      p_sim_order_->queue_size_behind_ = new_size_ - p_sim_order_->queue_size_ahead_;
    } else {
      if (p_sim_order_->queue_size_behind_ >= del_size_)
        p_sim_order_->queue_size_behind_ -= del_size_;
      else {
        p_sim_order_->queue_size_ahead_ -= del_size_ - p_sim_order_->queue_size_behind_;
        p_sim_order_->queue_size_behind_ = 0;
      }
    }
  } else
    p_sim_order_->queue_size_behind_ += new_size_ - prev_size_;
  // the following part is same as original Enqueue ... just ensuring that sA = sB = Tsz and sA <= Tsz
  p_sim_order_->num_events_seen_++;
}

void PriceLevelSimMarketMaker::UpdateQueueSizesTargetPrice(int new_size_, int prev_size_, BaseSimOrder* p_sim_order_) {
  /* Target price based sim marketmaker,
   * cancels based on price prediction
   * */
  if (new_size_ < prev_size_) {  // currently only in this case are we doing anythng about place in line
    int del_size_ = 0;
    del_size_ = prev_size_ - new_size_;
    if (del_size_ > 0) {
      double midprice_ = dep_market_view_.market_update_info_.mid_price_;
      double simtgtpx = 0;
      if (p_sim_order_->buysell_ == kTradeTypeBuy) {
        simtgtpx =
            w1 *
                func(time_decayed_trade_info_manager_.onelvlsumsz_ / time_decayed_trade_info_manager_.onelvlmaxsumsz_) *
                (time_decayed_trade_info_manager_.onelvlsumpxsz_ / time_decayed_trade_info_manager_.onelvlsumsz_ -
                 midprice_) +
            w2 * prev_dep_price_change_;
      } else {
        simtgtpx =
            w1 *
                func(time_decayed_trade_info_manager_.onelvlsumsz_ / time_decayed_trade_info_manager_.onelvlmaxsumsz_) *
                (midprice_ -
                 time_decayed_trade_info_manager_.onelvlsumpxsz_ / time_decayed_trade_info_manager_.onelvlsumsz_) -
            w2 * prev_dep_price_change_;
      }
      simtgtpx /=
          dep_market_view_.market_update_info_.bestask_price_ - dep_market_view_.market_update_info_.bestbid_price_;

      if (simtgtpx >= 0) {
        p_sim_order_->queue_size_ahead_ -= MathUtils::GetFlooredMultipleOf(
            round((((p_sim_order_->queue_size_ahead_ * p_sim_order_->queue_size_ahead_) * del_size_)) /
                  double((prev_size_ * prev_size_)) * (1 + simtgtpx)),
            dep_market_view_.min_order_size());
        if (p_sim_order_->queue_size_ahead_ > new_size_) {
          p_sim_order_->queue_size_ahead_ = new_size_;
        }

        p_sim_order_->queue_size_behind_ = new_size_ - p_sim_order_->queue_size_ahead_;
        p_sim_order_->num_events_seen_++;
      } else {
        if (p_sim_order_->queue_size_behind_ >= del_size_) {
          p_sim_order_->queue_size_behind_ -= del_size_;
        } else {
          p_sim_order_->queue_size_ahead_ -= del_size_ - p_sim_order_->queue_size_behind_;
          p_sim_order_->queue_size_behind_ = 0;
        }
      }
    }
  } else {
    p_sim_order_->queue_size_behind_ += new_size_ - prev_size_;
  }
}

void PriceLevelSimMarketMaker::UpdateQueueSizesBasePriceBasedTargetPrice(int new_size_, int prev_size_,
                                                                         BaseSimOrder* p_sim_order_) {
  /* Target price based sim marketmaker,
   * cancels based on price prediction
   * */
  if (new_size_ < prev_size_) {  // currently only in this case are we doing anythng about place in line
    int del_size_ = 0;
    del_size_ = prev_size_ - new_size_;

    double midprice_ = dep_market_view_.market_update_info_.mid_price_;

    double base_price_ = dep_market_view_.market_update_info_.mkt_size_weighted_price_;
    double simtgtpx =
        base_price_ +
        (w1 * func(time_decayed_trade_info_manager_.onelvlsumsz_ / time_decayed_trade_info_manager_.onelvlmaxsumsz_) *
         ((time_decayed_trade_info_manager_.onelvlsumpxsz_ / time_decayed_trade_info_manager_.onelvlsumsz_) -
          midprice_)) +
        (w2 * prev_dep_price_change_);

    double orderpxlikelihood = (midprice_ - simtgtpx) / (dep_market_view_.market_update_info_.bestask_price_ -
                                                         dep_market_view_.market_update_info_.bestbid_price_);
    if (p_sim_order_->buysell_ == kTradeTypeSell) {
      orderpxlikelihood = -orderpxlikelihood;
    }

    if (orderpxlikelihood <= low_likelihood_thresh_) {  // front_cancel_tendency_ = 0
    } else {
      double front_cancel_tendency_ = 1.0;
      if (orderpxlikelihood <= 0.5) {  // linearly from 0 to 1
        front_cancel_tendency_ = (orderpxlikelihood - low_likelihood_thresh_) / (0.5 - low_likelihood_thresh_);
      }

      p_sim_order_->queue_size_ahead_ -= MathUtils::GetFlooredMultipleOf(
          round((p_sim_order_->queue_size_ahead_ * del_size_ * front_cancel_tendency_) /
                double(p_sim_order_->queue_size_ahead_ * front_cancel_tendency_ + p_sim_order_->queue_size_behind_)),
          dep_market_view_.min_order_size());
    }
    if (p_sim_order_->queue_size_ahead_ > new_size_) {
      p_sim_order_->queue_size_ahead_ = new_size_;
    }

    p_sim_order_->queue_size_behind_ = new_size_ - p_sim_order_->queue_size_ahead_;
    p_sim_order_->num_events_seen_++;
  } else {
    p_sim_order_->queue_size_behind_ += new_size_ - prev_size_;
  }
}

// For matching trade with market quote (only for CFE)
TradeType_t PriceLevelSimMarketMaker::MatchTradeAndQuote(int old_bestask_int_price_, int old_bestask_size_,
                                                         int old_bestbid_int_price_, int old_bestbid_size_,
                                                         int bestask_int_price_, int bestask_size_,
                                                         int bestbid_int_price_, int bestbid_size_,
                                                         CSM_MDS::CSMCommonStruct event_) {
  int trade_int_price_ = dep_market_view_.GetIntPx(event_.data_.csm_trds_.trd_px_);
  if ((old_bestbid_int_price_ == bestbid_int_price_) && (trade_int_price_ == bestbid_int_price_) &&
      (old_bestbid_size_ - bestbid_size_ ==
       (int)event_.data_.csm_trds_.trd_qty_)) {  // Aggressive sell (found a perfect match)
    return HFSAT::kTradeTypeSell;
  } else if ((old_bestbid_int_price_ > bestbid_int_price_) && (trade_int_price_ == old_bestbid_int_price_) &&
             (old_bestbid_size_ ==
              (int)event_.data_.csm_trds_.trd_qty_)) {  // Aggressive sell with best bid price change
    return HFSAT::kTradeTypeSell;
  } else if ((old_bestask_int_price_ == bestask_int_price_) && (trade_int_price_ == bestask_int_price_) &&
             (old_bestask_size_ - bestask_size_ == (int)event_.data_.csm_trds_.trd_qty_)) {  // Aggressive buy
    return HFSAT::kTradeTypeBuy;
  } else if ((old_bestask_int_price_ < bestask_int_price_) && (trade_int_price_ == old_bestask_int_price_) &&
             (old_bestask_size_ ==
              (int)event_.data_.csm_trds_.trd_qty_)) {  // Aggressive buy with best ask price change
    return HFSAT::kTradeTypeBuy;
  } else if ((old_bestbid_int_price_ == bestbid_int_price_) && (trade_int_price_ == bestbid_int_price_) &&
             (old_bestbid_size_ - bestbid_size_ >
              (int)event_.data_.csm_trds_.trd_qty_)) {  // Aggressive sell (possibly)
    return HFSAT::kTradeTypeNoInfo;
  } else if ((old_bestask_int_price_ == bestask_int_price_) && (trade_int_price_ == bestask_int_price_) &&
             (old_bestask_size_ - bestask_size_ >
              (int)event_.data_.csm_trds_.trd_qty_)) {  // Aggressive sell (possibly)
    return HFSAT::kTradeTypeNoInfo;
  }
  return HFSAT::kTradeTypeNoInfo;
}
}
